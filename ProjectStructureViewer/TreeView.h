// Copyright 2026 MrSeagull. All Rights Reserved.
#pragma once
#include "SharedTypes.h"
#include "ConsoleManager.h"
#include <memory>
#include <vector>
#include <string>

class TreeView {
public:
    explicit TreeView(ConsoleManager& console);
    ~TreeView() = default;

    void JumpToLine(int line);

    // 设置要显示的项目结构（传入所有权，或拷贝，这里采用指针）
    void SetProject(const ProjectStructure* pProject);

    // 渲染树，高亮指定行（-1 表示不高亮）
    void Render(int highlightLine = -1);

    // 获取当前选中的信息
    const SelectedFolderInfo& GetSelectedInfo() const { return m_selectedInfo; }

    // 设置高亮行（不立即渲染）
    void SetHighlightLine(int line) { m_highlightLine = line; }
    int  GetHighlightLine() const { return m_highlightLine; }

    // 行数相关
    int  GetTotalLines() const { return static_cast<int>(m_lineToPath.size()); }

    // 根据行号获取文件夹绝对路径
    std::string GetPathByLine(int line) const;

    int FindLineByPath(const std::string& path) const;

    // 移动高亮行（内部调用Render）
    void MoveHighlightUp();
    void MoveHighlightDown();

private:
    // 递归构建输出字符串，同时填充映射
    void BuildTreeString(const FolderStructure& folder,
        const std::string& plainPrefix,
        bool isLast,
        const std::string& currentPath,
        int& currentLine,
        std::ostringstream& out);

    ConsoleManager& m_console;
    const ProjectStructure* m_pProject;
    TreeDisplayInfo            m_displayInfo;
    std::vector<std::string>   m_lineToPath;
    SelectedFolderInfo         m_selectedInfo;
    int                        m_highlightLine;
};