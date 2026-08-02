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
#include "APS_ALPHA/Generation/AstroGenerator.h"
#include "APS_ALPHA/Pawns/Characters/GravityCharacterPawn.h"
#include "APS_ALPHA/Pawns/Characters/GravityDetectorComponent.h"
#include "APS_ALPHA/Pawns/Spaceships/ShipNavigationComponent.h"
#include "Camera/CameraComponent.h"
#include "Components/BoxComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/PlayerController.h"
#include "Camera/PlayerCameraManager.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "InputCoreTypes.h"
#include "Kismet/GameplayStatics.h"
#include "Rendering/DrawElements.h"
#include "Styling/CoreStyle.h"
#include "Widgets/SLeafWidget.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Layout/SBackgroundBlur.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SConstraintCanvas.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SOverlay.h"
#include "Widgets/SViewport.h"
#include "Widgets/Text/STextBlock.h"
#include "Components/MeshComponent.h"

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
	const bool bForceGeneratedConfiguration = bGenerateSimpleHullCollision
		|| Tags.Contains(TEXT("APS.GeneratedShip"));

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

	UpdateFlightEnvironment(DeltaTime);
	if (ShipNavigation && IsValid(Pilot) && (bNavigationMarkersVisible || bNavigationPanelVisible))
	{
		ShipNavigation->RefreshContacts(GetActorLocation());
	}

	const double TargetBoost = bEngineRunning && bIsAccelerating ? ActiveClassPreset.MaximumBoost : 1.0;
	const double BoostResponse = bIsAccelerating
		? ActiveClassPreset.BoostGrowthPerSecond
		: BoostRecoverySpeed;
	CurrentBoostMultiplier = FMath::FInterpConstantTo(
		CurrentBoostMultiplier, TargetBoost, DeltaTime, BoostResponse);

	AdvanceEngineModeTransition(DeltaTime);
	ApplyFlightInput(DeltaTime);
	ApplyRotationInput(DeltaTime);
	UpdateAdaptiveFlightCamera(DeltaTime);
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

bool ASpaceship::IsGeneratedShipMeshAsset(const UStaticMesh* Mesh)
{
	return IsValid(Mesh)
		&& Mesh->GetPathName().Contains(TEXT("/Game/APS/APS_ALPHA/Assets/AI_Shpis/"), ESearchCase::IgnoreCase);
}

bool ASpaceship::IsGeneratedShipSkeletalMeshAsset(const USkeletalMesh* Mesh)
{
	return IsValid(Mesh)
		&& Mesh->GetPathName().Contains(TEXT("/Game/APS/APS_ALPHA/Assets/AI_Shpis/"), ESearchCase::IgnoreCase);
}

void ASpaceship::ConfigureFlightReferenceFromHull(UPrimitiveComponent* Hull, const FVector& LocalExtent)
{
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

void ASpaceship::ConfigureFromHull()
{
	UPrimitiveComponent* MainMesh = GetPrimaryHullComponent();
	if (MainMesh)
	{
		MainMesh->UpdateBounds();
		FVector LocalMin;
		FVector LocalMax;
		if (GetPrimaryHullLocalBounds(MainMesh, LocalMin, LocalMax))
		{
			ConfigureFlightReferenceFromHull(MainMesh, (LocalMax - LocalMin) * 0.5);
		}
		if (bInferSizeClassFromHull)
		{
			const FVector Size = MainMesh->Bounds.BoxExtent * 2.0;
			SizeClass = InferSizeClassFromLength(Size.GetMax());
		}
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
	EnvironmentDetectionElapsed = 0.0f;

	const EShipFlightEnvironment PreviousEnvironment = CurrentFlightEnvironment;
	AActor* PreviousSource = ActiveGravitySource.Get();
	if (FlightGravityDetector)
	{
		FlightGravityDetector->RunGravityCheckForActor(this);
		ActiveGravitySource = FlightGravityDetector->GravityTargetActor;
		ActiveGravityDirection = FlightGravityDetector->GetGravityDirectionAtLocation(GetActorLocation());
	}
	else
	{
		ActiveGravitySource = nullptr;
		ActiveGravityDirection = FVector::ZeroVector;
	}

	ActiveGravityAcceleration = 0.0;
	CurrentFlightEnvironment = EShipFlightEnvironment::DeepSpace;
	if (ActiveGravitySource && !ActiveGravityDirection.IsNearlyZero())
	{
		ActiveGravityAcceleration = 980.0;
		if (const APlanetaryBody* Planet = Cast<APlanetaryBody>(ActiveGravitySource))
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
			ActiveGravityAcceleration = FMath::Clamp(SurfaceAcceleration * DistanceFalloff, 5.0, 3000.0);

			if (IsNearGravitySurface(ActiveGravityDirection))
			{
				CurrentFlightEnvironment = EShipFlightEnvironment::Surface;
			}
			else if (AltitudeKm <= AtmosphereHeightKm)
			{
				CurrentFlightEnvironment = EShipFlightEnvironment::Atmosphere;
			}
			else
			{
				CurrentFlightEnvironment = EShipFlightEnvironment::GravityWell;
			}
		}
		else
		{
			CurrentFlightEnvironment = IsNearGravitySurface(ActiveGravityDirection)
				? EShipFlightEnvironment::Surface : EShipFlightEnvironment::GravityWell;
		}
	}

	if (PreviousEnvironment != CurrentFlightEnvironment || PreviousSource != ActiveGravitySource.Get())
	{
		EnforceDriveModeForEnvironment();
		UE_LOG(LogTemp, Log, TEXT("[APS.Ships] Environment ship=%s environment=%s gravity=%s acceleration=%.2f m/s2"),
			*GetName(), *GetFlightEnvironmentName(), *GetGravitySourceName(), ActiveGravityAcceleration / 100.0);
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
	// immediately after every mode change.
	const double ClassReferenceSpeed = FMath::Max(ActiveClassPreset.MaxImpulseSpeed, 1.0);
	const float TargetCameraAlpha = FMath::Clamp(static_cast<float>(
		FMath::Log2(1.0 + Speed / ClassReferenceSpeed) / 8.0), 0.0f, 1.0f);
	SmoothedCameraSpeedAlpha = FMath::FInterpTo(
		SmoothedCameraSpeedAlpha, TargetCameraAlpha, DeltaTime, 2.0f);
	const float CameraAlpha = SmoothedCameraSpeedAlpha;

	// Hull bounds define the baseline. Speed adds a restrained pull-back while FOV
	// and positional lag provide the remaining sensation without losing the ship.
	const float TargetArmLength = BaseCameraArmLength * FMath::Lerp(1.0f, 1.22f, CameraAlpha);
	SpringArmComponent->TargetArmLength = FMath::FInterpTo(
		SpringArmComponent->TargetArmLength, TargetArmLength, DeltaTime, 2.6f);
	// Positional lag is intentionally disabled. At astronomical velocities a
	// spring-arm positional integrator alternates between a huge error and a huge
	// correction, while smoothly interpolated arm length/FOV retain the chase feel.
	SpringArmComponent->bEnableCameraLag = false;
	SpringArmComponent->CameraRotationLagSpeed = FMath::Lerp(7.0f, 12.0f, CameraAlpha);
	SpringArmComponent->CameraLagMaxDistance = 0.0f;
	if (CameraComponent)
	{
		const float TargetFieldOfView = BaseCameraFieldOfView + CameraAlpha * 12.0f;
		CameraComponent->SetFieldOfView(FMath::FInterpTo(
			CameraComponent->FieldOfView, TargetFieldOfView, DeltaTime, 2.6f));
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
		RebuildSimpleHullCollision();
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
	AddActorWorldOffset(KinematicVelocity * DeltaTime, bSweep, &Hit, ETeleportType::None);
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
	return FText::FromString(FString::Printf(
		TEXT("SHIP %s   |   ENGINE %s\nPOWER %d/%d: %s   |   ENVIRONMENT: %s\nGRAVITY: %s (%.2f m/s2, %s)   |   %s   |   BOOST x%.2f"),
		*GetSizeClassName(), *GetEngineModeName(), ModeNumber, MaximumModeNumber, *GetDriveModeName(),
		*GetFlightEnvironmentName(), *GetGravitySourceName(), ActiveGravityAcceleration / 100.0, *GravityAssist,
		*SpeedText, CurrentBoostMultiplier));
}

FText ASpaceship::GetShipHintText() const
{
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
	if (!FocusPlanet || CurrentFlightEnvironment == EShipFlightEnvironment::DeepSpace)
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
	if (Contact->Type == EShipNavigationContactType::Planet)
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
	if (!PlayerController || !GEngine || !GEngine->GameViewport
		|| !UWidgetLayoutLibrary::ProjectWorldLocationToWidgetPosition(
			PlayerController, WorldLocation, OutScreenPosition, true))
	{
		return false;
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
	FVector2D& OutLabelPosition) const
{
	if (!ShipNavigation || !ShouldShowNavigationMarker(ContactIndex)
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
		if (ShouldShowNavigationMarker(Index) && ProjectNavigationContactToScreen(Index, ProjectedAnchor))
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
		if (Placement.Index == ContactIndex)
		{
			OutAnchorPosition = Placement.Anchor;
			OutLabelPosition = Chosen;
			return true;
		}
	}
	return false;
}

bool ASpaceship::IsNavigationMarkerRightEdgeFlag(int32 ContactIndex) const
{
	FVector2D Anchor;
	FVector2D Label;
	if (!GetNavigationMarkerLayout(ContactIndex, Anchor, Label))
	{
		return false;
	}
	const float LeftEdgeDistance = FMath::Abs(Anchor.X - (Label.X + 1.5f));
	const float RightEdgeDistance = FMath::Abs(
		Anchor.X - (Label.X + APSNavigationHud::MarkerWidth - 1.5f));
	return RightEdgeDistance < LeftEdgeDistance;
}

int32 ASpaceship::PaintNavigationOverlay(const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
	FSlateWindowElementList& OutDrawElements, int32 LayerId) const
{
	if (!ShipNavigation)
	{
		return LayerId;
	}

	const FPaintGeometry PaintGeometry = AllottedGeometry.ToPaintGeometry();
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
			const bool bValid = ProjectWorldLocationToNavigationScreen(WorldPoint, ScreenPoint, false);
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
		const int32 ContactCount = FMath::Min(ShipNavigation->GetContacts().Num(), MaximumNavigationMarkers);
		for (int32 ContactIndex = 0; ContactIndex < ContactCount; ++ContactIndex)
		{
			FVector2D Anchor;
			FVector2D Label;
			if (!GetNavigationMarkerLayout(ContactIndex, Anchor, Label)) continue;
			const FLinearColor Color = GetNavigationMarkerColor(ContactIndex);
			const bool bSelected = ContactIndex == ShipNavigation->GetSelectedContactIndex();
			const bool bRightEdgeFlag = IsNavigationMarkerRightEdgeFlag(ContactIndex);
			const FVector2D FlagPoleEnd(
				Label.X + (bRightEdgeFlag ? APSNavigationHud::MarkerWidth - 1.5f : 1.5f),
				Label.Y + APSNavigationHud::MarkerHeight);
			DrawScreenLine({Anchor, FlagPoleEnd}, FLinearColor(Color.R, Color.G, Color.B,
				bSelected ? 0.82f : 0.42f), bSelected ? 1.15f : 0.65f, LayerId + 2);
			const float CrossExtent = bSelected ? 4.5f : 2.75f;
			DrawScreenLine({Anchor + FVector2D(-CrossExtent, 0.0f), Anchor + FVector2D(CrossExtent, 0.0f)},
				Color, bSelected ? 1.35f : 0.8f, LayerId + 2);
			DrawScreenLine({Anchor + FVector2D(0.0f, -CrossExtent), Anchor + FVector2D(0.0f, CrossExtent)},
				Color, bSelected ? 1.35f : 0.8f, LayerId + 2);
		}
	}
	return LayerId + 2;
}

FLinearColor ASpaceship::GetNavigationMarkerColor(int32 ContactIndex) const
{
	if (ShipNavigation && ContactIndex == ShipNavigation->GetSelectedContactIndex())
	{
		return FLinearColor(1.0f, 0.68f, 0.16f, 1.0f);
	}
	const FShipNavigationContact* Contact = ShipNavigation ? ShipNavigation->GetContact(ContactIndex) : nullptr;
	if (!Contact) return FLinearColor::Transparent;
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
	TSharedRef<SConstraintCanvas> MarkerCanvas = SNew(SConstraintCanvas);
	for (int32 MarkerIndex = 0; MarkerIndex < MaximumNavigationMarkers; ++MarkerIndex)
	{
		MarkerCanvas->AddSlot()
		.Offset_Lambda([WeakThis, MarkerIndex]()
		{
			FVector2D Anchor;
			FVector2D Label(-10000.0, -10000.0);
			if (WeakThis.IsValid()) WeakThis->GetNavigationMarkerLayout(MarkerIndex, Anchor, Label);
			return FMargin(Label.X, Label.Y, APSNavigationHud::MarkerWidth, APSNavigationHud::MarkerHeight);
		})
		[
			SNew(SBox)
			.WidthOverride(APSNavigationHud::MarkerWidth)
			.HeightOverride(APSNavigationHud::MarkerHeight)
			.Visibility_Lambda([WeakThis, MarkerIndex]()
			{
				FVector2D Anchor;
				FVector2D Label;
				return WeakThis.IsValid() && WeakThis->bNavigationMarkersVisible
					&& MarkerIndex < WeakThis->MaximumNavigationMarkers
					&& WeakThis->GetNavigationMarkerLayout(MarkerIndex, Anchor, Label)
					? EVisibility::HitTestInvisible : EVisibility::Collapsed;
			})
			[
				SNew(SBorder)
				.BorderBackgroundColor_Lambda([WeakThis, MarkerIndex]()
				{
					const FLinearColor Accent = WeakThis.IsValid()
						? WeakThis->GetNavigationMarkerColor(MarkerIndex) : FLinearColor::Transparent;
					const bool bSelected = WeakThis.IsValid() && WeakThis->ShipNavigation
						&& MarkerIndex == WeakThis->ShipNavigation->GetSelectedContactIndex();
					return FSlateColor(FLinearColor(Accent.R * 0.055f, Accent.G * 0.055f,
						Accent.B * 0.055f, bSelected ? 0.91f : 0.68f));
				})
				.Padding(0.0f)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
					.AutoWidth()
					[
						SNew(SBox)
						.WidthOverride(3.0f)
						[
							SNew(SImage)
							.Image(FCoreStyle::Get().GetBrush("WhiteBrush"))
							.ColorAndOpacity_Lambda([WeakThis, MarkerIndex]()
							{
								if (!WeakThis.IsValid() || WeakThis->IsNavigationMarkerRightEdgeFlag(MarkerIndex))
								{
									return FLinearColor::Transparent;
								}
								return WeakThis->GetNavigationMarkerColor(MarkerIndex);
							})
						]
					]
					+ SHorizontalBox::Slot()
					.FillWidth(1.0f)
					.Padding(8.0f, 4.0f, 7.0f, 3.0f)
					[
						SNew(STextBlock)
						.Text_Lambda([WeakThis, MarkerIndex]()
						{
							return WeakThis.IsValid()
								? WeakThis->GetNavigationMarkerText(MarkerIndex) : FText::GetEmpty();
						})
						.Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
						.ColorAndOpacity_Lambda([WeakThis, MarkerIndex]()
						{
							if (!WeakThis.IsValid()) return FSlateColor(FLinearColor::Transparent);
							const FLinearColor Accent = WeakThis->GetNavigationMarkerColor(MarkerIndex);
							return FSlateColor(FLinearColor(
								FMath::Lerp(Accent.R, 0.9f, 0.38f),
								FMath::Lerp(Accent.G, 0.94f, 0.38f),
								FMath::Lerp(Accent.B, 0.98f, 0.38f), 0.96f));
						})
					]
					+ SHorizontalBox::Slot()
					.AutoWidth()
					[
						SNew(SBox)
						.WidthOverride(3.0f)
						[
							SNew(SImage)
							.Image(FCoreStyle::Get().GetBrush("WhiteBrush"))
							.ColorAndOpacity_Lambda([WeakThis, MarkerIndex]()
							{
								if (!WeakThis.IsValid() || !WeakThis->IsNavigationMarkerRightEdgeFlag(MarkerIndex))
								{
									return FLinearColor::Transparent;
								}
								return WeakThis->GetNavigationMarkerColor(MarkerIndex);
							})
						]
					]
				]
			]
		];
	}
	RootOverlay->AddSlot()[MarkerCanvas];

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
	UpdateFlightEnvironment(0.0f, true);
	SetFlightCollisionOptimization(true);
	SetActorTickEnabled(true);
	CreateShipHud();
}

void ASpaceship::UnPossessed()
{
	RemoveShipHud();
	if (CameraComponent && bCameraFieldOfViewInitialized)
	{
		CameraComponent->SetFieldOfView(BaseCameraFieldOfView);
	}
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
	if (!OnboardComputer) return;
	const uint8 Current = static_cast<uint8>(SelectedDriveMode);
	if (Current <= static_cast<uint8>(EShipDriveMode::Landing)) return;
	SetDriveMode(static_cast<EShipDriveMode>(Current - 1), false);
}

void ASpaceship::SelectImpulseEngine()
{
	SetEngineMode(EEngineMode::Impulse, false);
}

void ASpaceship::SelectSpaceWrapEngine()
{
	SetEngineMode(EEngineMode::SpaceWrap, false);
}

void ASpaceship::SelectOffsetEngine()
{
	SetEngineMode(EEngineMode::Offset, false);
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
