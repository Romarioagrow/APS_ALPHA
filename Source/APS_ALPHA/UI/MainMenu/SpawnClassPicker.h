#pragma once
#include "APS_ALPHA/UI/BaseWidget.h"
#include "SpawnClassPicker.generated.h"

class USpawnDropbox;

UCLASS()
class USpawnClassPicker : public UBaseWidget
{
	GENERATED_BODY()
	
public:
	virtual void NativeConstruct() override;

	/*
	 * BP_HomeSpaceHeadquarters
	 * BP_HomeSpaceStation
	 * BP_HomeSpaceship
	 * BP_CharacterClass
	 */
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Widgets")
	TSubclassOf<UUserWidget> BP_SpawnItemClass;

	UPROPERTY(meta = (BindWidget))
	USpawnDropbox* CharacterDropbox;
	
	UPROPERTY(meta = (BindWidget))
	USpawnDropbox* SpaceshipDropbox;

	UPROPERTY(meta = (BindWidget))
	USpawnDropbox* SpaceStationDropbox;

	UPROPERTY(meta = (BindWidget))
	USpawnDropbox* SpaceHeadquartersDropbox;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Classes")
	TArray<TSubclassOf<AActor>> SpaceshipClasses;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Classes")
	TArray<TSubclassOf<AActor>> CharacterClasses;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Classes")
	TArray<TSubclassOf<AActor>> SpaceStationClasses;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Classes")
	TArray<TSubclassOf<AActor>> SpaceHeadquartersClasses;

	void InitializeClassPicker();
	
	void AddClassToDropbox(TSubclassOf<AActor> Class, USpawnDropbox* Dropbox);
};
