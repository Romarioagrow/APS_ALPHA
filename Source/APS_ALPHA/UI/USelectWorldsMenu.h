#pragma once

#include "BaseWidget.h"
#include "USelectWorldsMenu.generated.h"

class UInhabitedPlanet;
class UWorldDetailsCard;
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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Worlds")
	TSubclassOf<UInhabitedPlanet> BP_InhabitedPlanetWidgetClass;

	UFUNCTION(BlueprintCallable, Category = "Worlds")
	void UpdateWorldDetails(FString SaveFileName);
	
protected:
	virtual void NativeConstruct() override;
	
	UFUNCTION()
	void GenerateWorld();
	
	void PopulateExistingWorlds();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (BindWidget))
	UUniformGridPanel* UniformGridPanel_ExistingWorlds;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (BindWidget))
	UUniformGridPanel* UniformGridPanel_InhabitedPlanets;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (BindWidget))
	UWorldDetailsCard* ClusterDetailsCard;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (BindWidget))
	UWorldDetailsCard* StarDetailsCard;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (BindWidget))
	UWorldDetailsCard* StarSystemDetailsCard;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (BindWidget))
	UWorldDetailsCard* PlanetDetailsCard;
};
