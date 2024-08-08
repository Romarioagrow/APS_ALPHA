#pragma once
#include "APS_ALPHA/UI/BaseWidget.h"
#include "WorldDetailsCard.generated.h"

class UGameSave;

UCLASS()
class UWorldDetailsCard : public UBaseWidget
{
	GENERATED_BODY()
	
public:
	UFUNCTION(BlueprintImplementableEvent, Category = "World Details")
	void UpdateDetails(UGameSave* LoadedGame);
};
