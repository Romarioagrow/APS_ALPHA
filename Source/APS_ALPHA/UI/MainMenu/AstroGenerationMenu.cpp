#include "AstroGenerationMenu.h"

#include "GenerationSlider.h"
#include "APS_ALPHA/Core/Enums/AstroGenerationLevel.h"
#include "APS_ALPHA/Core/Enums/PlanetType.h"
#include "APS_ALPHA/Core/Model/GeneratedWorld.h"

void UAstroGenerationMenu::NativeConstruct()
{
	Super::NativeConstruct();

	CreateNewGeneratedWorld();

	SetupSliders();
}

/*
 * Setup required generation sliders
 */
void UAstroGenerationMenu::SetupSliders()
{
	SetupSlider(GS_GenerationLevel, StaticEnum<EAstroGenerationLevel>());

	SetupSlider(GS_StarClusterSize, StaticEnum<EStarClusterSize>());

	SetupSlider(GS_StarClusterType, StaticEnum<EStarClusterType>());

	SetupSlider(GS_StarClusterPopulation, StaticEnum<EStarClusterPopulation>());

	SetupSlider(GS_StarClusterComposition, StaticEnum<EStarClusterComposition>());

	SetupSlider(GS_GalaxyType, StaticEnum<EGalaxyType>());

	SetupSlider(GS_GalaxyClass, StaticEnum<EGalaxyClass>());

	SetupSlider(GS_StarType, StaticEnum<EStarSystemType>());

	SetupSlider(GS_StellarType, StaticEnum<EStellarType>());

	SetupSlider(GS_SpectralClass, StaticEnum<ESpectralClass>());

	SetupSlider(GS_PlanetType, StaticEnum<EPlanetType>());

	SetupSlider(GS_SystemDistributionType, StaticEnum<EOrbitDistributionType>());

	SetupSlider(GS_SystemPlanetaryType, StaticEnum<EPlanetarySystemType>());

	SetupSlider(GS_StarClusterComposition, StaticEnum<EStarClusterComposition>());
}

void UAstroGenerationMenu::CreateNewGeneratedWorld()
{
	NewGeneratedWorld = NewObject<UGeneratedWorld>(this, UGeneratedWorld::StaticClass());
}

void UAstroGenerationMenu::SetupSlider(UGenerationSlider* Slider, UEnum* EnumType)
{
	if (Slider)
	{
		Slider->OnGenerationSliderChanged.AddDynamic(this, &UAstroGenerationMenu::HandleGenerationSlider);
		Slider->SetEnumContent(EnumType);
	}
}

void UAstroGenerationMenu::HandleGenerationSlider(const float Value, const UEnum* EnumClass, UGenerationSlider* Slider)
{
	if (!GEngine) return;
	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Yellow, FString::Printf(TEXT("Slider Value: %f"), Value));

	if (EnumClass)
	{
		FString EnumClassName = EnumClass->GetName();
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Yellow,
		                                 FString::Printf(TEXT("Enum Class: %s"), *EnumClassName));

		// Получить все значения EnumClass и поместить их в массив
		TArray<int32> EnumValues;
		for (int32 i = 0; i < EnumClass->NumEnums() - 1; ++i)
		{
			EnumValues.Add(EnumClass->GetValueByIndex(i)); //TODO: by index directly
		}

		if (const int32 EnumIndex = static_cast<int32>(Value); EnumValues.IsValidIndex(EnumIndex))
		{
			const int32 SelectedValue = EnumValues[EnumIndex];

			// Вывод значения Enum на экран
			FString EnumValueName = EnumClass->GetNameStringByIndex(EnumIndex);
			GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green,
			                                 FString::Printf(TEXT("Selected Enum Value: %s"), *EnumValueName));

			// Динамически обновить значение енама в AGeneratedWorld
			UpdateGeneratedWorldEnumValue(EnumClass, SelectedValue);

			Slider->UpdateCurrentValueText(EnumValueName);
		}
	}
}

FString UAstroGenerationMenu::HandleEnumClassName(const UEnum* EnumClass)
{
	FString LEnumClassName = EnumClass->GetName();
	if (LEnumClassName.StartsWith("E"))
	{
		LEnumClassName = LEnumClassName.RightChop(1); // Удаляем первую букву 'E'
	}
	return LEnumClassName;
}

void UAstroGenerationMenu::UpdateGeneratedWorldEnumValue(const UEnum* EnumClass, int32 SelectedValue)
{
	if (!NewGeneratedWorld || !EnumClass || !GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("GeneratedWorld или EnumClass == nullptr"));
		return;
	}

	// Получить имя енама и соответствующее имя свойства
	const FString EnumClassName = HandleEnumClassName(EnumClass);
	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Emerald,
	                                 FString::Printf(TEXT("EnumClassName: %s"), *EnumClassName));

	// Используем рефлексию для поиска свойства в классе UGeneratedWorld
	if (FProperty* Property = FindFProperty<FProperty>(UGeneratedWorld::StaticClass(), *EnumClassName))
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Blue, TEXT("Property найден"));

		// Обновляем значение свойства
		void* ValuePtr = Property->ContainerPtrToValuePtr<void>(NewGeneratedWorld);
		if (FByteProperty* ByteProperty = CastField<FByteProperty>(Property))
		{
			ByteProperty->SetPropertyValue(ValuePtr, static_cast<uint8>(SelectedValue));
			GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Blue, TEXT("ByteProperty обновлен"));
		}
		else if (const FEnumProperty* EnumProperty = CastField<FEnumProperty>(Property))
		{
			const FNumericProperty* UnderlyingProperty = EnumProperty->GetUnderlyingProperty();
			UnderlyingProperty->SetIntPropertyValue(ValuePtr, static_cast<int64>(SelectedValue));
			GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Blue, TEXT("EnumProperty обновлен"));
		}

		FString EnumValueName = EnumClass->GetNameStringByValue(SelectedValue);
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Purple,
		                                 FString::Printf(
			                                 TEXT("Updated Enum Value: %s = %d"), *EnumClassName, SelectedValue));

		NewGeneratedWorld->PrintAllValues();
	}
	else
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Property не найден"));
	}
}
