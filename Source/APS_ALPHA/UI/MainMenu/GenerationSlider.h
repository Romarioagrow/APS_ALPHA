#pragma once

#include "CoreMinimal.h"
#include "APS_ALPHA/UI/BaseWidget.h"
#include "GenerationSlider.generated.h"

class USlider;
class UImage;
enum class EBaseType : uint8;

//DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnValueChangedSignature, float, Value);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnGenerationSliderChanged, const float, Value, const UEnum*, EnumClass);

UCLASS()
class UGenerationSlider : public UBaseWidget
{
	GENERATED_BODY()
	
public:

	UPROPERTY(BlueprintAssignable, Category = "Enum")
	FOnGenerationSliderChanged OnGenerationSliderChanged;

	void PopulateEnumArray(const UEnum* Enum);
	
	void SetEnumContent(UEnum* InEnum);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enum")
	TArray<uint8> EnumArray;
		
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Icons")
	TArray<UTexture2D*> IconArray;
	
	UFUNCTION(BlueprintCallable, Category = "Enum")
	void UpdateIcon(int32 Index);
	
	UFUNCTION(BlueprintCallable, Category = "Enum")
	void OnSliderValueChanged(float Value);

protected:
	virtual void NativeConstruct() override;

private:
	int32 CurrentIndex;

	UPROPERTY(meta = (BindWidget))
	UImage* IconImage;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true", BindWidget))
	USlider* EnumSlider;

	UEnum* EnumType;
};
