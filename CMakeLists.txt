# CMake entry point
cmake_minimum_required (VERSION 3.0.2)

project (my_project)

# Set the path to the CMake modules
set (CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")
set (CMAKE_MACOSX_RPATH 1)

# Specify bin path
set (EXECUTABLE_OUTPUT_PATH bin/)

# Generate the source files list
file (GLOB_RECURSE source_files src/*)

# Object
add_library (my_project-obj OBJECT ${source_files})

# Binary
add_executable        (my_project-bin $<TARGET_OBJECTS:my_project-obj>)
set_target_properties (my_project-bin PROPERTIES OUTPUT_NAME my_project)

# Add AFF3CT as a library
add_definitions (-DMULTI_PREC)
set(ENABLE_EXEC OFF CACHE BOOL "Enable to compile AFF3CT executable" FORCE)
add_subdirectory(lib/aff3ct/)
target_link_libraries (my_project-bin aff3ct-shared-lib)
include_directories (lib/aff3ct/src/)

# Enable C++11 and pthread
if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang"
        OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "AppleClang")
    add_definitions (-std=c++1y -Wno-overloaded-virtual)
    target_link_libraries (my_project-bin -lpthread)
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    add_definitions (-std=c++11 -pthread)
    target_link_libraries (my_project-bin -lpthread)
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Intel")
    add_definitions (-std=c++11 -pthread)
    target_link_libraries (my_project-bin -pthread)
endif()
