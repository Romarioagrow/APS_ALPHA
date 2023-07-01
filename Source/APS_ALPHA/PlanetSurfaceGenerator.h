// Fill out your copyright notice in the Description page of Project Settings.

#pragma once
#include "WorldScapeCore/Public/WorldScapeRoot.h"

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PlanetSurfaceGenerator.generated.h"

UCLASS()
class APS_ALPHA_API APlanetEnvironmentGenerator : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	APlanetEnvironmentGenerator();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

public:
	UPROPERTY(VisibleAnywhere, Category = "World Scape")
		AWorldScapeRoot* WorldScapeRootInstance;

	void InitWorldScape();

	void SetupDefaultGenerator();

};
