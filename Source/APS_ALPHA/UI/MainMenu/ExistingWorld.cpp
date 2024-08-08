#include "ExistingWorld.h"

#include "APS_ALPHA/Core/Saves/GameSave.h"
#include "Components/Button.h"
#include "Kismet/GameplayStatics.h"

void UExistingWorld::InitializeFromSave(const FString& InSaveFileName)
{
	SaveFileName = InSaveFileName;

	if (Button) 
	{
		Button->OnClicked.AddDynamic(this, &UExistingWorld::HandleOnClicked);
	}
}

void UExistingWorld::HandleOnClicked()
{
	OnClicked.Broadcast(SaveFileName);
}

void UExistingWorld::NativeConstruct()
{
	Super::NativeConstruct();
}