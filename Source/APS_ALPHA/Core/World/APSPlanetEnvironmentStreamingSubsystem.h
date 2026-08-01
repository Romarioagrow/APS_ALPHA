#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "APSPlanetEnvironmentStreamingSubsystem.generated.h"

class APlanet;
class APlanetaryBody;

/**
 * Preloads one complete planet/moon family, updates one nearby WorldScape surface,
 * and freezes already generated siblings instead of making them disappear. Separate
 * preload, activation and retention radii prevent both traversal stalls and thrashing.
 */
UCLASS()
class APS_ALPHA_API UAPSPlanetEnvironmentStreamingSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;

private:
	void UpdateActiveEnvironment();
	APlanet* ResolveFamilyPlanet(APlanetaryBody* Body) const;

	float UpdateElapsed{0.0f};
	float UpdateInterval{0.5f};
	TWeakObjectPtr<APlanetaryBody> ActiveBody;
	TWeakObjectPtr<APlanet> ResidentFamily;
};
