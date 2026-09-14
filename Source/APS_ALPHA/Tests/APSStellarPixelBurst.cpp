// Opt-in, bounded copy of a sky-only PIE backbuffer rectangle at presentation rate.
// No SceneCapture, screenshot flag, camera/AA/exposure changes or GPU waits while sampling.
#if WITH_EDITOR && WITH_DEV_AUTOMATION_TESTS && PLATFORM_WINDOWS
#include "Async/Async.h"
#include "Containers/Ticker.h"
#include "Editor.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "Framework/Application/SlateApplication.h"
#include "HAL/FileManager.h"
#include "HAL/IConsoleManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Rendering/SlateRenderer.h"
#include "RenderingThread.h"
#include "RHIGPUReadback.h"
#include "Serialization/Archive.h"
#include "Windows/WindowsHWrapper.h"
#include "Widgets/SWindow.h"

namespace APSStellarPixelBurst
{
static constexpr int32 Width=512, Height=384, MaxFrames=768, SlotCount=8;
static constexpr int32 FrameBytes=Width*Height*4;
static int64 Qpc() { LARGE_INTEGER T; ::QueryPerformanceCounter(&T); return T.QuadPart; }
struct FFrame
{
    int32 Sequence=0; uint32 RenderFrame=0;
    int64 CopyQpc=0, ReadyQpc=0;
    TArray<uint8> Bytes;
};
struct FSlot
{
    TUniquePtr<FRHIGPUTextureReadback> Readback;
    FFrame Frame;
    bool Pending=false;
};
struct FProbe
{
    TWeakObjectPtr<UWorld> World;
    TWeakPtr<SWindow> Window;
    const SWindow* Target=nullptr;
    FDelegateHandle Handle;
    FTSTicker::FDelegateHandle Ticker;
    FIntPoint Origin=FIntPoint::ZeroValue, BufferSize=FIntPoint::ZeroValue;
    EPixelFormat Format=PF_Unknown;
    FSlot Slots[SlotCount];
    TArray<FFrame> Frames;
    TArray<FString> Events;
    FString Directory;
    double Started=FPlatformTime::Seconds(), FirstCopy=0.;
    int64 Frequency=0;
    int32 Sequence=0, Missed=0;
    bool Failed=false;

    void ReadReady()
    {
        for (FSlot& Slot:Slots)
        {
            if (!Slot.Pending || !Slot.Readback->IsReady()) continue;
            int32 Pitch=0, Rows=0;
            const uint8* Source=static_cast<const uint8*>(Slot.Readback->Lock(Pitch,&Rows));
            Slot.Frame.ReadyQpc=Qpc();
            if (Source && Pitch>=Width && Rows>=Height)
            {
                Slot.Frame.Bytes.SetNumUninitialized(FrameBytes);
                for (int32 Y=0;Y<Height;++Y)
                    FMemory::Memcpy(Slot.Frame.Bytes.GetData()+Y*Width*4,
                        Source+static_cast<int64>(Y)*Pitch*4,Width*4);
                Frames.Add(MoveTemp(Slot.Frame));
            }
            else { Failed=true; Events.Add(TEXT("invalid_readback")); }
            Slot.Readback->Unlock();
            Slot.Pending=false;
        }
    }
    void OnBuffer(SWindow& W,const FTexture2DRHIRef& Buffer)
    {
        if (&W!=Target) return;
        ReadReady();
        if (Failed || Sequence>=MaxFrames) return;
        const double Now=FPlatformTime::Seconds();
        if (FirstCopy>0. && Now-FirstCopy>=6.) return;
        const FIntPoint Extent=Buffer->GetSizeXY();
        const EPixelFormat Fmt=Buffer->GetFormat();
        if (Origin.X<0 || Origin.Y<0 || Origin.X+Width>Extent.X || Origin.Y+Height>Extent.Y
            || (Fmt!=PF_B8G8R8A8 && Fmt!=PF_R8G8B8A8 && Fmt!=PF_A2B10G10R10)
            || (Format!=PF_Unknown && (Format!=Fmt || BufferSize!=Extent)))
        { Failed=true; Events.Add(TEXT("unsupported_or_changed_backbuffer")); return; }
        BufferSize=Extent; Format=Fmt;
        if (FirstCopy==0.) FirstCopy=Now;
        const int32 Id=Sequence++;
        FSlot* Available=nullptr;
        for (FSlot& Slot:Slots) if (!Slot.Pending) { Available=&Slot; break; }
        if (!Available) { ++Missed; Events.Add(FString::Printf(TEXT("no_ready_slot:%d"),Id)); return; }
        FSlot& Slot=*Available;
        if (!Slot.Readback) Slot.Readback=MakeUnique<FRHIGPUTextureReadback>(TEXT("APSBoundedPixelBurst"));
        Slot.Frame=FFrame(); Slot.Frame.Sequence=Id;
        Slot.Frame.RenderFrame=GFrameNumberRenderThread; Slot.Frame.CopyQpc=Qpc();
        auto& Commands=FRHICommandListExecutor::GetImmediateCommandList();
        // SlateRHIRenderer transitions to SRVGraphics immediately before this delegate.
        Commands.Transition(FRHITransitionInfo(Buffer,ERHIAccess::SRVGraphics,ERHIAccess::CopySrc));
        Slot.Readback->EnqueueCopy(Commands,Buffer.GetReference(),
            FIntVector(Origin.X,Origin.Y,0),0,FIntVector(Width,Height,1));
        Commands.Transition(FRHITransitionInfo(Buffer,ERHIAccess::CopySrc,ERHIAccess::SRVGraphics));
        Slot.Pending=true;
    }
    void Save()
    {
        Frames.Sort([](const FFrame& A,const FFrame& B){return A.Sequence<B.Sequence;});
        TArray<FString> Lines;
        Lines.Add(FString::Printf(TEXT("# qpcFrequency=%lld,x=%d,y=%d,width=%d,height=%d,pixelFormat=%d,bufferWidth=%d,bufferHeight=%d,attempted=%d,missed=%d,failed=%d"),
            Frequency,Origin.X,Origin.Y,Width,Height,static_cast<int32>(Format),BufferSize.X,BufferSize.Y,Sequence,Missed,Failed));
        Lines.Add(TEXT("sequence,renderFrame,copyQpc,readyQpc,byteOffset,byteLength"));
        bool Saved=false;
        TUniquePtr<FArchive> Output(IFileManager::Get().CreateFileWriter(*(Directory/TEXT("frames.raw")),FILEWRITE_NoReplaceExisting));
        if (Output)
        {
            int64 Offset=0;
            for (FFrame& F:Frames)
            {
                Lines.Add(FString::Printf(TEXT("%d,%u,%lld,%lld,%lld,%d"),F.Sequence,F.RenderFrame,F.CopyQpc,F.ReadyQpc,Offset,F.Bytes.Num()));
                Output->Serialize(F.Bytes.GetData(),F.Bytes.Num());
                Offset+=F.Bytes.Num(); F.Bytes.Empty();
            }
            Saved=Output->Close() && !Output->IsError();
        }
        Saved=FFileHelper::SaveStringArrayToFile(Lines,*(Directory/TEXT("frames.csv")),FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM) && Saved;
        FFileHelper::SaveStringArrayToFile(Events,*(Directory/TEXT("events.txt")));
        UE_LOG(LogTemp,Display,TEXT("[APS.PixelBurst] frames=%d attempted=%d missed=%d failed=%d saved=%d directory=%s"),Frames.Num(),Sequence,Missed,Failed,Saved,*Directory);
    }
};
static TSharedPtr<FProbe,ESPMode::ThreadSafe> Active;
static void Stop()
{
    if (!Active) return;
    auto Finished=MoveTemp(Active);
    FTSTicker::GetCoreTicker().RemoveTicker(Finished->Ticker);
    FSlateApplication::Get().GetRenderer()->OnBackBufferReadyToPresent().Remove(Finished->Handle);
    ENQUEUE_RENDER_COMMAND(APSFinishPixelBurst)([Finished](FRHICommandListImmediate&)
    {
        Finished->ReadReady();
        for (FSlot& Slot:Finished->Slots)
        {
            if (Slot.Pending) { ++Finished->Missed; Finished->Events.Add(TEXT("pending_at_stop")); }
            Slot.Readback.Reset();
        }
        Async(EAsyncExecution::ThreadPool,[Finished](){Finished->Save();});
    });
}
static void Start(const TArray<FString>& Args)
{
    if (Active || Args.Num()!=2 || !Args[0].IsNumeric() || !Args[1].IsNumeric() || !FSlateApplication::IsInitialized()) return;
    UWorld* W=GEditor?GEditor->PlayWorld:nullptr;
    if (!W || W->WorldType!=EWorldType::PIE || W->GetName()!=TEXT("L_WorldGeneration") || !W->GetGameViewport()) return;
    auto Window=W->GetGameViewport()->GetWindow();
    if (!Window) return;
    Active=MakeShared<FProbe,ESPMode::ThreadSafe>();
    Active->World=W; Active->Window=Window; Active->Target=Window.Get();
    Active->Origin=FIntPoint(FCString::Atoi(*Args[0]),FCString::Atoi(*Args[1]));
    LARGE_INTEGER Frequency; ::QueryPerformanceFrequency(&Frequency); Active->Frequency=Frequency.QuadPart;
    Active->Directory=FPaths::Combine(FPaths::ProjectSavedDir(),TEXT("Diagnostics"),TEXT("PixelBurst-")+FDateTime::UtcNow().ToString(TEXT("%Y%m%d-%H%M%S")));
    if (IFileManager::Get().DirectoryExists(*Active->Directory)) { Active.Reset(); return; }
    IFileManager::Get().MakeDirectory(*Active->Directory,true);
    Active->Frames.Reserve(MaxFrames);
    Active->Handle=FSlateApplication::Get().GetRenderer()->OnBackBufferReadyToPresent().AddSP(Active.ToSharedRef(),&FProbe::OnBuffer);
    Active->Ticker=FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([](float)
    {
        if (!Active) return false;
        if (!Active->World.IsValid() || GEditor->PlayWorld!=Active->World.Get() || !Active->Window.IsValid()
            || FPlatformTime::Seconds()-Active->Started>=8.) { Stop(); return false; }
        return true;
    }));
    UE_LOG(LogTemp,Display,TEXT("[APS.PixelBurst] six-second 512x384 readback armed; max768frames; no image/settings changes."));
}
static FAutoConsoleCommand StartCommand(TEXT("APS.Diagnostics.StartPixelBurst"),TEXT("Copy sky-only PIE rectangle at x y for six seconds, bounded memory; does not change image or input."),FConsoleCommandWithArgsDelegate::CreateStatic(&Start),ECVF_Cheat);
static FAutoConsoleCommand StopCommand(TEXT("APS.Diagnostics.StopPixelBurst"),TEXT("Detach diagnostic pixel readback; incomplete frames remain explicit."),FConsoleCommandDelegate::CreateStatic(&Stop),ECVF_Cheat);
}
#endif
