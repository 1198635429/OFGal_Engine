#include "AudioClip.h"

AudioClip::AudioClip(const std::string& path, bool loop)
    : path(path), loop(loop)  
{
    
}

AudioClip::~AudioClip() {
    // 智能指针会自动管理内存，无需手动释放
}