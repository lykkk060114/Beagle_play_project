/*** 
 * @Author: LYK && 2586356361@qq.com
 * @Date: 2026-05-24 10:31:13
 * @LastEditors: LYK && 2586356361@qq.com
 * @LastEditTime: 2026-05-25 19:14:33
 * @FilePath: /beagle_sender_remote/udp_send/src/udp_send.cpp
 * @Description: udp的发送程序， 在beagle_play板子上
 * @
 * @Copyright (c) 2026  All Rights Reserved. 
 */

#include "udp_send.hpp"

namespace BeagleSender {

    // 制造假数据的函数， 用来测试用
    // SensorData make_fake_data(const std::string& node, int seq) {
    //     SensorData data;
    //     data.node = node;
    //     // 数据段生成
    //     data.temperature = 28.0 + std::sin(seq * 0.1) * 1.5;
    //     data.humidity = 60.0 + std::cos(seq * 0.07) * 5.0;
    //     data.light = 20.0 + std::sin(seq * 0.2) * 10.0;
    //     data.rssi = -55 + static_cast<int>(std::sin(seq * 0.15) * 8);

    //     return data;
    // }
    SensorData make_real_data(const std::string &node, int seq) {
        SensorData data;
        data.node = node;
        // 读取传感器数据

        return data;
    }
}; /// namespace BeagleSender

int main() {
    const std::string ip = "192.168.7.1";
    constexpr int HOST_PORT = 9000;
    // freedom设备识别id
    std::string node_id = "F1";
    int interval_ms = 1000;

    // 设置ipv4的UDP套接字
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("socket");
        return 1;
    }

    sockaddr_in send_addr;
    // 设置目标地址
    send_addr.sin_family = AF_INET;
    send_addr.sin_port = htons(HOST_PORT);

    if (inet_pton(AF_INET, ip.c_str(), &send_addr.sin_addr) <= 0) {
        perror("inet_pton");
        close(sockfd);
        return 1;
    }

    std::cout << "\033[32mUDP Sender is sending to " << ip << ":" << HOST_PORT << "...\033[0m" << std::endl;
    std::cout << "\033[32m ============= START =============\033[0m" << std::endl;

    int seq{0};

    while (true) {
        // 创建数据段
        SensorData data = BeagleSender::make_real_data(node_id, seq); // 

        // 创建JSON字符串
        std::string send_msg = BeagleSender::build_json(data, seq);

        ssize_t sent_bytes = sendto(sockfd, send_msg.c_str(), send_msg.size(), 0,
                                reinterpret_cast<sockaddr*>(&send_addr), sizeof(send_addr));

        if (sent_bytes < 0) {
            perror("sendto");
        } else {
            std::cout << "\033[34mSent: " << send_msg << "\033[0m" << std::endl;
        }

        if (seq < 100) {
            seq++;
        } 

        std::this_thread::sleep_for(std::chrono::milliseconds(interval_ms));
    }

    close(sockfd);
    return 0;
}
// int main() {
//     const std::string ip = "192.168.7.1";
//     constexpr int HOST_PORT = 9000;

//     int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
//     if (sockfd < 0) {
//         perror("socket");
//         return 1;
//     }

//     sockaddr_in host_addr;
//     host_addr.sin_family = AF_INET;
//     host_addr.sin_port = htons(HOST_PORT);

//     if (inet_pton(AF_INET, ip.c_str(), &host_addr.sin_addr) <= 0) {
//         perror("inet_pton");
//         close(sockfd);
//         return 1;
//     }

//     int seq{0};

//     while (true) {
//         // 先发一条假消息
//         std::string msg = 
//             "\033[32mHello from BeaglePlay! Seq: \033[0m";

//         size_t sent_bytes = sendto(sockfd, msg.c_str(), msg.size(), 0,
//                                reinterpret_cast<sockaddr*>(&host_addr), sizeof(host_addr));

    
//         if (sent_bytes < 0) {
//             perror("sendto");
//         } else {
//             std::cout << "Sent: " << msg << std::endl;
//         }
    
//         seq ++;
//         std::this_thread::sleep_for(std::chrono::seconds(1));
//     }

//     close(sockfd);
//     return 0;

// }