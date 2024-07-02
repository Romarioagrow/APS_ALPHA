// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

//#include "BaseInterface.h"
#include "CoreMinimal.h"
#include "APS_ALPHA/Core/Interfaces/BaseInterface.h"
#include "UObject/Interface.h"
#include "GravityObject.generated.h"

// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class UGravityObject : public UBaseInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class APS_ALPHA_API IGravityObject
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:

	// get up vector

	// get forward vector

	// get location 

};
