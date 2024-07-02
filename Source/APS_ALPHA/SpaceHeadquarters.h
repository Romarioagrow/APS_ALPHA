 // Fill out your copyright notice in the Description page of Project Settings.

#pragma once
#include "NavigatableBody.h"
#include "SpaceStation.h"
#include <Components/SphereComponent.h>

#include "CoreMinimal.h"
#include "Headquarters.h"
#include "SpaceHeadquarters.generated.h"

/**
 * 
 */
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
