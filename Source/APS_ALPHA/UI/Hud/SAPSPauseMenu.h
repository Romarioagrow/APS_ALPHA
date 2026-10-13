#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

class SWidgetSwitcher;

/**
 * Rio 10.10 ("the Escape menu is still the old one"): the in-game pause menu drawn natively in the interface theme,
 * as the SETTINGS page and the colony terminal are. CONTINUE, SETTINGS (the native page in place), LEAVE GAME (saves,
 * then the main menu) and QUIT GAME (saves, then quits). Escape closes it, or steps back out of SETTINGS.
 * Opened by UAPSPauseMenuSubsystem; the level Blueprint's old WBP_PauseMenu no longer gets the key.
 */
class SAPSPauseMenu final : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SAPSPauseMenu) : _World(nullptr) {}
		SLATE_ARGUMENT(UWorld*, World)
		SLATE_ARGUMENT(FText, WorldName)
		SLATE_EVENT(FSimpleDelegate, OnContinue)
		SLATE_EVENT(FSimpleDelegate, OnLeave)
		SLATE_EVENT(FSimpleDelegate, OnQuit)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	virtual bool SupportsKeyboardFocus() const override { return true; }
	virtual FReply OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent) override;
	virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override { return FReply::Handled(); }

private:
	TSharedRef<SWidget> BuildMain();
	TSharedRef<SWidget> BuildSettings();
	void ShowSettings(bool bShow);

	TWeakObjectPtr<UWorld> World;
	FText WorldName;
	FSimpleDelegate OnContinue;
	FSimpleDelegate OnLeave;
	FSimpleDelegate OnQuit;
	TSharedPtr<SWidgetSwitcher> Switcher;
	TSharedPtr<SWidget> SettingsHost;
};
