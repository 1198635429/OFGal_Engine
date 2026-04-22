#pragma once
#include "SharedTypes.h"
#include <vector>
#include <mutex>
#include <windows.h>

class InputSystem {
public:
    void clearEvent();
    void pushEvent(const InputEvent& event);
    const std::vector<InputEvent>& getEvents();

    void SetWindowHandle(HWND hWnd);
    void SetGlobalCapture(bool enable);
    bool GetGlobalCapture() const;
    bool ShouldCaptureInput() const;

private:
    std::vector<InputEvent> events;
    std::mutex mtx;
    HWND m_hWnd = nullptr;          // 本程序窗口句柄
    bool m_globalCapture = false;   // 默认仅焦点时捕捉
};