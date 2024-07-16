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
	UGenerationSlider* GS_GenerationLevel;

private:
	UFUNCTION()
	void Update_AstroGenerationLevel(float InValue);

	EAstroGenerationLevel AstroGenerationLevel;
};
