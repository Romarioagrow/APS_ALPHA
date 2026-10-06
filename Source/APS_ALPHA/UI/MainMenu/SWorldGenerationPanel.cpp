#include "SWorldGenerationPanel.h"
#include "APS_ALPHA/UI/Style/APSUINumber.h"
#include "SAPSChamferedOverlay.h"
#include "APSPreviewAnnotationLayout.h"
#include "APSGenerationModelCard.h"
#include "APS_ALPHA/Actors/Astro/APSBodyDesignation.h"
#include "APSAtmosphereColorControl.h"
#include "APSAtmosphereControlBounds.h"
#include "ProfilingDebugging/CsvProfiler.h"

CSV_DECLARE_CATEGORY_EXTERN(APSPreview);

#include "APS_ALPHA/Core/Enums/AstroGenerationLevel.h"
#include "APS_ALPHA/Core/Enums/GalaxyClass.h"
#include "APS_ALPHA/Core/Enums/GalaxyType.h"
#include "APS_ALPHA/Generation/APSGalaxyMorphology.h"
#include "APS_ALPHA/Core/Enums/OrbitDistributionType.h"
#include "APS_ALPHA/Core/Enums/PlanetHabitability.h"
#include "APS_ALPHA/Core/Enums/PlanetarySystemType.h"
#include "APS_ALPHA/Core/Enums/PlanetType.h"
#include "APS_ALPHA/Core/Enums/StarClusterComposition.h"
#include "APS_ALPHA/Core/Enums/StarClusterPopulation.h"
#include "APS_ALPHA/Core/Enums/StarClusterSize.h"
#include "APS_ALPHA/Core/Enums/StarClusterType.h"
#include "APS_ALPHA/Core/Enums/StarSpectralClass.h"
#include "APS_ALPHA/Core/Enums/StellarType.h"
#include "APS_ALPHA/Core/Model/GeneratedWorld.h"
#include "APS_ALPHA/Core/Planetary/APSPlanetSurfaceProfile.h"
#include "APS_ALPHA/Actors/Astro/Moon.h"
#include "APS_ALPHA/Actors/Astro/Planet.h"
#include "APS_ALPHA/Actors/Astro/PlanetOrbit.h"
#include "APS_ALPHA/Core/Rendering/APSPreviewVisibility.h"
#include "APS_ALPHA/Actors/Astro/Star.h"
#include "APS_ALPHA/Actors/Astro/StarSystem.h"
#include "APS_ALPHA/UI/MainMenu/WorldGenerationViewModel.h"
#include "APS_ALPHA/UI/Style/APSUIStyle.h"
#include "APS_ALPHA/UI/Style/APSMenuChrome.h"
#include "APS_ALPHA/UI/Style/APSUITheme.h"
#include "Engine/Font.h"
#include "Fonts/FontMeasure.h"
#include "Framework/Application/SlateApplication.h"
#include "Rendering/SlateRenderer.h"
#include "Engine/GameViewportClient.h"
#include "Engine/LocalPlayer.h"
#include "Engine/Engine.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/PlayerController.h"
#include "InputCoreTypes.h"
#include "Math/RotationMatrix.h"
#include "Rendering/DrawElements.h"
#include "SceneView.h"
#include "Styling/AppStyle.h"
#include "Widgets/Colors/SColorBlock.h"
#include "Widgets/Layout/SExpandableArea.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Input/SSlider.h"
#include "Widgets/Input/SSpinBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SGridPanel.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SWidgetSwitcher.h"
#include "Widgets/SLeafWidget.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SViewport.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "WorldGenerationPanel"

/**
 * SSpinBox deliberately scales mouse movement by 0.1 for ranges <= 10 before
 * applying Delta snapping. That makes the surface ranges look frozen during a
 * normal short drag. Keep exact numeric entry in the spin box, but use a real
 * direct-position slider for the visible/interactive track.
 */
class SAPSGenerationRangeSlider final : public SSlider
{
public:
	SLATE_BEGIN_ARGS(SAPSGenerationRangeSlider)
		: _Value(0.0f)
		, _MinValue(0.0f)
		, _MaxValue(1.0f)
		, _StepSize(0.01f)
		, _SliderBarColor(FLinearColor::White)
		, _SliderHandleColor(FLinearColor::White)
	{}
		SLATE_ATTRIBUTE(float, Value)
		SLATE_ARGUMENT(float, MinValue)
		SLATE_ARGUMENT(float, MaxValue)
		SLATE_ATTRIBUTE(float, StepSize)
		SLATE_ATTRIBUTE(FSlateColor, SliderBarColor)
		SLATE_ATTRIBUTE(FSlateColor, SliderHandleColor)
		SLATE_EVENT(FOnFloatValueChanged, OnValueChanged)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs)
	{
		SliderStyle = MakeShared<FSliderStyle>(FAPSUIStyle::MakeSliderStyle(
			FAPSUIStyle::GetPalette(EAPSUIDisplayProfile::Balanced22)));
		const FAPSUILayoutMetrics& Layout = FAPSUIStyle::Metrics();
		TrackBrush = MakeShared<FSlateRoundedBoxBrush>(
			FLinearColor::White, Layout.SliderTrackHeight * 0.5f);
		ThumbBrush = MakeShared<FSlateRoundedBoxBrush>(
			FLinearColor::White, Layout.SliderHandleSize.X * 0.5f);
		HoverThumbBrush = MakeShared<FSlateRoundedBoxBrush>(
			FLinearColor::White, Layout.SliderHandleHoverSize.X * 0.5f);
		SSlider::Construct(SSlider::FArguments()
			.Style(SliderStyle.Get())
			.Value(InArgs._Value)
			.MinValue(InArgs._MinValue)
			.MaxValue(InArgs._MaxValue)
			.StepSize(InArgs._StepSize)
			.MouseUsesStep(true)
			.IndentHandle(true)
			.RequiresControllerLock(false)
			.SliderBarColor(InArgs._SliderBarColor)
			.SliderHandleColor(InArgs._SliderHandleColor)
			.OnValueChanged(InArgs._OnValueChanged));
	}

	virtual FVector2D ComputeDesiredSize(float) const override
	{
		return FVector2D(160.0f, FAPSUIStyle::Metrics().SliderHitHeight);
	}

	virtual int32 OnPaint(const FPaintArgs&, const FGeometry& Geometry,
		const FSlateRect&, FSlateWindowElementList& OutDrawElements, int32 LayerId,
		const FWidgetStyle& WidgetStyle, bool bParentEnabled) const override
	{
		if (!TrackBrush.IsValid() || !ThumbBrush.IsValid() || !HoverThumbBrush.IsValid())
		{
			return LayerId;
		}
		const FAPSUILayoutMetrics& Layout = FAPSUIStyle::Metrics();
		const FVector2D Size = Geometry.GetLocalSize();
		const bool bEnabled = ShouldBeEnabled(bParentEnabled);
		const bool bHot = IsHovered() || HasMouseCapture() || HasKeyboardFocus();
		const float ThumbDiameter = bHot
			? Layout.SliderHandleHoverSize.X : Layout.SliderHandleSize.X;
		const float TrackLeft = Layout.SliderHandleHoverSize.X * 0.5f;
		const float TrackRight = FMath::Max(TrackLeft, Size.X - TrackLeft);
		const float TrackWidth = FMath::Max(TrackRight - TrackLeft, 1.0f);
		const float CenterY = Size.Y * 0.5f;
		const float NormalizedValue = FMath::Clamp(GetNormalizedValue(), 0.0f, 1.0f);
		const float ThumbX = TrackLeft + TrackWidth * NormalizedValue;
		const ESlateDrawEffect DrawEffect = bEnabled
			? ESlateDrawEffect::None : ESlateDrawEffect::DisabledEffect;
		FLinearColor QuietColor = GetSliderBarColorAttribute().Get().GetColor(WidgetStyle);
		FLinearColor FocusColor = GetSliderHandleColorAttribute().Get().GetColor(WidgetStyle);
		if (!bEnabled)
		{
			QuietColor.A *= 0.45f;
			FocusColor.A *= 0.45f;
		}

		const FVector2D TrackSize(TrackWidth, Layout.SliderTrackHeight);
		const FVector2D TrackPosition(TrackLeft, CenterY - Layout.SliderTrackHeight * 0.5f);
		FSlateDrawElement::MakeBox(OutDrawElements, LayerId,
			Geometry.ToPaintGeometry(TrackSize, FSlateLayoutTransform(TrackPosition)),
			TrackBrush.Get(), DrawEffect,
			FLinearColor(QuietColor.R, QuietColor.G, QuietColor.B, QuietColor.A * 0.72f));

		const float FilledWidth = TrackWidth * NormalizedValue;
		if (FilledWidth > 0.5f)
		{
			FSlateDrawElement::MakeBox(OutDrawElements, LayerId + 1,
				Geometry.ToPaintGeometry(
					FVector2D(FilledWidth, Layout.SliderTrackHeight),
					FSlateLayoutTransform(TrackPosition)),
				TrackBrush.Get(), DrawEffect,
				FLinearColor(FocusColor.R, FocusColor.G, FocusColor.B, FocusColor.A * 0.92f));
		}

		if (bHot && bEnabled)
		{
			const FVector2D GlowSize = Layout.SliderHandleHoverSize + FVector2D(8.0f, 8.0f);
			FSlateDrawElement::MakeBox(OutDrawElements, LayerId + 2,
				Geometry.ToPaintGeometry(GlowSize, FSlateLayoutTransform(
					FVector2D(ThumbX, CenterY) - GlowSize * 0.5f)),
				HoverThumbBrush.Get(), ESlateDrawEffect::None,
				FLinearColor(FocusColor.R, FocusColor.G, FocusColor.B, 0.16f));
		}

		const FVector2D ThumbSize(ThumbDiameter, ThumbDiameter);
		FSlateDrawElement::MakeBox(OutDrawElements, LayerId + 3,
			Geometry.ToPaintGeometry(ThumbSize, FSlateLayoutTransform(
				FVector2D(ThumbX, CenterY) - ThumbSize * 0.5f)),
			bHot ? HoverThumbBrush.Get() : ThumbBrush.Get(), DrawEffect,
			bHot && bEnabled ? FLinearColor::White : FocusColor);
		return LayerId + 3;
	}

protected:
	virtual void CommitValue(const float NewValue) override
	{
		SSlider::CommitValue(NewValue);

		// SSlider stores bound values in a TSlateAttribute cache that normally
		// refreshes during the next prepass. Our delegate has already updated the
		// editor buffer, so refresh that cache now as well: the handle, automation
		// reads and a second input event in the same frame must all see one value.
		GetValueAttribute().UpdateValue();
	}

#if WITH_DEV_AUTOMATION_TESTS
public:
	void CommitValueForAutomation(const float NewValue)
	{
		CommitValue(NewValue);
	}
#endif

private:
	TSharedPtr<FSliderStyle> SliderStyle;
	TSharedPtr<FSlateRoundedBoxBrush> TrackBrush;
	TSharedPtr<FSlateRoundedBoxBrush> ThumbBrush;
	TSharedPtr<FSlateRoundedBoxBrush> HoverThumbBrush;
};

namespace APSGenerationUI
{
	template <int32 SampleCount>
	const TArray<FVector2D>& UnitCircleSamples()
	{
		static_assert(SampleCount >= 3, "A circle needs at least three samples");
		static const TArray<FVector2D> Samples = []
		{
			TArray<FVector2D> Result;
			Result.Reserve(SampleCount + 1);
			for (int32 Sample = 0; Sample <= SampleCount; ++Sample)
			{
				const double Angle = UE_TWO_PI * static_cast<double>(Sample) / SampleCount;
				double SinAngle = 0.0;
				double CosAngle = 1.0;
				FMath::SinCos(&SinAngle, &CosAngle, Angle);
				Result.Emplace(CosAngle, SinAngle);
			}
			return Result;
		}();
		return Samples;
	}

	enum class EHierarchyGlyph : uint8
	{
		System,
		Star,
		Planet,
		Moon,
		Cluster,
		Galaxy
	};

	class SGenerationHierarchyBranch final : public SLeafWidget
	{
	public:
		SLATE_BEGIN_ARGS(SGenerationHierarchyBranch) {}
			SLATE_ARGUMENT(int32, Depth)
			SLATE_ATTRIBUTE(FLinearColor, Color)
		SLATE_END_ARGS()

		void Construct(const FArguments& InArgs)
		{
			Depth = FMath::Clamp(InArgs._Depth, 0, 4);
			Color = InArgs._Color;
			SetVisibility(EVisibility::HitTestInvisible);
		}

		virtual FVector2D ComputeDesiredSize(float) const override
		{
			return FVector2D(15.0f + static_cast<float>(Depth) * 14.0f, 1.0f);
		}

		virtual int32 OnPaint(const FPaintArgs&, const FGeometry& Geometry,
			const FSlateRect&, FSlateWindowElementList& OutDrawElements, int32 LayerId,
			const FWidgetStyle&, bool) const override
		{
			const FVector2D Size = Geometry.GetLocalSize();
			const FLinearColor DrawColor = Color.Get(FLinearColor::White);
			const float CenterY = Size.Y * 0.5f;
			const float NodeX = 7.0f + static_cast<float>(Depth) * 14.0f;
			for (int32 Level = 0; Level < Depth; ++Level)
			{
				const float X = 7.0f + static_cast<float>(Level) * 14.0f;
				FSlateDrawElement::MakeLines(OutDrawElements, LayerId, Geometry.ToPaintGeometry(),
					TArray<FVector2D>{FVector2D(X, 0.0f), FVector2D(X, Size.Y)},
					ESlateDrawEffect::None,
					FLinearColor(DrawColor.R, DrawColor.G, DrawColor.B, 0.42f), true, 1.0f);
			}
			if (Depth > 0)
			{
				FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 1, Geometry.ToPaintGeometry(),
					TArray<FVector2D>{FVector2D(NodeX - 14.0f, CenterY), FVector2D(NodeX, CenterY)},
					ESlateDrawEffect::None, DrawColor, true, 1.0f);
			}
			TArray<FVector2D> Circle;
			Circle.Reserve(17);
			for (int32 Point = 0; Point <= 16; ++Point)
			{
				const float Angle = UE_TWO_PI * static_cast<float>(Point) / 16.0f;
				Circle.Add(FVector2D(NodeX + FMath::Cos(Angle) * 3.25f,
					CenterY + FMath::Sin(Angle) * 3.25f));
			}
			FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 2, Geometry.ToPaintGeometry(),
				Circle, ESlateDrawEffect::None, DrawColor, true, 1.25f);
			return LayerId + 2;
		}

	private:
		int32 Depth{0};
		TAttribute<FLinearColor> Color{FLinearColor::White};
	};

	class SGenerationHierarchyGlyph final : public SLeafWidget
	{
	public:
		SLATE_BEGIN_ARGS(SGenerationHierarchyGlyph) {}
			SLATE_ARGUMENT(EHierarchyGlyph, Glyph)
			SLATE_ATTRIBUTE(FLinearColor, Color)
		SLATE_END_ARGS()

		void Construct(const FArguments& InArgs)
		{
			Glyph = InArgs._Glyph;
			Color = InArgs._Color;
			SetVisibility(EVisibility::HitTestInvisible);
		}

		virtual FVector2D ComputeDesiredSize(float) const override { return FVector2D(20.0f, 20.0f); }

		virtual int32 OnPaint(const FPaintArgs&, const FGeometry& Geometry,
			const FSlateRect&, FSlateWindowElementList& OutDrawElements, int32 LayerId,
			const FWidgetStyle&, bool) const override
		{
			const FVector2D Center = Geometry.GetLocalSize() * 0.5f;
			const FLinearColor DrawColor = Color.Get(FLinearColor::White);
			const auto DrawLine = [&OutDrawElements, &Geometry, LayerId, &DrawColor](
				const FVector2D& A, const FVector2D& B, float Width = 1.1f)
			{
				FSlateDrawElement::MakeLines(OutDrawElements, LayerId, Geometry.ToPaintGeometry(),
					TArray<FVector2D>{A, B}, ESlateDrawEffect::None, DrawColor, true, Width);
			};
			const auto DrawCircle = [&OutDrawElements, &Geometry, LayerId, &DrawColor](
				const FVector2D& Origin, float Radius, float Width = 1.1f)
			{
				TArray<FVector2D> Points;
				Points.Reserve(21);
				for (int32 Point = 0; Point <= 20; ++Point)
				{
					const float Angle = UE_TWO_PI * static_cast<float>(Point) / 20.0f;
					Points.Add(Origin + FVector2D(FMath::Cos(Angle), FMath::Sin(Angle)) * Radius);
				}
				FSlateDrawElement::MakeLines(OutDrawElements, LayerId, Geometry.ToPaintGeometry(),
					Points, ESlateDrawEffect::None, DrawColor, true, Width);
			};

			DrawCircle(Center, Glyph == EHierarchyGlyph::Moon ? 4.0f : 5.5f, 1.25f);
			switch (Glyph)
			{
			case EHierarchyGlyph::Star:
				DrawCircle(Center, 2.2f, 1.2f);
				DrawLine(Center + FVector2D(-8.0f, 0.0f), Center + FVector2D(8.0f, 0.0f));
				DrawLine(Center + FVector2D(0.0f, -8.0f), Center + FVector2D(0.0f, 8.0f));
				break;
			case EHierarchyGlyph::Planet:
				DrawLine(Center + FVector2D(-8.0f, 3.0f), Center + FVector2D(8.0f, -3.0f));
				break;
			case EHierarchyGlyph::Moon:
				DrawCircle(Center + FVector2D(2.0f, -1.0f), 4.0f, 0.75f);
				break;
			case EHierarchyGlyph::Galaxy:
			case EHierarchyGlyph::Cluster:
				DrawCircle(Center, 2.0f, 1.0f);
				DrawLine(Center + FVector2D(-8.0f, 4.0f), Center + FVector2D(8.0f, -4.0f));
				break;
			case EHierarchyGlyph::System:
			default:
				DrawCircle(Center, 2.0f, 1.0f);
				break;
			}
			return LayerId;
		}

	private:
		EHierarchyGlyph Glyph{EHierarchyGlyph::System};
		TAttribute<FLinearColor> Color{FLinearColor::White};
	};

	class SGenerationChamferedFrame final : public SLeafWidget
	{
	public:
		SLATE_BEGIN_ARGS(SGenerationChamferedFrame) {}
			SLATE_ARGUMENT(FLinearColor, Color)
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
			const FColor VertexColor = Color.ToFColor(true);
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
		FLinearColor Color{FLinearColor::White};
		float Thickness{1.0f};
	};

	/** Fill and frame share the same eight-corner polygon, so no rectangular
	 * brush can leak a one-pixel spur beyond a chamfer. */
	class SGenerationChamferedSurface final : public SLeafWidget
	{
	public:
		SLATE_BEGIN_ARGS(SGenerationChamferedSurface) {}
			SLATE_ARGUMENT(FLinearColor, Color)
		SLATE_END_ARGS()

		void Construct(const FArguments& InArgs)
		{
			Color = InArgs._Color;
			SetVisibility(EVisibility::HitTestInvisible);
		}

		virtual FVector2D ComputeDesiredSize(float) const override { return FVector2D::ZeroVector; }

		virtual int32 OnPaint(const FPaintArgs&, const FGeometry& Geometry,
			const FSlateRect&, FSlateWindowElementList& OutDrawElements, int32 LayerId,
			const FWidgetStyle&, bool) const override
		{
			const FVector2D Size = Geometry.GetLocalSize();
			if (Size.X <= 4.0f || Size.Y <= 4.0f) return LayerId;
			const FSlateBrush* Brush = FAppStyle::GetBrush("WhiteBrush");
			const FSlateResourceHandle ResourceHandle = Brush->GetRenderingResource();
			const float Cut = APSChamfer::Cut(Size);
			const TArray<FVector2f> Points = {
				FVector2f(Size.X * 0.5f, Size.Y * 0.5f),
				FVector2f(Cut, 0.0f), FVector2f(Size.X - Cut, 0.0f),
				FVector2f(Size.X, Cut), FVector2f(Size.X, Size.Y - Cut),
				FVector2f(Size.X - Cut, Size.Y), FVector2f(Cut, Size.Y),
				FVector2f(0.0f, Size.Y - Cut), FVector2f(0.0f, Cut)
			};
			const FColor VertexColor = Color.ToFColor(true);
			TArray<FSlateVertex> Vertices;
			Vertices.Reserve(Points.Num());
			for (const FVector2f& Point : Points)
			{
				Vertices.Add(FSlateVertex::Make<ESlateVertexRounding::Disabled>(
					Geometry.GetAccumulatedRenderTransform(), Point,
					FVector2f::ZeroVector, VertexColor));
			}
			TArray<SlateIndex> Indices;
			Indices.Reserve(24);
			for (SlateIndex Edge = 1; Edge <= 8; ++Edge)
			{
				Indices.Add(0);
				Indices.Add(Edge);
				Indices.Add(static_cast<SlateIndex>(Edge == 8 ? 1 : Edge + 1));
			}
			FSlateDrawElement::MakeCustomVerts(OutDrawElements, LayerId, ResourceHandle,
				Vertices, Indices, nullptr, 0, 0, ESlateDrawEffect::None);
			return LayerId;
		}

	private:
		FLinearColor Color{FLinearColor::Transparent};
	};

	// The astronomical preview is the real level viewport. Keep the root chrome
	// fully transparent so no Slate wash/vignette changes the scene exposure or
	// hides small galaxy/cluster instances in the middle of the screen.
	const FLinearColor Background(0.0f, 0.0f, 0.0f, 0.0f);
	FLinearColor SRGB(uint8 R, uint8 G, uint8 B, uint8 A = 255)
	{
		return FLinearColor::FromSRGBColor(FColor(R, G, B, A));
	}
	// Rio 06.10: the generation chrome follows the interface theme; ApplyTheme() (SWorldGenerationPanel::Construct)
	// copies it in. The values below are Classic, the look until 06.10. Only colours and type change, no logic.
	static FLinearColor Panel = SRGB(8, 32, 42, 238);
	static FLinearColor Cyan = SRGB(67, 214, 236);
	static FLinearColor CyanDim = SRGB(27, 83, 96, 176);
	static FLinearColor Amber = SRGB(242, 181, 29);
	static FLinearColor White = SRGB(234, 246, 248);
	static FLinearColor SecondaryText = SRGB(145, 171, 178);
	TWeakObjectPtr<UFont> DisplayFont;
	TWeakObjectPtr<UFont> BodyFont;
	static FSlateRoundedBoxBrush ControlBrush(FLinearColor(0.003f, 0.016f, 0.028f, 0.94f), 6.0f, CyanDim, 1.0f);
	/** Designation chips (A7.02): a tighter corner than controls, so the text keeps clear room inside (Rio 02.10). */
	static FSlateRoundedBoxBrush ChipBrush(FLinearColor(0.003f, 0.016f, 0.028f, 0.94f), 3.0f, CyanDim, 1.0f);
	static FSlateRoundedBoxBrush BadgeBrush(FLinearColor(0.005f, 0.045f, 0.070f, 0.98f), 16.0f, Cyan, 1.0f);
	static FLinearColor HierarchyRowFill(0.003f, 0.022f, 0.038f, 0.94f);
	static FLinearColor HierarchyHoverFill(0.010f, 0.075f, 0.105f, 0.98f);
	static FLinearColor HierarchyPressedFill(0.015f, 0.115f, 0.155f, 1.0f);
	static FLinearColor SelectedFill(Amber.R, Amber.G, Amber.B, 0.22f);
	static FLinearColor AncestorFill(Cyan.R, Cyan.G, Cyan.B, 0.08f);

	FSlateFontInfo Font(const FName Typeface, int32 Size)
	{
		// Rio 06.10: Chakra Petch for bold labels, Exo 2 otherwise (aps.UI.LegacyFonts 1: Orbitron, Roboto).
		if (!APSUITheme::UsesLegacyFonts())
		{
			return Typeface == TEXT("Bold") ? APSUITheme::DisplayFont(Typeface, Size) : APSUITheme::BodyFont(Typeface, Size);
		}
		if (!DisplayFont.IsValid())
		{
			DisplayFont = LoadObject<UFont>(nullptr, TEXT("/Game/APS/APS_ALPHA/UI/Fonts/Orbitron_Bold_Font.Orbitron_Bold_Font"));
			BodyFont = LoadObject<UFont>(nullptr, TEXT("/Game/APS/APS_ALPHA/UI/Fonts/Orbitron_Medium_Font.Orbitron_Medium_Font"));
			// Rooted like FAPSUIStyle's fonts: text blocks hold them as plain pointers GC never sees.
			for (UFont* Loaded : {DisplayFont.Get(), BodyFont.Get()})
			{
				if (Loaded)
				{
					Loaded->AddToRoot();
				}
			}
		}
		if (Typeface == TEXT("Bold"))
		{
			if (UFont* FontObject = DisplayFont.Get())
			{
				return FSlateFontInfo(FontObject, Size, Typeface);
			}
		}
		return FCoreStyle::GetDefaultFontStyle(Typeface, Size);
	}

	FSlateFontInfo ReadableFont(const FName Typeface, int32 Size)
	{
		// Reserve the display face for branding and compact technical headings.
		// Long descriptions, generated names and metadata need a neutral UI face (Exo 2 since 06.10).
		return APSUITheme::BodyFont(Typeface, Size);
	}

	/** Rio 03.10 ("everywhere the text strictly centred by height and width"): a label in a box sits by its capitals
	 * (APSChrome::CapsCenterShift), a lone symbol ("<", ">", "+", "-") by its own middle line (SymbolCenterShift). */
	TOptional<FSlateRenderTransform> CapsShift(const FName Typeface, const int32 Size)
	{
		return APSChrome::CapsCenterShift(Font(Typeface, Size));
	}

	TOptional<FSlateRenderTransform> SymbolShift(const FName Typeface, const int32 Size)
	{
		return APSChrome::SymbolCenterShift(Font(Typeface, Size));
	}


	FButtonStyle MakeButtonStyle(const FLinearColor& Outline, const FLinearColor& Fill,
		const FLinearColor& ActiveOutline, const FLinearColor& PressedFill)
	{
		return FButtonStyle()
			.SetNormal(FSlateRoundedBoxBrush(Fill, 6.0f, Outline, 1.0f))
			.SetHovered(FSlateRoundedBoxBrush(APSUITheme::Current() == EAPSUITheme::Classic
				? FLinearColor(Fill.R + 0.025f, Fill.G + 0.05f, Fill.B + 0.07f, 0.98f)
				: FMath::Lerp(Fill, ActiveOutline, 0.08f).CopyWithNewOpacity(0.98f), 6.0f, ActiveOutline, 1.5f))
			.SetPressed(FSlateRoundedBoxBrush(PressedFill, 6.0f, ActiveOutline, 1.5f));
	}

	FButtonStyle MakeHierarchyButton()
	{
		return FButtonStyle()
			.SetNormal(FSlateRoundedBoxBrush(HierarchyRowFill, 5.0f, CyanDim, 1.0f))
			.SetHovered(FSlateRoundedBoxBrush(HierarchyHoverFill, 5.0f, Cyan, 1.25f))
			.SetPressed(FSlateRoundedBoxBrush(HierarchyPressedFill, 5.0f, Cyan, 1.5f))
			.SetDisabled(FSlateRoundedBoxBrush(
				FLinearColor(HierarchyRowFill.R, HierarchyRowFill.G, HierarchyRowFill.B, 0.55f),
				5.0f, FLinearColor(CyanDim.R, CyanDim.G, CyanDim.B, 0.55f), 1.0f));
	}

	static FButtonStyle SecondaryButton = MakeButtonStyle(
		CyanDim, FLinearColor(0.003f, 0.022f, 0.038f, 0.94f), Cyan,
		FLinearColor(0.02f, 0.14f, 0.20f, 1.0f));
	static FButtonStyle PrimaryButton = MakeButtonStyle(
		Amber, FLinearColor(0.30f, 0.12f, 0.004f, 0.96f),
		FLinearColor(1.0f, 0.76f, 0.18f, 1.0f), FLinearColor(0.52f, 0.22f, 0.006f, 1.0f));
	static FButtonStyle HierarchyButton = MakeHierarchyButton();
	static uint32 AppliedThemeRevision = 0;

	/** Rio 06.10: copies the active interface theme into the palette, brushes and button styles above. */
	void ApplyTheme()
	{
		if (AppliedThemeRevision == APSUITheme::Revision())
		{
			return;
		}
		AppliedThemeRevision = APSUITheme::Revision();
		using APSUITheme::Retint;
		using APSUITheme::RetintAction;
		const FAPSUIThemePalette& P = APSUITheme::Palette();
		const bool bClassic = APSUITheme::Current() == EAPSUITheme::Classic;
		Panel = bClassic ? SRGB(8, 32, 42, 238) : P.Panel.CopyWithNewOpacity(238.0f / 255.0f);
		Cyan = P.Highlight;
		CyanDim = bClassic ? SRGB(27, 83, 96, 176) : APSUITheme::Fade(P.Frame, 176.0f / 178.0f);
		Amber = P.Action;
		White = P.Text;
		SecondaryText = bClassic ? SRGB(145, 171, 178) : FMath::Lerp(P.TextQuiet, P.TextSoft, 0.4f);
		ControlBrush = FSlateRoundedBoxBrush(Retint(FLinearColor(0.003f, 0.016f, 0.028f, 0.94f)), 6.0f, CyanDim, 1.0f);
		ChipBrush = FSlateRoundedBoxBrush(Retint(FLinearColor(0.003f, 0.016f, 0.028f, 0.94f)), 3.0f, CyanDim, 1.0f);
		BadgeBrush = FSlateRoundedBoxBrush(bClassic ? FLinearColor(0.005f, 0.045f, 0.070f, 0.98f) : P.HighlightFill, 16.0f, Cyan, 1.0f);
		HierarchyRowFill = Retint(FLinearColor(0.003f, 0.022f, 0.038f, 0.94f));
		HierarchyHoverFill = Retint(FLinearColor(0.010f, 0.075f, 0.105f, 0.98f));
		HierarchyPressedFill = Retint(FLinearColor(0.015f, 0.115f, 0.155f, 1.0f));
		SelectedFill = FLinearColor(Amber.R, Amber.G, Amber.B, 0.22f);
		AncestorFill = FLinearColor(Cyan.R, Cyan.G, Cyan.B, 0.08f);
		SecondaryButton = MakeButtonStyle(CyanDim, Retint(FLinearColor(0.003f, 0.022f, 0.038f, 0.94f)), Cyan,
			Retint(FLinearColor(0.02f, 0.14f, 0.20f, 1.0f)));
		PrimaryButton = MakeButtonStyle(Amber, bClassic ? FLinearColor(0.30f, 0.12f, 0.004f, 0.96f) : P.ActionFill,
			bClassic ? FLinearColor(1.0f, 0.76f, 0.18f, 1.0f) : P.ActionBright,
			bClassic ? FLinearColor(0.52f, 0.22f, 0.006f, 1.0f) : FMath::Lerp(P.ActionFill, P.Action, 0.35f).CopyWithNewOpacity(1.0f));
		HierarchyButton = MakeHierarchyButton();
	}

	TSharedRef<SWidget> ChamferPanel(TSharedRef<SWidget> Content)
	{
		return SNew(SAPSChamferedOverlay)
			+ SOverlay::Slot()
			[
				SNew(SGenerationChamferedSurface).Color(Panel)
			]
			+ SOverlay::Slot().Padding(16.0f)
			[
				Content
			]
			+ SOverlay::Slot()
			[
				SNew(SGenerationChamferedFrame).Color(Cyan).Thickness(1.0f)
			];
	}

	TArray<int64> GetSelectableEnumValues(const UEnum* Enum)
	{
		TArray<int64> Values;
		if (!Enum)
		{
			return Values;
		}
		for (int32 Index = 0; Index < Enum->NumEnums() - 1; ++Index)
		{
			const FString Name = Enum->GetNameStringByIndex(Index);
			if (Name.Equals(TEXT("Unknown"), ESearchCase::IgnoreCase)
				|| Name.EndsWith(TEXT("_Unknown"), ESearchCase::IgnoreCase))
			{
				continue;
			}
			const int64 Value = Enum->GetValueByIndex(Index);
			if (Enum == StaticEnum<EPlanetType>()
				&& !APSPlanetTypes::IsSelectable(static_cast<EPlanetType>(Value))) continue;
			// Rio 02.10: the remnants are stellar types; a star's class list does not offer theirs.
			if (Enum == StaticEnum<ESpectralClass>() && (Value == static_cast<int64>(ESpectralClass::BH)
				|| Value == static_cast<int64>(ESpectralClass::NS) || Value == static_cast<int64>(ESpectralClass::PS))) continue;
			Values.Add(Value);
		}
		return Values;
	}

	FText GetGenerationEnumDisplayName(const UEnum* Enum, const int64 Value)
	{
		// Several late-added planet values still carry the legacy Metallic Planet
		// metadata in the asset-facing enum. Keep serialization untouched, but show
		// the actual generator profile names in the new Slate workflow.
		if (Enum == StaticEnum<EPlanetType>())
		{
			switch (static_cast<EPlanetType>(Value))
			{
			case EPlanetType::Water: return LOCTEXT("WaterPlanet", "Water Planet");
			case EPlanetType::Nordic: return LOCTEXT("NordicPlanet", "Nordic Planet");
			case EPlanetType::Tundra: return LOCTEXT("TundraPlanet", "Tundra Planet");
			case EPlanetType::HighMountain: return LOCTEXT("HighMountainPlanet", "High Mountain Planet");
			case EPlanetType::Sand: return LOCTEXT("SandPlanet", "Sand Planet");
			case EPlanetType::Oasis: return LOCTEXT("OasisPlanet", "Oasis Planet");
			case EPlanetType::Archipelago: return LOCTEXT("ArchipelagoPlanet", "Archipelago Planet");
			case EPlanetType::Pangea: return LOCTEXT("PangeaPlanet", "Pangea Planet");
			default: break;
			}
		}
		return Enum ? Enum->GetDisplayNameTextByValue(Value) : FText::FromString(TEXT("--"));
	}

	constexpr int32 GasGiantSurfaceFamilyIndex = 9;

	int32 GetSurfaceFamilyIndex(EPlanetType PlanetType)
	{
		return UAPSPlanetSurfaceProfileResolver::SupportsWorldScape(PlanetType)
			? static_cast<int32>(UAPSPlanetSurfaceProfileResolver::GetArchetypeForType(PlanetType))
			: GasGiantSurfaceFamilyIndex;
	}

	FText GetSurfaceFamilyDisplayName(EPlanetType PlanetType)
	{
		if (!UAPSPlanetSurfaceProfileResolver::SupportsWorldScape(PlanetType))
		{
			return LOCTEXT("GasGiantFamily", "GAS GIANTS / NO WORLDSCAPE");
		}
		const UEnum* ArchetypeEnum = StaticEnum<EAPSPlanetSurfaceArchetype>();
		return ArchetypeEnum->GetDisplayNameTextByValue(static_cast<int64>(
			UAPSPlanetSurfaceProfileResolver::GetArchetypeForType(PlanetType)));
	}

	TArray<EPlanetType> GetSurfacePresetsForFamily(int32 FamilyIndex)
	{
		switch (FamilyIndex)
		{
		case static_cast<int32>(EAPSPlanetSurfaceArchetype::Rocky):
			return {EPlanetType::Rocky, EPlanetType::Dwarf, EPlanetType::Basalt};
		case static_cast<int32>(EAPSPlanetSurfaceArchetype::Temperate):
			return {EPlanetType::Terrestrial, EPlanetType::Pangea, EPlanetType::Nordic,
				EPlanetType::SuperEarth, EPlanetType::HighMountain};
		case static_cast<int32>(EAPSPlanetSurfaceArchetype::Oceanic):
			return {EPlanetType::Ocean, EPlanetType::Water, EPlanetType::Archipelago};
		case static_cast<int32>(EAPSPlanetSurfaceArchetype::Biosphere):
			return {EPlanetType::Forest, EPlanetType::Oasis, EPlanetType::Savanna};
		case static_cast<int32>(EAPSPlanetSurfaceArchetype::Desert):
			return {EPlanetType::Greenhouse, EPlanetType::Desert, EPlanetType::Sand};
		case static_cast<int32>(EAPSPlanetSurfaceArchetype::Cryogenic):
			return {EPlanetType::Ice, EPlanetType::Frozen, EPlanetType::Tundra, EPlanetType::Rogue};
		case static_cast<int32>(EAPSPlanetSurfaceArchetype::Magmatic):
			return {EPlanetType::Volcanic, EPlanetType::Melted, EPlanetType::Lava};
		case static_cast<int32>(EAPSPlanetSurfaceArchetype::Metallic):
			return {EPlanetType::Metal, EPlanetType::Metallic, EPlanetType::Carbon};
		case static_cast<int32>(EAPSPlanetSurfaceArchetype::ExoticChemical):
			return {EPlanetType::Ammonia, EPlanetType::Crystal, EPlanetType::Sulfur};
		case GasGiantSurfaceFamilyIndex:
		default:
			return {EPlanetType::GasGiant, EPlanetType::HotGiant, EPlanetType::IceGiant};
		}
	}

	template <typename TTextGetter, typename TStepper>
	TSharedRef<SWidget> ChoiceRow(const FText& Label, TWeakObjectPtr<UWorldGenerationViewModel> ViewModel,
		TTextGetter TextGetter, TStepper Stepper)
	{
		const auto ValueText = [ViewModel, TextGetter]()
		{
			const UWorldGenerationViewModel* VM = ViewModel.Get();
			return VM && VM->GeneratedWorld ? TextGetter(VM->GeneratedWorld)
				: FText::FromString(TEXT("--"));
		};

		return SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(STextBlock).Text(Label).Font(ReadableFont("Bold", 11)).ColorAndOpacity(SecondaryText)
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 3.0f, 0.0f, 8.0f)
			[
				SNew(SBorder).BorderImage(&ControlBrush).Padding(2.0f)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().AutoWidth()
					[
						SNew(SBox).WidthOverride(FAPSUIStyle::Metrics().IconButtonTarget)
						.HeightOverride(FAPSUIStyle::Metrics().IconButtonTarget)
						[
							SNew(SButton).ContentPadding(0.0f).ButtonStyle(&SecondaryButton)
							.HAlign(HAlign_Center).VAlign(VAlign_Center)
							.OnClicked_Lambda([ViewModel, Stepper]()
							{
								if (UWorldGenerationViewModel* VM = ViewModel.Get()) Stepper(VM, -1);
								return FReply::Handled();
							})
							[SNew(STextBlock).Text(FText::FromString(TEXT("<")))
							.Justification(ETextJustify::Center).Font(Font("Bold", 12)).ColorAndOpacity(Cyan).RenderTransform(SymbolShift("Bold", 12))]
						]
					]
					+ SHorizontalBox::Slot().FillWidth(1.0f).HAlign(HAlign_Center).VAlign(VAlign_Center)
					[
						SNew(STextBlock).Text_Lambda(ValueText).ToolTipText_Lambda(ValueText)
						.OverflowPolicy(ETextOverflowPolicy::Ellipsis)
						.Justification(ETextJustify::Center)
						.Font(Font("Bold", 11)).ColorAndOpacity(White).RenderTransform(CapsShift("Bold", 11))
					]
					+ SHorizontalBox::Slot().AutoWidth()
					[
						SNew(SBox).WidthOverride(FAPSUIStyle::Metrics().IconButtonTarget)
						.HeightOverride(FAPSUIStyle::Metrics().IconButtonTarget)
						[
							SNew(SButton).ContentPadding(0.0f).ButtonStyle(&SecondaryButton)
							.HAlign(HAlign_Center).VAlign(VAlign_Center)
							.OnClicked_Lambda([ViewModel, Stepper]()
							{
								if (UWorldGenerationViewModel* VM = ViewModel.Get()) Stepper(VM, 1);
								return FReply::Handled();
							})
							[SNew(STextBlock).Text(FText::FromString(TEXT(">")))
							.Justification(ETextJustify::Center).Font(Font("Bold", 12)).ColorAndOpacity(Cyan).RenderTransform(SymbolShift("Bold", 12))]
						]
					]
				]
			];
	}

	TSharedRef<SWidget> SectionTitle(const FText& Text)
	{
		const FString Upper = Text.ToString().ToUpper();
		const FText Glyph = FText::FromString(
			Upper.Contains(TEXT("ATMOSPHERE")) ? TEXT("ATM") :
			Upper.Contains(TEXT("SYSTEM")) ? TEXT("SYS") :
			Upper.Contains(TEXT("CLUSTER")) ? TEXT("CL") :
			Upper.Contains(TEXT("GALAXY")) ? TEXT("GX") : TEXT("AST"));
		return SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight()
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
			[
				SNew(SBox).WidthOverride(30.0f).HeightOverride(30.0f)
				.HAlign(HAlign_Fill).VAlign(VAlign_Fill)
				[
					SNew(SBorder).BorderImage(&BadgeBrush).Padding(0.0f)
					.HAlign(HAlign_Center).VAlign(VAlign_Center)
					[SNew(STextBlock).Text(Glyph).Justification(ETextJustify::Center)
					.Font(Font("Bold", 8)).ColorAndOpacity(Cyan).RenderTransform(CapsShift("Bold", 8))]
				]
			]
			+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center).Padding(10.0f, 0.0f)
			[SNew(STextBlock).Text(Text).Font(Font("Bold", 16)).ColorAndOpacity(Cyan).RenderTransform(CapsShift("Bold", 16))]
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 8.0f, 0.0f, 0.0f)
		[SNew(SBox).HeightOverride(1.0f)[SNew(SBorder).BorderImage(FAppStyle::GetBrush("WhiteBrush")).BorderBackgroundColor(CyanDim)]];
	}

	template <typename TEnum, typename TGetter>
	TSharedRef<SWidget> EnumRow(const FText& Label, TWeakObjectPtr<UWorldGenerationViewModel> ViewModel, TGetter Getter,
		TFunction<TArray<int64>(const UGeneratedWorld*)> ValuesGetter = {},
		TFunction<void(UWorldGenerationViewModel*, int32)> Setter = {})
	{
		// Rio 03.10: ValuesGetter narrows the cycle to a model-dependent list (galaxy CLASS per TYPE); Setter writes a
		// field that SetEnumValue cannot address (the galaxy's POPULATION/COMPOSITION share the cluster's enum types).
		const UEnum* Enum = StaticEnum<TEnum>();
		const auto ValueText = [ViewModel, Getter, Enum]()
		{
			if (const UWorldGenerationViewModel* VM = ViewModel.Get(); VM && VM->GeneratedWorld && Enum)
			{
				return GetGenerationEnumDisplayName(
					Enum, static_cast<int64>(Getter(VM->GeneratedWorld)));
			}
			return FText::FromString(TEXT("--"));
		};

		return SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(STextBlock).Text(Label).Font(ReadableFont("Bold", 11)).ColorAndOpacity(SecondaryText)
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 3.0f, 0.0f, 8.0f)
			[
				SNew(SBorder).BorderImage(&ControlBrush).Padding(2.0f)
				[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth()
				[
					SNew(SBox).WidthOverride(FAPSUIStyle::Metrics().IconButtonTarget)
					.HeightOverride(FAPSUIStyle::Metrics().IconButtonTarget)
					[
						SNew(SButton).ContentPadding(0.0f)
						.HAlign(HAlign_Center).VAlign(VAlign_Center)
						.ButtonStyle(&SecondaryButton)
						.OnClicked_Lambda([ViewModel, Getter, Enum, ValuesGetter, Setter]()
						{
							if (UWorldGenerationViewModel* VM = ViewModel.Get(); VM && VM->GeneratedWorld && Enum)
							{
								const TArray<int64> Values = ValuesGetter
									? ValuesGetter(VM->GeneratedWorld) : GetSelectableEnumValues(Enum);
								if (!Values.IsEmpty())
								{
									const int64 Current = static_cast<int64>(Getter(VM->GeneratedWorld));
									const int32 CurrentIndex = Values.IndexOfByKey(Current);
									const int32 NewIndex = CurrentIndex == INDEX_NONE
										? Values.Num() - 1 : (CurrentIndex - 1 + Values.Num()) % Values.Num();
									if (Setter) Setter(VM, static_cast<int32>(Values[NewIndex]));
									else VM->SetEnumValue(Enum, static_cast<int32>(Values[NewIndex]));
								}
							}
							return FReply::Handled();
						})
						[SNew(STextBlock).Text(FText::FromString(TEXT("<")))
						.Justification(ETextJustify::Center).Font(Font("Bold", 12)).ColorAndOpacity(Cyan).RenderTransform(SymbolShift("Bold", 12))]
					]
				]
				+ SHorizontalBox::Slot().FillWidth(1.0f).HAlign(HAlign_Center).VAlign(VAlign_Center)
				[
					SNew(STextBlock)
					.Text_Lambda(ValueText).ToolTipText_Lambda(ValueText)
					.OverflowPolicy(ETextOverflowPolicy::Ellipsis)
					.Justification(ETextJustify::Center)
					.Font(Font("Bold", 11)).ColorAndOpacity(White).RenderTransform(CapsShift("Bold", 11))
				]
				+ SHorizontalBox::Slot().AutoWidth()
				[
					SNew(SBox).WidthOverride(FAPSUIStyle::Metrics().IconButtonTarget)
					.HeightOverride(FAPSUIStyle::Metrics().IconButtonTarget)
					[
						SNew(SButton).ContentPadding(0.0f)
						.HAlign(HAlign_Center).VAlign(VAlign_Center)
						.ButtonStyle(&SecondaryButton)
						.OnClicked_Lambda([ViewModel, Getter, Enum, ValuesGetter, Setter]()
						{
							if (UWorldGenerationViewModel* VM = ViewModel.Get(); VM && VM->GeneratedWorld && Enum)
							{
								const TArray<int64> Values = ValuesGetter
									? ValuesGetter(VM->GeneratedWorld) : GetSelectableEnumValues(Enum);
								if (!Values.IsEmpty())
								{
									const int64 Current = static_cast<int64>(Getter(VM->GeneratedWorld));
									const int32 CurrentIndex = Values.IndexOfByKey(Current);
									const int32 NewIndex = CurrentIndex == INDEX_NONE
										? 0 : (CurrentIndex + 1) % Values.Num();
									if (Setter) Setter(VM, static_cast<int32>(Values[NewIndex]));
									else VM->SetEnumValue(Enum, static_cast<int32>(Values[NewIndex]));
								}
							}
							return FReply::Handled();
						})
						[SNew(STextBlock).Text(FText::FromString(TEXT(">")))
						.Justification(ETextJustify::Center).Font(Font("Bold", 12)).ColorAndOpacity(Cyan).RenderTransform(SymbolShift("Bold", 12))]
					]
				]
				]
			];
	}

	template <typename TValue, typename TGetter, typename TSetter>
	TSharedRef<SWidget> NumberRow(const FText& Label, TValue Min, TValue Max, TValue Delta,
		TWeakObjectPtr<UWorldGenerationViewModel> ViewModel, TGetter Getter, TSetter Setter,
		TSharedPtr<SAPSGenerationRangeSlider>* OutSlider = nullptr,
		TFunction<TValue(const UGeneratedWorld*)> MaximumGetter = {})
	{
		const bool bDynamicRange = static_cast<bool>(MaximumGetter);
		const auto ResolveMaximum = [ViewModel, Max, MaximumGetter]()
		{
			const UWorldGenerationViewModel* VM = ViewModel.Get();
			return MaximumGetter && VM && VM->GeneratedWorld ? MaximumGetter(VM->GeneratedWorld) : Max;
		};
		// SSlider has a fixed range. Normalize only rows with a type-dependent maximum
		// so selecting a gas/solid body updates its slider and numeric input together.
		TSharedPtr<SAPSGenerationRangeSlider> Slider;
		TSharedRef<SWidget> Row = SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(STextBlock).Text(Label).Font(ReadableFont("Bold", 11)).ColorAndOpacity(SecondaryText)
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 3.0f, 0.0f, 9.0f)
			[
				SNew(SBorder).BorderImage(&ControlBrush).Padding(2.0f)
				[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center).Padding(7.0f, 0.0f)
				[
					SNew(SBox).HeightOverride(FAPSUIStyle::Metrics().SliderHitHeight)
					.VAlign(VAlign_Fill)
					[
					SAssignNew(Slider, SAPSGenerationRangeSlider)
					.MinValue(bDynamicRange ? 0.0f : static_cast<float>(Min))
					.MaxValue(bDynamicRange ? 1.0f : static_cast<float>(Max))
					.StepSize_Lambda([bDynamicRange, ResolveMaximum, Min, Delta]()
					{
						return static_cast<float>(bDynamicRange ? Delta / (ResolveMaximum() - Min) : Delta);
					})
					.SliderBarColor(FSlateColor(CyanDim))
					.SliderHandleColor(FSlateColor(Cyan))
					.Value_Lambda([ViewModel, Getter, bDynamicRange, ResolveMaximum, Min]()
					{
						if (const UWorldGenerationViewModel* VM = ViewModel.Get(); VM && VM->GeneratedWorld)
						{
							const TValue Value = Getter(VM->GeneratedWorld);
							return static_cast<float>(bDynamicRange ? (Value - Min) / (ResolveMaximum() - Min) : Value);
						}
						return 0.0f;
					})
					.OnValueChanged_Lambda([ViewModel, Setter, bDynamicRange, ResolveMaximum, Min](const float Value)
					{
						if (UWorldGenerationViewModel* VM = ViewModel.Get())
						{
							const double ModelValue = bDynamicRange ? Min + Value * (ResolveMaximum() - Min) : Value;
							if constexpr (TIsIntegral<TValue>::Value)
							{
								Setter(VM, static_cast<TValue>(FMath::RoundToInt(ModelValue)));
							}
							else
							{
								Setter(VM, static_cast<TValue>(ModelValue));
							}
						}
					})
					]
				]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(7.0f, 0.0f, 0.0f, 0.0f)
				[
					SNew(SBox).WidthOverride(104.0f)
					[
						SNew(SSpinBox<TValue>)
						.MinValue(Min).MaxValue_Lambda([ResolveMaximum]() { return TOptional<TValue>(ResolveMaximum()); })
						.Delta(Delta).EnableSlider(false).MinDesiredWidth(96.0f)
						// Rio 03.10: the value centred in its box.
						.Justification(ETextJustify::Center)
						// Rio 02.10: readable inputs (860.18, 28547 instead of 860.179115, 28547.213784).
						.MaxFractionalDigits_Lambda([ViewModel, Getter]()
						{
							const UWorldGenerationViewModel* VM = ViewModel.Get();
							const double Magnitude = VM && VM->GeneratedWorld
								? FMath::Abs(static_cast<double>(Getter(VM->GeneratedWorld))) : 0.0;
							return TOptional<int32>(Magnitude >= 1000.0 ? 0 : Magnitude >= 10.0 ? 1 : 2);
						})
						.Value_Lambda([ViewModel, Getter]()
						{
							if (const UWorldGenerationViewModel* VM = ViewModel.Get(); VM && VM->GeneratedWorld)
							{
								return static_cast<TValue>(Getter(VM->GeneratedWorld));
							}
							return TValue{};
						})
						.OnValueChanged_Lambda([ViewModel, Setter](TValue Value)
						{
							if (UWorldGenerationViewModel* VM = ViewModel.Get()) Setter(VM, Value);
						})
						.OnValueCommitted_Lambda([ViewModel, Setter](TValue Value, ETextCommit::Type)
						{
							if (UWorldGenerationViewModel* VM = ViewModel.Get()) Setter(VM, Value);
						})
					]
				]
				]
			];
		if (OutSlider)
		{
			*OutSlider = Slider;
		}
		return Row;
	}

	/**
	 * Rio 03.10 (STARS 1,800..1,000,000): the slider moves over the logarithm of the value so the low end stays
	 * usable, and lands on three significant digits. The spin box keeps exact entry, applied on commit only
	 * (each change rebuilds the galaxy layer).
	 */
	template <typename TGetter, typename TSetter>
	TSharedRef<SWidget> LogNumberRow(const FText& Label, const int32 Min, const int32 Max,
		TWeakObjectPtr<UWorldGenerationViewModel> ViewModel, TGetter Getter, TSetter Setter)
	{
		const double LogMin = FMath::Loge(static_cast<double>(FMath::Max(Min, 1)));
		const double LogSpan = FMath::Max(FMath::Loge(static_cast<double>(FMath::Max(Max, Min + 1))) - LogMin, 1.0e-6);
		return SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(STextBlock).Text(Label).Font(ReadableFont("Bold", 11)).ColorAndOpacity(SecondaryText)
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 3.0f, 0.0f, 9.0f)
			[
				SNew(SBorder).BorderImage(&ControlBrush).Padding(2.0f)
				[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center).Padding(7.0f, 0.0f)
				[
					SNew(SBox).HeightOverride(FAPSUIStyle::Metrics().SliderHitHeight)
					.VAlign(VAlign_Fill)
					[
					SNew(SAPSGenerationRangeSlider)
					.MinValue(0.0f).MaxValue(1.0f).StepSize(0.001f)
					.SliderBarColor(FSlateColor(CyanDim))
					.SliderHandleColor(FSlateColor(Cyan))
					.Value_Lambda([ViewModel, Getter, Min, LogMin, LogSpan]()
					{
						const UWorldGenerationViewModel* VM = ViewModel.Get();
						const double Value = VM && VM->GeneratedWorld
							? static_cast<double>(Getter(VM->GeneratedWorld)) : static_cast<double>(Min);
						return static_cast<float>(FMath::Clamp(
							(FMath::Loge(FMath::Max(Value, 1.0)) - LogMin) / LogSpan, 0.0, 1.0));
					})
					.OnValueChanged_Lambda([ViewModel, Setter, LogMin, LogSpan](const float Position)
					{
						if (UWorldGenerationViewModel* VM = ViewModel.Get())
						{
							const double Raw = FMath::Exp(LogMin
								+ FMath::Clamp(static_cast<double>(Position), 0.0, 1.0) * LogSpan);
							const double Step = FMath::Pow(10.0,
								FMath::Max(FMath::FloorToDouble(FMath::LogX(10.0, FMath::Max(Raw, 1.0))) - 2.0, 0.0));
							Setter(VM, static_cast<int32>(FMath::RoundToDouble(Raw / Step) * Step));
						}
					})
					]
				]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(7.0f, 0.0f, 0.0f, 0.0f)
				[
					SNew(SBox).WidthOverride(104.0f)
					[
						SNew(SSpinBox<int32>)
						.MinValue(Min).MaxValue(Max).Delta(100).EnableSlider(false).MinDesiredWidth(96.0f)
						.Value_Lambda([ViewModel, Getter, Min]()
						{
							const UWorldGenerationViewModel* VM = ViewModel.Get();
							return VM && VM->GeneratedWorld ? static_cast<int32>(Getter(VM->GeneratedWorld)) : Min;
						})
						.OnValueCommitted_Lambda([ViewModel, Setter](const int32 Value, ETextCommit::Type)
						{
							if (UWorldGenerationViewModel* VM = ViewModel.Get()) Setter(VM, Value);
						})
					]
				]
				]
			];
	}


    struct FAtmosphereColorUIState
    {
        APSAtmosphereColorControl::FState Color;
        TWeakObjectPtr<UGeneratedWorld> Model;
        TWeakObjectPtr<AActor> Body;

        APSAtmosphereColorControl::FState& Read(UWorldGenerationViewModel* VM)
        {
            UGeneratedWorld* CurrentModel = VM ? VM->GeneratedWorld.Get() : nullptr;
            AActor* CurrentBody = VM ? VM->GetSelectedPreviewBody() : nullptr;
            if (Model.Get() != CurrentModel || Body.Get() != CurrentBody)
            {
                Color = APSAtmosphereColorControl::FState{};
                Model = CurrentModel;
                Body = CurrentBody;
            }
            if (CurrentModel) Color.Read(CurrentModel->AtmosphereColor);
            return Color;
        }
    };

    TSharedRef<SWidget> AtmosphereColorRows(TWeakObjectPtr<UWorldGenerationViewModel> VM)
    {
        const TSharedRef<FAtmosphereColorUIState> State = MakeShared<FAtmosphereColorUIState>();
        return SNew(SVerticalBox)
            + SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 8)
            [
                SNew(SHorizontalBox)
                + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0, 0, 9, 0)
                [
                    SNew(SColorBlock).Size(FVector2D(50, 26))
                    .AlphaDisplayMode(EColorBlockAlphaDisplayMode::Ignore)
                    .Color_Lambda([VM, State]() { return State->Read(VM.Get()).Swatch(); })
                ]
                + SHorizontalBox::Slot().FillWidth(1).VAlign(VAlign_Center)
                [
                    SNew(STextBlock).AutoWrapText(true)
                    .Font(ReadableFont("Regular", 9)).ColorAndOpacity(SecondaryText)
                    .Text_Lambda([VM, State]()
                    {
                        const auto& Color = State->Read(VM.Get());
                        if (!Color.bValid)
                            return LOCTEXT("AtmosphereInvalidColor", "INVALID COEFFICIENTS KEPT UNCHANGED. CORRECT THEM IN ADVANCED RGB.");
                        if (Color.Strength == 0.0f)
                            return LOCTEXT("AtmosphereZeroStrength", "STRENGTH 0 — NO SCATTERING. HUE/SATURATION APPLY WHEN STRENGTH IS RAISED.");
                        return FText::Format(LOCTEXT("AtmosphereColorSwatch",
                            "NORMALIZED COLOR · STRENGTH {0}\nSKY COLOR ALSO DEPENDS ON LIGHT AND VIEW."),
                            APSUINumber::Number(Color.Strength));
                    })
                ]
            ]
            + SVerticalBox::Slot().AutoHeight()
            [
                SNew(SVerticalBox)
                .IsEnabled_Lambda([VM, State]() { return VM.IsValid() && VM->GeneratedWorld && State->Read(VM.Get()).bValid; })
                + SVerticalBox::Slot().AutoHeight()
                [NumberRow<double>(LOCTEXT("AtmosphereHue", "HUE / DEG"), 0.0, 360.0, 1.0, VM,
                    [VM, State](const UGeneratedWorld*) { return static_cast<double>(State->Read(VM.Get()).Hue); },
                    [State](UWorldGenerationViewModel* V, double X)
                    {
                        if (V->GeneratedWorld && State->Read(V).SetHue(V->GeneratedWorld->AtmosphereColor, static_cast<float>(X)))
                            V->RefreshPlanetAppearancePreview(false);
                    })]
                + SVerticalBox::Slot().AutoHeight()
                [NumberRow<double>(LOCTEXT("AtmosphereSaturation", "SATURATION / %"), 0.0, 100.0, 1.0, VM,
                    [VM, State](const UGeneratedWorld*) { return static_cast<double>(State->Read(VM.Get()).Saturation) * 100.0; },
                    [State](UWorldGenerationViewModel* V, double X)
                    {
                        if (V->GeneratedWorld && State->Read(V).SetSaturation(V->GeneratedWorld->AtmosphereColor, static_cast<float>(X / 100.0)))
                            V->RefreshPlanetAppearancePreview(false);
                    })]
                + SVerticalBox::Slot().AutoHeight()
                [NumberRow<double>(LOCTEXT("AtmosphereStrength", "SCATTERING STRENGTH"), 0.0, 64.0, 0.1, VM,
                    [VM, State](const UGeneratedWorld*) { return static_cast<double>(State->Read(VM.Get()).Strength); },
                    [State](UWorldGenerationViewModel* V, double X)
                    {
                        if (V->GeneratedWorld && State->Read(V).SetStrength(V->GeneratedWorld->AtmosphereColor, static_cast<float>(X)))
                            V->RefreshPlanetAppearancePreview(false);
                    })]
            ]
            + SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 8)
            [
                SNew(STextBlock).AutoWrapText(true).Font(ReadableFont("Regular", 9)).ColorAndOpacity(SecondaryText)
                .Text(LOCTEXT("AtmosphereStrengthHelp",
                    "STRENGTH = PEAK LINEAR RGB COEFFICIENT, NOT HEIGHT OR OPACITY. HUE/SATURATION KEEP IT. EXISTING VALUES ABOVE 64 ARE RETAINED UNTIL STRENGTH IS EDITED."))
            ]
            + SVerticalBox::Slot().AutoHeight()
            [
                SNew(SExpandableArea).InitiallyCollapsed(true)
                .BorderBackgroundColor(FLinearColor::Transparent)
                .BodyBorderBackgroundColor(FLinearColor::Transparent)
                .HeaderContent()
                [SNew(STextBlock).Text(LOCTEXT("AtmosphereAdvancedRGB", "ADVANCED · LINEAR RGB"))
                    .Font(ReadableFont("Bold", 10)).ColorAndOpacity(SecondaryText)]
                .BodyContent()
                [
                    SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight()[NumberRow<double>(LOCTEXT("AtmosphereLinearR", "LINEAR COEFFICIENT / RED"), 0.0, 64.0, 0.1, VM, [](const UGeneratedWorld* W){ return FMath::IsFinite(W->AtmosphereColor.R) ? static_cast<double>(W->AtmosphereColor.R) : 0.0; }, [](UWorldGenerationViewModel* V, double X){ if(V->GeneratedWorld){V->GeneratedWorld->AtmosphereColor.R=static_cast<float>(X); V->RefreshPlanetAppearancePreview(false);} })]
		+ SVerticalBox::Slot().AutoHeight()[NumberRow<double>(LOCTEXT("AtmosphereLinearG", "LINEAR COEFFICIENT / GREEN"), 0.0, 64.0, 0.1, VM, [](const UGeneratedWorld* W){ return FMath::IsFinite(W->AtmosphereColor.G) ? static_cast<double>(W->AtmosphereColor.G) : 0.0; }, [](UWorldGenerationViewModel* V, double X){ if(V->GeneratedWorld){V->GeneratedWorld->AtmosphereColor.G=static_cast<float>(X); V->RefreshPlanetAppearancePreview(false);} })]
		+ SVerticalBox::Slot().AutoHeight()[NumberRow<double>(LOCTEXT("AtmosphereLinearB", "LINEAR COEFFICIENT / BLUE"), 0.0, 64.0, 0.1, VM, [](const UGeneratedWorld* W){ return FMath::IsFinite(W->AtmosphereColor.B) ? static_cast<double>(W->AtmosphereColor.B) : 0.0; }, [](UWorldGenerationViewModel* V, double X){ if(V->GeneratedWorld){V->GeneratedWorld->AtmosphereColor.B=static_cast<float>(X); V->RefreshPlanetAppearancePreview(false);} })]
                ]
            ];
    }

	class SPreviewInteractionSurface final : public SCompoundWidget
	{
	public:
		SLATE_BEGIN_ARGS(SPreviewInteractionSurface) {}
			SLATE_ARGUMENT(TWeakObjectPtr<UWorldGenerationViewModel>, ViewModel)
		SLATE_END_ARGS()

		void Construct(const FArguments& InArgs)
		{
			ViewModel = InArgs._ViewModel;
			ChildSlot
			[
				// Intentionally empty: this hit-test surface sits over the real scene.
				// No translucent card, labels or fake render target may cover the world.
				SNew(SBox)
			];
		}

		virtual FReply OnMouseButtonDown(const FGeometry&, const FPointerEvent& Event) override
		{
			if (Event.GetEffectingButton() == EKeys::RightMouseButton || Event.GetEffectingButton() == EKeys::MiddleMouseButton)
			{
				if (UWorldGenerationViewModel* VM = ViewModel.Get()) VM->BeginPreviewOrbit();
				return FReply::Handled().CaptureMouse(SharedThis(this));
			}
			if (Event.GetEffectingButton() == EKeys::LeftMouseButton)
			{
				// Rio 02.10: a click selects a cluster system (its record), a double-click focuses it.
				if (UWorldGenerationViewModel* VM = ViewModel.Get(); VM && VM->SelectPreviewUnderCursor())
				{
					return FReply::Handled();
				}
			}
			return FReply::Unhandled();
		}

		virtual FReply OnMouseButtonUp(const FGeometry&, const FPointerEvent& Event) override
		{
			if (HasMouseCapture())
			{
				if (UWorldGenerationViewModel* VM = ViewModel.Get()) VM->EndPreviewOrbit();
				return FReply::Handled().ReleaseMouseCapture();
			}
			return FReply::Unhandled();
		}

		virtual void OnMouseCaptureLost(const FCaptureLostEvent& CaptureLostEvent) override
		{
			if (UWorldGenerationViewModel* VM = ViewModel.Get()) VM->EndPreviewOrbit();
			SCompoundWidget::OnMouseCaptureLost(CaptureLostEvent);
		}

		virtual FReply OnMouseButtonDoubleClick(const FGeometry&, const FPointerEvent& Event) override
		{
			if (Event.GetEffectingButton() == EKeys::LeftMouseButton)
			{
				if (UWorldGenerationViewModel* VM = ViewModel.Get(); VM && VM->FocusPreviewUnderCursor())
				{
					return FReply::Handled();
				}
			}
			return FReply::Unhandled();
		}

		virtual FReply OnMouseMove(const FGeometry&, const FPointerEvent& Event) override
		{
			if (HasMouseCapture())
			{
				if (UWorldGenerationViewModel* VM = ViewModel.Get()) VM->OrbitPreview(Event.GetCursorDelta());
				return FReply::Handled();
			}
			return FReply::Unhandled();
		}

		virtual FReply OnMouseWheel(const FGeometry&, const FPointerEvent& Event) override
		{
			if (UWorldGenerationViewModel* VM = ViewModel.Get()) VM->ZoomPreview(Event.GetWheelDelta());
			return FReply::Handled();
		}

	private:
		TWeakObjectPtr<UWorldGenerationViewModel> ViewModel;
	};

	class SPreviewSystemOverlay final : public SLeafWidget
	{
	public:
		SLATE_BEGIN_ARGS(SPreviewSystemOverlay) {}
			SLATE_ARGUMENT(TWeakObjectPtr<UWorldGenerationViewModel>, ViewModel)
		SLATE_END_ARGS()

		void Construct(const FArguments& InArgs)
		{
			ViewModel = InArgs._ViewModel;
			SetVisibility(EVisibility::HitTestInvisible);
			SetClipping(EWidgetClipping::ClipToBounds);
		}

		virtual FVector2D ComputeDesiredSize(float) const override
		{
			return FVector2D(100.0f, 100.0f);
		}

		virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override
		{
			SLeafWidget::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);
			const UWorldGenerationViewModel* VM = ViewModel.Get();
			UWorld* World = VM ? VM->GetWorld() : nullptr;
			APlayerController* PC = World ? World->GetFirstPlayerController() : nullptr;
			// The menu's preview generator, not whatever actor the camera happens to view through.
			AAstroGenerator* Generator = PC ? VM->GetPreviewGenerator() : nullptr;
			const ULocalPlayer* Player = PC ? PC->GetLocalPlayer() : nullptr;
			if (!Generator || !Generator->UsesContinuousPreviewFrame() || !Player || !Player->ViewportClient) return;
			const TSharedPtr<SViewport> Viewport = Player->ViewportClient->GetGameViewportWidget();
			FSceneViewProjectionData Projection;
			if (!Viewport || !Player->GetProjectionData(Player->ViewportClient->Viewport, Projection)) return;
			const FGeometry ViewGeometry = Viewport->GetCachedGeometry();
			const FVector2D ViewSize = ViewGeometry.GetLocalSize();
			int32 Width = 0, Height = 0;
			PC->GetViewportSize(Width, Height);
			if (ViewSize.X <= 0 || ViewSize.Y <= 0 || Width <= 0 || Height <= 0) return;
			const FVector2D PixelScale(Width / ViewSize.X, Height / ViewSize.Y);
			const FVector2D Min = ViewGeometry.AbsoluteToLocal(AllottedGeometry.LocalToAbsolute(FVector2D::ZeroVector)) * PixelScale;
			const FVector2D Max = ViewGeometry.AbsoluteToLocal(AllottedGeometry.LocalToAbsolute(AllottedGeometry.GetLocalSize())) * PixelScale;
			const FIntRect Rect = Projection.GetConstrainedViewRect();
			const FVector2D Center((Rect.Min.X + Rect.Max.X) * 0.5, (Rect.Min.Y + Rect.Max.Y) * 0.5);
			const double Horizontal = FMath::Min(Center.X - Min.X, Max.X - Center.X)
				/ (Rect.Width() * 0.5 * Projection.ProjectionMatrix.M[0][0]);
			const double Vertical = FMath::Min(Center.Y - Min.Y, Max.Y - Center.Y)
				/ (Rect.Height() * 0.5 * Projection.ProjectionMatrix.M[1][1]);
			Generator->SetContinuousPreviewFramingTangent(FMath::Min(Horizontal, Vertical));
		}

		virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry,
			const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements,
			int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override
		{
			CSV_SCOPED_TIMING_STAT(APSPreview, AnnotationPaint);
			const UWorldGenerationViewModel* VM = ViewModel.Get();
			if (!VM || !VM->bPreviewReady)
			{
				return LayerId;
			}
			const EAstroPreviewFocus Focus = VM->GetPreviewFocus();
			if (Focus == EAstroPreviewFocus::Overview)
			{
				return LayerId;
			}
			if (VM->ArePreviewMarksHidden())
			{
				// Rio 03.10 ("after REGENERATE the orbits vanish, nothing highlights"): hidden marks were a process-wide
				// switch that outlived PIE sessions without a trace on screen. Now per menu session, and always said here.
				const FVector2D HintSize = AllottedGeometry.GetLocalSize();
				FSlateDrawElement::MakeText(OutDrawElements, LayerId + 1,
					AllottedGeometry.ToPaintGeometry(FVector2D(FMath::Max(HintSize.X - 8.0, 1.0), 16.0),
						FSlateLayoutTransform(FVector2D(4.0, FMath::Max(0.0, HintSize.Y - 18.0)))),
					LOCTEXT("PreviewMarksHiddenHint", "MARKS OFF: orbits, rings and labels are hidden (MARKS ON shows them)"),
					ReadableFont("Regular", 8), ESlateDrawEffect::None, FLinearColor(Amber.R, Amber.G, Amber.B, 0.9f));
				return LayerId + 1;
			}

			UWorld* World = VM->GetWorld();
			APlayerController* PC = World ? World->GetFirstPlayerController() : nullptr;
			if (!PC || !GEngine || !GEngine->GameViewport)
			{
				return LayerId;
			}

			int32 ViewWidth = 0;
			int32 ViewHeight = 0;
			PC->GetViewportSize(ViewWidth, ViewHeight);
			const TSharedPtr<SViewport> ViewportWidget = GEngine->GameViewport->GetGameViewportWidget();
			if (!ViewportWidget.IsValid() || ViewWidth <= 0 || ViewHeight <= 0)
			{
				return LayerId;
			}
			// Paint space, like AllottedGeometry: GetCachedGeometry() is the desktop-space tick geometry, and mixing the
			// two shifted every marker by the window's position (Rio 02.10: all rings ~8 px up-left in the editor).
			const FGeometry ViewportGeometry = ViewportWidget->GetPaintSpaceGeometry();
			const FVector2D ViewportLocalSize = ViewportGeometry.GetLocalSize();
			const FVector2D PanelSize = AllottedGeometry.GetLocalSize();
			const ULocalPlayer* LocalPlayer = PC->GetLocalPlayer();
			FSceneViewProjectionData Projection;
			if (!LocalPlayer || !LocalPlayer->ViewportClient
				|| !LocalPlayer->GetProjectionData(LocalPlayer->ViewportClient->Viewport, Projection)) return LayerId;
			// Same projection contract as UGameplayStatics::ProjectWorldToScreen,
			// captured once for the complete paint rather than recalculated for each
			// orbit vertex/clip endpoint. All guides then share one camera snapshot.
			const FMatrix ViewProjection = Projection.ComputeViewProjectionMatrix();
			const FIntRect ViewRect = Projection.GetConstrainedViewRect();

			const auto ProjectToPanel = [&](const FVector& WorldPosition, FVector2D& OutPanelPosition)
			{
				FVector2D ScreenPosition;
				if (!FSceneView::ProjectWorldToScreen(WorldPosition, ViewRect, ViewProjection, ScreenPosition))
				{
					return false;
				}
				ScreenPosition -= FVector2D(ViewRect.Min);
				if (!PC->PostProcessWorldToScreen(WorldPosition, ScreenPosition, true)) return false;
				const FVector2D ViewportLocal(
					ScreenPosition.X * ViewportLocalSize.X / static_cast<float>(ViewWidth),
					ScreenPosition.Y * ViewportLocalSize.Y / static_cast<float>(ViewHeight));
				OutPanelPosition = AllottedGeometry.AbsoluteToLocal(ViewportGeometry.LocalToAbsolute(ViewportLocal));
				return FMath::IsFinite(OutPanelPosition.X) && FMath::IsFinite(OutPanelPosition.Y);
			};
			const auto PresentationLocation = [VM](const AActor* Actor)
			{
				FVector Location = IsValid(Actor) ? Actor->GetActorLocation() : FVector::ZeroVector;
				if (IsValid(Actor))
				{
					VM->GetPreviewPresentationLocation(Actor, Location);
				}
				return Location;
			};
			const AAstroGenerator* PreviewGenerator = VM->GetPreviewGenerator();
			const AAstroGenerator* ContinuousGenerator = PreviewGenerator;
			if (ContinuousGenerator && !ContinuousGenerator->UsesContinuousPreviewFrame()) ContinuousGenerator = nullptr;

			// Galaxy and cluster retain one low-cost projected outline. STAR/SYSTEM use
			// real translucent 3D shells owned by AAstroGenerator, so Slate must never
			// draw a second camera-facing representation for those scopes.
			const FRotator ScopeViewRotation = IsValid(PC->PlayerCameraManager)
				? PC->PlayerCameraManager->GetCameraRotation() : FRotator::ZeroRotator;
			const FRotationMatrix ViewBasis(ScopeViewRotation);
			const FVector RingRight = ViewBasis.GetUnitAxis(EAxis::Y);
			const FVector RingUp = ViewBasis.GetUnitAxis(EAxis::Z);
			const auto DrawScopeRing = [&](EAstroPreviewFocus SphereFocus, double RadiusMultiplier,
				const FLinearColor& ScopeColor)
			{
				FVector ScopeCenter;
				double ScopeRadius = 0.0;
				if (!VM->GetPreviewFocusSphere(SphereFocus, ScopeCenter, ScopeRadius))
				{
					return;
				}
				const double VisualScopeRadius = ScopeRadius * RadiusMultiplier;
				TArray<FVector2D> Segment;
				const TArray<FVector2D>& Circle = UnitCircleSamples<72>();
				Segment.Reserve(Circle.Num());
				// Rio 04.10 evening ("the cluster outline is gone"): a thin translucent line vanished over a bright cluster
				// (giants, the GPU glow); a dark under-stroke keeps it readable on any background.
				const auto EmitSegment = [&]()
				{
					FSlateDrawElement::MakeLines(OutDrawElements, LayerId,
						AllottedGeometry.ToPaintGeometry(), Segment, ESlateDrawEffect::None,
						APSUITheme::Retint(FLinearColor(0.0f, 0.02f, 0.04f, 0.55f)), true, 2.6f);
					FSlateDrawElement::MakeLines(OutDrawElements, LayerId,
						AllottedGeometry.ToPaintGeometry(), Segment, ESlateDrawEffect::None,
						ScopeColor, true, 1.1f);
				};
				for (int32 Sample = 0; Sample < Circle.Num(); ++Sample)
				{
					const FVector2D& UnitPoint = Circle[Sample];
					FVector2D Point;
					if (ProjectToPanel(ScopeCenter
						+ (RingRight * UnitPoint.X + RingUp * UnitPoint.Y) * VisualScopeRadius,
						Point)
						&& Point.X > -PanelSize.X && Point.X < PanelSize.X * 2.0f
						&& Point.Y > -PanelSize.Y && Point.Y < PanelSize.Y * 2.0f)
					{
						Segment.Add(Point);
					}
					else
					{
						if (Segment.Num() > 1)
						{
							EmitSegment();
						}
						Segment.Reset();
					}
				}
				if (Segment.Num() > 1)
				{
					EmitSegment();
				}
			};

			if (Focus == EAstroPreviewFocus::Galaxy)
			{
				DrawScopeRing(Focus, 1.0, FLinearColor(0.52f, 0.32f, 1.0f, 0.34f));
			}
			else if (Focus == EAstroPreviewFocus::StarCluster)
			{
				DrawScopeRing(Focus, 1.0, FLinearColor(Cyan.R, Cyan.G, Cyan.B, 0.55f));
			}
			// Rio 05.10 ("a ruler would be fun", with the REAL SCALE experiment): a scale bar in the bottom right, a round
			// length in km, AU or light years at the focus's distance. The continuous preview keeps every direction exact,
			// so that physical length spans the same angle as the bar on screen.
			double FocusDistanceCm = 0.0;
			const double InverseHalfWidthTangent = Projection.ProjectionMatrix.M[0][0];
			if (ContinuousGenerator && ContinuousGenerator->GetPreviewFocusPhysicalDistance(Focus, FocusDistanceCm)
				&& InverseHalfWidthTangent > UE_SMALL_NUMBER && ViewportLocalSize.X > 1.0)
			{
				constexpr double KmCm = 1.0e5;
				constexpr double AuCm = 1.495978707e13;
				constexpr double LyCm = 9.4607304725808e17;
				const double CmPerPixel = FocusDistanceCm / (InverseHalfWidthTangent * ViewportLocalSize.X * 0.5);
				const double WantedCm = 150.0 * CmPerPixel;
				const double UnitCm = WantedCm >= 0.05 * LyCm ? LyCm : WantedCm >= 0.02 * AuCm ? AuCm : KmCm;
				const double Wanted = WantedCm / UnitCm;
				const double Decade = FMath::Pow(10.0, FMath::FloorToDouble(FMath::LogX(10.0, Wanted)));
				const double Step = Wanted >= 5.0 * Decade ? 5.0 * Decade : Wanted >= 2.0 * Decade ? 2.0 * Decade : Decade;
				const double BarPixels = Step * UnitCm / CmPerPixel;
				if (FMath::IsFinite(BarPixels) && BarPixels > 8.0 && BarPixels < PanelSize.X * 0.5)
				{
					const FVector2D End(PanelSize.X - 28.0, PanelSize.Y - 26.0);
					const FVector2D Start(End.X - BarPixels, End.Y);
					const TArray<FVector2D> Bar{Start + FVector2D(0.0, -6.0), Start, End, End + FVector2D(0.0, -6.0)};
					FSlateDrawElement::MakeLines(OutDrawElements, LayerId, AllottedGeometry.ToPaintGeometry(), Bar,
						ESlateDrawEffect::None, APSUITheme::Retint(FLinearColor(0.0f, 0.02f, 0.04f, 0.6f)), true, 3.2f);
					FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 1, AllottedGeometry.ToPaintGeometry(), Bar,
						ESlateDrawEffect::None, FLinearColor(Cyan.R, Cyan.G, Cyan.B, 0.85f), true, 1.4f);
					FNumberFormattingOptions Digits;
					Digits.MaximumFractionalDigits = Step < 1.0 ? 3 : 0;
					const FText UnitName = UnitCm == LyCm ? LOCTEXT("RulerLy", "LY")
						: UnitCm == AuCm ? LOCTEXT("RulerAu", "AU") : LOCTEXT("RulerKm", "KM");
					const FText RulerText = FText::Format(LOCTEXT("RulerLabel", "{0} {1}"),
						Step < 1.0 ? FText::AsNumber(Step, &Digits) : APSUINumber::Number(static_cast<int64>(FMath::RoundToDouble(Step))),
						UnitName);
					const FSlateFontInfo RulerFont = ReadableFont("Bold", 9);
					const FVector2D TextSize = FSlateApplication::Get().GetRenderer()->GetFontMeasureService()->Measure(RulerText, RulerFont);
					const FVector2D TextAt(Start.X + (BarPixels - TextSize.X) * 0.5, Start.Y - 8.0 - TextSize.Y);
					// A dark plate under the label keeps it readable over a bright cluster (Rio 05.10); Rio 06.10 ("white
					// on white"): the chip plate with its frame, as the preview's corner texts.
					FSlateDrawElement::MakeBox(OutDrawElements, LayerId,
						AllottedGeometry.ToPaintGeometry(TextSize + FVector2D(14.0, 6.0), FSlateLayoutTransform(TextAt - FVector2D(7.0, 2.0))),
						&ChipBrush, ESlateDrawEffect::None, FLinearColor::White);
					FSlateDrawElement::MakeText(OutDrawElements, LayerId + 1,
						AllottedGeometry.ToPaintGeometry(TextSize + FVector2D(2.0, 2.0), FSlateLayoutTransform(TextAt)),
						RulerText, RulerFont, ESlateDrawEffect::None, FLinearColor(Cyan.R, Cyan.G, Cyan.B, 0.95f));
				}
			}
			AActor* SelectedBody = VM->GetSelectedPreviewBody();
			// A rebuilt hierarchy (REGENERATE, a new seed) must never be annotated from the previous one's actors:
			// refresh on another home system or on any cached body that has since been destroyed.
			const AStarSystem* HomeSystem = PreviewGenerator ? PreviewGenerator->GetPreviewHomeSystem() : nullptr;
			bool bCachedBodyDestroyed = false;
			for (const FAPSPreviewBodyEntry& CachedEntry : CachedEntries)
			{
				if (!CachedEntry.Actor.IsExplicitlyNull() && !CachedEntry.Actor.IsValid())
				{
					bCachedBodyDestroyed = true;
					break;
				}
			}
			if (CachedPreviewRevision != VM->PreviewRevision
				|| CachedPreviewFocus != Focus
				|| CachedSelectedBody.Get() != SelectedBody
				|| CachedHomeSystem.Get() != HomeSystem
				|| bCachedBodyDestroyed)
			{
				CachedEntries.Reset();
				VM->GetPreviewBodyEntries(CachedEntries);
				// Rio 02.10: line 1 the type (spectrum and class of a star, type and radius of a planet or moon), line 2 the
				// full name and its designation (A1, A5.04). The box fits the text with even paddings.
				CachedDesignations.Reset();
				CachedLabelSizes.Reset();
				CachedLabelLayouts.Reset();
				for (const FAPSPreviewBodyEntry& Entry : CachedEntries)
				{
					// Rio 02.10: the designation follows the name on line 2; line 1 is the type alone.
					const FString Designation = APSBodyDesignation::Of(Entry.Actor.Get());
					const FLabelLayout& Layout = CachedLabelLayouts.Add_GetRef(LayoutLabel(Entry.Details, Entry.Label, Designation));
					CachedDesignations.Add(FText::FromString(Designation));
					CachedLabelSizes.Add(Layout.Size);
				}
				CachedPreviewRevision = VM->PreviewRevision;
				CachedPreviewFocus = Focus;
				CachedSelectedBody = SelectedBody;
				CachedHomeSystem = HomeSystem;
			}
			const TArray<FAPSPreviewBodyEntry>& Entries = CachedEntries;
			if (Entries.IsEmpty())
			{
				return LayerId;
			}
			FVector ViewLocation = FVector::ZeroVector;
			FRotator ViewRotation = FRotator::ZeroRotator;
			PC->GetPlayerViewPoint(ViewLocation, ViewRotation);
			const FQuat ViewQuaternion = ViewRotation.Quaternion();
			const FVector CameraRight = ViewQuaternion.GetRightVector();
			const FVector CameraUp = ViewQuaternion.GetUpVector();
			const FVector CameraForward = ViewQuaternion.GetForwardVector();
			TArray<FAPSPreviewOccluder> Occluders;
			TArray<const AActor*> OccluderBodies;
			for (const FAPSPreviewBodyEntry& Entry : Entries)
			{
				const AActor* Body = Entry.Actor.Get();
				if (!Body || OccluderBodies.Contains(Body)) continue;
				double Radius = 0.0;
				if (!PreviewGenerator || !PreviewGenerator->GetPreviewPresentationRadius(Body, Radius)) continue;
				const FVector Center = PresentationLocation(Body) - ViewLocation;
				// Ignore unresolved subpixel discs, not their optical glow. Neither an
				// atmosphere nor an invisible gravity-zone component is an opaque body.
				if (Radius <= 0.0 || Radius * ViewHeight < Center.Size() * 0.1
					|| FVector::DotProduct(Center, CameraForward) + Radius <= 0.0) continue;
				Occluders.Add({Center, Radius});
				OccluderBodies.Add(Body);
			}

			// World-space orbit planes are sampled and projected every paint. They remain
			// locked to the generated actors while the preview camera orbits and zooms.
			// Keep curves visibly round at 1440p while retaining a strict per-orbit cap.
			// Even a twenty-planet SYSTEM stays below 1,300 world projections per paint.
			const TArray<FVector2D>& OrbitCircle = Focus == EAstroPreviewFocus::HomeSystem
				? UnitCircleSamples<64>() : UnitCircleSamples<80>();
			TArray<FVector2D> OrbitSegment;
			OrbitSegment.Reserve(OrbitCircle.Num());
			TArray<FVector2D> VisibleIntervals;
			for (const FAPSPreviewBodyEntry& Entry : Entries)
			{
				if (Focus == EAstroPreviewFocus::HomeSystem && Entry.Depth > 1)
				{
					// The system overview labels planets; their moons remain available in
					// the hierarchy and become visible when the planet scope is opened.
					// Drawing both families here made dense systems unreadable.
					continue;
				}
				const AActor* Body = Entry.Actor.Get();
				const APlanetOrbit* Orbit = Body ? Cast<APlanetOrbit>(Body->GetAttachParentActor()) : nullptr;
				if (Focus == EAstroPreviewFocus::HomePlanet && Body && Body->IsA<APlanet>())
				{
					// Planet inspection keeps satellite orbits, but hides the parent system orbit.
					continue;
				}
				if (!Body || !Orbit)
				{
					continue;
				}
				const FVector BodyCenter = PresentationLocation(Body);
				FVector Center = Orbit->GetActorLocation();
				if (const AMoon* Moon = Cast<AMoon>(Body); IsValid(Moon) && IsValid(Moon->ParentPlanet))
				{
					Center = PresentationLocation(Moon->ParentPlanet);
				}
				else if (const APlanet* Planet = Cast<APlanet>(Body);
					IsValid(Planet) && IsValid(Planet->ParentStar))
				{
					Center = PresentationLocation(Planet->ParentStar);
				}
				const double Radius = FVector::Distance(Center, BodyCenter);
				if (!FMath::IsFinite(Radius) || Radius <= UE_SMALL_NUMBER)
				{
					continue;
				}
				const FVector AxisX = Orbit->GetActorQuat().GetAxisX();
				const FVector AxisY = Orbit->GetActorQuat().GetAxisY();
				FVector PhysicalOrbitCenter = Orbit->GetActorLocation();
				if (const AMoon* Moon = Cast<AMoon>(Body); Moon && IsValid(Moon->ParentPlanet))
					PhysicalOrbitCenter = Moon->ParentPlanet->GetActorLocation();
				else if (const APlanet* Planet = Cast<APlanet>(Body); Planet && IsValid(Planet->ParentStar))
					PhysicalOrbitCenter = Planet->ParentStar->GetActorLocation();
				const double PhysicalOrbitRadius = FVector::Distance(PhysicalOrbitCenter, Body->GetActorLocation());
				OrbitSegment.Reset();
				// Rio 06.10 ("strengthen the orbits like in the game"): a dark stroke under the line keeps it apart from the
				// stars, then a soft glow and a bright core; brighter towards the body, as the HUD's rings in flight.
				const FVector BodyDirection = Body->GetActorLocation() - PhysicalOrbitCenter;
				const double BodyAngle = FMath::Atan2(FVector::DotProduct(BodyDirection, AxisY), FVector::DotProduct(BodyDirection, AxisX));
				const bool bMoonOrbit = Entry.Depth > 1;
				const FLinearColor OrbitColour = bMoonOrbit ? FLinearColor(0.36f, 0.65f, 1.0f, 1.0f) : FLinearColor(Cyan.R, Cyan.G, Cyan.B, 1.0f);
				double SegmentAngleSum = 0.0;
				int32 SegmentAngleCount = 0;
				double SampleAngle = 0.0;
				const auto FlushOrbit = [&]()
				{
					if (OrbitSegment.Num() > 1)
					{
						const double Middle = SegmentAngleCount > 0 ? SegmentAngleSum / SegmentAngleCount - BodyAngle : UE_DOUBLE_PI;
						const float Near = FMath::Pow(0.5f + 0.5f * static_cast<float>(FMath::Cos(Middle)), 1.6f);
						const float Alpha = (bMoonOrbit ? 0.42f : 0.55f) + (bMoonOrbit ? 0.30f : 0.40f) * Near;
						const FPaintGeometry Paint = AllottedGeometry.ToPaintGeometry();
						FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 1, Paint, OrbitSegment, ESlateDrawEffect::None,
							APSUITheme::Retint(FLinearColor(0.0f, 0.008f, 0.016f, bMoonOrbit ? 0.45f : 0.6f)), true, bMoonOrbit ? 3.4f : 4.6f);
						FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 2, Paint, OrbitSegment, ESlateDrawEffect::None,
							FLinearColor(OrbitColour.R, OrbitColour.G, OrbitColour.B, Alpha * 0.2f), true, bMoonOrbit ? 3.0f : 4.4f);
						FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 2, Paint, OrbitSegment, ESlateDrawEffect::None,
							FLinearColor(OrbitColour.R, OrbitColour.G, OrbitColour.B, Alpha), true, bMoonOrbit ? 1.0f : 1.6f);
					}
					OrbitSegment.Reset();
					SegmentAngleSum = 0.0;
					SegmentAngleCount = 0;
				};
				FVector PreviousPoint = FVector::ZeroVector;
				bool bPreviousValid = false;
				for (const FVector2D& UnitPoint : OrbitCircle)
				{
					SampleAngle = FMath::Atan2(UnitPoint.Y, UnitPoint.X);
					const FVector PlaneDirection = AxisX * UnitPoint.X + AxisY * UnitPoint.Y;
					FVector PresentedOrbitPoint = Center + PlaneDirection * Radius;
					const bool bValid = !ContinuousGenerator || ContinuousGenerator->ProjectContinuousPreviewWorldPosition(
						PhysicalOrbitCenter + PlaneDirection * PhysicalOrbitRadius, PresentedOrbitPoint, Body);
					const FVector CurrentPoint = PresentedOrbitPoint - ViewLocation;
					if (bValid && bPreviousValid)
					{
						FVector A = PreviousPoint;
						FVector B = CurrentPoint;
						const double DepthA = FVector::DotProduct(A, CameraForward);
						const double DepthB = FVector::DotProduct(B, CameraForward);
						// Clip at the camera plane before projecting; a curve straddling
						// the observer must never connect across the back of the screen.
						if (FMath::Max(DepthA, DepthB) > 1.0)
						{
							if (DepthA <= 1.0) A = FMath::Lerp(A, B, (1.0 - DepthA) / (DepthB - DepthA));
							else if (DepthB <= 1.0) B = FMath::Lerp(A, B, (1.0 - DepthA) / (DepthB - DepthA));
							APSPreviewVisibility::VisibleIntervals(A, B, Occluders, VisibleIntervals);
							if (VisibleIntervals.IsEmpty()) FlushOrbit();
							for (const FVector2D& Interval : VisibleIntervals)
							{
								FVector2D Start, End;
								if (!ProjectToPanel(ViewLocation + FMath::Lerp(A, B, Interval.X), Start)
									|| !ProjectToPanel(ViewLocation + FMath::Lerp(A, B, Interval.Y), End)
									|| !APSPreviewVisibility::ClipToPanel(Start, End, PanelSize))
								{ FlushOrbit(); continue; }
								if (!OrbitSegment.IsEmpty() && !OrbitSegment.Last().Equals(Start, 0.01)) FlushOrbit();
								if (OrbitSegment.IsEmpty()) OrbitSegment.Add(Start);
								OrbitSegment.Add(End);
								// Short runs, so the brightness can follow the angle to the body (the sum keeps the
								// angles continuous across the turn's seam).
								const double Unwrapped = SegmentAngleCount > 0
									? SampleAngle + UE_DOUBLE_TWO_PI * FMath::RoundToDouble(
										(SegmentAngleSum / SegmentAngleCount - SampleAngle) / UE_DOUBLE_TWO_PI)
									: SampleAngle;
								SegmentAngleSum += Unwrapped;
								++SegmentAngleCount;
								if (Interval.Y < 1.0) FlushOrbit();
								else if (OrbitSegment.Num() >= 7)
								{
									const FVector2D Joint = OrbitSegment.Last();
									FlushOrbit();
									OrbitSegment.Add(Joint);
								}
							}
						}
						else FlushOrbit();
					}
					else FlushOrbit();
					PreviousPoint = CurrentPoint;
					bPreviousValid = bValid;
				}
				FlushOrbit();
			}

			TArray<FAPSPreviewAnnotationCandidate> AnnotationCandidates;
			// Rio 02.10: a body that no longer fits in the view whole docks its plate under the caption (the selected
			// body first, else the largest on screen).
			int32 DockedIndex = INDEX_NONE;
			double DockedPixels = 0.0;
			bool bDockedSelected = false;
			for (int32 EntryIndex = 0; EntryIndex < Entries.Num(); ++EntryIndex)
			{
				const FAPSPreviewBodyEntry& Entry = Entries[EntryIndex];
				if (Focus == EAstroPreviewFocus::HomeSystem && Entry.Depth > 1)
				{
					continue;
				}
				const AActor* Body = Entry.Actor.Get();
				if (!Body && !Entry.bHasExplicitWorldAnchor)
				{
					continue;
				}
				FVector2D Anchor;
				FVector BodyCenter = Entry.bHasExplicitWorldAnchor
					? Entry.ExplicitWorldAnchor : PresentationLocation(Body);
				if (ContinuousGenerator && Entry.ClusterSystemInstanceIndex != INDEX_NONE)
					ContinuousGenerator->GetContinuousPreviewClusterLocation(Entry.ClusterSystemInstanceIndex, BodyCenter);
				const bool bProjected = ProjectToPanel(BodyCenter, Anchor);
				const bool bAnchorInside = bProjected
					&& Anchor.X >= 0.0f && Anchor.X <= PanelSize.X
					&& Anchor.Y >= 0.0f && Anchor.Y <= PanelSize.Y;
				double DockRadius = 0.0;
				FVector2D DockLimb;
				if (Body && bProjected && PreviewGenerator
					&& PreviewGenerator->GetPreviewPresentationRadius(Body, DockRadius) && DockRadius > 0.0
					&& ProjectToPanel(BodyCenter + CameraRight * DockRadius, DockLimb))
				{
					const double Pixels = FVector2D::Distance(DockLimb, Anchor);
					const bool bFits = Anchor.X - Pixels >= 0.0 && Anchor.X + Pixels <= PanelSize.X
						&& Anchor.Y - Pixels >= 0.0 && Anchor.Y + Pixels <= PanelSize.Y;
					const double ToPanel = FVector2D::Distance(Anchor, FVector2D(
						FMath::Clamp(Anchor.X, 0.0, PanelSize.X), FMath::Clamp(Anchor.Y, 0.0, PanelSize.Y)));
					if (!bFits && ToPanel < Pixels && Pixels >= 0.08 * FMath::Min(PanelSize.X, PanelSize.Y))
					{
						const bool bSelectedBody = SelectedBody == Body;
						if (DockedIndex == INDEX_NONE || (bSelectedBody && !bDockedSelected)
							|| (bSelectedBody == bDockedSelected && Pixels > DockedPixels))
						{
							DockedIndex = EntryIndex;
							DockedPixels = Pixels;
							bDockedSelected = bSelectedBody;
						}
						continue;
					}
				}
				if (!bAnchorInside)
				{
					// Object inspection stays intentionally local. SYSTEM, however, promises
					// one marker per planet, so an off-screen/behind-camera body keeps a
					// marker at the nearest panel edge, not another floating text rack.
					if (Focus != EAstroPreviewFocus::HomeSystem)
					{
						continue;
					}
					if (bProjected)
					{
						Anchor.X = FMath::Clamp(Anchor.X, 2.0f, FMath::Max(2.0f, PanelSize.X - 2.0f));
						Anchor.Y = FMath::Clamp(Anchor.Y, 2.0f, FMath::Max(2.0f, PanelSize.Y - 2.0f));
					}
					else
					{
						const FVector BodyDirection =
							(BodyCenter - ViewLocation).GetSafeNormal();
						Anchor.X = FVector::DotProduct(BodyDirection, CameraRight) >= 0.0
							? FMath::Max(2.0f, PanelSize.X - 2.0f) : 2.0f;
						Anchor.Y = FMath::Clamp(
							PanelSize.Y * (0.5f - 0.45f * FVector::DotProduct(BodyDirection, CameraUp)),
							2.0f, FMath::Max(2.0f, PanelSize.Y - 2.0f));
					}
				}

				// Labels use the same exact opaque spheres as orbit visibility. Hidden
				// mesh/zone bounds and a camera-facing radius approximation disagree
				// with the rendered limb, especially when looking past a close planet.
				bool bOccluded = false;
				if (bAnchorInside && Focus != EAstroPreviewFocus::HomeSystem)
				{
					for (int32 Index = 0; Index < Occluders.Num(); ++Index)
					{
						if (OccluderBodies[Index] != Body && Occluders[Index].Occludes(BodyCenter - ViewLocation))
						{
							bOccluded = true;
							break;
						}
					}
				}
				if (bOccluded)
				{
					continue;
				}

				AnnotationCandidates.Add({EntryIndex, Anchor, Entry.bHighlighted || (Body && SelectedBody == Body), Body && Body->IsA<AStar>(),
					CachedLabelSizes.IsValidIndex(EntryIndex) ? CachedLabelSizes[EntryIndex]
						: APSPreviewAnnotationLayout::LabelSize()});
			}
			auto Placements = APSPreviewAnnotationLayout::Arrange(MoveTemp(AnnotationCandidates), PanelSize);
			if (Entries.IsValidIndex(DockedIndex))
			{
				// Under the caption, on its left edge; no ring and no leader.
				FAPSPreviewAnnotationPlacement& Docked = Placements.AddDefaulted_GetRef();
				Docked.Candidate.EntryIndex = DockedIndex;
				Docked.Candidate.bSelected = bDockedSelected;
				Docked.Candidate.Size = CachedLabelSizes.IsValidIndex(DockedIndex) ? CachedLabelSizes[DockedIndex]
					: APSPreviewAnnotationLayout::LabelSize();
				Docked.LabelPosition = FVector2D(10.0, 2.0);
				Docked.bHasLabel = true;
			}
			int32 FullLabelCount = 0;
			for (const FAPSPreviewAnnotationPlacement& Placement : Placements)
			{
				const FAPSPreviewBodyEntry& Entry = Entries[Placement.Candidate.EntryIndex];
				const FVector2D LabelSize = Placement.Candidate.Size;
				const FText Designation = CachedDesignations.IsValidIndex(Placement.Candidate.EntryIndex)
					? CachedDesignations[Placement.Candidate.EntryIndex] : FText::GetEmpty();
				const AActor* Body = Entry.Actor.Get();
				const FVector2D Anchor = Placement.Candidate.Anchor;
				// Rio 02.10 ("A, A1 ride on the plate while the camera turns"): the plate and its texts share one origin on
				// the pixel grid; at a fractional origin each text snapped on its own and drifted against the box.
				const FVector2D LabelAbsolute = AllottedGeometry.LocalToAbsolute(Placement.LabelPosition);
				const FVector2D LabelPosition = AllottedGeometry.AbsoluteToLocal(
					FVector2D(FMath::RoundToDouble(LabelAbsolute.X), FMath::RoundToDouble(LabelAbsolute.Y)));
				const bool bSelected = Placement.Candidate.bSelected;
				const bool bDocked = Placement.Candidate.EntryIndex == DockedIndex;
				FLinearColor MarkerColor = Entry.Depth == 0 ? Amber
					: Entry.Depth == 1 ? Cyan : FLinearColor(0.44f, 0.72f, 1.0f, 1.0f);
				if (const APlanet* Planet = Cast<APlanet>(Body))
				{
					switch (Planet->PlanetType)
					{
					case EPlanetType::Melted:
					case EPlanetType::Volcanic:
					case EPlanetType::Lava:
					case EPlanetType::HotGiant:
						MarkerColor = FLinearColor(1.0f, 0.25f, 0.08f, 1.0f); break;
					case EPlanetType::GasGiant:
						MarkerColor = FLinearColor(1.0f, 0.66f, 0.18f, 1.0f); break;
					case EPlanetType::IceGiant:
					case EPlanetType::Ice:
					case EPlanetType::Frozen:
						MarkerColor = FLinearColor(0.42f, 0.82f, 1.0f, 1.0f); break;
					case EPlanetType::Ocean:
					case EPlanetType::Water:
					case EPlanetType::Archipelago:
						MarkerColor = FLinearColor(0.12f, 0.56f, 1.0f, 1.0f); break;
					case EPlanetType::Terrestrial:
					case EPlanetType::Forest:
					case EPlanetType::Oasis:
						MarkerColor = FLinearColor(0.20f, 0.92f, 0.55f, 1.0f); break;
					case EPlanetType::Desert:
					case EPlanetType::Sand:
						MarkerColor = FLinearColor(0.96f, 0.72f, 0.28f, 1.0f); break;
					case EPlanetType::Metal:
					case EPlanetType::Metallic:
					case EPlanetType::Carbon:
						MarkerColor = FLinearColor(0.74f, 0.64f, 0.92f, 1.0f); break;
					default:
						break;
					}
				}
				// Rio 06.10: in the dark themes the picked body is marked like any other label (bar on the left) in the
				// bright action colour; the full outline in the action red read as a black edge. Classic keeps its amber
				// outline.
				const bool bClassicTheme = APSUITheme::Current() == EAPSUITheme::Classic;
				const FLinearColor PickedColour = bClassicTheme ? Amber : APSUITheme::Palette().ActionPeak;
				if (bSelected)
				{
					MarkerColor = PickedColour;
				}
				// Markers are independent of the text budget. Suppressing an annotation
				// box must never hide a body or move its physical selection anchor.
				// Rio 02.10: a ring that outlines the body on its projected limb, not a cross; it gives way once the
				// body itself fills a good part of the view.
				float RingRadius = 7.0f;
				bool bRing = !bDocked;
				double PresentedRadius = 0.0;
				const bool bInsidePanel = Anchor.X > 2.5f && Anchor.Y > 2.5f
					&& Anchor.X < PanelSize.X - 2.5f && Anchor.Y < PanelSize.Y - 2.5f;
				// A system's ring outlines its star (Rio 02.10: a super giant did not fit its system's ring, however far out).
				const AActor* RingBody = Body;
				if (const AStarSystem* System = Cast<AStarSystem>(Body); System && IsValid(System->MainStar))
				{
					RingBody = System->MainStar;
				}
				if (!bDocked && RingBody && bInsidePanel && PreviewGenerator
					&& PreviewGenerator->GetPreviewPresentationRadius(RingBody, PresentedRadius) && PresentedRadius > 0.0)
				{
					FVector2D Limb;
					FVector2D BodyCenter;
					if (ProjectToPanel(PresentationLocation(RingBody) + CameraRight * PresentedRadius, Limb)
						&& ProjectToPanel(PresentationLocation(RingBody), BodyCenter))
					{
						const float Projected = static_cast<float>(FVector2D::Distance(Limb, BodyCenter));
						// Room around the limb (Rio 02.10: "borders with a margin, not tight").
						RingRadius = FMath::Max(7.0f, Projected * 1.15f + 7.0f);
						bRing = Projected < FMath::Min(PanelSize.X, PanelSize.Y) * 0.16f;
					}
				}
				if (bRing)
				{
					TArray<FVector2D> Ring;
					const int32 Segments = FMath::Clamp(FMath::CeilToInt(RingRadius * 0.9f), 20, 72);
					Ring.Reserve(Segments + 1);
					for (int32 Point = 0; Point <= Segments; ++Point)
					{
						const float Angle = UE_TWO_PI * static_cast<float>(Point) / Segments;
						Ring.Add(Anchor + FVector2D(FMath::Cos(Angle), FMath::Sin(Angle)) * RingRadius);
					}
					FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 3, AllottedGeometry.ToPaintGeometry(), Ring,
						ESlateDrawEffect::None, FLinearColor(MarkerColor.R, MarkerColor.G, MarkerColor.B, bSelected ? 1.0f : 0.85f),
						true, bSelected ? 2.0f : 1.3f);
				}
				if (!Placement.bHasLabel) continue;
				++FullLabelCount;
				const FVector2D PoleEnd(LabelPosition.X + LabelSize.X * 0.5f, LabelPosition.Y + LabelSize.Y);
				// The leader meets the ring, not the body's centre.
				const FVector2D LeaderStart = bRing ? Anchor + (PoleEnd - Anchor).GetSafeNormal() * RingRadius : Anchor;
				if (!bDocked)
				{
					FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 2, AllottedGeometry.ToPaintGeometry(),
						TArray<FVector2D>{LeaderStart, PoleEnd}, ESlateDrawEffect::None,
						FLinearColor(MarkerColor.R, MarkerColor.G, MarkerColor.B, 0.72f), true, 1.0f);
				}
				FSlateDrawElement::MakeBox(OutDrawElements, LayerId + 3,
					AllottedGeometry.ToPaintGeometry(LabelSize, FSlateLayoutTransform(LabelPosition)),
					FAppStyle::GetBrush("WhiteBrush"), ESlateDrawEffect::None,
					APSUITheme::Retint(FLinearColor(0.002f, 0.014f, 0.026f, 0.96f)));
				FSlateDrawElement::MakeBox(OutDrawElements, LayerId + 4,
					AllottedGeometry.ToPaintGeometry(FVector2D(3.0f, LabelSize.Y), FSlateLayoutTransform(LabelPosition)),
					FAppStyle::GetBrush("WhiteBrush"), ESlateDrawEffect::None, MarkerColor);
				if (bSelected && bClassicTheme)
				{
					const TArray<FVector2D> SelectedOutline = {
						LabelPosition,
						LabelPosition + FVector2D(LabelSize.X, 0.0f),
						LabelPosition + LabelSize,
						LabelPosition + FVector2D(0.0f, LabelSize.Y),
						LabelPosition
					};
					FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 4,
						AllottedGeometry.ToPaintGeometry(), SelectedOutline,
						ESlateDrawEffect::None, Amber, true, 2.0f);
				}
				// Rio 02.10: line 1 is the type (spectrum and class, or type and radius); line 2 the name, then the
				// designation (A, A1, A5.04) in the marker colour, both on one baseline though the fonts differ.
				const FLabelLayout Layout = CachedLabelLayouts.IsValidIndex(Placement.Candidate.EntryIndex)
					? CachedLabelLayouts[Placement.Candidate.EntryIndex]
					: LayoutLabel(Entry.Details, Entry.Label, Designation.ToString());
				const float TextLeft = LabelBar + LabelPadX;
				if (!Entry.Details.IsEmpty())
				{
					FSlateDrawElement::MakeText(OutDrawElements, LayerId + 5,
						AllottedGeometry.ToPaintGeometry(FVector2D(LabelSize.X - TextLeft, 18.0f),
							FSlateLayoutTransform(LabelPosition + FVector2D(TextLeft, Layout.TypeTop))),
						Entry.Details, ReadableFont("Bold", 9), ESlateDrawEffect::None, SecondaryText);
				}
				FSlateDrawElement::MakeText(OutDrawElements, LayerId + 5,
					AllottedGeometry.ToPaintGeometry(FVector2D(Layout.NameWidth + 2.0f, 20.0f),
						FSlateLayoutTransform(LabelPosition + FVector2D(TextLeft, Layout.NameTop))),
					Entry.Label, ReadableFont("Bold", 11), ESlateDrawEffect::None, bSelected ? PickedColour : White);
				if (!Designation.IsEmpty())
				{
					FSlateDrawElement::MakeText(OutDrawElements, LayerId + 5,
						AllottedGeometry.ToPaintGeometry(FVector2D(LabelSize.X, 20.0f),
							FSlateLayoutTransform(LabelPosition + FVector2D(TextLeft + Layout.NameWidth + LabelDesignationGap,
								Layout.DesignationTop))),
						Designation, Font("Bold", 10), ESlateDrawEffect::None, bSelected ? PickedColour : MarkerColor);
				}
			}
			if (FullLabelCount < Placements.Num())
			{
				FSlateDrawElement::MakeText(OutDrawElements, LayerId + 5,
					AllottedGeometry.ToPaintGeometry(FVector2D(PanelSize.X - 8.0, 16.0),
						FSlateLayoutTransform(FVector2D(4.0, FMath::Max(0.0, PanelSize.Y - 18.0)))),
					FText::Format(LOCTEXT("PreviewDensityHint", "{0} markers / {1} labels — full body list on the right"),
						APSUINumber::Number(Placements.Num()), APSUINumber::Number(FullLabelCount)),
					ReadableFont("Regular", 8), ESlateDrawEffect::None, SecondaryText);
			}

			return LayerId + 5;
		}

	private:
		TWeakObjectPtr<UWorldGenerationViewModel> ViewModel;
		mutable TArray<FAPSPreviewBodyEntry> CachedEntries;
		mutable TArray<FText> CachedDesignations;
		mutable TArray<FVector2D> CachedLabelSizes;

		/** Rio 02.10 ("even paddings for all tags"): one visual margin above the capitals of the first line and below
		 * the baseline of the last; the type over the name; the designation after the name, on its baseline. */
		static constexpr float LabelBar = 3.0f;
		static constexpr float LabelPadX = 8.0f;
		static constexpr float LabelPadY = 6.0f;
		static constexpr float LabelLineGap = 5.0f;
		static constexpr float LabelDesignationGap = 6.0f;
		struct FLabelLayout
		{
			FVector2D Size{160.0, 36.0};
			float TypeTop{0.0f};
			float NameTop{0.0f};
			float NameWidth{0.0f};
			float DesignationTop{0.0f};
		};
		static FLabelLayout LayoutLabel(const FText& Type, const FText& Name, const FString& Designation)
		{
			const TSharedRef<FSlateFontMeasure> Measure = FSlateApplication::Get().GetRenderer()->GetFontMeasureService();
			const FSlateFontInfo TypeFont = ReadableFont("Bold", 9);
			const FSlateFontInfo NameFont = ReadableFont("Bold", 11);
			const FSlateFontInfo DesignationFont = Font("Bold", 10);
			// The labels are capitals: what reads as the text's height is the cap height (about 0.71 em), not the line box.
			const auto CapHeight = [](const FSlateFontInfo& Info) { return Info.Size * (96.0f / 72.0f) * 0.71f; };
			const auto TopToBaseline = [&Measure](const FSlateFontInfo& Info)
			{
				return static_cast<float>(Measure->GetMaxCharacterHeight(Info) + Measure->GetBaseline(Info));
			};
			FLabelLayout Layout;
			Layout.NameWidth = Measure->Measure(Name, NameFont).X;
			const float DesignationWidth = Designation.IsEmpty() ? 0.0f
				: LabelDesignationGap + Measure->Measure(Designation, DesignationFont).X;
			const float TypeWidth = Type.IsEmpty() ? 0.0f : Measure->Measure(Type, TypeFont).X;
			float Baseline = LabelPadY;
			if (!Type.IsEmpty())
			{
				Baseline += CapHeight(TypeFont);
				Layout.TypeTop = Baseline - TopToBaseline(TypeFont);
				Baseline += LabelLineGap;
			}
			Baseline += CapHeight(NameFont);
			Layout.NameTop = Baseline - TopToBaseline(NameFont);
			Layout.DesignationTop = Baseline - TopToBaseline(DesignationFont);
			Layout.Size = FVector2D(LabelBar + LabelPadX + FMath::Max(TypeWidth, Layout.NameWidth + DesignationWidth) + LabelPadX,
				Baseline + LabelPadY);
			return Layout;
		}
		mutable TArray<FLabelLayout> CachedLabelLayouts;
		mutable TWeakObjectPtr<AActor> CachedSelectedBody;
		mutable TWeakObjectPtr<const AStarSystem> CachedHomeSystem;
		mutable int32 CachedPreviewRevision{MIN_int32};
		mutable EAstroPreviewFocus CachedPreviewFocus{EAstroPreviewFocus::Overview};
	};
}

void SWorldGenerationPanel::Construct(const FArguments& InArgs)
{
	ViewModel = InArgs._ViewModel;
	OnBack = InArgs._OnBack;
	OnContinue = InArgs._OnContinue;
	APSGenerationUI::ApplyTheme();

	using namespace APSGenerationUI;
	const TWeakObjectPtr<UWorldGenerationViewModel> VM = ViewModel;
	SAssignNew(BodyHierarchyBox, SVerticalBox);

	const auto BoolRow = [VM](const FText& Label, TFunction<bool(const UGeneratedWorld*)> Getter,
		TFunction<void(UGeneratedWorld*, bool)> Setter)
	{
		return SNew(SBorder).BorderImage(&ControlBrush).Padding(FMargin(10.0f, 4.0f))
		[
		SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center)
			[SNew(STextBlock).Text(Label).Font(ReadableFont("Bold", 10)).ColorAndOpacity(SecondaryText)]
			+ SHorizontalBox::Slot().AutoWidth()
			[
				SNew(SButton).ButtonStyle(&SecondaryButton).ContentPadding(FMargin(14.0f, 4.0f))
				.HAlign(HAlign_Center).VAlign(VAlign_Center)
				.ButtonColorAndOpacity_Lambda([VM, Getter]()
				{
					return VM.IsValid() && VM->GeneratedWorld && Getter(VM->GeneratedWorld)
						? APSUITheme::RetintHighlight(FLinearColor(0.0f, 0.52f, 0.68f, 1.0f)) : APSUITheme::Retint(FLinearColor(0.18f, 0.23f, 0.27f, 1.0f));
				})
				.OnClicked_Lambda([VM, Getter, Setter]()
				{
					if (UWorldGenerationViewModel* MutableVM = VM.Get(); MutableVM && MutableVM->GeneratedWorld)
					{
						Setter(MutableVM->GeneratedWorld, !Getter(MutableVM->GeneratedWorld));
						MutableVM->RequestPreview();
					}
					return FReply::Handled();
				})
				[
					SNew(STextBlock)
					.Text_Lambda([VM, Getter](){ return FText::FromString(VM.IsValid() && VM->GeneratedWorld && Getter(VM->GeneratedWorld) ? TEXT("ON") : TEXT("OFF")); })
					.Justification(ETextJustify::Center).Font(Font("Bold", 10)).ColorAndOpacity(Cyan).RenderTransform(CapsShift("Bold", 10))
				]
			]
		];
	};

	// Rio 05.10 (real scale experiment): BoolRow's look, routed through the view model (the camera refits every scope,
	// whose size changes by orders of magnitude). It needs FULL-SCALE WORLD; the game starts from it too (Rio 05.10).
	const auto RealScaleRow = [VM]()
	{
		const auto IsOn = [VM]() { return VM.IsValid() && VM->GeneratedWorld && VM->GeneratedWorld->bRealScale; };
		return SNew(SVerticalBox)
			.IsEnabled_Lambda([VM]() { return VM.IsValid() && VM->GeneratedWorld && VM->GeneratedWorld->bGenerateFullScaledWorld; })
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(SBorder).BorderImage(&ControlBrush).Padding(FMargin(10.0f, 4.0f))
				[
				SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center)
					[SNew(STextBlock).Text(LOCTEXT("RealScale", "REAL DISTANCES")).Font(ReadableFont("Bold", 10)).ColorAndOpacity(SecondaryText)]
					+ SHorizontalBox::Slot().AutoWidth()
					[
						SNew(SButton).ButtonStyle(&SecondaryButton).ContentPadding(FMargin(14.0f, 4.0f))
						.HAlign(HAlign_Center).VAlign(VAlign_Center)
						.ButtonColorAndOpacity_Lambda([IsOn]()
						{
							return IsOn() ? APSUITheme::RetintHighlight(FLinearColor(0.0f, 0.52f, 0.68f, 1.0f)) : APSUITheme::Retint(FLinearColor(0.18f, 0.23f, 0.27f, 1.0f));
						})
						.OnClicked_Lambda([VM, IsOn]()
						{
							if (UWorldGenerationViewModel* MutableVM = VM.Get()) MutableVM->SetRealScale(!IsOn());
							return FReply::Handled();
						})
						[
							SNew(STextBlock)
							.Text_Lambda([IsOn](){ return FText::FromString(IsOn() ? TEXT("ON") : TEXT("OFF")); })
							.Justification(ETextJustify::Center).Font(Font("Bold", 10)).ColorAndOpacity(Cyan).RenderTransform(CapsShift("Bold", 10))
						]
					]
				]
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 4.0f, 0.0f, 0.0f)
			[
				SNew(STextBlock).Visibility_Lambda([IsOn]() { return IsOn() ? EVisibility::Visible : EVisibility::Collapsed; })
				.Text(LOCTEXT("RealScaleHint", "THE SAME WORLD AT REAL DISTANCES: NEIGHBOUR STARS SOME LIGHT YEARS APART, ORBITS IN REAL AU. THE GAME STARTS AT REAL DISTANCES TOO."))
				.AutoWrapText(true).Font(ReadableFont("Regular", 9)).ColorAndOpacity(SecondaryText)
			];
	};

	const TSharedRef<SWidget> OverviewControls = SNew(SScrollBox) + SScrollBox::Slot()
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 12.0f)[SectionTitle(LOCTEXT("Astro", "ASTRO GENERATION"))]
		+ SVerticalBox::Slot().AutoHeight()[EnumRow<EAstroGenerationLevel>(LOCTEXT("Level", "GENERATION LEVEL"), VM, [](const UGeneratedWorld* W){ return W->AstroGenerationLevel; })]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 5.0f)[BoolRow(LOCTEXT("FullScale", "FULL-SCALE WORLD"), [](const UGeneratedWorld* W){return W->bGenerateFullScaledWorld;}, [](UGeneratedWorld* W, bool V){W->bGenerateFullScaledWorld=V;})]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 5.0f)[RealScaleRow()]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 5.0f)[BoolRow(LOCTEXT("HomeSystemEnabled", "GENERATE HOME SYSTEM"), [](const UGeneratedWorld* W){return W->bGenerateHomeSystem;}, [](UGeneratedWorld* W, bool V){W->bGenerateHomeSystem=V;})]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 5.0f)[BoolRow(LOCTEXT("StartWithHomePlanet", "START WITH HOME PLANET"), [](const UGeneratedWorld* W){return W->bStartWithHomePlanet;}, [](UGeneratedWorld* W, bool V){W->bStartWithHomePlanet=V;})]
	];

	const TSharedRef<SWidget> GalaxyControls = SNew(SScrollBox) + SScrollBox::Slot()
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 12.0f)[SectionTitle(LOCTEXT("Galaxy", "GALAXY"))]
		+ SVerticalBox::Slot().AutoHeight()[EnumRow<EGalaxyType>(LOCTEXT("GalaxyType", "TYPE / PRESET"), VM, [](const UGeneratedWorld* W){ return W->GalaxyType; })]
		// Rio 03.10: CLASS offers only the subclasses of the current TYPE (a type change coerces the class).
		+ SVerticalBox::Slot().AutoHeight()[EnumRow<EGalaxyClass>(LOCTEXT("GalaxyClass", "CLASS"), VM, [](const UGeneratedWorld* W){ return W->GalaxyClass; },
			[](const UGeneratedWorld* W)
			{
				TArray<int64> Values;
				for (const EGalaxyClass Class : APSGalaxyMorphology::GetSubclasses(W->GalaxyType)) Values.Add(static_cast<int64>(Class));
				return Values;
			})]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 8.0f)
		[SNew(STextBlock).Text_Lambda([VM]()
		{
			return VM.IsValid() && VM->GeneratedWorld ? FText::FromString(FString(
				APSGalaxyMorphology::GetSubclassSummary(VM->GeneratedWorld->GalaxyClass)).ToUpper()) : FText::GetEmpty();
		}).AutoWrapText(true).Font(ReadableFont("Regular", 9)).ColorAndOpacity(SecondaryText)]
		+ SVerticalBox::Slot().AutoHeight()[NumberRow<int32>(LOCTEXT("GalaxySize", "SIZE"), APSGalaxyMorphology::MinGalaxySize, 100000, 1, VM, [](const UGeneratedWorld* W){ return W->GalaxySize; }, [](UWorldGenerationViewModel* V, int32 X){ V->SetGalaxySize(X); })]
		// Rio 03.10: STARS (placed, log slider) replaces MODELED STAR COUNT; the catalogue stays hidden. The ceiling is the
		// current renderer's (APSGalaxyMorphology::MaxPlacedStars) until the GPU star layer lands.
		+ SVerticalBox::Slot().AutoHeight()[LogNumberRow(LOCTEXT("PlacedStars", "STARS"), APSGalaxyMorphology::PreviewReferenceBudget, APSGalaxyMorphology::MaxPlacedStars, VM, [](const UGeneratedWorld* W){ return W->GalaxyPlacedStarCount > 0 ? W->GalaxyPlacedStarCount : APSGalaxyMorphology::PreviewReferenceBudget; }, [](UWorldGenerationViewModel* V, int32 X){ V->SetGalaxyPlacedStarCount(X); })]
		+ SVerticalBox::Slot().AutoHeight()[NumberRow<double>(LOCTEXT("Density", "STAR DENSITY"), 0.01, 1000.0, 0.1, VM, [](const UGeneratedWorld* W){ return W->GalaxyStarDensity; }, [](UWorldGenerationViewModel* V, double X){ V->SetGalaxyStarDensity(X); })]
		// Rio 03.10: the galaxy's star sizes and spectral classes, the same presets as the cluster's rows.
		+ SVerticalBox::Slot().AutoHeight()[EnumRow<EStarClusterPopulation>(LOCTEXT("GalaxyPopulation", "POPULATION"), VM, [](const UGeneratedWorld* W){ return W->GalaxyStarPopulation; }, {}, [](UWorldGenerationViewModel* V, int32 X){ V->SetGalaxyStarPopulation(X); })]
		+ SVerticalBox::Slot().AutoHeight()[EnumRow<EStarClusterComposition>(LOCTEXT("GalaxyComposition", "COMPOSITION"), VM, [](const UGeneratedWorld* W){ return W->GalaxyStarComposition; }, {}, [](UWorldGenerationViewModel* V, int32 X){ V->SetGalaxyStarComposition(X); })]
	];

	const TSharedRef<SWidget> ClusterControls = SNew(SScrollBox) + SScrollBox::Slot()
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 12.0f)[SectionTitle(LOCTEXT("Cluster", "STAR CLUSTER"))]
		+ SVerticalBox::Slot().AutoHeight()[EnumRow<EStarClusterSize>(LOCTEXT("ClusterSize", "SIZE"), VM, [](const UGeneratedWorld* W){ return W->StarClusterSize; })]
		+ SVerticalBox::Slot().AutoHeight()[EnumRow<EStarClusterType>(LOCTEXT("ClusterType", "FORMATION / PRESET"), VM, [](const UGeneratedWorld* W){ return W->StarClusterType; })]
		+ SVerticalBox::Slot().AutoHeight()[EnumRow<EStarClusterPopulation>(LOCTEXT("Population", "POPULATION"), VM, [](const UGeneratedWorld* W){ return W->StarClusterPopulation; })]
		+ SVerticalBox::Slot().AutoHeight()[EnumRow<EStarClusterComposition>(LOCTEXT("Composition", "COMPOSITION"), VM, [](const UGeneratedWorld* W){ return W->StarClusterComposition; })]
	];

	const TSharedRef<SWidget> SystemControls = SNew(SScrollBox) + SScrollBox::Slot()
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 12.0f)[SectionTitle(LOCTEXT("SelectedSystem", "SELECTED SYSTEM"))]
		+ SVerticalBox::Slot().AutoHeight()
		[
			SNew(SVerticalBox).IsEnabled_Lambda([VM](){ return VM.IsValid() && VM->CanEditSelectedSystem(); })
			+ SVerticalBox::Slot().AutoHeight()[EnumRow<EStarType>(LOCTEXT("StarType", "STAR SYSTEM TYPE"), VM, [VM](const UGeneratedWorld* W){ return VM.IsValid() ? VM->GetSelectedSystemStarType() : W->StarType; })]
			+ SVerticalBox::Slot().AutoHeight()[EnumRow<EPlanetarySystemType>(LOCTEXT("SelectedFamilyType", "PLANET FAMILY TYPE"), VM, [VM](const UGeneratedWorld* W){ return VM.IsValid() ? VM->GetSelectedSystemPlanetaryType() : W->PlanetarySystemType; })]
			+ SVerticalBox::Slot().AutoHeight()[EnumRow<EOrbitDistributionType>(LOCTEXT("Distribution", "ORBIT DISTRIBUTION"), VM, [VM](const UGeneratedWorld* W){ return VM.IsValid() ? VM->GetSelectedSystemOrbitDistribution() : W->OrbitDistributionType; })]
			+ SVerticalBox::Slot().AutoHeight()[NumberRow<double>(LOCTEXT("OrbitInclination", "MAX ORBIT INCLINATION / DEG"), 0.0, 90.0, 1.0, VM, [VM](const UGeneratedWorld*){ return VM.IsValid() ? VM->GetSelectedSystemMaxOrbitInclination() : 8.0; }, [](UWorldGenerationViewModel* V, double X){ V->SetSelectedSystemMaxOrbitInclination(X); })]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 2.0f, 0.0f, 8.0f)
			[SNew(STextBlock).Text_Lambda([VM]()
			{
				if (!VM.IsValid()) return FText::GetEmpty();
				switch (VM->GetSelectedSystemOrbitDistribution())
				{
				case EOrbitDistributionType::Uniform: return LOCTEXT("OrbitsUniformHint", "UNIFORM: EVEN RADIAL SPACING.");
				case EOrbitDistributionType::Gaussian: return LOCTEXT("OrbitsGaussianHint", "GAUSSIAN: DENSE MIDDLE, SPARSE INNER AND OUTER ORBITS.");
				case EOrbitDistributionType::Chaotic: return LOCTEXT("OrbitsChaoticHint", "CHAOTIC: IRREGULAR GAPS WITHIN THE SAME SYSTEM SIZE.");
				case EOrbitDistributionType::InnerOuter: return LOCTEXT("OrbitsBandsHint", "INNER / OUTER: TWO GROUPS WITH A CLEAR GAP.");
				default: return LOCTEXT("OrbitsDenseHint", "DENSE: COMPACT INNER GROUP.");
				}
			}).AutoWrapText(true).Font(ReadableFont("Regular", 9)).ColorAndOpacity(SecondaryText)]
			+ SVerticalBox::Slot().AutoHeight()
			[SNew(STextBlock).Text(LOCTEXT("InclinationHint", "0 DEG = FLAT ECLIPTIC. HIGHER VALUES ALLOW TILTED PLANES; DISTANCES STAY UNCHANGED."))
			.AutoWrapText(true).Font(ReadableFont("Regular", 9)).ColorAndOpacity(SecondaryText)]
			+ SVerticalBox::Slot().AutoHeight()[NumberRow<int32>(LOCTEXT("TotalPlanets", "TOTAL PLANETS IN SYSTEM"), 0, 20, 1, VM, [VM](const UGeneratedWorld* W){ return VM.IsValid() ? VM->GetSelectedSystemPlanetCount() : W->PlanetsAmount; }, [](UWorldGenerationViewModel* V, int32 X){ V->SetSelectedSystemPlanetCount(X); })]
		]
		+ SVerticalBox::Slot().AutoHeight()
		[
			SNew(STextBlock).Text(LOCTEXT("SelectedSystemHint", "EDITS APPLY TO THIS SYSTEM ONLY. THE TOTAL IS SHARED BETWEEN ITS STARS; CHANGING STAR COUNT KEEPS THAT TOTAL."))
			.AutoWrapText(true).Font(ReadableFont("Regular", 10)).ColorAndOpacity(SecondaryText)
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 16.0f, 0.0f, 10.0f)[SectionTitle(LOCTEXT("HomeStartDefaults", "HOME START DEFAULTS"))]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 5.0f)[BoolRow(LOCTEXT("RandomHomeSystem", "RANDOM HOME SYSTEM"), [](const UGeneratedWorld* W){return W->bRandomHomeSystem;}, [](UGeneratedWorld* W, bool V){W->bRandomHomeSystem=V;})]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 5.0f)[BoolRow(LOCTEXT("RandomSystemType", "RANDOM SYSTEM TYPE"), [](const UGeneratedWorld* W){return W->bRandomHomeSystemType;}, [](UGeneratedWorld* W, bool V){W->bRandomHomeSystemType=V;})]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 5.0f)[BoolRow(LOCTEXT("RandomHomeStar", "RANDOM HOME STAR"), [](const UGeneratedWorld* W){return W->bRandomHomeStar;}, [](UGeneratedWorld* W, bool V){W->bRandomHomeStar=V;})]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 5.0f)[BoolRow(LOCTEXT("RandomStartPlanet", "RANDOM START PLANET"), [](const UGeneratedWorld* W){return W->bRandomStartPlanetNumber;}, [](UGeneratedWorld* W, bool V){W->bRandomStartPlanetNumber=V;})]
		+ SVerticalBox::Slot().AutoHeight()
		[
			SNew(SBox).IsEnabled_Lambda([VM](){ return VM.IsValid() && VM->GetHomeStartPlanetCount() > 0; })
			[NumberRow<int32>(LOCTEXT("HomeStartIndex", "HOME START PLANET INDEX"), 1, 120, 1, VM, [VM](const UGeneratedWorld* W){ return FMath::Clamp(W->StartPlanetIndex, 1, FMath::Max(1, VM.IsValid() ? VM->GetHomeStartPlanetCount() : W->PlanetsAmount)); }, [](UWorldGenerationViewModel* V, int32 X){ V->SetStartPlanetIndex(X); })]
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 8.0f, 0.0f, 0.0f)
		[
			SNew(STextBlock).Text(LOCTEXT("HomeDefaultsHint", "THESE DEFAULTS BELONG TO THE HOME SYSTEM. EXPLICIT OBJECT EDITS ARE RETAINED UNTIL REGENERATE. EDIT MOONS ON THEIR PLANET PAGE."))
			.AutoWrapText(true).Font(ReadableFont("Regular", 10)).ColorAndOpacity(SecondaryText)
		]
	];

	const TSharedRef<SWidget> StarControls = SNew(SScrollBox) + SScrollBox::Slot()
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 12.0f)[SectionTitle(LOCTEXT("SelectedStar", "SELECTED STAR"))]
		+ SVerticalBox::Slot().AutoHeight()[EnumRow<EStellarType>(LOCTEXT("StellarType", "STELLAR TYPE"), VM, [VM](const UGeneratedWorld* W){ return VM.IsValid() ? VM->GetSelectedStellarType() : W->StellarType; })]
		+ SVerticalBox::Slot().AutoHeight()
		[
			// A remnant's class follows its type: the row shows it and stays locked.
			SNew(SBox).IsEnabled_Lambda([VM]() { return !VM.IsValid() || !VM->IsSelectedStarRemnant(); })
			[EnumRow<ESpectralClass>(LOCTEXT("Spectral", "SPECTRAL CLASS"), VM, [VM](const UGeneratedWorld* W){ return VM.IsValid() ? VM->GetSelectedSpectralClass() : W->SpectralClass; })]
		]
		+ SVerticalBox::Slot().AutoHeight()[NumberRow<double>(LOCTEXT("SelectedStarRadius", "SIZE / SOLAR RADII (0 = AUTO)"), 0.0, 1000.0, 0.1, VM, [VM](const UGeneratedWorld* W){ return VM.IsValid() ? VM->GetSelectedStarRadiusOverrideSolar() : W->HomeStarRadiusOverrideSolar; }, [](UWorldGenerationViewModel* V, double X){ V->SetSelectedStarRadiusOverrideSolar(X); })]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 14.0f, 0.0f, 0.0f)
		[
			SNew(STextBlock)
			.Text(LOCTEXT("SelectedStarScopeHint", "EDITS APPLY TO THIS STAR ONLY AND SURVIVE NAVIGATION. SAFE ORBITS USE ITS NEW PHYSICAL SIZE. SET 0 TO REMOVE THE SIZE OVERRIDE."))
			.AutoWrapText(true).Font(ReadableFont("Regular", 10)).ColorAndOpacity(SecondaryText)
		]
	];

	const TSharedRef<SWidget> PlanetControls = SNew(SScrollBox) + SScrollBox::Slot()
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 12.0f)
		[
			SNew(STextBlock).Font(Font("Bold", 13)).ColorAndOpacity(Cyan)
			.Text_Lambda([VM]() { return VM.IsValid() && VM->IsSelectedPreviewBodyMoon()
				? LOCTEXT("SelectedMoon", "SELECTED MOON") : LOCTEXT("Planet", "SELECTED PLANET"); })
		]
		+ SVerticalBox::Slot().AutoHeight()
		[
			ChoiceRow(LOCTEXT("SurfaceFamily", "SURFACE FAMILY"), VM,
				[](const UGeneratedWorld* W)
				{
					return GetSurfaceFamilyDisplayName(W->PlanetType);
				},
				[](UWorldGenerationViewModel* V, int32 Direction)
				{
					if (!V || !V->GeneratedWorld) return;
					// Rio 02.10: a moon cannot be a gas giant, so a moon's families stop before the giants.
					const int32 FamilyCount = V->IsSelectedPreviewBodyMoon()
						? GasGiantSurfaceFamilyIndex : GasGiantSurfaceFamilyIndex + 1;
					const int32 Current = GetSurfaceFamilyIndex(V->GeneratedWorld->PlanetType);
					const int32 Next = Current >= FamilyCount
						? (Direction < 0 ? FamilyCount - 1 : 0)
						: (Current + (Direction < 0 ? -1 : 1) + FamilyCount) % FamilyCount;
					const TArray<EPlanetType> Presets = GetSurfacePresetsForFamily(Next);
					if (!Presets.IsEmpty())
					{
						V->SetEnumValue(StaticEnum<EPlanetType>(), static_cast<int32>(Presets[0]));
					}
				})
		]
		+ SVerticalBox::Slot().AutoHeight()
		[
			ChoiceRow(LOCTEXT("SurfacePreset", "SURFACE PRESET / SUBTYPE"), VM,
				[](const UGeneratedWorld* W)
				{
					return GetGenerationEnumDisplayName(
						StaticEnum<EPlanetType>(), static_cast<int64>(W->PlanetType));
				},
				[](UWorldGenerationViewModel* V, int32 Direction)
				{
					if (!V || !V->GeneratedWorld) return;
					int32 Family = GetSurfaceFamilyIndex(V->GeneratedWorld->PlanetType);
					if (Family == GasGiantSurfaceFamilyIndex && V->IsSelectedPreviewBodyMoon())
					{
						// A moon left on a giant preset (older saves) steps into the rocky family.
						Family = static_cast<int32>(EAPSPlanetSurfaceArchetype::Rocky);
					}
					const TArray<EPlanetType> Presets = GetSurfacePresetsForFamily(Family);
					if (Presets.IsEmpty()) return;
					const int32 Current = Presets.IndexOfByKey(V->GeneratedWorld->PlanetType);
					// Loaded legacy Exoplanet stays untouched until the user steps the preset.
					// Its first forward step selects the first current family entry.
					const int32 Base = Current == INDEX_NONE ? (Direction < 0 ? 0 : -1) : Current;
					const int32 Next = (Base + (Direction < 0 ? -1 : 1) + Presets.Num()) % Presets.Num();
					V->SetEnumValue(StaticEnum<EPlanetType>(), static_cast<int32>(Presets[Next]));
				})
		]
		+ SVerticalBox::Slot().AutoHeight()
		[
			EnumRow<EPlanetHabitability>(LOCTEXT("Habitability", "HABITABILITY"), VM,
				[](const UGeneratedWorld* W){ return W->PlanetHabitability; })
		]
		+ SVerticalBox::Slot().AutoHeight()[NumberRow<double>(LOCTEXT("Radius", "PLANET RADIUS / KM"), 1.0, 200000.0, 100.0, VM, [](const UGeneratedWorld* W){ return W->PlanetRadius; }, [](UWorldGenerationViewModel* V, double X){ V->SetPlanetRadius(X); })]
		+ SVerticalBox::Slot().AutoHeight()
		[
			SNew(SVerticalBox)
			.Visibility_Lambda([VM](){ return VM.IsValid() && VM->CanEditSelectedPlanetOrbit() ? EVisibility::Visible : EVisibility::Collapsed; })
			+ SVerticalBox::Slot().AutoHeight()[NumberRow<double>(LOCTEXT("PlanetOrbitAU", "ORBIT DISTANCE / AU"), 0.001, 50.0, 0.05, VM,
				[VM](const UGeneratedWorld*){ return VM.IsValid() ? VM->GetSelectedPlanetOrbitDistanceAu() : 1.0; },
				[](UWorldGenerationViewModel* V, double X){ V->SetSelectedPlanetOrbitDistanceAu(X); })]
			+ SVerticalBox::Slot().AutoHeight()[NumberRow<double>(LOCTEXT("PlanetOrbitTilt", "ORBIT INCLINATION / DEG"), 0.0, 90.0, 1.0, VM,
				[VM](const UGeneratedWorld*){ return VM.IsValid() ? VM->GetSelectedPlanetOrbitInclination() : 0.0; },
				[](UWorldGenerationViewModel* V, double X){ V->SetSelectedPlanetOrbitInclination(X); })]
			+ SVerticalBox::Slot().AutoHeight()
			[SNew(SButton).ButtonStyle(&SecondaryButton).Text(LOCTEXT("PlanetOrbitAuto", "RESTORE AUTO ORBIT"))
				.HAlign(HAlign_Center).VAlign(VAlign_Center).ContentPadding(FMargin(14.0f, 6.0f))
				.IsEnabled_Lambda([VM](){ return VM.IsValid() && VM->HasSelectedPlanetOrbitEdit(); })
				.OnClicked_Lambda([VM](){ if (VM.IsValid()) VM->ResetSelectedPlanetOrbit(); return FReply::Handled(); })]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 4.0f, 0.0f, 8.0f)
			[SNew(STextBlock).AutoWrapText(true).Font(ReadableFont("Regular", 9)).ColorAndOpacity(SecondaryText)
				.Text(LOCTEXT("PlanetOrbitManualHint", "MANUAL VALUES SURVIVE DISTRIBUTION CHANGES. 0 DEG = FLAT. DISTANCE IS FROM THE STAR CENTRE; SURFACE CLEARANCE IS ALWAYS KEPT."))]
		]
		+ SVerticalBox::Slot().AutoHeight()
		[
			SNew(SBox)
			.Visibility_Lambda([VM]()
			{
				return VM.IsValid() && VM->IsSelectedPreviewBodyMoon()
					? EVisibility::Visible : EVisibility::Collapsed;
			})
			[
				NumberRow<double>(LOCTEXT("MoonOrbitRadius", "MOON ORBIT RADIUS / KM"),
					100.0, 100000000.0, 1000.0, VM,
					[](const UGeneratedWorld* W){ return FMath::Max(100.0, W->MoonOrbitRadiusKm); },
					[](UWorldGenerationViewModel* V, double X){ V->SetSelectedMoonOrbitRadiusKm(X); })
			]
		]
		+ SVerticalBox::Slot().AutoHeight()
		[
			SNew(SBox)
			.Visibility_Lambda([VM]()
			{
				return VM.IsValid() && !VM->IsSelectedPreviewBodyMoon()
					? EVisibility::Visible : EVisibility::Collapsed;
			})
			[NumberRow<int32>(LOCTEXT("PlanetMoons", "MOONS AMOUNT"), 0, 10, 1, VM,
				[](const UGeneratedWorld* W){ return FMath::Clamp(W->MoonsAmount, 0, 10); },
				[](UWorldGenerationViewModel* V, int32 X){ V->SetMoonsAmount(X); })]
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 12.0f, 0.0f, 10.0f)[SectionTitle(LOCTEXT("PlanetSurface", "PLANET SURFACE"))]
		// Gas giants use the same saved seed for their material, without WorldScape.
		+ SVerticalBox::Slot().AutoHeight()[NumberRow<int32>(LOCTEXT("SurfaceMaterialSeed", "SURFACE / MATERIAL SEED"), 0, 999983, 1, VM, [](const UGeneratedWorld* W){ return FMath::Clamp(W->PlanetSurfaceSeed, 0, 999983); }, [](UWorldGenerationViewModel* V, int32 X){ V->SetPlanetSurfaceSeed(X); }, &SurfaceControlSliders.FindOrAdd(EAPSGenerationSurfaceControl::Seed))]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 8.0f)
		[SNew(STextBlock).AutoWrapText(true).Font(ReadableFont("Regular", 9)).ColorAndOpacity(SecondaryText)
			.Text(LOCTEXT("SurfaceMaterialSeedHint", "0 = AUTO. GAS GIANTS: CHANGES BELTS AND VORTICES; KEEPS THE PLANET TYPE AND PALETTE."))]
		+ SVerticalBox::Slot().AutoHeight()
		[
			SNew(SVerticalBox)
			.IsEnabled_Lambda([VM]()
			{
				return VM.IsValid() && VM->GeneratedWorld
					&& UAPSPlanetSurfaceProfileResolver::SupportsWorldScape(VM->GeneratedWorld->PlanetType);
			})
		+ SVerticalBox::Slot().AutoHeight()[NumberRow<double>(LOCTEXT("SurfaceFeatureScale", "FEATURE SCALE"), 0.25, 4.0, 0.05, VM, [](const UGeneratedWorld* W){ return W->SurfaceFeatureScale; }, [](UWorldGenerationViewModel* V, double X){ V->SetSurfaceFeatureScale(X); }, &SurfaceControlSliders.FindOrAdd(EAPSGenerationSurfaceControl::FeatureScale))]
		+ SVerticalBox::Slot().AutoHeight()[NumberRow<double>(LOCTEXT("SurfaceReliefScale", "RELIEF"), 0.25, 2.5, 0.05, VM, [](const UGeneratedWorld* W){ return W->SurfaceReliefScale; }, [](UWorldGenerationViewModel* V, double X){ V->SetSurfaceReliefScale(X); }, &SurfaceControlSliders.FindOrAdd(EAPSGenerationSurfaceControl::ReliefScale))]
		+ SVerticalBox::Slot().AutoHeight()[NumberRow<double>(LOCTEXT("SurfaceLandScale", "LAND COVERAGE"), 0.25, 2.0, 0.05, VM, [](const UGeneratedWorld* W){ return W->SurfaceLandCoverageScale; }, [](UWorldGenerationViewModel* V, double X){ V->SetSurfaceLandCoverageScale(X); }, &SurfaceControlSliders.FindOrAdd(EAPSGenerationSurfaceControl::LandCoverage))]
		+ SVerticalBox::Slot().AutoHeight()[NumberRow<double>(LOCTEXT("SurfaceMountainScale", "MOUNTAINS"), 0.0, 2.0, 0.05, VM, [](const UGeneratedWorld* W){ return W->SurfaceMountainScale; }, [](UWorldGenerationViewModel* V, double X){ V->SetSurfaceMountainScale(X); }, &SurfaceControlSliders.FindOrAdd(EAPSGenerationSurfaceControl::Mountains))]
		+ SVerticalBox::Slot().AutoHeight()[NumberRow<double>(LOCTEXT("SurfaceCraterScale", "CRATERS"), 0.0, 2.0, 0.05, VM, [](const UGeneratedWorld* W){ return W->SurfaceCraterScale; }, [](UWorldGenerationViewModel* V, double X){ V->SetSurfaceCraterScale(X); }, &SurfaceControlSliders.FindOrAdd(EAPSGenerationSurfaceControl::Craters))]
		+ SVerticalBox::Slot().AutoHeight()[NumberRow<double>(LOCTEXT("SurfaceRoughnessScale", "ROUGHNESS"), 0.25, 2.0, 0.05, VM, [](const UGeneratedWorld* W){ return W->SurfaceRoughnessScale; }, [](UWorldGenerationViewModel* V, double X){ V->SetSurfaceRoughnessScale(X); }, &SurfaceControlSliders.FindOrAdd(EAPSGenerationSurfaceControl::Roughness))]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 5.0f, 0.0f, 0.0f)
		[
			SNew(STextBlock)
			.Text(LOCTEXT("SurfaceTerrainOnlyHint", "TERRAIN CONTROLS APPLY TO SOLID PLANETS ONLY."))
			.AutoWrapText(true).Font(ReadableFont("Regular", 9)).ColorAndOpacity(SecondaryText)
		]
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 12.0f, 0.0f, 10.0f)[SectionTitle(LOCTEXT("Atmosphere", "PLANET ATMOSPHERE"))]
		+ SVerticalBox::Slot().AutoHeight()[NumberRow<double>(LOCTEXT("AtmosphereHeight", "HEIGHT / KM"), 0.0, 2000.0, 5.0, VM, [](const UGeneratedWorld* W){ return W->AtmosphereHeight; }, [](UWorldGenerationViewModel* V, double X){ if(V->GeneratedWorld){V->GeneratedWorld->AtmosphereHeight=APSAtmosphereControlBounds::Height(X, V->GeneratedWorld->PlanetType); V->RefreshPlanetAppearancePreview(false);} }, nullptr,
			[](const UGeneratedWorld* W){ return APSAtmosphereControlBounds::HeightMaximum(W->PlanetType); })]
		+ SVerticalBox::Slot().AutoHeight()[NumberRow<double>(LOCTEXT("AtmosphereOpacity", "OPACITY"), 0.0, 40.0, 0.25, VM, [](const UGeneratedWorld* W){ return W->AtmosphereOpacity; }, [](UWorldGenerationViewModel* V, double X){ if(V->GeneratedWorld){V->GeneratedWorld->AtmosphereOpacity=X; V->RefreshPlanetAppearancePreview(false);} })]
		+ SVerticalBox::Slot().AutoHeight()[NumberRow<double>(LOCTEXT("AtmosphereMulti", "MULTI SCATTERING"), 0.0, 10.0, 0.05, VM, [](const UGeneratedWorld* W){ return W->AtmosphereMultiScattering; }, [](UWorldGenerationViewModel* V, double X){ if(V->GeneratedWorld){V->GeneratedWorld->AtmosphereMultiScattering=X; V->RefreshPlanetAppearancePreview(false);} })]
		+ SVerticalBox::Slot().AutoHeight()[NumberRow<double>(LOCTEXT("AtmosphereRayleigh", "RAYLEIGH SCALE HEIGHT / KM"), 0.0, 64.0, 0.25, VM, [](const UGeneratedWorld* W){ return W->AtmosphereRayleighScattering; }, [](UWorldGenerationViewModel* V, double X){ if(V->GeneratedWorld){V->GeneratedWorld->AtmosphereRayleighScattering=APSAtmosphereControlBounds::Rayleigh(X, V->GeneratedWorld->PlanetType); V->RefreshPlanetAppearancePreview(false);} }, nullptr,
			[](const UGeneratedWorld* W){ return APSAtmosphereControlBounds::RayleighMaximum(W->PlanetType); })]
		+ SVerticalBox::Slot().AutoHeight()[AtmosphereColorRows(VM)]
		+ SVerticalBox::Slot().AutoHeight().Padding(0,12,0,10)[SectionTitle(LOCTEXT("CloudSection","PLANET CLOUDS"))]
		+ SVerticalBox::Slot().AutoHeight()
		[SNew(STextBlock).AutoWrapText(true).Font(ReadableFont("Regular",9)).ColorAndOpacity(SecondaryText)
			.Text_Lambda([VM](){return VM.IsValid()?VM->GetCloudSummary():FText::GetEmpty();})]
		+ SVerticalBox::Slot().AutoHeight()
		[
			SNew(SVerticalBox).IsEnabled_Lambda([VM](){return VM.IsValid()&&VM->CanEditClouds();})
			+ SVerticalBox::Slot().AutoHeight()[NumberRow<double>(LOCTEXT("CloudCover","COVERAGE / CLIMATE MULTIPLIER"),0.,2.,.05,VM,
				[](const UGeneratedWorld* W){return W->CloudSettings.CoverageScale;},[](UWorldGenerationViewModel* V,double X){V->SetCloudParameter(EAPSCloudControl::Coverage,X);})]
			+ SVerticalBox::Slot().AutoHeight()[NumberRow<double>(LOCTEXT("CloudDensity","OPTICAL DENSITY"),0.,2.,.05,VM,
				[](const UGeneratedWorld* W){return W->CloudSettings.DensityScale;},[](UWorldGenerationViewModel* V,double X){V->SetCloudParameter(EAPSCloudControl::Density,X);})]
			+ SVerticalBox::Slot().AutoHeight()[NumberRow<double>(LOCTEXT("CloudScale","WEATHER FEATURE SIZE"),.25,3.,.05,VM,
				[](const UGeneratedWorld* W){return W->CloudSettings.FeatureScale;},[](UWorldGenerationViewModel* V,double X){V->SetCloudParameter(EAPSCloudControl::Scale,X);})]
			+ SVerticalBox::Slot().AutoHeight()[NumberRow<double>(LOCTEXT("CloudAltitude","LAYER ALTITUDE MULTIPLIER"),.25,2.,.05,VM,
				[](const UGeneratedWorld* W){return W->CloudSettings.AltitudeScale;},[](UWorldGenerationViewModel* V,double X){V->SetCloudParameter(EAPSCloudControl::Altitude,X);})]
			+ SVerticalBox::Slot().AutoHeight()[NumberRow<double>(LOCTEXT("CloudWind","MODEL WIND MULTIPLIER / 0 = STATIC"),0.,3.,.05,VM,
				[](const UGeneratedWorld* W){return W->CloudSettings.WindScale;},[](UWorldGenerationViewModel* V,double X){V->SetCloudParameter(EAPSCloudControl::Wind,X);})]
			+ SVerticalBox::Slot().AutoHeight()[NumberRow<double>(LOCTEXT("CloudStorms","CLIMATE VORTEX STRENGTH"),0.,2.,.05,VM,
				[](const UGeneratedWorld* W){return W->CloudSettings.StormScale;},[](UWorldGenerationViewModel* V,double X){V->SetCloudParameter(EAPSCloudControl::Storms,X);})]
			+ SVerticalBox::Slot().AutoHeight()[NumberRow<int32>(LOCTEXT("CloudSeed","WEATHER SEED OFFSET"),0,999983,1,VM,
				[](const UGeneratedWorld* W){return W->CloudSettings.SeedOffset;},[](UWorldGenerationViewModel* V,int32 X){V->SetCloudParameter(EAPSCloudControl::Seed,X);})]
			+ SVerticalBox::Slot().AutoHeight()
			[SNew(SButton).ButtonStyle(&SecondaryButton).Text(LOCTEXT("CloudAuto","RESTORE CLOUD DEFAULTS"))
				.HAlign(HAlign_Center).VAlign(VAlign_Center).ContentPadding(FMargin(14.0f, 6.0f))
				.OnClicked_Lambda([VM](){if(VM.IsValid())VM->ResetCloudParameters();return FReply::Handled();})]
			+ SVerticalBox::Slot().AutoHeight()
			[SNew(STextBlock).AutoWrapText(true).Font(ReadableFont("Regular",9)).ColorAndOpacity(SecondaryText)
				.Text(LOCTEXT("CloudHint","DEFAULT COVERAGE = 0.5 X CLIMATE; OTHER MULTIPLIERS = 1. 0 COVERAGE = CLEAR. COMPOSITION FOLLOWS PLANET TYPE; VACUUM AND UNSUITABLE CLIMATE STAY CLEAR. NO TERRAIN REBUILD."))]
		]
	];

	const TSharedRef<SWidget> ControlsSwitcher = SNew(SWidgetSwitcher)
		.WidgetIndex_Lambda([VM]()
		{
			if (!VM.IsValid()) return 0;
			switch (VM->GetPreviewFocus())
			{
			case EAstroPreviewFocus::Galaxy: return 1;
			case EAstroPreviewFocus::StarCluster: return 2;
			case EAstroPreviewFocus::HomeSystem: return 3;
			case EAstroPreviewFocus::HomeStar: return 4;
			case EAstroPreviewFocus::HomePlanet: return 5;
			default: return 0;
			}
		})
		+ SWidgetSwitcher::Slot()[OverviewControls]
		+ SWidgetSwitcher::Slot()[GalaxyControls]
		+ SWidgetSwitcher::Slot()[ClusterControls]
		+ SWidgetSwitcher::Slot()[SystemControls]
		+ SWidgetSwitcher::Slot()[StarControls]
		+ SWidgetSwitcher::Slot()[PlanetControls];

	const TSharedRef<SWidget> ContextPanel = SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight()
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center)
			[
				SNew(STextBlock).Font(Font("Bold", 9)).ColorAndOpacity(Cyan)
				.Text_Lambda([VM]()
				{
					if (!VM.IsValid()) return FText::GetEmpty();
					switch (VM->GetPreviewFocus())
					{
					case EAstroPreviewFocus::Galaxy: return LOCTEXT("CrumbGalaxy", "GALAXY");
					case EAstroPreviewFocus::StarCluster: return LOCTEXT("CrumbCluster", "GALAXY  /  CLUSTER");
					case EAstroPreviewFocus::HomeSystem: return LOCTEXT("CrumbSystem", "GALAXY  /  CLUSTER  /  SYSTEM");
					case EAstroPreviewFocus::HomeStar: return LOCTEXT("CrumbStar", "GALAXY  /  CLUSTER  /  SYSTEM  /  STAR");
					case EAstroPreviewFocus::HomePlanet: return LOCTEXT("CrumbPlanet", "GALAXY  /  CLUSTER  /  SYSTEM  /  BODY");
					default: return LOCTEXT("CrumbOverview", "FULL-SCALE OVERVIEW");
					}
				})
			]
			+ SHorizontalBox::Slot().AutoWidth()
			[
				SNew(SButton).ButtonStyle(&SecondaryButton).ContentPadding(FMargin(14.0f, 4.0f))
				.HAlign(HAlign_Center).VAlign(VAlign_Center)
				.OnClicked(this, &SWorldGenerationPanel::FocusPreviewUp)
				.IsEnabled_Lambda([VM]() { return VM.IsValid() && VM->CanFocusPreviewParent(); })
				[SNew(STextBlock).Text(LOCTEXT("Up", "^  UP")).Justification(ETextJustify::Center)
				.Font(Font("Bold", 8)).ColorAndOpacity(White).RenderTransform(CapsShift("Bold", 8))]
			]
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 11.0f, 0.0f, 0.0f)
		[
			SNew(SBox)
			.Visibility_Lambda([VM]()
			{
				return VM.IsValid() && VM->CanRenameCurrentScope()
					? EVisibility::Visible : EVisibility::Collapsed;
			})
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 5.0f)
				[
					// Galaxy and cluster are named here too (Rio 02.10); the title follows the scope.
					SNew(STextBlock).Font(Font("Bold", 10)).ColorAndOpacity(Cyan)
					.Text_Lambda([VM]() { return VM.IsValid() ? VM->GetCurrentScopeNameTitle() : FText::GetEmpty(); })
				]
				+ SVerticalBox::Slot().AutoHeight()
				[
					// Text comes from RefreshNameBox while unfocused, so typing in capitals never drops a binding.
					SAssignNew(BodyNameTextBox, SEditableTextBox)
					.Font(ReadableFont("Bold", 12))
					.HintText(LOCTEXT("BodyDisplayNameHint", "ENTER A NAME"))
					.ToolTipText(LOCTEXT("BodyDisplayNameHelp", "1–128 characters. Enter or leave the field to save. Renaming does not regenerate the body."))
					.OnVerifyTextChanged_Lambda([](const FText& Text, FText& Error)
					{
						const FString Name = Text.ToString().TrimStartAndEnd();
						if (Name.Equals(TEXT("None"), ESearchCase::IgnoreCase))
						{
							Error = LOCTEXT("BodyDisplayNameReserved", "None is reserved for an unnamed body. Choose another name.");
							return false;
						}
						bool bValid = !Name.IsEmpty() && Name.Len() <= 128;
						for (const TCHAR Character : Name)
							bValid = bValid && !FChar::IsControl(Character);
						Error = bValid ? FText::GetEmpty()
							: LOCTEXT("BodyDisplayNameInvalid", "Use 1–128 characters without line breaks or control characters.");
						return bValid;
					})
					.OnTextChanged_Lambda([this, VM](const FText& Text)
					{
						if (!VM.IsValid() || !BodyNameTextBox.IsValid() || !BodyNameTextBox->HasKeyboardFocus()) return;
						if (PendingRenameBodyKey.IsEmpty()) PendingRenameBodyKey = VM->GetCurrentScopeNameKey();
						// Rio 02.10: names are typed in capitals.
						const FString Typed = Text.ToString();
						const FString Upper = Typed.ToUpper();
						if (!Upper.Equals(Typed, ESearchCase::CaseSensitive))
						{
							BodyNameTextBox->SetText(FText::FromString(Upper));
							BodyNameTextBox->GoTo(ETextLocation::EndOfDocument);
						}
					})
					.OnTextCommitted_Lambda([this, VM](const FText& Text, ETextCommit::Type CommitType)
					{
						if (VM.IsValid() && CommitType != ETextCommit::OnCleared && !PendingRenameBodyKey.IsEmpty())
							VM->SetCurrentScopeName(FText::FromString(Text.ToString().ToUpper()), PendingRenameBodyKey);
						PendingRenameBodyKey.Reset();
						NameBoxKey.Reset();
					})
				]
			]
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 11.0f, 0.0f, 0.0f)[SectionTitle(LOCTEXT("CurrentScope", "CURRENT SCOPE"))]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 14.0f, 0.0f, 10.0f)
		[
			SNew(STextBlock).AutoWrapText(true).Font(ReadableFont("Regular", 11)).ColorAndOpacity(SecondaryText)
			.Text_Lambda([VM]()
			{
				if (!VM.IsValid()) return FText::GetEmpty();
				switch (VM->GetPreviewFocus())
				{
				case EAstroPreviewFocus::Galaxy: return LOCTEXT("GalaxyHelp", "Shape the parent galaxy. Modeled star count remains full-scale while the live preview uses a bounded sample.");
				case EAstroPreviewFocus::StarCluster: return LOCTEXT("ClusterHelp", "Configure the selected cluster population and formation inside its parent galaxy.");
				case EAstroPreviewFocus::HomeSystem: return LOCTEXT("SystemHelp", "Edit the complete star system. Orbits, planets and moons remain children of its selected star hierarchy.");
				case EAstroPreviewFocus::HomeStar: return LOCTEXT("StarHelp", "Edit the selected star while retaining its parent system, planets and camera context.");
				case EAstroPreviewFocus::HomePlanet: return LOCTEXT("PlanetHelp", "Edit the selected planet and atmosphere without randomizing its parent system.");
				default: return LOCTEXT("OverviewHelp", "Choose a hierarchy level below, then drag the live scene to orbit or scroll to zoom.");
				}
			})
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 8.0f)[SectionTitle(LOCTEXT("LiveModel", "LIVE MODEL"))]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 10.0f, 0.0f, 12.0f)
		[
			SNew(SBorder).BorderImage(&ControlBrush).Padding(FMargin(12.0f, 10.0f))
			[
				SAssignNew(ModelCardBox, SVerticalBox)
			]
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 2.0f, 0.0f, 8.0f)
		[
			SNew(STextBlock).Font(Font("Bold", 13)).ColorAndOpacity(Cyan)
			.Text_Lambda([VM]() { return VM.IsValid() ? VM->GetPreviewHierarchyTitle() : FText::GetEmpty(); })
		]
		+ SVerticalBox::Slot().FillHeight(1.0f).Padding(0.0f, 0.0f, 0.0f, 8.0f)
		[
			SNew(SBorder).BorderImage(&ControlBrush).Padding(FMargin(7.0f, 6.0f))
			[
				SNew(SScrollBox)
				+ SScrollBox::Slot()
				[BodyHierarchyBox.ToSharedRef()]
			]
		]
		+ SVerticalBox::Slot().AutoHeight()
		[SNew(STextBlock).Text(LOCTEXT("PickHintReadable", "Double-click a body to focus. Use hierarchy controls to move up."))
		.AutoWrapText(true).Font(ReadableFont("Regular", 9)).ColorAndOpacity(SecondaryText)];

	const auto FocusButton = [this](const FText& Glyph, const FText& Label, EAstroPreviewFocus Focus)
	{
		return SNew(SButton)
			.ButtonStyle(&SecondaryButton)
			.HAlign(HAlign_Center).VAlign(VAlign_Center)
			.IsEnabled_Lambda([this, Focus]()
			{
				const UWorldGenerationViewModel* VMValue = ViewModel.Get();
				return VMValue && VMValue->bPreviewReady
					&& VMValue->IsPreviewFocusAvailable(Focus);
			})
			.ButtonColorAndOpacity_Lambda([this, Focus]()
			{
				const UWorldGenerationViewModel* VMValue = ViewModel.Get();
				const EAstroPreviewFocus CurrentFocus = VMValue ? VMValue->GetPreviewFocus() : EAstroPreviewFocus::Overview;
				const bool bSelected = CurrentFocus == Focus;
				return bSelected
					? APSUITheme::RetintAction(FLinearColor(0.32f, 0.13f, 0.005f, 1.0f)) : FLinearColor::White;
			})
			.ContentPadding(FMargin(14.0f, 8.0f))
			.OnClicked(this, &SWorldGenerationPanel::FocusPreview, static_cast<uint8>(Focus))
			[
				// Rio 03.10: the glyph and the name on one middle line, by their capitals.
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.0f, 0.0f, 7.0f, 0.0f)
				[SNew(STextBlock).Text(Glyph).Font(APSGenerationUI::Font("Bold", 9)).ColorAndOpacity(APSGenerationUI::Cyan)
				.RenderTransform(APSGenerationUI::CapsShift("Bold", 9))]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
				[SNew(STextBlock).Text(Label).Font(APSGenerationUI::Font("Bold", 10)).ColorAndOpacity(APSGenerationUI::White)
				.RenderTransform(APSGenerationUI::CapsShift("Bold", 10))]
			];
	};

	ChildSlot
	[
		SNew(SBorder).BorderImage(FAppStyle::GetBrush("WhiteBrush")).BorderBackgroundColor(Background).Padding(FMargin(22.0f, 16.0f))
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
				[
					SNew(SButton).ButtonStyle(&SecondaryButton).OnClicked(this, &SWorldGenerationPanel::GoBack).ContentPadding(FMargin(15.0f, 8.0f))
					.HAlign(HAlign_Center).VAlign(VAlign_Center)
					[SNew(STextBlock).Text(LOCTEXT("Back", "<  BACK")).Justification(ETextJustify::Center)
					.Font(Font("Bold", 12)).ColorAndOpacity(White).OverflowPolicy(ETextOverflowPolicy::Ellipsis)
					.RenderTransform(CapsShift("Bold", 12))]
				]
				+ SHorizontalBox::Slot().FillWidth(1.0f).HAlign(HAlign_Center)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)[SNew(STextBlock).Text(LOCTEXT("Title", "A P O S F E R A")).Font(Font("Bold", 42)).ColorAndOpacity(White)]
					+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0.0f, 2.0f)
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)[SNew(SBox).WidthOverride(78.0f).HeightOverride(1.0f)[SNew(SBorder).BorderImage(FAppStyle::GetBrush("WhiteBrush")).BorderBackgroundColor(Cyan)]]
						+ SHorizontalBox::Slot().AutoWidth().Padding(16.0f, 0.0f)[SNew(STextBlock).Text(LOCTEXT("Subtitle", "A S T R O N O M I C A L   G E N E R A T I O N")).Font(Font("Bold", 13)).ColorAndOpacity(Cyan)]
						+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)[SNew(SBox).WidthOverride(78.0f).HeightOverride(1.0f)[SNew(SBorder).BorderImage(FAppStyle::GetBrush("WhiteBrush")).BorderBackgroundColor(Cyan)]]
					]
				]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
				[
					SNew(SButton).ButtonStyle(&SecondaryButton).OnClicked(this, &SWorldGenerationPanel::RefreshPreview).ContentPadding(FMargin(14.0f, 8.0f))
					.HAlign(HAlign_Center).VAlign(VAlign_Center)
					.ToolTipText(LOCTEXT("RefreshHint", "Roll a new world: stars, planets, the start world and its moons. Clears the edits."))
					[SNew(STextBlock).Text(LOCTEXT("Refresh", "REGENERATE")).Justification(ETextJustify::Center)
					.Font(Font("Bold", 11)).ColorAndOpacity(Cyan).OverflowPolicy(ETextOverflowPolicy::Ellipsis)
					.RenderTransform(CapsShift("Bold", 11))]
				]
			]
			+ SVerticalBox::Slot().FillHeight(1.0f).Padding(0.0f, 14.0f)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().FillWidth(0.23f).Padding(0.0f, 0.0f, 10.0f, 0.0f)
				[
					ChamferPanel(ControlsSwitcher)
				]
				+ SHorizontalBox::Slot().FillWidth(0.54f).Padding(6.0f, 0.0f)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().FillHeight(1.0f)
					[
						SNew(SOverlay)
						+ SOverlay::Slot()[SNew(SPreviewInteractionSurface).ViewModel(VM)]
						+ SOverlay::Slot()
						[
							// Status text owns real layout space. The annotation layer is
							// clipped between these rows, and still projects world positions
							// through its absolute geometry. Camera/input bounds do not move.
							// Only the MARKS toggle takes clicks here; everything else lets drags and picks through.
							SNew(SVerticalBox).Visibility(EVisibility::SelfHitTestInvisible)
							+ SVerticalBox::Slot().AutoHeight().Padding(10.0f, 6.0f, 6.0f, 6.0f)
							[
								SNew(SHorizontalBox).Visibility(EVisibility::SelfHitTestInvisible)
								+ SHorizontalBox::Slot().FillWidth(1.0f).HAlign(HAlign_Left).VAlign(VAlign_Center)
								[
									// Rio 06.10 ("these labels don't read, white on white"): a dark chip plate under the corner text.
									SNew(SBorder).Tag(TEXT("PreviewStatusHeading")).Visibility(EVisibility::HitTestInvisible)
									.BorderImage(&ChipBrush).Padding(FMargin(8.0f, 3.0f))
									[SNew(STextBlock).Text(LOCTEXT("PreviewCornerTL", "+  LIVE FULL-SCALE PREVIEW")).Font(Font("Bold", 9)).ColorAndOpacity(APSUITheme::Current() == EAPSUITheme::Classic ? FLinearColor(0.20f, 0.90f, 0.55f, 0.95f) : APSUITheme::Fade(APSUITheme::Palette().TextSoft, 0.95f))]
								]
								+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
								[
									// Rio 02.10: hide or show all the preview's marks, for just space. Rio 03.10: a view
									// setting, so it lives in the preview's corner, not among the focus modes below.
									SNew(SButton)
									.ButtonStyle(&SecondaryButton)
									.HAlign(HAlign_Center).VAlign(VAlign_Center)
									.ContentPadding(FMargin(14.0f, 4.0f))
									.ToolTipText(LOCTEXT("MarksHint", "Show or hide the preview's orbits, rings and labels."))
									.ButtonColorAndOpacity_Lambda([VM]()
									{
										return VM.IsValid() && VM->ArePreviewMarksHidden() ? APSUITheme::RetintAction(FLinearColor(0.32f, 0.13f, 0.005f, 1.0f)) : FLinearColor::White;
									})
									.OnClicked_Lambda([VM]()
									{
										if (UWorldGenerationViewModel* MutableVM = VM.Get())
										{
											MutableVM->SetPreviewMarksHidden(!MutableVM->ArePreviewMarksHidden());
										}
										return FReply::Handled();
									})
									[
										SNew(STextBlock)
										.Text_Lambda([VM]() { return VM.IsValid() && VM->ArePreviewMarksHidden() ? LOCTEXT("MarksHidden", "MARKS OFF") : LOCTEXT("MarksShown", "MARKS ON"); })
										.Font(Font("Bold", 9)).ColorAndOpacity(White)
										.Justification(ETextJustify::Center).RenderTransform(CapsShift("Bold", 9))
									]
								]
							]
							+ SVerticalBox::Slot().FillHeight(1.0f)
							[SNew(SPreviewSystemOverlay).ViewModel(VM)]
							+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Right).Padding(10.0f)
							[
								SNew(SBorder).Tag(TEXT("PreviewStatusFooter")).Visibility(EVisibility::HitTestInvisible)
								.BorderImage(&ChipBrush).Padding(FMargin(8.0f, 3.0f))
								[SNew(STextBlock).Text(LOCTEXT("PreviewCornerBR", "FULL SCALE  +")).Font(Font("Bold", 8)).ColorAndOpacity(FLinearColor(Cyan.R, Cyan.G, Cyan.B, 0.9f))]
							]
						]
					]
					+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0.0f, 10.0f)
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().AutoWidth().Padding(2.0f)[FocusButton(LOCTEXT("OverviewGlyph", "O"), LOCTEXT("FocusOverview", "OVERVIEW"), EAstroPreviewFocus::Overview)]
						+ SHorizontalBox::Slot().AutoWidth().Padding(2.0f)[FocusButton(LOCTEXT("GalaxyGlyph", "GX"), LOCTEXT("FocusGalaxy", "GALAXY"), EAstroPreviewFocus::Galaxy)]
						+ SHorizontalBox::Slot().AutoWidth().Padding(2.0f)[FocusButton(LOCTEXT("ClusterGlyph", "CL"), LOCTEXT("FocusCluster", "CLUSTER"), EAstroPreviewFocus::StarCluster)]
						+ SHorizontalBox::Slot().AutoWidth().Padding(2.0f)[FocusButton(LOCTEXT("SystemGlyph", "SYS"), LOCTEXT("FocusSystem", "SYSTEM"), EAstroPreviewFocus::HomeSystem)]
						+ SHorizontalBox::Slot().AutoWidth().Padding(2.0f)[FocusButton(LOCTEXT("StarGlyph", "S"), LOCTEXT("FocusStar", "STAR"), EAstroPreviewFocus::HomeStar)]
						+ SHorizontalBox::Slot().AutoWidth().Padding(2.0f)[FocusButton(LOCTEXT("PlanetGlyph", "P"), LOCTEXT("FocusPlanet", "PLANET"), EAstroPreviewFocus::HomePlanet)]
					]
					+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0.0f, 7.0f, 0.0f, 0.0f)
					[SNew(STextBlock).Text(LOCTEXT("PreviewHintReadable", "RMB drag to rotate   /   Mouse wheel to zoom   /   Double-click to focus"))
					.Font(ReadableFont("Regular", 9)).ColorAndOpacity(SecondaryText)]
				]
				+ SHorizontalBox::Slot().FillWidth(0.23f).Padding(10.0f, 0.0f, 0.0f, 0.0f)
				[
					ChamferPanel(ContextPanel)
				]
			]
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().FillWidth(1.0f).HAlign(HAlign_Left).VAlign(VAlign_Center)
				[
					// Rio 06.10: the same plate as the preview's corner texts, so the status reads over any frame.
					SNew(SBorder).BorderImage(&ChipBrush).Padding(FMargin(10.0f, 4.0f))
					[
						SNew(STextBlock).Text(this, &SWorldGenerationPanel::GetPreviewStatus).Font(Font("Bold", 11)).ColorAndOpacity(Cyan)
						.RenderTransform(CapsShift("Bold", 11))
					]
				]
				+ SHorizontalBox::Slot().AutoWidth()
				[
					SNew(SButton).ButtonStyle(&PrimaryButton).OnClicked(this, &SWorldGenerationPanel::CommitWorld)
					.HAlign(HAlign_Center).VAlign(VAlign_Center)
					.IsEnabled_Lambda([VM]() { return VM.IsValid() && VM->bPreviewReady
						&& (VM->GetGenerationRoute() != EAPSGenerationRoute::Civilization || VM->GetHomeStartPlanetCount() > 0); })
					.ToolTipText_Lambda([VM]() { return VM.IsValid() && VM->IsRealScaleActive()
						? LOCTEXT("RealScaleContinueHint", "REAL DISTANCES: the game starts at real distances, neighbour stars light years away.")
						: VM.IsValid() && VM->GetGenerationRoute() == EAPSGenerationRoute::Civilization
						&& VM->GetHomeStartPlanetCount() == 0 ? LOCTEXT("HomePlanetRequiredHint", "A civilization needs a planet in the home system.") : FText::GetEmpty(); })
					.ContentPadding(FMargin(52.0f, 13.0f))
					[SNew(STextBlock).Text(this, &SWorldGenerationPanel::GetContinueLabel)
					.Justification(ETextJustify::Center).OverflowPolicy(ETextOverflowPolicy::Ellipsis)
					.Font(Font("Bold", 14)).ColorAndOpacity(White).RenderTransform(CapsShift("Bold", 14))]
				]
			]
		]
	];

	if (UWorldGenerationViewModel* MutableVM = ViewModel.Get())
	{
		// Rio 03.10: a new world flow starts from a freshly rolled world, not the one default world everybody shared.
		MutableVM->RollFreshWorldOnce();
		MutableVM->RequestPreview();
	}
	RegisterActiveTimer(0.35f, FWidgetActiveTimerDelegate::CreateSP(this, &SWorldGenerationPanel::RefreshBodyHierarchy));
}

#if WITH_DEV_AUTOMATION_TESTS
bool SWorldGenerationPanel::CommitSurfaceControlForAutomation(
	const EAPSGenerationSurfaceControl Control, const double Value)
{
	const TSharedPtr<SAPSGenerationRangeSlider>* Slider = SurfaceControlSliders.Find(Control);
	if (!Slider || !Slider->IsValid() || !FMath::IsFinite(Value))
	{
		return false;
	}
	(*Slider)->CommitValueForAutomation(static_cast<float>(Value));
	return true;
}

double SWorldGenerationPanel::GetSurfaceControlValueForAutomation(
	const EAPSGenerationSurfaceControl Control) const
{
	const TSharedPtr<SAPSGenerationRangeSlider>* Slider = SurfaceControlSliders.Find(Control);
	return Slider && Slider->IsValid()
		? static_cast<double>((*Slider)->GetValue())
		: TNumericLimits<double>::Lowest();
}
#endif

FText SWorldGenerationPanel::GetPreviewStatus() const
{
	if (const UWorldGenerationViewModel* VM = ViewModel.Get()) return VM->PreviewStatus;
	return LOCTEXT("NoViewModel", "PREVIEW OFFLINE");
}

FText SWorldGenerationPanel::GetContinueLabel() const
{
	const UWorldGenerationViewModel* VM = ViewModel.Get();
	if (!VM) return LOCTEXT("ContinueUnavailable", "CONTINUE   >");
	if (VM->GetGenerationRoute() == EAPSGenerationRoute::Civilization && VM->GetHomeStartPlanetCount() == 0)
		return LOCTEXT("HomePlanetRequired", "HOME PLANET REQUIRED");
	switch (VM->GetGenerationRoute())
	{
	case EAPSGenerationRoute::Space: return LOCTEXT("GenerateSpace", "GENERATE SPACE WORLD   >");
	case EAPSGenerationRoute::Planet: return LOCTEXT("GeneratePlanet", "GENERATE PLANET   >");
	default: return LOCTEXT("ContinueCivilization", "CONTINUE TO CIVILIZATION   >");
	}
}

FReply SWorldGenerationPanel::CommitWorld()
{
	if (OnContinue.IsBound()) OnContinue.Execute();
	else if (UWorldGenerationViewModel* VM = ViewModel.Get()) VM->CommitAndOpenLevel();
	return FReply::Handled();
}

FReply SWorldGenerationPanel::RefreshPreview()
{
	if (UWorldGenerationViewModel* VM = ViewModel.Get()) VM->RegeneratePreviewVariant();
	return FReply::Handled();
}

FReply SWorldGenerationPanel::GoBack()
{
	OnBack.ExecuteIfBound();
	return FReply::Handled();
}

FReply SWorldGenerationPanel::FocusPreview(uint8 FocusValue)
{
	PendingRenameBodyKey.Reset();
	if (UWorldGenerationViewModel* VM = ViewModel.Get()) VM->SetPreviewFocus(static_cast<EAstroPreviewFocus>(FocusValue));
	return FReply::Handled();
}

FReply SWorldGenerationPanel::FocusPreviewUp()
{
	PendingRenameBodyKey.Reset();
	if (UWorldGenerationViewModel* VM = ViewModel.Get())
	{
		VM->FocusPreviewParent();
	}
	return FReply::Handled();
}

FReply SWorldGenerationPanel::FocusPreviewBody(TWeakObjectPtr<AActor> BodyActor)
{
	PendingRenameBodyKey.Reset();
	if (UWorldGenerationViewModel* VM = ViewModel.Get())
	{
		VM->FocusPreviewBody(BodyActor);
	}
	return FReply::Handled();
}

FReply SWorldGenerationPanel::FocusPreviewHierarchyEntry(
	TWeakObjectPtr<AActor> BodyActor, int32 ClusterSystemInstanceIndex, int32 PreviewFocusValue)
{
	PendingRenameBodyKey.Reset();
	if (UWorldGenerationViewModel* VM = ViewModel.Get())
	{
		if (PreviewFocusValue != INDEX_NONE)
		{
			VM->SetPreviewFocus(static_cast<EAstroPreviewFocus>(PreviewFocusValue));
		}
		else if (ClusterSystemInstanceIndex != INDEX_NONE)
		{
			VM->FocusPreviewClusterSystem(ClusterSystemInstanceIndex);
		}
		else
		{
			VM->FocusPreviewBody(BodyActor);
		}
	}
	return FReply::Handled();
}

EActiveTimerReturnType SWorldGenerationPanel::RefreshBodyHierarchy(double CurrentTime, float DeltaTime)
{
	UWorldGenerationViewModel* VM = ViewModel.Get();
	if (!VM || !BodyHierarchyBox.IsValid())
	{
		return EActiveTimerReturnType::Continue;
	}
	RefreshModelCard();
	RefreshNameBox();

	TArray<FAPSPreviewBodyEntry> Entries;
	VM->GetPreviewHierarchyEntries(Entries);
	if (LastHierarchySelection.Get() != VM->GetSelectedPreviewBody())
	{
		// The user may already be typing on the new body before this timer runs.
		// Discard only an edit for a different identity, not that body's new edit.
		if (PendingRenameBodyKey != VM->GetPreviewObjectStableKey(VM->GetSelectedPreviewBody()))
			PendingRenameBodyKey.Reset();
		LastHierarchySelection = VM->GetSelectedPreviewBody();
		const int32 SelectedIndex = Entries.IndexOfByPredicate([this](const FAPSPreviewBodyEntry& Entry)
		{
			return Entry.Actor.IsValid() && Entry.Actor == LastHierarchySelection;
		});
		if (SelectedIndex != INDEX_NONE)
		{
			// Selecting through the scene reveals the path, without expanding siblings.
			int32 AncestorDepth = Entries[SelectedIndex].Depth;
			for (int32 Index = SelectedIndex - 1; Index >= 0; --Index)
			{
				if (Entries[Index].Depth >= AncestorDepth) continue;
				AncestorDepth = Entries[Index].Depth;
				if (CollapsedHierarchyEntries.Remove(GetHierarchyEntryKey(Entries[Index])) > 0)
					++HierarchyExpansionRevision;
			}
		}
	}
	uint32 Signature = HashCombineFast(GetTypeHash(VM->PreviewRevision), GetTypeHash(Entries.Num()));
	Signature = HashCombineFast(Signature, GetTypeHash(HierarchyExpansionRevision));
	for (const FAPSPreviewBodyEntry& Entry : Entries)
	{
		Signature = HashCombineFast(Signature, GetTypeHash(Entry.Actor.Get()));
		Signature = HashCombineFast(Signature, GetTypeHash(Entry.ClusterSystemInstanceIndex));
		Signature = HashCombineFast(Signature, GetTypeHash(Entry.PreviewFocusValue));
		Signature = HashCombineFast(Signature, GetTypeHash(Entry.Depth));
		Signature = HashCombineFast(Signature, GetTypeHash(Entry.Label.ToString()));
		Signature = HashCombineFast(Signature, GetTypeHash(Entry.Details.ToString()));
	}
	if (Signature != BodyHierarchySignature)
	{
		RebuildBodyHierarchy(Entries, Signature);
	}
	return EActiveTimerReturnType::Continue;
}

FString SWorldGenerationPanel::GetHierarchyEntryKey(const FAPSPreviewBodyEntry& Entry) const
{
	if (Entry.Actor.IsValid())
	{
		if (const UWorldGenerationViewModel* VM = ViewModel.Get())
		{
			const FString StableKey = VM->GetPreviewObjectStableKey(Entry.Actor.Get());
			if (!StableKey.IsEmpty()) return StableKey;
		}
		return Entry.Actor->GetPathName();
	}
	if (Entry.ClusterSystemInstanceIndex != INDEX_NONE)
		return FString::Printf(TEXT("cluster-record/%d"), Entry.ClusterSystemInstanceIndex);
	return FString::Printf(TEXT("scope/%d"), Entry.PreviewFocusValue);
}

FReply SWorldGenerationPanel::ToggleHierarchyChildren(FString EntryKey)
{
	if (CollapsedHierarchyEntries.Contains(EntryKey)) CollapsedHierarchyEntries.Remove(EntryKey);
	else CollapsedHierarchyEntries.Add(MoveTemp(EntryKey));
	++HierarchyExpansionRevision;
	// The timer rebuilds after this button's event has finished, not during it.
	return FReply::Handled();
}

void SWorldGenerationPanel::RebuildBodyHierarchy(const TArray<FAPSPreviewBodyEntry>& Entries, uint32 Signature)
{
	using namespace APSGenerationUI;
	if (!BodyHierarchyBox.IsValid())
	{
		return;
	}

	BodyHierarchyBox->ClearChildren();
	BodyHierarchySignature = Signature;
	if (Entries.IsEmpty())
	{
		BodyHierarchyBox->AddSlot().AutoHeight().Padding(0.0f, 6.0f)
		[
			SNew(STextBlock).Text(LOCTEXT("BodiesPending", "MATERIALIZING SYSTEM HIERARCHY..."))
			.Font(ReadableFont("Regular", 10)).ColorAndOpacity(SecondaryText).AutoWrapText(true)
		];
		return;
	}

	int32 CollapsedParentDepth = MAX_int32;
	for (int32 EntryIndex = 0; EntryIndex < Entries.Num(); ++EntryIndex)
	{
		const FAPSPreviewBodyEntry& Entry = Entries[EntryIndex];
		if (CollapsedParentDepth != MAX_int32)
		{
			if (Entry.Depth > CollapsedParentDepth) continue;
			CollapsedParentDepth = MAX_int32;
		}
		const FString EntryKey = GetHierarchyEntryKey(Entry);
		const bool bCollapsed = CollapsedHierarchyEntries.Contains(EntryKey);
		const TWeakObjectPtr<AActor> BodyActor = Entry.Actor;
		const int32 ClusterSystemInstanceIndex = Entry.ClusterSystemInstanceIndex;
		const int32 PreviewFocusValue = Entry.PreviewFocusValue;
		const bool bCanFocus = BodyActor.IsValid() || ClusterSystemInstanceIndex != INDEX_NONE
			|| PreviewFocusValue != INDEX_NONE;
		const int32 VisualDepth = FMath::Clamp(Entry.Depth, 0, 4);
		int32 ImmediateChildCount = 0;
		for (int32 ChildIndex = EntryIndex + 1; ChildIndex < Entries.Num(); ++ChildIndex)
		{
			if (Entries[ChildIndex].Depth <= Entry.Depth)
			{
				break;
			}
			if (Entries[ChildIndex].Depth == Entry.Depth + 1)
			{
				++ImmediateChildCount;
			}
		}
		if (bCollapsed && ImmediateChildCount > 0) CollapsedParentDepth = Entry.Depth;
		const EHierarchyGlyph Glyph = [&BodyActor, PreviewFocusValue]()
		{
			if (BodyActor.IsValid())
			{
				if (BodyActor->IsA<AStar>()) return EHierarchyGlyph::Star;
				if (BodyActor->IsA<AMoon>()) return EHierarchyGlyph::Moon;
				if (BodyActor->IsA<APlanet>()) return EHierarchyGlyph::Planet;
			}
			if (PreviewFocusValue == static_cast<int32>(EAstroPreviewFocus::Galaxy))
			{
				return EHierarchyGlyph::Galaxy;
			}
			if (PreviewFocusValue == static_cast<int32>(EAstroPreviewFocus::StarCluster))
			{
				return EHierarchyGlyph::Cluster;
			}
			return EHierarchyGlyph::System;
		}();
		const auto IsSelected = [this, BodyActor, PreviewFocusValue]()
		{
			const UWorldGenerationViewModel* VM = ViewModel.Get();
			return VM && ((BodyActor.IsValid()
				&& VM->GetSelectedPreviewBody() == BodyActor.Get())
				|| (PreviewFocusValue != INDEX_NONE
					&& static_cast<int32>(VM->GetPreviewFocus()) == PreviewFocusValue));
		};
		// Rio 02.10: who contains whom. A row whose subtree holds the selection is tinted cyan.
		TArray<TWeakObjectPtr<AActor>> Descendants;
		for (int32 ChildIndex = EntryIndex + 1; ChildIndex < Entries.Num() && Entries[ChildIndex].Depth > Entry.Depth;
			++ChildIndex)
		{
			if (Entries[ChildIndex].Actor.IsValid()) Descendants.Add(Entries[ChildIndex].Actor);
		}
		const auto ContainsSelection = [this, Descendants]()
		{
			const UWorldGenerationViewModel* VM = ViewModel.Get();
			const AActor* Selected = VM ? VM->GetSelectedPreviewBody() : nullptr;
			return Selected && Descendants.ContainsByPredicate(
				[Selected](const TWeakObjectPtr<AActor>& Actor) { return Actor.Get() == Selected; });
		};
		const FString Designation = APSBodyDesignation::Of(BodyActor.Get());
		BodyHierarchyBox->AddSlot().AutoHeight().Padding(0.0f, 2.0f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.0f, 0.0f, 3.0f, 0.0f)
			[
				SNew(SBox).WidthOverride(23.0f)
				[
					SNew(SButton).ButtonStyle(&SecondaryButton).ContentPadding(0.0f)
					.HAlign(HAlign_Center).VAlign(VAlign_Center)
					.Visibility(ImmediateChildCount > 0 ? EVisibility::Visible : EVisibility::Hidden)
					.OnClicked(this, &SWorldGenerationPanel::ToggleHierarchyChildren, EntryKey)
					.ToolTipText(bCollapsed ? LOCTEXT("ExpandChildren", "Show children") : LOCTEXT("CollapseChildren", "Hide children"))
					[SNew(STextBlock).Text(FText::FromString(bCollapsed ? TEXT("+") : TEXT("-")))
					.Justification(ETextJustify::Center).Font(Font("Bold", 10)).ColorAndOpacity(Cyan)
					.RenderTransform(SymbolShift("Bold", 10))]
				]
			]
			+ SHorizontalBox::Slot().FillWidth(1.0f)
			[
			SNew(SButton)
			.ButtonStyle(&HierarchyButton)
			.ContentPadding(FMargin(7.0f, 5.0f))
			.IsEnabled(bCanFocus)
			.OnClicked(this, &SWorldGenerationPanel::FocusPreviewHierarchyEntry,
				BodyActor, ClusterSystemInstanceIndex, PreviewFocusValue)
			[
				SNew(SOverlay)
				+ SOverlay::Slot()
				[
					SNew(SBorder).BorderImage(FAppStyle::GetBrush("WhiteBrush"))
					.BorderBackgroundColor_Lambda([IsSelected, ContainsSelection]()
					{
						return IsSelected() ? SelectedFill : ContainsSelection() ? AncestorFill : FLinearColor::Transparent;
					})
					.Visibility(EVisibility::HitTestInvisible)
				]
				+ SOverlay::Slot()
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Fill)
					[
						SNew(SGenerationHierarchyBranch)
						.Depth(VisualDepth)
						.Color_Lambda([IsSelected]() { return IsSelected() ? Amber : Cyan; })
					]
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(2.0f, 0.0f, 6.0f, 0.0f)
					[
						SNew(SGenerationHierarchyGlyph)
						.Glyph(Glyph)
						.Color_Lambda([IsSelected]() { return IsSelected() ? Amber : Cyan; })
					]
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.0f, 0.0f, 6.0f, 0.0f)
					[
						SNew(SBox).Visibility(Designation.IsEmpty() ? EVisibility::Collapsed : EVisibility::Visible)
						.MinDesiredWidth(26.0f)
						[
							// Rio 03.10: even padding, the designation centred by its capitals.
							SNew(SBorder).BorderImage(&ChipBrush).Padding(FMargin(7.0f, 3.0f)).HAlign(HAlign_Center).VAlign(VAlign_Center)
							[
								SNew(STextBlock).Text(FText::FromString(Designation)).Font(Font("Bold", 8))
								.Justification(ETextJustify::Center).RenderTransform(CapsShift("Bold", 8))
								.ColorAndOpacity_Lambda([IsSelected]() { return IsSelected() ? Amber : Cyan; })
							]
						]
					]
					+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center)
					[
						SNew(SVerticalBox)
						+ SVerticalBox::Slot().AutoHeight()
						[
							SNew(STextBlock).Text(Entry.Label).Font(ReadableFont("Bold", 10))
							.ColorAndOpacity_Lambda([IsSelected]() { return IsSelected() ? Amber : White; })
							.OverflowPolicy(ETextOverflowPolicy::Ellipsis)
							.ToolTipText(Entry.Label)
						]
						+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 2.0f, 0.0f, 0.0f)
						[
							SNew(STextBlock).Text(Entry.Details).Font(ReadableFont("Regular", 9))
							.ColorAndOpacity(SecondaryText).OverflowPolicy(ETextOverflowPolicy::Ellipsis)
							.ToolTipText(Entry.Details)
						]
					]
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(7.0f, 0.0f)
					[
						SNew(SBox).WidthOverride(25.0f).HeightOverride(20.0f)
						.Visibility(ImmediateChildCount > 0 ? EVisibility::Visible : EVisibility::Collapsed)
						[
							// The count in the middle of its box (it sat at the top).
							SNew(SBorder).BorderImage(&ControlBrush).Padding(0.0f).HAlign(HAlign_Center).VAlign(VAlign_Center)
							[
								SNew(STextBlock).Text(APSUINumber::Number(ImmediateChildCount))
								.Justification(ETextJustify::Center).Font(Font("Bold", 8)).ColorAndOpacity(Cyan)
								.RenderTransform(CapsShift("Bold", 8))
							]
						]
					]
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
					[
						SNew(STextBlock).Text(bCanFocus ? FText::FromString(TEXT(">")) : FText::GetEmpty())
						.Font(Font("Bold", 9)).RenderTransform(SymbolShift("Bold", 9)).ColorAndOpacity_Lambda([IsSelected]()
						{
							return IsSelected() ? Amber : Cyan;
						})
					]
				]
				+ SOverlay::Slot().HAlign(HAlign_Left)
				[
					SNew(SBox).WidthOverride(3.0f)
					.Visibility_Lambda([IsSelected, ContainsSelection]()
					{
						return IsSelected() || ContainsSelection() ? EVisibility::HitTestInvisible : EVisibility::Collapsed;
					})
					[
						SNew(SBorder).BorderImage(FAppStyle::GetBrush("WhiteBrush"))
						.BorderBackgroundColor_Lambda([IsSelected]() { return IsSelected() ? Amber : Cyan; })
					]
				]
			]
			]
		];
	}
}

void SWorldGenerationPanel::RefreshModelCard()
{
	const UWorldGenerationViewModel* VM = ViewModel.Get();
	if (!VM || !ModelCardBox.IsValid())
	{
		return;
	}
	FAPSModelCard Card;
	VM->GetPreviewModelCard(Card);
	// Rebuilt only when a shown text changes; values such as the live star count settle within a few refreshes.
	FString Signature = FString::Printf(TEXT("%d|%s|%s|%s|%s"), static_cast<int32>(Card.Glyph), *Card.Kind.ToString(),
		*Card.Designation.ToString(), *Card.Title.ToString(), *Card.Subtitle.ToString());
	for (const FAPSModelFact& Fact : Card.Facts)
	{
		Signature += FString::Printf(TEXT("|%d%d%s=%s %s %s"), static_cast<int32>(Fact.Glyph), Fact.bAccent ? 1 : 0,
			*Fact.Label.ToString(), *Fact.Value.ToString(), *Fact.Unit.ToString(), *Fact.Note.ToString());
	}
	if (Signature != ModelCardSignature)
	{
		ModelCardSignature = MoveTemp(Signature);
		RebuildModelCard(Card);
	}
}

void SWorldGenerationPanel::RebuildModelCard(const FAPSModelCard& Card)
{
	using namespace APSGenerationUI;
	ModelCardBox->ClearChildren();
	const FLinearColor TileFill = APSUITheme::Retint(FLinearColor(0.006f, 0.034f, 0.052f, 0.92f));
	const bool bClassicTheme = APSUITheme::Current() == EAPSUITheme::Classic;
	// Rio 06.10: three tiles to a row (two in Classic, as it was).
	const int32 Columns = bClassicTheme ? 2 : 3;

	// Header: icon badge, kind and designation, the name, a one-line description.
	ModelCardBox->AddSlot().AutoHeight()
	[
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Top).Padding(0.0f, 2.0f, 10.0f, 0.0f)
		[
			SNew(SBox).WidthOverride(36.0f).HeightOverride(36.0f)
			[
				SNew(SBorder).BorderImage(&BadgeBrush).Padding(0.0f).HAlign(HAlign_Center).VAlign(VAlign_Center)
				[SNew(SAPSModelGlyph).Glyph(Card.Glyph).Color(Cyan).Size(22.0f)]
			]
		]
		+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
				[SNew(STextBlock).Text(Card.Kind).Font(Font("Bold", 9)).ColorAndOpacity(Cyan).RenderTransform(CapsShift("Bold", 9))]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(8.0f, 0.0f, 0.0f, 0.0f)
				[
					SNew(SBox).Visibility(Card.Designation.IsEmpty() ? EVisibility::Collapsed : EVisibility::Visible)
					[
						// Rio 03.10: even padding, the designation centred by its capitals.
						SNew(SBorder).BorderImage(&ChipBrush).Padding(FMargin(8.0f, 3.0f)).HAlign(HAlign_Center).VAlign(VAlign_Center)
						[SNew(STextBlock).Text(Card.Designation).Font(Font("Bold", 9)).ColorAndOpacity(Amber)
						.Justification(ETextJustify::Center).RenderTransform(CapsShift("Bold", 9))]
					]
				]
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 3.0f, 0.0f, 0.0f)
			[SNew(STextBlock).Text(Card.Title).Font(Font("Bold", 15)).ColorAndOpacity(White).AutoWrapText(true)]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 2.0f, 0.0f, 0.0f)
			[
				SNew(STextBlock).Text(Card.Subtitle).Font(ReadableFont("Regular", 9)).ColorAndOpacity(SecondaryText)
				.AutoWrapText(true).Visibility(Card.Subtitle.IsEmpty() ? EVisibility::Collapsed : EVisibility::Visible)
			]
		]
	];
	if (Card.Facts.IsEmpty())
	{
		return;
	}
	ModelCardBox->AddSlot().AutoHeight().Padding(0.0f, 10.0f, 0.0f, 8.0f)
	[SNew(SBox).HeightOverride(1.0f)[SNew(SBorder).BorderImage(FAppStyle::GetBrush("WhiteBrush")).BorderBackgroundColor(CyanDim)]];

	// Facts: two tiles per row. Label with its icon on top, the value highlighted, the unit beside it.
	TSharedRef<SGridPanel> Grid = SNew(SGridPanel).FillColumn(0, 1.0f).FillColumn(1, 1.0f).FillColumn(2, Columns > 2 ? 1.0f : 0.0f);
	for (int32 Index = 0; Index < Card.Facts.Num(); ++Index)
	{
		const FAPSModelFact& Fact = Card.Facts[Index];
		const FLinearColor ValueColor = Fact.bAccent ? (bClassicTheme ? Amber : APSUITheme::Palette().ActionPeak) : Cyan;
		// Enum readings ("MAIN SEQUENCE STAR") are words, numbers stay large.
		const bool bWords = Fact.Value.ToString().Len() > (Columns > 2 ? 7 : 9);
		const int32 Column = Index % Columns;
		const float Gap = Columns > 2 ? 3.0f : 4.0f;
		Grid->AddSlot(Column, Index / Columns)
			.Padding(FMargin(Column == 0 ? 0.0f : Gap, 0.0f, Column == Columns - 1 ? 0.0f : Gap, Columns > 2 ? 5.0f : 6.0f))
		[
			SNew(SBorder).BorderImage(FAppStyle::GetBrush("WhiteBrush")).BorderBackgroundColor(TileFill)
			.Padding(Columns > 2 ? FMargin(7.0f, 5.0f) : FMargin(8.0f, 6.0f))
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.0f, 0.0f, 5.0f, 0.0f)
					[SNew(SAPSModelGlyph).Glyph(Fact.Glyph).Color(FLinearColor(ValueColor.R, ValueColor.G, ValueColor.B, 0.75f)).Size(14.0f)]
					+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center)
					[SNew(STextBlock).Text(Fact.Label).Font(Font("Bold", 8)).ColorAndOpacity(SecondaryText).RenderTransform(CapsShift("Bold", 8))]
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 4.0f, 0.0f, 0.0f)
				[
					bWords
					? StaticCastSharedRef<SWidget>(SNew(STextBlock).Text(Fact.Unit.IsEmpty() ? Fact.Value
						: FText::Format(LOCTEXT("ModelWordsUnit", "{0}  {1}"), Fact.Value, Fact.Unit))
						.Font(Font("Bold", 10)).ColorAndOpacity(ValueColor).AutoWrapText(true))
					: StaticCastSharedRef<SWidget>(SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Bottom)
						[
							SNew(STextBlock).Text(Fact.Value).Font(Font("Bold", Columns > 2 ? 13 : 15)).ColorAndOpacity(ValueColor)
						]
						+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Bottom).Padding(5.0f, 0.0f, 0.0f, 2.0f)
						[
							SNew(STextBlock).Text(Fact.Unit).Font(Font("Bold", 8)).ColorAndOpacity(SecondaryText)
							.AutoWrapText(true).Visibility(Fact.Unit.IsEmpty() ? EVisibility::Collapsed : EVisibility::Visible)
						])
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 2.0f, 0.0f, 0.0f)
				[
					SNew(STextBlock).Text(Fact.Note).Font(ReadableFont("Regular", 8)).ColorAndOpacity(SecondaryText)
					.AutoWrapText(true).Visibility(Fact.Note.IsEmpty() ? EVisibility::Collapsed : EVisibility::Visible)
				]
			]
		];
	}
	ModelCardBox->AddSlot().AutoHeight()[Grid];
}

void SWorldGenerationPanel::RefreshNameBox()
{
	const UWorldGenerationViewModel* VM = ViewModel.Get();
	if (!VM || !BodyNameTextBox.IsValid() || BodyNameTextBox->HasKeyboardFocus())
	{
		return;
	}
	const FString Key = VM->GetCurrentScopeNameKey();
	const FString Value = VM->GetCurrentScopeName().ToString();
	if (Key == NameBoxKey && Value == NameBoxValue)
	{
		return;
	}
	NameBoxKey = Key;
	NameBoxValue = Value;
	BodyNameTextBox->SetText(FText::FromString(Value));
}

#undef LOCTEXT_NAMESPACE
