cmake_minimum_required(VERSION 3.12)
set(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR}/lib)

add_library(lib-catch2 STATIC catch_amalgamated.cpp)
target_include_directories(lib-catch2 PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(lib-catch2 PRIVATE ${CMAKE_HOME_DIRECTORY}/src/)
