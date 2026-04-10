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

/*
=================================================
树状结构显示信息（用于控制台缓冲区调整）
=================================================
*/
struct TreeDisplayInfo {
	int width;   // 最大列数（不包含 ANSI 转义序列）
	int height;  // 总行数
};

/*
=================================================
光标选中信息
=================================================
*/
struct SelectedFolderInfo {
	std::string absolutePath;   // 当前选中文件夹的绝对路径
	int lineNumber;             // 选中行号（0 为根目录）
};