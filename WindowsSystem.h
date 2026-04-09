// Copyright 2026 MrSeagull. All Rights Reserved.
#pragma once
#include <iostream>
#include "SharedTypes.h"

class WindowsSystem {
public:
	WindowsSystem(const WindowsSystem&) = delete;
	WindowsSystem& operator=(const WindowsSystem&) = delete;
	static WindowsSystem& getInstance() {
        static WindowsSystem instance;
        return instance;
	}

	bool OpenProjectStructureViewer(const char* RootDirectory) {

	}
private:
	std::vector<LevelData> levels;

	WindowsSystem() {

	}
	~WindowsSystem() {

	}
};