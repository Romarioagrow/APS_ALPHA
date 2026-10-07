#pragma once

#include "CoreMinimal.h"
#include "Rendering/DrawElements.h"
#include "Layout/Geometry.h"
#include "APS_ALPHA/UI/Style/APSSlateLineGuard.h"
#include "APS_ALPHA/UI/Style/APSUITheme.h"

/**
 * Rio 06.10 (strategic map: "the Home System orbits are still the old ones, make them as on the generation screen"):
 * the generation preview's orbit stroke as a shared piece. A dark under-stroke keeps the line apart from the star band,
 * then a soft glow and a bright core; cyan for planets, blue for moons, brighter towards the body.
 *
 * Values lifted verbatim from SWorldGenerationPanel.cpp SPreviewSystemOverlay FlushOrbit (Rio 06.10 "strengthen the
 * orbits"); the menu keeps its own copy for now.
 *
 * An orbit is cut into short runs (Begin, then Add per visible projected segment, Flush where the curve breaks), so the
 * brightness can follow the angle to the body; Paint draws every run's under-stroke first and then the glows and cores,
 * so a crossing orbit's dark stroke never cuts another orbit's core.
 */
namespace APSOrbitStroke
{
	struct FStyle
	{
		FLinearColor Colour{FLinearColor::White};
		float UnderAlpha{0.f};
		float UnderThickness{1.f};
		float GlowThickness{1.f};
		float CoreThickness{1.f};
		/** The core's alpha far from the body, and what is added towards it. */
		float BaseAlpha{0.f};
		float NearAlpha{0.f};
	};

	/** A planet's orbit round its star: the theme's highlight (the generation screen's Cyan). */
	inline FStyle Planet(const FLinearColor& Cyan)
	{
		return {FLinearColor(Cyan.R, Cyan.G, Cyan.B, 1.f), 0.6f, 4.6f, 4.4f, 1.6f, 0.55f, 0.40f};
	}

	/** A moon's orbit round its planet. */
	inline FStyle Moon()
	{
		return {FLinearColor(0.36f, 0.65f, 1.0f, 1.0f), 0.45f, 3.4f, 3.0f, 1.0f, 0.42f, 0.30f};
	}

	class FRuns
	{
	public:
		struct FRun
		{
			TArray<FVector2f> Points;
			FStyle Style;
			float Alpha{0.f};
			float CoreBoost{0.f};
			float HaloScale{1.f};
		};

		/** Starts an orbit: its style, the body's angle in the orbit plane, the selection's core boost, the halo's share. */
		void Begin(const FStyle& InStyle, const double InBodyAngle, const float InCoreBoost = 0.f, const float InHaloScale = 1.f,
			const int32 InMaxRunPoints = 7)
		{
			Points.Reset();
			Sum = 0.0;
			Count = 0;
			Style = InStyle;
			BodyAngle = InBodyAngle;
			CoreBoost = InCoreBoost;
			HaloScale = InHaloScale;
			MaxRunPoints = FMath::Max(InMaxRunPoints, 2);
		}

		/**
		 * One visible projected piece of the orbit, SampleAngle being the orbit angle of its sample. A piece that does not
		 * start where the run ends opens a new run; bCut closes the run after it (an occluder or the panel's edge cuts it).
		 */
		void Add(const FVector2D& Start, const FVector2D& End, const double SampleAngle, const bool bCut, const double JoinTolerance)
		{
			const FVector2f Start2f(Start);
			if (!Points.IsEmpty() && !Points.Last().Equals(Start2f, static_cast<float>(JoinTolerance)))
			{
				Flush();
			}
			if (Points.IsEmpty())
			{
				Points.Add(Start2f);
			}
			Points.Add(FVector2f(End));
			// Short runs, so the brightness can follow the angle to the body (the sum keeps the angles continuous across
			// the turn's seam).
			const double Unwrapped = Count > 0
				? SampleAngle + UE_DOUBLE_TWO_PI * FMath::RoundToDouble((Sum / Count - SampleAngle) / UE_DOUBLE_TWO_PI)
				: SampleAngle;
			Sum += Unwrapped;
			++Count;
			if (bCut)
			{
				Flush();
			}
			else if (Points.Num() >= MaxRunPoints)
			{
				const FVector2f Joint = Points.Last();
				Flush();
				Points.Add(Joint);
			}
		}

		/** Closes the current run (kept when it has a line to draw). */
		void Flush()
		{
			if (Points.Num() > 1)
			{
				const double Middle = Count > 0 ? Sum / Count - BodyAngle : UE_DOUBLE_PI;
				const float Near = FMath::Pow(0.5f + 0.5f * static_cast<float>(FMath::Cos(Middle)), 1.6f);
				FRun& Run = Runs.AddDefaulted_GetRef();
				Run.Points = MoveTemp(Points);
				Run.Style = Style;
				Run.Alpha = Style.BaseAlpha + Style.NearAlpha * Near;
				Run.CoreBoost = CoreBoost;
				Run.HaloScale = HaloScale;
			}
			Points.Reset();
			Sum = 0.0;
			Count = 0;
		}

		/**
		 * Draws every stored run (all under-strokes first, then glow and core per run) and forgets them. With one layer for
		 * both, the order holds through Slate's batching (lines of one width on one layer join the first batch sent there)
		 * only while no line of these widths was sent on that layer before Paint.
		 */
		void Paint(FSlateWindowElementList& Out, const FGeometry& Geometry, const int32 UnderLayer, const int32 CoreLayer)
		{
			const FPaintGeometry PaintGeometry = Geometry.ToPaintGeometry();
			for (const FRun& Run : Runs)
			{
				if (Run.HaloScale > 0.f && APSSlateLineGuard::IsDrawable(Run.Points))
				{
					FSlateDrawElement::MakeLines(Out, UnderLayer, PaintGeometry, Run.Points, ESlateDrawEffect::None,
						APSUITheme::Retint(FLinearColor(0.0f, 0.008f, 0.016f, Run.Style.UnderAlpha * Run.HaloScale)), true,
						Run.Style.UnderThickness);
				}
			}
			for (FRun& Run : Runs)
			{
				if (!APSSlateLineGuard::IsDrawable(Run.Points))
				{
					continue;
				}
				const FLinearColor& C = Run.Style.Colour;
				if (Run.HaloScale > 0.f)
				{
					FSlateDrawElement::MakeLines(Out, CoreLayer, PaintGeometry, Run.Points, ESlateDrawEffect::None,
						FLinearColor(C.R, C.G, C.B, Run.Alpha * 0.2f * Run.HaloScale), true, Run.Style.GlowThickness);
				}
				FSlateDrawElement::MakeLines(Out, CoreLayer, PaintGeometry, MoveTemp(Run.Points), ESlateDrawEffect::None,
					FLinearColor(C.R, C.G, C.B, FMath::Min(1.f, Run.Alpha + Run.CoreBoost)), true,
					Run.Style.CoreThickness + Run.CoreBoost * 2.4f);
			}
			Reset();
		}

		void Reset()
		{
			Runs.Reset();
			Points.Reset();
			Sum = 0.0;
			Count = 0;
		}

	private:
		TArray<FRun> Runs;
		TArray<FVector2f> Points;
		FStyle Style;
		double BodyAngle{0.0};
		double Sum{0.0};
		int32 Count{0};
		float CoreBoost{0.f};
		float HaloScale{1.f};
		int32 MaxRunPoints{7};
	};
}
