/*** 
 * @Author: LYK && 2586356361@qq.com
 * @Date: 2026-05-24 20:48:05
 * @LastEditors: LYK && 2586356361@qq.com
 * @LastEditTime: 2026-05-25 11:20:03
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
        vue_js_ = readFile(std::string(DASHBOARD_SOURCE_DIR) + "/web/vue.global.prod.js");

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
    // 开两个循环线程
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

// 打开端口并监听UDP数据包
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

void DashboardServer::setFanLocked(bool enabled) {
        state_.actuators.fan = enabled;
        if (!enabled) {
            state_.actuators.fan_pwm_percent = 0;
            return;
        }
        state_.actuators.fan_pwm_percent =
            state_.mode == "auto" ? kAutoFanPwmPercent : state_.actuators.manual_fan_pwm_percent;
    }

void DashboardServer::setManualFanPwmLocked(int percent) {
        state_.actuators.manual_fan_pwm_percent = std::clamp(percent, 0, 100);
        if (state_.actuators.fan && state_.mode == "manual") {
            setFanLocked(true);
        }
    }

void DashboardServer::setAllActuatorsOffLocked() {
        state_.actuators.light = false;
        state_.actuators.pump = false;
        setFanLocked(false);
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

void DashboardServer::refreshGatewayOnlineLocked() {
        const long long now = nowMs();
        const bool online = state_.last_gateway_seen_ms > 0 &&
                            (now - state_.last_gateway_seen_ms) <=
                                std::chrono::duration_cast<std::chrono::milliseconds>(
                                    kGatewayOfflineTimeout).count();
        state_.gateway = online ? "online" : "offline";
        if (!online) {
            has_gateway_addr_ = false;
        }
    }

void DashboardServer::refreshNodeControlsLocked() {
        state_.node_controls.clear();

        for (const auto& [name, node] : state_.nodes) {
            NodeControlState control;
            control.online = node.online;

            if (node.online) {
                control.light_on = node.light < state_.config.light_low;
                control.pump_on = node.humidity < state_.config.humidity_low;
                control.fan_on = node.temperature > state_.config.temperature_high;
                control.fan_pwm_percent = control.fan_on ? kAutoFanPwmPercent : 0;
            }

            state_.node_controls[name] = control;
        }
    }

void DashboardServer::triggerPumpOnceLocked(const std::string& event_text) {
        state_.actuators.pump = true;
        appendEventLocked("ok", event_text);
        const int duration_sec = std::max(0, state_.config.pump_duration_sec);
        std::thread([this, duration_sec] {
            std::this_thread::sleep_for(std::chrono::seconds(duration_sec));
            std::lock_guard<std::mutex> lock(state_mutex_);
            state_.actuators.pump = false;
            state_.last_update_ms = nowMs();
            sendControlStateLocked();
        }).detach();
    }

bool DashboardServer::applyAutomaticControlLocked() {
        refreshNodeControlsLocked();

        if (!state_.running || state_.mode != "auto") {
            return false;
        }

        bool changed = false;
        bool has_online_node = false;
        bool any_light_on = false;
        bool all_light_above = true;
        bool any_pump_on = false;
        bool any_fan_on = false;

        for (const auto& [name, node] : state_.nodes) {
            if (!node.online) {
                continue;
            }
            has_online_node = true;
            const NodeControlState& control = state_.node_controls[name];
            any_light_on = any_light_on || control.light_on;
            any_pump_on = any_pump_on || control.pump_on;
            any_fan_on = any_fan_on || control.fan_on;
            if (!(node.light >= state_.config.light_low)) {
                all_light_above = false;
            }
        }

        if (!has_online_node) {
            if (state_.actuators.fan) {
                setFanLocked(false);
                appendEventLocked("ok", "Auto fan turned off");
                changed = true;
            }
            return changed;
        }

        if (any_light_on && !state_.actuators.light) {
            state_.actuators.light = true;
            appendEventLocked("ok", "Auto light turned on");
            changed = true;
        } else if (all_light_above && state_.actuators.light) {
            state_.actuators.light = false;
            appendEventLocked("ok", "Auto light turned off");
            changed = true;
        }

        if (any_fan_on && !state_.actuators.fan) {
            setFanLocked(true);
            appendEventLocked("ok", "Auto fan turned on");
            changed = true;
        } else if (!any_fan_on && state_.actuators.fan) {
            setFanLocked(false);
            appendEventLocked("ok", "Auto fan turned off");
            changed = true;
        } else if (state_.actuators.fan) {
            const int next_pwm = kAutoFanPwmPercent;
            if (state_.actuators.fan_pwm_percent != next_pwm) {
                state_.actuators.fan_pwm_percent = next_pwm;
                changed = true;
            }
        }

        const long long now = nowMs();
        const long long cooldown_ms =
            static_cast<long long>(std::max(0, state_.config.pump_cooldown_sec)) * 1000LL;
        const bool pump_ready = last_auto_pump_ms_ == 0 ||
                                (now - last_auto_pump_ms_) >= cooldown_ms;
        if (any_pump_on && !state_.actuators.pump && pump_ready) {
            last_auto_pump_ms_ = now;
            triggerPumpOnceLocked("Auto pump triggered once");
            changed = true;
        }

        return changed;
    }

void DashboardServer::rememberGatewayAddressLocked(const sockaddr_in& sender) {
        last_gateway_addr_ = sender;
        last_gateway_addr_.sin_port = htons(kGatewayControlPort);
        has_gateway_addr_ = true;
        state_.last_gateway_seen_ms = nowMs();
        state_.gateway = "online";
    }

std::string DashboardServer::buildControlJsonLocked() {
        refreshNodeControlsLocked();
        const auto node_light_on = [this](const std::string& node_name) {
            if (!state_.running || state_.mode == "safe") {
                return false;
            }
            if (state_.mode == "manual") {
                return state_.actuators.light;
            }

            const auto it = state_.node_controls.find(node_name);
            return it != state_.node_controls.end() && it->second.online && it->second.light_on;
        };

        std::ostringstream oss;
        oss << "{";
        oss << "\"type\":\"control_state\",";
        oss << "\"seq\":" << ++control_seq_ << ",";
        oss << "\"running\":" << jsonBool(state_.running) << ",";
        oss << "\"mode\":\"" << jsonEscape(state_.mode) << "\",";
        oss << "\"light\":" << jsonBool(state_.actuators.light) << ",";
        oss << "\"pump\":" << jsonBool(state_.actuators.pump) << ",";
        oss << "\"fan\":" << jsonBool(state_.actuators.fan) << ",";
        oss << "\"pwm\":" << state_.actuators.fan_pwm_percent << ",";
        oss << "\"fan_on\":" << jsonBool(state_.actuators.fan) << ",";
        oss << "\"fan_pwm\":" << state_.actuators.fan_pwm_percent << ",";
        oss << "\"fan_auto\":" << jsonBool(state_.running && state_.mode == "auto") << ",";
        oss << "\"temperature_high\":" << jsonNumber(state_.config.temperature_high) << ",";
        oss << "\"light_F1\":" << jsonBool(node_light_on("F1")) << ",";
        oss << "\"light_F2\":" << jsonBool(node_light_on("F2"));
        oss << "}";
        return oss.str();
    }

void DashboardServer::sendControlStateLocked() {
        refreshGatewayOnlineLocked();
        if (!has_gateway_addr_ || udp_sock_ < 0) {
            return;
        }

        const std::string payload = buildControlJsonLocked();
        const ssize_t sent = sendto(udp_sock_, payload.c_str(), payload.size(), 0,
                                    reinterpret_cast<const sockaddr*>(&last_gateway_addr_),
                                    sizeof(last_gateway_addr_));
        if (sent < 0) {
            perror("sendto(control)");
        }
    }

std::string DashboardServer::buildStatusJsonLocked() {
        refreshGatewayOnlineLocked();
        refreshNodeOnlineLocked();
        applyAutomaticControlLocked();
        sendControlStateLocked();

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

        oss << "\"node_controls\":{";
        bool first_control = true;
        for (const auto& [name, control] : state_.node_controls) {
            if (!first_control) {
                oss << ",";
            }
            first_control = false;
            oss << "\"" << jsonEscape(name) << "\":{";
            oss << "\"online\":" << jsonBool(control.online) << ",";
            oss << "\"light_on\":" << jsonBool(control.light_on) << ",";
            oss << "\"pump_on\":" << jsonBool(control.pump_on) << ",";
            oss << "\"fan_on\":" << jsonBool(control.fan_on) << ",";
            oss << "\"fan_pwm_percent\":" << control.fan_pwm_percent;
            oss << "}";
        }
        oss << "},";

        oss << "\"actuators\":{";
        oss << "\"light\":" << jsonBool(state_.actuators.light) << ",";
        oss << "\"pump\":" << jsonBool(state_.actuators.pump) << ",";
        oss << "\"fan\":" << jsonBool(state_.actuators.fan) << ",";
        oss << "\"fan_pwm_percent\":" << state_.actuators.fan_pwm_percent << ",";
        oss << "\"manual_fan_pwm_percent\":" << state_.actuators.manual_fan_pwm_percent << ",";
        oss << "\"auto_fan_pwm_percent\":" << kAutoFanPwmPercent;
        oss << "},";

        oss << "\"config\":{";
        oss << "\"light_low\":" << jsonNumber(state_.config.light_low) << ",";
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

        if (request.method == "GET" && request.path == "/vue.global.prod.js") {
            return {200, "application/javascript; charset=utf-8", vue_js_};
        }

        if (request.method == "GET" && request.path == "/api/status") {
            std::lock_guard<std::mutex> lock(state_mutex_);
            return {200, "application/json; charset=utf-8", buildStatusJsonLocked()};
        }

        // 当前端执行 开启 关闭 后端对应的系统状态就会改变
        if (request.method == "POST" && request.path == "/api/system/start") {
            std::lock_guard<std::mutex> lock(state_mutex_);
            state_.running = true;
            appendEventLocked("ok", "System started");
            applyAutomaticControlLocked();
            sendControlStateLocked();
            return {200, "application/json; charset=utf-8", buildStatusJsonLocked()};
        }

        if (request.method == "POST" && request.path == "/api/system/stop") {
            std::lock_guard<std::mutex> lock(state_mutex_);
            state_.running = false;
            setAllActuatorsOffLocked();
            appendEventLocked("ok", "System stopped");
            sendControlStateLocked();
            return {200, "application/json; charset=utf-8", buildStatusJsonLocked()};
        }

        if (request.method == "POST" && request.path == "/api/system/restart_gateway") {
            std::lock_guard<std::mutex> lock(state_mutex_);
            appendEventLocked("ok", "Gateway restart requested");
            return {200, "application/json; charset=utf-8", buildStatusJsonLocked()};
        }

        // 当前端切换模式时走这个窗口
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
            } else if (state_.actuators.fan) {
                setFanLocked(true);
            }
            appendEventLocked("ok", "Mode changed to " + state_.mode);
            applyAutomaticControlLocked();
            return {200, "application/json; charset=utf-8", buildStatusJsonLocked()};
        }


        // 当前端修改config的 阈值 时， 也会走这个接口， 这里会根据前端传来的JSON来更新config的值
        if (request.method == "POST" && request.path == "/api/config") {
            std::map<std::string, std::string> body;
            std::string error;
            if (!parseFlatJsonObject(request.body, body, error)) {
                return {400, "application/json; charset=utf-8", buildErrorJson(400, "invalid json: " + error)};
            }

            ConfigState next = {};
            if (!parseDoubleToken(body["light_low"], next.light_low) ||
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


        // 当前端发送控制指令， 对应的设备和动作就会执行
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
            if (action != "on" && action != "off" && action != "once" && action != "pwm") {
                return {400, "application/json; charset=utf-8", buildErrorJson(400, "invalid action")};
            }
            if (action == "pwm" && device != "fan") {
                return {400, "application/json; charset=utf-8", buildErrorJson(400, "pwm action only supports fan")};
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
                if (action == "pwm") {
                    if (state_.mode != "manual") {
                        return {403, "application/json; charset=utf-8",
                                buildErrorJson(403, "fan pwm can only be adjusted in manual mode")};
                    }
                    int pwm_percent = 0;
                    if (!parseIntToken(body["pwm"], pwm_percent)) {
                        return {400, "application/json; charset=utf-8", buildErrorJson(400, "invalid pwm value")};
                    }
                    setManualFanPwmLocked(pwm_percent);
                    appendEventLocked("ok", "Fan PWM set to " + std::to_string(state_.actuators.manual_fan_pwm_percent) + "%");
                } else {
                    setFanLocked(action == "on");
                    appendEventLocked("ok", std::string("Fan turned ") + (state_.actuators.fan ? "on" : "off"));
                }
            } else if (device == "pump") {
                if (action == "once") {
                    triggerPumpOnceLocked("Pump triggered once");
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
                rememberGatewayAddressLocked(sender);
                appendEventLocked("warn", "UDP JSON parse error");
                continue;
            }

            const auto node_it = data.find("node");
            if (node_it == data.end()) {
                std::lock_guard<std::mutex> lock(state_mutex_);
                rememberGatewayAddressLocked(sender);
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
                rememberGatewayAddressLocked(sender);
                appendEventLocked("warn", "UDP packet has invalid sensor values");
                continue;
            }

            const std::string node_name = trim(node_it->second);
            {
                std::lock_guard<std::mutex> lock(state_mutex_);
                rememberGatewayAddressLocked(sender);
                state_.nodes[node_name].online = true;
                state_.nodes[node_name].temperature = snapshot.temperature;
                state_.nodes[node_name].humidity = snapshot.humidity;
                state_.nodes[node_name].light = snapshot.light;
                state_.nodes[node_name].rssi = snapshot.rssi;
                state_.nodes[node_name].last_seen_ms = snapshot.last_seen_ms;
                state_.last_update_ms = nowMs();
                applyAutomaticControlLocked();
                sendControlStateLocked();
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
