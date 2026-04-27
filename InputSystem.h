#pragma once
#define GLOBAL_H
#include "InputEvent.h"
#include <vector>
#include <mutex>

class InputListener {
public:
	virtual void onInputEvent(const InputEvent& e) = 0;
};
class InputSystem {
public:
	void clearEvent();  //每一帧更新，用于管理生命周期
	void pushEvent(const InputEvent& event);   //添加输入事件,
	const std::vector<InputEvent>& getEvents() ;  //获取输入事件
	void addListener(InputListener* l);
	void processEvents(); // 这是核心的监听函数
private:
	std::vector<InputEvent>events;  //输入事件队列
	std::vector<InputListener*> listeners;
	std::mutex mtx;   //创建一个互斥锁；
};
extern InputSystem g_inputSystem;  //全局输入系统实例
