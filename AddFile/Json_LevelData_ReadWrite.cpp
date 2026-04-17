// Copyright 2026 MrSeagull. All Rights Reserved.
#include "Json_LevelData_ReadWrite.h"
#include <fstream>
#include <filesystem>
#include <stdexcept>
//#include "FileSystem.h"

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

void to_json(json& j, const RotationEuler& v) {
	j = json{ {"r", v.r} };
}
void from_json(const json& j, RotationEuler& v) {
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

// ========== ObjectData 序列化/反序列化（仅组件） ==========
void to_json(json& j, const ObjectData& v) {
	// 只输出组件，关系由 LevelData 处理
	if (v.Transform) j["Transform"] = *v.Transform;
	if (v.Picture) j["Picture"] = *v.Picture;
	if (v.Textblock) j["Textblock"] = *v.Textblock;
	if (v.TriggerArea) j["TriggerArea"] = *v.TriggerArea;
	if (v.Blueprint) j["Blueprint"] = *v.Blueprint;
}

void from_json(const json& j, ObjectData& v) {
	// 只解析组件，关系由 LevelData 处理
	v.Transform.reset();
	v.Picture.reset();
	v.Textblock.reset();
	v.TriggerArea.reset();
	v.Blueprint.reset();

	if (j.contains("Transform")) v.Transform = j.at("Transform").get<TransformComponent>();
	if (j.contains("Picture")) v.Picture = j.at("Picture").get<PictureComponent>();
	if (j.contains("Textblock")) v.Textblock = j.at("Textblock").get<TextblockComponent>();
	if (j.contains("TriggerArea")) v.TriggerArea = j.at("TriggerArea").get<TriggerAreaComponent>();
	if (j.contains("Blueprint")) v.Blueprint = j.at("Blueprint").get<BlueprintComponent>();
}

// ========== LevelData 序列化/反序列化（处理关系） ==========
void to_json(json& j, const LevelData& v) {
	json levelObj;
	// 收集根对象（parent == nullptr）
	std::vector<std::string> rootObjects;
	for (const auto& [name, obj] : v.objects) {
		if (obj->parent == nullptr) {
			rootObjects.push_back(name);
		}
	}
	if (!rootObjects.empty()) levelObj["SubObjects"] = rootObjects;

	// 输出每个对象的完整信息
	for (const auto& [name, obj] : v.objects) {
		json objJson = *obj;  // 先获取组件部分
		// 添加 ParObject
		if (obj->parent != nullptr) {
			objJson["ParObject"] = obj->parent->name;
		}
		// 添加 SubObjects
		if (!obj->objects.empty()) {
			std::vector<std::string> subNames;
			for (const auto& [subName, subObj] : obj->objects) {
				subNames.push_back(subName);
			}
			objJson["SubObjects"] = subNames;
		}
		levelObj[name] = objJson;
	}
	j = json{ {v.name, levelObj} };
}

void from_json(const json& j, LevelData& v) {
	if (j.size() != 1) throw std::runtime_error("Level JSON must have exactly one top-level key (level name)");
	auto it = j.begin();
	v.name = it.key();
	const json& levelObj = it.value();

	// 存储临时关系信息
	std::unordered_map<std::string, std::string> parentNames;
	std::unordered_map<std::string, std::vector<std::string>> childrenNames;

	// 第一步：创建所有对象并填充组件，记录关系字符串
	for (auto& [objName, objJson] : levelObj.items()) {
		if (objName == "SubObjects") continue;  // 跳过顶层子对象列表
		auto obj = std::make_unique<ObjectData>();
		obj->name = objName;
		objJson.get_to(*obj);  // 填充组件（忽略关系字段）
		// 记录 ParObject
		if (objJson.contains("ParObject")) {
			parentNames[objName] = objJson["ParObject"].get<std::string>();
		}
		// 记录 SubObjects
		if (objJson.contains("SubObjects")) {
			childrenNames[objName] = objJson["SubObjects"].get<std::vector<std::string>>();
		}
		v.objects[objName] = obj.release();  // 转移所有权
	}

	// 第二步：建立父子关系（根据 parentNames 和 childrenNames）
	for (auto& [objName, obj] : v.objects) {
		// 设置 parent 指针
		auto itParent = parentNames.find(objName);
		if (itParent != parentNames.end()) {
			const std::string& parentName = itParent->second;
			if (parentName == v.name) {
				obj->parent = nullptr;  // 父对象为场景
			}
			else {
				auto parentIt = v.objects.find(parentName);
				if (parentIt != v.objects.end()) {
					obj->parent = parentIt->second;
				}
				else {
					throw std::runtime_error("Parent object not found: " + parentName);
				}
			}
		}
		else {
			obj->parent = nullptr;  // 默认父对象为场景，稍后可能被场景 SubObjects 覆盖
		}

		// 设置子对象映射
		auto itChildren = childrenNames.find(objName);
		if (itChildren != childrenNames.end()) {
			for (const std::string& subName : itChildren->second) {
				auto subIt = v.objects.find(subName);
				if (subIt != v.objects.end()) {
					obj->objects[subName] = subIt->second;
				}
				else {
					throw std::runtime_error("Child object not found: " + subName);
				}
			}
		}
	}

	// 第三步：处理场景的直接子对象（顶层 SubObjects）
	if (levelObj.contains("SubObjects")) {
		auto rootNames = levelObj["SubObjects"].get<std::vector<std::string>>();
		for (const std::string& rootName : rootNames) {
			auto objIt = v.objects.find(rootName);
			if (objIt != v.objects.end()) {
				objIt->second->parent = nullptr;  // 确保根对象父指针为 nullptr
			}
			else {
				throw std::runtime_error("Root object not found: " + rootName);
			}
		}
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