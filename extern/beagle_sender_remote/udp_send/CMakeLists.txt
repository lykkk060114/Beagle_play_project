cmake_minimum_required(VERSION 3.10)
project(beagle_play_sender LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(SENDER_SRC_FILES 
    ${CMAKE_CURRENT_SOURCE_DIR}/src/udp_send.cpp
)
add_executable(${PROJECT_NAME} ${SENDER_SRC_FILES})
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
