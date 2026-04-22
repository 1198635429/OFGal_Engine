// Copyright 2026 Nagato-Yuki-708. All Rights Reserved.
#pragma once

#include <windows.h>
#include <string>
#include <chrono>
#include <thread>
#include <memory>
#include <vector>

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

    void ProcessInputEvents();

    struct DisplayNode {
        std::string displayText;      // 完整显示行（含前缀和名称）
        std::string prefix;           // 树形前缀（用于颜色控制）
        std::string name;             // 对象/场景名
        ObjectData* object;           // 对应的对象指针（场景时为 nullptr）
        int depth;                    // 深度（0 为场景）
    };

    void BuildDisplayList();                                           // 构建 m_displayNodes
    void RecursiveBuild(const std::map<std::string, ObjectData*>& children,
        int depth, const std::string& prefix);        // 递归构建子节点
    void RenderTree();                                                 // 绘制整个树状列表
    void ClearScreen();                                                // ANSI 清屏并归位光标

    // --- 成员变量（部分已有）---
    bool m_running;

    HANDLE m_hEvent;
    HANDLE m_hMapFile;
    LPVOID m_pView;

    InputSystem* m_inputSystem;
    std::unique_ptr<InputCollector> m_inputCollector;
    std::chrono::milliseconds m_loopInterval;

    std::unique_ptr<LevelData> m_currentLevel;          // 当前加载的场景数据

    std::vector<DisplayNode> m_displayNodes;            // 扁平化的显示列表
    int m_selectedIndex;                                // 当前选中项的索引（0 为场景）
    bool m_needRender;                                  // 是否需要重绘（按键后置 true）
};