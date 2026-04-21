// Copyright 2026 MrSeagull. All Rights Reserved.
#include <windows.h>
#include <iostream>
#include <string>
#include "SharedTypes.h"

#define SHARED_MEM_NAME     L"Global\\OFGal_Engine_YesOrNo_SharedMem"
#define EVENT_GET_RESULT    L"Global\\OFGal_Engine_YesOrNo_GetResult"
#define EVENT_EXIT          L"Global\\OFGal_Engine_YesOrNo_Exit"

void WriteLine(const std::wstring& text) {
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD written;
    WriteConsoleW(hOut, text.c_str(), static_cast<DWORD>(text.length()), &written, nullptr);
    WriteConsoleW(hOut, L"\r\n", 2, &written, nullptr);
}

void Write(const std::wstring& text) {
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD written;
    WriteConsoleW(hOut, text.c_str(), static_cast<DWORD>(text.length()), &written, nullptr);
}

int main() {
    SetConsoleTitleW(L"OFGal_Engine/Yes_or_No");

    HANDLE hSharedMem = OpenFileMappingW(FILE_MAP_ALL_ACCESS, FALSE, SHARED_MEM_NAME);
    if (!hSharedMem) {
        WriteLine(L"[YesOrNo] 无法打开共享内存。");
        return 1;
    }

    YesNoDialogData* pData = (YesNoDialogData*)MapViewOfFile(hSharedMem, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(YesNoDialogData));
    if (!pData) {
        WriteLine(L"[YesOrNo] 无法映射共享内存。");
        CloseHandle(hSharedMem);
        return 1;
    }

    HANDLE hEventGetResult = OpenEventW(EVENT_MODIFY_STATE, FALSE, EVENT_GET_RESULT);
    HANDLE hEventExit = OpenEventW(SYNCHRONIZE, FALSE, EVENT_EXIT);

    if (!hEventGetResult || !hEventExit) {
        WriteLine(L"[YesOrNo] 无法打开事件对象。");
        UnmapViewOfFile(pData);
        CloseHandle(hSharedMem);
        return 1;
    }

    // 输出提示信息（宽字符直接输出）
    WriteLine(L"========================================");
    WriteLine(pData->message);
    WriteLine(L"========================================");
    Write(L"请输入 Y (确认) 或 N (取消): ");

    // 从控制台读取一个字符（使用 _getwch() 避免回显问题）
    wchar_t ch = _getwch();
    WriteLine(std::wstring(1, ch));  // 回显用户输入的字符

    pData->result = (ch == L'Y' || ch == L'y') ? TRUE : FALSE;

    SetEvent(hEventGetResult);

    WriteLine(L"等待父进程确认...");
    DWORD waitResult = WaitForSingleObject(hEventExit, 1000);
    if (waitResult == WAIT_OBJECT_0) {
        WriteLine(L"收到退出信号。");
    }
    else if (waitResult == WAIT_TIMEOUT) {
        WriteLine(L"等待超时，主动退出。");
    }

    UnmapViewOfFile(pData);
    CloseHandle(hSharedMem);
    CloseHandle(hEventGetResult);
    CloseHandle(hEventExit);

    return 0;
}