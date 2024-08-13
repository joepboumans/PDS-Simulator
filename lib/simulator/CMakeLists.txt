cmake_minimum_required(VERSION 3.12)
include_directories(${CMAKE_SOURCE_DIR})
set(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR}/lib)
add_library(lib-simulator STATIC simulator.cpp)
