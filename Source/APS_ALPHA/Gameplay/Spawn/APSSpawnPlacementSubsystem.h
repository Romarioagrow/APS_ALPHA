#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Templates/Function.h"
#include "APSSpawnPlacementSubsystem.generated.h"

class APlanetaryBody;

/** Where a spawned structure stands (S5, Rio 29.09: one spawn system for space and surfaces). */
UENUM(BlueprintType)
enum class EAPSSpawnSite : uint8
{
	/** On a planet's WorldScape terrain beside an anchor, the colony base. */
	Surface,
	/** In space beside an anchor, the headquarters, moving with it. */
	Orbit
};

/** One request for any spawned structure: what it stands beside, how big it is, what it keeps clear of. */
USTRUCT(BlueprintType)
struct APS_ALPHA_API FAPSSpawnRequest
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="APS|Spawn")
	EAPSSpawnSite Site{EAPSSpawnSite::Surface};

	/** The structure the new one stands beside: the colony base or the headquarters. Required. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="APS|Spawn")
	TObjectPtr<AActor> Anchor;

	/** Surface only: the body whose WorldScape terrain carries the structure. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="APS|Spawn")
	TObjectPtr<APlanetaryBody> Body;

	/** Footprint (X along the structure's front, which faces the anchor; Y across) and height, centimetres. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="APS|Spawn")
	FVector SizeCm{1000.0, 1000.0, 500.0};

	/** Gap kept to the anchor and to every obstacle. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="APS|Spawn")
	double SpacingCm{800.0};

	/** The farthest a candidate stands beyond the closest free ring. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="APS|Spawn")
	double MaximumReachCm{12000.0};

	/** Surface: the steepest footprint accepted, rise over run (the base itself takes 0.12). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="APS|Spawn")
	double MaximumSlope{0.15};

	/** Surface: the lowest footprint sample stays this far above the ocean. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="APS|Spawn")
	double MinimumDryMarginCm{500.0};

	/** Rotates the order of candidate directions, for variety between otherwise equal requests. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="APS|Spawn")
	int32 Seed{0};

	/** Structures to keep clear of besides the anchor: earlier modules, the landing pad, a parked ship. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="APS|Spawn")
	TArray<TObjectPtr<AActor>> Obstacles;

	/** The walk from the anchor to each of these (the landing pad) stays free: no structure on that corridor. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="APS|Spawn")
	TArray<TObjectPtr<AActor>> RouteTargets;
};

USTRUCT(BlueprintType)
struct APS_ALPHA_API FAPSSpawnPlacement
{
	GENERATED_BODY()

	/** The structure's pivot on its support plane: X toward the anchor, Z outward from the body (surface) or along the
	 * anchor's up (orbit). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="APS|Spawn")
	FTransform Transform{FTransform::Identity};

	/** What the structure attaches to, so it moves with the planet or the station. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="APS|Spawn")
	TObjectPtr<AActor> AttachParent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="APS|Spawn")
	bool bValid{false};

	/** False while the site cannot be judged at all (the surface is not loaded): retry later instead of giving up. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="APS|Spawn")
	bool bSiteReady{false};

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="APS|Spawn")
	double Slope{0.0};

	/** Surface: the support plane above the lowest footprint sample, so the foundation reaches the ground everywhere. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="APS|Spawn")
	double FoundationDepthCm{0.0};

	/** Orbit: from the structure's front face to the anchor along its X axis, the length of a connecting boom. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="APS|Spawn")
	double GapToAnchorCm{0.0};

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="APS|Spawn")
	int32 CandidatesTried{0};

	/** APS.Spawn.* code: SurfaceNotLoaded, InvalidRequest, Crowded, TooSteep, Flooded. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="APS|Spawn")
	FName FailureCode;
};

/**
 * One placement service for spawned structures on WorldScape surfaces and in orbit (S5). A surface request samples the
 * canonical WorldScape height function around the anchor, the same GetGroundHeight the colony's own resolver uses, and
 * takes the nearest dry footprint within the slope limit that keeps clear of the anchor, the obstacles and the walk to
 * the landing pad. An orbit request rings the anchor station in its own plane. Nothing here moves terrain, the player's
 * LOD or collision, and nothing activates surface streaming: an unloaded surface is simply "not ready yet".
 */
UCLASS()
class APS_ALPHA_API UAPSSpawnPlacementSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	/** Fills the placement; false with a FailureCode when nothing fits (bSiteReady tells a retry from a refusal). */
	bool ResolvePlacement(const FAPSSpawnRequest& Request, FAPSSpawnPlacement& OutPlacement) const;

	/** Spawns the class at a valid placement, lets the caller configure it before construction, and attaches it. */
	AActor* SpawnAtPlacement(TSubclassOf<AActor> ActorClass, const FAPSSpawnPlacement& Placement,
		TFunctionRef<void(AActor*)> BeforeFinish) const;

	/**
	 * Candidate centres around a rectangle (the anchor's footprint), ring by ring, nearest first; within a ring the
	 * sides come first and the front last (the colony base faces its landing pad). Every candidate clears the
	 * rectangle by the spacing plus the footprint radius. Pure geometry, shared with the tests.
	 */
	static void BuildRingCandidates(const FVector2D& RectCenter, const FVector2D& RectHalfSize,
		double FootprintRadiusCm, double SpacingCm, double MaximumReachCm, int32 Seed, TArray<FVector2D>& OutCandidates);

	static double DistanceToRectangle(const FVector2D& Point, const FVector2D& RectCenter, const FVector2D& RectHalfSize);
	static double DistanceToSegment(const FVector2D& Point, const FVector2D& Start, const FVector2D& End);
	/** Visible static-mesh bounds in the actor's own (unscaled) space: invisible volumes do not count as footprint. */
	static FBox VisualLocalBounds(const AActor* Actor);

	/**
	 * Surface: how far a structure's underside stands above the WorldScape ground at the lowest point beneath it. The
	 * underside is the bottom of its largest visible part (the base's foundation, the pad's deck), returned in the
	 * actor's own space with whether it is round. Negative when the surface is not loaded.
	 */
	double MeasureGroundGap(const AActor* Structure, APlanetaryBody* Body, FBox& OutLocalFootprint,
		bool& bOutRound) const;

private:
	bool ResolveSurface(const FAPSSpawnRequest& Request, FAPSSpawnPlacement& OutPlacement) const;
	bool ResolveOrbit(const FAPSSpawnRequest& Request, FAPSSpawnPlacement& OutPlacement) const;
};
