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
class USpotLightComponent;
class UPointLightComponent;
class USkeletalMesh;
class USkeletalMeshComponent;
class UPrimitiveComponent;
class UGravityDetectorComponent;
class UShipNavigationComponent;
class UAPSShipFlightModel;
class AWorldActor;
class SWidget;
class SAPSShipNavigationOverlay;
class FSlateWindowElementList;
struct FGeometry;
class FSlateRect;
class UBoxComponent;
class UArrowComponent; class UInstancedStaticMeshComponent;
enum class EAPSGroundVehicleKind : uint8; // Rio 02.10: ground vehicles; defined in Gameplay/Vehicles/APSGroundVehicleTypes.h
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

	/** Camera-side fill active only on the controlled ship, keeping dark hull materials readable. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ship|Visuals")
	USpotLightComponent* PilotFillLight;

	/** Direction-independent camera fill; the legacy spot could point past rotated hulls. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ship|Visuals")
	UPointLightComponent* PilotFillPointLight;

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

	/** Band flight model (keys 1-5); moves every ship while aps.Ship.FlightModel is 1. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UAPSShipFlightModel* FlightModel;

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

	/**
	 * Makes ForwardVector's rotation the ship's nose, even when it is zero and has no mesh.
	 * Ship Blueprints turn this on so hulls never fall back to guessing from the longest bounds axis.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ship|Flight")
	bool bUseAuthoredNoseDirection{false};

#if WITH_EDITORONLY_DATA
	/** Editor-only arrow on ForwardVector that shows the flight nose in the viewport. */
	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UArrowComponent> NoseArrow;
#endif

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

	/**
	 * Ship gravity in effect: its own artificial gravity, or, for a ship with an interior, once it is no longer parked on
	 * a world (Rio 02.10: "walk about a flying ship"; a colony-parked ship leaves its deck to the world's pull).
	 */
	bool ProvidesShipGravity() const;
	/** Its gravity zone answers a walking character only while ship gravity is in effect. */
	void RefreshShipGravityZone();

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

	/** The static hull mesh carries convex hulls fitted to it (a convex decomposition), not just a box. */
	bool HullHasFittedConvexCollision() const;

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

	/** Legacy RCS tuning retained for asset compatibility; mouse steering now applies angular torque only. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight|Handling", meta = (ClampMin = "0.0", ClampMax = "2.0"))
	float SteeringThrustFraction{0.65f};

	/** Minimum time in which strategic propulsion may substantially change linear momentum. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight|Handling", meta = (ClampMin = "0.1", ClampMax = "5.0"))
	float HighSpeedVelocityResponseTime{0.8f};

	/** Fraction of the class turn rate available for bending a high-speed velocity vector. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight|Handling", meta = (ClampMin = "0.05", ClampMax = "1.0"))
	float HighSpeedHeadingResponseScale{0.35f};

	/** Small passive rotational drag; counter-steering remains the fast way to stop a turn. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight|Handling", meta = (ClampMin = "0.0", ClampMax = "2.0"))
	float PassiveAngularDamping{0.22f};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight|Handling", meta = (ClampMin = "0.0"))
	float BrakingResponseSpeed{1.5f};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight|Transition", meta = (ClampMin = "0.05"))
	float EngineModeTransitionDuration{0.9f};

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

	/** A new gravity source/environment must remain stable for this long before it can alter flight physics. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight|Environment", meta = (ClampMin = "0.0", ClampMax = "2.0"))
	float EnvironmentTransitionConfirmationTime{0.35f};

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

	/** Recomputes the flight nose and up axes from ForwardVector, or from the hull bounds as a fallback. */
	UFUNCTION(BlueprintCallable, Category = "Ship|Flight")
	void RefreshFlightReferenceFromHull();

	/** World-space nose direction used by flight, camera and navigation. */
	UFUNCTION(BlueprintPure, Category = "Ship|Flight")
	FVector GetShipForwardVector() const;

	UFUNCTION(BlueprintPure, Category = "Ship|Flight")
	FString GetSizeClassName() const;

	UFUNCTION(BlueprintPure, Category = "Ship|Flight")
	FString GetFlightModeName() const;

	UFUNCTION(BlueprintPure, Category = "Ship|Flight")
	FString GetDriveModeName() const;

	UFUNCTION(BlueprintPure, Category = "Ship|Flight")
	FString GetFlightEnvironmentName() const;
	/** Z (Rio 02.10): the autopilot to the selected navigation target, or off. */
	void ToggleAutopilot();

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

	/** Read-only presentation input for engine audio; does not consume or change controls. */
	FVector GetPilotTranslationInput() const { return FVector(ForwardInput, SideInput, VerticalInput); }

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

	/** Keys 1-3: a flight band in the band model, the engine in the power-step model. */
	void SelectImpulseEngine();
	void SelectSpaceWrapEngine();
	void SelectOffsetEngine();
	/** Keys 4-5: the Cruise and Stellar bands (band model only). */
	void SelectCruiseBand();
	void SelectStellarBand();
	/** Key 0: the band model picks the band itself again (AUTO). */
	void SelectAutoBands();
	void ToggleNavigationMarkers();
	void ToggleNavigationPanel();
	void ToggleNavigationGuides();
	/** Rio 04.10: labels on the nearest stars on and off (Y). */
	void ToggleNearStarLabels();
	void SelectNextNavigationTarget();
	void SelectPreviousNavigationTarget();

protected:
	virtual USceneComponent* GetPilotSeatComponent() const override;
	virtual FTransform GetPilotExitTransform() const override;

	/**
	 * Alternative flight models move the ship here: the band model (UAPSShipFlightModel) by default. Return true
	 * when this frame's translation is handled; the power-step ApplyFlightInput is then skipped. Environment
	 * detection, rotation, camera and HUD keep running in ASpaceship.
	 */
	virtual bool ApplyCustomFlightTranslation(float DeltaTime);
	/** Extra HUD status line of the active flight model; empty keeps the power-step text only. */
	virtual FString GetCustomFlightStatus() const;
	/** Key hints of the active flight model; empty keeps the power-step hints. */
	virtual FString GetCustomFlightHint() const;

	UPrimitiveComponent* GetPrimaryHullComponent() const;
	FVector GetShipRightVector() const;
	FVector GetShipUpVector() const;
	double GetEnvironmentDrag() const;
	/**
	 * Kinematic move of the whole ship. A requested sweep first tests the root hull's bounding sphere
	 * (aps.Ship.SweepPrecheck) and runs the per-hull body sweep only when that sphere may hit.
	 */
	bool MoveShipKinematic(const FVector& Delta, bool bSweep, FHitResult& OutHit);
	/** Flight proxies are child boxes and MoveComponent sweeps only the root: sweep the boxes themselves. */
	bool MoveShipWithProxySweep(const FVector& Delta, FHitResult& OutHit);

	float ForwardInput{0.0f};
	float SideInput{0.0f};
	float VerticalInput{0.0f};
	double CurrentBoostMultiplier{1.0};
	FVector KinematicVelocity{FVector::ZeroVector};

private:
	bool GetPrimaryHullLocalBounds(UPrimitiveComponent* Hull, FVector& OutMin, FVector& OutMax) const;
	void ConfigureFromHull();
	void ConfigureCameraFromHull();
	void ConfigurePilotFillLight();
	void UpdatePilotFillLightVisibility();
	void ConfigureFlightReferenceFromHull(UPrimitiveComponent* Hull, const FVector& LocalExtent);
	void UpdateAdaptiveFlightCamera(float DeltaTime);
	/** Rio 05.10: the camera arm ticks every frame only while the player flies the ship (aps.Ship.IdleCameraTickSeconds). */
	void UpdateCameraArmTicking();
	void InitializeFlightPostProcess();
	void RestoreFlightPostProcess();
	/** While piloted, keeps the ship out of the distance-field and Lumen scene representations (see
	 * aps.Ship.HullSceneLightingInFlight); restores the authored flags when the pilot leaves. */
	void SetHullSceneLightingExcluded(bool bExcluded);
	struct FHullSceneLightingFlags
	{
		TWeakObjectPtr<UPrimitiveComponent> Component;
		bool bAffectDistanceField{true};
		bool bAffectIndirect{true};
	};
	TArray<FHullSceneLightingFlags> ExcludedHullSceneLighting;
	/** aps.Ship.HullSceneLightingInFlight value applied to the piloted hull; a console change re-applies it live. */
	int32 AppliedHullSceneLightingMode{1};
	void StabilizeFullScaleVisualVelocity();
	void SetFlightCollisionOptimization(bool bEnabled);
	/**
	 * Rio 06.10 (collision by motion, aps.Ship.KeepHullOutWhileMoving): Codex's detailed hull collision is kept for a ship
	 * at rest; a ship that moves (with or without a pilot, a fleet unit under orders too) flies on the proxy boxes with the
	 * detailed body out of the physics scene. A walker aboard stands on the walk shell meanwhile (the boxes let pawns, the
	 * camera and visibility traces through). The body comes back once the ship has rested, when the walker steps off.
	 * Rio 07.10: the walk shell is test-only (aps.Ship.WalkOnShellAtSpeed 0, a walker aboard gets the hull's body back), and
	 * without a pilot in the seat the body is held in the scene, inert, rather than destroyed (aps.Ship.HullHold).
	 */
	bool HasDetailedHullProxy() const;
	/** A query-only walk shell (tag APS.Ship.CollisionShell) that carries a walker while the hull's body is out. */
	bool HasWalkShell() const;
	/** A pawn other than the pilot (a walker) rides attached to the ship. */
	bool IsWalkerAboard() const;
	/** Why the ship counts as moving for its collision (speed, autopilot, flow, owed travel, fleet order); null at rest. */
	const TCHAR* GetHullMotionReason() const;
	/** The player's character on foot within the hull's radius + ExtraCm. */
	bool IsPlayerOnFootNear(double ExtraCm) const;
	/** The proxy boxes ignore Pawn, Camera and Visibility (a walker aboard a moving ship), or block all again. */
	void SetProxyBoxesPassWalkers(bool bPass);
	/** UnPossessed: keeps the hull's body out for the walker (true), or leaves the restore to the caller (false). */
	bool KeepHullOutForWalker();
	/** Timer (0.1 s) of a ship with a detailed hull: takes the body out while it moves, gives it back after a rest. */
	void UpdateHullCollisionByMotion();
	void StartHullMotionWatch();
	/**
	 * Rio 07.10 (aps.Ship.HullHold, fleet on proxies without the restore freeze): the take-out holds the hull's body in the
	 * scene, inert (UAPSShipHullComponent), instead of destroying it: a root static hull with a physics body, nobody in the
	 * seat (or HullHold 2), not frozen far away.
	 */
	bool ShouldHoldHullBody(const UPrimitiveComponent& PrimaryHull) const;
	/** Makes the held body inert and stops sending it the hull's moves, or teleports it back and restores its filters. */
	void SetHullBodyHeld(bool bHold);
	/** Rio 07.10 (fleet audit): a restore may run now: not while frozen far away (no body could be built), and a build waits
	 * aps.Ship.HullRestoreSpacingSeconds after the last one when paced. */
	bool MayRestoreHullBody(bool bPacedBuild) const;
	/** EndPlay, a ship being destroyed or a world torn down: no body is rebuilt (aps.Ship.HullRestoreOnce). */
	bool IsHullGoingAway() const;
	/** Rio 06.10 (aps.Ship.CameraAlignNose): the camera keeps its place above and behind but looks along the nose. */
	void UpdateCameraNoseAlignment();
	void UpdateFlightEnvironment(float DeltaTime, bool bForce = false);
	void ApplyEnvironmentForces(float DeltaTime);
	void EnforceDriveModeForEnvironment();
	void SetDriveMode(EShipDriveMode NewDriveMode, bool bImmediate);
	void SetEngineMode(EEngineMode NewEngineMode, bool bImmediate);
	EShipDriveMode GetMaximumDriveModeForClass() const;
	EShipDriveMode GetMaximumDriveModeForEnvironment() const;
	EEngineMode ResolveEngineModeForDriveMode(EShipDriveMode DriveMode) const;
	EFlightMode ResolveLegacyFlightModeForDriveMode(EShipDriveMode DriveMode) const;
	bool IsNearGravitySurface(const FVector& GravityDirection) const;
	void ApplyFlightInput(float DeltaTime);
	void ApplyRotationInput(float DeltaTime);
	FVector GetControlledFlightAcceleration(const FVector& WorldInput, const FVector& CurrentVelocity,
		double RequestedAcceleration, double MaximumSpeed) const;
	float GetEngineTransitionAuthority() const;
	void ApplyEngineState();
	void CommitEngineModeSwitch(EEngineMode NewEngineMode);
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
	const APlanet* GetNavigationFocusPlanet() const;
	bool IsInsideNavigationFocusGravity(const APlanet* FocusPlanet) const;
	bool ShouldShowNavigationMarker(int32 ContactIndex) const;
	bool ProjectWorldLocationToNavigationScreen(const FVector& WorldLocation, FVector2D& OutScreenPosition,
		bool bRequireInsideViewport = true, bool bSnapToPixel = true) const;
	bool ProjectNavigationContactToScreen(int32 ContactIndex, FVector2D& OutScreenPosition) const;
	bool GetNavigationMarkerLayout(int32 ContactIndex, FVector2D& OutAnchorPosition,
		FVector2D& OutLabelPosition, const TSet<int32>* OccludedContacts = nullptr) const;
	int32 PaintNavigationOverlay(const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
		FSlateWindowElementList& OutDrawElements, int32 LayerId) const;
	FLinearColor GetNavigationMarkerColor(int32 ContactIndex) const;
	friend class SAPSShipNavigationOverlay;
	friend class FAPSShipFlightBenchmark;
	friend class UAPSShipFlightModel;
	friend class FAPSFleetCommand;

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
	float SmoothedCameraSpeedAlpha{0.0f};
	/** Chase camera feel (29.09): rate of ln(speed) in 1/s, the same for a 1 -> 100 km/s burn and 1 -> 100 c. */
	float SmoothedLogSpeedRate{0.0f};
	double PreviousCameraLogSpeed{-1.0};
	/** Rate of the critically damped arm-length follow, cm/s. */
	float CameraArmLengthRate{0.0f};
	bool bCameraFieldOfViewInitialized{false};
	bool bCameraPostProcessInitialized{false};
	float BaseCameraPostProcessBlendWeight{0.0f};
	float BaseSceneFringeIntensity{0.0f};
	float BaseChromaticAberrationStartOffset{0.0f};
	float BaseVignetteIntensity{0.0f};
	float BaseBloomIntensity{0.0f};
	float BaseAutoExposureBias{0.0f};
	bool bBaseOverrideSceneFringe{false};
	bool bBaseOverrideChromaticStart{false};
	bool bBaseOverrideVignette{false};
	bool bBaseOverrideBloom{false};
	bool bBaseOverrideExposureBias{false};
	float EnvironmentDetectionElapsed{0.0f};
	float PendingEnvironmentTransitionElapsed{0.0f};
	EShipFlightEnvironment PendingFlightEnvironment{EShipFlightEnvironment::DeepSpace};
	TWeakObjectPtr<AActor> PendingGravitySource;
	bool bFlightEnvironmentInitialized{false};
	bool bFlightCollisionOptimizationActive{false};
	/** Set while SetFlightCollisionOptimization builds the in-flight box proxy of a detailed ship. */
	bool bBuildingFlightCollisionProxy{false};
	FName OriginalHullCollisionProfile{NAME_None};
	ECollisionEnabled::Type OriginalHullCollisionEnabled{ECollisionEnabled::QueryAndPhysics};
	FCollisionResponseContainer OriginalHullCollisionResponses;
	bool bOriginalHullSimulatesPhysics{false};
	/** Rio 06.10: the proxy build turns the hull's overlap events off; aps.Ship.HullRestoreOnce gives them back. */
	bool bOriginalHullGenerateOverlapEvents{false};
	/** Collision by motion (HasDetailedHullProxy): the body is out without a pilot and waits for the ship to rest. */
	bool bHullRestorePending{false};
	/** The proxy boxes let a walker, the camera and visibility traces through (the walk shell carries the walker). */
	bool bProxyBoxesPassWalkers{false};
	/** EndPlay ran: the ship is going away and never gets its body back. */
	bool bHullGoingAway{false};
	/** Rio 07.10 (aps.Ship.HullHold): the hull's body is in the scene, held and inert, while the ship flies on its boxes. */
	bool bHullBodyHeld{false};
	/** Rio 07.10 (aps.Ship.ProxyUnstick): real time of the last 'climbs out of' log line (one a second). */
	double ProxyUnstickLogSeconds{0.0};
	/** Rio 07.10 (aps.Ship.InstancedResend): the ship's own plain instanced meshes and the render matrix last seen. */
	struct FAPSInstancedRider
	{
		TWeakObjectPtr<UInstancedStaticMeshComponent> Mesh;
		FMatrix Last;
		bool bHasLast = false;
	};
	TArray<FAPSInstancedRider> InstancedRiders;
	FDelegateHandle InstancedResendHandle;
	void ResendInstancedRiders(UWorld* World);
	/** Seconds the pending ship has rested so far, and the world time of the timer's last check. */
	float HullRestSeconds{0.0f};
	double HullMotionLastCheckSeconds{0.0};
	/** World time the pilot got up, keeping the boxes open for the walker a moment before it is attached aboard. */
	double HullPendingSinceSeconds{-1.0e9};
	/** World time this ship last carried a world flow or an owed step (MoveShipKinematic). */
	double HullLastFlowSeconds{-1.0e9};
	FTimerHandle HullMotionTimer;
	/** aps.Ship.CameraAlignNose as applied to the camera, and the camera's own relative rotation before it. */
	bool bCameraAlignNoseApplied{false};
	FRotator CameraAlignBaseRotation{FRotator::ZeroRotator};

	float YawInput{0.0f};
	float PitchInput{0.0f};
	float RollInput{0.0f};
	/** Mouse look (IsMouseLookActive): the mode chosen with C, the camera's orbit from the chase view (degrees) and how
	 * long the mouse has been still. */
	bool bMouseLook{false};
	double MouseLookYaw{0.0};
	double MouseLookPitch{0.0};
	double MouseLookIdleSeconds{0.0};
	bool bMouseLookApplied{false};
	/** HasWalkableInterior, found once: -1 not yet, 0 no cabin, 1 a modelled cabin. */
	mutable int8 WalkableInteriorState{-1};
	FVector CurrentAngularVelocityDegrees{FVector::ZeroVector};
	EEngineMode PendingEngineMode{EEngineMode::Impulse};
	float EngineModeTransitionElapsed{0.0f};
	bool bEngineModeTransitionActive{false};
	bool bEngineModeSwitchedAtMidpoint{false};
	bool bNavigationMarkersVisible{true};
	bool bNavigationPanelVisible{true};
	bool bNavigationGuidesVisible{true};
	int32 MaximumNavigationMarkers{128};
	TSharedPtr<SWidget> ShipHudWidget;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UBoxComponent>> GeneratedCollisionBoxes;

public:
	/**
	 * Ground vehicles (Rio 02.10: a rover, a hover and a drone at the colony; Gameplay/Vehicles/APSGroundVehicles.h parks
	 * them). Any kind but None makes the flight model drive this pawn in the local gravity frame instead of the bands;
	 * boarding, exit, camera and HUD stay the ship's. Called on a deferred spawn, before FinishSpawning: it loads and
	 * scales the vehicle's meshes and sets its collision, nose, interaction and name. A ship (None) is never touched.
	 */
	void ConfigureAsGroundVehicle(EAPSGroundVehicleKind Kind);
	EAPSGroundVehicleKind GetGroundVehicleKind() const { return GroundVehicleKind; }
	bool IsGroundVehicle() const { return static_cast<uint8>(GroundVehicleKind) != 0; }
	/** ROVER, HOVER or DRONE; empty for a ship. */
	FString GetGroundVehicleName() const;
	/** The world a vehicle belongs to: its gravity, terrain height and the drone's ceiling fall back to it. */
	void SetGroundVehicleHomeBody(AActor* Body) { GroundVehicleHomeBody = Body; }
	AActor* GetGroundVehicleHomeBody() const { return GroundVehicleHomeBody.Get(); }
	/** The actor rotation that points the flight nose along Forward and the flight up along Up, whatever the hull axes. */
	FQuat GetActorRotationForFlightAxes(const FVector& Forward, const FVector& Up) const;
	/**
	 * Rio 02.10: the mouse orbits the camera instead of steering. C switches it; a rover or a hover starts so, the drone
	 * and the ships steer. The autopilot always keeps it on, so a touch of the mouse never takes the helm back.
	 */
	bool IsMouseLookActive() const;
	void ToggleMouseLook();
	/**
	 * Rio 03.10: the hull carries an authored pilot-seat socket (a modelled cabin, like S_P3_01's): its pilot walks in to
	 * the seat and gets up behind it. Every other ship is boarded from anywhere beside it and left through its outside
	 * exit ("if a ship has no interior yet, do not touch it").
	 */
	bool HasWalkableInterior() const;
	/** The band and vehicle models' own velocity (the hull moves kinematically, so GetVelocity says little). */
	const FVector& GetKinematicVelocity() const { return KinematicVelocity; }

	/** A rover's visual tyre: its centre and radius in the hull's own (unscaled) space, its side and axle. */
	struct FGroundVehicleWheel
	{
		TWeakObjectPtr<USceneComponent> Tire;
		FVector LocalCenter{FVector::ZeroVector};
		double LocalRadius{50.0};
		bool bLeft{false};
		bool bFront{false};
	};
	const TArray<FGroundVehicleWheel>& GetGroundVehicleWheels() const { return GroundVehicleWheels; }

	/**
	 * A bone of the buggy's suspension (SKM_Offroad: control arms, dampers, hubs; the body and the tyres are their own
	 * meshes), at rest in the suspension mesh's component space. The flight model poses it after its tyre's travel.
	 */
	struct FGroundVehicleSuspensionBone
	{
		FName Name;
		int32 Index{INDEX_NONE};
		int32 Wheel{INDEX_NONE};
		/** Hubs (and the arm ends, the wheel bones) ride with the tyre; arms turn about their root toward their moved
		 * end; a damper turns about its top toward its mount on the lower arm, and its end sits on that mount. */
		enum class ERole : uint8 { Hub, Arm, Damper, DamperEnd };
		ERole Role{ERole::Hub};
		bool bSteers{false};
		FTransform Rest;
		/** Arm: the end it reaches; damper and its end: the mount on the lower arm (component space, at rest). */
		FVector Target{FVector::ZeroVector};
		/** Damper and its end: the damper's top, which stays. */
		FVector Pivot{FVector::ZeroVector};
		/** Damper and its end: the lower arm's root and end, about which the mount turns with the arm. */
		FVector ArmRoot{FVector::ZeroVector};
		FVector ArmEnd{FVector::ZeroVector};
	};
	class UPoseableMeshComponent* GetGroundVehicleSuspension() const;
	const TArray<FGroundVehicleSuspensionBone>& GetGroundVehicleSuspensionBones() const { return GroundVehicleSuspensionBones; }

private:
	/** A vehicle's driver steps out to the left of the nose, on the ground beside the hull (BeginPlay, after the
	 * automatic interaction setup). */
	void ConfigureGroundVehicleExit();
	/** A vehicle's chase camera: level with the gravity and behind the heading (UpdateAdaptiveFlightCamera). */
	void UpdateGroundVehicleCamera();
	/** Mouse look: the orbit eases back behind when it is off, and behind a ground vehicle driving on with a still mouse. */
	void UpdateMouseLook(float DeltaTime);

	EAPSGroundVehicleKind GroundVehicleKind{};
	TWeakObjectPtr<AActor> GroundVehicleHomeBody;
	TArray<FGroundVehicleWheel> GroundVehicleWheels;
	/** Owned by the actor as an instance component; the bones are sorted parents first (by bone index). */
	TWeakObjectPtr<class UPoseableMeshComponent> GroundVehicleSuspension;
	TArray<FGroundVehicleSuspensionBone> GroundVehicleSuspensionBones;
};
