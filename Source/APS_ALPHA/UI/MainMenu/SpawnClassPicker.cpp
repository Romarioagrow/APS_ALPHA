#include "SpawnClassPicker.h"
#include "SpawnDropbox.h"
#include "SpawnItem.h"
#include "Blueprint/WidgetTree.h"

void USpawnClassPicker::NativeConstruct()
{
	Super::NativeConstruct();

	InitializeClassPicker();

}

void USpawnClassPicker::InitializeClassPicker()
{
	CharacterDropbox->ClearChildren();
	SpaceshipDropbox->ClearChildren();
	SpaceStationDropbox->ClearChildren();
	SpaceHeadquartersDropbox->ClearChildren();

	if (CharacterClasses.Num() > 0)
	{
		CharacterDropbox->SetContent(CharacterClasses[0]);
		for (const TSubclassOf<AActor> Class : CharacterClasses)
		{
			AddClassToDropbox(Class, CharacterDropbox);
		}
	}

	if (SpaceshipClasses.Num() > 0)
	{
		SpaceshipDropbox->SetContent(SpaceshipClasses[0]);
		for (const TSubclassOf<AActor> Class : SpaceshipClasses)
		{
			AddClassToDropbox(Class, SpaceshipDropbox);
		}
	}

	if (SpaceStationClasses.Num() > 0)
	{
		SpaceStationDropbox->SetContent(SpaceStationClasses[0]);
		for (const TSubclassOf<AActor> Class : SpaceStationClasses)
		{
			AddClassToDropbox(Class, SpaceStationDropbox);
		}
	}

	if (SpaceHeadquartersClasses.Num() > 0)
	{
		SpaceHeadquartersDropbox->SetContent(SpaceHeadquartersClasses[0]);
		for (const TSubclassOf<AActor> Class : SpaceHeadquartersClasses)
		{
			AddClassToDropbox(Class, SpaceHeadquartersDropbox);
		}
	}
}

void USpawnClassPicker::AddClassToDropbox(const TSubclassOf<AActor> Class, USpawnDropbox* Dropbox)
{
	if (!BP_SpawnItemClass) return;

	if (USpawnItem* SpawnItem = CreateWidget<USpawnItem>(this, BP_SpawnItemClass))
	{
		SpawnItem->SetContent(Class);
		Dropbox->AddChild(SpawnItem);
	}
}

