#pragma once

#include <common/node_state.hpp>

#include <optional>
#include <string>

namespace gateway {

struct FreedomPacket {
    NodeState node;
    std::string payload;
    std::string source;
};

class FreedomReceiver {
public:
    explicit FreedomReceiver(int port = 9999);
    ~FreedomReceiver();

    bool open();
    std::optional<FreedomPacket> receive();

private:
    int port_;
    int sockfd_;
};

}  // namespace gateway
