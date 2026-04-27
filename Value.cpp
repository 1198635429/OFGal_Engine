#include"SharedTypes.h"
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
Value Value::makeBool(bool b) {
	Value val;
	val.type = ValueType::BOOL;
	val.b = b;
	return val;
}
Value Value::makeString(std::string s) {
	Value val;
	val.type = ValueType::STRING;
	val.s = s;
	return val;
}
