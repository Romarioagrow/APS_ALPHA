#pragma once

#include "CoreMinimal.h"
#include "Rendering/DrawElements.h"
#include "Widgets/SLeafWidget.h"

/** LIVE MODEL card of the generation menu (Rio, 02.10: icons, headings apart from values, values highlighted, units). */
enum class EAPSModelGlyph : uint8
{
	Galaxy,
	Cluster,
	System,
	Star,
	Planet,
	Moon,
	Radius,
	Temperature,
	Light,
	Mass,
	Orbit,
	Count,
	Life,
	Surface,
	Atmosphere,
	Type,
	Spectrum,
	Scale,
	Home
};

struct FAPSModelFact
{
	EAPSModelGlyph Glyph{EAPSModelGlyph::Type};
	FText Label;
	FText Value;
	FText Unit;
	/** A second reading under the value, e.g. kilometres under solar radii. */
	FText Note;
	/** Amber instead of cyan: the fact that matters most for this scope. */
	bool bAccent{false};
};

struct FAPSModelCard
{
	EAPSModelGlyph Glyph{EAPSModelGlyph::System};
	/** STAR, PLANET, MOON, STAR SYSTEM, ... */
	FText Kind;
	/** A, A1, A5.04 (APSBodyDesignation); empty above body level. */
	FText Designation;
	FText Title;
	FText Subtitle;
	TArray<FAPSModelFact> Facts;
};

/** Code-drawn line icons for the card, in the style of SAPSVectorGlyph. */
class SAPSModelGlyph final : public SLeafWidget
{
public:
	SLATE_BEGIN_ARGS(SAPSModelGlyph) : _Glyph(EAPSModelGlyph::Type), _Color(FLinearColor::White), _Size(16.0f) {}
		SLATE_ARGUMENT(EAPSModelGlyph, Glyph)
		SLATE_ARGUMENT(FLinearColor, Color)
		SLATE_ARGUMENT(float, Size)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs)
	{
		Glyph = InArgs._Glyph;
		Color = InArgs._Color;
		Size = InArgs._Size;
		SetVisibility(EVisibility::HitTestInvisible);
	}

	virtual FVector2D ComputeDesiredSize(float) const override { return FVector2D(Size); }

	virtual int32 OnPaint(const FPaintArgs&, const FGeometry& Geometry, const FSlateRect&,
		FSlateWindowElementList& Elements, int32 LayerId, const FWidgetStyle&, bool) const override
	{
		const FVector2D Box = Geometry.GetLocalSize();
		const FVector2D C = Box * 0.5f;
		const float R = FMath::Max(3.0f, FMath::Min(Box.X, Box.Y) * 0.40f);
		const float Width = FMath::Clamp(R * 0.17f, 1.0f, 1.6f);
		const auto Line = [&](const TArray<FVector2D>& Points, const bool bClosed = false)
		{
			if (Points.Num() < 2) return;
			TArray<FVector2D> Path = Points;
			if (bClosed) Path.Add(Points[0]);
			FSlateDrawElement::MakeLines(Elements, LayerId, Geometry.ToPaintGeometry(), Path,
				ESlateDrawEffect::None, Color, true, Width);
		};
		const auto Ellipse = [&](const FVector2D& Center, const float RX, const float RY, const float Rotation = 0.0f,
			const float From = 0.0f, const float To = UE_TWO_PI)
		{
			TArray<FVector2D> Points;
			const float CosR = FMath::Cos(Rotation);
			const float SinR = FMath::Sin(Rotation);
			constexpr int32 Segments = 22;
			for (int32 Index = 0; Index <= Segments; ++Index)
			{
				const float Angle = FMath::Lerp(From, To, static_cast<float>(Index) / Segments);
				const FVector2D P(FMath::Cos(Angle) * RX, FMath::Sin(Angle) * RY);
				Points.Add(Center + FVector2D(P.X * CosR - P.Y * SinR, P.X * SinR + P.Y * CosR));
			}
			Line(Points);
		};
		const auto Circle = [&](const FVector2D& Center, const float Radius) { Ellipse(Center, Radius, Radius); };
		const auto P = [&](const float X, const float Y) { return C + FVector2D(X, Y) * R; };

		switch (Glyph)
		{
		case EAPSModelGlyph::Galaxy:
		{
			TArray<FVector2D> Spiral;
			for (int32 Index = 0; Index < 26; ++Index)
			{
				const float Alpha = static_cast<float>(Index) / 25.0f;
				const float Angle = Alpha * UE_TWO_PI * 1.6f;
				Spiral.Add(C + FVector2D(FMath::Cos(Angle), FMath::Sin(Angle) * 0.62f) * R * Alpha);
			}
			Line(Spiral);
			Circle(C, R * 0.14f);
			break;
		}
		case EAPSModelGlyph::Cluster:
			Circle(P(-0.45f, -0.30f), R * 0.20f);
			Circle(P(0.42f, -0.42f), R * 0.15f);
			Circle(P(0.05f, 0.10f), R * 0.24f);
			Circle(P(-0.50f, 0.55f), R * 0.13f);
			Circle(P(0.55f, 0.50f), R * 0.17f);
			break;
		case EAPSModelGlyph::System:
			Circle(C, R * 0.20f);
			Ellipse(C, R, R * 0.48f, -0.25f);
			Circle(P(0.86f, -0.47f), R * 0.13f);
			break;
		case EAPSModelGlyph::Star:
			Circle(C, R * 0.42f);
			for (int32 Index = 0; Index < 8; ++Index)
			{
				const float Angle = UE_TWO_PI * Index / 8.0f;
				const FVector2D D(FMath::Cos(Angle), FMath::Sin(Angle));
				Line({C + D * R * 0.62f, C + D * R * (Index % 2 == 0 ? 1.0f : 0.82f)});
			}
			break;
		case EAPSModelGlyph::Planet:
			Circle(C, R * 0.62f);
			Ellipse(C, R * 1.05f, R * 0.30f, -0.35f, 0.15f * UE_PI, 0.85f * UE_PI);
			Ellipse(C, R * 1.05f, R * 0.30f, -0.35f, 1.12f * UE_PI, 1.88f * UE_PI);
			break;
		case EAPSModelGlyph::Moon:
			Ellipse(C, R * 0.8f, R * 0.8f, 0.0f, 0.35f * UE_PI, 1.65f * UE_PI);
			Ellipse(P(0.30f, 0.0f), R * 0.62f, R * 0.62f, 0.0f, 0.55f * UE_PI, 1.45f * UE_PI);
			break;
		case EAPSModelGlyph::Radius:
			Circle(C, R * 0.92f);
			Line({C, P(0.92f, 0.0f)});
			Circle(C, R * 0.10f);
			break;
		case EAPSModelGlyph::Temperature:
			Line({P(-0.18f, 0.35f), P(-0.18f, -0.80f), P(0.0f, -0.98f), P(0.18f, -0.80f), P(0.18f, 0.35f)});
			Circle(P(0.0f, 0.62f), R * 0.34f);
			Line({P(0.35f, -0.55f), P(0.62f, -0.55f)});
			Line({P(0.35f, -0.20f), P(0.55f, -0.20f)});
			break;
		case EAPSModelGlyph::Light:
			Circle(C, R * 0.30f);
			for (int32 Index = 0; Index < 6; ++Index)
			{
				const float Angle = UE_TWO_PI * Index / 6.0f;
				const FVector2D D(FMath::Cos(Angle), FMath::Sin(Angle));
				Line({C + D * R * 0.52f, C + D * R * 0.95f});
			}
			break;
		case EAPSModelGlyph::Mass:
			Line({P(-0.80f, 0.85f), P(-0.50f, -0.35f), P(0.50f, -0.35f), P(0.80f, 0.85f)}, true);
			Circle(P(0.0f, -0.62f), R * 0.24f);
			break;
		case EAPSModelGlyph::Orbit:
			Ellipse(C, R, R * 0.55f, 0.3f);
			Circle(C, R * 0.16f);
			Circle(P(-0.62f, -0.70f), R * 0.15f);
			break;
		case EAPSModelGlyph::Count:
			Line({P(-0.80f, -0.55f), P(0.80f, -0.55f)});
			Line({P(-0.80f, 0.0f), P(0.80f, 0.0f)});
			Line({P(-0.80f, 0.55f), P(0.80f, 0.55f)});
			Line({P(-0.35f, -0.95f), P(-0.50f, 0.95f)});
			Line({P(0.35f, -0.95f), P(0.20f, 0.95f)});
			break;
		case EAPSModelGlyph::Life:
			Ellipse(P(0.0f, 0.0f), R * 0.95f, R * 0.48f, -0.78f);
			Line({P(-0.62f, 0.62f), P(0.55f, -0.55f)});
			break;
		case EAPSModelGlyph::Surface:
			Line({P(-1.0f, 0.75f), P(-0.45f, -0.25f), P(-0.15f, 0.25f), P(0.30f, -0.70f), P(1.0f, 0.75f)}, true);
			break;
		case EAPSModelGlyph::Atmosphere:
			Ellipse(P(0.0f, 0.45f), R * 0.55f, R * 0.55f, 0.0f, UE_PI, UE_TWO_PI);
			Ellipse(P(0.0f, 0.45f), R * 0.95f, R * 0.95f, 0.0f, UE_PI * 1.08f, UE_PI * 1.92f);
			Line({P(-1.0f, 0.45f), P(1.0f, 0.45f)});
			break;
		case EAPSModelGlyph::Spectrum:
			Line({P(-0.95f, 0.75f), P(0.0f, -0.85f), P(0.95f, 0.75f)}, true);
			Line({P(-0.30f, -0.05f), P(0.95f, 0.15f)});
			break;
		case EAPSModelGlyph::Scale:
			Line({P(-1.0f, -0.35f), P(1.0f, -0.35f), P(1.0f, 0.35f), P(-1.0f, 0.35f)}, true);
			for (int32 Index = -2; Index <= 2; ++Index)
			{
				Line({P(Index * 0.38f, -0.35f), P(Index * 0.38f, Index % 2 == 0 ? 0.05f : -0.10f)});
			}
			break;
		case EAPSModelGlyph::Home:
			Line({P(-0.80f, -0.05f), P(0.0f, -0.85f), P(0.80f, -0.05f)});
			Line({P(-0.60f, -0.20f), P(-0.60f, 0.85f), P(0.60f, 0.85f), P(0.60f, -0.20f)});
			Line({P(-0.18f, 0.85f), P(-0.18f, 0.30f), P(0.18f, 0.30f), P(0.18f, 0.85f)});
			break;
		case EAPSModelGlyph::Type:
		default:
			Line({P(0.0f, -0.95f), P(0.80f, 0.0f), P(0.0f, 0.95f), P(-0.80f, 0.0f)}, true);
			Circle(C, R * 0.16f);
			break;
		}
		return LayerId;
	}

private:
	EAPSModelGlyph Glyph{EAPSModelGlyph::Type};
	FLinearColor Color{FLinearColor::White};
	float Size{16.0f};
};
