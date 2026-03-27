// Copyright 2026 MrSeagull. All Rights Reserved.
#include "RenderingSystem.h"

// 辅助函数：根据 TransformComponent 构造局部变换矩阵（3x3 齐次矩阵，支持平移、旋转、缩放）
// 具体实现由用户完成，此处仅提供接口
static Matrix3D ComputeLocalMatrix(const TransformComponent& transform) {
    Matrix3D mat; // 应返回单位矩阵，待实现
    // TODO: 根据 transform.Location (x,y), transform.Rotation.r (角度), transform.Scale (x,y) 构造矩阵
    // 建议矩阵形式为 [sx*cosθ, -sy*sinθ, tx; sx*sinθ, sy*cosθ, ty; 0, 0, 1]
    return mat;
}

// 递归遍历对象，计算世界矩阵
static void TraverseObject(
    ObjectData* obj,
    const Matrix3D& parentWorld,
    std::vector<RenderData>& outRenderObjects)
{
    if (!obj) return;

    // 1. 计算当前对象的世界矩阵
    Matrix3D world = parentWorld;
    if (obj->Transform.has_value()) {
        Matrix3D local = ComputeLocalMatrix(obj->Transform.value());
        // 矩阵乘法：world = parentWorld * local （取决于坐标系的定义，通常子级变换左乘父级）
        // 需要根据实际矩阵乘法实现，此处假设重载了 * 或使用函数
        // world = parentWorld * local;   // 待实现
    }

    // 2. 如果对象含有 Picture 组件，生成渲染数据
    if (obj->Picture.has_value()) {
        RenderData renderData;
        renderData.trans = world;               // 世界变换矩阵
        // renderData.inverse_trans = world.Inverse(); // 逆矩阵，后续计算
        // 从 Picture 组件获取纹理路径、局部偏移、尺寸等信息
        const auto& pic = obj->Picture.value();
        // TODO: 加载图片数据到 renderData.texture（BMP_Data）
        // TODO: 根据 pic.Location 和 pic.Size 计算四个角点（局部坐标），再通过世界矩阵转换到世界坐标
        // 此处简化：points 留待后续计算
        // renderData.depth = pic.Location.z;    // 深度用于排序
        outRenderObjects.push_back(renderData);
    }

    // 3. 递归处理子对象
    for (auto& [childName, childObj] : obj->objects) {
        TraverseObject(childObj, world, outRenderObjects);
    }
}

void RenderingSystem::RefreshRenderObjects(const LevelData& currentLevel) {
    RenderObjects.clear();

    // 场景根对象：所有直接属于 LevelData 的对象，其父对象为 nullptr
    Matrix3D identity; // 单位矩阵，需确保默认构造为单位阵，当前 Matrix3D 默认构造为 0，需要修正
    // 暂时手动设置为单位矩阵
    // identity.m[0][0] = 1; identity.m[1][1] = 1; identity.m[2][2] = 1;  // 待实现

    for (const auto& [objName, objPtr] : currentLevel.objects) {
        // 场景根对象的父指针为 nullptr
        if (objPtr->parent == nullptr) {
            TraverseObject(objPtr, identity, RenderObjects);
        }
    }
}