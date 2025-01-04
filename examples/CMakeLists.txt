cmake_minimum_required(VERSION 3.20)
project(jt-examples CXX)

# Gather all .cpp files in the current directory
file(GLOB examples "*.cpp")

# Iterate over each source file
foreach(src IN LISTS examples)
  # Get the filename without the extension
  get_filename_component(executable_name ${src} NAME_WE)

  # Create an executable with the name derived from the source file
  add_executable(${executable_name} ${src})

  # Include the parent directory (if needed)
  target_include_directories(${executable_name} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
endforeach()
