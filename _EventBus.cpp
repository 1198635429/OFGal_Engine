#include "_EventBus.h"
#include <iostream>

/*
==========================================
音频系统事件订阅与发布
==========================================
*/
void _EventBus::publish_SoundPlay(const char* sound_path)
{
    for (auto& handler : handlers_SoundPlay)
    {
        handler(sound_path);
    }
}
void _EventBus::subscribe_SoundPlay(SoundPlay_Handler handler)
{
    handlers_SoundPlay.push_back(handler);
}
/*
==========================================
文件系统事件订阅与发布
==========================================
*/
FolderStructure _EventBus::publish_GetFolderStructure(const char* Directory)
{
    for (auto& handler : handlers_GetFolderStructure)
    {
        return handler(Directory);
    }
}
void _EventBus::subscribe_GetFolderStructure(GetFolderStructure_Handler handler)
{
    handlers_GetFolderStructure.push_back(handler);
}
ProjectStructure _EventBus::publish_GetProjectStructure(const char* RootDirectory)
{
    for (auto& handler : handlers_GetProjectStructure)
    {
        return handler(RootDirectory);
    }
}
void _EventBus::subscribe_GetProjectStructure(GetProjectStructure_Handler handler)
{
    handlers_GetProjectStructure.push_back(handler);
}
BMP_Data _EventBus::publish_ReadBMP(const char* filepath)
{
    for (auto& handler : handlers_ReadBMP)
    {
        return handler(filepath);
    }
}
void _EventBus::subscribe_ReadBMP(ReadBMP_Handler handler)
{
    handlers_ReadBMP.push_back(handler);
}
LevelData _EventBus::publish_ReadLevelData(const std::string& filepath)
{
    for (auto& handler : handlers_ReadLevelData)
    {
        return handler(filepath);
    }
}
void _EventBus::subscribe_ReadLevelData(ReadLevelData_Handler handler)
{
    handlers_ReadLevelData.push_back(handler);
}
void _EventBus::publish_WriteLevelData(const std::string& filepath, const LevelData& data)
{
    for (auto& handler : handlers_WriteLevelData)
    {
        handler(filepath, data);
    }
}
void _EventBus::subscribe_WriteLevelData(WriteLevelData_Handler handler)
{
    handlers_WriteLevelData.push_back(handler);
}
void _EventBus::publish_WriteBPData(const std::string& filepath, const BlueprintData& data)
{
    for (auto& handler : handlers_WriteBPData)
    {
        handler(filepath, data);
    }
}
void _EventBus::subscribe_WriteBPData(WriteBPData_Handler handler)
{
    handlers_WriteBPData.push_back(handler);
}
BlueprintData _EventBus::publish_ReadBPData(const std::string& filepath)
{
    for (auto& handler : handlers_ReadBPData)
    {
        return handler(filepath);
    }
}
void _EventBus::subscribe_ReadBPData(ReadBPData_Handler handler)
{
    handlers_ReadBPData.push_back(handler);
}