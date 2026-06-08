/*** 
 * @Author: LYK && 2586356361@qq.com
 * @Date: 2026-06-08 22:20:00
 * @LastEditors: LYK && 2586356361@qq.com
 * @LastEditTime: 2026-06-08 22:20:00
 * @FilePath: /beagle_play/extern/beagle_recver/send_host/main.cpp
 * @Description: BeaglePlay把Freedom最新传感器数据转发给主机dashboard
 * @
 * @Copyright (c) 2026  All Rights Reserved. 
 */

// SOCKET
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

// 时间
#include <chrono>
#include <thread>

// IO
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

static std::string read_file(const char *path) {
    std::ifstream in(path);
    if (!in) {
        return {};
    }

    std::ostringstream oss;
    oss << in.rdbuf();
    return oss.str();
}

int main() {
    constexpr int HOST_PORT = 9000;
    constexpr int INTERVAL_MS = 200;
    const std::string host_ip = "192.168.7.1";
    const char *LATEST_FILE = "/tmp/beagle_freedom_latest.json";

    // 发送到主机dashboard
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("socket");
        return 1;
    }

    sockaddr_in host_addr{};
    host_addr.sin_family = AF_INET;
    host_addr.sin_port = htons(HOST_PORT);

    if (inet_pton(AF_INET, host_ip.c_str(), &host_addr.sin_addr) <= 0) {
        perror("inet_pton(host)");
        close(sockfd);
        return 1;
    }

    std::cout << "\033[32mBeagle send_host forwarding to "
              << host_ip << ":" << HOST_PORT << "\033[0m" << std::endl;
    std::cout << "\033[32mWatch latest file: " << LATEST_FILE << "\033[0m" << std::endl;

    std::string last_payload;
    while (true) {
        std::string payload = read_file(LATEST_FILE);
        if (payload.empty() || payload == last_payload) {
            std::this_thread::sleep_for(std::chrono::milliseconds(INTERVAL_MS));
            continue;
        }

        ssize_t sent = sendto(sockfd, payload.c_str(), payload.size(), 0,
                              reinterpret_cast<sockaddr*>(&host_addr), sizeof(host_addr));
        if (sent < 0) {
            perror("sendto(host)");
        } else {
            last_payload = payload;
            std::cout << "\033[34mForward host <- " << payload << "\033[0m" << std::endl;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(INTERVAL_MS));
    }

    close(sockfd);
    return 0;
}
