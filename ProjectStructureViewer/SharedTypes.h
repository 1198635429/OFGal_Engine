// Copyright 2026 MrSeagull. All Rights Reserved.
#pragma once
#include <string>
#include <vector>
#include <map>
#include <memory>

/*
=================================================
文件系统结构体定义
=================================================
*/
struct FolderStructure {
	std::string SelfName;
	std::vector<std::string> Files;
	std::map<std::string, std::unique_ptr<FolderStructure>> SubFolders;
};

struct ProjectStructure {
	std::string RootDirectory;
	FolderStructure Self;
};