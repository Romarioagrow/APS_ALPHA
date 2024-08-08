#pragma once
#include "APS_ALPHA/UI/BaseWidget.h"
#include "ExistingWorld.generated.h"

UCLASS()
class UExistingWorld : public UBaseWidget
{
	GENERATED_BODY()
	
public:
	void InitializeFromSave(const FString& SaveFileName);
};
