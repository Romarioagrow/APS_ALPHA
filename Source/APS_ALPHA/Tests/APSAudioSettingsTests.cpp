#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "APS_ALPHA/UI/Settings/APSAudioSettingsBridge.h"
#include "Blueprint/UserWidget.h"
#include "Components/Slider.h"
#include "UObject/StructOnScope.h"
#include "UObject/UnrealType.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAPSAudioSettingsWidgetTest, "APS.Audio.SettingsWidgetContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAPSAudioSettingsWidgetTest::RunTest(const FString& Parameters)
{
	for (const TCHAR* Path : {
		TEXT("/Game/APS/APS_ALPHA/UI/MainMenu/WBP_SettingsPanel.WBP_SettingsPanel_C"),
		TEXT("/Game/MBLS/Widgets/General/SettingsPanel/WB_SettingsPanel.WB_SettingsPanel_C")})
	{
		UClass* Class = LoadClass<UUserWidget>(nullptr, Path);
		if (!TestNotNull(Path, Class)) continue;
		// Initialize the real widget trees without TakeWidget/Construct: no viewport,
		// audio device, legacy settings load/save, or user's config is touched.
		UUserWidget* Panel = NewObject<UUserWidget>(GetTransientPackage(), Class);
		if (!TestTrue(TEXT("Authored widget tree initializes"), Panel->Initialize())) continue;
		if (!TestTrue(Path, UAPSAudioSettingsBridge::HasCompatibleControls(Panel))) continue;
		for (const TCHAR* RowName : {TEXT("WB_Slider_MasterVolume"), TEXT("WB_Slider_MusicVolume"),
			TEXT("WB_Slider_AmbientVolume"), TEXT("WB_Slider_SFXVolume"), TEXT("WB_Slider_UIVolume")})
		{
			UUserWidget* Row = CastChecked<UUserWidget>(Panel->GetWidgetFromName(RowName));
			USlider* Slider = CastChecked<USlider>(Row->GetWidgetFromName(TEXT("Slider")));
			UFunction* Setter = Row->FindFunction(TEXT("Set_SliderValue"));
			FNumericProperty* InValue = FindFProperty<FNumericProperty>(Setter, TEXT("InValue"));
			for (float Volume : {0.f, 0.37f, 1.f})
			{
				FStructOnScope Args(Setter);
				InValue->SetFloatingPointPropertyValue(InValue->ContainerPtrToValuePtr<void>(Args.GetStructMemory()), Volume);
				Row->ProcessEvent(Setter, Args.GetStructMemory());
				TestEqual(FString::Printf(TEXT("%s accepts normalized gain"), RowName), Slider->GetValue(), Volume);
			}
		}
	}
	return true;
}
#endif
