#include <host_recv/host_receiver.hpp>

#include <common/json_utils.hpp>

#include <arpa/inet.h>
#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <map>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

namespace gateway {
namespace {

constexpr int kBufferSize = 2048;

std::string sender_address(const sockaddr_in& sender) {
    char ip[INET_ADDRSTRLEN] {};
    inet_ntop(AF_INET, &sender.sin_addr, ip, sizeof(ip));
    return std::string(ip) + ":" + std::to_string(ntohs(sender.sin_port));
}

}  // namespace

HostReceiver::HostReceiver(int listen_port)
    : listen_port_(listen_port),
      sockfd_(-1) {}

HostReceiver::~HostReceiver() {
    if (sockfd_ >= 0) {
        close(sockfd_);
    }
}

bool HostReceiver::open() {
    sockfd_ = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd_ < 0) {
        perror("host receiver socket");
        return false;
    }

    int reuse = 1;
    setsockopt(sockfd_, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

    int flags = fcntl(sockfd_, F_GETFL, 0);
    if (flags >= 0) {
        fcntl(sockfd_, F_SETFL, flags | O_NONBLOCK);
    }

    sockaddr_in addr {};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(listen_port_);

    if (bind(sockfd_, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
        perror("host receiver bind");
        close(sockfd_);
        sockfd_ = -1;
        return false;
    }

    std::cout << "Host control receiver listening on UDP " << listen_port_ << std::endl;
    return true;
}

std::optional<HostCommand> HostReceiver::receive() {
    if (sockfd_ < 0) {
        return std::nullopt;
    }

    char buffer[kBufferSize] {};
    sockaddr_in sender {};
    socklen_t sender_len = sizeof(sender);

    const ssize_t n = recvfrom(sockfd_,
                               buffer,
                               sizeof(buffer) - 1,
                               0,
                               reinterpret_cast<sockaddr*>(&sender),
                               &sender_len);
    if (n < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR) {
            return std::nullopt;
        }
        perror("host recvfrom");
        return std::nullopt;
    }

    buffer[n] = '\0';
    HostCommand command;
    command.payload = buffer;
    command.source = sender_address(sender);

    std::map<std::string, std::string> fields;
    std::string error;
    if (!parse_flat_json(command.payload, fields, error)) {
        std::cerr << "Bad host command from " << command.source
                  << ": " << error << " payload=" << command.payload << std::endl;
        return std::nullopt;
    }

    const auto fan_on_it = fields.find("fan_on");
    if (fan_on_it != fields.end() && parse_bool_token(fan_on_it->second, command.fan_on)) {
        command.has_fan_on = true;
    }
    if (!command.has_fan_on) {
        const auto legacy_fan_it = fields.find("fan");
        if (legacy_fan_it != fields.end() && parse_bool_token(legacy_fan_it->second, command.fan_on)) {
            command.has_fan_on = true;
        }
    }

    const auto fan_auto_it = fields.find("fan_auto");
    if (fan_auto_it != fields.end() && parse_bool_token(fan_auto_it->second, command.fan_auto)) {
        command.has_fan_auto = true;
    }

    const auto pwm_it = fields.find("fan_pwm");
    if (pwm_it != fields.end() && parse_int_token(pwm_it->second, command.fan_pwm)) {
        command.has_fan_pwm = true;
    }
    if (!command.has_fan_pwm) {
        const auto legacy_pwm_it = fields.find("pwm");
        if (legacy_pwm_it != fields.end() && parse_int_token(legacy_pwm_it->second, command.fan_pwm)) {
            command.has_fan_pwm = true;
        }
    }

    const auto temp_high_it = fields.find("temperature_high");
    if (temp_high_it != fields.end() &&
        parse_double_token(temp_high_it->second, command.temperature_high)) {
        command.has_temperature_high = true;
    }

    const auto voice_it = fields.find("voice_enable");
    if (voice_it != fields.end() && parse_bool_token(voice_it->second, command.voice_enable)) {
        command.has_voice_enable = true;
    }

    const auto light_it = fields.find("light");
    if (light_it != fields.end() && parse_bool_token(light_it->second, command.light_on)) {
        command.has_light_on = true;
    }

    for (const auto& [name, value] : fields) {
        constexpr const char* prefix = "light_";
        if (name.rfind(prefix, 0) != 0) {
            continue;
        }

        bool light_on = false;
        if (parse_bool_token(value, light_on)) {
            command.node_lights[name.substr(std::strlen(prefix))] = light_on;
        }
    }

    return command;
}

}  // namespace gateway
