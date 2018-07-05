cmake_minimum_required(VERSION 3.10)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON) 

# decide on subdirectory name within native_libs
if(WIN32)
    set(PLATFORM_DIR_SUFFIX windows)
elseif(APPLE)
    set(PLATFORM_DIR_SUFFIX macos)
else()
    set(PLATFORM_DIR_SUFFIX linux)
endif()

# set output directory
set(OUT_DIR ${CMAKE_CURRENT_LIST_DIR}/${PLATFORM_DIR_SUFFIX})
if(WIN32)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${OUT_DIR})
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${OUT_DIR})
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO ${OUT_DIR})
else()
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${OUT_DIR})
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG ${OUT_DIR})
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELWITHDEBINFO ${OUT_DIR})
endif()

project(MongoHelper)

add_library(${PROJECT_NAME} SHARED main.cpp)

find_path(BSON_INCLUDE bson.h)
if(NOT BSON_INCLUDE)
    message(WARNING "Cannot find bson include dir")
else()
    target_include_directories(${PROJECT_NAME} PRIVATE ${BSON_INCLUDE})
endif()

find_path(MONGOC_INCLUDE mongoc.h)
if(NOT MONGOC_INCLUDE)
    message(WARNING "Cannot find mongoc include dir")
else()
    target_include_directories(${PROJECT_NAME} PRIVATE ${MONGOC_INCLUDE})
endif()

find_library(BSON_LIBRARY bson-1.0)
if(NOT BSON_LIBRARY)
    message(WARNING "Cannot find bson-1.0 library")
else()
    target_link_libraries(${PROJECT_NAME} PRIVATE ${BSON_LIBRARY})
endif()

find_library(MONGOC_LIBRARY mongoc-1.0)
if(NOT MONGOC_LIBRARY)
    message(WARNING "Cannot find mongoc-1.0 library")
else()
    target_link_libraries(${PROJECT_NAME} PRIVATE ${MONGOC_LIBRARY})
endif()
