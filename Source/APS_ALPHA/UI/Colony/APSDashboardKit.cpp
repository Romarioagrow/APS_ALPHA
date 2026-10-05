#include "APSDashboardKit.h"

#include "APS_ALPHA/Actors/Astro/PlanetaryBody.h"
#include "APS_ALPHA/Actors/Astro/Star.h"
#include "APS_ALPHA/Core/Enums/PlanetType.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Styling/AppStyle.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SNullWidget.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "APSDashboardKit"

namespace APSDashboardPrivate
{
	const FSlateBrush* Disc()
	{
		// The brush's default half-height rounding makes a circle of any square box.
		static const FSlateRoundedBoxBrush Brush(FLinearColor::White);
		return &Brush;
	}

	void Ellipse(FSlateWindowElementList& Elements, const int32 Layer, const FGeometry& Geometry, const FVector2D& Centre,
		const float RadiusX, const float RadiusY, const FLinearColor& Colour, const float Thickness)
	{
		constexpr int32 Steps = 72;
		TArray<FVector2D> Points;
		Points.Reserve(Steps + 1);
		for (int32 Index = 0; Index <= Steps; ++Index)
		{
			const float Angle = UE_TWO_PI * static_cast<float>(Index) / Steps;
			Points.Add(Centre + FVector2D(FMath::Cos(Angle) * RadiusX, FMath::Sin(Angle) * RadiusY));
		}
		FSlateDrawElement::MakeLines(Elements, Layer, Geometry.ToPaintGeometry(), Points, ESlateDrawEffect::None, Colour,
			true, Thickness);
	}
}

FLinearColor APSDashboard::CardFill()
{
	return FLinearColor(0.004f, 0.02f, 0.032f, 0.94f);
}

FSlateFontInfo APSDashboard::CaptionFont()
{
	return FCoreStyle::GetDefaultFontStyle("Bold", 10);
}

FLinearColor APSDashboard::StarColour(const AStar* Star)
{
	if (!Star || Star->SurfaceTemperature <= 0)
	{
		return APSChrome::Amber();
	}
	const FLinearColor Light = FLinearColor::MakeFromColorTemperature(
		FMath::Clamp(static_cast<float>(Star->SurfaceTemperature), 2000.0f, 15000.0f));
	const float Peak = FMath::Max3(Light.R, Light.G, Light.B);
	return Peak > 0.0f ? FLinearColor(Light.R / Peak, Light.G / Peak, Light.B / Peak, 1.0f) : APSChrome::Amber();
}

FLinearColor APSDashboard::BodyColour(const APlanetaryBody* Body)
{
	switch (Body ? Body->PlanetType : EPlanetType::Unknown)
	{
	case EPlanetType::Ice:
	case EPlanetType::Frozen:
	case EPlanetType::Nordic:
	case EPlanetType::Tundra:
	case EPlanetType::IceGiant:
	case EPlanetType::Ammonia:
		return FLinearColor(0.72f, 0.88f, 1.0f);
	case EPlanetType::Ocean:
	case EPlanetType::Water:
	case EPlanetType::Archipelago:
		return FLinearColor(0.22f, 0.6f, 1.0f);
	case EPlanetType::Terrestrial:
	case EPlanetType::Forest:
	case EPlanetType::SuperEarth:
	case EPlanetType::Pangea:
	case EPlanetType::Savanna:
	case EPlanetType::Oasis:
		return FLinearColor(0.38f, 0.86f, 0.5f);
	case EPlanetType::Desert:
	case EPlanetType::Sand:
		return FLinearColor(0.92f, 0.76f, 0.46f);
	case EPlanetType::Volcanic:
	case EPlanetType::Lava:
	case EPlanetType::Melted:
	case EPlanetType::Sulfur:
		return FLinearColor(1.0f, 0.42f, 0.22f);
	case EPlanetType::GasGiant:
	case EPlanetType::HotGiant:
		return FLinearColor(0.95f, 0.8f, 0.6f);
	default:
		return FLinearColor(0.66f, 0.74f, 0.78f);
	}
}

TSharedRef<SWidget> APSDashboard::CardSurface(const TSharedRef<SWidget>& Content, const FMargin& Padding)
{
	// The surface is a real octagon, so no clip zone is needed (a nested SAPSChamferedOverlay would push stencil clips).
	return SNew(SOverlay)
		+ SOverlay::Slot()
		[
			SNew(SAPSChamferedSurface).Brush(FAppStyle::GetBrush("WhiteBrush")).Tint(CardFill())
			.ChamferTop(true).ChamferBottom(true)
		]
		+ SOverlay::Slot().Padding(Padding)
		[
			Content
		]
		+ SOverlay::Slot()
		[
			SNew(SAPSChamferedFrame).Color(APSChrome::CyanDim()).Thickness(1.0f)
		];
}

TSharedRef<SWidget> APSDashboard::LinkButton(const FText& Label, FOnClicked OnClicked)
{
	using namespace APSChrome;
	const TSharedRef<SButton> Button = SNew(SButton)
		.ButtonStyle(FAppStyle::Get(), "NoBorder")
		.ContentPadding(0.0f)
		.OnClicked(MoveTemp(OnClicked));
	const TWeakPtr<SButton> WeakButton = Button;
	const auto Hovered = [WeakButton]()
	{
		const TSharedPtr<SButton> Pinned = WeakButton.Pin();
		return Pinned.IsValid() && Pinned->IsHovered();
	};
	const FSlateFontInfo LabelFont = Font("Bold", 10);
	Button->SetContent(
		SNew(SOverlay)
		+ SOverlay::Slot()
		[
			SNew(SAPSChamferedSurface).Brush(FAppStyle::GetBrush("WhiteBrush"))
			.Tint_Lambda([Hovered]() { return Hovered() ? FLinearColor(0.02f, 0.13f, 0.17f, 0.97f) : Panel(); })
			.ChamferTop(true).ChamferBottom(true)
		]
		+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center).Padding(ButtonPadding(LabelFont, 16.0f))
		[
			CenteredLabel(FText::Format(LOCTEXT("LinkLabel", "{0}  >"), Label), LabelFont, Cyan())
		]
		+ SOverlay::Slot()
		[
			SNew(SAPSChamferedFrame).Thickness(1.0f)
			.Color_Lambda([Hovered]() { return Hovered() ? Cyan() : CyanDim(); })
		]);
	return Button;
}

TSharedRef<SWidget> APSDashboard::Card(const EAPSChromeGlyph Glyph, const FText& Title, const FText& Subtitle,
	const TSharedRef<SWidget>& Body, const TSharedRef<SWidget>& Link)
{
	return CardSurface(
		SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight()
		[
			APSChrome::IconSectionHeading(Glyph, Title, Subtitle)
		]
		+ SVerticalBox::Slot().FillHeight(1.0f).Padding(0.0f, 16.0f, 0.0f, 0.0f)
		[
			Body
		]
		+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Left).Padding(0.0f, 18.0f, 0.0f, 0.0f)
		[
			Link
		],
		FMargin(20.0f, 18.0f, 20.0f, 18.0f));
}

TSharedRef<SWidget> APSDashboard::Caption(const TAttribute<FText>& Text)
{
	return SNew(STextBlock).Text(Text).Font(CaptionFont()).ColorAndOpacity(APSChrome::Muted());
}

TSharedRef<SWidget> APSDashboard::BigValue(const TAttribute<FText>& Value, const TAttribute<FText>& Unit,
	const TAttribute<FSlateColor>& Accent, const int32 Size)
{
	return SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Bottom)
		[
			SNew(STextBlock).Text(Value).Font(APSChrome::Font("Bold", Size)).ColorAndOpacity(Accent)
		]
		// The unit in the readable face: the display font is for digits and capitals.
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Bottom).Padding(7.0f, 0.0f, 0.0f, Size >= 22 ? 4.0f : 2.0f)
		[
			SNew(STextBlock).Text(Unit).Font(FCoreStyle::GetDefaultFontStyle("Bold", 13)).ColorAndOpacity(APSChrome::Muted())
		];
}

TSharedRef<SWidget> APSDashboard::Headline(const EAPSChromeGlyph Glyph, const FLinearColor& Accent, const FText& Label,
	const TAttribute<FText>& Value, const TAttribute<FText>& Unit, const TSharedRef<SWidget>& Under, const FText& Tooltip)
{
	const TSharedRef<SWidget> Content =
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
		[
			APSChrome::IconBadge(Glyph, Accent, 46.0f)
		]
		+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center).Padding(14.0f, 0.0f, 0.0f, 0.0f)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()
			[
				Caption(Label)
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 3.0f, 0.0f, 0.0f)
			[
				BigValue(Value, Unit, Accent, 22)
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, Under == SNullWidget::NullWidget ? 0.0f : 8.0f, 0.0f, 0.0f)
			[
				Under
			]
		];
	Content->SetToolTipText(Tooltip);
	return CardSurface(Content, FMargin(16.0f, 14.0f, 18.0f, 14.0f));
}

TSharedRef<SWidget> APSDashboard::VerticalRule(const float Height)
{
	return SNew(SBox).WidthOverride(1.0f).HeightOverride(Height)
		[
			SNew(SBorder).BorderImage(FAppStyle::GetBrush("WhiteBrush")).BorderBackgroundColor(APSChrome::CyanDim())
		];
}

TSharedRef<SWidget> APSDashboard::Swatch(const TAttribute<FSlateColor>& Colour, const float Size)
{
	return SNew(SBox).WidthOverride(Size).HeightOverride(Size)
		[
			SNew(SBorder).BorderImage(FAppStyle::GetBrush("WhiteBrush")).BorderBackgroundColor(Colour)
		];
}

TSharedRef<SWidget> APSDashboard::LegendItem(const FLinearColor& Colour, const FText& Label, const TAttribute<FText>& Count)
{
	return SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
		[
			Swatch(Colour)
		]
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(7.0f, 0.0f, 0.0f, 0.0f)
		[
			Caption(Label)
		]
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(6.0f, 0.0f, 0.0f, 0.0f)
		[
			SNew(STextBlock).Text(Count).Font(FCoreStyle::GetDefaultFontStyle("Bold", 12)).ColorAndOpacity(APSChrome::White())
		];
}

void SAPSDashboardSegments::Construct(const FArguments& InArgs)
{
	Colours = InArgs._Colours;
	Height = InArgs._Height;
	MaxSegment = InArgs._MaxSegment;
}

int32 SAPSDashboardSegments::OnPaint(const FPaintArgs&, const FGeometry& Geometry, const FSlateRect&,
	FSlateWindowElementList& Elements, const int32 LayerId, const FWidgetStyle& Style, bool) const
{
	TArray<FLinearColor> Segments;
	if (Colours)
	{
		Colours(Segments);
	}
	const FVector2D Size = Geometry.GetLocalSize();
	const float Top = FMath::Max(0.0f, (static_cast<float>(Size.Y) - Height) * 0.5f);
	const FSlateBrush* Solid = FAppStyle::GetBrush("WhiteBrush");
	const FLinearColor Tint = Style.GetColorAndOpacityTint();
	const auto Box = [&](const float Left, const float Width, const FLinearColor& Colour)
	{
		FSlateDrawElement::MakeBox(Elements, LayerId, Geometry.ToPaintGeometry(FVector2f(Width, Height),
			FSlateLayoutTransform(FVector2f(Left, Top))), Solid, ESlateDrawEffect::None, Colour * Tint);
	};
	const int32 Count = Segments.Num();
	if (Count == 0)
	{
		Box(0.0f, static_cast<float>(Size.X), APSChrome::CyanDim().CopyWithNewOpacity(0.35f));
		return LayerId;
	}
	float Gap = Count > 30 ? 1.0f : Count > 14 ? 2.0f : 3.0f;
	float Width = (static_cast<float>(Size.X) - Gap * (Count - 1)) / Count;
	if (Width < 1.5f)
	{
		// Too many to tell apart: one continuous run per colour.
		Gap = 0.0f;
		Width = static_cast<float>(Size.X) / Count;
	}
	Width = FMath::Min(Width, MaxSegment);
	for (int32 Segment = 0; Segment < Count; ++Segment)
	{
		Box(Segment * (Width + Gap), Width, Segments[Segment]);
	}
	return LayerId;
}

void SAPSDashboardOrbit::Construct(const FArguments& InArgs)
{
	StarColour = InArgs._StarColour;
	BodyColour = InArgs._BodyColour;
	Moons = InArgs._Moons;
	HasBody = InArgs._HasBody;
	SetVisibility(EVisibility::HitTestInvisible);
}

int32 SAPSDashboardOrbit::OnPaint(const FPaintArgs&, const FGeometry& Geometry, const FSlateRect&,
	FSlateWindowElementList& Elements, const int32 LayerId, const FWidgetStyle& Style, bool) const
{
	using namespace APSDashboardPrivate;
	const FVector2D Size = Geometry.GetLocalSize();
	const float Short = static_cast<float>(FMath::Min(Size.X, Size.Y));
	if (Short < 40.0f)
	{
		return LayerId;
	}
	// Line widths are screen pixels: scaled with the UI, so the drawing keeps its weight at any resolution.
	const float Line = Geometry.GetAccumulatedLayoutTransform().GetScale();
	const FLinearColor Tint = Style.GetColorAndOpacityTint();
	const auto DiscAt = [&](const int32 Layer, const FVector2D& At, const float Radius, const FLinearColor& Colour)
	{
		FSlateDrawElement::MakeBox(Elements, Layer, Geometry.ToPaintGeometry(FVector2f(Radius * 2.0f, Radius * 2.0f),
			FSlateLayoutTransform(FVector2f(At - FVector2D(Radius)))), Disc(), ESlateDrawEffect::None, Colour * Tint);
	};
	const FLinearColor Star = StarColour.Get(FLinearColor::White);
	const FVector2D Centre(Size.X * 0.5, Size.Y * 0.52);
	const float OrbitX = static_cast<float>(Size.X) * 0.40f;
	const float OrbitY = static_cast<float>(Size.Y) * 0.30f;
	Ellipse(Elements, LayerId, Geometry, Centre, OrbitX, OrbitY, APSChrome::CyanDim() * Tint, Line);

	const float StarRadius = Short * 0.13f;
	Ellipse(Elements, LayerId, Geometry, Centre, StarRadius + 6.0f, StarRadius + 6.0f, Star.CopyWithNewOpacity(0.38f) * Tint, Line);
	Ellipse(Elements, LayerId, Geometry, Centre, StarRadius + 12.0f, StarRadius + 12.0f, Star.CopyWithNewOpacity(0.16f) * Tint, Line);
	DiscAt(LayerId + 1, Centre, StarRadius, Star);
	if (!HasBody.Get(true))
	{
		return LayerId + 1;
	}

	// Upper right on the orbit; a ring of the card's fill cuts the orbit line around the world.
	constexpr float BodyAngle = -0.62f;
	const FVector2D BodyCentre = Centre + FVector2D(FMath::Cos(BodyAngle) * OrbitX, FMath::Sin(BodyAngle) * OrbitY);
	const float BodyRadius = Short * 0.075f;
	DiscAt(LayerId + 1, BodyCentre, BodyRadius + 3.0f, APSDashboard::CardFill());
	DiscAt(LayerId + 2, BodyCentre, BodyRadius, BodyColour.Get(FLinearColor::White));
	const int32 Shown = FMath::Clamp(Moons.Get(0), 0, 6);
	if (Shown > 0)
	{
		const float MoonOrbit = BodyRadius * 2.1f;
		Ellipse(Elements, LayerId + 1, Geometry, BodyCentre, MoonOrbit, MoonOrbit, APSChrome::CyanDim().CopyWithNewOpacity(0.5f) * Tint, Line);
		for (int32 Moon = 0; Moon < Shown; ++Moon)
		{
			const float Angle = -UE_HALF_PI + 0.4f + UE_TWO_PI * static_cast<float>(Moon) / Shown;
			DiscAt(LayerId + 3, BodyCentre + FVector2D(FMath::Cos(Angle), FMath::Sin(Angle)) * MoonOrbit, 2.6f, APSChrome::Muted());
		}
	}
	return LayerId + 3;
}

#undef LOCTEXT_NAMESPACE
