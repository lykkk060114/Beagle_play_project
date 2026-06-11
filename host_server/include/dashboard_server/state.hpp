/*** 
 * @Author: LYK && 2586356361@qq.com
 * @Date: 2026-05-24 21:06:42
 * @LastEditors: LYK && 2586356361@qq.com
 * @LastEditTime: 2026-05-31 16:09:56
 * @FilePath: /beagle_play/include/dashboard_server/state.hpp
 * @Description: 这个文件是状态相关的结构体定义
 * @
 * @Copyright (c) 2026  All Rights Reserved. 
 */
#pragma once

// STL
#include <chrono>
#include <deque>
#include <map>
#include <string>

namespace dashboard {

constexpr int kUdpPort = 9000;
constexpr int kHttpPort = 8080;
constexpr int kGatewayControlPort = 9001;
constexpr const char* kDefaultGatewaySshTarget = "debian@192.168.7.2";
constexpr int kAutoFanPwmPercent = 50;
constexpr std::chrono::seconds kNodeOfflineTimeout{5};
constexpr std::chrono::seconds kGatewayOfflineTimeout{5};
constexpr std::size_t kMaxEvents = 50;

long long nowMs();

/*** 
 * @description: 事件日志结构体
 * @return {*}
 */
struct EventEntry {
    std::string level;
    std::string text;
};

/*** 
 * @description: 执行器状态结构体
 * @return {*}
 */
struct ActuatorState {
    bool light{false};
    bool pump{false};
    bool fan{false};
    int fan_pwm_percent{0};
    int manual_fan_pwm_percent{50};
};

/*** 
 * @description: 一些config的结构体
 * @return {*}
 */
struct ConfigState {
    double light_low{30.0};
    double humidity_low{45.0};
    double temperature_high{32.0};
    int pump_duration_sec{3};
    int pump_cooldown_sec{60};
};

/*** 
 * @description: 节点状态结构体
 * @return {*}
 */
struct NodeState {
    bool online{false};
    double temperature{0.0};
    double humidity{0.0};
    double light{0.0};
    int rssi{0};
    long long last_seen_ms{0};
};

/***
 * @description: 每个 Freedom 节点对应的自动控制判断
 * @return {*}
 */
struct NodeControlState {
    bool online{false};
    bool light_on{false};
    bool pump_on{false};
    bool fan_on{false};
    int fan_pwm_percent{0};
};

/*** 
 * @description: 系统状态结构体
 * @return {*}
 */
struct SystemState {
    bool running{false};
    std::string mode{"safe"};
    std::string gateway{"offline"};
    long long last_gateway_seen_ms{0};
    long long last_update_ms{0};
    std::map<std::string, NodeState> nodes{};
    std::map<std::string, NodeControlState> node_controls{};
    ActuatorState actuators{};
    ConfigState config{};
    std::deque<EventEntry> events{};
};

}  // namespace dashboard
