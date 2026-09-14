// Opt-in editor diagnosis, no image readback, Present/Flush/Wait or render-setting writes.
#if WITH_EDITOR && WITH_DEV_AUTOMATION_TESTS && PLATFORM_WINDOWS
#include "Async/Async.h"
#include "Camera/PlayerCameraManager.h"
#include "Containers/Queue.h"
#include "Containers/Ticker.h"
#include "Editor.h"
#include "DynamicRHI.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "Framework/Application/SlateApplication.h"
#include "GameFramework/PlayerController.h"
#include "HAL/FileManager.h"
#include "HAL/IConsoleManager.h"
#include "Misc/CoreDelegates.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Rendering/SlateRenderer.h"
#include "RenderingThread.h"
#include "RHIResources.h"
#include "Windows/WindowsHWrapper.h"
#include "Windows/AllowWindowsPlatformTypes.h"
#include <dxgi.h>
#include "Windows/HideWindowsPlatformTypes.h"
#include "Widgets/SWindow.h"

namespace APSStellarPresentationTrace
{
static int64 Qpc()
{
    LARGE_INTEGER Value;
    ::QueryPerformanceCounter(&Value);
    return Value.QuadPart;
}
struct FInputSample
{
    int64 Time = 0;
    float MouseX = 0, MouseY = 0;
    FRotator Rotation = FRotator::ZeroRotator;
};
struct FRow
{
    int64 Time = 0;
    int64 QueryTicks = 0;
    FInputSample Input;
    uint32 StatsResult = 0, CountResult = 0, LastPresent = 0;
    DXGI_FRAME_STATISTICS Stats{};
};
struct FProbe
{
    TWeakPtr<SWindow> Window;
    const SWindow* Target = nullptr; // identity only, never dereferenced on render thread
    TWeakObjectPtr<UWorld> World;
    // Only retain the RHI object, never a raw swap-chain across a resize.
    FViewportRHIRef Viewport;
    FDelegateHandle RenderHandle, InputHandle;
    FTSTicker::FDelegateHandle TickerHandle;
    TQueue<FInputSample, EQueueMode::Spsc> Inputs;
    TArray<FRow> Rows; // exclusively render-thread-owned until final render command
    FInputSample Latest;
    double Started = FPlatformTime::Seconds();
    int64 Frequency = 0;
    DXGI_SWAP_CHAIN_DESC Description{};
    uint32 DescriptionResult = 0xffffffff;
    bool HaveDescription = false;
    void ReadInput()
    {
        UWorld* W = World.Get();
        APlayerController* PC = W ? W->GetFirstPlayerController() : nullptr;
        if (!PC || !PC->PlayerCameraManager) return;
        FInputSample Input;
        Input.Time = Qpc();
        PC->GetInputMouseDelta(Input.MouseX, Input.MouseY);
        Input.Rotation = PC->PlayerCameraManager->GetCameraRotation();
        Inputs.Enqueue(Input);
    }
    void ReadPresentation(SWindow& PresentedWindow, const FTexture2DRHIRef&)
    {
        if (Target != &PresentedWindow || Rows.Num() >= 20000
            || FPlatformTime::Seconds() - Started > 45.0) return;
        while (Inputs.Dequeue(Latest)) {}
        FRow Row;
        Row.Input = Latest;
        Row.Time = Qpc();
        // Start is D3D12-gated; its GetNativeSwapChain returns IDXGISwapChain1*.
        IDXGISwapChain* Chain = static_cast<IDXGISwapChain*>(Viewport->GetNativeSwapChain());
        if (!Chain) return;
        if (!HaveDescription)
        {
            DescriptionResult = static_cast<uint32>(Chain->GetDesc(&Description));
            HaveDescription = true;
        }
        Row.StatsResult = static_cast<uint32>(Chain->GetFrameStatistics(&Row.Stats));
        Row.CountResult = static_cast<uint32>(Chain->GetLastPresentCount(&Row.LastPresent));
        Row.QueryTicks = Qpc() - Row.Time;
        Rows.Add(Row);
    }
    void Save()
    {
        // Called only after the final render command; no live game or render data accessed.
        TArray<FString> Lines;
        Lines.Reserve(Rows.Num() + 3);
        Lines.Add(FString::Printf(TEXT("# qpcFrequency=%lld,descResult=%u,swapEffect=%u,windowed=%d,width=%u,height=%u,refreshNumerator=%u,refreshDenominator=%u,bufferCount=%u,flags=%u"),
            Frequency, DescriptionResult, static_cast<uint32>(Description.SwapEffect),
            Description.Windowed, Description.BufferDesc.Width, Description.BufferDesc.Height,
            Description.BufferDesc.RefreshRate.Numerator, Description.BufferDesc.RefreshRate.Denominator,
            Description.BufferCount, Description.Flags));
        Lines.Add(TEXT("qpc,queryTicks,inputQpc,mouseX,mouseY,pitch,yaw,roll,statsHRESULT,lastCountHRESULT,lastPresentCount,presentCount,presentRefreshCount,syncRefreshCount,syncQpc"));
        for (const FRow& R : Rows)
            Lines.Add(FString::Printf(TEXT("%lld,%lld,%lld,%.9g,%.9g,%.9g,%.9g,%.9g,%u,%u,%u,%u,%u,%u,%lld"),
                R.Time, R.QueryTicks, R.Input.Time, R.Input.MouseX, R.Input.MouseY,
                R.Input.Rotation.Pitch, R.Input.Rotation.Yaw, R.Input.Rotation.Roll,
                R.StatsResult, R.CountResult, R.LastPresent, R.Stats.PresentCount,
                R.Stats.PresentRefreshCount, R.Stats.SyncRefreshCount, R.Stats.SyncQPCTime.QuadPart));
        const FString Directory = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("Diagnostics"));
        IFileManager::Get().MakeDirectory(*Directory, true);
        const FString File = FPaths::Combine(Directory, TEXT("StellarPresentation-")
            + FDateTime::UtcNow().ToString(TEXT("%Y%m%d-%H%M%S")) + TEXT(".csv"));
        const bool Saved = FFileHelper::SaveStringArrayToFile(Lines, *File,
            FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
        UE_LOG(LogTemp, Display, TEXT("[APS.PresentationTrace] samples=%d saved=%d file=%s; no image or settings changed"), Rows.Num(), Saved, *File);
    }
};
static TSharedPtr<FProbe, ESPMode::ThreadSafe> Active;
static void Stop()
{
    if (!Active) return;
    TSharedPtr<FProbe, ESPMode::ThreadSafe> Finished = MoveTemp(Active);
    FCoreDelegates::OnEndFrame.Remove(Finished->InputHandle);
    FTSTicker::GetCoreTicker().RemoveTicker(Finished->TickerHandle);
    FSlateApplication::Get().GetRenderer()->OnBackBufferReadyToPresent().Remove(Finished->RenderHandle);
    ENQUEUE_RENDER_COMMAND(APSFinishPresentationTrace)([Finished](FRHICommandListImmediate&)
    {
        Finished->Viewport.SafeRelease();
        Async(EAsyncExecution::ThreadPool, [Finished]() { Finished->Save(); });
    });
}
static void Start()
{
    if (Active || !FSlateApplication::IsInitialized() || !GDynamicRHI
        || FString(GDynamicRHI->GetName()) != TEXT("D3D12")) return;
    UWorld* W = GEditor ? GEditor->PlayWorld : nullptr;
    if (!W || W->WorldType != EWorldType::PIE || !W->GetGameViewport()) return;
    TSharedPtr<SWindow> Window = W->GetGameViewport()->GetWindow();
    if (!Window) return;
    // Slate's implementation returns &ViewportInfo->ViewportRHI.
    void* Resource = FSlateApplication::Get().GetRenderer()->GetViewportResource(*Window);
    if (!Resource || !static_cast<FViewportRHIRef*>(Resource)->IsValid()) return;
    Active = MakeShared<FProbe, ESPMode::ThreadSafe>();
    Active->World = W;
    Active->Window = Window;
    Active->Target = Window.Get();
    Active->Viewport = *static_cast<FViewportRHIRef*>(Resource);
    Active->Rows.Reserve(20000);
    LARGE_INTEGER Frequency;
    ::QueryPerformanceFrequency(&Frequency);
    Active->Frequency = Frequency.QuadPart;
    Active->InputHandle = FCoreDelegates::OnEndFrame.AddSP(Active.ToSharedRef(), &FProbe::ReadInput);
    Active->RenderHandle = FSlateApplication::Get().GetRenderer()->OnBackBufferReadyToPresent().AddSP(
        Active.ToSharedRef(), &FProbe::ReadPresentation);
    Active->TickerHandle = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([](float)
    {
        if (!Active) return false;
        if (!Active->World.IsValid() || GEditor->PlayWorld != Active->World.Get()
            || !Active->Window.IsValid() || FPlatformTime::Seconds() - Active->Started >= 45.0)
        { Stop(); return false; }
        return true;
    }));
    UE_LOG(LogTemp, Display, TEXT("[APS.PresentationTrace] Read-only 45-second DXGI/input observation started; unsupported/disjoint results will remain explicit."));
}
static FAutoConsoleCommand StartCommand(TEXT("APS.Diagnostics.StartPresentationTrace"),
    TEXT("Observe only the current PIE D3D12 swap-chain counters and input for45seconds. No capture, flush, wait, admin or settings changes."),
    FConsoleCommandDelegate::CreateStatic(&Start), ECVF_Cheat);
static FAutoConsoleCommand StopCommand(TEXT("APS.Diagnostics.StopPresentationTrace"),
    TEXT("Detach the optional presentation observer and asynchronously save its bounded report."),
    FConsoleCommandDelegate::CreateStatic(&Stop), ECVF_Cheat);
}
#endif
