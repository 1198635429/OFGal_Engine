// Copyright 2026 MrSeagull. All Rights Reserved.
#include "WindowsSystem.h"
#include <sstream>
#include <cstring>
#include <vector>

WindowsSystem::WindowsSystem() {
    OpenProjectStructureViewer(exePath_ProjectStructureViewer.c_str());
    RefreshProjectStructureViewer();
    
    OpenLevelTreeList();
}
WindowsSystem::WindowsSystem(std::string path) {
    size_t len = path.size() + 1;
    currentProjectDirectory = new char[len];
    strcpy_s(currentProjectDirectory, len, path.c_str());
    
    OpenProjectStructureViewer(exePath_ProjectStructureViewer.c_str());
    RefreshProjectStructureViewer();

    OpenLevelTreeList();
    
}

WindowsSystem::~WindowsSystem() {
    // 先拷贝所有子进程的键，避免迭代器失效
    std::vector<std::string> keys;
    keys.reserve(childProcesses.size());
    for (const auto& pair : childProcesses) {
        keys.push_back(pair.first);
    }
    // 安全地逐个终止
    for (const auto& key : keys) {
        TerminateChildProcess(key, 1000);
    }
    if (m_hLevelTreeListPathUpdateEvent) {
        CloseHandle(m_hLevelTreeListPathUpdateEvent);
    }
    delete[] currentProjectDirectory;
}

// ---------- 通用接口实现 ----------

std::string WindowsSystem::MakeGlobalName(const std::string& processKey, const std::string& suffix) {
    return "Global\\OFGal_Engine_" + processKey + "_" + suffix;
}

bool WindowsSystem::CreateSharedMemoryBlock(const std::string& processKey, const std::string& blockName, DWORD size, bool readOnly) {
    auto it = childProcesses.find(processKey);
    if (it == childProcesses.end()) return false;
    ChildProcessInfo& info = it->second;

    // 如果块已存在，先清理
    auto blockIt = info.sharedMems.find(blockName);
    if (blockIt != info.sharedMems.end()) {
        SharedMemBlock& block = blockIt->second;
        if (block.pView) UnmapViewOfFile(block.pView);
        if (block.hSharedMem) CloseHandle(block.hSharedMem);
        info.sharedMems.erase(blockIt);
    }

    std::string globalName = MakeGlobalName(processKey, blockName);
    DWORD flProtect = readOnly ? PAGE_READONLY : PAGE_READWRITE;
    HANDLE hMap = CreateFileMappingA(INVALID_HANDLE_VALUE, NULL, flProtect, 0, size, globalName.c_str());
    if (!hMap) {
        OutputDebugStringA(("CreateFileMapping failed: " + globalName + " error: " + std::to_string(GetLastError())).c_str());
        return false;
    }

    DWORD dwAccess = readOnly ? FILE_MAP_READ : FILE_MAP_WRITE;
    char* pView = static_cast<char*>(MapViewOfFile(hMap, dwAccess, 0, 0, size));
    if (!pView) {
        OutputDebugStringA("MapViewOfFile failed");
        CloseHandle(hMap);
        return false;
    }

    if (!readOnly) {
        memset(pView, 0, size); // 写入方清零
    }

    SharedMemBlock block;
    block.hSharedMem = hMap;
    block.pView = pView;
    block.size = size;
    block.isReadOnly = readOnly;
    info.sharedMems[blockName] = block;

    return true;
}

bool WindowsSystem::CreateProcessEvent(const std::string& processKey, const std::string& eventName, bool initialState) {
    auto it = childProcesses.find(processKey);
    if (it == childProcesses.end()) return false;
    ChildProcessInfo& info = it->second;

    if (info.events.find(eventName) != info.events.end()) {
        // 已存在，直接返回成功
        return true;
    }

    std::string globalName = MakeGlobalName(processKey, eventName);
    HANDLE hEvent = CreateEventA(NULL, FALSE, initialState ? TRUE : FALSE, globalName.c_str());
    if (!hEvent) {
        OutputDebugStringA(("CreateEvent failed: " + globalName).c_str());
        return false;
    }

    info.events[eventName] = hEvent;
    return true;
}

bool WindowsSystem::LaunchChildProcessW(const ChildProcessConfig& config) {
    const std::string& key = config.processKey;
    if (childProcesses.find(key) != childProcesses.end()) {
        TerminateChildProcess(key, 1000);
    }

    // 先插入一个空的 ChildProcessInfo，以便后续函数能通过 find 找到并操作它
    ChildProcessInfo info;
    childProcesses[key] = info;
    ChildProcessInfo& storedInfo = childProcesses[key];

    // 1. 创建事件（现在 childProcesses 中已有 key）
    std::unordered_map<std::string, bool> eventsToCreate = config.eventsToCreate;
    if (eventsToCreate.find("Exit") == eventsToCreate.end()) {
        eventsToCreate["Exit"] = false;
    }
    for (const auto& ev : eventsToCreate) {
        if (!CreateProcessEvent(key, ev.first, ev.second)) {
            CleanupChildProcess(key);
            return false;
        }
    }

    // 2. 创建共享内存块
    for (const auto& blk : config.sharedMemBlocks) {
        if (!CreateSharedMemoryBlock(key, blk.first, blk.second, false)) {
            CleanupChildProcess(key);
            return false;
        }
    }

    // 3. 构建完整命令行（宽字符串）
    std::wstring fullCmdLine = L"\"" + config.exePath + L"\"";
    if (!config.commandLineArgs.empty()) {
        fullCmdLine += L" " + config.commandLineArgs;
    }

    // 为 CreateProcessW 准备可修改的缓冲区
    std::vector<wchar_t> cmdBuffer(fullCmdLine.begin(), fullCmdLine.end());
    cmdBuffer.push_back(L'\0');

    // 4. 设置启动信息（宽字符版本）
    STARTUPINFOW si = { sizeof(STARTUPINFOW) };
    PROCESS_INFORMATION pi = { 0 };

    DWORD creationFlags = 0;
    if (config.createNewConsole) {
        creationFlags |= CREATE_NEW_CONSOLE;
    }
    // redirectStdIO 与 CREATE_NEW_CONSOLE 组合时无需额外标志，新控制台自动接管

    BOOL success = CreateProcessW(
        nullptr,                     // lpApplicationName
        cmdBuffer.data(),            // lpCommandLine（可修改的宽字符缓冲区）
        nullptr, nullptr, FALSE,
        creationFlags,
        nullptr, nullptr,
        &si, &pi
    );

    if (!success) {
        // 输出错误信息（宽字符转 ANSI 仅用于调试）
        OutputDebugStringA(("CreateProcessW failed, error: " + std::to_string(GetLastError())).c_str());
        CleanupChildProcess(key);
        return false;
    }

    storedInfo.hProcess = pi.hProcess;
    storedInfo.hThread = pi.hThread;
    storedInfo.processId = pi.dwProcessId;

    // 关闭线程句柄（保留以便后续 WaitForSingleObject）
    // 不在此处关闭，保存在 storedInfo 中

    return true;
}

bool WindowsSystem::WriteToSharedMemory(const std::string& processKey, const std::string& blockName, const void* data, size_t dataSize) {
    auto it = childProcesses.find(processKey);
    if (it == childProcesses.end()) return false;
    auto& info = it->second;

    auto blockIt = info.sharedMems.find(blockName);
    if (blockIt == info.sharedMems.end()) return false;
    SharedMemBlock& block = blockIt->second;

    if (!block.pView || block.isReadOnly) return false;

    size_t copySize = (dataSize < block.size) ? dataSize : block.size;
    memcpy(block.pView, data, copySize);
    return true;
}

size_t WindowsSystem::ReadFromSharedMemory(const std::string& processKey, const std::string& blockName, void* outBuffer, size_t bufferSize) {
    auto it = childProcesses.find(processKey);
    if (it == childProcesses.end()) return 0;
    auto& info = it->second;

    auto blockIt = info.sharedMems.find(blockName);
    if (blockIt == info.sharedMems.end()) return 0;
    SharedMemBlock& block = blockIt->second;

    if (!block.pView) return 0;

    size_t copySize = (bufferSize < block.size) ? bufferSize : block.size;
    memcpy(outBuffer, block.pView, copySize);
    return copySize;
}

bool WindowsSystem::SignalEvent(const std::string& processKey, const std::string& eventName) {
    auto it = childProcesses.find(processKey);
    if (it == childProcesses.end()) return false;
    auto& info = it->second;

    auto evIt = info.events.find(eventName);
    if (evIt == info.events.end()) return false;
    return SetEvent(evIt->second) != 0;
}

bool WindowsSystem::ResetEvent(const std::string& processKey, const std::string& eventName) {
    auto it = childProcesses.find(processKey);
    if (it == childProcesses.end()) return false;
    auto& info = it->second;

    auto evIt = info.events.find(eventName);
    if (evIt == info.events.end()) return false;
    return ::ResetEvent(evIt->second) != 0;
}

bool WindowsSystem::WaitForChildExit(const std::string& processKey, DWORD timeoutMs) {
    auto it = childProcesses.find(processKey);
    if (it == childProcesses.end()) return true; // 已不存在
    HANDLE hProcess = it->second.hProcess;
    if (!hProcess) return true;
    DWORD ret = WaitForSingleObject(hProcess, timeoutMs);
    return (ret == WAIT_OBJECT_0);
}

bool WindowsSystem::TerminateChildProcess(const std::string& processKey, DWORD gracefulTimeoutMs) {
    auto it = childProcesses.find(processKey);
    if (it == childProcesses.end()) return true;

    ChildProcessInfo& info = it->second;

    // 发送 Exit 事件（如果存在）
    auto evIt = info.events.find("Exit");
    if (evIt != info.events.end() && evIt->second) {
        SetEvent(evIt->second);
    }

    // 等待进程自行退出
    if (info.hProcess) {
        DWORD waitResult = WaitForSingleObject(info.hProcess, gracefulTimeoutMs);
        if (waitResult == WAIT_TIMEOUT) {
            TerminateProcess(info.hProcess, 0);
        }
    }

    CleanupChildProcess(processKey);
    return true;
}

bool WindowsSystem::IsChildRunning(const std::string& processKey) {
    auto it = childProcesses.find(processKey);
    if (it == childProcesses.end()) return false;
    HANDLE hProcess = it->second.hProcess;
    if (!hProcess) return false;
    DWORD exitCode;
    if (GetExitCodeProcess(hProcess, &exitCode)) {
        return (exitCode == STILL_ACTIVE);
    }
    return false;
}

DWORD WindowsSystem::GetSharedMemorySize(const std::string& processKey, const std::string& blockName) {
    auto it = childProcesses.find(processKey);
    if (it == childProcesses.end()) return 0;
    auto& info = it->second;
    auto blkIt = info.sharedMems.find(blockName);
    if (blkIt == info.sharedMems.end()) return 0;
    return blkIt->second.size;
}

void WindowsSystem::CleanupChildProcess(const std::string& processKey) {
    auto it = childProcesses.find(processKey);
    if (it == childProcesses.end()) return;
    ChildProcessInfo& info = it->second;

    // 关闭事件句柄
    for (auto& pair : info.events) {
        if (pair.second) CloseHandle(pair.second);
    }
    info.events.clear();

    // 卸载共享内存
    for (auto& pair : info.sharedMems) {
        SharedMemBlock& block = pair.second;
        if (block.pView) UnmapViewOfFile(block.pView);
        if (block.hSharedMem) CloseHandle(block.hSharedMem);
    }
    info.sharedMems.clear();

    // 关闭进程/线程句柄
    if (info.hThread) CloseHandle(info.hThread);
    if (info.hProcess) CloseHandle(info.hProcess);

    childProcesses.erase(it);
}

bool WindowsSystem::OpenProjectStructureViewer(const wchar_t* ViewerExePath, const wchar_t* ProjectRoot) {
    ChildProcessConfig config;
    config.processKey = "ProjectStructureViewer";
    config.exePath = ViewerExePath;
    if (ProjectRoot && wcslen(ProjectRoot) > 0) {
        config.commandLineArgs = std::wstring(L"\"") + ProjectRoot + L"\"";
    }
    config.createNewConsole = true;
    config.redirectStdIO = true;

    // 原有：ProjectStructureViewer 自身使用的共享内存和事件
    config.sharedMemBlocks["Path"] = MAX_PATH;
    config.eventsToCreate["Exit"] = false;
    config.eventsToCreate["Refresh"] = false;

    if (!LaunchChildProcessW(config)) {
        return false;
    }

    if (currentProjectDirectory) {
        WriteToSharedMemory("ProjectStructureViewer", "Path", currentProjectDirectory, strlen(currentProjectDirectory) + 1);
    }

    // === 新增：为 FolderViewer 创建 IPC 对象 ===
    const std::string key = "ProjectStructureViewer";
    auto it = childProcesses.find(key);
    if (it == childProcesses.end()) return false;
    ChildProcessInfo& info = it->second;

    // 定义三个通道的名称后缀
    const std::vector<std::pair<std::string, std::string>> channels = {
        {"OpenLevel", "OpenLevelPath"},
        {"OpenBlueprint", "OpenBlueprintPath"},
        {"OpenText", "OpenTextPath"}
    };

    for (const auto& ch : channels) {
        const std::string& eventSuffix = ch.first;
        const std::string& memSuffix = ch.second;

        // 构造 FolderViewer 期望的全局名称
        std::string globalEventName = "Global\\OFGal_Engine_ProjectStructureViewer_FolderViewer_" + eventSuffix;
        std::string globalMemName = "Global\\OFGal_Engine_ProjectStructureViewer_FolderViewer_" + memSuffix;

        // 创建事件（手动重置？此处使用自动重置事件，与 FolderViewer 中一致）
        HANDLE hEvent = CreateEventA(NULL, FALSE, FALSE, globalEventName.c_str());
        if (!hEvent) {
            OutputDebugStringA(("CreateEvent failed for FolderViewer: " + globalEventName).c_str());
            return false;
        }
        info.events[eventSuffix] = hEvent; // 存入 events 映射，以便 Run() 中访问

        // 创建共享内存（大小 4096 字节，足够存放路径）
        const DWORD memSize = 4096;
        HANDLE hMap = CreateFileMappingA(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, memSize, globalMemName.c_str());
        if (!hMap) {
            OutputDebugStringA(("CreateFileMapping failed for FolderViewer: " + globalMemName).c_str());
            CloseHandle(hEvent);
            return false;
        }

        char* pView = static_cast<char*>(MapViewOfFile(hMap, FILE_MAP_WRITE, 0, 0, memSize));
        if (!pView) {
            OutputDebugStringA("MapViewOfFile failed for FolderViewer");
            CloseHandle(hMap);
            CloseHandle(hEvent);
            return false;
        }
        memset(pView, 0, memSize);

        SharedMemBlock block;
        block.hSharedMem = hMap;
        block.pView = pView;
        block.size = memSize;
        block.isReadOnly = false;
        info.sharedMems[memSuffix] = block;
    }

    return true;
}
bool WindowsSystem::OpenLevelTreeList() {
    ChildProcessConfig config;
    config.processKey = "LevelTreeList";
    config.exePath = exePath_LevelTreeList;
    // 如果需要传递命令行参数，可以设置 config.commandLineArgs
    config.createNewConsole = true;      // 必须分配新的控制台窗口
    config.redirectStdIO = true;         // 与 ProjectStructureViewer 保持一致

    // 可选：添加一个 Exit 事件，便于后续优雅关闭
    config.eventsToCreate["Exit"] = false;

    // 如果子进程需要共享内存，可以在此添加 config.sharedMemBlocks

    if (!LaunchChildProcessW(config)) {
        OutputDebugStringA("[WindowsSystem] Failed to launch LevelTreeList.exe\n");
        return false;
    }

    std::string levelTreeEventName = "Global\\OFGal_Engine_LevelTreeList_PathUpdate";
    m_hLevelTreeListPathUpdateEvent = CreateEventA(NULL, TRUE, FALSE, levelTreeEventName.c_str());
    if (!m_hLevelTreeListPathUpdateEvent)
        OutputDebugStringA(("CreateEvent failed for LevelTreeList: " + levelTreeEventName).c_str());

    OutputDebugStringA("[WindowsSystem] LevelTreeList.exe launched successfully\n");
    return true;
}

void WindowsSystem::Run() {
    const std::string key = "ProjectStructureViewer";
    auto it = childProcesses.find(key);
    if (it == childProcesses.end()) return;

    ChildProcessInfo& info = it->second;

    // 获取三个事件句柄（FolderViewer 专用）
    HANDLE hOpenLevel = info.events["OpenLevel"];
    HANDLE hOpenBlueprint = info.events["OpenBlueprint"];
    HANDLE hOpenText = info.events["OpenText"];
    HANDLE hExit = info.events["Exit"]; // 原有 Exit 事件

    HANDLE events[4] = { hOpenLevel, hOpenBlueprint, hOpenText, hExit };
    const DWORD numEvents = 4;

    while (true) {
        DWORD result = WaitForMultipleObjects(numEvents, events, FALSE, INFINITE);
        if (result == WAIT_OBJECT_0 + 3) // Exit 事件
            break;

        std::wstring* targetPath = nullptr;
        std::string blockName;

        if (result == WAIT_OBJECT_0) {
            targetPath = &m_lastOpenedLevelPath;
            blockName = "OpenLevelPath";
            if (m_hLevelTreeListPathUpdateEvent) {
                SetEvent(m_hLevelTreeListPathUpdateEvent);
            }
        }
        else if (result == WAIT_OBJECT_0 + 1) {
            targetPath = &m_lastOpenedBlueprintPath;
            blockName = "OpenBlueprintPath";
        }
        else if (result == WAIT_OBJECT_0 + 2) {
            targetPath = &m_lastOpenedTextPath;
            blockName = "OpenTextPath";
        }
        else {
            continue;
        }

        auto blockIt = info.sharedMems.find(blockName);
        if (blockIt != info.sharedMems.end() && blockIt->second.pView) {
            // 共享内存中存储的是 WCHAR 字符串
            const WCHAR* pWide = reinterpret_cast<const WCHAR*>(blockIt->second.pView);
            *targetPath = std::wstring(pWide);
            // 可选：输出调试信息或调用回调
            OutputDebugStringW((std::wstring(L"[OFGal_Engine] Opened file: ") + *targetPath + L"\n").c_str());
        }
    }
}

bool WindowsSystem::RefreshProjectStructureViewer() {
    const std::string key = "ProjectStructureViewer";
    if (!IsChildRunning(key)) return false;

    // 更新路径
    if (currentProjectDirectory) {
        WriteToSharedMemory(key, "Path", currentProjectDirectory, strlen(currentProjectDirectory) + 1);
    }
    // 触发 Refresh 事件
    return SignalEvent(key, "Refresh");
}

bool WindowsSystem::CloseProjectStructureViewer() {
    return TerminateChildProcess("ProjectStructureViewer", 1000);
}