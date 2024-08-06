#include "SpawnDropbox.h"

#include "SpawnItem.h"
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
		if (USpawnItem* SpawnItem = Cast<USpawnItem>(Widget))
		{
			SpawnItem->OnItemClicked.AddDynamic(this, &USpawnDropbox::OnItemClicked);
		}
		DropdownBody->AddChild(Widget);
	}
}

void USpawnDropbox::OnItemClicked(TSubclassOf<AActor> ClickedClass)
{
	SetContent(ClickedClass);
}

void USpawnDropbox::SetContent(TSubclassOf<AActor> Class)
{
	if (Class)
	{
		if (AActor* DefaultActor = Class->GetDefaultObject<AActor>())
		{
			SelectedClass = Class;

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

TSubclassOf<AActor> USpawnDropbox::GetSelectedClass() const
{
	return SelectedClass;
}