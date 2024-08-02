#include "SpawnItem.h"

#include "APS_ALPHA/Core/Interfaces/ItemInfoInterface.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"

void USpawnItem::SetContent(TSubclassOf<AActor> Class)
{
	if (!Class) return;

	const AActor* DefaultActor = Cast<AActor>(Class->GetDefaultObject());
	if (DefaultActor && DefaultActor->Implements<UItemInfoInterface>())
	{
		const FText InGameName = IItemInfoInterface::Execute_GetInGameName(DefaultActor);
		UTexture2D* AvatarPicture = IItemInfoInterface::Execute_GetAvatarPicture(DefaultActor);

		if (TextBlock_ClassName)
		{
			TextBlock_ClassName->SetText(InGameName);
		}

		if (Image_ClassAvatar && AvatarPicture)
		{
			Image_ClassAvatar->SetBrushFromTexture(AvatarPicture);
		}
	}
}
