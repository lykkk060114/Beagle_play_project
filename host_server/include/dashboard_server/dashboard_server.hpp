/*** 
 * @Author: LYK && 2586356361@qq.com
 * @Date: 2026-05-24 21:14:58
 * @LastEditors: LYK && 2586356361@qq.com
 * @LastEditTime: 2026-05-31 14:59:09
 * @FilePath: /beagle_play/include/dashboard_server/dashboard_server.hpp
 * @Description: 放置头文件在其中, 并在这里定义DashboardServer类
 * @
 * @Copyright (c) 2026  All Rights Reserved. 
 */

#pragma once

// Project
#include <dashboard_server/http_utils.hpp>
#include <dashboard_server/json_utils.hpp>
#include <dashboard_server/state.hpp>

// Socket
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>

// System
#include <algorithm>
#include <atomic>
#include <cerrno>
#include <cstdio>
#include <csignal>
#include <iostream>
#include <sstream>
#include <mutex>
#include <thread>
#include <string>

namespace dashboard {

extern std::atomic<bool> g_stop_requested;

void handleSignal(int);

/*** 
 * @description: 整个工程的核心类， 包含了HTTP服务器和UDP服务器的实现
 * 
 * @return {*}
 */
class DashboardServer {
public:
    bool start();
    void run();

private:
    int udp_sock_{-1};
    int http_sock_{-1};
    sockaddr_in last_gateway_addr_{};
    bool has_gateway_addr_{false};
    unsigned long control_seq_{0};
    long long last_auto_pump_ms_{0};
    std::string index_html_{};
    std::string vue_js_{};
    std::mutex state_mutex_{};
    // 整个后端维护的数据
    SystemState state_{};

private:

    bool openUdpSocket();
    bool openHttpSocket();
    void appendEventLocked(const std::string& level, const std::string& text);
    void setAllActuatorsOffLocked();
    void setFanLocked(bool enabled);
    void setManualFanPwmLocked(int percent);
    void refreshNodeOnlineLocked();
    void refreshGatewayOnlineLocked();
    void refreshNodeControlsLocked();
    bool applyAutomaticControlLocked();
    void triggerPumpOnceLocked(const std::string& event_text);
    void rememberGatewayAddressLocked(const sockaddr_in& sender);
    std::string buildControlJsonLocked();
    void sendControlStateLocked();
    std::string buildStatusJsonLocked();
    std::string buildErrorJson(int code, const std::string& message);

    /*** 
     * @description: 处理HTTP请求的函数， 这里会根据不同的URL和方法来执行不同的操作
     * @param {HttpRequest&} request
     * 通过request 结构体传递 HTTP请求的相关信息，包括 GET/POST 方法 ， 调用后端 api 路径, 相关http版本, 以及相关的header和body等信息
     * POST /api/control HTTP/1.1
        Host: 127.0.0.1:8080
        Content-Type: application/json
        Content-Length: 32

        {"device":"light","action":"on"}
        一条完整消息大概这样
     * @return {*}
     */    
    HttpReply handleRequest(const HttpRequest& request);
    void handleClient(int client_fd);
    void udpLoop();
    void httpLoop();
};

}  // namespace dashboard
