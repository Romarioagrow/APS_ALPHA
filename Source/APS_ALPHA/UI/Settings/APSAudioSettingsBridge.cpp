#include "APSAudioSettingsBridge.h"

#include "APS_ALPHA/Core/Audio/APSAudioPreferences.h"
#include "APS_ALPHA/Core/Audio/APSAudioSubsystem.h"
#include "Blueprint/UserWidget.h"
#include "Components/Slider.h"
#include "Engine/World.h"
#include "UObject/StructOnScope.h"
#include "UObject/UnrealType.h"
#include "UObject/UObjectIterator.h"

DEFINE_LOG_CATEGORY_STATIC(LogAPSAudioSettings, Log, All);

namespace
{
	struct FVolumeBinding
	{
		EAPSAudioChannel Channel;
		const TCHAR* Property;
		const TCHAR* Row;
	};

	constexpr FVolumeBinding Bindings[] = {
		{EAPSAudioChannel::Master, TEXT("MasterVolume"), TEXT("WB_Slider_MasterVolume")},
		{EAPSAudioChannel::Music, TEXT("MusicVolume"), TEXT("WB_Slider_MusicVolume")},
		{EAPSAudioChannel::Ambience, TEXT("AmbientVolume"), TEXT("WB_Slider_AmbientVolume")},
		{EAPSAudioChannel::Effects, TEXT("SFXVolume"), TEXT("WB_Slider_SFXVolume")},
		{EAPSAudioChannel::Interface, TEXT("UIVolume"), TEXT("WB_Slider_UIVolume")}
	};

	FNumericProperty* VolumeProperty(UObject* Object, const TCHAR* Name)
	{
		FNumericProperty* Property = FindFProperty<FNumericProperty>(Object->GetClass(), Name);
		return Property && Property->IsFloatingPoint() ? Property : nullptr;
	}

	float ReadVolume(UUserWidget* Panel, const FVolumeBinding& Binding)
	{
		FNumericProperty* Property = VolumeProperty(Panel, Binding.Property);
		const double Value = Property->GetFloatingPointPropertyValue(Property->ContainerPtrToValuePtr<void>(Panel));
		return FMath::IsFinite(Value) ? float(FMath::Clamp(Value, 0.0, 1.0)) : 0.f;
	}

	void WriteVolume(UUserWidget* Panel, const FVolumeBinding& Binding, float Value)
	{
		FNumericProperty* Property = VolumeProperty(Panel, Binding.Property);
		Property->SetFloatingPointPropertyValue(Property->ContainerPtrToValuePtr<void>(Panel), Value);
		UUserWidget* Row = CastChecked<UUserWidget>(Panel->GetWidgetFromName(Binding.Row));
		UFunction* Setter = Row->FindFunction(TEXT("Set_SliderValue"));
		FStructOnScope Parameters(Setter);
		FNumericProperty* InValue = FindFProperty<FNumericProperty>(Setter, TEXT("InValue"));
		InValue->SetFloatingPointPropertyValue(InValue->ContainerPtrToValuePtr<void>(Parameters.GetStructMemory()), Value);
		// The authored setter updates both the slider and its percentage label.
		Row->ProcessEvent(Setter, Parameters.GetStructMemory());
	}
}

bool UAPSAudioSettingsBridge::HasCompatibleControls(UUserWidget* Panel)
{
	if (!Panel || Panel->GetDesiredTickFrequency() == EWidgetTickFrequency::Never) return false;
	for (const FVolumeBinding& Binding : Bindings)
	{
		if (!VolumeProperty(Panel, Binding.Property)) return false;
		UUserWidget* Row = Cast<UUserWidget>(Panel->GetWidgetFromName(Binding.Row));
		if (!Row) return false;
		USlider* Slider = Cast<USlider>(Row->GetWidgetFromName(TEXT("Slider")));
		UFunction* Setter = Row->FindFunction(TEXT("Set_SliderValue"));
		FNumericProperty* Value = Setter ? FindFProperty<FNumericProperty>(Setter, TEXT("InValue")) : nullptr;
		if (!Slider || !FMath::IsNearlyZero(Slider->GetMinValue()) || !FMath::IsNearlyEqual(Slider->GetMaxValue(), 1.f)
			|| !Setter || Setter->NumParms != 1 || !Value || !Value->IsFloatingPoint()
			|| !Value->HasAnyPropertyFlags(CPF_Parm) || Value->HasAnyPropertyFlags(CPF_OutParm | CPF_ReturnParm)) return false;
	}
	UFunction* LegacySave = Panel->FindFunction(TEXT("Save_AudioSettings"));
	return LegacySave && LegacySave->NumParms == 0;
}

bool UAPSAudioSettingsBridge::Attach(UUserWidget* Panel)
{
	if (!HasCompatibleControls(Panel))
	{
		UE_LOG(LogAPSAudioSettings, Warning, TEXT("Existing Audio tab has an unsupported schema; showing native controls."));
		return false;
	}
	UAPSAudioSettingsBridge* Bridge = Panel->GetExtension<UAPSAudioSettingsBridge>();
	if (!Bridge) Bridge = Panel->AddExtension<UAPSAudioSettingsBridge>();
	return Bridge->bInitialized || Bridge->InitializeControls();
}

void UAPSAudioSettingsBridge::AttachWorldPanels(UWorld* World)
{
	// The pause menu creates the MBLS panel in Blueprint, outside the Slate root.
	// Only attach to the two authored panels after their Construct has finished.
	for (TObjectIterator<UUserWidget> It; It; ++It)
	{
		UUserWidget* Panel = *It;
		if (!IsValid(Panel) || Panel->GetWorld() != World || !Panel->GetCachedWidget().IsValid()
			|| Panel->GetExtension<UAPSAudioSettingsBridge>()) continue;
		const FString ClassPath = Panel->GetClass()->GetPathName();
		if (ClassPath == TEXT("/Game/APS/APS_ALPHA/UI/MainMenu/WBP_SettingsPanel.WBP_SettingsPanel_C")
			|| ClassPath == TEXT("/Game/MBLS/Widgets/General/SettingsPanel/WB_SettingsPanel.WB_SettingsPanel_C"))
		{
			Attach(Panel);
		}
	}
}

void UAPSAudioSettingsBridge::Construct()
{
	// UUserWidget extensions construct before Blueprint Construct. Defer sync
	// until Attach/first Tick so legacy loading cannot replace current preferences.
	bInitialized = false;
}

bool UAPSAudioSettingsBridge::InitializeControls()
{
	UUserWidget* Panel = GetUserWidget();
	if (!HasCompatibleControls(Panel)) return false;
	const UAPSAudioPreferences* Preferences = GetDefault<UAPSAudioPreferences>();
	for (int32 I = 0; I < UE_ARRAY_COUNT(Bindings); ++I)
	{
		LastValues[I] = Preferences->Get(Bindings[I].Channel);
		WriteVolume(Panel, Bindings[I], LastValues[I]);
	}
	bInitialized = true;
	// Keep the template's own save in sync for its Restore/Reload actions.
	Panel->ProcessEvent(Panel->FindFunction(TEXT("Save_AudioSettings")), nullptr);
	return true;
}

void UAPSAudioSettingsBridge::ApplyChanges()
{
	UUserWidget* Panel = GetUserWidget();
	UWorld* World = Panel->GetWorld();
	UAPSAudioSubsystem* Audio = World ? World->GetSubsystem<UAPSAudioSubsystem>() : nullptr;
	UAPSAudioPreferences* Preferences = GetMutableDefault<UAPSAudioPreferences>();
	for (int32 I = 0; I < UE_ARRAY_COUNT(Bindings); ++I)
	{
		const FVolumeBinding& Binding = Bindings[I];
		const float Value = ReadVolume(Panel, Binding);
		if (!FMath::IsNearlyEqual(Value, LastValues[I]))
		{
			// Existing BP events set these fields for dragging, keyboard and Reset.
			if (Audio) Audio->SetVolume(Binding.Channel, Value);
			else Preferences->Set(Binding.Channel, Value);
			LastValues[I] = Value;
			bDirty = true;
			SaveDelay = 0.35f;
		}
		else if (!FMath::IsNearlyEqual(Preferences->Get(Binding.Channel), LastValues[I]))
		{
			LastValues[I] = Preferences->Get(Binding.Channel);
			WriteVolume(Panel, Binding, LastValues[I]);
			bDirty = true;
			SaveDelay = 0.35f;
		}
	}
}

void UAPSAudioSettingsBridge::Tick(const FGeometry& Geometry, float DeltaTime)
{
	if (!bInitialized && !InitializeControls()) return;
	ApplyChanges();
	// Coalesce dragging/keyboard changes instead of writing a config every frame.
	if (bDirty && (SaveDelay -= DeltaTime) <= 0.f) Save();
}

void UAPSAudioSettingsBridge::Save()
{
	if (!bDirty) return;
	GetMutableDefault<UAPSAudioPreferences>()->SaveConfig();
	GetUserWidget()->ProcessEvent(GetUserWidget()->FindFunction(TEXT("Save_AudioSettings")), nullptr);
	bDirty = false;
}

void UAPSAudioSettingsBridge::Destruct()
{
	if (bInitialized) { ApplyChanges(); Save(); }
	bInitialized = false;
}
