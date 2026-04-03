// Copyright 2026 MrSeagull. All Rights Reserved.
// 含有基础的后处理函数
#pragma once
#include "SharedTypes.h"
#include <cuda_runtime.h>
#include <math.h>
#include <algorithm>

#ifdef __cplusplus
extern "C" {
#endif

// Bloom 泛光函数
// frame   : 输入输出帧（LDR RGB888）
// threshold : 亮度阈值（0～255），只有亮度高于此值的像素产生泛光
// intensity : 泛光强度（0.0～2.0），控制光晕叠加程度
// blurRadius : 高斯模糊半径（像素，推荐 3～15）
// sigma      : 高斯标准差（若 <=0 则自动取 blurRadius/3）
    void applyBloom(Frame& frame,
        float threshold = 220.0f,
        float intensity = 0.8f,
        int blurRadius = 4,
        float sigma = -1.0f);

// FXAA 抗锯齿函数
// frame          : 输入输出帧（LDR RGB888）
// edgeThreshold  : 边缘检测阈值（0.0~1.0），越高越敏感，默认 0.166（对应 0~255 的约 42）
// edgeThresholdMin: 最小边缘阈值，防止噪声，默认 0.05
// spanMax        : 最大搜索像素距离，默认 8.0
// reduceMul      : 方向搜索步长缩减系数，默认 1.0/8.0
// reduceMin      : 步长最小值，默认 1.0/128.0
    void applyFXAA(Frame& frame,
        float edgeThreshold = 0.166f,
        float edgeThresholdMin = 0.05f,
        float spanMax = 8.0f,
        float reduceMul = 1.0f / 8.0f,
        float reduceMin = 1.0f / 128.0f);

// SMAA 抗锯齿函数（1x 版本）
// frame          : 输入输出帧（LDR RGB888）
// edgeThreshold  : 边缘检测阈值（0.0~1.0），默认 0.05，值越高检测越敏感
// maxSearchSteps : 最大搜索步数（用于局部对比度自适应），默认 4
// enableDiag     : 是否启用对角线模式检测，默认 true
    void applySMAA(Frame& frame,
        float edgeThreshold = 0.05f,
        int maxSearchSteps = 4,
        bool enableDiag = true);

// 镜头畸变（桶形/枕形）
// frame    : 输入输出帧（LDR RGB888）
// strength : 畸变强度，正值 → 桶形畸变（中心向外凸），负值 → 枕形畸变（边缘向内凹），范围建议 -0.3 ~ 0.3
// centerX  : 畸变中心 X 坐标（归一化 0~1，0.5 为图像中心）
// centerY  : 畸变中心 Y 坐标（归一化 0~1，0.5 为图像中心）
    void applyLensDistortion(Frame& frame, float strength = 0.0f, float centerX = 0.5f, float centerY = 0.5f);

// 色差（Chromatic Aberration）
// frame    : 输入输出帧（LDR RGB888）
// strength : 偏移强度（像素），建议范围 0~10，值越大分离越明显
// mode     : 偏移模式，0 = 径向（从中心向外），1 = 水平（水平方向偏移）
// centerX  : 径向模式下的中心 X 坐标（归一化 0~1）
// centerY  : 径向模式下的中心 Y 坐标（归一化 0~1）
    void applyChromaticAberration(Frame& frame, float strength = 2.0f, int mode = 0, float centerX = 0.5f, float centerY = 0.5f);

// 模糊（高斯模糊）函数
// frame      : 输入输出帧（LDR RGB888）
// radius     : 模糊半径（像素），建议 1～15
// sigma      : 高斯标准差，若 <=0 则自动取 radius/3
// direction  : 模糊方向，0 = 无方向（二维高斯），1 = 水平方向，2 = 垂直方向
    void applyBlur(Frame& frame, int radius = 3, float sigma = -1.0f, int direction = 0);

// 锐化（Unsharp Mask）函数
// frame      : 输入输出帧（LDR RGB888）
// strength   : 锐化强度（0.0～2.0），值越大锐化效果越强，建议 0.3～1.0
// radius     : 高斯模糊半径（像素），建议 1～5
// sigma      : 高斯标准差，若 <=0 则自动取 radius/3
    void applySharpen(Frame& frame, float strength = 0.5f, int radius = 2, float sigma = -1.0f);

// 胶片颗粒（Film Grain）
// frame    : 输入输出帧（LDR RGB888）
// intensity: 颗粒强度（0.0～0.2 较自然，更大则明显），建议 0.05
// grainSize: 颗粒大小（像素块大小），1 表示每像素独立，2 表示 2x2 块等，建议 1
// dynamic  : 是否动态（每帧变化），true 时需传入 frameId，false 时基于固定坐标噪声
// frameId  : 当前帧序号（用于动态噪声种子，仅当 dynamic=true 时有效）
    void applyFilmGrain(Frame& frame, float intensity = 0.05f, int grainSize = 1, bool dynamic = true, int frameId = 0);

// 晕影（Vignette）
// frame     : 输入输出帧（LDR RGB888）
// intensity : 暗角强度（0.0～1.0），值越大边缘越暗
// innerRadius: 内半径（归一化距离），0~1，小于此值无暗角，默认 0.6
// outerRadius: 外半径（归一化距离），大于此值达到最大暗角，默认 1.0
// centerX, centerY: 晕影中心（归一化坐标 0~1），默认 (0.5, 0.5)
// exponent  : 衰减曲线指数，1.0 线性，>1 边缘衰减更快，默认 1.0
    void applyVignette(Frame& frame, float intensity = 0.3f, float innerRadius = 0.6f, float outerRadius = 1.0f,
        float centerX = 0.5f, float centerY = 0.5f, float exponent = 1.0f);

// 基础调色函数（逐像素调整）
// frame       : 输入输出帧
// brightness  : 亮度偏移（-1.0 ~ 1.0），0 不变
// contrast    : 对比度（0.0 ~ 2.0），1.0 不变
// saturation  : 饱和度（0.0 ~ 2.0），1.0 不变
// whiteBalance: 白平衡系数（RGB 分别乘），默认 (1,1,1)
// hueShift    : 色相偏移（-180° ~ 180°），单位度
    void applyColorCorrection(Frame& frame,
        float brightness = 0.0f,
        float contrast = 1.0f,
        float saturation = 1.0f,
        float3 whiteBalance = make_float3(1.0f, 1.0f, 1.0f),
        float hueShift = 0.0f);

// 风格化颜色分级（预设风格 + 自定义混合）
// frame       : 输入输出帧
// style       : 风格类型 0=冷色调, 1=暖色调, 2=黑白, 3=复古, 4=赛博朋克, 5=自定义
// intensity   : 风格强度（0.0 ~ 1.0），控制风格与原图的混合比例
// customColor : 自定义色调（仅当 style=5 时有效），作为叠加颜色
    void applyColorGrading(Frame& frame, int style = 0, float intensity = 0.8f, float3 customColor = make_float3(1.0f, 1.0f, 1.0f));

#ifdef __cplusplus
}
#endif