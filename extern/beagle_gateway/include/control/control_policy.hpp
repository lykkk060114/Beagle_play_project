#pragma once

#include <actuators/fan_controller.hpp>
#include <actuators/voice_player.hpp>
#include <common/node_state.hpp>
#include <host_recv/host_receiver.hpp>

#include <chrono>
#include <string>
#include <unordered_map>

namespace gateway {

class ControlPolicy {
public:
    ControlPolicy();

    void mark_offline_nodes(std::unordered_map<std::string, NodeState>& nodes);
    void apply_temperature_policy(const std::unordered_map<std::string, NodeState>& nodes,
                                  FanController& fan,
                                  VoicePlayer& voice);
    void apply_host_command(const HostCommand& command,
                            FanController& fan,
                            VoicePlayer& voice);

private:
    int last_policy_pwm_;
    double temperature_high_;
    bool manual_fan_control_;
    std::chrono::seconds offline_timeout_;

    int pwm_from_max_temp(double max_temp) const;
};

}  // namespace gateway
