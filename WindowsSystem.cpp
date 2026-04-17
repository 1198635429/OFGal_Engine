// Copyright 2026 MrSeagull. All Rights Reserved.
#include "WindowsSystem.h"
#include <sstream>
#include <cstring>

WindowsSystem::WindowsSystem() {
    // 初始化项目目录（测试用）
    std::string testpath = "E:\\Projects\\C++Projects\\OFGal_Engine\\docs";
    size_t len = testpath.size() + 1;
    currentProjectDirectory = new char[len];
    strcpy_s(currentProjectDirectory, len, testpath.c_str());

    OpenProjectStructureViewer(exePath_ProjectStructureViewer);
    RefreshProjectStructureViewer();
}

WindowsSystem::~WindowsSystem() {
    // 关闭所有子进程
    for (auto& pair : childProcesses) {
        TerminateChildProcess(pair.first, 1000);
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

bool WindowsSystem::LaunchChildProcess(const ChildProcessConfig& config) {
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

    // 3. 构建完整命令行
    std::string fullCmdLine = "\"" + config.exePath + "\"";
    if (!config.commandLineArgs.empty()) {
        fullCmdLine += " " + config.commandLineArgs;
    }

    // 4. 设置启动信息
    STARTUPINFOA si = { sizeof(STARTUPINFOA) };
    PROCESS_INFORMATION pi = { 0 };

    DWORD creationFlags = 0;
    if (config.createNewConsole) {
        creationFlags |= CREATE_NEW_CONSOLE;
    }
    if (config.redirectStdIO && config.createNewConsole) {
        // 无需特殊操作，新控制台自动接管
    }

    BOOL success = CreateProcessA(
        NULL,
        const_cast<LPSTR>(fullCmdLine.c_str()),
        NULL, NULL,
        FALSE,
        creationFlags,
        NULL, NULL,
        &si, &pi
    );

    if (!success) {
        OutputDebugStringA(("CreateProcess failed: " + fullCmdLine + " error: " + std::to_string(GetLastError())).c_str());
        CleanupChildProcess(key);
        return false;
    }

    storedInfo.hProcess = pi.hProcess;
    storedInfo.hThread = pi.hThread;
    storedInfo.processId = pi.dwProcessId;

    // 关闭线程句柄（可选，我们保留以便 WaitForSingleObject）
    // 不关闭，保存在 storedInfo 中

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
    // 若写入字符串且未占满，可选择添加终止符，但二进制数据不应自动添加。
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

// ---------- 便捷包装：ProjectStructureViewer ----------

bool WindowsSystem::OpenProjectStructureViewer(const wchar_t* ViewerExePath, const wchar_t* ProjectRoot) {
    ChildProcessConfig config;
    config.processKey = "ProjectStructureViewer";
    config.exePath = ViewerExePath;
    if (ProjectRoot && wcslen(ProjectRoot) > 0) {
        config.commandLineArgs = std::wstring(L"\"") + ProjectRoot + L"\"";
    }
    config.createNewConsole = true;
    config.redirectStdIO = true;

    config.sharedMemBlocks["Path"] = MAX_PATH;
    config.eventsToCreate["Exit"] = false;
    config.eventsToCreate["Refresh"] = false;

    if (!LaunchChildProcessW(config)) {   // 使用宽字符启动函数
        return false;
    }

    if (currentProjectDirectory) {
        WriteToSharedMemory("ProjectStructureViewer", "Path", currentProjectDirectory, strlen(currentProjectDirectory) + 1);
    }
    return true;
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