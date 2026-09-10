#pragma once

#include "CoreMinimal.h"
#include "TechActor.h"
#include "AutonomousOutpost.generated.h"

class USceneComponent;
class UStaticMeshComponent;

UCLASS()
class APS_ALPHA_API AAutonomousOutpost : public ATechActor
{
	GENERATED_BODY()

public:
	AAutonomousOutpost();

private:
	UPROPERTY(VisibleAnywhere, Category = "Outpost|Visual")
	TObjectPtr<USceneComponent> OutpostRoot;

	UPROPERTY(VisibleAnywhere, Category = "Outpost|Visual")
	TObjectPtr<UStaticMeshComponent> Core;

	UPROPERTY(VisibleAnywhere, Category = "Outpost|Visual")
	TObjectPtr<UStaticMeshComponent> SolarWingA;

	UPROPERTY(VisibleAnywhere, Category = "Outpost|Visual")
	TObjectPtr<UStaticMeshComponent> SolarWingB;

	UPROPERTY(VisibleAnywhere, Category = "Outpost|Visual")
	TObjectPtr<UStaticMeshComponent> Antenna;
};
