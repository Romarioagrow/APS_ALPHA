#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "ItemInfoInterface.generated.h"

UINTERFACE(MinimalAPI)
class UItemInfoInterface : public UInterface
{
	GENERATED_BODY()
};


class APS_ALPHA_API IItemInfoInterface
{
	GENERATED_BODY()

public:
	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Item Info")
	FText GetInGameName() const;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Item Info")
	UTexture2D* GetAvatarPicture() const;
};