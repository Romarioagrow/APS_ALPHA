// Rio 03.10 (galaxy phase 3): private declarations shared by the plugin's translation units.
#pragma once

#include "CoreMinimal.h"
#include "HAL/IConsoleManager.h"
#include <atomic>

DECLARE_LOG_CATEGORY_EXTERN(LogAPSStarRenderer, Log, All);

namespace APSStarRenderer::Private
{
	/** Virtual shader directory mapped to <plugin>/Shaders at PostConfigInit. */
	inline const TCHAR* const ShaderVirtualRoot = TEXT("/Plugin/APSStarRenderer");

	// Console variables (APSStarSettings.cpp).
	extern TAutoConsoleVariable<int32> CVarCompileShaders;
	extern TAutoConsoleVariable<int32> CVarGpuPoints;
	extern TAutoConsoleVariable<int32> CVarGpuPointBudget;
	extern TAutoConsoleVariable<float> CVarGpuPointIntensity;
	extern TAutoConsoleVariable<float> CVarGpuPointMinPixel;
	extern TAutoConsoleVariable<float> CVarGpuPointMaxPixel;
	extern TAutoConsoleVariable<float> CVarGpuPointCoreSigma;
	extern TAutoConsoleVariable<float> CVarGpuPointHalo;
	extern TAutoConsoleVariable<int32> CVarGpuPointPsfRadius;
	extern TAutoConsoleVariable<float> CVarSkyMaskLuminance;
	extern TAutoConsoleVariable<float> CVarGpuPointCoreGrow;
	extern TAutoConsoleVariable<int32> CVarGpuPointAtomic64;
	extern TAutoConsoleVariable<int32> CVarGpuUploadPointsPerFrame;
	extern TAutoConsoleVariable<int32> CVarGalaxyGlow;
	extern TAutoConsoleVariable<float> CVarGalaxyGlowIntensity;
	extern TAutoConsoleVariable<float> CVarGalaxyGlowMax;
	extern TAutoConsoleVariable<int32> CVarGalaxyGlowResolution;
	extern TAutoConsoleVariable<int32> CVarGalaxyGlowSteps;
	extern TAutoConsoleVariable<float> CVarGalaxyGlowDustOnScene;
	extern TAutoConsoleVariable<float> CVarGalaxyGlowDustOnPoints;
	extern TAutoConsoleVariable<int32> CVarGpuStats;
	extern TAutoConsoleVariable<int32> CVarGpuDebugView;

	/** Real directory of the plugin shaders (set once at module start-up, before any shader compile). */
	void SetShaderSourceDirectory(const FString& Directory);

	/**
	 * Whether the plugin's global shaders may enter the shader map. Decided once, on first use (during the
	 * start-up global shader compile, after the ini files): aps.Stars.CompileShaders=1 and no crash guard.
	 * Any thread.
	 */
	bool ShouldCompileShaders();
	bool IsCompileBlockedByCrashGuard();

	/** Start-up finished (OnPostEngineInit): the global shader compile survived, drop the crash guard. */
	void OnStartupFinished();

	/** Set by the render thread when a required shader is missing at runtime (compile error with r.AreShaderErrorsFatal=0). */
	extern std::atomic<bool> GShadersMissingAtRuntime;
	/** Set by the render thread when the 64-bit atomic path is in use. */
	extern std::atomic<bool> GUsingAtomic64;
}
