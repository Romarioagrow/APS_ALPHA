#pragma once

#include "SpaceStation.h"
#include "CoreMinimal.h"
#include "SpaceHeadquarters.generated.h"

UCLASS()
class APS_ALPHA_API ASpaceHeadquarters : public ASpaceStation // AHeadquarters
{
	GENERATED_BODY()

protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY(EditAnywhere)
	UStaticMeshComponent* StartPoint;
	
	UPROPERTY(EditAnywhere)
	UStaticMeshComponent* HQ;

public:
	ASpaceHeadquarters();

	FVector GetStartPointPosition();
};
