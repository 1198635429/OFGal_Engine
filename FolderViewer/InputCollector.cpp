#include "InputCollector.h"
#include "InputSystem.h"
#include "InputEvent.h"
#include "KeyCode.h"

InputCollector::InputCollector(InputSystem* system) :inputsystem(system) {}    //构造函数，接受一个输入系统的指针，并将其保存在成员变量中，以便在update函数中使用
void InputCollector::update() {
	bool ctrlDown = (GetAsyncKeyState(VK_LCONTROL) & 0x8000) || (GetAsyncKeyState(VK_LCONTROL) & 0x8000);
	bool sDown = (GetAsyncKeyState('S') & 0x8000) != 0;   //确认s被按下
	bool wDown = (GetAsyncKeyState('W') & 0x8000) != 0;   //确认w被按下
	bool sPressed = (sDown && !prevSSatate);     //确认s是刚刚按下
	if (wDown!= prevWState) {
		InputEvent event{};
		event.key = KeyCode::W;
		event.type = wDown ? InputType::KeyDown : InputType::KeyUp;
		inputsystem->pushEvent(event);
	}
	prevWState = wDown;
	if (ctrlDown && sPressed) {		//将事件加入事件的集合中
		InputEvent event{};
		event.key = KeyCode::CtrlS;
		event.type = InputType::KeyDown;
		inputsystem->pushEvent(event);   
	}
	prevSSatate = sDown ;
	struct  MouseButton { int vk; bool* prev; KeyCode code; };
	MouseButton buttons[]{     //用一个数据结构去概括所有的代码，更加简洁
	 { VK_LBUTTON, &prevMouseLeft, KeyCode::MouseLeft },
		{ VK_RBUTTON, &prevMouseRight, KeyCode::MouseRight },
		{ VK_MBUTTON, &prevMouseMiddle, KeyCode::MouseMiddle }
	};
	for (auto b : buttons) {
		bool down = (GetAsyncKeyState(b.vk) & 0x8000) != 0;
		if (down != *(b.prev)) {    //如果状态变化，那么就生成事件
			InputEvent event{};
			event.key = b.code;
			event.type = down ? InputType::KeyDown : InputType::KeyUp;
			inputsystem->pushEvent(event);
		
		}
		*(b.prev) = down;  //更新状态
	}
}