# MongoDB bindings for Luna


## Purpose
This project is a library with bindings allowing to use MongoDB from [Luna programming language](https://www.luna-lang.org/). 

## Project structure
It contains two subprojects:
* `MongoDB` Luna library;
* `MongoHelper` C++ library.

## Third-party dependencies
Third party dependencies include:
* `libbson` — C library with utilities for handling BSON documents;
* `libmongoc` — MongoDB C driver.

Please refer to [MongoDB C Driver](http://mongoc.org/libmongoc/current/installing.html) documentation pages for installation instructions for all platforms.

Obviously, to use the bindings, a running [MongoDB](https://www.mongodb.com/) server will be needed.

## Build & Install
1. Procure [`luna` compiler](https://github.com/luna/luna).
2. Make sure that third-party dependencies are [installed]((http://mongoc.org/libmongoc/current/installing.html)).
3. Build `MongoHelper`, place binary under `PATH` or in the repo under `native_libs/$PLATFORM` subdirectory, as described in the [Luna Book](https://luna-lang.gitbooks.io/docs/content/calling-c-functions.html).
4. Call `luna path/to/repo`.

## Tutorial

Please see the usage in [Main.luna](src/Main.luna).

## Known issues
* Bindings are still rudimentary and experimental.
* Some APIs are not consistent or not well-thought enough.
* Luna's `ManagedPointer a` is [not reliable yet](https://github.com/luna/luna/issues/212), so until the issue if fixed, resource leaks are unevitable.
* Tutorial is just a set of random pieces of code I used when developing the library.
* Missing proper build instructions / build system for `MongoHelper`.
* Checked only on Windows.
* Devise proper naming scheme for MongoDB C Driver binaries and/or keep them on the repo.
* Build & Install steps are actually for running my "tests", not use the bindings itself.

## API Reference
All public interface of the library is described below. Each class is in a module with its name. For example, to use `Client` class you'll need `import MongoDB.Client`. All other modules and APIs should be treated as internal to the library and not used by external code.

### class `MongoDB`

This class contains a few methods that are "global" for the Mongo bindings. Its objects can be freely constructed using the `MongoDB` constructor.

#### Methods:
* `init :: None` — should be called before making any other call into this library. Initializes a global state of the underlying C driver.
* `cleanup :: None` — should be called when there will be no more calls into this library. Releases all the resources allocated by the driver. **NOTE: it is not allowed to call `init` once again after `cleanup`!**
* `newClient uri :: Text -> Client` — creates a new `Client` object representing a MongoDB connection. `uri` is a `Text` parameter, for example `"mongodb://192.168.11.20:27017"`. Please refer to the [MongoDB documentation](https://docs.mongodb.com/manual/reference/connection-string/?_ga=2.226838301.1022409252.1529405873-838949899.1529405873) for more information about supported URI syntax.

### class `Client`
This class represents a MongoDB connection. It is typically obtained by a `MongoDB.newClient "uri"` call. It is reccemended to set the application name (`setAppname`) right after creating the client.

#### Methods:
* `setAppname name :: Text -> None` — takes a name that will be sent to the server as part of an initial handshake. Should be called before initializaing connection.
* `collection databaseName collectionName :: Text -> Text -> Collection` — creates a `Collection` object providing access to the collection. 
* `database databaseName :: Text -> Database` — creates an object accessing database with a given name.
* `databaseNames :: [Text]` — queries the server for the list known database names.
* `defaultDatabase :: Maybe Database` — creates an object accessing the default database. The database in such case is inferred from the URI — e.g. when client's URI was `mongodb://host/db_name` then default database name is `db_name`. If URI did not have database name specified (like in `mongodb://host/`) then `Nothing` is returned.
* `simpleCommand databaseName commandJson :: Text -> JSON -> JSON` — runs the command on database under given name, returning the first document from resulting cursor. Please refer to the [MongoDB documentation](https://docs.mongodb.com/manual/reference/command/?_ga=2.258977546.1022409252.1529405873-838949899.1529405873) for more information on database commands.

### class `Database`
The object of this class allows performing actions on a specific MongoDB database. Note that it is just a handle to the database, not the collection of documents itself.
* `collectionNames :: [Text]` — fetches names of all the collections contained by the database.
* `collection name :: Collection` — creates a `Collection` control object for collection with a given name. Note: this does not actually create the collection if not present (unless collection is written to later). To immediately create collection please use `createCollection` method. 
* `createCollection name :: Collection` — creates a `Collection` in the database. Throws an exception if such collection already exists. Please use `collection` if you just want to work on collection of given name.
* `drop :: None` — drops the database from the server.
* `hasCollection name :: Text -> Bool` — checks if collection with a given name exists in the database
* `name :: Text` — fetches the database name.

### class `Collection`
The object provides access for MongoDB collection, supporting CRUD operations.

#### Methods:
* `count query :: JSON -> Int` — executes a count query on the collection and returns the number of matching documents.
* `drop :: None` — drops the collection, including all associated indexes.
* `name :: Text` — fetches the name of the collection.
* `insertOne document :: JSON -> None` — inserts given document into the dollcetion.
* `updateOne selector updates :: JSON -> JSON -> JSON` — executes a query looking for a document matching the `selector` — if found, performs update described by `updates`. Please refer to [MongoDB documentation](https://docs.mongodb.com/master/reference/command/update/?_ga=2.267346574.1022409252.1529405873-838949899.1529405873) for more information on update descriptor syntax.
* `find query :: JSON -> Cursor` — queries the collection and returns cursor allowing iteration over matching documents. See the `Cursor` class documentation.
* `findAll query :: JSON -> [JSON]` — retrieves all documents in the collection matching to `query`.
* `findOne query :: JSON -> Maybe JSON` — returns any document from the collection that matches `query` or `Nothing` if there is none.
* `deleteOne query :: JSON -> Int` — looks for a document matching the `query` and deletes it if found. Returns the deleted documents count (0 or 1).
* `deleteMany query :: JSON -> Int` — deletes all documents matching the query. Returns deleted documents count.
* `rename newDatabaseName newCollectionName dropTargetPolicy :: Text -> Text -> RenamePolicy -> None` — renames the collection. It will remain safe to use after rename (object will point to the renamed collection). Drop dropTargetPolicy can be either `DropTargetBeforeRename` or `DontDropTargetBeforeRename`. The latter is selected, an exception will be raised if collection with the same name as target already exists.

### class `Cursor`
Class for iterating over results of MongoDB query. 

#### Methods:
* `current :: Maybe JSON` — obtains the current document under cursor. Returns `Nothing` after all documents were iterated or before the `next` method was called for the first time. 
* `next :: Maybe JSON` — iterates the cursor setting it to the next document and returning it. `Nothing` means that there are no more documents.
* `toList :: [Json]` — iterates over all the remaining documents and returns them as a list.