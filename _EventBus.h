#pragma once
#include <functional>
#include <vector>
#include "SharedTypes.h"		//结构体统一定义头文件，其中包含了所有需要用于系统间通信的结构体
#include "InputEvent.h"
#include "SoundEvents.h"

class _EventBus {
    //使用单例模式
public:
    _EventBus(const _EventBus&) = delete;
    _EventBus& operator=(const _EventBus&) = delete;

    static _EventBus& getInstance() {
        static _EventBus instance;
        return instance;
    }
    /*
    ==========================================
    音频系统事件
    ==========================================
    */
    //其它事件类型，请自行添加

    // ===== 音频委托类型 =====
    using PlaySound_Handler = std::function<void(const PlaySoundEvent&)>;
    using PauseSound_Handler = std::function<void(const PauseSoundEvent&)>;
    using StopSound_Handler = std::function<void(const StopSoundEvent&)>;
    using StopAllSound_Handler = std::function<void(const StopAllSoundEvent&)>;
    using SetVolume_Handler = std::function<void(const SetVolumeEvent&)>;
    using SetSoundVolume_Handler = std::function<void(const SetSoundVolumeEvent&)>;
    using SetSpeed_Handler = std::function<void(const SetSpeedEvent&)>;

    // ===== 音频订阅 =====
    void subscribe_PlaySound(PlaySound_Handler handler);
    void subscribe_PauseSound(PauseSound_Handler handler);
    void subscribe_StopSound(StopSound_Handler handler);
    void subscribe_StopAllSound(StopAllSound_Handler handler);
    void subscribe_SetVolume(SetVolume_Handler handler);
    void subscribe_SetSoundVolume(SetSoundVolume_Handler handler);
    void subscribe_SetSpeed(SetSpeed_Handler handler);

    // ===== 音频发布 =====
    void publish_PlaySound(const PlaySoundEvent& playEvent);
    void publish_PauseSound(const PauseSoundEvent& playEvent);
    void publish_StopSound(const StopSoundEvent& playEvent);
    void publish_StopAllSound(const StopAllSoundEvent& playEvent);      
    void publish_SetVolume(const SetVolumeEvent& playEvent);
    void publish_SetSoundVolume(const SetSoundVolumeEvent& playEvent);
    void publish_SetSpeed(const SetSpeedEvent& playEvent);

    /*
    ==========================================
    文件系统事件
    ==========================================
    */
    using ReadLevelData_Handler = std::function < LevelData(const std::string&) >;
    using WriteLevelData_Handler = std::function < bool(const std::string&, const LevelData&) >;
    using GetProjectStructure_Handler = std::function < ProjectStructure(const char*) >;
    using GetFolderStructure_Handler = std::function < FolderStructure(const char*) >;
    using ReadBMP_Handler = std::function < BMP_Data(const char*) >;
    using WriteBPData_Handler = std::function < bool(const std::string&, const BlueprintData&) >;
    using ReadBPData_Handler = std::function < BlueprintData(const std::string&) >;
    /*
    ==========================================
    输入处理系统事件
    ==========================================
    */
	using InputEvent_Handler = std::function <void(const InputEvent&)>;
    /*
    ==========================================
    渲染系统事件
    ==========================================
    */
    using RenderAndPrint_Handler = std::function < void(const LevelData&, TextureSamplingMethod, int) >;
    using RenderAndPrint_ANISOTROPIC_Handler = std::function < void(const LevelData&, int, int) >;
    using Render_A_Frame_Handler = std::function<Frame (const LevelData&, TextureSamplingMethod, int)>;
    using Render_A_Frame_ANISOTROPIC_Handler = std::function<Frame(const LevelData&, int, int)>;
    using Print_A_Frame_Handler = std::function<void(const Frame&)>;
    //后处理
    using applyBloom_Handler = std::function<void(Frame&, float, float, int, float)>;
    using applyBlur_Handler = std::function<void(Frame&, int, float, int)>;
    using applyFXAA_Handler = std::function<void(Frame&, float, float, float, float, float)>;
    using applySMAA_Handler = std::function<void(Frame&, float, int, bool)>;
    using applyLensDistortion_Handler = std::function<void(Frame&, float, float, float)>;
    using applyChromaticAberration_Handler = std::function<void(Frame&, float, int, float, float)>;
    using applySharpen_Handler = std::function<void(Frame&, float, int, float)>;
    using applyFilmGrain_Handler = std::function<void(Frame&, float, int, bool, int)>;
    using applyVignette_Handler = std::function<void(Frame&, float, float, float, float, float, float)>;
    using applyColorCorrection_Handler = std::function<void(Frame&, float, float, float, float3, float)>;
    using applyColorGrading_Handler = std::function<void(Frame&, int, float, float3)>;


    void subscribe_ReadLevelData(ReadLevelData_Handler handler);
    void subscribe_WriteLevelData(WriteLevelData_Handler handler);
    void subscribe_GetProjectStructure(GetProjectStructure_Handler handler);
    void subscribe_GetFolderStructure(GetFolderStructure_Handler handler);
    void subscribe_ReadBMP(ReadBMP_Handler handler);
    void subscribe_ReadBPData(ReadBPData_Handler handler);
    void subscribe_WriteBPData(WriteBPData_Handler handler);
    BMP_Data publish_ReadBMP(const char* filepath);
    void publish_WriteLevelData(const std::string& filepath, const LevelData& data);
    LevelData publish_ReadLevelData(const std::string& filepath);
    ProjectStructure publish_GetProjectStructure(const char* RootDirectory);
    FolderStructure publish_GetFolderStructure(const char* Directory);
    BlueprintData publish_ReadBPData(const std::string& filepath);
    void publish_WriteBPData(const std::string& filepath, const BlueprintData& data);

	
    void subscribe_RenderAndPrint(RenderAndPrint_Handler handler);
    void publish_RenderAndPrint(const LevelData& currentLevel, TextureSamplingMethod samplingMethod, int MSAA_Multiple);
    void subscribe_RenderAndPrint_ANISOTROPIC(RenderAndPrint_ANISOTROPIC_Handler handler);
    void publish_RenderAndPrint_ANISOTROPIC(const LevelData& currentLevel, int anisoLevel, int MSAA_Multiple);
    void subscribe_Render_A_Frame(Render_A_Frame_Handler handler);
    Frame publish_Render_A_Frame(const LevelData& currentLevel, TextureSamplingMethod samplingMethod, int MSAA_Multiple);
    void subscribe_Render_A_Frame_ANISOTROPIC(Render_A_Frame_ANISOTROPIC_Handler handler);
    Frame publish_Render_A_Frame_ANISOTROPIC(const LevelData& currentLevel, int anisoLevel, int MSAA_Multiple);
    void subscribe_Print_A_Frame(Print_A_Frame_Handler handler);
    void publish_Print_A_Frame(const Frame& frame);
    void subscribe_applyBloom(applyBloom_Handler handler);
    void publish_applyBloom(Frame& frame,
        float threshold,
        float intensity,
        int blurRadius,
        float sigma);
    void subscribe_applyBlur(applyBlur_Handler handler);
    void publish_applyBlur(Frame& frame, int radius, float sigma, int direction);
    void subscribe_applyFXAA(applyFXAA_Handler handler);
    void publish_applyFXAA(Frame& frame,
        float edgeThreshold,
        float edgeThresholdMin,
        float spanMax,
        float reduceMul,
        float reduceMin);
    void subscribe_applySMAA(applySMAA_Handler handler);
    void publish_applySMAA(Frame& frame,
        float edgeThreshold,
        int maxSearchSteps,
        bool enableDiag);
    void subscribe_applyLensDistortion(applyLensDistortion_Handler handler);
    void publish_applyLensDistortion(Frame& frame, float strength, float centerX, float centerY);
    void subscribe_applyChromaticAberration(applyChromaticAberration_Handler handler);
    void publish_applyChromaticAberration(Frame& frame, float strength, int mode, float centerX, float centerY);
    void subscribe_applySharpen(applySharpen_Handler handler);
    void publish_applySharpen(Frame& frame, float strength, int radius, float sigma);
    void subscribe_applyFilmGrain(applyFilmGrain_Handler handler);
    void publish_applyFilmGrain(Frame& frame, float intensity, int grainSize, bool dynamic, int frameId);
    void subscribe_applyVignette(applyVignette_Handler handler);
    void publish_applyVignette(Frame& frame, float intensity, float innerRadius, float outerRadius,
        float centerX, float centerY, float exponent);
    void subscribe_applyColorCorrection(applyColorCorrection_Handler handler);
    void publish_applyColorCorrection(Frame& frame,
        float brightness,
        float contrast,
        float saturation,
        float3 whiteBalance,
        float hueShift);
    void subscribe_applyColorGrading(applyColorGrading_Handler handler);
    void publish_applyColorGrading(Frame& frame, int style, float intensity, float3 customColor);


private:
    _EventBus() = default;
    ~_EventBus() = default;
    
    // ===== 委托容器 ====
    std::vector<PlaySound_Handler> Handlers_play;
    std::vector<PauseSound_Handler> Handlers_pause;
    std::vector<StopSound_Handler> Handlers_stop;
    std::vector<StopAllSound_Handler> Handlers_stopAll;
    std::vector<SetVolume_Handler> Handlers_volume;
    std::vector<SetSoundVolume_Handler> Handlers_soundVolume;
    std::vector<SetSpeed_Handler> Handlers_speed;
   
    // 所有播放音频事件订阅者，按理来说只有 音频系统 会订阅

    std::vector<ReadLevelData_Handler> handlers_ReadLevelData;
    std::vector<WriteLevelData_Handler> handlers_WriteLevelData;
    std::vector<GetProjectStructure_Handler> handlers_GetProjectStructure;
    std::vector<GetFolderStructure_Handler> handlers_GetFolderStructure;
    std::vector<ReadBMP_Handler> handlers_ReadBMP;
    std::vector<WriteBPData_Handler> handlers_WriteBPData;
    std::vector<ReadBPData_Handler> handlers_ReadBPData;

	

	std::vector<InputEvent_Handler> handlers_InputEvent;  //输入事件订阅者存放的容器

    std::vector<RenderAndPrint_Handler> handlers_RenderAndPrint;
    std::vector<RenderAndPrint_ANISOTROPIC_Handler> handlers_RenderAndPrint_ANISOTROPIC;
    std::vector<Render_A_Frame_Handler> handlers_Render_A_Frame;
    std::vector<Render_A_Frame_ANISOTROPIC_Handler> handlers_Render_A_Frame_ANISOTROPIC;
    std::vector<Print_A_Frame_Handler> handlers_Print_A_Frame;
    std::vector<applyBloom_Handler> handlers_applyBloom;
    std::vector<applyBlur_Handler> handlers_applyBlur;
    std::vector<applyChromaticAberration_Handler> handlers_applyChromaticAberration;
    std::vector<applyColorCorrection_Handler> handlers_applyColorCorrection;
    std::vector<applyColorGrading_Handler> handlers_applyColorGrading;
    std::vector<applyFXAA_Handler> handlers_applyFXAA;
    std::vector<applyFilmGrain_Handler> handlers_applyFilmGrain;
    std::vector<applyLensDistortion_Handler> handlers_applyLensDistortion;
    std::vector<applySMAA_Handler> handlers_applySMAA;
    std::vector<applySharpen_Handler> handlers_applySharpen;
    std::vector<applyVignette_Handler> handlers_applyVignette;
	// 其它订阅者类型，请像上面一样自行添加
};