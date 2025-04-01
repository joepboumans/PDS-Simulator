cmake_minimum_required(VERSION 3.12)
project(master-thesis)

if(UNIX)
  message("This is a ${CMAKE_SYSTEM_NAME} system")
  add_definitions(-DSYSTEM="UNIX")
elseif(WIN32)
  message("This is a Windows System")
  add_definitions(-DSYSTEM="WINDOWS")
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -O3 -lpthread -mavx -march=native -msse4.2 -msse4.1 -msse3 -msse2")
# set(CMAKE_BUILD_TYPE Debug)
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_SOURCE_DIR ${CMAKE_HOME_DIRECTORY}/src)


add_subdirectory(lib/count-min)
add_subdirectory(lib/fcm-sketch)
add_subdirectory(lib/simulator)
add_subdirectory(lib/iblt)
add_subdirectory(lib/waterfall)
add_subdirectory(lib/waterfall-fcm)
add_subdirectory(lib/qwaterfall)
add_subdirectory(lib/qwaterfall-fcm)
add_subdirectory(lib/catch2)
include_directories(${CMAKE_SOURCE_DIR})

# Added custom hash, not ideal setup like this TODO: Integrate with design
# find_package(xxHash CONFIG REQUIRED)
find_package(ZLIB REQUIRED)

add_executable(main main.cpp)
target_link_libraries(
  main
  PUBLIC  lib-count-min
         lib-fcm-sketch
         lib-simulator
         lib-iblt
         lib-waterfall
         lib-waterfall-fcm
        lib-qwaterfall
        lib-qwaterfall-fcm
)
target_include_directories(main PUBLIC ${CMAKE_SOURCE_DIR}
                                       ${CMAKE_HOME_DIRECTORY})
target_link_libraries(main PRIVATE ZLIB::ZLIB)

# https://github.com/catchorg/Catch2/blob/devel/docs/cmake-integration.md#installing-catch2-from-git-repository
# find_package(Catch2 3 REQUIRED)

# Find all tests
file(
  GLOB tests CONFIGURE_DEPENDS
  RELATIVE "${CMAKE_HOME_DIRECTORY}/test/"
  "*test/*.h" "*test/*.cpp")

# Setup each test, includes all libs to prevent linking issues
foreach(name ${tests})
  message("${name}")
  string(REPLACE ".cpp" "" basename "${name}")
  message("Adding ${name} with as ${basename}")
  add_executable("${basename}" test/${name} ${deps})
  target_link_libraries(${basename} PRIVATE lib-catch2)
  target_link_libraries(${basename} PRIVATE ZLIB::ZLIB)
  target_link_libraries(
    ${basename} PUBLIC  
        lib-count-min 
        lib-fcm-sketch
        lib-simulator
        lib-iblt
        lib-waterfall
        lib-waterfall-fcm
        lib-qwaterfall
        lib-qwaterfall-fcm)
  target_include_directories(${basename} PUBLIC ${CMAKE_SOURCE_DIR}
                                                ${CMAKE_HOME_DIRECTORY})
  # include(Catch)
  # catch_discover_tests(${basename})
endforeach()
