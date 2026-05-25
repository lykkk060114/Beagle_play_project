cmake_minimum_required(VERSION 3.20.0)
project(beagle_play_rtos_lyk)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

target_include_directories(app
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
# file(GLOB_RECURSE app_sources ${CMAKE_CURRENT_SOURCE_DIR}/src/*.c)

target_sources(app 
    PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/src/get_sensor/main.c)








