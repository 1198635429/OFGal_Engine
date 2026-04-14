// Copyright 2026 MrSeagull. All Rights Reserved.
#include "FolderViewer.h"
#include "InputSystem.h"
#include "InputCollector.h"
#include "InputEvent.h"
#include "KeyCode.h"
#include "SharedTypes.h"
#include <filesystem>
#include <iostream>
#include <sstream>
#include <vector>
#include <algorithm>
#include <chrono>
#include <thread>

namespace fs = std::filesystem;

// 辅助：构建全局对象名称
static std::string MakeGlobalName(const std::string& suffix) {
    return "Global\\OFGal_Engine_ProjectStructureViewer_FolderViewer_" + suffix;
}

FolderViewer::FolderViewer()
    : m_hSharedMem(nullptr)
    , m_pSharedView(nullptr)
    , m_hEventUpdatePath(nullptr)
    , m_hEventFolderChanged(nullptr)
    , m_hEventExit(nullptr)
    , m_running(false)
{
    screenWidth = GetSystemMetrics(SM_CXSCREEN);
    RECT rcWork;
    if (SystemParametersInfo(SPI_GETWORKAREA, 0, &rcWork, 0)) {
        workAreaHeight = rcWork.bottom - rcWork.top;
    }
    else {
        workAreaHeight = GetSystemMetrics(SM_CYSCREEN); // 降级方案
    }

    // === 调整当前控制台窗口的位置和大小 ===
    HWND hConsoleWnd = GetConsoleWindow();
    if (hConsoleWnd) {
        int windowX = double(double(screenWidth) * 0.1374);
        int windowY = double(double(workAreaHeight) * 0.7);
        int windowWidth = double(double(screenWidth) * 0.6613);
        int windowHeight = double(workAreaHeight) - double(double(workAreaHeight) * 0.7);

        SetWindowPos(hConsoleWnd, nullptr,
            windowX, windowY,
            windowWidth, windowHeight,
            SWP_NOZORDER | SWP_NOACTIVATE);
    }
    m_inputSystem = std::make_unique<InputSystem>();
    m_inputCollector = std::make_unique<InputCollector>(m_inputSystem.get());

    // 注册需要的按键绑定
    // 单键：Delete（边缘触发，只关心按下瞬间）
    m_inputCollector->AddBinding({ VK_DELETE, Modifier::None, KeyCode::Delete, true });

    m_inputCollector->AddBinding({ VK_RETURN, Modifier::None, KeyCode::Enter, true });
    //m_inputCollector->AddBinding({ VK_NUMPAD_ENTER, Modifier::None, KeyCode::Enter, true });

    // 组合键：Ctrl+N（边缘触发）
    m_inputCollector->AddBinding({ 'N', Modifier::Ctrl, KeyCode::CtrlN, true });

    // 鼠标右键（边缘或状态均可，这里用状态变化以支持按下/释放事件）
    m_inputCollector->AddBinding({ VK_RBUTTON, Modifier::None, KeyCode::MouseRight, false });

    // 其他可选绑定，可根据需要添加，例如鼠标左键、中键、W 等
    m_inputCollector->AddBinding({ 'W', Modifier::None, KeyCode::W, false });
    // 若还需 Ctrl+S，可继续添加
    m_inputCollector->AddBinding({ 'S', Modifier::Ctrl, KeyCode::CtrlS, true });

    m_actionMap[KeyCode::MouseRight] = [this]() { OnRightClick(); };
    m_actionMap[KeyCode::CtrlN] = [this]() { OnCtrlN(); };
    m_actionMap[KeyCode::Delete] = [this]() { OnDelete(); };
    m_actionMap[KeyCode::Enter] = [this]() { OnEnter(); };
}

FolderViewer::~FolderViewer() {
    CloseIPCResources();
}

bool FolderViewer::OpenIPCResources() {
    // 打开共享内存
    std::string memName = MakeGlobalName("Path");
    m_hSharedMem = OpenFileMappingA(FILE_MAP_WRITE, FALSE, memName.c_str());
    if (!m_hSharedMem) {
        std::cerr << "FolderViewer: OpenFileMapping failed, error=" << GetLastError() << std::endl;
        return false;
    }

    m_pSharedView = static_cast<char*>(MapViewOfFile(m_hSharedMem, FILE_MAP_WRITE, 0, 0, SHARED_MEM_SIZE));
    if (!m_pSharedView) {
        std::cerr << "FolderViewer: MapViewOfFile failed, error=" << GetLastError() << std::endl;
        return false;
    }

    // 打开事件
    std::string updateName = MakeGlobalName("UpdatePath");
    m_hEventUpdatePath = OpenEventA(SYNCHRONIZE, FALSE, updateName.c_str());
    if (!m_hEventUpdatePath) {
        std::cerr << "FolderViewer: OpenEvent(UpdatePath) failed, error=" << GetLastError() << std::endl;
        return false;
    }

    std::string changeName = MakeGlobalName("FolderChanged");
    m_hEventFolderChanged = OpenEventA(EVENT_MODIFY_STATE, FALSE, changeName.c_str());
    if (!m_hEventFolderChanged) {
        std::cerr << "FolderViewer: OpenEvent(FolderChanged) failed, error=" << GetLastError() << std::endl;
        return false;
    }

    std::string exitName = MakeGlobalName("Exit");
    m_hEventExit = OpenEventA(SYNCHRONIZE, FALSE, exitName.c_str());
    if (!m_hEventExit) {
        std::cerr << "FolderViewer: OpenEvent(Exit) failed, error=" << GetLastError() << std::endl;
        return false;
    }

    return true;
}

void FolderViewer::CloseIPCResources() {
    if (m_pSharedView) {
        UnmapViewOfFile(m_pSharedView);
        m_pSharedView = nullptr;
    }
    if (m_hSharedMem) {
        CloseHandle(m_hSharedMem);
        m_hSharedMem = nullptr;
    }
    if (m_hEventUpdatePath) {
        CloseHandle(m_hEventUpdatePath);
        m_hEventUpdatePath = nullptr;
    }
    if (m_hEventFolderChanged) {
        CloseHandle(m_hEventFolderChanged);
        m_hEventFolderChanged = nullptr;
    }
    if (m_hEventExit) {
        CloseHandle(m_hEventExit);
        m_hEventExit = nullptr;
    }
}

void FolderViewer::ClearScreen() {
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    DWORD count;
    DWORD cellCount;
    COORD homeCoords = { 0, 0 };

    if (hConsole == INVALID_HANDLE_VALUE) return;

    if (!GetConsoleScreenBufferInfo(hConsole, &csbi)) return;
    cellCount = csbi.dwSize.X * csbi.dwSize.Y;

    if (!FillConsoleOutputCharacter(hConsole, ' ', cellCount, homeCoords, &count)) return;
    if (!FillConsoleOutputAttribute(hConsole, csbi.wAttributes, cellCount, homeCoords, &count)) return;

    SetConsoleCursorPosition(hConsole, homeCoords);
}

void FolderViewer::RefreshDisplay(const std::string& folderPath) {
    ClearScreen();
    std::cout << "Current folder: " << folderPath << std::endl;
    std::cout << "----------------------------------------" << std::endl;

    std::error_code ec;
    if (!fs::exists(folderPath, ec) || !fs::is_directory(folderPath, ec)) {
        std::cout << "[Invalid directory]" << std::endl;
        return;
    }

    // 列出目录内容（仅直接子项）
    std::vector<std::string> folders;
    std::vector<std::string> files;

    for (const auto& entry : fs::directory_iterator(folderPath, ec)) {
        if (ec) break;
        if (entry.is_directory(ec)) {
            folders.push_back(entry.path().filename().string());
        }
        else if (entry.is_regular_file(ec)) {
            files.push_back(entry.path().filename().string());
        }
    }

    // 排序
    std::sort(folders.begin(), folders.end());
    std::sort(files.begin(), files.end());

    // 输出
    std::cout << "[Folders]" << std::endl;
    for (const auto& name : folders) {
        std::cout << "  " << name << "/" << std::endl;
    }
    std::cout << "[Files]" << std::endl;
    for (const auto& name : files) {
        std::cout << "  " << name << std::endl;
    }
    std::cout << "----------------------------------------" << std::endl;
    std::cout << "Right Click / Ctrl+N / Delete to trigger actions." << std::endl;
}

void FolderViewer::OnUpdatePath() {
    // 从共享内存读取新路径
    char buffer[SHARED_MEM_SIZE];
    memcpy(buffer, m_pSharedView, SHARED_MEM_SIZE);
    buffer[SHARED_MEM_SIZE - 1] = '\0';
    std::string newPath(buffer);

    if (newPath.empty()) return;

    m_currentFolderPath = newPath;
    RefreshDisplay(m_currentFolderPath);
}

void FolderViewer::OnExit() {
    m_running = false;
}

void FolderViewer::NotifyParentFolderChanged(const std::string& newPath) {
    if (!m_pSharedView || !m_hEventFolderChanged) return;

    // 写入新路径到共享内存
    size_t len = newPath.size() + 1;
    if (len > SHARED_MEM_SIZE) len = SHARED_MEM_SIZE;
    memcpy(m_pSharedView, newPath.c_str(), len);
    m_pSharedView[SHARED_MEM_SIZE - 1] = '\0';

    // 触发文件夹变更事件
    SetEvent(m_hEventFolderChanged);
}

// 占位回调函数，末尾调用 NotifyParentFolderChanged
void FolderViewer::OnRightClick() {
    // TODO: 实现右键功能
    std::cout << "[Right Click] triggered." << std::endl;

    // 示例：通知父进程路径改为当前文件夹（你可替换为实际需要的内容）
    NotifyParentFolderChanged(m_currentFolderPath);
}

void FolderViewer::OnCtrlN() {
    // TODO: 实现 Ctrl+N 功能
    std::cout << "[Ctrl+N] triggered." << std::endl;

    // 示例：通知父进程路径改为当前文件夹
    NotifyParentFolderChanged(m_currentFolderPath);
}

void FolderViewer::OnDelete() {
    // TODO: 实现 Delete 功能
    std::cout << "[Delete] triggered." << std::endl;

    // 示例：通知父进程路径改为当前文件夹
    NotifyParentFolderChanged(m_currentFolderPath);
}

void FolderViewer::OnEnter() {
    // TODO: 实现按下 Enter 后的具体逻辑
    std::cout << "[Enter] triggered on folder: " << m_currentFolderPath << std::endl;

    // 示例：进入当前选中的文件夹（需结合您自己的选中逻辑）
    // 这里仅为示意，实际可能需要读取用户选中的条目
    // NotifyParentFolderChanged(m_currentFolderPath + "\\selected_subfolder");
    NotifyParentFolderChanged(m_currentFolderPath);
}

void FolderViewer::MainLoop() {
    HANDLE events[2] = { m_hEventExit, m_hEventUpdatePath };
    const DWORD pollIntervalMs = 100;

    while (m_running) {
        // 1. 等待 IPC 事件（超时 100ms，以便轮询输入）
        DWORD result = WaitForMultipleObjects(2, events, FALSE, pollIntervalMs);

        if (result == WAIT_OBJECT_0) {
            // Exit 事件
            OnExit();
            break;
        }
        else if (result == WAIT_OBJECT_0 + 1) {
            // UpdatePath 事件
            OnUpdatePath();
        }

        // 2. 收集输入事件（非阻塞）
        m_inputCollector->update();

        // 3. 处理输入事件
        const auto& inputEvents = m_inputSystem->getEvents();
        for (const auto& ev : inputEvents) {
            if (ev.type == InputType::KeyDown) {
                auto it = m_actionMap.find(ev.key);
                if (it != m_actionMap.end()) {
                    it->second();  // 执行对应函数
                }
            }
        }

        // 4. 清除本帧输入事件
        m_inputSystem->clearEvent();
    }
}

int FolderViewer::Run() {
    if (!OpenIPCResources()) {
        std::cerr << "Failed to open IPC resources. Exiting." << std::endl;
        return -1;
    }

    // 读取初始路径并显示
    char buffer[SHARED_MEM_SIZE];
    memcpy(buffer, m_pSharedView, SHARED_MEM_SIZE);
    buffer[SHARED_MEM_SIZE - 1] = '\0';
    m_currentFolderPath = buffer;
    if (!m_currentFolderPath.empty()) {
        RefreshDisplay(m_currentFolderPath);
    }

    m_running = true;
    MainLoop();

    CloseIPCResources();
    return 0;
}