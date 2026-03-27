// Copyright 2026 MrSeagull. All Rights Reserved.

#pragma once
#include "BMP_Reader.h"
#include "cmdDrawer.h"
#include "SRP.cuh"
#include "PPRP.cuh"
#include "FrameProcessing.cuh"
#include "SharedTypes.h"

class RenderingSystem {

    Size2DInt CanvasSize;       //最大画布大小
    std::vector<RenderData> RenderObjects;

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

    void RefreshRenderObjects(const LevelData& currentLevel);
};