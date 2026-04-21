// Copyright 2026 MrSeagull. All Rights Reserved.
#include "ConsoleManager.h"
#include "Debug.h"
#include <iostream>
#include <algorithm>

std::string GetExecutableDirectoryA() {
    char buffer[MAX_PATH];
    // 1. 获取完整路径
    DWORD length = GetModuleFileNameA(NULL, buffer, MAX_PATH);
    if (length == 0) return "";
    // 2. 转换为string，手动截断
    std::string fullPath(buffer, length);
    size_t lastSlash = fullPath.find_last_of("\\/");
    if (lastSlash != std::string::npos) {
        return fullPath.substr(0, lastSlash);
    }
    return fullPath;
}

ConsoleManager::ConsoleManager()
    : m_hConsoleOut(GetStdHandle(STD_OUTPUT_HANDLE))
    , m_vtSupported(false)
{
    if (m_hConsoleOut == INVALID_HANDLE_VALUE) {
        DEBUG_LOG("ConsoleManager: Failed to get stdout handle\n");
        return;
    }
    // 检测VT支持
    DWORD dwMode = 0;
    if (GetConsoleMode(m_hConsoleOut, &dwMode)) {
        m_vtSupported = (dwMode & ENABLE_VIRTUAL_TERMINAL_PROCESSING) != 0;
    }
}

void ConsoleManager::SetupWindow() {
    HWND hwndConsole = GetConsoleWindow();
    if (hwndConsole) {
        HWND MAX_Window = FindWindow(NULL, L"OFGal_Engine");
        float scaleX = 1920.0f / 2560.0f;
        float scaleY = 1080.0f / 1600.0f;
        if (MAX_Window) {
            RECT rect;
            if (GetWindowRect(MAX_Window, &rect)) {
                int width = rect.right - rect.left;
                int height = rect.bottom - rect.top;
                scaleX = (float)width / 2560.0f;
                scaleY = (float)height / 1600.0f;
            }
        }
        // 先设置缓冲区，避免后续 SetWindowPos 后被覆盖
        if (m_hConsoleOut != INVALID_HANDLE_VALUE) {
            COORD bufferSize = { 120, 1000 };
            SetConsoleScreenBufferSize(m_hConsoleOut, bufferSize);
        }
        // 再设置窗口位置与尺寸
        SetWindowPos(hwndConsole, nullptr, 0, 1010.0f * scaleY, 350.0f * scaleX, 470.0f * scaleY, SWP_NOZORDER | SWP_NOACTIVATE);
    }

    if (m_hConsoleOut == INVALID_HANDLE_VALUE) return;

    DWORD dwMode = 0;
    if (GetConsoleMode(m_hConsoleOut, &dwMode)) {
        dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
        dwMode &= ~ENABLE_QUICK_EDIT_MODE;
        dwMode &= ~ENABLE_INSERT_MODE;
        dwMode |= ENABLE_MOUSE_INPUT;
        SetConsoleMode(m_hConsoleOut, dwMode);
        m_vtSupported = true;
    }
}

void ConsoleManager::ClearScreen() {
    if (m_vtSupported) {
        WriteColored("\033[0m\033[2J\033[H");
    }
    else {
        CONSOLE_SCREEN_BUFFER_INFO csbi;
        if (GetConsoleScreenBufferInfo(m_hConsoleOut, &csbi)) {
            DWORD size = csbi.dwSize.X * csbi.dwSize.Y;
            COORD topLeft = { 0, 0 };
            DWORD written;
            FillConsoleOutputCharacter(m_hConsoleOut, ' ', size, topLeft, &written);
            FillConsoleOutputAttribute(m_hConsoleOut, csbi.wAttributes, size, topLeft, &written);
            SetConsoleCursorPosition(m_hConsoleOut, topLeft);
        }
    }
}

void ConsoleManager::SetCursorPosition(int row, int col) {
    COORD pos = { static_cast<SHORT>(col), static_cast<SHORT>(row) };
    SetConsoleCursorPosition(m_hConsoleOut, pos);
}

void ConsoleManager::GetWindowSize(int& rows, int& cols) const {
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    rows = cols = 0;
    if (GetConsoleScreenBufferInfo(m_hConsoleOut, &csbi)) {
        cols = csbi.srWindow.Right - csbi.srWindow.Left + 1;
        rows = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
    }
}

void ConsoleManager::AdjustBufferSize(const TreeDisplayInfo& info) {
    if (m_hConsoleOut == INVALID_HANDLE_VALUE) return;

    CONSOLE_SCREEN_BUFFER_INFO csbi;
    if (!GetConsoleScreenBufferInfo(m_hConsoleOut, &csbi)) return;

    SHORT windowWidth = csbi.srWindow.Right - csbi.srWindow.Left + 1;
    SHORT windowHeight = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;

    SHORT desiredWidth = static_cast<SHORT>(info.width + 1);
    SHORT desiredHeight = static_cast<SHORT>(info.height + 1);

    SHORT newWidth = std::max(desiredWidth, windowWidth);
    SHORT newHeight = std::max(desiredHeight, windowHeight);

    if (csbi.dwSize.X != newWidth || csbi.dwSize.Y != newHeight) {
        COORD newSize = { newWidth, newHeight };
        SetConsoleScreenBufferSize(m_hConsoleOut, newSize);
    }

    SMALL_RECT windowRect = { 0, 0, newWidth - 1, newHeight - 1 };
    SetConsoleWindowInfo(m_hConsoleOut, TRUE, &windowRect);
    SetCursorPosition(0, 0);
}

void ConsoleManager::ScrollToLine(int lineNumber, int totalLines) {
    if (m_hConsoleOut == INVALID_HANDLE_VALUE) return;

    CONSOLE_SCREEN_BUFFER_INFO csbi;
    if (!GetConsoleScreenBufferInfo(m_hConsoleOut, &csbi)) return;

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

    if (!SetConsoleWindowInfo(m_hConsoleOut, TRUE, &newWindow)) {
        // 有时需要先重置缓冲区大小才能移动窗口
        COORD newSize = csbi.dwSize;
        SetConsoleScreenBufferSize(m_hConsoleOut, newSize);
        SetConsoleWindowInfo(m_hConsoleOut, TRUE, &newWindow);
    }

    SetCursorPosition(lineNumber, 0);
}

void ConsoleManager::WriteColored(const std::string& text, WORD attributesIfNoVT) {
    if (m_vtSupported) {
        std::cout << text;
    }
    else {
        HANDLE hOut = m_hConsoleOut;
        if (attributesIfNoVT != 0) {
            CONSOLE_SCREEN_BUFFER_INFO csbi;
            if (GetConsoleScreenBufferInfo(hOut, &csbi)) {
                SetConsoleTextAttribute(hOut, attributesIfNoVT);
            }
        }
        std::cout << text;
        if (attributesIfNoVT != 0) {
            // 恢复原属性（简化起见，这里不实现保存恢复）
        }
    }
}

void ConsoleManager::Flush() {
    std::cout.flush();
}

int ConsoleManager::GetCursorRow() const {
    if (m_hConsoleOut == INVALID_HANDLE_VALUE) return -1;
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    if (GetConsoleScreenBufferInfo(m_hConsoleOut, &csbi)) {
        return csbi.dwCursorPosition.Y;
    }
    return -1;
}