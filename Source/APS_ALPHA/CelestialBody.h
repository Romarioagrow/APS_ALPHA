// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "AstonomicalBody.h"
#include "CoreMinimal.h"
#include "AstroActor.h"
#include "CelestialBody.generated.h"

/**
 * 
 */
UCLASS()
class APS_ALPHA_API ACelestialBody : public AAstroActor, public IAstonomicalBody
{
	GENERATED_BODY()

private:
	UPROPERTY(VisibleAnywhere)
		float Radius;

	UPROPERTY(VisibleAnywhere)
		float Mass;
	
	UPROPERTY(VisibleAnywhere)
		float Age;
	
	UPROPERTY(VisibleAnywhere)
		FName AstroName;

public:
	ACelestialBody();

	void SetRadius(float Radius);

	void SetMass(float Mass);

	void SetAge(float Age);

	void SetName(FName Name);
	
};
