#include "APSPlanetSurfaceDiagnosticsSubsystem.h"

#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "Materials/MaterialInterface.h"
#include "WorldScapeCore/Public/WorldScapeRoot.h"

DEFINE_LOG_CATEGORY_STATIC(LogAPSPlanetSurfaceDiagnostics, Log, All);

namespace
{
	TAutoConsoleVariable<int32> CVarAPSSurfaceAnchorDiagnostics(
		TEXT("aps.Surface.Diagnostics.Anchor"),
		0,
		TEXT("Opt-in WorldScape gameplay-anchor diagnostics.\n")
		TEXT("0: disabled (default)\n")
		TEXT("1: log anchor drift, LOD0 publications, material identity and origin"),
		ECVF_Default);

	TAutoConsoleVariable<float> CVarAPSSurfaceAnchorDiagnosticsInterval(
		TEXT("aps.Surface.Diagnostics.AnchorInterval"),
		0.25f,
		TEXT("Sampling interval in seconds for aps.Surface.Diagnostics.Anchor."),
		ECVF_Default);

	constexpr float HeartbeatSeconds = 2.0f;
}

bool UAPSPlanetSurfaceDiagnosticsSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	const UWorld* World = Cast<UWorld>(Outer);
	return World && (World->WorldType == EWorldType::Game || World->WorldType == EWorldType::PIE);
}

void UAPSPlanetSurfaceDiagnosticsSubsystem::Deinitialize()
{
	ResetSample();
	Super::Deinitialize();
}

TStatId UAPSPlanetSurfaceDiagnosticsSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(
		UAPSPlanetSurfaceDiagnosticsSubsystem, STATGROUP_Tickables);
}

AWorldScapeRoot* UAPSPlanetSurfaceDiagnosticsSubsystem::ResolveAnchoredRoot(
	const APawn* Observer) const
{
	UWorld* World = GetWorld();
	if (!World || !IsValid(Observer))
	{
		return nullptr;
	}

	AWorldScapeRoot* BestRoot = nullptr;
	double BestAnchorErrorSq = TNumericLimits<double>::Max();
	for (TActorIterator<AWorldScapeRoot> It(World); It; ++It)
	{
		AWorldScapeRoot* Root = *It;
		if (!IsValid(Root) || !Root->bOverridePlayerPosition
			|| !Root->CollisionDependantActor.Contains(Observer))
		{
			continue;
		}

		const double AnchorErrorSq = FVector::DistSquared(
			Root->OverridedPlayerPosition, Observer->GetActorLocation());
		if (AnchorErrorSq < BestAnchorErrorSq)
		{
			BestAnchorErrorSq = AnchorErrorSq;
			BestRoot = Root;
		}
	}
	return BestRoot;
}

void UAPSPlanetSurfaceDiagnosticsSubsystem::ResetSample()
{
	SampleElapsed = 0.0f;
	HeartbeatElapsed = 0.0f;
	LastLod0RelativePosition = FVector::ZeroVector;
	LastSnappedAngle = FVector::ZeroVector;
	LastVertexCount = INDEX_NONE;
	LastTriangleCount = INDEX_NONE;
	PublicationSequence = 0;
	bSampleValid = false;
	LastRoot.Reset();
	LastTerrainMaterial.Reset();
}

void UAPSPlanetSurfaceDiagnosticsSubsystem::Tick(float DeltaTime)
{
	if (CVarAPSSurfaceAnchorDiagnostics.GetValueOnGameThread() <= 0)
	{
		if (bSampleValid || LastRoot.IsValid())
		{
			ResetSample();
		}
		return;
	}

	SampleElapsed += DeltaTime;
	HeartbeatElapsed += DeltaTime;
	const float SampleInterval = FMath::Max(
		0.05f, CVarAPSSurfaceAnchorDiagnosticsInterval.GetValueOnGameThread());
	if (SampleElapsed < SampleInterval)
	{
		return;
	}
	SampleElapsed = 0.0f;

	UWorld* World = GetWorld();
	APlayerController* PlayerController = World ? World->GetFirstPlayerController() : nullptr;
	APawn* Observer = PlayerController ? PlayerController->GetPawn() : nullptr;
	AWorldScapeRoot* Root = ResolveAnchoredRoot(Observer);
	if (!IsValid(Root) || !IsValid(Observer))
	{
		if ((bSampleValid || LastRoot.IsValid())
			&& HeartbeatElapsed >= HeartbeatSeconds)
		{
			UE_LOG(LogAPSPlanetSurfaceDiagnostics, Display,
				TEXT("[APS.Surface.Diagnostics] event=anchor-lost root=%s pawn=%s"),
				*GetNameSafe(LastRoot.Get()), *GetNameSafe(Observer));
			ResetSample();
		}
		return;
	}

	UWorldScapeLod* Lod0 = Root->WorldScapeLod.IsValidIndex(0)
		? Root->WorldScapeLod[0] : nullptr;
	if (!IsValid(Lod0))
	{
		if (HeartbeatElapsed >= HeartbeatSeconds)
		{
			HeartbeatElapsed = 0.0f;
			UE_LOG(LogAPSPlanetSurfaceDiagnostics, Display,
				TEXT("[APS.Surface.Diagnostics] event=waiting-lod0 root=%s pawn=%s lodCount=%d anchorErrorCm=%.3f"),
				*Root->GetPathName(), *Observer->GetPathName(), Root->WorldScapeLod.Num(),
				FVector::Distance(Root->OverridedPlayerPosition,
					Observer->GetActorLocation()));
		}
		return;
	}

	const FVector Lod0RelativePosition = Lod0->RelativePosition.ToFVector();
	const FVector SnappedAngle = Lod0->SnappedAngle;
	const int32 VertexCount = Lod0->Vertices.Num();
	const int32 TriangleCount = Lod0->Triangles.Num();
	UMaterialInterface* TerrainMaterial = IsValid(Lod0->Mesh)
		? Lod0->Mesh->GetMaterial(0) : nullptr;
	const bool bRootChanged = LastRoot.Get() != Root;
	const bool bMaterialChanged = LastTerrainMaterial.Get() != TerrainMaterial;
	const bool bPublicationChanged = !bSampleValid
		|| bRootChanged
		|| !Lod0RelativePosition.Equals(LastLod0RelativePosition, 0.01f)
		|| !SnappedAngle.Equals(LastSnappedAngle, 1.0e-7f)
		|| VertexCount != LastVertexCount
		|| TriangleCount != LastTriangleCount;
	const bool bHeartbeat = HeartbeatElapsed >= HeartbeatSeconds;
	if (!bPublicationChanged && !bMaterialChanged && !bHeartbeat)
	{
		return;
	}

	const double Lod0ShiftCm = bSampleValid && !bRootChanged
		? FVector::Distance(LastLod0RelativePosition, Lod0RelativePosition)
		: 0.0;
	if (bPublicationChanged)
	{
		++PublicationSequence;
	}
	HeartbeatElapsed = 0.0f;
	const bool bLod0Generating = Root->WorldScapeLodInGeneration.FindRef(Lod0);
	const double AnchorErrorCm = FVector::Distance(
		Root->OverridedPlayerPosition, Observer->GetActorLocation());
	// WorldScape multiplies this nominal half-step by its altitude factor internally.
	// At ground level the factor is one, making the current 120 cm LOD0 resnap near 60 cm.
	const double NominalSnapThresholdCm = FMath::Max(
		0.0, static_cast<double>(Root->TriangleSize) * 0.5);
	const FIntVector WorldOrigin = World ? World->OriginLocation : FIntVector::ZeroValue;
	UE_LOG(LogAPSPlanetSurfaceDiagnostics, Display,
		TEXT("[APS.Surface.Diagnostics] event=%s sequence=%llu root=%s owner=%s pawn=%s anchorErrorCm=%.3f override=%s lod0Relative=%s lod0ShiftCm=%.3f snapped=%s nominalSnapThresholdCm=%.3f lod0Generating=%d vertices=%d triangles=%d material=%s materialChanged=%d origin=%s"),
		bPublicationChanged ? TEXT("lod0-publication")
			: (bMaterialChanged ? TEXT("material-change") : TEXT("heartbeat")),
		PublicationSequence, *Root->GetPathName(), *GetNameSafe(Root->GetOwner()),
		*Observer->GetPathName(), AnchorErrorCm,
		*Root->OverridedPlayerPosition.ToCompactString(),
		*Lod0RelativePosition.ToCompactString(), Lod0ShiftCm,
		*SnappedAngle.ToCompactString(), NominalSnapThresholdCm,
		bLod0Generating ? 1 : 0, VertexCount, TriangleCount,
		*GetNameSafe(TerrainMaterial), bMaterialChanged ? 1 : 0,
		*WorldOrigin.ToString());

	LastRoot = Root;
	LastTerrainMaterial = TerrainMaterial;
	LastLod0RelativePosition = Lod0RelativePosition;
	LastSnappedAngle = SnappedAngle;
	LastVertexCount = VertexCount;
	LastTriangleCount = TriangleCount;
	bSampleValid = true;
}
