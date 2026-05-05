// Copyright 2026 MrSeagull. All Rights Reserved.
#pragma once
#include<thread>
#include<iostream>
#include "SharedTypes.h"
#include "_EventBus.h"
#include "SoundEvents.h"
#include"InputCollector.h"
#include "InputSystem.h"
#include "InputEvent.h"
#include <unordered_map>

// ============================================================
// 前向声明
// ============================================================
class NODE;  // ExecutionContext 需要 NODE* 成员

// ============================================================
// 执行上下文（必须在 NODE 之前定义，因为 NODE::func_for_VM 需要 ExecutionContext 完整类型）
// ============================================================
class ExecutionContext {   //执行引擎,起到一个记录上下文的作用
public:
	NODE* current = nullptr;
	bool running = true;
	NODE* lastExecuted = nullptr;  //在调试的时候使用
};

// ============================================================
// NODE 基类
// ============================================================
class NODE { //这是父类
public:
	NODE* lastNode = nullptr;
	NODE* nextNode = nullptr;
	NODE* loopNode = nullptr;

	// ★ 编译注意：所有节点统一签名 void func_for_VM(ExecutionContext& ctx)
	//   RunVM 在调用前将 ctx.current 默认设为 node->nextNode
	//   单出口节点无需修改 ctx.current；分支节点覆盖 ctx.current 即可改变执行流
	virtual void func_for_VM(ExecutionContext& ctx) = 0;
};

// ============================================================
// 运算节点
// ============================================================

class BinaryOpNode :public NODE {  // 蓝图节点类型：ADD / Sub / Mul / Div 的公共基类
public:
	std::vector<Value*>InData;
	std::vector<Value> OutData;
	virtual Value compute(const Value& a, const Value& b) = 0;
	void func_for_VM(ExecutionContext& ctx) override {
		Value a = InData[0] ? *InData[0] : Value();
		Value b = InData[1] ? *InData[1] : Value();
		Value result = compute(a, b);
			OutData[0] = result;
	}
};
class Node_Equal : public BinaryOpNode {    //这个是等号的比较节点
public:
	Value compute(const Value& a, const Value& b);
};
class Node_Greater : public BinaryOpNode {
public:
	Value compute(const Value& a, const Value& b);
};
class Node_Less : public BinaryOpNode {
public:
	Value compute(const Value& a, const Value& b);
};
class Node_ADD :public BinaryOpNode {
public:
	Value compute(const Value& a, const Value& b);
};

class Node_Sub : public BinaryOpNode {  // 蓝图节点类型："Sub"
public:
	Value compute(const Value& a, const Value& b);
};

class Node_Mul :public BinaryOpNode {  // 蓝图节点类型："Mul"
public:
	Value compute(const Value& a, const Value& b);
};

class Node_Div :public BinaryOpNode {  // 蓝图节点类型："Div"
public:
	Value compute(const Value& a, const Value& b);
};

// ============================================================
// 入口 / 退出节点
// ============================================================

class BeginPlay_Node :public NODE {  // 蓝图节点类型："BeginPlay"
public:
	void func_for_VM(ExecutionContext& ctx) override {
		
	}

};

class PlayPerNMsNode :public NODE {  // 蓝图节点类型："PlayPerNMs" —— 另类的开始节点
	int intervalMs = 1000;  //此处定义的是间隔的时间
	std::atomic<bool> running = false;
	void start();
	void stop();
	void func_for_VM(ExecutionContext& ctx) override {
		start();
	}
};

class PlayWhenKeyNode : public NODE, public InputListener {  // 蓝图节点类型："PlayWhenKey"
public:
	KeyCode targetKey;

	PlayWhenKeyNode(KeyCode key) {     //这是一个初始化函数，在使用节点的时候记得调用它很好用
		targetKey = key;

		g_inputSystem.addListener(this);
	}

	void onInputEvent(const InputEvent& e) override;  // ★ 声明，定义在文件末尾（需要RunVM完整定义）

	void func_for_VM(ExecutionContext& ctx) override {
		// 由事件驱动，onInputEvent中直接调用RunVM启动执行链
	}

};

class Exit : public NODE {  // 蓝图节点类型："Exit"
public:
	void func_for_VM(ExecutionContext& ctx) override {
		ctx.running = false;
	}

};

// ============================================================
// 变换节点
// ============================================================

class SetTransforNode : public NODE {  // 蓝图节点类型："SetTransform"
public:
	ObjectData* obj = nullptr;
	Value* in_loc_x = nullptr;
	Value* in_loc_y = nullptr;
	Value* in_loc_z = nullptr;
	Value* in_rotation = nullptr;
	Value* in_scale_x = nullptr;
	Value* in_scale_y = nullptr;
	// ★ 编译注意：不要在此类中声明 NODE* nextNode，会遮蔽基类成员！已删除
	void func_for_VM(ExecutionContext& ctx);  // 定义在 GameVM.cpp
	// ★ 编译注意：
	//   1. BuildDataLinks 中根据 targetPin 名称绑定到对应字段：
	//      "Location.x"→in_loc_x, "Location.y"→in_loc_y, "Location.z"→in_loc_z 等
	//   2. InitNodeData 中无需额外操作（指针已默认 nullptr，由 BuildDataLinks 绑定）
	//   3. obj 指针需要由编译器在链接阶段设置（当前代码中未实现，需要补充）
};

// ============================================================
// 条件分支 / 循环节点
// ============================================================

class If_Node : public NODE {  // 蓝图节点类型："If"
public:
	Value* condition = nullptr;
	NODE* trueBranch = nullptr;
	NODE* falseBranch = nullptr;
	void func_for_VM(ExecutionContext& ctx) override {
		bool cond = (condition && condition->type == ValueType::BOOL) ? condition->b : false;
		ctx.current = cond ? trueBranch : falseBranch;
	}
	// ★ 编译注意：
	//   1. BuildExecLinks 需要处理 sourcePin=="true"→trueBranch, sourcePin=="false"→falseBranch
	//   2. If_Node 不使用基类 nextNode（双出口节点用 trueBranch/falseBranch 代替）
	//   3. BuildDataLinks 需要处理 targetPin=="condition"→condition 指针绑定
};

class While_Node : public NODE {  // 蓝图节点类型："While"
public:
	Value* condition = nullptr;
	NODE* loopBody = nullptr;
	Value iterationCount;
	int _count = 0;
	void func_for_VM(ExecutionContext& ctx) override {
		bool cond = (condition && condition->type == ValueType::BOOL) ? condition->b : false;
		if (!cond) {
			_count = 0;
			iterationCount = Value::makeInt(0);
			// ctx.current 已被 RunVM 默认设为 nextNode（退出循环），无需覆盖
		} else {
			iterationCount = Value::makeInt(_count);
			_count++;
			ctx.current = loopBody;
		}
	}
	// ★ 编译注意：
	//   1. BuildExecLinks 需要处理 sourcePin=="loopBody"→loopBody
	//   2. BuildExecLinks 中需将此节点的 loopNode 指向自身（回边到条件判断）
	//   3. 循环体最后一个节点的 nextNode → 指向此节点（循环回边）
	//   4. 循环体最后一个节点的 loopNode → 指向此节点（标记所属循环）
	//   5. 识别"循环体最后一个节点"需要遍历 BlueprintData 的 links 结构
	//   6. BuildDataLinks 需要处理 targetPin=="condition"→condition 指针绑定
};

class Break_Node : public NODE {  // 蓝图节点类型："Break"
public:
	void func_for_VM(ExecutionContext& ctx) override {
		// ctx.current 已被 RunVM 默认设为 nextNode（循环外）
		// 无需额外操作
	}
	// ★ 编译注意：
	//   1. BuildExecLinks 需将此节点的 nextNode → While_Node 的 nextNode（循环外首节点）
	//   2. BuildExecLinks 需将此节点的 loopNode → 所属 While_Node
	//   3. 确定 Break 所属 While 的方法：沿 links 逆推，找到包含此 Break 的 While
};

class Continue_Node : public NODE {  // 蓝图节点类型："Continue"
public:
	void func_for_VM(ExecutionContext& ctx) override {
		ctx.current = loopNode;  // 跳回 While 重新判断条件
	}
	// ★ 编译注意：
	//   1. BuildExecLinks 需将此节点的 loopNode → 所属 While_Node
	//   2. 此节点不使用 nextNode（Continue 走 loopNode，不走 nextNode）
	//   3. 确定 Continue 所属 While 的方法同 Break_Node
};

// ============================================================
// 显示节点
// ============================================================

class PrintText_Node : public NODE {  // 蓝图节点类型："PrintText"
public:
	Value* text = nullptr;
	void func_for_VM(ExecutionContext& ctx) override {
		if (text && text->type == ValueType::STRING) {
			// TODO: 查找当前对象的文本框组件，若存在则更新内容
		}
	}
	// ★ 编译注意：
	//   1. BuildDataLinks 需要处理 targetPin=="text"→text 指针绑定
	//   2. "当前对象" 的绑定方式未定——可能需要额外输入引脚或节点内存储对象引用
};

// ============================================================
// 音频节点（通过 _EventBus 事件总线与 SoundSystem 通信）
// ============================================================

class PlaySound_Node : public NODE {  // 蓝图节点类型："PlaySound"
public:
	Value* path = nullptr;
	Value* loop = nullptr;
	void func_for_VM(ExecutionContext& ctx) override {
		std::string soundPath = (path && path->type == ValueType::STRING) ? path->s : "";
		bool shouldLoop       = (loop && loop->type == ValueType::BOOL)   ? loop->b : false;

		if (!soundPath.empty()) {
			// 通过 _EventBus 发布 PlaySoundEvent，SoundSystem 在构造时已订阅此事件
			// 回调内执行 SoundSystem::playSound(playEvent.path, playEvent.loop)
			// SoundSystem::playSound 使用 Windows MCI API 打开设备并播放
			_EventBus::getInstance().publish_PlaySound(PlaySoundEvent{soundPath, shouldLoop});
		}
		// 单出口节点，不修改 ctx.current，RunVM 自动走 nextNode
	}
	// ★ 编译注意：BuildDataLinks 需处理 targetPin=="path"/"loop"
};

class PauseSound_Node : public NODE {  // 蓝图节点类型："PauseSound"
public:
	Value* path = nullptr;
	void func_for_VM(ExecutionContext& ctx) override {
		std::string soundPath = (path && path->type == ValueType::STRING) ? path->s : "";

		if (!soundPath.empty()) {
			// 通过 _EventBus 发布 PauseSoundEvent
			// SoundSystem 回调内执行 SoundSystem::pauseSound(playEvent.path)
			// pauseSound 会发送 MCI "pause" 命令，并设置 AudioClip::isPaused = true
			_EventBus::getInstance().publish_PauseSound(PauseSoundEvent{soundPath});
		}
		// 单出口节点，不修改 ctx.current，RunVM 自动走 nextNode
	}
	// ★ 编译注意：BuildDataLinks 需处理 targetPin=="path"
};

// ============================================================
// 蓝图变量上下文
// ============================================================

// 蓝图运行时变量表，由 BlueprintCompiler 在编译阶段从 BlueprintData::variables 初始化
// 使用方式：编译器在 Run() 前创建实例，调用 setCurrent()，执行链中 GET_VAR / SET_VAR 即可访问
class BlueprintContext {
public:
	std::unordered_map<std::string, Value> variables;

	// 根据变量名查找，返回指针（不存在则 nullptr）
	Value* getVariable(const std::string& name) {
		auto it = variables.find(name);
		return (it != variables.end()) ? &it->second : nullptr;
	}

	// 写入变量（不存在则创建，已存在则覆盖）
	void setVariable(const std::string& name, const Value& val) {
		variables[name] = val;  // 拷贝写入
	}

	// 获取当前线程的蓝图上下文（线程局部存储，PlayPerNMsNode 等多线程场景安全）
	static BlueprintContext* current() {
		return currentRef();
	}

	// 设置当前线程的蓝图上下文
	static void setCurrent(BlueprintContext* ctx) {
		currentRef() = ctx;
	}

private:
	// 函数级 thread_local，头文件安全（无需 .cpp 定义）
	static BlueprintContext*& currentRef() {
		thread_local BlueprintContext* s_currentContext = nullptr;
		return s_currentContext;
	}
};

// ============================================================
// 变量节点
// ============================================================

class GET_VAR : public NODE {  // 蓝图节点类型："GetVariable"
	// 引脚结构（4个）：
	//   IEXEC    (I, exec)      执行入口
	//   VarToGet (I, string)    要获取的变量名，必定存在字面值，且必定是变量数组中的某个变量名
	//   OEXEC    (O, exec)      执行出口
	//   VarCopy  (O, int/float/string/bool)  变量值副本，类型必定与目标变量相同
public:
	// —— 数据输入 ——
	Value* VarToGet = nullptr;  // 变量名（必定存在字面值，编译期确定）

	// —— 数据输出 ——
	Value VarCopy;   // 变量值副本，初始为 NONE

	void func_for_VM(ExecutionContext& ctx) override {
		std::string name = (VarToGet && VarToGet->type == ValueType::STRING) ? VarToGet->s : "";

		if (!name.empty()) {
			auto* bpCtx = BlueprintContext::current();
			if (bpCtx) {
				Value* stored = bpCtx->getVariable(name);
				if (stored) {
					VarCopy = *stored;  // 拷贝值到输出引脚
				} else {
					VarCopy = Value();  // 变量不存在，输出 NONE
				}
			}
		}
		// 单出口节点，不修改 ctx.current，RunVM 自动走 nextNode
	}
	// ★ 编译注意：
	//   1. BuildDataLinks 需处理 targetPin=="VarToGet"→VarToGet 指针绑定
	//   2. VarToGet 必定存在字面值，InitNodeData 中需从 literal 解析为 Value::makeString(...)
	//   3. BuildDataLinks 中其他节点可能需要从此节点的 VarCopy 读取数据：
	//      绑定方式为 dst->xxx = &GET_VAR->VarCopy（取地址）
	//   4. 节点工厂中类型字符串为 "GetVariable"
	//   5. VarToGet 的字面值必定是 BlueprintData::variables 中的某个变量名
};

class SET_VAR : public NODE {  // 蓝图节点类型："SetVariable"
	// 引脚结构（5个）：
	//   IEXEC    (I, exec)      执行入口
	//   VarToSet (I, string)    要设置的变量名，必定存在字面值，且必定是变量数组中的某个变量名
	//   NewValue (I, int/float/string/bool)  新值，可能来自字面值或其他节点的Link
	//   OEXEC    (O, exec)      执行出口
	//   VarCopy  (O, int/float/string/bool)  设置后的变量值副本，类型必定与目标变量相同
public:
	// —— 数据输入 ——
	Value* VarToSet = nullptr;  // 变量名（必定存在字面值，编译期确定）
	Value* NewValue = nullptr;  // 新值（可能来自字面值或其他节点的 Link）

	// —— 数据输出 ——
	Value VarCopy;   // 设置后的变量值副本

	void func_for_VM(ExecutionContext& ctx) override {
		std::string name = (VarToSet && VarToSet->type == ValueType::STRING) ? VarToSet->s : "";

		if (!name.empty() && NewValue && NewValue->type != ValueType::NONE) {
			auto* bpCtx = BlueprintContext::current();
			if (bpCtx) {
				bpCtx->setVariable(name, *NewValue);  // 拷贝写入变量表
				VarCopy = *NewValue;                   // 输出设置后的值副本
			}
		}
		// 单出口节点，不修改 ctx.current，RunVM 自动走 nextNode
	}
	// ★ 编译注意：
	//   1. BuildDataLinks 需处理 targetPin=="VarToSet"→VarToSet, targetPin=="NewValue"→NewValue
	//   2. VarToSet 必定存在字面值，InitNodeData 中需从 literal 解析为 Value::makeString(...)
	//   3. BuildDataLinks 中其他节点可能需要从此节点的 VarCopy 读取数据：
	//      绑定方式为 dst->xxx = &SET_VAR->VarCopy（取地址）
	//   4. 节点工厂中类型字符串为 "SetVariable"
	//   5. VarToSet 的字面值必定是 BlueprintData::variables 中的某个变量名
	//   6. NewValue 类型必定与目标变量类型相同
};

// ============================================================
// 渲染相关节点
// ============================================================

class Render_Node : public NODE {  // 蓝图节点类型："Render"
public:
	// —— 数据输入 ——
	LevelData* levelData = nullptr;               // 场景指针，不通过 Value 传递
	Value* samplingMethod = nullptr;              // 0=NEAREST, 1=BILINEAR, 2=BICUBIC, 3=ANISOTROPIC
	Value* msaaMultiple = nullptr;

	// —— 数据输出 ——
	Value outFrame;   // 执行成功后覆盖为 FRAME

	void func_for_VM(ExecutionContext& ctx) override {
		if (!levelData) return;   // 无场景数据，跳过渲染

		// 读取采样方法（默认 BICUBIC）
		int methodVal = (samplingMethod && samplingMethod->type == ValueType::INT)
			? samplingMethod->i : 2;

		// 读取 MSAA 倍数（默认 1x）
		int msaaVal = (msaaMultiple && msaaMultiple->type == ValueType::INT)
			? msaaMultiple->i : 1;

		// 整数映射为枚举（防止非法值）
		TextureSamplingMethod method = SAMPLING_BICUBIC;
		switch (methodVal) {
			case 0: method = SAMPLING_NEAREST;    break;
			case 1: method = SAMPLING_BILINEAR;   break;
			case 2: method = SAMPLING_BICUBIC;    break;
			case 3: method = SAMPLING_ANISOTROPIC; break;
			default: method = SAMPLING_BICUBIC;   break;
		}

		// 通过 _EventBus 调用渲染系统
		Frame f = _EventBus::getInstance().publish_Render_A_Frame(
			*levelData, method, msaaVal);
		outFrame = Value::makeFrame(f);

		// 单出口节点，不修改 ctx.current，RunVM 自动走 nextNode
	}
	// ★ 编译注意：
	//   1. BuildDataLinks 需处理 targetPin=="samplingMethod"/"msaaMultiple"
	//   2. levelData 是 LevelData* 直接指针，编译器需要一种方式从 BlueprintData 获取或绑定
	//   3. InitNodeData 无需额外操作（outFrame 是值类型，默认构造即可）
};

class FrameProcess_Node : public NODE {  // 蓝图节点类型："FrameProcess"
public:
	Value* processName = nullptr;
	Value* inFrame = nullptr;
	Value* processParams = nullptr;
	Value outFrame;
	void func_for_VM(ExecutionContext& ctx) override {
		// 检查输入帧有效性
		if (!inFrame || inFrame->type != ValueType::FRAME) {
			outFrame = inFrame ? *inFrame : Value();   // 透传或 NONE
			return;
		}

		// 拷贝帧数据（后处理函数可能就地修改）
		Frame f = inFrame->frame;
		outFrame = Value::makeFrame(f);

		// 读取后处理名称
		std::string name = (processName && processName->type == ValueType::STRING)
			? processName->s : "";

		auto& bus = _EventBus::getInstance();

		// ---- 根据名称分发到对应后处理函数 ----
		if (name == "Bloom") {
			float threshold = 0.8f, intensity = 0.5f;
			int blurRadius = 5;
			float sigma = 1.5f;
			bus.publish_applyBloom(outFrame.frame, threshold, intensity, blurRadius, sigma);
		}
		else if (name == "Blur") {
			int radius = 3; float sigma = 1.0f; int direction = 0;
			bus.publish_applyBlur(outFrame.frame, radius, sigma, direction);
		}
		else if (name == "FXAA") {
			float edgeThreshold = 0.125f, edgeThresholdMin = 0.0625f;
			float spanMax = 8.0f, reduceMul = 0.125f, reduceMin = 0.0078125f;
			bus.publish_applyFXAA(outFrame.frame, edgeThreshold, edgeThresholdMin, spanMax, reduceMul, reduceMin);
		}
		else if (name == "SMAA") {
			float edgeThreshold = 0.1f; int maxSearchSteps = 8; bool enableDiag = true;
			bus.publish_applySMAA(outFrame.frame, edgeThreshold, maxSearchSteps, enableDiag);
		}
		else if (name == "LensDistortion") {
			float strength = 0.5f, centerX = 0.5f, centerY = 0.5f;
			bus.publish_applyLensDistortion(outFrame.frame, strength, centerX, centerY);
		}
		else if (name == "ChromaticAberration") {
			float strength = 0.01f; int mode = 0; float centerX = 0.5f, centerY = 0.5f;
			bus.publish_applyChromaticAberration(outFrame.frame, strength, mode, centerX, centerY);
		}
		else if (name == "Sharpen") {
			float strength = 0.5f; int radius = 1; float sigma = 1.0f;
			bus.publish_applySharpen(outFrame.frame, strength, radius, sigma);
		}
		else if (name == "FilmGrain") {
			float intensity = 0.05f; int grainSize = 1; bool dynamic = true; int frameId = 0;
			bus.publish_applyFilmGrain(outFrame.frame, intensity, grainSize, dynamic, frameId);
		}
		else if (name == "Vignette") {
			float intensity = 0.5f, innerRadius = 0.3f, outerRadius = 0.8f;
			float centerX = 0.5f, centerY = 0.5f, exponent = 1.0f;
			bus.publish_applyVignette(outFrame.frame, intensity, innerRadius, outerRadius, centerX, centerY, exponent);
		}
		else if (name == "ColorCorrection") {
			float brightness = 0.0f, contrast = 0.0f, saturation = 0.0f;
			float3 whiteBalance = {1.0f, 1.0f, 1.0f}; float hueShift = 0.0f;
			bus.publish_applyColorCorrection(outFrame.frame, brightness, contrast, saturation, whiteBalance, hueShift);
		}
		else if (name == "ColorGrading") {
			int style = 0; float intensity = 1.0f; float3 customColor = {1.0f, 1.0f, 1.0f};
			bus.publish_applyColorGrading(outFrame.frame, style, intensity, customColor);
		}
		// 未知名称 → 透传原始帧（outFrame 已是 inFrame 的拷贝，无需额外操作）

		// 单出口节点，不修改 ctx.current，RunVM 自动走 nextNode
	}
	// ★ 编译注意：BuildDataLinks 需处理 targetPin=="processName"/"inFrame"/"processParams"
};

class ShowtheFrame_Node : public NODE {  // 蓝图节点类型："ShowtheFrame"
public:
	Value* inFrame = nullptr;
	void func_for_VM(ExecutionContext& ctx) override {
		if (inFrame && inFrame->type == ValueType::FRAME) {
			_EventBus::getInstance().publish_Print_A_Frame(inFrame->frame);
		}
		// 单出口节点，不修改 ctx.current，RunVM 自动走 nextNode
	}
	// ★ 编译注意：BuildDataLinks 需处理 targetPin=="inFrame"
};

// ============================================================
// 执行引擎
// ============================================================

inline void RunVM(ExecutionContext& ctx) {
	while (ctx.current && ctx.running) {
		NODE* node = ctx.current;
		
		ctx.current = node->nextNode;
		ctx.lastExecuted = node;
		node->func_for_VM(ctx);
	}
}

// PlayWhenKeyNode::onInputEvent 定义
inline void PlayWhenKeyNode::onInputEvent(const InputEvent& e) {
	if (e.type == InputType::KeyDown && e.key == targetKey) {
		ExecutionContext ctx;
		ctx.current = this->nextNode;
		RunVM(ctx);
	}
}
//PlayWhenKeyNode::onInputEvent 里调用了 RunVM(ctx)，而 RunVM 的完整定义在所有节点类之后。
// C++ 要求调用函数时必须能看到完整定义，所以 onInputEvent 的定义必须放在 RunVM 之后
//头文件里在类体外定义方法必须加 inline，否则多个.cpp include 同一个.h 时会报"多重定义"链接错误。