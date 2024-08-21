cmake_minimum_required(VERSION 3.12)
project(master-thesis)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_SOURCE_DIR ${CMAKE_HOME_DIRECTORY}/src)

add_subdirectory(lib/count-min)
add_subdirectory(lib/fcm-sketch)
add_subdirectory(lib/simulator)
add_subdirectory(lib/iblt)
add_subdirectory(lib/cuckoo-hash)
include_directories(${CMAKE_SOURCE_DIR})

add_executable(main main.cpp)
target_link_libraries(main lib-count-min lib-fcm-sketch lib-simulator lib-iblt
                      lib-cuckoo-hash)
target_include_directories(main PUBLIC ${CMAKE_SOURCE_DIR}
                                       ${CMAKE_HOME_DIRECTORY})
target_compile_options(main PRIVATE -Wall -O0)

# https://github.com/catchorg/Catch2/blob/devel/docs/cmake-integration.md#installing-catch2-from-git-repository
find_package(Catch2 3 REQUIRED)

# Setup all tests
file(
  GLOB tests CONFIGURE_DEPENDS
  RELATIVE "${CMAKE_HOME_DIRECTORY}/test/"
  "*test/*.h" "*test/*.cpp")

foreach(name ${tests})
  message("${name}")
  string(REPLACE ".cpp" "" basename "${name}")
  message("Adding ${name} with as ${basename}")
  # Add dependent class from src for linking if(EXISTS
  # "${CMAKE_SOURCE_DIR}/${basename}/") file(GLOB deps CONFIGURE_DEPENDS
  # "*src/${basename}/*.cpp" "*src/${basename}/*.h") endif()
  add_executable("${basename}" test/${name} ${deps})
  target_link_libraries(${basename} PRIVATE Catch2::Catch2WithMain)
  target_link_libraries(
    ${basename} PUBLIC lib-count-min lib-fcm-sketch lib-simulator lib-iblt
                       lib-cuckoo-hash)
  target_include_directories(${basename} PUBLIC ${CMAKE_SOURCE_DIR}
                                                ${CMAKE_HOME_DIRECTORY})
  include(CTest)
  include(Catch)
  catch_discover_tests(${basename})
endforeach()
