cmake_minimum_required(VERSION 3.12)
set(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR}/lib)

find_package(xxHash CONFIG REQUIRED)
add_library(lib-qwaterfall OBJECT qwaterfall.cpp)
target_include_directories(lib-qwaterfall PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(lib-qwaterfall
                           PRIVATE ${CMAKE_HOME_DIRECTORY}/src/)
