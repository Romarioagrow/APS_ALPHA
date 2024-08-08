#include "MainMenuController.h"
#include "APS_ALPHA/Core/Instances/MainGameplayInstance.h"

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
	if (UGameInstance* GameInstance = GetWorld()->GetGameInstance())
	{
		if (UMainGameplayInstance* MainGameplayInstance = GameInstance->GetSubsystem<UMainGameplayInstance>())
		{
			MainGameplayInstance->bIsLoadingMode = true;
		}
	}
}