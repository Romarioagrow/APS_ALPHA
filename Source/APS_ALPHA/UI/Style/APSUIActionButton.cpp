#include "APSUIActionButton.h"

#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

namespace APSUIActionButtonPrivate
{
	FButtonStyle MakeDangerStyle(const FAPSUIColorPalette& Palette)
	{
		const FAPSUILayoutMetrics& Layout = FAPSUIStyle::Metrics();
		const FLinearColor Normal(Palette.Danger.R * 0.18f, Palette.Danger.G * 0.08f,
			Palette.Danger.B * 0.10f, 0.96f);
		const FLinearColor Hovered(Palette.Danger.R * 0.32f, Palette.Danger.G * 0.12f,
			Palette.Danger.B * 0.14f, 1.0f);
		return FButtonStyle()
			.SetNormal(FSlateRoundedBoxBrush(Normal, Layout.ControlRadius, Palette.Danger, Layout.QuietBorder))
			.SetHovered(FSlateRoundedBoxBrush(Hovered, Layout.ControlRadius, Palette.Danger, Layout.FocusBorder))
			.SetPressed(FSlateRoundedBoxBrush(Hovered, Layout.ControlRadius, Palette.TextPrimary, Layout.FocusBorder))
			.SetDisabled(FSlateRoundedBoxBrush(Normal.CopyWithNewOpacity(0.45f), Layout.ControlRadius,
				Palette.Danger.CopyWithNewOpacity(0.45f), Layout.QuietBorder));
	}
}

void SAPSUIActionButton::Construct(const FArguments& InArgs)
{
	const FAPSUIColorPalette Palette = FAPSUIStyle::GetPalette(InArgs._Profile);
	const FAPSUILayoutMetrics& Layout = FAPSUIStyle::Metrics();
	Description = InArgs._Description;
	Shortcut = InArgs._Shortcut;

	switch (InArgs._Tone)
	{
	case EAPSUIActionTone::Primary:
		ButtonStyle = MakeShared<FButtonStyle>(FAPSUIStyle::MakePrimaryButtonStyle(Palette));
		break;
	case EAPSUIActionTone::Danger:
		ButtonStyle = MakeShared<FButtonStyle>(APSUIActionButtonPrivate::MakeDangerStyle(Palette));
		break;
	case EAPSUIActionTone::Secondary:
	default:
		ButtonStyle = MakeShared<FButtonStyle>(FAPSUIStyle::MakeSecondaryButtonStyle(Palette));
		break;
	}

	ShortcutBrush = MakeShared<FSlateRoundedBoxBrush>(
		Palette.Control, Layout.ControlRadius, Palette.BorderQuiet, Layout.QuietBorder);

	ChildSlot
	[
		SNew(SBox).MinDesiredHeight(Layout.ControlMinHeight)
		[
			SNew(SButton)
			.ButtonStyle(ButtonStyle.Get())
			.ContentPadding(FMargin(Layout.Space4, Layout.Space2))
			.IsEnabled(InArgs._IsEnabled)
			.OnClicked(InArgs._OnClicked)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight()
					[
						SNew(STextBlock).Text(InArgs._Label)
						.Font(FAPSUIStyle::BodyFont(TEXT("Bold"), 11)).ColorAndOpacity(Palette.TextPrimary)
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 2.0f, 0.0f, 0.0f)
					[
						SNew(STextBlock).Text(Description).AutoWrapText(true)
						.Visibility_Lambda([this]()
						{
							return Description.Get().IsEmpty() ? EVisibility::Collapsed : EVisibility::Visible;
						})
						.Font(FAPSUIStyle::BodyFont(TEXT("Regular"), 9)).ColorAndOpacity(Palette.TextSecondary)
					]
				]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(Layout.Space3, 0.0f, 0.0f, 0.0f)
				[
					SNew(SBox).MinDesiredWidth(Layout.IconButtonTarget).HeightOverride(28.0f)
					.Visibility_Lambda([this]()
					{
						return Shortcut.Get().IsEmpty() ? EVisibility::Collapsed : EVisibility::Visible;
					})
					[
						SNew(SBorder).BorderImage(ShortcutBrush.Get()).Padding(FMargin(Layout.Space2, Layout.Space1))
						[
							SNew(STextBlock).Text(Shortcut).Justification(ETextJustify::Center)
							.Font(FAPSUIStyle::DisplayFont(TEXT("Bold"), 8)).ColorAndOpacity(Palette.FocusCyan)
						]
					]
				]
			]
		]
	];
}
