// Rio 03.10 (galaxy phase 3): console variables and the start-up shader compile decision.
#include "APSStarRendererPrivate.h"

#include "HAL/FileManager.h"
#include "HAL/PlatformProcess.h"
#include "Misc/CommandLine.h"
#include "Misc/CoreMisc.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

namespace APSStarRenderer::Private
{
	TAutoConsoleVariable<int32> CVarCompileShaders(
		TEXT("aps.Stars.CompileShaders"), 1,
		TEXT("Rio 03.10: 1 compiles the GPU star and galaxy glow global shaders at start-up (set it in [SystemSettings] or with ")
		TEXT("-ini:Engine:[SystemSettings]:aps.Stars.CompileShaders=1 and restart). 0: the shaders never enter the shader ")
		TEXT("map, the feature cannot run and nothing else changes. Default 1 since Rio 03.10 09:10 (\"enable it by default\"). A start that dies during the global shader compile turns it off ")
		TEXT("until the shader sources change (Saved/APSStarRenderer/ShaderCompileAttempt.txt)."),
		ECVF_ReadOnly);

	TAutoConsoleVariable<int32> CVarGpuPoints(
		TEXT("aps.Stars.GpuPoints"), 1,
		TEXT("Rio 03.10: 1 draws the registered GPU star point sets (compute raster after TSR, before bloom). 0: off. On by default since Rio 03.10 09:10."),
		ECVF_RenderThreadSafe);

	TAutoConsoleVariable<int32> CVarGpuPointBudget(
		TEXT("aps.Stars.GpuPointBudget"), 16 * 1024 * 1024,
		TEXT("Points tested per view and frame (all sets, priority order). Sets keep their catalogue order, so a cut prefix is a uniform sample."),
		ECVF_RenderThreadSafe);

	TAutoConsoleVariable<float> CVarGpuPointIntensity(
		TEXT("aps.Stars.GpuPointIntensity"), 1.0f,
		TEXT("Global brightness multiplier of the GPU points (calibration against the HISM stars)."),
		ECVF_RenderThreadSafe);

	TAutoConsoleVariable<float> CVarGpuPointMinPixel(
		TEXT("aps.Stars.GpuPointMinPixel"), 0.002f,
		TEXT("Brightness LOD: a point whose pre-exposed pixel value is below this is skipped; the glow carries its light."),
		ECVF_RenderThreadSafe);

	TAutoConsoleVariable<float> CVarGpuPointMaxPixel(
		TEXT("aps.Stars.GpuPointMaxPixel"), 16.0f,
		TEXT("Clamp of a point's pre-exposed pixel value (keeps bloom finite for very near stars)."),
		ECVF_RenderThreadSafe);

	TAutoConsoleVariable<float> CVarGpuPointCoreSigma(
		TEXT("aps.Stars.GpuPointCoreSigma"), 0.55f,
		TEXT("Gaussian sigma of the point core in pixels."),
		ECVF_RenderThreadSafe);

	TAutoConsoleVariable<float> CVarGpuPointHalo(
		TEXT("aps.Stars.GpuPointHalo"), 0.25f,
		TEXT("Largest halo share of a bright point (its halo widens from 0.9 to 1.6 px with brightness)."),
		ECVF_RenderThreadSafe);

	TAutoConsoleVariable<int32> CVarGpuPointPsfRadius(
		TEXT("aps.Stars.GpuPointPsfRadius"), 2,
		TEXT("Gather radius of the point PSF in pixels (1..3)."),
		ECVF_RenderThreadSafe);

	TAutoConsoleVariable<int32> CVarGpuPointAtomic64(
		TEXT("aps.Stars.GpuPointAtomic64"), 1,
		TEXT("1: 64-bit image atomics when the GPU supports them (float brightness, 1/256 px position). 0: 32-bit path (1/16 px)."),
		ECVF_RenderThreadSafe);

	TAutoConsoleVariable<int32> CVarGpuUploadPointsPerFrame(
		TEXT("aps.Stars.GpuUploadPointsPerFrame"), 1024 * 1024,
		TEXT("Points uploaded to the GPU per frame and set (8 bytes each); a new set fills in progressively in catalogue order."),
		ECVF_RenderThreadSafe);

	TAutoConsoleVariable<int32> CVarGalaxyGlow(
		TEXT("aps.Stars.GalaxyGlow"), 1,
		TEXT("Rio 03.10: 1 draws the registered galaxy glow volume (unresolved light and dust lanes). 0: off. On by default since Rio 03.10 09:10."),
		ECVF_RenderThreadSafe);

	TAutoConsoleVariable<float> CVarGalaxyGlowIntensity(
		TEXT("aps.Stars.GalaxyGlowIntensity"), 1.0f,
		TEXT("Global brightness multiplier of the galaxy glow."),
		ECVF_RenderThreadSafe);

	TAutoConsoleVariable<float> CVarGalaxyGlowMax(
		TEXT("aps.Stars.GalaxyGlowMax"), 0.35f,
		TEXT("Rio 03.10 (a dense galaxy core whited out the screen): the glow's pre-exposed luminance saturates softly at ")
		TEXT("this value (linear well below it), so halos stay but never wash the frame out. 0: no limit (the old look)."),
		ECVF_RenderThreadSafe);

	TAutoConsoleVariable<int32> CVarGalaxyGlowResolution(
		TEXT("aps.Stars.GalaxyGlowResolution"), 2,
		TEXT("Glow resolution divisor (2 = half, 4 = quarter resolution, bilinear upsample)."),
		ECVF_RenderThreadSafe);

	TAutoConsoleVariable<int32> CVarGalaxyGlowSteps(
		TEXT("aps.Stars.GalaxyGlowSteps"), 48,
		TEXT("Ray-march steps of the glow (8..256). The vertical profile is integrated exactly inside every step."),
		ECVF_RenderThreadSafe);

	// Rio 04.10 (a dark band along the galaxy disc across a planet's blue sky, a dark wedge through the sun's glare): the
	// composite runs after all translucency, so the scene it dims holds the atmosphere's own light and the glare too,
	// and sky pixels have no depth: they took the dust of the whole galaxy column. The scene is local; it stays undimmed.
	TAutoConsoleVariable<float> CVarGalaxyGlowDustOnScene(
		TEXT("aps.Stars.GalaxyGlowDustOnScene"), 0.0f,
		TEXT("0..1: how much the glow's dust dims the existing scene (HISM stars, but also atmospheres and glare, which then turn ")
		TEXT("dark along the disc). 0 (default) leaves the scene untouched; the GPU points keep their own dust."),
		ECVF_RenderThreadSafe);

	TAutoConsoleVariable<float> CVarGalaxyGlowDustOnPoints(
		TEXT("aps.Stars.GalaxyGlowDustOnPoints"), 1.0f,
		TEXT("0..1: how much the glow's dust dims the GPU points."),
		ECVF_RenderThreadSafe);

	TAutoConsoleVariable<int32> CVarGpuStats(
		TEXT("aps.Stars.GpuStats"), 0,
		TEXT("1 reads the GPU counters back (points tested / in frustum / visible / written, a few frames late); see aps.Stars.GpuReport."),
		ECVF_RenderThreadSafe);

	TAutoConsoleVariable<int32> CVarGpuDebugView(
		TEXT("aps.Stars.GpuDebugView"), 0,
		TEXT("0 normal, 1 GPU points only, 2 glow only, 3 dust transmittance."),
		ECVF_RenderThreadSafe);

	std::atomic<bool> GShadersMissingAtRuntime{false};
	std::atomic<bool> GUsingAtomic64{false};

	namespace
	{
		FString GShaderSourceDirectory;
		bool GCompileBlockedByCrashGuard = false;
		std::atomic<bool> GCrashGuardWritten{false};
		std::atomic<bool> GStartupFinished{false};

		FString GetCrashGuardPath()
		{
			return FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("APSStarRenderer"), TEXT("ShaderCompileAttempt.txt"));
		}

		uint32 HashShaderSources()
		{
			uint32 Hash = 0x41505331u;
			const FString PrivateDir = FPaths::Combine(GShaderSourceDirectory, TEXT("Private"));
			TArray<FString> Files;
			IFileManager::Get().FindFiles(Files, *FPaths::Combine(PrivateDir, TEXT("*.*")), true, false);
			Files.Sort();
			for (const FString& File : Files)
			{
				TArray<uint8> Bytes;
				if (FFileHelper::LoadFileToArray(Bytes, *FPaths::Combine(PrivateDir, File)))
				{
					Hash = FCrc::MemCrc32(Bytes.GetData(), Bytes.Num(), Hash);
				}
				Hash = FCrc::StrCrc32(*File, Hash);
			}
			return Hash;
		}

		bool DecideShaderCompile()
		{
			if (CVarCompileShaders.GetValueOnAnyThread() == 0)
			{
				UE_LOG(LogAPSStarRenderer, Log,
					TEXT("[APS.GpuStars] shaders not compiled (aps.Stars.CompileShaders=0): GPU points and glow are unavailable"));
				return false;
			}
			if (GShaderSourceDirectory.IsEmpty())
			{
				UE_LOG(LogAPSStarRenderer, Error, TEXT("[APS.GpuStars] shader directory is not mapped: shaders stay off"));
				return false;
			}
			if (IsRunningCommandlet() || GStartupFinished.load())
			{
				// Cooks and test commandlets follow the ini only; after start-up there is no global compile to guard.
				return true;
			}

			// Crash guard: a global shader compile error is fatal by default (r.AreShaderErrorsFatal). If the last
			// start with these exact sources died before start-up finished, do not try them again.
			const FString GuardPath = GetCrashGuardPath();
			const uint32 SourceHash = HashShaderSources();
			const uint32 ProcessId = FPlatformProcess::GetCurrentProcessId();
			FString Previous;
			if (FFileHelper::LoadFileToString(Previous, *GuardPath))
			{
				TArray<FString> Parts;
				Previous.ParseIntoArrayWS(Parts);
				if (Parts.Num() >= 2)
				{
					const uint32 PreviousHash = static_cast<uint32>(FCString::Strtoui64(*Parts[0], nullptr, 16));
					const uint32 PreviousProcess = static_cast<uint32>(FCString::Strtoui64(*Parts[1], nullptr, 10));
					if (PreviousHash == SourceHash && PreviousProcess != ProcessId
						&& !FPlatformProcess::IsApplicationRunning(PreviousProcess))
					{
						GCompileBlockedByCrashGuard = true;
						UE_LOG(LogAPSStarRenderer, Error,
							TEXT("[APS.GpuStars] a previous start (pid %u) with these shader sources stopped during start-up: ")
							TEXT("GPU star shaders stay off. Fix the shader error or delete %s to retry."),
							PreviousProcess, *GuardPath);
						return false;
					}
				}
			}
			GCrashGuardWritten.store(
				FFileHelper::SaveStringToFile(FString::Printf(TEXT("%08x %u"), SourceHash, ProcessId), *GuardPath));
			UE_LOG(LogAPSStarRenderer, Log, TEXT("[APS.GpuStars] compiling the GPU star shaders (sources %08x)"), SourceHash);
			return true;
		}
	}

	void SetShaderSourceDirectory(const FString& Directory)
	{
		GShaderSourceDirectory = Directory;
	}

	bool ShouldCompileShaders()
	{
		// Thread-safe one-time initialisation; first called by the start-up global shader compile.
		static const bool bDecision = DecideShaderCompile();
		return bDecision;
	}

	bool IsCompileBlockedByCrashGuard()
	{
		return ShouldCompileShaders() ? false : GCompileBlockedByCrashGuard;
	}

	void OnStartupFinished()
	{
		GStartupFinished.store(true);
		if (GCrashGuardWritten.exchange(false))
		{
			IFileManager::Get().Delete(*GetCrashGuardPath(), false, true, true);
		}
	}
}
