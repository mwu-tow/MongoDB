#include <iostream>
#include <memory>
#include <string>
#include <bson.h>
#include <mongoc.h>

#ifdef _MSC_VER
	#pragma comment(lib, "bson-1.0.lib")
	#pragma comment(lib, "mongoc-1.0.lib")
#endif

#define EXPORT __declspec(dllexport)

using namespace std::literals;

static std::string lastError;

struct unique_bson_ptr
{
	std::unique_ptr<bson_t, void(*)(bson_t*)> ptr;
	unique_bson_ptr(bson_t *ptr) : ptr(ptr, bson_destroy) {}
	explicit operator bool() const noexcept { return ptr != nullptr; }
	auto get() const noexcept { return ptr.get(); }
};

template<typename Ptr, typename ...Args>
auto callHandlingError(const std::string &errorMsg, Ptr ptr, Args... args)
{
	bson_error_t error;
	const auto ret = (*ptr)(args..., &error);

	// we assume that returned type is either pointer or boolean.
	using RetType = std::invoke_result_t<Ptr, Args..., bson_error_t*>;
	static_assert(std::is_same_v<bool, RetType> || std::is_pointer_v<RetType>);

	// for pointers, nullptr means error
	// for boolean, false means error
	// in both cases bool conversion does desired job
	if(ret)
		lastError.clear();
	else
		lastError = "Failed to " + errorMsg + ": " + error.message;

	return ret;
}

extern "C" 
{
	// Returned document should be freed with bson_destroy
	EXPORT bson_t *jsonToBson(const char *jsonText)
	{
		const auto jsonData = reinterpret_cast<const uint8_t*>(jsonText);
		const auto ret = callHandlingError("parse as JSON `"s + jsonText + "`", &bson_new_from_json, jsonData, -1);
		return ret;
	}

	// Returned c-string should be freed with bson_free
	EXPORT char * bsonToJson(const bson_t *bson)
	{
		size_t len;
		return bson_as_json(bson, &len);
	}

	// Returned c-string should be freed with bson_free
	EXPORT const char* mongoh_get_error()
	{
		return lastError.size() ? lastError.c_str() : nullptr;
	}

	EXPORT const char* mongoh_insert_one(mongoc_collection_t *collection, const char *documentJsonText)
	{
		const unique_bson_ptr document = jsonToBson(documentJsonText);
		if(!document)
			return nullptr;

		bson_t reply;
		if(callHandlingError("insert one", mongoc_collection_insert_one, collection, document.get(), nullptr, &reply))
			return bsonToJson(&reply);

		return nullptr;
	}

	// use bson_strfreev on result
	EXPORT char **mongoh_get_collection_names(mongoc_database_t *database)
	{
		auto result = callHandlingError("get collection names", mongoc_database_get_collection_names, database);
		if(!result)
			return nullptr;

		return result;
		//bson_strfreev(result);
	}

	// returns text json to be freed with bson_free
	EXPORT char *mongoh_find_all(mongoc_collection_t *collection, const char *queryJsonText)
	{
		const unique_bson_ptr query = jsonToBson(queryJsonText);
		if(!query)
			return nullptr;

		auto documentsToRet = bson_new(); // list of all matching documents
		auto cursor = mongoc_collection_find_with_opts(collection, query.get(), nullptr, nullptr);

		int i = 0;
		const bson_t *doc;
		while (mongoc_cursor_next(cursor, &doc)) 
		{
			bson_append_document(documentsToRet, std::to_string(i).c_str(), -1, doc);
		}
		mongoc_cursor_destroy (cursor);

		auto ret = bson_array_as_json(documentsToRet, nullptr);
		bson_destroy(documentsToRet);
		return ret;
	}

	EXPORT const char* getLastError()
	{
		return lastError.size() ? lastError.c_str() : nullptr;
	}

	// Returned c-string should be freed with bson_free
	EXPORT char* simpleCommand(mongoc_client_t *client, const char*dbname, const char*jsonCommandText)
	{
		unique_bson_ptr commandBson = jsonToBson(jsonCommandText);
		if(!commandBson)
			return nullptr;

		bson_t reply;
		const auto description = "call simple command `"s + jsonCommandText + "` on db `" + dbname + "`";
		if(callHandlingError(description, mongoc_client_command_simple, client, dbname, commandBson.get(), nullptr, &reply))
			return bsonToJson(&reply);

		return nullptr;
	}

	EXPORT void foo()
	{
		std::cout << "Hello from C++" << std::endl;
	}
}
