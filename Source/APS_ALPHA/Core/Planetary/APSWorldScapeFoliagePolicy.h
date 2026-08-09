#pragma once

#include "CoreMinimal.h"
#include "APS_ALPHA/Core/Planetary/APSPlanetSurfaceProfile.h"

class AWorldScapeRoot;
class UWorldScapeFoliagesCollection;

/** Immutable decision produced before WorldScape receives any foliage objects. */
struct APS_ALPHA_API FAPSFoliageActivationPlan
{
	bool bEnabled{false};
	float HabitatDensityScale{0.0f};
	int32 MaxCollections{0};
	int32 MaxTypesPerCollection{0};
	int32 MaxInstancesPerSectorPerCollection{0};
	int32 MaxClusterMeshesPerType{0};
	float MinSectorSizeCm{0.0f};
	float MaxCullDistanceMultiplier{0.0f};
	bool bUseNoiseMask{true};
	bool bCastShadows{false};
};

/**
 * Safety boundary between APS surface profiles and WorldScape 5.4 foliage.
 *
 * The plugin has no global instance cap and its root constructor enables foliage
 * by default. This policy keeps roots disabled unless both opt-ins are present,
 * then builds transient HISM-only copies whose collection/type/sector budgets are
 * enforced without modifying the authored source assets.
 */
class APS_ALPHA_API FAPSWorldScapeFoliagePolicy
{
public:
	static FAPSFoliageActivationPlan BuildActivationPlan(
		const FAPSResolvedPlanetSurfaceProfile& Profile,
		bool bRuntimeOptIn,
		bool bScaledOrbitalPreview);

	/** WorldScape 5.4 cannot safely mutate an applied root involving foliage. */
	static bool RequiresFreshRootForProfileTransition(
		const FAPSResolvedPlanetSurfaceProfile& AppliedProfile,
		const FAPSResolvedPlanetSurfaceProfile& RequestedProfile,
		bool bRootHasActivatedFoliage);

	/** Global kill switch. aps.WorldScapeFoliage.Enable defaults to zero. */
	static bool IsRuntimeOptInEnabled();

	/**
	 * Builds sanitized transient collections for a precomputed plan.
	 * Actor/Blueprint foliage is rejected, collision is disabled, and classic
	 * static-mesh entries are forced through WorldScape's HISM branch.
	 */
	static int32 BuildBudgetedCollections(
		UObject* Outer,
		const TArray<UWorldScapeFoliagesCollection*>& SourceCollections,
		const FAPSFoliageActivationPlan& Plan,
		TArray<UWorldScapeFoliagesCollection*>& OutCollections);

	/**
	 * Configures a newly-created owned runtime root before its first terrain tick.
	 * Callers must not use this to hot-swap foliage on an active WorldScape root;
	 * the plugin's foliage worker has no public drain primitive in version 5.4.
	 */
	static int32 ApplyToFreshOwnedRuntimeRoot(
		AWorldScapeRoot* Root,
		const FAPSResolvedPlanetSurfaceProfile& Profile,
		bool bScaledOrbitalPreview);
};
