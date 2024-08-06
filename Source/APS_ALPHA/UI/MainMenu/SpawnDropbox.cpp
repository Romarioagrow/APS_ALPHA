#include "SpawnDropbox.h"

#include "APS_ALPHA/Core/Interfaces/ItemInfoInterface.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
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
	if (Class)
	{
		if (AActor* DefaultActor = Class->GetDefaultObject<AActor>())
		{
			if (const IItemInfoInterface* GameActor = Cast<IItemInfoInterface>(DefaultActor))
			{
				if (TextBlock_ItemName)
				{
					TextBlock_ItemName->SetText(GameActor->Execute_GetInGameName(DefaultActor));
				}
				if (ItemImage)
				{
					ItemImage->SetBrushFromTexture(GameActor->Execute_GetAvatarPicture(DefaultActor));
				}
			}
		}
	}
}
