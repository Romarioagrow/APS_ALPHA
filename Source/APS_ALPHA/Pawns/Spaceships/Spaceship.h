#pragma once

#include <Components/SphereComponent.h>
#include "SpaceshipOnboardComputer.h"
#include "CoreMinimal.h"
#include "Spacecraft.h"
#include "APS_ALPHA/Gameplay/Gravity/GravitySource.h"
#include "Spaceship.generated.h"

class APlanet;
class AStarSystem;
class AStarCluster;
class UCameraComponent;
class USpringArmComponent;
class USkeletalMesh;
class USkeletalMeshComponent;
class UPrimitiveComponent;
class UGravityDetectorComponent;
class UShipNavigationComponent;
class AWorldActor;
class SWidget;
class SAPSShipNavigationOverlay;
class FSlateWindowElementList;
struct FGeometry;
class FSlateRect;
class UBoxComponent;

/** Gameplay size class. The display names intentionally match the in-world ship taxonomy. */
UENUM(BlueprintType)
enum class ESpaceshipSizeClass : uint8
{
	XXS UMETA(DisplayName = "XXS"),
	XS UMETA(DisplayName = "XS"),
	S UMETA(DisplayName = "S"),
	M UMETA(DisplayName = "M"),
	L UMETA(DisplayName = "L"),
	XL UMETA(DisplayName = "XL"),
	XXL UMETA(DisplayName = "XXL"),
	Titan UMETA(DisplayName = "T")
};

/** Player-selected propulsion scale. Environmental flight state is detected separately. */
UENUM(BlueprintType)
enum class EShipDriveMode : uint8
{
	Landing UMETA(DisplayName = "Landing"),
	Local UMETA(DisplayName = "Local"),
	Orbital UMETA(DisplayName = "Orbital"),
	Interplanetary UMETA(DisplayName = "Interplanetary"),
	Stellar UMETA(DisplayName = "Stellar"),
	Interstellar UMETA(DisplayName = "Interstellar")
};

/** Actual physical environment around the ship, independent of the selected drive. */
UENUM(BlueprintType)
enum class EShipFlightEnvironment : uint8
{
	Surface UMETA(DisplayName = "Surface"),
	Atmosphere UMETA(DisplayName = "Atmosphere"),
	GravityWell UMETA(DisplayName = "Gravity Well"),
	DeepSpace UMETA(DisplayName = "Deep Space")
};

/** Native, data-oriented handling preset used by every legacy BP and generated mesh ship. */
USTRUCT(BlueprintType)
struct FSpaceshipClassPreset
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	double ImpulseAcceleration{900.0};

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	double MaxImpulseSpeed{160000.0};

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	double RotationSpeed{55.0};

	/** Maximum rate at which angular velocity changes, in degrees per second squared. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	double AngularAcceleration{75.0};

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	double LinearDamping{0.18};

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	double AngularDamping{2.5};

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	double MaximumBoost{4.0};

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	double BoostGrowthPerSecond{0.9};

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	bool bSupportsSpaceWrap{true};

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	bool bSupportsOffset{true};

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	bool bUsesPhysicalImpulse{true};

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	bool bHasInteriorByDefault{true};

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	EFlightMode MaximumFlightMode{EFlightMode::Interstellar};
};

struct FActorDistance
{
	AWorldActor* Actor;

	double Distance;

	FActorDistance(AWorldActor* InActor, double InDistance) : Actor(InActor), Distance(InDistance)
	{
	}
};

// To component
struct ZoneData
{
	AWorldActor* Actor;

	double Distance;

	ZoneData(AWorldActor* InActor, double InDistance)
		: Actor(InActor)
		  , Distance(InDistance)
	{
	}
};


UCLASS()
class APS_ALPHA_API ASpaceship : public ASpacecraft, public IGravitySource
{
	GENERATED_BODY()

public:
	// Определение типа делегата
	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnFlightModeChangedDelegate);

	// Определение экземпляров делегата
	FOnFlightModeChangedDelegate OnInterstellarMode;

	FOnFlightModeChangedDelegate OnStellarMode;

	FOnFlightModeChangedDelegate OnInterplanetaryMode;

	// Объявление функций
	UFUNCTION()
	void UpdateNavigatableActorsForInterstellar();

	UFUNCTION()
	void UpdateNavigatableActorsForStellar();

	UFUNCTION()
	void UpdateNavigatableActorsForInterplanetary();

	ASpaceship();

	virtual void OnConstruction(const FTransform& Transform) override;

	virtual void Tick(float DeltaTime) override;

	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	virtual void PossessedBy(AController* NewController) override;

	virtual void UnPossessed() override;

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

protected:
	virtual void BeginPlay() override;

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Camera)
	USpringArmComponent* SpringArmComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Camera)
	UCameraComponent* CameraComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Meshes")
	UStaticMesh* SmallScaleHullMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Meshes")
	UStaticMesh* LargeScaleHullMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Flight Mode")
	EFlightMode LastFlightMode = EFlightMode::Basic;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Components")
	USphereComponent* SphereCollisionComponent;

	/** Reuses the character gravity selection rules without ticking on parked ships. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UGravityDetectorComponent* FlightGravityDetector;

	/** Native navigation bridge over live actors and generated star instances. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UShipNavigationComponent* ShipNavigation;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Components")
	UStaticMeshComponent* SpaceshipHull;

	/** Optional visual hull for AI-generated skeletal ships (for example Pack_1/24). */
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Components")
	USkeletalMeshComponent* SkeletalSpaceshipHull;

	/** Cheap query-only bounds used by the character's camera trace. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UBoxComponent* InteractionBoundsComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Components")
	UStaticMeshComponent* ForwardVector;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Components")
	USceneComponent* PilotChair;

	/** Safe character return point. Existing ship Blueprints inherit it automatically. */
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Components")
	USceneComponent* PilotExitPoint;

	/** Shared class preset. Existing BP_Spaceship_M* assets inherit M without asset edits. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ship|Class")
	ESpaceshipSizeClass SizeClass{ESpaceshipSizeClass::M};

	/** Runtime mesh adapters enable this so full-scale generated hulls choose their class from bounds. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ship|Class")
	bool bInferSizeClassFromHull{false};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ship|Interior")
	bool bHasInterior{true};

	/** Temporary exterior interaction keeps generated hollow hulls playable. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ship|Interior")
	bool bAllowExteriorInteraction{true};

	/** Hollow generated hulls keep this off until an interior gravity volume exists. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ship|Gravity")
	bool bProvidesArtificialGravity{true};

	/** Replaces unsuitable generated-mesh collision with a cheap tapered box hull. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ship|Collision")
	bool bGenerateSimpleHullCollision{false};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ship|Collision", meta = (ClampMin = "3", ClampMax = "9"))
	int32 SimpleCollisionSliceCount{5};

	/** Existing detailed ships switch to lightweight proxies only while piloted. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ship|Collision")
	bool bOptimizeCollisionWhilePiloted{true};

	UFUNCTION(BlueprintCallable, Category = "Ship|Collision")
	void RebuildSimpleHullCollision();

	UFUNCTION(BlueprintPure, Category = "Ship|Collision")
	int32 GetGeneratedCollisionCount() const { return GeneratedCollisionBoxes.Num(); }

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ship|Class")
	FSpaceshipClassPreset ActiveClassPreset;

	/**
	 * Configures interaction volume, seat and safe exit from mesh bounds/sockets.
	 * Native defaults make every ASpaceship Blueprint usable without editing it.
	 */
	UFUNCTION(BlueprintCallable, Category = "Piloting|Automatic Setup")
	void RefreshInteractionGeometry();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Piloting|Automatic Setup")
	bool bAutoConfigureInteractionGeometry{true};

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Piloting|Automatic Setup", meta = (ClampMin = "0.0"))
	float AutoInteractionPadding{300.0f};

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Piloting|Automatic Setup", meta = (ClampMin = "0.0"))
	float AutoExitClearance{180.0f};

	UPROPERTY(VisibleAnywhere, Category = "Astro Actor")
	AStarCluster* GeneratedStarCluster;

	UPROPERTY(VisibleAnywhere, Category = "Astro Actor")
	AActor* GeneratedWorld;

	UPROPERTY(VisibleAnywhere, Category = "Onboard Computer")
	USpaceshipOnboardComputer* OnboardComputer;

	UPROPERTY(VisibleAnywhere, Category = "Onboard Computer")
	AStarSystem* OffsetSystem;

	UPROPERTY(VisibleAnywhere, Category = "Onboard Computer")
	AAstroActor* OffsetGalaxy;

	TArray<AWorldActor*> WorldNavigatableActors{};

	TArray<AWorldActor*> CurrentZonesInfluence{};

	TArray<FActorDistance> Distances;

	TArray<FActorDistance> CurrentZones;

	APlanet* AffectedPlanet{nullptr};

	AWorldActor* AffectedActor{nullptr};

	AWorldActor* ClosestActor{nullptr};

	UStaticMeshComponent* GetSpaceshipHull();

	void ToggleScale();

	bool bIsScaledUp{true};

	bool bIsScaled{false};

	UPROPERTY(BlueprintReadWrite, Category = "Engine Settings")
	bool bEngineRunning{false};

	UFUNCTION(BlueprintPure, Category="Engine Settings")
	bool GetEngineRunning() const { return bEngineRunning; }

	bool bIsAccelerating{false};

	bool bIsDecelerating{false};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight|Handling", meta = (ClampMin = "0.0"))
	float RotationSpeedDegreesPerSecond{30.0f};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight|Handling", meta = (ClampMin = "0.0"))
	float ImpulseRotationAcceleration{0.5f};

	/** Prevents raw mouse delta from turning a heavy hull arbitrarily fast. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight|Handling", meta = (ClampMin = "0.05", ClampMax = "10.0"))
	float SteeringInputLimit{1.0f};

	/** Fine tuning on top of the class turn-rate preset. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight|Handling", meta = (ClampMin = "0.1", ClampMax = "2.0"))
	float SteeringRateScale{0.8f};

	/** Lateral thruster authority that bends the velocity vector while mouse-steering. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight|Handling", meta = (ClampMin = "0.0", ClampMax = "2.0"))
	float SteeringThrustFraction{0.65f};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight|Handling", meta = (ClampMin = "0.0"))
	float BrakingResponseSpeed{1.5f};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight|Transition", meta = (ClampMin = "0.05"))
	float EngineModeTransitionDuration{0.65f};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight|Boost", meta = (ClampMin = "0.1"))
	float BoostRecoverySpeed{2.0f};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
	bool bUseAdaptiveFlightCamera{true};

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Flight|Drive")
	EShipDriveMode SelectedDriveMode{EShipDriveMode::Landing};

	/** Propulsion principle selected independently with keys 1/2/3. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Flight|Drive")
	EEngineMode SelectedEngineMode{EEngineMode::Impulse};

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Flight|Environment")
	EShipFlightEnvironment CurrentFlightEnvironment{EShipFlightEnvironment::DeepSpace};

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Flight|Environment")
	TObjectPtr<AActor> ActiveGravitySource;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Flight|Environment")
	FVector ActiveGravityDirection{FVector::ZeroVector};

	/** Current acceleration generated by the selected source, in cm/s^2. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Flight|Environment")
	double ActiveGravityAcceleration{0.0};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight|Environment", meta = (ClampMin = "0.05"))
	float EnvironmentDetectionInterval{0.2f};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight|Environment", meta = (ClampMin = "0.0"))
	float GravityWellDrag{0.02f};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight|Environment", meta = (ClampMin = "0.0"))
	float AtmosphericDrag{0.65f};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight|Environment", meta = (ClampMin = "0.0"))
	float SurfaceDrag{1.5f};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight|Environment")
	bool bApplyExternalGravity{true};

	/** Running manoeuvring thrusters automatically hold the ship against the active gravity source. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight|Environment")
	bool bFlightAssistCompensatesGravity{true};

	UFUNCTION(BlueprintPure, Category = "Ship|Class")
	static FSpaceshipClassPreset GetPresetForSizeClass(ESpaceshipSizeClass InSizeClass);

	UFUNCTION(BlueprintPure, Category = "Ship|Class")
	static ESpaceshipSizeClass InferSizeClassFromLength(double LengthCentimeters);

	UFUNCTION(BlueprintPure, Category = "Ship|Runtime Fleet")
	static bool IsGeneratedShipMeshAsset(const UStaticMesh* Mesh);

	UFUNCTION(BlueprintPure, Category = "Ship|Runtime Fleet")
	static bool IsGeneratedShipSkeletalMeshAsset(const USkeletalMesh* Mesh);

	UFUNCTION(BlueprintPure, Category = "Ship|Flight")
	FString GetSizeClassName() const;

	UFUNCTION(BlueprintPure, Category = "Ship|Flight")
	FString GetFlightModeName() const;

	UFUNCTION(BlueprintPure, Category = "Ship|Flight")
	FString GetDriveModeName() const;

	UFUNCTION(BlueprintPure, Category = "Ship|Flight")
	FString GetFlightEnvironmentName() const;

	UFUNCTION(BlueprintPure, Category = "Ship|Flight")
	FString GetGravitySourceName() const;

	UFUNCTION(BlueprintPure, Category = "Ship|Flight")
	bool CanUseDriveMode(EShipDriveMode DriveMode) const;

	UFUNCTION(BlueprintPure, Category = "Ship|Flight")
	static double GetDriveSpeedScale(EShipDriveMode DriveMode);

	UFUNCTION(BlueprintPure, Category = "Ship|Flight")
	static double GetDriveAccelerationScale(EShipDriveMode DriveMode);

	UFUNCTION(BlueprintPure, Category = "Ship|Flight")
	static double GetMinimumDriveAcceleration(EShipDriveMode DriveMode);

	UFUNCTION(BlueprintPure, Category = "Ship|Flight")
	bool CanUseEngineMode(EEngineMode EngineMode) const;

	UFUNCTION(BlueprintPure, Category = "Ship|Flight")
	static double GetEngineSpeedMultiplier(EEngineMode EngineMode);

	UFUNCTION(BlueprintPure, Category = "Ship|Flight")
	static double GetEngineAccelerationMultiplier(EEngineMode EngineMode);

	UFUNCTION(BlueprintPure, Category = "Ship|Flight")
	FVector GetCurrentAngularVelocityDegrees() const { return CurrentAngularVelocityDegrees; }

	UFUNCTION(BlueprintPure, Category = "Ship|Flight")
	FString GetEngineModeName() const;

	UFUNCTION(BlueprintPure, Category = "Ship|Flight")
	double GetShipSpeedMetersPerSecond() const;

	UFUNCTION(BlueprintPure, Category = "Ship|Flight")
	double GetCurrentBoostMultiplier() const { return CurrentBoostMultiplier; }

	void ComputeProximity();

	void UpdateNavigatableActors();

	void CheckFlightModeChange();

	void PrintOnboardComputerBasicIformation();

	void SwitchCamera();

	void SwitchEngines();

	void ThrustForward(float Value);

	void ThrustSide(float Value);

	void ThrustVertical(float Value);

	void ThrustYaw(float Value);

	void ThrustPitch(float Value);

	void ThrustRoll(float Value);

	void SetPilot(AGravityCharacterPawn* NewPilot);

	void CalculateDistanceAndAddToZones(AWorldActor* WorldActor);

	void StartAccelerationBoost();

	void StopAccelerationBoost();

	void StartDecelerationBoost();

	void StopDecelerationBoost();

	void HandleAccelerationBoost(float Value);

	void HandleDecelerationBoost(float Value);

	void IncreaseFlightMode();

	void DecreaseFlightMode();

	void SelectImpulseEngine();
	void SelectSpaceWrapEngine();
	void SelectOffsetEngine();
	void ToggleNavigationMarkers();
	void ToggleNavigationPanel();
	void ToggleNavigationGuides();
	void SelectNextNavigationTarget();
	void SelectPreviousNavigationTarget();

protected:
	virtual USceneComponent* GetPilotSeatComponent() const override;
	virtual FTransform GetPilotExitTransform() const override;

private:
	UPrimitiveComponent* GetPrimaryHullComponent() const;
	bool GetPrimaryHullLocalBounds(UPrimitiveComponent* Hull, FVector& OutMin, FVector& OutMax) const;
	void ConfigureFromHull();
	void ConfigureCameraFromHull();
	void ConfigureFlightReferenceFromHull(UPrimitiveComponent* Hull, const FVector& LocalExtent);
	void UpdateAdaptiveFlightCamera(float DeltaTime);
	FVector GetShipForwardVector() const;
	FVector GetShipRightVector() const;
	FVector GetShipUpVector() const;
	void SetFlightCollisionOptimization(bool bEnabled);
	void UpdateFlightEnvironment(float DeltaTime, bool bForce = false);
	void ApplyEnvironmentForces(float DeltaTime);
	void EnforceDriveModeForEnvironment();
	void SetDriveMode(EShipDriveMode NewDriveMode, bool bImmediate);
	void SetEngineMode(EEngineMode NewEngineMode, bool bImmediate);
	EShipDriveMode GetMaximumDriveModeForClass() const;
	EShipDriveMode GetMaximumDriveModeForEnvironment() const;
	EEngineMode ResolveEngineModeForDriveMode(EShipDriveMode DriveMode) const;
	EFlightMode ResolveLegacyFlightModeForDriveMode(EShipDriveMode DriveMode) const;
	double GetEnvironmentDrag() const;
	bool IsNearGravitySurface(const FVector& GravityDirection) const;
	void ApplyFlightInput(float DeltaTime);
	void ApplyRotationInput(float DeltaTime);
	void ApplyEngineState();
	void RequestEngineModeForFlightMode(bool bImmediate);
	void AdvanceEngineModeTransition(float DeltaTime);
	EEngineMode ResolveEngineModeForFlightMode(EFlightMode FlightMode) const;
	double GetFlightModeSpeedScale(EFlightMode FlightMode) const;
	void CreateShipHud();
	void RemoveShipHud();
	FText GetShipStatusText() const;
	FText GetShipHintText() const;
	FText GetNavigationPanelText() const;
	FText GetNavigationMarkerText(int32 ContactIndex) const;
	FVector GetNavigationContactWorldAnchor(int32 ContactIndex) const;
	bool ShouldShowNavigationMarker(int32 ContactIndex) const;
	bool ProjectWorldLocationToNavigationScreen(const FVector& WorldLocation, FVector2D& OutScreenPosition) const;
	bool ProjectNavigationContactToScreen(int32 ContactIndex, FVector2D& OutScreenPosition) const;
	bool GetNavigationMarkerLayout(int32 ContactIndex, FVector2D& OutAnchorPosition,
		FVector2D& OutLabelPosition) const;
	int32 PaintNavigationOverlay(const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
		FSlateWindowElementList& OutDrawElements, int32 LayerId) const;
	FLinearColor GetNavigationMarkerColor(int32 ContactIndex) const;
	friend class SAPSShipNavigationOverlay;

	bool bSeatWasAutoConfigured{false};
	bool bExitWasAutoConfigured{false};
	bool bInteractionZoneWasAutoConfigured{false};
	FTransform LastAutoSeatRelativeTransform;
	FTransform LastAutoExitRelativeTransform;
	FVector LastAutoInteractionZoneRelativeLocation{FVector::ZeroVector};
	float LastAutoInteractionRadius{1000.0f};
	FVector FlightForwardLocalAxis{FVector::ForwardVector};
	FVector FlightUpLocalAxis{FVector::UpVector};
	float BaseCameraArmLength{1200.0f};
	float BaseCameraFieldOfView{90.0f};
	bool bCameraFieldOfViewInitialized{false};
	float EnvironmentDetectionElapsed{0.0f};
	bool bFlightCollisionOptimizationActive{false};
	FName OriginalHullCollisionProfile{NAME_None};
	ECollisionEnabled::Type OriginalHullCollisionEnabled{ECollisionEnabled::QueryAndPhysics};
	FCollisionResponseContainer OriginalHullCollisionResponses;
	bool bOriginalHullSimulatesPhysics{false};

	float ForwardInput{0.0f};
	float SideInput{0.0f};
	float VerticalInput{0.0f};
	float YawInput{0.0f};
	float PitchInput{0.0f};
	float RollInput{0.0f};
	FVector CurrentAngularVelocityDegrees{FVector::ZeroVector};
	double CurrentBoostMultiplier{1.0};
	FVector KinematicVelocity{FVector::ZeroVector};
	EEngineMode PendingEngineMode{EEngineMode::Impulse};
	float EngineModeTransitionElapsed{0.0f};
	bool bEngineModeTransitionActive{false};
	bool bEngineModeSwitchedAtMidpoint{false};
	bool bNavigationMarkersVisible{true};
	bool bNavigationPanelVisible{true};
	bool bNavigationGuidesVisible{true};
	int32 MaximumNavigationMarkers{24};
	TSharedPtr<SWidget> ShipHudWidget;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UBoxComponent>> GeneratedCollisionBoxes;
};
