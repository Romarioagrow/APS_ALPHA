#include "GravityPlayerController.h"
#include "APS_ALPHA/Generation/APSBodyNames.h"
#include <ctime> 
#include <random>
#include "APS_ALPHA/Gameplay/Civilizations/APSCivilizationSave.h"
#include "APS_ALPHA/Actors/Astro/WorldActor.h"
#include "APS_ALPHA/Core/Instances/MainGameplayInstance.h"
#include "APS_ALPHA/Core/Model/SpawnParameters.h"
#include "APS_ALPHA/Core/Saves/APSWorldSaveSnapshot.h"
#include "APS_ALPHA/Core/Saves/GameSave.h"
#include "APS_ALPHA/Core/Saves/SavedActorData.h"
#include "APS_ALPHA/Gameplay/Civilizations/APSCivilizationIdentityComponent.h"
#include "APS_ALPHA/Gameplay/Civilizations/APSCivilizationMaterializationSubsystem.h"
#include "APS_ALPHA/Gameplay/Civilizations/APSCivilizationRuntimeManifest.h"
#include "APS_ALPHA/Generation/AstroGenerator.h"
#include "APS_ALPHA/Core/World/APSWorldOriginSubsystem.h"
#include "APS_ALPHA/Pawns/Vehicles/PilotingVehicle.h"
#include "Kismet/GameplayStatics.h"
#include "APS_ALPHA/Core/Structs/PlanetarySystemGenerationModel.h"
#include "Serialization/ObjectAndNameAsStringProxyArchive.h"
#include "APS_ALPHA/UI/StrategicMap/SAPSStrategicMapPanel.h"
#include "APS_ALPHA/UI/MainMenu/APSWorldBrowserMetadata.h"
#include "Engine/GameViewportClient.h"
#include "GameFramework/Pawn.h"
#include "InputCoreTypes.h"
#include "Misc/App.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/Paths.h"
#include "Widgets/SWeakWidget.h"

namespace
{
	/** The world browser's .apsmeta sidecar. Rio 03.10: it used to copy the menu model's editor buffer (a G star and one
	 * frozen planet in almost every save); it now records the live home system. Format and reader live together in
	 * APSWorldBrowserMetadata; the old keys stay, so older builds still read it. */
	void WriteWorldMetadataSidecar(const UGameSave* Save, const FGeneratedWorldData& WorldData, const UWorld* World,
		const UGeneratedWorld* GeneratedWorldModel)
	{
		APSWorldBrowserMetadata::WriteForSave(Save, WorldData, World, GeneratedWorldModel);
	}
}

void AGravityPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();
	if (InputComponent)
	{
		InputComponent->BindKey(EKeys::F10, IE_Pressed, this, &AGravityPlayerController::ToggleStrategicMap);
	}
}

namespace APSSaveFrame
{
	/** Saved transforms are in the generation frame; the live world may have its origin on the player. */
	FTransform ToWorld(const UWorld* World, FTransform Transform)
	{
		if (const UAPSWorldOriginSubsystem* WorldOrigin = World ? World->GetSubsystem<UAPSWorldOriginSubsystem>() : nullptr)
		{
			Transform.SetLocation(WorldOrigin->FromGenerationFrame(Transform.GetLocation()));
		}
		return Transform;
	}
}

void AGravityPlayerController::PlayerTick(const float DeltaTime)
{
	Super::PlayerTick(DeltaTime);
	CapturePlayerStateForSave();
}

void AGravityPlayerController::CapturePlayerStateForSave()
{
	const APawn* PlayerPawn = GetPawn();
	// Seated in a ship the player is still its pilot: the save keeps the pilot, who sits down in the ship again after
	// loading (APSCivilizationSave). Saving the ship as the player's pawn spawned a second, empty ship on load and
	// removed the pilot, who then could not get out (audit B2).
	if (const APilotingVehicle* Vehicle = Cast<APilotingVehicle>(PlayerPawn); Vehicle && IsValid(Vehicle->Pilot))
	{
		PlayerPawn = Vehicle->Pilot;
	}
	if (!IsValid(PlayerPawn))
	{
		return;
	}
	CachedPlayerPawnClass = PlayerPawn->GetClass()->GetPathName();
	CachedPlayerPawnTransform = PlayerPawn->GetActorTransform();
	// Saves stay in the generation frame (headquarters at 0,0,0) after the world origin moved to the player.
	if (const UAPSWorldOriginSubsystem* WorldOrigin = GetWorld() ? GetWorld()->GetSubsystem<UAPSWorldOriginSubsystem>() : nullptr)
	{
		// Rio 06.10 (still ship): riding a ship that owes its travel, the player is truly that much further on.
		CachedPlayerPawnTransform.SetLocation(WorldOrigin->ToGenerationFrame(CachedPlayerPawnTransform.GetLocation())
			- WorldOrigin->GetSkyOffset());
	}
	CachedPlayerControlRotation = GetControlRotation();
	bHasCachedPlayerState = true;
}

void AGravityPlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// PIE/editor owns F5, and players should never have to know a debug hotkey.
	// Persist the latest pawn/camera state while the generated hierarchy is still
	// alive. The initial slot is already created synchronously by SaveNewWorld.
	UMainGameplayInstance* GameplayState = GetWorld() && GetWorld()->GetGameInstance()
		? GetWorld()->GetGameInstance()->GetSubsystem<UMainGameplayInstance>() : nullptr;
	if (!GIsAutomationTesting && !FApp::IsUnattended()
		&& GameplayState && !GameplayState->bPendingSavedWorldReplay
		&& IsValid(GameplayState->NewGeneratedWorld))
	{
		UE_LOG(LogTemp, Log, TEXT("[APS.Save] Lifecycle autosave reason=%d"),
			static_cast<int32>(EndPlayReason));
		SaveCurrentWorld();
	}
	CloseStrategicMap(false);
	Super::EndPlay(EndPlayReason);
}

void AGravityPlayerController::ToggleStrategicMap()
{
	if (StrategicMapWidget.IsValid())
	{
		CloseStrategicMap(true);
		return;
	}
	if (!GEngine || !GEngine->GameViewport || !GetWorld()) return;

	// The live generator (never a menu preview one): the map reads the home star, planet and system from it. The map
	// works without one too; it never calls the generator's preview camera or presentation (Rio 02.10: "HOME SYSTEM
	// shows nothing, the camera falls into the star; the planet is see-through; FPS drops when rotating").
	AAstroGenerator* Generator = nullptr;
	TArray<AActor*> Generators;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AAstroGenerator::StaticClass(), Generators);
	for (AActor* Candidate : Generators)
	{
		AAstroGenerator* Live = Cast<AAstroGenerator>(Candidate);
		if (IsValid(Live) && !Live->ActorHasTag(TEXT("WorldGenerationPreview")) && !Live->UsesContinuousPreviewFrame())
		{
			Generator = Live;
			break;
		}
	}

	StrategicMapPreviousViewTarget = GetViewTarget();
	// The panel spawns the map's own camera at the current view and takes the view target (SAPSStrategicMapPanel).
	StrategicMapWidget = SNew(SAPSStrategicMapPanel)
		.Controller(this)
		.Generator(Generator)
		.OnClose(FSimpleDelegate::CreateUObject(this, &AGravityPlayerController::ToggleStrategicMap));
	StrategicMapContainer = SNew(SWeakWidget).PossiblyNullContent(StrategicMapWidget.ToSharedRef());
	GEngine->GameViewport->AddViewportWidgetContent(StrategicMapContainer.ToSharedRef(), 900);

	// UI only: the pawn and the ship take no keys while the map is open (typing a star's name must not fly the ship);
	// the panel itself closes on F10 and Esc.
	bShowMouseCursor = true;
	FInputModeUIOnly InputMode;
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	InputMode.SetWidgetToFocus(StrategicMapWidget);
	SetInputMode(InputMode);
}

void AGravityPlayerController::CloseStrategicMap(bool bRestoreView)
{
	if (StrategicMapContainer.IsValid() && GEngine && GEngine->GameViewport)
	{
		GEngine->GameViewport->RemoveViewportWidgetContent(StrategicMapContainer.ToSharedRef());
	}
	StrategicMapContainer.Reset();
	// The panel releases the map camera here; it stays a moment for the blend back below, then goes by itself.
	StrategicMapWidget.Reset();

	if (bRestoreView)
	{
		AActor* RestoreTarget = StrategicMapPreviousViewTarget.Get();
		if (!IsValid(RestoreTarget)) RestoreTarget = GetPawn();
		if (IsValid(RestoreTarget)) SetViewTargetWithBlend(RestoreTarget, 0.30f, VTBlend_Cubic);
		bShowMouseCursor = false;
		SetInputMode(FInputModeGameOnly());
	}
	StrategicMapPreviousViewTarget.Reset();
}

FString AGravityPlayerController::GetCurrentSaveSlotName() const
{
	if (const UGameInstance* GameInstance = GetWorld()->GetGameInstance())
	{
		if (const UMainGameplayInstance* MainGameplayInstance = GameInstance->GetSubsystem<UMainGameplayInstance>())
		{
			return FPaths::GetBaseFilename(MainGameplayInstance->SaveSlotName);
		}
	}
	return FString();
}

void AGravityPlayerController::SaveNewWorld(const EAstroGenerationLevel AstroGenerationLevel,
                                            UGeneratedWorld* GeneratedWorldModel)
{
	if (const UGameInstance* GameInstance = GetWorld() ? GetWorld()->GetGameInstance() : nullptr)
	{
		if (const UMainGameplayInstance* GameplayState =
			GameInstance->GetSubsystem<UMainGameplayInstance>();
			GameplayState && GameplayState->bPendingSavedWorldReplay)
		{
			UE_LOG(LogTemp, Verbose,
				TEXT("[APS.Save] Suppressed new-slot autosave while replaying %s"),
				*GameplayState->SaveSlotName);
			return;
		}
	}

	const FString SaveSlotName = GenerateUniqueSaveSlotName(AstroGenerationLevel);
	SaveWorldToSlot(SaveSlotName, GeneratedWorldModel);
}

void AGravityPlayerController::SaveCurrentWorld()
{
	UMainGameplayInstance* GameplayState = GetWorld() && GetWorld()->GetGameInstance()
		? GetWorld()->GetGameInstance()->GetSubsystem<UMainGameplayInstance>() : nullptr;
	if (!GameplayState || GameplayState->bPendingSavedWorldReplay
		|| !IsValid(GameplayState->NewGeneratedWorld))
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[APS.Save] Quick-save ignored: no fully loaded generated world"));
		return;
	}

	FString SlotName = GetCurrentSaveSlotName();
	if (SlotName.IsEmpty())
	{
		SlotName = CurrentSaveSlotName;
	}
	if (SlotName.IsEmpty())
	{
		SlotName = GenerateUniqueSaveSlotName(
			GameplayState->NewGeneratedWorld->AstroGenerationLevel);
	}

	// Do not synchronously read the previous (potentially very large) save just to
	// recover its display name. SaveWorldToSlot derives the same stable name from
	// the slot when ExistingWorldName is empty. This removes the long read-before-
	// write hitch that previously made the obsolete F5 quick-save appear frozen.
	SaveWorldToSlot(SlotName, GameplayState->NewGeneratedWorld);
}

bool AGravityPlayerController::SaveWorldToSlot(const FString& SlotName,
	UGeneratedWorld* GeneratedWorldModel, const FString& ExistingWorldName)
{
	if (SlotName.IsEmpty() || !IsValid(GeneratedWorldModel) || !GetWorld())
	{
		return false;
	}

	// Rio 06.10 (still ship): a ship owing its travel pays it first, so every saved place is in one frame.
	if (UAPSWorldOriginSubsystem* WorldOrigin = GetWorld()->GetSubsystem<UAPSWorldOriginSubsystem>())
	{
		WorldOrigin->SettleDeferredTravel(TEXT("a save"));
	}

	UGameSave* SaveGameInstance = Cast<UGameSave>(
		UGameplayStatics::CreateSaveGameObject(UGameSave::StaticClass()));
	if (!SaveGameInstance)
	{
		return false;
	}

	FString WorldName = ExistingWorldName;
	if (WorldName.IsEmpty())
	{
		if (int32 LastSpaceIndex; SlotName.FindLastChar(TEXT(' '), LastSpaceIndex))
		{
			WorldName = SlotName.Left(LastSpaceIndex);
		}
		else
		{
			WorldName = SlotName;
		}
	}

	SaveGameInstance->SaveFormatVersion = APSWorldSaveSnapshot::LatestSaveFormatVersion;
	SaveGameInstance->SaveSlotName = SlotName;
	SaveGameInstance->UserIndex = 0;
	SaveGameInstance->WorldName = WorldName;

	const FGeneratedWorldData WorldSaveData = GeneratedWorldModel->SaveWorldData();
	SaveGameInstance->GeneratedWorldsDataArray.Add(WorldSaveData);
	if (!APSWorldSaveSnapshot::Capture(
		GeneratedWorldModel, SaveGameInstance->GeneratedWorldModelData))
	{
		UE_LOG(LogTemp, Error,
			TEXT("[APS.Save] Refusing incomplete save: generated model snapshot failed"));
		return false;
	}

	SaveGameInstance->InhabitedPlanetsDataArray = GeneratedWorldModel->GetInhabitedPlanets();

	UWorld* World = GetWorld();
	UMainGameplayInstance* GameplayState = World->GetGameInstance()
		? World->GetGameInstance()->GetSubsystem<UMainGameplayInstance>() : nullptr;
	SaveGameInstance->bHadGeneratedCivilization = GameplayState
		&& GameplayState->bSpawnGeneratedCivilization;
	if (SaveGameInstance->bHadGeneratedCivilization
		&& (!IsValid(GameplayState->SpawnParameters)
			|| !APSWorldSaveSnapshot::CaptureSpawnParameters(
				GameplayState->SpawnParameters, SaveGameInstance->SpawnParametersData)))
	{
		UE_LOG(LogTemp, Error,
			TEXT("[APS.Save] Refusing incomplete civilization save: spawn recipe failed"));
		return false;
	}

	if (const UAPSCivilizationMaterializationSubsystem* CivilizationSubsystem =
		World->GetSubsystem<UAPSCivilizationMaterializationSubsystem>())
	{
		const FAPSCivilizationRuntimeManifest& Manifest =
			CivilizationSubsystem->GetRuntimeManifest();
		FString ManifestValidationReason;
		if (Manifest.IsStructurallyValid(&ManifestValidationReason))
		{
			SaveGameInstance->CivilizationManifest = Manifest;
			SaveGameInstance->bHasCivilizationManifest = true;
		}
	}
	// Modules, fleet, surveys, outposts and the journal: the progress the actor archive below does not hold.
	APSCivilizationSave::Capture(World, SaveGameInstance->CivilizationState);

	TArray<AActor*> AllActors;
	UGameplayStatics::GetAllActorsOfClass(World, ABaseActor::StaticClass(), AllActors);
	for (AActor* Actor : AllActors)
	{
		// Astronomy is reconstructed from the canonical snapshot. Serializing those
		// actors duplicated large transient graphs and still lost their TSharedPtr data.
		if (!IsValid(Actor) || Actor->IsA<AWorldActor>())
		{
			continue;
		}

		FActorSaveData SaveData;
		SaveData.ActorTransform = Actor->GetActorTransform();
		if (const UAPSWorldOriginSubsystem* WorldOrigin = World->GetSubsystem<UAPSWorldOriginSubsystem>())
		{
			SaveData.ActorTransform.SetLocation(WorldOrigin->ToGenerationFrame(SaveData.ActorTransform.GetLocation()));
		}
		SaveData.ActorName = Actor->GetName();
		SaveData.ActorClass = Actor->GetClass()->GetPathName();
		if (const UAPSCivilizationIdentityComponent* Identity =
			Actor->FindComponentByClass<UAPSCivilizationIdentityComponent>())
		{
			SaveData.StableEntityId = Identity->StableEntityId;
		}
		FMemoryWriter MemoryWriter(SaveData.ActorData, true);
		FObjectAndNameAsStringProxyArchive Archive(MemoryWriter, true);
		Actor->Serialize(Archive);
		if (AActor* ParentActor = Actor->GetAttachParentActor())
		{
			SaveData.ParentActorName = ParentActor->GetName();
		}
		SaveGameInstance->ActorSaveDataArray.Add(MoveTemp(SaveData));
	}

	// EndPlay may run after the pawn has already been torn down. Cache the latest
	// valid frame continuously so a lifecycle autosave can never replace a valid
	// player record with an empty one during level travel or application exit.
	CapturePlayerStateForSave();
	if (bHasCachedPlayerState)
	{
		SaveGameInstance->bHasPlayerPawnState = true;
		SaveGameInstance->PlayerPawnClass = CachedPlayerPawnClass;
		SaveGameInstance->PlayerPawnTransform = CachedPlayerPawnTransform;
		SaveGameInstance->PlayerControlRotation = CachedPlayerControlRotation;
	}

	if (!UGameplayStatics::SaveGameToSlot(SaveGameInstance, SlotName,
		SaveGameInstance->UserIndex))
	{
		UE_LOG(LogTemp, Error, TEXT("[APS.Save] Failed to write slot: %s"), *SlotName);
		return false;
	}

	CurrentSaveSlotName = SlotName;
	if (GameplayState)
	{
		GameplayState->SaveSlotName = SlotName;
	}
	WriteWorldMetadataSidecar(SaveGameInstance, WorldSaveData, World, GeneratedWorldModel);
	UE_LOG(LogTemp, Log,
		TEXT("[APS.Save] Saved slot=%s modelBytes=%d spawnBytes=%d civilization=%s actors=%d player=%s"),
		*SlotName, SaveGameInstance->GeneratedWorldModelData.Num(),
		SaveGameInstance->SpawnParametersData.Num(),
		SaveGameInstance->bHadGeneratedCivilization ? TEXT("yes") : TEXT("no"),
		SaveGameInstance->ActorSaveDataArray.Num(),
		SaveGameInstance->bHasPlayerPawnState ? TEXT("yes") : TEXT("no"));
	return true;
}

void AGravityPlayerController::LoadWorld()
{
	if (UWorld* World = GetWorld())
	{
		// Rio 06.10 (still ship): saved places are restored into a world that owes nothing.
		if (UAPSWorldOriginSubsystem* WorldOrigin = World->GetSubsystem<UAPSWorldOriginSubsystem>())
		{
			WorldOrigin->SettleDeferredTravel(TEXT("a load"));
		}
		UMainGameplayInstance* GameplayState = World->GetGameInstance()
			? World->GetGameInstance()->GetSubsystem<UMainGameplayInstance>() : nullptr;
		if (GameplayState && GameplayState->bPendingSavedWorldReplay
			&& !GameplayState->bSavedWorldHierarchyReady)
		{
			// L_WorldGeneration contains a legacy BeginPlay call.  Loading its actor
			// archive before deterministic generation creates a partial/duplicate world.
			UE_LOG(LogTemp, Verbose,
				TEXT("[APS.Save] Actor overlay deferred until generated hierarchy is ready"));
			return;
		}

		FString LoadingName = GetCurrentSaveSlotName();
		if (UGameSave* LoadedGame = Cast<UGameSave>(UGameplayStatics::LoadGameFromSlot(LoadingName, 0)))
		{
			TMap<FString, AActor*> NameToActorMap;
			TMap<FGuid, AActor*> StableIdToActorMap;
			TSet<FString> RestoredActorNames;

			FAPSCivilizationRuntimeManifest LoadedManifest;
			bool bHasValidManifest = false;
			if (LoadedGame->bHasCivilizationManifest)
			{
				LoadedManifest = LoadedGame->CivilizationManifest;
				FString MigrationReason;
				bHasValidManifest = LoadedManifest.MigrateToLatest(&MigrationReason);
				if (bHasValidManifest)
				{
					if (UAPSCivilizationMaterializationSubsystem* CivilizationSubsystem =
						World->GetSubsystem<UAPSCivilizationMaterializationSubsystem>())
					{
						CivilizationSubsystem->RestoreRuntimeManifest(LoadedManifest);
					}
				}
				else
				{
					UE_LOG(LogTemp, Warning,
						TEXT("[APS.Save] Civilization manifest rejected during load: %s"),
						*MigrationReason);
				}
			}

			TArray<AActor*> ExistingActors;
			UGameplayStatics::GetAllActorsOfClass(World, ABaseActor::StaticClass(), ExistingActors);
			for (AActor* ExistingActor : ExistingActors)
			{
				if (!IsValid(ExistingActor))
				{
					continue;
				}
				NameToActorMap.FindOrAdd(ExistingActor->GetName()) = ExistingActor;
				if (const UAPSCivilizationIdentityComponent* Identity =
					ExistingActor->FindComponentByClass<UAPSCivilizationIdentityComponent>();
					Identity && Identity->StableEntityId.IsValid())
				{
					StableIdToActorMap.FindOrAdd(Identity->StableEntityId) = ExistingActor;
				}
			}

			for (const FActorSaveData& SaveData : LoadedGame->ActorSaveDataArray)
			{
				if (UClass* ActorClass = LoadClass<AActor>(nullptr, *SaveData.ActorClass))
				{
					// Legacy archives contain astronomy actors whose non-reflected model
					// graphs cannot be restored.  The canonical model already rebuilt them.
					if (ActorClass->IsChildOf(AWorldActor::StaticClass()))
					{
						continue;
					}
					AActor* Actor = SaveData.StableEntityId.IsValid()
						? StableIdToActorMap.FindRef(SaveData.StableEntityId) : nullptr;
					if (!IsValid(Actor))
					{
						Actor = NameToActorMap.FindRef(SaveData.ActorName);
					}
					if (IsValid(Actor) && !Actor->IsA(ActorClass))
					{
						UE_LOG(LogTemp, Warning,
							TEXT("[APS.Save] Refusing incompatible upsert name=%s existing=%s saved=%s"),
							*SaveData.ActorName, *Actor->GetClass()->GetPathName(),
							*ActorClass->GetPathName());
						continue;
					}
					if (!IsValid(Actor))
					{
						FActorSpawnParameters SpawnParams;
						SpawnParams.Name = FName(*SaveData.ActorName);
						SpawnParams.SpawnCollisionHandlingOverride =
							ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
						Actor = World->SpawnActor<AActor>(ActorClass,
							APSSaveFrame::ToWorld(World, SaveData.ActorTransform), SpawnParams);
					}

					if (Actor)
					{
						FMemoryReader MemoryReader(SaveData.ActorData, true);
						FObjectAndNameAsStringProxyArchive Archive(MemoryReader, true);
						Actor->Serialize(Archive);
						Actor->SetActorTransform(APSSaveFrame::ToWorld(World, SaveData.ActorTransform), false, nullptr,
							ETeleportType::TeleportPhysics);

						if (SaveData.StableEntityId.IsValid() && bHasValidManifest)
						{
							const FAPSCivilizationManifestEntity* ManifestEntity =
								LoadedManifest.Entities.FindByPredicate(
									[&SaveData](const FAPSCivilizationManifestEntity& Entity)
									{
										return Entity.StableId == SaveData.StableEntityId;
									});
							if (ManifestEntity)
							{
								UAPSCivilizationIdentityComponent* Identity =
									Actor->FindComponentByClass<UAPSCivilizationIdentityComponent>();
								if (!Identity)
								{
									Identity = NewObject<UAPSCivilizationIdentityComponent>(Actor,
										TEXT("CivilizationIdentity"));
									Actor->AddInstanceComponent(Identity);
									Identity->RegisterComponent();
								}
								Identity->InitializeFromManifest(LoadedManifest, *ManifestEntity);
							}
						}

						NameToActorMap.Add(SaveData.ActorName, Actor);
						RestoredActorNames.Add(SaveData.ActorName);
						if (SaveData.StableEntityId.IsValid())
						{
							StableIdToActorMap.Add(SaveData.StableEntityId, Actor);
						}
					}
				}
			}

			for (const FActorSaveData& SaveData : LoadedGame->ActorSaveDataArray)
			{
				if (RestoredActorNames.Contains(SaveData.ActorName)
					&& !SaveData.ParentActorName.IsEmpty())
				{
					AActor** ParentActor = NameToActorMap.Find(SaveData.ParentActorName);
					AActor** ChildActor = NameToActorMap.Find(SaveData.ActorName);

					if (ParentActor && ChildActor)
					{
						(*ChildActor)->AttachToActor(*ParentActor, FAttachmentTransformRules::KeepWorldTransform);
					}
				}
			}

			APSCivilizationSave::Restore(World, LoadedGame->CivilizationState);

			if (LoadedGame->bHasPlayerPawnState && !LoadedGame->PlayerPawnClass.IsEmpty())
			{
				UClass* SavedPawnClass = LoadClass<APawn>(nullptr, *LoadedGame->PlayerPawnClass);
				APawn* PlayerPawn = GetPawn();
				if (SavedPawnClass && (!IsValid(PlayerPawn) || !PlayerPawn->IsA(SavedPawnClass)))
				{
					APawn* PreviousPawn = PlayerPawn;
					FActorSpawnParameters SpawnParams;
					SpawnParams.SpawnCollisionHandlingOverride =
						ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
					PlayerPawn = World->SpawnActor<APawn>(SavedPawnClass,
						APSSaveFrame::ToWorld(World, LoadedGame->PlayerPawnTransform), SpawnParams);
					if (IsValid(PlayerPawn))
					{
						Possess(PlayerPawn);
						if (IsValid(PreviousPawn) && PreviousPawn != PlayerPawn)
						{
							PreviousPawn->Destroy();
						}
					}
				}
				if (IsValid(PlayerPawn))
				{
					PlayerPawn->SetActorTransform(APSSaveFrame::ToWorld(World, LoadedGame->PlayerPawnTransform),
						false, nullptr, ETeleportType::TeleportPhysics);
					SetControlRotation(LoadedGame->PlayerControlRotation);
					SetViewTarget(PlayerPawn);
					if (UAPSWorldOriginSubsystem* WorldOrigin = World->GetSubsystem<UAPSWorldOriginSubsystem>())
					{
						WorldOrigin->RebaseOnto(PlayerPawn->GetActorLocation(), TEXT("load"));
					}
					CapturePlayerStateForSave();
				}
			}

			CurrentSaveSlotName = LoadingName;
			if (GameplayState)
			{
				GameplayState->bIsLoadingMode = false;
				GameplayState->bPendingSavedWorldReplay = false;
				GameplayState->bSavedWorldHierarchyReady = false;
			}

			UE_LOG(LogTemp, Log,
				TEXT("[APS.Save] Loaded slot=%s actors=%d player=%s"),
				*LoadingName, RestoredActorNames.Num(),
				LoadedGame->bHasPlayerPawnState ? TEXT("restored") : TEXT("legacy-default"));
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("[APS.Save] No save game found in slot: %s"), *LoadingName);
		}
	}
}



bool AGravityPlayerController::GetLoadingMode()
{
	if (const UGameInstance* GameInstance = GetWorld()->GetGameInstance())
	{
		if (const UMainGameplayInstance* MainGameplayInstance = GameInstance->GetSubsystem<UMainGameplayInstance>())
		{
			return MainGameplayInstance->bIsLoadingMode;
		}
	}
	return false;
}

void AGravityPlayerController::SetLoadingModeFalse()
{
	if (UGameInstance* GameInstance = GetWorld()->GetGameInstance())
	{
		if (UMainGameplayInstance* MainGameplayInstance = GameInstance->GetSubsystem<UMainGameplayInstance>())
		{
			MainGameplayInstance->bIsLoadingMode = false;
		}
	}
}

FName AGravityPlayerController::GenerateUniqueName(const FString& ObjectType)
{
	// Rio, 02.10: generated bodies carry a name only, no PLANET/MOON/spectral suffix (APSBodyNames).
	return FName(*APSBodyNames::Random(APSBodyNames::KindFromLegacy(ObjectType)));
}

FString AGravityPlayerController::GenerateUniqueSaveSlotName(const EAstroGenerationLevel AstroGenerationLevel) const
{
    FString GeneratedName;
    const FString Vowels = TEXT("aeiou");
    const FString Consonants = TEXT("bcdfghjklmnpqrstvwxyz");

    std::random_device Rd;
    std::mt19937 Generator(Rd());
    const int32 MinLength = 3; 
    const int32 MaxLength = 8;
	std::uniform_int_distribution LengthDist(MinLength, MaxLength);
    const int32 WordLength = LengthDist(Generator);

    for (int32 i = 0; i < WordLength; i++)
    {
        if (i % 2 == 0)
        {
            GeneratedName += Consonants[Generator() % Consonants.Len()];
        }
        else
        {
            GeneratedName += Vowels[Generator() % Vowels.Len()];
        }
    }

    if (GeneratedName.Len() > 0)
    {
        GeneratedName[0] = FChar::ToUpper(GeneratedName[0]);
    }

    int32 TimeStamp = static_cast<int32>(std::time(nullptr)) % 10000; // ����� ��������� 5 ���� �� ���������

	auto GetGenerationType = [](EAstroGenerationLevel Level) -> FString
	{
		switch (Level)
		{
		case EAstroGenerationLevel::StarCluster: return TEXT("Cluster");
		case EAstroGenerationLevel::GalaxiesCluster: return TEXT("GalaxiesCluster");
		case EAstroGenerationLevel::Galaxy: return TEXT("Galaxy");
		case EAstroGenerationLevel::StarSystem: return TEXT("StarSystem");
		case EAstroGenerationLevel::PlanetSystem: return TEXT("PlanetSystem");
		case EAstroGenerationLevel::SinglePlanet: return TEXT("SinglePlanet");
		default: return TEXT("Unknown");
		}
	};

	FString GenerationType = GetGenerationType(AstroGenerationLevel);
    return FString::Printf(TEXT("%s %s %d"), *GeneratedName, *GenerationType, TimeStamp);
}
