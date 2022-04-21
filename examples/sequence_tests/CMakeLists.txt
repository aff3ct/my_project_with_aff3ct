cmake_minimum_required(VERSION 3.2)
cmake_policy(SET CMP0054 NEW)

project (my_project)

# Enable C++11
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Specify bin path
set (EXECUTABLE_OUTPUT_PATH bin/)

# Generate the source files list
file(GLOB_RECURSE source_files ${CMAKE_CURRENT_SOURCE_DIR}/src/*)

# Create the executable from sources
# add_executable(my_project ${source_files})
add_executable(my_project ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp)

target_include_directories(my_project PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/)

# Link with the "Threads library (required to link with AFF3CT after)
set(CMAKE_THREAD_PREFER_PTHREAD ON)
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

# Link with AFF3CT
set (AFF3CT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/cmake/Modules/")
find_package(AFF3CT CONFIG 3.0.2 REQUIRED)
target_link_libraries(my_project PRIVATE aff3ct::aff3ct-static-lib)
