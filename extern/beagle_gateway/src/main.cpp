#include <actuators/fan_controller.hpp>
#include <actuators/voice_player.hpp>
#include <common/node_state.hpp>
#include <control/control_policy.hpp>
#include <freedom_recv/freedom_receiver.hpp>
#include <host_recv/host_receiver.hpp>
#include <host_send/host_sender.hpp>

#include <iostream>
#include <string>
#include <unordered_map>

#include <arpa/inet.h>
#include <cstring>
#include <net/if.h>
#include <sys/socket.h>
#include <unistd.h>

namespace {

constexpr int kFreedomControlPort = 10000;

int open_freedom_control_socket() {
    int sock = socket(AF_INET6, SOCK_DGRAM, 0);
    if (sock < 0) {
        perror("freedom control socket");
        return -1;
    }

    const unsigned int ifindex = if_nametoindex("lowpan0");
    if (ifindex != 0) {
        setsockopt(sock, IPPROTO_IPV6, IPV6_MULTICAST_IF, &ifindex, sizeof(ifindex));
    }

    return sock;
}

void send_freedom_light_command(int sock, const std::string& node, bool light_on) {
    if (sock < 0) {
        return;
    }

    sockaddr_in6 addr {};
    addr.sin6_family = AF_INET6;
    addr.sin6_port = htons(kFreedomControlPort);
    addr.sin6_scope_id = if_nametoindex("lowpan0");

    if (inet_pton(AF_INET6, "ff02::1", &addr.sin6_addr) != 1) {
        return;
    }

    const std::string payload = std::string("{\"node\":\"") + node +
                                "\",\"light_on\":" +
                                (light_on ? "true" : "false") + "}";

    const ssize_t sent = sendto(sock,
                                payload.c_str(),
                                payload.size(),
                                0,
                                reinterpret_cast<const sockaddr*>(&addr),
                                sizeof(addr));
    if (sent < 0) {
        perror("send freedom light");
    } else {
        std::cout << "TX Freedom " << node
                  << " light_on=" << (light_on ? "true" : "false") << std::endl;
    }
}

}  // namespace

int main() {
    gateway::FreedomReceiver freedom_receiver(9999);
    gateway::HostSender host_sender("192.168.7.1", 9000);
    gateway::HostReceiver host_receiver(9001);
    gateway::ControlPolicy control_policy;
    FanController fan;
    VoicePlayer voice;
    int freedom_control_sock = open_freedom_control_socket();

    std::unordered_map<std::string, gateway::NodeState> nodes;

    if (!fan.init()) {
        std::cerr << "Fan init failed, gateway will keep receiving UDP" << std::endl;
    }
    voice.play_event("system_start");

    if (!freedom_receiver.open()) {
        return 1;
    }

    if (!host_sender.open()) {
        std::cerr << "Host sender init failed, Freedom receive still works" << std::endl;
    }

    if (!host_receiver.open()) {
        std::cerr << "Host control receiver init failed, local policy still works" << std::endl;
    }

    std::cout << "Beagle gateway started" << std::endl;

    while (true) {
        const auto freedom_packet = freedom_receiver.receive();
        if (freedom_packet.has_value()) {
            const gateway::NodeState& incoming = freedom_packet->node;
            gateway::NodeState& state = nodes[incoming.node];
            const bool was_online = state.online;
            state = incoming;

            if (!was_online) {
                std::cout << "Node online: " << state.node << std::endl;
            }

            std::cout << "RX Freedom " << state.node
                      << " from " << freedom_packet->source
                      << " temp=" << state.temperature
                      << " hum=" << state.humidity
                      << " light=" << state.light
                      << " rssi=" << state.rssi
                      << " seq=" << state.seq << std::endl;

            host_sender.send_node(state);
        }

        while (true) {
            const auto command = host_receiver.receive();
            if (!command.has_value()) {
                break;
            }
            if (!command->node_lights.empty()) {
                for (const auto& [node, light_on] : command->node_lights) {
                    send_freedom_light_command(freedom_control_sock, node, light_on);
                }
            } else if (command->has_light_on) {
                send_freedom_light_command(freedom_control_sock, "F1", command->light_on);
                send_freedom_light_command(freedom_control_sock, "F2", command->light_on);
            }
            control_policy.apply_host_command(*command, fan, voice);
        }

        control_policy.mark_offline_nodes(nodes);
        control_policy.apply_temperature_policy(nodes, fan, voice);
    }

    return 0;
}
