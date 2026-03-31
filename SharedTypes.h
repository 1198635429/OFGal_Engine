#pragma once
#include <string>
#include <memory>

struct AudioClip {
    std::string path;        // 音频文件的绝对路径
    bool loop;               // 是否循环播放
    float volume;            // 音频的音量
    bool isPaused;           // 音频是否被暂停
    std::shared_ptr<void> audioData;  // 音频数据的智能指针

    // 默认构造函数
    AudioClip()
        : path(""), loop(false), volume(1.0f), isPaused(false) {
    }

    // 构造函数，初始化音频路径和循环标志,
    AudioClip(const std::string& path, bool loop)
        : path(path), loop(loop), volume(1.0f), isPaused(false) {
    }
};