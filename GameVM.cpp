// Copyright 2026 MrSeagull. All Rights Reserved.
#include"SharedTypes.h"
#include "GameVM.h"

Value calcBinary(const Value& a, const Value& b, char op) {
	if (a.type == ValueType::INT && b.type == ValueType::INT) {
		int x = a.i;
		int y = b.i;
		switch (op) {
		case '+':return Value::makeInt(x + y);
		case '-':return Value::makeInt(x - y);
		case'*':return Value::makeInt(x * y);
		case'/':return Value::makeInt(y != 0 ? x / y : 0.0f);
		}
	}
	float x = (a.type == ValueType::INT) ? a.i : a.f;
	float y = (b.type == ValueType::INT) ? b.i : b.f;
	switch (op) {
	case '+': return Value::makeFloat(x + y);
	case '-': return Value::makeFloat(x - y);
	case '*': return Value::makeFloat(x * y);
	case '/': return Value::makeFloat(y != 0 ? x / y : 0.0f);
	}
}

Value Node_Equal::compute(const Value& a, const Value& b) {
	if (a.type == ValueType::INT && b.type == ValueType::INT) {
		return Value::makeBool(a.i == b.i);
	}
	float fa = (a.type == ValueType::INT) ? a.i : a.f;
	float fb = (b.type == ValueType::INT) ? b.i : b.f;
	return Value::makeBool(fa == fb);
}
Value Node_Greater::compute(const Value& a, const Value& b) {

	// int > int
	if (a.type == ValueType::INT && b.type == ValueType::INT) {
		return Value::makeBool(a.i > b.i);
	}

	// 转 float 比较
	float fa = (a.type == ValueType::INT) ? a.i : a.f;
	float fb = (b.type == ValueType::INT) ? b.i : b.f;

	return Value::makeBool(fa > fb);
}
Value Node_Less::compute(const Value& a, const Value& b) {

	// int < int
	if (a.type == ValueType::INT && b.type == ValueType::INT) {
		return Value::makeBool(a.i < b.i);
	}

	float fa = (a.type == ValueType::INT) ? a.i : a.f;
	float fb = (b.type == ValueType::INT) ? b.i : b.f;

	return Value::makeBool(fa < fb);
}

Value Node_ADD::compute(const Value& a, const Value& b) {

	// int + int
	if (a.type == ValueType::INT && b.type == ValueType::INT) {
		return Value::makeInt(a.i + b.i);
	}

	// float 参与 → float
	if ((a.type == ValueType::FLOAT || b.type == ValueType::FLOAT)) {
		float fa = (a.type == ValueType::INT) ? a.i : a.f;
		float fb = (b.type == ValueType::INT) ? b.i : b.f;
		return Value::makeFloat(fa + fb);
	}

	// string 拼接
	if (a.type == ValueType::STRING && b.type == ValueType::STRING) {
		return Value::makeString(a.s + b.s);
	}

	std::cout << "Add type error\n";
	return Value();
}
Value Node_Sub::compute(const Value& a, const Value& b) {

	if (a.type == ValueType::INT && b.type == ValueType::INT) {
		return Value::makeInt(a.i - b.i);
	}

	float fa = (a.type == ValueType::INT) ? a.i : a.f;
	float fb = (b.type == ValueType::INT) ? b.i : b.f;
	return Value::makeFloat(fa - fb);
}
Value Node_Mul::compute(const Value& a, const Value& b) {
	if (a.type == ValueType::STRING && b.type == ValueType::INT) {
		std::string res;
		for (int i = 0; i < b.i; i++) {
			res += a.s;
		}
		return Value::makeString(res);
	}
	if (a.type == ValueType::INT && b.type == ValueType::INT) {
		return Value::makeInt(a.i * b.i);
	}
	float fa = (a.type == ValueType::INT) ? a.i : a.f;
	float fb = (b.type == ValueType::INT) ? b.i : b.f;
	return Value::makeFloat(fa * fb);
}
Value Node_Div::compute(const Value& a, const Value& b) {
	float fb = (b.type == ValueType::INT) ? b.i : b.f;

	if (fb == 0.0f) {
		std::cout << "Divide by zero!\n";
		return Value();
	}

	float fa = (a.type == ValueType::INT) ? a.i : a.f;

	return Value::makeFloat(fa / fb);
}
void PlayPerNMsNode::start() {
	running = true;
	std::thread([this]() {
		while (running) {
			std::this_thread::sleep_for(std::chrono::milliseconds(intervalMs));

			ExecutionContext ctx;
			ctx.current = this->nextNode;

			RunVM(ctx);   // 用引擎执行
		}
		}).detach();
}
void PlayPerNMsNode::stop() {
	running = false;
}

void SetTransforNode::func_for_VM(ExecutionContext& ctx) {
	if (!obj->Transform.has_value()) return;
	auto& tf = obj->Transform.value();
	if (in_loc_x) tf.Location.x = in_loc_x->f;
	if (in_loc_y) tf.Location.y = in_loc_y->f;
	if (in_loc_z) tf.Location.z = in_loc_z->i;

	if (in_rotation) tf.Rotation.r = in_rotation->f;

	if (in_scale_x) tf.Scale.x = in_scale_x->f;
	if (in_scale_y) tf.Scale.y = in_scale_y->f;
}
