#include "Spaceship.h"
#include "APS_ALPHA/Actors/Astro/Moon.h"
#include "APS_ALPHA/Actors/Astro/Planet.h"
#include "APS_ALPHA/Actors/Astro/PlanetOrbit.h"
#include "APS_ALPHA/Actors/Astro/PlanetaryBody.h"
#include "APS_ALPHA/Actors/Astro/Star.h"
#include "APS_ALPHA/Actors/Astro/StarCluster.h"
#include "APS_ALPHA/Actors/Astro/StarSystem.h"
#include "APS_ALPHA/Actors/Astro/WorldActor.h"
#include "APS_ALPHA/Actors/Tech/TechActor.h"
#include "APS_ALPHA/Actors/Tech/SpaceStation.h"
#include "APS_ALPHA/Core/Enums/MoonType.h"
#include "APS_ALPHA/Core/Enums/PlanetType.h"
#include "APS_ALPHA/Core/Interfaces/NavigatableBody.h"
#include "APS_ALPHA/Core/Structs/StarGenerationModel.h"
#include "APS_ALPHA/Core/World/APSPlanetEnvironmentStreamingSubsystem.h"
#include "APS_ALPHA/Generation/AstroGenerator.h"
#include "APS_ALPHA/Pawns/Characters/GravityCharacterPawn.h"
#include "APS_ALPHA/Pawns/Characters/GravityDetectorComponent.h"
#include "APS_ALPHA/Pawns/Spaceships/ShipNavigationComponent.h"
#include "Camera/CameraComponent.h"
#include "Components/ArrowComponent.h"
#include "Components/BoxComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "PhysicsEngine/BodySetup.h"
#include "HAL/IConsoleManager.h"
#include "ProfilingDebugging/CpuProfilerTrace.h"
#include "APSShipFlightBenchmark.h"
#include "APSShipFlightModel.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/PlayerController.h"
#include "Camera/PlayerCameraManager.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Blueprint/SlateBlueprintLibrary.h"
#include "Engine/LocalPlayer.h"
#include "SceneView.h"
#include "InputCoreTypes.h"
#include "Kismet/GameplayStatics.h"
#include "Rendering/DrawElements.h"
#include "Styling/CoreStyle.h"
#include "Widgets/SLeafWidget.h"
#include "Widgets/Layout/SBackgroundBlur.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SOverlay.h"
#include "Widgets/SViewport.h"
#include "Widgets/Text/STextBlock.h"
#include "Components/MeshComponent.h"
#include "Components/SpotLightComponent.h"
#include "Components/PointLightComponent.h"

class SAPSShipNavigationOverlay final : public SLeafWidget
{
public:
	SLATE_BEGIN_ARGS(SAPSShipNavigationOverlay) {}
		SLATE_ARGUMENT(TWeakObjectPtr<ASpaceship>, Ship)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs)
	{
		Ship = InArgs._Ship;
		SetVisibility(EVisibility::HitTestInvisible);
	}

	virtual FVector2D ComputeDesiredSize(float) const override
	{
		return FVector2D(1.0f, 1.0f);
	}

	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry,
		const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId,
		const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override
	{
		return Ship.IsValid()
			? Ship->PaintNavigationOverlay(AllottedGeometry, MyCullingRect, OutDrawElements, LayerId)
			: LayerId;
	}

private:
	TWeakObjectPtr<ASpaceship> Ship;
};

namespace APSNavigationHud
{
	constexpr float MarkerWidth = 176.0f;
	constexpr float MarkerHeight = 38.0f;
	constexpr float MarkerGap = 5.0f;
	constexpr float FlagHorizontalShift = 0.62f;
	constexpr float FlagPoleLength = 18.0f;

	/**
	 * The overlay projects hundreds of points per frame (orbit rings, markers). The engine's widget projection
	 * rebuilds the view from the camera on every call; this keeps one view per frame and derives the
	 * screen-to-viewport mapping (DPI scale and offset) from the same engine call, so results are unchanged.
	 */
	struct FProjectionFrame
	{
		const ASpaceship* Ship{nullptr};
		uint64 Frame{MAX_uint64};
		bool bValid{false};
		FMatrix ViewProjection{FMatrix::Identity};
		FIntRect ViewRect;
		FVector2D Offset{FVector2D::ZeroVector};
		FVector2D Scale{FVector2D::UnitVector};
	};
	FProjectionFrame GProjectionFrame;

	/** Marker label placement solved once per frame for all markers (was solved again for every marker). */
	struct FLayoutFrame
	{
		const ASpaceship* Ship{nullptr};
		uint64 Frame{MAX_uint64};
		const TSet<int32>* OccludedContacts{nullptr};
		TMap<int32, TPair<FVector2D, FVector2D>> Layouts;
	};
	FLayoutFrame GLayoutFrame;
}

namespace APSAutomaticShipInteraction
{
	const FVector NativeExitLocation(0.0, -200.0, 100.0);

	bool IsNativeSeatTransform(const USceneComponent* Component)
	{
		return Component
			&& Component->GetRelativeLocation().IsNearlyZero(0.1)
			&& Component->GetRelativeRotation().IsNearlyZero(0.1);
	}

	bool IsNativeExitTransform(const USceneComponent* Component)
	{
		return Component
			&& Component->GetRelativeLocation().Equals(NativeExitLocation, 0.1)
			&& Component->GetRelativeRotation().IsNearlyZero(0.1);
	}

	UStaticMeshComponent* FindLargestMesh(const ASpaceship* Ship)
	{
		TArray<UStaticMeshComponent*> MeshComponents;
		Ship->GetComponents(MeshComponents);

		UStaticMeshComponent* BestMesh = nullptr;
		double BestBoundsSizeSquared = 0.0;
		for (UStaticMeshComponent* MeshComponent : MeshComponents)
		{
			if (!IsValid(MeshComponent) || !MeshComponent->GetStaticMesh() || MeshComponent == Ship->ForwardVector)
			{
				continue;
			}

			MeshComponent->UpdateBounds();
			const double BoundsSizeSquared = MeshComponent->Bounds.BoxExtent.SizeSquared();
			if (BoundsSizeSquared > BestBoundsSizeSquared)
			{
				BestMesh = MeshComponent;
				BestBoundsSizeSquared = BoundsSizeSquared;
			}
		}

		return BestMesh;
	}

	bool FindSocketTransform(const USceneComponent* MeshComponent, const TArray<FName>& SocketNames,
		FTransform& OutTransform)
	{
		for (const FName SocketName : SocketNames)
		{
			if (MeshComponent->DoesSocketExist(SocketName))
			{
				OutTransform = MeshComponent->GetSocketTransform(SocketName, RTS_World);
				return true;
			}
		}
		return false;
	}
}

/*
press shift + hold Lshift = busting
double press Lshift = increase engine mode
*/

void ASpaceship::UpdateNavigatableActorsForInterstellar()
{
	GEngine->AddOnScreenDebugMessage(-1, 1.f, FColor::Green, TEXT("UpdateNavigatableActorsForInterstellar"));

	WorldNavigatableActors.Empty();
	if (LastFlightMode == EFlightMode::Stellar)
	{
		ToggleScale();
		bIsScaled = true;
		GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Green, FString::Printf(TEXT("Galaxy is Downscaled")));
	}
}

void ASpaceship::UpdateNavigatableActorsForStellar()
{
	GEngine->AddOnScreenDebugMessage(-1, 1.f, FColor::Green, TEXT("UpdateNavigatableActorsForStellar"));

	TArray<AActor*> WorldActors;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AWorldActor::StaticClass(), WorldActors);
	for (AActor* Actor : WorldActors)
	{
		if (Actor->GetClass()->ImplementsInterface(UNavigatableBody::StaticClass()))
		{
			AWorldActor* WorldNavigatableActor = Cast<AWorldActor>(Actor);
			WorldNavigatableActors.Add(WorldNavigatableActor);
			GEngine->AddOnScreenDebugMessage(-1, 1.f, FColor::Yellow,
			                                 FString::Printf(
				                                 TEXT("WorldActor: %s"), *WorldNavigatableActor->GetName()));
		}
	}

	if (LastFlightMode == EFlightMode::Interstellar)
	{
		// reorigin star system and ship to 0 0 0
		FVector PlayerLocation = this->GetActorLocation();
		FVector NewSystemLocation = OffsetSystem->GetActorLocation() - PlayerLocation;
		OffsetSystem->SetActorLocation(NewSystemLocation, false);
		this->SetActorLocation(FVector(0, 0, 0), false);

		ToggleScale();
		bIsScaled = false;
		GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Green, FString::Printf(TEXT("Galaxy is Upscaled")));
	}
}

void ASpaceship::UpdateNavigatableActorsForInterplanetary()
{
	GEngine->AddOnScreenDebugMessage(-1, 1.f, FColor::Green, TEXT("UpdateNavigatableActorsForInterplanetary"));
}

namespace APSShipPerf
{
	TAutoConsoleVariable<int32> CVarShipPerfLog(
		TEXT("aps.Ship.PerfLog"), 0,
		TEXT("1 logs the piloted ship's tick cost per section (environment, navigation, move/sweep, rotation, camera, HUD) every 2 s."));
	TAutoConsoleVariable<int32> CVarSweepPrecheck(
		TEXT("aps.Ship.SweepPrecheck"), 1,
		TEXT("1 tests a kinematic ship move with the hull's bounding sphere first and sweeps the hull body only when "
			"that sphere may hit. 0 always sweeps the hull body (every convex hull separately)."));
	TAutoConsoleVariable<int32> CVarHudProjectionCheck(
		TEXT("aps.Ship.HudProjectionCheck"), 0,
		TEXT("1 compares the navigation HUD's per-frame projection with the engine's per-call projection and logs ")
		TEXT("the largest difference every second (verification only)."));
	TAutoConsoleVariable<int32> CVarResetHullVelocity(
		TEXT("aps.Ship.ResetHullVelocity"), 1,
		TEXT("1 removes the piloted kinematic ship's primitives from the renderer's velocity data every frame (the behaviour ")
		TEXT("before 29.09): TSR and Lumen then reproject the hull as static world geometry while the camera flies with it. ")
		TEXT("0 leaves the engine's own motion vectors (A/B for the hull shimmer at speed)."));
	TAutoConsoleVariable<int32> CVarProxySweep(
		TEXT("aps.Ship.ProxySweep"), 1,
		TEXT("1 sweeps the flight proxy boxes of detailed hulls (M3 and similar) so they collide with stations, ships and ")
		TEXT("terrain; 0 restores the collision-free root move of those ships (before 29.09)."));
	TAutoConsoleVariable<int32> CVarHullSceneLightingInFlight(
		TEXT("aps.Ship.HullSceneLightingInFlight"), 2,
		TEXT("2 (default since Rio's check on 29.09) takes a piloted ship out of the global distance field: that copy ")
		TEXT("updates a frame behind a hull moving hundreds of metres per frame and lit it on alternate frames (shimmer ")
		TEXT("27 -> 4; the hull loses some self-bounce light). 1 keeps the ship in the distance field and the Lumen scene ")
		TEXT("(before 29.09), 3 takes it out of the Lumen scene only, 0 out of both. Changes apply in flight; the flags ")
		TEXT("come back when the pilot leaves."));
	TAutoConsoleVariable<int32> CVarSpeedFov(
		TEXT("aps.Ship.SpeedFov"), 0,
		TEXT("1 widens the flight camera's field of view with speed (up to +12 deg, the camera before 29.09). Every ")
		TEXT("frame of that widening re-sizes and re-uploads the full-scale star catalogue (APS_GameplayStellarView), ")
		TEXT("which cost 6-8 ms per frame and 80-100 ms hitches while accelerating at power 3. 0 keeps the base FOV."));

	enum ESection : int32 { Environment, Navigation, Move, Rotation, Stabilize, Camera, Hud, Count };
	const TCHAR* const SectionNames[Count] = {TEXT("env"), TEXT("nav"), TEXT("move"), TEXT("rot"),
		TEXT("stab"), TEXT("cam"), TEXT("hud")};

	struct FWindow
	{
		double Sum[Count] = {};
		double Max[Count] = {};
		double FrameSum = 0.0;
		double FrameMax = 0.0;
		int32 Frames = 0;
		int32 Hitches = 0;
		double Elapsed = 0.0;
		double MaxMove = 0.0;
	};
	FWindow GPiloted;

	bool Enabled() { return CVarShipPerfLog.GetValueOnGameThread() != 0; }

	struct FScope
	{
		FScope(bool bInActive, ESection InSection) : bActive(bInActive), Section(InSection),
			Start(bInActive ? FPlatformTime::Seconds() : 0.0) {}
		~FScope()
		{
			if (bActive)
			{
				const double Ms = (FPlatformTime::Seconds() - Start) * 1000.0;
				GPiloted.Sum[Section] += Ms;
				GPiloted.Max[Section] = FMath::Max(GPiloted.Max[Section], Ms);
			}
		}
		bool bActive;
		ESection Section;
		double Start;
	};
}

ASpaceship::ASpaceship()
{
	SpaceshipHull = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("SpaceshipHull"));
	RootComponent = SpaceshipHull;
	SpaceshipHull->SetMobility(EComponentMobility::Movable);
	SpaceshipHull->SetEnableGravity(false);
	SpaceshipHull->SetSimulatePhysics(false);

	SkeletalSpaceshipHull = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("SkeletalSpaceshipHull"));
	SkeletalSpaceshipHull->SetupAttachment(SpaceshipHull);
	SkeletalSpaceshipHull->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SkeletalSpaceshipHull->SetGenerateOverlapEvents(false);

	OnboardComputer = CreateDefaultSubobject<USpaceshipOnboardComputer>(TEXT("OnboardComputer"));
	
	SphereCollisionComponent = CreateDefaultSubobject<USphereComponent>(TEXT("SphereCollisionComponent"));
	SphereCollisionComponent->SetupAttachment(SpaceshipHull);
	SphereCollisionComponent->InitSphereRadius(1000.0f);
	SphereCollisionComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	SphereCollisionComponent->SetCollisionResponseToAllChannels(ECR_Ignore);
	SphereCollisionComponent->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	SphereCollisionComponent->SetGenerateOverlapEvents(true);
	SphereCollisionComponent->SetAbsolute(false, false, true);

	FlightGravityDetector = CreateDefaultSubobject<UGravityDetectorComponent>(TEXT("FlightGravityDetector"));
	FlightGravityDetector->bAutomaticDetection = false;
	FlightGravityDetector->PrimaryComponentTick.bStartWithTickEnabled = false;

	ShipNavigation = CreateDefaultSubobject<UShipNavigationComponent>(TEXT("ShipNavigation"));
	FlightModel = CreateDefaultSubobject<UAPSShipFlightModel>(TEXT("FlightModel"));

	InteractionBoundsComponent = CreateDefaultSubobject<UBoxComponent>(TEXT("InteractionBounds"));
	InteractionBoundsComponent->SetupAttachment(SpaceshipHull);
	InteractionBoundsComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	InteractionBoundsComponent->SetCollisionResponseToAllChannels(ECR_Ignore);
	InteractionBoundsComponent->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	InteractionBoundsComponent->SetGenerateOverlapEvents(false);
	InteractionBoundsComponent->SetCanEverAffectNavigation(false);

	PilotChair = CreateDefaultSubobject<USceneComponent>(TEXT("PilotChair"));
	PilotChair->SetupAttachment(SpaceshipHull);
	PilotChair->SetAbsolute(false, false, true);

	PilotExitPoint = CreateDefaultSubobject<USceneComponent>(TEXT("PilotExitPoint"));
	PilotExitPoint->SetupAttachment(SpaceshipHull);
	PilotExitPoint->SetRelativeLocation(APSAutomaticShipInteraction::NativeExitLocation);
	PilotExitPoint->SetAbsolute(false, false, true);

	ForwardVector = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ForwardVector"));
	ForwardVector->SetupAttachment(SpaceshipHull);
	ForwardVector->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ForwardVector->SetGenerateOverlapEvents(false);
	ForwardVector->SetVisibility(false, true);
	ForwardVector->SetHiddenInGame(true);

#if WITH_EDITORONLY_DATA
	NoseArrow = CreateEditorOnlyDefaultSubobject<UArrowComponent>(TEXT("NoseArrow"));
	if (NoseArrow)
	{
		NoseArrow->SetupAttachment(ForwardVector);
		NoseArrow->ArrowColor = FColor(242, 181, 29);
		NoseArrow->ArrowSize = 2.0f;
		NoseArrow->bIsScreenSizeScaled = true;
		// Arrow bounds scale with the hull and would inflate the ship's actor bounds.
		NoseArrow->bUseAttachParentBound = true;
		NoseArrow->SetHiddenInGame(true);
	}
#endif

	OnInterstellarMode.AddDynamic(this, &ASpaceship::UpdateNavigatableActorsForInterstellar);
	OnStellarMode.AddDynamic(this, &ASpaceship::UpdateNavigatableActorsForStellar);
	OnInterplanetaryMode.AddDynamic(this, &ASpaceship::UpdateNavigatableActorsForInterplanetary);
	
	SpringArmComponent = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
	SpringArmComponent->SetupAttachment(RootComponent);
	SpringArmComponent->SetAbsolute(false, false, true);
	SpringArmComponent->TargetArmLength = 1000.0;
	SpringArmComponent->SetRelativeLocation(FVector::ZeroVector);
	SpringArmComponent->SetRelativeRotation(FRotator(-12.0, 0.0, 0.0));
	SpringArmComponent->bDoCollisionTest = false;
	// Translation lag becomes numerically unstable at full-scale travel speeds:
	// the arm falls kilometres behind the root and periodically catches up. Arm
	// length and FOV still communicate speed without allowing camera/ship separation.
	SpringArmComponent->bEnableCameraLag = false;
	SpringArmComponent->CameraLagSpeed = 12.0f;
	SpringArmComponent->bEnableCameraRotationLag = true;
	SpringArmComponent->CameraRotationLagSpeed = 9.0f;
	SpringArmComponent->bUseCameraLagSubstepping = false;
	SpringArmComponent->CameraLagMaxTimeStep = 1.0f / 120.0f;
	SpringArmComponent->bClampToMaxPhysicsDeltaTime = false;
	SpringArmComponent->PrimaryComponentTick.TickGroup = TG_PostPhysics;

	CameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	CameraComponent->SetupAttachment(SpringArmComponent, USpringArmComponent::SocketName);
	CameraComponent->SetAbsolute(false, false, true);
	CameraComponent->bUsePawnControlRotation = false;
#if WITH_EDITORONLY_DATA
	CameraComponent->bDrawFrustumAllowed = false;
	CameraComponent->bCameraMeshHiddenInGame = true;
#endif
	#if WITH_EDITOR
	CameraComponent->SetCameraMesh(nullptr);
	#endif

	PilotFillLight = CreateDefaultSubobject<USpotLightComponent>(TEXT("PilotFillLight"));
	PilotFillLight->SetupAttachment(CameraComponent);
	PilotFillLight->SetRelativeLocation(FVector::ZeroVector);
	PilotFillLight->SetRelativeRotation(FRotator::ZeroRotator);
	PilotFillLight->SetMobility(EComponentMobility::Movable);
	PilotFillLight->SetIntensity(3200.0f);
	PilotFillLight->SetLightColor(FLinearColor(0.72f, 0.82f, 1.0f));
	PilotFillLight->SetInnerConeAngle(38.0f);
	PilotFillLight->SetOuterConeAngle(72.0f);
	PilotFillLight->SetCastShadows(false);
	PilotFillLight->SetAffectTranslucentLighting(false);
	PilotFillLight->SetVisibility(false, true);

	PilotFillPointLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("PilotFillPointLight"));
	PilotFillPointLight->SetupAttachment(CameraComponent);
	PilotFillPointLight->SetRelativeLocation(FVector::ZeroVector);
	PilotFillPointLight->SetMobility(EComponentMobility::Movable);
	PilotFillPointLight->SetUseInverseSquaredFalloff(false);
	PilotFillPointLight->SetLightFalloffExponent(2.0f);
	PilotFillPointLight->SetIntensity(18.0f);
	PilotFillPointLight->SetLightColor(FLinearColor(0.72f, 0.82f, 1.0f));
	PilotFillPointLight->SetInverseExposureBlend(1.0f);
	PilotFillPointLight->SetCastShadows(false);
	PilotFillPointLight->SetAffectTranslucentLighting(false);
	// Channel 2 is reserved for the controlled ship's readability fill. Keeping
	// channel 0 disabled prevents the camera light from bleaching stations,
	// planets and characters near the ship.
	PilotFillPointLight->SetLightingChannels(false, false, true);
	PilotFillPointLight->SetVisibility(false, true);
}

void ASpaceship::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	RefreshInteractionGeometry();
}

void ASpaceship::BeginPlay()
{
	Super::BeginPlay();
	ConfigureFromHull();
	InitializeFlightPostProcess();
	RefreshInteractionGeometry();
	UpdateFlightEnvironment(0.0f, true);

	GeneratedWorld = Cast<AAstroGenerator>(
		UGameplayStatics::GetActorOfClass(GetWorld(), AAstroGenerator::StaticClass()));
	GeneratedStarCluster = Cast<AStarCluster>(
		UGameplayStatics::GetActorOfClass(GetWorld(), AStarCluster::StaticClass()));

	if (!OffsetSystem)
	{
		OffsetSystem = Cast<AStarSystem>(UGameplayStatics::GetActorOfClass(GetWorld(), AStarSystem::StaticClass()));
		if (OffsetSystem)
		{
			UE_LOG(LogTemp, Log, TEXT("OffsetSystem successfully obtained."));
			//GEngine->AddOnScreenDebugMessage(-1, 1.f, FColor::Green, TEXT("OffsetSystem successfully obtained!"));
		}
		else
		{
			UE_LOG(LogTemp, Verbose, TEXT("No OffsetSystem is present in this level."));
			//GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Failed to obtain OffsetSystem."));
		}
	}

	if (OnboardComputer)
	{
		OnboardComputer->SpaceshipHull = SpaceshipHull;
		OnboardComputer->OffsetSystem = OffsetSystem;
		SelectedEngineMode = EEngineMode::Impulse;
		OnboardComputer->FlightSystem.CurrentFlightMode = ResolveLegacyFlightModeForDriveMode(SelectedDriveMode);
		OnboardComputer->ComputeFlightParams();
		RequestEngineModeForFlightMode(true);
	}
	ApplyEngineState();

	// Parked fleet actors do not scan the universe. The possessed ship's navigation component owns that work.
	SetActorTickEnabled(IsValid(Pilot) || bEngineRunning);

	//ComputeProximity();
}

UPrimitiveComponent* ASpaceship::GetPrimaryHullComponent() const
{
	if (SpaceshipHull && SpaceshipHull->GetStaticMesh())
	{
		return SpaceshipHull;
	}
	if (SkeletalSpaceshipHull && SkeletalSpaceshipHull->GetSkeletalMeshAsset())
	{
		return SkeletalSpaceshipHull;
	}
	return nullptr;
}

bool ASpaceship::GetPrimaryHullLocalBounds(UPrimitiveComponent* Hull, FVector& OutMin, FVector& OutMax) const
{
	if (!Hull)
	{
		return false;
	}
	const FBoxSphereBounds LocalBounds = Hull->CalcBounds(FTransform::Identity);
	if (LocalBounds.BoxExtent.IsNearlyZero())
	{
		return false;
	}
	OutMin = LocalBounds.Origin - LocalBounds.BoxExtent;
	OutMax = LocalBounds.Origin + LocalBounds.BoxExtent;
	return true;
}

void ASpaceship::RefreshInteractionGeometry()
{
	if (!bAutoConfigureInteractionGeometry || !PilotChair || !PilotExitPoint || !SphereCollisionComponent
		|| !InteractionBoundsComponent)
	{
		return;
	}
	SphereCollisionComponent->SetCollisionEnabled(
		bProvidesArtificialGravity ? ECollisionEnabled::QueryOnly : ECollisionEnabled::NoCollision);

	UPrimitiveComponent* InteractionMesh = GetPrimaryHullComponent();
	if (!InteractionMesh)
	{
		return;
	}

	FVector LocalBoundsMin;
	FVector LocalBoundsMax;
	if (!GetPrimaryHullLocalBounds(InteractionMesh, LocalBoundsMin, LocalBoundsMax))
	{
		return;
	}
	const FVector LocalCenter = (LocalBoundsMin + LocalBoundsMax) * 0.5;
	const FVector LocalExtent = (LocalBoundsMax - LocalBoundsMin) * 0.5;
	const bool bForceGeneratedConfiguration = bGenerateSimpleHullCollision;

	static const TArray<FName> SeatSocketNames{
		TEXT("PilotSeat"), TEXT("PilotChair"), TEXT("CockpitSeat"), TEXT("DriverSeat"), TEXT("Seat")};
	static const TArray<FName> ExitSocketNames{
		TEXT("PilotExit"), TEXT("ShipExit"), TEXT("RampExit"), TEXT("Entry"), TEXT("Door"), TEXT("Exit")};

	const bool bSeatStillUsesLastAutoTransform = bSeatWasAutoConfigured
		&& PilotChair->GetRelativeTransform().Equals(LastAutoSeatRelativeTransform, 0.1);
	const bool bCanConfigureSeat = bForceGeneratedConfiguration || bSeatStillUsesLastAutoTransform
		|| APSAutomaticShipInteraction::IsNativeSeatTransform(PilotChair);
	if (bCanConfigureSeat)
	{
		FTransform SeatTransform;
		if (!APSAutomaticShipInteraction::FindSocketTransform(InteractionMesh, SeatSocketNames, SeatTransform))
		{
			const FVector FallbackSeatLocation = LocalCenter
				+ FVector(LocalExtent.X * 0.2, 0.0, LocalExtent.Z * 0.15);
			SeatTransform = FTransform(
				InteractionMesh->GetComponentQuat(),
				InteractionMesh->GetComponentTransform().TransformPosition(FallbackSeatLocation),
				FVector::OneVector);
		}
		PilotChair->SetWorldTransform(SeatTransform);
		bSeatWasAutoConfigured = true;
		LastAutoSeatRelativeTransform = PilotChair->GetRelativeTransform();
	}

	const bool bExitStillUsesLastAutoTransform = bExitWasAutoConfigured
		&& PilotExitPoint->GetRelativeTransform().Equals(LastAutoExitRelativeTransform, 0.1);
	const bool bCanConfigureExit = bForceGeneratedConfiguration || bExitStillUsesLastAutoTransform
		|| APSAutomaticShipInteraction::IsNativeExitTransform(PilotExitPoint);
	if (bCanConfigureExit)
	{
		FTransform ExitTransform;
		if (!APSAutomaticShipInteraction::FindSocketTransform(InteractionMesh, ExitSocketNames, ExitTransform))
		{
			const double MeshYScale = FMath::Max(FMath::Abs(InteractionMesh->GetComponentScale().Y), 0.01);
			const FVector FallbackExitLocation = LocalCenter
				+ FVector(0.0, -(LocalExtent.Y + AutoExitClearance / MeshYScale), 0.0);
			ExitTransform = FTransform(
				InteractionMesh->GetComponentQuat(),
				InteractionMesh->GetComponentTransform().TransformPosition(FallbackExitLocation),
				FVector::OneVector);
		}
		PilotExitPoint->SetWorldTransform(ExitTransform);
		bExitWasAutoConfigured = true;
		LastAutoExitRelativeTransform = PilotExitPoint->GetRelativeTransform();
	}

	const bool bZoneUsesNativeDefaults = SphereCollisionComponent->GetRelativeLocation().IsNearlyZero(0.1)
		&& FMath::IsNearlyEqual(SphereCollisionComponent->GetUnscaledSphereRadius(), 1000.0f, 0.1f);
	const bool bZoneStillUsesLastAutoValues = bInteractionZoneWasAutoConfigured
		&& SphereCollisionComponent->GetRelativeLocation().Equals(LastAutoInteractionZoneRelativeLocation, 0.1)
		&& FMath::IsNearlyEqual(
			SphereCollisionComponent->GetUnscaledSphereRadius(), LastAutoInteractionRadius, 0.1f);
	if (bForceGeneratedConfiguration || bZoneStillUsesLastAutoValues || bZoneUsesNativeDefaults)
	{
		// The root is the visual hull and imported ships are commonly actor-scaled.
		// Keep the interaction sphere in world units instead of inheriting that scale.
		SphereCollisionComponent->SetAbsolute(false, false, true);
		// Component bounds can still describe the previous mesh for one frame when a
		// runtime fleet ship has just received its static mesh.  Local mesh bounds are
		// available immediately, so derive the world-space interaction sphere from them.
		SphereCollisionComponent->SetWorldLocation(
			InteractionMesh->GetComponentTransform().TransformPosition(LocalCenter));
		const FVector ScaledMeshExtent = LocalExtent * InteractionMesh->GetComponentScale().GetAbs();
		const double MeshSphereRadius = ScaledMeshExtent.Size();
		const double SphereScale = FMath::Max(SphereCollisionComponent->GetComponentScale().GetAbsMax(), 0.01);
		const double InteractionRadius = (MeshSphereRadius + AutoInteractionPadding) / SphereScale;
		SphereCollisionComponent->SetSphereRadius(FMath::Max(1000.0, InteractionRadius), true);
		bInteractionZoneWasAutoConfigured = true;
		LastAutoInteractionZoneRelativeLocation = SphereCollisionComponent->GetRelativeLocation();
		LastAutoInteractionRadius = SphereCollisionComponent->GetUnscaledSphereRadius();
	}

	InteractionBoundsComponent->AttachToComponent(
		InteractionMesh, FAttachmentTransformRules::KeepRelativeTransform);
	InteractionBoundsComponent->SetRelativeLocation(LocalCenter);
	InteractionBoundsComponent->SetRelativeRotation(FRotator::ZeroRotator);
	const FVector MeshScale = InteractionMesh->GetComponentScale().GetAbs().ComponentMax(FVector(0.01));
	const FVector LocalPadding(
		AutoInteractionPadding / MeshScale.X,
		AutoInteractionPadding / MeshScale.Y,
		AutoInteractionPadding / MeshScale.Z);
	InteractionBoundsComponent->SetBoxExtent(LocalExtent + LocalPadding, true);
	InteractionBoundsComponent->SetCollisionEnabled(
		bAllowExteriorInteraction ? ECollisionEnabled::QueryOnly : ECollisionEnabled::NoCollision);
	InteractionBoundsComponent->SetCollisionResponseToAllChannels(ECR_Ignore);
	InteractionBoundsComponent->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
}

void GetAttachedActorsRecursively(AActor* ParentActor, TArray<AActor*>& OutActors)
{
	TArray<AActor*> AttachedActors;
	ParentActor->GetAttachedActors(AttachedActors);
	for (AActor* ChildActor : AttachedActors)
	{
		OutActors.Add(ChildActor);
		GetAttachedActorsRecursively(ChildActor, OutActors);
	}
}

void ASpaceship::CalculateDistanceAndAddToZones(AWorldActor* WorldActor)
{
	if (WorldActor)
	{
		double DistanceSquared = 0;
		FVector ShipLocation = GetActorLocation();

		double InfluenceRadius = WorldActor->AffectionRadiusKM * 100000;
		// �������������� ��������� � ����� Unreal (1 unit = 1 cm)
		DistanceSquared = FVector::DistSquared(WorldActor->GetActorLocation(), ShipLocation) - FMath::Square(
			InfluenceRadius);

		// �������� ������������� ������ � ������� ��� �� �����
		AActor* ParentActor = WorldActor->GetRootComponent()->GetAttachParent()->GetOwner();
		FString ParentName = ParentActor ? *ParentActor->GetName() : FString("No Parent");

		// ������� ��������� �� �����
		double DistanceInKm = FMath::Sqrt(DistanceSquared) / 100000;
		FString Message = FString::Printf(
			TEXT("Distance to %s (orbiting %s): %f kilometers."), *WorldActor->GetName(), *ParentName, DistanceInKm);

		GEngine->AddOnScreenDebugMessage(-1, 0.f, FColor::Cyan, Message);
	}
}


TSharedPtr<FStarModel> FindNearestStar(TMap<FVector, TSharedPtr<FStarModel>>& Stars, const FVector& ReferencePoint)
{
	TSharedPtr<FStarModel> NearestStar = nullptr;
	float NearestDistanceSquared = FLT_MAX;

	for (const auto& KeyValuePair : Stars)
	{
		float TotalDistSquared = FVector::DistSquared(ReferencePoint, KeyValuePair.Key);
		float RadiusSquared = FMath::Square(KeyValuePair.Value->Radius);
		float DistanceSquared = TotalDistSquared > RadiusSquared ? TotalDistSquared - RadiusSquared : 0.0f;

		if (DistanceSquared < NearestDistanceSquared)
		{
			NearestDistanceSquared = DistanceSquared;
			NearestStar = KeyValuePair.Value;
		}
	}

	return NearestStar;
}

void ASpaceship::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!OnboardComputer || !SpaceshipHull)
	{
		return;
	}

	const bool bMeasure = IsValid(Pilot) && APSShipPerf::Enabled();
	const FVector LocationBeforeMove = GetActorLocation();
	if (IsValid(Pilot) && APSShipPerf::CVarHullSceneLightingInFlight.GetValueOnGameThread() != AppliedHullSceneLightingMode)
	{
		// A console A/B of the hull lighting takes effect in flight, without leaving the seat.
		SetHullSceneLightingExcluded(false);
		SetHullSceneLightingExcluded(true);
	}
	{
		TRACE_CPUPROFILER_EVENT_SCOPE(APS_Ship_Environment);
		APSShipPerf::FScope Scope(bMeasure, APSShipPerf::Environment);
		UpdateFlightEnvironment(DeltaTime);
	}
	if (ShipNavigation && IsValid(Pilot) && (bNavigationMarkersVisible || bNavigationPanelVisible))
	{
		TRACE_CPUPROFILER_EVENT_SCOPE(APS_Ship_Navigation);
		APSShipPerf::FScope Scope(bMeasure, APSShipPerf::Navigation);
		ShipNavigation->RefreshContacts(GetActorLocation());
	}

	const double TargetBoost = bEngineRunning && bIsAccelerating ? ActiveClassPreset.MaximumBoost : 1.0;
	const double BoostResponse = bIsAccelerating
		? ActiveClassPreset.BoostGrowthPerSecond
		: BoostRecoverySpeed;
	CurrentBoostMultiplier = FMath::FInterpConstantTo(
		CurrentBoostMultiplier, TargetBoost, DeltaTime, BoostResponse);

	AdvanceEngineModeTransition(DeltaTime);
	{
		TRACE_CPUPROFILER_EVENT_SCOPE(APS_Ship_Move);
		APSShipPerf::FScope Scope(bMeasure, APSShipPerf::Move);
		if (!FAPSShipFlightBenchmark::TickShip(*this, DeltaTime) && !ApplyCustomFlightTranslation(DeltaTime))
		{
			ApplyFlightInput(DeltaTime);
		}
	}
	{
		TRACE_CPUPROFILER_EVENT_SCOPE(APS_Ship_Rotation);
		APSShipPerf::FScope Scope(bMeasure, APSShipPerf::Rotation);
		ApplyRotationInput(DeltaTime);
	}
	{
		TRACE_CPUPROFILER_EVENT_SCOPE(APS_Ship_Stabilize);
		APSShipPerf::FScope Scope(bMeasure, APSShipPerf::Stabilize);
		StabilizeFullScaleVisualVelocity();
	}
	{
		TRACE_CPUPROFILER_EVENT_SCOPE(APS_Ship_Camera);
		APSShipPerf::FScope Scope(bMeasure, APSShipPerf::Camera);
		UpdateAdaptiveFlightCamera(DeltaTime);
	}
	if (bMeasure)
	{
		APSShipPerf::FWindow& Window = APSShipPerf::GPiloted;
		const double FrameMs = DeltaTime * 1000.0;
		Window.FrameSum += FrameMs;
		Window.FrameMax = FMath::Max(Window.FrameMax, FrameMs);
		Window.Hitches += FrameMs > 33.4 ? 1 : 0;
		Window.MaxMove = FMath::Max(Window.MaxMove, FVector::Distance(LocationBeforeMove, GetActorLocation()));
		++Window.Frames;
		Window.Elapsed += DeltaTime;
		if (Window.Elapsed >= 2.0 && Window.Frames > 0)
		{
			FString Sections;
			for (int32 Index = 0; Index < APSShipPerf::Count; ++Index)
			{
				Sections += FString::Printf(TEXT(" %s %.2f/%.2f"), APSShipPerf::SectionNames[Index],
					Window.Sum[Index] / Window.Frames, Window.Max[Index]);
			}
			UE_LOG(LogTemp, Log,
				TEXT("[APS.ShipPerf] ship=%s speed=%.1f m/s env=%s engine=%s drive=%s frame %.2f/%.2f ms hitches=%d maxMovePerFrame=%.0f m | avg/max ms:%s"),
				*GetName(), GetShipSpeedMetersPerSecond(), *GetFlightEnvironmentName(), *GetEngineModeName(),
				*GetDriveModeName(), Window.FrameSum / Window.Frames, Window.FrameMax, Window.Hitches,
				Window.MaxMove / 100.0, *Sections);
			Window = APSShipPerf::FWindow();
		}
	}
	/*uint64 StartCycles = FPlatformTime::Cycles();

	if (!bEngineRunning)
	{
		if (OnboardComputer->FlightSystem.CurrentFlightMode == EFlightMode::Station || OnboardComputer->FlightSystem.
			CurrentFlightMode == EFlightMode::Planetary)
		{
			const float GravityImpulseStrength = -100.0f;
			FVector DownwardImpulse = GetActorUpVector() * GravityImpulseStrength;
			SpaceshipHull->AddImpulse(DownwardImpulse, NAME_None, true);
		}
	}
	else
	{
		if (OffsetSystem && OnboardComputer->EngineSystem.CurrentEngineMode == EEngineMode::SpaceWrap)
		{
			double EngineThrustForce = OnboardComputer->GetEngineThrustForce();
			const FVector Direction = ForwardVector->GetForwardVector();
			OffsetSystem->AddActorLocalOffset(-Direction * EngineThrustForce); /// CRASHED PIE!
		}

		PrintOnboardComputerBasicIformation();

		if (SpaceshipHull->IsSimulatingPhysics())
		{
			FVector OppositeTorque = -SpaceshipHull->GetPhysicsAngularVelocityInRadians() * 0.5;
			SpaceshipHull->AddTorqueInRadians(OppositeTorque, NAME_None, true);
		}
		if (bIsAccelerating)
		{
			GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Green, FString::Printf(TEXT("Accelerating")));
			OnboardComputer->AccelerateBoost(DeltaTime);
		}
		if (bIsDecelerating)
		{
			GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Green, FString::Printf(TEXT("Decelerating")));
			OnboardComputer->DecelerateBoost(DeltaTime);
		}
		if (ClosestActor != nullptr)
		{
			FString ClosestActorMessage = FString::Printf(TEXT("Closest actor is %s"), *ClosestActor->GetName());
			GEngine->AddOnScreenDebugMessage(-1, 0.f, FColor::Orange, ClosestActorMessage);
		}
		if (AffectedActor != nullptr)
		{
			FString ClosestAffectedActorMessage = FString::Printf(
				TEXT("Affection zone is %s"), *AffectedActor->GetName());
			GEngine->AddOnScreenDebugMessage(-1, 0.f, FColor::Orange, ClosestAffectedActorMessage);
		}

		ComputeProximity();

		// ���� �� � ������ Interstellar, ��������� ��������� ������
		if (OnboardComputer->FlightSystem.CurrentFlightMode == EFlightMode::Interstellar)
		{
			//ComputeClosestStar(); // �������� OffsetSystem

			if (!OffsetSystem) return;

			double Distance = FVector::Dist(this->GetActorLocation(), OffsetSystem->GetActorLocation());
			double AffectionRadiusUnits = OffsetSystem->AffectionRadiusKM * 100000 / 1000000000.0; /// CRASH PIE!!!
			// ��������� �� � ����� � ��������� ���������������
			GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Orange,
			                                 FString::Printf(
				                                 TEXT("Distance: %f, Affection Radius: %f"), Distance,
				                                 AffectionRadiusUnits));

			// ���� �� � �������� ���� �������, ������������� �� Stellar
			if (Distance < AffectionRadiusUnits && LastFlightMode != EFlightMode::Stellar)
			{
				OnboardComputer->FlightSystem.CurrentFlightMode = EFlightMode::Stellar;
				OnboardComputer->FlightSystem.CurrentFlightType = EFlightType::LightSpeed;
				OnboardComputer->SwitchEngineMode(EEngineMode::SpaceWrap);

				GEngine->AddOnScreenDebugMessage(-1, 0.f, FColor::Orange, TEXT("AffectionRadiusUnits!!!"));
			}
		}
		else
		{
			if (!AffectedActor) //return;
			{
				OnboardComputer->FlightSystem.CurrentFlightMode = EFlightMode::Interstellar;
				OnboardComputer->FlightSystem.CurrentFlightType = EFlightType::FTL;
				OnboardComputer->SwitchEngineMode(EEngineMode::Offset);
			}
			else if (AffectedActor->IsA(APlanet::StaticClass()))
			{
				APlanet* Planet = Cast<APlanet>(AffectedActor);



				// �������� ������� ������� � �������.
				FVector ShipPosition = GetActorLocation();
				FVector PlanetPosition = Planet->GetActorLocation();
				// ��������� ���������� ����� �������� � ��������.
				double DistanceToPlanet = FVector::Dist(ShipPosition, PlanetPosition) - Planet->GetRadius();
				DistanceToPlanet /= 100000.0;
				// ����� ������ ������ �� ������ ���������� �� �������.
				if (DistanceToPlanet <= Planet->RadiusKM + Planet->AtmosphereHeight)
				{
					// ���� ������� ������ ��������� �������.
					if (DistanceToPlanet - Planet->RadiusKM <= 10.0)
					{
						GEngine->AddOnScreenDebugMessage(-1, 0.f, FColor::Red, TEXT("Surface Flight!"));

						// ���� ������� ����� 10 �� � ����������� �������.
						OnboardComputer->FlightSystem.CurrentFlightMode = EFlightMode::Surface;
						//OnboardComputer->FlightSystem.CurrentFlightType = EFlightType::Atmospheric;
						OnboardComputer->SwitchEngineMode(EEngineMode::Impulse);
					}
					else
					{
						// ���� ������� ������ ���������, �� ������ 10 �� �� �����������.
						GEngine->AddOnScreenDebugMessage(-1, 0.f, FColor::Red, TEXT("Atmospheric Flight!"));

						OnboardComputer->FlightSystem.CurrentFlightMode = EFlightMode::Atmospheric;
						//OnboardComputer->FlightSystem.CurrentFlightType = EFlightType::Atmospheric;
						//OnboardComputer->SwitchEngineMode(EEngineMode::Impulse);
					}
					GEngine->AddOnScreenDebugMessage(-1, 0.f, FColor::Yellow,
					                                 FString::Printf(
						                                 TEXT("Distance to Planet Surface: %f"),
						                                 DistanceToPlanet - Planet->RadiusKM));
				}
				else if (DistanceToPlanet <= Planet->RadiusKM + Planet->OrbitHeight)
				{
					// ���� ������� ������ ������ �������, �� �� ��������� ���������.
					OnboardComputer->FlightSystem.CurrentFlightMode = EFlightMode::Orbital;
					//OnboardComputer->FlightSystem.CurrentFlightType = EFlightType::Orbital;
					//OnboardComputer->SwitchEngineMode(EEngineMode::SpaceWrap);
					//GEngine->AddOnScreenDebugMessage(-1, 0.f, FColor::Yellow, FString::Printf(TEXT("Orbit Height: %f"), DistanceToPlanet - Planet->RadiusKM));
				}
				else
				{
					OnboardComputer->FlightSystem.CurrentFlightMode = EFlightMode::Planetary;
					//OnboardComputer->FlightSystem.CurrentFlightType = EFlightType::LightSpeed;
					//OnboardComputer->SwitchEngineMode(EEngineMode::SpaceWrap);
				}
			}
			else if (AffectedActor->IsA(AMoon::StaticClass()))
			{
				OnboardComputer->FlightSystem.CurrentFlightMode = EFlightMode::Orbital;
				//OnboardComputer->FlightSystem.CurrentFlightType = EFlightType::Orbital;
				//OnboardComputer->SwitchEngineMode(EEngineMode::SpaceWrap);//InitiateOffsetMode();
			}
			else if (AffectedActor->IsA(AStar::StaticClass()))
			{
				OnboardComputer->FlightSystem.CurrentFlightMode = EFlightMode::Interplanetary;
				//OnboardComputer->FlightSystem.CurrentFlightType = EFlightType::LightSpeed;
				//OnboardComputer->SwitchEngineMode(EEngineMode::SpaceWrap);//InitiateOffsetMode();
			}
			// Techical objects: space stations, satellites, etc.
			else if (AffectedActor->IsA(ATechActor::StaticClass()))
			{
				OnboardComputer->FlightSystem.CurrentFlightMode = EFlightMode::Station;
				//OnboardComputer->FlightSystem.CurrentFlightType = EFlightType::ArtificialGravity;
				//OnboardComputer->SwitchEngineMode(EEngineMode::Impulse);
			}
			// Star clusters: galaxies, nebulae, etc.
			else if (AffectedActor->IsA(AStarCluster::StaticClass()))
			{
				OnboardComputer->FlightSystem.CurrentFlightMode = EFlightMode::Interstellar;
				//OnboardComputer->FlightSystem.CurrentFlightType = EFlightType::FTL;
				//OnboardComputer->SwitchEngineMode(EEngineMode::Offset);

				OffsetGalaxy = Cast<AAstroActor>(AffectedActor);
			}
			// Between star system and star cluster
			else if (AffectedActor->IsA(AStarSystem::StaticClass()))
			{
				OnboardComputer->FlightSystem.CurrentFlightMode = EFlightMode::Stellar;
				//OnboardComputer->FlightSystem.CurrentFlightType = EFlightType::LightSpeed;
				//OnboardComputer->SwitchEngineMode(EEngineMode::SpaceWrap);
				//CalculateProximity = false;
				OffsetGalaxy = Cast<AAstroActor>(AffectedActor);
			}
			// All others
			else
			{
				OnboardComputer->FlightSystem.CurrentFlightMode = EFlightMode::Basic;
				OnboardComputer->FlightSystem.CurrentFlightType = EFlightType::ZeroG;
				OnboardComputer->SwitchEngineMode(EEngineMode::Impulse);
			}
		}

		if (OnboardComputer->FlightSystem.CurrentFlightMode != LastFlightMode)
		{
			if (LastFlightMode == EFlightMode::Interplanetary && OnboardComputer->FlightSystem.CurrentFlightMode ==
				EFlightMode::Planetary)
			{
				// WorldScape lifetime is owned by APSPlanetEnvironmentStreamingSubsystem.
				// The legacy InitWSC path spawned another root for the planet and every
				// moon whenever flight mode changed, leaving duplicate runtime actors.
				AffectedPlanet = Cast<APlanet>(AffectedActor);
			}
			else if (LastFlightMode == EFlightMode::Planetary && OnboardComputer->FlightSystem.CurrentFlightMode ==
				EFlightMode::Interplanetary)
			{
				AffectedPlanet = nullptr;
			}


			switch (OnboardComputer->FlightSystem.CurrentFlightMode)
			{
			case EFlightMode::Interstellar:
				OnInterstellarMode.Broadcast();
				break;

			case EFlightMode::Stellar:
				OnStellarMode.Broadcast();
				break;

			case EFlightMode::Interplanetary:
				OnInterplanetaryMode.Broadcast();
				break;
			default:
				break;
			}

			UpdateNavigatableActors();
			LastFlightMode = OnboardComputer->FlightSystem.CurrentFlightMode;
		}

		//OnboardComputer->ComputeFlightParams();
		// Switch Engine mode
	}
	if (Pilot)
	{
		GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Green,
		                                 FString::Printf(TEXT("Pilot Actor Name: %s"), *Pilot->GetName()));
	}
	uint64 EndCycles = FPlatformTime::Cycles();
	double ElapsedTime = FPlatformTime::ToSeconds(EndCycles - StartCycles);
	GEngine->AddOnScreenDebugMessage(-1, 0.f, FColor::Red,
	                                 FString::Printf(TEXT("Time elapsed: %f seconds"), ElapsedTime));*/
}

FSpaceshipClassPreset ASpaceship::GetPresetForSizeClass(ESpaceshipSizeClass InSizeClass)
{
	FSpaceshipClassPreset Preset;
	switch (InSizeClass)
	{
	case ESpaceshipSizeClass::XXS:
		Preset.ImpulseAcceleration = 1800.0;
		Preset.MaxImpulseSpeed = 60000.0;
		Preset.RotationSpeed = 105.0;
		Preset.AngularAcceleration = 240.0;
		Preset.LinearDamping = 0.08;
		Preset.AngularDamping = 3.5;
		Preset.MaximumBoost = 5.0;
		Preset.BoostGrowthPerSecond = 1.35;
		Preset.bSupportsSpaceWrap = false;
		Preset.bSupportsOffset = false;
		Preset.bUsesPhysicalImpulse = true;
		Preset.bHasInteriorByDefault = false;
		Preset.MaximumFlightMode = EFlightMode::Planetary;
		break;
	case ESpaceshipSizeClass::XS:
		Preset.ImpulseAcceleration = 1500.0;
		Preset.MaxImpulseSpeed = 85000.0;
		Preset.RotationSpeed = 88.0;
		Preset.AngularAcceleration = 180.0;
		Preset.LinearDamping = 0.1;
		Preset.AngularDamping = 3.2;
		Preset.MaximumBoost = 5.0;
		Preset.BoostGrowthPerSecond = 1.2;
		Preset.bSupportsSpaceWrap = false;
		Preset.bSupportsOffset = false;
		Preset.bUsesPhysicalImpulse = true;
		Preset.bHasInteriorByDefault = false;
		Preset.MaximumFlightMode = EFlightMode::Planetary;
		break;
	case ESpaceshipSizeClass::S:
		Preset.ImpulseAcceleration = 1200.0;
		Preset.MaxImpulseSpeed = 120000.0;
		Preset.RotationSpeed = 70.0;
		Preset.AngularAcceleration = 125.0;
		Preset.LinearDamping = 0.13;
		Preset.AngularDamping = 2.9;
		Preset.MaximumBoost = 4.5;
		Preset.BoostGrowthPerSecond = 1.05;
		Preset.bSupportsSpaceWrap = true;
		Preset.bSupportsOffset = false;
		Preset.bUsesPhysicalImpulse = true;
		Preset.bHasInteriorByDefault = true;
		Preset.MaximumFlightMode = EFlightMode::Stellar;
		break;
	case ESpaceshipSizeClass::M:
		Preset.ImpulseAcceleration = 900.0;
		Preset.MaxImpulseSpeed = 160000.0;
		Preset.RotationSpeed = 55.0;
		Preset.AngularAcceleration = 75.0;
		Preset.LinearDamping = 0.18;
		Preset.AngularDamping = 2.5;
		Preset.MaximumBoost = 4.0;
		Preset.BoostGrowthPerSecond = 0.9;
		Preset.bSupportsSpaceWrap = true;
		Preset.bSupportsOffset = true;
		Preset.bUsesPhysicalImpulse = true;
		Preset.bHasInteriorByDefault = true;
		Preset.MaximumFlightMode = EFlightMode::Interstellar;
		break;
	case ESpaceshipSizeClass::L:
		Preset.ImpulseAcceleration = 650.0;
		Preset.MaxImpulseSpeed = 220000.0;
		Preset.RotationSpeed = 36.0;
		Preset.AngularAcceleration = 42.0;
		Preset.LinearDamping = 0.22;
		Preset.AngularDamping = 2.2;
		Preset.MaximumBoost = 3.5;
		Preset.BoostGrowthPerSecond = 0.72;
		Preset.bSupportsSpaceWrap = true;
		Preset.bSupportsOffset = true;
		Preset.bUsesPhysicalImpulse = false;
		Preset.bHasInteriorByDefault = true;
		Preset.MaximumFlightMode = EFlightMode::Intergalaxy;
		break;
	case ESpaceshipSizeClass::XL:
		Preset.ImpulseAcceleration = 450.0;
		Preset.MaxImpulseSpeed = 300000.0;
		Preset.RotationSpeed = 25.0;
		Preset.AngularAcceleration = 25.0;
		Preset.LinearDamping = 0.27;
		Preset.AngularDamping = 2.0;
		Preset.MaximumBoost = 3.2;
		Preset.BoostGrowthPerSecond = 0.58;
		Preset.bSupportsSpaceWrap = true;
		Preset.bSupportsOffset = true;
		Preset.bUsesPhysicalImpulse = false;
		Preset.bHasInteriorByDefault = true;
		Preset.MaximumFlightMode = EFlightMode::Intergalaxy;
		break;
	case ESpaceshipSizeClass::XXL:
		Preset.ImpulseAcceleration = 300.0;
		Preset.MaxImpulseSpeed = 450000.0;
		Preset.RotationSpeed = 16.0;
		Preset.AngularAcceleration = 12.0;
		Preset.LinearDamping = 0.32;
		Preset.AngularDamping = 1.8;
		Preset.MaximumBoost = 2.8;
		Preset.BoostGrowthPerSecond = 0.45;
		Preset.bSupportsSpaceWrap = true;
		Preset.bSupportsOffset = true;
		Preset.bUsesPhysicalImpulse = false;
		Preset.bHasInteriorByDefault = true;
		Preset.MaximumFlightMode = EFlightMode::Intergalaxy;
		break;
	case ESpaceshipSizeClass::Titan:
		Preset.ImpulseAcceleration = 180.0;
		Preset.MaxImpulseSpeed = 600000.0;
		Preset.RotationSpeed = 9.0;
		Preset.AngularAcceleration = 6.0;
		Preset.LinearDamping = 0.38;
		Preset.AngularDamping = 1.6;
		Preset.MaximumBoost = 2.4;
		Preset.BoostGrowthPerSecond = 0.32;
		Preset.bSupportsSpaceWrap = true;
		Preset.bSupportsOffset = true;
		Preset.bUsesPhysicalImpulse = false;
		Preset.bHasInteriorByDefault = true;
		Preset.MaximumFlightMode = EFlightMode::Intergalaxy;
		break;
	}
	return Preset;
}

ESpaceshipSizeClass ASpaceship::InferSizeClassFromLength(double LengthCentimeters)
{
	if (LengthCentimeters <= 2000.0) return ESpaceshipSizeClass::XXS;
	if (LengthCentimeters <= 5000.0) return ESpaceshipSizeClass::XS;
	if (LengthCentimeters <= 15000.0) return ESpaceshipSizeClass::S;
	if (LengthCentimeters <= 50000.0) return ESpaceshipSizeClass::M;
	if (LengthCentimeters <= 200000.0) return ESpaceshipSizeClass::L;
	if (LengthCentimeters <= 1000000.0) return ESpaceshipSizeClass::XL;
	if (LengthCentimeters <= 10000000.0) return ESpaceshipSizeClass::XXL;
	return ESpaceshipSizeClass::Titan;
}

void ASpaceship::ConfigureFlightReferenceFromHull(UPrimitiveComponent* Hull, const FVector& LocalExtent)
{
	// Legacy interior ships already author their actual nose direction with this hidden arrow.
	// Prefer it when authored; generated hulls keep the bounds-axis fallback below.
	const bool bHasAuthoredForward = ForwardVector
		&& (bUseAuthoredNoseDirection
			|| ForwardVector->GetStaticMesh() != nullptr
			|| !ForwardVector->GetRelativeRotation().IsNearlyZero(0.1));
	if (bHasAuthoredForward && SpaceshipHull)
	{
		const FTransform HullTransform = SpaceshipHull->GetComponentTransform();
		FlightForwardLocalAxis = HullTransform.InverseTransformVectorNoScale(
			ForwardVector->GetForwardVector()).GetSafeNormal();
		FlightUpLocalAxis = HullTransform.InverseTransformVectorNoScale(
			ForwardVector->GetUpVector()).GetSafeNormal();
		if (!FlightForwardLocalAxis.IsNearlyZero() && !FlightUpLocalAxis.IsNearlyZero())
		{
			return;
		}
	}

	int32 ForwardAxisIndex = 0;
	if (LocalExtent.Y > LocalExtent.X && LocalExtent.Y >= LocalExtent.Z)
	{
		ForwardAxisIndex = 1;
	}
	else if (LocalExtent.Z > LocalExtent.X && LocalExtent.Z > LocalExtent.Y)
	{
		ForwardAxisIndex = 2;
	}

	FlightForwardLocalAxis = FVector::ZeroVector;
	FlightForwardLocalAxis[ForwardAxisIndex] = 1.0;
	FlightUpLocalAxis = ForwardAxisIndex == 2 ? FVector::RightVector : FVector::UpVector;
	if (Hull && Hull != SpaceshipHull)
	{
		const FQuat RelativeRotation = Hull->GetRelativeRotation().Quaternion();
		FlightForwardLocalAxis = RelativeRotation.RotateVector(FlightForwardLocalAxis).GetSafeNormal();
		FlightUpLocalAxis = RelativeRotation.RotateVector(FlightUpLocalAxis).GetSafeNormal();
	}
}

void ASpaceship::RefreshFlightReferenceFromHull()
{
	UPrimitiveComponent* MainMesh = GetPrimaryHullComponent();
	if (!MainMesh)
	{
		return;
	}
	MainMesh->UpdateBounds();
	FVector LocalMin;
	FVector LocalMax;
	if (GetPrimaryHullLocalBounds(MainMesh, LocalMin, LocalMax))
	{
		ConfigureFlightReferenceFromHull(MainMesh, (LocalMax - LocalMin) * 0.5);
	}
}

void ASpaceship::ConfigureFromHull()
{
	RefreshFlightReferenceFromHull();
	UPrimitiveComponent* MainMesh = GetPrimaryHullComponent();
	if (MainMesh && bInferSizeClassFromHull)
	{
		const FVector Size = MainMesh->Bounds.BoxExtent * 2.0;
		SizeClass = InferSizeClassFromLength(Size.GetMax());
	}

	ActiveClassPreset = GetPresetForSizeClass(SizeClass);
	if (bGenerateSimpleHullCollision)
	{
		// Generated AI hulls can have complex-as-simple or otherwise unusable bodies.
		// Their lightweight proxy collision is moved kinematically with the actor.
		ActiveClassPreset.bUsesPhysicalImpulse = false;
		RebuildSimpleHullCollision();
	}
	RotationSpeedDegreesPerSecond = ActiveClassPreset.RotationSpeed;
	ImpulseRotationAcceleration = FMath::DegreesToRadians(ActiveClassPreset.AngularAcceleration);
	if (static_cast<uint8>(SelectedDriveMode) > static_cast<uint8>(GetMaximumDriveModeForClass()))
	{
		SelectedDriveMode = GetMaximumDriveModeForClass();
	}

	if (SpaceshipHull)
	{
		SpaceshipHull->SetMobility(EComponentMobility::Movable);
		SpaceshipHull->SetEnableGravity(false);
		SpaceshipHull->SetLinearDamping(ActiveClassPreset.LinearDamping);
		SpaceshipHull->SetAngularDamping(ActiveClassPreset.AngularDamping);
	}
	ConfigureCameraFromHull();
	ConfigurePilotFillLight();
}

bool ASpaceship::HullHasFittedConvexCollision() const
{
	const UStaticMesh* HullMesh = SpaceshipHull ? SpaceshipHull->GetStaticMesh() : nullptr;
	const UBodySetup* BodySetup = HullMesh ? HullMesh->GetBodySetup() : nullptr;
	return BodySetup && BodySetup->CollisionTraceFlag != CTF_UseComplexAsSimple
		&& BodySetup->AggGeom.ConvexElems.Num() > 0;
}

void ASpaceship::RebuildSimpleHullCollision()
{
	for (UBoxComponent* Box : GeneratedCollisionBoxes)
	{
		if (IsValid(Box))
		{
			Box->DestroyComponent();
		}
	}
	GeneratedCollisionBoxes.Reset();

	if (!bGenerateSimpleHullCollision)
	{
		return;
	}

	UPrimitiveComponent* MainMesh = GetPrimaryHullComponent();
	if (!MainMesh)
	{
		return;
	}
	// Hull meshes with fitted convex collision already hug the hull; the tapered boxes below span the
	// whole bounding box and block the pilot metres away from wings and fins.
	if (!bBuildingFlightCollisionProxy && HullHasFittedConvexCollision())
	{
		MainMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		return;
	}

	FVector LocalMin;
	FVector LocalMax;
	if (!GetPrimaryHullLocalBounds(MainMesh, LocalMin, LocalMax))
	{
		return;
	}
	const FVector Center = (LocalMin + LocalMax) * 0.5;
	const FVector Extent = (LocalMax - LocalMin) * 0.5;
	if (Extent.IsNearlyZero())
	{
		return;
	}

	int32 MajorAxis = 0;
	if (Extent.Y > Extent.X && Extent.Y >= Extent.Z) MajorAxis = 1;
	else if (Extent.Z > Extent.X && Extent.Z > Extent.Y) MajorAxis = 2;

	const int32 SliceCount = FMath::Clamp(SimpleCollisionSliceCount, 3, 9);
	const double HalfSliceLength = Extent[MajorAxis] / SliceCount;
	for (int32 SliceIndex = 0; SliceIndex < SliceCount; ++SliceIndex)
	{
		const double Along01 = (static_cast<double>(SliceIndex) + 0.5) / SliceCount;
		const double DistanceFromCenter = FMath::Abs(Along01 * 2.0 - 1.0);
		const double Taper = FMath::Lerp(1.0, 0.58, FMath::Pow(DistanceFromCenter, 1.55));

		FVector BoxExtent = Extent * Taper;
		BoxExtent[MajorAxis] = HalfSliceLength * 1.03;
		BoxExtent.X = FMath::Max(BoxExtent.X, 25.0);
		BoxExtent.Y = FMath::Max(BoxExtent.Y, 25.0);
		BoxExtent.Z = FMath::Max(BoxExtent.Z, 25.0);

		FVector BoxCenter = Center;
		BoxCenter[MajorAxis] = LocalMin[MajorAxis] + (SliceIndex * 2.0 + 1.0) * HalfSliceLength;
		UBoxComponent* Box = NewObject<UBoxComponent>(this,
			*FString::Printf(TEXT("SimpleHullCollision_%02d"), SliceIndex), RF_Transient);
		Box->SetupAttachment(MainMesh);
		Box->SetMobility(EComponentMobility::Movable);
		Box->SetBoxExtent(BoxExtent, false);
		Box->SetRelativeLocation(BoxCenter);
		Box->SetCollisionProfileName(TEXT("BlockAll"));
		Box->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		Box->SetCollisionResponseToAllChannels(ECR_Block);
		Box->SetGenerateOverlapEvents(false);
		Box->SetCanEverAffectNavigation(false);
		AddInstanceComponent(Box);
		Box->RegisterComponent();
		GeneratedCollisionBoxes.Add(Box);
	}

	// The imported body is never queried after the proxy hull exists.
	MainMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	MainMesh->SetGenerateOverlapEvents(false);
	UE_LOG(LogTemp, Verbose, TEXT("[APS.Ships] %s generated %d low-cost collision slices for %s"),
		*GetName(), GeneratedCollisionBoxes.Num(), *GetNameSafe(MainMesh));
}

void ASpaceship::ConfigureCameraFromHull()
{
	if (!SpringArmComponent)
	{
		return;
	}

	UPrimitiveComponent* MainMesh = GetPrimaryHullComponent();
	if (!MainMesh)
	{
		return;
	}

	FVector LocalMin;
	FVector LocalMax;
	if (!GetPrimaryHullLocalBounds(MainMesh, LocalMin, LocalMax))
	{
		return;
	}
	const FVector LocalCenter = (LocalMin + LocalMax) * 0.5;
	const FVector LocalExtent = (LocalMax - LocalMin) * 0.5;
	const FVector ScaledExtent = LocalExtent * MainMesh->GetComponentScale().GetAbs();
	const double WorldRadius = FMath::Max(ScaledExtent.Size(), 400.0);
	BaseCameraArmLength = FMath::Max(820.0, WorldRadius * 1.82);
	if (CameraComponent && !bCameraFieldOfViewInitialized)
	{
		BaseCameraFieldOfView = CameraComponent->FieldOfView;
		bCameraFieldOfViewInitialized = true;
	}
	SpringArmComponent->TargetArmLength = BaseCameraArmLength;
	// Never hard-clamp the lagged camera origin. The spring-arm clamp produces a
	// discontinuity whenever the moving ship crosses the limit, which looks like
	// a periodic stop/go pulse even though the pawn trajectory itself is smooth.
	SpringArmComponent->CameraLagMaxDistance = 0.0f;
	SpringArmComponent->bEnableCameraLag = false;
	SpringArmComponent->bUseCameraLagSubstepping = false;
	SpringArmComponent->CameraLagMaxTimeStep = 1.0f / 120.0f;
	SpringArmComponent->bClampToMaxPhysicsDeltaTime = false;
	SpringArmComponent->SetWorldLocation(
		MainMesh->GetComponentTransform().TransformPosition(LocalCenter) + GetShipUpVector() * WorldRadius * 0.2);
	const FRotator FlightViewRotation = FRotationMatrix::MakeFromXZ(
		FlightForwardLocalAxis, FlightUpLocalAxis).Rotator();
	SpringArmComponent->SetRelativeRotation(FlightViewRotation + FRotator(-12.0, 0.0, 0.0));
	SpringArmComponent->bDoCollisionTest = false;
}

void ASpaceship::ConfigurePilotFillLight()
{
	const UPrimitiveComponent* MainMesh = GetPrimaryHullComponent();
	const float HullRadius = MainMesh ? FMath::Max(MainMesh->Bounds.SphereRadius, 400.0f) : 400.0f;
	const float CameraDistance = FMath::Max(BaseCameraArmLength, 820.0f);
	if (PilotFillLight)
	{
		PilotFillLight->SetAttenuationRadius(CameraDistance + HullRadius * 2.5f);
		PilotFillLight->SetIntensity(FMath::Clamp(2200.0f + HullRadius * 0.35f, 2600.0f, 8500.0f));
	}
	if (PilotFillPointLight)
	{
		PilotFillPointLight->SetAttenuationRadius(CameraDistance + HullRadius * 3.0f);
		PilotFillPointLight->SetIntensity(FMath::Clamp(
			12.0f + HullRadius / 900.0f, 14.0f, 32.0f));
	}

	TArray<UMeshComponent*> ShipMeshComponents;
	GetComponents<UMeshComponent>(ShipMeshComponents);
	for (UMeshComponent* ShipMesh : ShipMeshComponents)
	{
		if (IsValid(ShipMesh))
		{
			// Preserve normal world lighting and opt only this ship into the
			// private camera-fill channel.
			ShipMesh->SetLightingChannels(true, false, true);
		}
	}
}

void ASpaceship::UpdatePilotFillLightVisibility()
{
	if (PilotFillLight)
	{
		// Retained for serialized BP compatibility; the point fill is independent
		// of legacy mesh-forward conventions and cannot accidentally light empty space.
		PilotFillLight->SetVisibility(false, true);
	}

	if (PilotFillPointLight)
	{
		// The private fill is a readability fallback for interplanetary darkness,
		// not a replacement for physically meaningful local star/planet lighting.
		// Rio 2026-09-28: this fill lit only the piloted ship, so it read white next to black
		// neighbours. The object fill (aps.Lighting.ObjectFill) now lights every ship alike.
		static const IConsoleVariable* PilotFill = IConsoleManager::Get().FindConsoleVariable(TEXT("aps.Lighting.PilotFill"));
		const bool bShouldUseFill = IsValid(Pilot)
			&& CurrentFlightEnvironment == EShipFlightEnvironment::DeepSpace
			&& PilotFill && PilotFill->GetInt() != 0;
		PilotFillPointLight->SetVisibility(bShouldUseFill, true);
	}
}

FVector ASpaceship::GetShipForwardVector() const
{
	return SpaceshipHull
		? SpaceshipHull->GetComponentTransform().TransformVectorNoScale(FlightForwardLocalAxis).GetSafeNormal()
		: GetActorForwardVector();
}

FVector ASpaceship::GetShipUpVector() const
{
	return SpaceshipHull
		? SpaceshipHull->GetComponentTransform().TransformVectorNoScale(FlightUpLocalAxis).GetSafeNormal()
		: GetActorUpVector();
}

FVector ASpaceship::GetShipRightVector() const
{
	return FVector::CrossProduct(GetShipUpVector(), GetShipForwardVector()).GetSafeNormal();
}

EShipDriveMode ASpaceship::GetMaximumDriveModeForClass() const
{
	// All spacecraft keep the complete six-step power range. Engine availability is a separate axis.
	return EShipDriveMode::Interstellar;
}

EShipDriveMode ASpaceship::GetMaximumDriveModeForEnvironment() const
{
	// Environment affects drag and telemetry, never removes player authority.
	return EShipDriveMode::Interstellar;
}

bool ASpaceship::CanUseDriveMode(EShipDriveMode DriveMode) const
{
	return static_cast<uint8>(DriveMode) <= static_cast<uint8>(GetMaximumDriveModeForClass())
		&& static_cast<uint8>(DriveMode) <= static_cast<uint8>(GetMaximumDriveModeForEnvironment());
}

double ASpaceship::GetDriveSpeedScale(EShipDriveMode DriveMode)
{
	switch (DriveMode)
	{
	case EShipDriveMode::Local: return 1.0;
	case EShipDriveMode::Orbital: return 32.0;
	case EShipDriveMode::Interplanetary: return 256.0;
	case EShipDriveMode::Stellar: return 2048.0;
	case EShipDriveMode::Interstellar: return 8192.0;
	case EShipDriveMode::Landing:
	default: return 0.05;
	}
}

double ASpaceship::GetDriveAccelerationScale(EShipDriveMode DriveMode)
{
	switch (DriveMode)
	{
	case EShipDriveMode::Local: return 4.0;
	case EShipDriveMode::Orbital: return 192.0;
	case EShipDriveMode::Interplanetary: return 1024.0;
	case EShipDriveMode::Stellar: return 4096.0;
	case EShipDriveMode::Interstellar: return 16384.0;
	case EShipDriveMode::Landing:
	default: return 0.75;
	}
}

double ASpaceship::GetMinimumDriveAcceleration(EShipDriveMode DriveMode)
{
	switch (DriveMode)
	{
	case EShipDriveMode::Orbital: return 500000.0;
	case EShipDriveMode::Interplanetary: return 1500000.0;
	case EShipDriveMode::Stellar: return 6000000.0;
	case EShipDriveMode::Interstellar: return 24000000.0;
	case EShipDriveMode::Landing:
	case EShipDriveMode::Local:
	default: return 0.0;
	}
}

bool ASpaceship::CanUseEngineMode(EEngineMode EngineMode) const
{
	switch (EngineMode)
	{
	case EEngineMode::Impulse: return true;
	case EEngineMode::SpaceWrap: return ActiveClassPreset.bSupportsSpaceWrap;
	case EEngineMode::Offset: return ActiveClassPreset.bSupportsOffset;
	default: return false;
	}
}

double ASpaceship::GetEngineSpeedMultiplier(EEngineMode EngineMode)
{
	switch (EngineMode)
	{
	case EEngineMode::SpaceWrap: return 64.0;
	case EEngineMode::Offset: return 4096.0;
	case EEngineMode::Impulse:
	default: return 1.0;
	}
}

double ASpaceship::GetEngineAccelerationMultiplier(EEngineMode EngineMode)
{
	switch (EngineMode)
	{
	case EEngineMode::SpaceWrap: return 24.0;
	case EEngineMode::Offset: return 256.0;
	case EEngineMode::Impulse:
	default: return 1.0;
	}
}

EEngineMode ASpaceship::ResolveEngineModeForDriveMode(EShipDriveMode DriveMode) const
{
	return SelectedEngineMode;
}

EFlightMode ASpaceship::ResolveLegacyFlightModeForDriveMode(EShipDriveMode DriveMode) const
{
	switch (DriveMode)
	{
	case EShipDriveMode::Local: return EFlightMode::Basic;
	case EShipDriveMode::Orbital: return EFlightMode::Orbital;
	case EShipDriveMode::Interplanetary: return EFlightMode::Interplanetary;
	case EShipDriveMode::Stellar: return EFlightMode::Stellar;
	case EShipDriveMode::Interstellar: return EFlightMode::Interstellar;
	case EShipDriveMode::Landing:
	default: return EFlightMode::Surface;
	}
}

void ASpaceship::SetDriveMode(EShipDriveMode NewDriveMode, bool bImmediate)
{
	if (!OnboardComputer || !CanUseDriveMode(NewDriveMode))
	{
		return;
	}

	SelectedDriveMode = NewDriveMode;
	OnboardComputer->FlightSystem.CurrentFlightMode = ResolveLegacyFlightModeForDriveMode(NewDriveMode);
	OnboardComputer->ComputeFlightParams();
	// Power changes stay within the selected engine and therefore never masquerade as an engine transition.
	RequestEngineModeForFlightMode(true);
	CheckFlightModeChange();
}

void ASpaceship::SetEngineMode(EEngineMode NewEngineMode, bool bImmediate)
{
	if (!OnboardComputer || !CanUseEngineMode(NewEngineMode) || SelectedEngineMode == NewEngineMode)
	{
		return;
	}
	SelectedEngineMode = NewEngineMode;
	RequestEngineModeForFlightMode(bImmediate);
}

void ASpaceship::EnforceDriveModeForEnvironment()
{
	const uint8 Maximum = FMath::Min(
		static_cast<uint8>(GetMaximumDriveModeForClass()),
		static_cast<uint8>(GetMaximumDriveModeForEnvironment()));
	if (static_cast<uint8>(SelectedDriveMode) > Maximum)
	{
		SetDriveMode(static_cast<EShipDriveMode>(Maximum), false);
	}
}

bool ASpaceship::IsNearGravitySurface(const FVector& GravityDirection) const
{
	if (!GetWorld() || GravityDirection.IsNearlyZero())
	{
		return false;
	}

	const UPrimitiveComponent* MainMesh = GetPrimaryHullComponent();
	const double ProbeDistance = MainMesh
		? FMath::Max(static_cast<double>(MainMesh->Bounds.SphereRadius) * 1.25, 2000.0)
		: 2000.0;
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(APSShipSurfaceProbe), false, this);
	if (IsValid(Pilot))
	{
		QueryParams.AddIgnoredActor(Pilot);
	}
	FHitResult Hit;
	return GetWorld()->LineTraceSingleByChannel(
		Hit,
		GetActorLocation(),
		GetActorLocation() + GravityDirection.GetSafeNormal() * ProbeDistance,
		ECC_Visibility,
		QueryParams);
}

void ASpaceship::UpdateFlightEnvironment(float DeltaTime, bool bForce)
{
	EnvironmentDetectionElapsed += FMath::Max(DeltaTime, 0.0f);
	if (!bForce && EnvironmentDetectionElapsed < EnvironmentDetectionInterval)
	{
		return;
	}
	// A single long frame (for example while WorldScape allocates its first chunks)
	// must not satisfy the whole transition window by itself. Count at most one
	// detector interval so a new state still needs a repeated stable observation.
	const float DetectionSampleTime = FMath::Min(
		EnvironmentDetectionElapsed, FMath::Max(EnvironmentDetectionInterval, 0.01f));
	EnvironmentDetectionElapsed = 0.0f;

	AActor* DetectedGravitySource = nullptr;
	FVector DetectedGravityDirection = FVector::ZeroVector;
	if (FlightGravityDetector)
	{
		FlightGravityDetector->RunGravityCheckForActor(this);
		DetectedGravitySource = FlightGravityDetector->GravityTargetActor;
		DetectedGravityDirection = FlightGravityDetector->GetGravityDirectionAtLocation(GetActorLocation());
	}

	double DetectedGravityAcceleration = 0.0;
	EShipFlightEnvironment DetectedEnvironment = EShipFlightEnvironment::DeepSpace;
	if (DetectedGravitySource && !DetectedGravityDirection.IsNearlyZero())
	{
		DetectedGravityAcceleration = 980.0;
		if (const APlanetaryBody* Planet = Cast<APlanetaryBody>(DetectedGravitySource))
		{
			const double RadiusKm = Planet->RadiusKM > UE_SMALL_NUMBER
				? Planet->RadiusKM : FMath::Max(static_cast<double>(Planet->PlanetRadiusKM), 0.0);
			const double CenterDistanceKm = FVector::Distance(GetActorLocation(), Planet->GetActorLocation()) / 100000.0;
			const double AltitudeKm = FMath::Max(0.0, CenterDistanceKm - RadiusKm);
			const double AtmosphereHeightKm = Planet->AtmosphereHeight > UE_SMALL_NUMBER
				? Planet->AtmosphereHeight : FMath::Max(RadiusKm / 30.0, 1.0);
			const double SurfaceAcceleration = Planet->PlanetGravityStrength > UE_SMALL_NUMBER
				? Planet->PlanetGravityStrength * 980.0 : 980.0;
			const double DistanceFalloff = RadiusKm > UE_SMALL_NUMBER && CenterDistanceKm > RadiusKm
				? FMath::Square(RadiusKm / CenterDistanceKm) : 1.0;
			DetectedGravityAcceleration = FMath::Clamp(SurfaceAcceleration * DistanceFalloff, 5.0, 3000.0);

			if (IsNearGravitySurface(DetectedGravityDirection))
			{
				DetectedEnvironment = EShipFlightEnvironment::Surface;
			}
			else if (AltitudeKm <= AtmosphereHeightKm)
			{
				DetectedEnvironment = EShipFlightEnvironment::Atmosphere;
			}
			else
			{
				DetectedEnvironment = EShipFlightEnvironment::GravityWell;
			}
		}
		else
		{
			DetectedEnvironment = IsNearGravitySurface(DetectedGravityDirection)
				? EShipFlightEnvironment::Surface : EShipFlightEnvironment::GravityWell;
		}
	}

	const bool bMatchesActiveState = bFlightEnvironmentInitialized
		&& DetectedEnvironment == CurrentFlightEnvironment
		&& DetectedGravitySource == ActiveGravitySource.Get();
	bool bCommitDetectedState = bForce || !bFlightEnvironmentInitialized;
	if (!bCommitDetectedState && bMatchesActiveState)
	{
		// Direction and acceleration continuously change in a planetary field, but
		// the source and environment remain stable and do not need debouncing.
		ActiveGravityDirection = DetectedGravityDirection;
		ActiveGravityAcceleration = DetectedGravityAcceleration;
		PendingGravitySource.Reset();
		PendingEnvironmentTransitionElapsed = 0.0f;
	}
	else if (!bCommitDetectedState)
	{
		const bool bMatchesPendingState = DetectedEnvironment == PendingFlightEnvironment
			&& DetectedGravitySource == PendingGravitySource.Get();
		if (!bMatchesPendingState)
		{
			PendingFlightEnvironment = DetectedEnvironment;
			PendingGravitySource = DetectedGravitySource;
			PendingEnvironmentTransitionElapsed = 0.0f;
		}

		PendingEnvironmentTransitionElapsed += DetectionSampleTime;
		bCommitDetectedState = PendingEnvironmentTransitionElapsed
			>= EnvironmentTransitionConfirmationTime;
	}

	if (bCommitDetectedState)
	{
		const EShipFlightEnvironment PreviousEnvironment = CurrentFlightEnvironment;
		AActor* PreviousSource = ActiveGravitySource.Get();
		CurrentFlightEnvironment = DetectedEnvironment;
		ActiveGravitySource = DetectedGravitySource;
		ActiveGravityDirection = DetectedGravityDirection;
		ActiveGravityAcceleration = DetectedGravityAcceleration;
		bFlightEnvironmentInitialized = true;
		PendingGravitySource.Reset();
		PendingEnvironmentTransitionElapsed = 0.0f;

		if (PreviousEnvironment != CurrentFlightEnvironment || PreviousSource != ActiveGravitySource.Get())
		{
			EnforceDriveModeForEnvironment();
			UE_LOG(LogTemp, Log, TEXT("[APS.Ships] Environment ship=%s environment=%s gravity=%s acceleration=%.2f m/s2"),
				*GetName(), *GetFlightEnvironmentName(), *GetGravitySourceName(), ActiveGravityAcceleration / 100.0);
		}
	}
	if (OnboardComputer)
	{
		switch (CurrentFlightEnvironment)
		{
		case EShipFlightEnvironment::Surface:
			OnboardComputer->FlightSystem.CurrentFlightType = EFlightType::Landing;
			break;
		case EShipFlightEnvironment::Atmosphere:
			OnboardComputer->FlightSystem.CurrentFlightType = EFlightType::Atmospheric;
			break;
		case EShipFlightEnvironment::GravityWell:
			OnboardComputer->FlightSystem.CurrentFlightType = EFlightType::Orbital;
			break;
		case EShipFlightEnvironment::DeepSpace:
		default:
			OnboardComputer->FlightSystem.CurrentFlightType = EFlightType::ZeroG;
			break;
		}
	}

	if (SpaceshipHull && SpaceshipHull->IsSimulatingPhysics())
	{
		SpaceshipHull->SetLinearDamping(GetEnvironmentDrag());
	}
	UpdatePilotFillLightVisibility();
}

double ASpaceship::GetEnvironmentDrag() const
{
	switch (CurrentFlightEnvironment)
	{
	case EShipFlightEnvironment::Surface: return SurfaceDrag;
	case EShipFlightEnvironment::Atmosphere: return AtmosphericDrag;
	case EShipFlightEnvironment::GravityWell: return GravityWellDrag;
	case EShipFlightEnvironment::DeepSpace:
	default: return 0.0;
	}
}

void ASpaceship::ApplyEnvironmentForces(float DeltaTime)
{
	if (!bApplyExternalGravity || !ActiveGravitySource || ActiveGravityDirection.IsNearlyZero()
		|| ActiveGravityAcceleration <= UE_SMALL_NUMBER)
	{
		return;
	}
	if (bEngineRunning && bFlightAssistCompensatesGravity)
	{
		return;
	}

	const FVector GravityAcceleration = ActiveGravityDirection.GetSafeNormal() * ActiveGravityAcceleration;
	if (SpaceshipHull && SpaceshipHull->IsSimulatingPhysics())
	{
		SpaceshipHull->AddForce(GravityAcceleration, NAME_None, true);
	}
	else
	{
		KinematicVelocity += GravityAcceleration * DeltaTime;
	}
}

void ASpaceship::UpdateAdaptiveFlightCamera(float DeltaTime)
{
	if (!bUseAdaptiveFlightCamera || !SpringArmComponent || !IsValid(Pilot))
	{
		return;
	}
	const double Speed = SpaceshipHull && SpaceshipHull->IsSimulatingPhysics()
		? SpaceshipHull->GetPhysicsLinearVelocity().Size() : KinematicVelocity.Size();
	// Camera response is driven only by continuous physical speed. Using the selected
	// drive/engine limit here made the same velocity produce a different camera pose
	// immediately after every mode change. Speeds span metres to light years per second, so the
	// pose reads the logarithm of the speed (29.09: the old log2/8 mapping saturated at ~250 km/s
	// and left the camera still for every faster flight).
	const double ClassReferenceSpeed = FMath::Max(ActiveClassPreset.MaxImpulseSpeed, 1.0);
	const float TargetCameraAlpha = FMath::Clamp(static_cast<float>(
		FMath::LogX(10.0, 1.0 + Speed / ClassReferenceSpeed) / 7.0), 0.0f, 1.0f);
	SmoothedCameraSpeedAlpha = FMath::FInterpTo(
		SmoothedCameraSpeedAlpha, TargetCameraAlpha, DeltaTime, 1.2f);
	const float CameraAlpha = SmoothedCameraSpeedAlpha;

	// Acceleration feel at every scale: the rate of ln(speed), smoothed over about a second and only outward. Speeding
	// up lets the ship ease away from the camera; slowing down never pulls the camera in (Rio, 30.09: jerky).
	const double LogSpeed = FMath::Loge(FMath::Max(Speed, 100.0));
	const float LogSpeedRate = PreviousCameraLogSpeed < 0.0 || DeltaTime <= 0.0f ? 0.0f
		: FMath::Clamp(static_cast<float>((LogSpeed - PreviousCameraLogSpeed) / DeltaTime), -4.0f, 4.0f);
	PreviousCameraLogSpeed = LogSpeed;
	SmoothedLogSpeedRate = FMath::FInterpTo(SmoothedLogSpeedRate, LogSpeedRate, DeltaTime, 1.0f);
	const float Thrust = FMath::Clamp(SmoothedLogSpeedRate / 2.0f, 0.0f, 1.0f);

	// Hull bounds define the baseline, a third farther at rest than the hull alone asks for (Rio, 30.09: too close at
	// low speed). Speed and thrust ease the camera back on a critically damped follow (Game Programming Gems 4, 1.10)
	// that settles in about a second without overshoot: no kick on band shifts, no shake.
	const float TargetArmLength = BaseCameraArmLength * (1.3f + 0.30f * CameraAlpha + 0.12f * Thrust);
	if (DeltaTime > 0.0f)
	{
		constexpr float ArmSmoothSeconds = 0.9f;
		const float Omega = 2.0f / ArmSmoothSeconds;
		const float X = Omega * DeltaTime;
		const float Decay = 1.0f / (1.0f + X + 0.48f * X * X + 0.235f * X * X * X);
		const float Change = SpringArmComponent->TargetArmLength - TargetArmLength;
		const float Carried = (CameraArmLengthRate + Omega * Change) * DeltaTime;
		CameraArmLengthRate = (CameraArmLengthRate - Omega * Carried) * Decay;
		SpringArmComponent->TargetArmLength = TargetArmLength + (Change + Carried) * Decay;
	}
	// Positional lag is intentionally disabled. At astronomical velocities a
	// spring-arm positional integrator alternates between a huge error and a huge
	// correction, while smoothly interpolated arm length/FOV retain the chase feel.
	SpringArmComponent->bEnableCameraLag = false;
	SpringArmComponent->CameraRotationLagSpeed = FMath::Lerp(7.0f, 12.0f, CameraAlpha);
	SpringArmComponent->CameraLagMaxDistance = 0.0f;
	if (CameraComponent)
	{
		// Speed reads through arm length, vignette and bloom. A changing FOV invalidates the star optics every
		// frame (see aps.Ship.SpeedFov), so the widening is opt-in.
		const float SpeedFieldOfView = APSShipPerf::CVarSpeedFov.GetValueOnGameThread() != 0 ? CameraAlpha * 12.0f : 0.0f;
		const float TargetFieldOfView = BaseCameraFieldOfView + SpeedFieldOfView;
		CameraComponent->SetFieldOfView(FMath::FInterpTo(
			CameraComponent->FieldOfView, TargetFieldOfView, DeltaTime, 2.6f));

		FPostProcessSettings& PostProcess = CameraComponent->PostProcessSettings;
		PostProcess.bOverride_SceneFringeIntensity = true;
		PostProcess.bOverride_ChromaticAberrationStartOffset = true;
		PostProcess.bOverride_VignetteIntensity = true;
		PostProcess.bOverride_BloomIntensity = true;
		PostProcess.bOverride_AutoExposureBias = true;
		const float CinematicAlpha = FMath::Square(CameraAlpha);
		// Chromatic aberration splits every high-contrast HDR point into red/green/
		// blue copies. At interplanetary boost this turned the galaxy into a fringe
		// test chart and destroyed the physically authored spectral colour. Keep the
		// speed sensation in FOV, arm length, vignette and bloom; stellar light must
		// remain spectrally coherent at every velocity.
		PostProcess.SceneFringeIntensity = 0.0f;
		PostProcess.ChromaticAberrationStartOffset = 0.0f;
		// Sustained thrust deepens the vignette and bloom a little on top of the speed level.
		const float Pulse = 0.08f * Thrust;
		PostProcess.VignetteIntensity = FMath::FInterpTo(PostProcess.VignetteIntensity,
			FMath::Max(BaseVignetteIntensity, 0.16f + CinematicAlpha * 0.12f + Pulse), DeltaTime, 1.5f);
		PostProcess.BloomIntensity = FMath::FInterpTo(PostProcess.BloomIntensity,
			FMath::Max(BaseBloomIntensity, 0.45f + CinematicAlpha * 0.22f + Pulse), DeltaTime, 1.5f);
		// Exposure is intentionally speed-invariant; only the lens response changes with velocity.
		PostProcess.AutoExposureBias = BaseAutoExposureBias;
		CameraComponent->PostProcessBlendWeight = FMath::Max(BaseCameraPostProcessBlendWeight, 1.0f);
	}
}

void ASpaceship::InitializeFlightPostProcess()
{
	if (!CameraComponent || bCameraPostProcessInitialized)
	{
		return;
	}
	const FPostProcessSettings& PostProcess = CameraComponent->PostProcessSettings;
	BaseCameraPostProcessBlendWeight = CameraComponent->PostProcessBlendWeight;
	BaseSceneFringeIntensity = PostProcess.SceneFringeIntensity;
	BaseChromaticAberrationStartOffset = PostProcess.ChromaticAberrationStartOffset;
	BaseVignetteIntensity = PostProcess.VignetteIntensity;
	BaseBloomIntensity = PostProcess.BloomIntensity;
	BaseAutoExposureBias = PostProcess.AutoExposureBias;
	bBaseOverrideSceneFringe = PostProcess.bOverride_SceneFringeIntensity;
	bBaseOverrideChromaticStart = PostProcess.bOverride_ChromaticAberrationStartOffset;
	bBaseOverrideVignette = PostProcess.bOverride_VignetteIntensity;
	bBaseOverrideBloom = PostProcess.bOverride_BloomIntensity;
	bBaseOverrideExposureBias = PostProcess.bOverride_AutoExposureBias;
	bCameraPostProcessInitialized = true;
}

void ASpaceship::RestoreFlightPostProcess()
{
	if (!CameraComponent || !bCameraPostProcessInitialized)
	{
		return;
	}
	FPostProcessSettings& PostProcess = CameraComponent->PostProcessSettings;
	PostProcess.SceneFringeIntensity = BaseSceneFringeIntensity;
	PostProcess.ChromaticAberrationStartOffset = BaseChromaticAberrationStartOffset;
	PostProcess.VignetteIntensity = BaseVignetteIntensity;
	PostProcess.BloomIntensity = BaseBloomIntensity;
	PostProcess.AutoExposureBias = BaseAutoExposureBias;
	PostProcess.bOverride_SceneFringeIntensity = bBaseOverrideSceneFringe;
	PostProcess.bOverride_ChromaticAberrationStartOffset = bBaseOverrideChromaticStart;
	PostProcess.bOverride_VignetteIntensity = bBaseOverrideVignette;
	PostProcess.bOverride_BloomIntensity = bBaseOverrideBloom;
	PostProcess.bOverride_AutoExposureBias = bBaseOverrideExposureBias;
	CameraComponent->PostProcessBlendWeight = BaseCameraPostProcessBlendWeight;
}

void ASpaceship::SetHullSceneLightingExcluded(bool bExcluded)
{
	if (!bExcluded)
	{
		for (const FHullSceneLightingFlags& Flags : ExcludedHullSceneLighting)
		{
			if (UPrimitiveComponent* Component = Flags.Component.Get())
			{
				Component->SetAffectDistanceFieldLighting(Flags.bAffectDistanceField);
				Component->SetAffectDynamicIndirectLighting(Flags.bAffectIndirect);
			}
		}
		ExcludedHullSceneLighting.Reset();
		AppliedHullSceneLightingMode = 1;
		return;
	}
	const int32 Mode = APSShipPerf::CVarHullSceneLightingInFlight.GetValueOnGameThread();
	if (!ExcludedHullSceneLighting.IsEmpty() || Mode == 1)
	{
		return;
	}
	AppliedHullSceneLightingMode = Mode;
	const bool bLeaveDistanceField = Mode == 0 || Mode == 2;
	const bool bLeaveLumenScene = Mode == 0 || Mode == 3;
	TArray<AActor*> Actors{this};
	GetAttachedActors(Actors, false, true);
	for (const AActor* Actor : Actors)
	{
		// The pilot keeps its own flags; it is hidden while seated.
		if (!IsValid(Actor) || (Actor != this && Actor->IsA<APawn>()))
		{
			continue;
		}
		TArray<UPrimitiveComponent*> Components;
		Actor->GetComponents(Components);
		for (UPrimitiveComponent* Component : Components)
		{
			if (!IsValid(Component) || (!Component->bAffectDistanceFieldLighting && !Component->bAffectDynamicIndirectLighting))
			{
				continue;
			}
			ExcludedHullSceneLighting.Add({Component, Component->bAffectDistanceFieldLighting != 0,
				Component->bAffectDynamicIndirectLighting != 0});
			if (bLeaveDistanceField)
			{
				Component->SetAffectDistanceFieldLighting(false);
			}
			if (bLeaveLumenScene)
			{
				Component->SetAffectDynamicIndirectLighting(false);
			}
		}
	}
}

void ASpaceship::StabilizeFullScaleVisualVelocity()
{
	if (!IsValid(Pilot) || (SpaceshipHull && SpaceshipHull->IsSimulatingPhysics())
		|| APSShipPerf::CVarResetHullVelocity.GetValueOnGameThread() == 0)
	{
		return;
	}
	TArray<UPrimitiveComponent*> PrimitiveComponents;
	GetComponents(PrimitiveComponents);
	for (UPrimitiveComponent* Component : PrimitiveComponents)
	{
		if (IsValid(Component) && Component->IsVisible())
		{
			// The camera shares the kinematic full-scale translation. Treating that offset as
			// local object motion produces enormous invalid temporal velocities on the hull.
			Component->ResetSceneVelocity();
		}
	}
}

void ASpaceship::SetFlightCollisionOptimization(bool bEnabled)
{
	UPrimitiveComponent* PrimaryHull = GetPrimaryHullComponent();
	if (!bOptimizeCollisionWhilePiloted || bGenerateSimpleHullCollision || !PrimaryHull)
	{
		return;
	}

	if (bEnabled && !bFlightCollisionOptimizationActive)
	{
		OriginalHullCollisionProfile = PrimaryHull->GetCollisionProfileName();
		OriginalHullCollisionEnabled = PrimaryHull->GetCollisionEnabled();
		OriginalHullCollisionResponses = PrimaryHull->GetCollisionResponseToChannels();
		bOriginalHullSimulatesPhysics = PrimaryHull->IsSimulatingPhysics();
		bGenerateSimpleHullCollision = true;
		bBuildingFlightCollisionProxy = true;
		RebuildSimpleHullCollision();
		bBuildingFlightCollisionProxy = false;
		bGenerateSimpleHullCollision = false;
		ActiveClassPreset.bUsesPhysicalImpulse = false;
		bFlightCollisionOptimizationActive = true;
		ApplyEngineState();
	}
	else if (!bEnabled && bFlightCollisionOptimizationActive)
	{
		for (UBoxComponent* Box : GeneratedCollisionBoxes)
		{
			if (IsValid(Box))
			{
				Box->DestroyComponent();
			}
		}
		GeneratedCollisionBoxes.Reset();
		PrimaryHull->SetCollisionProfileName(OriginalHullCollisionProfile);
		PrimaryHull->SetCollisionEnabled(OriginalHullCollisionEnabled);
		PrimaryHull->SetCollisionResponseToChannels(OriginalHullCollisionResponses);
		PrimaryHull->SetSimulatePhysics(false);
		bFlightCollisionOptimizationActive = false;
		ConfigureFromHull();
		ApplyEngineState();
	}
}

FVector ASpaceship::GetControlledFlightAcceleration(const FVector& WorldInput,
	const FVector& CurrentVelocity, double RequestedAcceleration, double MaximumSpeed) const
{
	if (WorldInput.IsNearlyZero() || RequestedAcceleration <= UE_DOUBLE_SMALL_NUMBER)
	{
		return FVector::ZeroVector;
	}

	const FVector RawAcceleration = WorldInput.GetClampedToMaxSize(1.0) * RequestedAcceleration;
	const double CurrentSpeed = CurrentVelocity.Size();
	const double DriveReferenceScale = FMath::Clamp(
		GetDriveSpeedScale(SelectedDriveMode), 1.0, 64.0);
	const double ReferenceSpeed = FMath::Max(
		CurrentSpeed, ActiveClassPreset.MaxImpulseSpeed * DriveReferenceScale);
	const double MaximumParallelAcceleration = ReferenceSpeed
		/ FMath::Max(static_cast<double>(HighSpeedVelocityResponseTime), 0.1);

	if (CurrentSpeed <= 1.0)
	{
		return RawAcceleration.GetClampedToMaxSize(MaximumParallelAcceleration);
	}

	const FVector VelocityDirection = CurrentVelocity / CurrentSpeed;
	double ParallelAcceleration = FVector::DotProduct(RawAcceleration, VelocityDirection);
	FVector LateralAcceleration = RawAcceleration - VelocityDirection * ParallelAcceleration;
	ParallelAcceleration = FMath::Clamp(
		ParallelAcceleration, -MaximumParallelAcceleration, MaximumParallelAcceleration);
	if (CurrentSpeed >= MaximumSpeed && ParallelAcceleration > 0.0)
	{
		ParallelAcceleration = 0.0;
	}

	const double MaximumHeadingRateRadians = FMath::DegreesToRadians(FMath::Max(
		ActiveClassPreset.RotationSpeed * SteeringRateScale * HighSpeedHeadingResponseScale, 1.0));
	const double MaximumLateralAcceleration = ReferenceSpeed * MaximumHeadingRateRadians;
	LateralAcceleration = LateralAcceleration.GetClampedToMaxSize(MaximumLateralAcceleration);
	return VelocityDirection * ParallelAcceleration + LateralAcceleration;
}

bool ASpaceship::MoveShipKinematic(const FVector& Delta, bool bSweep, FHitResult& OutHit)
{
	OutHit = FHitResult();
	if (Delta.IsNearlyZero())
	{
		return false;
	}
	UPrimitiveComponent* RootPrimitive = Cast<UPrimitiveComponent>(GetRootComponent());
	if (bSweep && RootPrimitive && !RootPrimitive->IsQueryCollisionEnabled() && !GeneratedCollisionBoxes.IsEmpty()
		&& APSShipPerf::CVarProxySweep.GetValueOnGameThread() != 0)
	{
		// Detailed hulls fly on child proxy boxes with a collision-free root (SetFlightCollisionOptimization);
		// the root sweep below would never hit anything, so a piloted M3 passed through stations and ground.
		return MoveShipWithProxySweep(Delta, OutHit);
	}
	if (bSweep && (!RootPrimitive || !RootPrimitive->IsQueryCollisionEnabled()))
	{
		// MoveComponent sweeps only the root body; without query collision the move is a plain offset anyway.
		bSweep = false;
	}
	if (bSweep && APSShipPerf::CVarSweepPrecheck.GetValueOnGameThread() != 0 && GetWorld())
	{
		// The body sweep queries every convex hull of the root separately (12-32 on generated hulls) and its
		// cost grows with the swept length. A sphere around the body's bounding box encloses all of them, so a
		// clear sphere path with the same channel, responses and ignore lists proves the body sweep clear too.
		const FBox BodyBox = RootPrimitive->BodyInstance.GetBodyBounds();
		const FVector SphereCenter = BodyBox.IsValid ? BodyBox.GetCenter() : RootPrimitive->Bounds.Origin;
		const double SphereRadius = BodyBox.IsValid ? BodyBox.GetExtent().Size() : RootPrimitive->Bounds.BoxExtent.Size();
		FCollisionQueryParams Params(SCENE_QUERY_STAT(APSShipSweepPrecheck), RootPrimitive->bTraceComplexOnMove, this);
		Params.AddIgnoredActors(RootPrimitive->GetMoveIgnoreActors());
		Params.AddIgnoredComponents(RootPrimitive->GetMoveIgnoreComponents());
		if (IsValid(Pilot))
		{
			Params.AddIgnoredActor(Pilot);
		}
		bSweep = GetWorld()->SweepTestByChannel(SphereCenter, SphereCenter + Delta, FQuat::Identity,
			RootPrimitive->GetCollisionObjectType(), FCollisionShape::MakeSphere(SphereRadius), Params,
			FCollisionResponseParams(RootPrimitive->GetCollisionResponseToChannels()));
	}
	AddActorWorldOffset(Delta, bSweep, &OutHit, ETeleportType::None);
	return OutHit.bBlockingHit;
}

bool ASpaceship::MoveShipWithProxySweep(const FVector& Delta, FHitResult& OutHit)
{
	UWorld* World = GetWorld();
	TArray<UBoxComponent*, TInlineAllocator<9>> Boxes;
	FBox ProxyBounds(ForceInit);
	for (UBoxComponent* Box : GeneratedCollisionBoxes)
	{
		if (IsValid(Box) && Box->IsQueryCollisionEnabled())
		{
			Boxes.Add(Box);
			ProxyBounds += Box->Bounds.GetBox();
		}
	}
	if (!World || Boxes.IsEmpty() || !ProxyBounds.IsValid)
	{
		AddActorWorldOffset(Delta, false, nullptr, ETeleportType::None);
		return false;
	}
	FCollisionQueryParams Params(SCENE_QUERY_STAT(APSShipProxySweep), false, this);
	TArray<AActor*> Attached;
	GetAttachedActors(Attached, false, true);
	Params.AddIgnoredActors(Attached);
	if (IsValid(Pilot))
	{
		Params.AddIgnoredActor(Pilot);
	}
	const ECollisionChannel Channel = Boxes[0]->GetCollisionObjectType();
	const FCollisionResponseParams Responses(Boxes[0]->GetCollisionResponseToChannels());

	// Nearly every frame of flight is clear: one sphere around all boxes proves that cheaply.
	const FVector Center = ProxyBounds.GetCenter();
	if (!World->SweepTestByChannel(Center, Center + Delta, FQuat::Identity, Channel,
		FCollisionShape::MakeSphere(ProxyBounds.GetExtent().Size()), Params, Responses))
	{
		AddActorWorldOffset(Delta, false, nullptr, ETeleportType::None);
		return false;
	}
	FHitResult Earliest;
	bool bBlocked = false;
	for (const UBoxComponent* Box : Boxes)
	{
		const FVector Start = Box->GetComponentLocation();
		TArray<FHitResult> Hits;
		World->SweepMultiByChannel(Hits, Start, Start + Delta, Box->GetComponentQuat(), Channel,
			FCollisionShape::MakeBox(Box->GetScaledBoxExtent()), Params, Responses);
		for (const FHitResult& Hit : Hits)
		{
			// A contact the ship is already leaving (parked on a deck, grazing a wall) must not hold it.
			if (!Hit.bBlockingHit || (Hit.bStartPenetrating && FVector::DotProduct(Delta, Hit.ImpactNormal) >= 0.0))
			{
				continue;
			}
			if (!bBlocked || Hit.Time < Earliest.Time)
			{
				Earliest = Hit;
				bBlocked = true;
			}
		}
	}
	if (!bBlocked)
	{
		AddActorWorldOffset(Delta, false, nullptr, ETeleportType::None);
		return false;
	}
	// Up to the contact with a centimetre to spare; the caller removes the velocity into the surface.
	const double Travel = FMath::Max(Earliest.Time * Delta.Size() - 1.0, 0.0);
	if (Travel > 0.0)
	{
		AddActorWorldOffset(Delta.GetSafeNormal() * Travel, false, nullptr, ETeleportType::None);
	}
	OutHit = Earliest;
	return true;
}

void ASpaceship::ApplyFlightInput(float DeltaTime)
{
	if (!bEngineRunning || !OnboardComputer || !SpaceshipHull)
	{
		return;
	}

	const FVector LocalInput(ForwardInput, SideInput, VerticalInput);
	const FVector ClampedInput = LocalInput.GetClampedToMaxSize(1.0);
	const FVector WorldInput = GetShipForwardVector() * ClampedInput.X
		+ GetShipRightVector() * ClampedInput.Y
		+ GetShipUpVector() * ClampedInput.Z;
	const EEngineMode EngineMode = OnboardComputer->EngineSystem.CurrentEngineMode;
	const double SpeedScale = GetDriveSpeedScale(SelectedDriveMode) * GetEngineSpeedMultiplier(EngineMode);
	const double AccelerationScale = GetDriveAccelerationScale(SelectedDriveMode)
		* GetEngineAccelerationMultiplier(EngineMode);
	const double DriveAcceleration = FMath::Max(
		ActiveClassPreset.ImpulseAcceleration * AccelerationScale,
		GetMinimumDriveAcceleration(SelectedDriveMode) * GetEngineAccelerationMultiplier(EngineMode));
	const double TransitionAlpha = GetEngineTransitionAuthority();
	const double RequestedAcceleration = DriveAcceleration * CurrentBoostMultiplier * TransitionAlpha;
	const double MaxSpeed = ActiveClassPreset.MaxImpulseSpeed * SpeedScale * CurrentBoostMultiplier;

	if (EngineMode == EEngineMode::Impulse && ActiveClassPreset.bUsesPhysicalImpulse
		&& SpaceshipHull->IsSimulatingPhysics())
	{
		const FVector VelocityBeforeForces = SpaceshipHull->GetPhysicsLinearVelocity();
		const double SpeedBeforeForces = VelocityBeforeForces.Size();
		const FVector ControlledAcceleration = GetControlledFlightAcceleration(
			WorldInput, VelocityBeforeForces, RequestedAcceleration, MaxSpeed);
		if (!ControlledAcceleration.IsNearlyZero())
		{
			SpaceshipHull->AddForce(ControlledAcceleration, NAME_None, true);
		}
		ApplyEnvironmentForces(DeltaTime);

		FVector Velocity = SpaceshipHull->GetPhysicsLinearVelocity();
		const double SpeedAfterForces = Velocity.Size();
		if (SpeedAfterForces > MaxSpeed && SpeedAfterForces > SpeedBeforeForces)
		{
			// Boost and mode changes alter available thrust, not existing momentum.
			// Clamp only newly added overspeed so releasing Shift cannot delete most
			// of the ship velocity in a single frame.
			Velocity = Velocity.GetClampedToMaxSize(FMath::Max(MaxSpeed, SpeedBeforeForces));
			SpaceshipHull->SetPhysicsLinearVelocity(Velocity);
		}
		if (bIsDecelerating)
		{
			SpaceshipHull->SetPhysicsLinearVelocity(FMath::VInterpTo(
				Velocity, FVector::ZeroVector, DeltaTime, BrakingResponseSpeed));
		}
		return;
	}

	const double SpeedBeforeAcceleration = KinematicVelocity.Size();
	if (!WorldInput.IsNearlyZero())
	{
		KinematicVelocity += GetControlledFlightAcceleration(
			WorldInput, KinematicVelocity, RequestedAcceleration, MaxSpeed) * DeltaTime;
	}
	else
	{
		const double EnvironmentalDrag = GetEnvironmentDrag();
		if (EnvironmentalDrag > UE_SMALL_NUMBER)
		{
			KinematicVelocity = FMath::VInterpTo(
				KinematicVelocity, FVector::ZeroVector, DeltaTime, EnvironmentalDrag);
		}
	}
	ApplyEnvironmentForces(DeltaTime);
	const double SpeedAfterAcceleration = KinematicVelocity.Size();
	if (SpeedAfterAcceleration > MaxSpeed && SpeedAfterAcceleration > SpeedBeforeAcceleration)
	{
		KinematicVelocity = KinematicVelocity.GetClampedToMaxSize(
			FMath::Max(MaxSpeed, SpeedBeforeAcceleration));
	}
	if (bIsDecelerating)
	{
		KinematicVelocity = FMath::VInterpTo(
			KinematicVelocity, FVector::ZeroVector, DeltaTime, BrakingResponseSpeed);
	}

	FHitResult Hit;
	const bool bSweep = EngineMode == EEngineMode::Impulse;
	MoveShipKinematic(KinematicVelocity * DeltaTime, bSweep, Hit);
	if (Hit.bBlockingHit)
	{
		// A grazing contact must not consume the ship's whole impulse. Multiplying the
		// projected velocity by 0.25 made a held thrust repeatedly accelerate the ship
		// and then discard 75% of its speed on the next sweep, which was perceived as
		// a regular stop/go pulse even while flying straight. Remove only the velocity
		// directed into the obstacle and preserve both tangential and separating motion.
		const FVector SurfaceNormal = Hit.ImpactNormal.GetSafeNormal();
		const double VelocityIntoSurface = FVector::DotProduct(KinematicVelocity, SurfaceNormal);
		if (!SurfaceNormal.IsNearlyZero() && VelocityIntoSurface < 0.0)
		{
			KinematicVelocity -= SurfaceNormal * VelocityIntoSurface;
		}
	}
}

void ASpaceship::ApplyRotationInput(float DeltaTime)
{
	if (!bEngineRunning || !SpaceshipHull)
	{
		CurrentAngularVelocityDegrees = FVector::ZeroVector;
		return;
	}

	const float TransitionAlpha = GetEngineTransitionAuthority();
	const double RotationSpeed = ActiveClassPreset.RotationSpeed * SteeringRateScale * TransitionAlpha;
	const double SafeInputLimit = FMath::Max(static_cast<double>(SteeringInputLimit), 0.05);
	const FVector DesiredAngularVelocityDegrees(
		FMath::Clamp(static_cast<double>(PitchInput) / SafeInputLimit, -1.0, 1.0) * RotationSpeed,
		FMath::Clamp(static_cast<double>(YawInput) / SafeInputLimit, -1.0, 1.0) * RotationSpeed,
		FMath::Clamp(static_cast<double>(RollInput) / SafeInputLimit, -1.0, 1.0) * RotationSpeed);
	const bool bHasRotationInput = !DesiredAngularVelocityDegrees.IsNearlyZero(0.001);

	const bool bUsePhysicalRotation = OnboardComputer
		&& OnboardComputer->EngineSystem.CurrentEngineMode == EEngineMode::Impulse
		&& ActiveClassPreset.bUsesPhysicalImpulse
		&& SpaceshipHull->IsSimulatingPhysics();
	if (bUsePhysicalRotation)
	{
		const FVector ShipRight = GetShipRightVector();
		const FVector ShipUp = GetShipUpVector();
		const FVector ShipForward = GetShipForwardVector();
		const FVector WorldAngularVelocityRadians = SpaceshipHull->GetPhysicsAngularVelocityInRadians();
		const FVector MeasuredAngularVelocityDegrees(
			FMath::RadiansToDegrees(FVector::DotProduct(WorldAngularVelocityRadians, ShipRight)),
			FMath::RadiansToDegrees(FVector::DotProduct(WorldAngularVelocityRadians, ShipUp)),
			FMath::RadiansToDegrees(FVector::DotProduct(WorldAngularVelocityRadians, ShipForward)));
		if (!bHasRotationInput)
		{
			CurrentAngularVelocityDegrees = MeasuredAngularVelocityDegrees;
			return;
		}
		const FVector CommandedAngularVelocityDegrees = FMath::VInterpConstantTo(
			MeasuredAngularVelocityDegrees,
			DesiredAngularVelocityDegrees,
			DeltaTime,
			ActiveClassPreset.AngularAcceleration * TransitionAlpha);

		if (DeltaTime > UE_SMALL_NUMBER)
		{
			const FVector LocalAngularAccelerationDegrees =
				(CommandedAngularVelocityDegrees - MeasuredAngularVelocityDegrees) / DeltaTime;
			const FVector WorldAngularAccelerationRadians =
				ShipRight * FMath::DegreesToRadians(LocalAngularAccelerationDegrees.X)
				+ ShipUp * FMath::DegreesToRadians(LocalAngularAccelerationDegrees.Y)
				+ ShipForward * FMath::DegreesToRadians(LocalAngularAccelerationDegrees.Z);
			SpaceshipHull->AddTorqueInRadians(WorldAngularAccelerationRadians, NAME_None, true);
		}
		CurrentAngularVelocityDegrees = CommandedAngularVelocityDegrees;
		return;
	}

	CurrentAngularVelocityDegrees = bHasRotationInput
		? FMath::VInterpConstantTo(
			CurrentAngularVelocityDegrees,
			DesiredAngularVelocityDegrees,
			DeltaTime,
			ActiveClassPreset.AngularAcceleration * TransitionAlpha)
		: FMath::VInterpTo(
			CurrentAngularVelocityDegrees, FVector::ZeroVector, DeltaTime, PassiveAngularDamping);

	const double PitchRadians = FMath::DegreesToRadians(CurrentAngularVelocityDegrees.X * DeltaTime);
	const double YawRadians = FMath::DegreesToRadians(CurrentAngularVelocityDegrees.Y * DeltaTime);
	const double RollRadians = FMath::DegreesToRadians(CurrentAngularVelocityDegrees.Z * DeltaTime);
	if (FMath::IsNearlyZero(YawRadians) && FMath::IsNearlyZero(PitchRadians) && FMath::IsNearlyZero(RollRadians))
	{
		return;
	}

	const FQuat Current = GetActorQuat();
	const FQuat DeltaYaw(GetShipUpVector(), YawRadians);
	const FQuat DeltaPitch(GetShipRightVector(), PitchRadians);
	const FQuat DeltaRoll(GetShipForwardVector(), RollRadians);
	const FQuat Target = (DeltaRoll * DeltaPitch * DeltaYaw * Current).GetNormalized();
	SetActorRotation(Target, ETeleportType::None);
}

float ASpaceship::GetEngineTransitionAuthority() const
{
	if (!bEngineModeTransitionActive || EngineModeTransitionDuration <= UE_SMALL_NUMBER)
	{
		return 1.0f;
	}

	const float LinearAlpha = FMath::Clamp(
		EngineModeTransitionElapsed / EngineModeTransitionDuration, 0.0f, 1.0f);
	const float SmoothAlpha = LinearAlpha * LinearAlpha * (3.0f - 2.0f * LinearAlpha);
	return 1.0f - FMath::Sin(PI * SmoothAlpha) * 0.18f;
}

EEngineMode ASpaceship::ResolveEngineModeForFlightMode(EFlightMode FlightMode) const
{
	if (static_cast<uint8>(FlightMode) >= static_cast<uint8>(EFlightMode::Interstellar)
		&& ActiveClassPreset.bSupportsOffset)
	{
		return EEngineMode::Offset;
	}
	if (static_cast<uint8>(FlightMode) >= static_cast<uint8>(EFlightMode::Interplanetary)
		&& ActiveClassPreset.bSupportsSpaceWrap)
	{
		return EEngineMode::SpaceWrap;
	}
	return EEngineMode::Impulse;
}

double ASpaceship::GetFlightModeSpeedScale(EFlightMode FlightMode) const
{
	switch (FlightMode)
	{
	case EFlightMode::Surface: return 1.25;
	case EFlightMode::Atmospheric: return 2.0;
	case EFlightMode::Orbital: return 4.0;
	case EFlightMode::Planetary: return 8.0;
	case EFlightMode::Interplanetary: return 200.0;
	case EFlightMode::Stellar: return 1000.0;
	case EFlightMode::Interstellar: return 10000.0;
	case EFlightMode::Intergalaxy: return 100000.0;
	default: return 1.0;
	}
}

void ASpaceship::RequestEngineModeForFlightMode(bool bImmediate)
{
	if (!OnboardComputer)
	{
		return;
	}
	PendingEngineMode = ResolveEngineModeForDriveMode(SelectedDriveMode);
	if (OnboardComputer->EngineSystem.CurrentEngineMode == PendingEngineMode)
	{
		bEngineModeTransitionActive = false;
		bEngineModeSwitchedAtMidpoint = false;
		ApplyEngineState();
		return;
	}
	if (bImmediate || !bEngineRunning)
	{
		bEngineModeTransitionActive = false;
		bEngineModeSwitchedAtMidpoint = false;
		CommitEngineModeSwitch(PendingEngineMode);
		return;
	}

	EngineModeTransitionElapsed = 0.0f;
	bEngineModeTransitionActive = true;
	bEngineModeSwitchedAtMidpoint = false;
}

void ASpaceship::CommitEngineModeSwitch(EEngineMode NewEngineMode)
{
	if (!OnboardComputer || !SpaceshipHull)
	{
		return;
	}

	const EEngineMode PreviousMode = OnboardComputer->EngineSystem.CurrentEngineMode;
	const FVector PreservedLinearVelocity = SpaceshipHull->IsSimulatingPhysics()
		? SpaceshipHull->GetPhysicsLinearVelocity() : KinematicVelocity;
	FVector PreservedAngularVelocityRadians = FVector::ZeroVector;
	if (SpaceshipHull->IsSimulatingPhysics())
	{
		PreservedAngularVelocityRadians = SpaceshipHull->GetPhysicsAngularVelocityInRadians();
	}
	else
	{
		PreservedAngularVelocityRadians =
			GetShipRightVector() * FMath::DegreesToRadians(CurrentAngularVelocityDegrees.X)
			+ GetShipUpVector() * FMath::DegreesToRadians(CurrentAngularVelocityDegrees.Y)
			+ GetShipForwardVector() * FMath::DegreesToRadians(CurrentAngularVelocityDegrees.Z);
	}

	// The onboard computer changes the simulation backend. Capture momentum before
	// that happens, then explicitly seed whichever backend owns the next frame.
	KinematicVelocity = PreservedLinearVelocity;
	OnboardComputer->SwitchEngineMode(NewEngineMode);
	ApplyEngineState();

	if (SpaceshipHull->IsSimulatingPhysics())
	{
		SpaceshipHull->SetPhysicsLinearVelocity(PreservedLinearVelocity);
		SpaceshipHull->SetPhysicsAngularVelocityInRadians(PreservedAngularVelocityRadians);
	}
	else
	{
		KinematicVelocity = PreservedLinearVelocity;
		CurrentAngularVelocityDegrees = FVector(
			FMath::RadiansToDegrees(FVector::DotProduct(PreservedAngularVelocityRadians, GetShipRightVector())),
			FMath::RadiansToDegrees(FVector::DotProduct(PreservedAngularVelocityRadians, GetShipUpVector())),
			FMath::RadiansToDegrees(FVector::DotProduct(PreservedAngularVelocityRadians, GetShipForwardVector())));
	}

	UE_LOG(LogTemp, Log,
		TEXT("[APS.Ships] Engine handoff ship=%s from=%s to=%s speedBefore=%.2f m/s speedAfter=%.2f m/s physics=%s"),
		*GetName(), *UEnum::GetValueAsString(PreviousMode), *UEnum::GetValueAsString(NewEngineMode),
		PreservedLinearVelocity.Size() / 100.0, GetShipSpeedMetersPerSecond(),
		SpaceshipHull->IsSimulatingPhysics() ? TEXT("true") : TEXT("false"));
}

void ASpaceship::AdvanceEngineModeTransition(float DeltaTime)
{
	if (!bEngineModeTransitionActive || !OnboardComputer)
	{
		return;
	}
	EngineModeTransitionElapsed += DeltaTime;
	if (!bEngineModeSwitchedAtMidpoint && EngineModeTransitionElapsed >= EngineModeTransitionDuration * 0.5f)
	{
		CommitEngineModeSwitch(PendingEngineMode);
		bEngineModeSwitchedAtMidpoint = true;
	}
	if (EngineModeTransitionElapsed >= EngineModeTransitionDuration)
	{
		bEngineModeTransitionActive = false;
		bEngineModeSwitchedAtMidpoint = false;
		EngineModeTransitionElapsed = 0.0f;
	}
}

void ASpaceship::ApplyEngineState()
{
	if (!SpaceshipHull || !OnboardComputer)
	{
		return;
	}

	SpaceshipHull->SetMobility(EComponentMobility::Movable);
	SpaceshipHull->SetEnableGravity(false);
	if (!bEngineRunning)
	{
		if (SpaceshipHull->IsSimulatingPhysics())
		{
			KinematicVelocity = SpaceshipHull->GetPhysicsLinearVelocity();
			SpaceshipHull->SetPhysicsLinearVelocity(FVector::ZeroVector);
			SpaceshipHull->SetPhysicsAngularVelocityInRadians(FVector::ZeroVector);
		}
		SpaceshipHull->SetSimulatePhysics(false);
		KinematicVelocity = FVector::ZeroVector;
		CurrentAngularVelocityDegrees = FVector::ZeroVector;
		return;
	}

	const bool bUsePhysics = OnboardComputer->EngineSystem.CurrentEngineMode == EEngineMode::Impulse
		&& ActiveClassPreset.bUsesPhysicalImpulse;
	if (bUsePhysics)
	{
		SpaceshipHull->SetSimulatePhysics(true);
		SpaceshipHull->SetLinearDamping(GetEnvironmentDrag());
		SpaceshipHull->SetAngularDamping(PassiveAngularDamping);
		if (!KinematicVelocity.IsNearlyZero())
		{
			SpaceshipHull->SetPhysicsLinearVelocity(KinematicVelocity);
		}
		if (!CurrentAngularVelocityDegrees.IsNearlyZero())
		{
			const FVector WorldAngularVelocityRadians =
				GetShipRightVector() * FMath::DegreesToRadians(CurrentAngularVelocityDegrees.X)
				+ GetShipUpVector() * FMath::DegreesToRadians(CurrentAngularVelocityDegrees.Y)
				+ GetShipForwardVector() * FMath::DegreesToRadians(CurrentAngularVelocityDegrees.Z);
			SpaceshipHull->SetPhysicsAngularVelocityInRadians(WorldAngularVelocityRadians);
		}
	}
	else
	{
		if (SpaceshipHull->IsSimulatingPhysics())
		{
			KinematicVelocity = SpaceshipHull->GetPhysicsLinearVelocity();
			const FVector WorldAngularVelocityRadians = SpaceshipHull->GetPhysicsAngularVelocityInRadians();
			CurrentAngularVelocityDegrees = FVector(
				FMath::RadiansToDegrees(FVector::DotProduct(WorldAngularVelocityRadians, GetShipRightVector())),
				FMath::RadiansToDegrees(FVector::DotProduct(WorldAngularVelocityRadians, GetShipUpVector())),
				FMath::RadiansToDegrees(FVector::DotProduct(WorldAngularVelocityRadians, GetShipForwardVector())));
		}
		SpaceshipHull->SetSimulatePhysics(false);
	}
}

FString ASpaceship::GetSizeClassName() const
{
	if (const UEnum* Enum = StaticEnum<ESpaceshipSizeClass>())
	{
		return Enum->GetDisplayNameTextByValue(static_cast<int64>(SizeClass)).ToString();
	}
	return TEXT("M");
}

FString ASpaceship::GetFlightModeName() const
{
	return GetDriveModeName();
}

FString ASpaceship::GetDriveModeName() const
{
	if (SelectedDriveMode == EShipDriveMode::Orbital
		&& CurrentFlightEnvironment == EShipFlightEnvironment::Atmosphere)
	{
		return TEXT("EXIT ATMOSPHERE");
	}

	const int32 PowerIndex = FMath::Clamp(static_cast<int32>(SelectedDriveMode), 0, 5);
	static const TCHAR* ImpulseLabels[] = {
		TEXT("LANDING"), TEXT("LOCAL"), TEXT("ORBITAL"), TEXT("PLANETARY TRANSFER"),
		TEXT("INTERPLANETARY"), TEXT("RAPID INTERPLANETARY")
	};
	static const TCHAR* SpaceWrapLabels[] = {
		TEXT("PLANETARY ENTRY"), TEXT("LOCAL WARP"), TEXT("SYSTEM TRANSFER"), TEXT("SYSTEM CRUISE"),
		TEXT("STELLAR APPROACH"), TEXT("STELLAR TRANSFER")
	};
	static const TCHAR* OffsetLabels[] = {
		TEXT("LOCAL OFFSET"), TEXT("PLANETARY OFFSET"), TEXT("SYSTEM OFFSET"), TEXT("STELLAR OFFSET"),
		TEXT("INTERSTELLAR OFFSET"), TEXT("DEEP OFFSET")
	};
	if (SelectedEngineMode == EEngineMode::SpaceWrap)
	{
		return SpaceWrapLabels[PowerIndex];
	}
	if (SelectedEngineMode == EEngineMode::Offset)
	{
		return OffsetLabels[PowerIndex];
	}
	return ImpulseLabels[PowerIndex];
}

FString ASpaceship::GetFlightEnvironmentName() const
{
	if (const UEnum* Enum = StaticEnum<EShipFlightEnvironment>())
	{
		return Enum->GetDisplayNameTextByValue(static_cast<int64>(CurrentFlightEnvironment)).ToString().ToUpper();
	}
	return TEXT("DEEP SPACE");
}

FString ASpaceship::GetGravitySourceName() const
{
	if (!ActiveGravitySource)
	{
		return TEXT("NONE");
	}
	FString SourceName = ActiveGravitySource->GetName();
	SourceName.RemoveFromStart(TEXT("BP_"));
	const int32 ClassMarker = SourceName.Find(TEXT("_C_"));
	if (ClassMarker != INDEX_NONE)
	{
		SourceName.LeftInline(ClassMarker);
	}
	SourceName.ReplaceInline(TEXT("_"), TEXT(" "));
	return SourceName.ToUpper();
}

FString ASpaceship::GetEngineModeName() const
{
	if (!bEngineRunning) return TEXT("OFF / PARKED");
	if (bEngineModeTransitionActive)
	{
		const UEnum* EngineEnum = StaticEnum<EEngineMode>();
		const FString PendingName = EngineEnum
			? EngineEnum->GetDisplayNameTextByValue(static_cast<int64>(PendingEngineMode)).ToString().ToUpper()
			: TEXT("ENGINE");
		return FString::Printf(TEXT("TRANSITION -> %s"), *PendingName);
	}
	if (!OnboardComputer) return TEXT("UNKNOWN");
	switch (OnboardComputer->EngineSystem.CurrentEngineMode)
	{
	case EEngineMode::Impulse: return TEXT("IMPULSE");
	case EEngineMode::SpaceWrap: return TEXT("SPACE WRAP");
	case EEngineMode::Offset: return TEXT("OFFSET");
	default: return TEXT("UNKNOWN");
	}
}

double ASpaceship::GetShipSpeedMetersPerSecond() const
{
	const FVector Velocity = SpaceshipHull && SpaceshipHull->IsSimulatingPhysics()
		? SpaceshipHull->GetPhysicsLinearVelocity()
		: KinematicVelocity;
	return Velocity.Size() / 100.0;
}

FText ASpaceship::GetShipStatusText() const
{
	const int32 ModeNumber = static_cast<int32>(SelectedDriveMode) + 1;
	const int32 MaximumModeNumber = static_cast<int32>(GetMaximumDriveModeForClass()) + 1;
	const FString GravityAssist = !ActiveGravitySource
		? TEXT("NONE")
		: (bEngineRunning && bFlightAssistCompensatesGravity ? TEXT("COMPENSATED") : TEXT("ACTIVE"));
	const double SpeedMetersPerSecond = GetShipSpeedMetersPerSecond();
	FString SpeedText;
	if (SpeedMetersPerSecond < 1000.0) SpeedText = FString::Printf(TEXT("%.1f m/s"), SpeedMetersPerSecond);
	else if (SpeedMetersPerSecond < 1000000.0) SpeedText = FString::Printf(TEXT("%.2f km/s"), SpeedMetersPerSecond / 1000.0);
	else if (SpeedMetersPerSecond < 299792458.0) SpeedText = FString::Printf(TEXT("%.2f Mm/s"), SpeedMetersPerSecond / 1000000.0);
	else SpeedText = FString::Printf(TEXT("%.3f c"), SpeedMetersPerSecond / 299792458.0);
	const FString CustomStatus = GetCustomFlightStatus();
	if (!CustomStatus.IsEmpty())
	{
		// The band model's own two lines carry speed, mode and surroundings.
		return FText::FromString(CustomStatus);
	}
	return FText::FromString(FString::Printf(
		TEXT("SHIP %s   |   ENGINE %s\nPOWER %d/%d: %s   |   ENVIRONMENT: %s\nGRAVITY: %s (%.2f m/s2, %s)   |   %s   |   BOOST x%.2f"),
		*GetSizeClassName(), *GetEngineModeName(), ModeNumber, MaximumModeNumber, *GetDriveModeName(),
		*GetFlightEnvironmentName(), *GetGravitySourceName(), ActiveGravityAcceleration / 100.0, *GravityAssist,
		*SpeedText, CurrentBoostMultiplier));
}

FText ASpaceship::GetShipHintText() const
{
	const FString CustomHint = GetCustomFlightHint();
	if (!CustomHint.IsEmpty())
	{
		return FText::FromString(CustomHint);
	}
	const TCHAR* SpaceWrapHint = CanUseEngineMode(EEngineMode::SpaceWrap) ? TEXT("2 SPACE WRAP") : TEXT("2 SPACE WRAP [N/A]");
	const TCHAR* OffsetHint = CanUseEngineMode(EEngineMode::Offset) ? TEXT("3 OFFSET") : TEXT("3 OFFSET [N/A]");
	return FText::FromString(FString::Printf(
		TEXT("1 IMPULSE   %s   %s   |   G POWER   F EXIT   |   WASD / SPACE / ALT THRUST\nRIGHT SHIFT/CTRL POWER STEP   LEFT SHIFT BOOST   LEFT CTRL BRAKE   |   N MARKERS   M NAV LIST   T TARGET   V ORBITS"),
		SpaceWrapHint, OffsetHint));
}

FText ASpaceship::GetNavigationPanelText() const
{
	if (!ShipNavigation)
	{
		return FText::FromString(TEXT("NAVIGATION OFFLINE"));
	}

	FString Text = FString::Printf(TEXT("NAVIGATION // LOCAL SYSTEM\nENVIRONMENT  %s   |   CELESTIAL CONTACTS  %d"),
		*GetFlightEnvironmentName(), ShipNavigation->GetDiscoveredContactCount());
	if (const FShipNavigationContact* Selected = ShipNavigation->GetSelectedContact())
	{
		const double SpeedCmPerSecond = GetShipSpeedMetersPerSecond() * 100.0;
		FString Eta = TEXT("--");
		if (SpeedCmPerSecond > 1.0)
		{
			const double Seconds = Selected->DistanceCentimeters / SpeedCmPerSecond;
			if (Seconds < 120.0) Eta = FString::Printf(TEXT("%.0f s"), Seconds);
			else if (Seconds < 7200.0) Eta = FString::Printf(TEXT("%.1f min"), Seconds / 60.0);
			else if (Seconds < 172800.0) Eta = FString::Printf(TEXT("%.1f h"), Seconds / 3600.0);
			else Eta = FString::Printf(TEXT("%.1f d"), Seconds / 86400.0);
		}
		Text += FString::Printf(TEXT("\n\nACTIVE TARGET\n%s\n%s  //  %s\nRANGE  %s   |   ETA  %s"),
			*Selected->DisplayName, *Selected->TypeLabel, *Selected->Detail,
			*UShipNavigationComponent::FormatDistance(Selected->DistanceCentimeters), *Eta);
		if (!Selected->HierarchyLabel.IsEmpty())
		{
			Text += TEXT("\nSYSTEM  ") + Selected->HierarchyLabel;
		}
	}

	Text += TEXT("\n\nNEAREST CELESTIAL BODIES");
	const TArray<FShipNavigationContact>& Contacts = ShipNavigation->GetContacts();
	const int32 ListCount = FMath::Min(Contacts.Num(), 5);
	for (int32 Index = 0; Index < ListCount; ++Index)
	{
		const TCHAR* Prefix = Index == ShipNavigation->GetSelectedContactIndex() ? TEXT(">") : TEXT(" ");
		Text += FString::Printf(TEXT("\n%s %-12s  %-24s  %s"), Prefix, *Contacts[Index].TypeLabel,
			*Contacts[Index].DisplayName, *UShipNavigationComponent::FormatDistance(Contacts[Index].DistanceCentimeters));
	}
	return FText::FromString(Text);
}

FText ASpaceship::GetNavigationMarkerText(int32 ContactIndex) const
{
	const FShipNavigationContact* Contact = ShipNavigation ? ShipNavigation->GetContact(ContactIndex) : nullptr;
	if (!Contact)
	{
		return FText::GetEmpty();
	}
	FString Detail = Contact->Detail;
	FString FamilySummary;
	if (const APlanet* Planet = Cast<APlanet>(Contact->Actor.Get());
		Planet && !IsInsideNavigationFocusGravity(Planet) && ShipNavigation)
	{
		int32 MoonCount = 0;
		for (const FShipNavigationContact& FamilyContact : ShipNavigation->GetContacts())
		{
			const AMoon* Moon = Cast<AMoon>(FamilyContact.Actor.Get());
			MoonCount += Moon && Moon->ParentPlanet == Planet ? 1 : 0;
		}
		if (MoonCount > 0)
		{
			Detail.RemoveFromEnd(TEXT(" PLANET"));
			FamilySummary = FString::Printf(TEXT("   |   %d MOONS"), MoonCount);
		}
	}

	return FText::FromString(FString::Printf(TEXT("%s  //  %s\n%s   |   %s%s"),
		*Contact->TypeLabel, *Contact->DisplayName, *Detail,
		*UShipNavigationComponent::FormatDistance(Contact->DistanceCentimeters), *FamilySummary));
}

FVector ASpaceship::GetNavigationContactWorldAnchor(int32 ContactIndex) const
{
	const FShipNavigationContact* Contact = ShipNavigation ? ShipNavigation->GetContact(ContactIndex) : nullptr;
	if (!Contact)
	{
		return FVector::ZeroVector;
	}

	AActor* Actor = Contact->Actor.Get();
	if (!Actor)
	{
		return Contact->GetWorldLocation();
	}

	FVector VisualCenter = Actor->GetActorLocation();
	double LargestVisualRadius = 0.0;
	TInlineComponentArray<UMeshComponent*> MeshComponents(Actor);
	for (const UMeshComponent* MeshComponent : MeshComponents)
	{
		if (!IsValid(MeshComponent) || !MeshComponent->IsRegistered()
			|| !MeshComponent->IsVisible() || MeshComponent->bHiddenInGame)
		{
			continue;
		}
		if (MeshComponent->Bounds.SphereRadius > LargestVisualRadius)
		{
			LargestVisualRadius = MeshComponent->Bounds.SphereRadius;
			VisualCenter = MeshComponent->Bounds.Origin;
		}
	}
	return VisualCenter;
}

const APlanet* ASpaceship::GetNavigationFocusPlanet() const
{
	auto ResolvePlanetFamily = [](const AActor* Actor) -> const APlanet*
	{
		if (const APlanet* Planet = Cast<APlanet>(Actor))
		{
			return Planet;
		}
		if (const AMoon* Moon = Cast<AMoon>(Actor))
		{
			return Moon->ParentPlanet;
		}
		return nullptr;
	};

	if (const APlanet* ActivePlanet = ResolvePlanetFamily(ActiveGravitySource.Get()))
	{
		return ActivePlanet;
	}
	if (ShipNavigation)
	{
		if (const FShipNavigationContact* SelectedContact = ShipNavigation->GetSelectedContact())
		{
			if (const APlanet* SelectedPlanet = ResolvePlanetFamily(SelectedContact->Actor.Get()))
			{
				return SelectedPlanet;
			}
		}
		for (const FShipNavigationContact& Contact : ShipNavigation->GetContacts())
		{
			if (const APlanet* Planet = Cast<APlanet>(Contact.Actor.Get()))
			{
				return Planet;
			}
		}
	}
	return nullptr;
}

bool ASpaceship::IsInsideNavigationFocusGravity(const APlanet* FocusPlanet) const
{
	if (!FocusPlanet)
	{
		return false;
	}
	// Marker expansion follows the complete resident WorldScape family rather
	// than the momentary gravity source. Gravity can briefly drop while crossing
	// empty space between a planet and its moons; that must not collapse every
	// moon flag while the family is still loaded and retained by the streamer.
	if (const UWorld* World = GetWorld())
	{
		if (const UAPSPlanetEnvironmentStreamingSubsystem* Streaming =
			World->GetSubsystem<UAPSPlanetEnvironmentStreamingSubsystem>())
		{
			if (Streaming->GetResidentFamily() == FocusPlanet)
			{
				return true;
			}
		}
	}
	if (CurrentFlightEnvironment == EShipFlightEnvironment::DeepSpace)
	{
		return false;
	}
	if (ActiveGravitySource.Get() == FocusPlanet)
	{
		return true;
	}
	const AMoon* ActiveMoon = Cast<AMoon>(ActiveGravitySource.Get());
	return ActiveMoon && ActiveMoon->ParentPlanet == FocusPlanet;
}

bool ASpaceship::ShouldShowNavigationMarker(int32 ContactIndex) const
{
	if (!ShipNavigation || ContactIndex < 0 || ContactIndex >= MaximumNavigationMarkers
		|| !ShipNavigation->GetContact(ContactIndex))
	{
		return false;
	}
	const int32 SelectedIndex = ShipNavigation->GetSelectedContactIndex();
	if (ContactIndex == SelectedIndex)
	{
		return true;
	}

	const FShipNavigationContact* Contact = ShipNavigation->GetContact(ContactIndex);
	if (Contact->Type == EShipNavigationContactType::Planet || Contact->bOwnColony)
	{
		return true;
	}
	if (const AMoon* Moon = Cast<AMoon>(Contact->Actor.Get()))
	{
		// At system scale a planet and all of its moons are one compact flag.
		// Individual moon flags unfold only after gravity capture; an explicitly
		// selected moon remains visible so target cycling never loses feedback.
		return IsInsideNavigationFocusGravity(Moon->ParentPlanet);
	}
	return false;
}

bool ASpaceship::ProjectWorldLocationToNavigationScreen(const FVector& WorldLocation,
	FVector2D& OutScreenPosition, bool bRequireInsideViewport) const
{
	APlayerController* PlayerController = Cast<APlayerController>(GetController());
	if (!PlayerController || !GEngine || !GEngine->GameViewport)
	{
		return false;
	}
	// Same result as UWidgetLayoutLibrary::ProjectWorldLocationToWidgetPosition(..., true), one view per frame.
	APSNavigationHud::FProjectionFrame& Projection = APSNavigationHud::GProjectionFrame;
	if (Projection.Ship != this || Projection.Frame != GFrameCounter)
	{
		Projection = APSNavigationHud::FProjectionFrame();
		Projection.Ship = this;
		Projection.Frame = GFrameCounter;
		const ULocalPlayer* LocalPlayer = PlayerController->GetLocalPlayer();
		FSceneViewProjectionData ProjectionData;
		if (LocalPlayer && LocalPlayer->ViewportClient
			&& LocalPlayer->GetProjectionData(LocalPlayer->ViewportClient->Viewport, ProjectionData))
		{
			Projection.ViewProjection = ProjectionData.ComputeViewProjectionMatrix();
			Projection.ViewRect = ProjectionData.GetConstrainedViewRect();
			FVector2D Origin;
			FVector2D Reference;
			USlateBlueprintLibrary::ScreenToViewport(PlayerController, FVector2D::ZeroVector, Origin);
			USlateBlueprintLibrary::ScreenToViewport(PlayerController, FVector2D(1000.0, 1000.0), Reference);
			Projection.Offset = Origin;
			Projection.Scale = (Reference - Origin) / 1000.0;
			Projection.bValid = true;
		}
	}
	FVector2D PixelPosition;
	if (!Projection.bValid || !FSceneView::ProjectWorldToScreen(
			WorldLocation, Projection.ViewRect, Projection.ViewProjection, PixelPosition))
	{
		return false;
	}
	PixelPosition -= FVector2D(Projection.ViewRect.Min);
	OutScreenPosition = Projection.Offset
		+ FVector2D(FMath::RoundToInt(PixelPosition.X), FMath::RoundToInt(PixelPosition.Y)) * Projection.Scale;
	if (APSShipPerf::CVarHudProjectionCheck.GetValueOnGameThread() != 0)
	{
		static double LargestDifference = 0.0;
		static double LastReportSeconds = 0.0;
		FVector2D EnginePosition;
		if (UWidgetLayoutLibrary::ProjectWorldLocationToWidgetPosition(PlayerController, WorldLocation, EnginePosition, true))
		{
			LargestDifference = FMath::Max(LargestDifference, FVector2D::Distance(EnginePosition, OutScreenPosition));
		}
		const double Now = FPlatformTime::Seconds();
		if (Now - LastReportSeconds >= 1.0)
		{
			UE_LOG(LogTemp, Log, TEXT("[APS.ShipPerf] HUD projection check: largest difference to the engine %.4f slate units"),
				LargestDifference);
			LargestDifference = 0.0;
			LastReportSeconds = Now;
		}
	}

	const TSharedPtr<SViewport> ViewportWidget = GEngine->GameViewport->GetGameViewportWidget();
	const FVector2D SlateViewportSize = ViewportWidget.IsValid()
		? ViewportWidget->GetCachedGeometry().GetLocalSize() : FVector2D::ZeroVector;
	if (!bRequireInsideViewport)
	{
		return SlateViewportSize.X > 0.0f && SlateViewportSize.Y > 0.0f;
	}
	return OutScreenPosition.X >= 8.0 && OutScreenPosition.Y >= 8.0
		&& OutScreenPosition.X <= SlateViewportSize.X - 8.0
		&& OutScreenPosition.Y <= SlateViewportSize.Y - 8.0;
}

bool ASpaceship::ProjectNavigationContactToScreen(int32 ContactIndex, FVector2D& OutScreenPosition) const
{
	const FShipNavigationContact* Contact = ShipNavigation ? ShipNavigation->GetContact(ContactIndex) : nullptr;
	return Contact && ProjectWorldLocationToNavigationScreen(
		GetNavigationContactWorldAnchor(ContactIndex), OutScreenPosition);
}

bool ASpaceship::GetNavigationMarkerLayout(int32 ContactIndex, FVector2D& OutAnchorPosition,
	FVector2D& OutLabelPosition, const TSet<int32>* OccludedContacts) const
{
	if (!ShipNavigation || !ShouldShowNavigationMarker(ContactIndex)
		|| (OccludedContacts && OccludedContacts->Contains(ContactIndex))
		|| !GEngine || !GEngine->GameViewport)
	{
		return false;
	}

	const FVector2D LabelSize(APSNavigationHud::MarkerWidth, APSNavigationHud::MarkerHeight);
	const FVector2D ViewportSize = GEngine->GameViewport->GetGameViewportWidget().IsValid()
		? GEngine->GameViewport->GetGameViewportWidget()->GetCachedGeometry().GetLocalSize()
		: FVector2D::ZeroVector;
	if (ViewportSize.X <= LabelSize.X || ViewportSize.Y <= LabelSize.Y)
	{
		return false;
	}

	FVector2D Anchor;
	if (!ProjectNavigationContactToScreen(ContactIndex, Anchor))
	{
		return false;
	}

	APSNavigationHud::FLayoutFrame& LayoutFrame = APSNavigationHud::GLayoutFrame;
	if (LayoutFrame.Ship == this && LayoutFrame.Frame == GFrameCounter && LayoutFrame.OccludedContacts == OccludedContacts)
	{
		if (const TPair<FVector2D, FVector2D>* Cached = LayoutFrame.Layouts.Find(ContactIndex))
		{
			OutAnchorPosition = Cached->Key;
			OutLabelPosition = Cached->Value;
			return true;
		}
		return false;
	}
	LayoutFrame.Ship = this;
	LayoutFrame.Frame = GFrameCounter;
	LayoutFrame.OccludedContacts = OccludedContacts;
	LayoutFrame.Layouts.Reset();

	struct FMarkerPlacement
	{
		int32 Index{INDEX_NONE};
		FVector2D Anchor{FVector2D::ZeroVector};
	};
	TArray<FMarkerPlacement, TInlineAllocator<32>> Placements;
	const int32 ContactCount = FMath::Min(ShipNavigation->GetContacts().Num(), MaximumNavigationMarkers);
	for (int32 Index = 0; Index < ContactCount; ++Index)
	{
		FVector2D ProjectedAnchor;
		if (ShouldShowNavigationMarker(Index)
			&& (!OccludedContacts || !OccludedContacts->Contains(Index))
			&& ProjectNavigationContactToScreen(Index, ProjectedAnchor))
		{
			Placements.Add({Index, ProjectedAnchor});
		}
	}
	const int32 SelectedIndex = ShipNavigation->GetSelectedContactIndex();
	Placements.Sort([this, SelectedIndex](const FMarkerPlacement& Left, const FMarkerPlacement& Right)
	{
		if (Left.Index == Right.Index) return false;
		const bool bLeftSelected = Left.Index == SelectedIndex;
		const bool bRightSelected = Right.Index == SelectedIndex;
		if (bLeftSelected != bRightSelected) return bLeftSelected;
		const FShipNavigationContact* LeftContact = ShipNavigation->GetContact(Left.Index);
		const FShipNavigationContact* RightContact = ShipNavigation->GetContact(Right.Index);
		const bool bLeftPlanet = LeftContact && LeftContact->Type == EShipNavigationContactType::Planet;
		const bool bRightPlanet = RightContact && RightContact->Type == EShipNavigationContactType::Planet;
		return bLeftPlanet != bRightPlanet ? bLeftPlanet : Left.Index < Right.Index;
	});

	TArray<FSlateRect, TInlineAllocator<32>> OccupiedRects;
	if (bNavigationPanelVisible)
	{
		OccupiedRects.Add(FSlateRect(
			FMath::Max(0.0f, ViewportSize.X - 420.0f), 24.0f, ViewportSize.X - 20.0f, 275.0f));
	}
	constexpr float ScreenMargin = 10.0f;
	const float StepY = LabelSize.Y + APSNavigationHud::MarkerGap + 4.0f;
	for (const FMarkerPlacement& Placement : Placements)
	{
		// Default to a left-facing flag: the card sits to the left of the
		// object's vertical pole. Only flip it when the left viewport edge
		// cannot contain the full card.
		const bool bExtendFlagRight = Placement.Anchor.X - LabelSize.X + 1.5f < ScreenMargin;
		const FVector2D Desired(
			bExtendFlagRight
				? Placement.Anchor.X - 1.5f
				: Placement.Anchor.X - LabelSize.X + 1.5f,
			Placement.Anchor.Y - LabelSize.Y - APSNavigationHud::FlagPoleLength);
		FVector2D Chosen = Desired;
		bool bFoundFreeSlot = false;
		for (int32 RowMagnitude = 0; RowMagnitude <= 16 && !bFoundFreeSlot; ++RowMagnitude)
		{
			const int32 SignCount = RowMagnitude == 0 ? 1 : 2;
			for (int32 SignIndex = 0; SignIndex < SignCount; ++SignIndex)
			{
				const int32 SignedRow = RowMagnitude == 0 ? 0
					: (SignIndex == 0 ? -RowMagnitude : RowMagnitude);
				const FVector2D Candidate = Desired + FVector2D(0.0f, SignedRow * StepY);
				if (Candidate.X < ScreenMargin || Candidate.Y < ScreenMargin
					|| Candidate.X + LabelSize.X > ViewportSize.X - ScreenMargin
					|| Candidate.Y + LabelSize.Y > ViewportSize.Y - ScreenMargin)
				{
					continue;
				}
				const FSlateRect CandidateRect(Candidate.X - 4.0f, Candidate.Y - 4.0f,
					Candidate.X + LabelSize.X + 4.0f, Candidate.Y + LabelSize.Y + 4.0f);
				const bool bOverlaps = OccupiedRects.ContainsByPredicate(
					[&CandidateRect](const FSlateRect& Occupied)
					{
						return FSlateRect::DoRectanglesIntersect(CandidateRect, Occupied);
					});
				if (!bOverlaps)
				{
					Chosen = Candidate;
					bFoundFreeSlot = true;
					break;
				}
			}
		}
		Chosen.X = FMath::Clamp(Chosen.X, ScreenMargin, ViewportSize.X - LabelSize.X - ScreenMargin);
		Chosen.Y = FMath::Clamp(Chosen.Y, ScreenMargin, ViewportSize.Y - LabelSize.Y - ScreenMargin);
		OccupiedRects.Add(FSlateRect(Chosen.X - 4.0f, Chosen.Y - 4.0f,
			Chosen.X + LabelSize.X + 4.0f, Chosen.Y + LabelSize.Y + 4.0f));
		// Every placement depends only on the ones before it, so the full pass gives each marker the same
		// position the per-marker early exit used to give.
		LayoutFrame.Layouts.Add(Placement.Index, TPair<FVector2D, FVector2D>(Placement.Anchor, Chosen));
	}
	if (const TPair<FVector2D, FVector2D>* Layout = LayoutFrame.Layouts.Find(ContactIndex))
	{
		OutAnchorPosition = Layout->Key;
		OutLabelPosition = Layout->Value;
		return true;
	}
	return false;
}

int32 ASpaceship::PaintNavigationOverlay(const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
	FSlateWindowElementList& OutDrawElements, int32 LayerId) const
{
	if (!ShipNavigation)
	{
		return LayerId;
	}
	TRACE_CPUPROFILER_EVENT_SCOPE(APS_Ship_NavigationHud);
	APSShipPerf::FScope PerfScope(IsValid(Pilot) && APSShipPerf::Enabled(), APSShipPerf::Hud);

	const FPaintGeometry PaintGeometry = AllottedGeometry.ToPaintGeometry();
	struct FNavigationOccluder
	{
		const AActor* Actor{nullptr};
		FVector Center{FVector::ZeroVector};
		double Radius{0.0};
	};
	TArray<FNavigationOccluder, TInlineAllocator<32>> Occluders;
	const int32 NavigationContactCount = FMath::Min(
		ShipNavigation->GetContacts().Num(), MaximumNavigationMarkers);
	for (int32 ContactIndex = 0; ContactIndex < NavigationContactCount; ++ContactIndex)
	{
		const FShipNavigationContact* Contact = ShipNavigation->GetContact(ContactIndex);
		const APlanetaryBody* Body = Contact ? Cast<APlanetaryBody>(Contact->Actor.Get()) : nullptr;
		if (!Body)
		{
			continue;
		}
		const double OcclusionRadius = Body->GetWorldScapeBodyRadiusCm();
		if (OcclusionRadius > UE_DOUBLE_SMALL_NUMBER)
		{
			Occluders.Add({Body, GetNavigationContactWorldAnchor(ContactIndex), OcclusionRadius});
		}
	}

	const APlayerController* NavigationPlayerController = Cast<APlayerController>(GetController());
	const APlayerCameraManager* NavigationCameraManager = NavigationPlayerController
		? NavigationPlayerController->PlayerCameraManager : nullptr;
	const FVector NavigationCameraLocation = NavigationCameraManager
		? NavigationCameraManager->GetCameraLocation() : FVector::ZeroVector;
	// Markers test against 98% of a body (a flag on its limb stays visible); orbit rings against the whole body, so a
	// ring never shows through the ground (Rio, 30.09).
	auto IsWorldPointOccluded = [&](const FVector& WorldPoint, const AActor* IgnoredActor = nullptr,
		const double RadiusScale = 0.98)
	{
		if (!NavigationCameraManager)
		{
			return false;
		}
		const FVector CameraToPoint = WorldPoint - NavigationCameraLocation;
		const double SegmentLengthSquared = CameraToPoint.SizeSquared();
		if (SegmentLengthSquared <= UE_DOUBLE_SMALL_NUMBER)
		{
			return false;
		}
		for (const FNavigationOccluder& Occluder : Occluders)
		{
			if (Occluder.Actor == IgnoredActor)
			{
				continue;
			}
			const FVector CameraToCenter = Occluder.Center - NavigationCameraLocation;
			// In a valley the camera can sit below the body's base radius: the ground still hides what lies beneath the
			// horizon, so the occluder shrinks to just below the camera instead of being skipped.
			const double Radius = FMath::Min(Occluder.Radius * RadiusScale, CameraToCenter.Size() - 100.0);
			if (Radius <= 0.0)
			{
				continue;
			}
			const double RadiusSquared = FMath::Square(Radius);
			const double SegmentFraction = FVector::DotProduct(CameraToCenter, CameraToPoint)
				/ SegmentLengthSquared;
			if (SegmentFraction <= 0.0 || SegmentFraction >= 0.9995)
			{
				continue;
			}
			const FVector ClosestPoint = NavigationCameraLocation + CameraToPoint * SegmentFraction;
			if (FVector::DistSquared(ClosestPoint, Occluder.Center) < RadiusSquared)
			{
				return true;
			}
		}
		return false;
	};

	auto DrawScreenLine = [&](const TArray<FVector2D>& Points, const FLinearColor& Color, float Thickness,
		int32 DrawLayer)
	{
		if (Points.Num() >= 2)
		{
			FSlateDrawElement::MakeLines(OutDrawElements, DrawLayer, PaintGeometry, Points,
				ESlateDrawEffect::None, Color, true, Thickness);
		}
	};

	auto DrawProjectedRing = [&](const FVector& Center, const FVector& AxisX, const FVector& AxisY,
		double Radius, const FLinearColor& Color, float Thickness, bool bDashed, int32 DrawLayer)
	{
		if (Radius <= UE_DOUBLE_SMALL_NUMBER) return;
		constexpr int32 SegmentCount = 128;
		TArray<FVector2D> ContinuousSegment;
		FVector2D PreviousPoint;
		bool bPreviousValid = false;
		for (int32 SegmentIndex = 0; SegmentIndex <= SegmentCount; ++SegmentIndex)
		{
			const double Angle = UE_TWO_PI * static_cast<double>(SegmentIndex) / SegmentCount;
			const FVector WorldPoint = Center + AxisX * (FMath::Cos(Angle) * Radius)
				+ AxisY * (FMath::Sin(Angle) * Radius);
			FVector2D ScreenPoint;
			const bool bValid = !IsWorldPointOccluded(WorldPoint, nullptr, 1.0)
				&& ProjectWorldLocationToNavigationScreen(WorldPoint, ScreenPoint, false);
			if (bDashed)
			{
				if (bValid && bPreviousValid && SegmentIndex % 3 != 0)
				{
					DrawScreenLine({PreviousPoint, ScreenPoint}, Color, Thickness, DrawLayer);
				}
			}
			else if (bValid)
			{
				ContinuousSegment.Add(ScreenPoint);
			}
			else
			{
				DrawScreenLine(ContinuousSegment, Color, Thickness, DrawLayer);
				ContinuousSegment.Reset();
			}
			PreviousPoint = ScreenPoint;
			bPreviousValid = bValid;
		}
		DrawScreenLine(ContinuousSegment, Color, Thickness, DrawLayer);
	};

	if (bNavigationGuidesVisible)
	{
		TSet<const APlanetOrbit*> PaintedOrbits;
		const int32 SelectedIndex = ShipNavigation->GetSelectedContactIndex();
		const FShipNavigationContact* SelectedContact = ShipNavigation->GetSelectedContact();
		const APlanetaryBody* SelectedBody = SelectedContact
			? Cast<APlanetaryBody>(SelectedContact->Actor.Get()) : nullptr;
		const APlanet* SelectedPlanetFamily = Cast<APlanet>(SelectedBody);
		if (const AMoon* SelectedMoon = Cast<AMoon>(SelectedBody))
		{
			SelectedPlanetFamily = SelectedMoon->ParentPlanet;
		}
		int32 PaintedPlanetCount = 0;
		int32 PaintedMoonCount = 0;
		const int32 ContactCount = FMath::Min(ShipNavigation->GetContacts().Num(), MaximumNavigationMarkers);
		for (int32 ContactIndex = 0; ContactIndex < ContactCount; ++ContactIndex)
		{
			const FShipNavigationContact* Contact = ShipNavigation->GetContact(ContactIndex);
			const APlanetaryBody* Body = Contact ? Cast<APlanetaryBody>(Contact->Actor.Get()) : nullptr;
			const APlanetOrbit* Orbit = Body ? Cast<APlanetOrbit>(Body->GetAttachParentActor()) : nullptr;
			if (!Orbit || PaintedOrbits.Contains(Orbit)) continue;

			const bool bSelectedOrbit = ContactIndex == SelectedIndex;
			bool bShouldPaintOrbit = bSelectedOrbit;
			if (const APlanet* Planet = Cast<APlanet>(Body))
			{
				bShouldPaintOrbit |= Planet == SelectedPlanetFamily || PaintedPlanetCount < 4;
				if (bShouldPaintOrbit) ++PaintedPlanetCount;
			}
			else if (const AMoon* Moon = Cast<AMoon>(Body))
			{
				bShouldPaintOrbit |= Moon->ParentPlanet == SelectedPlanetFamily && PaintedMoonCount < 2;
				if (bShouldPaintOrbit) ++PaintedMoonCount;
			}
			if (!bShouldPaintOrbit) continue;
			PaintedOrbits.Add(Orbit);

			const FVector OrbitCenter = Orbit->GetActorLocation();
			const FVector BodyCenter = GetNavigationContactWorldAnchor(ContactIndex);
			const FVector RadialVector = BodyCenter - OrbitCenter;
			const double OrbitRadius = RadialVector.Size();
			const FVector OrbitRadial = RadialVector.GetSafeNormal();
			FVector OrbitTangent = FVector::CrossProduct(Orbit->GetActorUpVector(), OrbitRadial).GetSafeNormal();
			if (OrbitTangent.IsNearlyZero()) OrbitTangent = Orbit->GetActorRightVector();
			const FLinearColor MarkerColor = GetNavigationMarkerColor(ContactIndex);
			FLinearColor OrbitColor(MarkerColor.R, MarkerColor.G, MarkerColor.B,
				bSelectedOrbit ? 0.62f : (Contact->Type == EShipNavigationContactType::Moon ? 0.16f : 0.14f));
			DrawProjectedRing(OrbitCenter, OrbitRadial, OrbitTangent, OrbitRadius,
				OrbitColor, bSelectedOrbit ? 1.25f : 0.65f, false, LayerId);
		}

		if (SelectedContact)
		{
			const APlayerController* PC = Cast<APlayerController>(GetController());
			const APlayerCameraManager* CameraManager = PC ? PC->PlayerCameraManager : nullptr;
			const FVector CameraRight = CameraManager ? CameraManager->GetActorRightVector() : FVector::RightVector;
			const FVector CameraUp = CameraManager ? CameraManager->GetActorUpVector() : FVector::UpVector;

			const AStar* ParentStar = nullptr;
			if (const APlanet* Planet = Cast<APlanet>(SelectedBody)) ParentStar = Planet->ParentStar;
			else if (const AMoon* Moon = Cast<AMoon>(SelectedBody))
			{
				ParentStar = Moon->ParentPlanet ? Moon->ParentPlanet->ParentStar : nullptr;
			}

			if (const APlanet* Planet = Cast<APlanet>(SelectedBody);
				Planet && Planet->GravityCollisionZone && Planet->GravityCollisionZone->GetScaledSphereRadius() > 0.0f)
			{
				DrawProjectedRing(Planet->GravityCollisionZone->GetComponentLocation(), CameraRight, CameraUp,
					Planet->GravityCollisionZone->GetScaledSphereRadius(),
					FLinearColor(0.28f, 1.0f, 0.58f, 0.34f), 0.9f, true, LayerId + 1);
			}
			if (ParentStar && ParentStar->PlanetarySystemZone
				&& ParentStar->PlanetarySystemZone->GetScaledSphereRadius() > 0.0f)
			{
				DrawProjectedRing(ParentStar->PlanetarySystemZone->GetComponentLocation(), CameraRight, CameraUp,
					ParentStar->PlanetarySystemZone->GetScaledSphereRadius(),
					FLinearColor(0.22f, 0.65f, 1.0f, 0.16f), 0.7f, true, LayerId);
			}

			const AActor* Ancestor = ParentStar;
			while (Ancestor && !Ancestor->IsA<AStarSystem>()) Ancestor = Ancestor->GetAttachParentActor();
			if (const AStarSystem* System = Cast<AStarSystem>(Ancestor);
				System && System->StarSystemZone && System->StarSystemZone->GetScaledSphereRadius() > 0.0f)
			{
				DrawProjectedRing(System->StarSystemZone->GetComponentLocation(), CameraRight, CameraUp,
					System->StarSystemZone->GetScaledSphereRadius(),
					FLinearColor(0.65f, 0.42f, 1.0f, 0.13f), 0.65f, true, LayerId);
			}
		}
	}

	if (bNavigationMarkersVisible)
	{
		TSet<int32> OccludedContacts;
		for (int32 ContactIndex = 0; ContactIndex < NavigationContactCount; ++ContactIndex)
		{
			const FShipNavigationContact* Contact = ShipNavigation->GetContact(ContactIndex);
			if (Contact && IsWorldPointOccluded(
				GetNavigationContactWorldAnchor(ContactIndex), Contact->Actor.Get()))
			{
				OccludedContacts.Add(ContactIndex);
			}
		}
		const int32 ContactCount = NavigationContactCount;
		for (int32 ContactIndex = 0; ContactIndex < ContactCount; ++ContactIndex)
		{
			FVector2D Anchor;
			FVector2D Label;
			if (!GetNavigationMarkerLayout(ContactIndex, Anchor, Label, &OccludedContacts)) continue;
			const FLinearColor Color = GetNavigationMarkerColor(ContactIndex);
			const bool bSelected = ContactIndex == ShipNavigation->GetSelectedContactIndex();
			const bool bAccentOnRight = FMath::Abs(
				Anchor.X - (Label.X + APSNavigationHud::MarkerWidth - 1.5f))
				< FMath::Abs(Anchor.X - (Label.X + 1.5f));
			const FVector2D FlagPoleEnd(Anchor.X, Label.Y + APSNavigationHud::MarkerHeight);
			DrawScreenLine({Anchor, FlagPoleEnd}, FLinearColor(Color.R, Color.G, Color.B,
				bSelected ? 0.82f : 0.42f), bSelected ? 1.15f : 0.65f, LayerId + 2);

			// Draw the flag and its pole in the same OnPaint pass. A separate
			// ConstraintCanvas was laid out before the post-physics camera update,
			// so at high speed the card used an older projection than its pole.
			const FVector2f LabelPosition(static_cast<float>(Label.X), static_cast<float>(Label.Y));
			const FVector2f LabelSize(APSNavigationHud::MarkerWidth, APSNavigationHud::MarkerHeight);
			const FPaintGeometry LabelGeometry = AllottedGeometry.ToPaintGeometry(
				LabelSize, FSlateLayoutTransform(LabelPosition));
			const FLinearColor BackgroundColor(
				Color.R * 0.055f, Color.G * 0.055f, Color.B * 0.055f, bSelected ? 0.91f : 0.68f);
			FSlateDrawElement::MakeBox(OutDrawElements, LayerId + 3, LabelGeometry,
				FCoreStyle::Get().GetBrush("WhiteBrush"), ESlateDrawEffect::None, BackgroundColor);

			const float AccentX = bAccentOnRight
				? Label.X + APSNavigationHud::MarkerWidth - 3.0f : Label.X;
			const FPaintGeometry AccentGeometry = AllottedGeometry.ToPaintGeometry(
				FVector2f(3.0f, APSNavigationHud::MarkerHeight),
				FSlateLayoutTransform(FVector2f(AccentX, Label.Y)));
			FSlateDrawElement::MakeBox(OutDrawElements, LayerId + 4, AccentGeometry,
				FCoreStyle::Get().GetBrush("WhiteBrush"), ESlateDrawEffect::None, Color);

			const float TextLeftPadding = bAccentOnRight ? 8.0f : 11.0f;
			const FPaintGeometry TextGeometry = AllottedGeometry.ToPaintGeometry(
				FVector2f(APSNavigationHud::MarkerWidth - 18.0f, APSNavigationHud::MarkerHeight - 7.0f),
				FSlateLayoutTransform(FVector2f(Label.X + TextLeftPadding, Label.Y + 4.0f)));
			const FLinearColor TextColor(
				FMath::Lerp(Color.R, 0.9f, 0.38f),
				FMath::Lerp(Color.G, 0.94f, 0.38f),
				FMath::Lerp(Color.B, 0.98f, 0.38f), 0.96f);
			FSlateDrawElement::MakeText(OutDrawElements, LayerId + 5, TextGeometry,
				GetNavigationMarkerText(ContactIndex), FCoreStyle::GetDefaultFontStyle("Regular", 8),
				ESlateDrawEffect::None, TextColor);

			const float CrossExtent = bSelected ? 4.5f : 2.75f;
			DrawScreenLine({Anchor + FVector2D(-CrossExtent, 0.0f), Anchor + FVector2D(CrossExtent, 0.0f)},
				Color, bSelected ? 1.35f : 0.8f, LayerId + 6);
			DrawScreenLine({Anchor + FVector2D(0.0f, -CrossExtent), Anchor + FVector2D(0.0f, CrossExtent)},
				Color, bSelected ? 1.35f : 0.8f, LayerId + 6);
		}

		// Course target: corner brackets that stand apart from every flag and orbit, pulsing gently, and while it is out
		// of view an arrow at the screen edge toward it (Rio, 30.09: the target has to be clearly marked).
		const int32 TargetIndex = ShipNavigation->GetSelectedContactIndex();
		const FShipNavigationContact* Target = ShipNavigation->GetSelectedContact();
		const TSharedPtr<SViewport> TargetViewport = GEngine && GEngine->GameViewport
			? GEngine->GameViewport->GetGameViewportWidget() : nullptr;
		if (Target && NavigationCameraManager && TargetViewport.IsValid())
		{
			const FVector2D ViewportSize = TargetViewport->GetCachedGeometry().GetLocalSize();
			const FVector TargetWorld = GetNavigationContactWorldAnchor(TargetIndex);
			const FLinearColor TargetColor(1.0f, 0.74f, 0.18f,
				0.78f + 0.22f * static_cast<float>(FMath::Sin(FPlatformTime::Seconds() * 3.0)));
			const FString Caption = FString::Printf(TEXT("COURSE  %s"),
				*UShipNavigationComponent::FormatDistance(FVector::Distance(GetActorLocation(), TargetWorld)));
			const FSlateFontInfo CaptionFont = FCoreStyle::GetDefaultFontStyle("Bold", 8);
			FVector2D TargetScreen;
			if (ProjectWorldLocationToNavigationScreen(TargetWorld, TargetScreen, true))
			{
				// Around a resolved disc (a planet or moon) the brackets hug its rim, otherwise they keep a fixed box.
				double DiscPixels = 0.0;
				FVector2D Rim;
				if (const APlanetaryBody* TargetBody = Cast<APlanetaryBody>(Target->Actor.Get()); TargetBody
					&& ProjectWorldLocationToNavigationScreen(TargetWorld + NavigationCameraManager->GetActorRightVector()
						* TargetBody->GetWorldScapeBodyRadiusCm(), Rim, false))
				{
					DiscPixels = FVector2D::Distance(Rim, TargetScreen);
				}
				const double Half = FMath::Clamp(DiscPixels + 10.0, 16.0, 160.0);
				const double Arm = FMath::Clamp(Half * 0.42, 6.0, 18.0);
				for (const FVector2D& Corner : {FVector2D(-1.0, -1.0), FVector2D(1.0, -1.0), FVector2D(1.0, 1.0), FVector2D(-1.0, 1.0)})
				{
					const FVector2D Tip = TargetScreen + Corner * Half;
					DrawScreenLine({Tip - FVector2D(Corner.X * Arm, 0.0), Tip, Tip - FVector2D(0.0, Corner.Y * Arm)},
						TargetColor, 1.6f, LayerId + 7);
				}
				FSlateDrawElement::MakeText(OutDrawElements, LayerId + 7, AllottedGeometry.ToPaintGeometry(
					FVector2f(220.0f, 14.0f), FSlateLayoutTransform(FVector2f(static_cast<float>(TargetScreen.X - Half),
						static_cast<float>(TargetScreen.Y + Half + 4.0)))),
					Caption, CaptionFont, ESlateDrawEffect::None, TargetColor);
			}
			else if (ViewportSize.X > 160.0 && ViewportSize.Y > 160.0)
			{
				// Off screen or behind the camera: the target's direction from the view axis, on an inset ellipse.
				const FVector Local = NavigationCameraManager->GetCameraRotation().UnrotateVector(
					TargetWorld - NavigationCameraLocation);
				FVector2D Direction(Local.Y, -Local.Z);
				if (!Direction.Normalize())
				{
					Direction = FVector2D(0.0, 1.0);
				}
				const FVector2D Centre = ViewportSize * 0.5;
				const FVector2D Tip = Centre + FVector2D(Direction.X * (Centre.X - 56.0), Direction.Y * (Centre.Y - 56.0));
				const FVector2D Side(-Direction.Y, Direction.X);
				DrawScreenLine({Tip - Direction * 16.0 + Side * 10.0, Tip, Tip - Direction * 16.0 - Side * 10.0},
					TargetColor, 2.2f, LayerId + 7);
				const FVector2D CaptionAt = Tip - Direction * 40.0 - FVector2D(40.0, 7.0);
				FSlateDrawElement::MakeText(OutDrawElements, LayerId + 7, AllottedGeometry.ToPaintGeometry(
					FVector2f(220.0f, 14.0f), FSlateLayoutTransform(FVector2f(static_cast<float>(CaptionAt.X),
						static_cast<float>(CaptionAt.Y)))),
					Caption, CaptionFont, ESlateDrawEffect::None, TargetColor);
			}
		}
	}
	return LayerId + 7;
}

FLinearColor ASpaceship::GetNavigationMarkerColor(int32 ContactIndex) const
{
	if (ShipNavigation && ContactIndex == ShipNavigation->GetSelectedContactIndex())
	{
		return FLinearColor(1.0f, 0.68f, 0.16f, 1.0f);
	}
	const FShipNavigationContact* Contact = ShipNavigation ? ShipNavigation->GetContact(ContactIndex) : nullptr;
	if (!Contact) return FLinearColor::Transparent;
	if (Contact->bOwnColony)
	{
		return FLinearColor(0.36f, 1.0f, 0.58f, 0.98f);
	}
	if (const APlanet* Planet = Cast<APlanet>(Contact->Actor.Get()))
	{
		switch (Planet->PlanetType)
		{
		case EPlanetType::Ice:
		case EPlanetType::Frozen:
		case EPlanetType::Nordic:
		case EPlanetType::Tundra:
		case EPlanetType::IceGiant:
			return FLinearColor(0.72f, 0.9f, 1.0f, 0.96f);
		case EPlanetType::Ocean:
		case EPlanetType::Water:
		case EPlanetType::Archipelago:
			return FLinearColor(0.16f, 0.62f, 1.0f, 0.96f);
		case EPlanetType::Terrestrial:
		case EPlanetType::Forest:
		case EPlanetType::Oasis:
		case EPlanetType::Pangea:
		case EPlanetType::SuperEarth:
			return FLinearColor(0.2f, 0.92f, 0.58f, 0.96f);
		case EPlanetType::Desert:
		case EPlanetType::Sand:
			return FLinearColor(1.0f, 0.68f, 0.24f, 0.96f);
		case EPlanetType::Volcanic:
		case EPlanetType::Melted:
		case EPlanetType::Lava:
		case EPlanetType::HotGiant:
			return FLinearColor(1.0f, 0.25f, 0.1f, 0.96f);
		case EPlanetType::GasGiant:
		case EPlanetType::Greenhouse:
		case EPlanetType::Ammonia:
			return FLinearColor(0.92f, 0.72f, 0.3f, 0.96f);
		case EPlanetType::Metal:
		case EPlanetType::Metallic:
		case EPlanetType::Carbon:
			return FLinearColor(0.74f, 0.72f, 0.88f, 0.96f);
		default:
			return FLinearColor(0.28f, 0.84f, 0.75f, 0.95f);
		}
	}
	if (const AMoon* Moon = Cast<AMoon>(Contact->Actor.Get()))
	{
		switch (Moon->MoonType)
		{
		case EMoonType::Icy:
			return FLinearColor(0.82f, 0.93f, 1.0f, 0.96f);
		case EMoonType::Ocean:
			return FLinearColor(0.22f, 0.64f, 1.0f, 0.96f);
		case EMoonType::Continental:
			return FLinearColor(0.38f, 0.84f, 0.65f, 0.96f);
		case EMoonType::Desert:
			return FLinearColor(0.96f, 0.67f, 0.34f, 0.96f);
		case EMoonType::Volcanic:
			return FLinearColor(1.0f, 0.31f, 0.12f, 0.96f);
		case EMoonType::Iron:
			return FLinearColor(0.68f, 0.74f, 0.82f, 0.96f);
		case EMoonType::Gas:
		case EMoonType::Peculiar:
			return FLinearColor(0.72f, 0.52f, 1.0f, 0.96f);
		default:
			return FLinearColor(0.66f, 0.76f, 0.9f, 0.95f);
		}
	}
	switch (Contact->Type)
	{
	case EShipNavigationContactType::Star: return FLinearColor(0.45f, 0.78f, 1.0f, 0.95f);
	case EShipNavigationContactType::Planet: return FLinearColor(0.18f, 0.92f, 0.74f, 0.94f);
	case EShipNavigationContactType::Moon: return FLinearColor(0.48f, 0.67f, 1.0f, 0.94f);
	case EShipNavigationContactType::Station:
	case EShipNavigationContactType::Settlement:
	case EShipNavigationContactType::Infrastructure: return FLinearColor(1.0f, 0.82f, 0.3f, 0.95f);
	default: return FLinearColor(0.72f, 0.82f, 0.9f, 0.9f);
	}
}

void ASpaceship::CreateShipHud()
{
	if (ShipHudWidget.IsValid() || !IsValid(Pilot) || !IsPlayerControlled()
		|| !GEngine || !GEngine->GameViewport)
	{
		return;
	}

	if (ShipNavigation)
	{
		ShipNavigation->RefreshContacts(GetActorLocation(), true);
	}

	const TWeakObjectPtr<ASpaceship> WeakThis(this);
	TSharedRef<SOverlay> RootOverlay = SNew(SOverlay);
	RootOverlay->AddSlot()
	[
		SNew(SAPSShipNavigationOverlay)
		.Ship(WeakThis)
	];

	RootOverlay->AddSlot()
		.HAlign(HAlign_Right)
		.VAlign(VAlign_Top)
		.Padding(0.0f, 38.0f, 36.0f, 0.0f)
		[
			SNew(SBackgroundBlur)
			.Visibility_Lambda([WeakThis]()
			{
				return WeakThis.IsValid() && WeakThis->bNavigationPanelVisible
					? EVisibility::HitTestInvisible : EVisibility::Collapsed;
			})
			.BlurStrength(10.0f)
			.BlurRadius(8)
			.LowQualityFallbackBrush(FCoreStyle::Get().GetBrush("WhiteBrush"))
			[
				SNew(SBorder)
				.BorderBackgroundColor(FLinearColor(0.005f, 0.018f, 0.035f, 0.86f))
				.Padding(FMargin(18.0f, 13.0f))
				[
					SNew(STextBlock)
					.Text_Lambda([WeakThis]()
					{
						return WeakThis.IsValid() ? WeakThis->GetNavigationPanelText() : FText::GetEmpty();
					})
					.ColorAndOpacity(FLinearColor(0.72f, 0.9f, 1.0f, 0.96f))
				]
			]
		];

	RootOverlay->AddSlot()
		.HAlign(HAlign_Left)
		.VAlign(VAlign_Bottom)
		.Padding(36.0f, 0.0f, 0.0f, 34.0f)
		[
			SNew(SBackgroundBlur)
			.BlurStrength(12.0f)
			.BlurRadius(10)
			.LowQualityFallbackBrush(FCoreStyle::Get().GetBrush("WhiteBrush"))
			[
				SNew(SBorder)
				.BorderBackgroundColor(FLinearColor(0.005f, 0.018f, 0.035f, 0.88f))
				.Padding(FMargin(20.0f, 14.0f))
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot()
					.AutoHeight()
					[
						SNew(STextBlock)
						.Text_Lambda([WeakThis]()
						{
							return WeakThis.IsValid() ? WeakThis->GetShipStatusText() : FText::GetEmpty();
						})
						.ColorAndOpacity(FLinearColor(0.18f, 0.84f, 1.0f, 1.0f))
					]
					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(0.0f, 7.0f, 0.0f, 0.0f)
					[
						SNew(STextBlock)
						.Text_Lambda([WeakThis]()
						{
							return WeakThis.IsValid() ? WeakThis->GetShipHintText() : FText::GetEmpty();
						})
						.ColorAndOpacity(FLinearColor(0.78f, 0.86f, 0.92f, 0.95f))
					]
				]
			]
		];
	ShipHudWidget = RootOverlay;
	GEngine->GameViewport->AddViewportWidgetContent(ShipHudWidget.ToSharedRef(), 60);
}

void ASpaceship::RemoveShipHud()
{
	if (ShipHudWidget.IsValid() && GEngine && GEngine->GameViewport)
	{
		GEngine->GameViewport->RemoveViewportWidgetContent(ShipHudWidget.ToSharedRef());
	}
	ShipHudWidget.Reset();
}

void ASpaceship::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);
	SetHullSceneLightingExcluded(true);
	if (FlightModel)
	{
		FlightModel->OnPossessed();
	}
	ConfigurePilotFillLight();
	UpdateFlightEnvironment(0.0f, true);
	UpdatePilotFillLightVisibility();
	SetFlightCollisionOptimization(true);
	SetActorTickEnabled(true);
	CreateShipHud();
}

void ASpaceship::UnPossessed()
{
	RemoveShipHud();
	if (PilotFillLight)
	{
		PilotFillLight->SetVisibility(false, true);
	}
	if (PilotFillPointLight)
	{
		PilotFillPointLight->SetVisibility(false, true);
	}
	RestoreFlightPostProcess();
	SetHullSceneLightingExcluded(false);
	if (CameraComponent && bCameraFieldOfViewInitialized)
	{
		CameraComponent->SetFieldOfView(BaseCameraFieldOfView);
	}
	CameraArmLengthRate = SmoothedLogSpeedRate = 0.0f;
	PreviousCameraLogSpeed = -1.0;
	if (!bEngineRunning)
	{
		SetFlightCollisionOptimization(false);
	}
	bIsAccelerating = false;
	bIsDecelerating = false;
	ForwardInput = SideInput = VerticalInput = 0.0f;
	YawInput = PitchInput = RollInput = 0.0f;
	CurrentAngularVelocityDegrees = FVector::ZeroVector;
	Super::UnPossessed();
	SetActorTickEnabled(bEngineRunning);
}

void ASpaceship::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	RemoveShipHud();
	Super::EndPlay(EndPlayReason);
}

void ASpaceship::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	PlayerInputComponent->BindAction("ReleaseControl", IE_Pressed, this, &ASpaceship::ReleaseControl);
	PlayerInputComponent->BindAction("SwitchEngines", IE_Pressed, this, &ASpaceship::SwitchEngines);
	PlayerInputComponent->BindAxis("ThrustForward", this, &ASpaceship::ThrustForward);
	PlayerInputComponent->BindAxis("ThrustSide", this, &ASpaceship::ThrustSide);
	PlayerInputComponent->BindAxis("ThrustVertical", this, &ASpaceship::ThrustVertical);
	PlayerInputComponent->BindAxis("ThrustYaw", this, &ASpaceship::ThrustYaw);
	PlayerInputComponent->BindAxis("ThrustPitch", this, &ASpaceship::ThrustPitch);
	PlayerInputComponent->BindAxis("ThrustRoll", this, &ASpaceship::ThrustRoll);

	PlayerInputComponent->BindAction("IncreaseFlightMode", IE_Pressed, this, &ASpaceship::IncreaseFlightMode);
	PlayerInputComponent->BindAction("DecreaseFlightMode", IE_Pressed, this, &ASpaceship::DecreaseFlightMode);
	PlayerInputComponent->BindAction("AccelerationBoost", IE_Pressed, this, &ASpaceship::StartAccelerationBoost);
	PlayerInputComponent->BindAction("AccelerationBoost", IE_Released, this, &ASpaceship::StopAccelerationBoost);
	PlayerInputComponent->BindAction("DecelerationBoost", IE_Pressed, this, &ASpaceship::StartDecelerationBoost);
	PlayerInputComponent->BindAction("DecelerationBoost", IE_Released, this, &ASpaceship::StopDecelerationBoost);

	// Native bindings keep the sprint entirely C++-driven; no InputSettings or Blueprint edits are required.
	PlayerInputComponent->BindKey(EKeys::One, IE_Pressed, this, &ASpaceship::SelectImpulseEngine);
	PlayerInputComponent->BindKey(EKeys::Two, IE_Pressed, this, &ASpaceship::SelectSpaceWrapEngine);
	PlayerInputComponent->BindKey(EKeys::Three, IE_Pressed, this, &ASpaceship::SelectOffsetEngine);
	PlayerInputComponent->BindKey(EKeys::Four, IE_Pressed, this, &ASpaceship::SelectCruiseBand);
	PlayerInputComponent->BindKey(EKeys::Five, IE_Pressed, this, &ASpaceship::SelectStellarBand);
	PlayerInputComponent->BindKey(EKeys::Zero, IE_Pressed, this, &ASpaceship::SelectAutoBands);
	PlayerInputComponent->BindKey(EKeys::N, IE_Pressed, this, &ASpaceship::ToggleNavigationMarkers);
	PlayerInputComponent->BindKey(EKeys::M, IE_Pressed, this, &ASpaceship::ToggleNavigationPanel);
	PlayerInputComponent->BindKey(EKeys::T, IE_Pressed, this, &ASpaceship::SelectNextNavigationTarget);
	PlayerInputComponent->BindKey(EKeys::V, IE_Pressed, this, &ASpaceship::ToggleNavigationGuides);
}

void ASpaceship::StartAccelerationBoost()
{
	bIsAccelerating = true;
}

void ASpaceship::StopAccelerationBoost()
{
	bIsAccelerating = false;
}

void ASpaceship::StartDecelerationBoost()
{
	bIsDecelerating = true;
}

void ASpaceship::StopDecelerationBoost()
{
	bIsDecelerating = false;
}

void ASpaceship::HandleAccelerationBoost(float Value)
{
	if (Value > 0)
	{
		OnboardComputer->AccelerateBoost(GetWorld()->GetDeltaSeconds());
	}
}

void ASpaceship::HandleDecelerationBoost(float Value)
{
	if (Value > 0)
	{
		OnboardComputer->DecelerateBoost(GetWorld()->GetDeltaSeconds());
	}
}

void ASpaceship::IncreaseFlightMode()
{
	if (FlightModel && FlightModel->IsBandFlightActive())
	{
		FlightModel->StepFlightBand(1);
		return;
	}
	if (!OnboardComputer) return;
	const uint8 Current = static_cast<uint8>(SelectedDriveMode);
	const uint8 Maximum = FMath::Min(
		static_cast<uint8>(GetMaximumDriveModeForClass()),
		static_cast<uint8>(GetMaximumDriveModeForEnvironment()));
	if (Current >= Maximum) return;
	SetDriveMode(static_cast<EShipDriveMode>(Current + 1), false);
}

void ASpaceship::DecreaseFlightMode()
{
	if (FlightModel && FlightModel->IsBandFlightActive())
	{
		FlightModel->StepFlightBand(-1);
		return;
	}
	if (!OnboardComputer) return;
	const uint8 Current = static_cast<uint8>(SelectedDriveMode);
	if (Current <= static_cast<uint8>(EShipDriveMode::Landing)) return;
	SetDriveMode(static_cast<EShipDriveMode>(Current - 1), false);
}

void ASpaceship::SelectImpulseEngine()
{
	if (FlightModel && FlightModel->IsBandFlightActive())
	{
		FlightModel->SetFlightBand(EAPSFlightBand::Maneuver);
		return;
	}
	SetEngineMode(EEngineMode::Impulse, false);
}

void ASpaceship::SelectSpaceWrapEngine()
{
	if (FlightModel && FlightModel->IsBandFlightActive())
	{
		FlightModel->SetFlightBand(EAPSFlightBand::Flight);
		return;
	}
	SetEngineMode(EEngineMode::SpaceWrap, false);
}

void ASpaceship::SelectOffsetEngine()
{
	if (FlightModel && FlightModel->IsBandFlightActive())
	{
		FlightModel->SetFlightBand(EAPSFlightBand::Orbital);
		return;
	}
	SetEngineMode(EEngineMode::Offset, false);
}

void ASpaceship::SelectCruiseBand()
{
	if (FlightModel && FlightModel->IsBandFlightActive())
	{
		FlightModel->SetFlightBand(EAPSFlightBand::Cruise);
	}
}

void ASpaceship::SelectStellarBand()
{
	if (FlightModel && FlightModel->IsBandFlightActive())
	{
		FlightModel->SetFlightBand(EAPSFlightBand::Stellar);
	}
}

void ASpaceship::SelectAutoBands()
{
	if (FlightModel && FlightModel->IsBandFlightActive())
	{
		FlightModel->SetAutoBands();
	}
}

bool ASpaceship::ApplyCustomFlightTranslation(float DeltaTime)
{
	return FlightModel && FlightModel->ApplyTranslation(DeltaTime);
}

FString ASpaceship::GetCustomFlightStatus() const
{
	return FlightModel ? FlightModel->GetStatusText() : FString();
}

FString ASpaceship::GetCustomFlightHint() const
{
	return FlightModel ? FlightModel->GetHintText() : FString();
}

void ASpaceship::ToggleNavigationMarkers()
{
	bNavigationMarkersVisible = !bNavigationMarkersVisible;
	if (bNavigationMarkersVisible && ShipNavigation)
	{
		ShipNavigation->RefreshContacts(GetActorLocation(), true);
	}
}

void ASpaceship::ToggleNavigationPanel()
{
	bNavigationPanelVisible = !bNavigationPanelVisible;
	if (bNavigationPanelVisible && ShipNavigation)
	{
		ShipNavigation->RefreshContacts(GetActorLocation(), true);
	}
}

void ASpaceship::ToggleNavigationGuides()
{
	bNavigationGuidesVisible = !bNavigationGuidesVisible;
}

void ASpaceship::SelectNextNavigationTarget()
{
	if (ShipNavigation)
	{
		ShipNavigation->CycleTarget(1);
	}
}

void ASpaceship::SelectPreviousNavigationTarget()
{
	if (ShipNavigation)
	{
		ShipNavigation->CycleTarget(-1);
	}
}


void ASpaceship::ToggleScale()
{
	if (GeneratedWorld)
	{
		if (bIsScaledUp)
		{
			GeneratedWorld->SetActorScale3D(FVector(1.0f, 1.0f, 1.0f));
			GeneratedWorld->SetActorLocation(GeneratedWorld->GetActorLocation() / 1000000000.0);
			this->SetActorLocation(this->GetActorLocation() / 1000000000.0);
			this->SetActorScale3D(FVector(0.01, 0.01, 0.01));
			/// this->SetSmallHullMesh
			//this->SpaceshipHull = SmallScaleHullMesh;
			//this->SpaceshipHull->SetStaticMesh(SmallScaleHullMesh);
			this->SpringArmComponent->TargetArmLength = 35;
			//this->CameraComponent->FieldOfView = 45;
			//this->CameraComponent->FOV

			GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Green, FString::Printf(TEXT("Scaling to 1")));
		}
		else
		{
			GeneratedWorld->SetActorScale3D(FVector(1000000000.0, 1000000000.0, 1000000000.0));
			GeneratedWorld->SetActorLocation(GeneratedWorld->GetActorLocation() * 1000000000.0);
			this->SetActorLocation(this->GetActorLocation() * 1000000000.0);
			this->SetActorScale3D(FVector(1, 1, 1));
			/// this->SetBigHullMesh
			//this->SpaceshipHull->SetStaticMesh(LargeScaleHullMesh);
			this->SpringArmComponent->TargetArmLength = 1000;
			//this->CameraComponent->FieldOfView = 90;

			GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Green, FString::Printf(TEXT("Scaling to 1000000000")));
		}

		// ����������� ��������� ���������������
		bIsScaledUp = !bIsScaledUp;
	}
}


void ASpaceship::PrintOnboardComputerBasicIformation()
{
	GEngine->AddOnScreenDebugMessage(-1, 0.f, FColor::Red, FString::Printf(TEXT("Current Flight Safe Mode: %s"),
	                                                                       *OnboardComputer->GetEnumValueAsString(
		                                                                       TEXT("EFlightSafeMode"),
		                                                                       (int32)OnboardComputer->FlightSystem.
		                                                                       CurrentFlightSafeMode)));
	GEngine->AddOnScreenDebugMessage(-1, 0.f, FColor::Red, FString::Printf(TEXT("Current Flight Range: %s"),
	                                                                       *OnboardComputer->GetEnumValueAsString(
		                                                                       TEXT("EFlightRange"),
		                                                                       (int32)OnboardComputer->FlightSystem.
		                                                                       CurrentFlightRange)));
	GEngine->AddOnScreenDebugMessage(-1, 0.f, FColor::Red, FString::Printf(TEXT("Current Flight Status: %s"),
	                                                                       *OnboardComputer->GetEnumValueAsString(
		                                                                       TEXT("EFlightStatus"),
		                                                                       (int32)OnboardComputer->FlightSystem.
		                                                                       CurrentFlightStatus)));
	GEngine->AddOnScreenDebugMessage(-1, 0.f, FColor::Red, FString::Printf(TEXT("Current Engine State:	%s"),
	                                                                       *OnboardComputer->GetEnumValueAsString(
		                                                                       TEXT("EEngineState"),
		                                                                       (int32)OnboardComputer->EngineSystem.
		                                                                       CurrentEngineState)));
	GEngine->AddOnScreenDebugMessage(-1, 0.f, FColor::Red, FString::Printf(TEXT("Current Engine Type:	%s"),
	                                                                       *OnboardComputer->GetEnumValueAsString(
		                                                                       TEXT("EEngineMode"),
		                                                                       (int32)OnboardComputer->EngineSystem.
		                                                                       CurrentEngineMode)));
	GEngine->AddOnScreenDebugMessage(-1, 0.f, FColor::Red, FString::Printf(TEXT("Current Flight Type:	%s"),
	                                                                       *OnboardComputer->GetEnumValueAsString(
		                                                                       TEXT("EFlightType"),
		                                                                       (int32)OnboardComputer->FlightSystem.
		                                                                       CurrentFlightType)));
	GEngine->AddOnScreenDebugMessage(-1, 0.f, FColor::Red, FString::Printf(TEXT("Current Flight Mode:	%s"),
	                                                                       *OnboardComputer->GetEnumValueAsString(
		                                                                       TEXT("EFlightMode"),
		                                                                       (int32)OnboardComputer->FlightSystem.
		                                                                       CurrentFlightMode)));

	// Telemetry
	FVector ActorLocation = GetActorLocation();
	FVector ActorVelocity = GetVelocity();
	double ActorSpeed = ActorVelocity.Size();

	// Displaying them on screen
	GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Green,
	                                 FString::Printf(TEXT("Actor Location: %s"), *ActorLocation.ToString()));
	GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Green, FString::Printf(TEXT("Actor Velocity: %f"), ActorSpeed));
	GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Green,
	                                 FString::Printf(
		                                 TEXT("Thrust Force: %f"), OnboardComputer->GetEngineThrustForce()));
	GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Green,
	                                 FString::Printf(
		                                 TEXT("Hull Angular Damping: %f"), SpaceshipHull->GetAngularDamping()));
	GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Green,
	                                 FString::Printf(
		                                 TEXT("Hull Linear Damping: %f"), SpaceshipHull->GetLinearDamping()));
}

void ASpaceship::SwitchCamera()
{
	GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Blue, FString::Printf(TEXT("Camera")));
}

void ASpaceship::SwitchEngines()
{
	bEngineRunning = !bEngineRunning;
	if (bEngineRunning)
	{
		UpdateFlightEnvironment(0.0f, true);
		EnforceDriveModeForEnvironment();
		RequestEngineModeForFlightMode(true);
	}
	ApplyEngineState();
	SetActorTickEnabled(IsValid(Pilot) || bEngineRunning);
	GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Green,
	                                 FString::Printf(
		                                 TEXT("Engine running: %s"), bEngineRunning ? TEXT("true") : TEXT("false")));
}

void ASpaceship::ThrustForward(float Value)
{
	ForwardInput = Value;
}

void ASpaceship::ThrustSide(float Value)
{
	SideInput = Value;
	#if 0 // Legacy per-axis movement is intentionally replaced by ApplyFlightInput.
	if (!bEngineRunning || !OnboardComputer || !SpaceshipHull || FMath::Abs(Value) < KINDA_SMALL_NUMBER) return;

	const FVector Direction = ForwardVector->GetRightVector();
	const float DeltaTime = GetWorld()->GetDeltaSeconds();
	if (OffsetSystem && OnboardComputer->EngineSystem.CurrentEngineMode == EEngineMode::SpaceWrap)
	{
		// �������� StarSystem
		OffsetSystem->AddActorWorldOffset(-Direction * Value * OnboardComputer->GetEngineThrustForce() * DeltaTime);
	}
	else if (OnboardComputer->EngineSystem.CurrentEngineMode == EEngineMode::Impulse && SpaceshipHull->IsSimulatingPhysics())
	{
		// �������� ������ ������ �������.
		SpaceshipHull->AddForce(Direction * Value * OnboardComputer->GetEngineThrustForce(), NAME_None, true);
	}
	else if (OnboardComputer->EngineSystem.CurrentEngineMode == EEngineMode::Offset)
	{
		const FVector Offset = Direction * Value * OnboardComputer->GetEngineThrustForce() * DeltaTime;
		SpaceshipHull->AddWorldOffset(Offset, true);
	}
	#endif
}

void ASpaceship::ThrustVertical(float Value)
{
	VerticalInput = Value;
	#if 0 // Legacy per-axis movement is intentionally replaced by ApplyFlightInput.
	if (!bEngineRunning || !OnboardComputer || !SpaceshipHull || FMath::Abs(Value) < KINDA_SMALL_NUMBER) return;

	const FVector Direction = ForwardVector->GetUpVector();
	const float DeltaTime = GetWorld()->GetDeltaSeconds();
	if (OffsetSystem && OnboardComputer->EngineSystem.CurrentEngineMode == EEngineMode::SpaceWrap)
	{
		// �������� StarSystem
		OffsetSystem->AddActorWorldOffset(-Direction * Value * OnboardComputer->GetEngineThrustForce() * DeltaTime);
	}
	else if (OnboardComputer->EngineSystem.CurrentEngineMode == EEngineMode::Impulse && SpaceshipHull->IsSimulatingPhysics())
	{
		// �������� ������ ������ �������.
		SpaceshipHull->AddForce(Direction * Value * OnboardComputer->GetEngineThrustForce(), NAME_None, true);
	}
	else if (OnboardComputer->EngineSystem.CurrentEngineMode == EEngineMode::Offset)
	{
		const FVector Offset = Direction * Value * OnboardComputer->GetEngineThrustForce() * DeltaTime;
		SpaceshipHull->AddWorldOffset(Offset, true);
	}
	#endif
}

void ASpaceship::ThrustYaw(float Value)
{
	YawInput = Value;
	#if 0 // Legacy per-axis rotation is intentionally replaced by ApplyRotationInput.
	if (!bEngineRunning || !OnboardComputer || !SpaceshipHull || FMath::Abs(Value) < KINDA_SMALL_NUMBER) return;

	const float RotationAmount = Value * RotationSpeedDegreesPerSecond * GetWorld()->GetDeltaSeconds();
	if (OnboardComputer->EngineSystem.CurrentEngineMode == EEngineMode::SpaceWrap || OnboardComputer->EngineSystem.
		CurrentEngineMode == EEngineMode::Offset)
	{
		FRotator NewRotation = FRotator(0, RotationAmount, 0);
		AddActorLocalRotation(NewRotation);
	}
	else if (OnboardComputer->EngineSystem.CurrentEngineMode == EEngineMode::Impulse)
	{
		FVector TorqueVector = ForwardVector->GetUpVector() * Value * ImpulseRotationAcceleration;
		SpaceshipHull->AddTorqueInRadians(TorqueVector, NAME_None, true);
	}
	#endif
}

void ASpaceship::ThrustPitch(float Value)
{
	PitchInput = Value;
	#if 0 // Legacy per-axis rotation is intentionally replaced by ApplyRotationInput.
	if (!bEngineRunning || !OnboardComputer || !SpaceshipHull || FMath::Abs(Value) < KINDA_SMALL_NUMBER) return;

	const float RotationAmount = Value * RotationSpeedDegreesPerSecond * GetWorld()->GetDeltaSeconds();
	if (OnboardComputer->EngineSystem.CurrentEngineMode == EEngineMode::SpaceWrap || OnboardComputer->EngineSystem.
		CurrentEngineMode == EEngineMode::Offset)
	{
		FRotator NewRotation = FRotator(RotationAmount, 0, 0);
		AddActorLocalRotation(NewRotation);
	}
	else if (OnboardComputer->EngineSystem.CurrentEngineMode == EEngineMode::Impulse)
	{
		FVector TorqueVector = ForwardVector->GetRightVector() * Value * ImpulseRotationAcceleration;
		SpaceshipHull->AddTorqueInRadians(TorqueVector, NAME_None, true);
	}
	#endif
}

void ASpaceship::ThrustRoll(float Value)
{
	RollInput = Value;
	#if 0 // Legacy per-axis rotation is intentionally replaced by ApplyRotationInput.
	if (!bEngineRunning || !OnboardComputer || !SpaceshipHull || FMath::Abs(Value) < KINDA_SMALL_NUMBER) return;

	const float RotationAmount = Value * RotationSpeedDegreesPerSecond * GetWorld()->GetDeltaSeconds();

	if (OnboardComputer->EngineSystem.CurrentEngineMode == EEngineMode::SpaceWrap || OnboardComputer->EngineSystem.
		CurrentEngineMode == EEngineMode::Offset)
	{
		FRotator NewRotation = FRotator(0, 0, RotationAmount);
		AddActorLocalRotation(NewRotation);
	}
	else if (OnboardComputer->EngineSystem.CurrentEngineMode == EEngineMode::Impulse)
	{
		FVector TorqueVector = ForwardVector->GetForwardVector() * Value * ImpulseRotationAcceleration;
		SpaceshipHull->AddTorqueInRadians(TorqueVector, NAME_None, true);
	}
	#endif
}

void ASpaceship::SetPilot(AGravityCharacterPawn* NewPilot)
{
}

USceneComponent* ASpaceship::GetPilotSeatComponent() const
{
	return PilotChair ? PilotChair : Super::GetPilotSeatComponent();
}

FTransform ASpaceship::GetPilotExitTransform() const
{
	return PilotExitPoint ? PilotExitPoint->GetComponentTransform() : Super::GetPilotExitTransform();
}

void ASpaceship::ComputeProximity()
{
	FVector ShipLocation = this->GetActorLocation();
	double ClosestDistance = DBL_MAX;
	double ClosestAffectionDistance = DBL_MAX;
	CurrentZonesInfluence.Empty();

	GEngine->AddOnScreenDebugMessage(-1, 0.f, FColor::Yellow,
	                                 FString::Printf(TEXT("Navigatable proximity: %d"), WorldNavigatableActors.Num()));

	for (AWorldActor* Actor : WorldNavigatableActors)
	{
		FVector ActorLocation = Actor->GetActorLocation();
		double AffectionRadius = Actor->AffectionRadiusKM * 100000;
		double SurfaceRadius = Actor->RadiusKM * 100000;
		double DistanceToActor = (ActorLocation - ShipLocation).Size() - SurfaceRadius;
		double DistanceToAffectionZone = (ActorLocation - ShipLocation).Size();

		// ������� ���������� � ����������
		//FString DistanceMessage = FString::Printf(TEXT("Distance to actor %s is %f km"), *Actor->GetName(), DistanceToActor / 100000);
		//GEngine->AddOnScreenDebugMessage(-1, 0.f, FColor::Yellow, DistanceMessage);

		// ���� ������� ��������� � ���� �������� ����� (�.�. ���������� ������ ��� ����� ����)
		if (DistanceToAffectionZone <= AffectionRadius)
		{
			CurrentZonesInfluence.Add(Actor);

			if (DistanceToAffectionZone < ClosestAffectionDistance)
			{
				ClosestAffectionDistance = DistanceToAffectionZone;
				AffectedActor = Actor;
			}
		}

		if (DistanceToActor < ClosestDistance)
		{
			ClosestDistance = DistanceToActor;
			ClosestActor = Actor;
		}
	}

	GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Green,
	                                 FString::Printf(TEXT("CurrentZonesInfluence: %d"), CurrentZonesInfluence.Num()));
	if (CurrentZonesInfluence.Num() > 0)
	{
		for (AWorldActor* Actor : CurrentZonesInfluence)
		{
			FString RadiusMessage = FString::Printf(TEXT("Ship is affected by %s"), *Actor->GetName());
			GEngine->AddOnScreenDebugMessage(-1, 0.f, FColor::Green, RadiusMessage);
		}
		//OnboardComputer->ComputeFlightStatus(AffectedActor);
	}
	else
	{
		AffectedActor = nullptr;
	}
}

void ASpaceship::UpdateNavigatableActors()
{
	GEngine->AddOnScreenDebugMessage(-1, 1.0f, FColor::Red, FString::Printf(TEXT("UpdateNavigatableActors!!!")));
}

void ASpaceship::CheckFlightModeChange()
{
	// ���� ����� ������ ���������
	if (OnboardComputer->FlightSystem.CurrentFlightMode != LastFlightMode)
	{
		// ��������� ������ �������
		UpdateNavigatableActors();

		// ��������� LastFlightMode
		LastFlightMode = OnboardComputer->FlightSystem.CurrentFlightMode;
	}
}

UStaticMeshComponent* ASpaceship::GetSpaceshipHull()
{
	return SpaceshipHull;
}
