// Copyright 2026 MrSeagull. All Rights Reserved.
#include "InputHandler.h"
#include <conio.h>

InputAction InputHandler::PollKeyboard() {
    if (!_kbhit()) return InputAction::None;

    int ch = _getch();
    if (ch == 0 || ch == 224) { // 功能键前缀
        ch = _getch();
        switch (ch) {
        case 72: return InputAction::MoveUp;    // 上箭头
        case 80: return InputAction::MoveDown;  // 下箭头
        default: return InputAction::None;
        }
    }
    else {
        switch (ch) {
        case 'w': case 'W': return InputAction::MoveUp;
        case 's': case 'S': return InputAction::MoveDown;
        default: return InputAction::None;
        }
    }
}

bool InputHandler::CheckMouseLeftClick(int& outRow) {
    HANDLE hConsole = GetStdHandle(STD_INPUT_HANDLE);
    if (hConsole == INVALID_HANDLE_VALUE) return false;

    DWORD numEvents = 0;
    if (!GetNumberOfConsoleInputEvents(hConsole, &numEvents) || numEvents == 0)
        return false;

    // 只读取第一个事件，避免积压
    INPUT_RECORD ir;
    DWORD read;
    if (PeekConsoleInput(hConsole, &ir, 1, &read) && read > 0) {
        if (ir.EventType == MOUSE_EVENT) {
            const MOUSE_EVENT_RECORD& mouse = ir.Event.MouseEvent;
            if (mouse.dwButtonState == FROM_LEFT_1ST_BUTTON_PRESSED) {
                // 读取该事件以从队列中移除
                ReadConsoleInput(hConsole, &ir, 1, &read);
                outRow = mouse.dwMousePosition.Y;
                return true;
            }
            else if (mouse.dwEventFlags == 0) {
                // 如果是按钮释放等其他鼠标事件，直接读取丢弃，防止堆积
                ReadConsoleInput(hConsole, &ir, 1, &read);
            }
        }
    }
    return false;
}