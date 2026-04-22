// Copyright 2026 MrSeagull. All Rights Reserved.
#pragma once
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <windows.h>

struct FolderStructure {
    std::string SelfName;
    std::vector<std::string> Files;
    std::map<std::string, std::unique_ptr<FolderStructure>> SubFolders;
};

struct ProjectStructure {
    std::string RootDirectory;
    FolderStructure Self;
};

struct TreeDisplayInfo {
    int width;   // 纯文本宽度（不含ANSI转义）
    int height;  // 总行数
};

struct SelectedFolderInfo {
    std::string absolutePath;
    int lineNumber;
};

// ==================== 输入系统相关 ====================
// 键码枚举
enum class KeyCode {
    Unknown = 0,

    // 字母键 (A-Z)
    A, B, C, D, E, F, G, H, I, J, K, L, M,
    N, O, P, Q, R, S, T, U, V, W, X, Y, Z,

    // 数字键 (主键盘区)
    Num0, Num1, Num2, Num3, Num4, Num5, Num6, Num7, Num8, Num9,

    // 功能键
    F1, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12,

    // 修饰键
    LShift, RShift, LControl, RControl, LAlt, RAlt,
    LWin, RWin,
    CapsLock, NumLock, ScrollLock,

    // 导航与编辑键
    Space, Enter, Backspace, Tab,
    Escape, Pause, PrintScreen,
    Insert, Delete, Home, End, PageUp, PageDown,
    Left, Right, Up, Down,

    // 符号键（无 Shift 状态）
    Grave, Minus, Equal, LeftBracket, RightBracket, Backslash,
    Semicolon, Apostrophe, Comma, Period, Slash,

    // 小键盘区
    Numpad0, Numpad1, Numpad2, Numpad3, Numpad4,
    Numpad5, Numpad6, Numpad7, Numpad8, Numpad9,
    NumpadAdd, NumpadSubtract, NumpadMultiply, NumpadDivide, NumpadDecimal,
    NumpadEnter,

    // 多媒体与浏览器键
    VolumeMute, VolumeDown, VolumeUp,
    MediaNext, MediaPrev, MediaStop, MediaPlayPause,
    BrowserBack, BrowserForward, BrowserRefresh, BrowserHome,

    // 应用程序特定组合
    CtrlS,          // Ctrl + S
    CtrlN,          // Ctrl + N

    // 鼠标按键
    MouseLeft,
    MouseRight,
    MouseMiddle,
    MouseX1,        // 侧键后退
    MouseX2         // 侧键前进
};

// 修饰键枚举
enum class Modifier {
    None = 0,
    Ctrl = 1 << 0,
    Shift = 1 << 1,
    Alt = 1 << 2
};

inline Modifier operator|(Modifier a, Modifier b) {
    return static_cast<Modifier>(static_cast<int>(a) | static_cast<int>(b));
}

// 键位绑定结构
struct KeyBinding {
    int      vk;           // 虚拟键码 (如 'N', VK_DELETE)
    Modifier modifiers;    // 需要的修饰键
    KeyCode  eventCode;    // 触发时生成的事件码
    bool     edgeOnly;     // true = 仅按下瞬间触发一次；false = 按下/释放均触发
};

// 输入事件类型
enum class InputType {
    KeyDown,
    KeyUp,
    MouseMove,
    MouseUp,
    MouseDown
};

// 输入事件结构
struct InputEvent {
    InputType type;
    KeyCode   key;
    int       mouseX = 0;
    int       mouseY = 0;
};

// ==================== IPC / 对话框相关 ====================
struct YesNoDialogData {
    WCHAR message[300];
    BOOL  result;
};