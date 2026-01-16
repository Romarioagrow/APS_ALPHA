#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "W_CivScreen_Overview.generated.h"

UCLASS()
class APS_ALPHA_API UW_CivScreen_Overview : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;
};
