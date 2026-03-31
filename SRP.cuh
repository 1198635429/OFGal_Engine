// Copyright 2026 MrSeagull. All Rights Reserved.
// Sequential Rendering Pipeline (顺序渲染管线：从后向前渲染)
#pragma once
#define NOMINMAX
#include "SharedTypes.h"

#ifdef __cplusplus
extern "C" {
#endif

    // 纹理采样方法枚举
    enum TextureSamplingMethod {
        SAMPLING_NEAREST = 0,
        SAMPLING_BILINEAR = 1,
        SAMPLING_BICUBIC = 2,
        SAMPLING_ANISOTROPIC = 3
    };

    // 渲染一个四边形对象，支持多种纹理采样方法
    // anisoLevel: 各向异性采样倍数（仅在方法为 ANISOTROPIC 时有效，范围 1~16）
    void Rasterize_An_Object(Frame& frame, const RenderData& obj,
        TextureSamplingMethod method, int anisoLevel = 1, int MSAA = 1);

#ifdef __cplusplus
}
#endif