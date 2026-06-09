cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(beagle_play_rtos_lyk)

target_sources(app PRIVATE
    src/get_sensor/main.c
)
