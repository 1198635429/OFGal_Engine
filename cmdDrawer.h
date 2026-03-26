// Copyright 2026 MrSeagull. All Rights Reserved.
#pragma once
#define NOMINMAX
#include <windows.h>
#include <vector>
#include "SharedTypes.h"

// 初始化控制台绘图环境（设置字体、窗口、缓冲区、VT 模式等）
// 返回 true 表示成功，false 表示失败
bool initializeConsoleDrawer();

// 获取当前控制台可绘制的最大画布尺寸（单位：像素）
Size2DInt getMaxCanvasSize();

// 绘制帧到控制台（要求 frame 的宽高与最大画布尺寸完全一致）
void drawFrame(const Frame& frame);

// 恢复控制台原始设置并释放资源
void shutdownConsoleDrawer();