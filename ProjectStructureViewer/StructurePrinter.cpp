// Copyright 2026 MrSeagull. All Rights Reserved.
#include "StructurePrinter.h"
#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <sstream>

#define NOMINMAX
#include <windows.h>
#include <algorithm>

// 使用标准 4-bit 颜色，兼容性更好
#define ANSI_RESET       "\033[0m"
#define ANSI_BLUE        "\033[36m"
#define ANSI_REVERSE     "\033[7m"

static TreeDisplayInfo g_displayInfo = { 0, 0 };
static std::vector<std::string> g_lineToPathMap;
static SelectedFolderInfo g_selectedInfo = { "", 0 };

// 检测虚拟终端是否真正启用
static bool IsVTSupported() {
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut == INVALID_HANDLE_VALUE) return false;
    DWORD dwMode = 0;
    if (!GetConsoleMode(hOut, &dwMode)) return false;
    return (dwMode & ENABLE_VIRTUAL_TERMINAL_PROCESSING) != 0;
}
static bool g_vtSupported = IsVTSupported();
void InitConsoleVTMode() {
    g_vtSupported = IsVTSupported();
}

// 返回带颜色的树枝符号（若不支持VT则返回纯文本）
std::string GetBranchString(const std::string& branchSymbol) {
    if (!g_vtSupported) return branchSymbol;
    return std::string(ANSI_BLUE) + branchSymbol + ANSI_RESET;
}

// 将纯文本前缀转换为带颜色的字符串（若不支持VT则返回纯文本）
std::string GetColoredPrefixString(const std::string& plainPrefix) {
    if (!g_vtSupported) return plainPrefix;
    std::string result;
    for (size_t pos = 0; pos < plainPrefix.length(); ) {
        if (plainPrefix[pos] == '|' && pos + 1 < plainPrefix.length() && plainPrefix[pos + 1] == ' ') {
            result += ANSI_BLUE "|  " ANSI_RESET;
            pos += 2;
        }
        else {
            result += plainPrefix[pos];
            ++pos;
        }
    }
    return result;
}

// 将控制台滚动到指定行附近
void ScrollConsoleToLine(int lineNumber, int totalLines) {
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hConsole == INVALID_HANDLE_VALUE)
        return;

    CONSOLE_SCREEN_BUFFER_INFO csbi;
    if (!GetConsoleScreenBufferInfo(hConsole, &csbi))
        return;

    SHORT windowHeight = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
    if (windowHeight <= 0) return;

    int desiredTopRow = lineNumber - (windowHeight / 2);
    int maxTopRow = totalLines - windowHeight;
    if (maxTopRow < 0) maxTopRow = 0;
    int newTopRow = std::max(0, std::min(desiredTopRow, maxTopRow));

    SMALL_RECT newWindow;
    newWindow.Left = 0;
    newWindow.Top = static_cast<SHORT>(newTopRow);
    newWindow.Right = csbi.dwSize.X - 1;
    newWindow.Bottom = static_cast<SHORT>(newTopRow + windowHeight - 1);

    if (!SetConsoleWindowInfo(hConsole, TRUE, &newWindow)) {
        COORD newSize = csbi.dwSize;
        SetConsoleScreenBufferSize(hConsole, newSize);
        SetConsoleWindowInfo(hConsole, TRUE, &newWindow);
    }

    COORD cursorPos = { 0, static_cast<SHORT>(lineNumber) };
    SetConsoleCursorPosition(hConsole, cursorPos);
}

// 递归辅助函数，将输出累积到 out 流中
void PrintFolderTree(const FolderStructure& folder,
    const std::string& plainPrefix,
    bool isLast,
    const std::string& currentPath,
    int highlightLine,
    int& currentLine,
    std::ostringstream& out)
{
    size_t count = folder.SubFolders.size();
    size_t i = 0;
    for (const auto& [name, subFolderPtr] : folder.SubFolders) {
        ++i;
        bool lastChild = (i == count);
        bool highlightThis = (currentLine == highlightLine);

        if (highlightThis && g_vtSupported) out << ANSI_REVERSE;
        out << GetColoredPrefixString(plainPrefix);
        if (lastChild)
            out << GetBranchString("`--");
        else
            out << GetBranchString("|--");
        out << name;
        if (highlightThis && g_vtSupported) out << ANSI_RESET;
        out << "\n";

        int lineLength = static_cast<int>(plainPrefix.length()) + 3 + static_cast<int>(name.length());
        g_displayInfo.width = std::max(g_displayInfo.width, lineLength);
        g_displayInfo.height++;

        std::string subPath = currentPath;
        if (!subPath.empty() && subPath.back() != '\\') subPath += '\\';
        subPath += name;
        g_lineToPathMap.push_back(subPath);
        currentLine++;

        std::string newPlainPrefix = plainPrefix;
        if (lastChild) newPlainPrefix += "   ";
        else           newPlainPrefix += "|  ";

        PrintFolderTree(*subFolderPtr, newPlainPrefix, lastChild, subPath, highlightLine, currentLine, out);
    }
}

void PrintProjectStructureTree(const ProjectStructure& project, int highlightLine) {
    g_displayInfo.width = 0;
    g_displayInfo.height = 0;
    g_lineToPathMap.clear();
    std::ostringstream buffer;

    // 提取根目录名
    std::string rootName = project.RootDirectory;
    while (!rootName.empty() && (rootName.back() == '\\' || rootName.back() == '/')) rootName.pop_back();
    size_t lastSlash = rootName.find_last_of("\\/");
    if (lastSlash != std::string::npos) rootName = rootName.substr(lastSlash + 1);
    if (rootName.empty()) rootName = "ProjectRoot";

    int currentLine = 0;
    bool highlightRoot = (currentLine == highlightLine);
    if (highlightRoot && g_vtSupported) buffer << ANSI_REVERSE;
    buffer << rootName;
    if (highlightRoot && g_vtSupported) buffer << ANSI_RESET;
    buffer << "\n";

    g_displayInfo.height = 1;
    g_displayInfo.width = std::max(g_displayInfo.width, static_cast<int>(rootName.length()));

    std::string rootPath = project.RootDirectory;
    if (!rootPath.empty() && rootPath.back() != '\\') rootPath += '\\';
    g_lineToPathMap.push_back(rootPath);
    currentLine++;

    const FolderStructure& rootFolder = project.Self;
    size_t count = rootFolder.SubFolders.size();
    size_t i = 0;
    for (const auto& [name, subFolderPtr] : rootFolder.SubFolders) {
        ++i;
        bool lastChild = (i == count);
        bool highlightThis = (currentLine == highlightLine);

        if (highlightThis && g_vtSupported) buffer << ANSI_REVERSE;
        buffer << GetColoredPrefixString("");
        if (lastChild) buffer << GetBranchString("`--");
        else           buffer << GetBranchString("|--");
        buffer << name;
        if (highlightThis && g_vtSupported) buffer << ANSI_RESET;
        buffer << "\n";

        int lineLength = 3 + static_cast<int>(name.length());
        g_displayInfo.width = std::max(g_displayInfo.width, lineLength);
        g_displayInfo.height++;

        std::string subPath = rootPath + name;
        g_lineToPathMap.push_back(subPath);
        currentLine++;

        std::string plainPrefix = lastChild ? "   " : "|  ";
        PrintFolderTree(*subFolderPtr, plainPrefix, lastChild, subPath, highlightLine, currentLine, buffer);
    }

    // 确保结尾重置颜色（仅当VT支持时）
    if (g_vtSupported) buffer << ANSI_RESET;

    AdjustConsoleBufferSizeToContent(g_displayInfo);
    std::cout << buffer.str() << std::flush;

    SetSelectedLine((highlightLine >= 0) ? highlightLine : 0);
    int totalLines = static_cast<int>(g_lineToPathMap.size());
    int lineToScroll = (highlightLine >= 0) ? highlightLine : 0;
    ScrollConsoleToLine(lineToScroll, totalLines);
}

// 根据树状显示信息调整控制台缓冲区大小
void AdjustConsoleBufferSizeToContent(const TreeDisplayInfo& info) {
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hConsole == INVALID_HANDLE_VALUE)
        return;

    CONSOLE_SCREEN_BUFFER_INFO csbi;
    if (!GetConsoleScreenBufferInfo(hConsole, &csbi))
        return;

    SHORT windowWidth = csbi.srWindow.Right - csbi.srWindow.Left + 1;
    SHORT windowHeight = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;

    SHORT desiredWidth = static_cast<SHORT>(info.width + 1);
    SHORT desiredHeight = static_cast<SHORT>(info.height + 1);

    SHORT newWidth = std::max(desiredWidth, windowWidth);
    SHORT newHeight = std::max(desiredHeight, windowHeight);

    if (!(csbi.dwSize.X == newWidth && csbi.dwSize.Y == newHeight)) {
        COORD newSize = { newWidth, newHeight };
        SetConsoleScreenBufferSize(hConsole, newSize);
    }

    SMALL_RECT windowRect;
    windowRect.Left = 0;
    windowRect.Top = 0;
    windowRect.Right = newWidth - 1;
    windowRect.Bottom = newHeight - 1;

    if (!SetConsoleWindowInfo(hConsole, TRUE, &windowRect)) {
        SMALL_RECT moveRect;
        moveRect.Left = 0;
        moveRect.Top = 0;
        moveRect.Right = csbi.srWindow.Right - csbi.srWindow.Left;
        moveRect.Bottom = csbi.srWindow.Bottom - csbi.srWindow.Top;
        SetConsoleWindowInfo(hConsole, TRUE, &moveRect);
    }

    COORD cursorPos = { 0, 0 };
    SetConsoleCursorPosition(hConsole, cursorPos);
}

const SelectedFolderInfo& GetSelectedFolderInfo() {
    return g_selectedInfo;
}

void SetSelectedLine(int lineNumber) {
    if (lineNumber >= 0 && lineNumber < static_cast<int>(g_lineToPathMap.size())) {
        g_selectedInfo.lineNumber = lineNumber;
        g_selectedInfo.absolutePath = g_lineToPathMap[lineNumber];
    }
    else {
        g_selectedInfo.lineNumber = 0;
        if (!g_lineToPathMap.empty())
            g_selectedInfo.absolutePath = g_lineToPathMap[0];
    }
}

std::string GetFolderPathByLine(int lineNumber) {
    if (lineNumber >= 0 && lineNumber < static_cast<int>(g_lineToPathMap.size()))
        return g_lineToPathMap[lineNumber];
    return "";
}

int GetTotalLines() {
    return static_cast<int>(g_lineToPathMap.size());
}