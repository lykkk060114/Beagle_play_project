#include <actuators/fan_controller.hpp>

#include <algorithm>
#include <chrono>
#include <fstream>
#include <iostream>
#include <dirent.h>
#include <limits.h>
#include <string>
#include <sys/stat.h>
#include <thread>
#include <unistd.h>
#include <utility>

namespace {

constexpr const char* kPwmClassDir = "/sys/class/pwm";
constexpr const char* kFanPwmDevice = "23120000.pwm";

std::string resolve_fan_pwm_chip(const std::string& fallback) {
    DIR* dir = opendir(kPwmClassDir);
    if (!dir) {
        return fallback;
    }

    std::string result = fallback;
    while (dirent* entry = readdir(dir)) {
        const std::string name = entry->d_name;
        if (name.rfind("pwmchip", 0) != 0) {
            continue;
        }

        const std::string chip = std::string(kPwmClassDir) + "/" + name;
        const std::string device_link = chip + "/device";
        char resolved[PATH_MAX] {};
        if (realpath(device_link.c_str(), resolved) && std::string(resolved).find(kFanPwmDevice) != std::string::npos) {
            result = chip;
            break;
        }
    }

    closedir(dir);
    return result;
}

}  // namespace

FanController::FanController(std::string pwm_chip, int channel, int period_ns)
    : pwm_chip_(resolve_fan_pwm_chip(pwm_chip)),
      pwm_path_(pwm_chip_ + "/pwm" + std::to_string(channel)),
      channel_(channel),
      period_ns_(period_ns),
      current_pwm_(0),
      initialized_(false),
      enabled_(false) {}

bool FanController::path_exists(const std::string& path) const {
    struct stat st {};
    return stat(path.c_str(), &st) == 0;
}

bool FanController::write_value(const std::string& path, const std::string& value) const {
    std::ofstream file(path);
    if (!file.is_open()) {
        std::cerr << "PWM write open failed: " << path << std::endl;
        return false;
    }

    file << value;
    if (!file.good()) {
        std::cerr << "PWM write failed: " << path << std::endl;
        return false;
    }

    return true;
}

bool FanController::export_pwm() {
    if (path_exists(pwm_path_)) {
        return true;
    }

    if (!write_value(pwm_chip_ + "/export", std::to_string(channel_))) {
        if (!path_exists(pwm_path_)) {
            return false;
        }
    }

    for (int i = 0; i < 20; ++i) {
        if (path_exists(pwm_path_)) {
            return true;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    std::cerr << "PWM export timeout: " << pwm_path_ << std::endl;
    return false;
}

bool FanController::init() {
    if (!export_pwm()) {
        std::cerr << "PWM init failed, need root or valid pwm path: " << pwm_path_ << std::endl;
        initialized_ = false;
        return false;
    }

    write_value(pwm_path_ + "/enable", "0");
    if (!write_value(pwm_path_ + "/period", std::to_string(period_ns_))) {
        initialized_ = false;
        return false;
    }
    if (!write_value(pwm_path_ + "/duty_cycle", "0")) {
        initialized_ = false;
        return false;
    }

    current_pwm_ = 0;
    enabled_ = false;
    initialized_ = true;
    return true;
}

bool FanController::set_pwm_percent(int percent) {
    if (!initialized_ && !init()) {
        return false;
    }

    const int pwm = std::clamp(percent, 0, 100);
    const int duty_ns = period_ns_ * pwm / 100;

    if (pwm == 0) {
        const bool duty_ok = write_value(pwm_path_ + "/duty_cycle", "0");
        const bool enable_ok = write_value(pwm_path_ + "/enable", "0");
        if (duty_ok && enable_ok) {
            current_pwm_ = 0;
            enabled_ = false;
        }
        return duty_ok && enable_ok;
    }

    write_value(pwm_path_ + "/enable", "0");
    if (!write_value(pwm_path_ + "/period", std::to_string(period_ns_))) {
        return false;
    }
    if (!write_value(pwm_path_ + "/duty_cycle", std::to_string(duty_ns))) {
        return false;
    }
    if (!write_value(pwm_path_ + "/enable", "1")) {
        return false;
    }

    current_pwm_ = pwm;
    enabled_ = true;
    return true;
}

bool FanController::on(int percent) {
    return set_pwm_percent(percent);
}

bool FanController::off() {
    return set_pwm_percent(0);
}

int FanController::current_pwm() const {
    return current_pwm_;
}

bool FanController::is_on() const {
    return enabled_;
}
