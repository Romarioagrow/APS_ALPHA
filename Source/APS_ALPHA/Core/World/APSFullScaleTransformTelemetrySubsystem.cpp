#include "APSFullScaleTransformTelemetrySubsystem.h"

#include "APS_ALPHA/Actors/Astro/Galaxy.h"
#include "APS_ALPHA/Actors/Astro/StarCluster.h"
#include "APS_ALPHA/Actors/Astro/StarSystem.h"
#include "APS_ALPHA/Generation/AstroGenerator.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "UObject/UnrealType.h"

DEFINE_LOG_CATEGORY_STATIC(LogAPSFullScaleTransformTelemetry, Log, All);

namespace
{
	TAutoConsoleVariable<int32> CVarAPSFullScaleTransformTelemetry(
		TEXT("aps.FullScale.TransformTelemetry"),
		0,
		TEXT("Default-off post-generation transform stability telemetry.\n")
		TEXT("0: disabled\n")
		TEXT("1: sample generator, home system, pawn, camera, parent scales and world origin"),
		ECVF_Default);

	TAutoConsoleVariable<float> CVarAPSFullScaleTransformTelemetryInterval(
		TEXT("aps.FullScale.TransformTelemetryInterval"),
		0.0f,
		TEXT("Sampling interval in seconds for aps.FullScale.TransformTelemetry. "
			"0 samples every game-thread tick."),
		ECVF_Default);

	constexpr double StationarySpeedCmPerSecond = 0.10;
	constexpr double UnexpectedStationaryDeltaCm = 0.50;
	constexpr double RenderedViewStationaryDeltaCm = 0.01;
	constexpr double FarFieldPrecisionRiskDistanceCm = 1.0e12;

	template <typename TActor>
	TActor* FindFirstValidActor(UWorld* World)
	{
		if (!World)
		{
			return nullptr;
		}
		for (TActorIterator<TActor> It(World); It; ++It)
		{
			if (IsValid(*It))
			{
				return *It;
			}
		}
		return nullptr;
	}

	bool TryReadBoolProperty(const UObject* Container, const FName PropertyName,
		bool& OutValue)
	{
		if (!IsValid(Container))
		{
			return false;
		}
		const FBoolProperty* Property = FindFProperty<FBoolProperty>(
			Container->GetClass(), PropertyName);
		if (!Property)
		{
			return false;
		}
		OutValue = Property->GetPropertyValue_InContainer(Container);
		return true;
	}

	template <typename TActor>
	TActor* ReadActorProperty(const UObject* Container, const FName PropertyName)
	{
		if (!IsValid(Container))
		{
			return nullptr;
		}
		const FObjectPropertyBase* Property = FindFProperty<FObjectPropertyBase>(
			Container->GetClass(), PropertyName);
		return Property
			? Cast<TActor>(Property->GetObjectPropertyValue_InContainer(Container))
			: nullptr;
	}

	int32 ResolveDeterministicSentinelIndex(
		UHierarchicalInstancedStaticMeshComponent* Component,
		FAPSFullScaleHismSentinelState& State)
	{
		const int32 InstanceCount = IsValid(Component) ? Component->GetInstanceCount() : 0;
		if (State.Component.Get() == Component && State.InstanceCount == InstanceCount
			&& State.InstanceIndex >= 0 && State.InstanceIndex < InstanceCount)
		{
			return State.InstanceIndex;
		}

		State = FAPSFullScaleHismSentinelState{};
		State.Component = Component;
		State.InstanceCount = InstanceCount;
		double FarthestLocalDistanceSquared = -1.0;
		for (int32 InstanceIndex = 0; InstanceIndex < InstanceCount; ++InstanceIndex)
		{
			FTransform LocalTransform;
			if (!Component->GetInstanceTransform(InstanceIndex, LocalTransform, false))
			{
				continue;
			}
			const double LocalDistanceSquared = LocalTransform.GetLocation().SizeSquared();
			if (FMath::IsFinite(LocalDistanceSquared)
				&& LocalDistanceSquared > FarthestLocalDistanceSquared)
			{
				FarthestLocalDistanceSquared = LocalDistanceSquared;
				State.InstanceIndex = InstanceIndex;
			}
		}
		return State.InstanceIndex;
	}

	bool ProjectWorldLocationToNdc(const UWorld* World, const int32 ViewIndex,
		const FVector& WorldLocation, FVector2D& OutProjectedNdc)
	{
		if (!World || !World->CachedViewInfoRenderedLastFrame.IsValidIndex(ViewIndex))
		{
			return false;
		}
		const FVector4 ClipLocation =
			World->CachedViewInfoRenderedLastFrame[ViewIndex].ViewProjectionMatrix
			.TransformFVector4(FVector4(WorldLocation, 1.0));
		if (!FMath::IsFinite(ClipLocation.X) || !FMath::IsFinite(ClipLocation.Y)
			|| !FMath::IsFinite(ClipLocation.W)
			|| FMath::Abs(ClipLocation.W) <= UE_SMALL_NUMBER)
		{
			return false;
		}
		OutProjectedNdc = FVector2D(
			ClipLocation.X / ClipLocation.W, ClipLocation.Y / ClipLocation.W);
		return FMath::IsFinite(OutProjectedNdc.X) && FMath::IsFinite(OutProjectedNdc.Y);
	}

	void SampleHismSentinel(const TCHAR* Scope,
		UHierarchicalInstancedStaticMeshComponent* Component,
		FAPSFullScaleHismSentinelState& State, const uint64 Sequence,
		const UWorld* World, const bool bRenderedViewAvailable,
		const FVector& ReferenceViewLocation, const int32 ProjectionViewIndex)
	{
		const int32 SentinelIndex = ResolveDeterministicSentinelIndex(Component, State);
		if (!IsValid(Component) || SentinelIndex == INDEX_NONE)
		{
			UE_LOG(LogAPSFullScaleTransformTelemetry, Display,
				TEXT("[APS.FullScale.HISM] seq=%llu scope=%s component=%s instances=%d sentinel=none"),
				Sequence, Scope, *GetNameSafe(Component),
				IsValid(Component) ? Component->GetInstanceCount() : 0);
			return;
		}

		FTransform LocalTransform;
		FTransform ApiWorldTransform;
		if (!Component->GetInstanceTransform(SentinelIndex, LocalTransform, false)
			|| !Component->GetInstanceTransform(SentinelIndex, ApiWorldTransform, true))
		{
			UE_LOG(LogAPSFullScaleTransformTelemetry, Warning,
				TEXT("[APS.FullScale.HISM] seq=%llu scope=%s component=%s instances=%d sentinel=%d readFailed=1"),
				Sequence, Scope, *GetNameSafe(Component), Component->GetInstanceCount(),
				SentinelIndex);
			return;
		}

		const FTransform ComponentTransform = Component->GetComponentTransform();
		const FVector TranslatedInstanceSpaceOrigin =
			static_cast<const UInstancedStaticMeshComponent*>(Component)
			->GetTranslatedInstanceSpaceOrigin();
		const FVector CpuWorldLocation =
			ComponentTransform.TransformPosition(LocalTransform.GetLocation());
		const FVector ApiWorldLocation = ApiWorldTransform.GetLocation();
		const FVector ViewRelativeLocation = bRenderedViewAvailable
			? CpuWorldLocation - ReferenceViewLocation : FVector::ZeroVector;
		FVector2D ProjectedNdc = FVector2D::ZeroVector;
		const bool bProjectedNdc = bRenderedViewAvailable
			&& ProjectWorldLocationToNdc(World, ProjectionViewIndex,
				CpuWorldLocation, ProjectedNdc);

		const bool bSameSentinel = State.bHasSample
			&& State.Component.Get() == Component
			&& State.InstanceIndex == SentinelIndex;
		const double WorldDelta = bSameSentinel
			? FVector::Distance(State.LastWorldLocation, CpuWorldLocation) : 0.0;
		const double ViewRelativeDelta = bSameSentinel && State.bHadRenderedView
			&& bRenderedViewAvailable
			? FVector::Distance(State.LastViewRelativeLocation, ViewRelativeLocation) : 0.0;
		const double ProjectedNdcDelta = bSameSentinel && State.bHadProjectedNdc
			&& bProjectedNdc
			? FVector2D::Distance(State.LastProjectedNdc, ProjectedNdc) : 0.0;
		const double ViewRelativeMagnitude = bRenderedViewAvailable
			? ViewRelativeLocation.Size() : 0.0;
		const bool bFarFieldPrecisionRisk = bRenderedViewAvailable
			&& ViewRelativeMagnitude >= FarFieldPrecisionRiskDistanceCm;

		UE_LOG(LogAPSFullScaleTransformTelemetry, Display,
			TEXT("[APS.FullScale.HISM] seq=%llu scope=%s component=%s owner=%s instances=%d "
				"sentinelPolicy=farthestLocalLowestIndex sentinel=%d componentLoc=%s componentScale=%s "
				"componentTickCanEver=%d componentTickEnabled=%d translatedInstanceSpace=%d translatedOrigin=%s localLoc=%s "
				"cpuWorldLoc=%s apiWorldLoc=%s apiWorldError=%.6f worldDelta=%.6f "
				"renderedViewAvailable=%d viewRelative=%s viewRelativeMagnitude=%.6e viewRelativeDelta=%.6f "
				"projectedNdcAvailable=%d projectedNdc=(%.9f,%.9f) projectedNdcDelta=%.9f precisionRisk=%d"),
			Sequence, Scope, *GetNameSafe(Component), *GetNameSafe(Component->GetOwner()),
			Component->GetInstanceCount(), SentinelIndex,
			*ComponentTransform.GetLocation().ToCompactString(),
			*ComponentTransform.GetScale3D().ToCompactString(),
			Component->PrimaryComponentTick.bCanEverTick ? 1 : 0,
			Component->IsComponentTickEnabled() ? 1 : 0,
			Component->bUseTranslatedInstanceSpace ? 1 : 0,
			*TranslatedInstanceSpaceOrigin.ToCompactString(),
			*LocalTransform.GetLocation().ToCompactString(),
			*CpuWorldLocation.ToCompactString(), *ApiWorldLocation.ToCompactString(),
			FVector::Distance(CpuWorldLocation, ApiWorldLocation), WorldDelta,
			bRenderedViewAvailable ? 1 : 0,
			*ViewRelativeLocation.ToCompactString(), ViewRelativeMagnitude,
			ViewRelativeDelta, bProjectedNdc ? 1 : 0,
			ProjectedNdc.X, ProjectedNdc.Y, ProjectedNdcDelta,
			bFarFieldPrecisionRisk ? 1 : 0);

		State.bHasSample = true;
		State.bHadRenderedView = bRenderedViewAvailable;
		State.bHadProjectedNdc = bProjectedNdc;
		State.LastWorldLocation = CpuWorldLocation;
		State.LastViewRelativeLocation = ViewRelativeLocation;
		State.LastProjectedNdc = ProjectedNdc;
	}
}

bool UAPSFullScaleTransformTelemetrySubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	const UWorld* World = Cast<UWorld>(Outer);
	return World && (World->WorldType == EWorldType::Game || World->WorldType == EWorldType::PIE);
}

void UAPSFullScaleTransformTelemetrySubsystem::Deinitialize()
{
	ResetSample();
	Super::Deinitialize();
}

TStatId UAPSFullScaleTransformTelemetrySubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(
		UAPSFullScaleTransformTelemetrySubsystem, STATGROUP_Tickables);
}

void UAPSFullScaleTransformTelemetrySubsystem::ResetSample()
{
	SampleElapsed = 0.0f;
	Sequence = 0;
	bHasSample = false;
	LastGenerator.Reset();
	LastHomeSystem.Reset();
	LastPawn.Reset();
	LastGeneratorLocation = FVector::ZeroVector;
	LastHomeSystemLocation = FVector::ZeroVector;
	LastPawnLocation = FVector::ZeroVector;
	LastCameraLocation = FVector::ZeroVector;
	bHadRenderedView = false;
	LastRenderedViewLocation = FVector::ZeroVector;
	GalaxySentinel = FAPSFullScaleHismSentinelState{};
	ClusterSentinel = FAPSFullScaleHismSentinelState{};
}

void UAPSFullScaleTransformTelemetrySubsystem::Tick(float DeltaTime)
{
	if (CVarAPSFullScaleTransformTelemetry.GetValueOnGameThread() <= 0)
	{
		if (bHasSample)
		{
			ResetSample();
		}
		return;
	}

	SampleElapsed += FMath::Max(DeltaTime, 0.0f);
	const float RequestedInterval =
		CVarAPSFullScaleTransformTelemetryInterval.GetValueOnGameThread();
	const float Interval = RequestedInterval > 0.0f
		? FMath::Clamp(RequestedInterval, 0.02f, 5.0f) : 0.0f;
	if (Interval > 0.0f && SampleElapsed < Interval)
	{
		return;
	}
	SampleElapsed = 0.0f;

	UWorld* World = GetWorld();
	APlayerController* Controller = World ? World->GetFirstPlayerController() : nullptr;
	APawn* Pawn = Controller ? Controller->GetPawn() : nullptr;
	APlayerCameraManager* CameraManager = Controller ? Controller->PlayerCameraManager : nullptr;
	AAstroGenerator* Generator = FindFirstValidActor<AAstroGenerator>(World);
	AGalaxy* Galaxy = ReadActorProperty<AGalaxy>(Generator, TEXT("GeneratedGalaxy"));
	AStarCluster* Cluster = ReadActorProperty<AStarCluster>(
		Generator, TEXT("GeneratedStarCluster"));
	AStarSystem* HomeSystem = ReadActorProperty<AStarSystem>(
		Generator, TEXT("GeneratedHomeStarSystem"));
	const bool bExactGalaxyReference = IsValid(Galaxy);
	const bool bExactClusterReference = IsValid(Cluster);
	const bool bExactHomeReference = IsValid(HomeSystem);
	if (!IsValid(Galaxy))
	{
		Galaxy = FindFirstValidActor<AGalaxy>(World);
	}
	if (!IsValid(Cluster))
	{
		Cluster = FindFirstValidActor<AStarCluster>(World);
	}
	if (!IsValid(HomeSystem))
	{
		HomeSystem = FindFirstValidActor<AStarSystem>(World);
	}
	bool bIsPreviewGeneration = false;
	const bool bPreviewFlagKnown = TryReadBoolProperty(
		Generator, TEXT("bIsPreviewGeneration"), bIsPreviewGeneration);

	const FVector GeneratorLocation = IsValid(Generator)
		? Generator->GetActorLocation() : FVector::ZeroVector;
	const FVector HomeSystemLocation = IsValid(HomeSystem)
		? HomeSystem->GetActorLocation() : FVector::ZeroVector;
	const FVector PawnLocation = IsValid(Pawn) ? Pawn->GetActorLocation() : FVector::ZeroVector;
	const FVector CameraLocation = IsValid(CameraManager)
		? CameraManager->GetCameraLocation() : FVector::ZeroVector;
	const double PawnSpeed = IsValid(Pawn) ? Pawn->GetVelocity().Size() : 0.0;
	const int32 RenderedViewCount = World
		? World->ViewLocationsRenderedLastFrame.Num() : 0;
	const bool bRenderedViewAvailable = RenderedViewCount > 0;
	const FVector RenderedViewLocation = bRenderedViewAvailable
		? World->ViewLocationsRenderedLastFrame[0] : FVector::ZeroVector;
	const FVector ReferenceViewLocation = bRenderedViewAvailable
		? RenderedViewLocation : CameraLocation;
	const double RenderedViewDelta = bRenderedViewAvailable && bHadRenderedView
		? FVector::Distance(LastRenderedViewLocation, RenderedViewLocation) : 0.0;
	const double CameraToRenderedViewDelta = bRenderedViewAvailable && IsValid(CameraManager)
		? FVector::Distance(CameraLocation, RenderedViewLocation) : 0.0;
	const bool bRenderedViewStationary = bRenderedViewAvailable && bHadRenderedView
		&& RenderedViewDelta <= RenderedViewStationaryDeltaCm;
	const double LastRenderAge = World
		? FMath::Max(0.0, World->GetTimeSeconds() - World->LastRenderTime) : 0.0;
	const TCHAR* ViewAuthority = bRenderedViewAvailable
		? (RenderedViewCount == 1
			? TEXT("RenderedLastFrame")
			: TEXT("RenderedLastFrame0_AmbiguousMultipleViews"))
		: TEXT("PlayerCameraManagerFallback_NonAuthoritativeForEditorEject");

	const bool bSameActors = bHasSample
		&& LastGenerator.Get() == Generator
		&& LastHomeSystem.Get() == HomeSystem
		&& LastPawn.Get() == Pawn;
	const double GeneratorDelta = bSameActors
		? FVector::Distance(LastGeneratorLocation, GeneratorLocation) : 0.0;
	const double HomeSystemDelta = bSameActors
		? FVector::Distance(LastHomeSystemLocation, HomeSystemLocation) : 0.0;
	const double PawnDelta = bSameActors
		? FVector::Distance(LastPawnLocation, PawnLocation) : 0.0;
	const double CameraDelta = bSameActors
		? FVector::Distance(LastCameraLocation, CameraLocation) : 0.0;

	AActor* HomeParent = IsValid(HomeSystem) ? HomeSystem->GetAttachParentActor() : nullptr;
	const double GeneratorScale = IsValid(Generator)
		? Generator->GetActorTransform().GetScale3D().GetAbsMax() : 1.0;
	const double ParentScale = IsValid(HomeParent)
		? HomeParent->GetActorTransform().GetScale3D().GetAbsMax() : 1.0;
	const USceneComponent* HomeRoot = IsValid(HomeSystem) ? HomeSystem->GetRootComponent() : nullptr;
	const double HomeRelativeScale = IsValid(HomeRoot)
		? HomeRoot->GetRelativeScale3D().GetAbsMax() : 1.0;
	const bool bCompensatedHierarchy = IsValid(HomeParent)
		&& ParentScale >= 1.0e6 && HomeRelativeScale <= 1.0e-6;
	const bool bUnexpectedStationaryMotion = bSameActors
		&& PawnSpeed <= StationarySpeedCmPerSecond
		&& (GeneratorDelta > UnexpectedStationaryDeltaCm
			|| HomeSystemDelta > UnexpectedStationaryDeltaCm
			|| PawnDelta > UnexpectedStationaryDeltaCm);

	++Sequence;
	UE_LOG(LogAPSFullScaleTransformTelemetry, Display,
		TEXT("[APS.FullScale.View] seq=%llu authority=%s renderedViewAvailable=%d renderedViewCount=%d "
			"renderedView0=%s renderedView0Delta=%.6f renderedView0Stationary=%d "
			"cachedViewCount=%d lastRenderAge=%.6f playerCameraManager=%s pcmLoc=%s pcmToRenderedViewDelta=%.6f"),
		Sequence, ViewAuthority, bRenderedViewAvailable ? 1 : 0, RenderedViewCount,
		*RenderedViewLocation.ToCompactString(), RenderedViewDelta,
		bRenderedViewStationary ? 1 : 0,
		World ? World->CachedViewInfoRenderedLastFrame.Num() : 0, LastRenderAge,
		*GetNameSafe(CameraManager), *CameraLocation.ToCompactString(),
		CameraToRenderedViewDelta);
	UE_LOG(LogAPSFullScaleTransformTelemetry, Display,
		TEXT("[APS.FullScale.Transform] seq=%llu origin=%s generator=%s genLoc=%s genScale=%.3e genDelta=%.6f "
			"home=%s homeLoc=%s homeParent=%s parentScale=%.3e homeRelativeScale=%.3e homeDelta=%.6f "
			"pawn=%s pawnLoc=%s pawnSpeed=%.6f pawnDelta=%.6f cameraLoc=%s cameraDelta=%.6f "
			"generatorTickCanEver=%d generatorTickEnabled=%d previewFlagKnown=%d preview=%d "
			"galaxy=%s galaxyExact=%d cluster=%s clusterExact=%d homeExact=%d "
			"compensated=%d stationaryDrift=%d"),
		Sequence, World ? *World->OriginLocation.ToString() : TEXT("None"),
		*GetNameSafe(Generator), *GeneratorLocation.ToCompactString(), GeneratorScale, GeneratorDelta,
		*GetNameSafe(HomeSystem), *HomeSystemLocation.ToCompactString(), *GetNameSafe(HomeParent),
		ParentScale, HomeRelativeScale, HomeSystemDelta,
		*GetNameSafe(Pawn), *PawnLocation.ToCompactString(), PawnSpeed, PawnDelta,
		*CameraLocation.ToCompactString(), CameraDelta,
		IsValid(Generator) && Generator->PrimaryActorTick.bCanEverTick ? 1 : 0,
		IsValid(Generator) && Generator->IsActorTickEnabled() ? 1 : 0,
		bPreviewFlagKnown ? 1 : 0, bIsPreviewGeneration ? 1 : 0,
		*GetNameSafe(Galaxy), bExactGalaxyReference ? 1 : 0,
		*GetNameSafe(Cluster), bExactClusterReference ? 1 : 0,
		bExactHomeReference ? 1 : 0,
		bCompensatedHierarchy ? 1 : 0, bUnexpectedStationaryMotion ? 1 : 0);

	SampleHismSentinel(TEXT("Galaxy"),
		IsValid(Galaxy) ? Galaxy->StarMeshInstances : nullptr,
		GalaxySentinel, Sequence, World, bRenderedViewAvailable,
		ReferenceViewLocation, 0);
	SampleHismSentinel(TEXT("Cluster"),
		IsValid(Cluster) ? Cluster->StarMeshInstances : nullptr,
		ClusterSentinel, Sequence, World, bRenderedViewAvailable,
		ReferenceViewLocation, 0);

	if (bCompensatedHierarchy || bUnexpectedStationaryMotion)
	{
		UE_LOG(LogAPSFullScaleTransformTelemetry, Warning,
			TEXT("[APS.FullScale.Transform] invariant violation compensated=%d stationaryDrift=%d"),
			bCompensatedHierarchy ? 1 : 0, bUnexpectedStationaryMotion ? 1 : 0);
	}

	bHasSample = true;
	LastGenerator = Generator;
	LastHomeSystem = HomeSystem;
	LastPawn = Pawn;
	LastGeneratorLocation = GeneratorLocation;
	LastHomeSystemLocation = HomeSystemLocation;
	LastPawnLocation = PawnLocation;
	LastCameraLocation = CameraLocation;
	bHadRenderedView = bRenderedViewAvailable;
	LastRenderedViewLocation = RenderedViewLocation;
}
