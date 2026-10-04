#include "SAPSSystemScheme.h"
#include "APS_ALPHA/UI/Style/APSUINumber.h"

#include "APS_ALPHA/Actors/Astro/APSBodyDesignation.h"
#include "APS_ALPHA/Actors/Astro/Moon.h"
#include "APS_ALPHA/Actors/Astro/Planet.h"
#include "APS_ALPHA/Actors/Astro/PlanetarySystem.h"
#include "APS_ALPHA/Actors/Astro/Star.h"
#include "APS_ALPHA/Actors/Astro/StarSystem.h"
#include "APS_ALPHA/UI/Style/APSMenuChrome.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Fonts/FontMeasure.h"
#include "Framework/Application/SlateApplication.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Rendering/DrawElements.h"
#include "Rendering/SlateRenderer.h"
#include "Styling/AppStyle.h"
#include "Styling/CoreStyle.h"

#define LOCTEXT_NAMESPACE "APSSystemScheme"

namespace APSSystemSchemePrivate
{
	// The largest planet's disc radius at zoom 1; every other disc uses the same kilometre scale.
	constexpr double LargestPlanetPixels = 34.0;
	// Bodies that would vanish at this scale keep a visible dot (marked in the legend).
	constexpr float MinimumPlanetPixels = 3.0f;
	constexpr float MinimumMoonPixels = 2.5f;
	// Between neighbouring slots (each slot fits its disc, its labels and its moon column).
	constexpr double SlotGapPixels = 34.0;
	// Above a lane's axis: the star's labels.
	constexpr double LaneHeaderPixels = 70.0;
	// From the largest disc of a lane to its label row; the row itself (designation, name, type); to the first moon.
	constexpr double LabelGapPixels = 14.0;
	constexpr double LabelRowPixels = 52.0;
	constexpr double MoonGapPixels = 12.0;
	constexpr double MoonRowPixels = 20.0;
	constexpr double MoonTextGapPixels = 7.0;

	FSlateFontInfo DesignationFont() { return APSChrome::Font(TEXT("Bold"), 10); }
	FSlateFontInfo NameFont() { return FCoreStyle::GetDefaultFontStyle(TEXT("Bold"), 12); }
	FSlateFontInfo DetailFont() { return FCoreStyle::GetDefaultFontStyle(TEXT("Regular"), 10); }
	/** Secondary text, brighter than the chrome's muted grey: readable on the dark scheme. */
	FLinearColor Soft() { return FLinearColor(0.64f, 0.75f, 0.80f, 1.0f); }

	FVector2D Measure(const FText& Text, const FSlateFontInfo& Font)
	{
		return Text.IsEmpty() ? FVector2D::ZeroVector
			: FSlateApplication::Get().GetRenderer()->GetFontMeasureService()->Measure(Text, Font);
	}
	// Rio 04.10 ("what are these stars? it was fine"): a star is always a filled round disc inside its lane. Up to the
	// lane's room it is whole and to scale; a larger one slides off the left edge (whole radius, up to twice the room) and
	// then shows its round edge, flatter for the system's largest star, never an empty outline and never cut flat by
	// the lane. Every step follows the size continuously, so zooming never jumps.
	constexpr double StarMarginPixels = 12.0;
	constexpr double LaneMarginPixels = 12.0;
	constexpr double StarSlideFrom = 1.0;
	constexpr double StarSlideTo = 2.0;
	constexpr double StarEdgeTo = 4.0;
	// The part of a large star in view, as a fraction of its room: the smallest star of the system and the largest.
	constexpr double StarEdgeSmallest = 0.32;
	constexpr double StarEdgeLargest = 0.50;

	FLinearColor TypeColor(const EPlanetType Type)
	{
		switch (Type)
		{
		case EPlanetType::Melted: case EPlanetType::Volcanic: case EPlanetType::Lava: case EPlanetType::HotGiant:
			return FLinearColor(1.0f, 0.32f, 0.12f);
		case EPlanetType::GasGiant: return FLinearColor(0.98f, 0.70f, 0.30f);
		case EPlanetType::IceGiant: case EPlanetType::Ice: case EPlanetType::Frozen: return FLinearColor(0.55f, 0.85f, 1.0f);
		case EPlanetType::Ocean: case EPlanetType::Water: case EPlanetType::Archipelago: return FLinearColor(0.18f, 0.55f, 1.0f);
		case EPlanetType::Terrestrial: case EPlanetType::Forest: case EPlanetType::Oasis: return FLinearColor(0.25f, 0.88f, 0.55f);
		case EPlanetType::Desert: case EPlanetType::Sand: return FLinearColor(0.95f, 0.74f, 0.36f);
		case EPlanetType::Metal: case EPlanetType::Metallic: case EPlanetType::Carbon: return FLinearColor(0.72f, 0.66f, 0.90f);
		default: return FLinearColor(0.70f, 0.72f, 0.76f);
		}
	}

	FText Upper(const FName Name, const FText& Fallback)
	{
		return Name.IsNone() ? Fallback : FText::FromString(Name.ToString().ToUpper());
	}

	/** "ROCKY", "FROZEN", "GAS GIANT" (no " PLANET": moons are not planets). */
	FText EnumText(const EPlanetType Type)
	{
		FString Name = UEnum::GetDisplayValueAsText(Type).ToString().ToUpper();
		Name.RemoveFromEnd(TEXT(" PLANET"));
		return FText::FromString(Name);
	}

	const FSlateBrush* Disc()
	{
		// No corner radius: half the height, so a square box paints a circle.
		static const FSlateRoundedBoxBrush Brush(FLinearColor::White);
		return &Brush;
	}

	void Arc(FSlateWindowElementList& Elements, const int32 Layer, const FGeometry& Geometry, const FVector2D& Centre,
		const float Radius, const float From, const float To, const FLinearColor& Color, const float Width)
	{
		TArray<FVector2D> Points;
		const int32 Segments = FMath::Clamp(FMath::CeilToInt((To - From) * Radius / 6.0f), 12, 96);
		for (int32 Index = 0; Index <= Segments; ++Index)
		{
			const float Angle = FMath::Lerp(From, To, static_cast<float>(Index) / Segments);
			Points.Add(Centre + FVector2D(FMath::Cos(Angle), FMath::Sin(Angle)) * Radius);
		}
		FSlateDrawElement::MakeLines(Elements, Layer, Geometry.ToPaintGeometry(), Points, ESlateDrawEffect::None, Color,
			true, Width);
	}
}

void SAPSSystemScheme::Construct(const FArguments& InArgs)
{
	World = InArgs._World;
	OnPicked = InArgs._OnPicked;
	Refresh();
}

void SAPSSystemScheme::Refresh()
{
	using namespace APSSystemSchemePrivate;
	Bodies.Reset();
	Lanes = 0;
	UWorld* LiveWorld = World.Get();
	if (!LiveWorld)
	{
		return;
	}
	const APlayerController* Controller = LiveWorld->GetFirstPlayerController();
	const APawn* Pawn = Controller ? Controller->GetPawn() : nullptr;
	const FVector Origin = Pawn ? Pawn->GetActorLocation() : FVector::ZeroVector;
	// The current system: the star nearest the player, with every star of its system.
	const AStar* Nearest = nullptr;
	double NearestDistance = TNumericLimits<double>::Max();
	for (TActorIterator<AStar> It(LiveWorld); It; ++It)
	{
		if (!IsValid(*It) || !IsValid(It->PlanetarySystem)) continue;
		const double Distance = FVector::DistSquared(It->GetActorLocation(), Origin);
		if (Distance < NearestDistance)
		{
			NearestDistance = Distance;
			Nearest = *It;
		}
	}
	if (!Nearest)
	{
		return;
	}
	TArray<AStar*> Stars;
	if (const AStarSystem* System = Cast<AStarSystem>(Nearest->GetAttachParentActor()))
	{
		Stars = System->GetStars();
	}
	if (Stars.IsEmpty())
	{
		Stars.Add(const_cast<AStar*>(Nearest));
	}
	for (AStar* Star : Stars)
	{
		if (!IsValid(Star)) continue;
		FBody& StarBody = Bodies.AddDefaulted_GetRef();
		StarBody.Actor = Star;
		StarBody.bStar = true;
		StarBody.Lane = Lanes;
		StarBody.RadiusKm = Star->RadiusKM > 0.0 ? Star->RadiusKM : static_cast<double>(Star->StarRadiusKM);
		StarBody.Designation = FText::FromString(APSBodyDesignation::Of(Star));
		StarBody.Name = Upper(Star->AstroName, LOCTEXT("Star", "STAR"));
		StarBody.Detail = Star->FullSpectralName.IsNone() ? FText::GetEmpty()
			: FText::FromString(Star->FullSpectralName.ToString().ToUpper());
		StarBody.Color = FLinearColor(1.0f, 0.82f, 0.46f);
		if (IsValid(Star->PlanetarySystem))
		{
			for (APlanet* Planet : Star->PlanetarySystem->PlanetsActorsList)
			{
				if (!IsValid(Planet)) continue;
				const int32 PlanetIndex = Bodies.Num();
				FBody& PlanetBody = Bodies.AddDefaulted_GetRef();
				PlanetBody.Actor = Planet;
				PlanetBody.Lane = Lanes;
				PlanetBody.RadiusKm = Planet->PlanetRadiusKM;
				PlanetBody.Designation = FText::FromString(APSBodyDesignation::Of(Planet));
				PlanetBody.Name = Upper(Planet->AstroName, LOCTEXT("Planet", "PLANET"));
				PlanetBody.Detail = FText::Format(LOCTEXT("PlanetDetail", "{0}  /  {1} KM"), EnumText(Planet->PlanetType),
					APSUINumber::Number(Planet->PlanetRadiusKM));
				PlanetBody.Color = TypeColor(Planet->PlanetType);
				for (AMoon* Moon : Planet->Moons)
				{
					if (!IsValid(Moon)) continue;
					FBody& MoonBody = Bodies.AddDefaulted_GetRef();
					MoonBody.Actor = Moon;
					MoonBody.bMoon = true;
					MoonBody.Lane = Lanes;
					MoonBody.Parent = PlanetIndex;
					MoonBody.RadiusKm = Moon->PlanetRadiusKM;
					MoonBody.Designation = FText::FromString(APSBodyDesignation::Of(Moon));
					MoonBody.Name = Upper(Moon->AstroName, LOCTEXT("Moon", "MOON"));
					MoonBody.Detail = FText::Format(LOCTEXT("MoonDetail", "{0}  /  {1} KM"), EnumText(Moon->PlanetType),
						APSUINumber::Number(Moon->PlanetRadiusKM));
					// The moon column reads "A1.02  SIAX".
					MoonBody.Label = FText::Format(LOCTEXT("MoonLabel", "{0}  {1}"), MoonBody.Designation, MoonBody.Name);
					MoonBody.Color = TypeColor(Moon->PlanetType);
				}
			}
		}
		++Lanes;
	}
	LaidOutSize = FVector2D::ZeroVector;
}

void SAPSSystemScheme::Layout(const FVector2D& Size) const
{
	using namespace APSSystemSchemePrivate;
	Centres.SetNumZeroed(Bodies.Num());
	Radii.SetNumZeroed(Bodies.Num());
	LabelRows.SetNumZeroed(FMath::Max(Lanes, 1));
	LaidOutSize = Size;
	if (Bodies.IsEmpty() || Lanes == 0)
	{
		return;
	}
	double LargestPlanetKm = 1.0;
	double LargestStarKm = 1.0;
	for (const FBody& Body : Bodies)
	{
		if (!Body.bStar) LargestPlanetKm = FMath::Max(LargestPlanetKm, Body.RadiusKm);
		else LargestStarKm = FMath::Max(LargestStarKm, Body.RadiusKm);
	}
	PixelsPerKm = LargestPlanetPixels / LargestPlanetKm * Zoom;
	const double LaneHeight = Size.Y / Lanes;
	// First pass: each lane's largest planet disc (the axis and the label row follow it) and each planet's moon column.
	TArray<double> LaneLargest;
	LaneLargest.Init(MinimumPlanetPixels, Lanes);
	TArray<double> MoonColumns;
	MoonColumns.Init(0.0, Bodies.Num());
	TArray<double> MoonHeights;
	MoonHeights.Init(0.0, Bodies.Num());
	for (int32 Index = 0; Index < Bodies.Num(); ++Index)
	{
		const FBody& Body = Bodies[Index];
		const double Radius = Body.RadiusKm * PixelsPerKm;
		if (Body.bMoon && Bodies.IsValidIndex(Body.Parent))
		{
			const double Dot = 2.0 * FMath::Max(Radius, static_cast<double>(MinimumMoonPixels));
			MoonColumns[Body.Parent] = FMath::Max(MoonColumns[Body.Parent],
				Dot + MoonTextGapPixels + Measure(Body.Label, DetailFont()).X);
			MoonHeights[Body.Parent] += FMath::Max(Dot, MoonRowPixels) + 4.0;
		}
		else if (!Body.bStar && Body.Lane < Lanes)
		{
			LaneLargest[Body.Lane] = FMath::Max(LaneLargest[Body.Lane], FMath::Max(Radius, static_cast<double>(MinimumPlanetPixels)));
		}
	}
	// Under each lane's axis: the label row and the tallest moon column.
	TArray<double> LaneBelow;
	LaneBelow.Init(0.0, Lanes);
	for (int32 Index = 0; Index < Bodies.Num(); ++Index)
	{
		const FBody& Body = Bodies[Index];
		if (!Body.bStar && !Body.bMoon && Body.Lane < Lanes && MoonHeights[Index] > 0.0)
		{
			LaneBelow[Body.Lane] = FMath::Max(LaneBelow[Body.Lane], MoonGapPixels + MoonHeights[Index]);
		}
	}
	// Rio 04.10: the axis sits mid-lane when the labels and moons fit under it, so the star is as tall as its lane allows.
	TArray<double> Axes;
	Axes.SetNumZeroed(Lanes);
	TArray<double> StarRoom;
	StarRoom.SetNumZeroed(Lanes);
	for (int32 Lane = 0; Lane < Lanes; ++Lane)
	{
		const double Below = LaneLargest[Lane] + LabelGapPixels + LabelRowPixels + LaneBelow[Lane] + LaneMarginPixels;
		const double Offset = FMath::Max(LaneHeaderPixels + LaneLargest[Lane], FMath::Min(LaneHeight * 0.5, LaneHeight - Below));
		Axes[Lane] = LaneHeight * Lane + Offset;
		// Never smaller than the lane's largest planet (zoomed far in, the planets outgrow the lanes anyway).
		StarRoom[Lane] = FMath::Max3(FMath::Min(Offset, LaneHeight - Offset) - StarMarginPixels, 1.3 * LaneLargest[Lane], 8.0);
		LabelRows[Lane] = Axes[Lane] + LaneLargest[Lane] + LabelGapPixels + Pan.Y;
	}
	double Along = 0.0;
	double MoonY = 0.0;
	double MoonLeft = 0.0;
	for (int32 Index = 0; Index < Bodies.Num(); ++Index)
	{
		const FBody& Body = Bodies[Index];
		const double AxisY = Axes[FMath::Clamp(Body.Lane, 0, Lanes - 1)];
		const double Radius = Body.RadiusKm * PixelsPerKm;
		if (Body.bStar)
		{
			// Whole and to scale up to the lane's room; then the whole disc slides off the left edge until half of it is in
			// view; then its edge flattens to the round cap whose chord at the left edge is the room's height again.
			const double Room = StarRoom[FMath::Clamp(Body.Lane, 0, Lanes - 1)];
			const double Scale = Radius / Room;
			double DrawnRadius = FMath::Max(Radius, 6.0);
			double RightEdge = 24.0 + 2.0 * DrawnRadius;
			if (Scale > StarSlideTo)
			{
				const double Edge = Room * FMath::Lerp(StarEdgeSmallest, StarEdgeLargest, FMath::Clamp(Body.RadiusKm / LargestStarKm, 0.0, 1.0));
				RightEdge = FMath::Lerp(Room, Edge, FMath::SmoothStep(StarSlideTo, StarEdgeTo, Scale));
				DrawnRadius = (Room * Room + RightEdge * RightEdge) / (2.0 * RightEdge);
			}
			else if (Scale > StarSlideFrom)
			{
				DrawnRadius = Room;
				RightEdge = FMath::Lerp(24.0 + 2.0 * Room, Room, FMath::SmoothStep(StarSlideFrom, StarSlideTo, Scale));
			}
			Radii[Index] = static_cast<float>(DrawnRadius);
			Centres[Index] = FVector2D(RightEdge - DrawnRadius, AxisY) + Pan;
			Along = RightEdge + 48.0;
			continue;
		}
		if (!Body.bMoon)
		{
			Radii[Index] = FMath::Max(static_cast<float>(Radius), MinimumPlanetPixels);
			const double Labels = FMath::Max3(Measure(Body.Designation, DesignationFont()).X,
				Measure(Body.Name, NameFont()).X, Measure(Body.Detail, DetailFont()).X);
			const double Slot = FMath::Max3(2.0 * Radii[Index], Labels, MoonColumns[Index]) + SlotGapPixels;
			Centres[Index] = FVector2D(Along + Slot * 0.5, AxisY) + Pan;
			Along += Slot;
			// The moon column sits centred under the planet's labels.
			MoonY = AxisY + LaneLargest[FMath::Clamp(Body.Lane, 0, Lanes - 1)] + LabelGapPixels + LabelRowPixels + MoonGapPixels;
			MoonLeft = Centres[Index].X - Pan.X - MoonColumns[Index] * 0.5;
			continue;
		}
		Radii[Index] = FMath::Max(static_cast<float>(Radius), MinimumMoonPixels);
		const double Row = FMath::Max(2.0 * Radii[Index], MoonRowPixels);
		Centres[Index] = FVector2D(MoonLeft + Radii[Index], MoonY + Row * 0.5) + Pan;
		MoonY += Row + 4.0;
	}
}

int32 SAPSSystemScheme::HitTest(const FVector2D& Local) const
{
	int32 Best = INDEX_NONE;
	double BestDistance = TNumericLimits<double>::Max();
	for (int32 Index = 0; Index < Bodies.Num() && Index < Centres.Num(); ++Index)
	{
		const double Distance = FVector2D::Distance(Local, Centres[Index]);
		// A star's limb is picked anywhere on its disc; small bodies get a few pixels of slack.
		if (Distance <= Radii[Index] + 6.0 && Distance - Radii[Index] < BestDistance)
		{
			BestDistance = Distance - Radii[Index];
			Best = Index;
		}
	}
	return Best;
}

int32 SAPSSystemScheme::OnPaint(const FPaintArgs&, const FGeometry& Geometry, const FSlateRect&,
	FSlateWindowElementList& Elements, int32 LayerId, const FWidgetStyle&, bool) const
{
	using namespace APSSystemSchemePrivate;
	const FVector2D Size = Geometry.GetLocalSize();
	Layout(Size);
	const FSlateFontInfo DesignationFont = APSSystemSchemePrivate::DesignationFont();
	const FSlateFontInfo NameFont = APSSystemSchemePrivate::NameFont();
	const FSlateFontInfo DetailFont = APSSystemSchemePrivate::DetailFont();
	// Positions are line tops; bCentred centres the line on X, bMiddle centres its capitals on Y (moon labels beside their
	// dots; Rio 03.10: by the letters, not by Slate's line box).
	const auto Text = [&](const FText& Value, const FVector2D& Position, const FSlateFontInfo& Font, const FLinearColor& Color,
		const bool bCentred, const bool bMiddle = false)
	{
		if (Value.IsEmpty()) return;
		const FVector2D Measured = APSSystemSchemePrivate::Measure(Value, Font);
		const FVector2D At = Position - FVector2D(bCentred ? Measured.X * 0.5 : 0.0,
			bMiddle ? Measured.Y * 0.5 - APSChrome::CapsCenterOffset(Font) : 0.0);
		FSlateDrawElement::MakeText(Elements, LayerId + 4, Geometry.ToPaintGeometry(Measured, FSlateLayoutTransform(At)),
			Value, Font, ESlateDrawEffect::None, Color);
	};
	FSlateDrawElement::MakeBox(Elements, LayerId, Geometry.ToPaintGeometry(), FAppStyle::GetBrush("WhiteBrush"),
		ESlateDrawEffect::None, FLinearColor(0.002f, 0.010f, 0.018f, 0.92f));
	if (Bodies.IsEmpty())
	{
		Text(LOCTEXT("NoSystem", "NO STAR SYSTEM NEARBY"), Size * 0.5, NameFont, APSChrome::Muted(), true);
		return LayerId + 4;
	}
	const double LaneHeight = Size.Y / FMath::Max(Lanes, 1);
	for (int32 Lane = 1; Lane < Lanes; ++Lane)
	{
		FSlateDrawElement::MakeLines(Elements, LayerId + 1, Geometry.ToPaintGeometry(),
			TArray<FVector2D>{FVector2D(0.0, Lane * LaneHeight), FVector2D(Size.X, Lane * LaneHeight)},
			ESlateDrawEffect::None, APSChrome::CyanDim(), true, 1.0f);
	}
	const AActor* PickedActor = Picked.Get();
	for (int32 Index = 0; Index < Bodies.Num(); ++Index)
	{
		const FBody& Body = Bodies[Index];
		const FVector2D Centre = Centres[Index];
		const float Radius = Radii[Index];
		const bool bPicked = PickedActor && Body.Actor.Get() == PickedActor;
		if (Body.bStar)
		{
			// The orbital order axis from the star through its planets.
			FSlateDrawElement::MakeLines(Elements, LayerId + 1, Geometry.ToPaintGeometry(),
				TArray<FVector2D>{FVector2D(FMath::Max(0.0, Centre.X + Radius), Centre.Y), FVector2D(Size.X, Centre.Y)},
				ESlateDrawEffect::None, FLinearColor(APSChrome::Cyan().R, APSChrome::Cyan().G, APSChrome::Cyan().B, 0.18f),
				true, 1.0f);
			if (Centre.X + Radius < -2.0) continue;
			// Rio 04.10 ("what are these stars? it was fine"): the filled disc with a soft glow and a bright limb, always;
			// Layout keeps it inside its lane, and the widget's left edge cuts the part that slid off (the limb is drawn
			// only where it is in view).
			const float Span = FMath::Acos(static_cast<float>(FMath::Clamp(-Centre.X / Radius, -1.0, 1.0)));
			const FVector2D ClipMin = Geometry.LocalToAbsolute(FVector2D::ZeroVector);
			const FVector2D ClipMax = Geometry.LocalToAbsolute(Size);
			Elements.PushClip(FSlateClippingZone(FSlateRect(ClipMin.X, ClipMin.Y, ClipMax.X, ClipMax.Y)));
			FSlateDrawElement::MakeBox(Elements, LayerId + 2, Geometry.ToPaintGeometry(FVector2D(2.0f * Radius),
				FSlateLayoutTransform(Centre - FVector2D(Radius))), Disc(), ESlateDrawEffect::None,
				FLinearColor(Body.Color.R, Body.Color.G, Body.Color.B, 0.85f));
			for (const TPair<float, float>& Stroke : {TPair<float, float>(14.0f, 0.10f), TPair<float, float>(6.0f, 0.30f),
				TPair<float, float>(1.6f, 1.0f)})
			{
				Arc(Elements, LayerId + 2, Geometry, Centre, Radius, -Span, Span,
					FLinearColor(Body.Color.R, Body.Color.G, Body.Color.B, Stroke.Value), Stroke.Key);
			}
			Elements.PopClip();
			// The star's labels on a dark plate (Rio 02.10: "the star's label drifts off somewhere"): a small star has them
			// under its disc like a planet; a large one centred on the part of the disc in view, on the axis, so the
			// label moves with the star under zoom and pan instead of sticking near the left edge.
			// A star drawn smaller than to scale says by how much.
			const double TrueRadius = Body.RadiusKm * PixelsPerKm;
			const FText ScaleNote = TrueRadius > Radius * 1.05
				? FText::Format(LOCTEXT("StarScaleNote", "SHOWN {0}x SMALLER"), APSUINumber::Number(FMath::RoundToInt(TrueRadius / Radius)))
				: FText::GetEmpty();
			const double PlateWidth = FMath::Max(FMath::Max3(Measure(Body.Designation, DesignationFont).X,
				Measure(Body.Name, NameFont).X, Measure(Body.Detail, DetailFont).X), Measure(ScaleNote, DetailFont).X) + 20.0;
			FVector2D LabelAt;
			if (2.0 * Radius < LaneHeight * 0.5)
			{
				LabelAt = FVector2D(Centre.X - PlateWidth * 0.5 + 10.0, Centre.Y + Radius + LabelGapPixels);
			}
			else
			{
				const double VisibleLeft = FMath::Max(0.0, Centre.X - Radius);
				const double VisibleRight = FMath::Min(Size.X, Centre.X + Radius);
				const double Left = (VisibleLeft + VisibleRight - PlateWidth) * 0.5;
				LabelAt = FVector2D(FMath::Max(VisibleLeft + 4.0, Left) + 10.0, Centre.Y - 22.0);
			}
			{
				FSlateDrawElement::MakeBox(Elements, LayerId + 3, Geometry.ToPaintGeometry(FVector2D(PlateWidth, ScaleNote.IsEmpty() ? 56.0 : 73.0),
					FSlateLayoutTransform(LabelAt - FVector2D(10.0, 6.0))), FAppStyle::GetBrush("WhiteBrush"), ESlateDrawEffect::None,
					FLinearColor(0.004f, 0.016f, 0.026f, 0.82f));
			}
			Text(Body.Designation, LabelAt, DesignationFont, bPicked ? APSChrome::Amber() : Body.Color, false);
			Text(Body.Name, LabelAt + FVector2D(0.0, 16.0), NameFont, bPicked ? APSChrome::Amber() : APSChrome::White(), false);
			Text(Body.Detail, LabelAt + FVector2D(0.0, 35.0), DetailFont, Soft(), false);
			Text(ScaleNote, LabelAt + FVector2D(0.0, 52.0), DetailFont, APSChrome::Muted(), false);
			continue;
		}
		FSlateDrawElement::MakeBox(Elements, LayerId + 2, Geometry.ToPaintGeometry(FVector2D(2.0f * Radius),
			FSlateLayoutTransform(Centre - FVector2D(Radius))), Disc(), ESlateDrawEffect::None, Body.Color);
		if (bPicked)
		{
			Arc(Elements, LayerId + 3, Geometry, Centre, Radius + 4.0f, 0.0f, UE_TWO_PI, APSChrome::Amber(), 1.6f);
		}
		if (!Body.bMoon)
		{
			// Every label of a lane on one row under its largest disc; a faint stem joins a small disc to its labels.
			const double Row = LabelRows.IsValidIndex(Body.Lane) ? LabelRows[Body.Lane] : Centre.Y + Radius + 14.0;
			if (Row - (Centre.Y + Radius) > 10.0)
			{
				FSlateDrawElement::MakeLines(Elements, LayerId + 1, Geometry.ToPaintGeometry(),
					TArray<FVector2D>{FVector2D(Centre.X, Centre.Y + Radius + 4.0), FVector2D(Centre.X, Row - 4.0)},
					ESlateDrawEffect::None, FLinearColor(Body.Color.R, Body.Color.G, Body.Color.B, 0.28f), true, 1.0f);
			}
			Text(Body.Designation, FVector2D(Centre.X, Row), DesignationFont, bPicked ? APSChrome::Amber() : Body.Color, true);
			Text(Body.Name, FVector2D(Centre.X, Row + 16.0), NameFont, bPicked ? APSChrome::Amber() : APSChrome::White(), true);
			Text(Body.Detail, FVector2D(Centre.X, Row + 35.0), DetailFont, Soft(), true);
		}
		else
		{
			Text(Body.Label, Centre + FVector2D(Radius + MoonTextGapPixels, 0.0), DetailFont,
				bPicked ? APSChrome::Amber() : Soft(), false, true);
		}
	}
	// Scale bar: 100 pixels in kilometres, and the honest caption.
	const double BarKm = PixelsPerKm > 0.0 ? 100.0 / PixelsPerKm : 0.0;
	const FVector2D BarEnd(Size.X - 16.0, Size.Y - 20.0);
	FSlateDrawElement::MakeLines(Elements, LayerId + 3, Geometry.ToPaintGeometry(),
		TArray<FVector2D>{BarEnd - FVector2D(100.0, 0.0), BarEnd}, ESlateDrawEffect::None, APSChrome::Cyan(), true, 1.5f);
	Text(FText::Format(LOCTEXT("ScaleBar", "{0} KM"), APSUINumber::Number(FMath::RoundToInt(BarKm))),
		BarEnd - FVector2D(50.0, 20.0), DetailFont, APSChrome::Cyan(), true);
	Text(LOCTEXT("SchemeHint", "SIZES TO SCALE, DISTANCES NOT  /  WHEEL ZOOM  /  DRAG TO PAN  /  CLICK A BODY"),
		FVector2D(14.0, Size.Y - 24.0), DetailFont, Soft(), false);
	return LayerId + 4;
}

FReply SAPSSystemScheme::OnMouseWheel(const FGeometry& Geometry, const FPointerEvent& Event)
{
	const double Previous = Zoom;
	Zoom = FMath::Clamp(Zoom * FMath::Pow(1.2, Event.GetWheelDelta()), 0.25, 60.0);
	// Keep the point under the cursor roughly in place: horizontal positions scale with the discs. Rio 04.10: never past
	// the left edge, where only empty space was left after zooming out with the cursor on the right.
	const FVector2D Pointer = Geometry.AbsoluteToLocal(Event.GetScreenSpacePosition());
	Pan.X = FMath::Min(Pointer.X - (Pointer.X - Pan.X) * (Zoom / Previous), 0.0);
	return FReply::Handled();
}

FReply SAPSSystemScheme::OnMouseButtonDown(const FGeometry& Geometry, const FPointerEvent& Event)
{
	if (Event.GetEffectingButton() != EKeys::LeftMouseButton)
	{
		return FReply::Unhandled();
	}
	bDragging = true;
	bDragged = false;
	DragStart = Geometry.AbsoluteToLocal(Event.GetScreenSpacePosition());
	PanStart = Pan;
	return FReply::Handled().CaptureMouse(SharedThis(this));
}

FReply SAPSSystemScheme::OnMouseMove(const FGeometry& Geometry, const FPointerEvent& Event)
{
	if (!bDragging)
	{
		return FReply::Unhandled();
	}
	const FVector2D Local = Geometry.AbsoluteToLocal(Event.GetScreenSpacePosition());
	bDragged |= FVector2D::Distance(Local, DragStart) > 4.0;
	if (bDragged)
	{
		Pan = PanStart + (Local - DragStart);
		Pan.X = FMath::Min(Pan.X, 0.0);
	}
	return FReply::Handled();
}

FReply SAPSSystemScheme::OnMouseButtonUp(const FGeometry& Geometry, const FPointerEvent& Event)
{
	if (!bDragging || Event.GetEffectingButton() != EKeys::LeftMouseButton)
	{
		return FReply::Unhandled();
	}
	bDragging = false;
	if (!bDragged)
	{
		const int32 Hit = HitTest(Geometry.AbsoluteToLocal(Event.GetScreenSpacePosition()));
		if (Bodies.IsValidIndex(Hit))
		{
			Picked = Bodies[Hit].Actor;
			OnPicked.ExecuteIfBound(Picked.Get());
		}
	}
	return FReply::Handled().ReleaseMouseCapture();
}

FCursorReply SAPSSystemScheme::OnCursorQuery(const FGeometry&, const FPointerEvent&) const
{
	return FCursorReply::Cursor(bDragged ? EMouseCursor::GrabHandClosed : EMouseCursor::Default);
}

#undef LOCTEXT_NAMESPACE
