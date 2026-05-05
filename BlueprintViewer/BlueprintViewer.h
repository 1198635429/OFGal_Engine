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
#include <sstream>
#include <unordered_set>

static const char* RESET = "\x1b[0m";
static const char* CYAN = "\x1b[36m";
static const char* YELLOW = "\x1b[33m";
static const char* WHITE = "\x1b[37m";
static const char* ORANGE = "\x1b[38;5;208m";

class BlueprintViewer {
public:
    BlueprintViewer();
    ~BlueprintViewer();

    void Run();

private:
    std::wstring currentBPPath;
    BlueprintData currentBPData;

    int currentEntryNodeId;
    int currentEntryIndex = 0;
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
    void AdjustBufferSize();

    std::string WideToUTF8(const std::wstring& wstr) const;
    int GetConsoleColumns();

    int GetMaxNodeId() const;

    // 启动子进程（新控制台窗口）
    bool LaunchChildProcess(const std::wstring& exePath);

private:
    static constexpr int maxNodeNameLength = 21;
    static constexpr int boxWidth = maxNodeNameLength + 4; // 25

    // 执行流树节点
    struct ExecTreeNode {
        int nodeId;
        std::string nodeType;
        std::vector<std::pair<std::string, std::unique_ptr<ExecTreeNode>>> branches; // 分支标签 -> 子树
    };

    // 渲染结果
    struct RenderBlock {
        std::vector<std::string> lines;
        int centerCol; // 框中心相对于 lines 的列索引
    };

    // 辅助函数
    std::vector<int> GetEntryNodeIds() const;
    std::unique_ptr<ExecTreeNode> BuildExecTree(int startNodeId) const;
    RenderBlock RenderExecTree(const ExecTreeNode* node, std::unordered_set<int>& visited, int depth) const;
    void PrintRenderBlock(const RenderBlock& block);

    void MoveSelection1Up();
    void MoveSelection1Down();
    void MoveSelection2Up();
    void MoveSelection2Down();
    void MoveToNextFlow();
    void MoveToPrevFlow();
    void OnDelete();
    void Edit();
    void BuildAndPrintCurrentFlow();
};