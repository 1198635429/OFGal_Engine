// Copyright 2026 Nagato-Yuki-708. All Rights Reserved.
#pragma once
#include <windows.h>
#include <sstream>

#define DEBUG_A(msg) do { \
    std::ostringstream oss; \
    oss << msg; \
    OutputDebugStringA(oss.str().c_str()); \
} while(0)

#define DEBUG_W(msg) do { \
    std::wostringstream woss; \
    woss << msg; \
    OutputDebugStringW(woss.str().c_str()); \
} while(0)