cmake_minimum_required(VERSION 3.25)
project(nlpm VERSION 0.0.1 LANGUAGES C)

include(CTest)
include(cmake/CompilerWarnings.cmake)
include(cmake/Dependencies.cmake)

add_subdirectory(src)

if(BUILD_TESTING)
  add_subdirectory(tests)
endif()
