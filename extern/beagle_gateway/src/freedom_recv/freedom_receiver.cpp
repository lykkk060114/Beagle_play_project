#include <freedom_recv/freedom_receiver.hpp>

#include <common/json_utils.hpp>

#include <arpa/inet.h>
#include <cerrno>
#include <cstring>
#include <iostream>
#include <map>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

namespace gateway {
namespace {

constexpr int kBufferSize = 2048;

std::string sender_address(const sockaddr_in6& sender) {
    char ip[INET6_ADDRSTRLEN] {};
    inet_ntop(AF_INET6, &sender.sin6_addr, ip, sizeof(ip));
    return std::string(ip) + ":" + std::to_string(ntohs(sender.sin6_port));
}

std::optional<NodeState> parse_sensor_packet(const std::map<std::string, std::string>& fields) {
    const auto node_it = fields.find("node");
    if (node_it == fields.end() || node_it->second.empty()) {
        return std::nullopt;
    }

    NodeState state;
    state.node = node_it->second;

    auto get_double = [&](const std::string& key, double& value) {
        const auto it = fields.find(key);
        return it != fields.end() && parse_double_token(it->second, value);
    };
    auto get_int = [&](const std::string& key, int& value) {
        const auto it = fields.find(key);
        return it != fields.end() && parse_int_token(it->second, value);
    };

    if (!get_double("temperature", state.temperature) ||
        !get_double("humidity", state.humidity) ||
        !get_double("light", state.light)) {
        return std::nullopt;
    }

    get_int("rssi", state.rssi);
    get_int("seq", state.seq);
    return state;
}

}  // namespace

FreedomReceiver::FreedomReceiver(int port)
    : port_(port),
      sockfd_(-1) {}

FreedomReceiver::~FreedomReceiver() {
    if (sockfd_ >= 0) {
        close(sockfd_);
    }
}

bool FreedomReceiver::open() {
    sockfd_ = socket(AF_INET6, SOCK_DGRAM, 0);
    if (sockfd_ < 0) {
        perror("freedom socket");
        return false;
    }

    int reuse = 1;
    setsockopt(sockfd_, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

    timeval timeout {};
    timeout.tv_sec = 1;
    timeout.tv_usec = 0;
    setsockopt(sockfd_, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

    sockaddr_in6 addr {};
    addr.sin6_family = AF_INET6;
    addr.sin6_addr = in6addr_any;
    addr.sin6_port = htons(port_);

    if (bind(sockfd_, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
        perror("freedom bind");
        close(sockfd_);
        sockfd_ = -1;
        return false;
    }

    std::cout << "Freedom receiver listening on [::]:" << port_ << std::endl;
    return true;
}

std::optional<FreedomPacket> FreedomReceiver::receive() {
    if (sockfd_ < 0) {
        return std::nullopt;
    }

    char buffer[kBufferSize] {};
    sockaddr_in6 sender {};
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
        perror("freedom recvfrom");
        return std::nullopt;
    }

    buffer[n] = '\0';
    const std::string payload(buffer);
    const std::string source = sender_address(sender);

    std::map<std::string, std::string> fields;
    std::string error;
    if (!parse_flat_json(payload, fields, error)) {
        std::cerr << "Bad Freedom JSON from " << source
                  << ": " << error << " payload=" << payload << std::endl;
        return std::nullopt;
    }

    auto node = parse_sensor_packet(fields);
    if (!node.has_value()) {
        std::cerr << "Ignore non-sensor Freedom packet from " << source
                  << ": " << payload << std::endl;
        return std::nullopt;
    }

    node->last_seen = std::chrono::steady_clock::now();
    node->online = true;
    return FreedomPacket{*node, payload, source};
}

}  // namespace gateway
