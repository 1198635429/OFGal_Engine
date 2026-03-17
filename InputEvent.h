#pragma once
#include "KeyCode.h"
enum class InputType {
	KeyDown,
	KeyUp,
	MouseMove,
	MouseUp,
	MouseDown
};
struct InputEvent {
	InputType type;
	KeyCode key;  //¼üÅÌ
	int mouseX;     //Êó±êÎ»ÖÃ
	int mouseY;
};
