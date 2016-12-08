cmake_minimum_required(VERSION 3.3)
project(ClientUnixUDP)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set(SOURCE_FILES main.cpp)
add_executable(ServerUnix ${SOURCE_FILES})

target_link_libraries(ServerUnix pthread)