/*** 
 * @Author: LYK && 2586356361@qq.com
 * @Date: 2026-05-24 21:14:07
 * @LastEditors: LYK && 2586356361@qq.com
 * @LastEditTime: 2026-05-24 21:15:32
 * @FilePath: /beagle_play/src/dashboard_server/main.cpp
 * @Description: main文件 
 * @
 * @Copyright (c) 2026  All Rights Reserved. 
 */


#include <dashboard_server/dashboard_server.hpp>

int main() {
    std::signal(SIGINT, dashboard::handleSignal);
    std::signal(SIGTERM, dashboard::handleSignal);

    dashboard::DashboardServer server;
    if (!server.start()) {
        return 1;
    }

    std::cout << "dashboard_server listening on http://127.0.0.1:" << dashboard::kHttpPort << std::endl;
    std::cout << "udp listener ready on 0.0.0.0:" << dashboard::kUdpPort << std::endl;

    server.run();
    return 0;
}
