#pragma once

#include "BaseWidget.h"
#include "USelectWorldsMenu.generated.h"

class UExistingWorld;
class UUniformGridPanel;
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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Worlds")
	TSubclassOf<UExistingWorld> BP_ExistingWorldWidgetClass;
	
protected:
	virtual void NativeConstruct() override;
	
	UFUNCTION()
	void GenerateWorld();
	
	void PopulateExistingWorlds();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (BindWidget))
	UUniformGridPanel* UniformGridPanel_ExistingWorlds;
};
