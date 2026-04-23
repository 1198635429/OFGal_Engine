// Copyright 2026 Nagato-Yuki-708. All Rights Reserved.
#include "LevelTreeList.h"
#include "Json_LevelData_ReadWrite.h"
#include <iostream>
#include <string>
#include <algorithm>
#include <conio.h>

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
    // m_currentLevel 为 unique_ptr，会自动调用析构，递归释放所有 ObjectData
}

static bool SafeReadLine(std::string& out, const std::string& prompt, bool allowCancel = true) {
    std::cout << prompt;
    out.clear();
    HANDLE hIn = GetStdHandle(STD_INPUT_HANDLE);
    DWORD oldMode;
    GetConsoleMode(hIn, &oldMode);

    // 清空所有残留的输入事件（包括键盘事件）
    FlushConsoleInputBuffer(hIn);
    INPUT_RECORD dummy;
    DWORD read;
    while (PeekConsoleInputW(hIn, &dummy, 1, &read) && read > 0) {
        ReadConsoleInputW(hIn, &dummy, 1, &read);
    }

    // 调整控制台模式：禁用行缓冲和回显，启用窗口输入
    DWORD newMode = oldMode;
    newMode |= ENABLE_WINDOW_INPUT;
    newMode &= ~(ENABLE_LINE_INPUT | ENABLE_ECHO_INPUT);
    SetConsoleMode(hIn, newMode);

    while (true) {
        INPUT_RECORD ir;
        if (!ReadConsoleInputW(hIn, &ir, 1, &read) || read == 0)
            continue;
        if (ir.EventType != KEY_EVENT || !ir.Event.KeyEvent.bKeyDown)
            continue;

        WORD vk = ir.Event.KeyEvent.wVirtualKeyCode;
        WCHAR ch = ir.Event.KeyEvent.uChar.UnicodeChar;

        if (vk == VK_RETURN) {
            std::cout << "\n";
            break;
        }
        if (allowCancel && vk == VK_ESCAPE) {
            out = "#esc#";
            std::cout << "#esc#\n";
            SetConsoleMode(hIn, oldMode);
            return false;
        }
        if (vk == VK_BACK) {
            if (!out.empty()) {
                out.pop_back();
                std::cout << "\b \b";
            }
            continue;
        }
        if (ch >= 0x20 && ch <= 0x7E) {
            char ascii = static_cast<char>(ch);
            out.push_back(ascii);
            std::cout << ascii;
        }
    }

    SetConsoleMode(hIn, oldMode);
    return true;
}

void LevelTreeList::ClearScreen() {
    std::cout << "\033[2J\033[H";
}

void LevelTreeList::RecursiveBuild(const std::map<std::string, ObjectData*>& children,
    int depth, const std::string& prefix) {
    size_t count = children.size();
    size_t idx = 0;
    for (const auto& pair : children) {
        ++idx;
        bool isLast = (idx == count);
        const std::string& objName = pair.first;
        ObjectData* obj = pair.second;

        std::string branch = isLast ? "`-- " : "|-- ";
        std::string line = prefix + branch + objName;
        std::string childPrefix = prefix + (isLast ? "    " : "|   ");

        m_displayNodes.push_back({ line, prefix + branch, objName, obj, depth });

        if (!obj->objects.empty()) {
            RecursiveBuild(obj->objects, depth + 1, childPrefix);
        }
    }
}

void LevelTreeList::BuildDisplayList() {
    m_displayNodes.clear();
    if (!m_currentLevel) return;
    m_displayNodes.push_back({ m_currentLevel->name, "", m_currentLevel->name, nullptr, 0 });
    RecursiveBuild(m_currentLevel->objects, 1, "");
}

void LevelTreeList::RenderTree() {
    ClearScreen();
    if (m_displayNodes.empty()) {
        std::cout << "\033[37mNo level loaded.\033[0m" << std::endl;
        return;
    }
    std::cout << "\033[36mW/Up: select previous  S/Down: select next  J: add object  K: delete object\033[0m\n" << std::endl;
    for (size_t i = 0; i < m_displayNodes.size(); ++i) {
        const auto& node = m_displayNodes[i];
        if (static_cast<int>(i) == m_selectedIndex) {
            std::cout << "\033[7m";
        }
        std::cout << "\033[36m" << node.prefix << "\033[37m" << node.name << "\033[0m\n";
    }
}

void LevelTreeList::ConfigureConsole() {
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
            static_cast<int>(0.0f),
            static_cast<int>(0.0f),
            static_cast<int>(430.0f * scaleX),
            static_cast<int>(1010.0f * scaleY),
            SWP_NOZORDER | SWP_NOACTIVATE);
    }
}

bool LevelTreeList::OpenIPC() {
    const std::wstring eventName = L"Global\\OFGal_Engine_LevelTreeList_PathUpdate";
    const std::wstring sharedMemName =
        L"Global\\OFGal_Engine_ProjectStructureViewer_"
        L"FolderViewer_OpenLevelPath";

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
    ConfigureConsole();
    SetConsoleTitleW(L"OFGal_Engine/LevelTreeList");
    SetWindowSizeAndPosition();
    OutputDebugStringW(L"[LevelTreeList] Started. Initializing IPC...\n");
    if (!OpenIPC()) {
        system("pause");
        return false;
    }

    HWND hConsole = GetConsoleWindow();
    m_inputSystem->SetWindowHandle(hConsole);
    m_inputSystem->SetGlobalCapture(false);

    m_inputCollector = std::make_unique<InputCollector>(m_inputSystem);
    m_inputCollector->AddBinding({ 'W',         Modifier::None, KeyCode::W,   true });
    m_inputCollector->AddBinding({ VK_UP,       Modifier::None, KeyCode::Up,  true });
    m_inputCollector->AddBinding({ 'S',         Modifier::None, KeyCode::S,    true });
    m_inputCollector->AddBinding({ VK_DOWN,     Modifier::None, KeyCode::Down, true });
    m_inputCollector->AddBinding({ 'J', Modifier::None, KeyCode::J, true });
    m_inputCollector->AddBinding({ 'K', Modifier::None, KeyCode::K, true });
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

    std::string utf8Path = WstrToUtf8(openedLevelPath);
    try {
        LevelData level = ReadLevelData(utf8Path);
        m_currentLevel = std::make_unique<LevelData>(std::move(level));
        m_currentLevelPath = utf8Path;
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

bool LevelTreeList::IsNameUsed(const std::string& name) const {
    if (!m_currentLevel) return false;
    if (m_currentLevel->name == name) return true;
    for (const auto& pair : m_currentLevel->objects) {
        if (IsNameUsedInObject(pair.second, name)) return true;
    }
    return false;
}

bool LevelTreeList::IsNameUsedInObject(const ObjectData* obj, const std::string& name) const {
    if (!obj) return false;
    if (obj->name == name) return true;
    for (const auto& pair : obj->objects) {
        if (IsNameUsedInObject(pair.second, name)) return true;
    }
    return false;
}

void LevelTreeList::SaveCurrentLevel() {
    if (!m_currentLevel || m_currentLevelPath.empty()) return;
    try {
        WriteLevelData(m_currentLevelPath, *m_currentLevel);
        OutputDebugStringA(("[LevelTreeList] Saved to " + m_currentLevelPath + "\n").c_str());
    }
    catch (const std::exception& e) {
        std::string err = "[LevelTreeList] Failed to save level: " + std::string(e.what()) + "\n";
        OutputDebugStringA(err.c_str());
    }
}

// 改进后的删除函数：仅通过对象指针删除
void LevelTreeList::RemoveObjectFromParent(ObjectData* obj) {
    if (!obj || !m_currentLevel) return;

    ObjectData* parent = obj->parent;
    std::map<std::string, ObjectData*>* container = nullptr;

    if (parent == nullptr) {
        // 父容器为场景根
        container = &m_currentLevel->objects;
    }
    else {
        container = &parent->objects;
    }

    // 根据对象名称从容器中移除（注意：名称是唯一的）
    auto it = container->find(obj->name);
    if (it != container->end() && it->second == obj) {
        // 直接删除对象，其析构函数会递归删除所有子对象
        delete obj;
        container->erase(it);
    }
}

// 清除输入流中的残留数据
void LevelTreeList::ClearInputStream() {
    std::cin.clear();
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
}

// 询问单个组件，支持取消指令
bool LevelTreeList::AskComponent(const std::string& componentName) {
    while (true) {
        std::cout << "  " << componentName << "? (y/n): ";
        std::string input;
        std::getline(std::cin, input);
        if (input == "#esc#") {
            throw std::runtime_error("cancelled");
        }
        if (input == "y" || input == "Y") {
            return true;
        }
        if (input == "n" || input == "N") {
            return false;
        }
        std::cout << "Invalid input, please enter y/n or #esc# to cancel.\n";
    }
}

void LevelTreeList::AddObjectInteractive() {
    if (!m_currentLevel) return;

    m_inInteractiveMode = true;  // 暂停快捷键处理

    if (m_selectedIndex < 0 || m_selectedIndex >= static_cast<int>(m_displayNodes.size())) {
        m_inInteractiveMode = false;
        return;
    }
    const auto& selectedNode = m_displayNodes[m_selectedIndex];
    std::string parentName = selectedNode.name;
    ObjectData* parentObj = selectedNode.object;

    ClearScreen();
    std::cout << "You are about to create a new object under \"" << parentName << "\"\n";
    std::cout << "(Press ESC at any prompt to cancel)\n\n";

    std::string newName;
    while (true) {
        if (!SafeReadLine(newName, "Enter object name: ", true)) {
            std::cout << "Operation cancelled.\n";
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            m_inInteractiveMode = false;
            RenderTree();
            return;
        }
        if (newName.empty()) {
            std::cout << "Name cannot be empty. Try again.\n";
            continue;
        }
        if (IsNameUsed(newName)) {
            std::cout << "Name \"" << newName << "\" already exists. Please choose a different name.\n";
            continue;
        }
        break;
    }

    // 询问组件（同样使用 SafeReadLine）
    std::cout << "\nSelect components to add:\n";
    bool addTransform = false, addPicture = false, addTextblock = false;
    bool addTrigger = false, addBlueprint = false;

    // 辅助 lambda
    auto ask = [&](const std::string& compName) -> bool {
        while (true) {
            std::string ans;
            std::string prompt = "  " + compName + "? (y/n): ";
            if (!SafeReadLine(ans, prompt, true)) {
                throw std::runtime_error("cancelled");
            }
            if (ans == "y" || ans == "Y") return true;
            if (ans == "n" || ans == "N") return false;
            std::cout << "Invalid input, please enter y/n or press ESC to cancel.\n";
        }
        };

    try {
        addTransform = ask("TransformComponent");
        addPicture = ask("PictureComponent");
        addTextblock = ask("TextblockComponent");
        addTrigger = ask("TriggerAreaComponent");
        addBlueprint = ask("BlueprintComponent");
    }
    catch (const std::runtime_error&) {
        std::cout << "Operation cancelled.\n";
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        m_inInteractiveMode = false;
        RenderTree();
        return;
    }

    // 3. 创建对象
    ObjectData* newObj = new ObjectData();
    newObj->name = newName;
    newObj->parent = parentObj;

    if (addTransform) {
        TransformComponent tc;
        tc.Location = { 0.0f, 0.0f, 0 };
        tc.Rotation = { 0.0f };
        tc.Scale = { 1.0f, 1.0f };
        newObj->Transform = tc;
    }
    if (addPicture) {
        PictureComponent pc;
        pc.Path = "";
        pc.Location = { 0.0f, 0.0f, 0 };
        pc.Rotation = { 0.0f };
        pc.Size = { 0.0f, 0.0f };
        newObj->Picture = pc;
    }
    if (addTextblock) {
        TextblockComponent tc;
        tc.Location = { 0, 0 };
        tc.Size = { 0, 0 };
        tc.Text.component = "";
        tc.Text.Font_size = 0;
        tc.Text.ANSI_Print = false;
        tc.Scale = { 1.0f, 1.0f };
        newObj->Textblock = tc;
    }
    if (addTrigger) {
        TriggerAreaComponent tac;
        tac.Location = { 0.0f, 0.0f };
        tac.Size = { 0.0f, 0.0f };
        newObj->TriggerArea = tac;
    }
    if (addBlueprint) {
        BlueprintComponent bc;
        bc.Path = "";
        newObj->Blueprint = bc;
    }

    if (parentObj == nullptr) {
        m_currentLevel->objects[newName] = newObj;
    }
    else {
        parentObj->objects[newName] = newObj;
    }

    BuildDisplayList();
    if (m_selectedIndex >= static_cast<int>(m_displayNodes.size()))
        m_selectedIndex = static_cast<int>(m_displayNodes.size()) - 1;

    m_inInteractiveMode = false;
    RenderTree();
    SaveCurrentLevel();
}

void LevelTreeList::DeleteSelectedObject() {
    if (!m_currentLevel) return;

    m_inInteractiveMode = true;

    if (m_selectedIndex < 0 || m_selectedIndex >= static_cast<int>(m_displayNodes.size())) {
        m_inInteractiveMode = false;
        return;
    }
    const auto& selectedNode = m_displayNodes[m_selectedIndex];
    if (selectedNode.object == nullptr) {
        m_inInteractiveMode = false;
        return;
    }

    std::string objName = selectedNode.name;
    ObjectData* obj = selectedNode.object;

    ClearScreen();
    std::cout << "You are about to delete \"" << objName << "\" and all its children.\n";
    std::string confirm;
    SafeReadLine(confirm, "Confirm (Y/N): ", true);
    // 允许 ESC 取消，但若输入 #esc# 则视为取消
    if (confirm == "#esc#" || confirm.empty()) {
        std::cout << "Deletion cancelled.\n";
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        m_inInteractiveMode = false;
        RenderTree();
        return;
    }

    if (confirm == "Y" || confirm == "y") {
        RemoveObjectFromParent(obj);
        BuildDisplayList();
        if (m_selectedIndex >= static_cast<int>(m_displayNodes.size()))
            m_selectedIndex = static_cast<int>(m_displayNodes.size()) - 1;
        SaveCurrentLevel();
    }

    m_inInteractiveMode = false;
    RenderTree();
}

void LevelTreeList::ProcessInputEvents() {
    if (m_inInteractiveMode) return;

    const auto& events = m_inputSystem->getEvents();
    bool selectionChanged = false;

    for (const auto& ev : events) {
        if (ev.type == InputType::KeyDown) {
            switch (ev.key) {
            case KeyCode::W:   // W 键
            case KeyCode::Up:  // 上箭头
                if (m_selectedIndex > 0) {
                    --m_selectedIndex;
                    selectionChanged = true;
                }
                break;
            case KeyCode::S:    // S 键
            case KeyCode::Down: // 下箭头
                if (m_selectedIndex < static_cast<int>(m_displayNodes.size()) - 1) {
                    ++m_selectedIndex;
                    selectionChanged = true;
                }
                break;
            case KeyCode::J:
                AddObjectInteractive();
                selectionChanged = false;
                break;
            case KeyCode::K:
                DeleteSelectedObject();
                selectionChanged = false;
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
    if (m_hEvent == NULL || m_hEvent == INVALID_HANDLE_VALUE) {
        OutputDebugStringW(L"[LevelTreeList] ERROR: Event handle is invalid!\n");
        return;
    }
    while (m_running) {
        DWORD waitResult = WaitForSingleObject(m_hEvent, 0);
        if (waitResult == WAIT_OBJECT_0) {
            OutputDebugStringW(L"[LevelTreeList] Event signaled!\n");
            HandleOpenLevelEvent();
        }
        else if (waitResult == WAIT_TIMEOUT) {
            // normal
        }
        else {
            WCHAR errBuf[128];
            swprintf_s(errBuf, L"[LevelTreeList] WaitForSingleObject error: %lu, handle: %p\n", GetLastError(), m_hEvent);
            OutputDebugStringW(errBuf);
            m_running = false;
            break;
        }

        // 交互模式下不收集新输入事件
        if (!m_inInteractiveMode) {
            m_inputCollector->update();
        }
        ProcessInputEvents();
        m_inputSystem->clearEvent();
        std::this_thread::sleep_for(m_loopInterval);
    }
    OutputDebugStringW(L"[LevelTreeList] Exited cleanly.\n");
}

void LevelTreeList::Stop() {
    m_running = false;
}