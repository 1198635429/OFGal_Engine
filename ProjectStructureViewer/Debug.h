// Copyright 2026 MrSeagull. All Rights Reserved.
#pragma once
#include <windows.h>
#include <sstream>

#define DEBUG_LOG(msg) do { \
    std::ostringstream oss; \
    oss << msg; \
    OutputDebugStringA(oss.str().c_str()); \
} while(0)