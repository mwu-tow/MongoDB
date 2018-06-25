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

static thread_local std::string lastError;

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
	error.code = 0;

	const auto ret = (*ptr)(args..., &error);
	if(error.code == 0)
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

	// takes a pointer and does nothing
	EXPORT void mongoh_do_nothing([[maybe_unused]] void *pointerArgument)
	{}

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

	EXPORT const char* mongoh_update_one(mongoc_collection_t *collection, const char *queryJsonText, const char *updateJsonText)
	{
		const unique_bson_ptr selectorQuery = jsonToBson(queryJsonText);
		if(!selectorQuery)
			return nullptr;
		const unique_bson_ptr update = jsonToBson(updateJsonText);
		if(!update)
			return nullptr;

		bson_t reply;
		if(callHandlingError("update one", &mongoc_collection_update_one, collection, selectorQuery.get(), update.get(), nullptr, &reply))
			return bsonToJson(&reply);

		return nullptr;
	}

	EXPORT bool mongoh_has_collection(mongoc_database_t *database, const char *name)
	{
		return callHandlingError("has collection", mongoc_database_has_collection, database, name);
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

	EXPORT mongoc_collection_t *mongoh_database_create_collection(mongoc_database_t *database, const char *name)
	{
		return callHandlingError(__FUNCTION__, mongoc_database_create_collection, database, name, nullptr);
	}

	// use bson_strfreev on result
	EXPORT bool mongoh_database_drop(mongoc_database_t *database)
	{
		return callHandlingError(__FUNCTION__, mongoc_database_drop, database);
	}


	// returns text json to be freed with bson_free
	EXPORT int64_t mongoh_count(mongoc_collection_t *collection, const char *queryJsonText)
	{
		const unique_bson_ptr selectorQuery = jsonToBson(queryJsonText);
		if(!selectorQuery)
			return -1;

		return callHandlingError("collection count", &mongoc_collection_count, collection, MONGOC_QUERY_NONE, selectorQuery.get(), 0, 0, nullptr);
	}

	// use bson_strfreev on result
	EXPORT bool mongoh_collection_drop(mongoc_collection_t *collection)
	{
		return callHandlingError(__FUNCTION__, mongoc_collection_drop, collection);
	}


	// returns text json to be freed with bson_free
	EXPORT char *mongoh_delete_one(mongoc_collection_t *collection, const char *queryJsonText)
	{
		const unique_bson_ptr selectorQuery = jsonToBson(queryJsonText);
		if(!selectorQuery)
			return nullptr;

		bson_t reply;
		if(callHandlingError("delete one", &mongoc_collection_delete_one, collection, selectorQuery.get(), nullptr, &reply))
			return bsonToJson(&reply);

		return nullptr;

	}

	// returns text json to be freed with bson_free
	EXPORT char *mongoh_delete_many(mongoc_collection_t *collection, const char *queryJsonText)
	{
		const unique_bson_ptr selectorQuery = jsonToBson(queryJsonText);
		if(!selectorQuery)
			return nullptr;

		bson_t reply;
		if(callHandlingError("delete many", &mongoc_collection_delete_many, collection, selectorQuery.get(), nullptr, &reply))
			return bsonToJson(&reply);

		return nullptr;
	}

	// returns text json to be freed with bson_free
	EXPORT char *mongoh_find_one(mongoc_collection_t *collection, const char *queryJsonText)
	{
		const unique_bson_ptr query = jsonToBson(queryJsonText);
		if(!query)
			return nullptr;

		auto documentsToRet = bson_new(); // list of all matching documents
		auto cursor = mongoc_collection_find_with_opts(collection, query.get(), nullptr, nullptr);

		int i = 0;
		const bson_t *doc;
		if(mongoc_cursor_next(cursor, &doc)) 
		{
			bson_append_document(documentsToRet, std::to_string(i).c_str(), -1, doc);
		}
		mongoc_cursor_destroy (cursor);

		auto ret = bson_array_as_json(documentsToRet, nullptr);
		bson_destroy(documentsToRet);
		return ret;
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


	// use bson_strfreev on result
	EXPORT char **mongoh_get_database_names(mongoc_client_t *client)
	{
		return callHandlingError("get database names", mongoc_client_get_database_names_with_opts, client, nullptr);
	}

	EXPORT void foo()
	{
		std::cout << "Hello from C++" << std::endl;
	}
}
