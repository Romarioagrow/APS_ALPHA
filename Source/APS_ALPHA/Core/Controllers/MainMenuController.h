// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "MainMenuController.generated.h"

class SAPSMainMenuRoot;
class SWidget;
class UGeneratedWorld;
class UWorldGenerationViewModel;

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
	UWorldGenerationViewModel* GetWorldGenerationViewModel() const { return WorldGenerationViewModel; }
	void HoldSlateResource(UObject* Resource);

private:
	void InstallSlateMenu();
	void RemoveSlateMenu();

	UPROPERTY(Transient)
	TObjectPtr<UGeneratedWorld> MenuGeneratedWorld;

	UPROPERTY(Transient)
	TObjectPtr<UWorldGenerationViewModel> WorldGenerationViewModel;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UObject>> SlateResources;

	TSharedPtr<SAPSMainMenuRoot> SlateMenuRoot;
	TSharedPtr<SWidget> SlateMenuContainer;
	FTimerHandle InstallSlateMenuTimer;
};
