// Copyright 2026 MrSeagull. All Rights Reserved.
#include "SRP.cuh"
#include <cuda_runtime.h>
#include <cmath>
#include <algorithm>

// 设备端：矩阵乘向量（3x3 * 3x1）
__device__ Vector3D mulMatrixVector(const Matrix3D& mat, const Vector3D& vec) {
    Vector3D result;
    result.x = mat.m[0][0] * vec.x + mat.m[0][1] * vec.y + mat.m[0][2] * vec.z;
    result.y = mat.m[1][0] * vec.x + mat.m[1][1] * vec.y + mat.m[1][2] * vec.z;
    result.z = mat.m[2][0] * vec.x + mat.m[2][1] * vec.y + mat.m[2][2] * vec.z;
    return result;
}
// 三次卷积核权重计算 (a = -0.5)，可能会产生过冲和振铃伪影
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
// B-spline 三次卷积核 (a=1, 无负瓣)，完全无伪影，但边缘相对更模糊
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

__device__ bool isPointInTriangle(float px, float py,
    float ax, float ay,
    float bx, float by,
    float cx, float cy) {

    float abx = bx - ax, aby = by - ay;
    float apx = px - ax, apy = py - ay;
    float cross1 = abx * apy - aby * apx;

    float bcx = cx - bx, bcy = cy - by;
    float bpx = px - bx, bpy = py - by;
    float cross2 = bcx * bpy - bcy * bpx;

    float cax = ax - cx, cay = ay - cy;
    float cpx = px - cx, cpy = py - cy;
    float cross3 = cax * cpy - cay * cpx;

    // 允许边界上的点（包含零）
    bool hasNeg = (cross1 < 0) || (cross2 < 0) || (cross3 < 0);
    bool hasPos = (cross1 > 0) || (cross2 > 0) || (cross3 > 0);
    return !(hasNeg && hasPos);
}

// 内核函数：并行处理四边形覆盖的像素
// 1.最近邻采样
__global__ void Nearest_RasterizeQuadKernel(
    unsigned char* framePixels,
    int width, int height,
    int offsetX, int offsetY,
    Matrix3D inverse_trans,
    Vector3D points[4],
    const unsigned char* texPixels,
    int texWidth, int texHeight,
    int msaaMultiple)
{
    int x = offsetX + blockIdx.x * blockDim.x + threadIdx.x;
    int y = offsetY + blockIdx.y * blockDim.y + threadIdx.y;
    if (x >= width || y >= height) return;

    float px = x + 0.5f;
    float py = y + 0.5f;

    // 将四边形拆成两个三角形： (0,1,2) 和 (0,2,3)
    Vector3D tri1[3] = { points[0], points[1], points[2] };
    Vector3D tri2[3] = { points[0], points[2], points[3] };

    // 判断点是否在任一三角形内
    bool inside = isPointInTriangle(px, py,
        tri1[0].x, tri1[0].y,
        tri1[1].x, tri1[1].y,
        tri1[2].x, tri1[2].y) ||
        isPointInTriangle(px, py,
            tri2[0].x, tri2[0].y,
            tri2[1].x, tri2[1].y,
            tri2[2].x, tri2[2].y);

    if (inside) {
        Vector3D P = { px, py, 1 };
        Vector3D TargetPixel = mulMatrixVector(inverse_trans, P);

        int i = (int)TargetPixel.x;
        int j = (int)TargetPixel.y;

        i = max(0, min(i, texWidth - 1));
        j = max(0, min(j, texHeight - 1));

        unsigned char r = texPixels[(j * texWidth + i) * 4 + 0];
        unsigned char g = texPixels[(j * texWidth + i) * 4 + 1];
        unsigned char b = texPixels[(j * texWidth + i) * 4 + 2];

        // 正确写入帧缓冲
        int idx = (y * width + x) * 3;
        float alpha_multiple = float(texPixels[(j * texWidth + i) * 4 + 3]) / 255.0f;
        framePixels[idx + 0] = r * alpha_multiple + framePixels[idx + 0] * (1.0f - alpha_multiple);
        framePixels[idx + 1] = g * alpha_multiple + framePixels[idx + 1] * (1.0f - alpha_multiple);
        framePixels[idx + 2] = b * alpha_multiple + framePixels[idx + 2] * (1.0f - alpha_multiple);
    }
}
// 2.双线性插值
__global__ void Bilinear_RasterizeQuadKernel(
    unsigned char* framePixels,
    int width, int height,
    int offsetX, int offsetY,
    Matrix3D inverse_trans,
    Vector3D points[4],
    const unsigned char* texPixels,
    int texWidth, int texHeight,
    int msaaMultiple)
{
    int x = offsetX + blockIdx.x * blockDim.x + threadIdx.x;
    int y = offsetY + blockIdx.y * blockDim.y + threadIdx.y;
    if (x >= width || y >= height) return;

    float px = x + 0.5f;
    float py = y + 0.5f;

    // 将四边形拆成两个三角形
    Vector3D tri1[3] = { points[0], points[1], points[2] };
    Vector3D tri2[3] = { points[0], points[2], points[3] };

    bool inside = isPointInTriangle(px, py,
        tri1[0].x, tri1[0].y,
        tri1[1].x, tri1[1].y,
        tri1[2].x, tri1[2].y) ||
        isPointInTriangle(px, py,
            tri2[0].x, tri2[0].y,
            tri2[1].x, tri2[1].y,
            tri2[2].x, tri2[2].y);

    if (inside) {
        Vector3D P = { px, py, 1 };
        Vector3D TargetPixel = mulMatrixVector(inverse_trans, P);

        float i = TargetPixel.x;
        float j = TargetPixel.y;

        int left = int(i);
        int right = int(i + 1);
        int top = int(j);
        int bottom = int(j + 1);

        left = max(0, min(left, texWidth - 1));
        right = max(0, min(right, texWidth - 1));
        top = max(0, min(top, texHeight - 1));
        bottom = max(0, min(bottom, texHeight - 1));

        float u = i - left;          // 水平方向小数部分
        float v = j - top;           // 垂直方向小数部分
        float one_minus_u = 1.0f - u;
        float one_minus_v = 1.0f - v;

        // 获取四个纹素的指针（简化索引计算）
        int idxTL = (top * texWidth + left) * 4;
        int idxTR = (top * texWidth + right) * 4;
        int idxBL = (bottom * texWidth + left) * 4;
        int idxBR = (bottom * texWidth + right) * 4;

        // 双线性插值：R,G,B
        unsigned char r = (unsigned char)(
            (texPixels[idxTL + 0] * one_minus_u + texPixels[idxTR + 0] * u) * one_minus_v +
            (texPixels[idxBL + 0] * one_minus_u + texPixels[idxBR + 0] * u) * v
            );
        unsigned char g = (unsigned char)(
            (texPixels[idxTL + 1] * one_minus_u + texPixels[idxTR + 1] * u) * one_minus_v +
            (texPixels[idxBL + 1] * one_minus_u + texPixels[idxBR + 1] * u) * v
            );
        unsigned char b = (unsigned char)(
            (texPixels[idxTL + 2] * one_minus_u + texPixels[idxTR + 2] * u) * one_minus_v +
            (texPixels[idxBL + 2] * one_minus_u + texPixels[idxBR + 2] * u) * v
            );

        // Alpha 插值并归一化
        float alpha = (
            (texPixels[idxTL + 3] * one_minus_u + texPixels[idxTR + 3] * u) * one_minus_v +
            (texPixels[idxBL + 3] * one_minus_u + texPixels[idxBR + 3] * u) * v
            ) / 255.0f;

        // Alpha 混合写入帧缓冲
        int idx = (y * width + x) * 3;
        framePixels[idx + 0] = r * alpha + framePixels[idx + 0] * (1.0f - alpha);
        framePixels[idx + 1] = g * alpha + framePixels[idx + 1] * (1.0f - alpha);
        framePixels[idx + 2] = b * alpha + framePixels[idx + 2] * (1.0f - alpha);
    }
}
// 3.双三性插值采样
__global__ void Bicubic_RasterizeQuadKernel(
    unsigned char* framePixels,
    int width, int height,
    int offsetX, int offsetY,
    Matrix3D inverse_trans,
    Vector3D points[4],
    const unsigned char* texPixels,
    int texWidth, int texHeight,
    int msaaMultiple)
{
    int x = offsetX + blockIdx.x * blockDim.x + threadIdx.x;
    int y = offsetY + blockIdx.y * blockDim.y + threadIdx.y;
    if (x >= width || y >= height) return;

    float px = x + 0.5f;
    float py = y + 0.5f;

    // 将四边形拆成两个三角形
    Vector3D tri1[3] = { points[0], points[1], points[2] };
    Vector3D tri2[3] = { points[0], points[2], points[3] };

    bool inside = isPointInTriangle(px, py,
        tri1[0].x, tri1[0].y,
        tri1[1].x, tri1[1].y,
        tri1[2].x, tri1[2].y) ||
        isPointInTriangle(px, py,
            tri2[0].x, tri2[0].y,
            tri2[1].x, tri2[1].y,
            tri2[2].x, tri2[2].y);

    if (inside) {
        Vector3D P = { px, py, 1 };
        Vector3D TargetPixel = mulMatrixVector(inverse_trans, P);

        float i = TargetPixel.x;   // 纹理坐标 x
        float j = TargetPixel.y;   // 纹理坐标 y

        // 获取整数部分和小数部分
        int i0 = (int)floorf(i);
        int j0 = (int)floorf(j);
        float u = i - i0;   // 水平方向小数部分
        float v = j - j0;   // 垂直方向小数部分

        // 累加器
        float sumR = 0.0f, sumG = 0.0f, sumB = 0.0f, sumA = 0.0f;
        float totalWeight = 0.0f;

        // 遍历周围 4x4 邻域
        for (int dy = -1; dy <= 2; ++dy) {
            for (int dx = -1; dx <= 2; ++dx) {
                // 纹理坐标整数索引
                int ix = i0 + dx;
                int iy = j0 + dy;

                // 边界处理：clamp 到有效范围
                ix = max(0, min(ix, texWidth - 1));
                iy = max(0, min(iy, texHeight - 1));

                // 计算权重：水平权重 * 垂直权重
                float wx = bsplineWeight(dx - u);   // 注意：dx - u 范围 [-1-u, 2-u] 约 [-1.5,2.5]
                float wy = bsplineWeight(dy - v);
                float w = wx * wy;

                // 获取纹素颜色
                int idx = (iy * texWidth + ix) * 4;
                unsigned char r = texPixels[idx + 0];
                unsigned char g = texPixels[idx + 1];
                unsigned char b = texPixels[idx + 2];
                unsigned char a = texPixels[idx + 3];

                sumR += r * w;
                sumG += g * w;
                sumB += b * w;
                sumA += a * w;
                totalWeight += w;
            }
        }

        // 归一化（避免边界处权重和不为1）
        if (totalWeight > 1e-6f) {
            sumR /= totalWeight;
            sumG /= totalWeight;
            sumB /= totalWeight;
            sumA /= totalWeight;
        }

        // 转换为 unsigned char
        unsigned char r = (unsigned char)(sumR + 0.5f);
        unsigned char g = (unsigned char)(sumG + 0.5f);
        unsigned char b = (unsigned char)(sumB + 0.5f);
        float alpha = sumA / 255.0f;   // 归一化

        // Alpha 混合写入帧缓冲
        int idx = (y * width + x) * 3;
        framePixels[idx + 0] = r * alpha + framePixels[idx + 0] * (1.0f - alpha);
        framePixels[idx + 1] = g * alpha + framePixels[idx + 1] * (1.0f - alpha);
        framePixels[idx + 2] = b * alpha + framePixels[idx + 2] * (1.0f - alpha);
    }
}
// 4.各向异性过滤
// 辅助函数：双线性采样一个点（带边界clamp）
__device__ void bilinearSample(
    float i, float j,
    const unsigned char* texPixels,
    int texWidth, int texHeight,
    float& outR, float& outG, float& outB, float& outA)
{
    // 边界clamp
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

// 各向异性过滤的主kernel
__global__ void Anisotropic_RasterizeQuadKernel(
    unsigned char* framePixels,
    int width, int height,
    int offsetX, int offsetY,
    Matrix3D inverse_trans,
    Vector3D points[4],
    const unsigned char* texPixels,
    int texWidth, int texHeight,
    int msaaMultiple,
    int anisoLevel)          // 各向异性倍数（例如 2, 4, 8, 16）
{
    int x = offsetX + blockIdx.x * blockDim.x + threadIdx.x;
    int y = offsetY + blockIdx.y * blockDim.y + threadIdx.y;
    if (x >= width || y >= height) return;

    float px = x + 0.5f;
    float py = y + 0.5f;

    // 三角形包含测试（同双线性）
    Vector3D tri1[3] = { points[0], points[1], points[2] };
    Vector3D tri2[3] = { points[0], points[2], points[3] };
    bool inside = isPointInTriangle(px, py,
        tri1[0].x, tri1[0].y,
        tri1[1].x, tri1[1].y,
        tri1[2].x, tri1[2].y) ||
        isPointInTriangle(px, py,
            tri2[0].x, tri2[0].y,
            tri2[1].x, tri2[1].y,
            tri2[2].x, tri2[2].y);
    if (!inside) return;

    // 计算纹理坐标
    Vector3D P = { px, py, 1 };
    Vector3D TargetPixel = mulMatrixVector(inverse_trans, P);
    float i = TargetPixel.x;
    float j = TargetPixel.y;

    // 从逆变换矩阵中提取纹理坐标对屏幕坐标的偏导数
    float dudx = inverse_trans.m[0][0];
    float dudy = inverse_trans.m[0][1];
    float dvdx = inverse_trans.m[1][0];
    float dvdy = inverse_trans.m[1][1];

    // 计算两个梯度向量的长度平方
    float lenX2 = dudx * dudx + dvdx * dvdx;
    float lenY2 = dudy * dudy + dvdy * dvdy;

    // 选择主方向（梯度较大的方向）
    float dir_u, dir_v;
    if (lenX2 >= lenY2) {
        dir_u = dudx;
        dir_v = dvdx;
    }
    else {
        dir_u = dudy;
        dir_v = dvdy;
    }

    // 采样数至少为1，不超过最大倍数（避免过多循环）
    int samples = max(1, min(anisoLevel, 16));
    float step_u, step_v;
    if (samples > 1) {
        step_u = dir_u / (samples - 1);
        step_v = dir_v / (samples - 1);
    }
    else {
        step_u = 0.0f;
        step_v = 0.0f;
    }

    // 起始偏移量：使采样范围以原始点为中心
    float start_u = -(samples - 1) * 0.5f * step_u;
    float start_v = -(samples - 1) * 0.5f * step_v;

    float sumR = 0.0f, sumG = 0.0f, sumB = 0.0f, sumA = 0.0f;
    for (int s = 0; s < samples; ++s) {
        float sample_i = i + start_u + s * step_u;
        float sample_j = j + start_v + s * step_v;
        float r, g, b, a;
        bilinearSample(sample_i, sample_j, texPixels, texWidth, texHeight, r, g, b, a);
        sumR += r;
        sumG += g;
        sumB += b;
        sumA += a;
    }

    // 取平均
    float invSamples = 1.0f / samples;
    unsigned char r = (unsigned char)(sumR * invSamples + 0.5f);
    unsigned char g = (unsigned char)(sumG * invSamples + 0.5f);
    unsigned char b = (unsigned char)(sumB * invSamples + 0.5f);
    float alpha = (sumA * invSamples) / 255.0f;

    // Alpha混合写入帧缓冲
    int idx = (y * width + x) * 3;
    framePixels[idx + 0] = r * alpha + framePixels[idx + 0] * (1.0f - alpha);
    framePixels[idx + 1] = g * alpha + framePixels[idx + 1] * (1.0f - alpha);
    framePixels[idx + 2] = b * alpha + framePixels[idx + 2] * (1.0f - alpha);
}

extern "C" void Rasterize_An_Object(Frame& frame, const RenderData& obj, const int& MSAA_Multiple) {
    const int width = frame.width;
    const int height = frame.height;

    const Vector3D* p = obj.points;  // 主机端顶点指针

    // 计算 AABB
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

    if (roiWidth <= 0 || roiHeight <= 0) return;  // 无有效像素

    dim3 blockSize(16, 16);
    dim3 gridSize((roiWidth + blockSize.x - 1) / blockSize.x,
        (roiHeight + blockSize.y - 1) / blockSize.y);

    // ----- 分配设备内存并拷贝数据 -----
    // 1. 帧缓冲
    unsigned char* d_frame = nullptr;
    size_t frameBytes = frame.pixels.size() * sizeof(StdPixel);
    cudaMalloc(&d_frame, frameBytes);
    cudaMemcpy(d_frame, frame.pixels.data(), frameBytes, cudaMemcpyHostToDevice);

    // 2. 纹理数据 (注意 const 处理)
    const unsigned char* d_tex = nullptr;
    size_t texBytes = obj.texture.pixels.size() * sizeof(BMP_Pixel);
    cudaMalloc((void**)&d_tex, texBytes);
    cudaMemcpy((void*)d_tex, obj.texture.pixels.data(), texBytes, cudaMemcpyHostToDevice);

    // 3. 顶点数组 (4个 Vector3D)
    Vector3D* d_points = nullptr;
    size_t pointsBytes = 4 * sizeof(Vector3D);
    cudaMalloc(&d_points, pointsBytes);
    cudaMemcpy(d_points, obj.points, pointsBytes, cudaMemcpyHostToDevice);

    // ----- 启动内核 -----
    Bicubic_RasterizeQuadKernel <<< gridSize, blockSize >>> (
        d_frame, width, height,
        xStart, yStart,
        obj.inverse_trans,
        d_points,
        d_tex,
        obj.texture.width, obj.texture.height,
        MSAA_Multiple
        );

    // ----- 错误检查 -----
    cudaError_t err = cudaGetLastError();
    // 检查1
    cudaDeviceSynchronize();
    err = cudaDeviceSynchronize();
    // 检查2

    // ----- 拷贝结果回主机 -----
    cudaMemcpy(frame.pixels.data(), d_frame, frameBytes, cudaMemcpyDeviceToHost);

    // ----- 释放设备内存 -----
    cudaFree(d_frame);
    cudaFree((void*)d_tex);
    cudaFree(d_points);
}