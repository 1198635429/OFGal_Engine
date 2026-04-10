// Copyright 2026 MrSeagull. All Rights Reserved.

#pragma once
#define NOMINMAX
#include "BMP_Reader.h"
#include "cmdDrawer.h"
#include "SRP.cuh"
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
    Frame Render_A_Frame(const LevelData& currentLevel, TextureSamplingMethod samplingMethod = SAMPLING_BICUBIC, int MSAA_Multiple = 1);
    Frame Render_A_Frame_ANISOTROPIC(const LevelData& currentLevel, int anisoLevel = 1, int MSAA_Multiple = 1);
    void RenderAndPrint(const LevelData& currentLevel, TextureSamplingMethod samplingMethod = SAMPLING_BICUBIC, int MSAA_Multiple = 1);
    void RenderAndPrint_ANISOTROPIC(const LevelData& currentLevel, int anisoLevel = 1, int MSAA_Multiple = 1);
    void Print_A_Frame(const Frame& frame);
private:
    RenderingSystem() {
        initializeConsoleDrawer();
        CanvasSize = getMaxCanvasSize();

        _EventBus::getInstance().subscribe_RenderAndPrint(
            [this](const LevelData& level, TextureSamplingMethod method, int msaa) {
                this->RenderAndPrint(level, method, msaa);
            }
        );
        _EventBus::getInstance().subscribe_RenderAndPrint_ANISOTROPIC(
            [this](const LevelData& level, int anisoLevel, int msaa) {
                this->RenderAndPrint_ANISOTROPIC(level, anisoLevel, msaa);
            }
        );
        _EventBus::getInstance().subscribe_Render_A_Frame(
            [this](const LevelData& level, TextureSamplingMethod method, int msaa) {
                return this->Render_A_Frame(level, method, msaa);
            }
        );
        _EventBus::getInstance().subscribe_Render_A_Frame_ANISOTROPIC(
            [this](const LevelData& level, int anisoLevel, int msaa) {
                return this->Render_A_Frame_ANISOTROPIC(level, anisoLevel, msaa);
            }
        );
        _EventBus::getInstance().subscribe_Print_A_Frame(
            [this](const Frame& frame) {
                this->Print_A_Frame(frame);
            }
        );
        // 后处理事件订阅（调用 FrameProcessing.cuh 中的全局函数）
        _EventBus::getInstance().subscribe_applyBloom(
            [](Frame& frame, float threshold, float intensity, int blurRadius, float sigma) {
                applyBloom(frame, threshold, intensity, blurRadius, sigma);
            }
        );
        _EventBus::getInstance().subscribe_applyBlur(
            [](Frame& frame, int radius, float sigma, int direction) {
                applyBlur(frame, radius, sigma, direction);
            }
        );
        _EventBus::getInstance().subscribe_applyChromaticAberration(
            [](Frame& frame, float strength, int mode, float centerX, float centerY) {
                applyChromaticAberration(frame, strength, mode, centerX, centerY);
            }
        );
        _EventBus::getInstance().subscribe_applyColorCorrection(
            [](Frame& frame, float brightness, float contrast, float saturation, float3 whiteBalance, float hueShift) {
                applyColorCorrection(frame, brightness, contrast, saturation, whiteBalance, hueShift);
            }
        );
        _EventBus::getInstance().subscribe_applyColorGrading(
            [](Frame& frame, int style, float intensity, float3 customColor) {
                applyColorGrading(frame, style, intensity, customColor);
            }
        );
        _EventBus::getInstance().subscribe_applyFXAA(
            [](Frame& frame, float edgeThreshold, float edgeThresholdMin, float spanMax, float reduceMul, float reduceMin) {
                applyFXAA(frame, edgeThreshold, edgeThresholdMin, spanMax, reduceMul, reduceMin);
            }
        );
        _EventBus::getInstance().subscribe_applyFilmGrain(
            [](Frame& frame, float intensity, int grainSize, bool dynamic, int frameId) {
                applyFilmGrain(frame, intensity, grainSize, dynamic, frameId);
            }
        );
        _EventBus::getInstance().subscribe_applyLensDistortion(
            [](Frame& frame, float strength, float centerX, float centerY) {
                applyLensDistortion(frame, strength, centerX, centerY);
            }
        );
        _EventBus::getInstance().subscribe_applySMAA(
            [](Frame& frame, float edgeThreshold, int maxSearchSteps, bool enableDiag) {
                applySMAA(frame, edgeThreshold, maxSearchSteps, enableDiag);
            }
        );
        _EventBus::getInstance().subscribe_applySharpen(
            [](Frame& frame, float strength, int radius, float sigma) {
                applySharpen(frame, strength, radius, sigma);
            }
        );
        _EventBus::getInstance().subscribe_applyVignette(
            [](Frame& frame, float intensity, float innerRadius, float outerRadius,
                float centerX, float centerY, float exponent) {
                    applyVignette(frame, intensity, innerRadius, outerRadius, centerX, centerY, exponent);
            }
        );
    }
    ~RenderingSystem() = default;

    void RefreshRenderObjects(const LevelData& currentLevel);
    void AABB_Remove(std::vector<RenderData>& renderObjects);
    void SortByDepth(std::vector<RenderData>& renderObjects);
    void RefreshDepth(std::vector<RenderData>& renderObjects);
    Frame Rasterize(std::vector<RenderData>& renderObjects, TextureSamplingMethod samplingMethod = SAMPLING_BICUBIC, int MSAA_Multiple = 1);
    Frame Rasterize_ANISOTROPIC(std::vector<RenderData>& renderObjects, int anisoLevel = 1, int MSAA_Multiple = 1);
};