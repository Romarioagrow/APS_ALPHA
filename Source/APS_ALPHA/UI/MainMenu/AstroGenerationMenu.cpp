#include "AstroGenerationMenu.h"

#include "GenerationInput.h"
#include "GenerationSlider.h"
#include "SWorldGenerationPanel.h"
#include "SpawnClassPicker.h"
#include "WorldGenerationViewModel.h"
#include "APS_ALPHA/Core/Enums/AstroGenerationLevel.h"
#include "APS_ALPHA/Core/Enums/PlanetType.h"
#include "APS_ALPHA/Core/Instances/MainGameplayInstance.h"
#include "APS_ALPHA/Core/Model/GeneratedWorld.h"
#include "APS_ALPHA/Generation/AstroGenerator.h"
#include "Components/Slider.h"
#include "Components/SpinBox.h"
#include "Kismet/GameplayStatics.h"

class UMainGameplayInstance;

TSharedRef<SWidget> UAstroGenerationMenu::RebuildWidget()
{
	if (bUseSlateLayout)
	{
		EnsureGenerationViewModel();
		return SNew(SWorldGenerationPanel)
			.ViewModel(WorldGenerationViewModel);
	}

	return Super::RebuildWidget();
}

void UAstroGenerationMenu::NativeConstruct()
{
	Super::NativeConstruct();

	CreateNewGeneratedWorld();

	SetupSliders();

	SetupInputs();

	if (WorldGenerationViewModel)
	{
		WorldGenerationViewModel->RequestPreview();
	}
}

void UAstroGenerationMenu::NativeDestruct()
{
	if (WorldGenerationViewModel)
	{
		WorldGenerationViewModel->Shutdown();
		bGenerationViewModelActive = false;
	}
	Super::NativeDestruct();
}

void UAstroGenerationMenu::EnsureGenerationViewModel()
{
	if (!NewGeneratedWorld)
	{
		NewGeneratedWorld = NewObject<UGeneratedWorld>(this, UGeneratedWorld::StaticClass());
	}

	if (!WorldGenerationViewModel)
	{
		WorldGenerationViewModel = NewObject<UWorldGenerationViewModel>(this);
	}

	if (!bGenerationViewModelActive)
	{
		WorldGenerationViewModel->Initialize(this, NewGeneratedWorld);
		bGenerationViewModelActive = true;
	}
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

}

void UAstroGenerationMenu::SetupInputs()
{
	if (GI_GalaxySize && GI_GalaxySize->SpinBox_Value)
	{
		GI_GalaxySize->SpinBox_Value->OnValueChanged.AddUniqueDynamic(this, &UAstroGenerationMenu::OnGalaxySizeChanged);
		OnGalaxySizeChanged(GI_GalaxySize->GetCurrentValue());
	}

	if (GI_GalaxyStarCount && GI_GalaxyStarCount->SpinBox_Value)
	{
		GI_GalaxyStarCount->SpinBox_Value->OnValueChanged.AddUniqueDynamic(this, &UAstroGenerationMenu::OnGalaxyStarCountChanged);
		OnGalaxyStarCountChanged(GI_GalaxyStarCount->GetCurrentValue());

	}

	if (GI_GalaxyStarDensity && GI_GalaxyStarDensity->SpinBox_Value)
	{
		GI_GalaxyStarDensity->SpinBox_Value->OnValueChanged.AddUniqueDynamic(this, &UAstroGenerationMenu::OnGalaxyStarDensityChanged);
		OnGalaxyStarDensityChanged(GI_GalaxyStarDensity->GetCurrentValue());

	}

	if (GI_PlanetRadius && GI_PlanetRadius->SpinBox_Value)
	{
		GI_PlanetRadius->SpinBox_Value->OnValueChanged.AddUniqueDynamic(this, &UAstroGenerationMenu::OnPlanetRadiusChanged);
		OnPlanetRadiusChanged(GI_PlanetRadius->GetCurrentValue());

	}

	if (GI_MoonsAmount && GI_MoonsAmount->SpinBox_Value)
	{
		GI_MoonsAmount->SpinBox_Value->OnValueChanged.AddUniqueDynamic(this, &UAstroGenerationMenu::OnMoonsAmountChanged);
		OnMoonsAmountChanged(GI_MoonsAmount->GetCurrentValue());

	}

	if (GI_PlanetsAmount && GI_PlanetsAmount->SpinBox_Value)
	{
		GI_PlanetsAmount->SpinBox_Value->OnValueChanged.AddUniqueDynamic(this, &UAstroGenerationMenu::OnPlanetsAmountChanged);
		OnPlanetsAmountChanged(GI_PlanetsAmount->GetCurrentValue());

	}

	if (GI_StartPlanetIndex && GI_StartPlanetIndex->SpinBox_Value)
	{
		GI_StartPlanetIndex->SpinBox_Value->OnValueChanged.AddUniqueDynamic(this, &UAstroGenerationMenu::OnStartPlanetIndexChanged);
		OnStartPlanetIndexChanged(GI_StartPlanetIndex->GetCurrentValue());

	}
}

void UAstroGenerationMenu::OnGalaxySizeChanged(const float InValue) 
{
	EnsureGenerationViewModel();
	WorldGenerationViewModel->SetGalaxySize(InValue);
}

void UAstroGenerationMenu::OnGalaxyStarCountChanged(const float InValue) 
{
	EnsureGenerationViewModel();
	WorldGenerationViewModel->SetGalaxyStarCount(InValue);
}

void UAstroGenerationMenu::OnGalaxyStarDensityChanged(const float InValue) 
{
	EnsureGenerationViewModel();
	WorldGenerationViewModel->SetGalaxyStarDensity(InValue);
}

void UAstroGenerationMenu::OnPlanetRadiusChanged(const float InValue) 
{
	EnsureGenerationViewModel();
	WorldGenerationViewModel->SetPlanetRadius(InValue);
}

void UAstroGenerationMenu::OnMoonsAmountChanged(const float InValue) 
{
	EnsureGenerationViewModel();
	WorldGenerationViewModel->SetMoonsAmount(InValue);
}

void UAstroGenerationMenu::OnPlanetsAmountChanged(const float InValue) 
{
	EnsureGenerationViewModel();
	WorldGenerationViewModel->SetPlanetsAmount(InValue);
}

void UAstroGenerationMenu::OnStartPlanetIndexChanged(const float InValue) 
{
	EnsureGenerationViewModel();
	WorldGenerationViewModel->SetStartPlanetIndex(InValue);
}

void UAstroGenerationMenu::GenerateWorldByModel()
{
	EnsureGenerationViewModel();
	WorldGenerationViewModel->CommitAndOpenLevel(FName(*LevelName));
}

void UAstroGenerationMenu::CreateNewGeneratedWorld()
{
	EnsureGenerationViewModel();
}

void UAstroGenerationMenu::SetupSlider(UGenerationSlider* Slider, UEnum* EnumType)
{
	if (Slider && IsValid(Slider))
	{
		// Проверяем, что делегат существует и не содержит уже нашу функцию
		if (!Slider->OnGenerationSliderChanged.IsBound())
		{
			Slider->OnGenerationSliderChanged.AddDynamic(this, &UAstroGenerationMenu::HandleGenerationSlider);
		}
		Slider->SetEnumContent(EnumType);
	}
}

void UAstroGenerationMenu::HandleGenerationSlider(const float Value, const UEnum* EnumClass, UGenerationSlider* Slider)
{
	if (!GEngine) return;

	if (EnumClass)
	{
		// Get all EnumClass values and put them in an array
		TArray<int32> EnumValues;
		for (int32 i = 0; i < EnumClass->NumEnums() - 1; ++i)
		{
			// Получение имени элемента Enum
			FString EnumValueName = EnumClass->GetNameStringByIndex(i);

			// Проверка, содержит ли имя строку "Unknown"
			if (EnumValueName.Contains("Unknown"))
			{
				Slider->EnumSlider->SetMaxValue(EnumValues.Num() - 1);
				UE_LOG(LogTemp, Warning, TEXT("Skipped enum: %s"), *EnumValueName); // Логирование пропуска
				continue; // Пропускаем этот элемент, если он содержит "Unknown"
			}

			// Добавляем значение Enum только если оно не содержит "Unknown"
			EnumValues.Add(EnumClass->GetValueByIndex(i));
			
		}

		if (const int32 EnumIndex = static_cast<int32>(Value); EnumValues.IsValidIndex(EnumIndex))
		{
			const int32 SelectedValue = EnumValues[EnumIndex];

			// Вывод значения Enum на экран
			FString EnumValueName = EnumClass->GetNameStringByIndex(EnumIndex);

			// Dynamically update the enum value in AGEneratedWorld
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
		// Remove the first letter 'E'
		LEnumClassName = LEnumClassName.RightChop(1); 
	}
	return LEnumClassName;
}

void UAstroGenerationMenu::UpdateGeneratedWorldEnumValue(const UEnum* EnumClass, int32 SelectedValue)
{
	EnsureGenerationViewModel();
	if (WorldGenerationViewModel)
	{
		WorldGenerationViewModel->SetEnumValue(EnumClass, SelectedValue);
		return;
	}

	if (!NewGeneratedWorld || !EnumClass || !GEngine)
	{
		UE_LOG(LogTemp, Warning, TEXT("GeneratedWorld или EnumClass == nullptr!")); // Логирование пропуска
		//GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("GeneratedWorld или EnumClass == nullptr"));
		return;
	}

	const FString EnumClassName = HandleEnumClassName(EnumClass);
	// Use reflection to find a property in the UGeneratedWorld class
	if (FProperty* Property = FindFProperty<FProperty>(UGeneratedWorld::StaticClass(), *EnumClassName))
	{
		// Update the property value
		void* ValuePtr = Property->ContainerPtrToValuePtr<void>(NewGeneratedWorld);
		if (FByteProperty* ByteProperty = CastField<FByteProperty>(Property))
		{
			ByteProperty->SetPropertyValue(ValuePtr, static_cast<uint8>(SelectedValue));
		}
		else if (const FEnumProperty* EnumProperty = CastField<FEnumProperty>(Property))
		{
			const FNumericProperty* UnderlyingProperty = EnumProperty->GetUnderlyingProperty();
			UnderlyingProperty->SetIntPropertyValue(ValuePtr, static_cast<int64>(SelectedValue));
		}
	}
}
