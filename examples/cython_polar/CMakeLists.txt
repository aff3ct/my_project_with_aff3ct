cmake_minimum_required(VERSION 3.2)
cmake_policy(SET CMP0054 NEW)

project (codec)

# Enable C++14
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Specify bin path
set (EXECUTABLE_OUTPUT_PATH bin/)

# Create the executable from sources
add_executable(codec ${CMAKE_CURRENT_SOURCE_DIR}/src/codec.cpp)

# Link with the "Threads library (required to link with AFF3CT after)
set(CMAKE_THREAD_PREFER_PTHREAD ON)
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

# Link with AFF3CT
set (AFF3CT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/cmake/Modules/")
find_package(AFF3CT CONFIG 2.3.2 REQUIRED)
target_link_libraries(codec PRIVATE aff3ct::aff3ct-static-lib)
