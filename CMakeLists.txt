cmake_minimum_required(VERSION 3.6)
project(LockFreeQueue)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)


include_directories (include)
add_subdirectory(src)
enable_testing()
#add_subdirectory(test)
