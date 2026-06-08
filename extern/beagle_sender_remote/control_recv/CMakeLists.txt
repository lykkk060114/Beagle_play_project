cmake_minimum_required(VERSION 3.10)
project(beagle_control_recv LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(beagle_control_recv
    src/control_recv.cpp
)
