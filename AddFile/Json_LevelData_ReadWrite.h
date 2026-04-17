// Copyright 2026 MrSeagull. All Rights Reserved.
#pragma once
#include <string>
#include <unordered_map>
#include <optional>
#include <vector>
#include "nlohmann/json.hpp"
#include "SharedTypes.h"

using json = nlohmann::json;

// 读写函数声明
void to_json(json& j, const Location3D& v);
void from_json(const json& j, Location3D& v);
void to_json(json& j, const Location2D& v);
void from_json(const json& j, Location2D& v);
void to_json(json& j, const Location2DInt& v);
void from_json(const json& j, Location2DInt& v);
void to_json(json& j, const RotationEuler& v);
void from_json(const json& j, RotationEuler& v);
void to_json(json& j, const Scale2D& v);
void from_json(const json& j, Scale2D& v);
void to_json(json& j, const Size2D& v);
void from_json(const json& j, Size2D& v);
void to_json(json& j, const Size2DInt& v);
void from_json(const json& j, Size2DInt& v);
void to_json(json& j, const TransformComponent& v);
void from_json(const json& j, TransformComponent& v);
void to_json(json& j, const PictureComponent& v);
void from_json(const json& j, PictureComponent& v);
void to_json(json& j, const TextblockComponent::TextInfo& v);
void from_json(const json& j, TextblockComponent::TextInfo& v);
void to_json(json& j, const TextblockComponent& v);
void from_json(const json& j, TextblockComponent& v);
void to_json(json& j, const TriggerAreaComponent& v);
void from_json(const json& j, TriggerAreaComponent& v);
void to_json(json& j, const BlueprintComponent& v);
void from_json(const json& j, BlueprintComponent& v);
void to_json(json& j, const ObjectData& v);
void from_json(const json& j, ObjectData& v);
void to_json(json& j, const LevelData& v);
void from_json(const json& j, LevelData& v);

// 文件读写接口
bool WriteLevelData(const std::string& filepath, const LevelData& data);
LevelData ReadLevelData(const std::string& filepath);