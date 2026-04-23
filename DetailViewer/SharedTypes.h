// Copyright 2026 Nagato-Yuki-708 & MrSeagull. All Rights Reserved.
#pragma once

#include <string>
#include <vector>
#include <map>
#include <optional>
#include <memory>
#include <windows.h>

/*
=================================================
文件系统以及场景结构体定义
=================================================
*/
struct FolderStructure {
    std::string SelfName;
    std::vector<std::string> Files;
    std::map<std::string, std::unique_ptr<FolderStructure>> SubFolders;
};

struct ProjectStructure {
    std::string RootDirectory;
    FolderStructure Self;
};

struct BMP_Pixel {
    unsigned char red = 0;
    unsigned char green = 0;
    unsigned char blue = 0;
    unsigned char alpha = 0;
};

struct BMP_Data {
    int width;
    int height;
    std::vector<BMP_Pixel> pixels;
};

// 坐标与尺寸辅助结构
struct Location3D {
    float x = 0.0f;
    float y = 0.0f;
    int z = 0;
};

struct Location2D {
    float x = 0.0f;
    float y = 0.0f;
};

struct Location2DInt {
    int x = 0;
    int y = 0;
};

struct RotationEuler {
    float r = 0.0f;
};

struct Scale2D {
    float x = 1.0f;
    float y = 1.0f;
};

struct Size2D {
    float x = 0.0f;   // <=0 表示使用原始图片大小
    float y = 0.0f;
};

struct Size2DInt {
    int x = 0;         // <=0 表示使用默认大小
    int y = 0;
};

// 组件结构
struct TransformComponent {
    Location3D Location;
    RotationEuler Rotation;
    Scale2D Scale;
};

struct PictureComponent {
    std::string Path;
    Location3D Location;
    RotationEuler Rotation;
    Size2D Size;
};

struct TextblockComponent {
    struct TextInfo {
        std::string component;
        int Font_size = 0;      // <=0 表示默认大小
        bool ANSI_Print = false;
    };
    Location2DInt Location;
    Size2DInt Size;
    TextInfo Text;
    Scale2D Scale;
};

struct TriggerAreaComponent {
    Location2D Location;
    Size2D Size;                // <=0 视为0.0（文档要求）
};

struct BlueprintComponent {
    std::string Path;
};

// ========== ObjectData ==========
struct ObjectData {
    std::string name;
    ObjectData* parent = nullptr;
    std::map<std::string, ObjectData*> objects;
    std::optional<TransformComponent> Transform;
    std::optional<PictureComponent> Picture;
    std::optional<TextblockComponent> Textblock;
    std::optional<TriggerAreaComponent> TriggerArea;
    std::optional<BlueprintComponent> Blueprint;

    // 显式默认构造函数
    ObjectData() = default;

    // 递归析构：安全释放所有子对象
    ~ObjectData() {
        for (auto& pair : objects) {
            delete pair.second;
        }
        objects.clear();
    }

    // 禁止拷贝
    ObjectData(const ObjectData&) = delete;
    ObjectData& operator=(const ObjectData&) = delete;

    // 禁止移动（移动会导致指针复制，同样引发双重释放）
    ObjectData(ObjectData&&) = delete;
    ObjectData& operator=(ObjectData&&) = delete;
};

// ========== LevelData ==========
struct LevelData {
    std::string name;
    std::map<std::string, ObjectData*> objects;

    // 显式默认构造函数
    LevelData() = default;

    // 析构：释放所有对象
    ~LevelData() {
        for (auto& pair : objects) {
            delete pair.second;
        }
        objects.clear();
    }

    // 禁止拷贝
    LevelData(const LevelData&) = delete;
    LevelData& operator=(const LevelData&) = delete;

    // 允许移动（需要显式声明，因为定义了析构函数）
    LevelData(LevelData&& other) noexcept
        : name(std::move(other.name)), objects(std::move(other.objects)) {
        // 将源对象的 map 清空，防止其析构时再次删除
        other.objects.clear();
    }

    LevelData& operator=(LevelData&& other) noexcept {
        if (this != &other) {
            // 先释放当前对象拥有的资源
            for (auto& pair : objects) {
                delete pair.second;
            }
            objects.clear();

            name = std::move(other.name);
            objects = std::move(other.objects);
            other.objects.clear();
        }
        return *this;
    }
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
// （保持不变）
enum class KeyCode {
    Unknown = 0,
    A, B, C, D, E, F, G, H, I, J, K, L, M,
    N, O, P, Q, R, S, T, U, V, W, X, Y, Z,
    Num0, Num1, Num2, Num3, Num4, Num5, Num6, Num7, Num8, Num9,
    F1, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12,
    LShift, RShift, LControl, RControl, LAlt, RAlt,
    LWin, RWin,
    CapsLock, NumLock, ScrollLock,
    Space, Enter, Backspace, Tab,
    Escape, Pause, PrintScreen,
    Insert, Delete, Home, End, PageUp, PageDown,
    Left, Right, Up, Down,
    Grave, Minus, Equal, LeftBracket, RightBracket, Backslash,
    Semicolon, Apostrophe, Comma, Period, Slash,
    Numpad0, Numpad1, Numpad2, Numpad3, Numpad4,
    Numpad5, Numpad6, Numpad7, Numpad8, Numpad9,
    NumpadAdd, NumpadSubtract, NumpadMultiply, NumpadDivide, NumpadDecimal,
    NumpadEnter,
    VolumeMute, VolumeDown, VolumeUp,
    MediaNext, MediaPrev, MediaStop, MediaPlayPause,
    BrowserBack, BrowserForward, BrowserRefresh, BrowserHome,
    CtrlS, CtrlN,
    MouseLeft, MouseRight, MouseMiddle, MouseX1, MouseX2
};

enum class Modifier {
    None = 0,
    Ctrl = 1 << 0,
    Shift = 1 << 1,
    Alt = 1 << 2
};

inline Modifier operator|(Modifier a, Modifier b) {
    return static_cast<Modifier>(static_cast<int>(a) | static_cast<int>(b));
}

struct KeyBinding {
    int      vk;
    Modifier modifiers;
    KeyCode  eventCode;
    bool     edgeOnly;
};

enum class InputType {
    KeyDown,
    KeyUp,
    MouseMove,
    MouseUp,
    MouseDown
};

struct InputEvent {
    InputType type;
    KeyCode   key;
    int       mouseX = 0;
    int       mouseY = 0;
};

struct YesNoDialogData {
    WCHAR message[300];
    BOOL  result;
};