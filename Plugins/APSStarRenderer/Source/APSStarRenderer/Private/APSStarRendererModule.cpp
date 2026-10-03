// Rio 03.10 (galaxy phase 3): module start-up. Loads at PostConfigInit only to map the shader directory
// before shader types initialise; everything that needs the engine waits for OnPostEngineInit.
#include "APSStarRendererPrivate.h"
#include "APSStarRegistry.h"
#include "APSStarViewExtension.h"

#include "Engine/World.h"
#include "Interfaces/IPluginManager.h"
#include "Misc/CoreDelegates.h"
#include "Misc/Paths.h"
#include "Modules/ModuleManager.h"
#include "SceneViewExtension.h"
#include "ShaderCore.h"

DEFINE_LOG_CATEGORY(LogAPSStarRenderer);

class FAPSStarRendererModule final : public IModuleInterface
{
public:
	virtual void StartupModule() override
	{
		const TSharedPtr<IPlugin> Plugin = IPluginManager::Get().FindPlugin(TEXT("APSStarRenderer"));
		if (!Plugin.IsValid())
		{
			UE_LOG(LogAPSStarRenderer, Error, TEXT("[APS.GpuStars] plugin descriptor not found: shaders stay off"));
			return;
		}
		const FString ShaderDirectory = FPaths::Combine(Plugin->GetBaseDir(), TEXT("Shaders"));
		const FString VirtualRoot(APSStarRenderer::Private::ShaderVirtualRoot);
		if (!FPaths::DirectoryExists(ShaderDirectory))
		{
			UE_LOG(LogAPSStarRenderer, Error, TEXT("[APS.GpuStars] shader directory missing: %s"), *ShaderDirectory);
		}
		else if (!AllShaderSourceDirectoryMappings().Contains(VirtualRoot))
		{
			AddShaderSourceDirectoryMapping(VirtualRoot, ShaderDirectory);
			APSStarRenderer::Private::SetShaderSourceDirectory(ShaderDirectory);
		}
		else
		{
			APSStarRenderer::Private::SetShaderSourceDirectory(ShaderDirectory);
		}

		PostEngineInitHandle = FCoreDelegates::OnPostEngineInit.AddRaw(this, &FAPSStarRendererModule::OnPostEngineInit);
		EnginePreExitHandle = FCoreDelegates::OnEnginePreExit.AddRaw(this, &FAPSStarRendererModule::OnEnginePreExit);
	}

	virtual void ShutdownModule() override
	{
		FCoreDelegates::OnPostEngineInit.Remove(PostEngineInitHandle);
		FCoreDelegates::OnEnginePreExit.Remove(EnginePreExitHandle);
		if (WorldCleanupHandle.IsValid())
		{
			FWorldDelegates::OnWorldCleanup.Remove(WorldCleanupHandle);
			WorldCleanupHandle.Reset();
		}
		ViewExtension.Reset();
	}

private:
	void OnPostEngineInit()
	{
		// The start-up global shader compile is over; a crash guard written for it is no longer needed.
		APSStarRenderer::Private::OnStartupFinished();
		if (GEngine == nullptr)
		{
			return;
		}
		ViewExtension = FSceneViewExtensions::NewExtension<FAPSStarViewExtension>();
		WorldCleanupHandle = FWorldDelegates::OnWorldCleanup.AddRaw(this, &FAPSStarRendererModule::OnWorldCleanup);
		UE_LOG(LogAPSStarRenderer, Log, TEXT("[APS.GpuStars] ready: shaders %s (aps.Stars.CompileShaders), points %d, glow %d"),
			APSStarRenderer::Private::ShouldCompileShaders() ? TEXT("compiled")
				: (APSStarRenderer::Private::IsCompileBlockedByCrashGuard() ? TEXT("blocked by crash guard") : TEXT("off")),
			APSStarRenderer::Private::CVarGpuPoints.GetValueOnGameThread(),
			APSStarRenderer::Private::CVarGalaxyGlow.GetValueOnGameThread());
	}

	void OnEnginePreExit()
	{
		if (WorldCleanupHandle.IsValid())
		{
			FWorldDelegates::OnWorldCleanup.Remove(WorldCleanupHandle);
			WorldCleanupHandle.Reset();
		}
		// Releases GPU buffers and read-backs while the renderer still runs.
		APSStarRenderer::Private::FRegistry::Get().Shutdown();
		ViewExtension.Reset();
	}

	void OnWorldCleanup(UWorld* World, bool /*bSessionEnded*/, bool /*bCleanupResources*/)
	{
		APSStarRenderer::RemoveAll(World);
	}

	TSharedPtr<FAPSStarViewExtension, ESPMode::ThreadSafe> ViewExtension;
	FDelegateHandle PostEngineInitHandle;
	FDelegateHandle EnginePreExitHandle;
	FDelegateHandle WorldCleanupHandle;
};

IMPLEMENT_MODULE(FAPSStarRendererModule, APSStarRenderer)
