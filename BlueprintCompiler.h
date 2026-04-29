#pragma once
#include <vector>
#include <unordered_map>
#include"GameVM.h"
#include"SharedTypes.h"

class BlueprintCompiler {
public:
	std::unordered_map<int, NODE*> nodeMap;  //用来存储节点ID和节点实例的映射关系，方便后续构建链接和执行
	std::vector<NODE* > entryNodes;      //用来储存入口节点
	void Compile(const BlueprintData& data);
	void Run();
private:
	NODE* CreateNode(const Node& n);
	void BuildExecLinks(const BlueprintData& data);
	void BuildDataLinks(const  BlueprintData& data);
	void InitNodeData(const  BlueprintData& data);

};
