#pragma once
#include "APS_ALPHA/UI/BaseWidget.h"
#include "SpawnDropbox.generated.h"

class UImage;
class UTextBlock;
class UVerticalBox;
class UExpandableArea;

UCLASS()
class USpawnDropbox : public UBaseWidget
{
	GENERATED_BODY()
	
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (BindWidget))
	UExpandableArea* ExpandableArea;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (BindWidget))
	UVerticalBox* DropdownBody;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (BindWidget))
	UTextBlock* TextBlock_ItemName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (BindWidget))
	UImage* ItemImage;

	void ClearChildren();
	
	void AddChild(UUserWidget* Widget);
	
	void SetContent(TSubclassOf<AActor> Class);
};
