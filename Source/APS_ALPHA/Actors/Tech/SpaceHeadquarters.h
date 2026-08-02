#pragma once

#include "SpaceStation.h"
#include "CoreMinimal.h"
#include "SpaceHeadquarters.generated.h"

class UCivilization;

UCLASS()
class APS_ALPHA_API ASpaceHeadquarters : public ASpaceStation // AHeadquarters
{
	GENERATED_BODY()

protected:
	virtual void BeginPlay() override;
	virtual float GetGravityVolumeRadiusMultiplier() const override { return 3.0f; }

private:
	UPROPERTY(EditAnywhere)
	UStaticMeshComponent* StartPoint;
	
	UPROPERTY(EditAnywhere)
	UStaticMeshComponent* HQ;

public:
	ASpaceHeadquarters();

	/** Runtime civilization selected in the Slate generation flow. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Civilization")
	TObjectPtr<UCivilization> Civilization;

	FVector GetStartPointPosition();
};
