#pragma once

#include "APS_ALPHA/UI/BaseWidget.h"
#include "AstroGenerationMenu.generated.h"

class USpawnClassPicker;
class USpawnParameters;
class UGenerationInput;
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
	void SetupInputs();

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

	UFUNCTION(BlueprintCallable, Category = "World Generation")
	void GenerateWorldByModel();

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Astro Generation")
	TArray<EAstroGenerationLevel> EAstroGenerationLevelArray;

	UPROPERTY(VisibleAnywhere)
	UGeneratedWorld* NewGeneratedWorld;

	UPROPERTY(VisibleAnywhere)
	USpawnParameters* SpawnParameters;
	
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

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (BindWidget))
	UGenerationInput* GI_GalaxySize;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (BindWidget))
	UGenerationInput* GI_GalaxyStarCount;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (BindWidget))
	UGenerationInput* GI_GalaxyStarDensity;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (BindWidget))
	UGenerationInput* GI_PlanetRadius;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (BindWidget))
	UGenerationInput* GI_MoonsAmount;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (BindWidget))
	UGenerationInput* GI_PlanetsAmount;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (BindWidget))
	UGenerationInput* GI_StartPlanetIndex;

private:
	const FString LevelName = TEXT("L_WorldGeneration");
	
	UFUNCTION()
	void OnGalaxySizeChanged(const float InValue);

	UFUNCTION()
	void OnGalaxyStarCountChanged(const float InValue);

	UFUNCTION()
	void OnGalaxyStarDensityChanged(const float InValue);

	UFUNCTION()
	void OnPlanetRadiusChanged(const float InValue);

	UFUNCTION()
	void OnMoonsAmountChanged(const float InValue);

	UFUNCTION()
	void OnPlanetsAmountChanged(const float InValue);

	UFUNCTION()
	void OnStartPlanetIndexChanged(const float InValue);
};
