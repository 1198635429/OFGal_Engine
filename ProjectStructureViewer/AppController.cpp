// Copyright 2026 MrSeagull. All Rights Reserved.
#include "AppController.h"
#include "ProjectStructureGetter.h"
#include "Debug.h"

AppController::AppController()
    : m_console()
    , m_view(m_console)
    , m_ipc()
    , m_project(nullptr)
    , m_running(false)
    , m_folderViewerStarted(false)
{
    RECT rcWork;
    if (SystemParametersInfo(SPI_GETWORKAREA, 0, &rcWork, 0)) {
        workAreaHeight = rcWork.bottom - rcWork.top;
    }

    m_console.SetupWindow(screenWidth, workAreaHeight);
    m_ipc.SetOnUpdate([this](const std::string& path) { OnIpcUpdate(path); });
    m_ipc.SetOnExit([this]() { OnIpcExit(); });

    // 创建 FolderViewer 管理器
    m_folderViewerMgr = std::make_unique<FolderViewerManager>();
    m_folderViewerMgr->SetOnFolderChanged([this]() { OnFolderChanged(); });
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
    m_view.Render(0); // 默认高亮根目录

    // 第一次刷新列表后启动 FolderViewer（仅启动一次）
    if (!m_folderViewerStarted) {
        std::string initialFolderPath = m_view.GetSelectedInfo().absolutePath;
        if (m_folderViewerMgr->Start(initialFolderPath)) {
            m_folderViewerStarted = true;
            DEBUG_LOG("AppController: FolderViewer started with path: " << initialFolderPath << "\n");
        }
        else {
            DEBUG_LOG("AppController: Failed to start FolderViewer\n");
        }
    }
    else {
        // 若已启动，通知路径变更（当前选中的文件夹）
        std::string currentPath = m_view.GetSelectedInfo().absolutePath;
        m_folderViewerMgr->NotifyPathChange(currentPath);
    }
}

void AppController::OnIpcExit() {
    DEBUG_LOG("AppController: Exit signal received\n");
    m_running = false;
}

void AppController::OnFolderChanged() {
    DEBUG_LOG("AppController: FolderViewer reported a change in the folder.\n");

    if (!m_folderViewerMgr || !m_project) return;

    // 1. 读取子进程写入共享内存的变更路径
    std::string changedPath = m_folderViewerMgr->GetCurrentSharedPath();
    if (changedPath.empty()) {
        DEBUG_LOG("AppController: Shared path is empty, nothing to select.\n");
        return;
    }

    DEBUG_LOG("AppController: Received changed path: " << changedPath << "\n");

    // 2. 在树视图中查找并选中该路径
    int line = m_view.FindLineByPath(changedPath);
    if (line >= 0) {
        m_view.JumpToLine(line);
        DEBUG_LOG("AppController: Selected line " << line << " for path: " << changedPath << "\n");
    }
    else {
        DEBUG_LOG("AppController: Path not found in current tree view: " << changedPath << "\n");
        // 即使没找到，仍然尝试通知子进程（可能路径在根外，让子进程自行处理）
    }

    // 3. 通知子进程更新当前显示的路径（确保同步）
    if (m_folderViewerStarted) {
        m_folderViewerMgr->NotifyPathChange(changedPath);
    }
}

int AppController::Run(const char* initialPathFromArg) {
    if (!m_ipc.Initialize()) {
        DEBUG_LOG("AppController: IPC initialization failed\n");
        return -1;
    }

    // 处理初始路径
    if (initialPathFromArg && initialPathFromArg[0] != '\0') {
        OnIpcUpdate(initialPathFromArg);
    }
    else {
        std::string sharedPath = m_ipc.GetCurrentPath();
        if (!sharedPath.empty()) {
            OnIpcUpdate(sharedPath);
        }
    }

    m_running = true;
    while (m_running) {
        // 1. 处理主进程 IPC 事件
        bool continueRunning = m_ipc.WaitAndDispatch(100);
        if (!continueRunning) {
            break;
        }

        // 2. 检查 FolderViewer 发来的变更事件
        m_folderViewerMgr->PollFolderChange();

        // 3. 处理鼠标点击（选择文件夹）
        int clickedRow = -1;
        if (InputHandler::CheckMouseLeftClick(clickedRow)) {
            m_view.JumpToLine(clickedRow);
            // 选中项改变，通知 FolderViewer 更新路径
            if (m_folderViewerStarted) {
                std::string selectedPath = m_view.GetSelectedInfo().absolutePath;
                m_folderViewerMgr->NotifyPathChange(selectedPath);
            }
            continue;
        }

        // 4. 处理键盘（上下移动高亮）
        if (m_project) {
            InputAction action = InputHandler::PollKeyboard();
            bool moved = false;
            switch (action) {
            case InputAction::MoveUp:
                m_view.MoveHighlightUp();
                moved = true;
                break;
            case InputAction::MoveDown:
                m_view.MoveHighlightDown();
                moved = true;
                break;
            default:
                break;
            }
            if (moved && m_folderViewerStarted) {
                std::string selectedPath = m_view.GetSelectedInfo().absolutePath;
                m_folderViewerMgr->NotifyPathChange(selectedPath);
            }
        }
    }

    // 退出前关闭 FolderViewer
    if (m_folderViewerStarted) {
        m_folderViewerMgr->Shutdown();
    }

    DEBUG_LOG("AppController: Exiting\n");
    return 0;
}