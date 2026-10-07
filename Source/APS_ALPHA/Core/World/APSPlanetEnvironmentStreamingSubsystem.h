#pragma once

#include "CoreMinimal.h"
#include "APS_ALPHA/Core/World/APSPlanetArrivalForecast.h"
#include "Subsystems/WorldSubsystem.h"
#include "APSPlanetEnvironmentStreamingSubsystem.generated.h"

class APlanet;
class APlanetaryBody;
class APawn;
class AWorldScapeRoot;
class APlanetarySurfaceGenerator;

/**
 * Preloads a bounded set within one family, updates the nearby WorldScape surface, warms at most one sibling
 * producer at a time up to the speculative budget, and freezes already generated siblings instead of making them
 * disappear. Separate preload, activation and retention radii prevent both traversal stalls and thrashing.
 */
UCLASS()
class APS_ALPHA_API UAPSPlanetEnvironmentStreamingSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void Deinitialize() override;
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;
	APlanet* GetResidentFamily() const { return ResidentFamily.Get(); }
	APlanetaryBody* GetActiveBody() const { return ActiveBody.Get(); }
	/** True once a selection pass has run with a possessed observer. From then on an empty selection is deliberate
	 * (the observer is outside every family's range), not the start-up gap before the first pass. */
	bool HasObservedPawn() const { return bHasObservedPawn; }

private:
	void UpdateActiveEnvironment();
	APlanet* ResolveFamilyPlanet(APlanetaryBody* Body) const;
	void ApplyGameplayObserverContract(AWorldScapeRoot* Root, APawn* Observer);
	void RefreshGameplayObserverPosition();
	void ClearGameplayCollisionAnchor();
	void RefreshVisibleLiquidAppearance();
	void UpdateStandbyWarmup(APlanetaryBody* Candidate, APawn* Observer);
	void UpdateFlightResidency(float DeltaTime);
	void CancelFlightReplacement();
	/** The body the observer is flying to (APSPlanetArrivalForecast), or null. In APSPlanetEnvironmentArrival.cpp. */
	APlanetaryBody* UpdateArrivalForecast(APawn* Observer, const TArray<APlanetaryBody*>& Bodies);
	UPROPERTY(Transient)
	TObjectPtr<APlanetarySurfaceGenerator> FlightReplacement;
	TWeakObjectPtr<APlanetaryBody> FlightReplacementBody;
	TWeakObjectPtr<APlanetaryBody> MotionBody;
	TWeakObjectPtr<APawn> MotionObserver;
	FVector PreviousRelativePosition{FVector::ZeroVector};
	float TransitDwell{0};
	bool bFlightReplacementTransit{false};
	bool bFlightReplacementRetiring{false};

	float UpdateElapsed{0.0f};
	float UpdateInterval{0.5f};
	bool bHasObservedPawn{false};
	TWeakObjectPtr<APlanetaryBody> ActiveBody;
	// At most one hidden sibling may build its FIRST payload in the background.
	TWeakObjectPtr<APlanetaryBody> WarmingBody;
	// Published ahead of a visit. Limit speculative GPU residency as well as workers;
	// previously visited visible surfaces are retained by the existing family policy.
	TArray<TWeakObjectPtr<APlanetaryBody>> PrewarmedBodies;
	TWeakObjectPtr<APlanet> ResidentFamily;
	TWeakObjectPtr<AWorldScapeRoot> AnchoredWorldScapeRoot;
	TWeakObjectPtr<APawn> CollisionAnchorPawn;
	// No world/geometry traversal at frame rate. Rebuilt by the slow selection pass.
	TArray<TWeakObjectPtr<APlanetaryBody>> VisibleLiquidBodies;
	// Arrival forecast (Rio, 01.10: "planets do not load in time"): the observer's position relative to each streamed
	// body on the previous selection pass, and the body it is flying to.
	TMap<TWeakObjectPtr<APlanetaryBody>, FVector> ForecastRelative;
	TWeakObjectPtr<APawn> ForecastObserver;
	double ForecastSeconds{0.0};
	// Confirmed per family (the planet); ArrivingBody is the family's body the observer reaches first.
	APSPlanetArrivalForecast::TTracker<TWeakObjectPtr<APlanetaryBody>> ArrivalTracker;
	TWeakObjectPtr<APlanetaryBody> ArrivingBody;
	// The active surface was started ahead of arrival, far away: no collision until the observer is within its reach.
	bool bAnchorWithoutCollision{false};

	/** Holds a fresh root's native first Tick, which creates every base LOD component in one frame, and creates them
	 * a few per frame instead (aps.Surface.SlicedFirstBuild). In APSPlanetEnvironmentFirstBuild.cpp. */
	void HoldFirstBuild(AWorldScapeRoot* Root, bool bForeground);
	void AdvanceFirstBuilds();
	/** aps.Surface.ProxyProbe diagnostic: logs roots whose LOD meshes got new scene proxies this frame. */
	void ProbeWorldScapeProxies();
	struct FFirstBuild
	{
		TWeakObjectPtr<AWorldScapeRoot> Root;
		int32 Frames{0};
		double Seconds{0.0};
	};
	// Oldest first, the approached body at the front.
	TArray<FFirstBuild> FirstBuilds;
};
