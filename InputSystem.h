#pragma once
#include "InputEvent.h"
#include <vector>
#include <mutex>
class InputSystem {
public:
	void clearEvent();  //每一帧更新，用于管理生命周期
	void pushEvent(const InputEvent& event);   //添加输入事件,
	const std::vector<InputEvent>& getEvents() ;  //获取输入事件
private:
	std::vector<InputEvent>events;  //输入事件队列
	std::mutex mtx;   //创建一个互斥锁；
};
