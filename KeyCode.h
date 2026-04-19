// Copyright 2026 MrSeagull. All Rights Reserved.
#pragma once

enum class KeyCode {
    Unknown = 0,

    // === 字母键 (A-Z) ===
    A, B, C, D, E, F, G, H, I, J, K, L, M,
    N, O, P, Q, R, S, T, U, V, W, X, Y, Z,

    // === 数字键 (主键盘区) ===
    Num0, Num1, Num2, Num3, Num4, Num5, Num6, Num7, Num8, Num9,

    // === 功能键 ===
    F1, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12,

    // === 修饰键 ===
    LShift, RShift, LControl, RControl, LAlt, RAlt,
    LWin, RWin,           // Windows 键
    CapsLock, NumLock, ScrollLock,

    // === 导航与编辑键 ===
    Space, Enter, Backspace, Tab,
    Escape, Pause, PrintScreen,
    Insert, Delete, Home, End, PageUp, PageDown,
    Left, Right, Up, Down,

    // === 符号键（无 Shift 状态）===
    Grave,          // ` 或 ~
    Minus,          // - 或 _
    Equal,          // = 或 +
    LeftBracket,    // [ 或 {
    RightBracket,   // ] 或 }
    Backslash,      // \ 或 |
    Semicolon,      // ; 或 :
    Apostrophe,     // ' 或 "
    Comma,          // , 或 <
    Period,         // . 或 >
    Slash,          // / 或 ?

    // === 小键盘区 ===
    Numpad0, Numpad1, Numpad2, Numpad3, Numpad4,
    Numpad5, Numpad6, Numpad7, Numpad8, Numpad9,
    NumpadAdd, NumpadSubtract, NumpadMultiply, NumpadDivide, NumpadDecimal,
    NumpadEnter,

    // === 多媒体与浏览器键（常用）===
    VolumeMute, VolumeDown, VolumeUp,
    MediaNext, MediaPrev, MediaStop, MediaPlayPause,
    BrowserBack, BrowserForward, BrowserRefresh, BrowserHome,

    // === 应用程序特定组合（已使用，保留）===
    CtrlS,          // Ctrl + S
    CtrlN,          // Ctrl + N

    // === 鼠标按键 ===
    MouseLeft,
    MouseRight,
    MouseMiddle,
    MouseX1,        // 侧键后退
    MouseX2,        // 侧键前进
};