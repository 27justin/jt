cmake_minimum_required(VERSION 3.20)
project(jt CXX)

add_compile_options("-std=c++20")

include_directories("${CMAKE_CURRENT_SOURCE_DIR}")
add_subdirectory(examples)
