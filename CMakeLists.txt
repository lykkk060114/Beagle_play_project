cmake_minimum_required(VERSION 3.10)
project(beagle_play)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Threads REQUIRED)

set(RECV_SRC_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/udp_recv/udp_recv.cpp
)

# set(SEND_SRC_FILES
#     ${CMAKE_CURRENT_SOURCE_DIR}/src/udp_send/udp_send.cpp
# )

set(DASHBOARD_SRC_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/dashboard_server/main.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/dashboard_server/dashboard_server.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/dashboard_server/http_utils.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/dashboard_server/json_utils.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/dashboard_server/state.cpp
)
# === 生成可执行文件 ===
add_executable(udp_recv ${RECV_SRC_FILES})
# add_executable(udp_send ${SEND_SRC_FILES})
add_executable(dashboard_server ${DASHBOARD_SRC_FILES})

target_include_directories(udp_recv PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include/udp_recv
)

# target_include_directories(udp_send PRIVATE
#     ${CMAKE_CURRENT_SOURCE_DIR}/include/udp_send
# )

target_include_directories(dashboard_server PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_definitions(dashboard_server PRIVATE
    DASHBOARD_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
)

target_link_libraries(dashboard_server PRIVATE Threads::Threads)
