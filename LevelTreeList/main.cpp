// Copyright 2026 Nagato-Yuki-708. All Rights Reserved.
#include <iostream>
#include <windows.h>
void setWindow_SizeAndPosition() {
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
        SetWindowPos(hwndConsole, nullptr, 2040.0f * scaleX, 150.0f * scaleY, 520.0f * scaleX, 500.0f * scaleY, SWP_NOZORDER | SWP_NOACTIVATE);
        CloseHandle(hwndConsole);
    }
}
int main()
{
    SetConsoleTitleW(L"OFGal_Engine/LevelTreeList");
    setWindow_SizeAndPosition();
    std::cout << "Hello World!" << std::endl;

    system("pause");
    return 0;
}