// Copyright 2026 Nagato-Yuki-708. All Rights Reserved.

#include "DetailViewer.h"
#include <windows.h>
#include <string>
#include <thread>
#include <chrono>
#include <iostream>  // [新增] 用于 std::cout

// 共享内存大小定义（根据实际约定调整）
static const DWORD PATH_SHARED_MEM_SIZE = 4096; // 假设路径共享内存 4096 字节
static const DWORD OBJECT_SHARED_MEM_SIZE = 256;  // 假设对象名共享内存 256 字节

DetailViewer::DetailViewer()
    : m_hEventPathUpdate(nullptr)
    , m_hEventDataChanged(nullptr)
    , m_hEventObjectChanged(nullptr)
    , m_hMapPath(nullptr)
    , m_hMapObject(nullptr)
    , m_pViewPath(nullptr)
    , m_pViewObject(nullptr)
    , m_inputCollector(&m_inputSystem)
{
    // 打开全局事件对象（手动重置事件）
    m_hEventPathUpdate = OpenEventW(SYNCHRONIZE, FALSE, L"Global\\OFGal_Engine_LevelTreeList_DetailViewer_PathUpdate");
    m_hEventDataChanged = OpenEventW(SYNCHRONIZE, FALSE, L"Global\\OFGal_Engine_LevelTreeList_DetailViewer_DataChanged");
    m_hEventObjectChanged = OpenEventW(SYNCHRONIZE, FALSE, L"Global\\OFGal_Engine_LevelTreeList_DetailViewer_ObjectChanged");

    // 打开共享内存并映射到进程地址空间
    m_hMapPath = OpenFileMappingW(FILE_MAP_READ, FALSE, L"Global\\OFGal_Engine_LevelTreeList_DetailViewer_PathSharedMem");
    if (m_hMapPath) {
        m_pViewPath = MapViewOfFile(m_hMapPath, FILE_MAP_READ, 0, 0, PATH_SHARED_MEM_SIZE);
    }

    m_hMapObject = OpenFileMappingW(FILE_MAP_READ, FALSE, L"Global\\OFGal_Engine_LevelTreeList_DetailViewer_ObjectSharedMem");
    if (m_hMapObject) {
        m_pViewObject = MapViewOfFile(m_hMapObject, FILE_MAP_READ, 0, 0, OBJECT_SHARED_MEM_SIZE);
    }

    m_inputSystem.SetGlobalCapture(false);

    SetConsoleTitleW(L"OFGal_Engine/DetailViewer");
    ConfigureConsole();
    SetWindowSizeAndPosition();
}

DetailViewer::~DetailViewer()
{
    if (m_pViewPath)   UnmapViewOfFile(m_pViewPath);
    if (m_pViewObject) UnmapViewOfFile(m_pViewObject);
    if (m_hMapPath)    CloseHandle(m_hMapPath);
    if (m_hMapObject)  CloseHandle(m_hMapObject);

    if (m_hEventPathUpdate)   CloseHandle(m_hEventPathUpdate);
    if (m_hEventDataChanged)  CloseHandle(m_hEventDataChanged);
    if (m_hEventObjectChanged) CloseHandle(m_hEventObjectChanged);
}

// [新增] ANSI 清屏实现
void DetailViewer::ClearScreen() {
    // \x1b[2J 清除整个屏幕，\x1b[H 将光标移到左上角
    std::cout << "\x1b[2J\x1b[H" << std::flush;
}

void DetailViewer::ConfigureConsole() {
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

void DetailViewer::SetWindowSizeAndPosition() {
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
            static_cast<int>(0.0f),
            static_cast<int>(520.0f * scaleX),
            static_cast<int>(1480.0f * scaleY),
            SWP_NOZORDER | SWP_NOACTIVATE);
    }
}

void DetailViewer::Run()
{
    // ----- 首次运行时绑定按键 -----
    if (!m_bindingsAdded) {
        // w 和 上箭头 触发同一分支；s 和 下箭头 触发同一分支；F/J/K 各自触发独立分支
        // edgeOnly = true 表示只在按下瞬间产生事件
        m_inputCollector.AddBinding({ 'W',        Modifier::None, KeyCode::W,    true });
        m_inputCollector.AddBinding({ VK_UP,      Modifier::None, KeyCode::Up,   true });
        m_inputCollector.AddBinding({ 'S',        Modifier::None, KeyCode::S,    true });
        m_inputCollector.AddBinding({ VK_DOWN,    Modifier::None, KeyCode::Down, true });
        m_inputCollector.AddBinding({ 'F',        Modifier::None, KeyCode::F,    true });
        m_inputCollector.AddBinding({ 'J',        Modifier::None, KeyCode::J,    true });
        m_inputCollector.AddBinding({ 'K',        Modifier::None, KeyCode::K,    true });

        m_bindingsAdded = true;
    }

    // ----- 主轮询循环（周期 20ms） -----
    while (true) {
        // 1. 处理 PathUpdate 事件
        if (m_hEventPathUpdate &&
            WaitForSingleObject(m_hEventPathUpdate, 0) == WAIT_OBJECT_0)
        {
            if (m_pViewPath) {
                const wchar_t* wpath = static_cast<const wchar_t*>(m_pViewPath);
                std::wstring ws(wpath);

                // 宽字符转 UTF-8
                int len = WideCharToMultiByte(CP_UTF8, 0, ws.c_str(), -1, nullptr, 0, nullptr, nullptr);
                std::string utf8Path(len - 1, '\0');
                WideCharToMultiByte(CP_UTF8, 0, ws.c_str(), -1, &utf8Path[0], len, nullptr, nullptr);

                // 从 JSON 读取关卡
                LevelData newLevel = ReadLevelData(utf8Path);
                m_currentLevel = std::make_unique<LevelData>(std::move(newLevel));

                // 重置选中状态（场景重新载入，不选中具体对象）
                m_currentObject = nullptr;
                m_selectedComponent = { ComponentType::None, nullptr };

                OutputDebugStringA("[DetailViewer] PathUpdate: level reloaded.\n");
            }

            // [新增] 清屏并打印帮助信息和对象数量
            ClearScreen();
            std::cout << "W/up : last Component     S/down : next Component     J/Add Component     K/Delete Component     F/Edit  Component\n";
            if (m_currentLevel) {
                std::cout << "Objects in level: " << m_currentLevel->objects.size() << std::endl;
            }
            else {
                std::cout << "No level loaded." << std::endl;
            }

            ResetEvent(m_hEventPathUpdate);
        }

        // 2. 处理 ObjectChanged 事件
        if (m_hEventObjectChanged &&
            WaitForSingleObject(m_hEventObjectChanged, 0) == WAIT_OBJECT_0)
        {
            if (m_pViewObject && m_currentLevel) {
                const char* objName = static_cast<const char*>(m_pViewObject);
                std::string name(objName);

                auto it = m_currentLevel->objects.find(name);
                if (it != m_currentLevel->objects.end()) {
                    m_currentObject = it->second;

                    // 选取第一个存在的组件
                    m_selectedComponent = { ComponentType::None, nullptr };
                    if (m_currentObject->Transform.has_value()) {
                        m_selectedComponent.type = ComponentType::Transform;
                        m_selectedComponent.ptr = &(*m_currentObject->Transform);
                    }
                    else if (m_currentObject->Picture.has_value()) {
                        m_selectedComponent.type = ComponentType::Picture;
                        m_selectedComponent.ptr = &(*m_currentObject->Picture);
                    }
                    else if (m_currentObject->Textblock.has_value()) {
                        m_selectedComponent.type = ComponentType::Textblock;
                        m_selectedComponent.ptr = &(*m_currentObject->Textblock);
                    }
                    else if (m_currentObject->TriggerArea.has_value()) {
                        m_selectedComponent.type = ComponentType::TriggerArea;
                        m_selectedComponent.ptr = &(*m_currentObject->TriggerArea);
                    }
                    else if (m_currentObject->Blueprint.has_value()) {
                        m_selectedComponent.type = ComponentType::Blueprint;
                        m_selectedComponent.ptr = &(*m_currentObject->Blueprint);
                    }

                    OutputDebugStringA(("[DetailViewer] ObjectChanged: selected " + name + "\n").c_str());
                }
                else {
                    OutputDebugStringA(("[DetailViewer] ObjectChanged: object not found - " + name + "\n").c_str());
                    m_currentObject = nullptr;
                    m_selectedComponent = { ComponentType::None, nullptr };
                }
            }

            // [新增] 清屏并打印帮助信息和当前对象的组件数量
            ClearScreen();
            std::cout << "W/up : last Component     S/down : next Component     J/Add Component     K/Delete Component     F/Edit  Component\n";

            if (m_currentObject) {
                int componentCount = 0;
                if (m_currentObject->Transform.has_value())   componentCount++;
                if (m_currentObject->Picture.has_value())     componentCount++;
                if (m_currentObject->Textblock.has_value())   componentCount++;
                if (m_currentObject->TriggerArea.has_value()) componentCount++;
                if (m_currentObject->Blueprint.has_value())   componentCount++;
                std::cout << "Components in object \"" << m_currentObject->name << "\": " << componentCount << std::endl;
            }
            else {
                std::cout << "No object selected." << std::endl;
            }

            ResetEvent(m_hEventObjectChanged);
        }

        // 3. 更新输入并处理按键事件
        m_inputCollector.update();

        const auto& events = m_inputSystem.getEvents();
        for (const auto& ev : events) {
            if (ev.type == InputType::KeyDown) {
                switch (ev.key) {
                case KeyCode::W:
                case KeyCode::Up:
                    // 分支1（w / 上箭头） - 留空
                    OutputDebugStringA("[DetailViewer] Branch 1: W/Up\n");
                    break;

                case KeyCode::S:
                case KeyCode::Down:
                    // 分支2（s / 下箭头） - 留空
                    OutputDebugStringA("[DetailViewer] Branch 2: S/Down\n");
                    break;

                case KeyCode::F:
                    OutputDebugStringA("[DetailViewer] Branch F\n");
                    break;

                case KeyCode::J:
                    OutputDebugStringA("[DetailViewer] Branch J\n");
                    break;

                case KeyCode::K:
                    OutputDebugStringA("[DetailViewer] Branch K\n");
                    break;

                default:
                    break;
                }
            }
        }
        m_inputSystem.clearEvent();

        // 维持 20ms 周期
        Sleep(20);
    }
}