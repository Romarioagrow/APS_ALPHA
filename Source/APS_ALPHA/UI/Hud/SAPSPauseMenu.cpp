#include "SAPSPauseMenu.h"

#include "APS_ALPHA/UI/Settings/SAPSSettingsPage.h"
#include "APS_ALPHA/UI/Style/APSMenuChrome.h"
#include "APS_ALPHA/UI/Style/APSUITheme.h"
#include "Engine/World.h"
#include "Styling/AppStyle.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SWidgetSwitcher.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "APSPauseMenu"

namespace APSPauseMenuUI
{
	const FAPSUIThemePalette& P()
	{
		return APSUITheme::Palette();
	}

	/** The SETTINGS page's solid action button: the theme's action fill, its text colour, a chamfered frame. */
	TSharedRef<SWidget> ActionButton(const FText& Label, const FText& Key, TFunction<void()> OnPress, const bool bDanger = false)
	{
		const TSharedRef<SButton> Button = SNew(SButton)
			.ButtonStyle(FAppStyle::Get(), "NoBorder")
			.ContentPadding(0.0f)
			.OnClicked_Lambda([OnPress]() { OnPress(); return FReply::Handled(); });
		const TWeakPtr<SButton> Weak = Button;
		Button->SetContent(
			SNew(SBox).HeightOverride(48.0f).WidthOverride(360.0f)
			[
				SNew(SOverlay)
				+ SOverlay::Slot()
				[
					SNew(SAPSChamferedSurface).Brush(FAppStyle::GetBrush("WhiteBrush")).ChamferTop(true).ChamferBottom(true)
					.Tint_Lambda([Weak, bDanger]()
					{
						const TSharedPtr<SButton> Pinned = Weak.Pin();
						const bool bHover = Pinned.IsValid() && Pinned->IsHovered();
						if (bDanger)
						{
							return bHover ? P().Danger : P().PanelSoft;
						}
						return bHover ? P().ActionBright : P().Action;
					})
				]
				+ SOverlay::Slot().HAlign(HAlign_Fill).VAlign(VAlign_Center).Padding(FMargin(22.0f, 0.0f))
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center)
					[
						APSChrome::CenteredLabel(Label, APSChrome::Font("Bold", 15), TAttribute<FSlateColor>::CreateLambda([Weak, bDanger]()
						{
							const TSharedPtr<SButton> Pinned = Weak.Pin();
							const bool bHover = Pinned.IsValid() && Pinned->IsHovered();
							return FSlateColor(bDanger ? (bHover ? P().OnAction : P().Text) : P().OnAction);
						}))
					]
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(FMargin(12.0f, 0.0f, 0.0f, 0.0f))
					[
						Key.IsEmpty() ? SNullWidget::NullWidget : APSChrome::KeyChip(Key, bDanger ? P().Danger : P().ActionBright)
					]
				]
				+ SOverlay::Slot()
				[
					SNew(SAPSChamferedFrame).Thickness(1.0f)
					.Color_Lambda([Weak, bDanger]()
					{
						const TSharedPtr<SButton> Pinned = Weak.Pin();
						const bool bHover = Pinned.IsValid() && Pinned->IsHovered();
						return bDanger ? (bHover ? P().DangerBright : P().Frame) : P().ActionBright;
					})
				]
			]);
		return Button;
	}
}

void SAPSPauseMenu::Construct(const FArguments& InArgs)
{
	using namespace APSPauseMenuUI;
	World = InArgs._World;
	WorldName = InArgs._WorldName;
	OnContinue = InArgs._OnContinue;
	OnLeave = InArgs._OnLeave;
	OnQuit = InArgs._OnQuit;

	ChildSlot
	[
		SNew(SOverlay)
		// The scrim over the paused game.
		+ SOverlay::Slot()
		[
			SNew(SBorder).BorderImage(FAppStyle::GetBrush("WhiteBrush")).BorderBackgroundColor_Lambda([]() { return P().Scrim; })
		]
		+ SOverlay::Slot()
		[
			SAssignNew(Switcher, SWidgetSwitcher)
			+ SWidgetSwitcher::Slot()[BuildMain()]
			+ SWidgetSwitcher::Slot()[BuildSettings()]
		]
	];
}

TSharedRef<SWidget> SAPSPauseMenu::BuildMain()
{
	using namespace APSPauseMenuUI;
	return SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().AutoWidth()
		[
			// The left column, as the main menu's: the game's name, the world, the four actions.
			SNew(SBorder).BorderImage(FAppStyle::GetBrush("WhiteBrush")).BorderBackgroundColor_Lambda([]() { return P().Panel; })
			.Padding(FMargin(64.0f, 72.0f, 64.0f, 48.0f))
			[
				SNew(SBox).WidthOverride(360.0f)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight()
					[
						SNew(STextBlock).Text(LOCTEXT("Kicker", "APOSFERA")).Font(APSChrome::Font("Bold", 11))
						.ColorAndOpacity_Lambda([]() { return FSlateColor(P().TextQuiet); })
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 6.0f, 0.0f, 0.0f)
					[
						SNew(STextBlock).Text(LOCTEXT("Title", "PAUSED")).Font(APSChrome::Font("Bold", 30))
						.ColorAndOpacity_Lambda([]() { return FSlateColor(P().Text); })
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 8.0f, 0.0f, 0.0f)
					[
						SNew(STextBlock).Text(WorldName).Font(APSUITheme::BodyFont("Regular", 13)).AutoWrapText(true)
						.ColorAndOpacity_Lambda([]() { return FSlateColor(P().TextSoft); })
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 28.0f, 0.0f, 0.0f)
					[
						SNew(SBox).HeightOverride(1.0f)
						[
							SNew(SBorder).BorderImage(FAppStyle::GetBrush("WhiteBrush")).BorderBackgroundColor_Lambda([]() { return P().Frame; })
						]
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 28.0f, 0.0f, 0.0f)
					[
						ActionButton(LOCTEXT("Continue", "CONTINUE"), LOCTEXT("ContinueKey", "ESC"), [this]() { OnContinue.ExecuteIfBound(); })
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 12.0f, 0.0f, 0.0f)
					[
						ActionButton(LOCTEXT("Settings", "SETTINGS"), FText::GetEmpty(), [this]() { ShowSettings(true); })
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 36.0f, 0.0f, 0.0f)
					[
						ActionButton(LOCTEXT("Leave", "LEAVE GAME"), FText::GetEmpty(), [this]() { OnLeave.ExecuteIfBound(); }, true)
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 8.0f, 0.0f, 0.0f)
					[
						SNew(STextBlock).Text(LOCTEXT("LeaveHint", "Saves the world, then the main menu.")).Font(APSUITheme::BodyFont("Regular", 11))
						.ColorAndOpacity_Lambda([]() { return FSlateColor(P().TextQuiet); })
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 16.0f, 0.0f, 0.0f)
					[
						ActionButton(LOCTEXT("Quit", "QUIT GAME"), FText::GetEmpty(), [this]() { OnQuit.ExecuteIfBound(); }, true)
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 8.0f, 0.0f, 0.0f)
					[
						SNew(STextBlock).Text(LOCTEXT("QuitHint", "Saves the world, then closes the game.")).Font(APSUITheme::BodyFont("Regular", 11))
						.ColorAndOpacity_Lambda([]() { return FSlateColor(P().TextQuiet); })
					]
				]
			]
		];
}

TSharedRef<SWidget> SAPSPauseMenu::BuildSettings()
{
	using namespace APSPauseMenuUI;
	return SNew(SBorder).BorderImage(FAppStyle::GetBrush("WhiteBrush")).BorderBackgroundColor_Lambda([]() { return P().Panel; })
		.Padding(FMargin(48.0f, 40.0f, 48.0f, 32.0f))
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
				[
					ActionButton(LOCTEXT("Back", "BACK"), LOCTEXT("BackKey", "ESC"), [this]() { ShowSettings(false); })
				]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(FMargin(24.0f, 0.0f, 0.0f, 0.0f))
				[
					SNew(STextBlock).Text(LOCTEXT("SettingsTitle", "SETTINGS")).Font(APSChrome::Font("Bold", 22))
					.ColorAndOpacity_Lambda([]() { return FSlateColor(P().Text); })
				]
			]
			+ SVerticalBox::Slot().FillHeight(1.0f).Padding(0.0f, 20.0f, 0.0f, 0.0f)
			[
				SAssignNew(SettingsHost, SBox)
			]
		];
}

void SAPSPauseMenu::ShowSettings(const bool bShow)
{
	if (!Switcher.IsValid())
	{
		return;
	}
	if (bShow && SettingsHost.IsValid())
	{
		// Built on demand: the page reads the live settings when it opens.
		StaticCastSharedPtr<SBox>(SettingsHost)->SetContent(SNew(SAPSSettingsPage).World(World.Get()).InitialTab(EAPSSettingsTab::Video));
	}
	Switcher->SetActiveWidgetIndex(bShow ? 1 : 0);
	if (!bShow && SettingsHost.IsValid())
	{
		StaticCastSharedPtr<SBox>(SettingsHost)->SetContent(SNullWidget::NullWidget);
	}
}

FReply SAPSPauseMenu::OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent)
{
	if (InKeyEvent.GetKey() == EKeys::Escape && !InKeyEvent.IsRepeat())
	{
		if (Switcher.IsValid() && Switcher->GetActiveWidgetIndex() == 1)
		{
			ShowSettings(false);
		}
		else
		{
			OnContinue.ExecuteIfBound();
		}
		return FReply::Handled();
	}
	return SCompoundWidget::OnKeyDown(MyGeometry, InKeyEvent);
}

#undef LOCTEXT_NAMESPACE
