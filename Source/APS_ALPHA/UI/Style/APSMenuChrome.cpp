#include "APSMenuChrome.h"

#include "APSUIStyle.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

namespace APSChromePrivate
{
	// The values of SAPSMainMenuRoot's APSMenu palette and brushes.
	FLinearColor SRGB(uint8 R, uint8 G, uint8 B, uint8 A = 255)
	{
		return FLinearColor::FromSRGBColor(FColor(R, G, B, A));
	}

	const FSlateRoundedBoxBrush& CyanBadgeBrush()
	{
		static const FSlateRoundedBoxBrush Brush(FLinearColor(0.01f, 0.07f, 0.10f, 0.98f), 18.0f, APSChrome::Cyan(), 1.25f);
		return Brush;
	}

	const FSlateRoundedBoxBrush& InsetBrush()
	{
		static const FSlateRoundedBoxBrush Brush(FLinearColor(0.001f, 0.012f, 0.022f, 0.96f), 6.0f,
			FLinearColor(0.04f, 0.22f, 0.31f, 1.0f), 1.0f);
		return Brush;
	}

	const FSlateRoundedBoxBrush& MetricBrush()
	{
		static const FSlateRoundedBoxBrush Brush(FLinearColor(0.005f, 0.028f, 0.044f, 0.98f), 6.0f,
			FLinearColor(0.035f, 0.23f, 0.31f, 0.88f), 1.0f);
		return Brush;
	}
}

FLinearColor APSChrome::Panel() { return APSChromePrivate::SRGB(8, 32, 42, 242); }
FLinearColor APSChrome::PanelSoft() { return APSChromePrivate::SRGB(6, 19, 26, 232); }
FLinearColor APSChrome::Cyan() { return APSChromePrivate::SRGB(67, 214, 236); }
FLinearColor APSChrome::CyanDim() { return APSChromePrivate::SRGB(27, 83, 96, 178); }
FLinearColor APSChrome::Amber() { return APSChromePrivate::SRGB(242, 181, 29); }
FLinearColor APSChrome::White() { return APSChromePrivate::SRGB(234, 246, 248); }
FLinearColor APSChrome::Muted() { return APSChromePrivate::SRGB(138, 166, 174); }
FLinearColor APSChrome::Success() { return APSChromePrivate::SRGB(100, 214, 166); }
FLinearColor APSChrome::Scrim() { return APSChromePrivate::SRGB(2, 7, 11, 150); }

FSlateFontInfo APSChrome::Font(const FName Typeface, const int32 Size)
{
	return Typeface == TEXT("Bold")
		? FAPSUIStyle::DisplayFont(Typeface, Size)
		: FCoreStyle::GetDefaultFontStyle(Typeface, Size);
}

TSharedRef<SWidget> APSChrome::ChamferPanel(TSharedRef<SWidget> Content, const FMargin& Padding,
	const FLinearColor& Accent, const float Thickness)
{
	return SNew(SAPSChamferedOverlay)
		+ SOverlay::Slot()
		[
			SNew(SAPSChamferedSurface)
			.Brush(FAppStyle::GetBrush("WhiteBrush"))
			.Tint(Panel())
			.ChamferTop(true)
			.ChamferBottom(true)
		]
		+ SOverlay::Slot().Padding(Padding)
		[
			Content
		]
		+ SOverlay::Slot()
		[
			SNew(SAPSChamferedFrame).Color(Accent).Thickness(Thickness)
		];
}

TSharedRef<SWidget> APSChrome::Badge(const FText& Glyph, const FLinearColor& Accent, const float Size)
{
	return SNew(SBox).WidthOverride(Size).HeightOverride(Size)
	.HAlign(HAlign_Fill).VAlign(VAlign_Fill)
	[
		SNew(SBorder).BorderImage(&APSChromePrivate::CyanBadgeBrush()).BorderBackgroundColor(Accent).Padding(1.0f)
		[
			SNew(SBorder).BorderImage(&APSChromePrivate::InsetBrush()).Padding(0.0f)
			.HAlign(HAlign_Center).VAlign(VAlign_Center)
			[
				SNew(STextBlock).Text(Glyph).Justification(ETextJustify::Center)
				.Font(Font("Bold", FMath::RoundToInt(Size * 0.34f))).ColorAndOpacity(Accent)
			]
		]
	];
}

TSharedRef<SWidget> APSChrome::IconBadge(const EAPSChromeGlyph Glyph, const FLinearColor& Accent, const float Size)
{
	return SNew(SBox).WidthOverride(Size).HeightOverride(Size)
	.HAlign(HAlign_Fill).VAlign(VAlign_Fill)
	[
		SNew(SBorder).BorderImage(&APSChromePrivate::CyanBadgeBrush()).BorderBackgroundColor(Accent).Padding(1.0f)
		[
			SNew(SBorder).BorderImage(&APSChromePrivate::InsetBrush()).Padding(FMath::Max(4.0f, Size * 0.19f))
			.HAlign(HAlign_Fill).VAlign(VAlign_Fill)
			[
				SNew(SAPSVectorGlyph).Glyph(Glyph).Color(Accent).StrokeWidth(Size >= 40.0f ? 1.7f : 1.35f)
			]
		]
	];
}

TSharedRef<SWidget> APSChrome::IconSectionHeading(const EAPSChromeGlyph Glyph, const FText& Title, const FText& Subtitle)
{
	return SNew(SVerticalBox)
	+ SVerticalBox::Slot().AutoHeight()
	[
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
		[IconBadge(Glyph, Cyan(), 34.0f)]
		+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center).Padding(11.0f, 0.0f)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()
			[SNew(STextBlock).Text(Title).Font(Font("Bold", 14)).ColorAndOpacity(White())]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 2.0f, 0.0f, 0.0f)
			[
				SNew(STextBlock).Text(Subtitle).Font(Font("Regular", 9)).ColorAndOpacity(Muted())
				.Visibility(Subtitle.IsEmpty() ? EVisibility::Collapsed : EVisibility::Visible)
			]
		]
	]
	+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 9.0f, 0.0f, 0.0f)
	[
		SNew(SBox).HeightOverride(1.0f)
		[
			SNew(SBorder).BorderImage(FAppStyle::GetBrush("WhiteBrush")).BorderBackgroundColor(CyanDim())
		]
	];
}

TSharedRef<SWidget> APSChrome::KeyChip(const FText& Key, const FLinearColor& Accent)
{
	return SNew(SBorder).BorderImage(&APSChromePrivate::InsetBrush()).Padding(FMargin(7.0f, 3.0f))
	.HAlign(HAlign_Center).VAlign(VAlign_Center)
	[
		SNew(STextBlock).Text(Key).Font(Font("Bold", 9)).ColorAndOpacity(Accent)
	];
}

TSharedRef<SWidget> APSChrome::MetricTile(const TAttribute<FText>& Label, const TAttribute<FText>& Value,
	const FLinearColor& Accent)
{
	return SNew(SBorder).BorderImage(&APSChromePrivate::MetricBrush()).Padding(FMargin(8.0f, 7.0f))
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
		[
			SNew(STextBlock).Text(Value).Font(Font("Bold", 15)).ColorAndOpacity(Accent)
			.Justification(ETextJustify::Center)
		]
		+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0.0f, 2.0f, 0.0f, 0.0f)
		[
			SNew(STextBlock).Text(Label).Font(Font("Bold", 8)).ColorAndOpacity(Muted())
			.Justification(ETextJustify::Center)
		]
	];
}
