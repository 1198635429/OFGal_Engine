// Copyright 2026 MrSeagull. All Rights Reserved.
#include "SRP.cuh"
#include <cuda_runtime.h>
#include <cmath>
#include <algorithm>

// 三次卷积核权重计算 (a = -0.5)
__device__ float cubicWeight(float x) {
    float ax = fabsf(x);
    if (ax <= 1.0f) {
        return (1.5f * ax - 2.5f) * ax * ax + 1.0f;
    }
    else if (ax < 2.0f) {
        return ((-0.5f * ax + 2.5f) * ax - 4.0f) * ax + 2.0f;
    }
    else {
        return 0.0f;
    }
}

// 双三次插值内核
__global__ void resizeBicubicKernel(const BMP_Pixel* src, int srcWidth, int srcHeight,
    BMP_Pixel* dst, int dstWidth, int dstHeight) {
    int x = blockIdx.x * blockDim.x + threadIdx.x;
    int y = blockIdx.y * blockDim.y + threadIdx.y;
    if (x >= dstWidth || y >= dstHeight) return;

    // 目标像素对应源图像中的浮点坐标（保持中心对齐）
    float sx = (x + 0.5f) * (srcWidth / (float)dstWidth) - 0.5f;
    float sy = (y + 0.5f) * (srcHeight / (float)dstHeight) - 0.5f;

    // 整数坐标和小数部分
    int ix = floorf(sx);
    int iy = floorf(sy);
    float dx = sx - ix;
    float dy = sy - iy;

    // 累加器
    float r = 0.0f, g = 0.0f, b = 0.0f, a = 0.0f;
    float totalWeight = 0.0f;

    // 遍历4x4邻域 (i = -1..2, j = -1..2)
    for (int i = -1; i <= 2; ++i) {
        int srcX = ix + i;
        // 边界处理：clamp到有效范围
        srcX = max(0, min(srcX, srcWidth - 1));
        float wx = cubicWeight(dx - i);  // 注意：公式中使用 dx - i

        for (int j = -1; j <= 2; ++j) {
            int srcY = iy + j;
            srcY = max(0, min(srcY, srcHeight - 1));
            float wy = cubicWeight(dy - j);
            float w = wx * wy;

            const BMP_Pixel& p = src[srcY * srcWidth + srcX];
            r += p.red * w;
            g += p.green * w;
            b += p.blue * w;
            a += p.alpha * w;
            totalWeight += w;
        }
    }

    // 归一化（权重和可能不是1，但通常非常接近1；这里做归一化保证颜色准确）
    if (totalWeight > 1e-6f) {
        r /= totalWeight;
        g /= totalWeight;
        b /= totalWeight;
        a /= totalWeight;
    }

    // 钳位到[0,255]并存入目标
    BMP_Pixel& out = dst[y * dstWidth + x];
    out.red = (unsigned char)(max(0.0f, min(255.0f, r)));
    out.green = (unsigned char)(max(0.0f, min(255.0f, g)));
    out.blue = (unsigned char)(max(0.0f, min(255.0f, b)));
    out.alpha = (unsigned char)(max(0.0f, min(255.0f, a)));
}

// 主机端函数：将源图像缩放到目标尺寸
extern "C" BMP_Data ForceResizeImage_Bicubic(const BMP_Data& src, const Size2DInt& dstSize) {
    BMP_Data dst;
    dst.width = dstSize.x;
    dst.height = dstSize.y;
    dst.pixels.resize(dst.width * dst.height);

    // 如果源图像为空或目标尺寸无效，直接返回空图像
    if (src.pixels.empty() || dst.width <= 0 || dst.height <= 0) {
        return dst;
    }

    // 设备内存分配
    BMP_Pixel* d_src = nullptr, * d_dst = nullptr;
    size_t srcBytes = src.pixels.size() * sizeof(BMP_Pixel);
    size_t dstBytes = dst.pixels.size() * sizeof(BMP_Pixel);

    cudaMalloc(&d_src, srcBytes);
    cudaMalloc(&d_dst, dstBytes);

    // 拷贝源图像到设备
    cudaMemcpy(d_src, src.pixels.data(), srcBytes, cudaMemcpyHostToDevice);

    // 配置内核启动参数
    dim3 blockSize(16, 16);  // 可根据硬件调整
    dim3 gridSize((dst.width + blockSize.x - 1) / blockSize.x,
        (dst.height + blockSize.y - 1) / blockSize.y);

    // 启动内核
    resizeBicubicKernel << <gridSize, blockSize >> > (d_src, src.width, src.height,
        d_dst, dst.width, dst.height);

    // 拷贝结果回主机
    cudaMemcpy(dst.pixels.data(), d_dst, dstBytes, cudaMemcpyDeviceToHost);

    // 释放设备内存
    cudaFree(d_src);
    cudaFree(d_dst);

    // 检查CUDA错误
    cudaError_t err = cudaGetLastError();
    if (err != cudaSuccess) {
        BMP_Data empty;
        return empty;
    }

    return dst;
}