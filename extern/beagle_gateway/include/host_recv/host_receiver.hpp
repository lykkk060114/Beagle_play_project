#pragma once

#include <optional>
#include <string>

namespace gateway {

struct HostCommand {
    bool has_fan_auto = false;
    bool fan_auto = false;
    bool has_fan_on = false;
    bool fan_on = false;
    bool has_fan_pwm = false;
    int fan_pwm = 60;
    bool has_temperature_high = false;
    double temperature_high = 32.0;
    bool has_voice_enable = false;
    bool voice_enable = true;
    std::string payload;
    std::string source;
};

class HostReceiver {
public:
    explicit HostReceiver(int listen_port = 9001);
    ~HostReceiver();

    bool open();
    std::optional<HostCommand> receive();

private:
    int listen_port_;
    int sockfd_;
};

}  // namespace gateway
