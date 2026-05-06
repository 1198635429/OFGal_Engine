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
        eventsToWait[0] = hNodeChangedEvent;
        eventsToWait[1] = hVarChangedEvent;
        numEvents = 2;
    }

    for (;;) {
        DWORD dwWait = WaitForMultipleObjects(numEvents, eventsToWait, FALSE, 20);

        // 处理事件
        if (dwWait == WAIT_OBJECT_0) {
            // LoadBP 事件
            if (pSharedMem) {
                try {
                    const wchar_t* pPath = static_cast<const wchar_t*>(pSharedMem);
                    currentBPPath = pPath;
                    std::string filepath = WideToUTF8(currentBPPath);
                    currentBPData = ReadBPData(filepath);

                    // 设置默认入口节点
                    auto entryIds = GetEntryNodeIds();
                    if (!entryIds.empty()) {
                        currentEntryIndex = 0;
                        currentEntryNodeId = entryIds[0];
                    }
                    else {
                        currentEntryIndex = 0;
                        currentEntryNodeId = -1;
                    }

                    AdjustBufferSize();

                    if (hNodeViewerEvent) SetEvent(hNodeViewerEvent);
                    if (hVariablesViewerEvent) SetEvent(hVariablesViewerEvent);

                    RenderAll();
                }
                catch (const std::exception& e) {
                    ClearScreen();
                    std::cerr << "ERROR: Failed to load blueprint: " << e.what() << std::endl;
                    currentBPData = BlueprintData{};
                    currentBPPath.clear();
                }
                catch (...) {
                    ClearScreen();
                    std::cerr << "ERROR: Unknown error while loading blueprint." << std::endl;
                    currentBPData = BlueprintData{};
                    currentBPPath.clear();
                }
            }
            if (hLoadBPEvent)
                ResetEvent(hLoadBPEvent);
        }
        else if (dwWait == WAIT_OBJECT_0 + 1 ||
            (numEvents == 2 && eventsToWait[0] == hNodeChangedEvent && dwWait == WAIT_OBJECT_0)) {
            DEBUG_W(L"[BlueprintViewer] NodeChanged event signaled\n");
        }
        else if (dwWait == WAIT_OBJECT_0 + 2 ||
            (numEvents == 2 && eventsToWait[1] == hVarChangedEvent && dwWait == WAIT_OBJECT_0)) {
            DEBUG_W(L"[BlueprintViewer] VarChanged event signaled\n");
        }
        else if (dwWait == WAIT_FAILED) {
            break;
        }

        // 输入监听
        if (!isEditing) {
            m_inputCollector.update();
        }

        std::vector<InputEvent> eventsCopy = m_inputSystem.getEvents();
        m_inputSystem.clearEvent();

        for (const auto& ev : eventsCopy) {
            if (ev.type == InputType::KeyDown) {
                switch (ev.key) {
                case KeyCode::W:   MoveSelection1Up(); break;
                case KeyCode::S:   MoveSelection1Down(); break;
                case KeyCode::Up:  MoveSelection2Up(); break;
                case KeyCode::Down:MoveSelection2Down(); break;
                case KeyCode::A:   MoveToPrevFlow(); break;
                case KeyCode::D:   MoveToNextFlow(); break;
                case KeyCode::F:
                    isEditing = true;
                    Edit();
                    isEditing = false;
                    break;
                case KeyCode::Delete:
                    isEditing = true;
                    OnDelete();
                    isEditing = false;
                    break;
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
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut == INVALID_HANDLE_VALUE) return;

    CONSOLE_SCREEN_BUFFER_INFO csbi;
    if (!GetConsoleScreenBufferInfo(hOut, &csbi)) return;

    DWORD dwSize = csbi.dwSize.X * csbi.dwSize.Y;
    DWORD dwWritten;
    COORD coord = { 0, 0 };

    FillConsoleOutputCharacterW(hOut, L' ', dwSize, coord, &dwWritten);
    FillConsoleOutputAttribute(hOut, csbi.wAttributes, dwSize, coord, &dwWritten);
    SetConsoleCursorPosition(hOut, coord);
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

int BlueprintViewer::GetMaxNodeId() const {
    int maxId = -1;
    for (const auto& node : currentBPData.nodes) {
        if (node.id > maxId) {
            maxId = node.id;
        }
    }
    return maxId;
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
    int cols = GetConsoleColumns();
    std::string separator(cols, '=');
    std::ostringstream oss;

    oss << CYAN << "=== Help ===" << RESET << "\n";
    oss << separator << "\n";

    oss << CYAN << "W" << RESET << " - Move selection 1 up\n";
    oss << CYAN << "S" << RESET << " - Move selection 1 down\n";
    oss << CYAN << "Up" << RESET << " - Move selection 2 up\n";
    oss << CYAN << "Down" << RESET << " - Move selection 2 down\n";
    oss << CYAN << "Delete" << RESET << " - Remove selected node 1 and all its descendants\n";
    oss << CYAN << "F" << RESET << " - More operations\n";
    oss << CYAN << "A" << RESET << " - Previous execution flow\n";
    oss << CYAN << "D" << RESET << " - Next execution flow\n";

    oss << separator << "\n";
    std::cout << oss.str();
}

void BlueprintViewer::MoveSelection1Up() {
    ClearScreen();
    BuildAndPrintHelpText();
}
void BlueprintViewer::MoveSelection1Down() {
    ClearScreen();
    BuildAndPrintHelpText();
}
void BlueprintViewer::MoveSelection2Up() {
    ClearScreen();
    BuildAndPrintHelpText();
}
void BlueprintViewer::MoveSelection2Down() {
    ClearScreen();
    BuildAndPrintHelpText();
}
void BlueprintViewer::MoveToPrevFlow() {
    auto entryIds = GetEntryNodeIds();
    if (entryIds.empty()) return;
    if (currentEntryIndex <= 0)
        currentEntryIndex = static_cast<int>(entryIds.size()) - 1;
    else
        --currentEntryIndex;
    currentEntryNodeId = entryIds[currentEntryIndex];
    RenderAll();
}

void BlueprintViewer::MoveToNextFlow() {
    auto entryIds = GetEntryNodeIds();
    if (entryIds.empty()) return;
    if (currentEntryIndex >= static_cast<int>(entryIds.size()) - 1)
        currentEntryIndex = 0;
    else
        ++currentEntryIndex;
    currentEntryNodeId = entryIds[currentEntryIndex];
    RenderAll();
}
void BlueprintViewer::OnDelete() {
    ClearScreen();
    FlushInputBuffer();
}
void BlueprintViewer::Edit() {
    ClearScreen();
    FlushInputBuffer();
}

std::vector<int> BlueprintViewer::GetEntryNodeIds() const {
    std::vector<int> ids;
    for (const auto& node : currentBPData.nodes) {
        if (node.type == "BeginPlay" ||
            node.type == "Play_per_N_ms" ||
            node.type == "Play_when_N_push_down" ||
            node.type == "Play_when_triggered") {
            ids.push_back(node.id);
        }
    }
    return ids;
}

std::vector<std::pair<int, std::string>> BlueprintViewer::GetEntryNodes() const {
    std::vector<std::pair<int, std::string>> entries;
    for (const auto& node : currentBPData.nodes) {
        if (node.type == "BeginPlay" ||
            node.type == "Play_per_N_ms" ||
            node.type == "Play_when_N_push_down" ||
            node.type == "Play_when_triggered") {
            entries.emplace_back(node.id, node.type);
        }
    }
    return entries;
}

std::unique_ptr<BlueprintViewer::ExecTreeNode> BlueprintViewer::BuildExecTree(int startNodeId) const {
    std::unordered_set<int> visited;
    std::function<std::unique_ptr<ExecTreeNode>(int)> build = [&](int nodeId) -> std::unique_ptr<ExecTreeNode> {
        if (visited.count(nodeId)) return nullptr; // 环
        visited.insert(nodeId);

        const Node* node = nullptr;
        for (const auto& n : currentBPData.nodes) {
            if (n.id == nodeId) { node = &n; break; }
        }
        if (!node) return nullptr;

        auto treeNode = std::make_unique<ExecTreeNode>();
        treeNode->nodeId = node->id;
        treeNode->nodeType = node->type;

        // 收集所有 exec 输出连接
        for (const auto& pin : node->pins) {
            if (pin.io == "O" && pin.type == "exec") {
                for (const auto& link : currentBPData.links) {
                    if (link.sourceNode == nodeId && link.sourcePin == pin.name) {
                        auto child = build(link.targetNode);
                        if (child) {
                            std::string label = pin.name;
                            // 可美化标签
                            if (label == "OEXEC_A") label = "True";
                            else if (label == "OEXEC_B") label = "False";
                            else if (label == "OEXEC_Loop") label = "Loop";
                            else if (label == "OEXEC") label = "";
                            treeNode->branches.emplace_back(label, std::move(child));
                        }
                    }
                }
            }
        }
        return treeNode;
        };
    return build(startNodeId);
}

BlueprintViewer::RenderBlock BlueprintViewer::RenderExecTree(const ExecTreeNode* node,
    std::unordered_set<int>& visited,
    int depth) const {
    RenderBlock result;
    constexpr int MAX_DEPTH = 20;
    if (!node || depth > MAX_DEPTH || visited.count(node->nodeId)) {
        result.lines = { "[...]" };
        result.centerCol = 2;
        return result;
    }
    visited.insert(node->nodeId);

    // 1. 当前节点框
    std::string top = "+" + std::string(boxWidth - 2, '-') + "+";
    std::string middle = "| " + node->nodeType + std::string(maxNodeNameLength - node->nodeType.size(), ' ') + " |";
    std::string bottom = "+" + std::string(boxWidth - 2, '-') + "+";
    std::vector<std::string> nodeLines = { top, middle, bottom };
    int center = boxWidth / 2;

    if (node->branches.empty()) {
        result.lines = std::move(nodeLines);
        result.centerCol = center;
        return result;
    }

    // 2. 递归子分支
    std::vector<RenderBlock> childBlocks;
    for (auto& branch : node->branches) {
        childBlocks.push_back(RenderExecTree(branch.second.get(), visited, depth + 1));
    }

    if (node->branches.size() == 1) {
        // === 单分支：垂直对齐中心 ===
        const auto& child = childBlocks[0];
        // 确保子块中心不大于父中心，否则扩大总宽度并右移父框
        int requiredCenter = std::max(center, child.centerCol);
        int parentLeftPad = requiredCenter - center;        // 父框需要右侧填充的空格数（实际是左右一起移）
        int tempWidth = parentLeftPad + boxWidth;           // 父框最小需要的宽度
        int totalWidth = std::max(tempWidth, (int)child.lines[0].size());

        int newCenter = parentLeftPad + center;             // 新的父中心
        int childLeftPad = newCenter - child.centerCol;     // >=0 保证
        int childRightPad = totalWidth - (childLeftPad + (int)child.lines[0].size());

        // 父节点行：左边填充 parentLeftPad 个空格，右边填充剩余
        for (auto& line : nodeLines) {
            line = std::string(parentLeftPad, ' ') + line
                + std::string(totalWidth - parentLeftPad - (int)line.size(), ' ');
        }
        result.lines = std::move(nodeLines);

        // 连接线
        std::string line1(totalWidth, ' '); line1[newCenter] = '|';
        std::string line2(totalWidth, ' '); line2[newCenter] = 'v';
        result.lines.push_back(line1);
        result.lines.push_back(line2);

        // 子块行
        for (auto& line : child.lines) {
            result.lines.push_back(std::string(childLeftPad, ' ') + line + std::string(childRightPad, ' '));
        }
        result.centerCol = newCenter;
    }
    else {
        // === 多分支：水平拼接（原有代码未动）===
        int totalWidth = boxWidth;
        std::vector<int> offsets;
        int gap = 3;
        for (auto& blk : childBlocks) {
            offsets.push_back(totalWidth);
            totalWidth += (int)blk.lines[0].size() + gap;
        }
        if (!offsets.empty()) totalWidth -= gap;

        int parentStart = (totalWidth - boxWidth) / 2;
        result.lines.reserve(3 + std::max_element(childBlocks.begin(), childBlocks.end(),
            [](auto& a, auto& b) { return a.lines.size() < b.lines.size(); })->lines.size() + 2);

        std::string topPad(parentStart, ' ');
        result.lines.push_back(topPad + top + std::string(totalWidth - parentStart - boxWidth, ' '));
        result.lines.push_back(topPad + middle + std::string(totalWidth - parentStart - boxWidth, ' '));
        result.lines.push_back(topPad + bottom + std::string(totalWidth - parentStart - boxWidth, ' '));
        int parentCenter = parentStart + center;

        std::string conn1(totalWidth, ' '); conn1[parentCenter] = '|';
        result.lines.push_back(conn1);

        std::string conn2(totalWidth, ' ');
        for (size_t i = 0; i < childBlocks.size(); ++i) {
            int childCenter = offsets[i] + childBlocks[i].centerCol;
            int left = std::min(parentCenter, childCenter);
            int right = std::max(parentCenter, childCenter);
            for (int x = left; x <= right; ++x) {
                if (x == childCenter)
                    conn2[x] = 'v';
                else if (conn2[x] == ' ')
                    conn2[x] = '-';
            }
        }
        result.lines.push_back(conn2);

        size_t maxChildLines = 0;
        for (auto& blk : childBlocks) maxChildLines = std::max(maxChildLines, blk.lines.size());
        for (size_t row = 0; row < maxChildLines; ++row) {
            std::string line(totalWidth, ' ');
            for (size_t i = 0; i < childBlocks.size(); ++i) {
                size_t childRow = row < childBlocks[i].lines.size() ? row : childBlocks[i].lines.size() - 1;
                std::string childLine = childBlocks[i].lines[childRow];
                for (size_t col = 0; col < childLine.size(); ++col) {
                    if (offsets[i] + col < totalWidth)
                        line[offsets[i] + col] = childLine[col];
                }
            }
            result.lines.push_back(line);
        }
        result.centerCol = parentCenter;
    }

    return result;
}

void BlueprintViewer::PrintRenderBlock(const RenderBlock& block) {
    // 移除原有的 ClearScreen()，由 RenderAll 统一清屏
    for (const auto& line : block.lines) {
        std::cout << line << '\n';
    }
}

void BlueprintViewer::BuildAndPrintCurrentFlow() {
    auto entryIds = GetEntryNodeIds();
    if (entryIds.empty()) {
        std::cout << "The Blueprint is empty. Please Add your first Entry Node.\n";
        return;
    }
    if (currentEntryIndex < 0 || currentEntryIndex >= static_cast<int>(entryIds.size()))
        currentEntryIndex = 0;
    currentEntryNodeId = entryIds[currentEntryIndex];

    auto tree = BuildExecTree(currentEntryNodeId);
    if (!tree) {
        std::cout << "Failed to build execution flow.\n";
        return;
    }
    std::unordered_set<int> visited;
    auto block = RenderExecTree(tree.get(), visited, 0);
    PrintRenderBlock(block);
}

void BlueprintViewer::AdjustBufferSize() {
    // 预测所有入口节点的执行流渲染尺寸（取最大值）
    int requiredWidth = 0;
    int requiredHeight = 0;
    auto entryIds = GetEntryNodeIds();
    if (!entryIds.empty()) {
        for (int entryId : entryIds) {
            auto tree = BuildExecTree(entryId);
            if (!tree) continue;
            std::unordered_set<int> visited;
            auto block = RenderExecTree(tree.get(), visited, 0);
            int w = 0;
            for (const auto& line : block.lines) {
                if ((int)line.size() > w) w = (int)line.size();
            }
            int h = (int)block.lines.size();
            requiredWidth = std::max(requiredWidth, w);
            requiredHeight = std::max(requiredHeight, h);
        }
    }
    else {
        // 空蓝图：可用帮助文本的尺寸作为最小尺寸（例如帮助文本宽度约80，高度约10）
        requiredWidth = 80;
        requiredHeight = 10;
    }

    // 加20个单位的边距
    requiredWidth += 20;
    requiredHeight += 20;

    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut == INVALID_HANDLE_VALUE) return;

    CONSOLE_SCREEN_BUFFER_INFO csbi;
    if (!GetConsoleScreenBufferInfo(hOut, &csbi)) return;

    // 当前窗口尺寸
    int windowWidth = csbi.srWindow.Right - csbi.srWindow.Left + 1;
    int windowHeight = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;

    // 缓冲区必须大于窗口（至少大20）
    int newWidth = std::max(requiredWidth, windowWidth + 20);
    int newHeight = std::max(requiredHeight, windowHeight + 20);

    COORD bufferSize;
    bufferSize.X = static_cast<SHORT>(newWidth);
    bufferSize.Y = static_cast<SHORT>(newHeight);
    SetConsoleScreenBufferSize(hOut, bufferSize);
}


void BlueprintViewer::PrintEntryNodes() {
    auto entryNodes = GetEntryNodes();
    int cols = GetConsoleColumns();
    std::string separator(cols, '=');
    std::cout << separator << "\n";

    if (entryNodes.empty()) {
        std::cout << "No entry nodes.\n";
        std::cout << separator << "\n";
        return;
    }

    struct EntryItem {
        std::string plain;   // 无ANSI转义，用于计算宽度
        std::string styled;  // 可能包含颜色代码
    };
    std::vector<EntryItem> items;
    for (size_t i = 0; i < entryNodes.size(); ++i) {
        int id = entryNodes[i].first;
        const std::string& type = entryNodes[i].second;
        std::string plain = "No." + std::to_string(id) + " " + type;
        std::string styled;
        if (static_cast<int>(i) == currentEntryIndex) {
            styled = CYAN + plain + RESET;
        }
        else {
            styled = plain;
        }
        items.push_back({ plain, styled });
    }

    std::ostringstream out;
    int currentLineLen = 0;
    for (size_t i = 0; i < items.size(); ++i) {
        const auto& item = items[i];
        // 加上一个分隔空格（行首不加）
        int neededLen = (currentLineLen > 0 ? 1 : 0) + static_cast<int>(item.plain.length());
        if (currentLineLen > 0 && currentLineLen + neededLen > cols) {
            out << "\n";
            currentLineLen = 0;
            neededLen = static_cast<int>(item.plain.length()); // 无前导空格
        }
        if (currentLineLen > 0) {
            out << " ";
            currentLineLen++;
        }
        out << item.styled;
        currentLineLen += item.plain.length();
    }
    out << "\n";
    std::cout << out.str();
    std::cout << separator << "\n";
}

void BlueprintViewer::RenderAll() {
    ClearScreen();
    PrintEntryNodes();
    int cols = GetConsoleColumns();
    std::string splitLine(cols, '-');
    std::cout << splitLine << "\n";
    BuildAndPrintCurrentFlow();
}