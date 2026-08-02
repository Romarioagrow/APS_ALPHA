#include "StarCluster.h"
#include "Star.h"
#include "APS_ALPHA/Core/Enums/StarClusterType.h"
#include "Misc/Crc.h"

AStarCluster::AStarCluster()
{
    PrimaryActorTick.bCanEverTick = false;

    StarMeshInstances = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("StarMeshInstances"));
    RootComponent = Cast<USceneComponent>(StarMeshInstances);
	StarMeshInstances->NumCustomDataFloats = 6;
	StarMeshInstances->bAutoRebuildTreeOnInstanceChanges = false;
	StarMeshInstances->bUseTranslatedInstanceSpace = true;
	StarMeshInstances->bEnableDensityScaling = false;
	StarMeshInstances->bNeverDistanceCull = true;
	StarMeshInstances->SetCullDistances(0, 0);
	StarMeshInstances->bDisableCollision = true;
	StarMeshInstances->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	StarMeshInstances->SetGenerateOverlapEvents(false);
	StarMeshInstances->SetCanEverAffectNavigation(false);
	StarMeshInstances->bEvaluateWorldPositionOffset = false;
	StarMeshInstances->bWorldPositionOffsetWritesVelocity = false;
	StarMeshInstances->SetCastShadow(false);
	StarMeshInstances->bAffectDynamicIndirectLighting = false;
	StarMeshInstances->bAffectDistanceFieldLighting = false;
	StarMeshInstances->SetReceivesDecals(false);

    // ������������� �������� �� ���������
    StarAmount = 100;
    StarDensity = 1.0f;
    ClusterType = EStarClusterType::OpenCluster;
    ClusterBounds = FVector(1000.0f, 1000.0f, 1000.0f);
}

void AStarCluster::AddStarToCluster(AStar* Star)
{
    Stars.Add(Star);
}

void AStarCluster::AddStarToClusterModel(FVector StarLocation, TSharedPtr<FStarModel> StarModel)
{
    StarsModel.Add(StarLocation, StarModel);
}

FGuid AStarCluster::MakeStableSystemId(int32 InstanceIndex) const
{
	const uint32 SeedHash = GetTypeHash(GenerationSeed);
	const uint32 IndexHash = GetTypeHash(InstanceIndex);
	const uint32 ClusterHash = FCrc::StrCrc32(*GetClass()->GetPathName());
	return FGuid(
		HashCombine(SeedHash, IndexHash),
		HashCombine(IndexHash, ClusterHash),
		HashCombine(ClusterHash, SeedHash ^ 0x9E3779B9u),
		HashCombine(SeedHash ^ 0x85EBCA6Bu, IndexHash ^ 0xC2B2AE35u));
}

void AStarCluster::RegisterPotentialSystem(int32 InstanceIndex, const FVector& ClusterLocalLocation,
	const FStarModel& PrimaryStarModel, const FStarSystemModel& SystemModel)
{
	if (InstanceIndex < 0)
	{
		return;
	}
	if (PotentialStarSystems.Num() <= InstanceIndex)
	{
		PotentialStarSystems.SetNum(InstanceIndex + 1);
	}

	FClusterStarSystemRecord& Record = PotentialStarSystems[InstanceIndex];
	Record.StableId = MakeStableSystemId(InstanceIndex);
	Record.InstanceIndex = InstanceIndex;
	Record.ClusterLocalLocation = ClusterLocalLocation;
	Record.SystemModel = SystemModel;
	Record.SystemModel.StableId = Record.StableId;
	Record.SystemModel.Location = ClusterLocalLocation;
	Record.PrimaryStarModel = PrimaryStarModel;
	Record.PrimaryStarModel.Location = ClusterLocalLocation;
	Record.bMaterialized = false;
	Record.MaterializedSystem.Reset();
}

const FClusterStarSystemRecord* AStarCluster::FindPotentialSystem(int32 InstanceIndex) const
{
	return PotentialStarSystems.IsValidIndex(InstanceIndex)
		&& PotentialStarSystems[InstanceIndex].InstanceIndex == InstanceIndex
		? &PotentialStarSystems[InstanceIndex] : nullptr;
}

FClusterStarSystemRecord* AStarCluster::FindPotentialSystemMutable(int32 InstanceIndex)
{
	return PotentialStarSystems.IsValidIndex(InstanceIndex)
		&& PotentialStarSystems[InstanceIndex].InstanceIndex == InstanceIndex
		? &PotentialStarSystems[InstanceIndex] : nullptr;
}

FVector AStarCluster::GetPotentialSystemWorldLocation(const FClusterStarSystemRecord& Record) const
{
	return StarMeshInstances
		? StarMeshInstances->GetComponentTransform().TransformPosition(Record.ClusterLocalLocation)
		: GetActorTransform().TransformPosition(Record.ClusterLocalLocation);
}

bool AStarCluster::GetPotentialSystemRecord(int32 InstanceIndex, FClusterStarSystemRecord& OutRecord) const
{
	if (const FClusterStarSystemRecord* Record = FindPotentialSystem(InstanceIndex))
	{
		OutRecord = *Record;
		return true;
	}
	return false;
}

void AStarCluster::FinalizeGeneratedInstances()
{
	if (!StarMeshInstances)
	{
		return;
	}
	StarMeshInstances->BuildTreeIfOutdated(true, true);
	StarMeshInstances->MarkRenderStateDirty();
}

// ������� ��������� ���������
void AStarCluster::GenerateCluster()
{
    // ������ ��������� �����
}
