// Fill out your copyright notice in the Description page of Project Settings.

#pragma once
#include "NavigatableBody.h"

#include "CoreMinimal.h"
#include "Headquarters.h"
#include "SpaceHeadquarters.generated.h"

/**
 * 
 */
UCLASS()
class APS_ALPHA_API ASpaceHeadquarters : public AHeadquarters, public INavigatableBody
{
	GENERATED_BODY()

protected:
	virtual void BeginPlay() override;
	
};
