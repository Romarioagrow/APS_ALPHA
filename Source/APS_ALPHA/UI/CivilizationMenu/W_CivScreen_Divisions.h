#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "W_CivScreen_Divisions.generated.h"

UCLASS()
class APS_ALPHA_API UW_CivScreen_Divisions : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;
};
