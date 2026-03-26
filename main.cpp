//不要在代码中留下cout、cin等控制台输出语句，如要调试，请使用OutputDebugStringA("your message");它会输出到vs调试台中

#include <iostream>
#include "_EventBus.h"
#include "FileSystem.h"
#include "GameVM.h"
#include "RenderingSystem.h"

int main() {
	_EventBus::getInstance();
    FileSystem::getInstance();
	GameVM::getInstance();
	RenderingSystem::getInstance();
	
	system("pause");
	return 0;
}