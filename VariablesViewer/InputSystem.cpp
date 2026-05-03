#include <iostream>
#include "InputSystem.h"

InputSystem g_inputSystem;  // 全局输入系统实例

void InputSystem::clearEvent() {
    std::lock_guard<std::mutex> lock(mtx);
    events.clear();
}

void InputSystem::pushEvent(const InputEvent& event) {
    std::lock_guard<std::mutex> lock(mtx);
    events.push_back(event);
}

const std::vector<InputEvent>& InputSystem::getEvents() {
    std::lock_guard<std::mutex> lock(mtx);
    return events;
}

void InputSystem::SetWindowHandle(HWND hWnd) {
    m_hWnd = hWnd;
}

void InputSystem::SetGlobalCapture(bool enable) {
    m_globalCapture = enable;
}

bool InputSystem::GetGlobalCapture() const {
    return m_globalCapture;
}

bool InputSystem::ShouldCaptureInput() const {
    if (m_globalCapture)
        return true;
    if (m_hWnd == nullptr)
        return true;   // 若未设置窗口句柄，保守地允许捕捉
    return GetForegroundWindow() == m_hWnd;
}