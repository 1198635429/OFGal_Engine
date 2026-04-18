#include "SoundSystem.h"
#include "SharedTypes.h"
#include "_EventBus.h"
#include <windows.h>
#include <mmsystem.h>

#pragma comment(lib, "winmm.lib")

SoundSystem::SoundSystem() {
    OutputDebugStringA("=== SoundSystem Constructor START ===\n");
    auto& bus = _EventBus::getInstance();

    bus.subscribe_PlaySound([this](const PlaySoundEvent& playEvent) {
        playSound(playEvent.path, playEvent.loop);
        });
  
    bus.subscribe_PauseSound([this](const PauseSoundEvent& playEvent) {
        pauseSound(playEvent.path);
        });

    bus.subscribe_StopSound([this](const StopSoundEvent& playEvent) {
        stopSound(playEvent.path);
        });

    bus.subscribe_StopAllSound([this](const StopAllSoundEvent&) {
        stopAllSounds();
        });

    bus.subscribe_SetVolume([this](const SetVolumeEvent& playEvent) {
        SetVolume(playEvent.volume);
        });

    bus.subscribe_SetSoundVolume([this](const SetSoundVolumeEvent& playEvent) {
        SetSoundVolume(playEvent.path, playEvent.volume);
        });
    OutputDebugStringA("=== SoundSystem Constructor END (subscriptions done) ===\n");
}

SoundSystem::~SoundSystem() {
    Shutdown();
}

void SoundSystem::Init() {
    OutputDebugStringA("=== SoundSystem::Init ===\n");
    if (initialized) return;
    initialized = true;
}

void SoundSystem::Shutdown() {
    stopAllSounds();
    initialized = false;
}

void SoundSystem::update(float dt) {
    for (auto it = 播放集合.begin(); it != 播放集合.end(); ) {
        std::string name = it->first;
        char buffer[128] = {};
        std::string cmd = "status " + name + " mode";
        DWORD err = mciSendStringA(cmd.c_str(), buffer, sizeof(buffer), NULL);

        if (err == 0) {
            OutputDebugStringA(("Status of " + name + ": " + std::string(buffer) + "\n").c_str());

            if (strcmp(buffer, "stopped") == 0) {
                OutputDebugStringA(("Audio " + name + " stopped\n").c_str());

                if (it->second.loop) {
                    OutputDebugStringA("Loop flag true, restarting...\n");

                    // 方法1：先 seek 到开头再播放（适用于某些驱动）
                    std::string seekCmd = "seek " + name + " to start";
                    mciSendStringA(seekCmd.c_str(), NULL, 0, NULL);
                    std::string playCmd = "play " + name;
                    DWORD playErr = mciSendStringA(playCmd.c_str(), NULL, 0, NULL);

                    if (playErr != 0) {
                        // 方法2：如果 seek+play 失败，关闭设备重新打开
                        OutputDebugStringA("seek+play failed, reopening device...\n");
                        mciSendStringA(("close " + name).c_str(), NULL, 0, NULL);
                        std::string openCmd = "open \"" + it->second.path + "\" alias " + name;
                        mciSendStringA(openCmd.c_str(), NULL, 0, NULL);
                        mciSendStringA(("play " + name).c_str(), NULL, 0, NULL);
                    }
                    ++it;
                }
                else {
                    OutputDebugStringA("Loop flag false, closing and removing\n");
                    mciSendStringA(("close " + name).c_str(), NULL, 0, NULL);
                    it = 播放集合.erase(it);
                }
                continue;
            }
        }
        else {
            char errBuf[256];
            mciGetErrorStringA(err, errBuf, sizeof(errBuf));
            OutputDebugStringA(("Status query error: " + std::string(errBuf) + "\n").c_str());
        }
        ++it;
    }
}
// 工具函数：取文件名（无后缀）
static std::string GetName(const std::string& path) {
    size_t slash = path.find_last_of("\\/");
    std::string filename = (slash == std::string::npos) ? path : path.substr(slash + 1);
    size_t dot = filename.find_last_of('.');
    if (dot != std::string::npos) {
        filename = filename.substr(0, dot);
    }
    // 安全地替换非字母数字字符为下划线
    for (char& c : filename) {
        unsigned char uc = static_cast<unsigned char>(c);
        if (!isalnum(uc) && c != '_') {
            c = '_';
        }
    }
    return filename;
}

// 播放

void SoundSystem::playSound(const std::string& path, bool loop) {
    OutputDebugStringA(("playSound called, path=" + path + "\n").c_str());

    std::string name = GetName(path);
    if (播放集合.find(name) != 播放集合.end()) {
        OutputDebugStringA("Audio already in collection, skip.\n");
        return;
    }

    // 1. 打开设备
    std::string cmd = "open \"" + path + "\" alias " + name;
    char errorBuf[256];
    DWORD err = mciSendStringA(cmd.c_str(), NULL, 0, NULL);
    if (err != 0) {
        mciGetErrorStringA(err, errorBuf, sizeof(errorBuf));
        OutputDebugStringA(("MCI Open Error: " + std::string(errorBuf) + "\n").c_str());
        return;  // 打开失败，不插入集合
    }

    // 2. 播放
    if (loop) {
        cmd = "play " + name;
    }
    else {
        cmd = "play " + name;
    }
    err = mciSendStringA(cmd.c_str(), NULL, 0, NULL);
    if (err != 0) {
        mciGetErrorStringA(err, errorBuf, sizeof(errorBuf));
        OutputDebugStringA(("MCI Play Error: " + std::string(errorBuf) + "\n").c_str());
        // 播放失败，关闭设备并返回
        cmd = "close " + name;
        mciSendStringA(cmd.c_str(), NULL, 0, NULL);
        return;
    }

    // 3. 成功：加入集合
    AudioClip clip(path, loop);
    clip.volume = global_volumn;
    clip.isPaused = false;
    播放集合[name] = clip;    
    int mciVolume = static_cast<int>(global_volumn * 1000);// 初始化音量
    std::string volCmd = "setaudio " + name + " volume to " + std::to_string(mciVolume);
    mciSendStringA(volCmd.c_str(), NULL, 0, NULL);

    OutputDebugStringA(("Play started: " + name + "\n").c_str());
}

// 暂停
void SoundSystem::pauseSound(const std::string& pathOrAlias) {
    // 先尝试直接作为别名查找
    std::string name = pathOrAlias;
    auto it = 播放集合.find(name);
    if (it == 播放集合.end()) {
        // 找不到，则当作原始路径，计算别名
        name = GetName(pathOrAlias);
        it = 播放集合.find(name);
        if (it == 播放集合.end()) {
            OutputDebugStringA(("pauseSound: audio not found: " + pathOrAlias + "\n").c_str());
            return;  // 不抛出异常，静默返回
        }
    }
    std::string cmd = "pause " + name;   // 注意空格
    mciSendStringA(cmd.c_str(), NULL, 0, NULL);
    it->second.isPaused = true;
    OutputDebugStringA(("Pause: " + name + "\n").c_str());
}

// 停止
void SoundSystem::stopSound(const std::string& name)
{
    auto it = 播放集合.find(name);
    if (it == 播放集合.end())
        return;

    std::string cmd = "stop " + name;
    mciSendStringA(cmd.c_str(), NULL, 0, NULL);

    cmd = "close " + name;
    mciSendStringA(cmd.c_str(), NULL, 0, NULL);

    播放集合.erase(it);

    OutputDebugStringA(("Stop: " + name + "\n").c_str());
}

// 停止全部
void SoundSystem::stopAllSounds()
{
    for (auto& pair : 播放集合)
    {
        std::string name = pair.first;

        std::string cmd = "stop " + name;
        mciSendStringA(cmd.c_str(), NULL, 0, NULL);

        cmd = "close " + name;
        mciSendStringA(cmd.c_str(), NULL, 0, NULL);
    }

    播放集合.clear();

    OutputDebugStringA("Stop All Sounds\n");
}

// 全局音量
void SoundSystem::SetVolume(float volume) {
    global_volumn = std::clamp(volume, 0.0f, 1.0f);
    for (auto& pair : 播放集合) {
        int mciVolume = static_cast<int>(global_volumn * 1000);
        std::string cmd = "setaudio " + pair.first + " volume to " + std::to_string(mciVolume);
        mciSendStringA(cmd.c_str(), NULL, 0, NULL);
        pair.second.volume = global_volumn;
    }
    OutputDebugStringA(("Set Global Volume to " + std::to_string(global_volumn) + "\n").c_str());
}

// 单个音量
void SoundSystem::SetSoundVolume(const std::string& name, float volume) {
    auto it = 播放集合.find(name);
    if (it == 播放集合.end()) {
        OutputDebugStringA(("Audio not found: " + name + "\n").c_str());
        return;
    }
    volume = std::clamp(volume, 0.0f, 1.0f);
    int mciVolume = static_cast<int>(volume * 1000);
    std::string cmd = "setaudio " + name + " volume to " + std::to_string(mciVolume);
    mciSendStringA(cmd.c_str(), NULL, 0, NULL);
    it->second.volume = volume;
    OutputDebugStringA(("Set Volume of " + name + " to " + std::to_string(volume) + "\n").c_str());
}


// 清空集合

void SoundSystem::clearAllSounds()
{
    stopAllSounds();
}

// Load（目前简单构造）

std::shared_ptr<AudioClip> SoundSystem::Load(const std::string& path)
{
    return std::make_shared<AudioClip>(path, false);
}


// MCI 播放
void SoundSystem::MCIPlay(const std::string& path, bool loop)
{
    std::string name = GetName(path);

    std::string cmd = "open \"" + path + "\" alias " + name;
    mciSendStringA(cmd.c_str(), NULL, 0, NULL);

    if (loop)
        cmd = "play " + name + " repeat";
    else
        cmd = "play " + name;

    mciSendStringA(cmd.c_str(), NULL, 0, NULL);
}

// MCI 停止
void SoundSystem::MCIStop(const std::string& path)
{
    std::string name = GetName(path);

    std::string cmd = "stop " + name;
    mciSendStringA(cmd.c_str(), NULL, 0, NULL);

    cmd = "close " + name;
    mciSendStringA(cmd.c_str(), NULL, 0, NULL);
}