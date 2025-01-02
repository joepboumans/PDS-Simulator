cmake_minimum_required(VERSION 3.12)
set(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR}/lib)

add_library(lib-waterfall-fcm OBJECT waterfall-fcm.cpp)
target_link_libraries(lib-waterfall-fcm PRIVATE  lib-fcm-sketch
                                                lib-cuckoo-hash)
target_include_directories(lib-waterfall-fcm PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(lib-waterfall-fcm
                           PRIVATE ${CMAKE_HOME_DIRECTORY}/src/)
