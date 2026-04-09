// Copyright 2026 MrSeagull. All Rights Reserved.
#pragma once
#include <iostream>
#include "SharedTypes.h"

void PrintFolderTree(const FolderStructure& folder,
    const std::string& prefix,
    bool isLast);

void PrintProjectStructureTree(const ProjectStructure& project);