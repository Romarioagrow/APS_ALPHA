#include "AstroGenerationMenu.h"

#include "GenerationSlider.h"
#include "APS_ALPHA/Core/Enums/AstroGenerationLevel.h"

void UAstroGenerationMenu::NativeConstruct()
{
	Super::NativeConstruct();

	SetupSlider(GS_GenerationLevel, StaticEnum<EAstroGenerationLevel>());

	SetupSlider(GS_StarClusterSize, StaticEnum<EStarClusterSize>());

	SetupSlider(GS_StarClusterType, StaticEnum<EStarClusterType>());

	SetupSlider(GS_StarClusterPopulation, StaticEnum<EStarClusterPopulation>());

	SetupSlider(GS_StarClusterComposition, StaticEnum<EStarClusterComposition>());
}


void UAstroGenerationMenu::SetupSlider(UGenerationSlider* Slider, UEnum* EnumType)
{
	if (Slider)
	{
		Slider->OnGenerationSliderChanged.AddDynamic(this, &UAstroGenerationMenu::HandleGenerationSlider);
		Slider->SetEnumContent(EnumType);
	}
}


void UAstroGenerationMenu::HandleGenerationSlider(const float Value, const UEnum* EnumClass)
{
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Yellow, FString::Printf(TEXT("Slider Value: %f"), Value));
	}

	/*// Вывод информации о классе EnumClass на экран
	if (EnumClass)
	{
		FString EnumClassName = EnumClass->GetName();
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Yellow,
			                                 FString::Printf(TEXT("Enum Class: %s"), *EnumClassName));
		}
	}
	*/

	if (EnumClass)
	{
		FString EnumClassName = EnumClass->GetName();
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Yellow, FString::Printf(TEXT("Enum Class: %s"), *EnumClassName));
		}

		// Получить все значения EnumClass и поместить их в массив
		TArray<int32> EnumValues;
		for (int32 i = 0; i < EnumClass->NumEnums() - 1; ++i)
		{
			EnumValues.Add(EnumClass->GetValueByIndex(i));
		}

		// Найти нужное значение по индексу Value
		//int32 EnumIndex = FMath::RoundToInt(Value * (EnumValues.Num() - 1));
		if (EnumValues.IsValidIndex(Value))
		{
			//int32 SelectedValue = EnumValues[Value];

			// Вывод значения Enum на экран
			FString EnumValueName = EnumClass->GetNameStringByIndex(Value);
			if (GEngine)
			{
				GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, FString::Printf(TEXT("Selected Enum Value: %s"), *EnumValueName));
			}

			// Динамически обновить значение енама
			//UpdateEnumValue(EnumClass, SelectedValue);
		}
	}
}
template<typename EnumType>
void UAstroGenerationMenu::UpdateEnumValue(const UEnum* EnumClass, int32 SelectedValue)
{
	if (EnumClass == StaticEnum<EnumType>())
	{
		EnumType& EnumVariable = *reinterpret_cast<EnumType*>(FindField<FFloatProperty>(UAstroGenerationMenu::StaticClass(), EnumClass->GetName()));
		EnumVariable = static_cast<EnumType>(SelectedValue);

		if (AGeneratedWorld)
		{
			EnumType AGeneratedWorld::* WorldField = *reinterpret_cast<EnumType AGeneratedWorld::**>(FindField<FFloatProperty>(AGeneratedWorld::StaticClass(), EnumClass->GetName()));
			AGeneratedWorld->*WorldField = EnumVariable;
		}
	}
}


void UAstroGenerationMenu::Update_AstroGenerationLevel(float InValue)
{
	if (GS_GenerationLevel && GS_GenerationLevel->EnumArray.Num() > 0)
	{
		if (GS_GenerationLevel->EnumArray.IsValidIndex(InValue))
		{
			uint8 EnumValue = GS_GenerationLevel->EnumArray[InValue];
			AstroGenerationLevel = static_cast<EAstroGenerationLevel>(EnumValue);


			if (GEngine)
			{
				FString AstroGenerationLevelName = StaticEnum<EAstroGenerationLevel>()->GetNameStringByValue(
					static_cast<uint8>(AstroGenerationLevel));
				GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red,
				                                 FString::Printf(
					                                 TEXT("Astro Generation Level: %s"), *AstroGenerationLevelName));
			}
		}
	}
}
