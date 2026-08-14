#include "APSFullScaleTransformTelemetrySubsystem.h"

#include "APS_ALPHA/Actors/Astro/StarSystem.h"
#include "APS_ALPHA/Generation/AstroGenerator.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/SceneComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"

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
		0.10f,
		TEXT("Sampling interval in seconds for aps.FullScale.TransformTelemetry."),
		ECVF_Default);

	constexpr double StationarySpeedCmPerSecond = 0.10;
	constexpr double UnexpectedStationaryDeltaCm = 0.50;

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
	const float Interval = FMath::Clamp(
		CVarAPSFullScaleTransformTelemetryInterval.GetValueOnGameThread(), 0.02f, 5.0f);
	if (SampleElapsed < Interval)
	{
		return;
	}
	SampleElapsed = 0.0f;

	UWorld* World = GetWorld();
	APlayerController* Controller = World ? World->GetFirstPlayerController() : nullptr;
	APawn* Pawn = Controller ? Controller->GetPawn() : nullptr;
	APlayerCameraManager* CameraManager = Controller ? Controller->PlayerCameraManager : nullptr;
	AAstroGenerator* Generator = FindFirstValidActor<AAstroGenerator>(World);
	AStarSystem* HomeSystem = FindFirstValidActor<AStarSystem>(World);

	const FVector GeneratorLocation = IsValid(Generator)
		? Generator->GetActorLocation() : FVector::ZeroVector;
	const FVector HomeSystemLocation = IsValid(HomeSystem)
		? HomeSystem->GetActorLocation() : FVector::ZeroVector;
	const FVector PawnLocation = IsValid(Pawn) ? Pawn->GetActorLocation() : FVector::ZeroVector;
	const FVector CameraLocation = IsValid(CameraManager)
		? CameraManager->GetCameraLocation() : FVector::ZeroVector;
	const double PawnSpeed = IsValid(Pawn) ? Pawn->GetVelocity().Size() : 0.0;

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
		TEXT("[APS.FullScale.Transform] seq=%llu origin=%s generator=%s genLoc=%s genScale=%.3e genDelta=%.6f "
			"home=%s homeLoc=%s homeParent=%s parentScale=%.3e homeRelativeScale=%.3e homeDelta=%.6f "
			"pawn=%s pawnLoc=%s pawnSpeed=%.6f pawnDelta=%.6f cameraLoc=%s cameraDelta=%.6f compensated=%d stationaryDrift=%d"),
		Sequence, World ? *World->OriginLocation.ToString() : TEXT("None"),
		*GetNameSafe(Generator), *GeneratorLocation.ToCompactString(), GeneratorScale, GeneratorDelta,
		*GetNameSafe(HomeSystem), *HomeSystemLocation.ToCompactString(), *GetNameSafe(HomeParent),
		ParentScale, HomeRelativeScale, HomeSystemDelta,
		*GetNameSafe(Pawn), *PawnLocation.ToCompactString(), PawnSpeed, PawnDelta,
		*CameraLocation.ToCompactString(), CameraDelta,
		bCompensatedHierarchy ? 1 : 0, bUnexpectedStationaryMotion ? 1 : 0);

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
}
