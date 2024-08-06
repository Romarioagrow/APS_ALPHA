#include "SpawnClassPicker.h"
#include "SpawnDropbox.h"
#include "SpawnItem.h"
#include "APS_ALPHA/Core/Instances/MainGameplayInstance.h"
#include "APS_ALPHA/Core/Model/SpawnParameters.h"
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
	ShipyardsDropbox->ClearChildren();

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

	if (ShipyardClasses.Num() > 0)
	{
		ShipyardsDropbox->SetContent(ShipyardClasses[0]);
		for (const TSubclassOf<AActor> Class : ShipyardClasses)
		{
			AddClassToDropbox(Class, ShipyardsDropbox);
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

void USpawnClassPicker::CollectSelectedClasses() const
{
	if (!CharacterDropbox || !SpaceshipDropbox || !SpaceStationDropbox || !SpaceHeadquartersDropbox) return;

	USpawnParameters* SpawnParams = NewObject<USpawnParameters>();

	SpawnParams->BP_CharacterClass = CharacterDropbox->GetSelectedClass();
	SpawnParams->BP_HomeSpaceship = SpaceshipDropbox->GetSelectedClass();
	SpawnParams->BP_HomeSpaceStation = SpaceStationDropbox->GetSelectedClass();
	SpawnParams->BP_HomeSpaceHeadquarters = SpaceHeadquartersDropbox->GetSelectedClass();
	SpawnParams->BP_HomeSpaceShipyard = ShipyardsDropbox->GetSelectedClass();

	if (UGameInstance* GameInstance = GetWorld()->GetGameInstance())
	{
		GameInstance->GetSubsystem<UMainGameplayInstance>()->SpawnParameters = SpawnParams;
	}
}

