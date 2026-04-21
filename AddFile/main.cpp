// Copyright 2026 Nagato-Yuki-708. All Rights Reserved.

#include <windows.h>
#include <iostream>
#include <string>
#include <filesystem>
#include <fstream>
#include <cstdlib>
#include <stdexcept>
#include <io.h>
#include <fcntl.h>
#include <vector>

#include "SharedTypes.h"
#include "Json_BPData_ReadWrite.h"
#include "Json_LevelData_ReadWrite.h"
#include "nlohmann/json.hpp"

namespace fs = std::filesystem;
using json = nlohmann::json;

// 事件名称
const std::wstring EVENT_ADD_END = L"Global\\OFGal_Engine_AddItem_AddEnd";
const std::wstring EVENT_CANCEL = L"Global\\OFGal_Engine_AddItem_Cancel";
const std::wstring EVENT_EXIT = L"Global\\OFGal_Engine_AddItem_Exit";

// ------------------------------------------------------------
// 安全读取一行宽字符（使用 ReadConsoleW，避免 std::wcin 乱码）
// ------------------------------------------------------------
std::wstring ReadLineW() {
    HANDLE hConsole = GetStdHandle(STD_INPUT_HANDLE);
    if (hConsole == INVALID_HANDLE_VALUE) {
        throw std::runtime_error("Failed to get console input handle");
    }

    DWORD oldMode;
    GetConsoleMode(hConsole, &oldMode);
    SetConsoleMode(hConsole, ENABLE_LINE_INPUT | ENABLE_ECHO_INPUT | ENABLE_PROCESSED_INPUT);

    std::wstring result;
    WCHAR buffer[1024];
    DWORD charsRead;
    while (true) {
        if (!ReadConsoleW(hConsole, buffer, 1024, &charsRead, nullptr)) {
            SetConsoleMode(hConsole, oldMode);
            throw std::runtime_error("ReadConsoleW failed");
        }
        // 检查是否包含回车换行
        bool hasNewline = false;
        for (DWORD i = 0; i < charsRead; ++i) {
            if (buffer[i] == L'\r' || buffer[i] == L'\n') {
                hasNewline = true;
                charsRead = i; // 截断换行符之前的内容
                break;
            }
        }
        result.append(buffer, charsRead);
        if (hasNewline) break;
    }
    SetConsoleMode(hConsole, oldMode);
    return result;
}

// ------------------------------------------------------------
// 去除首尾空白（宽字符）
// ------------------------------------------------------------
std::wstring TrimW(const std::wstring& str) {
    size_t first = str.find_first_not_of(L" \t\n\r\f\v");
    if (first == std::wstring::npos) return L"";
    size_t last = str.find_last_not_of(L" \t\n\r\f\v");
    return str.substr(first, last - first + 1);
}

// ------------------------------------------------------------
// 宽字符转 UTF-8
// ------------------------------------------------------------
std::string WideToUtf8(const std::wstring& wstr) {
    if (wstr.empty()) return {};
    int len = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, nullptr, 0, nullptr, nullptr);
    if (len <= 0) throw std::runtime_error("WideCharToMultiByte failed");
    std::string result(len - 1, 0);
    WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, &result[0], len, nullptr, nullptr);
    return result;
}

// ------------------------------------------------------------
// 内部 JSON 写入函数（直接使用宽字符路径，避免 ANSI 限制）
// ------------------------------------------------------------
bool WriteJsonToFile(const std::wstring& filepath, const json& j) {
    try {
        std::ofstream file(fs::path(filepath), std::ios::out | std::ios::binary);
        if (!file.is_open()) return false;
        file << j.dump(4);   // 美化输出
        return true;
    }
    catch (...) {
        return false;
    }
}

// ------------------------------------------------------------
// 创建空蓝图文件（.bp）
// ------------------------------------------------------------
bool CreateEmptyBlueprint(const std::wstring& filepath) {
    try {
        BlueprintData bp;
        bp.name = fs::path(filepath).stem().string();  // 文件名（UTF-8）
        bp.id = 1;

        json j = bp;  // 利用已定义的 to_json
        return WriteJsonToFile(filepath, j);
    }
    catch (const std::exception& e) {
        std::cerr << "[Blueprint] Exception: " << e.what() << std::endl;
        return false;
    }
}

// ------------------------------------------------------------
// 创建空场景文件（.level）
// ------------------------------------------------------------
bool CreateEmptyLevel(const std::wstring& filepath) {
    try {
        LevelData level;
        level.name = fs::path(filepath).stem().string();

        json j = level;  // 利用已定义的 to_json
        return WriteJsonToFile(filepath, j);
    }
    catch (const std::exception& e) {
        std::cerr << "[Level] Exception: " << e.what() << std::endl;
        return false;
    }
}

// ------------------------------------------------------------
// 创建空文本文件（UTF-8 无 BOM）
// ------------------------------------------------------------
bool CreateEmptyTextFile(const std::wstring& filepath) {
    try {
        std::ofstream file(fs::path(filepath), std::ios::out | std::ios::binary);
        return file.is_open();
    }
    catch (...) {
        return false;
    }
}

// ------------------------------------------------------------
// 主函数
// ------------------------------------------------------------
int wmain(int argc, wchar_t* argv[]) {
    try {
        SetConsoleTitleW(L"OFGal_Engine/Add_Item");

        // 启用控制台 Unicode 输出（用于显示中文）
        _setmode(_fileno(stdout), _O_U16TEXT);
        _setmode(_fileno(stderr), _O_U16TEXT);

        // 获取目标目录（命令行参数）
        if (argc < 2) {
            std::wcerr << L"Error: Missing target directory argument.\n";
            system("pause");
            return 1;
        }

        std::wstring targetDirW = argv[1];
        if (targetDirW.size() >= 2 && targetDirW.front() == L'"' && targetDirW.back() == L'"') {
            targetDirW = targetDirW.substr(1, targetDirW.size() - 2);
        }

        std::wcout << L"Target directory: " << targetDirW << std::endl;

        // 验证目录存在
        std::error_code ec;
        if (!fs::exists(targetDirW, ec) || !fs::is_directory(targetDirW, ec)) {
            std::wcerr << L"Error: Target directory does not exist or is not a directory.\n";
            system("pause");
            return 1;
        }

        // 打开 IPC 事件
        HANDLE hAddEnd = OpenEventW(EVENT_MODIFY_STATE, FALSE, EVENT_ADD_END.c_str());
        HANDLE hCancel = OpenEventW(EVENT_MODIFY_STATE, FALSE, EVENT_CANCEL.c_str());
        HANDLE hExit = OpenEventW(SYNCHRONIZE, FALSE, EVENT_EXIT.c_str());

        if (!hAddEnd || !hCancel || !hExit) {
            std::wcerr << L"Error: Failed to open IPC events. Code: " << GetLastError() << std::endl;
            if (hAddEnd) CloseHandle(hAddEnd);
            if (hCancel) CloseHandle(hCancel);
            if (hExit)   CloseHandle(hExit);
            system("pause");
            return 1;
        }

        // 用户交互
        std::wcout << L"Type #esc# to cancel the operation at any time.\n";
        std::wcout << L"Enter file type (bp / level / txt): ";
        std::wstring fileTypeW = ReadLineW();
        fileTypeW = TrimW(fileTypeW);

        if (fileTypeW == L"#esc#") {
            std::wcout << L"Operation cancelled.\n";
            SetEvent(hCancel);
            WaitForSingleObject(hExit, 1000);
            CloseHandle(hAddEnd);
            CloseHandle(hCancel);
            CloseHandle(hExit);
            return 0;
        }

        std::wstring extension;
        bool isJsonType = false;
        if (fileTypeW == L"bp") {
            extension = L".bp";
            isJsonType = true;
        }
        else if (fileTypeW == L"level") {
            extension = L".level";
            isJsonType = true;
        }
        else if (fileTypeW == L"txt") {
            extension = L".txt";
            isJsonType = false;
        }
        else {
            std::wcerr << L"Error: Invalid file type. Must be 'bp', 'level', or 'txt'.\n";
            SetEvent(hCancel);
            WaitForSingleObject(hExit, 1000);
            CloseHandle(hAddEnd);
            CloseHandle(hCancel);
            CloseHandle(hExit);
            system("pause");
            return 1;
        }

        std::wcout << L"Enter file name (without extension): ";
        std::wstring fileNameW = ReadLineW();
        fileNameW = TrimW(fileNameW);

        if (fileNameW == L"#esc#") {
            std::wcout << L"Operation cancelled.\n";
            SetEvent(hCancel);
            WaitForSingleObject(hExit, 1000);
            CloseHandle(hAddEnd);
            CloseHandle(hCancel);
            CloseHandle(hExit);
            return 0;
        }

        if (fileNameW.empty()) {
            std::wcerr << L"Error: File name cannot be empty.\n";
            SetEvent(hCancel);
            WaitForSingleObject(hExit, 1000);
            CloseHandle(hAddEnd);
            CloseHandle(hCancel);
            CloseHandle(hExit);
            system("pause");
            return 1;
        }

        // 构建完整路径
        fs::path fullPath = fs::path(targetDirW) / (fileNameW + extension);

        // 检查文件是否已存在
        if (fs::exists(fullPath, ec)) {
            std::wcerr << L"Error: File already exists: " << fullPath.wstring() << std::endl;
            SetEvent(hCancel);
            WaitForSingleObject(hExit, 1000);
            CloseHandle(hAddEnd);
            CloseHandle(hCancel);
            CloseHandle(hExit);
            system("pause");
            return 1;
        }

        // 创建文件
        bool success = false;
        if (isJsonType) {
            if (extension == L".bp")
                success = CreateEmptyBlueprint(fullPath.wstring());
            else
                success = CreateEmptyLevel(fullPath.wstring());
        }
        else {
            success = CreateEmptyTextFile(fullPath.wstring());
        }

        if (!success) {
            std::wcerr << L"Error: Failed to create file.\n";
            SetEvent(hCancel);
            WaitForSingleObject(hExit, 1000);
            CloseHandle(hAddEnd);
            CloseHandle(hCancel);
            CloseHandle(hExit);
            system("pause");
            return 1;
        }

        std::wcout << L"File created successfully: " << fullPath.wstring() << std::endl;

        // 通知主进程
        SetEvent(hAddEnd);

        // 等待 Exit 事件（最多 1 秒）
        WaitForSingleObject(hExit, 1000);

        CloseHandle(hAddEnd);
        CloseHandle(hCancel);
        CloseHandle(hExit);

        return 0;
    }
    catch (const std::exception& e) {
        std::cerr << "Fatal exception: " << e.what() << std::endl;
        system("pause");
        return 1;
    }
    catch (...) {
        std::cerr << "Unknown fatal exception." << std::endl;
        system("pause");
        return 1;
    }
}