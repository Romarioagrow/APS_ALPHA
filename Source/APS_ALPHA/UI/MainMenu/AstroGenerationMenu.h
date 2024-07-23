﻿#pragma once

#include "APS_ALPHA/UI/BaseWidget.h"
#include "AstroGenerationMenu.generated.h"

class UGenerationSlider;
class UGeneratedWorld;
enum class EStarClusterComposition : uint8;
enum class EStarClusterPopulation : uint8;
enum class EStarClusterType : uint8;
enum class EStarClusterSize : uint8;
enum class EAstroGenerationLevel : uint8;

UCLASS()
class UAstroGenerationMenu : public UBaseWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;

	UFUNCTION()
	void CreateNewGeneratedWorld();

	UFUNCTION()
	void SetupSliders();

	UFUNCTION()
	void SetupSlider(UGenerationSlider* Slider, UEnum* EnumType);

	UFUNCTION()
	void HandleGenerationSlider(float Value, const UEnum* EnumClass, UGenerationSlider* Slider);

	UFUNCTION()
	FString HandleEnumClassName(const UEnum* EnumClass);

	UFUNCTION()
	void UpdateGeneratedWorldEnumValue(const UEnum* EnumClass, int32 SelectedValue);

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Astro Generation")
	TArray<EAstroGenerationLevel> EAstroGenerationLevelArray;

	UPROPERTY(VisibleAnywhere)
	UGeneratedWorld* NewGeneratedWorld;
	
	UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
	UGenerationSlider* GS_GenerationLevel;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (BindWidget))
	UGenerationSlider* GS_StarClusterSize;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (BindWidget))
	UGenerationSlider* GS_StarClusterType;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (BindWidget))
	UGenerationSlider* GS_StarClusterPopulation;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (BindWidget))
	UGenerationSlider* GS_StarClusterComposition;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (BindWidget))
	UGenerationSlider* GS_GalaxyType;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (BindWidget))
	UGenerationSlider* GS_GalaxyClass;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (BindWidget))
	UGenerationSlider* GS_StarType;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (BindWidget))
	UGenerationSlider* GS_StellarType;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (BindWidget))
	UGenerationSlider* GS_SpectralClass;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (BindWidget))
	UGenerationSlider* GS_PlanetType;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (BindWidget))
	UGenerationSlider* GS_PlanetAtmosphere;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (BindWidget))
	UGenerationSlider* GS_SystemPlanetaryType;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (BindWidget))
	UGenerationSlider* GS_SystemDistributionType;
	
};
