#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CivMenuTypes.h"
#include "W_CivMenuRoot.generated.h"

class UWidgetSwitcher;
class UW_CivNavBar;
class UW_CivTopBar;

UCLASS()
class APS_ALPHA_API UW_CivMenuRoot : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable)
	void GoToScreen(ECivMenuScreen Screen);

protected:
	virtual void NativeConstruct() override;

	UPROPERTY(meta=(BindWidget))
	UWidgetSwitcher* Switcher_Content = nullptr;

	UPROPERTY(meta=(BindWidget))
	UW_CivNavBar* NavBar = nullptr;

	UPROPERTY(meta=(BindWidget))
	UW_CivTopBar* TopBar = nullptr;
};
