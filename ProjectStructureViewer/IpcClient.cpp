// Copyright 2026 MrSeagull. All Rights Reserved.
#include "IpcClient.h"
#include "Debug.h"
#include <cstring>

IpcClient::IpcClient()
    : m_hExitEvent(nullptr)
    , m_hUpdateEvent(nullptr)
    , m_hMapFile(nullptr)
    , m_pSharedBuf(nullptr)
{
}

IpcClient::~IpcClient() {
    if (m_pSharedBuf) UnmapViewOfFile(m_pSharedBuf);
    if (m_hMapFile)   CloseHandle(m_hMapFile);
    if (m_hExitEvent) CloseHandle(m_hExitEvent);
    if (m_hUpdateEvent) CloseHandle(m_hUpdateEvent);
}

bool IpcClient::Initialize() {
    m_hExitEvent = OpenEventA(EVENT_MODIFY_STATE | SYNCHRONIZE, FALSE,
        "Global\\OFGal_Engine_ProjectStructureViewer_Exit");
    m_hUpdateEvent = OpenEventA(EVENT_MODIFY_STATE | SYNCHRONIZE, FALSE,
        "Global\\OFGal_Engine_ProjectStructureViewer_Refresh");
    if (!m_hExitEvent || !m_hUpdateEvent) {
        DEBUG_LOG("IpcClient: Failed to open events, error=" << GetLastError() << "\n");
        return false;
    }

    m_hMapFile = OpenFileMappingA(FILE_MAP_READ, FALSE,
        "Global\\OFGal_Engine_ProjectStructureViewer_Path");
    if (!m_hMapFile) {
        DEBUG_LOG("IpcClient: Failed to open file mapping, error=" << GetLastError() << "\n");
        return false;
    }

    m_pSharedBuf = static_cast<char*>(MapViewOfFile(m_hMapFile, FILE_MAP_READ, 0, 0, MAX_PATH));
    if (!m_pSharedBuf) {
        DEBUG_LOG("IpcClient: Failed to map view, error=" << GetLastError() << "\n");
        return false;
    }

    DEBUG_LOG("IpcClient: Initialized successfully\n");
    return true;
}

std::string IpcClient::GetCurrentPath() const {
    if (!m_pSharedBuf) return "";
    char localPath[MAX_PATH] = { 0 };
    memcpy(localPath, m_pSharedBuf, MAX_PATH - 1);
    localPath[MAX_PATH - 1] = '\0';
    return std::string(localPath);
}

bool IpcClient::WaitAndDispatch(DWORD timeoutMs) {
    std::vector<HANDLE> events = { m_hExitEvent, m_hUpdateEvent };
    DWORD result = WaitForMultipleObjects(static_cast<DWORD>(events.size()),
        events.data(), FALSE, timeoutMs);

    if (result == WAIT_OBJECT_0) {
        DEBUG_LOG("IpcClient: Exit event triggered\n");
        if (m_onExit) m_onExit();
        return false;
    }
    else if (result == WAIT_OBJECT_0 + 1) {
        DEBUG_LOG("IpcClient: Update event triggered\n");
        if (m_onUpdate) {
            std::string path = GetCurrentPath();
            m_onUpdate(path);
        }
        return true;
    }
    else if (result == WAIT_TIMEOUT) {
        // 正常超时，继续循环
        return true;
    }
    else {
        DEBUG_LOG("IpcClient: Wait failed, error=" << GetLastError() << "\n");
        return false;
    }
}