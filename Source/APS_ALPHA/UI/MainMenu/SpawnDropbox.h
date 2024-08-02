#pragma once
#include "APS_ALPHA/UI/BaseWidget.h"
#include "SpawnDropbox.generated.h"

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

	void ClearChildren();
	
	void AddChild(UUserWidget* Widget);
	
	void SetContent(TSubclassOf<AActor> Class);
};
