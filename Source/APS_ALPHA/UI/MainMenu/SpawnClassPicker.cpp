#include "SpawnClassPicker.h"

#include "Components/TextBlock.h"

void USpawnClassPicker::NativeConstruct()
{
	Super::NativeConstruct();

	// Инициализация массива классов
	/*SpaceshipClasses = {
		BP_HomeSpaceHeadquarters::StaticClass(),
		BP_HomeSpaceStation::StaticClass(),
		BP_HomeSpaceship::StaticClass()
	};

	InitializeClassPicker();*/

	SpaceshipClasses = {
		// Замените на ваши конкретные классы
		//BP_HomeSpaceship::StaticClass(),
		//BP_HomeSpaceStation::StaticClass()
	};

	CharacterClasses = {
		// Замените на ваши конкретные классы
		//BP_CharacterClass::StaticClass()
	};

	SpaceStationClasses = {
		// Замените на ваши конкретные классы
		//BP_HomeSpaceHeadquarters::StaticClass()
	};
	
}



void USpawnClassPicker::InitializeClassPicker()
{
	// Очистка текущих элементов
	//SpaceshipVerticalBox->ClearChildren();
	//CharacterVerticalBox->ClearChildren();
	//SpaceStationVerticalBox->ClearChildren();

	for (TSubclassOf<AActor> Class : SpaceshipClasses)
	{
		//AddClassToVerticalBox(Class, SpaceshipVerticalBox);
	}

	for (TSubclassOf<AActor> Class : CharacterClasses)
	{
		//AddClassToVerticalBox(Class, CharacterVerticalBox);
	}

	for (TSubclassOf<AActor> Class : SpaceStationClasses)
	{
		//AddClassToVerticalBox(Class, SpaceStationVerticalBox);
	}
}

/*void USpawnClassPicker::AddClassToVerticalBox(const TSubclassOf<AActor> Class, UVerticalBox* VerticalBox)
{
	//UTextBlock* TextBlock = NewObject<UTextBlock>(this);
	//TextBlock->SetText(FText::FromString(Class->GetName()));
	//VerticalBox->AddChild(TextBlock);
}*/
