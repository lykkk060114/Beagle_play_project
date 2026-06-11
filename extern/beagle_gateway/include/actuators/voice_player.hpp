/***
 * @Author: LYK && 2586356361@qq.com
 * @Date: 2026-06-11
 * @LastEditors: LYK && 2586356361@qq.com
 * @LastEditTime: 2026-06-11
 * @FilePath: /beagle_play/include/actuators/voice_player.hpp
 * @Description: BeaglePlay 语音播放控制
 * @
 * @Copyright (c) 2026  All Rights Reserved.
 */
#pragma once

#include <chrono>
#include <string>
#include <unordered_map>

class VoicePlayer {
public:
    explicit VoicePlayer(std::string audio_dir = "/home/debian/audio");

    bool play_file(const std::string& filename);
    bool play_event(const std::string& event_name);
    void set_enabled(bool enabled);
    bool enabled() const;

private:
    std::string audio_dir_;
    bool enabled_;
    std::chrono::seconds cooldown_;
    std::unordered_map<std::string, std::string> event_files_;
    std::unordered_map<std::string, std::chrono::steady_clock::time_point> last_play_;

    bool filename_safe(const std::string& filename) const;
};
