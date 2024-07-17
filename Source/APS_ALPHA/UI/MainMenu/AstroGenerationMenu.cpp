#include "AstroGenerationMenu.h"

#include "GenerationSlider.h"
#include "APS_ALPHA/Core/Enums/AstroGenerationLevel.h"
#include "APS_ALPHA/Core/Model/GeneratedWorld.h"

void UAstroGenerationMenu::NativeConstruct()
{
	Super::NativeConstruct();

	NewGeneratedWorld = NewObject<UGeneratedWorld>(this, UGeneratedWorld::StaticClass());

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
	if (!GEngine) return;
	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Yellow, FString::Printf(TEXT("Slider Value: %f"), Value));

	if (EnumClass)
	{
		FString EnumClassName = EnumClass->GetName();
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Yellow, FString::Printf(TEXT("Enum Class: %s"), *EnumClassName));

		// Получить все значения EnumClass и поместить их в массив
		TArray<int32> EnumValues;
		for (int32 i = 0; i < EnumClass->NumEnums() - 1; ++i)
		{
			EnumValues.Add(EnumClass->GetValueByIndex(i));
		}

		int32 EnumIndex = static_cast<int32>(Value);  // Используем Value как индекс напрямую
		if (EnumValues.IsValidIndex(EnumIndex))
		{
			int32 SelectedValue = EnumValues[EnumIndex];

			// Вывод значения Enum на экран
			FString EnumValueName = EnumClass->GetNameStringByIndex(EnumIndex);
			GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, FString::Printf(TEXT("Selected Enum Value: %s"), *EnumValueName));

			// Динамически обновить значение енама в AGeneratedWorld
			UpdateGeneratedWorldEnumValue(EnumClass, SelectedValue);
		}
	}
}

void UAstroGenerationMenu::UpdateGeneratedWorldEnumValue(const UEnum* EnumClass, int32 SelectedValue)
{
    if (!NewGeneratedWorld || !EnumClass || !GEngine) 
    {
    	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("GeneratedWorld или EnumClass == nullptr"));
        return;
    }

    // Получить имя енама и соответствующее имя свойства
    FString EnumClassName = EnumClass->GetName();
	if (EnumClassName.StartsWith("E"))
	{
		EnumClassName = EnumClassName.RightChop(1); // Удаляем первую букву 'E'
	}
	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Emerald, FString::Printf(TEXT("EnumClassName: %s"), *EnumClassName));

    // Используем рефлексию для поиска свойства в классе UGeneratedWorld
    FProperty* Property = FindFProperty<FProperty>(UGeneratedWorld::StaticClass(), *EnumClassName);
    if (Property)
    {
    	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Blue, TEXT("Property найден"));

        // Обновляем значение свойства
        void* ValuePtr = Property->ContainerPtrToValuePtr<void>(NewGeneratedWorld);
        if (FByteProperty* ByteProperty = CastField<FByteProperty>(Property))
        {
            ByteProperty->SetPropertyValue(ValuePtr, static_cast<uint8>(SelectedValue));
        	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Blue, TEXT("ByteProperty обновлен"));

        }
        else if (FEnumProperty* EnumProperty = CastField<FEnumProperty>(Property))
        {
            FNumericProperty* UnderlyingProperty = EnumProperty->GetUnderlyingProperty();
            UnderlyingProperty->SetIntPropertyValue(ValuePtr, static_cast<int64>(SelectedValue));
        	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Blue, TEXT("EnumProperty обновлен"));
        }

    	FString EnumValueName = EnumClass->GetNameStringByValue(SelectedValue);
    	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Purple, FString::Printf(TEXT("Updated Enum Value: %s = %d"), *EnumClassName, SelectedValue));

    	NewGeneratedWorld->PrintAllValues();
    }
    else
    {
    	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Property не найден"));
    }
}
