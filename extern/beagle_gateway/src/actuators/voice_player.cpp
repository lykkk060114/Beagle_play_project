#include <actuators/voice_player.hpp>

#include <cstdlib>
#include <iostream>
#include <sys/wait.h>
#include <thread>
#include <unistd.h>
#include <utility>

VoicePlayer::VoicePlayer(std::string audio_dir)
    : audio_dir_(std::move(audio_dir)),
      enabled_(true),
      cooldown_(std::chrono::seconds(30)),
      event_files_({
          {"system_start", "system_start.wav"},
          {"temp_high", "temp_high.wav"},
          {"fan_on", "fan_on.wav"},
          {"fan_off", "fan_off.wav"},
      }) {}

bool VoicePlayer::filename_safe(const std::string& filename) const {
    return !filename.empty() &&
           filename.find('/') == std::string::npos &&
           filename.find("..") == std::string::npos;
}

bool VoicePlayer::play_file(const std::string& filename) {
    if (!enabled_) {
        return false;
    }
    if (!filename_safe(filename)) {
        std::cerr << "Voice filename rejected: " << filename << std::endl;
        return false;
    }

    const std::string path = audio_dir_ + "/" + filename;
    std::thread([path]() {
        const pid_t pid = fork();
        if (pid == 0) {
            execlp("aplay", "aplay", path.c_str(), static_cast<char*>(nullptr));
            _exit(127);
        }
        if (pid > 0) {
            int status = 0;
            waitpid(pid, &status, 0);
        }
    }).detach();

    return true;
}

bool VoicePlayer::play_event(const std::string& event_name) {
    const auto it = event_files_.find(event_name);
    if (it == event_files_.end()) {
        std::cerr << "Unknown voice event: " << event_name << std::endl;
        return false;
    }

    const auto now = std::chrono::steady_clock::now();
    const auto last_it = last_play_.find(event_name);
    if (last_it != last_play_.end() && now - last_it->second < cooldown_) {
        return false;
    }

    if (!play_file(it->second)) {
        return false;
    }

    last_play_[event_name] = now;
    return true;
}

void VoicePlayer::set_enabled(bool enabled) {
    enabled_ = enabled;
}

bool VoicePlayer::enabled() const {
    return enabled_;
}
