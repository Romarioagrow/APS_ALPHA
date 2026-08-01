#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "APSPlanetEnvironmentStreamingSubsystem.generated.h"

class APlanetaryBody;

/**
 * Keeps one nearby full-detail WorldScape surface active and leaves every distant
 * celestial body as its inexpensive placeholder mesh. Separate enter/exit radii
 * prevent streaming from oscillating near a boundary.
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

	float UpdateElapsed{0.0f};
	float UpdateInterval{0.5f};
	TWeakObjectPtr<APlanetaryBody> ActiveBody;
};
