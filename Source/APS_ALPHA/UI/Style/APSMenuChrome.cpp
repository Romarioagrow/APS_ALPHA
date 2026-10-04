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

	/** Vertical metrics in font units, read 03.10 from the hhea and OS/2 tables of the two faces the UI uses: the
	 * Orbitron TTF inside UI/Fonts/Orbitron_Bold (Medium has the same values) and the engine's Roboto. Neither sets
	 * USE_TYPO_METRICS and both font assets use the default Metrics layout, so Slate lays them out on these hhea values.
	 * XHeight is also the middle line of the symbols: Orbitron's < > + - v sit on 0.29 em, not on the capitals' 0.36. */
	struct FFaceMetrics
	{
		float UnitsPerEm;
		float Ascender;
		float Descender;
		float LineGap;
		float CapHeight;
		float XHeight;
	};
	constexpr FFaceMetrics OrbitronMetrics{1000.0f, 750.0f, -250.0f, 0.0f, 720.0f, 580.0f};
	constexpr FFaceMetrics RobotoMetrics{2048.0f, 1900.0f, -500.0f, 0.0f, 1456.0f, 1082.0f};

	/** How far below the line box's centre a glyph band of this height (from the baseline up) has its middle, in em:
	 * the box spans Ascender + LineGap over the baseline and -Descender under it. */
	constexpr float BandOffsetEm(const FFaceMetrics& Metrics, const float BandHeight)
	{
		return (BandHeight - (Metrics.Ascender + Metrics.LineGap + Metrics.Descender)) * 0.5f / Metrics.UnitsPerEm;
	}

	/** Slate sizes fonts in points at 96 DPI: one em is Size * 96 / 72 Slate units. */
	float EmUnits(const FSlateFontInfo& Font)
	{
		return static_cast<float>(Font.Size) * 96.0f / 72.0f;
	}

	/** The display face is the Orbitron UFont of the menu and the in-game chrome; everything else is the engine font. */
	bool IsDisplayFace(const FSlateFontInfo& Font)
	{
		const UObject* FontObject = Font.FontObject;
		return FontObject && FontObject->GetName().Contains(TEXT("Orbitron"));
	}

	const FFaceMetrics& MetricsOf(const FSlateFontInfo& Font)
	{
		return IsDisplayFace(Font) ? OrbitronMetrics : RobotoMetrics;
	}
}

FLinearColor APSChrome::Panel() { return APSChromePrivate::SRGB(8, 32, 42, 242); }
FLinearColor APSChrome::PanelSoft() { return APSChromePrivate::SRGB(6, 19, 26, 232); }
FLinearColor APSChrome::Cyan() { return APSChromePrivate::SRGB(67, 214, 236); }
FLinearColor APSChrome::CyanDim() { return APSChromePrivate::SRGB(27, 83, 96, 178); }
FLinearColor APSChrome::Amber() { return APSChromePrivate::SRGB(242, 181, 29); }
FLinearColor APSChrome::White() { return APSChromePrivate::SRGB(234, 246, 248); }
// Lighter than the original 138/166/174: secondary lines must still read on the dark panels (Rio 02.10).
FLinearColor APSChrome::Muted() { return APSChromePrivate::SRGB(170, 194, 202); }
FLinearColor APSChrome::Success() { return APSChromePrivate::SRGB(100, 214, 166); }
FLinearColor APSChrome::Scrim() { return APSChromePrivate::SRGB(2, 7, 11, 150); }

FSlateFontInfo APSChrome::Font(const FName Typeface, const int32 Size)
{
	// Rio 02.10 ("hard to read, especially the small text"): the display face stays for headings and large values;
	// below 10 pt its wide letters blur, so small bold text uses the readable face one size up, and regular text
	// is never below 11 pt.
	if (Typeface == TEXT("Bold"))
	{
		return Size >= 10 ? FAPSUIStyle::DisplayFont(Typeface, Size) : FCoreStyle::GetDefaultFontStyle(Typeface, Size + 1);
	}
	return FCoreStyle::GetDefaultFontStyle(Typeface, FMath::Max(Size, 11));
}

float APSChrome::CapsCenterOffset(const FSlateFontInfo& Font)
{
	using namespace APSChromePrivate;
	const FFaceMetrics& Metrics = MetricsOf(Font);
	return BandOffsetEm(Metrics, Metrics.CapHeight) * EmUnits(Font);
}

TOptional<FSlateRenderTransform> APSChrome::CapsCenterShift(const FSlateFontInfo& Font)
{
	return FSlateRenderTransform(FVector2f(0.0f, CapsCenterOffset(Font)));
}

float APSChrome::SymbolCenterOffset(const FSlateFontInfo& Font)
{
	using namespace APSChromePrivate;
	const FFaceMetrics& Metrics = MetricsOf(Font);
	return BandOffsetEm(Metrics, Metrics.XHeight) * EmUnits(Font);
}

TOptional<FSlateRenderTransform> APSChrome::SymbolCenterShift(const FSlateFontInfo& Font)
{
	return FSlateRenderTransform(FVector2f(0.0f, SymbolCenterOffset(Font)));
}

TSharedRef<STextBlock> APSChrome::CenteredLabel(const TAttribute<FText>& Text, const FSlateFontInfo& Font,
	const TAttribute<FSlateColor>& Color)
{
	return SNew(STextBlock).Text(Text).Font(Font).ColorAndOpacity(Color)
		.Justification(ETextJustify::Center).RenderTransform(CapsCenterShift(Font));
}

FMargin APSChrome::ButtonPadding(const FSlateFontInfo& Font, const float Horizontal)
{
	return FMargin(FMath::Max(Horizontal, 14.0f), FMath::RoundToFloat(APSChromePrivate::EmUnits(Font) * 0.45f) + 5.0f);
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
	const FSlateFontInfo GlyphFont = Font("Bold", FMath::RoundToInt(Size * 0.34f));
	return SNew(SBox).WidthOverride(Size).HeightOverride(Size)
	.HAlign(HAlign_Fill).VAlign(VAlign_Fill)
	[
		SNew(SBorder).BorderImage(&APSChromePrivate::CyanBadgeBrush()).BorderBackgroundColor(Accent).Padding(1.0f)
		[
			SNew(SBorder).BorderImage(&APSChromePrivate::InsetBrush()).Padding(0.0f)
			.HAlign(HAlign_Center).VAlign(VAlign_Center)
			[
				CenteredLabel(Glyph, GlyphFont, Accent)
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
				SNew(STextBlock).Text(Subtitle).Font(Font("Regular", 11)).ColorAndOpacity(Muted())
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
	// Rio 03.10: the key centred both ways in its chip, by its capitals.
	return SNew(SBorder).BorderImage(&APSChromePrivate::InsetBrush()).Padding(FMargin(8.0f, 3.0f))
	.HAlign(HAlign_Center).VAlign(VAlign_Center)
	[
		CenteredLabel(Key, Font("Bold", 9), Accent)
	];
}

TSharedRef<SWidget> APSChrome::MetricTile(const TAttribute<FText>& Label, const TAttribute<FText>& Value,
	const FLinearColor& Accent)
{
	// Rio 02.10: the label reads first, small but clear, then the value large (like the generation menu's LIVE MODEL).
	return SNew(SBorder).BorderImage(&APSChromePrivate::MetricBrush()).Padding(FMargin(12.0f, 9.0f, 12.0f, 10.0f))
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight()
		[
			SNew(STextBlock).Text(Label).Font(FCoreStyle::GetDefaultFontStyle("Bold", 10)).ColorAndOpacity(Muted())
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 4.0f, 0.0f, 0.0f)
		[
			SNew(STextBlock).Text(Value).Font(Font("Bold", 16)).ColorAndOpacity(Accent).AutoWrapText(true)
		]
	];
}
