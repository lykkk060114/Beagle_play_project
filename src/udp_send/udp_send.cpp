/*** 
 * @Author: LYK && 2586356361@qq.com
 * @Date: 2026-05-24 10:31:13
 * @LastEditors: LYK && 2586356361@qq.com
 * @LastEditTime: 2026-05-24 10:42:57
 * @FilePath: /beagle_play/src/udp_send/udp_send.cpp
 * @Description: udp的发送程序， 在beagle_play板子上
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

// IO
#include <iostream>
#include <cstring>
#include <string>

// 线程
#include <thread>

int main() {
    const std::string ip = "127.0.0.1";
    constexpr int HOST_PORT = 9000;

    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("socket");
        return 1;
    }

    sockaddr_in host_addr;
    host_addr.sin_family = AF_INET;
    host_addr.sin_port = htons(HOST_PORT);

    if (inet_pton(AF_INET, ip.c_str(), &host_addr.sin_addr) <= 0) {
        perror("inet_pton");
        close(sockfd);
        return 1;
    }

    int seq{0};

    while (true) {
        // 先发一条假消息
        std::string msg = 
            "\033[32mHello from BeaglePlay! Seq: \033[0m";

        size_t sent_bytes = sendto(sockfd, msg.c_str(), msg.size(), 0,
                               reinterpret_cast<sockaddr*>(&host_addr), sizeof(host_addr));

    
        if (sent_bytes < 0) {
            perror("sendto");
        } else {
            std::cout << "Sent: " << msg << std::endl;
        }
    
        seq ++;
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    close(sockfd);
    return 0;

}