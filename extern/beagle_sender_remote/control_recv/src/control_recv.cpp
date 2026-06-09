/*** 
 * @Author: LYK && 2586356361@qq.com
 * @Date: 2026-05-31 14:51:00
 * @LastEditors: LYK && 2586356361@qq.com
 * @LastEditTime: 2026-05-31 14:52:09
 * @FilePath: /beagle_play/extern/beagle_sender_remote/control_recv/src/control_recv.cpp
 * @Description: 板子端的控制状态接收程序
 * @
 * @Copyright (c) 2026  All Rights Reserved. 
 */
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cerrno>
#include <cctype>
#include <cstring>
#include <iostream>
#include <map>
#include <string>

namespace {

    // * 开放9001端口
constexpr int kDefaultListenPort = 9001;

std::string trim(std::string text)
{
    auto not_space = [](unsigned char ch) { return !std::isspace(ch); };

    while (!text.empty() && !not_space(static_cast<unsigned char>(text.front()))) {
        text.erase(text.begin());
    }
    while (!text.empty() && !not_space(static_cast<unsigned char>(text.back()))) {
        text.pop_back();
    }

    return text;
}

bool parseFlatJsonObject(const std::string& text, std::map<std::string, std::string>& out)
{
    std::size_t pos = 0;
    auto skip_ws = [&]() {
        while (pos < text.size() && std::isspace(static_cast<unsigned char>(text[pos]))) {
            ++pos;
        }
    };
    auto read_string = [&](std::string& value) {
        if (pos >= text.size() || text[pos] != '"') {
            return false;
        }
        const std::size_t start = ++pos;
        const std::size_t end = text.find('"', start);
        if (end == std::string::npos) {
            return false;
        }
        value = text.substr(start, end - start);
        pos = end + 1;
        return true;
    };
    auto read_value = [&](std::string& value) {
        skip_ws();
        if (pos < text.size() && text[pos] == '"') {
            return read_string(value);
        }
        const std::size_t start = pos;
        while (pos < text.size() && text[pos] != ',' && text[pos] != '}') {
            ++pos;
        }
        value = trim(text.substr(start, pos - start));
        return !value.empty();
    };

    skip_ws();
    if (pos >= text.size() || text[pos++] != '{') {
        return false;
    }

    while (true) {
        skip_ws();
        if (pos < text.size() && text[pos] == '}') {
            return true;
        }

        std::string key;
        std::string value;
        if (!read_string(key)) {
            return false;
        }
        skip_ws();
        if (pos >= text.size() || text[pos++] != ':') {
            return false;
        }
        if (!read_value(value)) {
            return false;
        }
        out[key] = value;

        skip_ws();
        if (pos < text.size() && text[pos] == ',') {
            ++pos;
            continue;
        }
        if (pos < text.size() && text[pos] == '}') {
            return true;
        }
        return false;
    }
}

std::string onOff(const std::map<std::string, std::string>& data, const std::string& key)
{
    const auto it = data.find(key);
    if (it == data.end()) {
        return "unknown";
    }

    return it->second == "true" ? "on" : "off";
}

int parsePort(int argc, char** argv)
{
    if (argc < 2) {
        return kDefaultListenPort;
    }

    try {
        return std::stoi(argv[1]);
    } catch (...) {
        return kDefaultListenPort;
    }
}

}  // namespace

int main(int argc, char** argv)
{
    const int listen_port = parsePort(argc, argv);
    const int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("socket");
        return 1;
    }

    int reuse = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(listen_port);

    if (bind(sockfd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
        perror("bind");
        close(sockfd);
        return 1;
    }

    std::cout << "BeaglePlay control receiver listening on UDP " << listen_port << std::endl;

    while (true) {
        char buffer[2048];
        sockaddr_in sender{};
        socklen_t sender_len = sizeof(sender);

        const ssize_t n = recvfrom(sockfd, buffer, sizeof(buffer) - 1, 0,
                                   reinterpret_cast<sockaddr*>(&sender), &sender_len);
        if (n < 0) {
            if (errno == EINTR) {
                continue;
            }
            perror("recvfrom");
            continue;
        }

        buffer[n] = '\0';
        const std::string payload(buffer);

        char sender_ip[INET_ADDRSTRLEN]{};
        inet_ntop(AF_INET, &sender.sin_addr, sender_ip, sizeof(sender_ip));

        std::map<std::string, std::string> data;
        if (!parseFlatJsonObject(payload, data)) {
            std::cout << "invalid control packet from " << sender_ip << ": " << payload << std::endl;
            continue;
        }

        std::cout << "control_state"
                  << " from=" << sender_ip
                  << " seq=" << data["seq"]
                  << " running=" << data["running"]
                  << " mode=" << data["mode"]
                  << " light=" << onOff(data, "light")
                  << " pump=" << onOff(data, "pump")
                  << " fan=" << onOff(data, "fan")
                  << std::endl;
    }

    close(sockfd);
    return 0;
}
