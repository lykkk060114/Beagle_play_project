#include <host_send/host_sender.hpp>

#include <common/json_utils.hpp>

#include <arpa/inet.h>
#include <iostream>
#include <sstream>
#include <sys/socket.h>
#include <unistd.h>
#include <utility>

namespace gateway {
namespace {

std::string build_node_json(const NodeState& node) {
    std::ostringstream oss;
    oss << "{"
        << "\"node\":\"" << json_escape(node.node) << "\","
        << "\"light\":" << node.light << ","
        << "\"temperature\":" << node.temperature << ","
        << "\"humidity\":" << node.humidity << ","
        << "\"rssi\":" << node.rssi << ","
        << "\"seq\":" << node.seq
        << "}";
    return oss.str();
}

}  // namespace

HostSender::HostSender(std::string host_ip, int host_port)
    : host_ip_(std::move(host_ip)),
      host_port_(host_port),
      sockfd_(-1),
      addr_ok_(false),
      host_addr_{} {}

HostSender::~HostSender() {
    if (sockfd_ >= 0) {
        close(sockfd_);
    }
}

bool HostSender::open() {
    sockfd_ = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd_ < 0) {
        perror("host sender socket");
        return false;
    }

    host_addr_.sin_family = AF_INET;
    host_addr_.sin_port = htons(host_port_);

    if (inet_pton(AF_INET, host_ip_.c_str(), &host_addr_.sin_addr) <= 0) {
        perror("host sender inet_pton");
        close(sockfd_);
        sockfd_ = -1;
        return false;
    }

    addr_ok_ = true;
    std::cout << "Host sender target " << host_ip_ << ":" << host_port_ << std::endl;
    return true;
}

bool HostSender::send_payload(const std::string& payload) {
    if (sockfd_ < 0 || !addr_ok_) {
        return false;
    }

    const ssize_t sent = sendto(sockfd_,
                                payload.c_str(),
                                payload.size(),
                                0,
                                reinterpret_cast<sockaddr*>(&host_addr_),
                                sizeof(host_addr_));
    if (sent < 0) {
        perror("send host");
        return false;
    }
    return true;
}

bool HostSender::send_node(const NodeState& node) {
    return send_payload(build_node_json(node));
}

}  // namespace gateway
