#pragma once
#include <string>
#include <memory> 

class AudioClip {
public:
    std::string path; // 音频文件路径
    bool loop; // 是否循环
    std::shared_ptr<void> audioData; // 使用智能指针来管理音频数据

    AudioClip(const std::string& path, bool is_loop);
    ~AudioClip();
};