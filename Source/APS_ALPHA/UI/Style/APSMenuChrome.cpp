#include "APSMenuChrome.h"

#include "APSUIStyle.h"
#include "APSUITheme.h"
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

	// Rio 06.10: the brushes follow the interface theme. Widgets keep a pointer to them, so a theme switch
	// recolours what is on screen at the next paint.
	struct FThemedBrushes
	{
		FSlateRoundedBoxBrush CyanBadge{FLinearColor::Black};
		FSlateRoundedBoxBrush Inset{FLinearColor::Black};
		FSlateRoundedBoxBrush Metric{FLinearColor::Black};
		uint32 Revision{0};
	};

	FThemedBrushes& ThemedBrushes()
	{
		static FThemedBrushes Brushes;
		if (Brushes.Revision != APSUITheme::Revision())
		{
			const FAPSUIThemePalette& P = APSUITheme::Palette();
			Brushes.CyanBadge = FSlateRoundedBoxBrush(P.HighlightFill, 18.0f, P.Highlight, 1.25f);
			Brushes.Inset = FSlateRoundedBoxBrush(P.InsetFill, 6.0f, P.InsetFrame, 1.0f);
			const bool bClassic = APSUITheme::Current() == EAPSUITheme::Classic;
			Brushes.Metric = FSlateRoundedBoxBrush(bClassic ? FLinearColor(0.005f, 0.028f, 0.044f, 0.98f)
					: FMath::Lerp(P.InsetFill, P.Panel, 0.35f).CopyWithNewOpacity(0.98f),
				6.0f, bClassic ? FLinearColor(0.035f, 0.23f, 0.31f, 0.88f) : APSUITheme::Fade(P.InsetFrame, 0.88f), 1.0f);
			Brushes.Revision = APSUITheme::Revision();
		}
		return Brushes;
	}

	const FSlateRoundedBoxBrush& CyanBadgeBrush() { return ThemedBrushes().CyanBadge; }
	const FSlateRoundedBoxBrush& InsetBrush() { return ThemedBrushes().Inset; }
	const FSlateRoundedBoxBrush& MetricBrush() { return ThemedBrushes().Metric; }

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
	// Rio 06.10 type, read from Content/Slate/Fonts (hhea = OS/2 typo, the same in every weight).
	constexpr FFaceMetrics ChakraPetchMetrics{1000.0f, 992.0f, -308.0f, 0.0f, 700.0f, 498.0f};
	constexpr FFaceMetrics Exo2Metrics{1000.0f, 999.0f, -201.0f, 0.0f, 690.0f, 490.0f};

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

	const FFaceMetrics& MetricsOf(const FSlateFontInfo& Font)
	{
		if (APSUITheme::IsDisplayFont(Font))
		{
			return Font.CompositeFont.IsValid() ? ChakraPetchMetrics : OrbitronMetrics;
		}
		return APSUITheme::IsBodyFont(Font) ? Exo2Metrics : RobotoMetrics;
	}
}

// Rio 06.10: the palette of the active interface theme (APSUITheme); Classic keeps the original values.
FLinearColor APSChrome::Panel() { return APSUITheme::Palette().Panel; }
FLinearColor APSChrome::PanelSoft() { return APSUITheme::Palette().PanelSoft; }
FLinearColor APSChrome::Cyan() { return APSUITheme::Palette().Highlight; }
FLinearColor APSChrome::CyanDim() { return APSUITheme::Palette().Frame; }
FLinearColor APSChrome::Amber() { return APSUITheme::Palette().Action; }
FLinearColor APSChrome::White() { return APSUITheme::Palette().Text; }
// Lighter than the original 138/166/174: secondary lines must still read on the dark panels (Rio 02.10).
FLinearColor APSChrome::Muted() { return APSUITheme::Palette().TextSoft; }
FLinearColor APSChrome::Success() { return APSUITheme::Palette().Success; }
FLinearColor APSChrome::Scrim() { return APSUITheme::Palette().Scrim; }
FLinearColor APSChrome::AmberBright()
{
	return APSUITheme::Current() == EAPSUITheme::Classic ? FLinearColor(1.0f, 0.83f, 0.38f, 1.0f)
		: APSUITheme::Palette().ActionBright;
}
FLinearColor APSChrome::OnAmber()
{
	return APSUITheme::Current() == EAPSUITheme::Classic ? FLinearColor(0.02f, 0.05f, 0.07f, 1.0f)
		: APSUITheme::Palette().OnAction;
}
const FSlateBrush* APSChrome::MetricTileBrush() { return &APSChromePrivate::MetricBrush(); }

FSlateFontInfo APSChrome::Font(const FName Typeface, const int32 Size)
{
	// Rio 02.10 ("hard to read, especially the small text"): the display face stays for headings and large values;
	// below 10 pt its wide letters blur, so small bold text uses the readable face one size up, and regular text
	// is never below 11 pt.
	// Rio 06.10: Chakra Petch stays readable small, so bold text keeps the display face down to 9 pt.
	if (Typeface == TEXT("Bold"))
	{
		if (!APSUITheme::UsesLegacyFonts())
		{
			return APSUITheme::DisplayFont(Typeface, FMath::Max(Size, 9));
		}
		return Size >= 10 ? APSUITheme::DisplayFont(Typeface, Size) : APSUITheme::BodyFont(Typeface, Size + 1);
	}
	return APSUITheme::BodyFont(Typeface, FMath::Max(Size, 11));
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
			SNew(STextBlock).Text(Label).Font(APSUITheme::BodyFont("Bold", 10)).ColorAndOpacity(Muted())
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 4.0f, 0.0f, 0.0f)
		[
			SNew(STextBlock).Text(Value).Font(Font("Bold", 16)).ColorAndOpacity(Accent).AutoWrapText(true)
		]
	];
}
