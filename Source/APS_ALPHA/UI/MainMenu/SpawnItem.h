#pragma once
#include "APS_ALPHA/UI/BaseWidget.h"
#include "SpawnItem.generated.h"

class UImage;
class UTextBlock;

UCLASS()
class USpawnItem : public UBaseWidget
{
	GENERATED_BODY()
	
public:
	void SetContent(TSubclassOf<AActor> Class);

protected:
	UPROPERTY(meta = (BindWidget))
	UTextBlock* TextBlock_ClassName;

	UPROPERTY(meta = (BindWidget))
	UImage* Image_ClassAvatar;
};
