cmake_minimum_required(VERSION 3.12)
set(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR}/lib)
add_library(lib-fcm-sketch STATIC fcm-sketch.cpp)
