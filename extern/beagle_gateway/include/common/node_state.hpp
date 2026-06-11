#pragma once

#include <chrono>
#include <string>

namespace gateway {

struct NodeState {
    std::string node;
    double temperature = 0.0;
    double humidity = 0.0;
    double light = 0.0;
    int rssi = 0;
    int seq = 0;
    std::chrono::steady_clock::time_point last_seen;
    bool online = false;
};

}  // namespace gateway
