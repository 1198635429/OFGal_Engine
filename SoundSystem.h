#pragma once
#include <unordered_map>
#include <windows.h>  // MCI头文件
#include <functional>  // 用于事件总线
#include <stdexcept>  // 用于抛出异常
#include "SharedTypes.h"  // 包含音频结构体定义
#pragma comment(lib, "winmm.lib") // 告知链接器链接winmm.lib库

class SoundSystem {
public:
    SoundSystem(const SoundSystem&) = delete;
    SoundSystem& operator=(const SoundSystem&) = delete;
    // 获取 SoundSystem 实例（单例模式）
    static SoundSystem& getInstance()
    {
        static SoundSystem instance;
        return instance;
    }

    // 初始化音频系统
    void Init();

    // 停止音频系统
    void Shutdown();

    // 更新音频状态
    void update(float dt);

    // 播放音频
    void playSound(const std::string& path, bool loop = false);

    // 暂停音频
    void pauseSound(const std::string& path);

    // 停止音频
    void stopSound(const std::string& path);

    // 停止所有音频
    void stopAllSounds();

    // 设置全局音量
    void SetVolume(float volume);

    // 单独调节某个音频的音量
    void SetSoundVolume(const std::string& path, float volume);

    // 清空播放集合
    void clearAllSounds();

private:
    SoundSystem();
    ~SoundSystem();

    // 加载音频
    static std::shared_ptr<AudioClip> Load(const std::string& path);

    // 使用 MCI 播放音频
    void MCIPlay(const std::string& path, bool loop);

    // 使用 MCI 停止音频
    void MCIStop(const std::string& path);

    bool initialized = false;  // 是否初始化
    float global_volumn = 1.0; // 全局音量
    std::unordered_map<std::string, AudioClip> 播放集合;// 当前播放的音频集合
};