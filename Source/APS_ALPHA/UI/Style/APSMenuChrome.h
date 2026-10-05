#pragma once

#include "CoreMinimal.h"
#include "APS_ALPHA/UI/MainMenu/SAPSChamferedOverlay.h"
#include "Rendering/DrawElements.h"
#include "Styling/AppStyle.h"
#include "Widgets/SLeafWidget.h"

class STextBlock;

/**
 * The main menu's visual language, shared (Rio, 30.09: UI for new mechanics in the project's common style, like the
 * menu). Chamfered cards, badges, line glyphs, section headings and the menu palette, extracted verbatim from
 * SAPSMainMenuRoot so in-game screens (the colony terminal first) look like the menu. The menu keeps its own copies
 * until it is switched over in a separate, coordinated step.
 */

enum class EAPSChromeGlyph : uint8
{
	Compass,
	World,
	Civilization,
	Space,
	Planet,
	Lock,
	Profile,
	Settings,
	Collection,
	Favorite,
	Recent,
	Pilot,
	Ship,
	Station,
	Headquarters,
	Shipyard,
	Fleet,
	Infrastructure,
	Divisions,
	System,
	/** Rio 05.10 (star map): the STAR level, three stars on a faint ring. */
	Stars
};

/** Small code-native line icons keep the Slate-only menu readable before the
 * final art pass, without depending on Unicode coverage or editor-authored
 * image widgets. */
class APS_ALPHA_API SAPSVectorGlyph final : public SLeafWidget
{
public:
	SLATE_BEGIN_ARGS(SAPSVectorGlyph) {}
		SLATE_ARGUMENT(EAPSChromeGlyph, Glyph)
		SLATE_ARGUMENT(FLinearColor, Color)
		SLATE_ARGUMENT(float, StrokeWidth)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs)
	{
		Glyph = InArgs._Glyph;
		Color = InArgs._Color;
		StrokeWidth = InArgs._StrokeWidth;
		SetVisibility(EVisibility::HitTestInvisible);
	}

	virtual FVector2D ComputeDesiredSize(float) const override { return FVector2D(24.0f); }

	virtual int32 OnPaint(const FPaintArgs&, const FGeometry& Geometry,
		const FSlateRect&, FSlateWindowElementList& OutDrawElements, int32 LayerId,
		const FWidgetStyle&, bool) const override
	{
		const FVector2D Size = Geometry.GetLocalSize();
		const FVector2D Center = Size * 0.5f;
		const float Radius = FMath::Max(3.0f, FMath::Min(Size.X, Size.Y) * 0.34f);
		const auto Draw = [&](const TArray<FVector2D>& Points, bool bClosed = false, float WidthScale = 1.0f)
		{
			if (Points.Num() < 2) return;
			TArray<FVector2D> Path = Points;
			if (bClosed) Path.Add(Points[0]);
			FSlateDrawElement::MakeLines(OutDrawElements, LayerId, Geometry.ToPaintGeometry(),
				Path, ESlateDrawEffect::None, Color, true, StrokeWidth * WidthScale);
		};
		const auto Circle = [&](const FVector2D& C, float RX, float RY, float Rotation = 0.0f)
		{
			TArray<FVector2D> Points;
			constexpr int32 Segments = 24;
			const float CosR = FMath::Cos(Rotation);
			const float SinR = FMath::Sin(Rotation);
			for (int32 Index = 0; Index < Segments; ++Index)
			{
				const float Angle = UE_TWO_PI * static_cast<float>(Index) / Segments;
				const FVector2D P(FMath::Cos(Angle) * RX, FMath::Sin(Angle) * RY);
				Points.Add(C + FVector2D(P.X * CosR - P.Y * SinR, P.X * SinR + P.Y * CosR));
			}
			Draw(Points, true);
		};

		switch (Glyph)
		{
		case EAPSChromeGlyph::Compass:
			Circle(Center, Radius * 0.88f, Radius * 0.88f);
			Draw({Center + FVector2D(0.0f, -Radius), Center + FVector2D(Radius * 0.22f, -Radius * 0.18f),
				Center, Center + FVector2D(-Radius * 0.22f, Radius * 0.18f), Center + FVector2D(0.0f, Radius)}, true);
			Draw({Center + FVector2D(-Radius, 0.0f), Center + FVector2D(Radius, 0.0f)}, false, 0.65f);
			break;
		case EAPSChromeGlyph::World:
			Circle(Center, Radius, Radius);
			Circle(Center, Radius * 0.43f, Radius);
			Draw({Center + FVector2D(-Radius, 0.0f), Center + FVector2D(Radius, 0.0f)});
			break;
		case EAPSChromeGlyph::Civilization:
			Circle(Center, Radius * 0.30f, Radius * 0.30f);
			for (int32 Index = 0; Index < 6; ++Index)
			{
				const float Angle = UE_TWO_PI * static_cast<float>(Index) / 6.0f;
				const FVector2D Direction(FMath::Cos(Angle), FMath::Sin(Angle));
				Draw({Center + Direction * Radius * 0.30f, Center + Direction * Radius});
				Circle(Center + Direction * Radius, Radius * 0.13f, Radius * 0.13f);
			}
			break;
		case EAPSChromeGlyph::Space:
			{
				TArray<FVector2D> Spiral;
				for (int32 Index = 0; Index < 30; ++Index)
				{
					const float Alpha = static_cast<float>(Index) / 29.0f;
					const float Angle = Alpha * UE_TWO_PI * 1.75f;
					Spiral.Add(Center + FVector2D(FMath::Cos(Angle), FMath::Sin(Angle)) * Radius * Alpha);
				}
				Draw(Spiral);
				Circle(Center, Radius * 0.12f, Radius * 0.12f);
			}
			break;
		case EAPSChromeGlyph::Planet:
			Circle(Center, Radius * 0.72f, Radius * 0.72f);
			Circle(Center, Radius * 1.18f, Radius * 0.35f, -0.32f);
			break;
		case EAPSChromeGlyph::Lock:
			Circle(Center + FVector2D(0.0f, -Radius * 0.26f), Radius * 0.48f, Radius * 0.55f);
			Draw({Center + FVector2D(-Radius * 0.62f, -Radius * 0.10f), Center + FVector2D(Radius * 0.62f, -Radius * 0.10f),
				Center + FVector2D(Radius * 0.62f, Radius * 0.70f), Center + FVector2D(-Radius * 0.62f, Radius * 0.70f)}, true);
			break;
		case EAPSChromeGlyph::Profile:
			{
				// Head and shoulders spanning -0.76..0.80 of the radius: centred on the box (Rio 03.10: the old full-ellipse
				// body hung 1.3 radii down and the person sat low in its badge).
				Circle(Center + FVector2D(0.0f, -Radius * 0.42f), Radius * 0.34f, Radius * 0.34f);
				TArray<FVector2D> Shoulders;
				for (int32 Index = 0; Index <= 16; ++Index)
				{
					const float Angle = UE_PI + UE_PI * static_cast<float>(Index) / 16.0f;
					Shoulders.Add(Center + FVector2D(FMath::Cos(Angle) * Radius * 0.74f,
						Radius * 0.80f + FMath::Sin(Angle) * Radius * 0.62f));
				}
				Draw(Shoulders, true);
			}
			break;
		case EAPSChromeGlyph::Settings:
			Circle(Center, Radius * 0.42f, Radius * 0.42f);
			for (int32 Index = 0; Index < 8; ++Index)
			{
				const float Angle = UE_TWO_PI * static_cast<float>(Index) / 8.0f;
				const FVector2D D(FMath::Cos(Angle), FMath::Sin(Angle));
				Draw({Center + D * Radius * 0.62f, Center + D * Radius});
			}
			break;
		case EAPSChromeGlyph::Collection:
			for (int32 X = -1; X <= 1; X += 2)
			for (int32 Y = -1; Y <= 1; Y += 2)
			{
				const FVector2D C = Center + FVector2D(X, Y) * Radius * 0.45f;
				Draw({C + FVector2D(-Radius * 0.25f), C + FVector2D(Radius * 0.25f, -Radius * 0.25f),
					C + FVector2D(Radius * 0.25f), C + FVector2D(-Radius * 0.25f, Radius * 0.25f)}, true);
			}
			break;
		case EAPSChromeGlyph::Favorite:
			{
				TArray<FVector2D> Star;
				for (int32 Index = 0; Index < 10; ++Index)
				{
					const float Angle = -UE_PI * 0.5f + UE_PI * static_cast<float>(Index) / 5.0f;
					const float R = Index % 2 == 0 ? Radius : Radius * 0.42f;
					Star.Add(Center + FVector2D(FMath::Cos(Angle), FMath::Sin(Angle)) * R);
				}
				Draw(Star, true);
			}
			break;
		case EAPSChromeGlyph::Recent:
			Circle(Center, Radius, Radius);
			Draw({Center, Center + FVector2D(0.0f, -Radius * 0.58f)});
			Draw({Center, Center + FVector2D(Radius * 0.48f, Radius * 0.28f)});
			break;
		case EAPSChromeGlyph::Pilot:
			Circle(Center + FVector2D(0.0f, -Radius * 0.48f), Radius * 0.32f, Radius * 0.32f);
			Draw({Center + FVector2D(-Radius * 0.68f, Radius * 0.82f),
				Center + FVector2D(-Radius * 0.42f, Radius * 0.10f),
				Center + FVector2D(0.0f, -Radius * 0.02f),
				Center + FVector2D(Radius * 0.42f, Radius * 0.10f),
				Center + FVector2D(Radius * 0.68f, Radius * 0.82f)}, false);
			break;
		case EAPSChromeGlyph::Ship:
			Draw({Center + FVector2D(0.0f, -Radius),
				Center + FVector2D(Radius * 0.62f, Radius * 0.78f), Center,
				Center + FVector2D(-Radius * 0.62f, Radius * 0.78f)}, true);
			Draw({Center + FVector2D(-Radius * 0.24f, Radius * 0.66f),
				Center + FVector2D(-Radius * 0.38f, Radius),
				Center + FVector2D(0.0f, Radius * 0.78f),
				Center + FVector2D(Radius * 0.38f, Radius)});
			break;
		case EAPSChromeGlyph::Station:
			Circle(Center, Radius * 0.38f, Radius * 0.38f);
			Circle(Center, Radius, Radius * 0.38f, -0.22f);
			Draw({Center + FVector2D(0.0f, -Radius), Center + FVector2D(0.0f, Radius)});
			break;
		case EAPSChromeGlyph::Headquarters:
			Draw({Center + FVector2D(0.0f, -Radius), Center + FVector2D(Radius * 0.82f, -Radius * 0.35f),
				Center + FVector2D(Radius * 0.68f, Radius * 0.82f), Center + FVector2D(-Radius * 0.68f, Radius * 0.82f),
				Center + FVector2D(-Radius * 0.82f, -Radius * 0.35f)}, true);
			Draw({Center + FVector2D(-Radius * 0.34f, Radius * 0.82f), Center + FVector2D(-Radius * 0.34f, 0.0f),
				Center + FVector2D(Radius * 0.34f, 0.0f), Center + FVector2D(Radius * 0.34f, Radius * 0.82f)});
			break;
		case EAPSChromeGlyph::Shipyard:
			Draw({Center + FVector2D(-Radius, -Radius), Center + FVector2D(-Radius, Radius),
				Center + FVector2D(-Radius * 0.55f, Radius), Center + FVector2D(-Radius * 0.55f, -Radius * 0.55f),
				Center + FVector2D(Radius * 0.55f, -Radius * 0.55f), Center + FVector2D(Radius * 0.55f, Radius),
				Center + FVector2D(Radius, Radius), Center + FVector2D(Radius, -Radius)});
			Draw({Center + FVector2D(0.0f, -Radius * 0.82f), Center + FVector2D(Radius * 0.28f, Radius * 0.28f),
				Center, Center + Radius * FVector2D(-0.28f, 0.28f)}, true, 0.8f);
			break;
		case EAPSChromeGlyph::Fleet:
			for (int32 Index = -1; Index <= 1; ++Index)
			{
				const FVector2D C = Center + FVector2D(Index * Radius * 0.62f, FMath::Abs(Index) * Radius * 0.32f);
				Draw({C + FVector2D(0.0f, -Radius * 0.58f), C + FVector2D(Radius * 0.28f, Radius * 0.42f),
					C + FVector2D(0.0f, Radius * 0.22f), C + FVector2D(-Radius * 0.28f, Radius * 0.42f)}, true, 0.75f);
			}
			break;
		case EAPSChromeGlyph::Infrastructure:
			Circle(Center, Radius * 0.22f, Radius * 0.22f);
			for (int32 Index = 0; Index < 4; ++Index)
			{
				const float Angle = UE_TWO_PI * static_cast<float>(Index) / 4.0f + UE_PI * 0.25f;
				const FVector2D Direction(FMath::Cos(Angle), FMath::Sin(Angle));
				Draw({Center + Direction * Radius * 0.22f, Center + Direction * Radius * 0.85f});
				Circle(Center + Direction * Radius * 0.85f, Radius * 0.15f, Radius * 0.15f);
			}
			break;
		case EAPSChromeGlyph::Divisions:
			for (int32 Row = -1; Row <= 1; ++Row)
			{
				const float Y = Center.Y + Row * Radius * 0.62f;
				Circle(FVector2D(Center.X - Radius * 0.72f, Y), Radius * 0.12f, Radius * 0.12f);
				Draw({FVector2D(Center.X - Radius * 0.42f, Y), FVector2D(Center.X + Radius, Y)});
			}
			break;
		case EAPSChromeGlyph::System:
			Circle(Center, Radius * 0.20f, Radius * 0.20f);
			Circle(Center, Radius * 0.72f, Radius * 0.46f, -0.28f);
			Circle(Center + FVector2D(Radius * 0.68f, -Radius * 0.23f), Radius * 0.12f, Radius * 0.12f);
			break;
		case EAPSChromeGlyph::Stars:
			Circle(Center, Radius * 0.95f, Radius * 0.95f);
			for (const FVector3f& Spot : {FVector3f(-0.35f, -0.25f, 0.30f), FVector3f(0.42f, 0.05f, 0.22f),
				FVector3f(-0.05f, 0.48f, 0.16f)})
			{
				TArray<FVector2D> Star;
				for (int32 Index = 0; Index < 8; ++Index)
				{
					const float Angle = -UE_PI * 0.5f + UE_PI * static_cast<float>(Index) / 4.0f;
					const float R = Radius * Spot.Z * (Index % 2 == 0 ? 1.0f : 0.35f);
					Star.Add(Center + FVector2D(Spot.X * Radius + FMath::Cos(Angle) * R, Spot.Y * Radius + FMath::Sin(Angle) * R));
				}
				Draw(Star, true, 0.9f);
			}
			break;
		}
		return LayerId;
	}

private:
	EAPSChromeGlyph Glyph{EAPSChromeGlyph::Compass};
	FLinearColor Color{FLinearColor::White};
	float StrokeWidth{1.35f};
};

// One fixed cut in Slate units keeps adjacent cards geometrically identical.
// Deriving it from each card's height made large and compact neighbours expose
// different corner wedges even when their perimeter stroke had the same width.
// Shared with astronomical panels and the full child-content stencil.

class APS_ALPHA_API SAPSChamferedFrame final : public SLeafWidget
{
public:
	SLATE_BEGIN_ARGS(SAPSChamferedFrame) {}
		SLATE_ATTRIBUTE(FLinearColor, Color)
		SLATE_ARGUMENT(float, Thickness)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs)
	{
		Color = InArgs._Color;
		Thickness = InArgs._Thickness;
		SetVisibility(EVisibility::HitTestInvisible);
	}

	virtual FVector2D ComputeDesiredSize(float) const override { return FVector2D::ZeroVector; }

	virtual int32 OnPaint(const FPaintArgs&, const FGeometry& Geometry,
		const FSlateRect&, FSlateWindowElementList& OutDrawElements, int32 LayerId,
		const FWidgetStyle&, bool) const override
	{
		const FVector2D Size = Geometry.GetLocalSize();
		if (Size.X <= 4.0f || Size.Y <= 4.0f)
		{
			return LayerId;
		}
		const FSlateBrush* Brush = FAppStyle::GetBrush("WhiteBrush");
		const FSlateResourceHandle ResourceHandle = Brush->GetRenderingResource();
		const float Cut = APSChamfer::Cut(Size);
		const float Inset = FMath::Clamp(Thickness, 0.75f,
			FMath::Min(Size.X, Size.Y) * 0.24f);
		const float InnerCut = Cut + Inset * 0.41421356f;
		const TArray<FVector2f> Points = {
			FVector2f(Cut, 0.0f), FVector2f(Size.X - Cut, 0.0f),
			FVector2f(Size.X, Cut), FVector2f(Size.X, Size.Y - Cut),
			FVector2f(Size.X - Cut, Size.Y), FVector2f(Cut, Size.Y),
			FVector2f(0.0f, Size.Y - Cut), FVector2f(0.0f, Cut),
			FVector2f(InnerCut, Inset), FVector2f(Size.X - InnerCut, Inset),
			FVector2f(Size.X - Inset, InnerCut), FVector2f(Size.X - Inset, Size.Y - InnerCut),
			FVector2f(Size.X - InnerCut, Size.Y - Inset), FVector2f(InnerCut, Size.Y - Inset),
			FVector2f(Inset, Size.Y - InnerCut), FVector2f(Inset, InnerCut)
		};
		const FColor VertexColor = Color.Get(FLinearColor::White).ToFColor(true);
		TArray<FSlateVertex> Vertices;
		Vertices.Reserve(Points.Num());
		for (const FVector2f& Point : Points)
		{
			Vertices.Add(FSlateVertex::Make<ESlateVertexRounding::Disabled>(
				Geometry.GetAccumulatedRenderTransform(), Point,
				FVector2f::ZeroVector, VertexColor));
		}
		TArray<SlateIndex> Indices;
		Indices.Reserve(48);
		for (SlateIndex Edge = 0; Edge < 8; ++Edge)
		{
			const SlateIndex Next = (Edge + 1) % 8;
			Indices.Add(Edge);
			Indices.Add(Next);
			Indices.Add(static_cast<SlateIndex>(8 + Next));
			Indices.Add(Edge);
			Indices.Add(static_cast<SlateIndex>(8 + Next));
			Indices.Add(static_cast<SlateIndex>(8 + Edge));
		}
		FSlateDrawElement::MakeCustomVerts(OutDrawElements, LayerId, ResourceHandle,
			Vertices, Indices, nullptr, 0, 0, ESlateDrawEffect::None);
		return LayerId;
	}

private:
	TAttribute<FLinearColor> Color{FLinearColor::White};
	float Thickness{1.0f};
};

/** A real filled chamfered polygon. Unlike a rectangular SBorder with a
 * chamfer outline on top, this leaves the cut corners genuinely empty. */
class APS_ALPHA_API SAPSChamferedSurface final : public SLeafWidget
{
public:
	SLATE_BEGIN_ARGS(SAPSChamferedSurface) {}
		SLATE_ARGUMENT(const FSlateBrush*, Brush)
		SLATE_ATTRIBUTE(FLinearColor, Tint)
		SLATE_ARGUMENT(bool, ChamferTop)
		SLATE_ARGUMENT(bool, ChamferBottom)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs)
	{
		Brush = InArgs._Brush;
		Tint = InArgs._Tint;
		bChamferTop = InArgs._ChamferTop;
		bChamferBottom = InArgs._ChamferBottom;
		SetVisibility(EVisibility::HitTestInvisible);
	}

	virtual FVector2D ComputeDesiredSize(float) const override { return FVector2D::ZeroVector; }

	virtual int32 OnPaint(const FPaintArgs&, const FGeometry& Geometry,
		const FSlateRect&, FSlateWindowElementList& OutDrawElements, int32 LayerId,
		const FWidgetStyle&, bool) const override
	{
		const FVector2D Size = Geometry.GetLocalSize();
		if (!Brush || Size.X <= 4.0f || Size.Y <= 4.0f)
		{
			return LayerId;
		}

		const float Cut = APSChamfer::Cut(Size);
		const float TopCut = bChamferTop ? Cut : 0.0f;
		const float BottomCut = bChamferBottom ? Cut : 0.0f;
		const TArray<FVector2f> LocalPoints = {
			FVector2f(Size.X * 0.5f, Size.Y * 0.5f),
			FVector2f(TopCut, 0.0f), FVector2f(Size.X - TopCut, 0.0f),
			FVector2f(Size.X, TopCut), FVector2f(Size.X, Size.Y - BottomCut),
			FVector2f(Size.X - BottomCut, Size.Y), FVector2f(BottomCut, Size.Y),
			FVector2f(0.0f, Size.Y - BottomCut), FVector2f(0.0f, TopCut)
		};

		const FSlateResourceHandle ResourceHandle = Brush->GetRenderingResource();
		const FSlateShaderResourceProxy* ResourceProxy = ResourceHandle.GetResourceProxy();
		const FVector2f UVOrigin = ResourceProxy ? FVector2f(ResourceProxy->StartUV) : FVector2f::ZeroVector;
		const FVector2f UVSize = ResourceProxy ? FVector2f(ResourceProxy->SizeUV) : FVector2f(1.0f, 1.0f);
		const FColor VertexColor = Tint.Get(FLinearColor::White).ToFColor(true);
		TArray<FSlateVertex> Vertices;
		Vertices.Reserve(LocalPoints.Num());
		for (const FVector2f& Point : LocalPoints)
		{
			const FVector2f Normalized(Point.X / static_cast<float>(Size.X), Point.Y / static_cast<float>(Size.Y));
			Vertices.Add(FSlateVertex::Make<ESlateVertexRounding::Disabled>(
				Geometry.GetAccumulatedRenderTransform(), Point, UVOrigin + Normalized * UVSize, VertexColor));
		}

		TArray<SlateIndex> Indices;
		Indices.Reserve(24);
		for (SlateIndex Edge = 1; Edge <= 8; ++Edge)
		{
			Indices.Add(0);
			Indices.Add(Edge);
			Indices.Add(Edge == 8 ? 1 : Edge + 1);
		}
		FSlateDrawElement::MakeCustomVerts(OutDrawElements, LayerId, ResourceHandle,
			Vertices, Indices, nullptr, 0, 0, ESlateDrawEffect::None);
		return LayerId;
	}

private:
	const FSlateBrush* Brush{nullptr};
	TAttribute<FLinearColor> Tint{FLinearColor::White};
	bool bChamferTop{true};
	bool bChamferBottom{true};
};

namespace APSChrome
{
	/** The main menu palette (SAPSMainMenuRoot, APSMenu). */
	APS_ALPHA_API FLinearColor Panel();
	APS_ALPHA_API FLinearColor PanelSoft();
	APS_ALPHA_API FLinearColor Cyan();
	APS_ALPHA_API FLinearColor CyanDim();
	APS_ALPHA_API FLinearColor Amber();
	APS_ALPHA_API FLinearColor White();
	APS_ALPHA_API FLinearColor Muted();
	APS_ALPHA_API FLinearColor Success();
	APS_ALPHA_API FLinearColor Scrim();

	/** Orbitron for "Bold" display text, the engine font for body text, as in the menu. */
	APS_ALPHA_API FSlateFontInfo Font(FName Typeface, int32 Size);

	/**
	 * Rio 03.10 ("everywhere the text strictly centred by height and width"). Slate centres a text's line box, not its
	 * letters: the box is ascender - descender tall and the baseline sits one ascender below its top (hhea metrics, read
	 * from the font files). Orbitron (1000 units: ascender 750, descender -250, cap height 720) keeps its capitals 0.03 em
	 * under the top of the box and 0.25 em over its bottom, so an upper-case label reads 0.11 em high; Roboto (2048 units:
	 * 1900, -500, cap height 1456) only 0.014 em. Returns the shift that centres capitals of this font, in Slate units,
	 * positive downwards.
	 */
	APS_ALPHA_API float CapsCenterOffset(const FSlateFontInfo& Font);

	/** CapsCenterOffset as a render translation: the glyphs move, the layout does not. Use on a centred label:
	 * SNew(STextBlock).Font(F).Justification(ETextJustify::Center).RenderTransform(APSChrome::CapsCenterShift(F)) */
	APS_ALPHA_API TOptional<FSlateRenderTransform> CapsCenterShift(const FSlateFontInfo& Font);

	/** The same for a label of symbols alone (< > + - v x): they sit on the x-height's middle, 0.07 em under the
	 * capitals' in Orbitron (0.04 em down in its box; Roboto's sit 0.08 em low, so they move up). */
	APS_ALPHA_API float SymbolCenterOffset(const FSlateFontInfo& Font);
	APS_ALPHA_API TOptional<FSlateRenderTransform> SymbolCenterShift(const FSlateFontInfo& Font);

	/** An upper-case label centred both ways in its box: centred justification plus CapsCenterShift. Put it in a slot
	 * (or a button) that centres it, e.g. SButton.HAlign(HAlign_Center).VAlign(VAlign_Center). */
	APS_ALPHA_API TSharedRef<STextBlock> CenteredLabel(const TAttribute<FText>& Text, const FSlateFontInfo& Font,
		const TAttribute<FSlateColor>& Color);

	/** The menu's button padding for a label in this font: Horizontal (at least 14) each side and the same air above and
	 * below the line at every size (10 pt: 11, 15 pt: 14, 18 pt: 16 Slate units). */
	APS_ALPHA_API FMargin ButtonPadding(const FSlateFontInfo& Font, float Horizontal = 16.0f);

	/** A filled chamfered card with a thin accent frame. */
	APS_ALPHA_API TSharedRef<SWidget> ChamferPanel(TSharedRef<SWidget> Content, const FMargin& Padding,
		const FLinearColor& Accent, float Thickness = 1.0f);
	APS_ALPHA_API TSharedRef<SWidget> Badge(const FText& Glyph, const FLinearColor& Accent, float Size = 34.0f);
	APS_ALPHA_API TSharedRef<SWidget> IconBadge(EAPSChromeGlyph Glyph, const FLinearColor& Accent, float Size = 34.0f);
	APS_ALPHA_API TSharedRef<SWidget> IconSectionHeading(EAPSChromeGlyph Glyph, const FText& Title,
		const FText& Subtitle = FText::GetEmpty());
	/** A keyboard shortcut chip (F10, TAB, ESC): the text sets its width. */
	APS_ALPHA_API TSharedRef<SWidget> KeyChip(const FText& Key, const FLinearColor& Accent);
	/** Label over a value, the menu's metric tile. */
	APS_ALPHA_API TSharedRef<SWidget> MetricTile(const TAttribute<FText>& Label, const TAttribute<FText>& Value,
		const FLinearColor& Accent);
}
