/*** 
 * @Author: LYK && 2586356361@qq.com
 * @Date: 2026-05-24 10:09:08
 * @LastEditors: LYK && 2586356361@qq.com
 * @LastEditTime: 2026-05-24 21:02:55
 * @FilePath: /beagle_play/src/udp_recv/udp_recv.cpp
 * @Description: 主机端的udp接收程序, 开放9000端口，等别人的数据传输过来
 * @
 * @Copyright (c) 2026  All Rights Reserved. 
 */

#include "udp_recv.hpp"
int main() {
    constexpr int PORT = 9000;

    /*** 
     * @description: 创建UDP套接字
     * @return {*}
     * @param AF_INET 表示 IPV4 协议, 
     * SOCK_DGRAM 表示使用UDP协议, SOCK_STREAM 则表示使用TCP协议
     */    
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    // 创建UDP套接字
    if (sockfd < 0) {
        // 创建套接字失败
        perror("socket");
        return 1;
    }
    // socket internet

    sockaddr_in server_addr{};  
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY; // 监听所有接口
    server_addr.sin_port = htons(PORT); // 绑定端口

    if (bind(sockfd, reinterpret_cast<sockaddr*>(&server_addr), sizeof(server_addr)) < 0) {
        // 绑定失败
        perror("bind");
        close(sockfd);
        return 1;
    }

    std::cout << "\033[32mUDP Server is listening on port\n\n " << PORT << "...\033[0m" << std::endl;

    while(true) {
        char buffer[2048];
        sockaddr_in sender{};
        socklen_t sender_len = sizeof(sender);

        ssize_t n = 
            recvfrom(sockfd, buffer, sizeof(buffer) - 1, 0, reinterpret_cast<sockaddr*>(&sender), &sender_len);

        if (n < 0) {
            perror("recvfrom");
            continue;
        }

        buffer[n] = '\0'; // 确保字符串以null结尾

        char sender_ip[INET_ADDRSTRLEN]{};
        inet_ntop(AF_INET, &sender.sin_addr, sender_ip, sizeof(sender_ip));

        std::cout << "\033[34mReceived from " << sender_ip << ":" << ntohs(sender.sin_port) 
                  << " - " << buffer << "\033[0m" << std::endl;

    }

    close(sockfd);
    return 0;
}
