// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include <Components/SphereComponent.h>
#include "GravitySource.h"
#include "CoreMinimal.h"
#include "TechActor.h"
#include "SpaceStation.generated.h"

/**
 * 
 */
UCLASS()
class APS_ALPHA_API ASpaceStation : public ATechActor, public IGravitySource
{
	GENERATED_BODY()
	
public:
	ASpaceStation();

	//virtual void BeginPlay() override;

	//virtual void Tick(float DeltaTime) override;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
		USphereComponent* SphereCollisionComponent;

};
