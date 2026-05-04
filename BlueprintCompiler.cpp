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
	for (auto& link : data.links) {
		NODE* A = nodeMap[link.sourceNode];
		NODE* B = nodeMap[link.targetNode];
		if (!A || !B) continue;
		if (link.sourcePin == "exec" || link.sourcePin == "then") {  //这里进行基础的链接
			A->nextNode = B;
			B->lastNode = A;
		}
		auto* whileNode = dynamic_cast<While_Node*>(A);
		if (whileNode && link.sourcePin == "OEXEC_Loop") {  //这里能够进行传导，将每一个节点都带上循环的属性
			nodeTowhile[B] = whileNode;
		}
		bool updated = true;
		while (updated) {
			updated = false;
			for (auto& link : data.links) {
				NODE* A = nodeMap[link.sourceNode];
				NODE* B = nodeMap[link.targetNode];
				if (nodeTowhile.count(A) && !nodeTowhile.count(B)) {
					nodeTowhile[B] = nodeTowhile[A];
					updated = true;
				}
			}
		}
		for (auto& pair : nodeTowhile) {
			NODE* node = pair.first;
			While_Node* whileNode = pair.second;
			if (auto* br = dynamic_cast<Break_Node*> (node)) {
				br->loopNode = whileNode;
			}
			if (auto* cont = dynamic_cast<Continue_Node*>(node)) {
				cont->loopNode = whileNode;
			}
		}

		auto* branch = dynamic_cast<If_Node*>(A);
		if (branch) {
			if (link.sourcePin == "OEXEC_A") {
				branch->trueNode = B;
			}
			if (link.sourcePin == "OEXEC_B") {
				branch->falseNode = B;
			}
		}
		auto* whileNode = dynamic_cast<While_Node*>(A);
		if (whileNode) {
			if (link.sourcePin == "OEXEC_Loop") {
				whileNode->loopBodyNode = B;
			}
			else if (link.sourcePin == "OEXEC") {
				whileNode->loopExitNode = B;
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
			for (int i = 0; i < 2; i++) {
				bin->InData[i] = nullptr;
			}
		}
		if (auto* st = dynamic_cast<SetTransforNode*>(node)) {
		
		}
	}
}


void BlueprintCompiler::BuildDataLinks(const BlueprintData& data) { 
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

		if (link.targetPin == "shouldRunA") {
			branch->condition = &srcBin->OutData[0];
		}

		if (auto* whileNode = dynamic_cast<While_Node*>(dst)) {

			Value* out = nullptr;

			if (auto* bin = dynamic_cast<BinaryOpNode*>(src)) {
				out = &bin->OutData[0];
			}

			if (link.targetPin == "shouldRunLoop") {
				whileNode->condition = out;
			}
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
