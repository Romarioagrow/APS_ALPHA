#pragma once

#include "CoreMinimal.h"
#include "AstroActor.h"
#include "PlanetOrbit.generated.h"

class APlanet;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnClearChildren);

UCLASS()
class APS_ALPHA_API APlanetOrbit : public AAstroActor
{
	GENERATED_BODY()

public:
	UPROPERTY()
	APlanet* Planet;

	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnClearChildren OnClearChildren;

	UFUNCTION(BlueprintCallable, Category = "PlanetOrbit")
	void TriggerClearChildren();
};
