// Fill out your copyright notice in the Description page of Project Settings.


#include "MainMenuController.h"

#include "GravityPlayerController.h"
#include "APS_ALPHA/Core/Saves/GameSave.h"
#include "Kismet/GameplayStatics.h"

void AMainMenuController::LoadGame()
{
	/*// «агрузка игры из сохраненного слота
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
	}*/
	// «агрузка игры из сохраненного слота
	UGameSave* LoadedGame = Cast<UGameSave>(UGameplayStatics::LoadGameFromSlot(TEXT("PlayerSaveSlot"), 0));

	if (LoadedGame)
	{
		UWorld* World = GetWorld();
		if (World)
		{
			// Open the saved level
			FName SavedLevelName = FName("L_WorldGeneration");
			UGameplayStatics::OpenLevel(World, SavedLevelName);

			// ƒелегат, чтобы подождать завершени€ загрузки уровн€
			FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(this, &AMainMenuController::OnLevelLoaded);
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("No save game found in slot: PlayerSaveSlot"));
	}
}

void AMainMenuController::OnLevelLoaded(UWorld* LoadedWorld)
{
	// «агрузка сохраненных акторов
	UGameSave* LoadedGame = Cast<UGameSave>(UGameplayStatics::LoadGameFromSlot(TEXT("PlayerSaveSlot"), 0));

	if (LoadedGame && LoadedWorld)
	{
		for (int32 i = 0; i < LoadedGame->ActorNames.Num(); i++)
		{
			AActor* Actor = LoadedWorld->SpawnActor<AActor>(AActor::StaticClass());
			Actor->Rename(*LoadedGame->ActorNames[i]);
			Actor->SetActorTransform(LoadedGame->ActorTransforms[i]);
		}

		UE_LOG(LogTemp, Warning, TEXT("Game loaded successfully from slot: %s"), *LoadedGame->SaveSlotName);
	}
}