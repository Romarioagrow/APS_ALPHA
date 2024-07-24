#pragma once

#include "APS_ALPHA/UI/BaseWidget.h"
#include "GenerationInput.generated.h"

class USpinBox;

UCLASS()
class UGenerationInput : public UBaseWidget
{
	GENERATED_BODY()
	
public:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (BindWidget))
	USpinBox* SpinBox_Value;

	UFUNCTION()
	float GetCurrentValue();
};
