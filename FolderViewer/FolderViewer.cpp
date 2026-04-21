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

// 将 UTF-8 std::string 转换为 std::wstring
std::wstring Utf8ToWide(const std::string& utf8Str) {
    if (utf8Str.empty()) return std::wstring();
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, utf8Str.c_str(), (int)utf8Str.size(), NULL, 0);
    std::wstring wstr(size_needed, 0);
    MultiByteToWideChar(CP_UTF8, 0, utf8Str.c_str(), (int)utf8Str.size(), &wstr[0], size_needed);
    return wstr;
}

std::string GbkToUtf8(const std::string& gbkStr) {
    if (gbkStr.empty()) return std::string();
    // GBK -> UTF-16
    int wlen = MultiByteToWideChar(CP_ACP, 0, gbkStr.c_str(), -1, nullptr, 0);
    std::wstring wstr(wlen, 0);
    MultiByteToWideChar(CP_ACP, 0, gbkStr.c_str(), -1, &wstr[0], wlen);
    // UTF-16 -> UTF-8
    int utf8len = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, nullptr, 0, nullptr, nullptr);
    std::string utf8Str(utf8len, 0);
    WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, &utf8Str[0], utf8len, nullptr, nullptr);
    return utf8Str;
}

std::wstring AnsiToWide(const std::string& ansiStr) {
    if (ansiStr.empty()) return std::wstring();
    int len = MultiByteToWideChar(CP_ACP, 0, ansiStr.c_str(), -1, nullptr, 0);
    std::wstring wstr(len, 0);
    MultiByteToWideChar(CP_ACP, 0, ansiStr.c_str(), -1, &wstr[0], len);
    return wstr;
}

void FolderViewer::OnUpdatePath() {
    char buffer[SHARED_MEM_SIZE];
    memcpy(buffer, m_pSharedView, SHARED_MEM_SIZE);
    buffer[SHARED_MEM_SIZE - 1] = '\0';
    std::string rawPath(buffer);
    m_currentFolderPath = rawPath;
    if (m_currentFolderPath.empty()) return;
    RefreshDisplay(m_currentFolderPath);
}

FolderViewer::FolderViewer()
    : m_hSharedMem(nullptr)
    , m_pSharedView(nullptr)
    , m_hEventUpdatePath(nullptr)
    , m_hEventFolderChanged(nullptr)
    , m_hEventExit(nullptr)
    , m_hSharedMem_OpenLevel(nullptr), m_pSharedView_OpenLevel(nullptr), m_hEvent_OpenLevel(nullptr)
    , m_hSharedMem_OpenBlueprint(nullptr), m_pSharedView_OpenBlueprint(nullptr), m_hEvent_OpenBlueprint(nullptr)
    , m_hSharedMem_OpenText(nullptr), m_pSharedView_OpenText(nullptr), m_hEvent_OpenText(nullptr)
    , m_running(false)
    , m_selectedIndex(-1)
{
    SetConsoleTitleW(L"OFGal_Engine/FolderViewer");

    // 启用虚拟终端模式
    EnableVirtualTerminal();

    // === 调整当前控制台窗口的位置和大小 ===
    HWND hwndConsole = GetConsoleWindow();
    if (hwndConsole) {
        HWND MAX_Window = FindWindow(NULL, L"OFGal_Engine");
        float scaleX = 1920.0f / 2560.0f;
        float scaleY = 1080.0f / 1600.0f;
        if (MAX_Window) {
            RECT rect;
            if (GetWindowRect(MAX_Window, &rect)) {
                int width = rect.right - rect.left;
                int height = rect.bottom - rect.top;
                scaleX = (float)width / 2560.0f;
                scaleY = (float)height / 1600.0f;
            }
        }
        SetWindowPos(hwndConsole, nullptr, 350.0f * scaleX, 1010.0f * scaleY, 1690.0f * scaleX, 470.0f * scaleY, SWP_NOZORDER | SWP_NOACTIVATE);
    }
    m_inputSystem = std::make_unique<InputSystem>();
    m_inputCollector = std::make_unique<InputCollector>(m_inputSystem.get());

    // 注册需要的按键绑定
    // 单键：Delete（边缘触发）
    m_inputCollector->AddBinding({ VK_DELETE, Modifier::None, KeyCode::Delete, true });
    // Enter
    m_inputCollector->AddBinding({ VK_RETURN, Modifier::None, KeyCode::Enter, true });
    // 组合键：Ctrl+N
    m_inputCollector->AddBinding({ 'N', Modifier::Ctrl, KeyCode::CtrlN, true });
    // 鼠标右键（状态变化）
    m_inputCollector->AddBinding({ VK_RBUTTON, Modifier::None, KeyCode::MouseRight, false });

    // 导航键（边缘触发）
    m_inputCollector->AddBinding({ 'W', Modifier::None, KeyCode::W, true });
    m_inputCollector->AddBinding({ 'S', Modifier::None, KeyCode::S, true });
    m_inputCollector->AddBinding({ VK_UP, Modifier::None, KeyCode::Up, true });
    m_inputCollector->AddBinding({ VK_DOWN, Modifier::None, KeyCode::Down, true });

    // Ctrl+S（保留）
    m_inputCollector->AddBinding({ 'S', Modifier::Ctrl, KeyCode::CtrlS, true });

    // 动作映射
    m_actionMap[KeyCode::MouseRight] = [this]() { OnRightClick(); };
    m_actionMap[KeyCode::CtrlN] = [this]() { OnCtrlN(); };
    m_actionMap[KeyCode::Delete] = [this]() { OnDelete(); };
    m_actionMap[KeyCode::Enter] = [this]() { OnEnter(); };
    m_actionMap[KeyCode::W] = [this]() { OnMoveUp(); };
    m_actionMap[KeyCode::Up] = [this]() { OnMoveUp(); };
    m_actionMap[KeyCode::S] = [this]() { OnMoveDown(); };
    m_actionMap[KeyCode::Down] = [this]() { OnMoveDown(); };
}

FolderViewer::~FolderViewer() {
    CloseIPCResources();
}

void FolderViewer::EnableVirtualTerminal() {
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut == INVALID_HANDLE_VALUE) return;

    DWORD dwMode = 0;
    if (!GetConsoleMode(hOut, &dwMode)) return;

    dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    if (SetConsoleMode(hOut, dwMode)) {
        m_virtualTerminalEnabled = true;
    }
    else {
        // 启用失败，后续使用传统 API 高亮
        m_virtualTerminalEnabled = false;
    }
}

void FolderViewer::SetConsoleHighlight(bool enable) {
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut == INVALID_HANDLE_VALUE) return;

    if (m_virtualTerminalEnabled) {
        // VT 模式：直接输出转义序列
        std::cout << (enable ? "\033[7m" : "\033[0m");
    }
    else {
        // 传统模式：使用 Console API
        CONSOLE_SCREEN_BUFFER_INFO csbi;
        if (GetConsoleScreenBufferInfo(hOut, &csbi)) {
            if (enable) {
                // 反色：交换前景色和背景色
                WORD wHighlight = (csbi.wAttributes & 0xF0) >> 4 | (csbi.wAttributes & 0x0F) << 4;
                SetConsoleTextAttribute(hOut, wHighlight);
            }
            else {
                // 恢复原始属性
                SetConsoleTextAttribute(hOut, csbi.wAttributes);
            }
        }
    }
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

    auto OpenBlockAndEvent = [this](const std::string& name, HANDLE& hMem, WCHAR*& pView, HANDLE& hEvent) -> bool {
        std::string memName = MakeGlobalName(name + "Path");
        hMem = OpenFileMappingW(FILE_MAP_WRITE, FALSE, Utf8ToWide(memName).c_str());
        if (!hMem) return false;
        pView = static_cast<WCHAR*>(MapViewOfFile(hMem, FILE_MAP_WRITE, 0, 0, SHARED_MEM_SIZE_LARGE));
        if (!pView) return false;

        std::string evName = MakeGlobalName(name);
        hEvent = OpenEventA(EVENT_MODIFY_STATE, FALSE, evName.c_str());
        if (!hEvent) return false;
        return true;
        };

    if (!OpenBlockAndEvent("OpenLevel", m_hSharedMem_OpenLevel, m_pSharedView_OpenLevel, m_hEvent_OpenLevel)) {
        std::cerr << "Failed to open OpenLevel resources" << std::endl;
        return false;
    }
    if (!OpenBlockAndEvent("OpenBlueprint", m_hSharedMem_OpenBlueprint, m_pSharedView_OpenBlueprint, m_hEvent_OpenBlueprint)) {
        std::cerr << "Failed to open OpenBlueprint resources" << std::endl;
        return false;
    }
    if (!OpenBlockAndEvent("OpenText", m_hSharedMem_OpenText, m_pSharedView_OpenText, m_hEvent_OpenText)) {
        std::cerr << "Failed to open OpenText resources" << std::endl;
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

    auto CloseBlockAndEvent = [](HANDLE& hMem, WCHAR*& pView, HANDLE& hEvent) {
        if (pView) { UnmapViewOfFile(pView); pView = nullptr; }
        if (hMem) { CloseHandle(hMem); hMem = nullptr; }
        if (hEvent) { CloseHandle(hEvent); hEvent = nullptr; }
        };

    CloseBlockAndEvent(m_hSharedMem_OpenLevel, m_pSharedView_OpenLevel, m_hEvent_OpenLevel);
    CloseBlockAndEvent(m_hSharedMem_OpenBlueprint, m_pSharedView_OpenBlueprint, m_hEvent_OpenBlueprint);
    CloseBlockAndEvent(m_hSharedMem_OpenText, m_pSharedView_OpenText, m_hEvent_OpenText);
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
        m_displayItems.clear();
        m_selectedIndex = -1;
        return;
    }

    // 收集目录项
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

    std::sort(folders.begin(), folders.end());
    std::sort(files.begin(), files.end());

    // 构建显示条目列表
    m_displayItems.clear();
    for (const auto& name : folders) {
        DisplayItem item;
        item.name = name;
        item.fullPath = (fs::path(folderPath) / name).string();
        item.isDirectory = true;
        m_displayItems.push_back(item);
    }
    for (const auto& name : files) {
        DisplayItem item;
        item.name = name;
        item.fullPath = (fs::path(folderPath) / name).string();
        item.isDirectory = false;
        m_displayItems.push_back(item);
    }

    // 调整选中索引
    if (m_selectedIndex >= 0 && m_selectedIndex < static_cast<int>(m_displayItems.size())) {
        // 保留原有索引
    }
    else {
        m_selectedIndex = m_displayItems.empty() ? -1 : 0;
    }

    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    int selectedLineY = -1;

    std::cout << "[Folders / Files]" << std::endl;
    for (size_t i = 0; i < m_displayItems.size(); ++i) {
        const auto& item = m_displayItems[i];
        bool selected = (static_cast<int>(i) == m_selectedIndex);

        if (selected && hOut != INVALID_HANDLE_VALUE) {
            CONSOLE_SCREEN_BUFFER_INFO csbi;
            if (GetConsoleScreenBufferInfo(hOut, &csbi)) {
                selectedLineY = csbi.dwCursorPosition.Y;
            }
        }

        SetConsoleHighlight(selected);

        std::cout << "  " << item.name;
        if (item.isDirectory) std::cout << "/";
        std::cout << std::endl;

        SetConsoleHighlight(false);
    }

    std::cout << "----------------------------------------" << std::endl;
    std::cout << "W/Up / S/Down: Navigate | Enter: Open folder" << std::endl;
    std::cout << "Right Click / Ctrl+N / Delete: Action on selected item" << std::endl;

    // 打印当前选中的对象名称（用于验证高亮功能）
    if (m_selectedIndex >= 0 && m_selectedIndex < static_cast<int>(m_displayItems.size())) {
        const auto& item = m_displayItems[m_selectedIndex];
        std::cout << "Selected: " << item.name << (item.isDirectory ? "/" : "") << std::endl;
    }
    else {
        std::cout << "Selected: None" << std::endl;
    }
}

void FolderViewer::MoveSelection(int delta) {
    if (m_displayItems.empty()) return;

    int newIndex = m_selectedIndex + delta;
    if (newIndex < 0) newIndex = 0;
    if (newIndex >= static_cast<int>(m_displayItems.size()))
        newIndex = static_cast<int>(m_displayItems.size()) - 1;

    if (newIndex != m_selectedIndex) {
        m_selectedIndex = newIndex;
        RefreshDisplay(m_currentFolderPath); // 重绘以更新高亮
    }
}

void FolderViewer::OnMoveUp() {
    MoveSelection(-1);
}

void FolderViewer::OnMoveDown() {
    MoveSelection(1);
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

void FolderViewer::OnRightClick() {
    if (m_selectedIndex < 0 || m_selectedIndex >= static_cast<int>(m_displayItems.size())) {
        std::cout << "[Right Click] No item selected." << std::endl;
        return;
    }
    const auto& item = m_displayItems[m_selectedIndex];
    std::cout << "[Right Click] Selected: " << item.fullPath << std::endl;

    // === 1. 定义事件名称 ===
    const std::wstring eventAddEndName = L"Global\\OFGal_Engine_AddItem_AddEnd";
    const std::wstring eventCancelName = L"Global\\OFGal_Engine_AddItem_Cancel";
    const std::wstring eventExitName = L"Global\\OFGal_Engine_AddItem_Exit";

    // === 2. 创建事件 ===
    HANDLE hEventAddEnd = CreateEventW(nullptr, FALSE, FALSE, eventAddEndName.c_str());
    HANDLE hEventCancel = CreateEventW(nullptr, FALSE, FALSE, eventCancelName.c_str());
    HANDLE hEventExit = CreateEventW(nullptr, FALSE, FALSE, eventExitName.c_str());

    if (!hEventAddEnd || !hEventCancel || !hEventExit) {
        std::cerr << "[Right Click] Failed to create events. Error: " << GetLastError() << std::endl;
        if (hEventAddEnd) CloseHandle(hEventAddEnd);
        if (hEventCancel) CloseHandle(hEventCancel);
        if (hEventExit)   CloseHandle(hEventExit);
        return;
    }

    // === 3. 直接从共享内存读取原始 ANSI 路径并转换为宽字符 ===
    char rawPath[SHARED_MEM_SIZE];
    memcpy(rawPath, m_pSharedView, SHARED_MEM_SIZE);
    rawPath[SHARED_MEM_SIZE - 1] = '\0';
    std::string ansiPath(rawPath);
    std::wstring currentDirW = AnsiToWide(ansiPath); // 使用本地代码页转换

    // 构造命令行
    std::wstring commandLine = L"\"" + exePath_AddItem + L"\" \"" + currentDirW + L"\"";

    // === 4. 启动子进程 ===
    STARTUPINFOW si = { sizeof(si) };
    PROCESS_INFORMATION pi = {};

    BOOL processCreated = CreateProcessW(
        exePath_AddItem.c_str(),
        commandLine.data(),
        nullptr, nullptr, FALSE,
        CREATE_NEW_CONSOLE,
        nullptr, nullptr,
        &si, &pi
    );

    if (!processCreated) {
        std::cerr << "[Right Click] Failed to launch AddItem.exe. Error: " << GetLastError() << std::endl;
        CloseHandle(hEventAddEnd);
        CloseHandle(hEventCancel);
        CloseHandle(hEventExit);
        return;
    }

    CloseHandle(pi.hThread);

    // === 5. 等待事件 ===
    HANDLE waitHandles[3] = { hEventAddEnd, hEventCancel, pi.hProcess };
    DWORD waitResult = WaitForMultipleObjects(3, waitHandles, FALSE, INFINITE);

    bool shouldRefresh = false;
    if (waitResult == WAIT_OBJECT_0) {
        std::cout << "[Right Click] AddItem completed (AddEnd)." << std::endl;
        shouldRefresh = true;
    }
    else if (waitResult == WAIT_OBJECT_0 + 1) {
        std::cout << "[Right Click] AddItem cancelled by user." << std::endl;
    }
    else if (waitResult == WAIT_OBJECT_0 + 2) {
        std::cout << "[Right Click] AddItem.exe terminated without signaling." << std::endl;
    }
    else {
        std::cerr << "[Right Click] Wait failed. Error: " << GetLastError() << std::endl;
    }

    // === 6. 通知退出 ===
    SetEvent(hEventExit);
    WaitForSingleObject(pi.hProcess, 1000);

    // === 7. 清理 ===
    CloseHandle(pi.hProcess);
    CloseHandle(hEventAddEnd);
    CloseHandle(hEventCancel);
    CloseHandle(hEventExit);

    if (shouldRefresh) {
        NotifyParentFolderChanged(m_currentFolderPath);
    }
}

void FolderViewer::OnCtrlN() {
    if (m_selectedIndex < 0 || m_selectedIndex >= static_cast<int>(m_displayItems.size())) {
        std::cout << "[Ctrl+N] No item selected." << std::endl;
        return;
    }
    const auto& item = m_displayItems[m_selectedIndex];
    std::cout << "[Ctrl+N] Selected: " << item.fullPath << std::endl;

    // 获取父目录路径
    fs::path parentPath = fs::path(item.fullPath).parent_path();
    if (parentPath.empty()) {
        std::cerr << "[Ctrl+N] Cannot get parent directory." << std::endl;
        return;
    }

    // TODO: 弹出获取名称的对话框
    std::string newFolderName = "";
    std::cout << "============================" << std::endl;
    std::cout << "Please enter the name of the new folder:" << std::endl;
    std::cin >> newFolderName;
    if (newFolderName.empty()) {
        std::cerr << "[Ctrl+N] Folder name cannot be empty." << std::endl;
        return;
    }

    fs::path newFolderPath = parentPath / newFolderName;
    std::error_code ec;
    if (fs::create_directory(newFolderPath, ec)) {
        std::cout << "[Ctrl+N] Created folder: " << newFolderPath.string() << std::endl;
        // 如果当前显示的文件夹就是父目录，则刷新视图
        if (fs::equivalent(parentPath, m_currentFolderPath, ec)) {
            RefreshDisplay(m_currentFolderPath);
            NotifyParentFolderChanged(m_currentFolderPath);
        }
    }
    else {
        std::cerr << "[Ctrl+N] Failed to create folder: " << ec.message() << std::endl;
    }
}

void FolderViewer::OnDelete() {
    if (m_selectedIndex < 0 || m_selectedIndex >= static_cast<int>(m_displayItems.size())) {
        std::cout << "[Delete] No item selected." << std::endl;
        return;
    }

    const auto& item = m_displayItems[m_selectedIndex];
    std::cout << "[Delete] Requesting confirmation for: " << item.fullPath << std::endl;

    // === 1. 定义 IPC 对象名称（宽字符版本） ===
    const std::wstring sharedMemName = L"Global\\OFGal_Engine_YesOrNo_SharedMem";
    const std::wstring eventGetResultName = L"Global\\OFGal_Engine_YesOrNo_GetResult";
    const std::wstring eventExitName = L"Global\\OFGal_Engine_YesOrNo_Exit";

    // === 2. 创建两个事件 ===
    HANDLE hEventGetResult = CreateEventW(nullptr, FALSE, FALSE, eventGetResultName.c_str());
    HANDLE hEventExit = CreateEventW(nullptr, FALSE, FALSE, eventExitName.c_str());

    if (!hEventGetResult || !hEventExit) {
        std::cerr << "[Delete] Failed to create events. Error: " << GetLastError() << std::endl;
        if (hEventGetResult) CloseHandle(hEventGetResult);
        if (hEventExit) CloseHandle(hEventExit);
        return;
    }

    // === 3. 创建共享内存 ===
    HANDLE hSharedMem = CreateFileMappingW(
        INVALID_HANDLE_VALUE,
        nullptr,
        PAGE_READWRITE,
        0,
        sizeof(YesNoDialogData),
        sharedMemName.c_str()
    );

    if (!hSharedMem) {
        std::cerr << "[Delete] Failed to create shared memory. Error: " << GetLastError() << std::endl;
        CloseHandle(hEventGetResult);
        CloseHandle(hEventExit);
        return;
    }

    YesNoDialogData* pSharedData = static_cast<YesNoDialogData*>(
        MapViewOfFile(hSharedMem, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(YesNoDialogData))
        );

    if (!pSharedData) {
        std::cerr << "[Delete] Failed to map shared memory. Error: " << GetLastError() << std::endl;
        CloseHandle(hSharedMem);
        CloseHandle(hEventGetResult);
        CloseHandle(hEventExit);
        return;
    }

    // === 4. 初始化共享数据（仅支持英文提示消息） ===
    std::wstring confirmMsg = L"Are you sure you want to delete " + AnsiToWide(item.name) + L"\" ?";
    wcsncpy_s(pSharedData->message, 300, confirmMsg.c_str(), _TRUNCATE);
    pSharedData->result = FALSE;

    // === 5. 启动子进程 YesOrNo.exe ===
    STARTUPINFOW si = { sizeof(si) };
    PROCESS_INFORMATION pi = {};

    BOOL processCreated = CreateProcessW(
        exePath_YesOrNo.c_str(),
        nullptr,                 // 命令行参数
        nullptr,                 // 进程安全属性
        nullptr,                 // 线程安全属性
        FALSE,                   // 不继承句柄
        CREATE_NEW_CONSOLE,      // 创建标志
        nullptr,                 // 环境变量
        nullptr,                 // 当前目录
        &si,
        &pi
    );

    if (!processCreated) {
        std::cerr << "[Delete] Failed to launch YesOrNo.exe. Error: " << GetLastError() << std::endl;
        UnmapViewOfFile(pSharedData);
        CloseHandle(hSharedMem);
        CloseHandle(hEventGetResult);
        CloseHandle(hEventExit);
        return;
    }

    // 不再需要子进程的主线程句柄
    CloseHandle(pi.hThread);

    // === 6. 等待子进程结束 或 GetResult 事件 ===
    HANDLE waitHandles[2] = { pi.hProcess, hEventGetResult };
    DWORD waitResult = WaitForMultipleObjects(2, waitHandles, FALSE, INFINITE);

    bool userConfirmed = false;

    if (waitResult == WAIT_OBJECT_0 + 1) {
        // GetResult 事件被触发，读取结果
        userConfirmed = (pSharedData->result != FALSE);
        std::cout << "[Delete] User " << (userConfirmed ? "confirmed" : "cancelled") << " the operation." << std::endl;
    }
    else if (waitResult == WAIT_OBJECT_0) {
        // 子进程自行退出（可能是异常关闭）
        std::cout << "[Delete] YesOrNo.exe terminated without sending result." << std::endl;
    }
    else {
        std::cerr << "[Delete] Wait failed. Error: " << GetLastError() << std::endl;
    }

    // === 7. 通知子进程退出 ===
    SetEvent(hEventExit);

    // 等待子进程真正结束
    WaitForSingleObject(pi.hProcess, 1000);

    // === 8. 清理资源 ===
    CloseHandle(pi.hProcess);
    UnmapViewOfFile(pSharedData);
    CloseHandle(hSharedMem);
    CloseHandle(hEventGetResult);
    CloseHandle(hEventExit);

    // === 9. 根据用户确认执行删除 ===
    if (userConfirmed) {
        try {
            std::filesystem::remove_all(item.fullPath);
            std::cout << "[Delete] Successfully deleted: " << item.fullPath << std::endl;
            RefreshDisplay(m_currentFolderPath);
            NotifyParentFolderChanged(m_currentFolderPath);
        }
        catch (const std::filesystem::filesystem_error& e) {
            std::cerr << "[Delete] Failed to delete: " << e.what() << std::endl;
        }
    }
    else {
        std::cout << "[Delete] Deletion cancelled by user." << std::endl;
    }
}

void FolderViewer::OnEnter() {
    if (m_selectedIndex < 0 || m_selectedIndex >= static_cast<int>(m_displayItems.size())) {
        std::cout << "[Enter] No item selected." << std::endl;
        return;
    }

    const auto& item = m_displayItems[m_selectedIndex];
    if (item.isDirectory) {
        // 进入文件夹
        m_currentFolderPath = item.fullPath;
        RefreshDisplay(m_currentFolderPath);
        NotifyParentFolderChanged(m_currentFolderPath);
        std::cout << "[Enter] Entered folder: " << item.fullPath << std::endl;
    }
    else {
        fs::path filePath(item.fullPath);
        std::string ext = filePath.extension().string();
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

        auto NotifyOpen = [&](WCHAR* pView, HANDLE hEvent, const std::string& typeName) {
            std::wstring widePath = AnsiToWide(item.fullPath);
            // 安全拷贝，留出结尾空字符
            wcsncpy_s(pView, SHARED_MEM_SIZE_LARGE / sizeof(WCHAR), widePath.c_str(), _TRUNCATE);
            SetEvent(hEvent);
            std::cout << "[Enter] Notified parent to open " << typeName << ": " << item.fullPath << std::endl;
            };

        if (ext == ".level") {
            NotifyOpen(m_pSharedView_OpenLevel, m_hEvent_OpenLevel, "Level");
        }
        else if (ext == ".bp") {
            NotifyOpen(m_pSharedView_OpenBlueprint, m_hEvent_OpenBlueprint, "Blueprint");
        }
        else if (ext == ".txt" || ext == ".md") {
            NotifyOpen(m_pSharedView_OpenText, m_hEvent_OpenText, "Text");
        }
        else {
            std::cout << "[Enter] Unsupported file type: " << item.fullPath << std::endl;
        }
    }
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

    char buffer[SHARED_MEM_SIZE];
    memcpy(buffer, m_pSharedView, SHARED_MEM_SIZE);
    buffer[SHARED_MEM_SIZE - 1] = '\0';
    std::string rawPath(buffer);
    m_currentFolderPath = rawPath;  // 添加编码转换
    if (!m_currentFolderPath.empty()) {
        RefreshDisplay(m_currentFolderPath);
    }

    m_running = true;
    MainLoop();
    CloseIPCResources();
    return 0;
}