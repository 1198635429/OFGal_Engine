// Copyright 2026 MrSeagull. All Rights Reserved.
#include <iostream>
#include <windows.h>
#include <vector>
#include <string>
#include "ProjectStructureGetter.h"
#include "StructurePrinter.h"
#include "SharedTypes.h"

// 设置控制台窗口位置，大小
void SetupConsoleWindow() {
    HWND hwndConsole = GetConsoleWindow();
    if (hwndConsole) {
        SetWindowPos(hwndConsole, nullptr, 0, 0, 400, 400, SWP_NOZORDER | SWP_NOACTIVATE);

        HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
        if (hConsole != INVALID_HANDLE_VALUE) {
            CONSOLE_FONT_INFO fontInfo;
            if (GetCurrentConsoleFont(hConsole, FALSE, &fontInfo)) {
                COORD bufferSize;
                //bufferSize.X = static_cast<SHORT>(400 / fontInfo.dwFontSize.X);
                //bufferSize.Y = static_cast<SHORT>(400 / fontInfo.dwFontSize.Y);
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

int main(int argc, char* argv[]) {
    ProjectStructure* pProjectStructure = nullptr;
    SetupConsoleWindow();

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

    std::cout << "子进程已启动，等待事件..." << std::endl;

    while (keepRunning) {
        DWORD waitResult = WaitForMultipleObjects(
            static_cast<DWORD>(events.size()),
            events.data(),
            FALSE,
            INFINITE
        );

        if (waitResult == WAIT_OBJECT_0) {
            std::cout << "收到退出信号，子进程退出。" << std::endl;
            keepRunning = false;
        }
        else if (waitResult == WAIT_OBJECT_0 + 1) {
            std::cout << "收到更新信号，刷新文件夹列表..." << std::endl;

            // 安全读取：最多拷贝 MAX_PATH-1 个字符
            char localPath[MAX_PATH] = { 0 };
            memcpy(localPath, pSharedBuf, MAX_PATH - 1);
            localPath[MAX_PATH - 1] = '\0';
            std::string newPath(localPath);

            if (newPath.empty()) {
                std::cout << "警告：共享内存中路径为空！" << std::endl;
            }
            else {
                std::cout << "当前项目路径: " << newPath << std::endl;
                pProjectStructure = new ProjectStructure(GetProjectStructure(newPath.c_str()));
                system("cls");
                PrintProjectStructureTree(*pProjectStructure);
            }
            std::cout.flush();
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