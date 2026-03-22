#include "InputCollector.h"
#include "InputSystem.h"
#include "InputEvent.h"
#include "KeyCode.h"

extern InputSystem g_inputSystem; 
static KeyCode TranslateKey(WPARAM wParam) {    //用于进行翻译工作
	switch (wParam) {
	case 'W':
	case 'w':
		return KeyCode::W;
	case 'A':
	case 'a':
		return KeyCode::A;
	case 'S':
	case 's':
		return KeyCode::S;
	case 'D':
	case 'd':
		return KeyCode::D;
	default:
		return KeyCode::Unknown;
	}
}
	LRESULT CALLBACK InputCollector::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {    //用于进行输入的读取
		InputEvent event{};
			switch (msg) {
			case WM_KEYDOWN:   //键盘按下
				event.type = InputType::KeyDown;
				event.key = TranslateKey(wParam);
				g_inputSystem.pushEvent(event);
				break;
			case WM_KEYUP:   //键盘抬起
				event.type = InputType::KeyUp;
				event.key = TranslateKey(wParam);
				g_inputSystem.pushEvent(event);
				break; 
			case WM_LBUTTONDOWN:   //鼠标左键按下
				event.type = InputType::MouseDown;
				event.key = KeyCode::MouseLeft;
				event.mouseX = GET_X_LPARAM(lParam);
				event.mouseY = GET_Y_LPARAM(lParam);
				g_inputSystem.pushEvent(event);
				break;
			case WM_LBUTTONUP:      //鼠标左键抬起
				event.type = InputType::MouseUp;
				event.key = KeyCode::MouseLeft;
				event.mouseX = GET_X_LPARAM(lParam);
				event.mouseY = GET_Y_LPARAM(lParam);
				g_inputSystem.pushEvent(event);
				break;
			case WM_MOUSEMOVE:     //鼠标移动
				event.type = InputType::MouseMove;
				event.mouseX = GET_X_LPARAM(lParam);
				event.mouseY = GET_Y_LPARAM(lParam);
				g_inputSystem.pushEvent(event);
				break;
			case WM_RBUTTONDOWN:   //鼠标右键按下
				event.type = InputType::MouseDown;
				event.key = KeyCode::MouseRight;
				event.mouseX = GET_X_LPARAM(lParam);
				event.mouseY = GET_Y_LPARAM(lParam);
				g_inputSystem.pushEvent(event);
				break;
			case WM_RBUTTONUP:    //鼠标右键抬起
				event.type = InputType::MouseUp;
				event.key = KeyCode::MouseRight;
				event.mouseX = GET_X_LPARAM(lParam);
				event.mouseY = GET_Y_LPARAM(lParam);
				g_inputSystem.pushEvent(event);
				break;
			}
			return DefWindowProc(hwnd, msg, wParam, lParam);    //返回的四个参数分别是：窗口句柄、消息类型、消息参数1、消息参数2
	}