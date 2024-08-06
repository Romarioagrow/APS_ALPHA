#pragma once
#include "APS_ALPHA/UI/BaseWidget.h"
#include "SpawnItem.generated.h"

class UButton;
class UImage;
class UTextBlock;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnItemClicked, TSubclassOf<AActor>, ClickedClass);

UCLASS()
class USpawnItem : public UBaseWidget
{
	GENERATED_BODY()
	
public:
	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnItemClicked OnItemClicked;
	
	void NativeConstruct();
	
	UFUNCTION()
	void HandleOnClick();
	
	void SetContent(TSubclassOf<AActor> Class);

	UPROPERTY(meta = (BindWidget))
	UButton* Button;

protected:
	UPROPERTY(meta = (BindWidget))
	UTextBlock* TextBlock_ClassName;

	UPROPERTY(meta = (BindWidget))
	UImage* Image_ClassAvatar;

private:
	TSubclassOf<AActor> ItemClass;
};
