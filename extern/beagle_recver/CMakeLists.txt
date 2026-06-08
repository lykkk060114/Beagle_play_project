cmake_minimum_required(VERSION 3.10)
project(beagle_play_recver LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(beagle_recv_freedom
    ${CMAKE_CURRENT_SOURCE_DIR}/recv_freedom/main.cpp
)

add_executable(beagle_send_host
    ${CMAKE_CURRENT_SOURCE_DIR}/send_host/main.cpp
)

add_executable(beagle_recv_host
    ${CMAKE_CURRENT_SOURCE_DIR}/recv_host/main.cpp
)
