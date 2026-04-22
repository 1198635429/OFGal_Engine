// Copyright 2026 Nagato-Yuki-708. All Rights Reserved.
#include "LevelTreeList.h"
#include "Json_LevelData_ReadWrite.h"
#include <iostream>
#include <string>

// 全局输入系统实例（定义在 InputSystem.cpp）
extern InputSystem g_inputSystem;

// 辅助函数：将宽字符串转换为 UTF-8 字符串
static std::string WstrToUtf8(const std::wstring& wstr) {
    if (wstr.empty()) return std::string();
    int size_needed = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), (int)wstr.size(), NULL, 0, NULL, NULL);
    std::string strTo(size_needed, 0);
    WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), (int)wstr.size(), &strTo[0], size_needed, NULL, NULL);
    return strTo;
}

LevelTreeList::LevelTreeList()
    : m_running(false)
    , m_hEvent(NULL)
    , m_hMapFile(NULL)
    , m_pView(nullptr)
    , m_inputSystem(&g_inputSystem)
    , m_loopInterval(std::chrono::milliseconds(20))
    , m_selectedIndex(0)
    , m_needRender(false)
{
}
LevelTreeList::~LevelTreeList() {
    CloseIPC();
}

void LevelTreeList::ClearScreen() {
    // \033[2J 清屏，\033[H 光标归位到左上角
    std::cout << "\033[2J\033[H";
}
void LevelTreeList::RecursiveBuild(const std::map<std::string, ObjectData*>& children,
    int depth, const std::string& prefix) {
    // 获取子项数量
    size_t count = children.size();
    size_t idx = 0;
    for (const auto& pair : children) {
        ++idx;
        bool isLast = (idx == count);
        const std::string& objName = pair.first;
        ObjectData* obj = pair.second;

        // 构造当前节点的分支符号
        std::string branch = isLast ? "`-- " : "|-- ";
        std::string line = prefix + branch + objName;

        // 计算传递给下一层的缩进前缀
        std::string childPrefix = prefix + (isLast ? "    " : "|   ");

        // 添加到显示列表
        m_displayNodes.push_back({ line, prefix + branch, objName, obj, depth });

        // 递归处理子对象
        if (!obj->objects.empty()) {
            RecursiveBuild(obj->objects, depth + 1, childPrefix);
        }
    }
}
void LevelTreeList::BuildDisplayList() {
    m_displayNodes.clear();

    if (!m_currentLevel) return;

    // 场景作为根节点（深度 0，无前缀）
    m_displayNodes.push_back({ m_currentLevel->name, "", m_currentLevel->name, nullptr, 0 });

    // 递归添加场景的直接子对象（深度 1，无前缀）
    RecursiveBuild(m_currentLevel->objects, 1, "");
}
void LevelTreeList::RenderTree() {
    ClearScreen();

    if (m_displayNodes.empty()) {
        std::cout << "\033[37mNo level loaded.\033[0m" << std::endl;
        return;
    }

    // 打印操作提示
    std::cout << "\033[36mA/D: select previous/next  F: more operations\033[0m\n" << std::endl;

    // 遍历显示节点
    for (size_t i = 0; i < m_displayNodes.size(); ++i) {
        const auto& node = m_displayNodes[i];

        // 选中项使用反色高亮
        if (static_cast<int>(i) == m_selectedIndex) {
            std::cout << "\033[7m";  // 反转前景/背景色
        }

        // 树形线条与分支用青色，名称用白色
        std::cout << "\033[36m" << node.prefix << "\033[37m" << node.name << "\033[0m\n";
    }
}

void LevelTreeList::ConfigureConsole() {
    // 启用虚拟终端处理
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut != INVALID_HANDLE_VALUE) {
        DWORD dwMode = 0;
        if (GetConsoleMode(hOut, &dwMode)) {
            dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
            SetConsoleMode(hOut, dwMode);
        }
    }

    // 禁用快速编辑模式
    HANDLE hIn = GetStdHandle(STD_INPUT_HANDLE);
    if (hIn != INVALID_HANDLE_VALUE) {
        DWORD dwMode = 0;
        if (GetConsoleMode(hIn, &dwMode)) {
            dwMode &= ~(ENABLE_QUICK_EDIT_MODE | ENABLE_INSERT_MODE);
            dwMode |= ENABLE_EXTENDED_FLAGS;
            SetConsoleMode(hIn, dwMode);
        }
    }
}

void LevelTreeList::SetWindowSizeAndPosition() {
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
        SetWindowPos(hwndConsole, nullptr,
            static_cast<int>(2040.0f * scaleX),
            static_cast<int>(150.0f * scaleY),
            static_cast<int>(520.0f * scaleX),
            static_cast<int>(500.0f * scaleY),
            SWP_NOZORDER | SWP_NOACTIVATE);
    }
}

bool LevelTreeList::OpenIPC() {
    const std::wstring eventName = L"Global\\OFGal_Engine_LevelTreeList_PathUpdate";

    const std::wstring sharedMemName =
        L"Global\\OFGal_Engine_ProjectStructureViewer_"
        L"FolderViewer_OpenLevelPath";

    // 1. 打开事件
    m_hEvent = OpenEventW(EVENT_ALL_ACCESS, FALSE, eventName.c_str());
    if (m_hEvent == NULL) {
        DWORD err = GetLastError();
        WCHAR buf[512];
        swprintf_s(buf, L"[LevelTreeList] Failed to open event: %s, error: %lu\n", eventName.c_str(), err);
        OutputDebugStringW(buf);
        if (err == ERROR_FILE_NOT_FOUND) {
            OutputDebugStringW(L"[LevelTreeList] Event object not found. Ensure main program is running.\n");
        }
        return false;
    }
    WCHAR buf[256];
    swprintf_s(buf, L"[LevelTreeList] Event opened successfully. Handle: %p\n", m_hEvent);
    OutputDebugStringW(buf);

    // 2. 打开共享内存
    m_hMapFile = OpenFileMappingW(FILE_MAP_READ, FALSE, sharedMemName.c_str());
    if (m_hMapFile == NULL) {
        DWORD err = GetLastError();
        WCHAR buf[512];
        swprintf_s(buf, L"[LevelTreeList] Failed to open shared memory: %s, error: %lu\n", sharedMemName.c_str(), err);
        OutputDebugStringW(buf);
        CloseHandle(m_hEvent);
        m_hEvent = NULL;
        return false;
    }
    swprintf_s(buf, L"[LevelTreeList] Shared memory opened successfully. Handle: %p\n", m_hMapFile);
    OutputDebugStringW(buf);

    // 3. 映射视图
    m_pView = MapViewOfFile(m_hMapFile, FILE_MAP_READ, 0, 0, 0);
    if (m_pView == NULL) {
        WCHAR buf[128];
        swprintf_s(buf, L"[LevelTreeList] Failed to map view, error: %lu\n", GetLastError());
        OutputDebugStringW(buf);
        CloseHandle(m_hMapFile);
        m_hMapFile = NULL;
        CloseHandle(m_hEvent);
        m_hEvent = NULL;
        return false;
    }
    swprintf_s(buf, L"[LevelTreeList] Shared memory view mapped. Address: %p\n", m_pView);
    OutputDebugStringW(buf);

    return true;
}

void LevelTreeList::CloseIPC() {
    if (m_pView) {
        UnmapViewOfFile(m_pView);
        m_pView = nullptr;
    }
    if (m_hMapFile) {
        CloseHandle(m_hMapFile);
        m_hMapFile = NULL;
    }
    if (m_hEvent) {
        CloseHandle(m_hEvent);
        m_hEvent = NULL;
    }
}

bool LevelTreeList::Initialize() {
    // 配置控制台
    ConfigureConsole();
    SetConsoleTitleW(L"OFGal_Engine/LevelTreeList");
    SetWindowSizeAndPosition();

    OutputDebugStringW(L"[LevelTreeList] Started. Initializing IPC...\n");

    if (!OpenIPC()) {
        system("pause");
        return false;
    }

    // 初始化输入收集器
    m_inputCollector = std::make_unique<InputCollector>(m_inputSystem);
    m_inputCollector->AddBinding({ 'A', Modifier::None, KeyCode::A, true });
    m_inputCollector->AddBinding({ 'D', Modifier::None, KeyCode::D, true });
    m_inputCollector->AddBinding({ 'F', Modifier::None, KeyCode::F, true });

    OutputDebugStringW(L"[LevelTreeList] Input bindings configured. Entering main loop...\n");

    m_running = true;
    return true;
}

void LevelTreeList::HandleOpenLevelEvent() {
    OutputDebugStringW(L"[LevelTreeList] OpenLevel event triggered!\n");

    const WCHAR* path = static_cast<const WCHAR*>(m_pView);
    if (path == nullptr || wcslen(path) == 0) {
        OutputDebugStringW(L"[LevelTreeList] ERROR: Shared memory path is empty or null.\n");
        return;
    }

    std::wstring openedLevelPath(path);
    std::wstring msg = L"[LevelTreeList] Path from shared memory: " + openedLevelPath + L"\n";
    OutputDebugStringW(msg.c_str());

    // 转换为 UTF-8 并读取 .level 文件
    std::string utf8Path = WstrToUtf8(openedLevelPath);
    try {
        LevelData level = ReadLevelData(utf8Path);
        m_currentLevel = std::make_unique<LevelData>(std::move(level));
        std::string countMsg = "Level loaded successfully. Object count: " + std::to_string(m_currentLevel->objects.size()) + "\n";
        OutputDebugStringA(countMsg.c_str());

        BuildDisplayList();
        m_selectedIndex = 0;
        m_needRender = true;
        RenderTree();
    }
    catch (const std::exception& e) {
        std::string errMsg = "Failed to read level file: " + std::string(e.what()) + "\n";
        OutputDebugStringA(errMsg.c_str());
    }

    if (m_hEvent) {
        ResetEvent(m_hEvent);
    }
}

void LevelTreeList::ProcessInputEvents() {
    const auto& events = m_inputSystem->getEvents();
    bool selectionChanged = false;

    for (const auto& ev : events) {
        if (ev.type == InputType::KeyDown) {
            switch (ev.key) {
            case KeyCode::A:
                if (m_selectedIndex > 0) {
                    --m_selectedIndex;
                    selectionChanged = true;
                }
                break;
            case KeyCode::D:
                if (m_selectedIndex < static_cast<int>(m_displayNodes.size()) - 1) {
                    ++m_selectedIndex;
                    selectionChanged = true;
                }
                break;
            case KeyCode::F:
                // 更多操作：暂时打印选中对象信息
                if (m_selectedIndex >= 0 && m_selectedIndex < static_cast<int>(m_displayNodes.size())) {
                    const auto& node = m_displayNodes[m_selectedIndex];
                    std::string msg = "[LevelTreeList] More ops on: " + node.name + "\n";
                    OutputDebugStringA(msg.c_str());
                    // TODO: 在此处添加具体操作（例如弹出属性对话框）
                }
                break;
            default:
                break;
            }
        }
    }

    if (selectionChanged) {
        RenderTree();
    }
}

void LevelTreeList::Run() {
    OutputDebugStringW(L"[LevelTreeList] Entering main loop.\n");

    // 检查事件句柄
    if (m_hEvent == NULL || m_hEvent == INVALID_HANDLE_VALUE) {
        OutputDebugStringW(L"[LevelTreeList] ERROR: Event handle is invalid!\n");
        return;
    }

    // 初始状态检查（调试用）
    DWORD initialWait = WaitForSingleObject(m_hEvent, 0);
    WCHAR buf[128];
    swprintf_s(buf, L"[LevelTreeList] Initial event state: %lu (0=signaled, 258=timeout, 0xFFFFFFFF=error)\n", initialWait);
    OutputDebugStringW(buf);

    while (m_running) {
        // 1. 检查 IPC 事件（非阻塞）
        DWORD waitResult = WaitForSingleObject(m_hEvent, 0);
        if (waitResult == WAIT_OBJECT_0) {
            OutputDebugStringW(L"[LevelTreeList] Event signaled!\n");
            HandleOpenLevelEvent();
        }
        else if (waitResult == WAIT_TIMEOUT) {
            // 正常超时，不输出（避免刷屏）
        }
        else {
            WCHAR errBuf[128];
            swprintf_s(errBuf, L"[LevelTreeList] WaitForSingleObject error: %lu, handle: %p\n", GetLastError(), m_hEvent);
            OutputDebugStringW(errBuf);
            m_running = false;
            break;
        }

        // 2. 更新输入收集器
        m_inputCollector->update();

        // 3. 处理输入事件
        ProcessInputEvents();

        // 4. 清除本帧事件
        m_inputSystem->clearEvent();

        // 5. 休眠 20ms
        std::this_thread::sleep_for(m_loopInterval);
    }

    OutputDebugStringW(L"[LevelTreeList] Exited cleanly.\n");
}

void LevelTreeList::Stop() {
    m_running = false;
}