// Copyright 2026 MrSeagull. All Rights Reserved.
#pragma once

#include <string>
#include <vector>
#include <map>
#include <optional>
#include <nlohmann/json.hpp>
#include "SharedTypes.h"

using json = nlohmann::json;

// ========== 结构体序列化声明 ==========
void to_json(json& j, const Pin& v);
void from_json(const json& j, Pin& v);

void to_json(json& j, const Node& v);
void from_json(const json& j, Node& v);

void to_json(json& j, const Variable& v);
void from_json(const json& j, Variable& v);

void to_json(json& j, const Parameter& v);
void from_json(const json& j, Parameter& v);

void to_json(json& j, const Event& v);
void from_json(const json& j, Event& v);

void to_json(json& j, const Link& v);
void from_json(const json& j, Link& v);

void to_json(json& j, const BlueprintData& v);
void from_json(const json& j, BlueprintData& v);

// ========== 文件读写接口 ==========
bool WriteBPData(const std::string& filepath, const BlueprintData& data);
BlueprintData ReadBPData(const std::string& filepath);