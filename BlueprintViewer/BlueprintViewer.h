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
#include <algorithm> // for std::find
#include <utility>  // for std::pair
#include <conio.h>

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
    int selectedNodeId1 = -1;
    int selectedNodeId2 = -1;

    // ---- 当前执行流的节点顺序（先序） ----
    std::vector<int> m_flowNodeOrder;

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
    void ScrollToTheTop();

    std::string WideToUTF8(const std::wstring& wstr) const;
    int GetConsoleColumns();

    int GetMaxNodeId() const;

    // 启动子进程（新控制台窗口）
    bool LaunchChildProcess(const std::wstring& exePath);

    // 工具：计算字符串中可见字符的长度（忽略ANSI转义序列）
    static size_t VisibleLength(const std::string& s);

private:
    static constexpr int maxNodeNameLength = 21;
    static constexpr int boxWidth = maxNodeNameLength + 4; // 25

    // 执行流树节点
    struct ExecTreeNode {
        int nodeId;
        std::string nodeType;
        std::vector<std::pair<std::string, std::unique_ptr<ExecTreeNode>>> branches; // 分支标签 -> 子树
    };

    // 着色指令：在指定行中，从 startCol 到 endCol 应用某种颜色
    struct ColorSpan {
        int startCol;      // 可见列起始（包含）
        int endCol;        // 可见列结束（不包含）
        const char* color; // ANSI 颜色码，如 CYAN
    };

    struct RenderBlock {
        std::vector<std::string> lines;            // 纯文本行（无 ANSI）
        int centerCol;
        // 按行索引存储该行中需要着色的区间
        std::vector<std::vector<ColorSpan>> spans; // spans[row] 即该行的着色列表
    };
    static void AddColorSpan(RenderBlock& block, int row, int startVisCol, int endVisCol, const char* color);
    static void MergeChildSpans(RenderBlock& parent, const RenderBlock& child,
        int rowOffset, int colOffset);
    static const char* GetTopBorderColor(int nodeId, int sel1, int sel2);
    static const char* GetBottomBorderColor(int nodeId, int sel1, int sel2);

    // 辅助函数
    std::vector<int> GetEntryNodeIds() const;
    std::vector<std::pair<int, std::string>> GetEntryNodes() const;
    std::unique_ptr<ExecTreeNode> BuildExecTree(int startNodeId) const;
    void CollectNodeOrder(const ExecTreeNode* node, std::vector<int>& order) const;
    RenderBlock RenderExecTree(const ExecTreeNode* node,
        std::unordered_set<int>& visited,
        int depth,
        int selectedId1,
        int selectedId2) const;
    void PrintRenderBlock(const RenderBlock& block);

    void MoveSelection1Up();
    void MoveSelection1Down();
    void MoveSelection2Up();
    void MoveSelection2Down();
    void MoveToNextFlow();
    void MoveToPrevFlow();
    bool OnDelete();
    bool Edit();
    void BuildAndPrintCurrentFlow();

    void PrintEntryNodes();
    void RenderAll();
};