cmake_minimum_required(VERSION 3.12)
project(master-thesis)
set(CMAKE_CXX_STANDARD 23)

set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_SOURCE_DIR src)

# Setup source files
file(GLOB_RECURSE sources CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/*.h"
     "${CMAKE_SOURCE_DIR}/*.cpp")
message("Building for sources:")
foreach(source ${sources})
  file(RELATIVE_PATH source "${CMAKE_HOME_DIRECTORY}/src" ${source})
  message("\t${source}")
endforeach()

add_executable(main main.cpp ${sources})
target_compile_options(main PRIVATE -Wall -O3)

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
  # Add dependent class from src for linking
  if(EXISTS "${CMAKE_SOURCE_DIR}/${basename}/")
    file(GLOB deps CONFIGURE_DEPENDS
         "*src/${basename}/*.cpp""*src/${basename}/*.h")
  endif()
  add_executable("${basename}" test/${name} ${deps})
  target_link_libraries(${basename} PRIVATE Catch2::Catch2WithMain)
  include(CTest)
  include(Catch)
  catch_discover_tests(${basename})
endforeach()
