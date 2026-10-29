cmake_minimum_required(VERSION 3.25)
project(nlpm C)

include(CTest)
include(cmake/CompilerWarnings.cmake)
include(cmake/Dependencies.cmake)

add_subdirectory(src)

if(BUILD_TESTING)
  add_subdirectory(tests)
endif()
