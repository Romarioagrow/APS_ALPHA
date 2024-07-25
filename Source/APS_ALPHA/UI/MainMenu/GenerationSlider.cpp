#include "GenerationSlider.h"

#include "Components/Slider.h"
#include "Components/TextBlock.h"

void UGenerationSlider::PopulateEnumArray(const UEnum* Enum)
{
	if (Enum)
	{
		EnumArray.Empty();
		const int32 NumEnums = Enum->NumEnums() - 1; // -1 to exclude the "_MAX" value
		EnumSlider->SetMaxValue(NumEnums - 1);
		for (int32 i = 0; i < NumEnums; i++)
		{
			const int64 EnumValue = Enum->GetValueByIndex(i);
			EnumArray.Add(static_cast<uint8>(EnumValue));

			// Получение имени элемента Enum
			FString EnumName = Enum->GetNameStringByIndex(i);
			/*UE_LOG(LogTemp, Warning, TEXT("Enum Element: %s"), *EnumName);
			if (GEngine)
			{
				GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Green, FString::Printf(TEXT("Populate Enum Element: %s"), *EnumName));
			}*/
		}
	}
}

float UGenerationSlider::GetSliderValue()
{
	return EnumSlider->GetValue();
}

void UGenerationSlider::SetEnumContent(UEnum* InEnum)
{

	this->EnumType = InEnum;
	
	PopulateEnumArray(InEnum);

	OnSliderValueChanged(GetSliderValue());
}

void UGenerationSlider::UpdateCurrentValueText(const FString& InString)
{
	this->TextBlock_ValueText->SetText(FText::FromString(InString));
}

void UGenerationSlider::UpdateIcon(int32 Index)
{
	
}

void UGenerationSlider::OnSliderValueChanged(float Value)
{
	OnGenerationSliderChanged.Broadcast(Value, EnumType, this);
	//GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Blue, FString::Printf(TEXT("Value: %f"), Value));
}

void UGenerationSlider::NativeConstruct()
{
	Super::NativeConstruct();

	if (EnumSlider)
	{
		EnumSlider->OnValueChanged.AddDynamic(this, &UGenerationSlider::OnSliderValueChanged);
	}
}
