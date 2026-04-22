// Copyright 2026 Nagato-Yuki-708. All Rights Reserved.
#pragma once

#define NOMINMAX
#include <windows.h>
#include <string>
#include <chrono>
#include <thread>
#include <memory>
#include <vector>
#include <functional>   // 新增：用于 std::function

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

    // 删除对象（仅需对象指针）
    void RemoveObjectFromParent(ObjectData* obj);

    // 辅助输入函数：清除 cin 缓冲区
    void ClearInputStream();

    // 辅助函数：询问单个组件
    bool AskComponent(const std::string& componentName);

    // --- 成员变量 ---
    bool m_running;

    HANDLE m_hEvent;
    HANDLE m_hMapFile;
    LPVOID m_pView;

    InputSystem* m_inputSystem;
    std::unique_ptr<InputCollector> m_inputCollector;
    std::chrono::milliseconds m_loopInterval;

    std::unique_ptr<LevelData> m_currentLevel;   // 自动管理内存
    std::string m_currentLevelPath;

    std::vector<DisplayNode> m_displayNodes;
    int m_selectedIndex;
    bool m_needRender;
    bool m_inInteractiveMode = false;
};