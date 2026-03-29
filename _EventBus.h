#pragma once
#include <functional>
#include <vector>
#include "SharedTypes.h"		//结构体统一定义头文件，其中包含了所有需要用于系统间通信的结构体
#include "InputEvent.h"

class _EventBus {
    //使用单例模式
public:
    _EventBus(const _EventBus&) = delete;
    _EventBus& operator=(const _EventBus&) = delete;

    static _EventBus& getInstance() {
        static _EventBus instance;
        return instance;
    }
    /*
    ==========================================
    音频系统事件
    ==========================================
    */
    using SoundPlay_Handler = std::function < bool(const char*) >;
    /*
    ==========================================
    文件系统事件
    ==========================================
    */
    using ReadLevelData_Handler = std::function < LevelData(const std::string&) >;
    using WriteLevelData_Handler = std::function < bool(const std::string&, const LevelData&) >;
    using GetProjectStructure_Handler = std::function < ProjectStructure(const char*) >;
    using GetFolderStructure_Handler = std::function < FolderStructure(const char*) >;
    using ReadBMP_Handler = std::function < BMP_Data(const char*) >;
    using WriteBPData_Handler = std::function < bool(const std::string&, const BlueprintData&) >;
    using ReadBPData_Handler = std::function < BlueprintData(const std::string&) >;

	using SoundPlay_Handler = std::function < bool(const char*) >;		//bool表示是否成功，char*则指向音频文件路径，视实际情况更改
	using InputEvent_Handler = std::function <void(const InputEvent&)>;   //这是我的输入系统的函数对应的储存标签
	//其它事件类型，请自行添加



    /*
    ==========================================
    音频系统事件订阅与发布
    ==========================================
    */
    void subscribe_SoundPlay(SoundPlay_Handler handler);
    void publish_SoundPlay(const char* sound_path);
    /*
    ==========================================
    文件系统事件订阅与发布
    ==========================================
    */
    void subscribe_ReadLevelData(ReadLevelData_Handler handler);
    void subscribe_WriteLevelData(WriteLevelData_Handler handler);
    void subscribe_GetProjectStructure(GetProjectStructure_Handler handler);
    void subscribe_GetFolderStructure(GetFolderStructure_Handler handler);
    void subscribe_ReadBMP(ReadBMP_Handler handler);
    void subscribe_ReadBPData(ReadBPData_Handler handler);
    void subscribe_WriteBPData(WriteBPData_Handler handler);
    BMP_Data publish_ReadBMP(const char* filepath);
    void publish_WriteLevelData(const std::string& filepath, const LevelData& data);
    LevelData publish_ReadLevelData(const std::string& filepath);
    ProjectStructure publish_GetProjectStructure(const char* RootDirectory);
    FolderStructure publish_GetFolderStructure(const char* Directory);
    BlueprintData publish_ReadBPData(const std::string& filepath);
    void publish_WriteBPData(const std::string& filepath, const BlueprintData& data);
	// 订阅播放音频事件
	void subscribe_SoundPlay(SoundPlay_Handler handler);
	// 发布播放音频事件（通知所有订阅者）
	void publish_SoundPlay(const char* sound_path);

	void subscribe_InputEvent(InputEvent_Handler handler);
	void publish_InputEvent(const InputEvent& event);
	// 其它事件订阅和发布函数，请像上面两个函数一样自行添加

private:
    _EventBus() = default;
    ~_EventBus() = default;

    /*
    ==========================================
    音频系统事件订阅者
    ==========================================
    */
    std::vector<SoundPlay_Handler> handlers_SoundPlay;  // 所有播放音频事件订阅者，按理来说只有 音频系统 会订阅
    /*
    ==========================================
    文件系统事件订阅者
    ==========================================
    */
    std::vector<ReadLevelData_Handler> handlers_ReadLevelData;
    std::vector<WriteLevelData_Handler> handlers_WriteLevelData;
    std::vector<GetProjectStructure_Handler> handlers_GetProjectStructure;
    std::vector<GetFolderStructure_Handler> handlers_GetFolderStructure;
    std::vector<ReadBMP_Handler> handlers_ReadBMP;
    std::vector<WriteBPData_Handler> handlers_WriteBPData;
    std::vector<ReadBPData_Handler> handlers_ReadBPData;
	std::vector<SoundPlay_Handler> handlers_SoundPlay;  // 所有播放音频事件订阅者，按理来说只有 音频系统 会订阅
	std::vector<InputEvent_Handler> handlers_InputEvent;  //输入事件订阅者存放的容器
	// 其它订阅者类型，请像上面一样自行添加
};