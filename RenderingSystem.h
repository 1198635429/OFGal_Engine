// Copyright 2026 MrSeagull. All Rights Reserved.
// 默认启用超采样
#pragma once
#include "BMP_Reader.h"
#include "cmdDrawer.h"
#include "SharedTypes.h"

class RenderingSystem {

    Size2DInt CanvasSize;       //最大画布大小

public:
    RenderingSystem(const RenderingSystem&) = delete;
    RenderingSystem& operator=(const RenderingSystem&) = delete;
    static RenderingSystem& getInstance() {
        static RenderingSystem instance;
        return instance;
    }
private:
    RenderingSystem() {
        CanvasSize = getMaxCanvasSize();
    }
    ~RenderingSystem() = default;
};