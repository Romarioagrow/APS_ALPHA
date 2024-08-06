#include "USelectWorldsMenu.h"

#include "APS_ALPHA/Core/Controllers/GravityPlayerController.h"
#include "Kismet/GameplayStatics.h"

void USelectWorldsMenu::LoadWorld()
{
	UWorld* World = GetWorld();
	if (World)
	{
		APlayerController* PlayerController = UGameplayStatics::GetPlayerController(World, 0);
		if (AGravityPlayerController* GravityPlayerController = Cast<AGravityPlayerController>(PlayerController))
		{
			//GravityPlayerController->LoadGame();
		}
	}
}

void USelectWorldsMenu::GenerateWorld()
{
	
}
