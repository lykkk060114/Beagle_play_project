/*** 
 * @Author: LYK && 2586356361@qq.com
 * @Date: 2026-05-24 20:48:05
 * @LastEditors: LYK && 2586356361@qq.com
 * @LastEditTime: 2026-05-24 22:07:50
 * @FilePath: /beagle_play/src/dashboard_server/dashboard_server.cpp
 * @Description: 
 * @
 * @Copyright (c) 2026  All Rights Reserved. 
 */


#include <dashboard_server/dashboard_server.hpp>

namespace dashboard {

std::atomic<bool> g_stop_requested{false};

void handleSignal(int) {
    g_stop_requested.store(true);
}

bool DashboardServer::start() {
        index_html_ = readFile(std::string(DASHBOARD_SOURCE_DIR) + "/web/index.html");
        if (index_html_.empty()) {
            index_html_ = "<!doctype html><html><body><pre>static/index.html not found</pre></body></html>";
        }

        if (!openUdpSocket()) {
            return false;
        }
        if (!openHttpSocket()) {
            close(udp_sock_);
            udp_sock_ = -1;
            return false;
        }
        return true;
    }

void DashboardServer::run() {
        std::thread udp_thread(&DashboardServer::udpLoop, this);
        std::thread http_thread(&DashboardServer::httpLoop, this);

        udp_thread.join();
        http_thread.join();

        if (udp_sock_ >= 0) {
            close(udp_sock_);
            udp_sock_ = -1;
        }
        if (http_sock_ >= 0) {
            close(http_sock_);
            http_sock_ = -1;
        }
    }

bool DashboardServer::openUdpSocket() {
        udp_sock_ = socket(AF_INET, SOCK_DGRAM, 0);
        if (udp_sock_ < 0) {
            perror("socket(udp)");
            return false;
        }

        int reuse = 1;
        setsockopt(udp_sock_, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

        timeval tv{};
        tv.tv_sec = 1;
        tv.tv_usec = 0;
        setsockopt(udp_sock_, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = INADDR_ANY;
        addr.sin_port = htons(kUdpPort);

        if (bind(udp_sock_, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
            perror("bind(udp)");
            return false;
        }
        return true;
    }

bool DashboardServer::openHttpSocket() {
        http_sock_ = socket(AF_INET, SOCK_STREAM, 0);
        if (http_sock_ < 0) {
            perror("socket(http)");
            return false;
        }

        int reuse = 1;
        setsockopt(http_sock_, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = INADDR_ANY;
        addr.sin_port = htons(kHttpPort);

        if (bind(http_sock_, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
            perror("bind(http)");
            return false;
        }
        if (listen(http_sock_, 16) < 0) {
            perror("listen(http)");
            return false;
        }
        return true;
    }

void DashboardServer::appendEventLocked(const std::string& level, const std::string& text) {
        state_.events.push_back(EventEntry{level, text});
        while (state_.events.size() > kMaxEvents) {
            state_.events.pop_front();
        }
        state_.last_update_ms = nowMs();
    }

void DashboardServer::setAllActuatorsOffLocked() {
        state_.actuators.light = false;
        state_.actuators.pump = false;
        state_.actuators.fan = false;
    }

void DashboardServer::refreshNodeOnlineLocked() {
        const long long now = nowMs();
        for (auto& [name, node] : state_.nodes) {
            const bool online = node.last_seen_ms > 0 &&
                                (now - node.last_seen_ms) <=
                                    std::chrono::duration_cast<std::chrono::milliseconds>(kNodeOfflineTimeout).count();
            node.online = online;
        }
    }

void DashboardServer::applyAutomaticControlLocked() {
        if (!state_.running || state_.mode != "auto") {
            return;
        }

        bool has_online_node = false;
        bool any_light_below = false;
        bool all_light_above = true;
        bool any_temp_high = false;
        bool all_temp_cool = true;

        for (auto& [name, node] : state_.nodes) {
            if (!node.online) {
                continue;
            }
            has_online_node = true;

            if (node.light < state_.config.light_low) {
                any_light_below = true;
            }
            if (!(node.light > state_.config.light_high)) {
                all_light_above = false;
            }
            if (node.temperature > state_.config.temperature_high) {
                any_temp_high = true;
            }
            if (!(node.temperature <= state_.config.temperature_high - 2.0)) {
                all_temp_cool = false;
            }
        }

        if (!has_online_node) {
            return;
        }

        if (any_light_below && !state_.actuators.light) {
            state_.actuators.light = true;
            appendEventLocked("ok", "Auto light turned on");
        } else if (all_light_above && state_.actuators.light) {
            state_.actuators.light = false;
            appendEventLocked("ok", "Auto light turned off");
        }

        if (any_temp_high && !state_.actuators.fan) {
            state_.actuators.fan = true;
            appendEventLocked("ok", "Auto fan turned on");
        } else if (all_temp_cool && state_.actuators.fan) {
            state_.actuators.fan = false;
            appendEventLocked("ok", "Auto fan turned off");
        }

    }

std::string DashboardServer::buildStatusJsonLocked() {
        refreshNodeOnlineLocked();
        applyAutomaticControlLocked();

        std::ostringstream oss;
        oss << "{";
        oss << "\"running\":" << jsonBool(state_.running) << ",";
        oss << "\"mode\":\"" << jsonEscape(state_.mode) << "\",";
        oss << "\"gateway\":\"" << jsonEscape(state_.gateway) << "\",";
        oss << "\"last_update_ms\":" << jsonNumber(state_.last_update_ms) << ",";

        oss << "\"nodes\":{";
        bool first_node = true;
        for (const auto& [name, node] : state_.nodes) {
            if (!first_node) {
                oss << ",";
            }
            first_node = false;
            oss << "\"" << jsonEscape(name) << "\":{";
            oss << "\"online\":" << jsonBool(node.online) << ",";
            oss << "\"temperature\":" << jsonNumber(node.temperature) << ",";
            oss << "\"humidity\":" << jsonNumber(node.humidity) << ",";
            oss << "\"light\":" << jsonNumber(node.light) << ",";
            oss << "\"rssi\":" << node.rssi << ",";
            oss << "\"last_seen_ms\":" << jsonNumber(node.last_seen_ms);
            oss << "}";
        }
        oss << "},";

        oss << "\"actuators\":{";
        oss << "\"light\":" << jsonBool(state_.actuators.light) << ",";
        oss << "\"pump\":" << jsonBool(state_.actuators.pump) << ",";
        oss << "\"fan\":" << jsonBool(state_.actuators.fan);
        oss << "},";

        oss << "\"config\":{";
        oss << "\"light_low\":" << jsonNumber(state_.config.light_low) << ",";
        oss << "\"light_high\":" << jsonNumber(state_.config.light_high) << ",";
        oss << "\"humidity_low\":" << jsonNumber(state_.config.humidity_low) << ",";
        oss << "\"temperature_high\":" << jsonNumber(state_.config.temperature_high) << ",";
        oss << "\"pump_duration_sec\":" << state_.config.pump_duration_sec << ",";
        oss << "\"pump_cooldown_sec\":" << state_.config.pump_cooldown_sec;
        oss << "},";

        oss << "\"events\":[";
        bool first_event = true;
        for (const auto& event : state_.events) {
            if (!first_event) {
                oss << ",";
            }
            first_event = false;
            oss << "{";
            oss << "\"level\":\"" << jsonEscape(event.level) << "\",";
            oss << "\"text\":\"" << jsonEscape(event.text) << "\"";
            oss << "}";
        }
        oss << "]";
        oss << "}";
        return oss.str();
    }

std::string DashboardServer::buildErrorJson(int code, const std::string& message) {
        std::ostringstream oss;
        oss << "{";
        oss << "\"ok\":false,";
        oss << "\"error\":\"" << jsonEscape(message) << "\",";
        oss << "\"code\":" << code;
        oss << "}";
        return oss.str();
    }

HttpReply DashboardServer::handleRequest(const HttpRequest& request) {
        if (request.method == "GET" && request.path == "/") {
            return {200, "text/html; charset=utf-8", index_html_};
        }

        if (request.method == "GET" && request.path == "/api/status") {
            std::lock_guard<std::mutex> lock(state_mutex_);
            return {200, "application/json; charset=utf-8", buildStatusJsonLocked()};
        }

        if (request.method == "POST" && request.path == "/api/system/start") {
            std::lock_guard<std::mutex> lock(state_mutex_);
            state_.running = true;
            state_.gateway = "online";
            appendEventLocked("ok", "System started");
            applyAutomaticControlLocked();
            return {200, "application/json; charset=utf-8", buildStatusJsonLocked()};
        }

        if (request.method == "POST" && request.path == "/api/system/stop") {
            std::lock_guard<std::mutex> lock(state_mutex_);
            state_.running = false;
            setAllActuatorsOffLocked();
            appendEventLocked("ok", "System stopped");
            return {200, "application/json; charset=utf-8", buildStatusJsonLocked()};
        }

        if (request.method == "POST" && request.path == "/api/system/restart_gateway") {
            std::lock_guard<std::mutex> lock(state_mutex_);
            appendEventLocked("ok", "Gateway restart requested");
            return {200, "application/json; charset=utf-8", buildStatusJsonLocked()};
        }

        if (request.method == "POST" && request.path == "/api/mode") {
            std::map<std::string, std::string> body;
            std::string error;
            if (!parseFlatJsonObject(request.body, body, error)) {
                return {400, "application/json; charset=utf-8", buildErrorJson(400, "invalid json: " + error)};
            }

            const auto it = body.find("mode");
            if (it == body.end()) {
                return {400, "application/json; charset=utf-8", buildErrorJson(400, "missing mode")};
            }
            const std::string mode = trim(it->second);
            if (mode != "auto" && mode != "manual" && mode != "safe") {
                return {400, "application/json; charset=utf-8", buildErrorJson(400, "mode must be auto/manual/safe")};
            }

            std::lock_guard<std::mutex> lock(state_mutex_);
            state_.mode = mode;
            if (state_.mode == "safe") {
                setAllActuatorsOffLocked();
            }
            appendEventLocked("ok", "Mode changed to " + state_.mode);
            applyAutomaticControlLocked();
            return {200, "application/json; charset=utf-8", buildStatusJsonLocked()};
        }

        if (request.method == "POST" && request.path == "/api/config") {
            std::map<std::string, std::string> body;
            std::string error;
            if (!parseFlatJsonObject(request.body, body, error)) {
                return {400, "application/json; charset=utf-8", buildErrorJson(400, "invalid json: " + error)};
            }

            ConfigState next = {};
            if (!parseDoubleToken(body["light_low"], next.light_low) ||
                !parseDoubleToken(body["light_high"], next.light_high) ||
                !parseDoubleToken(body["humidity_low"], next.humidity_low) ||
                !parseDoubleToken(body["temperature_high"], next.temperature_high) ||
                !parseIntToken(body["pump_duration_sec"], next.pump_duration_sec) ||
                !parseIntToken(body["pump_cooldown_sec"], next.pump_cooldown_sec)) {
                return {400, "application/json; charset=utf-8", buildErrorJson(400, "invalid config values")};
            }
            std::lock_guard<std::mutex> lock(state_mutex_);
            state_.config = next;
            appendEventLocked("ok", "Config updated");
            applyAutomaticControlLocked();
            return {200, "application/json; charset=utf-8", buildStatusJsonLocked()};
        }

        if (request.method == "POST" && request.path == "/api/control") {
            std::map<std::string, std::string> body;
            std::string error;
            if (!parseFlatJsonObject(request.body, body, error)) {
                return {400, "application/json; charset=utf-8", buildErrorJson(400, "invalid json: " + error)};
            }

            const auto device_it = body.find("device");
            const auto action_it = body.find("action");
            if (device_it == body.end() || action_it == body.end()) {
                return {400, "application/json; charset=utf-8", buildErrorJson(400, "missing device or action")};
            }

            const std::string device = trim(device_it->second);
            const std::string action = trim(action_it->second);
            if (device != "light" && device != "pump" && device != "fan") {
                return {400, "application/json; charset=utf-8", buildErrorJson(400, "invalid device")};
            }
            if (action != "on" && action != "off" && action != "once") {
                return {400, "application/json; charset=utf-8", buildErrorJson(400, "invalid action")};
            }

            std::lock_guard<std::mutex> lock(state_mutex_);
            if (state_.mode == "safe") {
                return {403, "application/json; charset=utf-8", buildErrorJson(403, "control rejected in safe mode")};
            }

            if (device == "light") {
                if (action == "once") {
                    return {400, "application/json; charset=utf-8", buildErrorJson(400, "light does not support once")};
                }
                state_.actuators.light = (action == "on");
                appendEventLocked("ok", std::string("Light turned ") + (state_.actuators.light ? "on" : "off"));
            } else if (device == "fan") {
                if (action == "once") {
                    return {400, "application/json; charset=utf-8", buildErrorJson(400, "fan does not support once")};
                }
                state_.actuators.fan = (action == "on");
                appendEventLocked("ok", std::string("Fan turned ") + (state_.actuators.fan ? "on" : "off"));
            } else if (device == "pump") {
                if (action == "once") {
                    state_.actuators.pump = true;
                    appendEventLocked("ok", "Pump triggered once");
                    const int duration_sec = std::max(0, state_.config.pump_duration_sec);
                    std::thread([this, duration_sec] {
                        std::this_thread::sleep_for(std::chrono::seconds(duration_sec));
                        std::lock_guard<std::mutex> lock(state_mutex_);
                        state_.actuators.pump = false;
                        state_.last_update_ms = nowMs();
                    }).detach();
                } else {
                    state_.actuators.pump = (action == "on");
                    appendEventLocked("ok", std::string("Pump turned ") + (state_.actuators.pump ? "on" : "off"));
                }
            }
            return {200, "application/json; charset=utf-8", buildStatusJsonLocked()};
        }

        return {404, "application/json; charset=utf-8", buildErrorJson(404, "not found")};
    }

void DashboardServer::handleClient(int client_fd) {
        HttpRequest request;
        if (!readHttpRequest(client_fd, request)) {
            close(client_fd);
            return;
        }

        if (request.method != "GET" && request.method != "POST") {
            sendResponse(client_fd, 405, "application/json; charset=utf-8",
                         buildErrorJson(405, "method not allowed"));
            close(client_fd);
            return;
        }

        const HttpReply reply = handleRequest(request);
        sendResponse(client_fd, reply.code, reply.content_type, reply.body);
        close(client_fd);
    }

void DashboardServer::udpLoop() {
        char buffer[2048];

        while (!g_stop_requested.load()) {
            sockaddr_in sender{};
            socklen_t sender_len = sizeof(sender);
            const ssize_t n = recvfrom(udp_sock_, buffer, sizeof(buffer) - 1, 0,
                                         reinterpret_cast<sockaddr*>(&sender), &sender_len);
            if (n < 0) {
                if (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK) {
                    continue;
                }
            perror("recvfrom");
                continue;
            }

            buffer[n] = '\0';
            std::string payload(buffer);

            std::map<std::string, std::string> data;
            std::string parse_error;
            if (!parseFlatJsonObject(payload, data, parse_error)) {
                std::lock_guard<std::mutex> lock(state_mutex_);
                state_.gateway = "online";
                appendEventLocked("warn", "UDP JSON parse error");
                continue;
            }

            const auto node_it = data.find("node");
            if (node_it == data.end()) {
                std::lock_guard<std::mutex> lock(state_mutex_);
                state_.gateway = "online";
                appendEventLocked("warn", "UDP packet missing node");
                continue;
            }

            NodeState snapshot;
            snapshot.online = true;
            snapshot.last_seen_ms = nowMs();

            if (!parseDoubleToken(data["temperature"], snapshot.temperature) ||
                !parseDoubleToken(data["humidity"], snapshot.humidity) ||
                !parseDoubleToken(data["light"], snapshot.light) ||
                !parseIntToken(data["rssi"], snapshot.rssi)) {
                std::lock_guard<std::mutex> lock(state_mutex_);
                state_.gateway = "online";
                appendEventLocked("warn", "UDP packet has invalid sensor values");
                continue;
            }

            const std::string node_name = trim(node_it->second);
            {
                std::lock_guard<std::mutex> lock(state_mutex_);
                state_.gateway = "online";
                state_.nodes[node_name].online = true;
                state_.nodes[node_name].temperature = snapshot.temperature;
                state_.nodes[node_name].humidity = snapshot.humidity;
                state_.nodes[node_name].light = snapshot.light;
                state_.nodes[node_name].rssi = snapshot.rssi;
                state_.nodes[node_name].last_seen_ms = snapshot.last_seen_ms;
                state_.last_update_ms = nowMs();
                applyAutomaticControlLocked();
            }
        }
    }

void DashboardServer::httpLoop() {
        while (!g_stop_requested.load()) {
            fd_set set;
            FD_ZERO(&set);
            FD_SET(http_sock_, &set);
            timeval tv{};
            tv.tv_sec = 1;
            tv.tv_usec = 0;

            const int ready = select(http_sock_ + 1, &set, nullptr, nullptr, &tv);
            if (ready < 0) {
                if (errno == EINTR) {
                    continue;
                }
            perror("select");
                continue;
            }
            if (ready == 0) {
                continue;
            }

            sockaddr_in client_addr{};
            socklen_t client_len = sizeof(client_addr);
            const int client_fd = accept(http_sock_, reinterpret_cast<sockaddr*>(&client_addr), &client_len);
            if (client_fd < 0) {
                if (errno == EINTR) {
                    continue;
                }
                perror("accept");
                continue;
            }

            std::thread(&DashboardServer::handleClient, this, client_fd).detach();
        }
    }

}  // namespace dashboard
