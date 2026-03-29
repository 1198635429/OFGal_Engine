// Copyright 2026 MrSeagull. All Rights Reserved.
#include "RenderingSystem.h"
#include <numbers>
#include <filesystem>
constexpr double pi = std::numbers::pi;
static Matrix3D ComputeLocalMatrix(const TransformComponent& transform) {
    Matrix3D mat; // 默认构造单位矩阵
    float rad = transform.Rotation.r * pi / 180.0f;
    mat[0][0] = transform.Scale.x * cos(rad);
    mat[0][1] = -transform.Scale.y * sin(rad);
    mat[0][2] = transform.Location.x;
    mat[1][0] = transform.Scale.x * sin(rad);
    mat[1][1] = transform.Scale.y * cos(rad);
    mat[1][2] = transform.Location.y;
    return mat;
}

// 递归遍历对象，计算世界矩阵
static void TraverseObject(
    ObjectData* obj,
    const Matrix3D& parentWorld,
    std::vector<int> parentDepth,
    std::vector<RenderData>& outRenderObjects)
{
    if (!obj) return;

    // 计算当前对象的世界矩阵和累积深度
    Matrix3D world = parentWorld;
    std::vector<int> depth = parentDepth;

    if (obj->Transform.has_value()) {
        const auto& trans = obj->Transform.value();
        Matrix3D local = ComputeLocalMatrix(trans);
        world = parentWorld * local;
        depth.push_back(trans.Location.z);   // 累积深度

        // 如果对象含有 Picture 组件，生成渲染数据
        if (obj->Picture.has_value() && std::filesystem::exists(obj->Picture->Path)) {
            const auto& pic = obj->Picture.value();

            // 1. 加载纹理
            BMP_Data texture = _EventBus::getInstance().publish_ReadBMP(pic.Path.c_str());

            // 2. 确定实际渲染尺寸（原始纹理尺寸 vs 指定视窗尺寸）
            int imgWidth = texture.width;
            int imgHeight = texture.height;
            if (pic.Size.x > 0) imgWidth = static_cast<int>(pic.Size.x);
            if (pic.Size.y > 0) imgHeight = static_cast<int>(pic.Size.y);

            // 3. 图片局部坐标系中的四个角点（单位矩形，未应用 Picture 的变换）
            Vector3D localPoints[4] = {
                {0.0f, 0.0f, 1.0f},
                {static_cast<float>(imgWidth - 1), 0.0f, 1.0f},
                {static_cast<float>(imgWidth - 1), static_cast<float>(imgHeight - 1), 1.0f},
                {0.0f, static_cast<float>(imgHeight - 1), 1.0f}
            };

            // 4. 为 Picture 组件构建局部变换矩阵（先缩放/旋转/平移）
            float rad = pic.Rotation.r * pi / 180.0f;
            Matrix3D picLocal;
            picLocal[0][0] = cos(rad);   // 旋转（假设无缩放）
            picLocal[0][1] = -sin(rad);
            picLocal[1][0] = sin(rad);
            picLocal[1][1] = cos(rad);
            picLocal[0][2] = pic.Location.x;
            picLocal[1][2] = pic.Location.y;

            // 5. 组合变换：世界变换 × Picture 局部变换
            Matrix3D finalTransform = world * picLocal;

            // 6. 生成 RenderData
            RenderData renderData;
            renderData.trans = finalTransform;
            renderData.depth = depth;
            renderData.texture = std::move(texture);

            // 变换四个角点到世界坐标
            for (int i = 0; i < 4; ++i) {
                renderData.points[i] = finalTransform * localPoints[i];
            }

            renderData.inverse_trans = finalTransform.inverse();
            outRenderObjects.push_back(std::move(renderData));
        }
    }
    // 递归子对象
    for (auto& [childName, childObj] : obj->objects) {
        TraverseObject(childObj, world, depth, outRenderObjects);
    }
}

void RenderingSystem::RefreshRenderObjects(const LevelData & currentLevel) {
    RenderObjects.clear();
    Matrix3D identity;
    for (const auto& [objName, objPtr] : currentLevel.objects) {
        if (objPtr->parent == nullptr) {
            std::vector<int> defaultDepth;
            TraverseObject(objPtr, identity, defaultDepth, RenderObjects);
        }
    }
}

void RenderingSystem::AABB_Remove(std::vector<RenderData>& renderObjects) {
    for (auto it = renderObjects.begin(); it != renderObjects.end(); ) {
        const auto& points = it->points;
        int x_min = std::min({ points[0].x, points[1].x, points[2].x, points[3].x });
        int x_max = std::max({ points[0].x, points[1].x, points[2].x, points[3].x });
        int y_min = std::min({ points[0].y, points[1].y, points[2].y, points[3].y });
        int y_max = std::max({ points[0].y, points[1].y, points[2].y, points[3].y });

        // 若完全在视口外
        if (x_max < 0 || x_min >= CanvasSize.x || y_max < 0 || y_min >= CanvasSize.y)
            it = renderObjects.erase(it); // erase 返回下一个元素的迭代器
        else
            ++it;
    }
}

void RenderingSystem::SortByDepth(std::vector<RenderData>& renderObjects) {
    std::sort(renderObjects.begin(), renderObjects.end(),
        [](const RenderData& a, const RenderData& b) -> bool {
            const auto& da = a.depth;
            const auto& db = b.depth;
            size_t minSize = std::min(da.size(), db.size());
            for (size_t i = 0; i < minSize; ++i) {
                if (da[i] != db[i]) {
                    return da[i] > db[i];   // 降序：更深的在前
                }
            }
            // 所有公共前缀相等，则较长的数组更深，应排在前面
            return da.size() > db.size();
        });
}