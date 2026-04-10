// Copyright 2026 MrSeagull. All Rights Reserved.
#include <iostream>
#define NOMINMAX
#include <windows.h>
#include <vector>
#include <string>
#include <conio.h>      // _kbhit, _getch
#include "ProjectStructureGetter.h"
#include "StructurePrinter.h"
#include "SharedTypes.h"

// 设置控制台窗口位置，大小，并启用虚拟终端模式（支持ANSI转义序列），同时禁用快速编辑模式
void SetupConsoleWindow() {
    HWND hwndConsole = GetConsoleWindow();
    if (hwndConsole) {
        SetWindowPos(hwndConsole, nullptr, 0, 0, 400, 400, SWP_NOZORDER | SWP_NOACTIVATE);

        HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
        if (hConsole != INVALID_HANDLE_VALUE) {
            // 获取当前控制台模式
            DWORD dwMode = 0;
            if (GetConsoleMode(hConsole, &dwMode)) {
                // 启用虚拟终端处理（支持ANSI转义序列）
                dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
                // 禁用快速编辑模式（防止鼠标单击移动光标）
                dwMode &= ~ENABLE_QUICK_EDIT_MODE;
                // 可选：禁用插入模式
                dwMode &= ~ENABLE_INSERT_MODE;
                SetConsoleMode(hConsole, dwMode);
            }

            CONSOLE_FONT_INFO fontInfo;
            if (GetCurrentConsoleFont(hConsole, FALSE, &fontInfo)) {
                COORD bufferSize;
                bufferSize.X = 120;
                bufferSize.Y = 1000;
                SetConsoleScreenBufferSize(hConsole, bufferSize);
            }
            else {
                COORD defaultSize = { 80, 25 };
                SetConsoleScreenBufferSize(hConsole, defaultSize);
            }
        }
    }
}

// 检查键盘输入（非阻塞），返回 true 表示处理了键盘事件（W/S/上下箭头移动选中行）
bool HandleKeyboardInput(int& currentLine, const ProjectStructure* pProject) {
    if (!_kbhit()) return false;

    int ch = _getch();
    int newLine = currentLine;
    bool handled = true;

    if (ch == 0 || ch == 224) { // 功能键前缀（上下箭头）
        ch = _getch();
        switch (ch) {
        case 72: // 上箭头
            newLine = std::max(0, currentLine - 1);
            break;
        case 80: // 下箭头
            newLine = std::min(GetTotalLines() - 1, currentLine + 1);
            break;
        default:
            handled = false;
            break;
        }
    }
    else {
        // 普通字符键
        switch (ch) {
        case 'w':
        case 'W':
            newLine = std::max(0, currentLine - 1);
            break;
        case 's':
        case 'S':
            newLine = std::min(GetTotalLines() - 1, currentLine + 1);
            break;
        default:
            handled = false;
            break;
        }
    }

    if (handled && newLine != currentLine) {
        currentLine = newLine;
        // 清屏并重新绘制，高亮新行
        std::cout << "\033[2J\033[H";
        PrintProjectStructureTree(*pProject, currentLine);
        // 打印选中信息
        const SelectedFolderInfo& info = GetSelectedFolderInfo();
        //std::cout << "\n当前选中: [" << info.lineNumber << "] " << info.absolutePath << std::endl;
        std::cout.flush();
        return true;
    }
    return false;
}

int main(int argc, char* argv[]) {
    ProjectStructure* pProjectStructure = nullptr;
    SetupConsoleWindow();
    InitConsoleVTMode();

    const char* projectRoot = nullptr;
    if (argc > 1) {
        projectRoot = argv[1];
        std::cout << "项目根目录: " << projectRoot << std::endl;
    }

    // 打开事件
    HANDLE hExitEvent = OpenEventA(EVENT_MODIFY_STATE | SYNCHRONIZE, FALSE,
        "Global\\OFGal_Engine_ProjectStructureViewer_Exit");
    HANDLE hUpdateEvent = OpenEventA(EVENT_MODIFY_STATE | SYNCHRONIZE, FALSE,
        "Global\\OFGal_Engine_ProjectStructureViewer_Refresh");
    if (!hExitEvent || !hUpdateEvent) {
        std::cerr << "打开事件失败，错误码: " << GetLastError() << std::endl;
        return -1;
    }

    // 打开共享内存
    HANDLE hMapFile = OpenFileMappingA(FILE_MAP_READ, FALSE,
        "Global\\OFGal_Engine_ProjectStructureViewer_Path");
    if (!hMapFile) {
        std::cerr << "打开共享内存失败，错误码: " << GetLastError() << std::endl;
        return -1;
    }
    char* pSharedBuf = static_cast<char*>(MapViewOfFile(hMapFile, FILE_MAP_READ, 0, 0, MAX_PATH));
    if (!pSharedBuf) {
        std::cerr << "映射共享内存视图失败，错误码: " << GetLastError() << std::endl;
        CloseHandle(hMapFile);
        return -1;
    }

    std::vector<HANDLE> events = { hExitEvent, hUpdateEvent };
    bool keepRunning = true;
    int currentSelectedLine = 0; // 当前高亮行号

    std::cout << "子进程已启动，等待事件..." << std::endl;

    // 首次读取共享内存中的路径（如果有）
    char localPath[MAX_PATH] = { 0 };
    memcpy(localPath, pSharedBuf, MAX_PATH - 1);
    localPath[MAX_PATH - 1] = '\0';
    std::string currentPath(localPath);
    if (!currentPath.empty()) {
        pProjectStructure = new ProjectStructure(GetProjectStructure(currentPath.c_str()));
        std::cout << "\033[2J\033[H";
        PrintProjectStructureTree(*pProjectStructure, currentSelectedLine);
        const SelectedFolderInfo& info = GetSelectedFolderInfo();
        //std::cout << "\n当前选中: [" << info.lineNumber << "] " << info.absolutePath << std::endl;
        std::cout.flush();
    }

    while (keepRunning) {
        // 使用超时100ms等待事件，以便周期性检查键盘输入
        DWORD waitResult = WaitForMultipleObjects(
            static_cast<DWORD>(events.size()),
            events.data(),
            FALSE,
            100 // 超时100ms
        );

        if (waitResult == WAIT_OBJECT_0) {
            std::cout << "收到退出信号，子进程退出。" << std::endl;
            keepRunning = false;
        }
        else if (waitResult == WAIT_OBJECT_0 + 1) {
            std::cout << "收到更新信号，刷新文件夹列表..." << std::endl;

            // 安全读取共享内存
            char localPath[MAX_PATH] = { 0 };
            memcpy(localPath, pSharedBuf, MAX_PATH - 1);
            localPath[MAX_PATH - 1] = '\0';
            std::string newPath(localPath);

            if (newPath.empty()) {
                std::cout << "警告：共享内存中路径为空！" << std::endl;
            }
            else {
                std::cout << "当前项目路径: " << newPath << std::endl;
                delete pProjectStructure;
                pProjectStructure = new ProjectStructure(GetProjectStructure(newPath.c_str()));
                currentSelectedLine = 0; // 刷新后选中根目录
                std::cout << "\033[2J\033[H";
                PrintProjectStructureTree(*pProjectStructure, currentSelectedLine);
                const SelectedFolderInfo& info = GetSelectedFolderInfo();
                //std::cout << "\n当前选中: [" << info.lineNumber << "] " << info.absolutePath << std::endl;
                std::cout.flush();
            }
        }
        else if (waitResult == WAIT_TIMEOUT) {
            // 超时，检查键盘输入（仅当已加载项目结构时）
            if (pProjectStructure) {
                HandleKeyboardInput(currentSelectedLine, pProjectStructure);
            }
        }
        else if (waitResult == WAIT_FAILED) {
            std::cerr << "WaitForMultipleObjects 失败，错误码: " << GetLastError() << std::endl;
            break;
        }
        else {
            std::cerr << "未知的等待结果" << std::endl;
            break;
        }
    }

    UnmapViewOfFile(pSharedBuf);
    CloseHandle(hMapFile);
    CloseHandle(hExitEvent);
    CloseHandle(hUpdateEvent);
    delete pProjectStructure;
    return 0;
}