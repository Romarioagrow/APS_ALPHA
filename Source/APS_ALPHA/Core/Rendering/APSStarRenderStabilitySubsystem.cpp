#include "APSStarRenderStabilitySubsystem.h"

#include "APS_ALPHA/Actors/Astro/Galaxy.h"
#include "APS_ALPHA/Actors/Astro/StarCluster.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Level.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Materials/MaterialInterface.h"
#include "TimerManager.h"

namespace APSRenderStability
{
	// These are the seven assets reported by UE 5.4's Nanite material audit in the current
	// gameplay map. Keeping the allow-list explicit avoids changing valid Nanite content.
	const TSet<FName> TranslucentNaniteFallbackMeshes{
		TEXT("SM_MERGED_StaticMeshActor_337"),
		TEXT("SM_MERGED_BP_SplineElevator_Vert36_2"),
		TEXT("SM_MERGED_StaticMeshActor_2"),
		TEXT("SM_MERGED_StaticMeshActor_124"),
		TEXT("SM_MERGED_StaticMeshActor_388"),
		TEXT("KB3D_MTM_BldgLgTerraformer_A_StairsB"),
		TEXT("SM_MERGED_STATION_RING")
	};
}

void UAPSStarRenderStabilitySubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	if (UWorld* World = GetWorld())
	{
		ActorSpawnedHandle = World->AddOnActorSpawnedHandler(
			FOnActorSpawned::FDelegate::CreateUObject(this, &UAPSStarRenderStabilitySubsystem::HandleActorSpawned));
	}
}

void UAPSStarRenderStabilitySubsystem::Deinitialize()
{
	if (UWorld* World = GetWorld(); World && ActorSpawnedHandle.IsValid())
	{
		World->RemoveOnActorSpawnedHandler(ActorSpawnedHandle);
	}
	ActorSpawnedHandle.Reset();
	Super::Deinitialize();
}

bool UAPSStarRenderStabilitySubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE || WorldType == EWorldType::GamePreview;
}

void UAPSStarRenderStabilitySubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);
	int32 StabilizedComponentCount = 0;
	int32 NaniteFallbackCount = 0;
	for (ULevel* Level : InWorld.GetLevels())
	{
		if (!Level)
		{
			continue;
		}
		for (AActor* Actor : Level->Actors)
		{
			if (!IsValid(Actor))
			{
				continue;
			}
			TArray<UStaticMeshComponent*> MeshComponents;
			Actor->GetComponents(MeshComponents);
			for (UStaticMeshComponent* MeshComponent : MeshComponents)
			{
				if (StabilizeTranslucentNaniteMesh(MeshComponent))
				{
					++NaniteFallbackCount;
				}
				if (Actor->IsA<AStarCluster>() || Actor->IsA<AGalaxy>())
				{
					if (UHierarchicalInstancedStaticMeshComponent* Instances =
						Cast<UHierarchicalInstancedStaticMeshComponent>(MeshComponent))
					{
						StabilizeInstances(Instances);
						++StabilizedComponentCount;
					}
				}
			}
		}
	}
	UE_LOG(LogTemp, Log,
		TEXT("[APS.Render] Stabilized stars=%d, translucent Nanite fallbacks=%d"),
		StabilizedComponentCount, NaniteFallbackCount);
}

void UAPSStarRenderStabilitySubsystem::HandleActorSpawned(AActor* Actor)
{
	StabilizeActor(Actor);

	// AddOnActorSpawned is called before Blueprint construction and before procedural generation.
	// Re-apply on the next game tick so serialized BP component defaults cannot restore animated WPO.
	if (UWorld* World = GetWorld(); World && IsValid(Actor)
		&& (Actor->IsA<AStarCluster>() || Actor->IsA<AGalaxy>()))
	{
		const TWeakObjectPtr<AActor> WeakActor(Actor);
		World->GetTimerManager().SetTimerForNextTick(FTimerDelegate::CreateWeakLambda(this,
			[this, WeakActor]()
			{
				StabilizeActor(WeakActor.Get());
			}));
	}
}

void UAPSStarRenderStabilitySubsystem::StabilizeActor(AActor* Actor) const
{
	if (!IsValid(Actor))
	{
		return;
	}
	TArray<UStaticMeshComponent*> MeshComponents;
	Actor->GetComponents(MeshComponents);
	for (UStaticMeshComponent* MeshComponent : MeshComponents)
	{
		StabilizeTranslucentNaniteMesh(MeshComponent);
		if (Actor->IsA<AStarCluster>() || Actor->IsA<AGalaxy>())
		{
			if (UHierarchicalInstancedStaticMeshComponent* Instances =
				Cast<UHierarchicalInstancedStaticMeshComponent>(MeshComponent))
			{
				StabilizeInstances(Instances);
			}
		}
	}
}

bool UAPSStarRenderStabilitySubsystem::StabilizeTranslucentNaniteMesh(UStaticMeshComponent* Component)
{
	UStaticMesh* Mesh = IsValid(Component) ? Component->GetStaticMesh() : nullptr;
	if (!Mesh || !APSRenderStability::TranslucentNaniteFallbackMeshes.Contains(Mesh->GetFName()))
	{
		return false;
	}

	bool bHasUnsupportedBlend = false;
	for (int32 MaterialIndex = 0; MaterialIndex < Component->GetNumMaterials(); ++MaterialIndex)
	{
		const UMaterialInterface* Material = Component->GetMaterial(MaterialIndex);
		if (Material && Material->GetBlendMode() != BLEND_Opaque && Material->GetBlendMode() != BLEND_Masked)
		{
			bHasUnsupportedBlend = true;
			break;
		}
	}
	if (!bHasUnsupportedBlend)
	{
		return false;
	}

	// UE 5.4 does not support translucent Nanite sections. The regular fallback proxy renders
	// the intended glass material instead of repeatedly auditing and substituting it.
	Component->bDisallowNanite = true;
	Component->SetForceDisableNanite(true);
	Component->MarkRenderStateDirty();
	return true;
}

void UAPSStarRenderStabilitySubsystem::StabilizeInstances(
	UHierarchicalInstancedStaticMeshComponent* Instances)
{
	if (!IsValid(Instances))
	{
		return;
	}

	// HISM cluster bounds and instance transforms are single precision in UE 5.4. The
	// translated tree mode exists specifically for large-coordinate precision loss.
	Instances->bUseTranslatedInstanceSpace = true;
	Instances->bEnableDensityScaling = false;
	Instances->bNeverDistanceCull = true;
	Instances->bAutoRebuildTreeOnInstanceChanges = false;
	Instances->SetCullDistances(0, 0);
	Instances->bDisableCollision = true;
	Instances->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Instances->SetGenerateOverlapEvents(false);
	Instances->SetCanEverAffectNavigation(false);

	// The legacy star material contains animated SimpleGrassWind/Noise WPO. That makes
	// sub-pixel emissive stars change geometry independently on every temporal sample,
	// which becomes severe flicker at relativistic camera velocities.
	Instances->bEvaluateWorldPositionOffset = false;
	Instances->bWorldPositionOffsetWritesVelocity = false;
	Instances->SetCastShadow(false);
	Instances->bAffectDynamicIndirectLighting = false;
	Instances->bAffectDistanceFieldLighting = false;
	Instances->SetReceivesDecals(false);
	Instances->bUseAsOccluder = false;

	// Cluster stars are sub-pixel impostors at gameplay distance. Switching AutoLOD sphere
	// silhouettes while TSR jitters the projection creates visible pulses, so keep the cheapest
	// authored LOD until the selected record is materialized as a real star actor.
	UStaticMesh* Mesh = Instances->GetStaticMesh();
	const int32 NumLODs = Mesh ? Mesh->GetNumLODs() : 0;
	if (NumLODs > 1)
	{
		Instances->SetForcedLodModel(NumLODs);
	}
	// BuildTreeIfOutdated is cheap when the tree is current and mandatory after generators
	// populated a component whose automatic rebuilds are deliberately disabled.
	if (Instances->GetInstanceCount() > 0)
	{
		Instances->BuildTreeIfOutdated(true, true);
	}
	Instances->MarkRenderStateDirty();
	UE_LOG(LogTemp, Log,
		TEXT("[APS.Render] Stable star instances component=%s instances=%d mesh=%s lods=%d forced=%d translated=%d"),
		*Instances->GetPathName(), Instances->GetInstanceCount(), *GetNameSafe(Mesh), NumLODs,
		Instances->ForcedLodModel, Instances->bUseTranslatedInstanceSpace ? 1 : 0);
}
