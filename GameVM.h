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
	NODE* loopNode = nullptr;    //这里记录了循环的节点

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
	NODE* trueNode = nullptr;
	NODE* falseNode = nullptr;
	void func_for_VM(ExecutionContext& ctx) override {
		bool cond = (condition && condition->type == ValueType::BOOL) ? condition->b : false;
		ctx.current = cond ? trueNode : falseNode;
	}
	// ★ 编译注意：
	//   1. BuildExecLinks 需要处理 sourcePin=="true"→trueBranch, sourcePin=="false"→falseBranch
	//   2. If_Node 不使用基类 nextNode（双出口节点用 trueBranch/falseBranch 代替）
	//   3. BuildDataLinks 需要处理 targetPin=="condition"→condition 指针绑定
};

class While_Node : public NODE {  // 蓝图节点类型："While"
public:
	Value* condition = nullptr;
	NODE* loopBodyNode = nullptr;
	NODE* loopExitNode = nullptr;
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
			ctx.current = loopBodyNode;
		}
	}
	
};

class Break_Node : public NODE {  // 蓝图节点类型："Break"
public:
	void func_for_VM(ExecutionContext& ctx) override {
		if (loopNode) {
			auto* whileNode = dynamic_cast<While_Node*>(loopNode);
			if (whileNode) {
				ctx.current = whileNode->loopExitNode;// 跳到循环外，不用改执行流
			}
		}

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
// 变量节点
// ============================================================

class GET_VAR : public NODE {  // 蓝图节点类型："GET_VAR"
public:
	Value* varName = nullptr;
	Value outValue;
	void func_for_VM(ExecutionContext& ctx) override {
		// TODO: 需要 BlueprintContext 集成后实现
		// 从蓝图上下文的变量表中按 varName 查找，将值拷贝到 outValue
	}
	// ★ 编译注意：
	//   1. BuildDataLinks 需处理 targetPin=="varName"→varName 指针绑定
	//   2. InitNodeData 无需额外操作（outValue 是值类型，默认构造即可）
	//   3. BuildDataLinks 中其他节点可能需要从此节点的 outValue 读取数据：
	//      绑定方式为 dst->xxx = &GET_VAR->outValue（取地址）
};

class SET_VAR : public NODE {  // 蓝图节点类型："SET_VAR"
public:
	Value* varName  = nullptr;
	Value* inValue  = nullptr;
	void func_for_VM(ExecutionContext& ctx) override {
		// TODO: 需要 BlueprintContext 集成后实现
		// 将 inValue 的值拷贝写入蓝图上下文变量表中 varName 对应的位置
	}
	// ★ 编译注意：
	//   1. BuildDataLinks 需处理 targetPin=="varName"→varName, targetPin=="inValue"→inValue
	//   2. 此节点无数据输出（只有执行流出口 nextNode）
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
		node->func_for_VM(ctx);
		if (!node->nextNode) {
			ctx.running = false;
		}
		ctx.current = node->nextNode;
		ctx.lastExecuted = node;
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