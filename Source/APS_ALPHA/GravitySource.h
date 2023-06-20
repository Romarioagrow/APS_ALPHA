// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "GravityObject.h"
#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "GravitySource.generated.h"

// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class UGravitySource : public UGravityObject
{
	GENERATED_BODY()
};

/**
 * 
 */
class APS_ALPHA_API IGravitySource
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:


	//virtual FVector GetActorUpVectorA();// = 0;
};
