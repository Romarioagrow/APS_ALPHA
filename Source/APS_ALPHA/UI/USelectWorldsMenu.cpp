#include "USelectWorldsMenu.h"

#include "APS_ALPHA/Core/Controllers/GravityPlayerController.h"
#include "APS_ALPHA/Core/Controllers/MainMenuController.h"
#include "APS_ALPHA/Core/Saves/GameSave.h"
#include "Components/UniformGridPanel.h"
#include "Kismet/GameplayStatics.h"
#include "MainMenu/ExistingWorld.h"
#include "MainMenu/InhabitedPlanet.h"
#include "MainMenu/WorldDetailsCard.h"

void USelectWorldsMenu::LoadWorld()
{
}

void USelectWorldsMenu::NativeConstruct()
{
	Super::NativeConstruct();

	PopulateExistingWorlds();
}

void USelectWorldsMenu::GenerateWorld()
{
}

void USelectWorldsMenu::PopulateExistingWorlds()
{
	if (!UniformGridPanel_ExistingWorlds || !BP_ExistingWorldWidgetClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("UniformGridPanel_ExistingWorlds or ExistingWorldWidgetClass is not set"));
		return;
	}

	// Get all save files
	FString SaveGameDir = FPaths::ProjectSavedDir() / TEXT("SaveGames");
	TArray<FString> SaveFiles;
	IFileManager::Get().FindFiles(SaveFiles, *SaveGameDir, TEXT("*.sav"));

	// Clear the panel before adding new instances
	UniformGridPanel_ExistingWorlds->ClearChildren();

	// Variables for placing widgets in the grid
	int32 Row = 0;
	int32 Column = 0;

	for (const FString& SaveFile : SaveFiles)
	{
		// Create an instance of the WBP_ExistingWorld widget
		if (UExistingWorld* ExistingWorldWidget = CreateWidget<UExistingWorld>(GetWorld(), BP_ExistingWorldWidgetClass))
		{
			// Set the required data for the widget
			ExistingWorldWidget->InitializeFromSave(SaveFile);

			// Add widget to UniformGridPanel
			UniformGridPanel_ExistingWorlds->AddChildToUniformGrid(ExistingWorldWidget, Row, Column);

			ExistingWorldWidget->OnClicked.AddDynamic(this, &USelectWorldsMenu::UpdateWorldDetails);

			if (Row == 0 && Column == 0)
			{
				ExistingWorldWidget->OnClicked.Broadcast(SaveFile);
			}
			
			// Update the position for the next widget
			Column++;
			if (Column >= 3) 
			{
				Column = 0;
				Row++;
			}
		}
	}
}

void USelectWorldsMenu::UpdateWorldDetails(FString SaveFileName)
{
	// Загружаем данные сохранения
	if (UGameSave* LoadedGame = Cast<UGameSave>(
		UGameplayStatics::LoadGameFromSlot(FPaths::GetBaseFilename(SaveFileName), 0)))
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("UpdateWorldDetails"));
		
		// Обновляем данные для карточек мира
		if (ClusterDetailsCard) { ClusterDetailsCard->UpdateDetails(LoadedGame); }
		if (StarDetailsCard) { StarDetailsCard->UpdateDetails(LoadedGame); }
		if (StarSystemDetailsCard) { StarSystemDetailsCard->UpdateDetails(LoadedGame); }
		if (PlanetDetailsCard) { PlanetDetailsCard->UpdateDetails(LoadedGame); }
		if (PlanetInfoCard) { PlanetInfoCard->UpdateDetails(LoadedGame); }


		if (UniformGridPanel_InhabitedPlanets)
		{
			UniformGridPanel_InhabitedPlanets->ClearChildren();

			// Создаем виджеты для каждой заселенной планеты
			for (int32 Index = 0; Index < LoadedGame->InhabitedPlanetsDataArray.Num(); ++Index)
			{
				const FPlanetData& PlanetData = LoadedGame->InhabitedPlanetsDataArray[Index];

				// Создаем виджет
				if (BP_InhabitedPlanetWidgetClass)
				{
					UInhabitedPlanet* PlanetWidget = CreateWidget<UInhabitedPlanet>(
						GetWorld(), BP_InhabitedPlanetWidgetClass);

					if (PlanetWidget)
					{
						// Настройте виджет на основе данных планеты
						PlanetWidget->Setup(PlanetData);

						// Добавляем виджет в панель
						UniformGridPanel_InhabitedPlanets->AddChildToUniformGrid(PlanetWidget, Index / 2, Index % 2);
					}
				}
			}
		}
		
		// Вызов метода из контроллера для установки SaveSlotName
		if (APlayerController* PlayerController = UGameplayStatics::GetPlayerController(GetWorld(), 0))
		{
			if (AMainMenuController* MainMenuController = Cast<AMainMenuController>(PlayerController))
			{
				MainMenuController->SetSaveSlotName(SaveFileName);
				UE_LOG(LogTemp, Log, TEXT("SaveSlotName set to: %s"), *SaveFileName);
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("Failed to cast to AMainMenuController"));
			}
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("PlayerController is null"));
		}
	}
}
