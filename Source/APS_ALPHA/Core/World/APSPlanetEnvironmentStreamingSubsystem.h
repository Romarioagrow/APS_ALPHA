#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "APSPlanetEnvironmentStreamingSubsystem.generated.h"

class APlanet;
class APlanetaryBody;
class APawn;
class AWorldScapeRoot;

/**
 * Preloads a bounded set within one family, updates the nearby WorldScape surface,
 * warms at most one sibling producer at a time up to the speculative budget,
 * and freezes already generated siblings instead of making them disappear. Separate
 * preload, activation and retention radii prevent both traversal stalls and thrashing.
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

private:
	void UpdateActiveEnvironment();
	APlanet* ResolveFamilyPlanet(APlanetaryBody* Body) const;
	void ApplyGameplayObserverContract(AWorldScapeRoot* Root, APawn* Observer);
	void RefreshGameplayObserverPosition();
	void ClearGameplayCollisionAnchor();
	void RefreshVisibleLiquidAppearance();
	void UpdateStandbyWarmup(APlanetaryBody* Candidate, APawn* Observer);

	float UpdateElapsed{0.0f};
	float UpdateInterval{0.5f};
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
};
