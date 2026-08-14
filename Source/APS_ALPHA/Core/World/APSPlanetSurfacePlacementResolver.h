#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "APSPlanetSurfacePlacementResolver.generated.h"

class APlanetaryBody;

USTRUCT(BlueprintType)
struct APS_ALPHA_API FAPSCivilizationFootprintRequest
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "APS|Surface Placement")
	int32 ManifestSeed{0};

	/** Default foundation envelope: 80 x 55 metres. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "APS|Surface Placement")
	FVector2D BaseSizeCm{8000.0, 5500.0};

	/** Default landing-pad diameter: 90 metres. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "APS|Surface Placement")
	double PadDiameterCm{9000.0};

	/** Default base-to-pad centre separation: 140 metres. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "APS|Surface Placement")
	double SeparationCm{14000.0};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "APS|Surface Placement")
	double MaximumStructureSlope{0.12};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "APS|Surface Placement")
	double MaximumRouteSlope{0.20};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "APS|Surface Placement")
	double MinimumDryMarginCm{1000.0};

	/** Support-plane clearance; asset-specific pivot offsets remain caller-owned. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "APS|Surface Placement")
	double SurfaceClearanceCm{25.0};
};

USTRUCT(BlueprintType)
struct APS_ALPHA_API FAPSCivilizationFootprintResult
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "APS|Surface Placement")
	FTransform BaseTransform{FTransform::Identity};

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "APS|Surface Placement")
	FTransform PadTransform{FTransform::Identity};

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "APS|Surface Placement")
	FVector Outward{FVector::ZeroVector};

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "APS|Surface Placement")
	FVector Forward{FVector::ZeroVector};

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "APS|Surface Placement")
	FVector Right{FVector::ZeroVector};

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "APS|Surface Placement")
	int64 PlacementKey{0};

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "APS|Surface Placement")
	int32 CandidateOrdinal{INDEX_NONE};

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "APS|Surface Placement")
	bool bTerrainResolved{false};

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "APS|Surface Placement")
	bool bDry{false};

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "APS|Surface Placement")
	bool bSlopeValid{false};

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "APS|Surface Placement")
	bool bWalkableRoute{false};

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "APS|Surface Placement")
	bool bLod0Ready{false};

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "APS|Surface Placement")
	bool bCollisionReady{false};

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "APS|Surface Placement")
	bool bReadyForMaterialization{false};

	/** APS-78 foundation slice intentionally performs no foliage mutation. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "APS|Surface Placement")
	bool bFoliageClearanceApplied{false};

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "APS|Surface Placement")
	double MinimumDryMarginCm{0.0};

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "APS|Surface Placement")
	double BaseMaximumSlope{0.0};

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "APS|Surface Placement")
	double PadMaximumSlope{0.0};

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "APS|Surface Placement")
	double RouteMaximumSlope{0.0};

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "APS|Surface Placement")
	double MaximumTerrainDeviationCm{0.0};

	/** Distance to the current visual LOD0 is evidence only, never candidate input. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "APS|Surface Placement")
	double Lod0AnchorDistanceCm{0.0};

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "APS|Surface Placement")
	FString FailureReason;
};

/**
 * Surface-owned resolver consuming the one canonical active WorldScape profile.
 * It never creates another terrain model and never spawns civilization actors.
 */
UCLASS()
class APS_ALPHA_API UAPSPlanetSurfacePlacementResolver : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * Returns true only when terrain, canonical LOD0 and collision are ready.
	 * On false, OutResult may still contain the deterministic terrain solution so the
	 * caller can request Surface anchors, wait, and retry without choosing another site.
	 */
	UFUNCTION(BlueprintCallable, Category = "APS|Planet Surface|Placement")
	static bool TryResolveCivilizationFootprint(
		APlanetaryBody* HomeBody,
		const FAPSCivilizationFootprintRequest& Request,
		FAPSCivilizationFootprintResult& OutResult);

	/** Adds transient base/pad/route collision invokers without moving player LOD. */
	UFUNCTION(BlueprintCallable, Category = "APS|Planet Surface|Placement")
	static bool RequestPlacementAnchors(
		APlanetaryBody* HomeBody,
		const FAPSCivilizationFootprintResult& ResolvedPlacement);

	UFUNCTION(BlueprintCallable, Category = "APS|Planet Surface|Placement")
	static void ReleasePlacementAnchors(APlanetaryBody* HomeBody, int64 PlacementKey);

	/** Pure helpers used by deterministic contract tests and persistence logs. */
	static uint32 BuildPlacementKey(
		int32 ManifestSeed, int32 SurfaceSeed, int32 PlanetTypeValue,
		int32 PlanetRadiusKm);
	static void BuildSeedFrame(
		uint32 PlacementKey, FVector& OutOutward, FVector& OutForward);
};
