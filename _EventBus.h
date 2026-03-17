#pragma once
#include <functional>
#include <vector>
#include "InputEvent.h"

class _EventBus {
	//使用单例模式
public:
	_EventBus(const _EventBus&) = delete;
	_EventBus& operator=(const _EventBus&) = delete;

	static _EventBus& getInstance() {
		static _EventBus instance;
		return instance;
	}

	using SoundPlay_Handler = std::function < bool(const char*) >;		//bool表示是否成功，char*则指向音频文件路径，视实际情况更改
	using InputEvent_Handler = std::function <void(const InputEvent&)>;   //这是我的输入系统的函数对应的储存标签
	//其它事件类型，请自行添加

	// 订阅播放音频事件
	void subscribe_SoundPlay(SoundPlay_Handler handler);
	// 发布播放音频事件（通知所有订阅者）
	void publish_SoundPlay(const char* sound_path);

	void subscribe_InputEvent(InputEvent_Handler handler);
	void publish_InputEvent(const InputEvent& event);
	// 其它事件订阅和发布函数，请像上面两个函数一样自行添加

private:
	_EventBus() = default;
	~_EventBus() = default;

	std::vector<SoundPlay_Handler> handlers_SoundPlay;  // 所有播放音频事件订阅者，按理来说只有 音频系统 会订阅
	std::vector<InputEvent_Handler> handlers_InputEvent;  //输入事件订阅者存放的容器
	// 其它订阅者类型，请像上面一样自行添加
};