// Copyright 2026 MrSeagull. All Rights Reserved.
#pragma once
#include <windows.h>
#include <string>
#include <memory>
#include <functional>

// 前向声明
class InputSystem;
class InputCollector;

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

    // 输入回调（占位，末尾会通知父进程变更）
    void OnRightClick();
    void OnCtrlN();
    void OnDelete();

    // 辅助：向父进程发送文件夹变更通知
    void NotifyParentFolderChanged(const std::string& newPath);

private:
    // IPC 句柄
    HANDLE m_hSharedMem;
    char* m_pSharedView;
    HANDLE m_hEventUpdatePath;
    HANDLE m_hEventFolderChanged;
    HANDLE m_hEventExit;

    // 当前显示的目录路径
    std::string m_currentFolderPath;

    // 输入系统
    std::unique_ptr<InputSystem> m_inputSystem;
    std::unique_ptr<InputCollector> m_inputCollector;

    // 运行标志
    bool m_running;

    // 常量
    static constexpr DWORD SHARED_MEM_SIZE = 512;
    static constexpr const char* PROCESS_KEY = "FolderViewer";
};