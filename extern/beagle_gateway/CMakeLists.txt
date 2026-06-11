cmake_minimum_required(VERSION 3.10)
project(beagle_gateway LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Threads REQUIRED)

add_library(fan_controller
    src/actuators/fan_controller.cpp
)

target_include_directories(fan_controller PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

add_library(voice_player
    src/actuators/voice_player.cpp
)

target_include_directories(voice_player PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(voice_player PRIVATE Threads::Threads)

add_library(gateway_common
    src/common/json_utils.cpp
)

target_include_directories(gateway_common PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

add_library(freedom_receiver
    src/freedom_recv/freedom_receiver.cpp
)

target_include_directories(freedom_receiver PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(freedom_receiver PUBLIC
    gateway_common
)

add_library(host_sender
    src/host_send/host_sender.cpp
)

target_include_directories(host_sender PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(host_sender PUBLIC
    gateway_common
)

add_library(host_receiver
    src/host_recv/host_receiver.cpp
)

target_include_directories(host_receiver PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(host_receiver PUBLIC
    gateway_common
)

add_library(control_policy
    src/control/control_policy.cpp
)

target_include_directories(control_policy PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(control_policy PUBLIC
    fan_controller
    voice_player
    host_receiver
)

add_executable(beagle_gateway
    src/main.cpp
)

target_include_directories(beagle_gateway PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(beagle_gateway PRIVATE
    fan_controller
    voice_player
    freedom_receiver
    host_sender
    host_receiver
    control_policy
    Threads::Threads
)
