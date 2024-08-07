// Fill out your copyright notice in the Description page of Project Settings.


#include "GravityPlayerController.h"

#include "EngineUtils.h"
#include "APS_ALPHA/Core/Saves/GameSave.h"
#include "APS_ALPHA/Core/Saves/SavedActorData.h"
#include "APS_ALPHA/Generation/AstroGenerator.h"
#include "Kismet/GameplayStatics.h"
#include "Serialization/ObjectAndNameAsStringProxyArchive.h"

void AGravityPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	InputComponent->BindAction("SaveGame", IE_Pressed, this, &AGravityPlayerController::SaveGame);
	//InputComponent->BindAction("LoadGame", IE_Pressed, this, &AGravityPlayerController::LoadGame);
}

void AGravityPlayerController::SaveGame()
{
	UGameSave* SaveGameInstance = Cast<UGameSave>(UGameplayStatics::CreateSaveGameObject(UGameSave::StaticClass()));
	if (SaveGameInstance)
	{
		SaveGameInstance->SaveSlotName = TEXT("PlayerSaveSlot");
		SaveGameInstance->UserIndex = 0;

		UWorld* World = GetWorld();
		if (World)
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
				UE_LOG(LogTemp, Warning, TEXT("Game saved successfully to slot: %s"), *SaveGameInstance->SaveSlotName);
			}
		}
	}
}


void AGravityPlayerController::LoadGame()
{
	UWorld* World = GetWorld();
	if (World)
	{
		UGameSave* LoadedGame = Cast<UGameSave>(UGameplayStatics::LoadGameFromSlot(TEXT("PlayerSaveSlot"), 0));

		if (LoadedGame)
		{
			TMap<FString, AActor*> NameToActorMap;

			// ѕервый проход: создание всех акторов
			for (const FActorSaveData& SaveData : LoadedGame->ActorSaveDataArray)
			{

				UClass* ActorClass = LoadClass<AActor>(nullptr, *SaveData.ActorClass);
				if (ActorClass)
				{
					// √енераци€ уникального имени дл€ актора
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

			// ¬торой проход: восстановление родительско-дочерних отношений
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
