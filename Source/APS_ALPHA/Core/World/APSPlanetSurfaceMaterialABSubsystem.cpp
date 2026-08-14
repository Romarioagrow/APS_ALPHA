#include "APSPlanetSurfaceMaterialABSubsystem.h"

#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "WorldScapeCore/Public/WorldScapeRoot.h"

DEFINE_LOG_CATEGORY_STATIC(LogAPSPlanetSurfaceMaterialAB, Log, All);

namespace APSPlanetSurfaceMaterialAB
{
	TAutoConsoleVariable<int32> CVarPreset(
		TEXT("aps.Surface.Diagnostics.MaterialAB"),
		0,
		TEXT("Temporary anchored WorldScape material A/B preset.\n")
		TEXT("0: disabled; restore accepted material values (default)\n")
		TEXT("1: disable all near color/detail normal/detail roughness\n")
		TEXT("2: disable only LOD seam radial-normal blend\n")
		TEXT("3: force radial normal and disable detail normal\n")
		TEXT("4: disable only near color\n")
		TEXT("5: disable only detail roughness"),
		ECVF_Cheat);

	constexpr float SampleIntervalSeconds = 0.25f;
	const FName NearColorStrength(TEXT("NearColorStrength"));
	const FName DetailNormalStrength(TEXT("DetailNormalStrength"));
	const FName DetailRoughnessStrength(TEXT("DetailRoughnessStrength"));
	const FName LodSeamNormalFadeStart(TEXT("LodSeamNormalFadeStartCm"));
	const FName LodSeamNormalFadeEnd(TEXT("LodSeamNormalFadeEndCm"));
	const FName LodSeamNormalFarBlend(TEXT("LodSeamNormalFarBlend"));
	const FName TrackedParameters[] = {
		NearColorStrength,
		DetailNormalStrength,
		DetailRoughnessStrength,
		LodSeamNormalFadeStart,
		LodSeamNormalFadeEnd,
		LodSeamNormalFarBlend};

	const TCHAR* PresetName(const int32 Preset)
	{
		switch (Preset)
		{
		case 1: return TEXT("near-all-off");
		case 2: return TEXT("seam-normal-off");
		case 3: return TEXT("radial-normal-forced");
		case 4: return TEXT("near-color-off");
		case 5: return TEXT("detail-roughness-off");
		default: return TEXT("accepted-baseline");
		}
	}
}

bool UAPSPlanetSurfaceMaterialABSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	const UWorld* World = Cast<UWorld>(Outer);
	return World && (World->WorldType == EWorldType::Game
		|| World->WorldType == EWorldType::PIE);
}

void UAPSPlanetSurfaceMaterialABSubsystem::Deinitialize()
{
	RestoreActiveMaterial();
	Super::Deinitialize();
}

TStatId UAPSPlanetSurfaceMaterialABSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(
		UAPSPlanetSurfaceMaterialABSubsystem, STATGROUP_Tickables);
}

UMaterialInstanceDynamic*
UAPSPlanetSurfaceMaterialABSubsystem::ResolveAnchoredTerrainMaterial(
	AWorldScapeRoot*& OutRoot, UWorldScapeLod*& OutLod0) const
{
	OutRoot = nullptr;
	OutLod0 = nullptr;
	UWorld* World = GetWorld();
	APlayerController* Controller = World ? World->GetFirstPlayerController() : nullptr;
	APawn* Pawn = Controller ? Controller->GetPawn() : nullptr;
	if (!IsValid(World) || !IsValid(Pawn))
	{
		return nullptr;
	}

	AWorldScapeRoot* BestRoot = nullptr;
	double BestErrorSq = TNumericLimits<double>::Max();
	for (TActorIterator<AWorldScapeRoot> It(World); It; ++It)
	{
		AWorldScapeRoot* Root = *It;
		if (!IsValid(Root) || !Root->bOverridePlayerPosition
			|| !Root->CollisionDependantActor.Contains(Pawn))
		{
			continue;
		}
		const double ErrorSq = FVector::DistSquared(
			Root->OverridedPlayerPosition, Pawn->GetActorLocation());
		if (ErrorSq < BestErrorSq)
		{
			BestErrorSq = ErrorSq;
			BestRoot = Root;
		}
	}
	if (!IsValid(BestRoot) || !BestRoot->WorldScapeLod.IsValidIndex(0))
	{
		OutRoot = BestRoot;
		return nullptr;
	}
	OutRoot = BestRoot;
	OutLod0 = BestRoot->WorldScapeLod[0];
	return IsValid(OutLod0) && IsValid(OutLod0->Mesh)
		? Cast<UMaterialInstanceDynamic>(OutLod0->Mesh->GetMaterial(0)) : nullptr;
}

void UAPSPlanetSurfaceMaterialABSubsystem::LogLodLifecycleState(
	const AWorldScapeRoot* Root, const UWorldScapeLod* Lod0, const int32 Preset)
{
	UWorldScapeMeshComponent* Mesh = IsValid(Lod0) ? Lod0->Mesh : nullptr;
	const bool bMeshValid = IsValid(Mesh);
	const int32 SectionCount = bMeshValid ? Mesh->GetNumSections() : 0;
	const FVector RootScale = IsValid(Root)
		? Root->GetActorScale3D() : FVector::ZeroVector;
	const FVector RelativeScale = bMeshValid
		? Mesh->GetRelativeScale3D() : FVector::ZeroVector;
	const bool bRegistered = bMeshValid && Mesh->IsRegistered();
	const bool bVisible = bMeshValid && Mesh->IsVisible();
	const bool bHiddenInGame = bMeshValid && Mesh->bHiddenInGame;
	const bool bSection0Visible = bMeshValid && SectionCount > 0
		&& Mesh->IsMeshSectionVisible(0);
	const bool bRootHidden = IsValid(Root) && Root->IsHidden();
	const bool bTransformKeeperParent = bMeshValid && IsValid(Root)
		&& Mesh->GetAttachParent() == Root->TransformKeeper;
	const bool bRootUnitScale = IsValid(Root)
		&& RootScale.Equals(FVector::OneVector, KINDA_SMALL_NUMBER);
	const bool bRelativeUnitScale = bMeshValid
		&& RelativeScale.Equals(FVector::OneVector, KINDA_SMALL_NUMBER);

	APawn* Pawn = GetWorld() && GetWorld()->GetFirstPlayerController()
		? GetWorld()->GetFirstPlayerController()->GetPawn() : nullptr;
	const bool bOverridePlayer = IsValid(Root) && Root->bOverridePlayerPosition;
	const bool bPawnInvoker = IsValid(Root) && IsValid(Pawn)
		&& Root->CollisionDependantActor.Contains(Pawn);
	const double OverrideErrorCm = IsValid(Root) && IsValid(Pawn)
		? FVector::Distance(Root->OverridedPlayerPosition, Pawn->GetActorLocation())
		: TNumericLimits<double>::Max();

	const uint32 RootId = IsValid(Root) ? Root->GetUniqueID() : 0;
	const uint32 Lod0Id = IsValid(Lod0) ? Lod0->GetUniqueID() : 0;
	const uint32 MeshId = bMeshValid ? Mesh->GetUniqueID() : 0;
	const FString StateKey = FString::Printf(
		TEXT("%d/%u/%u/%u/%d/%d/%d/%d/%d/%d/%d/%d/%d/%d/%d"),
		Preset, RootId, Lod0Id, MeshId, bRegistered ? 1 : 0,
		bVisible ? 1 : 0, bHiddenInGame ? 1 : 0, SectionCount,
		bSection0Visible ? 1 : 0, bRootHidden ? 1 : 0,
		bTransformKeeperParent ? 1 : 0, bRootUnitScale ? 1 : 0,
		bRelativeUnitScale ? 1 : 0, bOverridePlayer ? 1 : 0,
		bPawnInvoker ? 1 : 0);
	const FString State = FString::Printf(
		TEXT("preset=%d root=%s lod0=%s mesh=%s registered=%d visible=%d hiddenInGame=%d sections=%d section0Visible=%d rootHidden=%d transformKeeperParent=%d rootUnitScale=%d relativeUnitScale=%d rootScale=%s relativeScale=%s overridePlayer=%d pawnInvoker=%d overrideErrorCm=%.3f"),
		Preset, *GetNameSafe(Root), *GetNameSafe(Lod0), *GetNameSafe(Mesh),
		bRegistered ? 1 : 0, bVisible ? 1 : 0, bHiddenInGame ? 1 : 0,
		SectionCount, bSection0Visible ? 1 : 0, bRootHidden ? 1 : 0,
		bTransformKeeperParent ? 1 : 0, bRootUnitScale ? 1 : 0,
		bRelativeUnitScale ? 1 : 0, *RootScale.ToCompactString(),
		*RelativeScale.ToCompactString(), bOverridePlayer ? 1 : 0,
		bPawnInvoker ? 1 : 0, OverrideErrorCm);
	if (StateKey != LastLifecycleState)
	{
		UE_LOG(LogAPSPlanetSurfaceMaterialAB, Display,
			TEXT("[APS.Surface.MaterialAB.Lifecycle] action=state-change %s"), *State);
		LastLifecycleState = StateKey;
	}
}

void UAPSPlanetSurfaceMaterialABSubsystem::CaptureOriginals(
	UMaterialInstanceDynamic* Material)
{
	using namespace APSPlanetSurfaceMaterialAB;
	OriginalScalars.Reset();
	for (const FName Parameter : TrackedParameters)
	{
		OriginalScalars.Add(Parameter, Material->K2_GetScalarParameterValue(Parameter));
	}
}

void UAPSPlanetSurfaceMaterialABSubsystem::ApplyPreset(
	UMaterialInstanceDynamic* Material, const int32 Preset)
{
	using namespace APSPlanetSurfaceMaterialAB;
	if (!IsValid(Material) || Preset <= 0)
	{
		return;
	}
	if (Preset == 1 || Preset == 4)
	{
		Material->SetScalarParameterValue(NearColorStrength, 0.0f);
	}
	if (Preset == 1 || Preset == 3)
	{
		Material->SetScalarParameterValue(DetailNormalStrength, 0.0f);
	}
	if (Preset == 1 || Preset == 5)
	{
		Material->SetScalarParameterValue(DetailRoughnessStrength, 0.0f);
	}
	if (Preset == 2)
	{
		Material->SetScalarParameterValue(LodSeamNormalFarBlend, 0.0f);
	}
	else if (Preset == 3)
	{
		Material->SetScalarParameterValue(LodSeamNormalFadeStart, 0.0f);
		Material->SetScalarParameterValue(LodSeamNormalFadeEnd, 1.0f);
		Material->SetScalarParameterValue(LodSeamNormalFarBlend, 1.0f);
	}

	UE_LOG(LogAPSPlanetSurfaceMaterialAB, Display,
		TEXT("[APS.Surface.MaterialAB] action=apply preset=%d name=%s material=%s nearColor=%.4f detailNormal=%.4f detailRoughness=%.4f seam=[%.1f,%.1f,%.3f]"),
		Preset, PresetName(Preset), *Material->GetName(),
		Material->K2_GetScalarParameterValue(NearColorStrength),
		Material->K2_GetScalarParameterValue(DetailNormalStrength),
		Material->K2_GetScalarParameterValue(DetailRoughnessStrength),
		Material->K2_GetScalarParameterValue(LodSeamNormalFadeStart),
		Material->K2_GetScalarParameterValue(LodSeamNormalFadeEnd),
		Material->K2_GetScalarParameterValue(LodSeamNormalFarBlend));
}

void UAPSPlanetSurfaceMaterialABSubsystem::RestoreActiveMaterial()
{
	using namespace APSPlanetSurfaceMaterialAB;
	UMaterialInstanceDynamic* Material = ActiveMaterial.Get();
	if (IsValid(Material))
	{
		for (const TPair<FName, float>& Original : OriginalScalars)
		{
			Material->SetScalarParameterValue(Original.Key, Original.Value);
		}
		UE_LOG(LogAPSPlanetSurfaceMaterialAB, Display,
			TEXT("[APS.Surface.MaterialAB] action=restore material=%s"),
			*Material->GetName());
	}
	ActiveMaterial.Reset();
	OriginalScalars.Reset();
	AppliedPreset = 0;
}

void UAPSPlanetSurfaceMaterialABSubsystem::Tick(float DeltaTime)
{
	using namespace APSPlanetSurfaceMaterialAB;
	const int32 RequestedPreset = FMath::Clamp(CVarPreset.GetValueOnGameThread(), 0, 5);
	if (RequestedPreset == 0)
	{
		if (ActiveMaterial.IsValid())
		{
			RestoreActiveMaterial();
		}
		LastLifecycleState.Reset();
		return;
	}

	SampleElapsed += DeltaTime;
	if (SampleElapsed < SampleIntervalSeconds)
	{
		return;
	}
	SampleElapsed = 0.0f;
	AWorldScapeRoot* Root = nullptr;
	UWorldScapeLod* Lod0 = nullptr;
	UMaterialInstanceDynamic* Material = ResolveAnchoredTerrainMaterial(Root, Lod0);
	LogLodLifecycleState(Root, Lod0, RequestedPreset);
	if (!IsValid(Material))
	{
		return;
	}
	if (ActiveMaterial.Get() != Material || AppliedPreset != RequestedPreset)
	{
		RestoreActiveMaterial();
		ActiveMaterial = Material;
		CaptureOriginals(Material);
		ApplyPreset(Material, RequestedPreset);
		AppliedPreset = RequestedPreset;
	}
}
