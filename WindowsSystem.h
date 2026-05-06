// Copyright 2026 MrSeagull. All Rights Reserved.
#pragma once
#include <iostream>
#include <windows.h>
#include <unordered_map>
#include <string>
#include <vector>
#include <functional>
#include "SharedTypes.h"

class WindowsSystem {
public:
    WindowsSystem(const WindowsSystem&) = delete;
    WindowsSystem& operator=(const WindowsSystem&) = delete;
    static WindowsSystem& getInstance(const std::string& path = "") {
        static WindowsSystem instance(path);
        return instance;
    }

    // ---------- 通用子进程管理接口 ----------
    bool LaunchChildProcessW(const ChildProcessConfig& config);
    bool WriteToSharedMemory(const std::string& processKey, const std::string& blockName, const void* data, size_t dataSize);
    size_t ReadFromSharedMemory(const std::string& processKey, const std::string& blockName, void* outBuffer, size_t bufferSize);
    bool SignalEvent(const std::string& processKey, const std::string& eventName);
    bool ResetEvent(const std::string& processKey, const std::string& eventName);
    bool WaitForChildExit(const std::string& processKey, DWORD timeoutMs = INFINITE);
    bool TerminateChildProcess(const std::string& processKey, DWORD gracefulTimeoutMs = 1000);
    bool IsChildRunning(const std::string& processKey);
    DWORD GetSharedMemorySize(const std::string& processKey, const std::string& blockName);

    // ---------- 便捷包装 ----------
    bool OpenProjectStructureViewer(const wchar_t* ViewerExePath, const wchar_t* ProjectRoot = nullptr);
    bool RefreshProjectStructureViewer();
    bool CloseProjectStructureViewer();
    bool OpenLevelTreeList();
    void Run();

    // ---------- 蓝图编辑器控制 ----------
    void Start_BPEditor();                                 // 启动蓝图编辑器子进程
    void Notify_BPEditor(std::wstring BlueprintPath);     // 通知编辑器加载指定蓝图

private:
    WindowsSystem();
    WindowsSystem(std::string temppath);
    ~WindowsSystem();

    bool CreateSharedMemoryBlock(const std::string& processKey, const std::string& blockName, DWORD size, bool readOnly = false);
    bool CreateProcessEvent(const std::string& processKey, const std::string& eventName, bool initialState = false);
    void CleanupChildProcess(const std::string& processKey);
    std::string MakeGlobalName(const std::string& processKey, const std::string& suffix);

    // 蓝图编辑器 IPC 初始化 / 销毁
    void CreateBPEditorIPC();
    void DestroyBPEditorIPC();

    LevelData* currentLevel = nullptr;
    char* currentProjectDirectory = nullptr;
    std::wstring exePath_ProjectStructureViewer = L"E:\\Projects\\C++Projects\\OFGal_Engine\\x64\\Debug\\ProjectStructureViewer.exe";
    std::wstring exePath_LevelTreeList = L"E:\\Projects\\C++Projects\\OFGal_Engine\\x64\\Debug\\LevelTreeList.exe";
    std::wstring exePath_BlueprintViewer = L"E:\\Projects\\C++Projects\\OFGal_Engine\\x64\\Debug\\BlueprintViewer.exe";

    std::unordered_map<std::string, ChildProcessInfo> childProcesses;
    std::wstring m_lastOpenedLevelPath;
    std::wstring m_lastOpenedBlueprintPath;
    std::wstring m_lastOpenedTextPath;

    HANDLE m_hLevelTreeListPathUpdateEvent;

    // 蓝图编辑器 IPC 对象（独立于子进程管理）
    HANDLE m_hBPEditorLoadEvent = NULL;
    HANDLE m_hBPEditorPathMap = NULL;
    char* m_pBPEditorPathView = nullptr;
    static const DWORD BPEDITOR_PATH_SIZE = 4096;
};