// Copyright 2026 MrSeagull. All Rights Reserved.
#pragma once
#include "IpcClient.h"
#include "TreeView.h"
#include "ConsoleManager.h"
#include "InputHandler.h"
#include <memory>

class AppController {
public:
    AppController();
    ~AppController() = default;

    // 运行主循环
    int Run(const char* initialPathFromArg = nullptr);

private:
    void OnIpcUpdate(const std::string& newPath);
    void OnIpcExit();

    ConsoleManager            m_console;
    TreeView                  m_view;
    IpcClient                 m_ipc;
    std::unique_ptr<ProjectStructure> m_project; // 所有权
    bool                      m_running;
};