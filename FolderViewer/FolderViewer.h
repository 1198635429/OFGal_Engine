// Copyright 2026 MrSeagull. All Rights Reserved.
#pragma once
#include <windows.h>
#include <string>
#include <memory>
#include <functional>
#include <unordered_map>
#include <vector>

#include "KeyCode.h"

// 前向声明
class InputSystem;
class InputCollector;

// 用于存储显示条目的信息
struct DisplayItem {
    std::string name;       // 显示名称（文件/文件夹名）
    std::string fullPath;   // 完整路径
    bool isDirectory;       // 是否为文件夹
};

class FolderViewer {
public:

    FolderViewer();
    ~FolderViewer();

    int Run();

private:
    // IPC 初始化与清理
    bool OpenIPCResources();
    void CloseIPCResources();

    // 事件等待与分发
    void MainLoop();

    // 事件响应
    void OnUpdatePath();
    void OnExit();

    // 目录显示
    void RefreshDisplay(const std::string& folderPath);
    void ClearScreen();

    // 选择导航
    void MoveSelection(int delta);

    // 输入回调
    void OnRightClick();
    void OnCtrlN();
    void OnDelete();
    void OnEnter();
    void OnMoveUp();
    void OnMoveDown();

    // 辅助：向父进程发送文件夹变更通知
    void NotifyParentFolderChanged(const std::string& newPath);

private:
    // IPC 句柄
    HANDLE m_hSharedMem;
    char* m_pSharedView;
    HANDLE m_hEventUpdatePath;
    HANDLE m_hEventFolderChanged;
    HANDLE m_hEventExit;

    HANDLE m_hSharedMem_OpenLevel;
    WCHAR* m_pSharedView_OpenLevel;
    HANDLE m_hEvent_OpenLevel;

    HANDLE m_hSharedMem_OpenBlueprint;
    WCHAR* m_pSharedView_OpenBlueprint;
    HANDLE m_hEvent_OpenBlueprint;

    HANDLE m_hSharedMem_OpenText;
    WCHAR* m_pSharedView_OpenText;
    HANDLE m_hEvent_OpenText;

    // 当前显示的目录路径
    std::string m_currentFolderPath;

    // 显示条目列表与选中索引
    std::vector<DisplayItem> m_displayItems;
    int m_selectedIndex = -1;

    // 输入系统
    std::unique_ptr<InputSystem> m_inputSystem;
    std::unique_ptr<InputCollector> m_inputCollector;

    // 事件映射表
    std::unordered_map<KeyCode, std::function<void()>> m_actionMap;

    // 运行标志
    bool m_running;

    // 常量
    static constexpr DWORD SHARED_MEM_SIZE = 512;
    static constexpr DWORD SHARED_MEM_SIZE_LARGE = 4096;
    static constexpr const char* PROCESS_KEY = "FolderViewer";

    std::wstring exePath_YesOrNo = L"E:\\Projects\\C++Projects\\OFGal_Engine\\x64\\Debug\\YesOrNo.exe";
    std::wstring exePath_AddItem = L"E:\\Projects\\C++Projects\\OFGal_Engine\\x64\\Debug\\AddItem.exe";

    bool m_virtualTerminalEnabled;
    void EnableVirtualTerminal();
    void SetConsoleHighlight(bool enable);
};