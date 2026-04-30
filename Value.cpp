#include"SharedTypes.h"
Value Value::makeNone() {
	Value val;
	val.type = ValueType::NONE;
	return val;
}
Value Value::makeInt(int v) {
	Value val;
	val.type = ValueType::INT;
	val.i = v;
	return val;
}
Value Value::makeFloat(float v) {
	Value val;
	val.type = ValueType::FLOAT;
	val.f = v;
	return val;
}
Value Value::makeBool(bool v) {
	Value val;
	val.type = ValueType::BOOL;
	val.b = v;
	return val;
}
Value Value::makeString(const std::string& s) {
	Value val;
	val.type = ValueType::STRING;
	val.s = s;
	return val;
}
Value Value::makeFrame(const Frame& v) {
	Value val;
	val.type = ValueType::FRAME;
	val.frame = v;
	return val;
}
