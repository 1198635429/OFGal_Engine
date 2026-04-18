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
    // 启动一个子进程（根据配置）
    bool LaunchChildProcessW(const ChildProcessConfig& config);

    // 向指定子进程的指定共享内存块写入数据（二进制安全）
    bool WriteToSharedMemory(const std::string& processKey, const std::string& blockName, const void* data, size_t dataSize);

    // 从指定子进程的指定共享内存块读取数据（返回读取的字节数，需预分配缓冲区）
    size_t ReadFromSharedMemory(const std::string& processKey, const std::string& blockName, void* outBuffer, size_t bufferSize);

    // 触发指定子进程的某个事件
    bool SignalEvent(const std::string& processKey, const std::string& eventName);

    // 重置事件（用于手动重置事件）
    bool ResetEvent(const std::string& processKey, const std::string& eventName);

    // 等待子进程退出（可选超时毫秒，默认 INFINITE）
    bool WaitForChildExit(const std::string& processKey, DWORD timeoutMs = INFINITE);

    // 关闭子进程（先发送 Exit 事件，超时后强制终止）
    bool TerminateChildProcess(const std::string& processKey, DWORD gracefulTimeoutMs = 1000);

    // 检查子进程是否仍在运行
    bool IsChildRunning(const std::string& processKey);

    // 获取子进程的共享内存块大小
    DWORD GetSharedMemorySize(const std::string& processKey, const std::string& blockName);

    // ---------- 便捷包装 ----------
    bool OpenProjectStructureViewer(const wchar_t* ViewerExePath, const wchar_t* ProjectRoot = nullptr);
    bool RefreshProjectStructureViewer();
    bool CloseProjectStructureViewer();

private:
    WindowsSystem();
    WindowsSystem(std::string temppath);
    ~WindowsSystem();

    // 内部辅助：创建指定子进程的共享内存块
    bool CreateSharedMemoryBlock(const std::string& processKey, const std::string& blockName, DWORD size, bool readOnly = false);

    // 内部辅助：创建指定子进程的事件
    bool CreateProcessEvent(const std::string& processKey, const std::string& eventName, bool initialState = false);

    // 清理指定子进程的所有资源
    void CleanupChildProcess(const std::string& processKey);

    // 构建全局唯一名称（保持 ANSI 用于内核对象命名）
    std::string MakeGlobalName(const std::string& processKey, const std::string& suffix);

private:
    std::vector<LevelData> levels;
    char* currentProjectDirectory = nullptr;                            // 当前项目路径（ANSI）
    std::wstring exePath_ProjectStructureViewer = L"E:\\Projects\\C++Projects\\OFGal_Engine\\x64\\Debug\\ProjectStructureViewer.exe";

    std::unordered_map<std::string, ChildProcessInfo> childProcesses;   // 进程键 -> 信息
};