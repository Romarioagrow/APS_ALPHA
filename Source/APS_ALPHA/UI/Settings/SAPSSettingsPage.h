#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

class SWidgetSwitcher;
class UWorld;
enum class EAPSUITheme : uint8;

/** The tabs of the menu's SETTINGS page. */
enum class EAPSSettingsTab : uint8
{
	Video,
	Graphics,
	Audio,
	Interface
};

DECLARE_DELEGATE_OneParam(FAPSSettingsTabChanged, EAPSSettingsTab);

/**
 * Rio 06.10 ("the settings screen is broken: make it one, in our style, and make the sound settings work"): the
 * menu's SETTINGS page drawn natively in the interface theme. It replaces the MBLS template panel, a Blueprint whose
 * Audio tab drove the template's own sound classes and save, with the same choices: VIDEO and GRAPHICS write
 * UGameUserSettings, AUDIO the game's channels (UAPSAudioPreferences), INTERFACE the theme and the type.
 */
class SAPSSettingsPage final : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SAPSSettingsPage) : _World(nullptr), _InitialTab(EAPSSettingsTab::Video) {}
		SLATE_ARGUMENT(UWorld*, World)
		SLATE_ARGUMENT(EAPSSettingsTab, InitialTab)
		/** The page is rebuilt on a theme switch; the menu keeps the open tab through this. */
		SLATE_EVENT(FAPSSettingsTabChanged, OnTabChanged)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

private:
	TSharedRef<SWidget> BuildTabButton(EAPSSettingsTab Tab, const FText& Label, const FText& Hint);
	TSharedRef<SWidget> BuildVideoTab();
	TSharedRef<SWidget> BuildGraphicsTab();
	TSharedRef<SWidget> BuildAudioTab();
	TSharedRef<SWidget> BuildInterfaceTab();
	TSharedRef<SWidget> BuildThemeCard(EAPSUITheme Theme);
	void SelectTab(EAPSSettingsTab Tab);
	void RestoreDefaults();
	void ApplyVideo();
	void StepResolution(int32 Delta);
	void StepFrameRate(int32 Delta);
	int32 ResolutionIndex() const;
	int32 FrameRateIndex() const;

	TWeakObjectPtr<UWorld> World;
	EAPSSettingsTab CurrentTab{EAPSSettingsTab::Video};
	FAPSSettingsTabChanged OnTabChanged;
	TSharedPtr<SWidgetSwitcher> Switcher;
	TArray<FIntPoint> Resolutions;
	/** Display choices wait for APPLY, so a wrong mode never traps the player; everything else applies at once. */
	bool bVideoDirty{false};
	FText VideoStatus;
};
