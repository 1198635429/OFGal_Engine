// Copyright 2026 MrSeagull. All Rights Reserved.
#pragma once
#include<thread>
#include<iostream>
#include "SharedTypes.h"
#include "_EventBus.h"
#include"InputCollector.h"
#include "InputSystem.h"
#include "InputEvent.h"

class NODE { //这是父类
public:
	NODE* lastNode = nullptr;
	NODE* nextNode = nullptr;
	NODE* loopNode = nullptr;

	virtual void func_for_VM() = 0;
};
class BinaryOpNode :public NODE {
public:
	std::vector<Value>InData;
	std::vector<Value> OutData;
	virtual Value compute(const Value& a, const Value& b) = 0;
	void func_for_VM() {
		Value result = compute(InData[0], InData[1]);
		OutData[0] = result;
		if (nextNode) {
			nextNode->func_for_VM();
		}
	}
};
class Node_ADD :public BinaryOpNode {
public:
	Value compute(const Value& a, const Value& b);
};
class Node_Sub : public BinaryOpNode {
public:
	Value compute(const Value& a, const Value& b);
};
class Node_Mul :public BinaryOpNode {
public:
	Value compute(const Value& a, const Value& b);
};
class Node_Div :public BinaryOpNode {
public:
	Value compute(const Value& a, const Value& b);
};
class BeginPlay_Node :public NODE {
public:
	void fun_for_VM() {
		if (nextNode) {
			nextNode->func_for_VM();
		}
	}

};

class PlayPerNMsNode :public NODE {  //一种另类的开始节点
	int intervalMs = 1000;  //此处定义的是间隔的时间
	std::atomic<bool> running = false;
	void start();
	void stop();
	void func_for_VM() {
		start();
	}
};

class PlayWhenKeyNode : public NODE, public InputListener {
public:
	KeyCode targetKey;

	PlayWhenKeyNode(KeyCode key) {     //这是一个初始化函数，在使用节点的时候记得调用它很好用
		targetKey = key;

		g_inputSystem.addListener(this);
	}

	void onInputEvent(const InputEvent& e) override {
		if (e.type == InputType::KeyDown && e.key == targetKey) {
			func_for_VM();
		}
	}

	void func_for_VM() {
		if (nextNode) {
			nextNode->func_for_VM();
		}
	}
};

class Exit : public NODE {
public:
	void func_for_VM() {
		return;
	}

};

class SetTransforNode : public NODE {
public:
	ObjectData* obj = nullptr;
	PinValue in_loc_x;
	PinValue in_loc_y;
	PinValue in_loc_z;
	PinValue in_rotation;
	PinValue in_scale_x;
	PinValue in_scale_y;
	NODE* nextNode = nullptr;
	void func_for_VM();
};