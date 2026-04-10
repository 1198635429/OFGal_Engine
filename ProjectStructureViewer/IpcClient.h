// Copyright 2026 MrSeagull. All Rights Reserved.
#pragma once
#define NOMINMAX
#include <windows.h>
#include <string>
#include <vector>
#include <functional>

class IpcClient {
public:
    using UpdateCallback = std::function<void(const std::string& newPath)>;
    using ExitCallback = std::function<void()>;

    IpcClient();
    ~IpcClient();

    // 初始化 IPC 资源（打开事件、共享内存），返回是否成功
    bool Initialize();

    // 设置回调函数
    void SetOnUpdate(UpdateCallback cb) { m_onUpdate = std::move(cb); }
    void SetOnExit(ExitCallback cb) { m_onExit = std::move(cb); }

    // 主循环：等待事件（超时100ms），并触发相应回调
    // 返回 false 表示收到退出事件，应结束程序
    bool WaitAndDispatch(DWORD timeoutMs = 100);

    // 获取当前共享内存中的路径（线程安全读取）
    std::string GetCurrentPath() const;

private:
    HANDLE m_hExitEvent;
    HANDLE m_hUpdateEvent;
    HANDLE m_hMapFile;
    char* m_pSharedBuf;

    UpdateCallback m_onUpdate;
    ExitCallback   m_onExit;
};