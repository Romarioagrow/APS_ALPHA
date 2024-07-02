// Fill out your copyright notice in the Description page of Project Settings.

#pragma once
#include "WorldScapeCore/Public/WorldScapeRoot.h"
#include "AtmoScape/Public/Atmosphere.h"
//#include "PlanetType.h"
#include <Kismet/GameplayStatics.h>

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PlanetaryEnvironmentGenerator.generated.h"

class APlanet;  
class AMoon;  
class APlanetaryBody;  

USTRUCT(BlueprintType)
struct FAmbientParameters
{
	GENERATED_USTRUCT_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Atmosphere", meta = (UIMin = "0.1", UIMax = "100.0"))
		float Opacity = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Atmosphere", meta = (UIMin = "0.1", UIMax = "100.0"))
		float MultiScatering = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Atmosphere", meta = (UIMin = "0.1", UIMax = "80.0"))
		float RayleighHeight = 8.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Atmosphere", meta = (UIMin = "0.01", UIMax = "15.0"))
		float MieHeight = 1.2f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Atmosphere", meta = (UIMin = "-0.935", UIMax = "0.935"))
		float MiePhase = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Atmosphere", meta = (UIMin = "0.0", UIMax = "1.0"))
		float OzoneContribution = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Atmosphere")
		FColor RayleighColor = FColor::Blue;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Atmosphere")
		FColor MieColor = FColor::Blue;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Atmosphere")
		FColor AbsorptionColor = FColor::Blue;
};

USTRUCT(BlueprintType)
struct FPlanetColorParams
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Planet")
		FColor MinColor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Planet")
		FColor MaxColor;
};

UCLASS()
class APS_ALPHA_API APlanetaryEnvironmentGenerator : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	APlanetaryEnvironmentGenerator();

	FAmbientParameters AmbientParams;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Noises")
		UWorldScapeNoiseClass* MoonLikeNoise;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Noises")
		UWorldScapeNoiseClass* LavaWorldNoise;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Noises")
		UWorldScapeNoiseClass* SelenaeNoise;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Noises")
		UWorldScapeNoiseClass* SelenaeMetalNoise;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Noises")
		UWorldScapeNoiseClass* EarthLikeNoise;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Noises")
		UWorldScapeNoiseClass* EarthNoise;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Noises")
		UWorldScapeNoiseClass* TerraNoise;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Noises")
		UWorldScapeNoiseClass* IceWorldNoise;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Noises")
		UWorldScapeNoiseClass* TerraDesert;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Noises")
		UWorldScapeNoiseClass* TerraForestNoise;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Materials")
		UMaterialInstance* MI_Terra;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Materials")
		UMaterialInstance* MI_Selenae;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Materials")
		UMaterialInstance* MI_Magma;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Materials")
		UMaterialInstance* MI_Planetary_Ocean;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Materials")
		UMaterialInstance* MI_Lava_Ocean;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	void InitEnviroment(APlanet* NewPlanet, UWorld* World);

	void InitAtmoScape(UWorld* World, double Radius, APlanetaryBody* PlanetaryBody);

public:
	UPROPERTY(VisibleAnywhere, Category = "World Scape")
		AWorldScapeRoot* WorldScapeRootInstance;

	UPROPERTY(VisibleAnywhere, Category = "Atmo Scape")
		AAtmosphere* PlanetAtmosphere;

	UPROPERTY(VisibleAnywhere, Category = "World Scape")
		double RadiusKM;

	APlanetaryBody* PlanetaryBody;

	void InitWorldScape(UWorld* World);

	void GenerateWorldscapeSurfaceByModel(UWorld* World, APlanet* NewPlanet);

	void GenerateWorldscapeSurfaceByModel(UWorld* World, AMoon* NewMoon);

	void SpawnWorldScapeRoot();

	void DestroyPlanetEnvironment();

};
