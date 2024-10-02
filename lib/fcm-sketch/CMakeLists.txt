cmake_minimum_required(VERSION 3.12)
set(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR}/lib)
add_library(lib-fcm-sketch STATIC fcm-sketch.cpp fcm-sketches.cpp)
target_include_directories(lib-fcm-sketch PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

find_package(xxHash CONFIG REQUIRED)
target_link_libraries(lib-fcm-sketch PRIVATE xxHash::xxhash)
target_include_directories(lib-fcm-sketch PRIVATE ${CMAKE_HOME_DIRECTORY}/src/)
