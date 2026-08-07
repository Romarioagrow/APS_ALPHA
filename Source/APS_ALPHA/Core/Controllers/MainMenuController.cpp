#include "MainMenuController.h"
#include "APS_ALPHA/Core/Instances/MainGameplayInstance.h"
#include "APS_ALPHA/Core/Model/GeneratedWorld.h"
#include "APS_ALPHA/Core/Saves/GameSave.h"
#include "APS_ALPHA/UI/MainMenu/WorldGenerationViewModel.h"
#include "APS_ALPHA/UI/MainMenu/SAPSMainMenuRoot.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Engine/GameInstance.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Widgets/SWeakWidget.h"

void AMainMenuController::BeginPlay()
{
	Super::BeginPlay();
	UE_LOG(LogTemp, Log, TEXT("[APS.Menu] Main menu controller BeginPlay local=%s world=%s"),
		IsLocalController() ? TEXT("true") : TEXT("false"), *GetNameSafe(GetWorld()));
	SlateMenuInstallAttempts = 0;
	InstallSlateMenuTimer = GetWorldTimerManager().SetTimerForNextTick(
		this, &AMainMenuController::InstallSlateMenu);
}

void AMainMenuController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorldTimerManager().ClearTimer(InstallSlateMenuTimer);
	++MetadataRequestGeneration;
	PendingMetadataSlots.Reset();
	RemoveSlateMenu();
	if (WorldGenerationViewModel)
	{
		WorldGenerationViewModel->Shutdown();
	}
	Super::EndPlay(EndPlayReason);
}

void AMainMenuController::InstallSlateMenu()
{
	if (SlateMenuRoot.IsValid() && SlateMenuContainer.IsValid())
	{
		return;
	}

	++SlateMenuInstallAttempts;
	UGameViewportClient* ViewportClient = GetGameInstance()
		? GetGameInstance()->GetGameViewportClient() : nullptr;
	if (!ViewportClient && GEngine)
	{
		ViewportClient = GEngine->GameViewport;
	}
	if (!IsLocalController() || !ViewportClient)
	{
		UE_LOG(LogTemp, Verbose,
			TEXT("[APS.Menu] Slate install deferred attempt=%d local=%s viewport=%s"),
			SlateMenuInstallAttempts, IsLocalController() ? TEXT("true") : TEXT("false"),
			ViewportClient ? TEXT("ready") : TEXT("missing"));
		ScheduleSlateMenuInstallRetry();
		return;
	}

	// Blueprint menu assets remain available as a fallback, but the runtime path
	// is owned by code so every screen shares one data model and one visual system.
	UWidgetLayoutLibrary::RemoveAllWidgets(this);
	if (!MenuGeneratedWorld)
	{
		MenuGeneratedWorld = NewObject<UGeneratedWorld>(this);
	}
	if (!WorldGenerationViewModel)
	{
		WorldGenerationViewModel = NewObject<UWorldGenerationViewModel>(this);
		WorldGenerationViewModel->Initialize(this, MenuGeneratedWorld);
	}

	SlateMenuRoot = SNew(SAPSMainMenuRoot)
		.Controller(this)
		.ViewModel(WorldGenerationViewModel);
	SlateMenuContainer = SNew(SWeakWidget).PossiblyNullContent(SlateMenuRoot.ToSharedRef());
	ViewportClient->AddViewportWidgetContent(SlateMenuContainer.ToSharedRef(), 1000);

	bShowMouseCursor = true;
	FInputModeUIOnly InputMode;
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	InputMode.SetWidgetToFocus(SlateMenuRoot);
	SetInputMode(InputMode);
	UE_LOG(LogTemp, Log, TEXT("[APS.Menu] Slate root installed attempt=%d viewport=%s"),
		SlateMenuInstallAttempts, *GetNameSafe(ViewportClient));
}

void AMainMenuController::ScheduleSlateMenuInstallRetry()
{
	if (!GetWorld() || GetWorld()->bIsTearingDown)
	{
		return;
	}
	GetWorldTimerManager().SetTimer(
		InstallSlateMenuTimer, this, &AMainMenuController::InstallSlateMenu, 0.10f, false);
}

#if WITH_DEV_AUTOMATION_TESTS
bool AMainMenuController::OpenAstronomicalGenerationForAutomation(
	EAstroPreviewFocus Focus, EAPSGenerationRoute Route)
{
	if (!SlateMenuRoot.IsValid())
	{
		return false;
	}
	SlateMenuRoot->OpenAstronomicalGenerationForAutomation(Focus, Route);
	return true;
}

bool AMainMenuController::CommitSurfaceControlForAutomation(
	const EAPSGenerationSurfaceControl Control, const double Value)
{
	return SlateMenuRoot.IsValid()
		&& SlateMenuRoot->CommitSurfaceControlForAutomation(Control, Value);
}

double AMainMenuController::GetSurfaceControlValueForAutomation(
	const EAPSGenerationSurfaceControl Control) const
{
	return SlateMenuRoot.IsValid()
		? SlateMenuRoot->GetSurfaceControlValueForAutomation(Control)
		: TNumericLimits<double>::Lowest();
}
#endif

void AMainMenuController::RemoveSlateMenu()
{
	UGameViewportClient* ViewportClient = GetGameInstance()
		? GetGameInstance()->GetGameViewportClient() : nullptr;
	if (!ViewportClient && GEngine)
	{
		ViewportClient = GEngine->GameViewport;
	}
	if (SlateMenuContainer.IsValid() && ViewportClient)
	{
		ViewportClient->RemoveViewportWidgetContent(SlateMenuContainer.ToSharedRef());
	}
	SlateMenuContainer.Reset();
	SlateMenuRoot.Reset();
}

void AMainMenuController::LaunchSingleGame()
{
	if (UMainGameplayInstance* GameplayInstance = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UMainGameplayInstance>() : nullptr)
	{
		GameplayInstance->bIsLoadingMode = false;
		GameplayInstance->bUseAuthoredSinglePlayWorld = true;
		GameplayInstance->bSpawnGeneratedCivilization = false;
		GameplayInstance->SaveSlotName.Reset();
		GameplayInstance->NewGeneratedWorld = nullptr;
		GameplayInstance->SpawnParameters = nullptr;
		GameplayInstance->CurrentCivilization = nullptr;
	}
	// Deliberately no generation/commit work here. This route is equivalent to
	// pressing Play while L_APS_SinglePlay_StartLocation is open in the editor.
	UGameplayStatics::OpenLevel(this, TEXT("L_APS_SinglePlay_StartLocation"));
}

void AMainMenuController::LoadWorldSlot(const FString& SaveFileName)
{
	if (UMainGameplayInstance* GameplayInstance = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UMainGameplayInstance>() : nullptr)
	{
		// A visit must never accidentally reuse a model committed by an earlier
		// generation session in the same GameInstance.
		GameplayInstance->bUseAuthoredSinglePlayWorld = false;
		GameplayInstance->bSpawnGeneratedCivilization = false;
		GameplayInstance->NewGeneratedWorld = nullptr;
		GameplayInstance->SpawnParameters = nullptr;
		GameplayInstance->CurrentCivilization = nullptr;
	}
	SetSaveSlotName(SaveFileName);
	SetLoadingModeTrue();
	UGameplayStatics::OpenLevel(this, TEXT("L_WorldGeneration"));
}

void AMainMenuController::LoadWorldMetadataAsync(const TArray<FString>& SlotNames)
{
	// Opening the browser again replaces the previous request. Async save loading
	// cannot be cancelled by the engine, so generation-tagged callbacks below are
	// ignored instead of advancing a stale/replaced queue.
	++MetadataRequestGeneration;
	PendingMetadataSlots.Reset();
	for (const FString& SlotName : SlotNames)
	{
		if (!SlotName.IsEmpty())
		{
			PendingMetadataSlots.AddUnique(SlotName);
		}
	}
	PendingMetadataIndex = 0;
	LoadNextWorldMetadata();
}

void AMainMenuController::CancelWorldMetadataLoad()
{
	++MetadataRequestGeneration;
	PendingMetadataSlots.Reset();
	PendingMetadataIndex = 0;
}

void AMainMenuController::LoadNextWorldMetadata()
{
	if (PendingMetadataIndex >= PendingMetadataSlots.Num())
	{
		PendingMetadataSlots.Reset();
		return;
	}

	const FString SlotName = PendingMetadataSlots[PendingMetadataIndex++];
	const uint64 RequestGeneration = MetadataRequestGeneration;
	UGameplayStatics::AsyncLoadGameFromSlot(
		SlotName, 0,
		FAsyncLoadGameFromSlotDelegate::CreateWeakLambda(this,
			[this, RequestGeneration](const FString& LoadedSlotName, int32 UserIndex, USaveGame* LoadedGame)
			{
				OnWorldMetadataLoaded(RequestGeneration, LoadedSlotName, UserIndex, LoadedGame);
			}));
}

void AMainMenuController::OnWorldMetadataLoaded(uint64 RequestGeneration, const FString& SlotName,
	int32 UserIndex, USaveGame* LoadedGame)
{
	if (RequestGeneration != MetadataRequestGeneration)
	{
		return;
	}
	if (SlateMenuRoot.IsValid())
	{
		SlateMenuRoot->ApplyExistingWorldMetadata(SlotName, Cast<UGameSave>(LoadedGame));
	}
	LoadNextWorldMetadata();
}

void AMainMenuController::HoldSlateResource(UObject* Resource)
{
	if (Resource)
	{
		SlateResources.AddUnique(Resource);
	}
}

void AMainMenuController::SetSaveSlotName(FString OutSaveSlotName) 
{
	if (UWorld* World = GetWorld())
	{
		if (UGameInstance* GameInstance = World->GetGameInstance())
		{
			if (UMainGameplayInstance* MainGameplayInstance = GameInstance->GetSubsystem<UMainGameplayInstance>())
			{
				MainGameplayInstance->SaveSlotName = OutSaveSlotName;
				UE_LOG(LogTemp, Log, TEXT("SaveSlotName set to: %s"), *MainGameplayInstance->SaveSlotName);
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("MainGameplayInstance is null"));
			}
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("GameInstance is null"));
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("World is null"));
	}
}

void AMainMenuController::SetLoadingModeTrue()
{
	if (const UWorld* World = GetWorld())
	{
		if (UGameInstance* GameInstance = World->GetGameInstance())
		{
			if (UMainGameplayInstance* MainGameplayInstance = GameInstance->GetSubsystem<UMainGameplayInstance>())
			{
				MainGameplayInstance->bIsLoadingMode = true;
			}
		}
	}
}
