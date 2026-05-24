/*** 
 * @Author: LYK && 2586356361@qq.com
 * @Date: 2026-05-24 21:03:18
 * @LastEditors: LYK && 2586356361@qq.com
 * @LastEditTime: 2026-05-24 22:55:43
 * @FilePath: /beagle_play/extern/beagle_sender/include/udp_send.hpp
 * @Description: 这里放置所需要的头文件, 以及一些函数实现
 * @
 * @Copyright (c) 2026  All Rights Reserved. 
 */

#pragma once
// 头文件
#include "types.hpp"

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
#include <sstream>
// 数学库
#include <cmath>

// 线程
#include <thread>

namespace BeagleSender {

    /*** 
     * @description: 构建JSON字符串, 用来和主机传数据
     * @param {SensorData&} data
     * @param {int} seq
     * @return {*}
     */    
    inline std::string build_json(const SensorData& data, int seq) {
        std::ostringstream oss;

        oss << "\033[32m{" 
            << "\"node\":\"" << data.node << "\","
            << "\"light\":" << data.light << ","
            << "\"temperature\":" << data.temperature << ","
            << "\"humidity\":" << data.humidity << ","
            << "\"rssi\":" << data.rssi << ","
            << "\"seq\":" << seq
            << "\033[0m}";
        return oss.str();
    }

    /*** 
     * @description: 生成伪造的数据, 用来测试，以后会注释
     * @param {string&} node
     * @param {int} seq
     * @return {*}
     */    
    SensorData make_fake_data(const std::string& node, int seq);
}
