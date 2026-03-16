// Copyright 2026 MrSeagull. All Rights Reserved.

#include "ProjectStructureGetter.h"

ProjectStructure GetProjectStructure(const char* RootDirectory)
{
	ProjectStructure project_structure;

	return project_structure;
}
FolderStructure GetFolderStructure(const char* Directory) {
	FolderStructure folder_structure;
	if (!std::filesystem::exists(Directory) || !std::filesystem::is_directory(Directory))
		return folder_structure;

	return folder_structure;
}