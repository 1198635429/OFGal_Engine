// Copyright 2026 MrSeagull. All Rights Reserved.

#include "ProjectStructureGetter.h"
#include <system_error>   // for error_code

namespace fs = std::filesystem;

// 辅助函数：获取目录的自身名称（处理根目录特殊情况）
static std::string GetSelfName(const fs::path& dirPath) {
	std::string name = dirPath.filename().string();
	// 对于根路径（如 "/" 或 "C:\"），filename() 可能返回空或 "/"，此时用父路径名代替
	if (name.empty() || name == "/" || name == "\\") {
		name = dirPath.parent_path().filename().string();
		if (name.empty()) // 如果还是空，就用原始路径（极少情况）
			name = dirPath.string();
	}
	return name;
}

// 递归获取单个文件夹的结构
FolderStructure GetFolderStructure(const char* Directory) {
	fs::path dirPath(Directory);
	FolderStructure result;
	result.SelfName = GetSelfName(dirPath);

	std::error_code ec;  // 使用 error_code 避免异常抛出，提升性能
	fs::directory_iterator it(dirPath, ec);
	if (ec) {   // 目录无法访问，返回仅包含自身名称的空结构
		return result;
	}

	for (const auto& entry : it) {
		const auto& path = entry.path();
		std::error_code ignore_ec;

		if (fs::is_regular_file(path, ignore_ec)) {
			// 将文件名移入 vector（避免拷贝）
			result.Files.emplace_back(path.filename().string());
		}
		else if (fs::is_directory(path, ignore_ec)) {
			// 递归获取子目录结构，并使用 try_emplace 构造 map 条目（C++17）
			// try_emplace 不会构造临时 unique_ptr，直接原地构造
			auto sub = std::make_unique<FolderStructure>(
				GetFolderStructure(path.string().c_str())   // 递归调用
			);
			// 键为子目录名
			result.SubFolders.try_emplace(path.filename().string(), std::move(sub));
		}
		// 忽略其他类型（符号链接、设备文件等）
	}

	return result;
}

// 获取项目结构（根目录下的所有直接子目录）
ProjectStructure GetProjectStructure(const char* RootDirectory) {
	fs::path rootPath(RootDirectory);
	ProjectStructure result;
	result.RootDirectory = rootPath.string();
	result.Self = GetFolderStructure(RootDirectory);  // 获取根目录的完整结构
	return result;
}