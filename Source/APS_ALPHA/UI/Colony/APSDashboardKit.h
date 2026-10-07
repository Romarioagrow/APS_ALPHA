#pragma once

#include "CoreMinimal.h"
#include "APS_ALPHA/UI/Style/APSMenuChrome.h"
#include "Framework/SlateDelegates.h"
#include "Widgets/SLeafWidget.h"

class AStar;
class APlanetaryBody;

/**
 * The colony terminal's dashboards (Rio 04.10: OVERVIEW, then PILOT "the same way"): dark chamfered cards with an icon
 * heading and a link to the tab that goes deeper, headline numbers with icon badges, segment gauges, and a small drawing
 * of a star with a world on its orbit. Built from the menu chrome (APSMenuChrome), so they look like the rest.
 */
namespace APSDashboard
{
	/** The cards' fill: darker than the terminal's panel, so the cards stand off it. */
	FLinearColor CardFill();
	/** Captions over values, in the readable face (the display face blurs this small). */
	FSlateFontInfo CaptionFont();
	/** The star's own light, kept bright enough for a disc on a dark card (as the strategic map draws it). */
	FLinearColor StarColour(const AStar* Star);
	/** The world's colour on the colony map (SAPSCivilizationMap), so a world reads the same on both. */
	FLinearColor BodyColour(const APlanetaryBody* Body);

	TSharedRef<SWidget> CardSurface(const TSharedRef<SWidget>& Content, const FMargin& Padding);
	/** The card's way deeper: a chamfered button whose frame lights on hover. */
	TSharedRef<SWidget> LinkButton(const FText& Label, FOnClicked OnClicked);
	/** A card: the heading with its icon, the body, and the link at the bottom. */
	TSharedRef<SWidget> Card(EAPSChromeGlyph Glyph, const FText& Title, const FText& Subtitle,
		const TSharedRef<SWidget>& Body, const TSharedRef<SWidget>& Link);
	TSharedRef<SWidget> Caption(const TAttribute<FText>& Text);
	/** A big number with its unit after it, both on the number's baseline. */
	TSharedRef<SWidget> BigValue(const TAttribute<FText>& Value, const TAttribute<FText>& Unit, const TAttribute<FSlateColor>& Accent,
		int32 Size);
	/** One headline: an icon badge, the caption, the number large, and an optional line or gauge under it. */
	TSharedRef<SWidget> Headline(EAPSChromeGlyph Glyph, const FLinearColor& Accent, const FText& Label,
		const TAttribute<FText>& Value, const TAttribute<FText>& Unit, const TSharedRef<SWidget>& Under, const FText& Tooltip);
	TSharedRef<SWidget> VerticalRule(float Height);
	/** A small square of colour (legends, journal categories). */
	TSharedRef<SWidget> Swatch(const TAttribute<FSlateColor>& Colour, float Size = 10.0f);
	/** A swatch with a caption and a live count, for the legends under the bars. */
	TSharedRef<SWidget> LegendItem(const FLinearColor& Colour, const FText& Label, const TAttribute<FText>& Count);
}

/**
 * A row of segments in the caller's colours, one per unit, left to right; a dim track when there are none. Segments are
 * at most MaxSegment wide, so a few units read as countable pips rather than one long bar.
 */
class SAPSDashboardSegments final : public SLeafWidget
{
public:
	SLATE_BEGIN_ARGS(SAPSDashboardSegments) : _Height(10.0f), _MaxSegment(1000.0f) {}
		SLATE_ARGUMENT(TFunction<void(TArray<FLinearColor>&)>, Colours)
		SLATE_ARGUMENT(float, Height)
		SLATE_ARGUMENT(float, MaxSegment)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	virtual FVector2D ComputeDesiredSize(float) const override { return FVector2D(40.0f, Height); }
	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& Geometry, const FSlateRect& CullingRect,
		FSlateWindowElementList& Elements, int32 LayerId, const FWidgetStyle& Style, bool bParentEnabled) const override;

private:
	TFunction<void(TArray<FLinearColor>&)> Colours;
	float Height{10.0f};
	float MaxSegment{1000.0f};
};

/** A star in its light's colour with two thin rings, a world on its orbit, and the world's moons around it (up to six). */
class SAPSDashboardOrbit final : public SLeafWidget
{
public:
	SLATE_BEGIN_ARGS(SAPSDashboardOrbit)
		: _StarColour(FLinearColor::White), _BodyColour(FLinearColor::White), _Moons(0), _HasBody(true) {}
		SLATE_ATTRIBUTE(FLinearColor, StarColour)
		SLATE_ATTRIBUTE(FLinearColor, BodyColour)
		SLATE_ATTRIBUTE(int32, Moons)
		SLATE_ATTRIBUTE(bool, HasBody)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	virtual FVector2D ComputeDesiredSize(float) const override { return FVector2D(170.0f, 150.0f); }
	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& Geometry, const FSlateRect& CullingRect,
		FSlateWindowElementList& Elements, int32 LayerId, const FWidgetStyle& Style, bool bParentEnabled) const override;

private:
	TAttribute<FLinearColor> StarColour;
	TAttribute<FLinearColor> BodyColour;
	TAttribute<int32> Moons;
	TAttribute<bool> HasBody;
};
