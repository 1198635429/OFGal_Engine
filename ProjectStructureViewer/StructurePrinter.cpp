// Copyright 2026 MrSeagull. All Rights Reserved.
#include "StructurePrinter.h"
#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <memory>

// 递归辅助函数
void PrintFolderTree(const FolderStructure& folder,
    const std::string& prefix,
    bool isLast)
{
    // 打印当前文件夹名（根节点单独处理，这里假设递归只处理子文件夹）
    // 注意：根文件夹名称已经在 PrintProjectStructureTree 中打印，这里不重复打印当前节点自身，
    // 此辅助函数用于处理当前节点的子文件夹。
    // 因此循环遍历 SubFolders，对于每个子文件夹，构造其显示前缀并递归。
    size_t count = folder.SubFolders.size();
    size_t i = 0;
    for (const auto& [name, subFolderPtr] : folder.SubFolders) {
        ++i;
        bool lastChild = (i == count);
        std::cout << prefix;
        std::cout << (lastChild ? "`--" : "|--");
        std::cout << name << "\n";

        // 递归子文件夹，更新前缀
        std::string newPrefix = prefix;
        newPrefix += (lastChild ? "   " : "|  ");
        PrintFolderTree(*subFolderPtr, newPrefix, lastChild);
    }
}

// 对外接口：打印整个项目结构树（仅文件夹）
void PrintProjectStructureTree(const ProjectStructure& project) {
    // 打印根目录名
    std::cout << project.RootDirectory << "\n";

    // 准备处理根目录下的子文件夹
    const FolderStructure& rootFolder = project.Self;
    size_t count = rootFolder.SubFolders.size();
    size_t i = 0;
    for (const auto& [name, subFolderPtr] : rootFolder.SubFolders) {
        ++i;
        bool lastChild = (i == count);
        std::cout << (lastChild ? "`--" : "|--") << name << "\n";

        std::string prefix = lastChild ? "   " : "|  ";
        PrintFolderTree(*subFolderPtr, prefix, lastChild);
    }
}