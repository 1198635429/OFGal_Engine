// Copyright 2026 MrSeagull. All Rights Reserved.
#include "SRP.cuh"
#include <cuda_runtime.h>
#include <cmath>
#include <algorithm>

// ========== 辅助函数 ==========
__device__ Vector3D mulMatrixVector(const Matrix3D& mat, const Vector3D& vec) {
    Vector3D result;
    result.x = mat.m[0][0] * vec.x + mat.m[0][1] * vec.y + mat.m[0][2] * vec.z;
    result.y = mat.m[1][0] * vec.x + mat.m[1][1] * vec.y + mat.m[1][2] * vec.z;
    result.z = mat.m[2][0] * vec.x + mat.m[2][1] * vec.y + mat.m[2][2] * vec.z;
    return result;
}

// 点是否在三角形内（包含边界）
__device__ bool isPointInTriangle(float px, float py,
    float ax, float ay, float bx, float by, float cx, float cy) {
    float abx = bx - ax, aby = by - ay;
    float apx = px - ax, apy = py - ay;
    float cross1 = abx * apy - aby * apx;

    float bcx = cx - bx, bcy = cy - by;
    float bpx = px - bx, bpy = py - by;
    float cross2 = bcx * bpy - bcy * bpx;

    float cax = ax - cx, cay = ay - cy;
    float cpx = px - cx, cpy = py - cy;
    float cross3 = cax * cpy - cay * cpx;

    bool hasNeg = (cross1 < 0) || (cross2 < 0) || (cross3 < 0);
    bool hasPos = (cross1 > 0) || (cross2 > 0) || (cross3 > 0);
    return !(hasNeg && hasPos);
}

// ========== 采样权重函数 ==========
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

__device__ float bsplineWeight(float x) {
    x = fabsf(x);
    if (x < 1.0f) {
        return 0.5f * x * x * x - x * x + 2.0f / 3.0f;
    }
    else if (x < 2.0f) {
        return -x * x * x / 6.0f + x * x - 2.0f * x + 4.0f / 3.0f;
    }
    else {
        return 0.0f;
    }
}

// ========== 各向异性采样辅助：双线性采样单个点 ==========
__device__ void bilinearSample(float i, float j,
    const unsigned char* texPixels, int texWidth, int texHeight,
    float& outR, float& outG, float& outB, float& outA) {
    i = fmaxf(0.0f, fminf(i, texWidth - 1.0f));
    j = fmaxf(0.0f, fminf(j, texHeight - 1.0f));

    int left = (int)i;
    int top = (int)j;
    int right = left + 1;
    int bottom = top + 1;

    left = max(0, min(left, texWidth - 1));
    right = max(0, min(right, texWidth - 1));
    top = max(0, min(top, texHeight - 1));
    bottom = max(0, min(bottom, texHeight - 1));

    float u = i - left;
    float v = j - top;
    float one_minus_u = 1.0f - u;
    float one_minus_v = 1.0f - v;

    int idxTL = (top * texWidth + left) * 4;
    int idxTR = (top * texWidth + right) * 4;
    int idxBL = (bottom * texWidth + left) * 4;
    int idxBR = (bottom * texWidth + right) * 4;

    outR = (texPixels[idxTL + 0] * one_minus_u + texPixels[idxTR + 0] * u) * one_minus_v
        + (texPixels[idxBL + 0] * one_minus_u + texPixels[idxBR + 0] * u) * v;
    outG = (texPixels[idxTL + 1] * one_minus_u + texPixels[idxTR + 1] * u) * one_minus_v
        + (texPixels[idxBL + 1] * one_minus_u + texPixels[idxBR + 1] * u) * v;
    outB = (texPixels[idxTL + 2] * one_minus_u + texPixels[idxTR + 2] * u) * one_minus_v
        + (texPixels[idxBL + 2] * one_minus_u + texPixels[idxBR + 2] * u) * v;
    outA = (texPixels[idxTL + 3] * one_minus_u + texPixels[idxTR + 3] * u) * one_minus_v
        + (texPixels[idxBL + 3] * one_minus_u + texPixels[idxBR + 3] * u) * v;
}

// ========== 统一采样接口 ==========
__device__ void sampleTexture(float texX, float texY,
    const unsigned char* texPixels, int texWidth, int texHeight,
    TextureSamplingMethod method, int anisoLevel,
    float& outR, float& outG, float& outB, float& outA,
    float dudx = 0.0f, float dudy = 0.0f,
    float dvdx = 0.0f, float dvdy = 0.0f) {

    switch (method) {
    case SAMPLING_NEAREST: {
        int ix = (int)(texX + 0.5f);
        int iy = (int)(texY + 0.5f);
        ix = max(0, min(ix, texWidth - 1));
        iy = max(0, min(iy, texHeight - 1));
        int idx = (iy * texWidth + ix) * 4;
        outR = texPixels[idx + 0];
        outG = texPixels[idx + 1];
        outB = texPixels[idx + 2];
        outA = texPixels[idx + 3];
        break;
    }
    case SAMPLING_BILINEAR: {
        bilinearSample(texX, texY, texPixels, texWidth, texHeight, outR, outG, outB, outA);
        break;
    }
    case SAMPLING_BICUBIC: {
        int i0 = (int)floorf(texX);
        int j0 = (int)floorf(texY);
        float u = texX - i0;
        float v = texY - j0;

        float sumR = 0.0f, sumG = 0.0f, sumB = 0.0f, sumA = 0.0f;
        float totalWeight = 0.0f;

        for (int dy = -1; dy <= 2; ++dy) {
            for (int dx = -1; dx <= 2; ++dx) {
                int ix = i0 + dx;
                int iy = j0 + dy;
                ix = max(0, min(ix, texWidth - 1));
                iy = max(0, min(iy, texHeight - 1));

                float wx = bsplineWeight(dx - u);
                float wy = bsplineWeight(dy - v);
                float w = wx * wy;

                int idx = (iy * texWidth + ix) * 4;
                sumR += texPixels[idx + 0] * w;
                sumG += texPixels[idx + 1] * w;
                sumB += texPixels[idx + 2] * w;
                sumA += texPixels[idx + 3] * w;
                totalWeight += w;
            }
        }

        if (totalWeight > 1e-6f) {
            outR = sumR / totalWeight;
            outG = sumG / totalWeight;
            outB = sumB / totalWeight;
            outA = sumA / totalWeight;
        }
        else {
            outR = outG = outB = outA = 0.0f;
        }
        break;
    }
    case SAMPLING_ANISOTROPIC: {
        // 各向异性采样：沿主方向采样多个点并平均
        int samples = max(1, min(anisoLevel, 16));
        float dir_u, dir_v;
        if (dudx * dudx + dvdx * dvdx >= dudy * dudy + dvdy * dvdy) {
            dir_u = dudx;
            dir_v = dvdx;
        }
        else {
            dir_u = dudy;
            dir_v = dvdy;
        }

        float step_u = (samples > 1) ? dir_u / (samples - 1) : 0.0f;
        float step_v = (samples > 1) ? dir_v / (samples - 1) : 0.0f;
        float start_u = -(samples - 1) * 0.5f * step_u;
        float start_v = -(samples - 1) * 0.5f * step_v;

        float sumR = 0.0f, sumG = 0.0f, sumB = 0.0f, sumA = 0.0f;
        for (int s = 0; s < samples; ++s) {
            float sampleX = texX + start_u + s * step_u;
            float sampleY = texY + start_v + s * step_v;
            float r, g, b, a;
            bilinearSample(sampleX, sampleY, texPixels, texWidth, texHeight, r, g, b, a);
            sumR += r;
            sumG += g;
            sumB += b;
            sumA += a;
        }
        float inv = 1.0f / samples;
        outR = sumR * inv;
        outG = sumG * inv;
        outB = sumB * inv;
        outA = sumA * inv;
        break;
    }
    default:
        outR = outG = outB = outA = 0.0f;
        break;
    }
}

// ========== 通用渲染内核 ==========
__global__ void RasterizeQuadKernel(
    unsigned char* framePixels,
    int width, int height,
    int offsetX, int offsetY,
    Matrix3D inverse_trans,
    Vector3D points[4],
    const unsigned char* texPixels,
    int texWidth, int texHeight,
    TextureSamplingMethod method,
    int anisoLevel) {

    int x = offsetX + blockIdx.x * blockDim.x + threadIdx.x;
    int y = offsetY + blockIdx.y * blockDim.y + threadIdx.y;
    if (x >= width || y >= height) return;

    float px = x + 0.5f;
    float py = y + 0.5f;

    // 将四边形拆为两个三角形进行包含测试
    Vector3D tri1[3] = { points[0], points[1], points[2] };
    Vector3D tri2[3] = { points[0], points[2], points[3] };
    bool inside = isPointInTriangle(px, py,
        tri1[0].x, tri1[0].y, tri1[1].x, tri1[1].y, tri1[2].x, tri1[2].y) ||
        isPointInTriangle(px, py,
            tri2[0].x, tri2[0].y, tri2[1].x, tri2[1].y, tri2[2].x, tri2[2].y);
    if (!inside) return;

    // 计算纹理坐标
    Vector3D P = { px, py, 1 };
    Vector3D texCoord = mulMatrixVector(inverse_trans, P);
    float texX = texCoord.x;
    float texY = texCoord.y;

    // 获取偏导数（用于各向异性）
    float dudx = inverse_trans.m[0][0];
    float dudy = inverse_trans.m[0][1];
    float dvdx = inverse_trans.m[1][0];
    float dvdy = inverse_trans.m[1][1];

    // 采样纹理
    float r, g, b, a;
    sampleTexture(texX, texY, texPixels, texWidth, texHeight,
        method, anisoLevel, r, g, b, a,
        dudx, dudy, dvdx, dvdy);

    // Alpha 混合写入帧缓冲（帧缓冲为 RGB 格式）
    float alpha = a / 255.0f;
    int idx = (y * width + x) * 3;
    framePixels[idx + 0] = (unsigned char)(r * alpha + framePixels[idx + 0] * (1.0f - alpha));
    framePixels[idx + 1] = (unsigned char)(g * alpha + framePixels[idx + 1] * (1.0f - alpha));
    framePixels[idx + 2] = (unsigned char)(b * alpha + framePixels[idx + 2] * (1.0f - alpha));
}

// ========== 主机端函数 ==========
extern "C" void Rasterize_An_Object(Frame& frame, const RenderData& obj,
    TextureSamplingMethod method, int anisoLevel, int MSAA) {
    const int width = frame.width;
    const int height = frame.height;
    const Vector3D* p = obj.points;

    // 计算 AABB 确定 ROI
    float minX = min(p[0].x, min(p[1].x, min(p[2].x, p[3].x)));
    float maxX = max(p[0].x, max(p[1].x, max(p[2].x, p[3].x)));
    float minY = min(p[0].y, min(p[1].y, min(p[2].y, p[3].y)));
    float maxY = max(p[0].y, max(p[1].y, max(p[2].y, p[3].y)));

    int xStart = max(0, (int)floor(minX));
    int xEnd = min(width - 1, (int)ceil(maxX));
    int yStart = max(0, (int)floor(minY));
    int yEnd = min(height - 1, (int)ceil(maxY));
    int roiWidth = xEnd - xStart + 1;
    int roiHeight = yEnd - yStart + 1;

    if (roiWidth <= 0 || roiHeight <= 0) return;

    dim3 blockSize(16, 16);
    dim3 gridSize((roiWidth + blockSize.x - 1) / blockSize.x,
        (roiHeight + blockSize.y - 1) / blockSize.y);

    // 分配设备内存
    unsigned char* d_frame = nullptr;
    size_t frameBytes = frame.pixels.size() * sizeof(StdPixel);
    cudaMalloc(&d_frame, frameBytes);
    cudaMemcpy(d_frame, frame.pixels.data(), frameBytes, cudaMemcpyHostToDevice);

    const unsigned char* d_tex = nullptr;
    size_t texBytes = obj.texture.pixels.size() * sizeof(BMP_Pixel);
    cudaMalloc((void**)&d_tex, texBytes);
    cudaMemcpy((void*)d_tex, obj.texture.pixels.data(), texBytes, cudaMemcpyHostToDevice);

    Vector3D* d_points = nullptr;
    cudaMalloc(&d_points, 4 * sizeof(Vector3D));
    cudaMemcpy(d_points, obj.points, 4 * sizeof(Vector3D), cudaMemcpyHostToDevice);

    // 启动统一内核
    RasterizeQuadKernel << <gridSize, blockSize >> > (
        d_frame, width, height,
        xStart, yStart,
        obj.inverse_trans,
        d_points,
        d_tex,
        obj.texture.width, obj.texture.height,
        method, anisoLevel);

    cudaDeviceSynchronize();
    cudaMemcpy(frame.pixels.data(), d_frame, frameBytes, cudaMemcpyDeviceToHost);

    cudaFree(d_frame);
    cudaFree((void*)d_tex);
    cudaFree(d_points);
}