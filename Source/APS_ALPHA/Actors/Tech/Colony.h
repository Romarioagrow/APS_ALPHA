#pragma once

#include "CoreMinimal.h"
#include "TechActor.h"
#include "Colony.generated.h"

class USceneComponent;
class UStaticMeshComponent;

UCLASS()
class APS_ALPHA_API AColony : public ATechActor
{
	GENERATED_BODY()

public:
	AColony();

private:
	UPROPERTY(VisibleAnywhere, Category = "Colony|Visual")
	TObjectPtr<USceneComponent> ColonyRoot;

	UPROPERTY(VisibleAnywhere, Category = "Colony|Visual")
	TObjectPtr<UStaticMeshComponent> Foundation;

	UPROPERTY(VisibleAnywhere, Category = "Colony|Visual")
	TObjectPtr<UStaticMeshComponent> HabitatDome;

	UPROPERTY(VisibleAnywhere, Category = "Colony|Visual")
	TArray<TObjectPtr<UStaticMeshComponent>> Towers;
};
