#pragma once
#include <string>
#include <functional>
#include <unordered_map>
#include <memory>
#include <windows.h>  // MCI头文件

class AudioClip;

class SoundSystem {
public:
    // 获取 SoundSystem 实例
    static SoundSystem& getInstance();
    void Init(); // 初始化
    void Shutdown(); // 清理
    void update(float dt); // 更新音频状态
    void playSound(const std::string& path, bool loop = false); // 播放音频
    void stopSound(const std::string& path); // 停止某个音频
    void stopAllSounds(); // 停止所有音频
    void SetVolume(float volume); // 设置音量

private:
    // 私有构造函数，确保通过单例获取实例
    SoundSystem();
    ~SoundSystem();

    // 私有成员
    std::shared_ptr<AudioClip> Load(const std::string& path); // 加载音频
    void MCIPlay(const std::string& path, bool loop); // 使用 MCI 播放音频
    void MCIStop(const std::string& path); // 停止音频播放

    bool initialized = false;
    float master_volume = 1.0f;
    std::unordered_map<std::string, std::shared_ptr<AudioClip>> cache; // 音频缓存
    std::unordered_map<std::string, std::string> activeSounds; // 当前播放的音频
};