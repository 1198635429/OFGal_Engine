// Copyright 2026 MrSeagull. All Rights Reserved.
#pragma once
#include <windows.h>
#include <string>
#include <functional>

class FolderViewerManager {
public:
    using FolderChangeCallback = std::function<void()>;

    FolderViewerManager();
    ~FolderViewerManager();

    // 禁止拷贝
    FolderViewerManager(const FolderViewerManager&) = delete;
    FolderViewerManager& operator=(const FolderViewerManager&) = delete;

    // 启动 FolderViewer.exe，传入初始要显示的目录路径
    bool Start(const std::string& initialPath);

    // 关闭子进程（先发退出事件，超时后强制终止）
    void Shutdown(DWORD gracefulTimeoutMs = 1000);

    // 通知子进程更新显示路径（写入共享内存并触发事件）
    bool NotifyPathChange(const std::string& newPath);

    // 检查是否有来自子进程的文件夹变更事件（非阻塞）
    // 返回 true 表示有变更，并触发回调（如果已设置）
    bool PollFolderChange();

    // 设置文件夹变更回调
    void SetOnFolderChanged(FolderChangeCallback cb) { m_onFolderChanged = std::move(cb); }

    // 检查子进程是否仍在运行
    bool IsRunning() const;

private:
    // 构建全局唯一名称
    std::string MakeGlobalName(const std::string& suffix) const;

    // 创建共享内存和事件
    bool CreateIPCResources();

    // 清理所有 IPC 句柄
    void CleanupIPC();

    // 清理进程句柄
    void CleanupProcess();

    // 进程信息
    HANDLE m_hProcess;
    HANDLE m_hThread;
    DWORD  m_processId;

    // IPC 对象
    HANDLE m_hSharedMem;          // 共享内存句柄
    char* m_pSharedView;         // 映射视图
    HANDLE m_hEventUpdatePath;    // 主→子：路径更新事件
    HANDLE m_hEventFolderChanged; // 子→主：文件夹内容变更事件
    HANDLE m_hEventExit;          // 双向：退出事件

    // 回调
    FolderChangeCallback m_onFolderChanged;

    // 常量
    static constexpr DWORD SHARED_MEM_SIZE = 512; // MAX_PATH 足够
    static constexpr const char* PROCESS_KEY = "FolderViewer";
    static constexpr const char* FOLDER_VIEWER_EXE_PATH =
        "E:\\Projects\\C++Projects\\OFGal_Engine\\x64\\Debug\\FolderViewer.exe";
};