#include "ExistingWorld.h"
#include "Components/Button.h"

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
