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

int main() {
    gateway::FreedomReceiver freedom_receiver(9999);
    gateway::HostSender host_sender("192.168.7.1", 9000);
    gateway::HostReceiver host_receiver(9001);
    gateway::ControlPolicy control_policy;
    FanController fan;
    VoicePlayer voice;

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
            control_policy.apply_host_command(*command, fan, voice);
        }

        control_policy.mark_offline_nodes(nodes);
        control_policy.apply_temperature_policy(nodes, fan, voice);
    }

    return 0;
}
