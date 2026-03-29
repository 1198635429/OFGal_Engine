#pragma once
#include "Windows.h"
#include "InputSystem.h"
class InputCollector {
public:
	InputCollector(InputSystem* system);
	void update();   //核心轮询函数，能够查询当前的输入状态，并将输入事件添加到输入系统中
private:
	InputSystem* inputsystem;
	bool prevSSatate = false;   //四个变量记录按键前一刻的状态
	bool prevMouseLeft = false;
	bool prevMouseRight = false;
	bool prevMouseMiddle = false;
	bool prevWState = false;

};