#include <iostream>
#include "InputSystem.h"
void InputSystem::update() {   //用于检测每一帧的键盘状态
	events.clear();  //清楚上一帧的事件
}
void InputSystem::pushEvent(const InputEvent& event) {
	events.push_back(event);
}
const std::vector<InputEvent>& InputSystem::getEvents() const {
	return events;
}