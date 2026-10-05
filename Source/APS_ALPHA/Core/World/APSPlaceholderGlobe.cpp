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

namespace APSNativeGlobePrivate
{
	using namespace APSClosedGlobeMesh;
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
		FString LastError;
	};
	struct FWorldState
	{
		TMap<TWeakObjectPtr<APlanetaryBody>, TSharedPtr<FRecord>> Bodies;
		TSharedPtr<FRecord> Running;
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
			Record.Ready.Show(Record.bRequestedVisible && !Body->bWorldScapeSurfaceReady);
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
	Record->Ready.Show(bVisible);
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
	if (S.Running)
	{
		if (!Finish(*S.Running)) return;
		S.Running.Reset();
	}
	APawn* Observer = World && World->GetFirstPlayerController() ? World->GetFirstPlayerController()->GetPawn() : nullptr;
	const FVector ObserverPosition = Observer ? Observer->GetActorLocation() : FVector::ZeroVector;
	TSharedPtr<FRecord> Next;
	double BestScore = TNumericLimits<double>::Max();
	const double Now = FPlatformTime::Seconds();
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
