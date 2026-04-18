#include "InputCollector.h"
#include "InputEvent.h"

InputCollector::InputCollector(InputSystem* system) : inputsystem(system) {}

void InputCollector::AddBinding(const KeyBinding& binding) {
    m_bindings.push_back(binding);
}

bool InputCollector::GetKeyState(int vk) const {
    // 注意：VK_CONTROL / VK_SHIFT / VK_MENU 需要特殊处理左右键
    if (vk == VK_CONTROL) {
        return (GetAsyncKeyState(VK_LCONTROL) & 0x8000) || (GetAsyncKeyState(VK_RCONTROL) & 0x8000);
    }
    if (vk == VK_SHIFT) {
        return (GetAsyncKeyState(VK_LSHIFT) & 0x8000) || (GetAsyncKeyState(VK_RSHIFT) & 0x8000);
    }
    if (vk == VK_MENU) {  // Alt
        return (GetAsyncKeyState(VK_LMENU) & 0x8000) || (GetAsyncKeyState(VK_RMENU) & 0x8000);
    }
    return (GetAsyncKeyState(vk) & 0x8000) != 0;
}

bool InputCollector::GetPrevKeyState(int vk) const {
    auto it = m_keyStates.find(vk);
    return (it != m_keyStates.end()) ? it->second : false;
}

void InputCollector::SetKeyState(int vk, bool down) {
    m_keyStates[vk] = down;
}

bool InputCollector::CheckModifiers(Modifier required) const {
    if (required == Modifier::None)
        return true;

    bool ctrlOk = !(required & Modifier::Ctrl) || GetKeyState(VK_CONTROL);
    bool shiftOk = !(required & Modifier::Shift) || GetKeyState(VK_SHIFT);
    bool altOk = !(required & Modifier::Alt) || GetKeyState(VK_MENU);
    return ctrlOk && shiftOk && altOk;
}

void InputCollector::update() {
    for (const auto& binding : m_bindings) {
        bool currentDown = GetKeyState(binding.vk);
        bool prevDown = GetPrevKeyState(binding.vk);
        bool modifiersMatch = CheckModifiers(binding.modifiers);

        if (binding.edgeOnly) {
            // 仅在按键从 抬起 → 按下 且修饰键满足时触发一次
            if (currentDown && !prevDown && modifiersMatch) {
                InputEvent event{};
                event.key = binding.eventCode;
                event.type = InputType::KeyDown;
                inputsystem->pushEvent(event);
            }
        }
        else {
            // 状态变化时生成对应事件（要求修饰键满足）
            if (currentDown != prevDown && modifiersMatch) {
                InputEvent event{};
                event.key = binding.eventCode;
                event.type = currentDown ? InputType::KeyDown : InputType::KeyUp;
                inputsystem->pushEvent(event);
            }
        }

        // 更新状态记录
        SetKeyState(binding.vk, currentDown);
    }
}