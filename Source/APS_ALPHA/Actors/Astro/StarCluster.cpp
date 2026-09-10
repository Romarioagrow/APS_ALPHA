#include "StarCluster.h"
#include "Star.h"
#include "APS_ALPHA/Core/Enums/StarClusterType.h"
#include "APS_ALPHA/Core/Rendering/APSStarRenderStabilitySubsystem.h"
#include "APS_ALPHA/Core/Rendering/APSStellarMaterialContract.h"
#include "Misc/Crc.h"
#include "UObject/ConstructorHelpers.h"

AStarCluster::AStarCluster()
{
    PrimaryActorTick.bCanEverTick = false;

    StarMeshInstances = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("StarMeshInstances"));
    RootComponent = Cast<USceneComponent>(StarMeshInstances);
	// Stellar points use an additive material. Nanite does not support that blend
	// mode, so make the fallback an invariant of the component rather than relying
	// on the later world-level stabilization pass.
	StarMeshInstances->bDisallowNanite = true;
	StarMeshInstances->SetForceDisableNanite(true);
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
	static ConstructorHelpers::FObjectFinder<UMaterial> CanonicalHismMaterial(
		APSStellarMaterialContract::HismBaseObjectPath);
	if (CanonicalHismMaterial.Succeeded())
	{
		StarMeshInstances->SetMaterial(0, CanonicalHismMaterial.Object);
	}

    // ������������� �������� �� ���������
    StarAmount = 100;
    StarDensity = 1.0f;
    ClusterType = EStarClusterType::OpenCluster;
    ClusterBounds = FVector(1000.0f, 1000.0f, 1000.0f);
}

void AStarCluster::PostInitializeComponents()
{
	Super::PostInitializeComponents();
	EnsureCanonicalStellarMaterial();
}

bool AStarCluster::EnsureCanonicalStellarMaterial()
{
	if (!IsValid(StarMeshInstances))
	{
		return false;
	}

	// Blueprint defaults can serialize the mesh/material after the native
	// constructor. Reassert the additive-rendering contract once components are
	// initialized and before every material-validation early return.
	StarMeshInstances->bDisallowNanite = true;
	StarMeshInstances->SetForceDisableNanite(true);

	UMaterial* CanonicalBase = APSStellarMaterialContract::LoadCanonicalBase(
		APSStellarMaterialContract::HismBaseObjectPath);
	if (!IsValid(CanonicalBase))
	{
		UE_LOG(LogTemp, Error,
			TEXT("[APS.StellarMaterial] Canonical cluster HISM master is unavailable: %s"),
			APSStellarMaterialContract::HismBaseObjectPath);
		return false;
	}

	UMaterialInterface* CurrentMaterial = StarMeshInstances->GetMaterial(0);
	if (APSStellarMaterialContract::HasExactBase(
		CurrentMaterial, APSStellarMaterialContract::HismBaseObjectPath))
	{
		return true;
	}

	UMaterial* CurrentBase = APSStellarMaterialContract::GetBaseMaterial(CurrentMaterial);
	UE_LOG(LogTemp, Warning,
		TEXT("[APS.StellarMaterial] Replacing %s cluster HISM base %s with canonical %s%s"),
		*GetNameSafe(this),
		IsValid(CurrentBase) ? *CurrentBase->GetPathName() : TEXT("<null>"),
		APSStellarMaterialContract::HismBaseObjectPath,
		APSStellarMaterialContract::UsesWorldGrid(CurrentMaterial)
			? TEXT(" (WorldGrid fallback)") : TEXT(""));
	StarMeshInstances->SetMaterial(0, CanonicalBase);
	return APSStellarMaterialContract::HasExactBase(
		StarMeshInstances->GetMaterial(0), APSStellarMaterialContract::HismBaseObjectPath);
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

void AStarCluster::RegisterPotentialSystem(
	const int32 InstanceIndex, const FVector& ClusterLocalLocation,
	const FStarModel& PrimaryStarModel, const FStarSystemModel& SystemModel)
{
	const FTransform LegacyProxyTransform(
		FQuat::Identity, ClusterLocalLocation, FVector(PrimaryStarModel.Radius));
	RegisterPotentialSystem(InstanceIndex, ClusterLocalLocation, LegacyProxyTransform,
		PrimaryStarModel, SystemModel);
}

void AStarCluster::RegisterPotentialSystem(int32 InstanceIndex, const FVector& ClusterLocalLocation,
	const FTransform& ProxyBaseTransform, const FStarModel& PrimaryStarModel,
	const FStarSystemModel& SystemModel)
{
	if (InstanceIndex < 0)
	{
		return;
	}
	if (PotentialStarSystems.Num() <= InstanceIndex)
	{
		PotentialStarSystems.SetNum(InstanceIndex + 1);
	}
	if (SystemProxyBaseTransforms.Num() <= InstanceIndex)
	{
		SystemProxyBaseTransforms.SetNum(InstanceIndex + 1);
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
	SystemProxyBaseTransforms[InstanceIndex] = ProxyBaseTransform;
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
	if (SystemProxyBaseTransforms.IsValidIndex(Record.InstanceIndex))
	{
		const FTransform ComponentTransform = StarMeshInstances
			? StarMeshInstances->GetComponentTransform() : GetActorTransform();
		return ComponentTransform.TransformPosition(
			SystemProxyBaseTransforms[Record.InstanceIndex].GetLocation());
	}
	if (!CanonicalProjectionFrame.bEnabled && StarMeshInstances)
	{
		FTransform LegacyTransform;
		if (StarMeshInstances->GetInstanceTransform(
			Record.InstanceIndex, LegacyTransform, false))
		{
			return StarMeshInstances->GetComponentTransform().TransformPosition(
				LegacyTransform.GetLocation());
		}
	}
	return GetActorLocation();
}

FVector AStarCluster::GetPotentialSystemPresentedWorldLocation(
	const FClusterStarSystemRecord& Record) const
{
	FTransform LocalTransform;
	if (StarMeshInstances && StarMeshInstances->GetInstanceTransform(
		Record.InstanceIndex, LocalTransform, false))
	{
		return StarMeshInstances->GetComponentTransform().TransformPosition(
			LocalTransform.GetLocation());
	}
	return GetPotentialSystemWorldLocation(Record);
}

bool AStarCluster::GetPotentialSystemCanonicalRootLocationCm(
	const FClusterStarSystemRecord& Record, FVector& OutCanonicalRootCm) const
{
	OutCanonicalRootCm = FVector::ZeroVector;
	if (!CanonicalProjectionFrame.bEnabled || !CanonicalProjectionFrame.IsFinite()
		|| !PotentialStarSystems.IsValidIndex(Record.InstanceIndex)
		|| PotentialStarSystems[Record.InstanceIndex].StableId != Record.StableId)
	{
		return false;
	}
	OutCanonicalRootCm = CanonicalProjectionFrame.GetCanonicalRootPositionCm(
		Record.ClusterLocalLocation);
	return !OutCanonicalRootCm.ContainsNaN();
}

bool AStarCluster::GetPotentialSystemBaseProxyTransform(
	const int32 InstanceIndex, FTransform& OutTransform) const
{
	if (!SystemProxyBaseTransforms.IsValidIndex(InstanceIndex))
	{
		return false;
	}
	OutTransform = SystemProxyBaseTransforms[InstanceIndex];
	return true;
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
	UAPSStarRenderStabilitySubsystem::StabilizeInstances(StarMeshInstances);
}

// ������� ��������� ���������
void AStarCluster::GenerateCluster()
{
    // ������ ��������� �����
}
