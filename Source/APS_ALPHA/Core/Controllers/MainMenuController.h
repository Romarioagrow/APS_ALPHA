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
enum class EAstroPreviewFocus : uint8;
enum class EAPSGenerationRoute : uint8;
enum class EAPSGenerationSurfaceControl : uint8;

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
	void CancelWorldMetadataLoad();
	UWorldGenerationViewModel* GetWorldGenerationViewModel() const { return WorldGenerationViewModel; }
	void HoldSlateResource(UObject* Resource);

#if WITH_DEV_AUTOMATION_TESTS
	/** Direct access is test-only and lets rendered UI tests exercise real Slate
	 * hover/focus geometry without adding production controller surface area. */
	TSharedPtr<SAPSMainMenuRoot> GetSlateMenuRootForAutomation() const
	{
		return SlateMenuRoot;
	}

	/** Enters the actual Slate generator page without relying on synthetic widget clicks. */
	bool OpenAstronomicalGenerationForAutomation(
		EAstroPreviewFocus Focus, EAPSGenerationRoute Route);
	bool CommitSurfaceControlForAutomation(
		EAPSGenerationSurfaceControl Control, double Value);
	double GetSurfaceControlValueForAutomation(
		EAPSGenerationSurfaceControl Control) const;
#endif

private:
	void InstallSlateMenu();
	void ScheduleSlateMenuInstallRetry();
	void RemoveSlateMenu();
	void LoadNextWorldMetadata();
	void OnWorldMetadataLoaded(uint64 RequestGeneration, const FString& SlotName,
		int32 UserIndex, USaveGame* LoadedGame);

	UPROPERTY(Transient)
	TObjectPtr<UGeneratedWorld> MenuGeneratedWorld;

	UPROPERTY(Transient)
	TObjectPtr<UWorldGenerationViewModel> WorldGenerationViewModel;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UObject>> SlateResources;

	TSharedPtr<SAPSMainMenuRoot> SlateMenuRoot;
	TSharedPtr<SWidget> SlateMenuContainer;
	FTimerHandle InstallSlateMenuTimer;
	int32 SlateMenuInstallAttempts{0};
	TArray<FString> PendingMetadataSlots;
	int32 PendingMetadataIndex{0};
	/** Invalidates callbacks from an older browser page without blocking for I/O. */
	uint64 MetadataRequestGeneration{0};
};
