// Editor-only asynchronous copy of the existing PIE backbuffer. Never uses Shot,
// changes a view/material, or captures another editor/application window.
#if WITH_EDITOR && WITH_DEV_AUTOMATION_TESTS
#include "Editor.h"
#include "Async/Async.h"
#include "Containers/Queue.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "Framework/Application/SlateApplication.h"
#include "HAL/FileManager.h"
#include "HAL/IConsoleManager.h"
#include "ImageUtils.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Rendering/SlateRenderer.h"
#include "RenderingThread.h"
#include "RHIGPUReadback.h"
#include "Widgets/SWindow.h"

namespace APSStellarAsyncCapture
{
class FProbe
{
public:
    explicit FProbe(const SWindow* Window) : Target(Window), Started(FPlatformTime::Seconds()) {}
    const SWindow* Target;
    const double Started;
    FDelegateHandle Handle;
    TQueue<FString, EQueueMode::Mpsc> Requests;
    int32 Requested = 0; // game thread only
    TUniquePtr<FRHIGPUTextureReadback> Readback; // render thread only
    FString PendingFile;
    FIntPoint Size = FIntPoint::ZeroValue;
    EPixelFormat Format = PF_Unknown;
    bool Pending = false;

    void OnBackBuffer(SWindow& Window, const FTexture2DRHIRef& Buffer)
    {
        if (&Window != Target) return;
        // Poll a GPU fence; no waiting/flush/ReadPixels in the capture path.
        if (Pending && Readback->IsReady())
        {
            int32 Pitch = 0, Height = 0;
            const uint8* Data = static_cast<const uint8*>(Readback->Lock(Pitch, &Height));
            TArray<FColor> Pixels;
            if (Data && Pitch >= Size.X && Height >= Size.Y)
            {
                Pixels.SetNumUninitialized(Size.X * Size.Y);
                for (int32 Y = 0; Y < Size.Y; ++Y)
                    FMemory::Memcpy(Pixels.GetData() + Y * Size.X, Data + static_cast<int64>(Y) * Pitch * 4, Size.X * 4);
            }
            Readback->Unlock();
            Pending = false;
            if (Pixels.Num() == Size.X * Size.Y)
            {
                const FString File = PendingFile;
                const FIntPoint ImageSize = Size;
                const EPixelFormat ImageFormat = Format;
                // PNG compression and disk I/O never run on the render/game thread.
                Async(EAsyncExecution::ThreadPool, [File, ImageSize, ImageFormat, Pixels = MoveTemp(Pixels)]() mutable
                {
                    // Keep original packed bytes for numerical analysis, before PNG quantization.
                    FFileHelper::SaveArrayToFile(TArrayView64<const uint8>(
                        reinterpret_cast<const uint8*>(Pixels.GetData()), Pixels.Num() * sizeof(FColor)), *(File + TEXT(".raw")));
                    FFileHelper::SaveStringToFile(*FString::Printf(TEXT("{\"width\":%d,\"height\":%d,\"pixelFormat\":%d}"),
                        ImageSize.X, ImageSize.Y, static_cast<int32>(ImageFormat)), *(File + TEXT(".json")));
                    for (FColor& Pixel : Pixels)
                    {
                        if (ImageFormat == PF_A2B10G10R10)
                        {
                            const uint32 Packed = Pixel.DWColor();
                            Pixel = FColor(
                                static_cast<uint8>(((Packed & 1023u) * 255u + 511u) / 1023u),
                                static_cast<uint8>((((Packed >> 10u) & 1023u) * 255u + 511u) / 1023u),
                                static_cast<uint8>((((Packed >> 20u) & 1023u) * 255u + 511u) / 1023u), 255);
                        }
                        else if (ImageFormat == PF_R8G8B8A8) Swap(Pixel.R, Pixel.B);
                        Pixel.A = 255;
                    }
                    TArray64<uint8> Png;
                    FImageUtils::PNGCompressImageArray(ImageSize.X, ImageSize.Y,
                        TArrayView64<const FColor>(Pixels.GetData(), Pixels.Num()), Png);
                    const bool Saved = FFileHelper::SaveArrayToFile(Png, *File);
                    UE_LOG(LogTemp, Display, TEXT("[APS.AsyncStarCapture] saved=%d size=%dx%d file=%s"),
                        Saved ? 1 : 0, ImageSize.X, ImageSize.Y, *File);
                });
            }
            else UE_LOG(LogTemp, Error, TEXT("[APS.AsyncStarCapture] Invalid readback dimensions; no image written."));
        }
        if (Pending || FPlatformTime::Seconds() - Started > 30.0) return;
        FString File;
        if (!Requests.Dequeue(File)) return;
        const FIntPoint Extent = Buffer->GetSizeXY();
        const EPixelFormat PixelFormat = Buffer->GetFormat();
        if ((PixelFormat != PF_B8G8R8A8 && PixelFormat != PF_R8G8B8A8 && PixelFormat != PF_A2B10G10R10)
            || Extent.X <= 0 || Extent.Y <= 0 || Extent.X > 4096 || Extent.Y > 2160)
        {
            UE_LOG(LogTemp, Error, TEXT("[APS.AsyncStarCapture] Unsupported backbuffer format=%d size=%dx%d; no conversion or mode change."),
                static_cast<int32>(PixelFormat), Extent.X, Extent.Y);
            return;
        }
        if (Readback && (Size != Extent || Format != PixelFormat)) Readback.Reset();
        if (!Readback) Readback = MakeUnique<FRHIGPUTextureReadback>(TEXT("APSReadOnlyStarCapture"));
        Size = Extent;
        Format = PixelFormat;
        PendingFile = File;
        auto& Commands = FRHICommandListExecutor::GetImmediateCommandList();
        Commands.Transition(FRHITransitionInfo(Buffer, ERHIAccess::SRVGraphics, ERHIAccess::CopySrc));
        Readback->EnqueueCopy(Commands, Buffer.GetReference());
        Commands.Transition(FRHITransitionInfo(Buffer, ERHIAccess::CopySrc, ERHIAccess::SRVGraphics));
        Pending = true;
        UE_LOG(LogTemp, Display, TEXT("[APS.AsyncStarCapture] enqueued cycles=%llu file=%s"), FPlatformTime::Cycles64(), *File);
    }
};
static TUniquePtr<FProbe> Probe;

static void Stop()
{
    if (!Probe) return;
    FSlateApplication::Get().GetRenderer()->OnBackBufferReadyToPresent().Remove(Probe->Handle);
    // Cleanup only, after the camera measurement is over.
    FlushRenderingCommands();
    Probe.Reset();
    UE_LOG(LogTemp, Display, TEXT("[APS.AsyncStarCapture] detached."));
}
static void Request(const TArray<FString>& Args)
{
    UWorld* World = GEditor ? GEditor->PlayWorld : nullptr;
    if (!World || World->WorldType != EWorldType::PIE || World->GetName() != TEXT("L_WorldGeneration")
        || !World->GetGameViewport() || Args.Num() != 1 || Args[0].Len() > 120 || Args[0].IsEmpty()) return;
    for (TCHAR C : Args[0]) if (!FChar::IsAlnum(C) && C != TEXT('_')) return;
    TSharedPtr<SWindow> Window = World->GetGameViewport()->GetWindow();
    if (!Window.IsValid()) return;
    if (Probe && (Probe->Target != Window.Get() || FPlatformTime::Seconds() - Probe->Started > 30.0)) Stop();
    if (!Probe)
    {
        Probe = MakeUnique<FProbe>(Window.Get());
        Probe->Handle = FSlateApplication::Get().GetRenderer()->OnBackBufferReadyToPresent().AddRaw(Probe.Get(), &FProbe::OnBackBuffer);
    }
    if (Probe->Requested >= 8) return;
    const FString Directory = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("Diagnostics/AsyncStellar"));
    const FString File = FPaths::Combine(Directory, Args[0] + TEXT(".png"));
    if (IFileManager::Get().FileExists(*File)) return;
    IFileManager::Get().MakeDirectory(*Directory, true);
    ++Probe->Requested;
    Probe->Requests.Enqueue(File);
}
static FAutoConsoleCommand CaptureCommand(TEXT("APS.Diagnostics.AsyncStellarShot"),
    TEXT("Asynchronously copy only the existing PIE backbuffer into Saved/Diagnostics/AsyncStellar/<unique_name>.png; max8shots/30seconds. No renderer settings change."),
    FConsoleCommandWithArgsDelegate::CreateStatic(&Request), ECVF_Cheat);
static FAutoConsoleCommand StopCommand(TEXT("APS.Diagnostics.StopAsyncStellarShots"),
    TEXT("Detach the stellar backbuffer observer after measurement."),
    FConsoleCommandDelegate::CreateStatic(&Stop), ECVF_Cheat);
}
#endif
