// Copyright 2026 MrSeagull. All Rights Reserved.

#pragma once
#include <iostream>
#include <vector>
#include <string>
#include <map>
#include <filesystem>
#include <memory>
#include "SharedTypes.h"

ProjectStructure GetProjectStructure(const char* RootDirectory);
FolderStructure GetFolderStructure(const char* Directory);