#pragma once
#include "InputEvent.h"
#include <vector>
class InputSystem {
public:
	void update();  //每一帧更新，用于管理生命周期
	void pushEvent(const InputEvent& event);   //添加输入事件
	const std::vector<InputEvent>& getEvents() const;  //获取输入事件
private:
	vector<InputEvent>events;  //输入事件队列
};
