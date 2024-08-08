#pragma once
#include "APS_ALPHA/UI/BaseWidget.h"
#include "ExistingWorld.generated.h"

class UButton;
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnExistingWorldClicked, FString, SaveFileName);

UCLASS()
class UExistingWorld : public UBaseWidget
{
	GENERATED_BODY()
	
public:
	UPROPERTY(BlueprintAssignable, Category = "Worlds")
	FOnExistingWorldClicked OnClicked;
	
	void InitializeFromSave(const FString& SaveFileName);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (BindWidget))
	UButton* Button;

protected:
	UPROPERTY(BlueprintReadOnly, Category = "Worlds")
	FString SaveFileName;

	UFUNCTION()
	void HandleOnClicked();
};
