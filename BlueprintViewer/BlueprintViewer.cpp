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

    for (HANDLE h : childProcesses) {
        if (h) CloseHandle(h);
    }
}

bool BlueprintViewer::LaunchChildProcess(const std::wstring& exePath) {
    STARTUPINFOW si = { sizeof(STARTUPINFOW) };
    PROCESS_INFORMATION pi = {};

    BOOL success = CreateProcessW(
        exePath.c_str(),
        NULL,
        NULL, NULL,
        FALSE,
        CREATE_NEW_CONSOLE,
        NULL, NULL,
        &si, &pi);

    if (success) {
        childProcesses.push_back(pi.hProcess);
        CloseHandle(pi.hThread);
        return true;
    }
    else {
        DEBUG_W(L"[BlueprintViewer] LaunchChildProcess Failed for " << exePath << L", error=" << GetLastError() << L"\n");
        return false;
    }
}

void BlueprintViewer::Run() {
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

                    auto entryIds = GetEntryNodeIds();
                    if (!entryIds.empty()) {
                        currentEntryIndex = 0;
                        currentEntryNodeId = entryIds[0];
                    }
                    else {
                        currentEntryIndex = 0;
                        currentEntryNodeId = -1;
                    }

                    // 重置选中节点（将在 BuildAndPrintCurrentFlow 中更新）
                    selectedNodeId1 = currentEntryNodeId;
                    selectedNodeId2 = currentEntryNodeId;

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
    if (m_flowNodeOrder.empty()) return;
    auto it = std::find(m_flowNodeOrder.begin(), m_flowNodeOrder.end(), selectedNodeId1);
    if (it == m_flowNodeOrder.end()) {
        selectedNodeId1 = m_flowNodeOrder.front();
    }
    else {
        if (it == m_flowNodeOrder.begin())
            it = m_flowNodeOrder.end() - 1;
        else
            --it;
        selectedNodeId1 = *it;
    }
    RenderAll();
}
void BlueprintViewer::MoveSelection1Down() {
    if (m_flowNodeOrder.empty()) return;
    auto it = std::find(m_flowNodeOrder.begin(), m_flowNodeOrder.end(), selectedNodeId1);
    if (it == m_flowNodeOrder.end()) {
        selectedNodeId1 = m_flowNodeOrder.front();
    }
    else {
        ++it;
        if (it == m_flowNodeOrder.end())
            it = m_flowNodeOrder.begin();
        selectedNodeId1 = *it;
    }
    RenderAll();
}
void BlueprintViewer::MoveSelection2Up() {
    if (m_flowNodeOrder.empty()) return;
    auto it = std::find(m_flowNodeOrder.begin(), m_flowNodeOrder.end(), selectedNodeId2);
    if (it == m_flowNodeOrder.end()) {
        selectedNodeId2 = m_flowNodeOrder.front();
    }
    else {
        if (it == m_flowNodeOrder.begin())
            it = m_flowNodeOrder.end() - 1;
        else
            --it;
        selectedNodeId2 = *it;
    }
    RenderAll();
}
void BlueprintViewer::MoveSelection2Down() {
    if (m_flowNodeOrder.empty()) return;
    auto it = std::find(m_flowNodeOrder.begin(), m_flowNodeOrder.end(), selectedNodeId2);
    if (it == m_flowNodeOrder.end()) {
        selectedNodeId2 = m_flowNodeOrder.front();
    }
    else {
        ++it;
        if (it == m_flowNodeOrder.end())
            it = m_flowNodeOrder.begin();
        selectedNodeId2 = *it;
    }
    RenderAll();
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
        if (visited.count(nodeId)) return nullptr;
        visited.insert(nodeId);

        const Node* node = nullptr;
        for (const auto& n : currentBPData.nodes) {
            if (n.id == nodeId) { node = &n; break; }
        }
        if (!node) return nullptr;

        auto treeNode = std::make_unique<ExecTreeNode>();
        treeNode->nodeId = node->id;
        treeNode->nodeType = node->type;

        for (const auto& pin : node->pins) {
            if (pin.io == "O" && pin.type == "exec") {
                for (const auto& link : currentBPData.links) {
                    if (link.sourceNode == nodeId && link.sourcePin == pin.name) {
                        auto child = build(link.targetNode);
                        if (child) {
                            std::string label = pin.name;
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

void BlueprintViewer::CollectNodeOrder(const ExecTreeNode* node, std::vector<int>& order) const {
    if (!node) return;
    order.push_back(node->nodeId);
    for (const auto& branch : node->branches) {
        CollectNodeOrder(branch.second.get(), order);
    }
}

// 工具：去除ANSI转义序列后的可见字符长度
size_t BlueprintViewer::VisibleLength(const std::string& s) {
    size_t len = 0;
    for (size_t i = 0; i < s.size(); ++i) {
        if (s[i] == '\x1b' && i + 1 < s.size() && s[i + 1] == '[') {
            while (i < s.size() && s[i] != 'm') ++i;
        }
        else {
            ++len;
        }
    }
    return len;
}

BlueprintViewer::RenderBlock BlueprintViewer::RenderExecTree(const ExecTreeNode* node,
    std::unordered_set<int>& visited,
    int depth,
    int selectedId1,
    int selectedId2) const {
    RenderBlock result;
    constexpr int MAX_DEPTH = 20;
    if (!node || depth > MAX_DEPTH || visited.count(node->nodeId)) {
        result.lines = { "[...]" };
        result.centerCol = 2;
        return result;
    }
    visited.insert(node->nodeId);

    // 1. 当前节点框（纯文本构造，之后根据需要添加颜色）
    std::string top_raw = "+" + std::string(boxWidth - 2, '-') + "+";
    std::string middle = "| " + node->nodeType + std::string(maxNodeNameLength - node->nodeType.size(), ' ') + " |";
    std::string bottom_raw = "+" + std::string(boxWidth - 2, '-') + "+";

    // 根据选中状态决定边框颜色
    bool isSel1 = (node->nodeId == selectedId1);
    bool isSel2 = (node->nodeId == selectedId2);
    std::string top_colored, bottom_colored;

    if (isSel1 && isSel2) {
        top_colored = CYAN + top_raw + RESET;
        bottom_colored = ORANGE + bottom_raw + RESET;
    }
    else if (isSel1) {
        top_colored = CYAN + top_raw + RESET;
        bottom_colored = CYAN + bottom_raw + RESET;
    }
    else if (isSel2) {
        top_colored = ORANGE + top_raw + RESET;
        bottom_colored = ORANGE + bottom_raw + RESET;
    }
    else {
        top_colored = top_raw;
        bottom_colored = bottom_raw;
    }

    std::vector<std::string> nodeLines = { top_colored, middle, bottom_colored };
    int center = boxWidth / 2;

    if (node->branches.empty()) {
        result.lines = std::move(nodeLines);
        result.centerCol = center;
        return result;
    }

    // 2. 递归子分支
    std::vector<RenderBlock> childBlocks;
    for (auto& branch : node->branches) {
        childBlocks.push_back(RenderExecTree(branch.second.get(), visited, depth + 1, selectedId1, selectedId2));
    }

    if (node->branches.size() == 1) {
        // === 单分支：垂直对齐中心 ===
        const auto& child = childBlocks[0];
        int requiredCenter = std::max(center, child.centerCol);
        int parentLeftPad = requiredCenter - center;
        int tempWidth = parentLeftPad + boxWidth;
        int childWidth = (int)VisibleLength(child.lines[0]);
        int totalWidth = std::max(tempWidth, childWidth);

        int newCenter = parentLeftPad + center;
        int childLeftPad = newCenter - child.centerCol;
        int childRightPad = totalWidth - (childLeftPad + childWidth);

        // 父节点行填充（保证可见宽度为 totalWidth）
        for (auto& line : nodeLines) {
            int visLen = (int)VisibleLength(line);
            line = std::string(parentLeftPad, ' ') + line
                + std::string(totalWidth - parentLeftPad - visLen, ' ');
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
        // === 多分支：水平拼接 ===
        int totalWidth = boxWidth;
        std::vector<int> offsets;
        int gap = 3;
        for (auto& blk : childBlocks) {
            offsets.push_back(totalWidth);
            totalWidth += (int)VisibleLength(blk.lines[0]) + gap;
        }
        if (!offsets.empty()) totalWidth -= gap;

        int parentStart = (totalWidth - boxWidth) / 2;
        result.lines.reserve(3 + std::max_element(childBlocks.begin(), childBlocks.end(),
            [](auto& a, auto& b) { return a.lines.size() < b.lines.size(); })->lines.size() + 2);

        // 父节点框行（带颜色，需根据可见宽度计算右侧填充）
        {
            int topVisLen = (int)VisibleLength(top_colored);
            int midVisLen = (int)VisibleLength(middle);
            int botVisLen = (int)VisibleLength(bottom_colored);
            std::string topPad(parentStart, ' ');
            result.lines.push_back(topPad + top_colored + std::string(totalWidth - parentStart - topVisLen, ' '));
            result.lines.push_back(topPad + middle + std::string(totalWidth - parentStart - midVisLen, ' '));
            result.lines.push_back(topPad + bottom_colored + std::string(totalWidth - parentStart - botVisLen, ' '));
        }
        int parentCenter = parentStart + center;

        // 连接线
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

        // 子块行（修复：删除填充，高度不足时保留空格）
        size_t maxChildLines = 0;
        for (auto& blk : childBlocks) maxChildLines = std::max(maxChildLines, blk.lines.size());
        for (size_t row = 0; row < maxChildLines; ++row) {
            std::string line(totalWidth, ' ');
            for (size_t i = 0; i < childBlocks.size(); ++i) {
                if (row < childBlocks[i].lines.size()) {
                    const std::string& childLine = childBlocks[i].lines[row];
                    int visLen = (int)VisibleLength(childLine);
                    if (offsets[i] + visLen <= totalWidth)
                        line.replace(offsets[i], visLen, childLine);
                    else if (offsets[i] < totalWidth)
                        line.replace(offsets[i], totalWidth - offsets[i], childLine);
                }
                // 超出子块高度的行：不做任何填充，保留空格
            }
            result.lines.push_back(line);
        }
        result.centerCol = parentCenter;
    }

    return result;
}

void BlueprintViewer::PrintRenderBlock(const RenderBlock& block) {
    for (const auto& line : block.lines) {
        std::cout << line << '\n';
    }
}

void BlueprintViewer::BuildAndPrintCurrentFlow() {
    auto entryIds = GetEntryNodeIds();
    if (entryIds.empty()) {
        std::cout << "The Blueprint is empty. Please Add your first Entry Node.\n";
        m_flowNodeOrder.clear();
        return;
    }
    if (currentEntryIndex < 0 || currentEntryIndex >= static_cast<int>(entryIds.size()))
        currentEntryIndex = 0;
    currentEntryNodeId = entryIds[currentEntryIndex];

    auto tree = BuildExecTree(currentEntryNodeId);
    if (!tree) {
        std::cout << "Failed to build execution flow.\n";
        m_flowNodeOrder.clear();
        return;
    }

    // 收集节点顺序
    m_flowNodeOrder.clear();
    CollectNodeOrder(tree.get(), m_flowNodeOrder);

    // 如果当前选中节点不在顺序中，重置到入口节点
    if (!m_flowNodeOrder.empty()) {
        if (std::find(m_flowNodeOrder.begin(), m_flowNodeOrder.end(), selectedNodeId1) == m_flowNodeOrder.end())
            selectedNodeId1 = currentEntryNodeId;
        if (std::find(m_flowNodeOrder.begin(), m_flowNodeOrder.end(), selectedNodeId2) == m_flowNodeOrder.end())
            selectedNodeId2 = currentEntryNodeId;
    }
    else {
        selectedNodeId1 = -1;
        selectedNodeId2 = -1;
    }

    std::unordered_set<int> visited;
    auto block = RenderExecTree(tree.get(), visited, 0, selectedNodeId1, selectedNodeId2);
    PrintRenderBlock(block);
}

void BlueprintViewer::AdjustBufferSize() {
    int requiredWidth = 0;
    int requiredHeight = 0;
    auto entryIds = GetEntryNodeIds();
    if (!entryIds.empty()) {
        for (int entryId : entryIds) {
            auto tree = BuildExecTree(entryId);
            if (!tree) continue;
            std::unordered_set<int> visited;
            auto block = RenderExecTree(tree.get(), visited, 0, selectedNodeId1, selectedNodeId2);
            int w = 0;
            for (const auto& line : block.lines) {
                int visLen = (int)VisibleLength(line);
                if (visLen > w) w = visLen;
            }
            int h = (int)block.lines.size();
            requiredWidth = std::max(requiredWidth, w);
            requiredHeight = std::max(requiredHeight, h);
        }
    }
    else {
        requiredWidth = 80;
        requiredHeight = 10;
    }

    requiredWidth += 20;
    requiredHeight += 20;

    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut == INVALID_HANDLE_VALUE) return;

    CONSOLE_SCREEN_BUFFER_INFO csbi;
    if (!GetConsoleScreenBufferInfo(hOut, &csbi)) return;

    int windowWidth = csbi.srWindow.Right - csbi.srWindow.Left + 1;
    int windowHeight = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;

    int newWidth = std::max(requiredWidth, windowWidth + 20);
    int newHeight = std::max(requiredHeight, windowHeight + 20);

    COORD bufferSize;
    bufferSize.X = static_cast<SHORT>(newWidth);
    bufferSize.Y = static_cast<SHORT>(newHeight);
    SetConsoleScreenBufferSize(hOut, bufferSize);
}

void BlueprintViewer::ScrollToTheTop() {
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut == INVALID_HANDLE_VALUE) return;
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    if (!GetConsoleScreenBufferInfo(hOut, &csbi)) return;
    SMALL_RECT window = csbi.srWindow;
    SHORT height = window.Bottom - window.Top + 1;
    window.Top = 0;
    window.Bottom = height - 1;
    SetConsoleWindowInfo(hOut, TRUE, &window);
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
        std::string plain;
        std::string styled;
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
        int neededLen = (currentLineLen > 0 ? 1 : 0) + static_cast<int>(item.plain.length());
        if (currentLineLen > 0 && currentLineLen + neededLen > cols) {
            out << "\n";
            currentLineLen = 0;
            neededLen = static_cast<int>(item.plain.length());
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

    ScrollToTheTop();
}