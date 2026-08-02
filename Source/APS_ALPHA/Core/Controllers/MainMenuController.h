// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "MainMenuController.generated.h"

class SAPSMainMenuRoot;
class SWidget;
class UGeneratedWorld;
class UWorldGenerationViewModel;
class USaveGame;

/**
 * 
 */
UCLASS()
class APS_ALPHA_API AMainMenuController : public APlayerController
{
	GENERATED_BODY()
	
public:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UFUNCTION(BlueprintCallable, Category = "Saves")
	void SetSaveSlotName(FString OutSaveSlotName);

	UFUNCTION(BlueprintCallable, Category = "Saves")
	void SetLoadingModeTrue();

	void LaunchSingleGame();
	void LoadWorldSlot(const FString& SaveFileName);
	void LoadWorldMetadataAsync(const TArray<FString>& SlotNames);
	UWorldGenerationViewModel* GetWorldGenerationViewModel() const { return WorldGenerationViewModel; }
	void HoldSlateResource(UObject* Resource);

private:
	void InstallSlateMenu();
	void RemoveSlateMenu();
	void LoadNextWorldMetadata();
	void OnWorldMetadataLoaded(const FString& SlotName, int32 UserIndex, USaveGame* LoadedGame);

	UPROPERTY(Transient)
	TObjectPtr<UGeneratedWorld> MenuGeneratedWorld;

	UPROPERTY(Transient)
	TObjectPtr<UWorldGenerationViewModel> WorldGenerationViewModel;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UObject>> SlateResources;

	TSharedPtr<SAPSMainMenuRoot> SlateMenuRoot;
	TSharedPtr<SWidget> SlateMenuContainer;
	FTimerHandle InstallSlateMenuTimer;
	TArray<FString> PendingMetadataSlots;
	int32 PendingMetadataIndex{0};
};
