cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(beagle_play_rtos_lyk)

set(FREEDOM_NODE_ID "F1" CACHE STRING "Freedom node id, for example F1 or F2")
set(FREEDOM_NODE_IPV6 "2001:db8::1" CACHE STRING "Freedom node IPv6 address")

target_include_directories(app PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_definitions(app PRIVATE
    NODE_ID=\"${FREEDOM_NODE_ID}\"
    FREEDOM_IPV6_ADDR=\"${FREEDOM_NODE_IPV6}\"
)

target_sources(app PRIVATE
    src/get_sensor/main.c
    src/oled/oled_show.c
)
