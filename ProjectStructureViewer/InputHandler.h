// Copyright 2026 MrSeagull. All Rights Reserved.
#pragma once
#include <windows.h>

enum class InputAction {
    None,
    MoveUp,
    MoveDown,
    Quit,       // 可用于自定义退出键（如 ESC），本示例不使用
};

class InputHandler {
public:
    static bool CheckMouseLeftClick(int& outRow);

    // 非阻塞检测键盘输入，返回相应的动作
    static InputAction PollKeyboard();
};