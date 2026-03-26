// Copyright 2026 MrSeagull. All Rights Reserved.
#include "Json_LevelData_ReadWrite.h"
#include <fstream>
#include <filesystem>
#include <stdexcept>
#include "FileSystem.h"

namespace fs = std::filesystem;

// ========== 辅助结构序列化 ==========
void to_json(json& j, const Location3D& v) {
	j = json{ {"x", v.x}, {"y", v.y}, {"z", v.z} };
}
void from_json(const json& j, Location3D& v) {
	j.at("x").get_to(v.x);
	j.at("y").get_to(v.y);
	j.at("z").get_to(v.z);
}

void to_json(json& j, const Location2D& v) {
	j = json{ {"x", v.x}, {"y", v.y} };
}
void from_json(const json& j, Location2D& v) {
	j.at("x").get_to(v.x);
	j.at("y").get_to(v.y);
}

void to_json(json& j, const Location2DInt& v) {
	j = json{ {"x", v.x}, {"y", v.y} };
}
void from_json(const json& j, Location2DInt& v) {
	j.at("x").get_to(v.x);
	j.at("y").get_to(v.y);
}

void to_json(json& j, const Rotation& v) {
	j = json{ {"r", v.r} };
}
void from_json(const json& j, Rotation& v) {
	j.at("r").get_to(v.r);
}

void to_json(json& j, const Scale2D& v) {
	j = json{ {"x", v.x}, {"y", v.y} };
}
void from_json(const json& j, Scale2D& v) {
	j.at("x").get_to(v.x);
	j.at("y").get_to(v.y);
	// 文档：<=0 视为1.0，但这里保留原始值，使用时处理
}

void to_json(json& j, const Size2D& v) {
	j = json{ {"x", v.x}, {"y", v.y} };
}
void from_json(const json& j, Size2D& v) {
	j.at("x").get_to(v.x);
	j.at("y").get_to(v.y);
}

void to_json(json& j, const Size2DInt& v) {
	j = json{ {"x", v.x}, {"y", v.y} };
}
void from_json(const json& j, Size2DInt& v) {
	j.at("x").get_to(v.x);
	j.at("y").get_to(v.y);
}

// ========== 组件序列化 ==========
void to_json(json& j, const TransformComponent& v) {
	j = json{ {"Location", v.Location}, {"Rotation", v.Rotation}, {"Scale", v.Scale} };
}
void from_json(const json& j, TransformComponent& v) {
	j.at("Location").get_to(v.Location);
	j.at("Rotation").get_to(v.Rotation);
	j.at("Scale").get_to(v.Scale);
}

void to_json(json& j, const PictureComponent& v) {
	j = json{ {"Path", v.Path}, {"Location", v.Location}, {"Rotation", v.Rotation}, {"Size", v.Size} };
}
void from_json(const json& j, PictureComponent& v) {
	j.at("Path").get_to(v.Path);
	j.at("Location").get_to(v.Location);
	j.at("Rotation").get_to(v.Rotation);
	j.at("Size").get_to(v.Size);
}

void to_json(json& j, const TextblockComponent::TextInfo& v) {
	j = json{ {"component", v.component}, {"Font size", v.Font_size}, {"ANSI Print", v.ANSI_Print} };
}
void from_json(const json& j, TextblockComponent::TextInfo& v) {
	j.at("component").get_to(v.component);
	j.at("Font size").get_to(v.Font_size);
	j.at("ANSI Print").get_to(v.ANSI_Print);
}

void to_json(json& j, const TextblockComponent& v) {
	j = json{ {"Location", v.Location}, {"Size", v.Size}, {"Text", v.Text}, {"Scale", v.Scale} };
}
void from_json(const json& j, TextblockComponent& v) {
	j.at("Location").get_to(v.Location);
	j.at("Size").get_to(v.Size);
	j.at("Text").get_to(v.Text);
	j.at("Scale").get_to(v.Scale);
}

void to_json(json& j, const TriggerAreaComponent& v) {
	j = json{ {"Location", v.Location}, {"Size", v.Size} };
}
void from_json(const json& j, TriggerAreaComponent& v) {
	j.at("Location").get_to(v.Location);
	j.at("Size").get_to(v.Size);
}

void to_json(json& j, const BlueprintComponent& v) {
	j = json{ {"Path", v.Path} };
}
void from_json(const json& j, BlueprintComponent& v) {
	j.at("Path").get_to(v.Path);
}

void to_json(json& j, const SetComponent& v) {
	j = json{ {"Sub objects", v.Sub_objects} };
}
void from_json(const json& j, SetComponent& v) {
	j.at("Sub objects").get_to(v.Sub_objects);
}

// ========== 对象序列化 ==========
void to_json(json& j, const ObjectData& v) {
	if (!v.parent.empty()) j["ParObject"] = v.parent;
	if (!v.children.empty()) j["SubObjects"] = v.children;
	if (v.Transform) j["Transform"] = *v.Transform;
	if (v.Picture) j["Picture"] = *v.Picture;
	if (v.Textblock) j["Textblock"] = *v.Textblock;
	if (v.TriggerArea) j["TriggerArea"] = *v.TriggerArea;
	if (v.Blueprint) j["Blueprint"] = *v.Blueprint;
}

void from_json(const json& j, ObjectData& v) {
	if (j.contains("ParObject")) j.at("ParObject").get_to(v.parent);
	if (j.contains("SubObjects")) j.at("SubObjects").get_to(v.children);
	if (j.contains("Transform")) j.at("Transform").get_to(v.Transform.emplace());
	if (j.contains("Picture")) j.at("Picture").get_to(v.Picture.emplace());
	if (j.contains("Textblock")) j.at("Textblock").get_to(v.Textblock.emplace());
	if (j.contains("TriggerArea")) j.at("TriggerArea").get_to(v.TriggerArea.emplace());
	if (j.contains("Blueprint")) j.at("Blueprint").get_to(v.Blueprint.emplace());
}

// ========== 场景序列化 ==========
void to_json(json& j, const LevelData& v) {
	json levelObj;
	// 收集根对象（父对象为场景名的对象）
	std::vector<std::string> rootObjects;
	for (const auto& [name, obj] : v.objects) {
		if (obj.parent == v.name) {
			rootObjects.push_back(name);
		}
	}
	if (!rootObjects.empty()) levelObj["SubObjects"] = rootObjects;
	for (const auto& [name, obj] : v.objects) {
		levelObj[name] = obj;
	}
	j = json{ {v.name, levelObj} };
}

void from_json(const json& j, LevelData& v) {
	if (j.size() != 1) throw std::runtime_error("Level JSON must have exactly one top-level key (level name)");
	auto it = j.begin();
	v.name = it.key();
	const json& levelObj = it.value();

	// 解析所有对象（包括可能的 SubObjects 列表）
	for (auto& [objName, objJson] : levelObj.items()) {
		if (objName == "SubObjects") continue;  // 跳过根对象列表，不当作对象数据
		v.objects[objName] = objJson.get<ObjectData>();
	}
}

// ========== 文件读写 ==========
bool WriteLevelData(const std::string& filepath, const LevelData& data) {
	try {
		json j = data;  // 自动调用 to_json
		std::ofstream file(filepath, std::ios::binary);
		if (!file.is_open()) return false;
		file << j.dump(4);  // 美观输出，可调为紧凑格式(-1)以提升性能
		return true;
	}
	catch (...) {
		return false;
	}
}
bool FileSystem::WriteLevelData(const std::string& filepath, const LevelData& data) {
    return ::WriteLevelData(filepath, data);
}

LevelData ReadLevelData(const std::string& filepath) {
	if (!fs::exists(filepath)) {
		throw std::runtime_error("File not found: " + filepath);
	}
	std::ifstream file(filepath, std::ios::binary);
	if (!file.is_open()) {
		throw std::runtime_error("Cannot open file: " + filepath);
	}
	json j;
	file >> j;
	return j.get<LevelData>();  // 自动调用 from_json
}
LevelData FileSystem::ReadLevelData(const std::string& filepath) {
    return ::ReadLevelData(filepath);
}