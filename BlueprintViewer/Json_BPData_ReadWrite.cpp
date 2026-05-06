// Copyright 2026 MrSeagull. All Rights Reserved.
#include "Json_BPData_ReadWrite.h"
#include <fstream>
#include <filesystem>
#include <stdexcept>

namespace fs = std::filesystem;

// ========== Pin 序列化 ==========
void to_json(json& j, const Pin& v) {
    j = json{
        {"name", v.name},
        {"io", v.io},
        {"type", v.type}
    };
    if (v.literal.has_value()) {
        j["literal"] = v.literal.value();
    }
}

void from_json(const json& j, Pin& v) {
    j.at("name").get_to(v.name);
    j.at("io").get_to(v.io);
    j.at("type").get_to(v.type);
    if (j.contains("literal") && !j["literal"].is_null()) {
        v.literal = j["literal"].get<std::string>();
    }
    else {
        v.literal = std::nullopt;
    }
}

// ========== Node 序列化 ==========
void to_json(json& j, const Node& v) {
    j = json{
        {"id", v.id},
        {"type", v.type},
        {"pins", v.pins},
        {"properties", v.properties}
    };
}

void from_json(const json& j, Node& v) {
    j.at("id").get_to(v.id);
    j.at("type").get_to(v.type);
    j.at("pins").get_to(v.pins);
    j.at("properties").get_to(v.properties);
}

// ========== Variable 序列化 ==========
void to_json(json& j, const Variable& v) {
    j = json{
        {"name", v.name},
        {"type", v.type},
        {"value", v.value}
    };
}

void from_json(const json& j, Variable& v) {
    j.at("name").get_to(v.name);
    j.at("type").get_to(v.type);
    j.at("value").get_to(v.value);
}

// ========== Parameter 序列化 ==========
void to_json(json& j, const Parameter& v) {
    j = json{
        {"name", v.name},
        {"type", v.type},
        {"defaultValue", v.defaultValue}
    };
}

void from_json(const json& j, Parameter& v) {
    j.at("name").get_to(v.name);
    j.at("type").get_to(v.type);
    j.at("defaultValue").get_to(v.defaultValue);
}

// ========== Event 序列化 ==========
void to_json(json& j, const Event& v) {
    j = json{
        {"Event_Name", v.event_name},   // 固定使用事件名称键
        {"id", v.id}
    };
}

void from_json(const json& j, Event& v) {
    j.at("Event_Name").get_to(v.event_name); // 与文件键名一致
    j.at("id").get_to(v.id);
}

// ========== Link 序列化 ==========
void to_json(json& j, const Link& v) {
    j = json{
        {"sourceNode", v.sourceNode},
        {"sourcePin", v.sourcePin},
        {"targetNode", v.targetNode},
        {"targetPin", v.targetPin}
    };
}

void from_json(const json& j, Link& v) {
    j.at("sourceNode").get_to(v.sourceNode);
    j.at("sourcePin").get_to(v.sourcePin);
    j.at("targetNode").get_to(v.targetNode);
    j.at("targetPin").get_to(v.targetPin);
}

// ========== BlueprintData 序列化 ==========
void to_json(json& j, const BlueprintData& v) {
    j = json{
        {"Name", v.name},                // 大写，保持与现有文件一致
        {"id", v.id},
        {"Nodes", v.nodes},
        {"Variables", v.variables},
        {"InParameters", v.inParameters},
        {"OutParameters", v.outParameters},
        {"Events", v.events},
        {"Links", v.links}
    };
}

void from_json(const json& j, BlueprintData& v) {
    j.at("Name").get_to(v.name);
    j.at("id").get_to(v.id);
    j.at("Nodes").get_to(v.nodes);
    j.at("Variables").get_to(v.variables);
    j.at("InParameters").get_to(v.inParameters);
    j.at("OutParameters").get_to(v.outParameters);
    j.at("Events").get_to(v.events);
    j.at("Links").get_to(v.links);
}

// ========== 文件读写 ==========
bool WriteBPData(const std::string& filepath, const BlueprintData& data) {
    try {
        json j = data;               // 自动调用 to_json
        std::ofstream file(filepath, std::ios::binary);
        if (!file.is_open()) return false;
        file << j.dump(4);           // 美观输出
        return true;
    }
    catch (...) {
        return false;
    }
}

BlueprintData ReadBPData(const std::string& filepath) {
    if (!fs::exists(filepath)) {
        throw std::runtime_error("File not found: " + filepath);
    }
    std::ifstream file(filepath, std::ios::binary);
    if (!file.is_open()) {
        throw std::runtime_error("Cannot open file: " + filepath);
    }
    json j;
    file >> j;
    return j.get<BlueprintData>();   // 自动调用 from_json
}