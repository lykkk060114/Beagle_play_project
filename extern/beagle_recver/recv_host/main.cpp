/*** 
 * @Author: LYK && 2586356361@qq.com
 * @Date: 2026-06-08 22:00:00
 * @LastEditors: LYK && 2586356361@qq.com
 * @LastEditTime: 2026-06-08 22:00:00
 * @FilePath: /beagle_play/extern/beagle_recver/recv_host/main.cpp
 * @Description: BeaglePlay接收主机dashboard下发的控制消息
 * @
 * @Copyright (c) 2026  All Rights Reserved. 
 */

// SOCKET
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

// IO
#include <cstring>
#include <iostream>

int main() {
    constexpr int HOST_CONTROL_PORT = 9001;

    // 接收主机下发的控制状态，后续在这里接GPIO/PWM
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("socket");
        return 1;
    }

    int reuse = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

    sockaddr_in listen_addr{};
    listen_addr.sin_family = AF_INET;
    listen_addr.sin_addr.s_addr = INADDR_ANY;
    listen_addr.sin_port = htons(HOST_CONTROL_PORT);

    if (bind(sockfd, reinterpret_cast<sockaddr*>(&listen_addr), sizeof(listen_addr)) < 0) {
        perror("bind(host)");
        close(sockfd);
        return 1;
    }

    std::cout << "\033[32mBeagle recv_host listening on 0.0.0.0:"
              << HOST_CONTROL_PORT << "\033[0m" << std::endl;
    std::cout << "\033[32mWaiting host control_state json...\033[0m" << std::endl;

    while (true) {
        char buffer[2048];
        sockaddr_in sender{};
        socklen_t sender_len = sizeof(sender);

        ssize_t n = recvfrom(sockfd, buffer, sizeof(buffer) - 1, 0,
                             reinterpret_cast<sockaddr*>(&sender), &sender_len);
        if (n < 0) {
            perror("recvfrom(host)");
            continue;
        }

        buffer[n] = '\0';

        char sender_ip[INET_ADDRSTRLEN]{};
        inet_ntop(AF_INET, &sender.sin_addr, sender_ip, sizeof(sender_ip));

        std::cout << "\033[34mHost [" << sender_ip << "]:" << ntohs(sender.sin_port)
                  << " -> " << buffer << "\033[0m" << std::endl;

        // TODO: 这里后续解析control_state，驱动灯、水泵和风扇PWM
    }

    close(sockfd);
    return 0;
}
