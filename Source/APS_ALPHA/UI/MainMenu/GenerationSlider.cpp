#include "GenerationSlider.h"

#include "Components/Slider.h"

void UGenerationSlider::PopulateEnumArray(UEnum* Enum)
{
	if (Enum)
	{
		EnumArray.Empty();
		int32 NumEnums = Enum->NumEnums() - 1; // -1 to exclude the "_MAX" value
		for (int32 i = 0; i < NumEnums; i++)
		{
			int64 EnumValue = Enum->GetValueByIndex(i);
			EnumArray.Add(static_cast<uint8>(EnumValue));

			// Получение имени элемента Enum
			FString EnumName = Enum->GetNameStringByIndex(i);
			UE_LOG(LogTemp, Warning, TEXT("Enum Element: %s"), *EnumName);
			if (GEngine)
			{
				GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Green, FString::Printf(TEXT("Enum Element: %s"), *EnumName));
			}
		}
	}
}

void UGenerationSlider::SetEnumContent(UEnum* Enum)
{
	PopulateEnumArray(Enum);
}

/*UGenerationSlider::UGenerationSlider(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer), EnumContent(nullptr), CurrentIndex(0), IconImage(nullptr), EnumSlider(nullptr)
{
}*/


void UGenerationSlider::UpdateIcon(int32 Index)
{
	
}

void UGenerationSlider::OnSliderValueChanged(float Value)
{
	
}

void UGenerationSlider::NativeConstruct()
{
	Super::NativeConstruct();

	//EnumSlider->OnValueChanged()
}
