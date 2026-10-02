#include "APSPlanetEnvironmentStreamingSubsystem.h"

#include "EngineUtils.h"
#include "HAL/IConsoleManager.h"
#include "HAL/PlatformTime.h"
#include "Materials/MaterialInterface.h"
#include "PrimitiveSceneProxy.h"
#include "UObject/UObjectIterator.h"
// WorldScapeRoot.h brings UWorldScapeLod; WorldScapeLod.h itself has no include guard (WorldScapePayloadValidation.cpp).
#include "WorldScapeCore/Public/WorldScapeRoot.h"

// A new root's native first Tick creates every base LOD component in one frame: WS_BaseMeshBatch took 64-100 ms
// per body when a family was selected on approach, four bodies in four seconds (a4-trace-2b/-3, 02.10). Holding
// that Tick and creating the components a few per frame uses WorldScape's own AdvanceBaseMeshInitialization, the
// path the flight residency already uses for its hidden replacement.

namespace APSFirstBuildPrivate
{
	TAutoConsoleVariable<int32> CVarSliced(TEXT("aps.Surface.SlicedFirstBuild"), 1,
		TEXT("Create a streamed root's base LOD components a few per frame instead of all in its first Tick ")
		TEXT("(0 = native single-frame build)."));
	TAutoConsoleVariable<int32> CVarLodsPerFrame(TEXT("aps.Surface.FirstBuildLodsPerFrame"), 1,
		TEXT("Base LOD components created per frame across all held roots (1..8)."));
	TAutoConsoleVariable<int32> CVarProxyProbe(TEXT("aps.Surface.ProxyProbe"), 0,
		TEXT("Diagnostic: log every frame in which a WorldScape root's terrain LOD meshes got new scene proxies."));
	TMap<TWeakObjectPtr<const UPrimitiveComponent>, const FPrimitiveSceneProxy*> LastProxies;
	/** What a dirty mesh looked like last frame: material, visibility, shadow, collision, custom depth. */
	TMap<TWeakObjectPtr<const UPrimitiveComponent>, FString> LastStates;

	FString DescribeRenderState(const UPrimitiveComponent* Mesh)
	{
		const UMaterialInterface* Material = Mesh->GetMaterial(0);
		return FString::Printf(TEXT("mat=%s(%p) visible=%d hiddenGame=%d shadow=%d collision=%d depth=%d sections=%d mob=%d scale=%s parent=%s"),
			*GetNameSafe(Material), Material, Mesh->IsVisible(), Mesh->bHiddenInGame, Mesh->CastShadow,
			static_cast<int32>(Mesh->GetCollisionEnabled()), Mesh->bRenderCustomDepth, Mesh->GetNumMaterials(),
			static_cast<int32>(Mesh->Mobility), *Mesh->GetComponentScale().ToCompactString(), *GetNameSafe(Mesh->GetAttachParent()));
	}

	/** AdvanceBaseMeshInitialization's own preconditions, plus a live producer of a planet-type root. */
	bool CanAdvance(const AWorldScapeRoot* Root)
	{
		return IsValid(Root) && !Root->init && Root->IsHidden() && !Root->IsActorTickEnabled()
			&& Root->WorldScapeLodInGeneration.IsEmpty() && Root->bGenerateWorldScape && !Root->bFreezeGeneration
			&& Root->GenerationType == EWorldScapeType::Planet;
	}
}

void UAPSPlanetEnvironmentStreamingSubsystem::HoldFirstBuild(AWorldScapeRoot* Root, const bool bForeground)
{
	using namespace APSFirstBuildPrivate;
	// Only a root about to run its native first build: Tick just enabled, nothing built, no worker, still hidden.
	if (CVarSliced.GetValueOnGameThread() == 0 || !IsValid(Root) || Root->init || !Root->IsActorTickEnabled()
		|| !Root->IsHidden() || !Root->WorldScapeLodInGeneration.IsEmpty() || !Root->bGenerateWorldScape
		|| Root->bFreezeGeneration || Root->GenerationType != EWorldScapeType::Planet)
	{
		return;
	}
	Root->SetActorTickEnabled(false);
	if (FirstBuilds.ContainsByPredicate([Root](const FFirstBuild& Build) { return Build.Root.Get() == Root; }))
	{
		return;
	}
	// The body being approached goes first; a sibling warmed in the background waits behind it.
	FirstBuilds.Insert(FFirstBuild{Root, 0, 0.0}, bForeground ? 0 : FirstBuilds.Num());
}

void UAPSPlanetEnvironmentStreamingSubsystem::AdvanceFirstBuilds()
{
	using namespace APSFirstBuildPrivate;
	int32 Budget = FMath::Clamp(CVarLodsPerFrame.GetValueOnGameThread(), 1, 8);
	while (Budget > 0 && FirstBuilds.Num() > 0)
	{
		FFirstBuild& Build = FirstBuilds[0];
		AWorldScapeRoot* Root = Build.Root.Get();
		if (!CanAdvance(Root))
		{
			// Unload, preload, freeze or a profile drain took the root over; its own lifecycle continues from there
			// (an enabled Tick finishes the build natively).
			FirstBuilds.RemoveAt(0);
			continue;
		}
		// The native first Tick sets both before it builds (AWorldScapeRoot::Tick).
		Root->PlanetLocation = Root->GetActorLocation();
		Root->PlanetScaleCode = Root->PlanetScale;
		const double Start = FPlatformTime::Seconds();
		const bool bComplete = Root->AdvanceBaseMeshInitialization(Budget);
		Build.Seconds += FPlatformTime::Seconds() - Start;
		++Build.Frames;
		Budget = 0;
		if (bComplete)
		{
			UE_LOG(LogTemp, Log, TEXT("[APS.WorldScape] sliced first build root=%s lods=%d+%d frames=%d build %.1f ms"),
				*GetNameSafe(Root->GetOwner()), Root->WorldScapeLod.Num(), Root->WorldScapeLodOcean.Num(),
				Build.Frames, Build.Seconds * 1000.0);
			Root->SetActorTickEnabled(true);
			FirstBuilds.RemoveAt(0);
		}
	}
}

void UAPSPlanetEnvironmentStreamingSubsystem::ProbeWorldScapeProxies()
{
	using namespace APSFirstBuildPrivate;
	if (CVarProxyProbe.GetValueOnGameThread() == 0 || !GetWorld())
	{
		LastProxies.Reset();
		LastStates.Reset();
		return;
	}
	// Which roots recreate their LOD proxies (190 ms frames of "Create WorldScapeMesh Proxy" x40 on approach, 02.10),
	// and whether the meshes were already marked dirty before the roots' own Tick (TG_LastDemotable) this frame.
	const APawn* Observer = GetWorld()->GetFirstPlayerController() ? GetWorld()->GetFirstPlayerController()->GetPawn() : nullptr;
	// Is it only WorldScape? Every dirty primitive of the world this frame, outside the roots' LOD meshes.
	{
		int32 OtherDirty = 0;
		FString Examples;
		for (TObjectIterator<UPrimitiveComponent> Primitive; Primitive; ++Primitive)
		{
			if (Primitive->GetWorld() != GetWorld() || !Primitive->IsRenderStateDirty()
				|| Primitive->GetFName() == TEXT("WorldScapeMesh")) continue;
			if (++OtherDirty <= 6)
			{
				Examples += FString::Printf(TEXT(" %s.%s(mob %d)"), *GetNameSafe(Primitive->GetOwner()), *Primitive->GetName(),
					static_cast<int32>(Primitive->Mobility));
			}
		}
		if (OtherDirty > 0)
		{
			UE_LOG(LogTemp, Log, TEXT("[APS.ProxyProbe] frame=%llu other dirty primitives=%d:%s"), GFrameCounter, OtherDirty, *Examples);
		}
	}
	for (TActorIterator<AWorldScapeRoot> It(GetWorld()); It; ++It)
	{
		int32 Changed = 0;
		int32 Dirty = 0;
		int32 Meshes = 0;
		for (const UWorldScapeLod* Lod : It->WorldScapeLod)
		{
			const UPrimitiveComponent* Mesh = IsValid(Lod) ? Lod->Mesh : nullptr;
			if (!IsValid(Mesh))
			{
				continue;
			}
			++Meshes;
			Dirty += Mesh->IsRenderStateDirty() ? 1 : 0;
			const FPrimitiveSceneProxy*& Last = LastProxies.FindOrAdd(Mesh, nullptr);
			Changed += Last != Mesh->SceneProxy ? 1 : 0;
			Last = Mesh->SceneProxy;
			// The first dirty mesh of a root names what changed since the previous frame.
			FString& LastState = LastStates.FindOrAdd(Mesh);
			const FString State = DescribeRenderState(Mesh);
			if (Mesh->IsRenderStateDirty() && Dirty == 1 && State != LastState)
			{
				UE_LOG(LogTemp, Log, TEXT("[APS.ProxyProbe]   %s: %s -> %s"), *GetNameSafe(It->GetOwner()), *LastState, *State);
			}
			LastState = State;
		}
		if (Changed > 0 || Dirty > 0)
		{
			const double AltitudeKm = Observer
				? (FVector::Distance(Observer->GetActorLocation(), It->GetActorLocation()) - It->PlanetScale) / 100000.0 : -1.0;
			UE_LOG(LogTemp, Log, TEXT("[APS.ProxyProbe] frame=%llu root=%s owner=%s meshes=%d newProxies=%d dirtyNow=%d ")
				TEXT("hidden=%d tick=%d init=%d frozen=%d generating=%d altitude=%.0f km"),
				GFrameCounter, *It->GetName(), *GetNameSafe(It->GetOwner()), Meshes, Changed, Dirty, It->IsHidden(),
				It->IsActorTickEnabled(), It->init, It->bFreezeGeneration, It->WorldScapeLodInGeneration.Num(), AltitudeKm);
		}
	}
}
