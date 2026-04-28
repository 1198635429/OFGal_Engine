#pragma once
#include <vector>
#include <unordered_map>
#include"GameVM.h"
#include"SharedTypes.h"

class BlueprintCompiler {
public:
	std::unordered_map<int, NODE*> nodeMap;
	std::vector<NODE* > entryNodes;
	void Compile(const BlueprintData& data);
private:
	NODE* CreateNode(const Node& n);
	void BuildExecLinks(const BlueprintData& data);
	void BuildDataLinks(const  BlueprintData& data);
	void InitNodeData(const  BlueprintData& data);

};
