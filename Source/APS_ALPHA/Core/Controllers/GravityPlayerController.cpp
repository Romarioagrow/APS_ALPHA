#include "GravityPlayerController.h"
#include <ctime> 
#include <random>
#include "APS_ALPHA/Actors/Astro/WorldActor.h"
#include "APS_ALPHA/Core/Instances/MainGameplayInstance.h"
#include "APS_ALPHA/Core/Saves/APSWorldSaveSnapshot.h"
#include "APS_ALPHA/Core/Saves/GameSave.h"
#include "APS_ALPHA/Core/Saves/SavedActorData.h"
#include "APS_ALPHA/Gameplay/Civilizations/APSCivilizationIdentityComponent.h"
#include "APS_ALPHA/Gameplay/Civilizations/APSCivilizationMaterializationSubsystem.h"
#include "APS_ALPHA/Gameplay/Civilizations/APSCivilizationRuntimeManifest.h"
#include "APS_ALPHA/Generation/AstroGenerator.h"
#include "Kismet/GameplayStatics.h"
#include "APS_ALPHA/Core/Structs/PlanetarySystemGenerationModel.h"
#include "Serialization/ObjectAndNameAsStringProxyArchive.h"
#include "APS_ALPHA/UI/StrategicMap/SAPSStrategicMapPanel.h"
#include "Engine/GameViewportClient.h"
#include "GameFramework/Pawn.h"
#include "InputCoreTypes.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/Paths.h"
#include "Widgets/SWeakWidget.h"

namespace
{
	template <typename T>
	FString APSMetadataEnumLabel(T Value)
	{
		const UEnum* Enum = StaticEnum<T>();
		return Enum ? Enum->GetDisplayNameTextByValue(static_cast<int64>(Value)).ToString() : TEXT("UNKNOWN");
	}

	void WriteWorldMetadataSidecar(const UGameSave* Save, const FGeneratedWorldData& WorldData)
	{
		if (!Save || Save->SaveSlotName.IsEmpty())
		{
			return;
		}

		FConfigFile Metadata;
		Metadata.SetInt64(TEXT("APSWorld"), TEXT("Version"), 1);
		Metadata.SetString(TEXT("APSWorld"), TEXT("DisplayName"),
			*(Save->WorldName.IsEmpty() ? Save->SaveSlotName : Save->WorldName));
		Metadata.SetString(TEXT("APSWorld"), TEXT("SystemType"),
			*APSMetadataEnumLabel(WorldData.PlanetarySystemType));
		Metadata.SetString(TEXT("APSWorld"), TEXT("StarType"),
			*APSMetadataEnumLabel(WorldData.SpectralClass));
		const FString PlanetType = APSMetadataEnumLabel(WorldData.PlanetType);
		Metadata.SetString(TEXT("APSWorld"), TEXT("PlanetType"), *PlanetType);
		Metadata.SetString(TEXT("APSWorld"), TEXT("Habitability"),
			*APSMetadataEnumLabel(WorldData.PlanetHabitability));
		Metadata.SetString(TEXT("APSWorld"), TEXT("Environment"),
			*FString::Printf(TEXT("%s / %.0f KM"), *PlanetType, WorldData.PlanetRadius));
		Metadata.SetInt64(TEXT("APSWorld"), TEXT("TotalPlanets"), WorldData.PlanetsAmount);
		Metadata.SetInt64(TEXT("APSWorld"), TEXT("InhabitedPlanets"),
			Save->InhabitedPlanetsDataArray.Num());

		const FString MetadataPath = FPaths::ProjectSavedDir() / TEXT("SaveGames") /
			(Save->SaveSlotName + TEXT(".apsmeta"));
		if (!Metadata.Write(MetadataPath, false))
		{
			UE_LOG(LogTemp, Warning, TEXT("[APS.Save] Could not write metadata sidecar: %s"),
				*MetadataPath);
		}
	}
}

void AGravityPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();
	if (InputComponent)
	{
		InputComponent->BindKey(EKeys::F5, IE_Pressed, this,
			&AGravityPlayerController::SaveCurrentWorld);
		InputComponent->BindKey(EKeys::F10, IE_Pressed, this, &AGravityPlayerController::ToggleStrategicMap);
	}
}

void AGravityPlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
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

	AAstroGenerator* Generator = Cast<AAstroGenerator>(
		UGameplayStatics::GetActorOfClass(GetWorld(), AAstroGenerator::StaticClass()));
	if (!Generator) return;

	StrategicMapPreviousViewTarget = GetViewTarget();
	StrategicMapWidget = SNew(SAPSStrategicMapPanel)
		.Controller(this)
		.Generator(Generator)
		.OnClose(FSimpleDelegate::CreateUObject(this, &AGravityPlayerController::ToggleStrategicMap));
	StrategicMapContainer = SNew(SWeakWidget).PossiblyNullContent(StrategicMapWidget.ToSharedRef());
	GEngine->GameViewport->AddViewportWidgetContent(StrategicMapContainer.ToSharedRef(), 900);

	bShowMouseCursor = true;
	FInputModeGameAndUI InputMode;
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	InputMode.SetHideCursorDuringCapture(false);
	InputMode.SetWidgetToFocus(StrategicMapWidget);
	SetInputMode(InputMode);
	Generator->FocusPreviewTarget(EAstroPreviewFocus::Overview, this);
}

void AGravityPlayerController::CloseStrategicMap(bool bRestoreView)
{
	if (StrategicMapContainer.IsValid() && GEngine && GEngine->GameViewport)
	{
		GEngine->GameViewport->RemoveViewportWidgetContent(StrategicMapContainer.ToSharedRef());
	}
	StrategicMapContainer.Reset();
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

	FString ExistingWorldName;
	if (const UGameSave* Existing = Cast<UGameSave>(
		UGameplayStatics::LoadGameFromSlot(SlotName, 0)))
	{
		ExistingWorldName = Existing->WorldName;
	}
	SaveWorldToSlot(SlotName, GameplayState->NewGeneratedWorld, ExistingWorldName);
}

bool AGravityPlayerController::SaveWorldToSlot(const FString& SlotName,
	UGeneratedWorld* GeneratedWorldModel, const FString& ExistingWorldName)
{
	if (SlotName.IsEmpty() || !IsValid(GeneratedWorldModel) || !GetWorld())
	{
		return false;
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

	if (APawn* PlayerPawn = GetPawn())
	{
		SaveGameInstance->bHasPlayerPawnState = true;
		SaveGameInstance->PlayerPawnClass = PlayerPawn->GetClass()->GetPathName();
		SaveGameInstance->PlayerPawnTransform = PlayerPawn->GetActorTransform();
		SaveGameInstance->PlayerControlRotation = GetControlRotation();
	}

	if (!UGameplayStatics::SaveGameToSlot(SaveGameInstance, SlotName,
		SaveGameInstance->UserIndex))
	{
		UE_LOG(LogTemp, Error, TEXT("[APS.Save] Failed to write slot: %s"), *SlotName);
		return false;
	}

	CurrentSaveSlotName = SlotName;
	if (UMainGameplayInstance* GameplayState = World->GetGameInstance()
		? World->GetGameInstance()->GetSubsystem<UMainGameplayInstance>() : nullptr)
	{
		GameplayState->SaveSlotName = SlotName;
	}
	WriteWorldMetadataSidecar(SaveGameInstance, WorldSaveData);
	UE_LOG(LogTemp, Log,
		TEXT("[APS.Save] Saved slot=%s modelBytes=%d actors=%d player=%s"),
		*SlotName, SaveGameInstance->GeneratedWorldModelData.Num(),
		SaveGameInstance->ActorSaveDataArray.Num(),
		SaveGameInstance->bHasPlayerPawnState ? TEXT("yes") : TEXT("no"));
	return true;
}

void AGravityPlayerController::LoadWorld()
{
	if (UWorld* World = GetWorld())
	{
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
							SaveData.ActorTransform, SpawnParams);
					}

					if (Actor)
					{
						FMemoryReader MemoryReader(SaveData.ActorData, true);
						FObjectAndNameAsStringProxyArchive Archive(MemoryReader, true);
						Actor->Serialize(Archive);
						Actor->SetActorTransform(SaveData.ActorTransform, false, nullptr,
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
						LoadedGame->PlayerPawnTransform, SpawnParams);
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
					PlayerPawn->SetActorTransform(LoadedGame->PlayerPawnTransform,
						false, nullptr, ETeleportType::TeleportPhysics);
					SetControlRotation(LoadedGame->PlayerControlRotation);
					SetViewTarget(PlayerPawn);
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

	FString FullName = FString::Printf(TEXT("%s %s"), *GeneratedName, *ObjectType);
	return FName(*FullName);
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
