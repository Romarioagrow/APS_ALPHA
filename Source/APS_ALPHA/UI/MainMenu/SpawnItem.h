#pragma once
#include "APS_ALPHA/UI/BaseWidget.h"
#include "SpawnItem.generated.h"

UCLASS()
class USpawnItem : public UBaseWidget
{
	GENERATED_BODY()
	
public:
	void SetContent(TSubclassOf<AActor> Class);
};
