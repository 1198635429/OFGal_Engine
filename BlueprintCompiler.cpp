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
		if (n.type == "BeginPlay") {
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

	// ★ 以下是条件分支 / 循环 / 显示 / 变量类节点
	if (n.type == "If")            return new If_Node();
	if (n.type == "While")         return new While_Node();
	if (n.type == "Break")         return new Break_Node();
	if (n.type == "Continue")      return new Continue_Node();
	if (n.type == "PrintText")     return new PrintText_Node();
	if (n.type == "Render")        return new Render_Node();
	if (n.type == "FrameProcess")  return new FrameProcess_Node();
	if (n.type == "ShowtheFrame")  return new ShowtheFrame_Node();
	if (n.type == "PlaySound")     return new PlaySound_Node();
	if (n.type == "PauseSound")    return new PauseSound_Node();
	if (n.type == "GET_VAR")       return new GET_VAR();
	if (n.type == "SET_VAR")       return new SET_VAR();

	std::cout << "Unknown node: " << n.type << "\n";
	return nullptr;
}

void BlueprintCompiler::BuildExecLinks(const BlueprintData& data) {
	// ★ 编译注意：当前只处理 sourcePin=="exec" 或 "then" 的标准执行流链接
	// 以下节点类型的执行流链接需要特殊处理（当前未实现）：
	//   If_Node:       sourcePin=="true"→trueBranch, sourcePin=="false"→falseBranch
	//   While_Node:    sourcePin=="loopBody"→loopBody, 且需将 loopNode 指向自身
	//   Break_Node:    nextNode 需指向所属 While 的 nextNode（循环外），loopNode 需指向所属 While
	//   Continue_Node: loopNode 需指向所属 While
	//   循环体末尾:    nextNode → While（回边），loopNode → While（标记所属循环）
	for (auto& link : data.links) {
		NODE* A = nodeMap[link.sourceNode];
		NODE* B = nodeMap[link.targetNode];
		if (!A || !B) continue;
		if (link.sourcePin == "exec" || link.sourcePin == "then") {  //�����ͨ��ִ����
			A->nextNode = B;
			B->lastNode = A;
		}
		auto* branch = dynamic_cast<If_Node*>(A);
		if (branch) {
			if (link.sourcePin == "True") {
				branch->trueBranch = B;
			}
			if (link.sourcePin == "False") {
				branch->falseBranch = B;
			}
		}
	}
}



void BlueprintCompiler::InitNodeData(const BlueprintData& data) {     //数据空间初始化
	// ★ 编译注意：当前只处理 BinaryOpNode 和 SetTransforNode 的数据空间初始化
	// 新增节点的初始化需求：
	//   While_Node:       iterationCount 初始化为 Value::makeInt(0)（当前默认构造即可）
	//   Render_Node:      outFrame 初始化（当前默认构造即可）
	//   FrameProcess_Node: outFrame 初始化（同上）
	//   GET_VAR:          outValue 初始化（当前默认构造即可）
	//   其他节点：指针成员已默认 nullptr，无需额外操作
	for (auto& n : data.nodes) {
		NODE* node = nodeMap[n.id];
		if (auto* bin = dynamic_cast<BinaryOpNode*>(node)) {  //如果是运算节点，那么就给他们的输入输出数据分配空间
			bin->InData.resize(2);
			bin->OutData.resize(1);
			for (int i = 0; i < 2; i++) {
				bin->InData[i] = nullptr;
			}
		}
		if (auto* st = dynamic_cast<SetTransforNode*>(node)) {
		
		}
	}
}


void BlueprintCompiler::BuildDataLinks(const BlueprintData& data) {  //数据流绑定函数
	// ★ 编译注意：当前只处理 BinaryOpNode→BinaryOpNode 和 BinaryOpNode→SetTransforNode 的数据绑定
	// 新增节点的数据绑定需求（当前未实现）：
	//   If_Node:           targetPin=="condition"→condition (Value*)
	//   While_Node:        targetPin=="condition"→condition (Value*)
	//   PrintText_Node:    targetPin=="text"→text (Value*)
	//   Render_Node:       targetPin=="samplingMethod"/"msaaMultiple"
	//   FrameProcess_Node: targetPin=="processName"/"inFrame"/"processParams"
	//   ShowtheFrame_Node: targetPin=="inFrame"
	//   PlaySound_Node:    targetPin=="path"/"loop"
	//   PauseSound_Node:   targetPin=="path"
	//   GET_VAR:          targetPin=="varName"→varName (Value*)；其他节点可从此节点 outValue 读数据
	//   SET_VAR:          targetPin=="varName"→varName, targetPin=="inValue"→inValue
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
			dstBin->InData[dstIndex] = &srcBin->OutData[0];
		}
		auto* branch = dynamic_cast<If_Node*>(dst);

		if (link.targetPin == "Condition") {
			branch->condition = &srcBin->OutData[0];
		}
		auto* st = dynamic_cast<SetTransforNode*>(dst);
		if (srcBin && st) {
			if (link.targetPin == "Location.x") {
				st->in_loc_x = &srcBin->OutData[0];
			}
			if (link.targetPin == "Location.y") {
				st->in_loc_y = &srcBin->OutData[0];
			}
			if (link.targetPin == "Location.z") {
				st->in_loc_z = &srcBin->OutData[0];
			}
		}
	}
}
void BlueprintCompiler::Run() {
	for (auto* entry : entryNodes) {
		ExecutionContext ctx;
		ctx.current = entry;
		RunVM(ctx);
	}
}
