// Copyright 2026 Nagato-Yuki-708. All Rights Reserved.

#pragma once
#include "SharedTypes.h"
#include "InputCollector.h"
#include "InputSystem.h"
#include "Json_LevelData_ReadWrite.h"
#include <memory>
#include <string>

// 组件类型枚举，用于标识当前选中的组件
enum class ComponentType {
    None,
    Transform,
    Picture,
    Textblock,
    TriggerArea,
    Blueprint
};

// 当前选中组件信息
struct SelectedComponent {
    ComponentType type = ComponentType::None;
    void* ptr = nullptr; // 指向 ObjectData 内部对应 std::optional 的地址
};

class DetailViewer {
public:
    DetailViewer();
    ~DetailViewer();

    void ConfigureConsole();
    void SetWindowSizeAndPosition();

    void Run();

private:
    // ---- 事件句柄 ----
    HANDLE m_hEventPathUpdate;
    HANDLE m_hEventDataChanged;
    HANDLE m_hEventObjectChanged;

    // ---- 共享内存句柄 ----
    HANDLE m_hMapPath;
    HANDLE m_hMapObject;
    LPVOID m_pViewPath;   // 指向路径共享内存的数据视图
    LPVOID m_pViewObject; // 指向对象名共享内存的数据视图

    // ---- 关卡数据 ----
    std::unique_ptr<LevelData> m_currentLevel;
    ObjectData* m_currentObject = nullptr;
    SelectedComponent          m_selectedComponent;

    // ---- 输入系统 ----
    InputSystem     m_inputSystem;
    InputCollector  m_inputCollector;

    bool m_bindingsAdded = false; // 确保按键绑定只添加一次

    // ---- 工具 ----
    void ClearScreen();
};