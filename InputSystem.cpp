#include <iostream>
#include "SharedTypes.h"
InputSystem g_inputSystem;  //全局输入系统实例
void InputSystem::clearEvent() {   //用于检测每一帧的键盘状态
	std::lock_guard<std::mutex> lock(mtx);
	events.clear();  //清楚上一帧的事件
}
void InputSystem::pushEvent(const InputEvent& event) {		//加入事件
	std::lock_guard<std::mutex>  lock(mtx);  //创建一个锁
	events.push_back(event);         
}
const std::vector<InputEvent>& InputSystem::getEvents()  {
	std::lock_guard<std::mutex> lock(mtx);    //会自动释放锁，保证线程安全
	return events;
}