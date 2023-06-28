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

//private:
public:
	UPROPERTY(VisibleAnywhere, Category = "Celestial Body")
		double Radius{ 0 };

	

	UPROPERTY(VisibleAnywhere, Category = "Celestial Body")
		double Mass{ 0 };
	
	UPROPERTY(VisibleAnywhere, Category = "Celestial Body")
		FString Age { "Unknown" };
	
	UPROPERTY(VisibleAnywhere, Category = "Celestial Body")
		FName AstroName;

public:
	ACelestialBody();

	float GetRadius();

	void SetRadius(float Radius);

	void SetMass(float Mass);

	void SetAge(FString Age);

	void SetName(FName Name);
};
