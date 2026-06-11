#pragma once

#include <common/node_state.hpp>

#include <netinet/in.h>
#include <string>

namespace gateway {

class HostSender {
public:
    HostSender(std::string host_ip = "192.168.7.1", int host_port = 9000);
    ~HostSender();

    bool open();
    bool send_node(const NodeState& node);
    bool send_payload(const std::string& payload);

private:
    std::string host_ip_;
    int host_port_;
    int sockfd_;
    bool addr_ok_;
    sockaddr_in host_addr_;
};

}  // namespace gateway
