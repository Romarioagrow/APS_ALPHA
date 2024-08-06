#pragma once

#include "BaseWidget.h"
#include "USelectWorldsMenu.generated.h"

class UGeneratedWorld;
class ABaseActor;
class AAstroGenerator;

UCLASS()
class USelectWorldsMenu : public UBaseWidget
{
	GENERATED_BODY()
	
public:
	UPROPERTY()
	TArray<UGeneratedWorld*> ExistingWorlds;
	
	UPROPERTY()
	TArray<UGeneratedWorld*> ExistingPlanets;

	UPROPERTY()
	UGeneratedWorld* LastWorld; 

	UPROPERTY()
	AAstroGenerator* AstroGenerator;

	UPROPERTY()
	UGeneratedWorld* GeneratedWorld;

	UFUNCTION(BlueprintCallable)
	void LoadWorld();

protected:
	UFUNCTION()
	void GenerateWorld();
};
