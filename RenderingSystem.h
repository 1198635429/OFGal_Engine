// Copyright 2026 MrSeagull. All Rights Reserved.

#pragma once
#define NOMINMAX
#include "BMP_Reader.h"
#include "cmdDrawer.h"
#include "SRP.cuh"
#include "PPRP.cuh"
#include "FrameProcessing.cuh"
#include "SharedTypes.h"
#include "_EventBus.h"
#include <cmath>

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
        
        initializeConsoleDrawer();
        CanvasSize = getMaxCanvasSize();

        RefreshRenderObjects(_EventBus::getInstance().publish_ReadLevelData("E:\\Projects\\C++Projects\\OFGal_Engine\\TestLevel1.level"));
        AABB_Remove(RenderObjects);
        SortByDepth(RenderObjects);
        RefreshDepth(RenderObjects);

        Frame frame = Rasterize(RenderObjects);
        drawFrame(frame);
        
    }
    ~RenderingSystem() = default;

    void RefreshRenderObjects(const LevelData& currentLevel);
    void AABB_Remove(std::vector<RenderData>& renderObjects);
    void SortByDepth(std::vector<RenderData>& renderObjects);
    void RefreshDepth(std::vector<RenderData>& renderObjects);
    Frame Rasterize(std::vector<RenderData>& renderObjects, int MSAA_Multiple = 1);
};