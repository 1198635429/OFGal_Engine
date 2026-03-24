// Copyright 2026 MrSeagull. All Rights Reserved.

#include "GameVM.h"
void GameVM::testfunc() {
	levels.push_back(_EventBus::getInstance().publish_ReadLevelData("E:\\Projects\\C++Projects\\OFGal_Engine\\TestLevel1.level"));
}