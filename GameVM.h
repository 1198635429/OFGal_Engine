// Copyright 2026 MrSeagull. All Rights Reserved.
#pragma once
#include "SharedTypes.h"
#include "_EventBus.h"

class GameVM {
public:
	GameVM(const GameVM&) = delete;
	GameVM& operator=(const GameVM&) = delete;
	static GameVM& getInstance() {
        static GameVM instance;
        return instance;
	}

private:
	GameVM() {

	}
	~GameVM() {

	}

	LevelData currentLevel;
	char* currentProjectDirectory;
};