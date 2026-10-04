#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "APSSpectatorPawn.generated.h"

class APlayerController;
class SWidget;
class UCameraComponent;
class USceneComponent;

/**
 * Free-flight camera of the Generate Space route (Rio 03.10: "Generate Space, just space without a civilization: on
 * spawn a nice smooth spectator for flying, with pleasant switching of speeds, to look at things at different distances
 * and speeds").
 *
 * AGravityGameModeBase spawns it instead of the walker for a world committed without a civilization
 * (IsGeneratedSpaceRoute); Generate Civilization, Create Planet and the authored Start Single Game keep their pawns.
 * The keys are read from the player controller in Tick, so no input assets are needed:
 *  - mouse: look, lightly smoothed; no roll, the horizon stays level and, low over a planet or moon, follows its ground;
 *  - W/S, A/D: forward/back and strafe; Space and Ctrl (or C): up and down; Q/E: roll;
 *  - speed: the distance to the nearest surface sets it (metres per second at the ground, AU and light years per
 *    second in deep space), times the tier the mouse wheel (or +/-) picks, x0.1 to x1000; Shift boosts x10, Alt slows
 *    x0.1. Tiers, boost and the distance scale ease in log space; the keys ramp through a critically damped spring;
 *  - F: flies to the body under the crosshair (else the nearest) and stops at a standoff, looking at it.
 * No collision and no gravity. A step never enters a body's sphere and stops a few metres above its terrain, and it
 * closes at most a quarter of the gap to the nearest surface per frame (the ships' guard against tunnelling).
 */
UCLASS()
class APS_ALPHA_API AAPSSpectatorPawn : public APawn
{
	GENERATED_BODY()

public:
	AAPSSpectatorPawn();

	/**
	 * A Generate Space world, new or loaded: an astronomical model committed without a civilization that is not a single
	 * planet (Create Planet), outside the authored Start Single Game. aps.FreeFlight 0 keeps the walker there.
	 */
	static bool IsGeneratedSpaceRoute(const UWorld* World);

	virtual void Tick(float DeltaSeconds) override;
	virtual FVector GetVelocity() const override;
	virtual void ApplyWorldOffset(const FVector& InOffset, bool bWorldShift) override;
	virtual void PossessedBy(AController* NewController) override;
	virtual void UnPossessed() override;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	/** A planet, moon, star or station near enough to matter, or a cluster catalogue star (no actor). */
	struct FFlightBody
	{
		TWeakObjectPtr<AActor> Actor;
		int32 CatalogueIndex{INDEX_NONE};
		/** Stations: centre of the hull bounds in actor space. */
		FVector LocalCentre{FVector::ZeroVector};
		double RadiusCm{0.0};
		/** The camera stops this high above the surface. */
		double StopAltitudeCm{0.0};
		FString Name;
		/** Planets, moons and stars; a station is open structure the camera may enter. */
		bool bSolid{true};
		bool bPlanetary{false};
		bool bStar{false};
		bool bStation{false};
	};

	struct FFlightInput
	{
		/** Forward, right, up in the camera's frame, each -1..1. */
		FVector Move{FVector::ZeroVector};
		FVector2D LookDegrees{FVector2D::ZeroVector};
		double Roll{0.0};
		int32 TierSteps{0};
		bool bBoost{false};
		bool bSlow{false};
		bool bToggleFlyTo{false};
	};

	bool IsFlightInputBlocked(const APlayerController& PlayerController) const;
	void ReadFlightInput(const APlayerController& PlayerController, FFlightInput& OutInput) const;
	void RefreshBodies();
	bool GetBodyCentre(const FFlightBody& Body, FVector& OutCentre) const;
	double GetSurfaceRadius(const FFlightBody& Body) const;
	void MeasureSurroundings(double DeltaSeconds);
	void UpdateSpeedScale(const FFlightInput& Input, double DeltaSeconds);
	void UpdateOrientation(const FFlightInput& Input, double DeltaSeconds);
	/** Yaw about the reference up, pitch within +-89 degrees of it, no roll against it. */
	void ApplyLook(double YawDegrees, double PitchDegrees);
	void GetLookDeltaTo(const FVector& Direction, double& OutYawDegrees, double& OutPitchDegrees) const;
	FVector StepFromInput(const FFlightInput& Input, double DeltaSeconds);
	bool BeginFlyTo();
	void AimFlyTo(const FVector& Centre, double StandoffCm);
	bool GetFlyToCentre(FVector& OutCentre) const;
	FVector StepFlyTo(double DeltaSeconds);
	void EndFlyTo(const TCHAR* Reason, bool bKeepMotion);
	/** Moves by Step within the guards; returns the share of the step that was flown. */
	double MoveSafely(FVector Step, double DeltaSeconds);
	void TryStartView();
	void AdoptExternalTransform();
	void ApplyViewTransform();
	void UpdateHudText();
	void CreateHud();
	void RemoveHud();
	bool IsHudShown() const;

	UPROPERTY(VisibleAnywhere, Category = "Free Flight")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, Category = "Free Flight")
	TObjectPtr<UCameraComponent> Camera;

	// View: the camera's rotation and the up it keeps level to.
	FQuat ViewQuat{FQuat::Identity};
	FVector ReferenceUp{FVector::UpVector};
	FVector2D PendingLookDegrees{FVector2D::ZeroVector};
	double RollRateDegrees{0.0};
	double LevelHoldSeconds{0.0};
	/** Horizon assist toward the nearest planet's local up: weight 0..1 and that up. */
	double LevelWeight{0.0};
	FVector LevelUp{FVector::ZeroVector};

	// Motion: the smoothed keys (camera frame) and the speed they are scaled by.
	FVector ThrottleLocal{FVector::ZeroVector};
	FVector ThrottleRate{FVector::ZeroVector};
	FVector FlightVelocity{FVector::ZeroVector};
	int32 TierIndex{2};
	double LogMultiplier{0.0};
	double LogAutoSpeed{0.0};
	bool bAutoSpeedReady{false};
	double CurrentSpeedCm{100.0};
	bool bBoostHeld{false};
	bool bSlowHeld{false};

	// Surroundings.
	TArray<FFlightBody> FlightBodies;
	double BodyRefreshElapsed{1000.0};
	double SpeedDistanceCm{-1.0};
	bool bHasNearest{false};
	double NearestAltitudeCm{0.0};
	FString NearestName;
	bool bHasStopSurface{false};
	double StopGapCm{0.0};
	FVector StopOutward{FVector::ZeroVector};
	TWeakObjectPtr<AActor> TerrainBody;
	double TerrainRadiusCm{0.0};
	double TerrainSampleElapsed{1000.0};

	// F: fly to a body (the remaining way eases in log space, so a planet and a star take about as long).
	bool bFlyTo{false};
	TWeakObjectPtr<AActor> FlyToActor;
	int32 FlyToCatalogue{INDEX_NONE};
	FVector FlyToOffset{FVector::ZeroVector};
	FVector FlyToDirection{FVector::ForwardVector};
	double FlyToLog{0.0};
	double FlyToLogRate{0.0};
	double FlyToEpsilonCm{100.0};
	double FlyToStandoffCm{0.0};
	double FlyToBlockedSeconds{0.0};
	bool bFlyToLook{false};
	FString FlyToName;

	// Start view, outside moves, HUD.
	bool bStartViewPending{false};
	double SpawnSeconds{0.0};
	bool bAnyInput{false};
	FVector AppliedLocation{FVector::ZeroVector};
	FQuat AppliedRotation{FQuat::Identity};
	bool bHasAppliedTransform{false};
	double LogElapsed{0.0};
	TSharedPtr<SWidget> HudWidget;
	FText HudStatus;
	FText HudNearest;
};
