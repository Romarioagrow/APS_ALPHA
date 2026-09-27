#include "MainMenuController.h"
#include "APS_ALPHA/Core/Instances/MainGameplayInstance.h"
#include "APS_ALPHA/Core/Loading/APSAuthoredLevelLaunchSubsystem.h"
#include "APS_ALPHA/Core/Model/GeneratedWorld.h"
#include "APS_ALPHA/Core/Model/SpawnParameters.h"
#include "APS_ALPHA/Core/Saves/APSWorldSaveSnapshot.h"
#include "APS_ALPHA/Core/Saves/GameSave.h"
#include "APS_ALPHA/Core/Saves/SavedActorData.h"
#include "APS_ALPHA/Actors/Tech/SpaceHeadquarters.h"
#include "APS_ALPHA/Actors/Tech/SpaceShipyard.h"
#include "APS_ALPHA/Actors/Tech/SpaceStation.h"
#include "APS_ALPHA/Gameplay/Civilizations/Civilization.h"
#include "APS_ALPHA/Generation/AstroGenerator.h"
#include "APS_ALPHA/Pawns/Spaceships/Spaceship.h"
#include "APS_ALPHA/UI/MainMenu/WorldGenerationViewModel.h"
#include "APS_ALPHA/UI/MainMenu/SAPSMainMenuRoot.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Engine/GameInstance.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/Paths.h"
#include "Widgets/SWeakWidget.h"

namespace
{
	bool IsLegacyCivilizationSave(const UGameSave& Save)
	{
		if (Save.bHasCivilizationManifest)
		{
			return true;
		}
		for (const FActorSaveData& ActorData : Save.ActorSaveDataArray)
		{
			if (ActorData.StableEntityId.IsValid())
			{
				return true;
			}
			if (const UClass* ActorClass = LoadClass<AActor>(nullptr, *ActorData.ActorClass);
				ActorClass && ActorClass->IsChildOf(ASpaceship::StaticClass()))
			{
				return true;
			}
		}
		return false;
	}

	USpawnParameters* BuildLegacySpawnParameters(const UGameSave& Save, UObject* Outer)
	{
		USpawnParameters* Parameters = NewObject<USpawnParameters>(Outer);
		if (!Parameters)
		{
			return nullptr;
		}
		UClass* GeneratorClass = LoadClass<AAstroGenerator>(nullptr,
			TEXT("/Game/APS/APS_ALPHA/Core/BP_AstroGenerator.BP_AstroGenerator_C"));
		const AAstroGenerator* GeneratorDefaults = GeneratorClass
			? Cast<AAstroGenerator>(GeneratorClass->GetDefaultObject()) : nullptr;
		if (GeneratorDefaults)
		{
			Parameters->BP_CharacterClass = GeneratorDefaults->BP_CharacterClass;
			Parameters->BP_HomeSpaceStation = GeneratorDefaults->BP_HomeSpaceStation;
			Parameters->BP_HomeSpaceship = GeneratorDefaults->BP_HomeSpaceship;
			Parameters->BP_HomeSpaceShipyard = GeneratorDefaults->BP_HomeSpaceShipyard;
			Parameters->BP_HomeSpaceHeadquarters = GeneratorDefaults->BP_HomeSpaceHeadquarters;
		}
		// The menu's certified civilization route has always used this production
		// gravity pawn. Legacy files without a player record must recover the same
		// class instead of inheriting an experimental generator-CDO default.
		if (UClass* ProductionPilot = LoadClass<APawn>(nullptr,
			TEXT("/Game/APS/APS_ALPHA/Blueprints/BP_CustomGravityCharacter.BP_CustomGravityCharacter_C")))
		{
			Parameters->BP_CharacterClass = ProductionPilot;
		}

		if (!Save.PlayerPawnClass.IsEmpty())
		{
			if (UClass* PawnClass = LoadClass<APawn>(nullptr, *Save.PlayerPawnClass))
			{
				Parameters->BP_CharacterClass = PawnClass;
			}
		}
		if (Save.bHasCivilizationManifest)
		{
			if (UClass* ShipClass =
				Save.CivilizationManifest.SelectedShipClass.TryLoadClass<ASpaceship>())
			{
				Parameters->BP_HomeSpaceship = ShipClass;
			}
		}
		for (const FActorSaveData& ActorData : Save.ActorSaveDataArray)
		{
			if (UClass* ActorClass = LoadClass<AActor>(nullptr, *ActorData.ActorClass);
				ActorClass && ActorClass->IsChildOf(ASpaceship::StaticClass()))
			{
				Parameters->BP_HomeSpaceship = ActorClass;
				break;
			}
		}
		// If a malformed legacy slot lost the pawn snapshot as well as its spawn
		// recipe, recover onto the home planet instead of silently falling back to
		// the SpawnParameters orbit default.  Slots that still contain a pawn
		// snapshot are positioned exactly by the post-generation restore overlay.
		if (!Save.bHasPlayerPawnState)
		{
			Parameters->CharacterSpawnPlace = ECharSpawnPlace::PlanetSurface;
		}
		Parameters->SanitizeForGeneration();
		const bool bComplete = Parameters->BP_CharacterClass
			&& Parameters->BP_HomeSpaceStation && Parameters->BP_HomeSpaceship
			&& Parameters->BP_HomeSpaceShipyard && Parameters->BP_HomeSpaceHeadquarters;
		return bComplete ? Parameters : nullptr;
	}
}

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
	if (UAPSAuthoredLevelLaunchSubsystem* Launch = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UAPSAuthoredLevelLaunchSubsystem>() : nullptr)
	{
		// Cold authored assets must finish before entering a game world: LoadMap's
		// FinishCompilationsForGame otherwise blocks the game thread for minutes.
		// The gate keeps this menu alive and commits gameplay state only on success.
		Launch->RequestLaunch(this);
		return;
	}
	UE_LOG(LogTemp, Error, TEXT("[APS.SinglePlay] Launch subsystem unavailable; staying in menu"));
}

void AMainMenuController::LoadWorldSlot(const FString& SaveFileName)
{
	UMainGameplayInstance* GameplayInstance = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UMainGameplayInstance>() : nullptr;
	if (!GameplayInstance || SaveFileName.IsEmpty())
	{
		UE_LOG(LogTemp, Error, TEXT("[APS.Save] Cannot start load: invalid game state or slot"));
		return;
	}

	const FString SlotName = FPaths::GetBaseFilename(SaveFileName);
	UGameSave* LoadedSave = Cast<UGameSave>(UGameplayStatics::LoadGameFromSlot(SlotName, 0));
	if (!LoadedSave)
	{
		UE_LOG(LogTemp, Error, TEXT("[APS.Save] Cannot open selected save slot: %s"), *SlotName);
		return;
	}

	UGeneratedWorld* ReplayModel = APSWorldSaveSnapshot::Restore(
		LoadedSave, GameplayInstance, SlotName);
	if (!ReplayModel)
	{
		UE_LOG(LogTemp, Error,
			TEXT("[APS.Save] Slot %s has no recoverable generated-world model"), *SlotName);
		return;
	}

	USpawnParameters* ReplaySpawnParameters =
		APSWorldSaveSnapshot::RestoreSpawnParameters(LoadedSave, GameplayInstance);
	bool bReplayCivilization = LoadedSave->bHadGeneratedCivilization;
	// A short-lived pre-release v3 build stamped the new version number before
	// UnrealHeaderTool had emitted the two new properties. Those files look like
	// v3 but contain no spawn recipe. Detect migration need from the actual
	// civilization evidence, not only from the version integer.
	if (!ReplaySpawnParameters && IsLegacyCivilizationSave(*LoadedSave))
	{
		bReplayCivilization = true;
		ReplaySpawnParameters = BuildLegacySpawnParameters(*LoadedSave, GameplayInstance);
		UE_LOG(LogTemp, Log,
			TEXT("[APS.Save] Recovered missing civilization recipe for slot=%s stampedVersion=%d"),
			*SlotName, LoadedSave->SaveFormatVersion);
	}
	if (bReplayCivilization && !ReplaySpawnParameters)
	{
		UE_LOG(LogTemp, Error,
			TEXT("[APS.Save] Slot %s requires civilization replay but its spawn recipe is unavailable"),
			*SlotName);
		return;
	}
	UCivilization* ReplayCivilization = nullptr;
	if (bReplayCivilization)
	{
		ReplayCivilization = NewObject<UCivilization>(GameplayInstance);
		if (!ReplayCivilization)
		{
			UE_LOG(LogTemp, Error,
				TEXT("[APS.Save] Cannot recreate civilization for slot %s"), *SlotName);
			return;
		}
		ReplayCivilization->InitializeFromSpawnParameters(ReplaySpawnParameters);
	}

	// Rebuild astronomy first from the immutable model.  Interactive actor and
	// player transforms are restored only after the hierarchy reports ready.
	GameplayInstance->bUseAuthoredSinglePlayWorld = false;
	GameplayInstance->bSpawnGeneratedCivilization = bReplayCivilization;
	GameplayInstance->NewGeneratedWorld = ReplayModel;
	GameplayInstance->SpawnParameters = ReplaySpawnParameters;
	GameplayInstance->CurrentCivilization = ReplayCivilization;
	GameplayInstance->SaveSlotName = SlotName;
	GameplayInstance->bIsLoadingMode = true;
	GameplayInstance->bPendingSavedWorldReplay = true;
	GameplayInstance->bSavedWorldHierarchyReady = false;
	UE_LOG(LogTemp, Log,
		TEXT("[APS.Save] Prepared deterministic replay slot=%s version=%d modelBytes=%d seed=%d canonicalRecords=%d civilization=%s spawnBytes=%d"),
		*SlotName, LoadedSave->SaveFormatVersion, LoadedSave->GeneratedWorldModelData.Num(),
		ReplayModel->GenerationSeed, ReplayModel->CanonicalStellarDataset.ClusterRecords.Num(),
		bReplayCivilization ? TEXT("yes") : TEXT("no"), LoadedSave->SpawnParametersData.Num());
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
