// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AstroLocation.h"
#include "UObject/Interface.h"
#include "AstroObject.generated.h"

// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class UAstroObject : public UAstroLocation
{
	GENERATED_BODY()
};

/**
 * 
 */
class APS_ALPHA_API IAstroObject
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:
};
