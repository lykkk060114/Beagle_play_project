/***
 * @Author: LYK && 2586356361@qq.com
 * @Date: 2026-06-11
 * @LastEditors: LYK && 2586356361@qq.com
 * @LastEditTime: 2026-06-11
 * @FilePath: /beagle_play/include/actuators/fan_controller.hpp
 * @Description: BeaglePlay 风扇 PWM 控制
 * @
 * @Copyright (c) 2026  All Rights Reserved.
 */
#pragma once

#include <string>

class FanController {
public:
    FanController(std::string pwm_chip = "/sys/class/pwm/pwmchip2",
                  int channel = 0,
                  int period_ns = 1000000);

    bool init();
    bool set_pwm_percent(int percent);
    bool on(int percent);
    bool off();
    int current_pwm() const;
    bool is_on() const;

private:
    std::string pwm_chip_;
    std::string pwm_path_;
    int channel_;
    int period_ns_;
    int current_pwm_;
    bool initialized_;
    bool enabled_;

    bool export_pwm();
    bool write_value(const std::string& path, const std::string& value) const;
    bool path_exists(const std::string& path) const;
};
