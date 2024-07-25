#pragma once

#include "BaseWidget.h"
#include "UMainMenuUI.generated.h"

class UGeneratedWorld;
class ABaseActor;
class AAstroGenerator;

UCLASS()
class UMainMenuUI : public UBaseWidget
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

protected:
	UFUNCTION()
	void GenerateWorld();
};
