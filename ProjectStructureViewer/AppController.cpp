// Copyright 2026 MrSeagull. All Rights Reserved.
#include "AppController.h"
#include "ProjectStructureGetter.h" // 外部提供的函数
#include "Debug.h"

AppController::AppController()
    : m_console()
    , m_view(m_console)
    , m_ipc()
    , m_project(nullptr)
    , m_running(false)
{
    m_console.SetupWindow();
    m_ipc.SetOnUpdate([this](const std::string& path) { OnIpcUpdate(path); });
    m_ipc.SetOnExit([this]() { OnIpcExit(); });
}

void AppController::OnIpcUpdate(const std::string& newPath) {
    DEBUG_LOG("AppController: Update to path: " << newPath << "\n");
    if (newPath.empty()) {
        DEBUG_LOG("AppController: Warning - empty path received\n");
        return;
    }

    // 重新加载项目结构
    m_project.reset(new ProjectStructure(GetProjectStructure(newPath.c_str())));
    m_view.SetProject(m_project.get());
    m_view.Render(0); // 默认选中根目录
}

void AppController::OnIpcExit() {
    DEBUG_LOG("AppController: Exit signal received\n");
    m_running = false;
}

int AppController::Run(const char* initialPathFromArg) {
    if (!m_ipc.Initialize()) {
        DEBUG_LOG("AppController: IPC initialization failed\n");
        return -1;
    }

    // 如果有初始路径（命令行参数），则先加载一次
    if (initialPathFromArg && initialPathFromArg[0] != '\0') {
        OnIpcUpdate(initialPathFromArg);
    }
    else {
        // 尝试从共享内存读取当前路径
        std::string sharedPath = m_ipc.GetCurrentPath();
        if (!sharedPath.empty()) {
            OnIpcUpdate(sharedPath);
        }
    }

    m_running = true;
    while (m_running) {
        // 处理 IPC 事件（超时100ms）
        bool continueRunning = m_ipc.WaitAndDispatch(100);
        if (!continueRunning) {
            break; // 收到退出事件
        }

        // 处理鼠标输入（优先于键盘）
        int clickedRow = -1;
        if (InputHandler::CheckMouseLeftClick(clickedRow)) {
            m_view.JumpToLine(clickedRow);
            continue; // 处理完鼠标直接继续循环，避免与键盘冲突
        }

        // 处理键盘输入
        if (m_project) {
            InputAction action = InputHandler::PollKeyboard();
            switch (action) {
            case InputAction::MoveUp:
                m_view.MoveHighlightUp();
                break;
            case InputAction::MoveDown:
                m_view.MoveHighlightDown();
                break;
            default:
                break;
            }
        }
    }

    DEBUG_LOG("AppController: Exiting\n");
    return 0;
}