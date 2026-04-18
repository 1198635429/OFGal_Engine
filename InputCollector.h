/*添加要检测的按键事件的例子
// 示例：Ctrl+S → 触发 KeyCode::CtrlS（仅按下瞬间）
collector.AddBinding({ 'S', KeyCode::CtrlS, Modifier::Ctrl, true });

// 示例：W 键按下/抬起均触发 KeyCode::W
collector.AddBinding({ 'W', KeyCode::W, Modifier::None, false });

// 示例：鼠标左键状态变化
collector.AddBinding({ VK_LBUTTON, KeyCode::MouseLeft, Modifier::None, false });

// 示例：Ctrl+Shift+A 按下触发一次
collector.AddBinding({ 'A', KeyCode::A, Modifier::Ctrl | Modifier::Shift, true });
*/
#pragma once
#include "Windows.h"
#include "InputSystem.h"
#include <vector>
#include <unordered_map>

// 修饰键枚举（内部定义，避免改动 SharedTypes.h）
enum class Modifier : int {
    None = 0,
    Ctrl = 1 << 0,
    Shift = 1 << 1,
    Alt = 1 << 2
};

inline Modifier operator|(Modifier a, Modifier b) {
    return static_cast<Modifier>(static_cast<int>(a) | static_cast<int>(b));
}

inline bool operator&(Modifier a, Modifier b) {
    return (static_cast<int>(a) & static_cast<int>(b)) != 0;
}

// 按键绑定结构体（内部定义）
struct KeyBinding {
    int vk;                     // 虚拟键码 (VK_xxx)
    KeyCode eventCode;          // 触发时生成的事件代码
    Modifier modifiers = Modifier::None; // 需同时按下的修饰键
    bool edgeOnly = false;      // true = 仅按下瞬间触发一次，false = 状态变化均触发
};

class InputCollector {
public:
    InputCollector(InputSystem* system);
    void update();

    // 添加一个按键绑定
    void AddBinding(const KeyBinding& binding);

private:
    InputSystem* inputsystem;

    std::vector<KeyBinding> m_bindings;
    std::unordered_map<int, bool> m_keyStates;   // 记录各虚拟键的前一帧状态

    bool GetKeyState(int vk) const;
    bool GetPrevKeyState(int vk) const;
    void SetKeyState(int vk, bool down);

    bool CheckModifiers(Modifier required) const;
};