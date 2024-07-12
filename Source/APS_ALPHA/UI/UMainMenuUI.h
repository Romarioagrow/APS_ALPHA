#pragma once

#include "BaseWidget.h"
#include "UMainMenuUI.generated.h"

class AGeneratedWorld;
class ABaseActor;
class AAstroGenerator;

UCLASS()
class UMainMenuUI : public UBaseWidget
{
	GENERATED_BODY()
	
public:

	UPROPERTY()
	TArray<AGeneratedWorld*> ExistingWorlds;
	
	UPROPERTY()
	TArray<AGeneratedWorld*> ExistingPlanets;

	UPROPERTY()
	AGeneratedWorld* LastWorld; // ALastWorld*

	UPROPERTY()
	AAstroGenerator* AstroGenerator;

	UPROPERTY()
	AGeneratedWorld* GeneratedWorld; // AGeneratedWorld*

protected:
	UFUNCTION()
	void GenerateWorld();
};
