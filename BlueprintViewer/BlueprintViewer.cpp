// Copyright 2026 Nagato-Yuki-708. All Rights Reserved.
#include "BlueprintViewer.h"

BlueprintViewer::BlueprintViewer() {
    SetConsoleTitleW(L"OFGal_Engine/BlueprintViewer");
    ConfigureConsole();
    SetWindowSizeAndPosition();

    // ---------- 初始化同步对象 ----------
    hLoadBPEvent = OpenEventW(
        EVENT_MODIFY_STATE | SYNCHRONIZE,
        FALSE,
        L"Global\\OFGal_Engine_BlueprintViewer_LoadBP");
    if (!hLoadBPEvent) {
        DWORD err = GetLastError();
        DEBUG_W(L"[BlueprintViewer] OpenEvent LoadBP Failed, error=" << err << L"\n");
    }

    hFileMapping = OpenFileMappingW(
        FILE_MAP_READ,
        FALSE,
        L"Global\\OFGal_Engine_BlueprintViewer_BlueprintPath");
    if (hFileMapping) {
        pSharedMem = MapViewOfFile(hFileMapping, FILE_MAP_READ, 0, 0, 0);
        if (!pSharedMem) {
            DEBUG_W(L"[BlueprintViewer] MapViewOfFile Failed" << L"\n");
        }
    }
    else {
        pSharedMem = nullptr;
        DEBUG_W(L"[BlueprintViewer] OpenFileMapping Failed, error=" << GetLastError() << L"\n");
    }

    hNodeViewerEvent = CreateEventW(
        NULL, FALSE, FALSE,
        L"Global\\OFGal_Engine_BlueprintViewer_NodeViewer_LoadBP");
    if (!hNodeViewerEvent) {
        DEBUG_W(L"[BlueprintViewer] CreateEvent NodeViewer Failed" << L"\n");
    }

    hVariablesViewerEvent = CreateEventW(
        NULL, FALSE, FALSE,
        L"Global\\OFGal_Engine_BlueprintViewer_VariablesViewer_LoadBP");
    if (!hVariablesViewerEvent) {
        DEBUG_W(L"[BlueprintViewer] CreateEvent VariablesViewer Failed" << L"\n");
    }

    hNodeChangedEvent = CreateEventW(
        NULL, FALSE, FALSE,
        L"Global\\OFGal_Engine_BlueprintViewer_NodeViewer_NodeChanged");
    if (!hNodeChangedEvent) {
        DEBUG_W(L"[BlueprintViewer] CreateEvent NodeChanged Failed" << L"\n");
    }

    hVarChangedEvent = CreateEventW(
        NULL, FALSE, FALSE,
        L"Global\\OFGal_Engine_BlueprintViewer_VariablesViewer_VarChanged");
    if (!hVarChangedEvent) {
        DEBUG_W(L"[BlueprintViewer] CreateEvent VarChanged Failed" << L"\n");
    }

    // ---------- 按键绑定 ----------
    m_inputSystem.SetGlobalCapture(false);

    m_inputCollector.AddBinding({ 'W',        Modifier::None, KeyCode::W,      true });
    m_inputCollector.AddBinding({ 'S',        Modifier::None, KeyCode::S,      true });
    m_inputCollector.AddBinding({ VK_UP,      Modifier::None, KeyCode::Up,     true });
    m_inputCollector.AddBinding({ VK_DOWN,    Modifier::None, KeyCode::Down,   true });
    m_inputCollector.AddBinding({ 'A',        Modifier::None, KeyCode::A,      true });
    m_inputCollector.AddBinding({ 'D',        Modifier::None, KeyCode::D,      true });
    m_inputCollector.AddBinding({ 'F',        Modifier::None, KeyCode::F,      true });
    m_inputCollector.AddBinding({ VK_DELETE,  Modifier::None, KeyCode::Delete, true });

    // ---------- 启动子进程 ----------
    LaunchChildProcess(exePath_NodeViewer);
    LaunchChildProcess(exePath_VariablesViewer);
}

BlueprintViewer::~BlueprintViewer() {
    if (pSharedMem)      UnmapViewOfFile(pSharedMem);
    if (hFileMapping)    CloseHandle(hFileMapping);
    if (hLoadBPEvent)    CloseHandle(hLoadBPEvent);
    if (hNodeViewerEvent) CloseHandle(hNodeViewerEvent);
    if (hVariablesViewerEvent) CloseHandle(hVariablesViewerEvent);
    if (hNodeChangedEvent) CloseHandle(hNodeChangedEvent);
    if (hVarChangedEvent) CloseHandle(hVarChangedEvent);

    // 关闭子进程句柄（不强制终止进程）
    for (HANDLE h : childProcesses) {
        if (h) CloseHandle(h);
    }
}

bool BlueprintViewer::LaunchChildProcess(const std::wstring& exePath) {
    STARTUPINFOW si = { sizeof(STARTUPINFOW) };
    PROCESS_INFORMATION pi = {};

    // 创建新控制台窗口
    BOOL success = CreateProcessW(
        exePath.c_str(),
        NULL,                   // 命令行
        NULL, NULL,             // 安全属性
        FALSE,                  // 不继承句柄
        CREATE_NEW_CONSOLE,     // 新控制台窗口
        NULL, NULL,
        &si, &pi);

    if (success) {
        childProcesses.push_back(pi.hProcess);
        // 关闭线程句柄，不需要
        CloseHandle(pi.hThread);
        return true;
    }
    else {
        DEBUG_W(L"[BlueprintViewer] LaunchChildProcess Failed for " << exePath << L", error=" << GetLastError() << L"\n");
        return false;
    }
}

void BlueprintViewer::Run() {
    // 构建等待事件数组：LoadBP, NodeChanged, VarChanged
    HANDLE eventsToWait[3] = { hLoadBPEvent, hNodeChangedEvent, hVarChangedEvent };
    DWORD numEvents = 3;
    if (!hLoadBPEvent) {
        // 若 LoadBP 事件无效，只等待后两个
        eventsToWait[0] = hNodeChangedEvent;
        eventsToWait[1] = hVarChangedEvent;
        numEvents = 2;
    }

    for (;;) {
        // 等待任意事件，20ms 超时
        DWORD dwWait = WaitForMultipleObjects(numEvents, eventsToWait, FALSE, 20);

        // 处理事件
        if (dwWait == WAIT_OBJECT_0) {
            // LoadBP 事件（数组第一个）
            if (pSharedMem) {
                const wchar_t* pPath = static_cast<const wchar_t*>(pSharedMem);
                currentBPPath = pPath;
                std::string filepath = WideToUTF8(currentBPPath);
                currentBPData = ReadBPData(filepath);
            }
            if (hLoadBPEvent)
                ResetEvent(hLoadBPEvent);
        }
        else if (dwWait == WAIT_OBJECT_0 + 1 ||
            (numEvents == 2 && eventsToWait[0] == hNodeChangedEvent && dwWait == WAIT_OBJECT_0)) {
            // NodeChanged 事件
            // TODO: 处理节点数据变更，重新加载当前蓝图数据等
            DEBUG_W(L"[BlueprintViewer] NodeChanged event signaled\n");
        }
        else if (dwWait == WAIT_OBJECT_0 + 2 ||
            (numEvents == 2 && eventsToWait[1] == hVarChangedEvent && dwWait == WAIT_OBJECT_0)) {
            // VarChanged 事件
            // TODO: 处理变量数据变更
            DEBUG_W(L"[BlueprintViewer] VarChanged event signaled\n");
        }
        else if (dwWait == WAIT_FAILED) {
            // 事件句柄失效
            break;
        }
        // WAIT_TIMEOUT 或其他情况继续循环

        // 输入监听
        if (!isEditing) {
            m_inputCollector.update();
        }

        // 处理输入事件
        std::vector<InputEvent> eventsCopy = m_inputSystem.getEvents();
        m_inputSystem.clearEvent();

        for (const auto& ev : eventsCopy) {
            if (ev.type == InputType::KeyDown) {
                switch (ev.key) {
                case KeyCode::W:  break;
                case KeyCode::S:  break;
                case KeyCode::Up: break;
                case KeyCode::Down: break;
                case KeyCode::A:  break;
                case KeyCode::D:  break;
                case KeyCode::F:  break;
                case KeyCode::Delete: break;
                default: break;
                }
            }
        }
    }
}

std::string BlueprintViewer::WideToUTF8(const std::wstring& wstr) const {
    if (wstr.empty()) return {};
    int len = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, nullptr, 0, nullptr, nullptr);
    if (len <= 0) return {};
    std::string result(len - 1, 0);
    WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, &result[0], len, nullptr, nullptr);
    return result;
}

void BlueprintViewer::ConfigureConsole() {
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut != INVALID_HANDLE_VALUE) {
        DWORD dwMode = 0;
        if (GetConsoleMode(hOut, &dwMode)) {
            dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
            SetConsoleMode(hOut, dwMode);
        }
    }
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

void BlueprintViewer::ClearScreen() {
    std::cout << "\x1b[2J\x1b[H" << std::flush;
}

void BlueprintViewer::FlushInputBuffer() {
    HANDLE hIn = GetStdHandle(STD_INPUT_HANDLE);
    FlushConsoleInputBuffer(hIn);
}

int BlueprintViewer::GetConsoleColumns() {
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut == INVALID_HANDLE_VALUE) return 80;
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    if (GetConsoleScreenBufferInfo(hOut, &csbi)) {
        return csbi.srWindow.Right - csbi.srWindow.Left + 1;
    }
    return 80;
}

void BlueprintViewer::SetWindowSizeAndPosition() {
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
            static_cast<int>(0.0f),
            static_cast<int>(0.0f),
            static_cast<int>(2040.0f * scaleX),
            static_cast<int>(1010.0f * scaleY),
            SWP_NOZORDER | SWP_NOACTIVATE);
    }
}

void BlueprintViewer::BuildAndPrintHelpText() {
    // 待实现
}

void BlueprintViewer::BuildAndPrintCurrentFlow() {
    // 待实现
}