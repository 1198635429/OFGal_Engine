// Copyright 2026 MrSeagull. All Rights Reserved.
#include <iostream>
#include <windows.h>
#include "ProjectStructureGetter.h"
#include "SharedTypes.h"

int main() {
    ProjectStructure* pProjectStructure = new ProjectStructure;

    // 创建事件（自动重置，初始无信号）
    HANDLE hExitEvent = CreateEventA(nullptr, FALSE, FALSE, "Global\\FolderViewerExit");
    HANDLE hUpdateEvent = CreateEventA(nullptr, FALSE, FALSE, "Global\\FolderViewerUpdate");
    if (!hExitEvent || !hUpdateEvent) {
        std::cerr << "创建事件失败，错误码: " << GetLastError() << std::endl;
        return 1;
    }

    std::vector<HANDLE> events = { hExitEvent, hUpdateEvent };
    bool keepRunning = true;

    std::cout << "子进程已启动，等待事件..." << std::endl;

    while (keepRunning) {
        DWORD waitResult = WaitForMultipleObjects(
            static_cast<DWORD>(events.size()),
            events.data(),
            FALSE,          // 等待任意一个事件
            INFINITE        // 无限等待
        );

        if (waitResult == WAIT_OBJECT_0) {
            // 退出事件被触发
            std::cout << "收到退出信号，子进程退出。" << std::endl;
            keepRunning = false;
        }
        else if (waitResult == WAIT_OBJECT_0 + 1) {
            // 更新事件被触发
            std::cout << "收到更新信号，刷新文件夹列表..." << std::endl;
            // TODO: 从主进程获取最新的 ProjectStructure 并重新绘制
            // 例如通过共享内存、管道等读取数据，然后刷新控制台显示
        }
        else if (waitResult == WAIT_FAILED) {
            std::cerr << "WaitForMultipleObjects 失败，错误码: " << GetLastError() << std::endl;
            break;
        }
        else {
            // 理论上不会到这里（因为只有两个事件）
            std::cerr << "未知的等待结果" << std::endl;
            break;
        }
    }

    CloseHandle(hExitEvent);
    CloseHandle(hUpdateEvent);
    return 0;
}