#include "SAPSStarScheme.h"
#include "APS_ALPHA/UI/Style/APSUITheme.h"

#include "APS_ALPHA/Pawns/Spaceships/Spaceship.h"
#include "APS_ALPHA/UI/StrategicMap/APSStrategicMapScene.h"
#include "APS_ALPHA/UI/Style/APSMenuChrome.h"
#include "APS_ALPHA/UI/Style/APSUINumber.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "CoreGlobals.h"
#include "Engine/World.h"
#include "Fonts/FontMeasure.h"
#include "Framework/Application/SlateApplication.h"
#include "InputCoreTypes.h"
#include "Misc/ConfigCacheIni.h"
#include "Rendering/DrawElements.h"
#include "Rendering/SlateRenderer.h"
#include "Styling/AppStyle.h"
#include "Styling/CoreStyle.h"

#define LOCTEXT_NAMESPACE "APSStarScheme"

namespace APSStarSchemePrivate
{
	constexpr double HalfPi = 1.57079632679489661923;
	constexpr double TwoPi = 6.28318530717958647692;

	/** The legend's choice, read once from GameUserSettings and kept for the session (Rio 05.10). */
	const TCHAR* const ConfigSection = TEXT("APS.Terminal.StarMap");
	bool GLegendRead = false;
	bool GLegendShown = true;

	// The terminal's palette (APSMenuChrome) and the maps' own: the scheme's ground, the rings' edge, the relay teal.
	FLinearColor SchemeBackground() { return APSUITheme::Retint(FLinearColor(0.002f, 0.010f, 0.018f, 0.92f)); }
	FLinearColor EdgeColour(const float Opacity) { return APSUITheme::RetintHighlight(FLinearColor(0.26f, 0.84f, 0.93f, Opacity)); }
	FLinearColor LinkColour() { return FLinearColor(0.35f, 0.95f, 0.95f, 1.0f); }
	FLinearColor ColonyColour() { return FLinearColor(0.36f, 1.0f, 0.58f, 1.0f); }
	FLinearColor HeadquartersColour() { return FLinearColor(1.0f, 0.85f, 0.38f, 1.0f); }
	FLinearColor PlateColour() { return APSUITheme::Retint(FLinearColor(0.002f, 0.014f, 0.026f, 0.94f)); }
	FLinearColor SoftColour() { return APSUITheme::Retint(FLinearColor(0.64f, 0.75f, 0.80f, 1.0f)); }
	FLinearColor CompanionColour() { return FLinearColor::FromSRGBColor(FColor(255, 140, 90)); }
	FLinearColor WithAlpha(const FLinearColor& Colour, const float Opacity) { return FLinearColor(Colour.R, Colour.G, Colour.B, Opacity); }

	FSlateFontInfo ChipFont() { return APSUITheme::BodyFont("Bold", 10); }
	FSlateFontInfo NameFont() { return APSUITheme::BodyFont("Bold", 10); }
	FSlateFontInfo MarkFont() { return APSUITheme::BodyFont("Bold", 9); }
	FSlateFontInfo BangFont() { return APSUITheme::BodyFont("Bold", 8); }
	/** Captions, the legend and the hints: the chrome's regular 9 (11 pt, Rio 02.10: never smaller). */
	FSlateFontInfo SmallFont() { return APSChrome::Font(TEXT("Regular"), 9); }

	const FSlateBrush* DiscBrush()
	{
		// No corner radius: half the height, so a square box paints a circle.
		static const FSlateRoundedBoxBrush Brush(FLinearColor::White);
		return &Brush;
	}

	const FSlateBrush* ChipBrush()
	{
		static const FSlateRoundedBoxBrush Brush(FLinearColor::White, 3.0f);
		return &Brush;
	}

	const FSlateBrush* FlatBrush()
	{
		return FAppStyle::GetBrush("WhiteBrush");
	}

	FVector2D MeasureText(const FText& Value, const FSlateFontInfo& Font)
	{
		return Value.IsEmpty() ? FVector2D::ZeroVector
			: FVector2D(FSlateApplication::Get().GetRenderer()->GetFontMeasureService()->Measure(Value, Font));
	}

	double LineHeight(const FSlateFontInfo& Font)
	{
		return FSlateApplication::Get().GetRenderer()->GetFontMeasureService()->GetMaxCharacterHeight(Font);
	}

	FBox2D BoxAt(const double X, const double Y, const double Width, const double Height)
	{
		return FBox2D(FVector2D(X, Y), FVector2D(X + Width, Y + Height));
	}

	bool Overlaps(const FBox2D& A, const FBox2D& B, const double Pad)
	{
		return A.Min.X < B.Max.X + Pad && B.Min.X < A.Max.X + Pad && A.Min.Y < B.Max.Y + Pad && B.Min.Y < A.Max.Y + Pad;
	}

	void PaintLines(FSlateWindowElementList& Out, const int32 Layer, const FGeometry& Geometry, const TArray<FVector2D>& Points,
		const FLinearColor& Colour, const float Thickness)
	{
		if (Points.Num() >= 2)
		{
			FSlateDrawElement::MakeLines(Out, Layer, Geometry.ToPaintGeometry(), Points, ESlateDrawEffect::None, Colour, true, Thickness);
		}
	}

	void PaintDot(FSlateWindowElementList& Out, const int32 Layer, const FGeometry& Geometry, const FVector2D& At,
		const double Radius, const FLinearColor& Colour)
	{
		const float Diameter = static_cast<float>(Radius * 2.0);
		FSlateDrawElement::MakeBox(Out, Layer, Geometry.ToPaintGeometry(FVector2f(Diameter, Diameter),
			FSlateLayoutTransform(FVector2f(static_cast<float>(At.X - Radius), static_cast<float>(At.Y - Radius)))),
			DiscBrush(), ESlateDrawEffect::None, Colour);
	}

	/** Screen angles, y down: 0 to the right, pi / 2 down. */
	void PaintArc(FSlateWindowElementList& Out, const int32 Layer, const FGeometry& Geometry, const FVector2D& At,
		const double Radius, const double From, const double To, const FLinearColor& Colour, const float Thickness)
	{
		if (Radius < 0.5)
		{
			return;
		}
		const int32 Segments = FMath::Clamp(FMath::CeilToInt(FMath::Abs(To - From) * Radius / 3.0), 4, 180);
		TArray<FVector2D> Points;
		Points.Reserve(Segments + 1);
		for (int32 Step = 0; Step <= Segments; ++Step)
		{
			const double Angle = FMath::Lerp(From, To, static_cast<double>(Step) / Segments);
			Points.Add(At + FVector2D(FMath::Cos(Angle), FMath::Sin(Angle)) * Radius);
		}
		PaintLines(Out, Layer, Geometry, Points, Colour, Thickness);
	}

	void PaintCircle(FSlateWindowElementList& Out, const int32 Layer, const FGeometry& Geometry, const FVector2D& At,
		const double Radius, const FLinearColor& Colour, const float Thickness)
	{
		PaintArc(Out, Layer, Geometry, At, Radius, 0.0, TwoPi, Colour, Thickness);
	}

	/** A dashed circle: a boundary or something only catalogued, not a ring of the civilization's. */
	void PaintDashedCircle(FSlateWindowElementList& Out, const int32 Layer, const FGeometry& Geometry, const FVector2D& At,
		const double Radius, const FLinearColor& Colour, const float Thickness, const int32 Dashes)
	{
		const int32 Count = Dashes > 0 ? Dashes : FMath::Clamp(FMath::RoundToInt(Radius * 0.25), 12, 96);
		for (int32 Dash = 0; Dash < Count; ++Dash)
		{
			const double From = TwoPi * Dash / Count;
			PaintArc(Out, Layer, Geometry, At, Radius, From, From + TwoPi * 0.55 / Count, Colour, Thickness);
		}
	}

	/** A dashed polyline (an order under way: 6 on, 4 off; the planned route 8 on, 5 off). */
	void PaintDashed(FSlateWindowElementList& Out, const int32 Layer, const FGeometry& Geometry, const TArray<FVector2D>& Points,
		const FLinearColor& Colour, const float Thickness, const double Dash, const double Gap)
	{
		double Phase = 0.0;
		for (int32 Index = 1; Index < Points.Num(); ++Index)
		{
			const FVector2D From = Points[Index - 1];
			const FVector2D Along = Points[Index] - From;
			const double Length = Along.Size();
			if (Length < 1.0e-6)
			{
				continue;
			}
			const FVector2D Direction = Along / Length;
			double Walked = 0.0;
			while (Walked < Length)
			{
				const double Cycle = FMath::Fmod(Phase, Dash + Gap);
				const bool bOn = Cycle < Dash;
				const double Run = FMath::Max(FMath::Min((bOn ? Dash : Dash + Gap) - Cycle, Length - Walked), 1.0e-3);
				if (bOn)
				{
					PaintLines(Out, Layer, Geometry, {From + Direction * Walked, From + Direction * FMath::Min(Walked + Run, Length)},
						Colour, Thickness);
				}
				Walked += Run;
				Phase += Run;
			}
		}
	}

	void PaintFill(FSlateWindowElementList& Out, const int32 Layer, const FGeometry& Geometry, const TArray<FVector2D>& Points,
		const FLinearColor& Colour)
	{
		if (Points.Num() < 3)
		{
			return;
		}
		const FSlateResourceHandle Handle = FlatBrush()->GetRenderingResource();
		const FColor VertexColour = Colour.ToFColor(true);
		TArray<FSlateVertex> Vertices;
		Vertices.Reserve(Points.Num());
		for (const FVector2D& Point : Points)
		{
			Vertices.Add(FSlateVertex::Make<ESlateVertexRounding::Disabled>(Geometry.GetAccumulatedRenderTransform(),
				FVector2f(Point), FVector2f::ZeroVector, VertexColour));
		}
		TArray<SlateIndex> Indices;
		for (int32 Corner = 1; Corner + 1 < Points.Num(); ++Corner)
		{
			Indices.Add(0);
			Indices.Add(static_cast<SlateIndex>(Corner));
			Indices.Add(static_cast<SlateIndex>(Corner + 1));
		}
		FSlateDrawElement::MakeCustomVerts(Out, Layer, Handle, Vertices, Indices, nullptr, 0, 0);
	}

	void PaintTriangle(FSlateWindowElementList& Out, const int32 Layer, const FGeometry& Geometry, const FVector2D& At,
		const double Half, const FLinearColor& Colour, const float Thickness, const bool bUp, const FLinearColor* FillColour = nullptr)
	{
		const double Sign = bUp ? 1.0 : -1.0;
		TArray<FVector2D> Points = {At + FVector2D(0.0, -Half * Sign), At + FVector2D(Half * 0.95, Half * 0.65 * Sign),
			At + FVector2D(-Half * 0.95, Half * 0.65 * Sign)};
		if (FillColour)
		{
			PaintFill(Out, Layer, Geometry, Points, *FillColour);
		}
		// Rio 05.10 (the star map crashed: "a container element which already comes from the container being modified"):
		// the closing corner is copied first; Add(Points[0]) may grow the array under the reference it reads.
		const FVector2D First = Points[0];
		Points.Add(First);
		PaintLines(Out, Layer, Geometry, Points, Colour, Thickness);
	}

	void PaintDiamond(FSlateWindowElementList& Out, const int32 Layer, const FGeometry& Geometry, const FVector2D& At,
		const double Half, const FLinearColor& Colour, const float Thickness)
	{
		PaintLines(Out, Layer, Geometry, {At + FVector2D(0.0, -Half), At + FVector2D(Half, 0.0), At + FVector2D(0.0, Half),
			At + FVector2D(-Half, 0.0), At + FVector2D(0.0, -Half)}, Colour, Thickness);
	}

	/** The civilization map's ship triangle, turned to its heading (screen radians; -pi / 2 points up). */
	void PaintShip(FSlateWindowElementList& Out, const int32 Layer, const FGeometry& Geometry, const FVector2D& At,
		const FLinearColor& Colour, const double Tip, const double Heading, const float Thickness)
	{
		const double CosTurn = FMath::Cos(Heading + HalfPi);
		const double SinTurn = FMath::Sin(Heading + HalfPi);
		const auto Turned = [&At, CosTurn, SinTurn](const double X, const double Y)
		{
			return At + FVector2D(X * CosTurn - Y * SinTurn, X * SinTurn + Y * CosTurn);
		};
		TArray<FVector2D> Points = {Turned(0.0, -Tip), Turned(Tip * 0.75, Tip * 0.7), Turned(-Tip * 0.75, Tip * 0.7)};
		PaintFill(Out, Layer, Geometry, Points, WithAlpha(Colour, 0.35f * Colour.A));
		const FVector2D First = Points[0];
		Points.Add(First);
		PaintLines(Out, Layer, Geometry, Points, Colour, Thickness);
	}

	/** Four corner brackets: an order's target, as the ship HUD marks its course target. */
	void PaintBrackets(FSlateWindowElementList& Out, const int32 Layer, const FGeometry& Geometry, const FVector2D& At,
		const double Half, const FLinearColor& Colour, const float Thickness)
	{
		const double Arm = FMath::Clamp(Half * 0.45, 4.0, 10.0);
		for (const FVector2D& Corner : {FVector2D(-1.0, -1.0), FVector2D(1.0, -1.0), FVector2D(1.0, 1.0), FVector2D(-1.0, 1.0)})
		{
			const FVector2D Tip = At + Corner * Half;
			PaintLines(Out, Layer, Geometry, {Tip - FVector2D(Corner.X * Arm, 0.0), Tip, Tip - FVector2D(0.0, Corner.Y * Arm)},
				Colour, Thickness);
		}
	}

	void PaintBox(FSlateWindowElementList& Out, const int32 Layer, const FGeometry& Geometry, const FBox2D& Box,
		const FLinearColor& Colour, const FSlateBrush* Brush = nullptr)
	{
		FSlateDrawElement::MakeBox(Out, Layer, Geometry.ToPaintGeometry(Box.GetSize(), FSlateLayoutTransform(Box.Min)),
			Brush ? Brush : FlatBrush(), ESlateDrawEffect::None, Colour);
	}

	void PaintOutline(FSlateWindowElementList& Out, const int32 Layer, const FGeometry& Geometry, const FBox2D& Box,
		const FLinearColor& Colour, const float Thickness)
	{
		PaintLines(Out, Layer, Geometry, {Box.Min, FVector2D(Box.Max.X, Box.Min.Y), Box.Max, FVector2D(Box.Min.X, Box.Max.Y), Box.Min},
			Colour, Thickness);
	}

	void PaintText(FSlateWindowElementList& Out, const int32 Layer, const FGeometry& Geometry, const FText& Value,
		const FSlateFontInfo& Font, const FVector2D& TopLeft, const FLinearColor& Colour)
	{
		if (Value.IsEmpty())
		{
			return;
		}
		FSlateDrawElement::MakeText(Out, Layer, Geometry.ToPaintGeometry(MeasureText(Value, Font) + FVector2D(2.0, 0.0),
			FSlateLayoutTransform(TopLeft)), Value, Font, ESlateDrawEffect::None, Colour);
	}

	/** Capitals centred on MidY (Rio 03.10: by the letters, not by the line box); Align 0 starts at X, 0.5 centres, 1 ends. */
	void PaintTextMid(FSlateWindowElementList& Out, const int32 Layer, const FGeometry& Geometry, const FText& Value,
		const FSlateFontInfo& Font, const double X, const double MidY, const FLinearColor& Colour, const double Align = 0.0)
	{
		const FVector2D Measured = MeasureText(Value, Font);
		PaintText(Out, Layer, Geometry, Value, Font,
			FVector2D(X - Measured.X * Align, MidY - LineHeight(Font) * 0.5 + APSChrome::CapsCenterOffset(Font)), Colour);
	}

	/** A quadratic curve from A to B bowed to its side by Bulge of its length (the routes' arcs). */
	TArray<FVector2D> MakeCurve(const FVector2D& From, const FVector2D& To, const double Bulge)
	{
		constexpr int32 Steps = 40;
		const FVector2D Delta = To - From;
		const double Length = FMath::Max(Delta.Size(), 1.0e-6);
		const FVector2D Control = (From + To) * 0.5 + FVector2D(-Delta.Y, Delta.X) / Length * (Bulge * Length);
		TArray<FVector2D> Points;
		Points.Reserve(Steps + 1);
		for (int32 Step = 0; Step <= Steps; ++Step)
		{
			const double Share = static_cast<double>(Step) / Steps;
			Points.Add(From * FMath::Square(1.0 - Share) + Control * (2.0 * (1.0 - Share) * Share) + To * (Share * Share));
		}
		return Points;
	}

	double PathLength(const TArray<FVector2D>& Points)
	{
		double Total = 0.0;
		for (int32 Index = 1; Index < Points.Num(); ++Index)
		{
			Total += FVector2D::Distance(Points[Index - 1], Points[Index]);
		}
		return Total;
	}

	/** The point at a share of a polyline's length and the heading there (screen radians). */
	void PointOnPath(const TArray<FVector2D>& Points, const double Share, FVector2D& OutPoint, double& OutHeading)
	{
		OutPoint = Points.IsEmpty() ? FVector2D::ZeroVector : Points[0];
		OutHeading = -HalfPi;
		double Left = PathLength(Points) * FMath::Clamp(Share, 0.0, 1.0);
		for (int32 Index = 1; Index < Points.Num(); ++Index)
		{
			const FVector2D Step = Points[Index] - Points[Index - 1];
			const double Length = Step.Size();
			if (Length <= 0.0)
			{
				continue;
			}
			OutHeading = FMath::Atan2(Step.Y, Step.X);
			if (Left <= Length || Index == Points.Num() - 1)
			{
				OutPoint = Points[Index - 1] + Step * FMath::Clamp(Left / Length, 0.0, 1.0);
				return;
			}
			Left -= Length;
		}
	}

	/** The part of a polyline between two shares of its length. */
	TArray<FVector2D> CutPath(const TArray<FVector2D>& Points, const double From, const double To)
	{
		TArray<FVector2D> Part;
		if (Points.Num() < 2 || To <= From)
		{
			return Part;
		}
		const double Total = PathLength(Points);
		const double Start = Total * FMath::Clamp(From, 0.0, 1.0);
		const double End = Total * FMath::Clamp(To, 0.0, 1.0);
		FVector2D Point;
		double Heading = 0.0;
		PointOnPath(Points, From, Point, Heading);
		Part.Add(Point);
		double Walked = 0.0;
		for (int32 Index = 1; Index < Points.Num(); ++Index)
		{
			Walked += FVector2D::Distance(Points[Index - 1], Points[Index]);
			if (Walked > Start && Walked < End)
			{
				Part.Add(Points[Index]);
			}
		}
		PointOnPath(Points, To, Point, Heading);
		Part.Add(Point);
		return Part;
	}

	/** A route's arc between two stars, its ends kept off their discs. */
	TArray<FVector2D> RouteCurve(const FVector2D& From, const double FromDisc, const FVector2D& To, const double ToDisc,
		const double Bulge)
	{
		const double Length = FMath::Max(FVector2D::Distance(From, To), 1.0);
		return CutPath(MakeCurve(From, To, Bulge), FMath::Clamp((FromDisc + 12.0) / Length, 0.0, 0.45),
			FMath::Clamp(1.0 - (ToDisc + 12.0) / Length, 0.55, 1.0));
	}

	/** The civilization's marks above a star, left to right (the mock's presence row). */
	enum class EMark : uint8
	{
		Beacon,
		Colony,
		Headquarters,
		Outpost,
		Anomaly,
		Fleet
	};

	void MarksOf(const APSStarMap::FSystem& System, TArray<EMark, TInlineAllocator<8>>& OutMarks)
	{
		OutMarks.Reset();
		if (System.bClaimed)
		{
			OutMarks.Add(EMark::Beacon);
		}
		if (System.bColony)
		{
			OutMarks.Add(EMark::Colony);
		}
		if (System.bHeadquarters)
		{
			OutMarks.Add(EMark::Headquarters);
		}
		for (int32 Each = 0; Each < FMath::Min(System.Outposts, 3); ++Each)
		{
			OutMarks.Add(EMark::Outpost);
		}
		if (System.Anomaly == 1 || System.Anomaly == 2)
		{
			OutMarks.Add(EMark::Anomaly);
		}
		if (System.Ships > 0)
		{
			OutMarks.Add(EMark::Fleet);
		}
	}

	/** Where a mark sits: on a short arc above the star, wider round home with its rings. */
	FVector2D MarkAt(const FVector2D& Star, const double Disc, const bool bWide, const int32 Mark, const int32 Count)
	{
		const double Radius = Disc + (bWide ? 25.0 : 15.0);
		const double Angle = -HalfPi + (Mark - (Count - 1) * 0.5) * (bWide ? 0.42 : 0.62);
		return Star + FVector2D(FMath::Cos(Angle), FMath::Sin(Angle)) * Radius;
	}

	/** Labels go by this: the pick, home and the centre, the claimed, the fleet's, the surveyed, the scanned, the rest. */
	int32 PriorityOf(const APSStarMap::FSystem& System, const bool bPicked)
	{
		if (bPicked)
		{
			return 0;
		}
		if (System.bCentre || System.bHome)
		{
			return 1;
		}
		if (System.bClaimed)
		{
			return 2;
		}
		if (System.bInUse)
		{
			return 3;
		}
		return System.Knowledge == APSStars::EKnowledge::Surveyed ? 4 : System.Knowledge == APSStars::EKnowledge::Scanned ? 5 : 6;
	}

	bool IsUncharted(const APSStarMap::FSystem& System)
	{
		return System.Knowledge == APSStars::EKnowledge::Catalogued && !System.bClaimed && !System.bHome;
	}

	/** The worlds' dots under a plate: once their number is known (a scan), at most nine. */
	int32 WorldDots(const APSStarMap::FSystem& System)
	{
		return System.Knowledge == APSStars::EKnowledge::Catalogued || System.Worlds <= 0 ? 0 : FMath::Min(System.Worlds, 9);
	}

	/** The mock's plate places round a star: beside it, under, over its marks, diagonal, farther out, slid along. */
	void PlateCandidates(const FVector2D& Star, const double Ring, const double Width, const double Height, const bool bRight,
		TArray<FVector2D>& OutCandidates)
	{
		const double X = Star.X;
		const double Y = Star.Y;
		for (const double Extra : {0.0, 22.0, 44.0})
		{
			FVector2D Row[8] = {
				{X + Ring + 6.0 + Extra, Y - Height * 0.5}, {X - Ring - 6.0 - Width - Extra, Y - Height * 0.5},
				{X - Width * 0.5, Y + Ring + 6.0 + Extra}, {X + Ring + Extra * 0.7, Y + Ring + Extra * 0.7},
				{X - Ring - Width - Extra * 0.7, Y + Ring + Extra * 0.7}, {X - Width * 0.5, Y - Ring - 26.0 - Height - FMath::Min(Extra, 22.0)},
				{X + Ring + Extra * 0.7, Y - Ring - Height - Extra * 0.7}, {X - Ring - Width - Extra * 0.7, Y - Ring - Height - Extra * 0.7}};
			if (!bRight)
			{
				Swap(Row[0], Row[1]);
				Swap(Row[3], Row[4]);
				Swap(Row[6], Row[7]);
			}
			for (const FVector2D& Spot : Row)
			{
				OutCandidates.Add(Spot);
			}
			for (const double Slide : {12.0, -12.0, 24.0, -24.0})
			{
				OutCandidates.Add({X + Ring + 6.0 + Extra, Y - Height * 0.5 + Slide});
				OutCandidates.Add({X - Ring - 6.0 - Width - Extra, Y - Height * 0.5 + Slide});
				OutCandidates.Add({X - Width * 0.5 + Slide * 2.0, Y + Ring + 6.0 + Extra});
				OutCandidates.Add({X - Width * 0.5 + Slide * 2.0, Y - Ring - 26.0 - Height - FMath::Min(Extra, 22.0)});
			}
		}
	}

	/** F10's leader: from the star's ring to the nearest point of a plate set off from it. */
	void PaintLeader(FSlateWindowElementList& Out, const int32 Layer, const FGeometry& Geometry, const FVector2D& Star,
		const double Ring, const FBox2D& Box, const FLinearColor& Colour)
	{
		const FVector2D Nearest(FMath::Clamp(Star.X, Box.Min.X, Box.Max.X), FMath::Clamp(Star.Y, Box.Min.Y, Box.Max.Y));
		const double Gap = FVector2D::Distance(Nearest, Star);
		if (Gap > Ring + 10.0)
		{
			const FVector2D Direction = (Nearest - Star) / Gap;
			PaintLines(Out, Layer, Geometry, {Star + Direction * Ring, Nearest}, Colour, 1.0f);
		}
	}

	/** A label chip on a route or round work: its text over the plate fill, framed in its colour. */
	struct FChip
	{
		TArray<FVector2D> Anchors;
		FLinearColor Colour{FLinearColor::White};
		FText Label;
		float FrameOpacity{0.6f};
	};
}

void SAPSStarScheme::Construct(const FArguments& InArgs)
{
	World = InArgs._World;
	Style = InArgs._Style;
	OnPicked = InArgs._OnPicked;
	OnOpened = InArgs._OnOpened;
	// Marks at the rim (the core's direction) stay on the map, not on the panels beside it.
	SetClipping(EWidgetClipping::ClipToBounds);
	Refresh(true);
}

void SAPSStarScheme::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
	SLeafWidget::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);
	// Twice a second while shown: the model rebuilds only on a revision, the fleet's progress always.
	RefreshClock -= InDeltaTime;
	if (RefreshClock <= 0.0f)
	{
		RefreshClock = 0.5f;
		Refresh();
	}
}

void SAPSStarScheme::Refresh(const bool bForce)
{
	FAPSStarMapModel::FQuery Query;
	Query.Centre = Centre;
	if (Selected.IsValid())
	{
		Query.Pinned.Add(Selected);
	}
	Model.Update(World.Get(), Query, bForce);
	UpdateShipPlaces();
}

void SAPSStarScheme::UpdateShipPlaces()
{
	UWorld* LiveWorld = World.Get();
	const auto Locate = [this, LiveWorld](const ASpaceship* Ship)
	{
		FShipPlace Place;
		Place.bValid = Ship && Model.LocateShip(LiveWorld, Ship, Place.System, Place.Position);
		return Place;
	};
	PlannedPlaces.Reset();
	for (const FPlannedRoute& Route : PlannedRoutes)
	{
		PlannedPlaces.Add(Locate(Route.Ship.Get()));
	}
	HighlightedPlaces.Reset();
	for (const TWeakObjectPtr<ASpaceship>& Ship : HighlightedShips)
	{
		HighlightedPlaces.Add(Locate(Ship.Get()));
	}
}

void SAPSStarScheme::SetSelected(const FGuid& Id)
{
	if (Selected != Id)
	{
		Selected = Id;
		Refresh();
	}
}

void SAPSStarScheme::SetCentre(const FGuid& Id)
{
	if (Centre != Id)
	{
		Centre = Id;
		Zoom = 1.0;
		Pan = FVector2D::ZeroVector;
		Refresh();
	}
}

void SAPSStarScheme::SetPlannedRoutes(const TArray<FPlannedRoute>& Routes)
{
	PlannedRoutes = Routes;
	UpdateShipPlaces();
}

void SAPSStarScheme::SetHighlightedShips(const TArray<TWeakObjectPtr<ASpaceship>>& Ships)
{
	HighlightedShips = Ships;
	UpdateShipPlaces();
}

FText SAPSStarScheme::GetTitle() const
{
	const APSStarMap::FSnapshot& Snap = Model.Get();
	return Snap.bReady && Snap.Systems.Num() > 0
		? FText::Format(LOCTEXT("Title", "STARS  /  AROUND {0}"), FText::FromString(Snap.Systems[0].Name))
		: LOCTEXT("TitleNone", "STARS");
}

FText SAPSStarScheme::GetSubtitle() const
{
	const APSStarMap::FSnapshot& Snap = Model.Get();
	if (!Snap.bReady)
	{
		return Snap.Status;
	}
	return FText::Format(LOCTEXT("Subtitle", "{0} OF {1} SYSTEMS  /  {2} KNOWN"), APSUINumber::Number(Snap.Systems.Num() - 1),
		APSUINumber::Number(Snap.CatalogueCount), APSUINumber::Number(Snap.KnownCount));
}

bool SAPSStarScheme::IsLegendShown()
{
	using namespace APSStarSchemePrivate;
	if (!GLegendRead)
	{
		GLegendRead = true;
		if (GConfig)
		{
			GConfig->GetBool(ConfigSection, TEXT("Legend"), GLegendShown, GGameUserSettingsIni);
		}
	}
	return GLegendShown;
}

void SAPSStarScheme::SetLegendShown(const bool bShown)
{
	using namespace APSStarSchemePrivate;
	GLegendRead = true;
	GLegendShown = bShown;
	if (GConfig)
	{
		GConfig->SetBool(ConfigSection, TEXT("Legend"), bShown, GGameUserSettingsIni);
		GConfig->Flush(false, GGameUserSettingsIni);
	}
}

SAPSStarScheme::FFrame SAPSStarScheme::MakeFrame(const FVector2D& Size) const
{
	using namespace APSStarMap;
	const FSnapshot& Snap = Model.Get();
	const bool bFull = Style == EStyle::Full;
	FFrame Frame;
	Frame.Size = Size;
	// Over the map: the title and filters (or the target switch); under it the legend and hints, or nothing when hidden.
	Frame.TopBand = bFull ? 48.0 : 66.0;
	Frame.BottomBand = IsLegendShown() ? (bFull ? 66.0 : 46.0) : 10.0;
	Frame.OuterRing = Snap.Rings[OuterRing].Count > 0 ? OuterRing : Snap.Rings[2].Count > 0 ? 2 : Snap.Rings[1].Count > 0 ? 1 : 0;
	const double Outermost = RingRadius[Frame.OuterRing] + (Frame.OuterRing < OuterRing ? RingSpread * 0.5 : 0.0);
	const double Tall = (Size.Y - Frame.TopBand - Frame.BottomBand) * 0.5 - (bFull ? 4.0 : 18.0);
	const double Wide = Size.X * 0.5 - (bFull ? 80.0 : 56.0);
	Frame.PixelsPerUnit = FMath::Max(FMath::Min(Tall, Wide) / Outermost, 20.0) * Zoom;
	Frame.Centre = FVector2D(Size.X * 0.5, Frame.TopBand + (Size.Y - Frame.TopBand - Frame.BottomBand) * 0.5) + Pan;
	return Frame;
}

FVector2D SAPSStarScheme::ToScreen(const FFrame& Frame, const FVector2D& LayoutPosition) const
{
	return Frame.Centre + LayoutPosition * Frame.PixelsPerUnit;
}

float SAPSStarScheme::DiscOf(const APSStarMap::FSystem& System) const
{
	return Style == EStyle::Full ? System.DiscRadius : System.PickerRadius;
}

bool SAPSStarScheme::IsDrawn(const APSStarMap::FSystem& System) const
{
	// The pick stays drawn whatever the filter.
	return APSStarMap::PassesFilter(System, Filter) || (Selected.IsValid() && System.Id == Selected);
}

int32 SAPSStarScheme::HitTest(const FVector2D& Local) const
{
	const APSStarMap::FSnapshot& Snap = Model.Get();
	int32 Best = INDEX_NONE;
	double BestGap = TNumericLimits<double>::Max();
	for (int32 Index = 0; Index < Snap.Systems.Num() && Index < Screen.Num(); ++Index)
	{
		if (!IsDrawn(Snap.Systems[Index]))
		{
			continue;
		}
		const double Gap = FVector2D::Distance(Local, Screen[Index]) - DiscOf(Snap.Systems[Index]);
		if (Gap <= 8.0 && Gap < BestGap)
		{
			BestGap = Gap;
			Best = Index;
		}
	}
	return Best;
}

void SAPSStarScheme::PlaceLabels(const FFrame& Frame) const
{
	using namespace APSStarSchemePrivate;
	const APSStarMap::FSnapshot& Snap = Model.Get();
	const bool bFull = Style == EStyle::Full;
	// Laid out again only when what is drawn where, the frame, the pick, the filter or the planned routes change.
	uint32 Key = HashCombine(Model.GetLayoutVersion(), GetTypeHash(Selected));
	Key = HashCombine(Key, GetTypeHash(FIntPoint(FMath::RoundToInt(Frame.Size.X), FMath::RoundToInt(Frame.Size.Y))));
	Key = HashCombine(Key, GetTypeHash(FIntPoint(FMath::RoundToInt(Frame.Centre.X * 4.0), FMath::RoundToInt(Frame.Centre.Y * 4.0))));
	Key = HashCombine(Key, GetTypeHash(FMath::RoundToInt(Frame.PixelsPerUnit * 16.0)));
	Key = HashCombine(Key, GetTypeHash(static_cast<int32>(Filter) * 4 + static_cast<int32>(Style) * 2 + (IsLegendShown() ? 1 : 0)));
	for (const FShipPlace& Place : PlannedPlaces)
	{
		Key = HashCombine(Key, GetTypeHash(FIntPoint(Place.System, FMath::RoundToInt(Place.Position.X * 100.0 + Place.Position.Y * 7.0))));
	}
	for (const FShipPlace& Place : HighlightedPlaces)
	{
		Key = HashCombine(Key, GetTypeHash(Place.System));
	}
	if (Key == LabelKey && Named.Num() == Snap.Systems.Num())
	{
		return;
	}
	LabelKey = Key;
	Labels.Reset();
	LabelObstacles.Reset();
	SoftObstacles.Reset();
	Named.Init(false, Snap.Systems.Num());
	if (!Snap.bReady || Screen.Num() != Snap.Systems.Num())
	{
		return;
	}

	// Hard obstacles: what the overlay puts over the map, the rings' captions, the core's mark, every star with its rings
	// and marks.
	const auto AddHard = [this](const FBox2D& Box) { LabelObstacles.Add(Box); };
	if (bFull)
	{
		AddHard(BoxAt(0.0, 0.0, 270.0, 50.0));
		AddHard(BoxAt(Frame.Size.X - 560.0, 0.0, 560.0, 46.0));
	}
	else
	{
		AddHard(BoxAt(0.0, 0.0, 380.0, 70.0));
	}
	if (Frame.BottomBand > 12.0)
	{
		AddHard(BoxAt(0.0, Frame.Size.Y - Frame.BottomBand, Frame.Size.X, Frame.BottomBand));
	}
	for (int32 Ring = 0; Ring <= APSStarMap::OuterRing; ++Ring)
	{
		if (Snap.Rings[Ring].Count > 0)
		{
			const double Width = MeasureText(Snap.Rings[Ring].Caption, SmallFont()).X + 12.0;
			AddHard(BoxAt(Frame.Centre.X - Width * 0.5, Frame.Centre.Y - APSStarMap::RingRadius[Ring] * Frame.PixelsPerUnit - 8.0,
				Width, 16.0));
		}
	}
	if (bFull && Snap.bCore)
	{
		const FVector2D Core = Frame.Centre + FVector2D(FMath::Cos(Snap.CoreAngle), -FMath::Sin(Snap.CoreAngle))
			* (APSStarMap::RingRadius[Frame.OuterRing] * Frame.PixelsPerUnit + 15.0);
		AddHard(BoxAt(Core.X - 18.0, Core.Y - 8.0, 36.0, 30.0));
	}
	TArray<EMark, TInlineAllocator<8>> Marks;
	for (int32 Index = 0; Index < Snap.Systems.Num(); ++Index)
	{
		const APSStarMap::FSystem& System = Snap.Systems[Index];
		if (!IsDrawn(System))
		{
			continue;
		}
		const FVector2D Star = Screen[Index];
		const double Disc = DiscOf(System);
		const double Pad = Disc + 7.0;
		AddHard(BoxAt(Star.X - Pad, Star.Y - Pad, 2.0 * Pad, 2.0 * Pad));
		if (System.bCentre || System.bHome || System.Id == Selected)
		{
			AddHard(BoxAt(Star.X - Disc - 16.0, Star.Y - Disc - 16.0, 2.0 * Disc + 32.0, 2.0 * Disc + 32.0));
		}
		if (bFull)
		{
			MarksOf(System, Marks);
			for (int32 Mark = 0; Mark < Marks.Num(); ++Mark)
			{
				const FVector2D At = MarkAt(Star, Disc, System.bCentre || System.bHome, Mark, Marks.Num());
				AddHard(BoxAt(At.X - 7.0, At.Y - 7.0, Marks[Mark] == EMark::Fleet ? 24.0 : 14.0, 14.0));
			}
			if (FMath::Abs(System.Slot.Elevation) > APSStarMap::ElevationMark)
			{
				AddHard(BoxAt(Star.X - Disc - 18.0, Star.Y - 5.0, 10.0, 10.0));
			}
		}
		else if (System.bClaimed)
		{
			AddHard(BoxAt(Star.X - 6.0, Star.Y - Disc - 18.0, 12.0, 12.0));
		}
	}
	for (const FShipPlace& Place : HighlightedPlaces)
	{
		if (Place.bValid)
		{
			const FVector2D At = Snap.Systems.IsValidIndex(Place.System) ? Screen[Place.System] : ToScreen(Frame, Place.Position);
			AddHard(BoxAt(At.X - 22.0, At.Y - 22.0, 44.0, 44.0));
		}
	}

	// Soft obstacles: the lines a label crosses only when there is no other room (links, routes, planned routes).
	const auto AddSoft = [this](const TArray<FVector2D>& Points)
	{
		for (int32 Index = 1; Index < Points.Num(); ++Index)
		{
			const FVector2D From = Points[Index - 1];
			const FVector2D Delta = Points[Index] - From;
			const int32 Steps = FMath::Max(1, FMath::FloorToInt(Delta.Size() / 8.0));
			for (int32 Step = 0; Step <= Steps; ++Step)
			{
				const FVector2D At = From + Delta * (static_cast<double>(Step) / Steps);
				SoftObstacles.Add(BoxAt(At.X - 2.0, At.Y - 2.0, 4.0, 4.0));
			}
		}
	};
	for (const APSStarMap::FLink& Link : Snap.Links)
	{
		if (Screen.IsValidIndex(Link.A) && Screen.IsValidIndex(Link.B))
		{
			AddSoft({Screen[Link.A], Screen[Link.B]});
		}
	}
	for (const APSStarMap::FRoute& Route : Snap.Routes)
	{
		if (Route.From != Route.To && Screen.IsValidIndex(Route.From) && Screen.IsValidIndex(Route.To))
		{
			AddSoft(RouteCurve(Screen[Route.From], DiscOf(Snap.Systems[Route.From]), Screen[Route.To], DiscOf(Snap.Systems[Route.To]), 0.16));
		}
	}
	const int32 Pick = Model.IndexOf(Selected);
	if (Screen.IsValidIndex(Pick))
	{
		for (int32 Plan = 0; Plan < PlannedPlaces.Num(); ++Plan)
		{
			const FShipPlace& Place = PlannedPlaces[Plan];
			if (Place.bValid && Place.System != Pick)
			{
				const bool bAtSystem = Screen.IsValidIndex(Place.System);
				AddSoft(RouteCurve(bAtSystem ? Screen[Place.System] : ToScreen(Frame, Place.Position),
					bAtSystem ? DiscOf(Snap.Systems[Place.System]) : 3.0, Screen[Pick], DiscOf(Snap.Systems[Pick]),
					bFull || Plan % 2 == 0 ? -0.10 : 0.10));
			}
		}
	}

	const auto IsFree = [this, &Frame](const FBox2D& Box, const double Pad, const bool bUseSoft)
	{
		if (Box.Min.X < 4.0 || Box.Min.Y < 4.0 || Box.Max.X > Frame.Size.X - 4.0 || Box.Max.Y > Frame.Size.Y - 4.0)
		{
			return false;
		}
		for (const FBox2D& Other : LabelObstacles)
		{
			if (Overlaps(Box, Other, Pad))
			{
				return false;
			}
		}
		if (bUseSoft)
		{
			for (const FBox2D& Other : SoftObstacles)
			{
				if (Overlaps(Box, Other, 0.0))
				{
					return false;
				}
			}
		}
		return true;
	};
	// A place found: kept as an obstacle and remembered (from its star) for the next layout.
	const auto Keep = [this](FLabel& Label, const FBox2D& Box, const FVector2D& Star, const FGuid& Id)
	{
		Label.Rect = Box;
		LabelObstacles.Add(Box);
		LastLabelOffsets.Add(Id, Box.Min - Star);
		Named[Label.System] = true;
		Labels.Add(Label);
	};
	const auto PlacePlate = [&](const int32 Index)
	{
		const APSStarMap::FSystem& System = Snap.Systems[Index];
		FLabel Label;
		Label.Kind = ELabel::Plate;
		Label.System = Index;
		Label.bLeader = true;
		Label.Plate = APSStrategicMap::LayoutPlate(APSStarMap::TypeLine(System), FText::FromString(System.Name),
			FText::FromString(System.Designation));
		const int32 Dots = WorldDots(System);
		Label.StripHeight = Dots > 0 ? 11.0 : 0.0;
		Label.Plate.Size.X = FMath::Max(Label.Plate.Size.X, Dots > 0 ? 3.0 + 8.0 + 10.0 * Dots + 6.0 : 0.0);
		const double Width = Label.Plate.Size.X;
		const double Height = Label.Plate.Size.Y + Label.StripHeight;
		const FVector2D Star = Screen[Index];
		TArray<FVector2D> Candidates;
		// The place it had first, so labels do not jump when something else changes.
		if (const FVector2D* Last = LastLabelOffsets.Find(System.Id))
		{
			Candidates.Add(Star + *Last);
		}
		PlateCandidates(Star, DiscOf(System) + 8.0, Width, Height, Star.X >= Frame.Centre.X - 4.0, Candidates);
		for (const bool bUseSoft : {true, false})
		{
			for (const FVector2D& TopLeft : Candidates)
			{
				const FBox2D Box = BoxAt(FMath::RoundToDouble(TopLeft.X), FMath::RoundToDouble(TopLeft.Y), Width, Height);
				if (IsFree(Box, 4.0, bUseSoft))
				{
					Keep(Label, Box, Star, System.Id);
					return true;
				}
			}
		}
		return false;
	};
	const auto PlaceName = [&](const int32 Index)
	{
		const APSStarMap::FSystem& System = Snap.Systems[Index];
		FLabel Label;
		Label.Kind = ELabel::Name;
		Label.System = Index;
		const double Width = MeasureText(FText::FromString(System.Name), NameFont()).X;
		const double Height = 12.0;
		const FVector2D Star = Screen[Index];
		double Ring = DiscOf(System) + (bFull ? 7.0 : 6.0);
		if (!bFull)
		{
			for (const FShipPlace& Place : HighlightedPlaces)
			{
				Ring += Place.bValid && Place.System == Index ? 10.0 : 0.0;
			}
		}
		TArray<FVector2D> Candidates;
		if (const FVector2D* Last = LastLabelOffsets.Find(System.Id))
		{
			Candidates.Add(Star + *Last);
		}
		FVector2D Row[6] = {{Star.X + Ring + 5.0, Star.Y - Height * 0.5}, {Star.X - Ring - 5.0 - Width, Star.Y - Height * 0.5},
			{Star.X - Width * 0.5, Star.Y + Ring + 4.0}, {Star.X - Width * 0.5, Star.Y - Ring - 4.0 - Height},
			{Star.X + Ring, Star.Y + Ring - 2.0}, {Star.X - Ring - Width, Star.Y + Ring - 2.0}};
		if (Star.X < Frame.Centre.X - 4.0)
		{
			Swap(Row[0], Row[1]);
		}
		for (const FVector2D& Spot : Row)
		{
			Candidates.Add(Spot);
		}
		for (const bool bUseSoft : {true, false})
		{
			for (const FVector2D& TopLeft : Candidates)
			{
				const FBox2D Box = BoxAt(FMath::RoundToDouble(TopLeft.X), FMath::RoundToDouble(TopLeft.Y), Width, Height);
				if (IsFree(Box, 2.0, bUseSoft))
				{
					Keep(Label, Box, Star, System.Id);
					return true;
				}
			}
		}
		return false;
	};

	TArray<int32> Order;
	for (int32 Index = 0; Index < Snap.Systems.Num(); ++Index)
	{
		if (IsDrawn(Snap.Systems[Index]))
		{
			Order.Add(Index);
		}
	}
	Order.StableSort([this, &Snap](const int32 A, const int32 B)
	{
		const int32 PriorityA = PriorityOf(Snap.Systems[A], Snap.Systems[A].Id == Selected);
		const int32 PriorityB = PriorityOf(Snap.Systems[B], Snap.Systems[B].Id == Selected);
		return PriorityA != PriorityB ? PriorityA < PriorityB : Snap.Systems[A].Rank < Snap.Systems[B].Rank;
	});
	if (bFull)
	{
		// Plates: the pick, home, the claimed and the fleet's first; then the known; the uncharted get names only.
		for (const int32 Index : Order)
		{
			if (PriorityOf(Snap.Systems[Index], Snap.Systems[Index].Id == Selected) <= 3)
			{
				PlacePlate(Index);
			}
		}
		for (const int32 Index : Order)
		{
			const int32 Priority = PriorityOf(Snap.Systems[Index], Snap.Systems[Index].Id == Selected);
			if (Priority > 3 && Priority < 6)
			{
				PlacePlate(Index);
			}
		}
	}
	else if (Screen.IsValidIndex(Pick) && IsDrawn(Snap.Systems[Pick]))
	{
		// The picker: the target's plate, names for the rest.
		PlacePlate(Pick);
	}
	for (const int32 Index : Order)
	{
		if (!Named[Index])
		{
			PlaceName(Index);
		}
	}
}

int32 SAPSStarScheme::OnPaint(const FPaintArgs&, const FGeometry& Geometry, const FSlateRect&, FSlateWindowElementList& Elements,
	const int32 LayerId, const FWidgetStyle&, const bool) const
{
	using namespace APSStarSchemePrivate;
	using APSStarMap::FSystem;
	const APSStarMap::FSnapshot& Snap = Model.Get();
	const FVector2D Size = Geometry.GetLocalSize();
	const FFrame Frame = MakeFrame(Size);
	LastFrame = Frame;
	const bool bFull = Style == EStyle::Full;
	const int32 LayerBack = LayerId;
	const int32 LayerLines = LayerId + 1;
	const int32 LayerGlow = LayerId + 2;
	const int32 LayerBodies = LayerId + 3;
	const int32 LayerMarks = LayerId + 4;
	const int32 LayerLeaders = LayerId + 5;
	const int32 LayerPlates = LayerId + 6;
	const int32 LayerTexts = LayerId + 7;
	const int32 LayerTop = LayerId + 8;
	PaintBox(Elements, LayerBack, Geometry, BoxAt(0.0, 0.0, Size.X, Size.Y), SchemeBackground());
	Screen.SetNum(Snap.Systems.Num());
	for (int32 Index = 0; Index < Snap.Systems.Num(); ++Index)
	{
		Screen[Index] = ToScreen(Frame, Snap.Systems[Index].Slot.Position);
	}
	if (!Snap.bReady)
	{
		PaintTextMid(Elements, LayerTexts, Geometry, Snap.Status, NameFont(), Size.X * 0.5, Size.Y * 0.5, APSChrome::Muted(), 0.5);
		return LayerTop + 1;
	}
	PlaceLabels(Frame);
	const double Scale = Frame.PixelsPerUnit;
	const int32 Pick = Model.IndexOf(Selected);
	const bool bPickDrawn = Screen.IsValidIndex(Pick) && IsDrawn(Snap.Systems[Pick]);

	// Rings by rank (thin), the outer ring dashed with its bearings, the core's direction outside it.
	for (int32 Ring = 0; Ring < APSStarMap::OuterRing; ++Ring)
	{
		if (Snap.Rings[Ring].Count > 0)
		{
			PaintCircle(Elements, LayerLines, Geometry, Frame.Centre, APSStarMap::RingRadius[Ring] * Scale,
				WithAlpha(APSChrome::Cyan(), 0.16f), 1.0f);
		}
	}
	if (Snap.Rings[APSStarMap::OuterRing].Count > 0)
	{
		PaintDashedCircle(Elements, LayerLines, Geometry, Frame.Centre, APSStarMap::RingRadius[APSStarMap::OuterRing] * Scale,
			EdgeColour(0.30f), 1.0f, 0);
	}
	const double Outermost = APSStarMap::RingRadius[Frame.OuterRing] * Scale;
	if (bFull)
	{
		for (int32 Spoke = 0; Spoke < 12; ++Spoke)
		{
			const double Angle = Spoke * TwoPi / 12.0;
			const FVector2D Direction(FMath::Cos(Angle), -FMath::Sin(Angle));
			PaintLines(Elements, LayerLines, Geometry, {Frame.Centre + Direction * (Outermost - 5.0),
				Frame.Centre + Direction * (Outermost + 5.0)}, EdgeColour(0.55f), 1.0f);
		}
		if (Snap.bCore)
		{
			const FVector2D Direction(FMath::Cos(Snap.CoreAngle), -FMath::Sin(Snap.CoreAngle));
			const FVector2D Side(-Direction.Y, Direction.X);
			const FVector2D Core = Frame.Centre + Direction * (Outermost + 15.0);
			PaintFill(Elements, LayerLines, Geometry, {Core + Direction * 5.0, Core - Direction * 3.0 + Side * 5.0,
				Core - Direction * 3.0 - Side * 5.0}, WithAlpha(APSChrome::Muted(), 0.75f));
			PaintTextMid(Elements, LayerTexts, Geometry, LOCTEXT("Core", "CORE"), MarkFont(), Core.X, Core.Y + 14.0,
				WithAlpha(APSChrome::Muted(), 0.75f), 0.5);
		}
	}

	// The relay network: teal links with their traffic.
	for (const APSStarMap::FLink& Link : Snap.Links)
	{
		if (!Screen.IsValidIndex(Link.A) || !Screen.IsValidIndex(Link.B) || !IsDrawn(Snap.Systems[Link.A]) || !IsDrawn(Snap.Systems[Link.B]))
		{
			continue;
		}
		const FVector2D From = Screen[Link.A];
		const FVector2D To = Screen[Link.B];
		PaintLines(Elements, LayerLines, Geometry, {From, To}, WithAlpha(LinkColour(), bFull ? 0.5f : 0.35f), bFull ? 2.4f : 2.0f);
		if (bFull)
		{
			for (const double Pulse : {0.25, 0.55, 0.85})
			{
				PaintDot(Elements, LayerLines, Geometry, From + (To - From) * Pulse, 2.6, LinkColour());
			}
		}
	}

	// The fleet's orders: the way flown solid, the rest dashed, the ship on its share of it; work as an arc round its system.
	TArray<FChip> Chips;
	for (const APSStarMap::FRoute& Route : Snap.Routes)
	{
		if (!Screen.IsValidIndex(Route.To) || !Screen.IsValidIndex(Route.From) || !IsDrawn(Snap.Systems[Route.To]))
		{
			continue;
		}
		const FLinearColor Colour = APSFleet::DivisionColour(Route.Division);
		const FVector2D Target = Screen[Route.To];
		const double TargetDisc = DiscOf(Snap.Systems[Route.To]);
		if (Route.From == Route.To || Route.Phase == APSFleet::EPhase::Working)
		{
			if (Route.Phase == APSFleet::EPhase::Working && bFull)
			{
				PaintArc(Elements, LayerMarks, Geometry, Target, TargetDisc + 11.0, -HalfPi, -HalfPi + TwoPi * Route.Progress, Colour, 2.2f);
				const double Around = TargetDisc + 14.0;
				FChip& Chip = Chips.AddDefaulted_GetRef();
				Chip.Anchors = {Target + FVector2D(0.0, Around), Target + FVector2D(Around, 0.0), Target + FVector2D(-Around, 0.0),
					Target + FVector2D(0.0, -Around)};
				Chip.Colour = Colour;
				Chip.Label = Route.Chip;
			}
			continue;
		}
		const TArray<FVector2D> Curve = RouteCurve(Screen[Route.From], DiscOf(Snap.Systems[Route.From]), Target, TargetDisc, 0.16);
		const float Strength = bFull ? 1.0f : 0.5f;
		PaintLines(Elements, LayerLines, Geometry, CutPath(Curve, 0.0, Route.Share), WithAlpha(Colour, 0.5f * Strength), 1.4f);
		PaintDashed(Elements, LayerLines, Geometry, CutPath(Curve, Route.Share, 1.0), WithAlpha(Colour, 0.9f * Strength), 1.4f, 6.0, 4.0);
		if (bFull)
		{
			PaintCircle(Elements, LayerMarks, Geometry, Target, TargetDisc + 9.0, WithAlpha(Colour, 0.8f), 1.2f);
		}
		FVector2D ShipAt;
		double Heading = 0.0;
		PointOnPath(Curve, Route.Share, ShipAt, Heading);
		PaintShip(Elements, LayerMarks + 1, Geometry, ShipAt, bFull ? Colour : WithAlpha(Colour, 0.6f), bFull ? 7.0 : 5.5, Heading,
			bFull ? 1.6f : 1.2f);
		if (bFull)
		{
			FChip& Chip = Chips.AddDefaulted_GetRef();
			Chip.Anchors.Add(ShipAt);
			for (const double Along : {0.18, -0.15, 0.32, -0.28})
			{
				FVector2D Point;
				double Unused = 0.0;
				PointOnPath(Curve, FMath::Clamp(Route.Share + Along, 0.05, 0.95), Point, Unused);
				Chip.Anchors.Add(Point);
			}
			Chip.Colour = Colour;
			Chip.Label = Route.Chip;
		}
	}

	// The planned orders to the pick: amber dashed from where each ship is, with its label.
	if (bPickDrawn)
	{
		for (int32 Plan = 0; Plan < PlannedRoutes.Num() && Plan < PlannedPlaces.Num(); ++Plan)
		{
			const FShipPlace& Place = PlannedPlaces[Plan];
			if (!Place.bValid || Place.System == Pick)
			{
				continue;
			}
			const bool bAtSystem = Screen.IsValidIndex(Place.System);
			const TArray<FVector2D> Curve = RouteCurve(bAtSystem ? Screen[Place.System] : ToScreen(Frame, Place.Position),
				bAtSystem ? DiscOf(Snap.Systems[Place.System]) : 3.0, Screen[Pick], DiscOf(Snap.Systems[Pick]),
				bFull || Plan % 2 == 0 ? -0.10 : 0.10);
			PaintDashed(Elements, LayerLines, Geometry, Curve, WithAlpha(APSChrome::Amber(), 0.95f), 2.0f, 8.0, 5.0);
			if (!bFull && Curve.Num() >= 2)
			{
				FVector2D End;
				double Heading = 0.0;
				PointOnPath(Curve, 1.0, End, Heading);
				PaintFill(Elements, LayerLines, Geometry, {End + FVector2D(FMath::Cos(Heading), FMath::Sin(Heading)) * 6.0,
					End + FVector2D(FMath::Cos(Heading + 2.5), FMath::Sin(Heading + 2.5)) * 7.0,
					End + FVector2D(FMath::Cos(Heading - 2.5), FMath::Sin(Heading - 2.5)) * 7.0}, APSChrome::Amber());
			}
			FChip& Chip = Chips.AddDefaulted_GetRef();
			for (const double Along : {0.5, 0.62, 0.38, 0.75, 0.28, 0.85})
			{
				FVector2D Point;
				double Unused = 0.0;
				PointOnPath(Curve, Along, Point, Unused);
				Chip.Anchors.Add(Point);
			}
			Chip.Colour = APSChrome::Amber();
			Chip.Label = PlannedRoutes[Plan].Label;
			Chip.FrameOpacity = 0.7f;
		}
	}

	// The stars: a soft halo under a known one, the disc, its knowledge ring, a companion, above or below the plane.
	TArray<EMark, TInlineAllocator<8>> Marks;
	const FLinearColor LinkFill = WithAlpha(LinkColour(), 0.25f);
	for (int32 Index = 0; Index < Snap.Systems.Num(); ++Index)
	{
		const FSystem& System = Snap.Systems[Index];
		if (!IsDrawn(System))
		{
			continue;
		}
		const FVector2D Star = Screen[Index];
		const double Disc = DiscOf(System);
		const FLinearColor Colour = System.Colour;
		if (IsUncharted(System))
		{
			PaintDot(Elements, LayerBodies, Geometry, Star, Disc, WithAlpha(Colour, 0.55f));
			PaintDashedCircle(Elements, LayerBodies, Geometry, Star, Disc + (bFull ? 5.0 : 4.0), WithAlpha(APSChrome::Muted(), 0.45f),
				1.0f, bFull ? 12 : 10);
		}
		else
		{
			PaintDot(Elements, LayerGlow, Geometry, Star, Disc * 2.0, WithAlpha(Colour, 0.08f));
			PaintDot(Elements, LayerGlow, Geometry, Star, Disc * 1.45, WithAlpha(Colour, 0.14f));
			PaintDot(Elements, LayerBodies, Geometry, Star, Disc, Colour);
			PaintCircle(Elements, LayerBodies, Geometry, Star, Disc + (bFull ? 5.0 : 4.0), APSStars::KnowledgeColour(System.Knowledge),
				bFull ? 1.4f : 1.3f);
		}
		if (System.bGiant)
		{
			PaintDot(Elements, LayerGlow, Geometry, Star, Disc * 2.6, WithAlpha(Colour, 0.10f));
		}
		if (System.StarCount > 1)
		{
			const double Offset = (Disc + 1.5) * 0.75;
			PaintDot(Elements, LayerMarks, Geometry, Star + FVector2D(Offset, -Offset), FMath::Max(2.2, Disc * 0.5), CompanionColour());
		}
		if (System.bCentre || System.bHome)
		{
			PaintCircle(Elements, LayerMarks, Geometry, Star, Disc + (bFull ? 10.0 : 9.0), APSChrome::Amber(), bFull ? 1.6f : 1.5f);
		}
		if (!bFull)
		{
			if (System.bClaimed && !System.bHome)
			{
				PaintTriangle(Elements, LayerMarks, Geometry, Star + FVector2D(0.0, -Disc - 12.0), 4.0, LinkColour(), 1.4f, true, &LinkFill);
			}
			continue;
		}
		if (FMath::Abs(System.Slot.Elevation) > APSStarMap::ElevationMark)
		{
			PaintTriangle(Elements, LayerMarks, Geometry, Star + FVector2D(-Disc - 13.0, 0.0), 3.6, WithAlpha(APSChrome::Muted(), 0.85f),
				1.2f, System.Slot.Elevation > 0.0);
		}
		// The civilization's marks above it: claimed, colony, HQ, outposts, an anomaly, its ships in their division's colour.
		MarksOf(System, Marks);
		for (int32 Mark = 0; Mark < Marks.Num(); ++Mark)
		{
			const FVector2D At = MarkAt(Star, Disc, System.bCentre || System.bHome, Mark, Marks.Num());
			switch (Marks[Mark])
			{
			case EMark::Beacon:
				PaintTriangle(Elements, LayerMarks, Geometry, At, 4.6, LinkColour(), 1.6f, true, &LinkFill);
				break;
			case EMark::Colony:
				PaintDot(Elements, LayerMarks, Geometry, At, 2.6, ColonyColour());
				PaintCircle(Elements, LayerMarks, Geometry, At, 5.0, ColonyColour(), 1.2f);
				break;
			case EMark::Headquarters:
			{
				const double Half = 4.4;
				PaintLines(Elements, LayerMarks, Geometry, {At + FVector2D(-Half, -0.6), At + FVector2D(0.0, -Half - 1.4),
					At + FVector2D(Half, -0.6), At + FVector2D(Half, Half), At + FVector2D(-Half, Half), At + FVector2D(-Half, -0.6)},
					HeadquartersColour(), 1.4f);
				break;
			}
			case EMark::Outpost:
				PaintDiamond(Elements, LayerMarks, Geometry, At, 4.4, APSChrome::Amber(), 1.6f);
				break;
			case EMark::Anomaly:
				PaintDiamond(Elements, LayerMarks, Geometry, At, 5.4, APSChrome::Amber(), 1.6f);
				PaintTextMid(Elements, LayerTexts, Geometry, FText::FromString(TEXT("!")), BangFont(), At.X, At.Y, APSChrome::Amber(), 0.5);
				break;
			case EMark::Fleet:
			{
				const FLinearColor Division = APSFleet::DivisionColour(System.ShipsDivision);
				PaintShip(Elements, LayerMarks, Geometry, At + FVector2D(0.0, 0.5), Division, 5.2, -HalfPi, 1.4f);
				PaintTextMid(Elements, LayerTexts, Geometry, APSUINumber::Number(System.Ships), MarkFont(), At.X + 7.0, At.Y - 5.0, Division);
				break;
			}
			default:
				break;
			}
		}
	}

	// Where the pilot is (Full) and where the picked ships are (Picker): white rings.
	if (bFull && Snap.bPilot)
	{
		if (Screen.IsValidIndex(Snap.PilotSystem) && IsDrawn(Snap.Systems[Snap.PilotSystem]))
		{
			PaintCircle(Elements, LayerMarks, Geometry, Screen[Snap.PilotSystem], DiscOf(Snap.Systems[Snap.PilotSystem]) + 15.0,
				APSChrome::White(), 1.4f);
		}
		else
		{
			const FVector2D You = ToScreen(Frame, Snap.PilotPosition);
			PaintDiamond(Elements, LayerMarks, Geometry, You, 5.0, APSChrome::White(), 1.4f);
			PaintTextMid(Elements, LayerTexts, Geometry, LOCTEXT("You", "YOU"), MarkFont(), You.X + 9.0, You.Y, APSChrome::White());
		}
	}
	if (!bFull)
	{
		for (const FShipPlace& Place : HighlightedPlaces)
		{
			if (!Place.bValid)
			{
				continue;
			}
			if (Screen.IsValidIndex(Place.System))
			{
				PaintCircle(Elements, LayerMarks, Geometry, Screen[Place.System], DiscOf(Snap.Systems[Place.System]) + 14.0,
					APSChrome::White(), 1.4f);
			}
			else
			{
				PaintCircle(Elements, LayerMarks, Geometry, ToScreen(Frame, Place.Position), 6.0, APSChrome::White(), 1.4f);
			}
		}
	}

	// The pick: the white ring and the amber brackets of a target; the hovered star: a cyan ring.
	if (bPickDrawn)
	{
		const FSystem& Picked = Snap.Systems[Pick];
		const double Disc = DiscOf(Picked);
		PaintCircle(Elements, LayerMarks, Geometry, Screen[Pick], Disc + (bFull ? 10.0 : 9.0), APSChrome::White(), 1.6f);
		PaintBrackets(Elements, LayerMarks, Geometry, Screen[Pick], Disc + (bFull ? 15.0 : 14.0), APSChrome::Amber(), 1.6f);
		if (!bFull && (Picked.Anomaly == 1 || Picked.Anomaly == 2))
		{
			PaintDiamond(Elements, LayerMarks, Geometry, Screen[Pick] + FVector2D(0.0, -Disc - 20.0), 5.0, APSChrome::Amber(), 1.6f);
		}
	}
	if (Screen.IsValidIndex(Hovered) && Hovered != Pick && IsDrawn(Snap.Systems[Hovered]))
	{
		PaintCircle(Elements, LayerMarks, Geometry, Screen[Hovered], DiscOf(Snap.Systems[Hovered]) + 10.0, APSChrome::Cyan(), 1.2f);
	}

	// Plates and names, as laid out (PlaceLabels).
	for (const FLabel& Label : Labels)
	{
		if (!Snap.Systems.IsValidIndex(Label.System) || !Screen.IsValidIndex(Label.System))
		{
			continue;
		}
		const FSystem& System = Snap.Systems[Label.System];
		const bool bPicked = Label.System == Pick;
		if (Label.Kind == ELabel::Name)
		{
			const FLinearColor Colour = IsUncharted(System) ? WithAlpha(APSChrome::Muted(), 0.75f)
				: !bFull && (System.bHome || System.bCentre) ? APSChrome::Amber() : APSChrome::White();
			PaintTextMid(Elements, LayerTexts, Geometry, FText::FromString(System.Name), NameFont(), Label.Rect.Min.X,
				(Label.Rect.Min.Y + Label.Rect.Max.Y) * 0.5, Colour);
			continue;
		}
		const FLinearColor Marker = bPicked || System.bCentre || System.bHome ? APSChrome::Amber() : System.Colour;
		if (Label.bLeader)
		{
			PaintLeader(Elements, LayerLeaders, Geometry, Screen[Label.System], DiscOf(System) + 8.0, Label.Rect, WithAlpha(Marker, 0.72f));
		}
		const FVector2D TopLeft = Label.Rect.Min;
		const FVector2D PlateSize = Label.Rect.GetSize();
		PaintBox(Elements, LayerPlates, Geometry, Label.Rect, PlateColour());
		PaintBox(Elements, LayerPlates, Geometry, BoxAt(TopLeft.X, TopLeft.Y, APSStrategicMap::PlateBar, PlateSize.Y), Marker);
		if (bPicked)
		{
			PaintOutline(Elements, LayerPlates, Geometry, Label.Rect, APSChrome::Amber(), 2.0f);
		}
		const double TextLeft = APSStrategicMap::PlateBar + APSStrategicMap::PlatePadX;
		PaintText(Elements, LayerTexts, Geometry, APSStarMap::TypeLine(System), APSStrategicMap::PlateTypeFont(),
			TopLeft + FVector2D(TextLeft, Label.Plate.TypeTop), APSChrome::Muted());
		PaintText(Elements, LayerTexts, Geometry, FText::FromString(System.Name), APSStrategicMap::PlateNameFont(),
			TopLeft + FVector2D(TextLeft, Label.Plate.NameTop), bPicked ? APSChrome::Amber() : APSChrome::White());
		PaintText(Elements, LayerTexts, Geometry, FText::FromString(System.Designation), APSStrategicMap::PlateDesignationFont(),
			TopLeft + FVector2D(TextLeft + Label.Plate.NameWidth + APSStrategicMap::PlateDesignationGap, Label.Plate.DesignationTop), Marker);
		// The worlds: hollow dots while only their number is known, in their types' colours once surveyed where they stand.
		const int32 Dots = WorldDots(System);
		for (int32 Dot = 0; Dot < Dots && Label.StripHeight > 0.0; ++Dot)
		{
			const FVector2D At(TopLeft.X + 14.0 + Dot * 10.0, TopLeft.Y + Label.Plate.Size.Y + 3.0);
			if (System.WorldColours.IsValidIndex(Dot))
			{
				PaintDot(Elements, LayerTexts, Geometry, At, 3.2, System.WorldColours[Dot]);
			}
			else
			{
				PaintCircle(Elements, LayerTexts, Geometry, At, 2.8, WithAlpha(APSChrome::Muted(), 0.85f), 1.1f);
			}
		}
	}
	// A star without a label of its own shows its name on hover.
	if (Screen.IsValidIndex(Hovered) && Named.IsValidIndex(Hovered) && !Named[Hovered] && IsDrawn(Snap.Systems[Hovered]))
	{
		const FText Name = FText::FromString(Snap.Systems[Hovered].Name);
		const FVector2D Star = Screen[Hovered];
		const FBox2D Box = BoxAt(Star.X + DiscOf(Snap.Systems[Hovered]) + 12.0, Star.Y - 9.0, MeasureText(Name, NameFont()).X + 10.0, 18.0);
		PaintBox(Elements, LayerPlates, Geometry, Box, PlateColour(), ChipBrush());
		PaintTextMid(Elements, LayerTexts, Geometry, Name, NameFont(), Box.Min.X + 5.0, Star.Y, APSChrome::Cyan());
	}

	// Chips on the routes and round the work, placed now (ships move) into the room the labels left.
	TArray<FBox2D> ChipBoxes;
	const auto ChipFree = [this, &Frame, &ChipBoxes](const FBox2D& Box, const bool bUseSoft)
	{
		if (Box.Min.X < 4.0 || Box.Min.Y < 4.0 || Box.Max.X > Frame.Size.X - 4.0 || Box.Max.Y > Frame.Size.Y - 4.0)
		{
			return false;
		}
		for (const FBox2D& Other : LabelObstacles)
		{
			if (Overlaps(Box, Other, 3.0))
			{
				return false;
			}
		}
		for (const FBox2D& Other : ChipBoxes)
		{
			if (Overlaps(Box, Other, 3.0))
			{
				return false;
			}
		}
		if (bUseSoft)
		{
			for (const FBox2D& Other : SoftObstacles)
			{
				if (Overlaps(Box, Other, 0.0))
				{
					return false;
				}
			}
		}
		return true;
	};
	for (const FChip& Chip : Chips)
	{
		const double Width = MeasureText(Chip.Label, ChipFont()).X + 14.0;
		const double Height = 19.0;
		bool bPlaced = false;
		for (const bool bUseSoft : {true, false})
		{
			for (int32 Anchor = 0; Anchor < Chip.Anchors.Num() && !bPlaced; ++Anchor)
			{
				const FVector2D Point = Chip.Anchors[Anchor];
				const FVector2D Spots[12] = {{Point.X + 12.0, Point.Y - Height - 4.0}, {Point.X + 12.0, Point.Y + 4.0},
					{Point.X - 12.0 - Width, Point.Y - Height - 4.0}, {Point.X - 12.0 - Width, Point.Y + 4.0},
					{Point.X - Width * 0.5, Point.Y + 14.0}, {Point.X - Width * 0.5, Point.Y - Height - 14.0},
					{Point.X + 18.0, Point.Y - Height * 0.5}, {Point.X - 18.0 - Width, Point.Y - Height * 0.5},
					{Point.X - Width * 0.25, Point.Y + 14.0}, {Point.X - Width * 0.75, Point.Y + 14.0},
					{Point.X - Width * 0.25, Point.Y - Height - 14.0}, {Point.X - Width * 0.75, Point.Y - Height - 14.0}};
				for (const FVector2D& Spot : Spots)
				{
					const FBox2D Box = BoxAt(FMath::RoundToDouble(Spot.X), FMath::RoundToDouble(Spot.Y), Width, Height);
					if (!ChipFree(Box, bUseSoft))
					{
						continue;
					}
					ChipBoxes.Add(Box);
					// A chip set off from its ship, its work arc or its route keeps a thin leader to it.
					const FVector2D Nearest(FMath::Clamp(Point.X, Box.Min.X, Box.Max.X), FMath::Clamp(Point.Y, Box.Min.Y, Box.Max.Y));
					if (FVector2D::Distance(Nearest, Point) > 12.0)
					{
						PaintLines(Elements, LayerLeaders, Geometry, {Point, Nearest}, WithAlpha(Chip.Colour, 0.6f), 1.0f);
					}
					PaintBox(Elements, LayerPlates, Geometry, Box, PlateColour(), ChipBrush());
					PaintOutline(Elements, LayerPlates, Geometry, Box, WithAlpha(Chip.Colour, Chip.FrameOpacity), 1.0f);
					PaintTextMid(Elements, LayerTexts, Geometry, Chip.Label, ChipFont(), Box.Min.X + 7.0, (Box.Min.Y + Box.Max.Y) * 0.5,
						Chip.Colour);
					bPlaced = true;
					break;
				}
			}
			if (bPlaced)
			{
				break;
			}
		}
	}

	// Each ring's distance caption on its top: the map's only scale, in whatever unit the world uses.
	for (int32 Ring = 0; Ring <= APSStarMap::OuterRing; ++Ring)
	{
		if (Snap.Rings[Ring].Count == 0)
		{
			continue;
		}
		const FText& Caption = Snap.Rings[Ring].Caption;
		const double Width = MeasureText(Caption, SmallFont()).X + 12.0;
		const double Y = Frame.Centre.Y - APSStarMap::RingRadius[Ring] * Scale;
		PaintBox(Elements, LayerTop, Geometry, BoxAt(Frame.Centre.X - Width * 0.5, Y - 8.0, Width, 16.0), SchemeBackground());
		PaintTextMid(Elements, LayerTop + 1, Geometry, Caption, SmallFont(), Frame.Centre.X, Y, EdgeColour(0.80f), 0.5);
	}

	// The legend and the hints (Rio 05.10: hidden by the LEGEND switch; the map then takes their room).
	if (IsLegendShown())
	{
		const FSlateFontInfo LegendFont = SmallFont();
		if (bFull)
		{
			struct FEntry
			{
				int32 Kind;
				FText Label;
			};
			const TArray<FEntry> Rows[2] = {
				{{0, LOCTEXT("LegendUncharted", "UNCHARTED")}, {1, LOCTEXT("LegendScanned", "SCANNED")},
					{2, LOCTEXT("LegendSurveyed", "SURVEYED")}, {3, LOCTEXT("LegendClaimed", "CLAIMED")},
					{4, LOCTEXT("LegendOutpost", "OUTPOST")}, {5, LOCTEXT("LegendAnomaly", "ANOMALY")},
					{6, LOCTEXT("LegendPlane", "ABOVE / BELOW THE PLANE")}},
				{{7, LOCTEXT("LegendFleet", "FLEET, IN DIVISION COLOUR")}, {8, LOCTEXT("LegendUnderWay", "ORDER UNDER WAY")},
					{9, LOCTEXT("LegendPlanned", "PLANNED ROUTE")}, {10, LOCTEXT("LegendLink", "RELAY LINK")},
					{11, LOCTEXT("LegendHome", "HOME / YOU")}}};
			const auto PaintKey = [&](const int32 Kind, const FVector2D& At)
			{
				switch (Kind)
				{
				case 0:
					PaintDot(Elements, LayerTop, Geometry, At, 3.0, WithAlpha(FLinearColor(1.0f, 0.62f, 0.38f), 0.55f));
					PaintDashedCircle(Elements, LayerTop, Geometry, At, 6.5, WithAlpha(APSChrome::Muted(), 0.6f), 1.0f, 10);
					break;
				case 1:
				case 2:
					PaintDot(Elements, LayerTop, Geometry, At, 3.0, FLinearColor(1.0f, 0.9f, 0.75f));
					PaintCircle(Elements, LayerTop, Geometry, At, 6.5, APSStars::KnowledgeColour(Kind == 1
						? APSStars::EKnowledge::Scanned : APSStars::EKnowledge::Surveyed), 1.4f);
					break;
				case 3:
					PaintTriangle(Elements, LayerTop, Geometry, At, 4.6, LinkColour(), 1.6f, true, &LinkFill);
					break;
				case 4:
					PaintDiamond(Elements, LayerTop, Geometry, At, 4.4, APSChrome::Amber(), 1.6f);
					break;
				case 5:
					PaintDiamond(Elements, LayerTop, Geometry, At, 5.0, APSChrome::Amber(), 1.6f);
					PaintTextMid(Elements, LayerTop + 1, Geometry, FText::FromString(TEXT("!")), BangFont(), At.X, At.Y, APSChrome::Amber(), 0.5);
					break;
				case 6:
					PaintTriangle(Elements, LayerTop, Geometry, At, 3.6, WithAlpha(APSChrome::Muted(), 0.85f), 1.2f, true);
					break;
				case 7:
					PaintShip(Elements, LayerTop, Geometry, At, APSFleet::DivisionColour(APSFleet::EDivision::MainFleet), 5.2, -HalfPi, 1.4f);
					break;
				case 8:
					PaintDashed(Elements, LayerTop, Geometry, {At - FVector2D(7.0, 0.0), At + FVector2D(7.0, 0.0)},
						APSFleet::DivisionColour(APSFleet::EDivision::Exploration), 1.4f, 4.0, 3.0);
					break;
				case 9:
					PaintDashed(Elements, LayerTop, Geometry, {At - FVector2D(7.0, 0.0), At + FVector2D(7.0, 0.0)}, APSChrome::Amber(), 2.0f, 5.0, 3.0);
					break;
				case 10:
					PaintLines(Elements, LayerTop, Geometry, {At - FVector2D(7.0, 0.0), At + FVector2D(7.0, 0.0)}, WithAlpha(LinkColour(), 0.7f), 2.4f);
					break;
				default:
					PaintCircle(Elements, LayerTop, Geometry, At, 6.0, APSChrome::Amber(), 1.4f);
					PaintCircle(Elements, LayerTop, Geometry, At, 9.0, APSChrome::White(), 1.2f);
					break;
				}
			};
			double RowY = Size.Y - 58.0;
			for (const TArray<FEntry>& Row : Rows)
			{
				double X = 12.0;
				for (const FEntry& Entry : Row)
				{
					PaintKey(Entry.Kind, FVector2D(X + 7.0, RowY));
					PaintTextMid(Elements, LayerTop + 1, Geometry, Entry.Label, LegendFont, X + 18.0, RowY, APSChrome::Muted());
					X += 18.0 + MeasureText(Entry.Label, LegendFont).X + 16.0;
				}
				RowY += 19.0;
			}
			PaintTextMid(Elements, LayerTop + 1, Geometry, LOCTEXT("Hint", "ORDER BY DISTANCE, NOT TO SCALE  /  WHEEL ZOOM  /  DRAG TO PAN  /  CLICK: PICK  /  DOUBLE-CLICK: SYSTEM SCHEME"),
				LegendFont, 12.0, Size.Y - 15.0, SoftColour());
		}
		else
		{
			PaintTextMid(Elements, LayerTop + 1, Geometry, LOCTEXT("PickerLegend", "AMBER: THE PICKED SHIPS' ROUTES TO THE TARGET  /  WHITE RING: WHERE THEY ARE"),
				LegendFont, 10.0, Size.Y - 34.0, APSChrome::Muted());
			PaintTextMid(Elements, LayerTop + 1, Geometry, LOCTEXT("PickerHint", "CLICK A STAR: TARGET  /  DOUBLE-CLICK: ITS SYSTEM  /  WHEEL ZOOM  /  DRAG TO PAN"),
				LegendFont, 10.0, Size.Y - 14.0, SoftColour());
		}
	}
	return LayerTop + 1;
}

FReply SAPSStarScheme::OnMouseWheel(const FGeometry& Geometry, const FPointerEvent& Event)
{
	const FVector2D Pointer = Geometry.AbsoluteToLocal(Event.GetScreenSpacePosition());
	const double Previous = Zoom;
	Zoom = FMath::Clamp(Zoom * FMath::Pow(1.2, static_cast<double>(Event.GetWheelDelta())), 0.6, 5.0);
	// The point under the cursor stays: the rings scale about the frame's centre, which the pan moves.
	const FVector2D Middle = LastFrame.Centre - Pan;
	Pan = Pointer - Middle - (Pointer - Middle - Pan) * (Zoom / Previous);
	const double Reach = 0.5 * (LastFrame.Size.X + LastFrame.Size.Y) * Zoom;
	Pan = FVector2D(FMath::Clamp(Pan.X, -Reach, Reach), FMath::Clamp(Pan.Y, -Reach, Reach));
	if (Zoom <= 0.6001 || FMath::IsNearlyEqual(Zoom, 1.0, 1.0e-3))
	{
		// Back at the whole view: centred again.
		Pan = Zoom <= 1.0 ? FVector2D::ZeroVector : Pan;
	}
	return FReply::Handled();
}

FReply SAPSStarScheme::OnMouseButtonDown(const FGeometry& Geometry, const FPointerEvent& Event)
{
	if (Event.GetEffectingButton() != EKeys::LeftMouseButton)
	{
		return FReply::Unhandled();
	}
	bPressed = true;
	bDragged = false;
	PressPosition = Geometry.AbsoluteToLocal(Event.GetScreenSpacePosition());
	PressPan = Pan;
	return FReply::Handled().CaptureMouse(SharedThis(this));
}

FReply SAPSStarScheme::OnMouseMove(const FGeometry& Geometry, const FPointerEvent& Event)
{
	const FVector2D Local = Geometry.AbsoluteToLocal(Event.GetScreenSpacePosition());
	if (bPressed)
	{
		bDragged |= FVector2D::Distance(Local, PressPosition) > 4.0;
		if (bDragged)
		{
			const double Reach = 0.5 * (LastFrame.Size.X + LastFrame.Size.Y) * Zoom;
			const FVector2D Wanted = PressPan + (Local - PressPosition);
			Pan = FVector2D(FMath::Clamp(Wanted.X, -Reach, Reach), FMath::Clamp(Wanted.Y, -Reach, Reach));
			return FReply::Handled();
		}
	}
	Hovered = HitTest(Local);
	return FReply::Unhandled();
}

FReply SAPSStarScheme::OnMouseButtonUp(const FGeometry& Geometry, const FPointerEvent& Event)
{
	if (!bPressed || Event.GetEffectingButton() != EKeys::LeftMouseButton)
	{
		return FReply::Unhandled();
	}
	bPressed = false;
	if (!bDragged)
	{
		const int32 Hit = HitTest(Geometry.AbsoluteToLocal(Event.GetScreenSpacePosition()));
		const APSStarMap::FSnapshot& Snap = Model.Get();
		if (Snap.Systems.IsValidIndex(Hit))
		{
			const FGuid Id = Snap.Systems[Hit].Id;
			SetSelected(Id);
			OnPicked.ExecuteIfBound(Id);
		}
	}
	bDragged = false;
	return FReply::Handled().ReleaseMouseCapture();
}

FReply SAPSStarScheme::OnMouseButtonDoubleClick(const FGeometry& Geometry, const FPointerEvent& Event)
{
	const int32 Hit = HitTest(Geometry.AbsoluteToLocal(Event.GetScreenSpacePosition()));
	const APSStarMap::FSnapshot& Snap = Model.Get();
	if (Snap.Systems.IsValidIndex(Hit))
	{
		const FGuid Id = Snap.Systems[Hit].Id;
		SetSelected(Id);
		OnOpened.ExecuteIfBound(Id);
	}
	return FReply::Handled();
}

void SAPSStarScheme::OnMouseLeave(const FPointerEvent& Event)
{
	SLeafWidget::OnMouseLeave(Event);
	Hovered = INDEX_NONE;
}

FCursorReply SAPSStarScheme::OnCursorQuery(const FGeometry&, const FPointerEvent&) const
{
	return bDragged ? FCursorReply::Cursor(EMouseCursor::GrabHandClosed)
		: Hovered != INDEX_NONE ? FCursorReply::Cursor(EMouseCursor::Hand) : FCursorReply::Unhandled();
}

#undef LOCTEXT_NAMESPACE
