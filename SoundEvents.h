#pragma once
#include <string>

// 播放音频事件
struct PlaySoundEvent {
    std::string path;  // 音频路径
    bool loop = false; // 是否循环播放
};

// 停止音频事件
struct StopSoundEvent {
    std::string path; // 音频路径
};

// 调整音量事件
struct SetVolumeEvent {
    float volume = 1.0f;  // 音量（0.0 - 1.0）
};