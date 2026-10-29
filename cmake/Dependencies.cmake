find_library(SQLITE3_LIB NAMES sqlite3.so.0 libsqlite3.so.0 sqlite3 PATHS /usr/lib/x86_64-linux-gnu /usr/lib REQUIRED)
find_library(CURL_LIB NAMES curl.so.4 libcurl.so.4 curl PATHS /usr/lib/x86_64-linux-gnu /usr/lib REQUIRED)
find_library(ARCHIVE_LIB NAMES archive.so.13 libarchive.so.13 archive PATHS /usr/lib/x86_64-linux-gnu /usr/lib REQUIRED)

add_library(system_deps INTERFACE)
target_include_directories(system_deps INTERFACE
  ${CMAKE_CURRENT_SOURCE_DIR}/third_party/include
)
target_link_libraries(system_deps INTERFACE
  ${SQLITE3_LIB}
  ${CURL_LIB}
  ${ARCHIVE_LIB}
)

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/third_party/toml-c/toml.c")
  add_library(toml-c STATIC third_party/toml-c/toml.c)
  target_include_directories(toml-c PUBLIC third_party/toml-c)
endif()

if(BUILD_TESTING)
  add_library(unity STATIC
    third_party/unity/unity.c
    third_party/unity/unity.h
    third_party/unity/unity_internals.h
  )
  target_include_directories(unity PUBLIC third_party/unity)
endif()
