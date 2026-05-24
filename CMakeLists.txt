cmake_minimum_required(VERSION 3.10)
project(beagle_play)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(SRC_FILES
    src/udp_recv/udp_recv.cpp
)

add_executable(udp_recv ${SRC_FILES})