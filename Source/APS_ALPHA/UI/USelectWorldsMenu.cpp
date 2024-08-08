#include "USelectWorldsMenu.h"

#include "APS_ALPHA/Core/Controllers/GravityPlayerController.h"
#include "Components/UniformGridPanel.h"
#include "Kismet/GameplayStatics.h"
#include "MainMenu/ExistingWorld.h"

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

	// Получаем все файлы сохранений
	FString SaveGameDir = FPaths::ProjectSavedDir() / TEXT("SaveGames");
	TArray<FString> SaveFiles;
	IFileManager::Get().FindFiles(SaveFiles, *SaveGameDir, TEXT("*.sav"));

	// Очистка панели перед добавлением новых инстансов
	UniformGridPanel_ExistingWorlds->ClearChildren();

	// Переменные для размещения виджетов в сетке
	int32 Row = 0;
	int32 Column = 0;

	for (const FString& SaveFile : SaveFiles)
	{
		// Создаем инстанс виджета WBP_ExistingWorld
		if (UExistingWorld* ExistingWorldWidget = CreateWidget<UExistingWorld>(GetWorld(), BP_ExistingWorldWidgetClass))
		{
			// Задаем необходимые данные для виджета
			ExistingWorldWidget->InitializeFromSave(SaveFile);

			// Добавляем виджет в UniformGridPanel
			UUniformGridSlot* GridSlot = UniformGridPanel_ExistingWorlds->AddChildToUniformGrid(ExistingWorldWidget, Row, Column);

			//UniformGridPanel_ExistingWorlds->AddChildToUniformGrid(ExistingWorldWidget, Row, Column);

			// Обновляем позицию для следующего виджета
			Column++;
			if (Column >= 3) // Предположим, что в каждой строке будет по 3 элемента
			{
				Column = 0;
				Row++;
			}
		}
	}
}