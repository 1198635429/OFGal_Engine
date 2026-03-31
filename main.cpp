#include <iostream>
#include "_EventBus.h"
#include "BMP_Reader.h"
#include "cmdDrawer.h"
#include<windows.h>
#include <atomic>
#include <thread>
#include "FileSystem.h"
#include "GameVM.h"
#include "RenderingSystem.h"
#include "SharedTypes.h"
std::atomic<bool> running(true);    //创建原子变量，表示程序是否在运行
void InputThread(InputCollector* collector) {
	while (running) {
		collector->update();    //不断的调用采集函数，这就是单开一个线程之后的效果，能够不断的采集输入事件
	}
	Sleep(1);
}
int main() {
	_EventBus::getInstance();
	FileSystem::getInstance();
	GameVM::getInstance();
	RenderingSystem::getInstance();

	InputSystem inputSystem;   //创建输入系统实例
	InputCollector collector(&inputSystem);
	std::thread inputThread(InputThread, &collector);
	while (running)
	{
		// ===== 检测 ESC 退出 =====
		if (GetAsyncKeyState(VK_ESCAPE) & 0x8000)
		{
			running = false;
		}

		// ===== 控制帧率 =====
		Sleep(10);
	}
	running = false;
	inputThread.join();  //用于等待输入线程的结束
	system("pause");
	return 0;
}