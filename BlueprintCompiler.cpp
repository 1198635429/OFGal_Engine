#include"BlueprintCompiler.h"
#include<iostream>

void BlueprintCompiler::Compile(const BlueprintData& data) {  //这是蓝图编辑器的核心函数，能够调用其他的组件函数
	for (auto& n : data.nodes) {
		NODE* node = CreateNode(n);
		if (node) {
			nodeMap[n.id] = node;
		}
	}
	InitNodeData(data);
	BuildExecLinks(data);
	BuildDataLinks(data);
	for (auto& n : data.nodes) {
		if (n.type == "BegeinPlay") {
			entryNodes.push_back(nodeMap[n.id]);
		}
	}
}

NODE* BlueprintCompiler::CreateNode(const Node& n) {
	if (n.type == "ADD")return new Node_ADD();
	if (n.type == "Sub") return new Node_Sub();
	if (n.type == "Mul")return new Node_Mul();
	if (n.type == "Div")return new Node_Div();
	if (n.type == "BeginPlay") return new BeginPlay_Node();
	if (n.type == "Exit") return new Exit();

	if (n.type == "SetTransform") {
		return new SetTransforNode();
	}

	std::cout << "Unknown node: " << n.type << "\n";
	return nullptr;
}

void BlueprintCompiler::BuildExecLinks(const BlueprintData& data) {
	for (auto& link : data.links) {
		if (link.sourcePin == "exec" || link.sourcePin == "then") {
			NODE* A = nodeMap[link.sourceNode];
			NODE* B = nodeMap[link.targetNode];
			if (A && B) {
				A->nextNode = B;
				B->lastNode = A;
			}
		}
	}
}



void BlueprintCompiler::InitNodeData(const BlueprintData& data) {
	for (auto& n : data.nodes) {
		NODE* node = nodeMap[n.id];
		if (auto* bin = dynamic_cast<BinaryOpNode*>(node)) {  //如果是运算节点，那么就给他们的输入输出数据分配空间
			bin->InData.resize(2);
			bin->OutData.resize(1);
		}
		if (auto* st = dynamic_cast<SetTransforNode*>(node)) {
		
		}
	}
}


void BlueprintCompiler::BuildDataLinks(const BlueprintData& data) {  //数据流绑定函数
	for (auto& link : data.links) {
		if (link.sourcePin == "exec" || link.sourcePin == "then") {
			continue;   //如果是执行流的链接直接跳过
		}
		NODE* src = nodeMap[link.sourceNode];
		NODE* dst = nodeMap[link.targetNode];
		if (!src || !dst) {
			continue;
		}
		auto* srcBin = dynamic_cast<BinaryOpNode*>(src);
		auto* dstBin = dynamic_cast<BinaryOpNode*>(dst);
		if (srcBin && dstBin) {
			int dstIndex = (link.targetPin == "A") ? 0 : 1;
			dstBin->InData[dstIndex] = srcBin->OutData[0];
		}
		auto* st = dynamic_cast<SetTransforNode*>(dst);
		if (srcBin && st) {

			Value v = srcBin->OutData[0];

			if (link.targetPin == "Location.x") {
				st->in_loc_x.value = v;
				st->in_loc_x.hasValue = true;
			}
			if (link.targetPin == "Location.y") {
				st->in_loc_y.value = v;
				st->in_loc_y.hasValue = true;
			}
			if (link.targetPin == "Location.z") {
				st->in_loc_z.value = v;
				st->in_loc_z.hasValue = true;
			}
		}
	}
	


}

