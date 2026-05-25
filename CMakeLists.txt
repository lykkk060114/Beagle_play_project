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

# === 收集dashboard_server的源文件 ===， configure_depends 选项会让CMake在每次构建时重新评估这个glob，这样就能自动捕捉到新增的源文件了
file(GLOB DASHBOARD_SRC_FILES
    CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/src/dashboard_server/*.cpp
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
