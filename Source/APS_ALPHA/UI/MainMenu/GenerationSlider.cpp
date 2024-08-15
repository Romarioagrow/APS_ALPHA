#include "GenerationSlider.h"

#include "Components/Slider.h"
#include "Components/TextBlock.h"

void UGenerationSlider::PopulateEnumArray()
{
	if (EnumType)
	{
		EnumArray.Empty();
		const int32 NumEnums = EnumType->NumEnums() - 1; // -1 to exclude the "_MAX" value
		EnumSlider->SetMaxValue(NumEnums - 1);

		for (int32 i = 0; i < NumEnums; i++)
		{
			// Получение имени элемента Enum
			FString EnumName = EnumType->GetNameStringByIndex(i);

			// Проверка, содержит ли имя строку "Unknown"
			if (EnumName.Contains("Unknown"))
			{
				UE_LOG(LogTemp, Warning, TEXT("Skipped enum: %s"), *EnumName); // Логирование пропуска
				continue; // Пропускаем этот элемент, если он содержит "Unknown"
			}

			// Если элемент не содержит "Unknown", добавляем его в массив
			const int64 EnumValue = EnumType->GetValueByIndex(i);
			EnumArray.Add(static_cast<uint8>(EnumValue));

			// Логирование добавленного элемента
			UE_LOG(LogTemp, Log, TEXT("Added enum: %s"), *EnumName);
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
	
	PopulateEnumArray();

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
}

void UGenerationSlider::NativeConstruct()
{
	Super::NativeConstruct();

	if (EnumSlider)
	{

		//FilterEnumValues();
		
		EnumSlider->OnValueChanged.AddDynamic(this, &UGenerationSlider::OnSliderValueChanged);
	}
}

/*void UGenerationSlider::FilterEnumValues()
{
	if (EnumType)
	{
		// Очищаем массив перед заполнением
		EnumArray.Empty();

		// Получаем все значения EnumType и фильтруем их
		for (int32 i = 0; i < EnumType->NumEnums() - 1; ++i) // -1 to exclude "_MAX" value
		{
			// Получение имени элемента Enum
			FString EnumValueName = EnumType->GetNameStringByIndex(i);

			// Пропуск элементов, содержащих "Unknown"
			if (!EnumValueName.Contains("Unknown"))
			{
				// Добавляем значение Enum, если оно не содержит "Unknown"
				EnumArray.Add(static_cast<uint8>(EnumType->GetValueByIndex(i)));
			}
			else
			{
				EnumSlider->SetMaxValue(EnumArray.Num() - 1);
			}
		}

		// Устанавливаем максимальное значение слайдера на основе отфильтрованных данных
		

		// Устанавливаем начальное значение слайдера
		OnSliderValueChanged(0); // Обновляем слайдер до первого валидного значения
	}
}*/
