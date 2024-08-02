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
	SpaceshipDropbox->ClearChildren();
	CharacterDropbox->ClearChildren();
	SpaceStationDropbox->ClearChildren();

	for (const TSubclassOf<AActor> Class : SpaceshipClasses)
	{
		AddClassToDropbox(Class, SpaceshipDropbox);
	}

	for (const TSubclassOf<AActor> Class : CharacterClasses)
	{
		AddClassToDropbox(Class, CharacterDropbox);
	}

	for (const TSubclassOf<AActor> Class : SpaceStationClasses)
	{
		AddClassToDropbox(Class, SpaceStationDropbox);
	}

	for (const TSubclassOf<AActor> Class : SpaceHeadquartersClasses)
	{
		AddClassToDropbox(Class, SpaceHeadquartersDropbox);
	}
}

void USpawnClassPicker::AddClassToDropbox(TSubclassOf<AActor> Class, USpawnDropbox* Dropbox)
{
	if (!BP_SpawnItemClass) return;

	if (USpawnItem* SpawnItem = CreateWidget<USpawnItem>(this, BP_SpawnItemClass))
	{
		SpawnItem->SetContent(Class);
		Dropbox->AddChild(SpawnItem);
	}

}