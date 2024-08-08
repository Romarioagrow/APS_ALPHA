#include "GravityPlayerController.h"
#include <ctime> 
#include <random>
#include "APS_ALPHA/Core/Instances/MainGameplayInstance.h"
#include "APS_ALPHA/Core/Saves/GameSave.h"
#include "APS_ALPHA/Core/Saves/SavedActorData.h"
#include "APS_ALPHA/Generation/AstroGenerator.h"
#include "Kismet/GameplayStatics.h"
#include "Serialization/ObjectAndNameAsStringProxyArchive.h"

void AGravityPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();
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

		SaveGameInstance->SaveSlotName = SaveSlotName;
		SaveGameInstance->UserIndex = 0;
	
		FGeneratedWorldData WorldSaveData = GeneratedWorldModel->SaveWorldData();
		SaveGameInstance->GeneratedWorldsDataArray.Add(WorldSaveData);
		
		if (UWorld* World = GetWorld())
		{
			TArray<AActor*> AllActors;
			UGameplayStatics::GetAllActorsOfClass(World, ABaseActor::StaticClass(), AllActors);

			for (AActor* Actor : AllActors)
			{
				if (Actor->IsValidLowLevel() && !Actor->IsPendingKill())
				{
					FActorSaveData SaveData;
					SaveData.ActorTransform = Actor->GetActorTransform();
					SaveData.ActorName = Actor->GetName();
					SaveData.ActorClass = Actor->GetClass()->GetPathName();

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

			// ������ ������: �������� ���� �������
			for (const FActorSaveData& SaveData : LoadedGame->ActorSaveDataArray)
			{
				if (UClass* ActorClass = LoadClass<AActor>(nullptr, *SaveData.ActorClass))
				{
					// ��������� ����������� ����� ��� ������
					FString UniqueName = SaveData.ActorName;
					int32 Suffix = 1;
					while (FindObject<AActor>(World, *UniqueName) != nullptr)
					{
						UniqueName = SaveData.ActorName + FString::Printf(TEXT("_%d"), Suffix++);
					}

					FActorSpawnParameters SpawnParams;
					SpawnParams.Name = FName(*UniqueName);

					AActor* Actor = World->SpawnActor<AActor>(ActorClass, SaveData.ActorTransform, SpawnParams);
					if (Actor)
					{
						FMemoryReader MemoryReader(SaveData.ActorData, true);
						FObjectAndNameAsStringProxyArchive Archive(MemoryReader, true);
						Actor->Serialize(Archive);

						NameToActorMap.Add(SaveData.ActorName, Actor);
					}
				}
			}

			// ������ ������: �������������� �����������-�������� ���������
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


FString AGravityPlayerController::GenerateUniqueSaveSlotName(const EAstroGenerationLevel AstroGenerationLevel) const
{
    // ��������� ���������� ����� �� ��������
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

    // �������� ������ ����� � ���������
    if (GeneratedName.Len() > 0)
    {
        GeneratedName[0] = FChar::ToUpper(GeneratedName[0]);
    }

    // ���������� ���������� ����� �� ��������� � ����� ����������
    int32 TimeStamp = static_cast<int32>(std::time(nullptr)) % 100000; // ����� ��������� 5 ���� �� ���������

    // ����������� ���� ���������
    FString GenerationType;
    switch (AstroGenerationLevel)
    {
    case EAstroGenerationLevel::StarCluster:
        GenerationType = TEXT("Cluster");
        break;
    case EAstroGenerationLevel::GalaxiesCluster:
        GenerationType = TEXT("GalaxiesCluster");
        break;
    case EAstroGenerationLevel::Galaxy:
        GenerationType = TEXT("Galaxy");
        break;
    case EAstroGenerationLevel::StarSystem:
        GenerationType = TEXT("StarSystem");
        break;
    case EAstroGenerationLevel::PlanetSystem:
        GenerationType = TEXT("PlanetSystem");
        break;
    case EAstroGenerationLevel::SinglePlanet:
        GenerationType = TEXT("SinglePlanet");
        break;
    default:
        GenerationType = TEXT("Unknown");
        break;
    }

    return FString::Printf(TEXT("%s %s %05d"), *GeneratedName, *GenerationType, TimeStamp);
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