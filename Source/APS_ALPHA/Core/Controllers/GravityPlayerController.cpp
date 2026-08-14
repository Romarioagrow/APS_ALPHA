#include "GravityPlayerController.h"
#include <ctime> 
#include <random>
#include "APS_ALPHA/Core/Instances/MainGameplayInstance.h"
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
	UGameSave* SaveGameInstance = Cast<UGameSave>(UGameplayStatics::CreateSaveGameObject(UGameSave::StaticClass()));
	if (SaveGameInstance)
	{
		FString SaveSlotName = GenerateUniqueSaveSlotName(AstroGenerationLevel);

		FString WorldName;
		if (int32 LastSpaceIndex; SaveSlotName.FindLastChar(TEXT(' '), LastSpaceIndex))
		{
			WorldName = SaveSlotName.Left(LastSpaceIndex);
		}

		SaveGameInstance->SaveSlotName = SaveSlotName;
		SaveGameInstance->UserIndex = 0;
	
		FGeneratedWorldData WorldSaveData = GeneratedWorldModel->SaveWorldData();
		SaveGameInstance->GeneratedWorldsDataArray.Add(WorldSaveData);
		SaveGameInstance->WorldName = WorldName;

		const TArray<FPlanetData>& InhabitedPlanets = GeneratedWorldModel->GetInhabitedPlanets();
		SaveGameInstance->InhabitedPlanetsDataArray.Append(InhabitedPlanets);
		
		if (UWorld* World = GetWorld())
		{
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
				if (Actor->IsValidLowLevel() && IsValid(Actor))
				{
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

					SaveGameInstance->ActorSaveDataArray.Add(SaveData);
				}
			}

			if (UGameplayStatics::SaveGameToSlot(SaveGameInstance, SaveGameInstance->SaveSlotName,
			                                     SaveGameInstance->UserIndex))
			{
				CurrentSaveSlotName = SaveGameInstance->SaveSlotName;
				WriteWorldMetadataSidecar(SaveGameInstance, WorldSaveData);
				UE_LOG(LogTemp, Warning, TEXT("Game saved successfully to slot: %s"), *SaveGameInstance->SaveSlotName);
			}
		}
	}
}

void AGravityPlayerController::LoadWorld()
{
	if (UWorld* World = GetWorld())
	{
		FString LoadingName = GetCurrentSaveSlotName();
		if (UGameSave* LoadedGame = Cast<UGameSave>(UGameplayStatics::LoadGameFromSlot(LoadingName, 0)))
		{
			TMap<FString, AActor*> NameToActorMap;
			TMap<FGuid, AActor*> StableIdToActorMap;

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
						if (SaveData.StableEntityId.IsValid())
						{
							StableIdToActorMap.Add(SaveData.StableEntityId, Actor);
						}
					}
				}
			}

			for (const FActorSaveData& SaveData : LoadedGame->ActorSaveDataArray)
			{
				if (!SaveData.ParentActorName.IsEmpty())
				{
					AActor** ParentActor = NameToActorMap.Find(SaveData.ParentActorName);
					AActor** ChildActor = NameToActorMap.Find(SaveData.ActorName);

					if (ParentActor && ChildActor)
					{
						(*ChildActor)->AttachToActor(*ParentActor, FAttachmentTransformRules::KeepWorldTransform);
					}
				}
			}

			UE_LOG(LogTemp, Warning, TEXT("Game loaded successfully from slot: %s"), *LoadedGame->SaveSlotName);
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("No save game found in slot: PlayerSaveSlot"));
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
