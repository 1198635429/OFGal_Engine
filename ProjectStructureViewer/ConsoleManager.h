// Copyright 2026 MrSeagull. All Rights Reserved.
#pragma once
#define NOMINMAX
#include <windows.h>
#include "SharedTypes.h"

class ConsoleManager {
public:
    ConsoleManager();
    ~ConsoleManager() = default;

    // 获取光标行号
    int GetCursorRow() const;

    // 初始化控制台窗口（位置、大小、VT模式、禁用快速编辑）
    void SetupWindow(int screenWidth, int workAreaHeight);

    // 检测是否支持虚拟终端（ANSI转义）
    bool IsVTSupported() const { return m_vtSupported; }

    // 清屏（兼容VT与回退方案）
    void ClearScreen();

    // 设置光标位置（行、列）
    void SetCursorPosition(int row, int col);

    // 获取当前窗口尺寸（行数、列数）
    void GetWindowSize(int& rows, int& cols) const;

    // 调整屏幕缓冲区大小以适应内容，并可选重置窗口位置
    void AdjustBufferSize(const TreeDisplayInfo& info);

    // 滚动控制台窗口使指定行位于可视区域中央附近
    void ScrollToLine(int lineNumber, int totalLines);

    // 输出带颜色控制的字符串（内部处理VT开关）
    void WriteColored(const std::string& text, WORD attributesIfNoVT = 0);

    // 刷新输出缓冲区
    void Flush();

private:
    HANDLE m_hConsoleOut;
    bool   m_vtSupported;
};