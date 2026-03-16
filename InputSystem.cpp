#include <iostream>
#include "InputSystem.h"
#include "_EventBus.h"
#include "InputEvent,h"
#include "KeyCode.h"
#include "Windows.h"
InputSystem::update() {   //用于检测每一帧的键盘状态
	if (GetAsyncKeyState('W') & 0x8000) {
		InputEvent w;
		w.type = InputType::KeyDown;
		w.key = 'W';
	}
}