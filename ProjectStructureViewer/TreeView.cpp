// Copyright 2026 MrSeagull. All Rights Reserved.
#include "TreeView.h"
#include "Debug.h"
#include <sstream>
#include <algorithm>

// ANSI 颜色代码（仅当VT启用时有效）
#define ANSI_RESET   "\033[0m"
#define ANSI_BLUE    "\033[36m"
#define ANSI_REVERSE "\033[7m"

TreeView::TreeView(ConsoleManager& console)
    : m_console(console)
    , m_pProject(nullptr)
    , m_highlightLine(0)
{
    m_displayInfo = { 0, 0 };
}

void TreeView::SetProject(const ProjectStructure* pProject) {
    m_pProject = pProject;
    m_highlightLine = 0;  // 重置高亮到根目录
}

void TreeView::Render(int highlightLine) {
    if (!m_pProject) {
        DEBUG_LOG("TreeView::Render called with null project\n");
        return;
    }

    m_displayInfo = { 0, 0 };
    m_lineToPath.clear();
    std::ostringstream buffer;

    // 清屏并重置属性
    m_console.ClearScreen();

    const bool vt = m_console.IsVTSupported();

    // 辅助 lambda：根据VT支持返回带颜色的字符串
    auto colorBranch = [vt](const std::string& s) -> std::string {
        if (!vt) return s;
        return ANSI_BLUE + s + ANSI_RESET;
        };
    auto colorPrefix = [vt](const std::string& plain) -> std::string {
        if (!vt) return plain;
        std::string result;
        for (size_t i = 0; i < plain.length(); ) {
            if (plain[i] == '|' && i + 1 < plain.length() && plain[i + 1] == ' ') {
                result += ANSI_BLUE "|  " ANSI_RESET;
                i += 2;
            }
            else {
                result += plain[i++];
            }
        }
        return result;
        };

    // 提取根目录名
    std::string rootName = m_pProject->RootDirectory;
    while (!rootName.empty() && (rootName.back() == '\\' || rootName.back() == '/'))
        rootName.pop_back();
    size_t lastSlash = rootName.find_last_of("\\/");
    if (lastSlash != std::string::npos)
        rootName = rootName.substr(lastSlash + 1);
    if (rootName.empty()) rootName = "ProjectRoot";

    // 根行处理
    int currentLine = 0;
    bool highlightRoot = (currentLine == highlightLine);
    if (highlightRoot && vt) buffer << ANSI_REVERSE;
    buffer << rootName;
    if (highlightRoot && vt) buffer << ANSI_RESET;
    buffer << "\n";

    m_displayInfo.height = 1;
    m_displayInfo.width = std::max(m_displayInfo.width, static_cast<int>(rootName.length()));

    std::string rootPath = m_pProject->RootDirectory;
    if (!rootPath.empty() && rootPath.back() != '\\') rootPath += '\\';
    m_lineToPath.push_back(rootPath);
    currentLine++;

    const FolderStructure& rootFolder = m_pProject->Self;
    size_t count = rootFolder.SubFolders.size();
    size_t i = 0;
    for (const auto& [name, subFolderPtr] : rootFolder.SubFolders) {
        ++i;
        bool lastChild = (i == count);
        bool highlightThis = (currentLine == highlightLine);

        if (highlightThis && vt) buffer << ANSI_REVERSE;
        buffer << colorPrefix("");
        if (lastChild) buffer << colorBranch("`--");
        else           buffer << colorBranch("|--");
        buffer << name;
        if (highlightThis && vt) buffer << ANSI_RESET;
        buffer << "\n";

        int lineLen = 3 + static_cast<int>(name.length());
        m_displayInfo.width = std::max(m_displayInfo.width, lineLen);
        m_displayInfo.height++;

        std::string subPath = rootPath + name;
        m_lineToPath.push_back(subPath);
        currentLine++;

        std::string plainPrefix = lastChild ? "   " : "|  ";
        // 递归构建子树
        BuildTreeString(*subFolderPtr, plainPrefix, lastChild, subPath, currentLine, buffer);
        // 注意：BuildTreeString 内部会更新 currentLine、m_displayInfo 和 m_lineToPath
    }

    if (vt) buffer << ANSI_RESET;

    // 输出到控制台
    m_console.WriteColored(buffer.str());
    m_console.AdjustBufferSize(m_displayInfo);

    // 更新选中信息
    if (highlightLine >= 0 && highlightLine < static_cast<int>(m_lineToPath.size())) {
        m_selectedInfo.lineNumber = highlightLine;
        m_selectedInfo.absolutePath = m_lineToPath[highlightLine];
    }
    else {
        m_selectedInfo.lineNumber = 0;
        m_selectedInfo.absolutePath = m_lineToPath.empty() ? "" : m_lineToPath[0];
    }

    // 滚动到高亮行
    m_console.ScrollToLine(highlightLine, static_cast<int>(m_lineToPath.size()));
    m_console.Flush();
}

void TreeView::BuildTreeString(const FolderStructure& folder,
    const std::string& plainPrefix,
    bool isLast,
    const std::string& currentPath,
    int& currentLine,
    std::ostringstream& out)
{
    const bool vt = m_console.IsVTSupported();
    auto colorBranch = [vt](const std::string& s) -> std::string {
        if (!vt) return s;
        return ANSI_BLUE + s + ANSI_RESET;
        };
    auto colorPrefix = [vt](const std::string& plain) -> std::string {
        if (!vt) return plain;
        std::string result;
        for (size_t i = 0; i < plain.length(); ) {
            if (plain[i] == '|' && i + 1 < plain.length() && plain[i + 1] == ' ') {
                result += ANSI_BLUE "|  " ANSI_RESET;
                i += 2;
            }
            else {
                result += plain[i++];
            }
        }
        return result;
        };

    size_t count = folder.SubFolders.size();
    size_t i = 0;
    for (const auto& [name, subFolderPtr] : folder.SubFolders) {
        ++i;
        bool lastChild = (i == count);
        bool highlightThis = (currentLine == m_highlightLine);

        if (highlightThis && vt) out << ANSI_REVERSE;
        out << colorPrefix(plainPrefix);
        if (lastChild) out << colorBranch("`--");
        else           out << colorBranch("|--");
        out << name;
        if (highlightThis && vt) out << ANSI_RESET;
        out << "\n";

        int lineLen = static_cast<int>(plainPrefix.length()) + 3 + static_cast<int>(name.length());
        m_displayInfo.width = std::max(m_displayInfo.width, lineLen);
        m_displayInfo.height++;

        std::string subPath = currentPath;
        if (!subPath.empty() && subPath.back() != '\\') subPath += '\\';
        subPath += name;
        m_lineToPath.push_back(subPath);
        currentLine++;

        std::string newPlainPrefix = plainPrefix;
        if (lastChild) newPlainPrefix += "   ";
        else           newPlainPrefix += "|  ";

        BuildTreeString(*subFolderPtr, newPlainPrefix, lastChild, subPath, currentLine, out);
    }
}

std::string TreeView::GetPathByLine(int line) const {
    if (line >= 0 && line < static_cast<int>(m_lineToPath.size()))
        return m_lineToPath[line];
    return "";
}

void TreeView::MoveHighlightUp() {
    int newLine = std::max(0, m_highlightLine - 1);
    if (newLine != m_highlightLine) {
        m_highlightLine = newLine;
        Render(m_highlightLine);
    }
}

void TreeView::MoveHighlightDown() {
    int newLine = std::min(GetTotalLines() - 1, m_highlightLine + 1);
    if (newLine != m_highlightLine) {
        m_highlightLine = newLine;
        Render(m_highlightLine);
    }
}

void TreeView::JumpToLine(int line) {
    if (!m_pProject) return;
    int total = GetTotalLines();
    if (line < 0 || line >= total) return;
    m_highlightLine = line;
    Render(m_highlightLine);
}

int TreeView::FindLineByPath(const std::string& path) const {
    // 规范化传入的路径（去除尾部斜杠，统一格式）
    std::string normPath = path;
    while (!normPath.empty() && (normPath.back() == '\\' || normPath.back() == '/'))
        normPath.pop_back();

    for (size_t i = 0; i < m_lineToPath.size(); ++i) {
        std::string linePath = m_lineToPath[i];
        while (!linePath.empty() && (linePath.back() == '\\' || linePath.back() == '/'))
            linePath.pop_back();
        if (linePath == normPath)
            return static_cast<int>(i);
    }
    return -1;
}