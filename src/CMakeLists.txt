cmake_minimum_required(VERSION 3.6)
project(LockFreeQueueSources)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
add_library (LockFreeQueueLib main.cpp)
set(SOURCE_FILES main.cpp)
add_executable(LockFreeQueue ${SOURCE_FILES})