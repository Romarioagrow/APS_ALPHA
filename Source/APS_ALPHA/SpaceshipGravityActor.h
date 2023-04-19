// Fill out your copyright notice in the Description page of Project Settings.

#pragma once
#include "Components/StaticMeshComponent.h"
#include "CoreMinimal.h"
#include "GravityActor.h"
#include "SpaceshipGravityActor.generated.h"

/**
 * 
 */
UCLASS()
class APS_ALPHA_API ASpaceshipGravityActor : public AGravityActor
{
	GENERATED_BODY()

public:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
		UStaticMeshComponent* StaticMeshComponent;

	ASpaceshipGravityActor();
};
