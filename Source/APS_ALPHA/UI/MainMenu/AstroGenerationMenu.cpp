#include "AstroGenerationMenu.h"

#include "GenerationInput.h"
#include "GenerationSlider.h"
#include "APS_ALPHA/Core/Enums/AstroGenerationLevel.h"
#include "APS_ALPHA/Core/Enums/PlanetType.h"
#include "APS_ALPHA/Core/Model/GeneratedWorld.h"
#include "Components/SpinBox.h"

void UAstroGenerationMenu::NativeConstruct()
{
	Super::NativeConstruct();

	CreateNewGeneratedWorld();

	SetupSliders();

	SetupInputs();
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

	SetupSlider(GS_StarType, StaticEnum<EStarType>());

	SetupSlider(GS_StellarType, StaticEnum<EStellarType>());

	SetupSlider(GS_SpectralClass, StaticEnum<ESpectralClass>());

	SetupSlider(GS_PlanetType, StaticEnum<EPlanetType>());

	SetupSlider(GS_SystemDistributionType, StaticEnum<EOrbitDistributionType>());

	SetupSlider(GS_SystemPlanetaryType, StaticEnum<EPlanetarySystemType>());

	SetupSlider(GS_StarClusterComposition, StaticEnum<EStarClusterComposition>());
}

void UAstroGenerationMenu::SetupInputs()
{
	if (GI_GalaxySize && GI_GalaxySize->SpinBox_Value)
	{
		GI_GalaxySize->SpinBox_Value->OnValueChanged.AddDynamic(this, &UAstroGenerationMenu::OnGalaxySizeChanged);
	}

	if (GI_GalaxyStarCount && GI_GalaxyStarCount->SpinBox_Value)
	{
		GI_GalaxyStarCount->SpinBox_Value->OnValueChanged.AddDynamic(this, &UAstroGenerationMenu::OnGalaxyStarCountChanged);
	}

	if (GI_GalaxyStarDensity && GI_GalaxyStarDensity->SpinBox_Value)
	{
		GI_GalaxyStarDensity->SpinBox_Value->OnValueChanged.AddDynamic(this, &UAstroGenerationMenu::OnGalaxyStarDensityChanged);
	}

	if (GI_PlanetRadius && GI_PlanetRadius->SpinBox_Value)
	{
		GI_PlanetRadius->SpinBox_Value->OnValueChanged.AddDynamic(this, &UAstroGenerationMenu::OnPlanetRadiusChanged);
	}

	if (GI_MoonsAmount && GI_MoonsAmount->SpinBox_Value)
	{
		GI_MoonsAmount->SpinBox_Value->OnValueChanged.AddDynamic(this, &UAstroGenerationMenu::OnMoonsAmountChanged);
	}

	if (GI_PlanetsAmount && GI_PlanetsAmount->SpinBox_Value)
	{
		GI_PlanetsAmount->SpinBox_Value->OnValueChanged.AddDynamic(this, &UAstroGenerationMenu::OnPlanetsAmountChanged);
	}

	if (GI_StartPlanetIndex && GI_StartPlanetIndex->SpinBox_Value)
	{
		GI_StartPlanetIndex->SpinBox_Value->OnValueChanged.AddDynamic(this, &UAstroGenerationMenu::OnStartPlanetIndexChanged);
	}
}

void UAstroGenerationMenu::OnGalaxySizeChanged(const float InValue) 
{
	NewGeneratedWorld->GalaxySize = static_cast<int>(InValue);
}

void UAstroGenerationMenu::OnGalaxyStarCountChanged(const float InValue) 
{
	NewGeneratedWorld->GalaxyStarCount = static_cast<int>(InValue);
}

void UAstroGenerationMenu::OnGalaxyStarDensityChanged(const float InValue) 
{
	NewGeneratedWorld->GalaxyStarDensity = static_cast<double>(InValue);
}

void UAstroGenerationMenu::OnPlanetRadiusChanged(const float InValue) 
{
	NewGeneratedWorld->PlanetRadius = static_cast<int>(InValue);
}

void UAstroGenerationMenu::OnMoonsAmountChanged(const float InValue) 
{
	NewGeneratedWorld->GalaxyStarDensity = static_cast<double>(InValue);
}

void UAstroGenerationMenu::OnPlanetsAmountChanged(const float InValue) 
{
	NewGeneratedWorld->MoonsAmount = static_cast<double>(InValue);
}

void UAstroGenerationMenu::OnStartPlanetIndexChanged(const float InValue) 
{
	NewGeneratedWorld->StartPlanetIndex = static_cast<double>(InValue);
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
