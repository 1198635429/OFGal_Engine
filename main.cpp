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
const std::vector<std::string>& members = { "yerundong7", "epc-sg","Nagato-Yuki-708" };
void printWelcomeMessage(int version, const std::vector<std::string>& members) {
    system("cls");

    const std::string BORDER(92, '=');

    const char* OFGal_Art[] = {
        "   ___    _____    ____           _           _____                                   ",
        "  / _ \\  |  ___|  / ___|   __ _  | |         | ____|  _ __     __ _  (_)  _ __     ___ ",
        " | | | | | |_    | |  _   / _` | | |         |  _|   | '_ \\   / _` | | | | '_ \\   / _ \\",
        " | |_| | |  _|   | |_| | | (_| | | |         | |___  | | | | | (_| | | | | | | | |  __/",
        "  \\___/  |_|      \\____|  \\__,_| |_|  _____  |_____| |_| |_|  \\__, | |_| |_| |_|  \\___|",
        "                                     |_____|                  |___/                    "
    };

    // 打印顶部边框
    std::cout << BORDER << std::endl;

    // 打印 ASCII 艺术字（居中对齐效果可以通过前方加空格实现，这里简单左对齐）
    for (const auto& line : OFGal_Art) {
        std::cout << "  " << line << std::endl;
    }

    std::cout << BORDER << std::endl;

    // 显示版本号（格式化）
    int major = version / 10000;
    int minor = (version % 10000) / 100;
    int patch = version % 100;
    std::cout << "  Version: " << major << "." << minor << "." << patch << std::endl;

    // 显示制作人员
    std::cout << "  Created by: ";
    for (size_t i = 0; i < members.size(); ++i) {
        std::cout << members[i];
        if (i != members.size() - 1) {
            std::cout << ", ";
        }
    }
    std::cout << std::endl;

    // 底部边框
    std::cout << BORDER << std::endl;
}
std::string GetProjectPath() {
    printWelcomeMessage(10001, members);
	std::string ProjectPath = "";
	std::cout << "Please enter the project path:" << std::endl;
	std::cin >> ProjectPath;
	system("cls");
	return ProjectPath;
}
int main() {
	std::string ProjectPath = GetProjectPath();

	_EventBus* pEventBus = &_EventBus::getInstance();
	FileSystem* pFileSystem = &FileSystem::getInstance();
	SoundSystem* pSoundSystem = &SoundSystem::getInstance();
	RenderingSystem* pRenderingSystem = &RenderingSystem::getInstance();
    WindowsSystem* pWindowsSystem = &WindowsSystem::getInstance(ProjectPath);		//仅本次设置路径有效
	//GameVM* pGameVM = &GameVM::getInstance();		//由窗口管理系统持有

	InputSystem inputSystem;
	InputCollector collector(&inputSystem);
	std::thread inputThread(InputThread, &collector);

	system("pause");
	running = false;
	inputThread.join();  // 等待子线程结束
	return 0;
}