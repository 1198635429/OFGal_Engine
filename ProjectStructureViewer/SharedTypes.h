// Copyright 2026 MrSeagull. All Rights Reserved.
#pragma once
#include <string>
#include <vector>
#include <map>
#include <memory>

struct FolderStructure {
    std::string SelfName;
    std::vector<std::string> Files;
    std::map<std::string, std::unique_ptr<FolderStructure>> SubFolders;
};

struct ProjectStructure {
    std::string RootDirectory;
    FolderStructure Self;
};

struct TreeDisplayInfo {
    int width;   // 纯文本宽度（不含ANSI转义）
    int height;  // 总行数
};

struct SelectedFolderInfo {
    std::string absolutePath;
    int lineNumber;
};