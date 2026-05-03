// Copyright 2026 Nagato-Yuki-708. All Rights Reserved.
#pragma once
#define NOMINMAX
#include "SharedTypes.h"
#include "Json_BPData_ReadWrite.h"
#include "InputSystem.h"
#include "InputCollector.h"
#include "Debug.h"
#include <iostream>
#include <windows.h>
#include <string>
#include <vector>

static const char* RESET = "\x1b[0m";
static const char* CYAN = "\x1b[36m";
static const char* YELLOW = "\x1b[33m";
static const char* WHITE = "\x1b[37m";

class BlueprintViewer {
public:
    BlueprintViewer();
    ~BlueprintViewer();

    void Run();

private:
    std::wstring currentBPPath;
    BlueprintData currentBPData;

    int currentEntryNodeId;
    int selectedNodeId1;
    int selectedNodeId2;

    // ---- 同步对象 ----
    HANDLE hLoadBPEvent;
    HANDLE hFileMapping;
    LPVOID pSharedMem;
    HANDLE hNodeViewerEvent;          // 用于通知子进程加载
    HANDLE hVariablesViewerEvent;     // 用于通知子进程加载
    HANDLE hNodeChangedEvent;         // 子进程节点数据变更通知
    HANDLE hVarChangedEvent;          // 子进程变量数据变更通知

    // ---- 输入系统 ----
    InputSystem     m_inputSystem;
    InputCollector  m_inputCollector = InputCollector(&m_inputSystem);
    bool isEditing = false;

    // ---- 子进程路径 ----
    std::wstring exePath_NodeViewer = L"E:\\Projects\\C++Projects\\OFGal_Engine\\x64\\Debug\\NodeViewer.exe";
    std::wstring exePath_VariablesViewer = L"E:\\Projects\\C++Projects\\OFGal_Engine\\x64\\Debug\\VariablesViewer.exe";

    // ---- 子进程句柄 ----
    std::vector<HANDLE> childProcesses;

private:
    void ConfigureConsole();
    void ClearScreen();
    void SetWindowSizeAndPosition();
    void FlushInputBuffer();
    void BuildAndPrintHelpText();
    void BuildAndPrintCurrentFlow();

    std::string WideToUTF8(const std::wstring& wstr) const;
    int GetConsoleColumns();

    // 启动子进程（新控制台窗口）
    bool LaunchChildProcess(const std::wstring& exePath);
};