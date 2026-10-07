#pragma once

#include "CoreMinimal.h"
#include "APS_ALPHA/Core/Planetary/APSPlanetSurfaceProfile.h"

class AWorldScapeRoot;
class UWorld;
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
	// Selected without loading assets; authored settings are never overwritten.
	TArray<TSoftObjectPtr<UWorldScapeFoliagesCollection>> Collections;
	bool bPrototypePalette{false};
	bool bSurfaceScatterPalette{false};
	bool bPreserveEntryNoiseMask{false};
};

/**
 * Safety boundary between APS surface profiles and WorldScape 5.4 foliage.
 *
 * The plugin has no global instance cap and its root constructor enables foliage
 * by default. Mode2 supplies reviewed scatter type/habitat combinations on
 * generated planets (sparse Frozen/Forest V1 when scatter is disabled).
 * Mode1 retains explicit authored/prototype opt-in; mode0
 * disables all fresh-root foliage. Transient HISM-only copies enforce budgets
 * without modifying source assets, profiles or saved data. Unreviewed types stay off
 * in mode2; manual/moon, orbital and authored palettes are never auto-promoted.
 */
class APS_ALPHA_API FAPSWorldScapeFoliagePolicy
{
public:
	// WorldScape 5.4 retains sector centres in an inclusive +/-4*size cube:
	// at most 9^3 unique lattice positions per type, not the 3^3 generation ring.
	// Reserve one additional worker batch for the spawn-before-retire peak.
	// These are per-root admission bounds, not measured memory or a world-wide cap.
	static constexpr int32 RetainedSectorEnvelopePerType = 729;
	static constexpr int32 QueuedSectorEnvelopePerType = 27;
	static constexpr int32 PeakSectorEnvelopePerType =
		RetainedSectorEnvelopePerType + QueuedSectorEnvelopePerType;
	static constexpr int32 MaximumPeakInstancesPerRoot = 65536;
	static constexpr int32 MaximumPeakMeshComponentsPerRoot = 4096;
	// Admission reservations, including retired roots until their UObjects are
	// actually released. This bounds APS-owned roots only, not hand-authored roots
	// that bypass this policy, and is not a byte/VRAM guarantee.
	static constexpr int32 MaximumAdmittedRootsPerWorld = 2;
	static constexpr int32 MaximumPeakInstancesPerWorld =
		MaximumAdmittedRootsPerWorld * MaximumPeakInstancesPerRoot;
	static constexpr int32 MaximumPeakMeshComponentsPerWorld =
		MaximumAdmittedRootsPerWorld * MaximumPeakMeshComponentsPerRoot;
	static int32 GetReservedRootCount(const UWorld* World);

	/** No asset loading. Prototype choice is explicit, default-off and fresh-root only. */
	static FAPSFoliageActivationPlan BuildActivationPlan(
		const FAPSResolvedPlanetSurfaceProfile& Profile,
		bool bRuntimeOptIn,
		bool bScaledOrbitalPreview,
		bool bPrototypeOptIn = false);

	/** 0=off, 1=explicit profile/prototype, 2=validated sparse default only. */
	static FAPSFoliageActivationPlan BuildRuntimeActivationPlan(
		const FAPSResolvedPlanetSurfaceProfile& Profile, int32 RuntimeMode,
		bool bScaledOrbitalPreview, bool bPrototypeOptIn, bool bManualPlanet,
		bool bSurfaceScatterOptIn = false);

	/** WorldScape 5.4 cannot safely mutate an applied root involving foliage. */
	static bool RequiresFreshRootForProfileTransition(
		const FAPSResolvedPlanetSurfaceProfile& AppliedProfile,
		const FAPSResolvedPlanetSurfaceProfile& RequestedProfile,
		bool bRootHasActivatedFoliage);

	/** Fresh-root global mode (default2), not a live worker cancellation. */
	static bool IsRuntimeOptInEnabled();

	/** Non-blocking completion fence; caller must first stop the root producer. */
	static bool HasPendingNativeWorker(const AWorldScapeRoot* Root);

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
	 * A root denied by the world admission budget stays empty until recreated;
	 * no unsafe mid-flight collection swap or forced garbage collection is done.
	 */
	static int32 ApplyToFreshOwnedRuntimeRoot(
		AWorldScapeRoot* Root,
		const FAPSResolvedPlanetSurfaceProfile& Profile,
		bool bScaledOrbitalPreview, bool bManualPlanet = false);
};
