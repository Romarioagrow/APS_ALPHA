#pragma once

#include "APS_ALPHA/UI/BaseWidget.h"
#include "AstroGenerationMenu.generated.h"

enum class EStarClusterComposition : uint8;
enum class EStarClusterPopulation : uint8;
enum class EStarClusterType : uint8;
enum class EStarClusterSize : uint8;
class AGeneratedWorld;
enum class EAstroGenerationLevel : uint8;
class UGenerationSlider;

UCLASS()
class UAstroGenerationMenu : public UBaseWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;

	UFUNCTION()
	void SetupSlider(UGenerationSlider* Slider, UEnum* EnumType);

	UFUNCTION()
	void HandleGenerationSlider(float Value, const UEnum* EnumClass);
	template <class EnumType>
	void UpdateEnumValue(const UEnum* EnumClass, int32 SelectedValue);

	//void SetupSlider(UGenerationSlider* Slider, UEnum* EnumType, FName DelegateName);

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Astro Generation")
	TArray<EAstroGenerationLevel> EAstroGenerationLevelArray;

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

private:
	UFUNCTION()
	void Update_AstroGenerationLevel(float InValue);

	EAstroGenerationLevel AstroGenerationLevel;

	UPROPERTY()
	AGeneratedWorld* AGeneratedWorld;

	EStarClusterSize StarClusterSize;
	
	EStarClusterType StarClusterType;
	
	EStarClusterPopulation StarClusterPopulation;
	
	EStarClusterComposition StarClusterComposition;


};
