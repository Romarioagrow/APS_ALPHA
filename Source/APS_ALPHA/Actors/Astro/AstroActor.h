// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AstroObject.h"
#include "WorldActor.h"
#include "AstroActor.generated.h"

/**
 * 
 */
UCLASS()
class APS_ALPHA_API AAstroActor : public AWorldActor, public IAstroObject
{
	GENERATED_BODY()

public:
	AAstroActor();

	UPROPERTY(VisibleAnywhere)
	UStaticMeshComponent* StaticMeshComponent;
};
