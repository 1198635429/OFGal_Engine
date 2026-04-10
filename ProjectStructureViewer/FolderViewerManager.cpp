// Copyright 2026 MrSeagull. All Rights Reserved.
#include "FolderViewerManager.h"
#include "Debug.h"
#include <cstring>
#include <sstream>

FolderViewerManager::FolderViewerManager()
    : m_hProcess(nullptr)
    , m_hThread(nullptr)
    , m_processId(0)
    , m_hSharedMem(nullptr)
    , m_pSharedView(nullptr)
    , m_hEventUpdatePath(nullptr)
    , m_hEventFolderChanged(nullptr)
    , m_hEventExit(nullptr)
{
}

FolderViewerManager::~FolderViewerManager() {
    Shutdown();
}

std::string FolderViewerManager::MakeGlobalName(const std::string& suffix) const {
    return "Global\\OFGal_Engine_ProjectStructureViewer_" + std::string(PROCESS_KEY) + "_" + suffix;
}

bool FolderViewerManager::CreateIPCResources() {
    // 创建共享内存
    std::string memName = MakeGlobalName("Path");
    m_hSharedMem = CreateFileMappingA(
        INVALID_HANDLE_VALUE,
        nullptr,
        PAGE_READWRITE,
        0,
        SHARED_MEM_SIZE,
        memName.c_str()
    );
    if (!m_hSharedMem) {
        DEBUG_LOG("FolderViewerManager: CreateFileMapping failed, error=" << GetLastError() << "\n");
        return false;
    }

    m_pSharedView = static_cast<char*>(MapViewOfFile(m_hSharedMem, FILE_MAP_WRITE, 0, 0, SHARED_MEM_SIZE));
    if (!m_pSharedView) {
        DEBUG_LOG("FolderViewerManager: MapViewOfFile failed, error=" << GetLastError() << "\n");
        return false;
    }

    // 创建事件（均为自动重置）
    std::string updateName = MakeGlobalName("UpdatePath");
    m_hEventUpdatePath = CreateEventA(nullptr, FALSE, FALSE, updateName.c_str());
    if (!m_hEventUpdatePath) {
        DEBUG_LOG("FolderViewerManager: CreateEvent(UpdatePath) failed, error=" << GetLastError() << "\n");
        return false;
    }

    std::string changeName = MakeGlobalName("FolderChanged");
    m_hEventFolderChanged = CreateEventA(nullptr, FALSE, FALSE, changeName.c_str());
    if (!m_hEventFolderChanged) {
        DEBUG_LOG("FolderViewerManager: CreateEvent(FolderChanged) failed, error=" << GetLastError() << "\n");
        return false;
    }

    std::string exitName = MakeGlobalName("Exit");
    m_hEventExit = CreateEventA(nullptr, FALSE, FALSE, exitName.c_str());
    if (!m_hEventExit) {
        DEBUG_LOG("FolderViewerManager: CreateEvent(Exit) failed, error=" << GetLastError() << "\n");
        return false;
    }

    return true;
}

void FolderViewerManager::CleanupIPC() {
    if (m_pSharedView) {
        UnmapViewOfFile(m_pSharedView);
        m_pSharedView = nullptr;
    }
    if (m_hSharedMem) {
        CloseHandle(m_hSharedMem);
        m_hSharedMem = nullptr;
    }
    if (m_hEventUpdatePath) {
        CloseHandle(m_hEventUpdatePath);
        m_hEventUpdatePath = nullptr;
    }
    if (m_hEventFolderChanged) {
        CloseHandle(m_hEventFolderChanged);
        m_hEventFolderChanged = nullptr;
    }
    if (m_hEventExit) {
        CloseHandle(m_hEventExit);
        m_hEventExit = nullptr;
    }
}

void FolderViewerManager::CleanupProcess() {
    if (m_hThread) {
        CloseHandle(m_hThread);
        m_hThread = nullptr;
    }
    if (m_hProcess) {
        CloseHandle(m_hProcess);
        m_hProcess = nullptr;
    }
    m_processId = 0;
}

bool FolderViewerManager::Start(const std::string& initialPath) {
    if (IsRunning()) {
        DEBUG_LOG("FolderViewerManager: Already running\n");
        return true;
    }

    // 先清理可能残留的 IPC 资源（以防前次异常退出）
    CleanupIPC();

    // 创建 IPC 资源
    if (!CreateIPCResources()) {
        CleanupIPC();
        return false;
    }

    // 写入初始路径
    if (!initialPath.empty()) {
        size_t len = initialPath.size() + 1;
        if (len > SHARED_MEM_SIZE) len = SHARED_MEM_SIZE;
        memcpy(m_pSharedView, initialPath.c_str(), len);
        m_pSharedView[SHARED_MEM_SIZE - 1] = '\0';
    }

    // 构建命令行（FolderViewer.exe 可能需要接受一些参数，此处仅传递 exe 路径）
    std::string cmdLine = std::string("\"") + FOLDER_VIEWER_EXE_PATH + "\"";

    STARTUPINFOA si = { sizeof(STARTUPINFOA) };
    PROCESS_INFORMATION pi = { 0 };

    // 创建新控制台窗口，标准输入输出自动关联到该窗口
    DWORD creationFlags = CREATE_NEW_CONSOLE;

    BOOL success = CreateProcessA(
        nullptr,
        const_cast<LPSTR>(cmdLine.c_str()),
        nullptr,
        nullptr,
        FALSE,
        creationFlags,
        nullptr,
        nullptr,
        &si,
        &pi
    );

    if (!success) {
        DEBUG_LOG("FolderViewerManager: CreateProcess failed, error=" << GetLastError() << "\n");
        CleanupIPC();
        return false;
    }

    m_hProcess = pi.hProcess;
    m_hThread = pi.hThread;
    m_processId = pi.dwProcessId;

    DEBUG_LOG("FolderViewerManager: Started FolderViewer.exe (PID=" << m_processId << ")\n");
    return true;
}

bool FolderViewerManager::NotifyPathChange(const std::string& newPath) {
    if (!IsRunning() || !m_pSharedView || !m_hEventUpdatePath) {
        return false;
    }

    // 写入共享内存
    size_t len = newPath.size() + 1;
    if (len > SHARED_MEM_SIZE) len = SHARED_MEM_SIZE;
    memcpy(m_pSharedView, newPath.c_str(), len);
    m_pSharedView[SHARED_MEM_SIZE - 1] = '\0';

    // 触发更新事件
    if (!SetEvent(m_hEventUpdatePath)) {
        DEBUG_LOG("FolderViewerManager: SetEvent(UpdatePath) failed, error=" << GetLastError() << "\n");
        return false;
    }
    return true;
}

bool FolderViewerManager::PollFolderChange() {
    if (!m_hEventFolderChanged) return false;

    DWORD result = WaitForSingleObject(m_hEventFolderChanged, 0);
    if (result == WAIT_OBJECT_0) {
        // 事件已被触发（自动重置）
        DEBUG_LOG("FolderViewerManager: FolderChanged event signaled\n");
        if (m_onFolderChanged) {
            m_onFolderChanged();
        }
        return true;
    }
    return false;
}

void FolderViewerManager::Shutdown(DWORD gracefulTimeoutMs) {
    if (!IsRunning()) {
        CleanupIPC();
        return;
    }

    // 发送退出事件
    if (m_hEventExit) {
        SetEvent(m_hEventExit);
    }

    // 等待进程自行退出
    if (m_hProcess) {
        DWORD waitResult = WaitForSingleObject(m_hProcess, gracefulTimeoutMs);
        if (waitResult == WAIT_TIMEOUT) {
            DEBUG_LOG("FolderViewerManager: Force terminating FolderViewer.exe\n");
            TerminateProcess(m_hProcess, 0);
        }
    }

    CleanupProcess();
    CleanupIPC();
    DEBUG_LOG("FolderViewerManager: Shutdown complete\n");
}

bool FolderViewerManager::IsRunning() const {
    if (!m_hProcess) return false;
    DWORD exitCode;
    if (GetExitCodeProcess(m_hProcess, &exitCode)) {
        return (exitCode == STILL_ACTIVE);
    }
    return false;
}