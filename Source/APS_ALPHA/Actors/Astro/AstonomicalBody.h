// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "AstroObject.h"
#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "AstonomicalBody.generated.h"

// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class UAstonomicalBody : public UAstroObject
{
	GENERATED_BODY()
};

/**
 * 
 */
class APS_ALPHA_API IAstonomicalBody
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:
};
