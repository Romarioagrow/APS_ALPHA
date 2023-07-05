// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "PlanetaryEnvironmentGenerator.h"
#include "PlanetaryEnvironment.h"

#include "CoreMinimal.h"
#include "OrbitalBody.h"
#include "PlanetaryBody.generated.h"

/**
 * 
 */
UCLASS()
class APS_ALPHA_API APlanetaryBody : public AOrbitalBody, public IPlanetaryEnvironment
{
	GENERATED_BODY()

public:
		APlanetaryBody();

protected:
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

public:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Planet Body")
		APlanetaryEnvironmentGenerator* PlanetaryEnvironmentGenerator;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Planet Body")
		bool bGenerateByDefault{ false };

	APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Planet Body")
		double WSCZoneScale {10.0 };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Planet Body")
		bool bEnvironmentSpawned{ false };
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Planet Body")
		double AtmosphereHeight { 0.0 };
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Planet Body")
		double OrbitHeight { 0.0 };
};
