#include "APSAuthoredLevelLaunchSubsystem.h"

#include "APS_ALPHA/Core/Instances/MainGameplayInstance.h"
#include "APS_ALPHA/Core/Model/GeneratedWorld.h"
#include "APS_ALPHA/Core/Model/SpawnParameters.h"
#include "APS_ALPHA/Gameplay/Civilizations/Civilization.h"
#include "APS_ALPHA/Generation/AstroGenerator.h"
#include "Engine/AssetManager.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/StreamableManager.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "HAL/PlatformTime.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/Package.h"
#include "UObject/UObjectGlobals.h"
#if WITH_EDITOR
#include "AssetCompilingManager.h"
#endif

#define LOCTEXT_NAMESPACE "APSAuthoredLevelLaunch"

namespace APSAuthoredLevelLaunch
{
	constexpr const TCHAR* MapPackage = TEXT("/Game/APS/APS_ALPHA/Levels/Alpha/L_APS_SinglePlay_StartLocation");
	constexpr const TCHAR* MapObject = TEXT("/Game/APS/APS_ALPHA/Levels/Alpha/L_APS_SinglePlay_StartLocation.L_APS_SinglePlay_StartLocation");
	int32 GetRemainingAssets()
	{
#if WITH_EDITOR
		// UE 5.4 aggregates mesh, texture, shader and distance-field compilers here.
		// The normal editor tick processes their results within its own budget.
		return FAssetCompilingManager::Get().GetNumRemainingAssets();
#else
		return 0;
#endif
	}
}

void UAPSAuthoredLevelLaunchSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	bInitialized = true;
	PostLoadMapHandle = FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(
		this, &UAPSAuthoredLevelLaunchSubsystem::OnPostLoadMap);
	if (GEngine)
		TravelFailureHandle = GEngine->OnTravelFailure().AddUObject(
			this, &UAPSAuthoredLevelLaunchSubsystem::OnTravelFailure);
}

void UAPSAuthoredLevelLaunchSubsystem::Deinitialize()
{
	bInitialized = false;
	FCoreUObjectDelegates::PostLoadMapWithWorld.Remove(PostLoadMapHandle);
	PostLoadMapHandle.Reset();
	if (GEngine) GEngine->OnTravelFailure().Remove(TravelFailureHandle);
	TravelFailureHandle.Reset();
	Gate.Cancel();
	RequestController.Reset();
	RequestWorld.Reset();
	// No completion delegates capture this subsystem. Ending PIE cannot open a
	// level from an old request, or cancel another PIE instance's asset loading.
	PreloadHandle.Reset();
	ClearPreviousSelection();
	Super::Deinitialize();
}

bool UAPSAuthoredLevelLaunchSubsystem::IsTickable() const
{
	return bInitialized && !IsTemplate() && (Gate.IsPending() || PreloadHandle.IsValid());
}

UWorld* UAPSAuthoredLevelLaunchSubsystem::GetTickableGameObjectWorld() const
{
	return GetGameInstance() ? GetGameInstance()->GetWorld() : nullptr;
}

TStatId UAPSAuthoredLevelLaunchSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UAPSAuthoredLevelLaunchSubsystem, STATGROUP_Tickables);
}

void UAPSAuthoredLevelLaunchSubsystem::RequestLaunch(APlayerController* Controller)
{
	if (!bInitialized || !IsValid(Controller) || !Controller->IsLocalController()
		|| Controller->GetGameInstance() != GetGameInstance() || !Controller->GetWorld()
		|| Controller->GetWorld()->bIsTearingDown) return;
	const double Now = FPlatformTime::Seconds();
	if (!Gate.Begin(Now)) return;
	RequestController = Controller;
	RequestWorld = Controller->GetWorld();
	FailureReason = FText::GetEmpty();
	RemainingAssets = APSAuthoredLevelLaunch::GetRemainingAssets();
	// Retry can reuse a healthy preload retained after cancellation. A failed
	// request is only discarded after workers stop using its asset references.
	if (PreloadHandle.IsValid() && (PreloadHandle->WasCanceled()
		|| (PreloadHandle->HasLoadCompleted() && !Cast<UWorld>(PreloadHandle->GetLoadedAsset()))))
	{
		if (RemainingAssets > 0)
		{
			Fail(LOCTEXT("RetryBusy", "Asset jobs from the previous attempt are still running. Return and retry when they finish."));
			return;
		}
		PreloadHandle.Reset();
	}
	NextPollAt = Now + 0.1; // Give Slate a frame to display the cancellable dialog.
	LastProgressLogAt = Now;
	UE_LOG(LogTemp, Log, TEXT("[APS.SinglePlay] Preparing authored map asynchronously; gameplay state unchanged"));
}

void UAPSAuthoredLevelLaunchSubsystem::CancelLaunch()
{
	if (IsOpening()) return; // Final engine travel has already been queued.
	Gate.Cancel();
	RequestController.Reset();
	RequestWorld.Reset();
	FailureReason = FText::GetEmpty();
	// Retain in-flight references: releasing them into GC can synchronously join
	// cold mesh work. Idle ticking releases this handle once workers have finished.
	UE_LOG(LogTemp, Log, TEXT("[APS.SinglePlay] Launch cancelled; no gameplay handoff committed"));
}

void UAPSAuthoredLevelLaunchSubsystem::ClearOwnedTravelRequest()
{
	if (!GEngine || !RequestWorld.IsValid()) return;
	if (FWorldContext* Context = GEngine->GetWorldContextFromWorld(RequestWorld.Get()))
	{
		// Do not clear any unrelated travel or global asynchronous loading.
		if (Context->TravelURL == APSAuthoredLevelLaunch::MapPackage
			|| Context->TravelURL.StartsWith(FString(APSAuthoredLevelLaunch::MapPackage) + TEXT("?")))
			Context->TravelURL.Reset();
	}
}

void UAPSAuthoredLevelLaunchSubsystem::Fail(const FText& Reason)
{
	if (bGameplayCommitted)
	{
		ClearOwnedTravelRequest();
		RestoreGameplayState();
	}
	Gate.Fail();
	FailureReason = Reason;
	RequestController.Reset();
	RequestWorld.Reset();
	UE_LOG(LogTemp, Error, TEXT("[APS.SinglePlay] Launch failed: %s"), *Reason.ToString());
}

void UAPSAuthoredLevelLaunchSubsystem::Tick(float DeltaTime)
{
	const double Now = FPlatformTime::Seconds();
	if (Now < NextPollAt) return;
	NextPollAt = Now + 0.1;
	if (IsOpening())
	{
		// This protects queued travel, not an engine call already blocking a tick.
		// Cold compiler waits are prevented before OpenLevel, below.
		Gate.Advance(Now, true, false, 0, true);
		if (Gate.Phase == FAPSAuthoredLevelLaunchGate::EPhase::Failed)
			Fail(LOCTEXT("TravelTimeout", "The level transition timed out. Return to the menu and check the project log."));
		return;
	}
	RemainingAssets = APSAuthoredLevelLaunch::GetRemainingAssets();
	if (!Gate.IsPending())
	{
		if (PreloadHandle.IsValid() && (PreloadHandle->HasLoadCompleted() || PreloadHandle->WasCanceled()) && RemainingAssets == 0)
			PreloadHandle.Reset();
		return;
	}
	APlayerController* Controller = RequestController.Get();
	UWorld* World = RequestWorld.Get();
	if (!Controller || !World || World->bIsTearingDown || GetTickableGameObjectWorld() != World)
	{
		CancelLaunch();
		return;
	}
	if (!PreloadHandle.IsValid())
	{
		// Do NOT LoadStreamLevel: registering cold meshes in a live PIE world
		// invokes FinishCompilationsForGame and blocks the game/UI thread.
		PreloadHandle = UAssetManager::GetStreamableManager().RequestAsyncLoad(
			FSoftObjectPath(APSAuthoredLevelLaunch::MapObject), FStreamableDelegate(),
			FStreamableManager::DefaultAsyncLoadPriority, false, false, TEXT("APS.AuthoredSinglePlay"));
		if (!PreloadHandle.IsValid())
		{
			Fail(LOCTEXT("RequestFailed", "Unable to request the authored level. Return and try again."));
			return;
		}
	}
	const bool bLoaded = PreloadHandle->HasLoadCompleted();
	const bool bLoadFailed = PreloadHandle->WasCanceled()
		|| (bLoaded && !Cast<UWorld>(PreloadHandle->GetLoadedAsset()));
	bool bPreviewDrained = false;
	if (bLoaded && !bLoadFailed && RemainingAssets == 0)
	{
		bPreviewDrained = true;
		for (TActorIterator<AAstroGenerator> It(World); It; ++It)
			if (It->ActorHasTag(TEXT("WorldGenerationPreview")))
				bPreviewDrained &= It->PreparePreviewForTravel();
		RemainingAssets = APSAuthoredLevelLaunch::GetRemainingAssets();
	}
	const bool bOpen = Gate.Advance(Now, bLoaded, bLoadFailed, RemainingAssets, bPreviewDrained);
	if (Gate.Phase == FAPSAuthoredLevelLaunchGate::EPhase::Failed)
	{
		Fail(bLoadFailed
			? LOCTEXT("LoadFailed", "The authored level could not be loaded. Check the project log.")
			: LOCTEXT("TimedOut", "Preparation timed out. Return to the menu; asset jobs may finish in the background."));
		return;
	}
	if (Now - LastProgressLogAt >= 5.0)
	{
		LastProgressLogAt = Now;
		UE_LOG(LogTemp, Log, TEXT("[APS.SinglePlay] Preparing phase=%d loaded=%d remainingEditorAssets=%d elapsed=%.1fs"),
			static_cast<int32>(Gate.Phase), bLoaded, RemainingAssets, Now - Gate.StartedAt);
	}
	if (!bOpen) return;
	// A save/generated route can be triggered outside the disabled menu. If it
	// has queued travel, do not replace its destination or gameplay selection.
	const FWorldContext* Context = GEngine ? GEngine->GetWorldContextFromWorld(World) : nullptr;
	if (!Context || !Context->TravelURL.IsEmpty())
	{
		Gate.Cancel();
		CancelLaunch();
		return;
	}
	UMainGameplayInstance* Gameplay = GetGameInstance()->GetSubsystem<UMainGameplayInstance>();
	if (!Gameplay)
	{
		Fail(LOCTEXT("StateMissing", "Gameplay state is unavailable. The level was not opened."));
		return;
	}
	CommitGameplayState(Gameplay);
	UE_LOG(LogTemp, Log, TEXT("[APS.SinglePlay] Preparation complete in %.1fs; opening authored map once"), Now - Gate.StartedAt);
	UGameplayStatics::OpenLevel(Controller, FName(APSAuthoredLevelLaunch::MapPackage), true);
}

void UAPSAuthoredLevelLaunchSubsystem::CommitGameplayState(UMainGameplayInstance* Gameplay)
{
	PreviousGeneratedWorld = Gameplay->NewGeneratedWorld;
	PreviousSpawnParameters = Gameplay->SpawnParameters;
	PreviousCivilization = Gameplay->CurrentCivilization;
	PreviousSaveSlot = Gameplay->SaveSlotName;
	bPreviousLoading = Gameplay->bIsLoadingMode;
	bPreviousSavedReplay = Gameplay->bPendingSavedWorldReplay;
	bPreviousHierarchyReady = Gameplay->bSavedWorldHierarchyReady;
	bPreviousAuthored = Gameplay->bUseAuthoredSinglePlayWorld;
	bPreviousSpawnCivilization = Gameplay->bSpawnGeneratedCivilization;
	bGameplayCommitted = true;
	Gameplay->bIsLoadingMode = false;
	Gameplay->bPendingSavedWorldReplay = false;
	Gameplay->bSavedWorldHierarchyReady = false;
	Gameplay->bUseAuthoredSinglePlayWorld = true;
	Gameplay->bSpawnGeneratedCivilization = false;
	Gameplay->SaveSlotName.Reset();
	Gameplay->NewGeneratedWorld = nullptr;
	Gameplay->SpawnParameters = nullptr;
	Gameplay->CurrentCivilization = nullptr;
}

void UAPSAuthoredLevelLaunchSubsystem::RestoreGameplayState()
{
	if (!bGameplayCommitted) return;
	if (UMainGameplayInstance* Gameplay = GetGameInstance()->GetSubsystem<UMainGameplayInstance>())
	{
		// An external save/generated route may have committed while our travel
		// was queued. Never restore over a selection that no longer belongs to us.
		if (!Gameplay->bUseAuthoredSinglePlayWorld || Gameplay->bIsLoadingMode
			|| Gameplay->bPendingSavedWorldReplay || Gameplay->bSavedWorldHierarchyReady
			|| Gameplay->bSpawnGeneratedCivilization || !Gameplay->SaveSlotName.IsEmpty()
			|| Gameplay->NewGeneratedWorld || Gameplay->SpawnParameters || Gameplay->CurrentCivilization)
		{
			ClearPreviousSelection();
			return;
		}
		Gameplay->NewGeneratedWorld = PreviousGeneratedWorld.Get();
		Gameplay->SpawnParameters = PreviousSpawnParameters.Get();
		Gameplay->CurrentCivilization = PreviousCivilization;
		Gameplay->SaveSlotName = PreviousSaveSlot;
		Gameplay->bIsLoadingMode = bPreviousLoading;
		Gameplay->bPendingSavedWorldReplay = bPreviousSavedReplay;
		Gameplay->bSavedWorldHierarchyReady = bPreviousHierarchyReady;
		Gameplay->bUseAuthoredSinglePlayWorld = bPreviousAuthored;
		Gameplay->bSpawnGeneratedCivilization = bPreviousSpawnCivilization;
	}
	ClearPreviousSelection();
}

void UAPSAuthoredLevelLaunchSubsystem::ClearPreviousSelection()
{
	bGameplayCommitted = false;
	PreviousGeneratedWorld = nullptr;
	PreviousSpawnParameters = nullptr;
	PreviousCivilization = nullptr;
	PreviousSaveSlot.Reset();
}

void UAPSAuthoredLevelLaunchSubsystem::OnPostLoadMap(UWorld* LoadedWorld)
{
	// This delegate is global, including the null broadcast at LoadMap entry and
	// other PIE instances. Neither may complete or release this launch's preload.
	if (!bInitialized || !IsOpening() || !IsValid(LoadedWorld)
		|| LoadedWorld->GetGameInstance() != GetGameInstance()) return;
	if (UWorld::RemovePIEPrefix(LoadedWorld->GetOutermost()->GetName()) != APSAuthoredLevelLaunch::MapPackage)
	{
		// Another destination won the transition. Its BeginPlay may have already
		// consumed/committed state; relinquish ours instead of rolling it back.
		ClearPreviousSelection();
		Fail(LOCTEXT("WrongDestination", "Travel reached a different level; the Single Game request was stopped."));
		return;
	}
	Gate.CompleteOpening();
	RequestController.Reset();
	RequestWorld.Reset();
	ClearPreviousSelection();
	// The destination now owns its assets; until this point the streamable handle
	// survived controller EndPlay and LoadMap's garbage collection in the GI.
	PreloadHandle.Reset();
	UE_LOG(LogTemp, Log, TEXT("[APS.SinglePlay] Travel finished world=%s"), *GetNameSafe(LoadedWorld));
}

void UAPSAuthoredLevelLaunchSubsystem::OnTravelFailure(UWorld* World, ETravelFailure::Type FailureType, const FString& Error)
{
	if (!bInitialized || !IsOpening() || !IsValid(World)
		|| (World != RequestWorld.Get() && World->GetGameInstance() != GetGameInstance())) return;
	UE_LOG(LogTemp, Error, TEXT("[APS.SinglePlay] Engine travel failure type=%d: %s"), static_cast<int32>(FailureType), *Error);
	Fail(FText::Format(LOCTEXT("TravelFailed", "Unable to open Single Game: {0}"), FText::FromString(Error)));
}

FText UAPSAuthoredLevelLaunchSubsystem::GetStatusText() const
{
	using EPhase = FAPSAuthoredLevelLaunchGate::EPhase;
	const FText Seconds = FText::AsNumber(FMath::Max(0, FMath::FloorToInt(FPlatformTime::Seconds() - Gate.StartedAt)));
	switch (Gate.Phase)
	{
	case EPhase::Loading: return FText::Format(LOCTEXT("Loading", "LOADING AUTHORED LEVEL — {0}s"), Seconds);
	case EPhase::Compiling: return FText::Format(LOCTEXT("Compiling", "PREPARING EDITOR ASSETS — {0} REMAINING — {1}s"), FText::AsNumber(RemainingAssets), Seconds);
	case EPhase::Draining: return LOCTEXT("Draining", "FINISHING PREVIEW WORK");
	case EPhase::Opening: return LOCTEXT("Opening", "OPENING SINGLE GAME");
	case EPhase::Failed: return FailureReason;
	default: return FText::GetEmpty();
	}
}

#undef LOCTEXT_NAMESPACE
