#include <control/control_policy.hpp>

#include <algorithm>
#include <iostream>

namespace gateway {
namespace {

constexpr int kAutoFanPwmPercent = 50;

}  // namespace

ControlPolicy::ControlPolicy()
    : last_policy_pwm_(-1),
      temperature_high_(32.0),
      manual_fan_control_(false),
      offline_timeout_(std::chrono::seconds(10)) {}

int ControlPolicy::pwm_from_max_temp(double max_temp) const {
    return max_temp > temperature_high_ ? kAutoFanPwmPercent : 0;
}

void ControlPolicy::mark_offline_nodes(std::unordered_map<std::string, NodeState>& nodes) {
    const auto now = std::chrono::steady_clock::now();
    for (auto& pair : nodes) {
        NodeState& node = pair.second;
        if (node.online && now - node.last_seen > offline_timeout_) {
            node.online = false;
            std::cout << "Node offline: " << node.node << std::endl;
        }
    }
}

void ControlPolicy::apply_temperature_policy(const std::unordered_map<std::string, NodeState>& nodes,
                                             FanController& fan,
                                             VoicePlayer& voice) {
    if (manual_fan_control_) {
        return;
    }

    bool has_online = false;
    double max_temp = -1000.0;

    for (const auto& pair : nodes) {
        const NodeState& node = pair.second;
        if (!node.online) {
            continue;
        }
        has_online = true;
        max_temp = std::max(max_temp, node.temperature);
    }

    const int target_pwm = has_online ? pwm_from_max_temp(max_temp) : 0;
    if (target_pwm == last_policy_pwm_) {
        if (target_pwm > 0) {
            voice.play_event("temp_high");
        }
        return;
    }

    if (target_pwm == 0) {
        fan.off();
        if (last_policy_pwm_ > 0) {
            voice.play_event("fan_off");
        }
    } else {
        fan.on(target_pwm);
        if (last_policy_pwm_ == 0) {
            voice.play_event("fan_on");
        }
        if (target_pwm > 0) {
            voice.play_event("temp_high");
        }
    }

    last_policy_pwm_ = target_pwm;
    if (has_online) {
        std::cout << "Policy max_temp=" << max_temp
                  << " fan_pwm=" << target_pwm << std::endl;
    } else {
        std::cout << "Policy no online node, fan off" << std::endl;
    }
}

void ControlPolicy::apply_host_command(const HostCommand& command,
                                       FanController& fan,
                                       VoicePlayer& voice) {
    if (command.has_temperature_high) {
        temperature_high_ = command.temperature_high;
        std::cout << "Host command temperature_high=" << temperature_high_
                  << " from=" << command.source << std::endl;
    }

    if (command.has_voice_enable) {
        voice.set_enabled(command.voice_enable);
        std::cout << "Voice enabled: " << (command.voice_enable ? "true" : "false")
                  << " from=" << command.source << std::endl;
    }

    const bool auto_command = command.has_fan_auto && command.fan_auto;
    if (command.has_fan_auto && command.fan_auto) {
        manual_fan_control_ = false;
        last_policy_pwm_ = -1;
        std::cout << "Host command fan_auto from=" << command.source << std::endl;
    }

    if (!command.has_fan_on) {
        return;
    }

    manual_fan_control_ = !auto_command;
    if (command.fan_on) {
        const int pwm = command.has_fan_pwm ? command.fan_pwm : 60;
        fan.on(pwm);
        last_policy_pwm_ = pwm;
        voice.play_event("fan_on");
        std::cout << "Host command fan_on pwm=" << pwm
                  << " from=" << command.source << std::endl;
    } else {
        fan.off();
        last_policy_pwm_ = 0;
        voice.play_event("fan_off");
        std::cout << "Host command fan_off from=" << command.source << std::endl;
    }
}

}  // namespace gateway
