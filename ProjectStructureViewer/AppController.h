// Copyright 2026 MrSeagull. All Rights Reserved.
#pragma once
#include "IpcClient.h"
#include "TreeView.h"
#include "ConsoleManager.h"
#include "InputHandler.h"
#include "FolderViewerManager.h"   // 新增
#include <memory>

class AppController {
public:
    AppController();
    ~AppController() = default;

    int Run(const char* initialPathFromArg = nullptr);

private:
    void OnIpcUpdate(const std::string& newPath);
    void OnIpcExit();

    // 新增：处理文件夹变更（由 FolderViewer 触发）
    void OnFolderChanged();

    ConsoleManager            m_console;
    TreeView                  m_view;
    IpcClient                 m_ipc;
    std::unique_ptr<ProjectStructure> m_project;
    bool                      m_running;

    // 新增：FolderViewer 子进程管理器
    std::unique_ptr<FolderViewerManager> m_folderViewerMgr;
    bool                      m_folderViewerStarted;  // 标记是否已启动
};