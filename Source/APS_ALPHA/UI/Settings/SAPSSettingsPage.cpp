#include "SAPSSettingsPage.h"

#include "SAPSAudioSettings.h"
#include "APS_ALPHA/Core/Audio/APSAudioPreferences.h"
#include "APS_ALPHA/Core/Audio/APSAudioSubsystem.h"
#include "APS_ALPHA/UI/MainMenu/SAPSChamferedOverlay.h"
#include "APS_ALPHA/UI/Style/APSMenuChrome.h"
#include "APS_ALPHA/UI/Style/APSUITheme.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/GameUserSettings.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Styling/AppStyle.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SWidgetSwitcher.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "APSSettingsPage"

namespace APSSettingsUI
{
	using FIsOn = TFunction<bool()>;

	const FAPSUIThemePalette& P()
	{
		return APSUITheme::Palette();
	}

	FSlateFontInfo Display(const int32 Size)
	{
		return APSChrome::Font("Bold", Size);
	}

	FSlateFontInfo Body(const int32 Size)
	{
		return APSUITheme::BodyFont("Regular", Size);
	}

	FSlateFontInfo Kicker()
	{
		FSlateFontInfo Font = APSChrome::Font("Bold", 11);
		Font.LetterSpacing = 260;
		return Font;
	}

	UGameUserSettings* Settings()
	{
		return GEngine ? GEngine->GetGameUserSettings() : nullptr;
	}

	TSharedRef<SWidget> Hairline()
	{
		return SNew(SBox).HeightOverride(1.0f)
		[
			SNew(SBorder).BorderImage(FAppStyle::GetBrush("WhiteBrush"))
			.BorderBackgroundColor_Lambda([]() { return FSlateColor(APSUITheme::Fade(P().Frame, 0.7f)); })
		];
	}

	/** A chamfered chip: the action colour marks the picked one, the highlight the one under the pointer. */
	TSharedRef<SWidget> Chip(const FText& Label, FIsOn IsPicked, TFunction<void()> OnPick, const float MinWidth = 0.0f,
		TFunction<bool()> IsEnabled = nullptr)
	{
		const TSharedRef<SButton> Button = SNew(SButton)
			.ButtonStyle(FAppStyle::Get(), "NoBorder")
			.ContentPadding(0.0f)
			.IsEnabled_Lambda([IsEnabled]() { return !IsEnabled || IsEnabled(); })
			.OnClicked_Lambda([OnPick]() { OnPick(); return FReply::Handled(); });
		const TWeakPtr<SButton> Weak = Button;
		const auto Lit = [Weak]()
		{
			const TSharedPtr<SButton> Pinned = Weak.Pin();
			return Pinned.IsValid() && Pinned->IsHovered() && Pinned->IsEnabled();
		};
		const FSlateFontInfo LabelFont = Display(12);
		Button->SetContent(
			SNew(SBox).HeightOverride(38.0f).MinDesiredWidth(MinWidth)
			[
				SNew(SOverlay)
				+ SOverlay::Slot()
				[
					SNew(SAPSChamferedSurface).Brush(FAppStyle::GetBrush("WhiteBrush")).ChamferTop(true).ChamferBottom(true)
					.Tint_Lambda([IsPicked, Lit]()
					{
						if (IsPicked())
						{
							return FMath::Lerp(P().ActionFill, P().Action, 0.22f).CopyWithNewOpacity(0.97f);
						}
						return Lit() ? P().Raised : P().PanelSoft;
					})
				]
				+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center).Padding(FMargin(16.0f, 0.0f))
				[
					APSChrome::CenteredLabel(Label, LabelFont, TAttribute<FSlateColor>::CreateLambda([IsPicked, Weak]()
					{
						const TSharedPtr<SButton> Pinned = Weak.Pin();
						if (Pinned.IsValid() && !Pinned->IsEnabled())
						{
							return FSlateColor(P().TextDim);
						}
						return FSlateColor(IsPicked() ? P().Text : P().TextSoft);
					}))
				]
				+ SOverlay::Slot()
				[
					SNew(SAPSChamferedFrame).Thickness(1.0f)
					.Color_Lambda([IsPicked, Lit]()
					{
						return IsPicked() ? P().Action : Lit() ? P().Highlight : P().Frame;
					})
				]
			]);
		return Button;
	}

	/** < value >: the arrows step through a list; the value box keeps one width so the row does not jump. */
	TSharedRef<SWidget> Stepper(TFunction<FText()> Value, TFunction<void(int32)> Step, TFunction<bool(int32)> CanStep,
		const float ValueWidth = 190.0f)
	{
		const auto Arrow = [Step, CanStep](const int32 Delta)
		{
			const TSharedRef<SButton> Button = SNew(SButton)
				.ButtonStyle(FAppStyle::Get(), "NoBorder")
				.ContentPadding(0.0f)
				.IsEnabled_Lambda([CanStep, Delta]() { return CanStep(Delta); })
				.OnClicked_Lambda([Step, Delta]() { Step(Delta); return FReply::Handled(); });
			const TWeakPtr<SButton> Weak = Button;
			const FSlateFontInfo ArrowFont = Display(15);
			Button->SetContent(
				SNew(SBox).WidthOverride(38.0f).HeightOverride(38.0f)
				[
					SNew(SOverlay)
					+ SOverlay::Slot()
					[
						SNew(SAPSChamferedSurface).Brush(FAppStyle::GetBrush("WhiteBrush")).ChamferTop(true).ChamferBottom(true)
						.Tint_Lambda([Weak]()
						{
							const TSharedPtr<SButton> Pinned = Weak.Pin();
							return Pinned.IsValid() && Pinned->IsHovered() && Pinned->IsEnabled() ? P().Raised : P().PanelSoft;
						})
					]
					+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center)
					[
						SNew(STextBlock).Text(FText::FromString(Delta < 0 ? TEXT("<") : TEXT(">"))).Font(ArrowFont)
						.Justification(ETextJustify::Center).RenderTransform(APSChrome::SymbolCenterShift(ArrowFont))
						.ColorAndOpacity_Lambda([Weak]()
						{
							const TSharedPtr<SButton> Pinned = Weak.Pin();
							return FSlateColor(Pinned.IsValid() && Pinned->IsEnabled() ? P().Text : P().TextDim);
						})
					]
					+ SOverlay::Slot()
					[
						SNew(SAPSChamferedFrame).Thickness(1.0f)
						.Color_Lambda([Weak]()
						{
							const TSharedPtr<SButton> Pinned = Weak.Pin();
							return Pinned.IsValid() && Pinned->IsHovered() && Pinned->IsEnabled() ? P().Highlight : P().Frame;
						})
					]
				]);
			return Button;
		};
		const FSlateFontInfo ValueFont = Display(13);
		return SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth()[Arrow(-1)]
			+ SHorizontalBox::Slot().AutoWidth().Padding(6.0f, 0.0f)
			[
				SNew(SBox).WidthOverride(ValueWidth).HeightOverride(38.0f)
				[
					SNew(SOverlay)
					+ SOverlay::Slot()
					[
						SNew(SAPSChamferedSurface).Brush(FAppStyle::GetBrush("WhiteBrush")).ChamferTop(true).ChamferBottom(true)
						.Tint_Lambda([]() { return P().InsetFill; })
					]
					+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center)
					[
						APSChrome::CenteredLabel(TAttribute<FText>::CreateLambda([Value]() { return Value(); }), ValueFont,
							TAttribute<FSlateColor>::CreateLambda([]() { return FSlateColor(P().Text); }))
					]
					+ SOverlay::Slot()
					[
						SNew(SAPSChamferedFrame).Thickness(1.0f).Color_Lambda([]() { return P().Frame; })
					]
				]
			]
			+ SHorizontalBox::Slot().AutoWidth()[Arrow(1)];
	}

	/** One setting: its name and a short line under it on the left, the control on the right, a rule below. */
	TSharedRef<SWidget> Row(const FText& Title, const TAttribute<FText>& Hint, const TSharedRef<SWidget>& Control)
	{
		return SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 12.0f)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight()
					[SNew(STextBlock).Text(Title).Font(Display(14)).ColorAndOpacity_Lambda([]() { return FSlateColor(P().Text); })]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 3.0f, 0.0f, 0.0f)
					[
						SNew(STextBlock).Text(Hint).Font(Body(13)).AutoWrapText(true)
						.ColorAndOpacity_Lambda([]() { return FSlateColor(P().TextSoft); })
					]
				]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(28.0f, 0.0f, 0.0f, 0.0f)
				[Control]
			]
			+ SVerticalBox::Slot().AutoHeight()[Hairline()];
	}

	/** The tab's heading: SETTINGS / TAB, its title and one line on what it changes. */
	TSharedRef<SWidget> Heading(const FText& Tab, const FText& Title, const FText& Description)
	{
		return SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(STextBlock).Text(FText::Format(LOCTEXT("HeadingKicker", "SETTINGS  /  {0}"), Tab)).Font(Kicker())
				.ColorAndOpacity_Lambda([]() { return FSlateColor(P().TextQuiet); })
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 6.0f, 0.0f, 0.0f)
			[SNew(STextBlock).Text(Title).Font(Display(30)).ColorAndOpacity_Lambda([]() { return FSlateColor(P().Text); })]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 6.0f, 0.0f, 14.0f)
			[
				SNew(STextBlock).Text(Description).Font(Body(16)).AutoWrapText(true)
				.ColorAndOpacity_Lambda([]() { return FSlateColor(P().TextSoft); })
			]
			+ SVerticalBox::Slot().AutoHeight()[Hairline()];
	}

	/** A solid action button (APPLY, AUTO-SET): the theme's action fill and its text colour. */
	TSharedRef<SWidget> ActionButton(const FText& Label, TFunction<void()> OnPress, TFunction<bool()> IsEnabled = nullptr)
	{
		const TSharedRef<SButton> Button = SNew(SButton)
			.ButtonStyle(FAppStyle::Get(), "NoBorder")
			.ContentPadding(0.0f)
			.IsEnabled_Lambda([IsEnabled]() { return !IsEnabled || IsEnabled(); })
			.OnClicked_Lambda([OnPress]() { OnPress(); return FReply::Handled(); });
		const TWeakPtr<SButton> Weak = Button;
		const FSlateFontInfo LabelFont = Display(14);
		Button->SetContent(
			SNew(SBox).HeightOverride(44.0f).MinDesiredWidth(170.0f)
			[
				SNew(SOverlay)
				+ SOverlay::Slot()
				[
					SNew(SAPSChamferedSurface).Brush(FAppStyle::GetBrush("WhiteBrush")).ChamferTop(true).ChamferBottom(true)
					.Tint_Lambda([Weak]()
					{
						const TSharedPtr<SButton> Pinned = Weak.Pin();
						if (!Pinned.IsValid() || !Pinned->IsEnabled())
						{
							return P().PanelSoft;
						}
						return Pinned->IsHovered() ? P().ActionBright : P().Action;
					})
				]
				+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center).Padding(FMargin(26.0f, 0.0f))
				[
					APSChrome::CenteredLabel(Label, LabelFont, TAttribute<FSlateColor>::CreateLambda([Weak]()
					{
						const TSharedPtr<SButton> Pinned = Weak.Pin();
						return FSlateColor(Pinned.IsValid() && Pinned->IsEnabled() ? P().OnAction : P().TextDim);
					}))
				]
				+ SOverlay::Slot()
				[
					SNew(SAPSChamferedFrame).Thickness(1.0f)
					.Color_Lambda([Weak]()
					{
						const TSharedPtr<SButton> Pinned = Weak.Pin();
						return Pinned.IsValid() && Pinned->IsEnabled() ? P().ActionBright : P().Frame;
					})
				]
			]);
		return Button;
	}

	FText QualityName(const int32 Level)
	{
		switch (Level)
		{
		case 0: return LOCTEXT("QualityLow", "LOW");
		case 1: return LOCTEXT("QualityMedium", "MEDIUM");
		case 2: return LOCTEXT("QualityHigh", "HIGH");
		case 3: return LOCTEXT("QualityEpic", "EPIC");
		case 4: return LOCTEXT("QualityCinematic", "CINEMATIC");
		default: return LOCTEXT("QualityCustom", "CUSTOM");
		}
	}

	struct FQualityGroup
	{
		FText Label;
		FText Hint;
		int32 (UGameUserSettings::*Get)() const;
		void (UGameUserSettings::*Set)(int32);
	};

	/** Frame-rate caps the stepper walks through; 0 is no cap. */
	constexpr float FrameRates[] = {30.0f, 60.0f, 90.0f, 120.0f, 144.0f, 165.0f, 240.0f, 0.0f};
	constexpr int32 FrameRateCount = static_cast<int32>(UE_ARRAY_COUNT(FrameRates));

	// Rio 06.10: the defaults of UAPSAudioPreferences (Core/Audio/APSAudioPreferences.h), for RESTORE DEFAULTS.
	constexpr float DefaultVolumes[] = {0.8f, 0.35f, 0.4f, 0.65f, 0.5f};
	constexpr int32 ChannelCount = static_cast<int32>(UE_ARRAY_COUNT(DefaultVolumes));
}

void SAPSSettingsPage::Construct(const FArguments& InArgs)
{
	using namespace APSSettingsUI;
	World = InArgs._World;
	CurrentTab = InArgs._InitialTab;
	OnTabChanged = InArgs._OnTabChanged;
	if (!UKismetSystemLibrary::GetSupportedFullscreenResolutions(Resolutions) || Resolutions.Num() == 0)
	{
		UKismetSystemLibrary::GetConvenientWindowedResolutions(Resolutions);
	}
	if (const UGameUserSettings* S = Settings())
	{
		Resolutions.AddUnique(S->GetScreenResolution());
	}
	Resolutions.Sort([](const FIntPoint& A, const FIntPoint& B) { return A.X != B.X ? A.X < B.X : A.Y < B.Y; });

	const TSharedRef<SVerticalBox> Tabs = SNew(SVerticalBox);
	const auto AddTab = [this, &Tabs](const EAPSSettingsTab Tab, const FText& Label, const FText& Hint)
	{
		Tabs->AddSlot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 10.0f)[BuildTabButton(Tab, Label, Hint)];
	};
	AddTab(EAPSSettingsTab::Video, LOCTEXT("TabVideo", "VIDEO"), LOCTEXT("TabVideoHint", "Display, resolution, frame rate"));
	AddTab(EAPSSettingsTab::Graphics, LOCTEXT("TabGraphics", "GRAPHICS"), LOCTEXT("TabGraphicsHint", "Quality of each part of the image"));
	AddTab(EAPSSettingsTab::Audio, LOCTEXT("TabAudio", "AUDIO"), LOCTEXT("TabAudioHint", "Volume of each channel"));
	AddTab(EAPSSettingsTab::Interface, LOCTEXT("TabInterface", "INTERFACE"), LOCTEXT("TabInterfaceHint", "Theme and type"));
	Tabs->AddSlot().FillHeight(1.0f);
	Tabs->AddSlot().AutoHeight()
	[
		Chip(LOCTEXT("RestoreDefaults", "RESTORE DEFAULTS"), []() { return false; }, [this]() { RestoreDefaults(); }, 300.0f)
	];
	Tabs->AddSlot().AutoHeight().Padding(2.0f, 8.0f, 2.0f, 0.0f)
	[
		SNew(STextBlock).AutoWrapText(true).Font(Body(12))
		.ColorAndOpacity_Lambda([]() { return FSlateColor(P().TextQuiet); })
		.Text_Lambda([this]()
		{
			switch (CurrentTab)
			{
			case EAPSSettingsTab::Audio: return LOCTEXT("RestoreAudioHint", "Puts every volume back to its default.");
			case EAPSSettingsTab::Interface: return LOCTEXT("RestoreInterfaceHint", "Back to Obsidian and the new type.");
			default: return LOCTEXT("RestoreVideoHint", "Puts video and graphics back to their defaults.");
			}
		})
	];

	ChildSlot
	[
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().AutoWidth()
		[
			SNew(SBox).WidthOverride(300.0f)[Tabs]
		]
		+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(24.0f, 0.0f, 0.0f, 0.0f)
		[
			APSChrome::ChamferPanel(
				SAssignNew(Switcher, SWidgetSwitcher)
				.WidgetIndex_Lambda([this]() { return static_cast<int32>(CurrentTab); })
				+ SWidgetSwitcher::Slot()[BuildVideoTab()]
				+ SWidgetSwitcher::Slot()[BuildGraphicsTab()]
				+ SWidgetSwitcher::Slot()[BuildAudioTab()]
				+ SWidgetSwitcher::Slot()[BuildInterfaceTab()],
				FMargin(34.0f, 28.0f), APSChrome::CyanDim())
		]
	];
}

TSharedRef<SWidget> SAPSSettingsPage::BuildTabButton(const EAPSSettingsTab Tab, const FText& Label, const FText& Hint)
{
	using namespace APSSettingsUI;
	const TSharedRef<SButton> Button = SNew(SButton)
		.ButtonStyle(FAppStyle::Get(), "NoBorder")
		.ContentPadding(0.0f)
		.OnClicked_Lambda([this, Tab]() { SelectTab(Tab); return FReply::Handled(); });
	const TWeakPtr<SButton> Weak = Button;
	const auto Picked = [this, Tab]() { return CurrentTab == Tab; };
	const auto Lit = [Weak]()
	{
		const TSharedPtr<SButton> Pinned = Weak.Pin();
		return Pinned.IsValid() && Pinned->IsHovered();
	};
	Button->SetContent(
		SNew(SBox).HeightOverride(70.0f)
		[
			SNew(SOverlay)
			+ SOverlay::Slot()
			[
				SNew(SAPSChamferedSurface).Brush(FAppStyle::GetBrush("WhiteBrush")).ChamferTop(true).ChamferBottom(true)
				.Tint_Lambda([Picked, Lit]()
				{
					if (Picked())
					{
						return FMath::Lerp(P().ActionFill, P().Action, 0.16f).CopyWithNewOpacity(0.97f);
					}
					return Lit() ? P().Raised : P().Panel;
				})
			]
			+ SOverlay::Slot().HAlign(HAlign_Left).VAlign(VAlign_Fill).Padding(FMargin(0.0f, 14.0f))
			[
				SNew(SBox).WidthOverride(3.0f)
				[
					SNew(SBorder).BorderImage(FAppStyle::GetBrush("WhiteBrush"))
					.BorderBackgroundColor_Lambda([Picked]() { return FSlateColor(Picked() ? P().Action : FLinearColor::Transparent); })
				]
			]
			+ SOverlay::Slot().VAlign(VAlign_Center).Padding(FMargin(22.0f, 0.0f))
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(STextBlock).Text(Label).Font(Display(16))
					.ColorAndOpacity_Lambda([Picked]() { return FSlateColor(Picked() ? P().Text : P().TextSoft); })
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 3.0f, 0.0f, 0.0f)
				[
					SNew(STextBlock).Text(Hint).Font(Body(13))
					.ColorAndOpacity_Lambda([]() { return FSlateColor(P().TextQuiet); })
				]
			]
			+ SOverlay::Slot()
			[
				SNew(SAPSChamferedFrame).Thickness(1.0f)
				.Color_Lambda([Picked, Lit]() { return Picked() ? P().Action : Lit() ? P().Highlight : P().Frame; })
			]
		]);
	return Button;
}

TSharedRef<SWidget> SAPSSettingsPage::BuildVideoTab()
{
	using namespace APSSettingsUI;
	const auto ModeChip = [this](const FText& Label, const EWindowMode::Type Mode)
	{
		return Chip(Label,
			[Mode]() { const UGameUserSettings* S = Settings(); return S && S->GetFullscreenMode() == Mode; },
			[this, Mode]()
			{
				if (UGameUserSettings* S = Settings())
				{
					S->SetFullscreenMode(Mode);
					bVideoDirty = true;
				}
			}, 120.0f);
	};
	const auto VSyncChip = [this](const FText& Label, const bool bOn)
	{
		return Chip(Label,
			[bOn]() { const UGameUserSettings* S = Settings(); return S && S->IsVSyncEnabled() == bOn; },
			[this, bOn]()
			{
				if (UGameUserSettings* S = Settings())
				{
					S->SetVSyncEnabled(bOn);
					bVideoDirty = true;
				}
			}, 84.0f);
	};

	return SNew(SScrollBox)
		+ SScrollBox::Slot()
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()
			[
				Heading(LOCTEXT("VideoKicker", "VIDEO"), LOCTEXT("VideoTitle", "DISPLAY"),
					LOCTEXT("VideoDescription", "How the game fills your screen. Display changes wait for APPLY."))
			]
			+ SVerticalBox::Slot().AutoHeight()
			[
				Row(LOCTEXT("WindowMode", "WINDOW MODE"), LOCTEXT("WindowModeHint", "Borderless keeps the desktop one key away."),
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().AutoWidth()[ModeChip(LOCTEXT("ModeFullscreen", "FULLSCREEN"), EWindowMode::Fullscreen)]
					+ SHorizontalBox::Slot().AutoWidth().Padding(8.0f, 0.0f, 0.0f, 0.0f)[ModeChip(LOCTEXT("ModeBorderless", "BORDERLESS"), EWindowMode::WindowedFullscreen)]
					+ SHorizontalBox::Slot().AutoWidth().Padding(8.0f, 0.0f, 0.0f, 0.0f)[ModeChip(LOCTEXT("ModeWindowed", "WINDOWED"), EWindowMode::Windowed)])
			]
			+ SVerticalBox::Slot().AutoHeight()
			[
				Row(LOCTEXT("Resolution", "RESOLUTION"), LOCTEXT("ResolutionHint", "The size of the rendered image."),
					Stepper([this]()
						{
							const UGameUserSettings* S = Settings();
							const FIntPoint Size = S ? S->GetScreenResolution() : FIntPoint::ZeroValue;
							return FText::FromString(FString::Printf(TEXT("%d x %d"), Size.X, Size.Y));
						},
						[this](const int32 Delta) { StepResolution(Delta); },
						[this](const int32 Delta)
						{
							const int32 Index = ResolutionIndex() + Delta;
							return Resolutions.IsValidIndex(Index);
						}))
			]
			+ SVerticalBox::Slot().AutoHeight()
			[
				Row(LOCTEXT("FrameRate", "FRAME RATE LIMIT"), LOCTEXT("FrameRateHint", "A cap saves power and heat; UNLIMITED lets the GPU run free."),
					Stepper([]()
						{
							const UGameUserSettings* S = Settings();
							const float Limit = S ? S->GetFrameRateLimit() : 0.0f;
							return Limit <= 0.0f ? LOCTEXT("Unlimited", "UNLIMITED")
								: FText::Format(LOCTEXT("FrameRateValue", "{0} FPS"), FText::AsNumber(FMath::RoundToInt(Limit)));
						},
						[this](const int32 Delta) { StepFrameRate(Delta); },
						[this](const int32 Delta)
						{
							const int32 Index = FrameRateIndex() + Delta;
							return Index >= 0 && Index < FrameRateCount;
						}))
			]
			+ SVerticalBox::Slot().AutoHeight()
			[
				Row(LOCTEXT("VSync", "VSYNC"), LOCTEXT("VSyncHint", "Removes tearing; can add a little input delay."),
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().AutoWidth()[VSyncChip(LOCTEXT("On", "ON"), true)]
					+ SHorizontalBox::Slot().AutoWidth().Padding(8.0f, 0.0f, 0.0f, 0.0f)[VSyncChip(LOCTEXT("Off", "OFF"), false)])
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 22.0f, 0.0f, 0.0f)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth()
				[
					ActionButton(LOCTEXT("Apply", "APPLY"), [this]() { ApplyVideo(); }, [this]() { return bVideoDirty; })
				]
				+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center).Padding(18.0f, 0.0f, 0.0f, 0.0f)
				[
					SNew(STextBlock).Font(Body(14)).AutoWrapText(true)
					.ColorAndOpacity_Lambda([]() { return FSlateColor(P().TextSoft); })
					.Text_Lambda([this]()
					{
						if (bVideoDirty)
						{
							return LOCTEXT("VideoPending", "Changes are waiting: APPLY switches the display.");
						}
						return VideoStatus;
					})
				]
			]
		];
}

TSharedRef<SWidget> SAPSSettingsPage::BuildGraphicsTab()
{
	using namespace APSSettingsUI;
	const TArray<FQualityGroup> Groups = {
		{LOCTEXT("ViewDistance", "VIEW DISTANCE"), LOCTEXT("ViewDistanceHint", "How far objects keep their detail."),
			&UGameUserSettings::GetViewDistanceQuality, &UGameUserSettings::SetViewDistanceQuality},
		{LOCTEXT("Shadows", "SHADOWS"), LOCTEXT("ShadowsHint", "Shadow sharpness and reach."),
			&UGameUserSettings::GetShadowQuality, &UGameUserSettings::SetShadowQuality},
		{LOCTEXT("GlobalIllumination", "GLOBAL ILLUMINATION"), LOCTEXT("GlobalIlluminationHint", "Bounced light (Lumen)."),
			&UGameUserSettings::GetGlobalIlluminationQuality, &UGameUserSettings::SetGlobalIlluminationQuality},
		{LOCTEXT("Reflections", "REFLECTIONS"), LOCTEXT("ReflectionsHint", "Mirror and glossy reflections."),
			&UGameUserSettings::GetReflectionQuality, &UGameUserSettings::SetReflectionQuality},
		{LOCTEXT("AntiAliasing", "ANTI-ALIASING"), LOCTEXT("AntiAliasingHint", "Smooth edges (TSR)."),
			&UGameUserSettings::GetAntiAliasingQuality, &UGameUserSettings::SetAntiAliasingQuality},
		{LOCTEXT("Textures", "TEXTURES"), LOCTEXT("TexturesHint", "Texture resolution and memory."),
			&UGameUserSettings::GetTextureQuality, &UGameUserSettings::SetTextureQuality},
		{LOCTEXT("Effects", "EFFECTS"), LOCTEXT("EffectsHint", "Particles and visual effects."),
			&UGameUserSettings::GetVisualEffectQuality, &UGameUserSettings::SetVisualEffectQuality},
		{LOCTEXT("PostProcessing", "POST-PROCESSING"), LOCTEXT("PostProcessingHint", "Bloom, depth of field, motion blur."),
			&UGameUserSettings::GetPostProcessingQuality, &UGameUserSettings::SetPostProcessingQuality},
		{LOCTEXT("Foliage", "FOLIAGE"), LOCTEXT("FoliageHint", "Grass and plant density."),
			&UGameUserSettings::GetFoliageQuality, &UGameUserSettings::SetFoliageQuality},
		{LOCTEXT("Shading", "SHADING"), LOCTEXT("ShadingHint", "Material detail."),
			&UGameUserSettings::GetShadingQuality, &UGameUserSettings::SetShadingQuality},
	};

	const TSharedRef<SHorizontalBox> Overall = SNew(SHorizontalBox);
	for (int32 Level = 0; Level <= 4; ++Level)
	{
		Overall->AddSlot().AutoWidth().Padding(Level == 0 ? 0.0f : 6.0f, 0.0f, 0.0f, 0.0f)
		[
			Chip(QualityName(Level),
				[Level]() { const UGameUserSettings* S = Settings(); return S && S->GetOverallScalabilityLevel() == Level; },
				[Level]()
				{
					if (UGameUserSettings* S = Settings())
					{
						S->SetOverallScalabilityLevel(Level);
						S->ApplyNonResolutionSettings();
						S->SaveSettings();
					}
				}, 92.0f)
		];
	}

	const TSharedRef<SVerticalBox> Rows = SNew(SVerticalBox);
	Rows->AddSlot().AutoHeight()
	[
		Heading(LOCTEXT("GraphicsKicker", "GRAPHICS"), LOCTEXT("GraphicsTitle", "QUALITY"),
			LOCTEXT("GraphicsDescription", "Every change applies at once and is saved."))
	];
	Rows->AddSlot().AutoHeight()
	[
		Row(LOCTEXT("AutoSet", "AUTO-SET"), LOCTEXT("AutoSetHint", "Measures this computer and picks the levels (takes a second)."),
			ActionButton(LOCTEXT("AutoSetButton", "AUTO-SET"), []()
			{
				if (UGameUserSettings* S = Settings())
				{
					S->RunHardwareBenchmark();
					S->ApplyHardwareBenchmarkResults();
				}
			}))
	];
	Rows->AddSlot().AutoHeight()
	[
		Row(LOCTEXT("Overall", "OVERALL"),
			TAttribute<FText>::CreateLambda([]()
			{
				const UGameUserSettings* S = Settings();
				return S && S->GetOverallScalabilityLevel() < 0
					? LOCTEXT("OverallCustom", "Custom: the groups below differ.")
					: LOCTEXT("OverallHint", "Sets every group below to one level.");
			}),
			Overall)
	];
	for (const FQualityGroup& Group : Groups)
	{
		const auto Get = Group.Get;
		const auto Set = Group.Set;
		Rows->AddSlot().AutoHeight()
		[
			Row(Group.Label, Group.Hint,
				Stepper([Get]()
					{
						const UGameUserSettings* S = Settings();
						return QualityName(S ? (S->*Get)() : -1);
					},
					[Get, Set](const int32 Delta)
					{
						if (UGameUserSettings* S = Settings())
						{
							(S->*Set)(FMath::Clamp((S->*Get)() + Delta, 0, 4));
							S->ApplyNonResolutionSettings();
							S->SaveSettings();
						}
					},
					[Get](const int32 Delta)
					{
						const UGameUserSettings* S = Settings();
						const int32 Next = S ? (S->*Get)() + Delta : -1;
						return Next >= 0 && Next <= 4;
					}, 170.0f))
		];
	}
	return SNew(SScrollBox) + SScrollBox::Slot()[Rows];
}

TSharedRef<SWidget> SAPSSettingsPage::BuildAudioTab()
{
	using namespace APSSettingsUI;
	return SNew(SScrollBox)
		+ SScrollBox::Slot()
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()
			[
				Heading(LOCTEXT("AudioKicker", "AUDIO"), LOCTEXT("AudioTitle", "SOUND"),
					LOCTEXT("AudioDescription", "MASTER scales every channel. Saved when you let go of a slider."))
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 18.0f, 0.0f, 0.0f)
			[
				SNew(SAPSAudioSettings).World(World.Get()).ShowTitle(false)
			]
		];
}

TSharedRef<SWidget> SAPSSettingsPage::BuildThemeCard(const EAPSUITheme Theme)
{
	using namespace APSSettingsUI;
	const FAPSUIThemePalette& Own = APSUITheme::PaletteOf(Theme);
	const bool bActive = APSUITheme::Current() == Theme;
	const TSharedRef<SButton> Button = SNew(SButton)
		.ButtonStyle(FAppStyle::Get(), "NoBorder")
		.ContentPadding(0.0f)
		.OnClicked_Lambda([Theme]() { APSUITheme::SetCurrent(Theme); return FReply::Handled(); });
	const TWeakPtr<SButton> Weak = Button;
	const auto Lit = [Weak]()
	{
		const TSharedPtr<SButton> Pinned = Weak.Pin();
		return Pinned.IsValid() && Pinned->IsHovered();
	};
	// Carbon's labels are white: its action colour is white too.
	const FLinearColor LabelAccent = Theme == EAPSUITheme::Carbon ? Own.Text : Own.Action;
	const FSlateFontInfo ButtonFont = Display(13);
	const TSharedRef<SWidget> Preview =
		SNew(SBorder).BorderImage(FAppStyle::GetBrush("WhiteBrush")).BorderBackgroundColor(Own.Frame).Padding(1.0f)
		[
			SNew(SBorder).BorderImage(FAppStyle::GetBrush("WhiteBrush")).BorderBackgroundColor(Own.Panel.CopyWithNewOpacity(1.0f))
			.Padding(FMargin(16.0f, 14.0f))
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight()
				[SNew(STextBlock).Text(LOCTEXT("PreviewKicker", "ACTIVE TARGET")).Font(Kicker()).ColorAndOpacity(LabelAccent)]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 4.0f, 0.0f, 0.0f)
				[SNew(STextBlock).Text(LOCTEXT("PreviewName", "KROLEON")).Font(Display(22)).ColorAndOpacity(Own.Text)]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 2.0f, 0.0f, 0.0f)
				[SNew(STextBlock).Text(LOCTEXT("PreviewLine", "Frozen planet · Khythion system")).Font(Body(13)).ColorAndOpacity(Own.TextSoft)]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 14.0f, 0.0f, 0.0f)
				[
					SNew(SBox).HeightOverride(38.0f)
					[
						SNew(SOverlay)
						+ SOverlay::Slot()
						[SNew(SAPSChamferedSurface).Brush(FAppStyle::GetBrush("WhiteBrush")).ChamferTop(true).ChamferBottom(true).Tint(Own.Action)]
						+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center)
						[APSChrome::CenteredLabel(LOCTEXT("PreviewContinue", "CONTINUE"), ButtonFont, FSlateColor(Own.OnAction))]
					]
				]
			]
		];
	Button->SetContent(
		SNew(SOverlay)
		+ SOverlay::Slot()
		[
			SNew(SAPSChamferedSurface).Brush(FAppStyle::GetBrush("WhiteBrush")).ChamferTop(true).ChamferBottom(true)
			.Tint_Lambda([Lit]() { return Lit() ? P().Raised : P().PanelSoft; })
		]
		+ SOverlay::Slot().Padding(FMargin(18.0f, 16.0f, 18.0f, 18.0f))
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center)
				[SNew(STextBlock).Text(APSUITheme::DisplayName(Theme)).Font(Display(18)).ColorAndOpacity_Lambda([]() { return FSlateColor(P().Text); })]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
				[
					SNew(SBorder).BorderImage(FAppStyle::GetBrush("WhiteBrush")).BorderBackgroundColor(P().Action)
					.Padding(FMargin(8.0f, 3.0f)).Visibility(bActive ? EVisibility::Visible : EVisibility::Collapsed)
					[SNew(STextBlock).Text(LOCTEXT("ActiveTag", "ACTIVE")).Font(Display(10)).ColorAndOpacity(P().OnAction)]
				]
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 14.0f, 0.0f, 0.0f)[Preview]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 14.0f, 0.0f, 0.0f)
			[
				SNew(STextBlock).Text(APSUITheme::Description(Theme)).Font(Body(14)).AutoWrapText(true)
				.ColorAndOpacity_Lambda([]() { return FSlateColor(P().TextSoft); })
			]
		]
		+ SOverlay::Slot()
		[
			SNew(SAPSChamferedFrame).Thickness(bActive ? 2.0f : 1.0f)
			.Color_Lambda([bActive, Lit]() { return bActive ? P().Action : Lit() ? P().Highlight : P().Frame; })
		]);
	return Button;
}

TSharedRef<SWidget> SAPSSettingsPage::BuildInterfaceTab()
{
	using namespace APSSettingsUI;
	const TSharedRef<SHorizontalBox> Cards = SNew(SHorizontalBox);
	for (int32 Index = 0; Index < static_cast<int32>(EAPSUITheme::Count); ++Index)
	{
		Cards->AddSlot().FillWidth(1.0f).Padding(Index == 0 ? 0.0f : 16.0f, 0.0f, 0.0f, 0.0f)
		[
			BuildThemeCard(static_cast<EAPSUITheme>(Index))
		];
	}
	const auto TypeChip = [](const FText& Label, const bool bLegacy)
	{
		return Chip(Label, [bLegacy]() { return APSUITheme::UsesLegacyFonts() == bLegacy; },
			[bLegacy]() { APSUITheme::SetLegacyFonts(bLegacy); }, 120.0f);
	};
	return SNew(SScrollBox)
		+ SScrollBox::Slot()
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()
			[
				Heading(LOCTEXT("InterfaceKicker", "INTERFACE"), LOCTEXT("InterfaceTitle", "THEME"),
					LOCTEXT("InterfaceDescription", "The whole interface follows it: menu, generation, terminal and HUD. Switches at once."))
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 22.0f, 0.0f, 8.0f)[Cards]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 12.0f, 0.0f, 0.0f)
			[
				Row(LOCTEXT("Type", "TYPE"),
					LOCTEXT("TypeHint", "NEW: Chakra Petch for titles and numbers, Exo 2 for text. CLASSIC: Orbitron and Roboto."),
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().AutoWidth()[TypeChip(LOCTEXT("TypeNew", "NEW"), false)]
					+ SHorizontalBox::Slot().AutoWidth().Padding(8.0f, 0.0f, 0.0f, 0.0f)[TypeChip(LOCTEXT("TypeClassic", "CLASSIC"), true)])
			]
		];
}

void SAPSSettingsPage::SelectTab(const EAPSSettingsTab Tab)
{
	CurrentTab = Tab;
	OnTabChanged.ExecuteIfBound(Tab);
}

int32 SAPSSettingsPage::ResolutionIndex() const
{
	const UGameUserSettings* S = APSSettingsUI::Settings();
	return S ? Resolutions.IndexOfByKey(S->GetScreenResolution()) : INDEX_NONE;
}

int32 SAPSSettingsPage::FrameRateIndex() const
{
	const UGameUserSettings* S = APSSettingsUI::Settings();
	const float Limit = S ? S->GetFrameRateLimit() : 0.0f;
	int32 Best = APSSettingsUI::FrameRateCount - 1;
	float BestGap = TNumericLimits<float>::Max();
	for (int32 Index = 0; Index < APSSettingsUI::FrameRateCount; ++Index)
	{
		const float Gap = FMath::Abs(APSSettingsUI::FrameRates[Index] - Limit);
		if (Gap < BestGap)
		{
			BestGap = Gap;
			Best = Index;
		}
	}
	return Best;
}

void SAPSSettingsPage::StepResolution(const int32 Delta)
{
	UGameUserSettings* S = APSSettingsUI::Settings();
	const int32 Index = ResolutionIndex() + Delta;
	if (S && Resolutions.IsValidIndex(Index))
	{
		S->SetScreenResolution(Resolutions[Index]);
		bVideoDirty = true;
	}
}

void SAPSSettingsPage::StepFrameRate(const int32 Delta)
{
	UGameUserSettings* S = APSSettingsUI::Settings();
	const int32 Index = FrameRateIndex() + Delta;
	if (S && Index >= 0 && Index < APSSettingsUI::FrameRateCount)
	{
		S->SetFrameRateLimit(APSSettingsUI::FrameRates[Index]);
		bVideoDirty = true;
	}
}

void SAPSSettingsPage::ApplyVideo()
{
	if (UGameUserSettings* S = APSSettingsUI::Settings())
	{
		S->ApplySettings(false);
		S->ConfirmVideoMode();
		S->SaveSettings();
		bVideoDirty = false;
		VideoStatus = LOCTEXT("VideoApplied", "Applied and saved.");
	}
}

void SAPSSettingsPage::RestoreDefaults()
{
	switch (CurrentTab)
	{
	case EAPSSettingsTab::Audio:
	{
		UAPSAudioSubsystem* Audio = World.IsValid() ? World->GetSubsystem<UAPSAudioSubsystem>() : nullptr;
		for (int32 Index = 0; Index < APSSettingsUI::ChannelCount; ++Index)
		{
			const EAPSAudioChannel Channel = static_cast<EAPSAudioChannel>(Index);
			if (Audio)
			{
				Audio->SetVolume(Channel, APSSettingsUI::DefaultVolumes[Index]);
			}
			else
			{
				GetMutableDefault<UAPSAudioPreferences>()->Set(Channel, APSSettingsUI::DefaultVolumes[Index]);
			}
		}
		GetMutableDefault<UAPSAudioPreferences>()->SaveConfig();
		break;
	}
	case EAPSSettingsTab::Interface:
		APSUITheme::SetLegacyFonts(false);
		APSUITheme::SetCurrent(EAPSUITheme::Obsidian);
		break;
	default:
		if (UGameUserSettings* S = APSSettingsUI::Settings())
		{
			S->SetToDefaults();
			S->ApplySettings(false);
			S->ConfirmVideoMode();
			bVideoDirty = false;
			VideoStatus = LOCTEXT("VideoRestored", "Video and graphics are back to their defaults.");
		}
		break;
	}
}

#undef LOCTEXT_NAMESPACE
