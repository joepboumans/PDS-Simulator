cmake_minimum_required(VERSION 3.12)
set(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR}/lib)

add_library(lib-waterfall STATIC waterfall.cpp)
target_include_directories(lib-waterfall PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(lib-waterfall PRIVATE ${CMAKE_HOME_DIRECTORY}/src/)
