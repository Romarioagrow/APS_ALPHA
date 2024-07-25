#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "MainGameplayInstance.generated.h"

class UGeneratedWorld;

UCLASS()
class UMainGameplayInstance : public UGameInstanceSubsystem
{
	GENERATED_BODY()
	
public:
	UPROPERTY(BlueprintReadWrite, Category = "World Generation")
	UGeneratedWorld* NewGeneratedWorld;
};
