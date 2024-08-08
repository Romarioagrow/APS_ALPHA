#include "ExistingWorld.h"

#include "APS_ALPHA/Core/Saves/GameSave.h"
#include "Kismet/GameplayStatics.h"

void UExistingWorld::InitializeFromSave(const FString& SaveFileName)
{
	if (UGameSave* LoadedGame = Cast<UGameSave>(UGameplayStatics::LoadGameFromSlot(FPaths::GetBaseFilename(SaveFileName), 0)))
	{
		// Например, установить текст или изображения на виджете на основе данных сохранения
		//WorldNameText->SetText(FText::FromString(LoadedGame->SaveSlotName));
		// Другие данные, которые нужно установить
	}
}
