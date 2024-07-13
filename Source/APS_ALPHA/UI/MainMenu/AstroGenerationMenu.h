#pragma once

#include "APS_ALPHA/UI/BaseWidget.h"
#include "AstroGenerationMenu.generated.h"

enum class EAstroGenerationLevel : uint8;
class UGenerationSlider;

UCLASS()
class UAstroGenerationMenu : public UBaseWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;
	
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Astro Generation")
	TArray<EAstroGenerationLevel> EAstroGenerationLevelArray;

	UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
	UGenerationSlider* GS_GenerationSlider;

	/*UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Astro Generation")
	TMap<UEnum*, TSubclassOf<UGenerationSlider>> EnumSliderMap;*/
	/*UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Astro Generation")
	TMap<UEnum*, TSubclassOf<UGenerationSlider>> EnumSliderMap;*/

	
	
	//UGenerationSlider* GS_GenerationSlider;
	// Класс виджета
	/*UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Astro Generation")
	TSubclassOf<UGenerationSlider> GS_GenerationSliderClass;*/

	// Массив значений Enum
	//UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enum")
	/*TArray<TEnumAsByte<EAstroGenerationLevel>> AstroGenerationLevelEnumArray;*/

private:
	UFUNCTION()
	void Update_AstroGenerationLevel(float InValue);
};
