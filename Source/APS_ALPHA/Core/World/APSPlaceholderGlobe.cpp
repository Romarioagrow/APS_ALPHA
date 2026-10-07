#include "APSPlaceholderGlobe.h"
#include "APS_ALPHA/Core/Planetary/APSPlanetReliefRuntime.h"

#include "APS_ALPHA/Actors/Astro/Planet.h"
#include "APS_ALPHA/Actors/Astro/PlanetaryBody.h"
#include "APS_ALPHA/Generation/APSNativeGlobeSnapshot.h"
#include "APS_ALPHA/Generation/APSClosedGlobeMesh.h"
#include "APS_ALPHA/Generation/PlanetarySurfaceGenerator.h"
#include "Async/Async.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "RenderCommandFence.h"
#include "APS_ALPHA/Core/Controllers/GravityPlayerController.h"
#include "Camera/PlayerCameraManager.h"
#include "HAL/IConsoleManager.h"

namespace APSNativeGlobePrivate
{
	using namespace APSClosedGlobeMesh;
	TAutoConsoleVariable<int32> CVarMapSolidBodies(TEXT("aps.Map.SolidBodies"), 1,
		TEXT("Rio 06.10 (F10: transparent planets, stars and the galactic band through the disc): WorldScape relief is a patch built ")
		TEXT("for one observer; once it is ready the closed globe was hidden, so the map's free camera looked through the rest of the ")
		TEXT("body (only atmosphere and cloud shells, no depth). 1: while the strategic map is open, and 0.5 s of its blend back, every ")
		TEXT("body with ready relief also shows its closed globe from aps.Map.SolidBodyAltitude radii up. 0: the old rule."));
	TAutoConsoleVariable<float> CVarMapSolidBodyAltitude(TEXT("aps.Map.SolidBodyAltitude"), 0.5f,
		TEXT("Rio 06.10: aps.Map.SolidBodies shows a body's closed globe from this altitude (in body radii above the surface); once on it ")
		TEXT("stays on down to 80% of it."));
	struct FVisual
	{
		TWeakObjectPtr<USceneComponent> Frame;
		TWeakObjectPtr<UProceduralMeshComponent> Terrain;
		TWeakObjectPtr<UProceduralMeshComponent> Ocean;

		void Show(bool bVisible) const
		{
			for (auto* Mesh : {Terrain.Get(), Ocean.Get()})
				if (IsValid(Mesh))
				{
					Mesh->SetVisibility(bVisible, false);
					Mesh->SetHiddenInGame(!bVisible, false);
				}
		}
		void Destroy()
		{
			for (UActorComponent* Component : {static_cast<UActorComponent*>(Terrain.Get()),
				static_cast<UActorComponent*>(Ocean.Get()), static_cast<UActorComponent*>(Frame.Get())})
				if (IsValid(Component))
				{
					if (AActor* Owner = Component->GetOwner()) Owner->RemoveInstanceComponent(Component);
					Component->DestroyComponent();
				}
			*this = FVisual{};
		}
	};
	struct FJob
	{
		FSamplingFrame Frame;
		FBuildOptions Options;
		FMeshData Mesh;
		TAtomic<bool> Done{false};
		bool bSuccess = false; // Published by Done's release/acquire store/load.
	};
	struct FRecord
	{
		TWeakObjectPtr<APlanetaryBody> Body;
		FVisual Ready;
		FVisual Pending;
		TSharedPtr<FJob, ESPMode::ThreadSafe> Job;
		FRenderCommandFence CommitFence;
		uint32 DesiredIdentity = 0, ReadyIdentity = 0, PendingIdentity = 0;
		double IdentityCheckedAt = -1.0, RetryAt = 0.0, StartedAt = 0.0;
		bool bRequestedVisible = true, bAwaitingFence = false;
		// Rio 06.10 (F10 transparent planets): the strategic map holds this body's closed globe on next to its relief patch.
		bool bMapSolid = false;
		FString LastError;
	};
	struct FWorldState
	{
		TMap<TWeakObjectPtr<APlanetaryBody>, TSharedPtr<FRecord>> Bodies;
		TSharedPtr<FRecord> Running;
		// Rio 06.10: last time the strategic map was seen open (keeps solid bodies through its 0.5 s blend back).
		double LastMapSeconds = -1.0e9;
	};
	TMap<TWeakObjectPtr<UWorld>, TSharedPtr<FWorldState>> Worlds;

	FWorldState& State(UWorld* World)
	{
		auto& Found = Worlds.FindOrAdd(World);
		if (!Found) Found = MakeShared<FWorldState>();
		return *Found;
	}
	void HideAuthoredGlobes(APlanetaryBody* Body)
	{
		TInlineComponentArray<UStaticMeshComponent*> Meshes(Body);
		for (auto* Mesh : Meshes)
			if (IsValid(Mesh) && !Mesh->ComponentHasTag(TEXT("APS.GasGiantVisual")))
			{
				Mesh->SetVisibility(false, false);
				Mesh->SetHiddenInGame(true, false);
				Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			}
	}
	bool Current(const FRecord& Record)
	{
		return Record.Ready.Frame.IsValid() && Record.Ready.Terrain.IsValid()
			&& Record.ReadyIdentity == Record.DesiredIdentity;
	}
	FVisual CreateVisual(APlanetaryBody* Body, const AWorldScapeRoot* Root)
	{
		FVisual Result;
		auto* Frame = NewObject<USceneComponent>(Body, NAME_None, RF_Transient);
		Body->AddInstanceComponent(Frame);
		Frame->SetUsingAbsoluteScale(true);
		Frame->RegisterComponent();
		Frame->AttachToComponent(Body->GetRootComponent(), FAttachmentTransformRules::KeepWorldTransform);
		Frame->SetWorldTransform(FTransform(Root->GetActorQuat(), Root->GetActorLocation(), FVector::OneVector));
		Result.Frame = Frame;
		auto MakeMesh = [&](const FName Tag)
		{
			auto* Mesh = NewObject<UProceduralMeshComponent>(Body, NAME_None, RF_Transient);
			Body->AddInstanceComponent(Mesh);
			Mesh->SetupAttachment(Frame);
			Mesh->SetRelativeTransform(FTransform::Identity);
			Mesh->ComponentTags.Add(Tag);
			Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			Mesh->SetGenerateOverlapEvents(false);
			Mesh->SetCastShadow(false);
			Mesh->bUseAsOccluder = false;
			Mesh->SetVisibility(false);
			Mesh->SetHiddenInGame(true);
			Mesh->RegisterComponent();
			return Mesh;
		};
		Result.Terrain = MakeMesh(TEXT("APS.NativeClosedTerrain"));
		Result.Ocean = MakeMesh(TEXT("APS.NativeClosedOcean"));
		return Result;
	}
	void Wait(FRecord& Record, const FString& Error)
	{
		Record.RetryAt = FPlatformTime::Seconds() + 0.25;
		if (!Error.IsEmpty() && Error != Record.LastError)
		{
			Record.LastError = Error;
			UE_LOG(LogTemp, Log, TEXT("[APS.NativeGlobe] pending body=%s reason=%s"),
				*GetNameSafe(Record.Body.Get()), *Error);
		}
	}
	bool Start(FRecord& Record)
	{
		auto* Body = Record.Body.Get();
		if (!APSPlaceholderGlobe::Handles(Body)) return false;
		auto* Generator = Body->EnsurePlanetaryEnvironmentGenerator();
		if (!IsValid(Generator) || Generator->bPendingWorldScapeUnload) return false;
		// Reuse the real factory, including catalog, liquid and UnifiedLava policy.
		// A disabled root allocates no clipmaps, collision or foliage workers.
		const bool bHadRoot = IsValid(Generator->WorldScapeRootInstance);
		if (!Body->EnsureWorldScapeSurface()) return false;
		auto* Root = Generator->WorldScapeRootInstance;
		if (!Generator->IsSurfaceProfileCurrent(Body))
			Generator->ApplySurfaceProfile(Body);
		if (Generator->IsSurfaceProfileApplyPending() || !Generator->IsSurfaceProfileCurrent(Body))
		{
			Wait(Record, TEXT("native profile or material preparation"));
			return false;
		}
		auto Job = MakeShared<FJob, ESPMode::ThreadSafe>();
		uint32 Identity = 0;
		FString Error;
		if (!APSNativeGlobeSnapshot::Capture(Body, Generator, Job->Frame, Job->Options, Identity, Error))
		{
			Wait(Record, Error);
			return false;
		}
		Job->Options.FaceResolution = 96;
		Record.Pending = CreateVisual(Body, Root);
		UMaterialInstanceDynamic* Terrain = nullptr;
		UMaterialInstanceDynamic* Ocean = nullptr;
		if (!APSNativeGlobeSnapshot::CloneMaterials(Record.Pending.Frame.Get(),
			Record.Pending.Frame.Get(), Generator, Terrain, Ocean, Error))
		{
			Record.Pending.Destroy();
			Wait(Record, Error);
			return false;
		}
		// Subscribe before the lightweight root retires: copying overrides once
		// cannot receive a field that finishes later. Same existing MID and frame.
		if (APSPlanetReliefRuntime::IsEnabled())
			APSPlanetReliefRuntime::Register(Body,Job->Frame,Identity,Terrain);
		// The mesh references own these MIDs after the lightweight root is retired.
		Record.Pending.Terrain->SetMaterial(0, Terrain);
		if (Ocean) Record.Pending.Ocean->SetMaterial(0, Ocean);
		Record.PendingIdentity = Record.DesiredIdentity = Identity;
		Record.StartedAt = FPlatformTime::Seconds();
		Record.Job = Job;
		Record.LastError.Reset();
		AsyncTask(ENamedThreads::AnyBackgroundThreadNormalTask, [Job]()
		{
			Job->bSuccess = BuildClosedCubeSphere(Job->Frame, Job->Options, Job->Mesh);
			Job->Done.Store(true);
		});
		if (Body->GetWorldScapeStreamingState() == EWorldScapeSurfaceState::Unloaded
			&& !Root->bGenerateWorldScape && Root->WorldScapeLod.IsEmpty()
			&& Root->WorldScapeLodInGeneration.IsEmpty() && Generator->bOwnsWorldScapeRootInstance)
		{
			// Only a profile-only root: never destroy a published or building surface.
			Generator->UnloadWorldScapeRoot();
		}
		UE_LOG(LogTemp, Log, TEXT("[APS.NativeGlobe] queued body=%s seed=%d rootAlreadyAllocated=%d parent=%s"),
			*Body->GetName(), Body->WorldScapeSeed, int32(bHadRoot), *GetPathNameSafe(Terrain->Parent.Get()));
		return true;
	}
	bool Finish(FRecord& Record)
	{
		auto* Body = Record.Body.Get();
		if (IsValid(Body))
		{
			auto* Generator = Body->PlanetaryEnvironmentGenerator;
			Record.DesiredIdentity = APSNativeGlobeSnapshot::Identity(Body,
				IsValid(Generator) ? Generator->SurfaceProfileCatalog : nullptr);
		}
		if (!APSPlaceholderGlobe::Handles(Body) || Record.PendingIdentity != Record.DesiredIdentity)
		{
			if (Record.Job && !Record.Job->Done.Load()) return false;
			Record.Pending.Destroy();
			Record.Job.Reset();
			Record.bAwaitingFence = false;
			return true; // A changed profile never receives an old worker result.
		}
		if (Record.bAwaitingFence)
		{
			if (!Record.CommitFence.IsFenceComplete()) return false;
			Record.Ready.Destroy();
			Record.Ready = Record.Pending;
			Record.Pending = FVisual{};
			Record.ReadyIdentity = Record.PendingIdentity;
			Record.bAwaitingFence = false;
			Record.Job.Reset();
			Record.Ready.Show((Record.bRequestedVisible && !Body->bWorldScapeSurfaceReady) || Record.bMapSolid);
			HideAuthoredGlobes(Body);
			UE_LOG(LogTemp, Display, TEXT("[APS.NativeGlobe] published body=%s identity=%u seconds=%.3f terrain=%s sameNativeParent=1"),
				*Body->GetName(), Record.ReadyIdentity, FPlatformTime::Seconds() - Record.StartedAt,
				*GetPathNameSafe(Record.Ready.Terrain->GetMaterial(0)));
			return true;
		}
		if (!Record.Job || !Record.Job->Done.Load()) return false;
		if (!Record.Job->bSuccess || !Record.Pending.Terrain.IsValid())
		{
			Record.Pending.Destroy(); Record.Job.Reset();
			Wait(Record, TEXT("closed native mesh build failed; keeping previous complete geometry"));
			return true;
		}
		const auto& M = Record.Job->Mesh;
		Record.Pending.Terrain->CreateMeshSection_LinearColor(0, M.TerrainVertices, M.Indices,
			M.Normals, M.UV0, M.VertexColors, M.Tangents, false, false);
		if (Record.Job->Options.bBuildOcean)
			Record.Pending.Ocean->CreateMeshSection_LinearColor(0, M.OceanVertices, M.OceanIndices,
				M.OceanNormals, M.UV0, M.OceanDepthUV1, TArray<FVector2D>(), TArray<FVector2D>(),
				M.OceanVertexColors, M.Tangents, false, false);
		Record.CommitFence.BeginFence();
		Record.bAwaitingFence = true;
		return false;
	}
}

bool APSPlaceholderGlobe::Handles(const APlanetaryBody* Body)
{
	const auto* Planet = Cast<APlanet>(Body);
	if (!IsValid(Body) || !Body->GetWorld() || !Body->GetWorld()->IsGameWorld()
		|| !Body->bStreamWorldScapeSurface || (Planet && Planet->IsManual)
		|| !FMath::IsNearlyEqual(Body->WorldScapePresentationScale, 1.0)
		|| !UAPSPlanetSurfaceProfileResolver::SupportsWorldScape(Body->PlanetType)
		|| APSNativeGlobeSnapshot::IsExplicitAuthoredOverride(Body)) return false;
	for (const AActor* Parent = Body; IsValid(Parent); Parent = Parent->GetAttachParentActor())
		if (Parent->ActorHasTag(TEXT("WorldGenerationPreview"))) return false;
	return true;
}

void APSPlaceholderGlobe::Apply(APlanetaryBody* Body)
{
	using namespace APSNativeGlobePrivate;
	if (!Handles(Body)) return;
	auto& Record = State(Body->GetWorld()).Bodies.FindOrAdd(Body);
	if (!Record) { Record = MakeShared<FRecord>(); Record->Body = Body; }
	const double Now = FPlatformTime::Seconds();
	if (Now - Record->IdentityCheckedAt >= 0.5)
	{
		auto* Generator = Body->PlanetaryEnvironmentGenerator;
		Record->DesiredIdentity = APSNativeGlobeSnapshot::Identity(Body,
			IsValid(Generator) ? Generator->SurfaceProfileCatalog : nullptr);
		Record->IdentityCheckedAt = Now;
	}
	HideAuthoredGlobes(Body);
}

bool APSPlaceholderGlobe::SetVisible(APlanetaryBody* Body, bool bVisible)
{
	using namespace APSNativeGlobePrivate;
	if (!Handles(Body))
	{
		if (IsValid(Body))
			if (auto* WorldState = Worlds.Find(Body->GetWorld()))
				if (auto* Previous = (*WorldState)->Bodies.Find(Body))
				{
					(*Previous)->Ready.Show(false);
					(*Previous)->Pending.Show(false);
				}
		return false;
	}
	Apply(Body);
	auto& Record = State(Body->GetWorld()).Bodies.FindChecked(Body);
	Record->bRequestedVisible = bVisible;
	// Preserve the last complete same-body surface during an explicit profile edit.
	// Ordinary camera movement never invalidates this cache or its material.
	// Rio 06.10 (F10 transparent planets): the 0.5 s readiness cadence never hides a globe the open map holds solid.
	Record->Ready.Show(bVisible || Record->bMapSolid);
	return true;
}

bool APSPlaceholderGlobe::PrepareForUnload(APlanetaryBody* Body)
{
	using namespace APSNativeGlobePrivate;
	if (!Handles(Body)) return true;
	Apply(Body);
	auto& Record = *State(Body->GetWorld()).Bodies.FindChecked(Body);
	auto* Generator = Body->PlanetaryEnvironmentGenerator;
	Record.DesiredIdentity = APSNativeGlobeSnapshot::Identity(Body,
		IsValid(Generator) ? Generator->SurfaceProfileCatalog : nullptr);
	return Current(Record);
}

void APSPlaceholderGlobe::Tick(UWorld* World)
{
	using namespace APSNativeGlobePrivate;
	auto* Entry = Worlds.Find(World);
	if (!Entry) return;
	auto& S = **Entry;
	for (auto It = S.Bodies.CreateIterator(); It; ++It)
		if (!It.Key().IsValid() || !Handles(It.Key().Get()))
		{
			It.Value()->Ready.Destroy();
			// Worker owns values only; no join or access to a destroyed body.
			if (!S.Running || S.Running != It.Value()) It.Value()->Pending.Destroy();
			It.RemoveCurrent();
		}
	const double Now = FPlatformTime::Seconds();
	// Rio 06.10 (F10 transparent planets: SORYX/TETHOR showed GPU stars and the galactic band through the disc): the
	// WorldScape relief patch is built for one observer (the pawn, a frozen far direction, or the map camera only for the
	// anchored body), so once it is ready and the closed globe hides, the map's free camera sees through the rest of the
	// body. Map only: while the strategic map is open (plus 0.5 s of its blend back) every ready body above
	// aps.Map.SolidBodyAltitude radii keeps its existing closed globe on next to the patch, so the whole disc is opaque and
	// writes depth. The patch, its observer, collision, streaming and this job scheduling stay exactly as they are.
	// Placed before the running job's early return so a pending build never delays it.
	const AGravityPlayerController* MapController = World ? Cast<AGravityPlayerController>(World->GetFirstPlayerController()) : nullptr;
	if (MapController && MapController->IsStrategicMapOpen()) S.LastMapSeconds = Now;
	const bool bMapView = CVarMapSolidBodies.GetValueOnGameThread() != 0 && Now - S.LastMapSeconds < 0.5
		&& MapController && IsValid(MapController->PlayerCameraManager);
	const FVector MapCamera = bMapView ? MapController->PlayerCameraManager->GetCameraLocation() : FVector::ZeroVector;
	const float Altitude = FMath::Max(CVarMapSolidBodyAltitude.GetValueOnGameThread(), 0.0f);
	for (auto& Pair : S.Bodies)
	{
		FRecord& R = *Pair.Value;
		APlanetaryBody* Body = R.Body.Get();
		bool bSolid = false;
		if (bMapView && IsValid(Body) && Body->bWorldScapeSurfaceReady && Current(R))
		{
			const double Rad = Body->GetWorldScapeBodyRadiusCm();
			if (Rad > 0.0)
			{
				const double Alt = (FVector::Dist(MapCamera, Body->GetActorLocation()) - Rad) / Rad;
				bSolid = Alt >= (R.bMapSolid ? 0.8 * Altitude : Altitude);
			}
		}
		if (bSolid != R.bMapSolid)
		{
			R.bMapSolid = bSolid;
			// Same rule as Finish(): a body without ready relief keeps today's visibility.
			R.Ready.Show((R.bRequestedVisible && !(IsValid(Body) && Body->bWorldScapeSurfaceReady)) || bSolid);
			UE_LOG(LogTemp, Log, TEXT("[APS.Map] solid body %s: closed globe %s"), *GetNameSafe(Body), bSolid ? TEXT("on") : TEXT("off"));
		}
	}
	if (S.Running)
	{
		if (!Finish(*S.Running)) return;
		S.Running.Reset();
	}
	APawn* Observer = World && World->GetFirstPlayerController() ? World->GetFirstPlayerController()->GetPawn() : nullptr;
	const FVector ObserverPosition = Observer ? Observer->GetActorLocation() : FVector::ZeroVector;
	TSharedPtr<FRecord> Next;
	double BestScore = TNumericLimits<double>::Max();
	for (auto& Pair : S.Bodies)
	{
		const auto& R = Pair.Value;
		if (Current(*R) || R->RetryAt > Now || !R->Body.IsValid()) continue;
		auto* Body = R->Body.Get();
		const double Score = FVector::DistSquared(ObserverPosition, Body->GetActorLocation())
			/ FMath::Square(Body->GetWorldScapeBodyRadiusCm());
		if (Score < BestScore) { BestScore = Score; Next = R; }
	}
	if (Next && Start(*Next)) S.Running = Next;
}

void APSPlaceholderGlobe::Release(UWorld* World)
{
	using namespace APSNativeGlobePrivate;
	// Registered components are owned/destroyed by their bodies. Jobs retain only
	// immutable value snapshots and discard their result after world teardown.
	Worlds.Remove(World);
}

bool APSPlaceholderGlobe::HasInitialCoverage(UWorld* World)
{
	using namespace APSNativeGlobePrivate;
	if (!World) return false;
	bool bReady = true;
	for (TActorIterator<APlanetaryBody> It(World); It; ++It)
		if (Handles(*It) && !It->bWorldScapeSurfaceReady)
		{
			Apply(*It);
			bReady &= Current(*State(World).Bodies.FindChecked(*It));
		}
	return bReady;
}
