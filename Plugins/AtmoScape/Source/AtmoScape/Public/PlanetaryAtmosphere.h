// Copyright 2021 IOLACORP STUDIO. All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/Actor.h"
#include "Math/Color.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UObject/Object.h"
#include "UObject/ConstructorHelpers.h"
#if WITH_EDITOR
#include "AtmoScapeEditor/Public/EditorUtils.h"
#endif
#include "PlanetaryAtmosphere.generated.h"

class USceneComponent;
class UStaticMeshComponent;

UCLASS(BlueprintType)

class ATMOSCAPE_API AAtmoScape : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	AAtmoScape();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual bool ShouldTickIfViewportsOnly() const override;
	virtual void Tick(float DeltaTime) override;
	virtual void OnConstruction(const FTransform& Transform) override;
	void UpdateMaterialParameters(UMaterialInstanceDynamic* Material, float AtmosRadius, float PlanetScale,
	                              float LightSegLength, float ScaleHeightR, float ScaleHeightM,
	                              float ScaleSkylightIntensity, float ScaleSkylightShadow, float ScaleStartDistanceAO,
	                              float ScaleStepsNumAO, float ScaleAirGlowIntensity,
	                              FLinearColor MieScatteringCoef, FLinearColor RayleighScatteringCoef,
	                              FLinearColor OutterColorCoef, FLinearColor InsideColorCoef,
	                              FLinearColor RayleighOzoneScateringCoef, FVector Vec_PlanetScale,
	                              FLinearColor AtmoBackHSV
	);

	void UpdateScale();
	/** Same optical inputs as UpdateScale, with caller-owned preview shell transforms. */
	void UpdatePresentationScale();

	//Atmosphere Radius in Km.
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Planet")
	float PlanetRadius = 6360.0f;

	float LastPlanetRadius = 6360.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Atmosphere")
	UMaterialInterface* Atmo_Material;
	UPROPERTY(BlueprintReadOnly, Category = "Atmosphere")
	UMaterialInterface* SpaceAtmo_Material;
	UPROPERTY(BlueprintReadOnly, Category = "Skylight")
	UMaterialInterface* PlanetarySkylight_Material;
	UPROPERTY(BlueprintReadOnly, Category = "Absorbtion")
	UMaterialInterface* Absorbtion_Material;
	UPROPERTY(BlueprintReadOnly, Category = "AirGlow")
	UMaterialInterface* Outter_Material;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Atmosphere")
	bool bKeepRelativeScale = true;

	/**
	 * Optional centimetre radii used only by the ray-march material. They let a
	 * uniformly downscaled presentation shell retain physically coherent shader
	 * distances without changing the full-scale atmosphere geometry. Zero keeps
	 * the original physical-radius behaviour.
	 */
	UPROPERTY(BlueprintReadWrite, Transient, Category = "Atmosphere | Presentation", meta = (ClampMin = "0"))
	float PresentationPlanetRadiusCm = 0.0f;

	UPROPERTY(BlueprintReadWrite, Transient, Category = "Atmosphere | Presentation", meta = (ClampMin = "0"))
	float PresentationAtmosphereRadiusCm = 0.0f;

	/** Preview-only multiplier for the material's real LightIntensity parameter. */
	UPROPERTY(BlueprintReadWrite, Transient, Category = "Atmosphere | Presentation", meta = (ClampMin = "0"))
	float PresentationLightIntensity = 1.0f;

	/** Material-only opacity scale for uniformly downscaled orbital previews. */
	UPROPERTY(BlueprintReadWrite, Transient, Category = "Atmosphere | Presentation", meta = (ClampMin = "0"))
	float PresentationOpacityScale = 1.0f;

	//Change the height of the atmosphere, thicker atmospheres need to be balanced with opacity, default is physically accurate to earth
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Atmosphere",
		meta = (UIMin = "0", UIMax = "700", ClampMin = "0"))
	float AtmosphereHeight = 200.0f;
	//Change the opacity of the atmosphere 
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Atmosphere", meta = (UIMin = "0", UIMax = "100"))
	float AtmosphereOpacity = 20.0;
	//Boost the saturation of atmosphere colours
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Atmosphere",
		meta = (UIMin = "0.01", UIMax = "10", ClampMin = "0.01", ClampMax = "10"))
	float MultiScatering = 1.0f;
	//Adds hazier look to the horizon
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Atmosphere",
		meta = (UIMin = "0", UIMax = "40", ClampMin = "0", ClampMax = "40"))
	float AtmosphereParticulatesDensity = 15;

	float LastAtmosphereHeight = 100.0f;


	//AirGlow

	//Rayleigh Parameters

	// Controls the effect ozone has on the the atmosphere, the effect is subtly more accurate colours overall (value is *10^−6)
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Atmosphere | Rayleigh")
	FLinearColor RayleighScattering = FLinearColor(5.267816f, 10.828321f, 33.099998f, 0);
	// FLinearColor(3.8f, 13.5f, 33.099998f, 0);

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Atmosphere | Rayleigh",
		meta = (UIMin = "0", UIMax = "80.0"))
	float RayleighHeight = 8.0;

	//Mie Parameters

	//Controls the height of haze within the atmosphere
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Atmosphere | Mie", meta = (UIMin = "0.01", UIMax = "15"))
	float MieHeight = 1.2;
	//Controls the direction of the Mie scattering, should be positive for foward scattering (negative numbers scatter backwards), default is accurate to earth
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Atmosphere | Mie",
		meta = (AdvancedDisplay = "2", UIMin = "-0.935", UIMax = "0.935", ClampMin = "-0.935", ClampMax = "0.935"))
	float MiePhase = 0.25;
	//Controls the amount of haze in the atmosphere, has an influence on sunset colours (value is *10^−6)
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Atmosphere | Mie")
	FLinearColor MieScattering = FLinearColor(1.0f, 0.59f, 0.27f, 1.0f);

	//Absorption Parameters

	//Increase or decrease the ozone contribution, 0 turns ozone off
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Atmosphere | Absorption",
		meta = (UIMin = "0", UIMax = "1", ClampMin = "0", ClampMax = "10"))
	float OzoneContribution = 0.5;
	//Controls the effect ozone has on the the atmosphere, the effect is subtly more accurate colours overall (value is *10^−6)
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Atmosphere | Absorption")
	FLinearColor Absorption = FLinearColor(5.71f, 13.83f, 0.59333f, 1.0f);

	//Samples for camera ray, higher increases quality, lower increases performance
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Atmosphere",
		meta = (AdvancedDisplay = "2", UIMin = "8", UIMax = "64", ClampMin = "8", ClampMax = "128"))
	int CameraSamplesCount = 32;
	//Samples for light ray, higher increases quality, lower increases performance, setting this to half of camera samples is usually fine
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Atmosphere",
		meta = (AdvancedDisplay = "2", UIMin = "8", UIMax = "32", ClampMin = "8", ClampMax = "64"))
	int LightSamplesCount = 16;

	//SkyLight
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Skylight")
	float SkylightIntensity = 25.0f;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Skylight",
		meta = (UIMin = "0", UIMax = "1.9", ClampMin = "0.01", ClampMax = "1.98"))
	float SkylightShadow = 1.0f;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Skylight | AmbiantOcclusion",
		meta = (AdvancedDisplay = "2", UIMin = "8", UIMax = "64", ClampMin = "8", ClampMax = "128"))
	float StepsNumAO = 32;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Skylight | AmbiantOcclusion",
		meta = (AdvancedDisplay = "2", UIMin = "-8000.0", UIMax = "1.0", ClampMin = "-8000.0", ClampMax = "1.0"))
	float StartDistanceAO = 1.0f;

	//Controls the effect ozone has on the the atmosphere, the effect is subtly more accurate colours overall (value is *10^−6)
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "AirGlow")
	FLinearColor OutterColor = FLinearColor(0.0000f, 1.0000f, 0.0046f, 1.0000f);
	//Controls the effect ozone has on the the atmosphere, the effect is subtly more accurate colours overall (value is *10^−6)
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "AirGlow")
	FLinearColor InsideColor = FLinearColor(1.0000f, 0.6592f, 0.0000f, 1.0000f);

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "AirGlow")
	float AirGlowIntensity = 0.01f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Light")
	AActor* LightSource = nullptr;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Light", meta = (EditCondition = "LightSource != nullptr"))
	bool bSampleDistance = false;

private:
	void UpdateScaleInternal(bool bUpdatePhysicalTransforms);
	UPROPERTY()
	UMaterialInstanceDynamic* PlanetaryOutterMaterial;
	UPROPERTY()
	UMaterialInstanceDynamic* AtmosphereMaterial;
	UPROPERTY()
	UMaterialInstanceDynamic* PlanetaryAbsorptionMaterial;
	UPROPERTY()
	UMaterialInstanceDynamic* SpaceAtmosphereMaterial;
	UPROPERTY()
	UMaterialInstanceDynamic* PlanetarySkylightMaterial;
	UPROPERTY()
	class USceneComponent* Root;
	UPROPERTY()
	class UStaticMeshComponent* PlanetaryAtmoMesh;
	UPROPERTY()
	class UStaticMeshComponent* SpacePlanetaryAtmoMesh;
	UPROPERTY()
	class UStaticMeshComponent* PlanetarySkylightMesh;
	UPROPERTY()
	class UStaticMeshComponent* PlanetaryAbsorptionMesh;
	UPROPERTY()
	class UStaticMeshComponent* PlanetaryOutterMesh;
};
