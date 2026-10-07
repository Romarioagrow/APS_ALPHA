#include "APSStarCatalogueInstancesComponent.h"

#include "APS_ALPHA/Actors/Astro/Galaxy.h"
#include "APS_ALPHA/Actors/Astro/StarCluster.h"
#include "APS_ALPHA/Generation/AstroGenerator.h"
#include "Async/ParallelFor.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "HAL/IConsoleManager.h"
#include "HAL/PlatformTime.h"
#include "InstancedStaticMesh/ISMInstanceDataManager.h"
#include "InstancedStaticMesh/ISMInstanceUpdateChangeSet.h"
#include "Materials/MaterialInterface.h"
#include "ProfilingDebugging/CpuProfilerTrace.h"
#include "RenderTransform.h"
#include "UObject/UObjectIterator.h"

static_assert(sizeof(FRenderTransform) == 12 * sizeof(float), "FRenderTransform is compared with memcmp");

namespace APSStarCatalogueInstances
{
	TAutoConsoleVariable<int32> CVarParallelGather(
		TEXT("aps.Stars.CatalogueParallelGather"), 0,
		TEXT("Rio 06.10 (flight FPS after the star drive; audit: 1.6-3.6 ms of game thread a frame): the gameplay galaxy and ")
		TEXT("cluster catalogues (25k + 36k stars) re-send every instance transform whenever the sky moves, and UE 5.4 gathers ")
		TEXT("them on the game thread. 1 (default): that gather runs on the task workers, the engine's expression in the engine's ")
		TEXT("order, so the renderer receives the same bytes. 2: both gathers run and are compared bit for bit (the engine's is ")
		TEXT("sent; a difference is logged as an error; for tests, not for perf). 0: the engine's own gather (the rollback, a pure ")
		TEXT("pass-through). The menu preview's catalogues always keep the engine's."));
	TAutoConsoleVariable<int32> CVarParallelGatherMin(
		TEXT("aps.Stars.CatalogueParallelGatherMin"), 8192,
		TEXT("A catalogue with fewer instances than this keeps the engine's gather (aps.Stars.CatalogueParallelGather)."));
	constexpr int32 GatherBatch = 4096;

	/** FISMInstanceUpdateChangeSet::SetInstanceTransforms(View, Offset)'s per-instance expression (UE 5.4,
	 * ISMInstanceUpdateChangeSet.cpp:65) over its full-range iteration (index i -> item i). The task name differs from the
	 * game-thread scope APS_StarCatalogueGather so Insights keeps one timer per name (ParallelFor also opens its task name
	 * on the calling thread). */
	void ParallelGather(const TArray<FInstancedStaticMeshInstanceData>& Source, const FVector Offset,
		TArray<FRenderTransform>& Out)
	{
		const int32 Num = Source.Num();
		Out.Reset(Num);
		Out.SetNumUninitialized(Num);
		FRenderTransform* const Dest = Out.GetData();
		const FInstancedStaticMeshInstanceData* const From = Source.GetData();
		ParallelFor(TEXT("APS_StarCatalogueGatherTask"), FMath::DivideAndRoundUp(Num, GatherBatch), 1,
			[Dest, From, Num, Offset](const int32 Batch)
			{
				const int32 End = FMath::Min((Batch + 1) * GatherBatch, Num);
				for (int32 Index = Batch * GatherBatch; Index < End; ++Index)
				{
					Dest[Index] = FRenderTransform(From[Index].Transform.ConcatTranslation(Offset));
				}
			});
	}

	FAutoConsoleCommandWithWorld ReportCommand(
		TEXT("aps.Stars.CatalogueReport"),
		TEXT("Logs the galaxy and cluster catalogues' instanced components: class, mesh, material, instances, custom floats, gather."),
		FConsoleCommandWithWorldDelegate::CreateLambda([](UWorld*)
		{
			for (TObjectIterator<UHierarchicalInstancedStaticMeshComponent> It; It; ++It)
			{
				UHierarchicalInstancedStaticMeshComponent* Mesh = *It;
				const AActor* Owner = IsValid(Mesh) ? Mesh->GetOwner() : nullptr;
				const UWorld* World = Owner ? Owner->GetWorld() : nullptr;
				if (!World || !World->IsGameWorld() || !(Owner->IsA<AGalaxy>() || Owner->IsA<AStarCluster>()))
				{
					continue;
				}
				const UAPSStarCatalogueInstancesComponent* Catalogue = Cast<UAPSStarCatalogueInstancesComponent>(Mesh);
				UE_LOG(LogTemp, Log, TEXT("[APS.Stars] catalogue report: %s.%s class=%s mesh=%s material=%s instances=%d customFloats=%d forcedLod=%d translatedSpace=%d visible=%d proxy=%d gameplaySky=%d gather=%d"),
					*Owner->GetName(), *Mesh->GetName(), *Mesh->GetClass()->GetName(), *GetNameSafe(Mesh->GetStaticMesh()),
					*GetNameSafe(Mesh->GetMaterial(0)), Mesh->GetInstanceCount(), Mesh->NumCustomDataFloats, Mesh->ForcedLodModel,
					Mesh->bUseTranslatedInstanceSpace ? 1 : 0, Mesh->IsVisible() ? 1 : 0, Mesh->SceneProxy ? 1 : 0,
					Catalogue && Catalogue->IsGameplaySkyCatalogue() ? 1 : 0, CVarParallelGather.GetValueOnGameThread());
			}
		}));
}

UAPSStarCatalogueInstancesComponent::UAPSStarCatalogueInstancesComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UAPSStarCatalogueInstancesComponent::OnUpdateTransform(const EUpdateTransformFlags UpdateTransformFlags,
	const ETeleportType Teleport)
{
	Super::OnUpdateTransform(UpdateTransformFlags, Teleport);
	// Every normal sky move (PlaceSkyCatalogue, a carried flow/pay) passes here on the game thread before the end-of-frame
	// flush that gathers; BuildComponentInstanceData may run on a worker in a cooked build. A world shift may or may not pass
	// here (USceneComponent::ApplyWorldOffset calls OnUpdateTransform; APSWorldOrigin's instanced resend calls the ISM's
	// UpdateComponentTransform directly): either way the flag keeps the same value, as a shift does not change the hierarchy.
	bool bSky = false;
	if (APSStarCatalogueInstances::CVarParallelGather.GetValueOnAnyThread() > 0 && IsInGameThread())
	{
		const UWorld* World = GetWorld();
		const AActor* Owner = GetOwner();
		if (World && World->IsGameWorld() && Owner)
		{
			int32 Depth = 0;
			for (const AActor* Parent = Owner->GetAttachParentActor(); Parent && Depth < 8;
				Parent = Parent->GetAttachParentActor(), ++Depth)
			{
				if (const AAstroGenerator* Generator = Cast<AAstroGenerator>(Parent))
				{
					// The stellar view's own rule (APSWorldOrigin::IsStellarViewGenerator): never the menu preview.
					bSky = !Generator->ActorHasTag(TEXT("WorldGenerationPreview")) && !Generator->UsesContinuousPreviewFrame();
					break;
				}
			}
		}
	}
	bGameplaySky = bSky;
}

void UAPSStarCatalogueInstancesComponent::BuildComponentInstanceData(const ERHIFeatureLevel::Type FeatureLevel,
	FInstanceUpdateComponentDesc& OutData)
{
	Super::BuildComponentInstanceData(FeatureLevel, OutData);
	const int32 Mode = APSStarCatalogueInstances::CVarParallelGather.GetValueOnAnyThread();
	if (Mode <= 0 || !bGameplaySky
		|| PerInstanceSMData.Num() < FMath::Max(APSStarCatalogueInstances::CVarParallelGatherMin.GetValueOnAnyThread(), 1))
	{
		return; // the engine's change set builder, untouched
	}
	// UHierarchicalInstancedStaticMeshComponent::BuildComponentInstanceData's builder (UE 5.4,
	// HierarchicalInstancedStaticMesh.cpp:2602-2608), only its transform gather replaced when it covers every instance.
	OutData.BuildChangeSet = [this, Mode](FISMInstanceUpdateChangeSet& ChangeSet)
	{
		BuildInstanceDataDeltaChangeSetCommon(ChangeSet);
		const FVector Offset = -TranslatedInstanceSpaceOrigin;
		const auto Delta = ChangeSet.GetTransformDelta();
		const bool bEvery = !Delta.IsDelta() && !Delta.IsEmpty() && Delta.GetNumItems() == PerInstanceSMData.Num();
		if (!bEvery)
		{
			ChangeSet.SetInstanceTransforms(MakeStridedView(PerInstanceSMData, &FInstancedStaticMeshInstanceData::Transform), Offset);
		}
		else if (Mode == 1)
		{
			TRACE_CPUPROFILER_EVENT_SCOPE(APS_StarCatalogueGather);
			APSStarCatalogueInstances::ParallelGather(PerInstanceSMData, Offset, ChangeSet.Transforms);
			if (!bLoggedParallel)
			{
				bLoggedParallel = true;
				UE_LOG(LogTemp, Log, TEXT("[APS.Stars] catalogue gather: %s's %d instance transforms are gathered on the task workers when the sky moves (aps.Stars.CatalogueParallelGather 1)"),
					*GetNameSafe(GetOwner()), PerInstanceSMData.Num());
			}
		}
		else
		{
			{
				TRACE_CPUPROFILER_EVENT_SCOPE(APS_StarCatalogueGather_Engine);
				ChangeSet.SetInstanceTransforms(MakeStridedView(PerInstanceSMData, &FInstancedStaticMeshInstanceData::Transform), Offset);
			}
			TArray<FRenderTransform> ParallelTransforms;
			{
				TRACE_CPUPROFILER_EVENT_SCOPE(APS_StarCatalogueGather);
				APSStarCatalogueInstances::ParallelGather(PerInstanceSMData, Offset, ParallelTransforms);
			}
			CountVerify(ChangeSet.Transforms, ParallelTransforms);
		}
		ChangeSet.SetInstancePrevTransforms(MakeArrayView(PerInstancePrevTransform), Offset);
		ChangeSet.LegacyInstanceReorderTable = InstanceReorderTable;
	};
}

void UAPSStarCatalogueInstancesComponent::CountVerify(const TArray<FRenderTransform>& EngineTransforms,
	const TArray<FRenderTransform>& ParallelTransforms)
{
	++VerifyFlushes;
	int32 FirstDifference = INDEX_NONE;
	if (EngineTransforms.Num() != ParallelTransforms.Num())
	{
		FirstDifference = 0;
	}
	else if (FMemory::Memcmp(EngineTransforms.GetData(), ParallelTransforms.GetData(),
		EngineTransforms.Num() * sizeof(FRenderTransform)) != 0)
	{
		for (int32 Index = 0; Index < EngineTransforms.Num(); ++Index)
		{
			if (FMemory::Memcmp(&EngineTransforms[Index], &ParallelTransforms[Index], sizeof(FRenderTransform)) != 0)
			{
				FirstDifference = Index;
				break;
			}
		}
	}
	if (FirstDifference != INDEX_NONE && ++VerifyMismatches <= 5)
	{
		UE_LOG(LogTemp, Error, TEXT("[APS.Stars] catalogue gather verify: %s flush %lld differs at instance %d of %d/%d (the engine's data was sent)"),
			*GetNameSafe(GetOwner()), VerifyFlushes, FirstDifference, EngineTransforms.Num(), ParallelTransforms.Num());
	}
	const double Now = FPlatformTime::Seconds();
	if (Now - VerifyLogSeconds >= 1.0)
	{
		VerifyLogSeconds = Now;
		UE_LOG(LogTemp, Log, TEXT("[APS.Stars] catalogue gather verify: %s %lld whole gathers of %d instances compared, %lld differ (aps.Stars.CatalogueParallelGather 2)"),
			*GetNameSafe(GetOwner()), VerifyFlushes, EngineTransforms.Num(), VerifyMismatches);
	}
}
