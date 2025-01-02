cmake_minimum_required(VERSION 3.12)
set(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR}/lib)

find_package(xxHash CONFIG REQUIRED)
add_library(lib-qwaterfall-fcm OBJECT qwaterfall-fcm.cpp)
target_link_libraries(lib-qwaterfall-fcm PRIVATE lib-fcm-sketch lib-qwaterfall)
target_include_directories(lib-qwaterfall-fcm PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(lib-qwaterfall-fcm
                           PRIVATE ${CMAKE_HOME_DIRECTORY}/src/)
