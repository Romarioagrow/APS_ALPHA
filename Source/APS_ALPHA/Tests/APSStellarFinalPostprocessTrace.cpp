// Read-only, explicitly started trace of the final blended gameplay view.
#if WITH_EDITOR && WITH_DEV_AUTOMATION_TESTS
#include "Editor.h"
#include "Engine/World.h"
#include "HAL/FileManager.h"
#include "HAL/IConsoleManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "SceneView.h"
#include "SceneViewExtension.h"

namespace APSStellarFinalTrace
{
class FTrace final : public FWorldSceneViewExtension
{
public:
    FTrace(const FAutoRegister& Register, UWorld* World)
        : FWorldSceneViewExtension(Register, World), Started(FPlatformTime::Seconds())
    {
        File = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("Diagnostics"),
            TEXT("StellarFinalPP-") + FDateTime::UtcNow().ToString(TEXT("%Y%m%d-%H%M%S")) + TEXT(".csv"));
        Lines.Reserve(4096);
        Lines.Add(TEXT("time,pitch,yaw,roll,width,height,aa,cameraCut,exposureMode,minEV,maxEV,bias,physical,shutter,iso,fstop,motionBlur,motionBlurMax,bloom,bloomThreshold,localHighlights,localShadows,localDetail,localBlurBlend,localGrey,flagPP,flagMotion,flagEye,flagLocal,flagBloom"));
    }
    virtual void SetupViewFamily(FSceneViewFamily&) override {}
    virtual void BeginRenderViewFamily(FSceneViewFamily&) override {}
    virtual void SetupView(FSceneViewFamily& Family, FSceneView& View) override
    {
        if (Done || View.bIsSceneCapture) return;
        const double Elapsed = FPlatformTime::Seconds() - Started;
        if (Elapsed >= 30.0 || Lines.Num() >= 4000) { Flush(); return; }
        const FPostProcessSettings& P = View.FinalPostProcessSettings;
        const FRotator R = View.ViewRotation;
        const FEngineShowFlags& S = Family.EngineShowFlags;
        Lines.Add(FString::Printf(TEXT("%.9f,%.9f,%.9f,%.9f,%d,%d,%d,%d,%d,%.9g,%.9g,%.9g,%d,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%d,%d,%d,%d,%d"),
            Elapsed, R.Pitch, R.Yaw, R.Roll, View.UnscaledViewRect.Width(), View.UnscaledViewRect.Height(),
            static_cast<int32>(View.AntiAliasingMethod), View.bCameraCut ? 1 : 0, static_cast<int32>(P.AutoExposureMethod),
            P.AutoExposureMinBrightness, P.AutoExposureMaxBrightness, P.AutoExposureBias,
            P.AutoExposureApplyPhysicalCameraExposure ? 1 : 0, P.CameraShutterSpeed, P.CameraISO, P.DepthOfFieldFstop,
            P.MotionBlurAmount, P.MotionBlurMax, P.BloomIntensity, P.BloomThreshold,
            P.LocalExposureHighlightContrastScale, P.LocalExposureShadowContrastScale, P.LocalExposureDetailStrength,
            P.LocalExposureBlurredLuminanceBlend, P.LocalExposureMiddleGreyBias,
            S.PostProcessing ? 1 : 0, S.MotionBlur ? 1 : 0, S.EyeAdaptation ? 1 : 0,
            S.LocalExposure ? 1 : 0, S.Bloom ? 1 : 0));
    }
    void Flush()
    {
        if (Done) return;
        Done = true;
        IFileManager::Get().MakeDirectory(*FPaths::GetPath(File), true);
        const bool Saved = FFileHelper::SaveStringArrayToFile(Lines, *File, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
        UE_LOG(LogTemp, Display, TEXT("[APS.FinalPPTrace] finished samples=%d saved=%d file=%s; no view settings changed"),
            Lines.Num() - 1, Saved ? 1 : 0, *File);
        Lines.Reset();
    }
protected:
    virtual bool IsActiveThisFrame_Internal(const FSceneViewExtensionContext& Context) const override
    {
        return !Done && FWorldSceneViewExtension::IsActiveThisFrame_Internal(Context);
    }
private:
    const double Started;
    bool Done = false;
    FString File;
    TArray<FString> Lines;
};
static TSharedPtr<FTrace, ESPMode::ThreadSafe> Trace;
static void Start()
{
    UWorld* World = GEditor ? GEditor->PlayWorld : nullptr;
    if (!World || World->WorldType != EWorldType::PIE || World->GetName() != TEXT("L_WorldGeneration")) return;
    if (Trace) Trace->Flush();
    Trace = FSceneViewExtensions::NewExtension<FTrace>(World);
    UE_LOG(LogTemp, Display, TEXT("[APS.FinalPPTrace] Started 30-second read-only final view trace."));
}
static void Stop()
{
    if (Trace) Trace->Flush();
    Trace.Reset();
}
static FAutoConsoleCommand StartCommand(TEXT("APS.Diagnostics.StartFinalPostprocessTrace"),
    TEXT("Read the final blended gameplay view for 30 seconds without changing it; editor only."),
    FConsoleCommandDelegate::CreateStatic(&Start), ECVF_Cheat);
static FAutoConsoleCommand StopCommand(TEXT("APS.Diagnostics.StopFinalPostprocessTrace"),
    TEXT("Flush and detach the read-only final view trace."),
    FConsoleCommandDelegate::CreateStatic(&Stop), ECVF_Cheat);
}
#endif
