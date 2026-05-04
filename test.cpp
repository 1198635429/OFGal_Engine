#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <functional>
#include <memory>

// ---------------------------
// 类型定义（对应文档）
// ---------------------------
enum class PinDir { Input, Output };
enum class NodeType {
    BeginPlay,
    SetVar,
    GetVar,
    PrintText,
    If,
    While,
    Break,
    Exit,
    Add
};

// 变量存储
static std::map<std::string, int> Variables;

// 节点基类
struct Node {
    NodeType type;
    std::string name;
    Node* exec_next = nullptr;
    Node* exec_true = nullptr;
    Node* exec_false = nullptr;
    Node* exec_body = nullptr;
    Node* exec_done = nullptr;

    bool is_terminated = false;

    Node(NodeType t, std::string n) : type(t), name(n) {}
    virtual ~Node() = default;

    // 执行节点逻辑
    virtual void Execute() {}
};

// ---------------------------
// 具体节点
// ---------------------------

// 1. 入口：BeginPlay
struct BeginPlay : Node {
    BeginPlay() : Node(NodeType::BeginPlay, "BeginPlay") {}
};

// 2. 打印
struct PrintText : Node {
    std::string text;
    PrintText(std::string t) : Node(NodeType::PrintText, "PrintText"), text(t) {}
    void Execute() override {
        std::cout << "[Print] " << text << std::endl;
    }
};

// 3. 设置变量
struct SetVar : Node {
    std::string var_name;
    int value;
    SetVar(std::string v, int val) : Node(NodeType::SetVar, "SetVar"), var_name(v), value(val) {}
    void Execute() override {
        Variables[var_name] = value;
        std::cout << "[SetVar] " << var_name << " = " << value << std::endl;
    }
};

// 4. If 分支
struct IfNode : Node {
    bool condition;
    IfNode(bool cond) : Node(NodeType::If, "If"), condition(cond) {}
};

// 5. While 循环
struct WhileNode : Node {
    bool (*condition)();
    int iteration = 0;
    WhileNode(bool (*cond)()) : Node(NodeType::While, "While"), condition(cond) {}
};

// 6. Break
struct BreakNode : Node {
    BreakNode() : Node(NodeType::Break, "Break") {}
};

// 7. Exit
struct ExitNode : Node {
    ExitNode() : Node(NodeType::Exit, "Exit") {}
    void Execute() override {
        std::cout << "[Exit] 蓝图结束\n";
    }
};

// ---------------------------
// 虚拟机执行器（核心！跑执行流）
// ---------------------------
void RunExecFlow(Node* start) {
    if (!start) return;

    Node* current = start;

    while (current && !current->is_terminated) {
        std::cout << "\n→ 执行节点: " << current->name << std::endl;

        // 执行节点逻辑
        current->Execute();

        // --------------------------
        // 按 exec 引脚跳转（核心！）
        // --------------------------
        if (current->type == NodeType::BeginPlay) {
            current = current->exec_next;
        }
        else if (current->type == NodeType::If) {
            auto* ifn = static_cast<IfNode*>(current);
            if (ifn->condition) {
                std::cout << "  → If 条件 true → 走 exec_true\n";
                current = ifn->exec_true;
            }
            else {
                std::cout << "  → If 条件 false → 走 exec_false\n";
                current = ifn->exec_false;
            }
        }
        else if (current->type == NodeType::While) {
            auto* whn = static_cast<WhileNode*>(current);
            whn->iteration++;
            if (whn->condition()) {
                std::cout << "  → While 条件真 → 进入循环体(第" << whn->iteration << "次)\n";
                current = whn->exec_body;
            }
            else {
                std::cout << "  → While 条件假 → 退出到 exec_done\n";
                current = whn->exec_done;
            }
        }
        else if (current->type == NodeType::Break) {
            std::cout << "  → Break → 终止循环\n";
            current->is_terminated = true;
            break;
        }
        else if (current->type == NodeType::Exit) {
            current->is_terminated = true;
            break;
        }
        else {
            // 默认：顺序走 exec_next
            current = current->exec_next;
        }
    }

    std::cout << "\n=== 执行流结束 ===" << std::endl;
}

// ---------------------------
// 演示：你要的执行流程
// ---------------------------
int main() {
    // 1. 创建节点
    auto begin = std::make_shared<BeginPlay>();
    auto set_i = std::make_shared<SetVar>("i", 0);
    auto print = std::make_shared<PrintText>("循环运行中...");
    auto inc_i = std::make_shared<SetVar>("i", 1);
    auto exit_node = std::make_shared<ExitNode>();

    auto while_node = std::make_shared<WhileNode>([]() {
        return Variables["i"] < 3; // i < 3 循环
        });

    // 2. 连接 exec 执行流（关键！只连 exec）
    begin->exec_next = set_i.get();
    set_i->exec_next = while_node.get();

    while_node->exec_body = print.get();
    while_node->exec_done = exit_node.get();

    print->exec_next = inc_i.get();
    inc_i->exec_next = while_node.get(); // 跳回循环

    // 3. 运行
    std::cout << "=== 开始运行 OFGal 蓝图执行流 ===" << std::endl;
    RunExecFlow(begin.get());

    return 0;
}





class NODE {
public:
    NODE* lastNode;
    NODE* nextNode;

    NODE* loopNode;

    virtual void func_for_VM() = 0;
};

class Node_Add: public NODE {
public:
    vector<DATA> inData;
	vector<DATA*> outData;

    void func_for_VM() override {
        // 实现加法逻辑
	}

    std::string StringADD(string q, string b) {

    }
};

enum DataType {
    Int,
    Float,
    String,
    Bool,
    Frame
};

class DATA {
public:
	DataType type;

	int Value_int;
	float Value_float;
	std::string Value_string;
	bool Value_bool;
	frame Value_frame;
};