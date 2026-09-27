#pragma once

#include "CoreMinimal.h"
#include "Engine/EngineBaseTypes.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Tickable.h"
#include "APSAuthoredLevelLaunchGate.h"
#include "APSAuthoredLevelLaunchSubsystem.generated.h"

class APlayerController;
class UCivilization;
class UGeneratedWorld;
class USpawnParameters;
class UMainGameplayInstance;
struct FStreamableHandle;

/** Loads the authored map as an asset, not as an active PIE streaming level.
 * The GameInstance owns the preload until the destination owns its assets. */
UCLASS()
class APS_ALPHA_API UAPSAuthoredLevelLaunchSubsystem : public UGameInstanceSubsystem, public FTickableGameObject
{
	GENERATED_BODY()
public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	virtual void Tick(float DeltaTime) override;
	virtual bool IsTickable() const override;
	virtual bool IsTickableWhenPaused() const override { return true; }
	virtual UWorld* GetTickableGameObjectWorld() const override;
	virtual TStatId GetStatId() const override;

	void RequestLaunch(APlayerController* Controller);
	void CancelLaunch();
	bool IsDialogVisible() const { return Gate.Phase != FAPSAuthoredLevelLaunchGate::EPhase::Idle; }
	bool IsOpening() const { return Gate.Phase == FAPSAuthoredLevelLaunchGate::EPhase::Opening; }
	FText GetStatusText() const;

private:
	void OnPostLoadMap(UWorld* LoadedWorld);
	void OnTravelFailure(UWorld* World, ETravelFailure::Type FailureType, const FString& Error);
	void Fail(const FText& Reason);
	void CommitGameplayState(UMainGameplayInstance* Gameplay);
	void RestoreGameplayState();
	void ClearPreviousSelection();
	void ClearOwnedTravelRequest();

	FAPSAuthoredLevelLaunchGate Gate;
	TSharedPtr<FStreamableHandle> PreloadHandle;
	TWeakObjectPtr<APlayerController> RequestController;
	TWeakObjectPtr<UWorld> RequestWorld;
	FDelegateHandle PostLoadMapHandle;
	FDelegateHandle TravelFailureHandle;
	FText FailureReason;
	double NextPollAt{0.0};
	double LastProgressLogAt{0.0};
	int32 RemainingAssets{0};
	bool bInitialized{false};
	bool bGameplayCommitted{false};

	// Strong references preserve the previous selection if travel itself fails.
	UPROPERTY(Transient) TObjectPtr<UGeneratedWorld> PreviousGeneratedWorld;
	UPROPERTY(Transient) TObjectPtr<USpawnParameters> PreviousSpawnParameters;
	UPROPERTY(Transient) TObjectPtr<UCivilization> PreviousCivilization;
	FString PreviousSaveSlot;
	bool bPreviousLoading{false};
	bool bPreviousSavedReplay{false};
	bool bPreviousHierarchyReady{false};
	bool bPreviousAuthored{false};
	bool bPreviousSpawnCivilization{false};
};
