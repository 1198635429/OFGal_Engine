#include "_EventBus.h"
#include <iostream>

void _EventBus::publish_SoundPlay(const char* sound_path)
{
    for (auto& handler : handlers_SoundPlay)
    {
        handler(sound_path);
    }
}
void _EventBus::subscribe_SoundPlay(SoundPlay_Handler handler)
{
    handlers_SoundPlay.push_back(handler);
}
void _EventBus::publish_InputEvent(const InputEvent& event) {
    for (auto& handler : handlers_InputEvent) {
        handler(event);
    }
}
void _EvenBus::subscribe_InputEvent(InputEvent_Handler handler) {
    handlers_InputEvent.push_back(handler);
}
