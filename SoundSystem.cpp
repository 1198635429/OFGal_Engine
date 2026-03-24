#include "SoundSystem.h"
#include "AudioClip.h"
#include <iostream>


// 获取 SoundSystem 实例
SoundSystem& SoundSystem::getInstance() {
    static SoundSystem instance; // 使用静态局部变量来确保单例模式
    return instance;
}

// 构造函数
SoundSystem::SoundSystem() {
    
}

// 析构函数
SoundSystem::~SoundSystem() {
    Shutdown();
}

void SoundSystem::Init() {
    if (initialized)
        return;
    initialized = true;
}

void SoundSystem::Shutdown() {
    activeSounds.clear();
    initialized = false;
}

void SoundSystem::update(float dt) {
    if (!initialized)
        return;
    // 更新音频状态
}

void SoundSystem::playSound(const std::string& path, bool loop) {
    if (activeSounds.find(path) != activeSounds.end()) {
        return;  // 播放当前音频，避免重复播放同一音频
    }

    MCIPlay(path, loop);  // 使用 MCI 播放音频
    activeSounds[path] = path;  // 存储正在播放的音频路径
}

void SoundSystem::stopSound(const std::string& path) {
    auto it = activeSounds.find(path);
    if (it != activeSounds.end()) {
        MCIStop(path);  // 使用 MCI 停止音频播放
        activeSounds.erase(it);
    }
}

void SoundSystem::stopAllSounds() {
    for (auto& sound : activeSounds) {
        MCIStop(sound.first);  // 停止所有音频
    }
    activeSounds.clear();
}

void SoundSystem::SetVolume(float volume) {
    master_volume = volume;
    // 设置音量
}

std::shared_ptr<AudioClip> SoundSystem::Load(const std::string& path) {
    auto it = cache.find(path);
    if (it != cache.end())
        return it->second;

    auto clip = std::make_shared<AudioClip>(path, false);
    cache[path] = clip;
    return clip;
}

void SoundSystem::MCIPlay(const std::string& path, bool loop) {
    std::wstring widePath(path.begin(), path.end()); // 将 path 转换为 wstring
    LPCWSTR lpcwstrPath = widePath.c_str();  // 转换为 LPCWSTR

    MCI_OPEN_PARMS mciOpenParms = {};
    mciOpenParms.lpstrDeviceType = L"MPEGVideo";
    // 使用 L"设备类型"，将 ASCII 字符串改为宽字符字符串（LPCWSTR）
    mciOpenParms.lpstrElementName = lpcwstrPath; 
    // 使用 LPCWSTR,使得音频文件能正确传送给MCI函数
    //注意：只能进行二次转换，不能直接使用LPCWSTR，因为直接转换会无法识别路径，狠狠报错

    // 打开文件
    if (mciSendCommand(0, MCI_OPEN, MCI_OPEN_TYPE | MCI_OPEN_ELEMENT, (DWORD_PTR)&mciOpenParms) != 0) {
        std::cerr << "Error opening audio file: " << path << std::endl;
        return;
    }

    // 播放文件
    MCI_PLAY_PARMS mciPlayParms = {};
    if (mciSendCommand(mciOpenParms.wDeviceID, MCI_PLAY, 0, (DWORD_PTR)&mciPlayParms) != 0) {
        std::cerr << "Error playing audio file: " << path << std::endl;
    }

    // 如果需要循环播放，可以设置循环标志
    if (loop) {
       
    }
}

void SoundSystem::MCIStop(const std::string& path) {
    std::wstring widePath(path.begin(), path.end());
    LPCWSTR lpcwstrPath = widePath.c_str();

    MCI_OPEN_PARMS mciOpenParms = {};
    mciOpenParms.lpstrElementName = lpcwstrPath;

    // 使用 0 代替 nullptr 来符合 DWORD_PTR 类型的要求
    if (mciSendCommand(0, MCI_OPEN, MCI_OPEN_TYPE | MCI_OPEN_ELEMENT, (DWORD_PTR)&mciOpenParms) != 0) {
        std::cerr << "Error stopping audio file: " << path << std::endl;
        return;
    }

    mciSendCommand(mciOpenParms.wDeviceID, MCI_STOP, 0, 0);  
    mciSendCommand(mciOpenParms.wDeviceID, MCI_CLOSE, 0, 0); // 关闭文件
}