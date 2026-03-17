// Copyright 2026 MrSeagull. All Rights Reserved.
#pragma once
#include <string>
#include <unordered_map>
#include <optional>
#include <vector>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

// 坐标与尺寸辅助结构
struct Location3D {
	float x = 0.0f;
	float y = 0.0f;
	int z = 0;
};

struct Location2D {
	float x = 0.0f;
	float y = 0.0f;
};

struct Location2DInt {
	int x = 0;
	int y = 0;
};

struct Rotation {
	float r = 0.0f;
};

struct Scale2D {
	float x = 1.0f;
	float y = 1.0f;
};

struct Size2D {
	float x = 0.0f;   // <=0 表示使用原始图片大小
	float y = 0.0f;
};

struct Size2DInt {
	int x = 0;         // <=0 表示使用默认大小
	int y = 0;
};

// 组件结构
struct TransformComponent {
	Location3D Location;
	Rotation Rotation;
	Scale2D Scale;
};

struct PictureComponent {
	std::string Path;
	Location3D Location;
	Rotation Rotation;
	Size2D Size;
};

struct TextblockComponent {
	struct TextInfo {
		std::string component;
		int Font_size = 0;      // <=0 表示默认大小
		bool ANSI_Print = false;
	};
	Location2DInt Location;
	Size2DInt Size;
	TextInfo Text;
	Scale2D Scale;
};

struct TriggerAreaComponent {
	Location2D Location;
	Size2D Size;                // <=0 视为0.0（文档要求）
};

struct BlueprintComponent {
	std::string Path;
};

struct SetComponent {
	std::vector<std::string> Sub_objects;
};

// 对象结构（包含所有可选组件）
struct ObjectData {
	std::optional<TransformComponent> Transform;
	std::optional<PictureComponent> Picture;
	std::optional<TextblockComponent> Textblock;
	std::optional<TriggerAreaComponent> TriggerArea;
	std::optional<BlueprintComponent> Blueprint;
	std::optional<SetComponent> Set;
};

// 场景结构
struct LevelData {
	std::string name;                                    // 场景名（JSON顶层键）
	std::unordered_map<std::string, ObjectData> objects; // 对象名 -> 对象数据
};

// 读写函数声明
void to_json(json& j, const Location3D& v);
void from_json(const json& j, Location3D& v);
void to_json(json& j, const Location2D& v);
void from_json(const json& j, Location2D& v);
void to_json(json& j, const Location2DInt& v);
void from_json(const json& j, Location2DInt& v);
void to_json(json& j, const Rotation& v);
void from_json(const json& j, Rotation& v);
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
void to_json(json& j, const SetComponent& v);
void from_json(const json& j, SetComponent& v);
void to_json(json& j, const ObjectData& v);
void from_json(const json& j, ObjectData& v);
void to_json(json& j, const LevelData& v);
void from_json(const json& j, LevelData& v);

// 文件读写接口
bool WriteLevelData(const std::string& filepath, const LevelData& data);
LevelData ReadLevelData(const std::string& filepath);