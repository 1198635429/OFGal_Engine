// Copyright 2026 MrSeagull. All Rights Reserved.
#pragma once
#include <iostream>
#include "SharedTypes.h"

// 打印项目结构树，可指定高亮行号（-1 表示不高亮）
void PrintProjectStructureTree(const ProjectStructure& project, int highlightLine = -1);

// 调整控制台缓冲区大小
void AdjustConsoleBufferSizeToContent(const TreeDisplayInfo& info);

// 获取当前选中信息
const SelectedFolderInfo& GetSelectedFolderInfo();

// 设置选中行号（同时更新全局选中信息）
void SetSelectedLine(int lineNumber);

// 根据行号获取文件夹绝对路径
std::string GetFolderPathByLine(int lineNumber);

// 获取总行数（用于键盘边界判断）
int GetTotalLines();

void InitConsoleVTMode();