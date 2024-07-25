// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "APS_ALPHA/Generation/AstroGenerator.h"
#include "GameFramework/GameModeBase.h"
#include "GravityGameModeBase.generated.h"

/**
 *
 */
UCLASS()
class APS_ALPHA_API AGravityGameModeBase : public AGameModeBase
{
	GENERATED_BODY()

public:
	AGravityGameModeBase();
	void InitAstroClasses(AAstroGenerator* AstroGenerator);

	virtual void BeginPlay() override;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Astro Object BP")
	TSubclassOf<AGalaxy> BP_GalaxyClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Astro Object BP")
	TSubclassOf<AStarCluster> BP_StarClusterClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Astro Object BP")
	TSubclassOf<APlanetarySystem> BP_PlanetarySystemClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Astro Object BP")
	TSubclassOf<AStarSystem> BP_StarSystemClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Astro Object BP")
	TSubclassOf<AStar> BP_StarClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Astro Object BP")
	TSubclassOf<APlanet> BP_PlanetClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Astro Object BP")
	TSubclassOf<APlanetOrbit> BP_PlanetOrbitClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Astro Object BP")
	TSubclassOf<AMoon> BP_MoonClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Astro Object BP")
	TSubclassOf<AAstroAnchor> BP_AstroAnchorClass;
};
