// Fill out your copyright notice in the Description page of Project Settings.


#include "GravityPlayerController.h"

#include "EngineUtils.h"
#include "APS_ALPHA/Core/Saves/GameSave.h"
#include "Kismet/GameplayStatics.h"

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
		UWorld* World = GetWorld();
		if (World)
		{
			SaveGameInstance->ActorTransforms.Empty();
			SaveGameInstance->ActorNames.Empty();

			for (TActorIterator<AActor> It(World); It; ++It)
			{
				AActor* Actor = *It;
				if (Actor)
				{
					SaveGameInstance->ActorTransforms.Add(Actor->GetTransform());
					SaveGameInstance->ActorNames.Add(Actor->GetName());
				}
			}

			if (UGameplayStatics::SaveGameToSlot(SaveGameInstance, SaveGameInstance->SaveSlotName, SaveGameInstance->UserIndex))
			{
				UE_LOG(LogTemp, Warning, TEXT("Game saved successfully to slot: %s"), *SaveGameInstance->SaveSlotName);
			}
			else
			{
				UE_LOG(LogTemp, Error, TEXT("Failed to save game to slot: %s"), *SaveGameInstance->SaveSlotName);
			}
		}
	}
	/*UGameSave* SaveGameInstance = Cast<UGameSave>(UGameplayStatics::CreateSaveGameObject(UGameSave::StaticClass()));
	if (SaveGameInstance)
	{
		SaveGameInstance->SaveSlotName = TEXT("PlayerSaveSlot");
		SaveGameInstance->UserIndex = 0;

		// Example: Save the transforms of all actors of a specific type
		for (TActorIterator<AActor> It(GetWorld()); It; ++It)
		{
			AActor* Actor = *It;
			if (Actor->IsA(AActor::StaticClass()))
			{
				SaveGameInstance->ActorTransforms.Add(Actor->GetTransform());
			}
		}

		if (UGameplayStatics::SaveGameToSlot(SaveGameInstance, SaveGameInstance->SaveSlotName, SaveGameInstance->UserIndex))
		{
			UE_LOG(LogTemp, Warning, TEXT("Game saved successfully to slot: %s"), *SaveGameInstance->SaveSlotName);
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("Failed to save game to slot: %s"), *SaveGameInstance->SaveSlotName);
		}
	}*/
}

/*void AGravityPlayerController::LoadGame()
{
	// Загрузка игры из сохраненного слота
	UGameSave* LoadedGame = Cast<UGameSave>(UGameplayStatics::LoadGameFromSlot(TEXT("PlayerSaveSlot"), 0));

	if (LoadedGame)
	{
		UWorld* World = GetWorld();
		if (World)
		{
			for (int32 i = 0; i < LoadedGame->ActorNames.Num(); i++)
			{
				AActor* Actor = World->SpawnActor<AActor>(AActor::StaticClass());
				Actor->Rename(*LoadedGame->ActorNames[i]);
				Actor->SetActorTransform(LoadedGame->ActorTransforms[i]);
			}

			UE_LOG(LogTemp, Warning, TEXT("Game loaded successfully from slot: %s"), *LoadedGame->SaveSlotName);
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("No save game found in slot: PlayerSaveSlot"));
	}
}*/
