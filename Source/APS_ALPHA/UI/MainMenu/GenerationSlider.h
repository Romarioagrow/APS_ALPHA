#pragma once

#include "CoreMinimal.h"
#include "APS_ALPHA/UI/BaseWidget.h"
#include "GenerationSlider.generated.h"

class UTextBlock;
class USlider;
class UImage;
enum class EBaseType : uint8;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnGenerationSliderChanged, const float, Value, const UEnum*, EnumClass, UGenerationSlider*, Slider);

UCLASS()
class UGenerationSlider : public UBaseWidget
{
	GENERATED_BODY()

public:

	UPROPERTY(BlueprintAssignable, Category = "Enum")
	FOnGenerationSliderChanged OnGenerationSliderChanged;

	UFUNCTION()
	void PopulateEnumArray();

	UFUNCTION()
	float GetSliderValue();
	
	UFUNCTION()
	void SetEnumContent(UEnum* InEnum);

	UFUNCTION()
	void UpdateCurrentValueText(const FString& InString);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enum")
	TArray<uint8> EnumArray;
		
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Icons")
	TArray<UTexture2D*> IconArray;
	
	UFUNCTION(BlueprintCallable, Category = "Enum")
	void UpdateIcon(int32 Index);
	
	UFUNCTION(BlueprintCallable, Category = "Enum")
	void OnSliderValueChanged(float Value);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true", BindWidget))
	USlider* EnumSlider;

protected:
	virtual void NativeConstruct() override;
	
	//void FilterEnumValues();

private:
	int32 CurrentIndex;

	UPROPERTY(meta = (BindWidget))
	UImage* IconImage;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true", BindWidget))
	UTextBlock* TextBlock_ValueText;

	UEnum* EnumType;
};
