cmake_minimum_required(VERSION 3.20)
project(jt CXX)

option(JT_BUILD_EXAMPLES "Build jt examples" ON)
add_compile_options("-std=c++20")

add_library(jt INTERFACE)
target_include_directories(jt INTERFACE .)

if(${JT_BUILD_EXAMPLES})
  include_directories("${CMAKE_CURRENT_SOURCE_DIR}")
  add_subdirectory(examples)
endif()
