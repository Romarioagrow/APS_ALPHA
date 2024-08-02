#include "SpawnDropbox.h"

#include "Components/VerticalBox.h"

void USpawnDropbox::ClearChildren()
{
	if (DropdownBody)
	{
		DropdownBody->ClearChildren();
	}
}

void USpawnDropbox::AddChild(UUserWidget* Widget)
{
	if (DropdownBody && Widget)
	{
		DropdownBody->AddChild(Widget);
	}
}

void USpawnDropbox::SetContent(TSubclassOf<AActor> Class)
{
	
}
