// Copyright 2026 Nagato-Yuki-708. All Rights Reserved.
#pragma once

#define NOMINMAX
#include <windows.h>
#include <string>
#include <chrono>
#include <thread>
#include <memory>
#include <vector>
#include <functional>

#include "SharedTypes.h"
#include "InputSystem.h"
#include "InputCollector.h"

class LevelTreeList {
public:
    LevelTreeList();
    ~LevelTreeList();

    bool Initialize();
    void Run();
    void Stop();

private:
    void ConfigureConsole();
    void SetWindowSizeAndPosition();
    bool OpenIPC();
    void CloseIPC();
    void HandleOpenLevelEvent();
    void HandleDataChangedEvent();

    void ProcessInputEvents();

    struct DisplayNode {
        std::string displayText;
        std::string prefix;
        std::string name;
        ObjectData* object;
        int depth;
    };

    void BuildDisplayList();
    void RecursiveBuild(const std::map<std::string, ObjectData*>& children,
        int depth, const std::string& prefix);
    void RenderTree();
    void ClearScreen();

    bool IsNameUsed(const std::string& name) const;
    bool IsNameUsedInObject(const ObjectData* obj, const std::string& name) const;

    void AddObjectInteractive();
    void DeleteSelectedObject();

    void SaveCurrentLevel();
    void RemoveObjectFromParent(ObjectData* obj);
    void ClearInputStream();
    bool AskComponent(const std::string& componentName);

    bool CreateDetailViewerIPC();
    bool StartDetailViewer();

    // --- 成员变量 ---
    bool m_running;

    HANDLE m_hEvent;               // 原有：主程序 -> 打开关卡事件
    HANDLE m_hMapFile;             // 原有：主程序共享内存句柄
    LPVOID m_pView;                // 原有：主程序共享内存视图

    HANDLE m_hPathUpdateEvent;     // 信号：路径已更新
    HANDLE m_hDataChangedEvent;    // 信号：数据已变更（需重新加载）
    HANDLE m_hPathSharedMem;       // 共享内存句柄（存放路径）
    LPVOID m_pPathSharedView;      // 共享内存视图

    HANDLE m_hObjectChangedEvent;   // 事件：选中对象已更改
    HANDLE m_hObjectSharedMem;      // 共享内存句柄（存放对象名）
    LPVOID m_pObjectSharedView;     // 共享内存视图

    InputSystem* m_inputSystem;
    std::unique_ptr<InputCollector> m_inputCollector;
    std::chrono::milliseconds m_loopInterval;

    std::unique_ptr<LevelData> m_currentLevel;
    std::string m_currentLevelPath;
    std::wstring m_currentLevelPathW; // 宽字符路径，共享内存用

    std::wstring exePath_DetailViewer = L"E:\\Projects\\C++Projects\\OFGal_Engine\\x64\\Debug\\DetailViewer.exe";

    std::vector<DisplayNode> m_displayNodes;
    int m_selectedIndex;
    bool m_needRender;
    bool m_inInteractiveMode = false;
};