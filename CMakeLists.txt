cmake_minimum_required(VERSION 3.10)
project(beagle_play)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(RECV_SRC_FILES
    src/udp_recv/udp_recv.cpp
)

set(SEND_SRC_FILES
    src/udp_send/udp_send.cpp
)
add_executable(udp_recv ${RECV_SRC_FILES})

add_executable(udp_send ${SEND_SRC_FILES})