#include "InputCollector.h"
#include "SharedTypes.h"

InputCollector::InputCollector(InputSystem* system) : inputsystem(system) {}

void InputCollector::AddBinding(const KeyBinding& binding) {
    m_bindings.push_back(binding);
}

bool InputCollector::GetKeyState(int vk) const {
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
    bool ctrlOk = !(static_cast<int>(required) & static_cast<int>(Modifier::Ctrl)) || GetKeyState(VK_CONTROL);
    bool shiftOk = !(static_cast<int>(required) & static_cast<int>(Modifier::Shift)) || GetKeyState(VK_SHIFT);
    bool altOk = !(static_cast<int>(required) & static_cast<int>(Modifier::Alt)) || GetKeyState(VK_MENU);
    return ctrlOk && shiftOk && altOk;
}

void InputCollector::update() {
    if (!inputsystem->ShouldCaptureInput())
        return;

    for (const auto& binding : m_bindings) {
        bool currentDown = GetKeyState(binding.vk);
        bool prevDown = GetPrevKeyState(binding.vk);
        bool modifiersMatch = CheckModifiers(binding.modifiers);

        if (binding.edgeOnly) {
            if (currentDown && !prevDown && modifiersMatch) {
                InputEvent event{};
                event.key = binding.eventCode;
                event.type = InputType::KeyDown;
                inputsystem->pushEvent(event);
            }
        }
        else {
            if (currentDown != prevDown && modifiersMatch) {
                InputEvent event{};
                event.key = binding.eventCode;
                event.type = currentDown ? InputType::KeyDown : InputType::KeyUp;
                inputsystem->pushEvent(event);
            }
        }
        SetKeyState(binding.vk, currentDown);
    }
}