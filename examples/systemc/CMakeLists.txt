cmake_minimum_required(VERSION 3.2)
cmake_policy(SET CMP0054 NEW)

project (my_project)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake/Modules/")

# Enable C++11
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Specify bin path
set (EXECUTABLE_OUTPUT_PATH bin/)

set(MY_PROJECT_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp)

add_executable(my_project ${MY_PROJECT_SOURCES})

# Link with SystemC
find_package(SystemC REQUIRED)
if(SystemC_FOUND)
    target_include_directories(my_project PRIVATE "${SystemC_INCLUDE_DIRS}")
    target_link_libraries(my_project PRIVATE "${SystemC_LIBRARIES}")
endif(SystemC_FOUND)
find_package(TLM REQUIRED)
if(TLM_FOUND)
    target_include_directories(my_project PRIVATE "${TLM_INCLUDE_DIRS}")
endif(TLM_FOUND)

# Link with the "Threads library (required to link with AFF3CT after)
set(CMAKE_THREAD_PREFER_PTHREAD ON)
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

# Link with AFF3CT
set (AFF3CT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/cmake/Modules/")
find_package(AFF3CT CONFIG 2.3.0 REQUIRED)
target_link_libraries(my_project PRIVATE aff3ct::aff3ct-static-lib)