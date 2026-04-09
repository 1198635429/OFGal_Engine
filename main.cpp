#include <iostream>
#include "_EventBus.h"
#include "BMP_Reader.h"
#include "cmdDrawer.h"
#include "InputSystem.h"
#include<windows.h>
#include <atomic>
#include <thread>
#include"InputCollector.h"
#include "FileSystem.h"
#include "SoundSystem.h"
#include "GameVM.h"
#include "RenderingSystem.h"
#include "WindowsSystem.h"
std::atomic<bool> running(true);    //创建原子变量，表示程序是否在运行
void InputThread(InputCollector* collector) {
	while (running) {
		collector->update();
	}
	Sleep(20);
}
int main() {
	_EventBus* pEventBus = &_EventBus::getInstance();
	FileSystem* pFileSystem = &FileSystem::getInstance();
	SoundSystem* pSoundSystem = &SoundSystem::getInstance();
	GameVM* pGameVM = &GameVM::getInstance();
	RenderingSystem* pRenderingSystem = &RenderingSystem::getInstance();

	InputSystem inputSystem;
	InputCollector collector(&inputSystem);
	std::thread inputThread(InputThread, &collector);

	//_EventBus::getInstance().publish_SoundPlay("E:/Projects/C++Projects/OFGal_Engine/acane_madder___Think_of_You.wav");

	//system("pause");
	running = false;
	inputThread.join();  // 等待子线程结束
	return 0;
}