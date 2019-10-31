cmake_minimum_required(VERSION 3.2)
cmake_policy(SET CMP0054 NEW)

project (my_project)

# Enable C++11
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Specify bin path
set (EXECUTABLE_OUTPUT_PATH bin/)


set(SRC_FILES                        ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp)

add_executable(my_project            ${SRC_FILES})

# Link with the "Threads library (required to link with AFF3CT after)
set(CMAKE_THREAD_PREFER_PTHREAD ON)
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

# Link with AFF3CT
set (AFF3CT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/cmake/Modules/")
find_package(AFF3CT CONFIG 2.3.5 REQUIRED)
target_link_libraries(my_project PRIVATE aff3ct::aff3ct-static-lib)

file(GLOB_RECURSE SRC_FILES_CO       ${CMAKE_CURRENT_SOURCE_DIR}/src/common/*)
add_library(my_project_common STATIC ${SRC_FILES_CO})
target_include_directories(my_project_common     PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/common)
target_link_libraries(my_project_common PRIVATE aff3ct::aff3ct-static-lib)

target_include_directories(my_project            PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/common)
target_link_libraries(my_project            PRIVATE my_project_common)

# Enable pthread
# if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang" OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "AppleClang")
#     add_definitions (-Wno-overloaded-virtual)
#     target_link_libraries (my_project -lpthread)
# elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
#     add_definitions (-pthread)
#     target_link_libraries (my_project -lpthread)
# elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Intel")
#     add_definitions (-pthread)
#     target_link_libraries (my_project -pthread)
# endif()
