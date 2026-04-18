#include "_EventBus.h"
#include <iostream>
#include<Windows.h>  
/*
==========================================
音频系统事件订阅与发布
==========================================
*/
// 播放音频事件
void _EventBus::subscribe_PlaySound(PlaySound_Handler handler) {
    OutputDebugStringA("=== subscribe_PlaySound called ===\n");
    Handlers_play.push_back(handler);
    OutputDebugStringA(("Handlers_play size now = " + std::to_string(Handlers_play.size()) + "\n").c_str());
}
void _EventBus::publish_PlaySound(const PlaySoundEvent& playEvent) {
    OutputDebugStringA("=== publish_PlaySound called ===\n");
    OutputDebugStringA(("Handler count = " + std::to_string(Handlers_play.size()) + "\n").c_str());
    for (auto& h : Handlers_play) {
        OutputDebugStringA("  invoking handler\n");
        h(playEvent);
    }
}

// 暂停音频事件
void _EventBus::subscribe_PauseSound(PauseSound_Handler handler) {
    Handlers_pause.push_back(handler);
}
void _EventBus::publish_PauseSound(const PauseSoundEvent& playEvent) {
    for (auto& h : Handlers_pause) h(playEvent);
}

// 停止音频事件
void _EventBus::subscribe_StopSound(StopSound_Handler handler) {
    Handlers_stop.push_back(handler);
}
void _EventBus::publish_StopSound(const StopSoundEvent& playEvent) {
    for (auto& h : Handlers_stop) h(playEvent);
}

// 停止所有音频
void _EventBus::subscribe_StopAllSound(StopAllSound_Handler handler) {
    Handlers_stopAll.push_back(handler);
}
void _EventBus::publish_StopAllSound(const StopAllSoundEvent& playEvent) {
    for (auto& h : Handlers_stopAll) h(playEvent);
}

// 设置全局音量
void _EventBus::subscribe_SetVolume(SetVolume_Handler handler) {
    Handlers_volume.push_back(handler);
}
void _EventBus::publish_SetVolume(const SetVolumeEvent& playEvent) {
    for (auto& h : Handlers_volume) h(playEvent);
}

// 设置单个音频音量
void _EventBus::subscribe_SetSoundVolume(SetSoundVolume_Handler handler) {
    Handlers_soundVolume.push_back(handler);
}
void _EventBus::publish_SetSoundVolume(const SetSoundVolumeEvent& playEvent) {
    for (auto& h : Handlers_soundVolume) h(playEvent);
}

// 设置播放速度
void _EventBus::subscribe_SetSpeed(SetSpeed_Handler handler) {
    Handlers_speed.push_back(handler);
}
void _EventBus::publish_SetSpeed(const SetSpeedEvent& playEvent) {
    for (auto& h : Handlers_speed) h(playEvent);
}
/*
==========================================
文件系统事件订阅与发布
==========================================
*/
FolderStructure _EventBus::publish_GetFolderStructure(const char* Directory)
{
    for (auto& handler : handlers_GetFolderStructure)
    {
        return handler(Directory);
    }
}
void _EventBus::subscribe_GetFolderStructure(GetFolderStructure_Handler handler)
{
    handlers_GetFolderStructure.push_back(handler);
}
ProjectStructure _EventBus::publish_GetProjectStructure(const char* RootDirectory)
{
    for (auto& handler : handlers_GetProjectStructure)
    {
        return handler(RootDirectory);
    }
}
void _EventBus::subscribe_GetProjectStructure(GetProjectStructure_Handler handler)
{
    handlers_GetProjectStructure.push_back(handler);
}
BMP_Data _EventBus::publish_ReadBMP(const char* filepath)
{
    for (auto& handler : handlers_ReadBMP)
    {
        return handler(filepath);
    }
}
void _EventBus::subscribe_ReadBMP(ReadBMP_Handler handler)
{
    handlers_ReadBMP.push_back(handler);
}
LevelData _EventBus::publish_ReadLevelData(const std::string& filepath)
{
    for (auto& handler : handlers_ReadLevelData)
    {
        return handler(filepath);
    }
}
void _EventBus::subscribe_ReadLevelData(ReadLevelData_Handler handler)
{
    handlers_ReadLevelData.push_back(handler);
}
void _EventBus::publish_WriteLevelData(const std::string& filepath, const LevelData& data)
{
    for (auto& handler : handlers_WriteLevelData)
    {
        handler(filepath, data);
    }
}
void _EventBus::subscribe_WriteLevelData(WriteLevelData_Handler handler)
{
    handlers_WriteLevelData.push_back(handler);
}
void _EventBus::publish_WriteBPData(const std::string& filepath, const BlueprintData& data)
{
    for (auto& handler : handlers_WriteBPData)
    {
        handler(filepath, data);
    }
}
void _EventBus::subscribe_WriteBPData(WriteBPData_Handler handler)
{
    handlers_WriteBPData.push_back(handler);
}
BlueprintData _EventBus::publish_ReadBPData(const std::string& filepath)
{
    for (auto& handler : handlers_ReadBPData)
    {
        return handler(filepath);
    }
}
void _EventBus::subscribe_ReadBPData(ReadBPData_Handler handler)
{
    handlers_ReadBPData.push_back(handler);
}
/*
==========================================
渲染系统事件订阅与发布
==========================================
*/
void _EventBus::publish_RenderAndPrint(const LevelData& currentLevel, TextureSamplingMethod samplingMethod, int MSAA_Multiple)
{
    for (auto& handler : handlers_RenderAndPrint)
    {
        handler(currentLevel, samplingMethod, MSAA_Multiple);
    }
}
void _EventBus::subscribe_RenderAndPrint(RenderAndPrint_Handler handler)
{
    handlers_RenderAndPrint.push_back(handler);
}
void _EventBus::publish_RenderAndPrint_ANISOTROPIC(const LevelData& currentLevel, int anisoLevel, int MSAA_Multiple)
{
    for (auto& handler : handlers_RenderAndPrint_ANISOTROPIC)
    {
        handler(currentLevel, anisoLevel, MSAA_Multiple);
    }
}
void _EventBus::subscribe_RenderAndPrint_ANISOTROPIC(RenderAndPrint_ANISOTROPIC_Handler handler)
{
    handlers_RenderAndPrint_ANISOTROPIC.push_back(handler);
}
Frame _EventBus::publish_Render_A_Frame(const LevelData& currentLevel, TextureSamplingMethod samplingMethod, int MSAA_Multiple)
{
    for (auto& handler : handlers_Render_A_Frame)
    {
        return handler(currentLevel, samplingMethod, MSAA_Multiple);
    }
}
void _EventBus::subscribe_Render_A_Frame(Render_A_Frame_Handler handler)
{
    handlers_Render_A_Frame.push_back(handler);
}
Frame _EventBus::publish_Render_A_Frame_ANISOTROPIC(const LevelData& currentLevel, int anisoLevel, int MSAA_Multiple)
{
    for (auto& handler : handlers_Render_A_Frame_ANISOTROPIC)
    {
        return handler(currentLevel, anisoLevel, MSAA_Multiple);
    }
}
void _EventBus::subscribe_Render_A_Frame_ANISOTROPIC(Render_A_Frame_ANISOTROPIC_Handler handler)
{
    handlers_Render_A_Frame_ANISOTROPIC.push_back(handler);
}
void _EventBus::publish_Print_A_Frame(const Frame& frame)
{
    for (auto& handler : handlers_Print_A_Frame)
    {
        handler(frame);
    }
}
void _EventBus::subscribe_Print_A_Frame(Print_A_Frame_Handler handler)
{
    handlers_Print_A_Frame.push_back(handler);
}
void _EventBus::publish_applyBloom(Frame& frame,
    float threshold,
    float intensity,
    int blurRadius,
    float sigma)
{
    for (auto& handler : handlers_applyBloom)
    {
        handler(frame, threshold, intensity, blurRadius, sigma);
    }
}
void _EventBus::subscribe_applyBloom(applyBloom_Handler handler)
{
    handlers_applyBloom.push_back(handler);
}
void _EventBus::publish_applyBlur(Frame& frame, int radius, float sigma, int direction)
{
    for (auto& handler : handlers_applyBlur)
    {
        handler(frame, radius, sigma, direction);
    }
}
void _EventBus::subscribe_applyBlur(applyBlur_Handler handler)
{
    handlers_applyBlur.push_back(handler);
}
void _EventBus::publish_applyChromaticAberration(Frame& frame, float strength, int mode, float centerX, float centerY)
{
    for (auto& handler : handlers_applyChromaticAberration)
    {
        handler(frame, strength, mode, centerX, centerY);
    }
}
void _EventBus::subscribe_applyChromaticAberration(applyChromaticAberration_Handler handler)
{
    handlers_applyChromaticAberration.push_back(handler);
}
void _EventBus::publish_applyColorCorrection(Frame& frame,
    float brightness,
    float contrast,
    float saturation,
    float3 whiteBalance,
    float hueShift)
{
    for (auto& handler : handlers_applyColorCorrection)
    {
        handler(frame, brightness, contrast, saturation, whiteBalance, hueShift);
    }
}
void _EventBus::subscribe_applyColorCorrection(applyColorCorrection_Handler handler)
{
    handlers_applyColorCorrection.push_back(handler);
}
void _EventBus::publish_applyColorGrading(Frame& frame, int style, float intensity, float3 customColor)
{
    for (auto& handler : handlers_applyColorGrading)
    {
        handler(frame, style, intensity, customColor);
    }
}
void _EventBus::subscribe_applyColorGrading(applyColorGrading_Handler handler)
{
    handlers_applyColorGrading.push_back(handler);
}
void _EventBus::publish_applyFXAA(Frame& frame,
    float edgeThreshold,
    float edgeThresholdMin,
    float spanMax,
    float reduceMul,
    float reduceMin)
{
    for (auto& handler : handlers_applyFXAA)
    {
        handler(frame, edgeThreshold, edgeThresholdMin, spanMax, reduceMul, reduceMin);
    }
}
void _EventBus::subscribe_applyFXAA(applyFXAA_Handler handler)
{
    handlers_applyFXAA.push_back(handler);
}
void _EventBus::publish_applyFilmGrain(Frame& frame, float intensity, int grainSize, bool dynamic, int frameId)
{
    for (auto& handler : handlers_applyFilmGrain)
    {
        handler(frame, intensity, grainSize, dynamic, frameId);
    }
}
void _EventBus::subscribe_applyFilmGrain(applyFilmGrain_Handler handler)
{
    handlers_applyFilmGrain.push_back(handler);
}
void _EventBus::publish_applyLensDistortion(Frame& frame, float strength, float centerX, float centerY)
{
    for (auto& handler : handlers_applyLensDistortion)
    {
        handler(frame, strength, centerX, centerY);
    }
}
void _EventBus::subscribe_applyLensDistortion(applyLensDistortion_Handler handler)
{
    handlers_applyLensDistortion.push_back(handler);
}
void _EventBus::publish_applySMAA(Frame& frame,
    float edgeThreshold,
    int maxSearchSteps,
    bool enableDiag)
{
    for (auto& handler : handlers_applySMAA)
    {
        handler(frame, edgeThreshold, maxSearchSteps, enableDiag);
    }
}
void _EventBus::subscribe_applySMAA(applySMAA_Handler handler)
{
    handlers_applySMAA.push_back(handler);
}
void _EventBus::publish_applySharpen(Frame& frame, float strength, int radius, float sigma)
{
    for (auto& handler : handlers_applySharpen)
    {
        handler(frame, strength, radius, sigma);
    }
}
void _EventBus::subscribe_applySharpen(applySharpen_Handler handler)
{
    handlers_applySharpen.push_back(handler);
}
void _EventBus::publish_applyVignette(Frame& frame, float intensity, float innerRadius, float outerRadius,
    float centerX, float centerY, float exponent)
{
    for (auto& handler : handlers_applyVignette)
    {
        handler(frame, intensity, innerRadius, outerRadius, centerX, centerY, exponent);
    }
}
void _EventBus::subscribe_applyVignette(applyVignette_Handler handler)
{
    handlers_applyVignette.push_back(handler);
}