/*** 
 * @Author: LYK && 2586356361@qq.com
 * @Date: 2026-06-08 22:00:00
 * @LastEditors: LYK && 2586356361@qq.com
 * @LastEditTime: 2026-06-08 22:00:00
 * @FilePath: /beagle_play/extern/beagle_recver/recv_freedom/main.cpp
 * @Description: BeaglePlay接收Freedom传感器数据，保存最新一包
 * @
 * @Copyright (c) 2026  All Rights Reserved. 
 */

// SOCKET
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

// IO
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iostream>

int main() {
    constexpr int FREEDOM_PORT = 9999;
    const char *LATEST_FILE = "/tmp/beagle_freedom_latest.json";
    const char *TMP_FILE = "/tmp/beagle_freedom_latest.json.tmp";

    // 接收Freedom板子的IPv6 UDP数据
    int recv_sock = socket(AF_INET6, SOCK_DGRAM, 0);
    if (recv_sock < 0) {
        perror("socket(recv)");
        return 1;
    }

    int reuse = 1;
    setsockopt(recv_sock, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

    sockaddr_in6 freedom_addr{};
    freedom_addr.sin6_family = AF_INET6;
    freedom_addr.sin6_addr = in6addr_any;
    freedom_addr.sin6_port = htons(FREEDOM_PORT);

    if (bind(recv_sock, reinterpret_cast<sockaddr*>(&freedom_addr), sizeof(freedom_addr)) < 0) {
        perror("bind(freedom)");
        close(recv_sock);
        return 1;
    }

    std::cout << "\033[32mBeagle recv_freedom listening on [::]:" << FREEDOM_PORT << "\033[0m" << std::endl;
    std::cout << "\033[32mLatest payload file: " << LATEST_FILE << "\033[0m" << std::endl;

    while (true) {
        char buffer[2048];
        sockaddr_in6 sender{};
        socklen_t sender_len = sizeof(sender);

        ssize_t n = recvfrom(recv_sock, buffer, sizeof(buffer) - 1, 0,
                             reinterpret_cast<sockaddr*>(&sender), &sender_len);
        if (n < 0) {
            perror("recvfrom(freedom)");
            continue;
        }

        buffer[n] = '\0';

        char sender_ip[INET6_ADDRSTRLEN]{};
        inet_ntop(AF_INET6, &sender.sin6_addr, sender_ip, sizeof(sender_ip));

        std::cout << "\033[34mFreedom [" << sender_ip << "]:" << ntohs(sender.sin6_port)
                  << " -> " << buffer << "\033[0m" << std::endl;

        // 写入临时文件后再替换，避免send_host读到半包
        std::ofstream out(TMP_FILE);
        if (!out) {
            std::cerr << "\033[31mopen latest tmp file failed\033[0m" << std::endl;
            continue;
        }
        out << buffer;
        out.close();

        if (std::rename(TMP_FILE, LATEST_FILE) != 0) {
            perror("rename(latest)");
            continue;
        }
    }

    close(recv_sock);
    return 0;
}
