#include "SAPSMainMenuRoot.h"

#include "APS_ALPHA/Core/Controllers/MainMenuController.h"
#include "APS_ALPHA/Core/Enums/CharSpawnPlace.h"
#include "APS_ALPHA/Core/Enums/OrbitHeight.h"
#include "APS_ALPHA/Core/Interfaces/ItemInfoInterface.h"
#include "APS_ALPHA/Core/Model/SpawnParameters.h"
#include "APS_ALPHA/Core/Model/GeneratedWorld.h"
#include "APS_ALPHA/Core/Saves/GameSave.h"
#include "APS_ALPHA/Core/Saves/GeneratedWorldData.h"
#include "APS_ALPHA/Pawns/Spaceships/Spaceship.h"
#include "APS_ALPHA/Actors/Tech/SpaceHeadquarters.h"
#include "APS_ALPHA/Actors/Tech/SpaceShipyard.h"
#include "APS_ALPHA/Actors/Tech/SpaceStation.h"
#include "APS_ALPHA/UI/MainMenu/SWorldGenerationPanel.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Blueprint/BlueprintSupport.h"
#include "Engine/Blueprint.h"
#include "Engine/Texture2D.h"
#include "Engine/Font.h"
#include "Engine/AssetManager.h"
#include "Engine/StreamableManager.h"
#include "Blueprint/UserWidget.h"
#include "Framework/Application/SlateApplication.h"
#include "GameFramework/Pawn.h"
#include "HAL/FileManager.h"
#include "Input/Events.h"
#include "InputCoreTypes.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/Paths.h"
#include "Modules/ModuleManager.h"
#include "Rendering/DrawElements.h"
#include "Textures/SlateShaderResource.h"
#include "Styling/AppStyle.h"
#include "Styling/SlateBrush.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SSearchBox.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Input/SSpinBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScaleBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SUniformGridPanel.h"
#include "Widgets/Layout/SWidgetSwitcher.h"
#include "Widgets/Notifications/SProgressBar.h"
#include "Widgets/SLeafWidget.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "APSMainMenuRoot"

namespace
{
	enum class EAPSMenuGlyph : uint8
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
		System
	};

	/** Small code-native line icons keep the Slate-only menu readable before the
	 * final art pass, without depending on Unicode coverage or editor-authored
	 * image widgets. */
	class SVectorMenuGlyph final : public SLeafWidget
	{
	public:
		SLATE_BEGIN_ARGS(SVectorMenuGlyph) {}
			SLATE_ARGUMENT(EAPSMenuGlyph, Glyph)
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
			case EAPSMenuGlyph::Compass:
				Circle(Center, Radius * 0.88f, Radius * 0.88f);
				Draw({Center + FVector2D(0.0f, -Radius), Center + FVector2D(Radius * 0.22f, -Radius * 0.18f),
					Center, Center + FVector2D(-Radius * 0.22f, Radius * 0.18f), Center + FVector2D(0.0f, Radius)}, true);
				Draw({Center + FVector2D(-Radius, 0.0f), Center + FVector2D(Radius, 0.0f)}, false, 0.65f);
				break;
			case EAPSMenuGlyph::World:
				Circle(Center, Radius, Radius);
				Circle(Center, Radius * 0.43f, Radius);
				Draw({Center + FVector2D(-Radius, 0.0f), Center + FVector2D(Radius, 0.0f)});
				break;
			case EAPSMenuGlyph::Civilization:
				Circle(Center, Radius * 0.30f, Radius * 0.30f);
				for (int32 Index = 0; Index < 6; ++Index)
				{
					const float Angle = UE_TWO_PI * static_cast<float>(Index) / 6.0f;
					const FVector2D Direction(FMath::Cos(Angle), FMath::Sin(Angle));
					Draw({Center + Direction * Radius * 0.30f, Center + Direction * Radius});
					Circle(Center + Direction * Radius, Radius * 0.13f, Radius * 0.13f);
				}
				break;
			case EAPSMenuGlyph::Space:
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
			case EAPSMenuGlyph::Planet:
				Circle(Center, Radius * 0.72f, Radius * 0.72f);
				Circle(Center, Radius * 1.18f, Radius * 0.35f, -0.32f);
				break;
			case EAPSMenuGlyph::Lock:
				Circle(Center + FVector2D(0.0f, -Radius * 0.26f), Radius * 0.48f, Radius * 0.55f);
				Draw({Center + FVector2D(-Radius * 0.62f, -Radius * 0.10f), Center + FVector2D(Radius * 0.62f, -Radius * 0.10f),
					Center + FVector2D(Radius * 0.62f, Radius * 0.70f), Center + FVector2D(-Radius * 0.62f, Radius * 0.70f)}, true);
				break;
			case EAPSMenuGlyph::Profile:
				Circle(Center + FVector2D(0.0f, -Radius * 0.43f), Radius * 0.36f, Radius * 0.36f);
				Circle(Center + FVector2D(0.0f, Radius * 0.68f), Radius * 0.78f, Radius * 0.62f);
				break;
			case EAPSMenuGlyph::Settings:
				Circle(Center, Radius * 0.42f, Radius * 0.42f);
				for (int32 Index = 0; Index < 8; ++Index)
				{
					const float Angle = UE_TWO_PI * static_cast<float>(Index) / 8.0f;
					const FVector2D D(FMath::Cos(Angle), FMath::Sin(Angle));
					Draw({Center + D * Radius * 0.62f, Center + D * Radius});
				}
				break;
			case EAPSMenuGlyph::Collection:
				for (int32 X = -1; X <= 1; X += 2)
				for (int32 Y = -1; Y <= 1; Y += 2)
				{
					const FVector2D C = Center + FVector2D(X, Y) * Radius * 0.45f;
					Draw({C + FVector2D(-Radius * 0.25f), C + FVector2D(Radius * 0.25f, -Radius * 0.25f),
						C + FVector2D(Radius * 0.25f), C + FVector2D(-Radius * 0.25f, Radius * 0.25f)}, true);
				}
				break;
			case EAPSMenuGlyph::Favorite:
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
			case EAPSMenuGlyph::Recent:
				Circle(Center, Radius, Radius);
				Draw({Center, Center + FVector2D(0.0f, -Radius * 0.58f)});
				Draw({Center, Center + FVector2D(Radius * 0.48f, Radius * 0.28f)});
				break;
			case EAPSMenuGlyph::Pilot:
				Circle(Center + FVector2D(0.0f, -Radius * 0.48f), Radius * 0.32f, Radius * 0.32f);
				Draw({Center + FVector2D(-Radius * 0.68f, Radius * 0.82f),
					Center + FVector2D(-Radius * 0.42f, Radius * 0.10f),
					Center + FVector2D(0.0f, -Radius * 0.02f),
					Center + FVector2D(Radius * 0.42f, Radius * 0.10f),
					Center + FVector2D(Radius * 0.68f, Radius * 0.82f)}, false);
				break;
			case EAPSMenuGlyph::Ship:
				Draw({Center + FVector2D(0.0f, -Radius),
					Center + FVector2D(Radius * 0.62f, Radius * 0.78f), Center,
					Center + FVector2D(-Radius * 0.62f, Radius * 0.78f)}, true);
				Draw({Center + FVector2D(-Radius * 0.24f, Radius * 0.66f),
					Center + FVector2D(-Radius * 0.38f, Radius),
					Center + FVector2D(0.0f, Radius * 0.78f),
					Center + FVector2D(Radius * 0.38f, Radius)});
				break;
			case EAPSMenuGlyph::Station:
				Circle(Center, Radius * 0.38f, Radius * 0.38f);
				Circle(Center, Radius, Radius * 0.38f, -0.22f);
				Draw({Center + FVector2D(0.0f, -Radius), Center + FVector2D(0.0f, Radius)});
				break;
			case EAPSMenuGlyph::Headquarters:
				Draw({Center + FVector2D(0.0f, -Radius), Center + FVector2D(Radius * 0.82f, -Radius * 0.35f),
					Center + FVector2D(Radius * 0.68f, Radius * 0.82f), Center + FVector2D(-Radius * 0.68f, Radius * 0.82f),
					Center + FVector2D(-Radius * 0.82f, -Radius * 0.35f)}, true);
				Draw({Center + FVector2D(-Radius * 0.34f, Radius * 0.82f), Center + FVector2D(-Radius * 0.34f, 0.0f),
					Center + FVector2D(Radius * 0.34f, 0.0f), Center + FVector2D(Radius * 0.34f, Radius * 0.82f)});
				break;
			case EAPSMenuGlyph::Shipyard:
				Draw({Center + FVector2D(-Radius, -Radius), Center + FVector2D(-Radius, Radius),
					Center + FVector2D(-Radius * 0.55f, Radius), Center + FVector2D(-Radius * 0.55f, -Radius * 0.55f),
					Center + FVector2D(Radius * 0.55f, -Radius * 0.55f), Center + FVector2D(Radius * 0.55f, Radius),
					Center + FVector2D(Radius, Radius), Center + FVector2D(Radius, -Radius)});
				Draw({Center + FVector2D(0.0f, -Radius * 0.82f), Center + FVector2D(Radius * 0.28f, Radius * 0.28f),
					Center, Center + Radius * FVector2D(-0.28f, 0.28f)}, true, 0.8f);
				break;
			case EAPSMenuGlyph::Fleet:
				for (int32 Index = -1; Index <= 1; ++Index)
				{
					const FVector2D C = Center + FVector2D(Index * Radius * 0.62f, FMath::Abs(Index) * Radius * 0.32f);
					Draw({C + FVector2D(0.0f, -Radius * 0.58f), C + FVector2D(Radius * 0.28f, Radius * 0.42f),
						C + FVector2D(0.0f, Radius * 0.22f), C + FVector2D(-Radius * 0.28f, Radius * 0.42f)}, true, 0.75f);
				}
				break;
			case EAPSMenuGlyph::Infrastructure:
				Circle(Center, Radius * 0.22f, Radius * 0.22f);
				for (int32 Index = 0; Index < 4; ++Index)
				{
					const float Angle = UE_TWO_PI * static_cast<float>(Index) / 4.0f + UE_PI * 0.25f;
					const FVector2D Direction(FMath::Cos(Angle), FMath::Sin(Angle));
					Draw({Center + Direction * Radius * 0.22f, Center + Direction * Radius * 0.85f});
					Circle(Center + Direction * Radius * 0.85f, Radius * 0.15f, Radius * 0.15f);
				}
				break;
			case EAPSMenuGlyph::Divisions:
				for (int32 Row = -1; Row <= 1; ++Row)
				{
					const float Y = Center.Y + Row * Radius * 0.62f;
					Circle(FVector2D(Center.X - Radius * 0.72f, Y), Radius * 0.12f, Radius * 0.12f);
					Draw({FVector2D(Center.X - Radius * 0.42f, Y), FVector2D(Center.X + Radius, Y)});
				}
				break;
			case EAPSMenuGlyph::System:
				Circle(Center, Radius * 0.20f, Radius * 0.20f);
				Circle(Center, Radius * 0.72f, Radius * 0.46f, -0.28f);
				Circle(Center + FVector2D(Radius * 0.68f, -Radius * 0.23f), Radius * 0.12f, Radius * 0.12f);
				break;
			}
			return LayerId;
		}

	private:
		EAPSMenuGlyph Glyph{EAPSMenuGlyph::Compass};
		FLinearColor Color{FLinearColor::White};
		float StrokeWidth{1.35f};
	};

	class SChamferedFrame final : public SLeafWidget
	{
	public:
		SLATE_BEGIN_ARGS(SChamferedFrame) {}
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
			const float Cut = FMath::Clamp(FMath::Min(Size.X, Size.Y) * 0.025f, 8.0f, 18.0f);
			const TArray<FVector2D> Points = {
				FVector2D(Cut, 0.5f), FVector2D(Size.X - Cut, 0.5f),
				FVector2D(Size.X - 0.5f, Cut), FVector2D(Size.X - 0.5f, Size.Y - Cut),
				FVector2D(Size.X - Cut, Size.Y - 0.5f), FVector2D(Cut, Size.Y - 0.5f),
				FVector2D(0.5f, Size.Y - Cut), FVector2D(0.5f, Cut), FVector2D(Cut, 0.5f)
			};
			FSlateDrawElement::MakeLines(OutDrawElements, LayerId, Geometry.ToPaintGeometry(),
				Points, ESlateDrawEffect::None, Color.Get(FLinearColor::White), true, Thickness);
			return LayerId;
		}

	private:
		TAttribute<FLinearColor> Color{FLinearColor::White};
		float Thickness{1.0f};
	};

	/** A real filled chamfered polygon. Unlike a rectangular SBorder with a
	 * chamfer outline on top, this leaves the cut corners genuinely empty. */
	class SChamferedSurface final : public SLeafWidget
	{
	public:
		SLATE_BEGIN_ARGS(SChamferedSurface) {}
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

			const float Cut = FMath::Clamp(FMath::Min(Size.X, Size.Y) * 0.025f, 8.0f, 18.0f);
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
				Vertices, Indices, nullptr, 0, 0, ESlateDrawEffect::PreMultipliedAlpha);
			return LayerId;
		}

	private:
		const FSlateBrush* Brush{nullptr};
		TAttribute<FLinearColor> Tint{FLinearColor::White};
		bool bChamferTop{true};
		bool bChamferBottom{true};
	};

	/** Animated, code-native mission diagram used by the Choose Your Path cards.
	 * It deliberately has no texture/resource dependency: the card remains crisp
	 * at every resolution and its meaning is carried by geometry, motion and data
	 * rather than by an unrelated screenshot. */
	class SPathCardVisual final : public SLeafWidget
	{
	public:
		SLATE_BEGIN_ARGS(SPathCardVisual) {}
			SLATE_ARGUMENT(EAPSPathVisual, Visual)
			SLATE_ATTRIBUTE(FLinearColor, Accent)
			SLATE_ATTRIBUTE(bool, Hovered)
			SLATE_ARGUMENT(bool, Enabled)
		SLATE_END_ARGS()

		void Construct(const FArguments& InArgs)
		{
			Visual = InArgs._Visual;
			Accent = InArgs._Accent;
			Hovered = InArgs._Hovered;
			bEnabled = InArgs._Enabled;
			SetVisibility(EVisibility::HitTestInvisible);
			RegisterActiveTimer(1.0f / 30.0f,
				FWidgetActiveTimerDelegate::CreateSP(this, &SPathCardVisual::Animate));
		}

		virtual FVector2D ComputeDesiredSize(float) const override
		{
			return FVector2D::ZeroVector;
		}

		virtual int32 OnPaint(const FPaintArgs&, const FGeometry& Geometry,
			const FSlateRect&, FSlateWindowElementList& OutDrawElements, int32 LayerId,
			const FWidgetStyle&, bool) const override
		{
			const FVector2D Size = Geometry.GetLocalSize();
			if (Size.X < 12.0f || Size.Y < 12.0f)
			{
				return LayerId;
			}

			const bool bCardHovered = bEnabled && Hovered.Get(false);
			FLinearColor Main = Accent.Get(FLinearColor(0.12f, 0.82f, 1.0f));
			if (!bEnabled)
			{
				Main = FLinearColor(0.30f, 0.40f, 0.46f, 1.0f);
			}
			const float Energy = bCardHovered ? 1.0f : 0.58f;
			const float Pulse = 0.5f + 0.5f * FMath::Sin(AnimationSeconds * (bCardHovered ? 2.8f : 1.35f));
			const FVector2D Center(Size.X * 0.5f, Size.Y * 0.43f);
			const float Unit = FMath::Min(Size.X, Size.Y);

			const auto DrawLine = [&](const TArray<FVector2D>& Points, const FLinearColor& Color,
				float Width = 1.0f, int32 LayerOffset = 0, bool bAntialias = true)
			{
				if (Points.Num() >= 2)
				{
					FSlateDrawElement::MakeLines(OutDrawElements, LayerId + LayerOffset,
						Geometry.ToPaintGeometry(), Points, ESlateDrawEffect::None,
						Color, bAntialias, Width);
				}
			};
			const auto DrawCircle = [&](const FVector2D& C, float RadiusX, float RadiusY,
				const FLinearColor& Color, float Width = 1.0f, float Rotation = 0.0f, int32 Segments = 48)
			{
				TArray<FVector2D> Points;
				Points.Reserve(Segments + 1);
				const float CosR = FMath::Cos(Rotation);
				const float SinR = FMath::Sin(Rotation);
				for (int32 Index = 0; Index <= Segments; ++Index)
				{
					const float Angle = UE_TWO_PI * static_cast<float>(Index) / Segments;
					const FVector2D P(FMath::Cos(Angle) * RadiusX, FMath::Sin(Angle) * RadiusY);
					Points.Add(C + FVector2D(P.X * CosR - P.Y * SinR, P.X * SinR + P.Y * CosR));
				}
				DrawLine(Points, Color, Width, 2);
			};
			const auto DrawDot = [&](const FVector2D& P, float Radius, const FLinearColor& Color,
				int32 LayerOffset = 3)
			{
				FSlateDrawElement::MakeBox(OutDrawElements, LayerId + LayerOffset,
					Geometry.ToPaintGeometry(FVector2D(Radius * 2.0f),
						FSlateLayoutTransform(P - FVector2D(Radius))),
					FAppStyle::GetBrush("WhiteBrush"), ESlateDrawEffect::None, Color);
			};
			const auto DrawBracket = [&](const FVector2D& C, float Radius, const FLinearColor& Color)
			{
				const float Arm = Radius * 0.42f;
				for (int32 XSign : {-1, 1})
				{
					for (int32 YSign : {-1, 1})
					{
						const FVector2D Corner = C + FVector2D(XSign * Radius, YSign * Radius);
						DrawLine({Corner, Corner - FVector2D(XSign * Arm, 0.0f)}, Color, 1.25f, 3);
						DrawLine({Corner, Corner - FVector2D(0.0f, YSign * Arm)}, Color, 1.25f, 3);
					}
				}
			};

			// Deep technical plate and sparse coordinate grid.  The alternating bands
			// approximate a soft vignette without a bitmap or post-process material.
			for (int32 Band = 0; Band < 6; ++Band)
			{
				const float Alpha = 0.80f - static_cast<float>(Band) * 0.075f;
				const float Inset = static_cast<float>(Band) * 7.0f;
				if (Size.X <= Inset * 2.0f + 1.0f || Size.Y <= Inset * 2.0f + 1.0f)
				{
					continue;
				}
				FSlateDrawElement::MakeBox(OutDrawElements, LayerId,
					Geometry.ToPaintGeometry(Size - FVector2D(Inset * 2.0f),
						FSlateLayoutTransform(FVector2D(Inset))),
					FAppStyle::GetBrush("WhiteBrush"), ESlateDrawEffect::None,
					FLinearColor(0.001f, 0.009f + 0.002f * Band, 0.018f + 0.003f * Band, Alpha));
			}
			const FLinearColor Grid(Main.R, Main.G, Main.B, 0.045f + (bCardHovered ? 0.025f : 0.0f));
			for (float X = 24.0f; X < Size.X; X += 36.0f)
			{
				DrawLine({FVector2D(X, 0.0f), FVector2D(X, Size.Y)}, Grid, 0.55f, 1, false);
			}
			for (float Y = 20.0f; Y < Size.Y; Y += 32.0f)
			{
				DrawLine({FVector2D(0.0f, Y), FVector2D(Size.X, Y)}, Grid, 0.55f, 1, false);
			}

			const FLinearColor Trace(Main.R, Main.G, Main.B, 0.34f + Energy * 0.36f);
			const FLinearColor Soft(Main.R, Main.G, Main.B, 0.10f + Energy * 0.13f);
			const FLinearColor Bright(
				FMath::Lerp(Main.R, 1.0f, 0.34f), FMath::Lerp(Main.G, 1.0f, 0.34f),
				FMath::Lerp(Main.B, 1.0f, 0.34f), 0.74f + 0.22f * Pulse);

			switch (Visual)
			{
			case EAPSPathVisual::LiveSystem:
				{
					const float Radius = Unit * 0.115f;
					DrawCircle(Center, Radius * 0.45f, Radius * 0.45f, Soft, 7.0f);
					DrawCircle(Center, Radius * 0.28f, Radius * 0.28f, Bright, 2.0f);
					DrawDot(Center, Radius * 0.17f, Bright);
					for (int32 Orbit = 1; Orbit <= 5; ++Orbit)
					{
						const float OrbitRadius = Radius * (0.80f + Orbit * 0.47f);
						DrawCircle(Center, OrbitRadius, OrbitRadius * 0.38f, Soft, Orbit == 3 ? 1.2f : 0.75f, -0.08f);
						const float Angle = AnimationSeconds * (0.23f + Orbit * 0.045f) + Orbit * 1.27f;
						const FVector2D P = Center + FVector2D(FMath::Cos(Angle) * OrbitRadius,
							FMath::Sin(Angle) * OrbitRadius * 0.38f);
						DrawDot(P, Orbit == 3 ? 3.2f : 1.8f, Orbit == 3 ? Bright : Trace);
						if (Orbit == 3) DrawBracket(P, 9.0f + 2.0f * Pulse, Bright);
					}
					DrawLine({Center + FVector2D(-Radius * 3.4f, 0.0f), Center + FVector2D(Radius * 3.4f, 0.0f)}, Soft, 0.65f, 1);
					DrawLine({Center + FVector2D(0.0f, -Radius * 1.65f), Center + FVector2D(0.0f, Radius * 1.65f)}, Soft, 0.65f, 1);
				}
				break;
			case EAPSPathVisual::WorldArchive:
				{
					const float Radius = Unit * 0.19f;
					const FVector2D C = Center + FVector2D(0.0f, Unit * 0.015f);
					DrawCircle(C, Radius * 1.08f + Pulse * 2.0f, Radius * 1.08f + Pulse * 2.0f, Soft, 3.0f);
					DrawCircle(C, Radius, Radius, Trace, 1.6f);
					for (int32 Latitude = -2; Latitude <= 2; ++Latitude)
					{
						const float Y = Latitude * Radius * 0.30f;
						const float Width = FMath::Sqrt(FMath::Max(0.0f, Radius * Radius - Y * Y));
						DrawCircle(C + FVector2D(0.0f, Y), Width, Radius * 0.085f, Soft, 0.75f);
					}
					DrawCircle(C, Radius * 0.42f, Radius, Soft, 0.9f);
					DrawCircle(C, Radius * 1.48f, Radius * 0.28f, Trace, 1.0f, -0.28f);
					const float MoonAngle = AnimationSeconds * 0.34f;
					DrawDot(C + FVector2D(FMath::Cos(MoonAngle) * Radius * 1.48f,
						FMath::Sin(MoonAngle) * Radius * 0.28f), 2.8f, Bright);
					DrawBracket(C, Radius * 1.24f, FLinearColor(Main.R, Main.G, Main.B, 0.42f));
				}
				break;
			case EAPSPathVisual::CivilizationNetwork:
				{
					const float Radius = Unit * 0.205f;
					DrawCircle(Center, Radius, Radius, Soft, 1.0f);
					DrawCircle(Center, Radius * 0.76f, Radius * 0.24f, Soft, 0.8f);
					DrawCircle(Center, Radius * 0.42f, Radius, Soft, 0.8f);
					TArray<FVector2D> Nodes;
					for (int32 Index = 0; Index < 9; ++Index)
					{
						const float Angle = Index * 2.399963f + 0.14f * FMath::Sin(AnimationSeconds * 0.45f + Index);
						const float R = Radius * (0.24f + 0.072f * Index);
						Nodes.Add(Center + FVector2D(FMath::Cos(Angle) * R, FMath::Sin(Angle) * R * 0.74f));
					}
					for (int32 Index = 0; Index < Nodes.Num(); ++Index)
					{
						DrawLine({Nodes[Index], Nodes[(Index + 3) % Nodes.Num()]}, Soft, 0.8f, 2);
						DrawDot(Nodes[Index], Index % 3 == 0 ? 3.0f : 1.8f,
							Index == FMath::FloorToInt(AnimationSeconds * 1.4f) % Nodes.Num() ? Bright : Trace);
					}
					DrawDot(Center, 4.0f + Pulse * 1.2f, Bright);
				}
				break;
			case EAPSPathVisual::GalaxySynthesis:
				{
					const float Radius = Unit * 0.225f;
					DrawCircle(Center, Radius * 1.15f, Radius * 1.15f, Soft, 0.8f);
					for (int32 Arm = 0; Arm < 3; ++Arm)
					{
						TArray<FVector2D> Spiral;
						for (int32 Index = 0; Index < 42; ++Index)
						{
							const float Alpha = static_cast<float>(Index) / 41.0f;
							const float Angle = Arm * UE_TWO_PI / 3.0f + Alpha * UE_TWO_PI * 1.65f + AnimationSeconds * 0.055f;
							Spiral.Add(Center + FVector2D(FMath::Cos(Angle), FMath::Sin(Angle) * 0.55f) * Radius * Alpha);
							if (Index > 8 && Index % 7 == 0)
							{
								DrawDot(Spiral.Last(), 1.0f + (Index % 3) * 0.45f, Index % 2 ? Trace : Bright);
							}
						}
						DrawLine(Spiral, Trace, 1.1f, 2);
					}
					DrawCircle(Center, Radius * 0.13f, Radius * 0.09f, Bright, 4.0f);
					DrawDot(Center, 3.0f + Pulse * 1.5f, Bright);
				}
				break;
			case EAPSPathVisual::PlanetLaboratory:
				{
					const float Radius = Unit * 0.205f;
					DrawCircle(Center, Radius + Pulse * 1.5f, Radius + Pulse * 1.5f, Trace, 1.5f);
					for (int32 Latitude = -2; Latitude <= 2; ++Latitude)
					{
						const float Y = Latitude * Radius * 0.30f;
						const float Width = FMath::Sqrt(FMath::Max(0.0f, Radius * Radius - Y * Y));
						DrawCircle(Center + FVector2D(0.0f, Y), Width, Radius * 0.075f, Soft, 0.72f);
					}
					for (int32 Longitude = 0; Longitude <= 2; ++Longitude)
					{
						DrawCircle(Center, Radius * (0.20f + 0.19f * Longitude), Radius,
							Soft, 0.72f);
					}
					const float ScanY = Center.Y - Radius + FMath::Fmod(AnimationSeconds * Radius * 0.38f, Radius * 2.0f);
					const float Span = FMath::Sqrt(FMath::Max(0.0f, Radius * Radius - FMath::Square(ScanY - Center.Y)));
					DrawLine({FVector2D(Center.X - Span, ScanY), FVector2D(Center.X + Span, ScanY)}, Bright, 1.3f, 3);
					DrawBracket(Center, Radius * 1.22f, Trace);
				}
				break;
			case EAPSPathVisual::StoryArchive:
				{
					const float Radius = Unit * 0.19f;
					TArray<FVector2D> Nodes;
					for (int32 Index = 0; Index < 8; ++Index)
					{
						const float Angle = Index * 0.91f + (Index % 2) * 0.37f;
						const float R = Radius * (0.35f + 0.08f * Index);
						Nodes.Add(Center + FVector2D(FMath::Cos(Angle) * R, FMath::Sin(Angle) * R * 0.72f));
					}
					for (int32 Index = 1; Index < Nodes.Num(); ++Index)
					{
						DrawLine({Nodes[Index - 1], Nodes[Index]}, Soft, 0.85f, 2);
					}
					for (const FVector2D& Node : Nodes) DrawDot(Node, 1.7f, Trace);
					DrawCircle(Center + FVector2D(0.0f, -Radius * 0.13f), Radius * 0.32f, Radius * 0.38f, Trace, 1.2f);
					DrawLine({Center + FVector2D(-Radius * 0.42f, -Radius * 0.02f), Center + FVector2D(Radius * 0.42f, -Radius * 0.02f),
						Center + FVector2D(Radius * 0.42f, Radius * 0.52f), Center + FVector2D(-Radius * 0.42f, Radius * 0.52f),
						Center + FVector2D(-Radius * 0.42f, -Radius * 0.02f)}, Trace, 1.4f, 3);
				}
				break;
			}

			// Persistent reticle makes every tile read as an interactive instrument.
			DrawLine({FVector2D(18.0f, 18.0f), FVector2D(54.0f, 18.0f)}, Soft, 1.0f, 3);
			DrawLine({FVector2D(18.0f, 18.0f), FVector2D(18.0f, 34.0f)}, Soft, 1.0f, 3);
			DrawLine({FVector2D(Size.X - 54.0f, 18.0f), FVector2D(Size.X - 18.0f, 18.0f)}, Soft, 1.0f, 3);
			DrawLine({FVector2D(Size.X - 18.0f, 18.0f), FVector2D(Size.X - 18.0f, 34.0f)}, Soft, 1.0f, 3);
			return LayerId + 4;
		}

	private:
		EActiveTimerReturnType Animate(double, float DeltaTime)
		{
			AnimationSeconds = FMath::Fmod(AnimationSeconds + DeltaTime, 4096.0f);
			Invalidate(EInvalidateWidgetReason::Paint);
			return EActiveTimerReturnType::Continue;
		}

		EAPSPathVisual Visual{EAPSPathVisual::LiveSystem};
		TAttribute<FLinearColor> Accent{FLinearColor::White};
		TAttribute<bool> Hovered{false};
		bool bEnabled{true};
		float AnimationSeconds{0.0f};
	};
}

namespace APSMenu
{
	// Landing and Choose Path sit over the real generated astronomical scene.
	// Keep only a faint readability veil here; the old turquoise wash hid every
	// small star in the live background.
	const FLinearColor Background(0.001f, 0.004f, 0.009f, 0.07f);
	const FLinearColor Panel(0.002f, 0.012f, 0.022f, 0.94f);
	const FLinearColor PanelSoft(0.004f, 0.025f, 0.040f, 0.90f);
	const FLinearColor Cyan(0.12f, 0.82f, 1.0f, 1.0f);
	const FLinearColor CyanDim(0.035f, 0.23f, 0.32f, 1.0f);
	const FLinearColor Amber(1.0f, 0.55f, 0.04f, 1.0f);
	const FLinearColor White(0.92f, 0.97f, 1.0f, 1.0f);
	const FLinearColor Muted(0.48f, 0.62f, 0.70f, 1.0f);
	const FLinearColor Success(0.35f, 0.94f, 0.78f, 1.0f);
	const FLinearColor CivPanel(0.003f, 0.015f, 0.027f, 0.975f);
	const FLinearColor CivRaised(0.008f, 0.034f, 0.052f, 0.985f);
	const FLinearColor CivControl(0.004f, 0.023f, 0.038f, 0.99f);
	TWeakObjectPtr<UFont> DisplayFont;
	TWeakObjectPtr<UFont> BodyFont;
	const FSlateRoundedBoxBrush PanelBrush(Panel, 10.0f, CyanDim, 1.0f);
	const FSlateRoundedBoxBrush PanelSoftBrush(PanelSoft, 9.0f, CyanDim, 1.0f);
	const FSlateRoundedBoxBrush InsetBrush(FLinearColor(0.001f, 0.012f, 0.022f, 0.96f), 6.0f, FLinearColor(0.04f, 0.22f, 0.31f, 1.0f), 1.0f);
	const FSlateRoundedBoxBrush CyanBadgeBrush(FLinearColor(0.01f, 0.07f, 0.10f, 0.98f), 18.0f, Cyan, 1.25f);
	const FSlateRoundedBoxBrush AmberPanelBrush(FLinearColor(0.11f, 0.045f, 0.002f, 0.96f), 9.0f, Amber, 1.4f);
	const FSlateRoundedBoxBrush CivCardBrush(CivRaised, 9.0f, FLinearColor(0.045f, 0.30f, 0.40f, 0.95f), 1.0f);
	const FSlateRoundedBoxBrush CivControlBrush(CivControl, 6.0f, FLinearColor(0.025f, 0.18f, 0.25f, 0.95f), 1.0f);
	const FSlateRoundedBoxBrush CivMetricBrush(FLinearColor(0.005f, 0.028f, 0.044f, 0.98f), 6.0f,
		FLinearColor(0.035f, 0.23f, 0.31f, 0.88f), 1.0f);
	const FSlateRoundedBoxBrush CivStatusBrush(FLinearColor(0.004f, 0.04f, 0.06f, 0.96f), 5.0f,
		FLinearColor(0.06f, 0.45f, 0.57f, 0.95f), 1.0f);

	TSharedRef<SWidget> ChamferPanel(TSharedRef<SWidget> Content, const FMargin& Padding,
		const FLinearColor& Accent = CyanDim, float Thickness = 1.0f)
	{
		return SNew(SOverlay)
			+ SOverlay::Slot()
			[
				SNew(SChamferedSurface)
				.Brush(FAppStyle::GetBrush("WhiteBrush"))
				.Tint(Panel)
				.ChamferTop(true)
				.ChamferBottom(true)
			]
			+ SOverlay::Slot().Padding(Padding)
			[
				Content
			]
			+ SOverlay::Slot()
			[
				SNew(SChamferedFrame).Color(Accent).Thickness(Thickness)
			];
	}

	FSlateFontInfo Font(const FName Typeface, int32 Size)
	{
		if (UFont* FontObject = (Typeface == TEXT("Bold") ? DisplayFont.Get() : BodyFont.Get()))
		{
			return FSlateFontInfo(FontObject, Size, Typeface);
		}
		return FCoreStyle::GetDefaultFontStyle(Typeface, Size);
	}

	TSharedRef<SWidget> Badge(const FText& Glyph, const FLinearColor& Accent, float Size = 34.0f)
	{
		return SNew(SBox).WidthOverride(Size).HeightOverride(Size)
		[
			SNew(SBorder).BorderImage(&CyanBadgeBrush).BorderBackgroundColor(Accent).Padding(1.0f)
			[
				SNew(SBorder).BorderImage(&InsetBrush).Padding(0.0f)
				[
					SNew(STextBlock).Text(Glyph).Justification(ETextJustify::Center)
					.Font(Font("Bold", FMath::RoundToInt(Size * 0.34f))).ColorAndOpacity(Accent)
				]
			]
		];
	}

	TSharedRef<SWidget> IconBadge(EAPSMenuGlyph Glyph, const FLinearColor& Accent, float Size = 34.0f)
	{
		return SNew(SBox).WidthOverride(Size).HeightOverride(Size)
		[
			SNew(SBorder).BorderImage(&CyanBadgeBrush).BorderBackgroundColor(Accent).Padding(1.0f)
			[
				SNew(SBorder).BorderImage(&InsetBrush).Padding(FMath::Max(4.0f, Size * 0.19f))
				[
					SNew(SVectorMenuGlyph).Glyph(Glyph).Color(Accent).StrokeWidth(Size >= 40.0f ? 1.7f : 1.35f)
				]
			]
		];
	}

	TSharedRef<SWidget> SectionHeading(const FText& Glyph, const FText& Title)
	{
		return SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight()
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)[Badge(Glyph, Cyan, 30.0f)]
			+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center).Padding(10.0f, 0.0f)
			[SNew(STextBlock).Text(Title).Font(Font("Bold", 13)).ColorAndOpacity(Cyan)]
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 8.0f, 0.0f, 0.0f)
		[SNew(SBox).HeightOverride(1.0f)[SNew(SBorder).BorderImage(FAppStyle::GetBrush("WhiteBrush")).BorderBackgroundColor(CyanDim)]];
	}

	TSharedRef<SWidget> IconSectionHeading(EAPSMenuGlyph Glyph, const FText& Title,
		const FText& Subtitle = FText::GetEmpty())
	{
		return SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight()
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
			[IconBadge(Glyph, Cyan, 34.0f)]
			+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center).Padding(11.0f, 0.0f)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight()
				[SNew(STextBlock).Text(Title).Font(Font("Bold", 14)).ColorAndOpacity(White)]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 2.0f, 0.0f, 0.0f)
				[
					SNew(STextBlock).Text(Subtitle).Font(Font("Regular", 9)).ColorAndOpacity(Muted)
					.Visibility(Subtitle.IsEmpty() ? EVisibility::Collapsed : EVisibility::Visible)
				]
			]
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 9.0f, 0.0f, 0.0f)
		[SNew(SBox).HeightOverride(1.0f)[SNew(SBorder).BorderImage(FAppStyle::GetBrush("WhiteBrush")).BorderBackgroundColor(CyanDim)]];
	}

	FText SaveSizeText(int64 Bytes)
	{
		const double MB = static_cast<double>(Bytes) / (1024.0 * 1024.0);
		return FText::FromString(MB >= 1.0
			? FString::Printf(TEXT("%.1f MB"), MB)
			: FString::Printf(TEXT("%.0f KB"), static_cast<double>(Bytes) / 1024.0));
	}

	bool LoadWorldMetadataSidecar(const FString& MetadataPath, FAPSExistingWorldEntry& Entry)
	{
		if (!IFileManager::Get().FileExists(*MetadataPath))
		{
			return false;
		}

		FConfigFile Metadata;
		Metadata.Read(MetadataPath);
		int64 Version = 0;
		if (!Metadata.GetInt64(TEXT("APSWorld"), TEXT("Version"), Version) || Version < 1)
		{
			return false;
		}

		Metadata.GetString(TEXT("APSWorld"), TEXT("DisplayName"), Entry.DisplayName);
		Metadata.GetString(TEXT("APSWorld"), TEXT("SystemType"), Entry.SystemType);
		Metadata.GetString(TEXT("APSWorld"), TEXT("StarType"), Entry.StarType);
		Metadata.GetString(TEXT("APSWorld"), TEXT("PlanetType"), Entry.PlanetType);
		Entry.Habitability = TEXT("UNKNOWN");
		Metadata.GetString(TEXT("APSWorld"), TEXT("Habitability"), Entry.Habitability);
		Metadata.GetString(TEXT("APSWorld"), TEXT("Environment"), Entry.Environment);
		int64 Value = 0;
		if (Metadata.GetInt64(TEXT("APSWorld"), TEXT("TotalPlanets"), Value))
		{
			Entry.TotalPlanets = static_cast<int32>(FMath::Clamp<int64>(Value, 0, MAX_int32));
		}
		if (Metadata.GetInt64(TEXT("APSWorld"), TEXT("InhabitedPlanets"), Value))
		{
			Entry.InhabitedPlanets = static_cast<int32>(FMath::Clamp<int64>(Value, 0, MAX_int32));
		}
		Entry.bMetadataLoaded = true;
		return true;
	}

	void WriteWorldMetadataSidecar(const FString& MetadataPath, const FAPSExistingWorldEntry& Entry)
	{
		FConfigFile Metadata;
		Metadata.SetInt64(TEXT("APSWorld"), TEXT("Version"), 1);
		Metadata.SetString(TEXT("APSWorld"), TEXT("DisplayName"), *Entry.DisplayName);
		Metadata.SetString(TEXT("APSWorld"), TEXT("SystemType"), *Entry.SystemType);
		Metadata.SetString(TEXT("APSWorld"), TEXT("StarType"), *Entry.StarType);
		Metadata.SetString(TEXT("APSWorld"), TEXT("PlanetType"), *Entry.PlanetType);
		Metadata.SetString(TEXT("APSWorld"), TEXT("Habitability"), *Entry.Habitability);
		Metadata.SetString(TEXT("APSWorld"), TEXT("Environment"), *Entry.Environment);
		Metadata.SetInt64(TEXT("APSWorld"), TEXT("TotalPlanets"), Entry.TotalPlanets);
		Metadata.SetInt64(TEXT("APSWorld"), TEXT("InhabitedPlanets"), Entry.InhabitedPlanets);
		IFileManager::Get().MakeDirectory(*FPaths::GetPath(MetadataPath), true);
		if (!Metadata.Write(MetadataPath, false))
		{
			UE_LOG(LogTemp, Warning, TEXT("[APS.Menu] Could not cache save metadata: %s"),
				*MetadataPath);
		}
	}

	FString BrowserPreferencesPath()
	{
		return FPaths::ProjectSavedDir() / TEXT("Config") / TEXT("APSWorldBrowser.ini");
	}

	void LoadFavoriteSlots(TSet<FString>& OutSlots)
	{
		FConfigFile Preferences;
		Preferences.Read(BrowserPreferencesPath());
		TArray<FString> Slots;
		Preferences.GetArray(TEXT("WorldBrowser"), TEXT("FavoriteSlots"), Slots);
		for (const FString& Slot : Slots)
		{
			OutSlots.Add(Slot);
		}
	}

	void SaveFavoriteSlots(const TArray<TSharedPtr<FAPSExistingWorldEntry>>& Entries)
	{
		TArray<FString> Slots;
		for (const TSharedPtr<FAPSExistingWorldEntry>& Entry : Entries)
		{
			if (Entry.IsValid() && Entry->bFavorite)
			{
				Slots.Add(Entry->SaveFileName);
			}
		}
		FConfigFile Preferences;
		Preferences.SetArray(TEXT("WorldBrowser"), TEXT("FavoriteSlots"), Slots);
		const FString PreferencesPath = BrowserPreferencesPath();
		IFileManager::Get().MakeDirectory(*FPaths::GetPath(PreferencesPath), true);
		if (!Preferences.Write(PreferencesPath, false))
		{
			UE_LOG(LogTemp, Warning, TEXT("[APS.Menu] Could not persist world favorites"));
		}
	}

	template <typename T>
	FString EnumLabel(T Value)
	{
		const UEnum* Enum = StaticEnum<T>();
		return Enum ? Enum->GetDisplayNameTextByValue(static_cast<int64>(Value)).ToString() : TEXT("UNKNOWN");
	}
}

SAPSMainMenuRoot::SAPSMainMenuRoot()
{
	PrimaryButtonStyle = FButtonStyle()
		.SetNormal(FSlateRoundedBoxBrush(FLinearColor(0.18f, 0.08f, 0.005f, 0.94f), 8.0f, APSMenu::Amber, 1.5f))
		.SetHovered(FSlateRoundedBoxBrush(FLinearColor(0.42f, 0.18f, 0.01f, 0.98f), 8.0f, FLinearColor(1.0f, 0.76f, 0.18f), 2.0f))
		.SetPressed(FSlateRoundedBoxBrush(FLinearColor(0.62f, 0.26f, 0.01f, 1.0f), 8.0f, APSMenu::Amber, 2.0f))
		.SetNormalPadding(FMargin(2.0f)).SetPressedPadding(FMargin(2.0f, 3.0f, 2.0f, 1.0f));
	SecondaryButtonStyle = FButtonStyle()
		.SetNormal(FSlateRoundedBoxBrush(FLinearColor(0.01f, 0.04f, 0.065f, 0.92f), 7.0f, APSMenu::CyanDim, 1.0f))
		.SetHovered(FSlateRoundedBoxBrush(FLinearColor(0.015f, 0.10f, 0.15f, 0.96f), 7.0f, APSMenu::Cyan, 1.5f))
		.SetPressed(FSlateRoundedBoxBrush(FLinearColor(0.02f, 0.16f, 0.22f, 1.0f), 7.0f, APSMenu::Cyan, 1.5f));
	CardButtonStyle = SecondaryButtonStyle;
	DisabledCardButtonStyle = FButtonStyle()
		.SetNormal(FSlateRoundedBoxBrush(FLinearColor(0.015f, 0.025f, 0.035f, 0.82f), 8.0f, FLinearColor(0.20f, 0.25f, 0.28f), 1.0f))
		.SetHovered(FSlateRoundedBoxBrush(FLinearColor(0.015f, 0.025f, 0.035f, 0.82f), 8.0f, FLinearColor(0.20f, 0.25f, 0.28f), 1.0f));
	ScrollBarStyle = FAppStyle::Get().GetWidgetStyle<FScrollBarStyle>("ScrollBar");
}

void SAPSMainMenuRoot::Construct(const FArguments& InArgs)
{
	Controller = InArgs._Controller;
	ViewModel = InArgs._ViewModel;
	LoadVisualResources();
	BeginAuxiliaryMenuLoad();

	ChildSlot
	[
		SNew(SBorder)
		.BorderImage(FAppStyle::GetBrush("WhiteBrush"))
		.BorderBackgroundColor(APSMenu::Background)
		.Padding(0.0f)
		[
			SAssignNew(ContentHost, SBox)
		]
	];
	Navigate(EAPSMenuPage::Landing);
}

void SAPSMainMenuRoot::Navigate(EAPSMenuPage NewPage)
{
	if (bHasBuiltCurrentPage && CurrentPage == NewPage && ContentHost.IsValid())
	{
		return;
	}
	if (CurrentPage == EAPSMenuPage::AstronomicalGeneration
		&& NewPage != EAPSMenuPage::AstronomicalGeneration && ViewModel.IsValid())
	{
		// A slider can leave a debounced regeneration queued for 0.2 seconds.
		// Any route leaving this page must freeze the accepted live scene. Otherwise
		// a queued edit can rebuild it and move the camera underneath Civilization
		// or the newly visible menu page.
		ViewModel->CancelPendingPreview();
	}
	if (CurrentPage == EAPSMenuPage::ExistingWorlds && NewPage != EAPSMenuPage::ExistingWorlds)
	{
		if (AMainMenuController* PC = Controller.Get())
		{
			PC->CancelWorldMetadataLoad();
		}
	}
	PreviousPage = CurrentPage;
	CurrentPage = NewPage;
	if (!ContentHost.IsValid()) return;
	if (CurrentPage != EAPSMenuPage::AstronomicalGeneration)
	{
		WorldGenerationPanel.Reset();
	}
	if ((CurrentPage == EAPSMenuPage::Landing || CurrentPage == EAPSMenuPage::ChoosePath)
		&& ViewModel.IsValid())
	{
		bool bRestoredAstronomicalRoute = false;
		if (ViewModel->GetGenerationRoute() == EAPSGenerationRoute::Planet)
		{
			// The dedicated PLANET route intentionally owns no galaxy/system. Rebuild
			// the normal space hierarchy before it becomes the live menu background.
			ViewModel->SetGenerationRoute(EAPSGenerationRoute::Space);
			bRestoredAstronomicalRoute = true;
		}
		ViewModel->SetPreviewFocus(EAstroPreviewFocus::Galaxy);
		// The clean menu map intentionally has no authored planet/station backdrop.
		// Start the same bounded procedural hierarchy used by the editor so Landing
		// and Choose Path share a real, live astronomical background.
		if (!ViewModel->bPreviewReady && !bRestoredAstronomicalRoute)
		{
			ViewModel->RequestPreview();
		}
	}

	switch (CurrentPage)
	{
	case EAPSMenuPage::Landing: ContentHost->SetContent(BuildLandingPage()); break;
	case EAPSMenuPage::ChoosePath: ContentHost->SetContent(BuildChoosePathPage()); break;
	case EAPSMenuPage::ExistingWorlds: ContentHost->SetContent(BuildExistingWorldsPage()); break;
	case EAPSMenuPage::AstronomicalGeneration:
		ContentHost->SetContent(
			SAssignNew(WorldGenerationPanel, SWorldGenerationPanel)
			.ViewModel(ViewModel)
			.OnBack(FSimpleDelegate::CreateSP(this, &SAPSMainMenuRoot::Navigate, EAPSMenuPage::ChoosePath))
			.OnContinue(FSimpleDelegate::CreateSP(this, &SAPSMainMenuRoot::ContinueAstronomicalGeneration)));
		break;
	case EAPSMenuPage::Civilization: ContentHost->SetContent(BuildCivilizationPage()); break;
	case EAPSMenuPage::Profile: ContentHost->SetContent(BuildProfilePage()); break;
	case EAPSMenuPage::Settings: ContentHost->SetContent(BuildSettingsPage()); break;
	}
	bHasBuiltCurrentPage = true;
	UE_LOG(LogTemp, Log, TEXT("[APS.Menu] Slate page built page=%d"), static_cast<int32>(CurrentPage));
}

TSharedRef<SWidget> SAPSMainMenuRoot::BuildHeader(const FText& SectionTitle, bool bShowBack)
{
	return SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
		[
			SNew(SBox).WidthOverride(190.0f).HeightOverride(58.0f)
			[
				SNew(SButton)
				.Visibility(bShowBack ? EVisibility::Visible : EVisibility::Collapsed)
				.ButtonStyle(&SecondaryButtonStyle)
				.OnClicked(this, &SAPSMainMenuRoot::Back)
				.ContentPadding(FMargin(22.0f, 13.0f))
				[
					SNew(STextBlock).Text(LOCTEXT("Back", "<   BACK")).Font(APSMenu::Font("Bold", 15)).ColorAndOpacity(APSMenu::White)
				]
			]
		]
		+ SHorizontalBox::Slot().FillWidth(1.0f).HAlign(HAlign_Center).VAlign(VAlign_Center)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
			[
				SNew(STextBlock).Text(LOCTEXT("Brand", "A P O S F E R A")).Font(APSMenu::Font("Bold", 58)).ColorAndOpacity(APSMenu::White)
			]
			+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0.0f, 2.0f)
			[
				SNew(STextBlock).Text(LOCTEXT("SubBrand", "S P A C E T R I P S   G E N E R A T I O N")).Font(APSMenu::Font("Regular", 15)).ColorAndOpacity(APSMenu::White)
			]
			+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0.0f, 8.0f, 0.0f, 0.0f)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
				[SNew(SBox).WidthOverride(92.0f).HeightOverride(1.0f)[SNew(SBorder).BorderImage(FAppStyle::GetBrush("WhiteBrush")).BorderBackgroundColor(APSMenu::Cyan)]]
				+ SHorizontalBox::Slot().AutoWidth().Padding(18.0f, 0.0f)
				[SNew(STextBlock).Text(SectionTitle).Font(APSMenu::Font("Bold", 18)).ColorAndOpacity(APSMenu::Cyan)]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
				[SNew(SBox).WidthOverride(92.0f).HeightOverride(1.0f)[SNew(SBorder).BorderImage(FAppStyle::GetBrush("WhiteBrush")).BorderBackgroundColor(APSMenu::Cyan)]]
			]
		]
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
		[
			SNew(SBox).WidthOverride(190.0f)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().Padding(5.0f)
				[
					SNew(SButton).ButtonStyle(&SecondaryButtonStyle).ContentPadding(4.0f)
					.OnClicked(this, &SAPSMainMenuRoot::OpenProfile).ToolTipText(LOCTEXT("ProfileTip", "PLAYER PROFILE"))
					[APSMenu::Badge(LOCTEXT("ProfileGlyph", "ID"), APSMenu::Cyan, 38.0f)]
				]
				+ SHorizontalBox::Slot().AutoWidth().Padding(5.0f)
				[
					SNew(SButton).ButtonStyle(&SecondaryButtonStyle).ContentPadding(4.0f)
					.OnClicked(this, &SAPSMainMenuRoot::OpenSettings).ToolTipText(LOCTEXT("SettingsTip", "SETTINGS"))
					[APSMenu::Badge(LOCTEXT("SettingsGlyph", "CFG"), APSMenu::Cyan, 38.0f)]
				]
			]
		];
}

TSharedRef<SWidget> SAPSMainMenuRoot::BuildLandingPage()
{
	auto MenuEntry = [this](const FText& Label, const FOnClicked& Action,
		bool bEnabled = true, bool bPrimary = false, int32 FontSize = 16)
	{
		return SNew(SBox).WidthOverride(210.0f).HeightOverride(bPrimary ? 50.0f : 40.0f)
		[
			SNew(SButton)
			.IsEnabled(bEnabled)
			.ButtonStyle(bPrimary ? &PrimaryButtonStyle : &SecondaryButtonStyle)
			.OnClicked(Action)
			.ContentPadding(FMargin(18.0f, 8.0f))
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.0f, 0.0f, 12.0f, 0.0f)
				[
					SNew(STextBlock).Text(FText::FromString(bEnabled ? TEXT(">") : TEXT("-")))
					.Font(APSMenu::Font("Bold", FontSize)).ColorAndOpacity(bEnabled ? (bPrimary ? APSMenu::Amber : APSMenu::Cyan) : APSMenu::Muted)
				]
				+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center)
				[
					SNew(STextBlock).Text(Label).Font(APSMenu::Font(bPrimary ? "Bold" : "Regular", FontSize))
					.ColorAndOpacity(bEnabled ? APSMenu::White : APSMenu::Muted)
				]
			]
		];
	};

	return SNew(SOverlay)
		+ SOverlay::Slot().HAlign(HAlign_Left).VAlign(VAlign_Center).Padding(112.0f, 40.0f)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 50.0f)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight()
				[SNew(STextBlock).Text(LOCTEXT("LandingTitle", "APOSFERA")).Font(APSMenu::Font("Bold", 44)).ColorAndOpacity(APSMenu::White)]
				+ SVerticalBox::Slot().AutoHeight().Padding(2.0f, 2.0f, 0.0f, 0.0f)
				[SNew(STextBlock).Text(LOCTEXT("LandingSubtitle", "S P A C E T R I P S")).Font(APSMenu::Font("Regular", 11)).ColorAndOpacity(APSMenu::Cyan)]
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 5.0f)
			[MenuEntry(LOCTEXT("SingleGame", "SINGLE GAME"), FOnClicked::CreateSP(this, &SAPSMainMenuRoot::OpenChoosePath), true, true, 18)]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 6.0f)
			[MenuEntry(LOCTEXT("Multiplayer", "MULTIPLAYER GAME"), FOnClicked(), false)]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 6.0f)
			[MenuEntry(LOCTEXT("Profile", "PROFILE"), FOnClicked::CreateSP(this, &SAPSMainMenuRoot::OpenProfile))]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 20.0f, 0.0f, 10.0f)
			[SNew(SBox).WidthOverride(210.0f).HeightOverride(1.0f)[SNew(SBorder).BorderImage(FAppStyle::GetBrush("WhiteBrush")).BorderBackgroundColor(APSMenu::CyanDim)]]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 5.0f)
			[MenuEntry(LOCTEXT("Settings", "SETTINGS"), FOnClicked::CreateSP(this, &SAPSMainMenuRoot::OpenSettings), true, false, 14)]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 6.0f)
			[MenuEntry(LOCTEXT("Quit", "QUIT"), FOnClicked::CreateSP(this, &SAPSMainMenuRoot::QuitGame), true, false, 14)]
		];
}

TSharedRef<SWidget> SAPSMainMenuRoot::BuildAuxiliaryPage(const FText& SectionTitle,
	TSoftClassPtr<UUserWidget>& WidgetClass, TWeakObjectPtr<UUserWidget>& WidgetInstance,
	const FText& LoadingText)
{
	UUserWidget* RuntimeWidget = WidgetInstance.Get();
	if (!RuntimeWidget && WidgetClass.Get())
	{
		if (AMainMenuController* PC = Controller.Get())
		{
			RuntimeWidget = CreateWidget<UUserWidget>(PC, WidgetClass.Get());
			if (RuntimeWidget)
			{
				WidgetInstance = RuntimeWidget;
				PC->HoldSlateResource(RuntimeWidget);
			}
		}
	}

	TSharedRef<SWidget> Body = RuntimeWidget
		? RuntimeWidget->TakeWidget()
		: StaticCastSharedRef<SWidget>(
			SNew(SBorder).BorderImage(&APSMenu::PanelBrush).Padding(32.0f)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
				[APSMenu::Badge(LOCTEXT("LoadingAuxGlyph", "..."), APSMenu::Cyan, 48.0f)]
				+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0.0f, 16.0f)
				[SNew(STextBlock).Text(LoadingText).Font(APSMenu::Font("Regular", 16)).ColorAndOpacity(APSMenu::Muted)]
			]
		);

	return SNew(SOverlay)
		+ SOverlay::Slot()
		[SNew(SScaleBox).Stretch(EStretch::ScaleToFill)[SNew(SImage).Image(&BackgroundImage).ColorAndOpacity(FLinearColor(0.10f, 0.20f, 0.30f, 0.36f))]]
		+ SOverlay::Slot()
		[SNew(SBorder).BorderImage(FAppStyle::GetBrush("WhiteBrush")).BorderBackgroundColor(FLinearColor(0.0f, 0.008f, 0.016f, 0.78f))]
		+ SOverlay::Slot().Padding(24.0f)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()[BuildHeader(SectionTitle)]
			+ SVerticalBox::Slot().FillHeight(1.0f).Padding(0.0f, 18.0f, 0.0f, 0.0f)
			[
				SNew(SBorder).BorderImage(&APSMenu::PanelBrush).Padding(10.0f)[Body]
			]
		];
}

TSharedRef<SWidget> SAPSMainMenuRoot::BuildProfilePage()
{
	return BuildAuxiliaryPage(LOCTEXT("ProfileTitle", "PLAYER PROFILE"),
		ProfilePanelClass, ProfilePanelInstance,
		LOCTEXT("ProfileLoading", "LOADING PLAYER PROFILE..."));
}

TSharedRef<SWidget> SAPSMainMenuRoot::BuildSettingsPage()
{
	return BuildAuxiliaryPage(LOCTEXT("SettingsTitle", "SETTINGS"),
		SettingsPanelClass, SettingsPanelInstance,
		LOCTEXT("SettingsLoading", "LOADING SETTINGS..."));
}

TSharedRef<SWidget> SAPSMainMenuRoot::BuildPathCard(const FText& Title, const FText& Description,
	EAPSPathVisual Visual, const FLinearColor& Accent, FSimpleDelegate Action, bool bLarge, bool bEnabled)
{
	EAPSMenuGlyph Glyph = EAPSMenuGlyph::Compass;
	FText RouteCode;
	FText StateLabel;
	switch (Visual)
	{
	case EAPSPathVisual::LiveSystem:
		Glyph = EAPSMenuGlyph::Compass;
		RouteCode = LOCTEXT("PathRouteLive", "01 / LIVE SYSTEM");
		StateLabel = LOCTEXT("PathStateLaunch", "LAUNCH READY");
		break;
	case EAPSPathVisual::WorldArchive:
		Glyph = EAPSMenuGlyph::World;
		RouteCode = LOCTEXT("PathRouteArchive", "02 / WORLD ARCHIVE");
		StateLabel = LOCTEXT("PathStateBrowse", "BROWSE SAVES");
		break;
	case EAPSPathVisual::CivilizationNetwork:
		Glyph = EAPSMenuGlyph::Civilization;
		RouteCode = LOCTEXT("PathRouteCivilization", "03 / CIVILIZATION LAB");
		StateLabel = LOCTEXT("PathStateSynthesis", "SYNTHESIS READY");
		break;
	case EAPSPathVisual::GalaxySynthesis:
		Glyph = EAPSMenuGlyph::Space;
		RouteCode = LOCTEXT("PathRouteGalaxy", "04 / DEEP SPACE");
		StateLabel = LOCTEXT("PathStateGenerate", "GENERATOR READY");
		break;
	case EAPSPathVisual::PlanetLaboratory:
		Glyph = EAPSMenuGlyph::Planet;
		RouteCode = LOCTEXT("PathRoutePlanet", "05 / PLANET LAB");
		StateLabel = LOCTEXT("PathStateDesign", "DESIGN READY");
		break;
	case EAPSPathVisual::StoryArchive:
		Glyph = EAPSMenuGlyph::Lock;
		RouteCode = LOCTEXT("PathRouteStory", "06 / STORY ARCHIVE");
		StateLabel = LOCTEXT("PathStateLocked", "LOCKED");
		break;
	}
	TSharedRef<SButton> CardButton = SNew(SButton)
		.IsEnabled(bEnabled)
		.IsFocusable(bEnabled)
		.ButtonStyle(&FAppStyle::Get().GetWidgetStyle<FButtonStyle>("NoBorder"))
		.OnClicked_Lambda([Action]() mutable { Action.ExecuteIfBound(); return FReply::Handled(); })
		.ContentPadding(0.0f);
	const TWeakPtr<SButton> WeakCardButton = CardButton;
#if WITH_DEV_AUTOMATION_TESTS
	ChoosePathCardButtons.Add(CardButton);
	++ChoosePathProceduralVisualCount;
#endif
	CardButton->SetContent(
			SNew(SOverlay)
			+ SOverlay::Slot()
			[
				SNew(SChamferedSurface)
				.Brush(FAppStyle::GetBrush("WhiteBrush"))
				.Tint_Lambda([WeakCardButton, Accent, bEnabled]()
				{
					if (!bEnabled)
					{
						return FLinearColor(0.003f, 0.009f, 0.014f, 0.96f);
					}
					const TSharedPtr<SButton> Button = WeakCardButton.Pin();
					const bool bActive = Button.IsValid()
						&& (Button->IsHovered() || Button->HasKeyboardFocus());
					return bActive
						? FLinearColor(Accent.R * 0.055f, Accent.G * 0.055f, Accent.B * 0.055f, 0.98f)
						: FLinearColor(0.001f, 0.007f, 0.014f, 0.97f);
				})
				.ChamferTop(true)
				.ChamferBottom(true)
			]
			+ SOverlay::Slot()
			[
				SNew(SPathCardVisual)
				.Visual(Visual)
				.Accent(Accent)
				.Hovered_Lambda([WeakCardButton]()
				{
					const TSharedPtr<SButton> Button = WeakCardButton.Pin();
					return Button.IsValid() && (Button->IsHovered() || Button->HasKeyboardFocus());
				})
				.Enabled(bEnabled)
			]
			+ SOverlay::Slot().HAlign(HAlign_Left).VAlign(VAlign_Top)
				.Padding(FMargin(bLarge ? 25.0f : 18.0f, bLarge ? 23.0f : 17.0f))
			[
				SNew(SBorder)
				.BorderImage(&APSMenu::InsetBrush)
				.Padding(FMargin(10.0f, 5.0f))
				[
					SNew(STextBlock)
					.Text(RouteCode)
					.Font(APSMenu::Font("Bold", bLarge ? 10 : 8))
					.ColorAndOpacity(bEnabled ? Accent : APSMenu::Muted)
				]
			]
			+ SOverlay::Slot().HAlign(HAlign_Right).VAlign(VAlign_Top)
				.Padding(FMargin(bLarge ? 25.0f : 18.0f, bLarge ? 23.0f : 17.0f))
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.0f, 0.0f, 7.0f, 0.0f)
				[
					SNew(SBox).WidthOverride(5.0f).HeightOverride(5.0f)
					[
						SNew(SBorder).BorderImage(FAppStyle::GetBrush("WhiteBrush"))
						.BorderBackgroundColor(bEnabled ? Accent : APSMenu::Muted)
					]
				]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
				[
					SNew(STextBlock)
					.Text(StateLabel)
					.Font(APSMenu::Font("Bold", bLarge ? 9 : 7))
					.ColorAndOpacity(bEnabled ? APSMenu::White : APSMenu::Muted)
				]
			]
			+ SOverlay::Slot()
			[
				SNew(SChamferedSurface)
				.Brush(FAppStyle::GetBrush("WhiteBrush"))
				.Tint_Lambda([WeakCardButton, Accent, bEnabled]()
				{
					const TSharedPtr<SButton> Button = WeakCardButton.Pin();
					if (!bEnabled || !Button.IsValid()
						|| (!Button->IsHovered() && !Button->HasKeyboardFocus()))
					{
						return FLinearColor::Transparent;
					}
					return FLinearColor(Accent.R, Accent.G, Accent.B, 0.075f);
				})
				.ChamferTop(true)
				.ChamferBottom(true)
			]
			+ SOverlay::Slot().VAlign(VAlign_Bottom)
			[
				// Every compact card owns the same full-width information rail. Keeping
				// this geometry identical prevents Story/Generate cards from looking
				// like unrelated debug panels when their descriptions differ.
				SNew(SBox).HeightOverride(bLarge ? 210.0f : 132.0f)
				[
				SNew(SOverlay)
				+ SOverlay::Slot()
				[
					SNew(SChamferedSurface)
					.Brush(FAppStyle::GetBrush("WhiteBrush"))
					.Tint(FLinearColor(0.001f, 0.010f, 0.018f, 0.975f))
					.ChamferTop(false)
					.ChamferBottom(true)
				]
				+ SOverlay::Slot().VAlign(VAlign_Top)
				[
					SNew(SBox).HeightOverride(2.0f)
					[
						SNew(SBorder).BorderImage(FAppStyle::GetBrush("WhiteBrush"))
						.BorderBackgroundColor(bEnabled ? Accent : APSMenu::Muted)
					]
				]
				+ SOverlay::Slot().Padding(FMargin(bLarge ? 30.0f : 19.0f, bLarge ? 21.0f : 13.0f))
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight()
					[
						SNew(SBox).HeightOverride(bLarge ? 48.0f : 38.0f)
						[
							SNew(SOverlay)
							+ SOverlay::Slot().HAlign(HAlign_Left).VAlign(VAlign_Center)
							[
								SNew(SHorizontalBox)
								+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
								[APSMenu::IconBadge(Glyph, bEnabled ? Accent : APSMenu::Muted, bLarge ? 44.0f : 34.0f)]
								+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(12.0f, 0.0f)
								[
									SNew(STextBlock).Text(Title).Justification(ETextJustify::Left)
									.Font(APSMenu::Font("Bold", bLarge ? 28 : 17))
									.ColorAndOpacity_Lambda([WeakCardButton, bEnabled, Accent]()
									{
										if (!bEnabled) return APSMenu::Muted;
										const TSharedPtr<SButton> Button = WeakCardButton.Pin();
										return Button.IsValid() && (Button->IsHovered() || Button->HasKeyboardFocus())
											? Accent : APSMenu::White;
									})
								]
							]
							+ SOverlay::Slot().HAlign(HAlign_Right).VAlign(VAlign_Center)
							[
								SNew(STextBlock).Text(FText::FromString(bEnabled ? TEXT(">") : TEXT("LOCK")))
								.Font(APSMenu::Font("Bold", bEnabled ? 22 : 9))
								.ColorAndOpacity(bEnabled ? Accent : APSMenu::Muted)
							]
						]
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 7.0f, 0.0f, 0.0f)
					[
						SNew(SBox).HeightOverride(bLarge ? 42.0f : 34.0f)
						[
							SNew(STextBlock).Text(Description).AutoWrapText(true)
							.Justification(ETextJustify::Left)
							.Font(APSMenu::Font("Regular", bLarge ? 15 : 12))
							.ColorAndOpacity(bEnabled
								? FLinearColor(0.68f, 0.79f, 0.85f, 1.0f) : APSMenu::Muted)
						]
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, bLarge ? 18.0f : 0.0f, 0.0f, 0.0f)
					[
						SNew(SBorder).Visibility(bLarge ? EVisibility::Visible : EVisibility::Collapsed)
						.BorderImage(&APSMenu::AmberPanelBrush).Padding(FMargin(18.0f, 11.0f))
						[
							SNew(SHorizontalBox)
							+ SHorizontalBox::Slot().FillWidth(1.0f)
							[SNew(STextBlock).Text(LOCTEXT("LaunchGame", "LAUNCH GAME")).Font(APSMenu::Font("Bold", 15)).ColorAndOpacity(APSMenu::White)]
							+ SHorizontalBox::Slot().AutoWidth()
							[SNew(STextBlock).Text(FText::FromString(TEXT(">"))).Font(APSMenu::Font("Bold", 18)).ColorAndOpacity(APSMenu::Amber)]
						]
					]
				]
				]
			]
			+ SOverlay::Slot()
			[
				SNew(SChamferedFrame)
				.Color_Lambda([WeakCardButton, Accent, bEnabled]()
				{
					if (!bEnabled)
					{
						return APSMenu::Muted;
					}
					const TSharedPtr<SButton> Button = WeakCardButton.Pin();
					return Button.IsValid() && (Button->IsHovered() || Button->HasKeyboardFocus())
						? FLinearColor::White
						: Accent;
				})
				.Thickness(bLarge ? 1.8f : 1.2f)
			]
			+ SOverlay::Slot()
			[
				// A broad translucent trace reads as a restrained holographic glow
				// and makes hover unambiguous without moving or resizing the card.
				SNew(SChamferedFrame)
				.Color_Lambda([WeakCardButton, Accent, bEnabled]()
				{
					const TSharedPtr<SButton> Button = WeakCardButton.Pin();
					return bEnabled && Button.IsValid()
						&& (Button->IsHovered() || Button->HasKeyboardFocus())
						? FLinearColor(Accent.R, Accent.G, Accent.B, 0.48f)
						: FLinearColor::Transparent;
				})
				.Thickness(bLarge ? 4.5f : 3.5f)
			]
		);
	return CardButton;
}

TSharedRef<SWidget> SAPSMainMenuRoot::BuildChoosePathPage()
{
#if WITH_DEV_AUTOMATION_TESTS
	ChoosePathCardButtons.Reset();
	ChoosePathProceduralVisualCount = 0;
	ChoosePathStaticTextureResourceCount = 0;
#endif
	TSharedRef<SWidget> Foreground = SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().Padding(30.0f, 20.0f, 30.0f, 10.0f)[BuildHeader(LOCTEXT("ChoosePath", "CHOOSE YOUR PATH"))]
		+ SVerticalBox::Slot().FillHeight(1.0f).Padding(58.0f, 18.0f, 58.0f, 46.0f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(0.31f).Padding(7.0f)
			[
				BuildPathCard(LOCTEXT("StartGame", "START SINGLE GAME"),
					LOCTEXT("StartGameDesc", "Begin a new journey through the live full-scale world."), EAPSPathVisual::LiveSystem,
					APSMenu::Amber, FSimpleDelegate::CreateLambda([this]() { StartSingleGame(); }), true)
			]
			+ SHorizontalBox::Slot().FillWidth(0.69f).Padding(7.0f)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().FillHeight(0.5f).Padding(0.0f, 0.0f, 0.0f, 6.0f)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(6.0f, 0.0f)
					[BuildPathCard(LOCTEXT("VisitWorld", "VISIT EXISTING WORLD"), LOCTEXT("VisitDesc", "Explore worlds you have already created."), EAPSPathVisual::WorldArchive, APSMenu::Cyan, FSimpleDelegate::CreateLambda([this]() { OpenExistingWorlds(); }))]
					+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(6.0f, 0.0f)
					[BuildPathCard(LOCTEXT("GenCiv", "GENERATE CIVILIZATION"), LOCTEXT("GenCivDesc", "Create a civilization and shape its astronomical home."), EAPSPathVisual::CivilizationNetwork, APSMenu::Cyan, FSimpleDelegate::CreateLambda([this]() { OpenAstronomicalGeneration(EAstroPreviewFocus::HomePlanet, EAPSGenerationRoute::Civilization); }))]
					+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(6.0f, 0.0f)
					[BuildPathCard(LOCTEXT("GenSpace", "GENERATE SPACE"), LOCTEXT("GenSpaceDesc", "Procedurally generate stellar systems and clusters."), EAPSPathVisual::GalaxySynthesis, APSMenu::Cyan, FSimpleDelegate::CreateLambda([this]() { OpenAstronomicalGeneration(EAstroPreviewFocus::StarCluster, EAPSGenerationRoute::Space); }))]
				]
				+ SVerticalBox::Slot().FillHeight(0.5f).Padding(0.0f, 6.0f, 0.0f, 0.0f)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(6.0f, 0.0f)
					[BuildPathCard(LOCTEXT("CreatePlanet", "CREATE PLANET"), LOCTEXT("PlanetDesc", "Design a planet with atmosphere, terrain and moons."), EAPSPathVisual::PlanetLaboratory, APSMenu::Cyan, FSimpleDelegate::CreateLambda([this]() { OpenAstronomicalGeneration(EAstroPreviewFocus::HomePlanet, EAPSGenerationRoute::Planet); }))]
					+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(6.0f, 0.0f)
					[BuildPathCard(LOCTEXT("Story", "STORY MODE"), LOCTEXT("StoryDesc", "Unravel the deeper story of Aposfera.  COMING SOON"), EAPSPathVisual::StoryArchive, APSMenu::Muted, FSimpleDelegate(), false, false)]
				]
			]
		];

	return SNew(SOverlay)
		+ SOverlay::Slot()
		[SNew(SBorder).BorderImage(FAppStyle::GetBrush("WhiteBrush")).BorderBackgroundColor(FLinearColor(0.0f, 0.004f, 0.010f, 0.16f))]
		+ SOverlay::Slot()[Foreground];
}

TSharedRef<SWidget> SAPSMainMenuRoot::BuildExistingWorldsPage()
{
	LoadExistingWorlds();
	ExistingWorldPage = 0;

	const auto NavRow = [this](const FText& Label, EAPSWorldCollection Collection, TFunction<int32()> CountGetter)
	{
		const FText Glyph = FText::FromString(Label.ToString().Left(1));
		return SNew(SButton)
			.ButtonStyle(&SecondaryButtonStyle)
			.ButtonColorAndOpacity_Lambda([this, Collection]()
			{
				return WorldCollection == Collection
					? FLinearColor(0.72f, 0.32f, 0.03f, 1.0f) : FLinearColor::White;
			})
			.ContentPadding(FMargin(14.0f, 11.0f))
			.OnClicked(this, &SAPSMainMenuRoot::SetWorldCollection, Collection)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.0f, 0.0f, 10.0f, 0.0f)
				[APSMenu::Badge(Glyph, WorldCollection == Collection ? APSMenu::Amber : APSMenu::Cyan, 26.0f)]
				+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center)
				[SNew(STextBlock).Text(Label).Font(APSMenu::Font("Bold", 13)).ColorAndOpacity_Lambda([this, Collection](){ return WorldCollection == Collection ? APSMenu::Amber : APSMenu::White; })]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
				[SNew(STextBlock).Text_Lambda([CountGetter](){ return FText::AsNumber(CountGetter()); }).Font(APSMenu::Font("Bold", 12)).ColorAndOpacity(APSMenu::Muted)]
			];
	};

	const auto FilterRow = [this](const FText& Label, EAPSWorldFilterKind Kind)
	{
		return SNew(SButton).ButtonStyle(&FAppStyle::Get().GetWidgetStyle<FButtonStyle>("NoBorder"))
			.ContentPadding(FMargin(2.0f, 5.0f)).OnClicked(this, &SAPSMainMenuRoot::CycleWorldFilter, Kind)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().FillWidth(1.0f)[SNew(STextBlock).Text(Label).Font(APSMenu::Font("Regular", 10)).ColorAndOpacity(APSMenu::Muted)]
				+ SHorizontalBox::Slot().AutoWidth()[SNew(STextBlock).Text_Lambda([this, Kind](){ return GetWorldFilterLabel(Kind); }).Font(APSMenu::Font("Bold", 10)).ColorAndOpacity(APSMenu::Cyan)]
			];
	};

	TSharedRef<SWidget> Page = SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().Padding(30.0f, 18.0f, 30.0f, 8.0f)[BuildHeader(LOCTEXT("VisitExisting", "VISIT EXISTING WORLD"))]
		+ SVerticalBox::Slot().FillHeight(1.0f).Padding(24.0f, 8.0f, 24.0f, 24.0f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(0.17f).Padding(5.0f)
			[
				SNew(SOverlay)
				+ SOverlay::Slot()
				[
					SNew(SBorder).BorderImage(&APSMenu::PanelBrush).Padding(14.0f)
					[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 5.0f)[NavRow(LOCTEXT("AllWorlds", "ALL WORLDS"), EAPSWorldCollection::All, [this](){ return ExistingWorlds.Num(); })]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 5.0f)[NavRow(LOCTEXT("MyWorlds", "MY WORLDS"), EAPSWorldCollection::MyWorlds, [this](){ return ExistingWorlds.Num(); })]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 5.0f)[NavRow(LOCTEXT("Favorites", "FAVORITES"), EAPSWorldCollection::Favorites, [this](){ int32 Count=0; for(const TSharedPtr<FAPSExistingWorldEntry>& E:ExistingWorlds){ if(E.IsValid()&&E->bFavorite){++Count;} } return Count; })]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 5.0f)[NavRow(LOCTEXT("Recent", "RECENT"), EAPSWorldCollection::Recent, [this](){ return FMath::Min(6, ExistingWorlds.Num()); })]
					+ SVerticalBox::Slot().AutoHeight().Padding(4.0f, 20.0f, 4.0f, 10.0f)[SNew(STextBlock).Text(LOCTEXT("Filters", "FILTERS")).Font(APSMenu::Font("Bold", 11)).ColorAndOpacity(APSMenu::Muted)]
					+ SVerticalBox::Slot().AutoHeight().Padding(4.0f, 5.0f)[FilterRow(LOCTEXT("StarTypeFilter", "STAR TYPE"), EAPSWorldFilterKind::StarType)]
					+ SVerticalBox::Slot().AutoHeight().Padding(4.0f, 5.0f)[FilterRow(LOCTEXT("WorldTypeFilter", "WORLD TYPE"), EAPSWorldFilterKind::WorldType)]
					+ SVerticalBox::Slot().AutoHeight().Padding(4.0f, 5.0f)[FilterRow(LOCTEXT("InhabitedFilter", "INHABITED"), EAPSWorldFilterKind::Inhabited)]
					+ SVerticalBox::Slot().AutoHeight().Padding(4.0f, 5.0f)[FilterRow(LOCTEXT("EnvironmentFilter", "ENVIRONMENT"), EAPSWorldFilterKind::Environment)]
					+ SVerticalBox::Slot().FillHeight(1.0f)
					+ SVerticalBox::Slot().AutoHeight()[SNew(SButton).IsEnabled(false).ToolTipText(LOCTEXT("ImportSaveHint", "Place .sav files in Saved/SaveGames; the browser discovers them without blocking.")).ButtonStyle(&SecondaryButtonStyle).ContentPadding(FMargin(15.0f, 12.0f))[SNew(STextBlock).Text(LOCTEXT("ImportSave", "IMPORT SAVE")).Justification(ETextJustify::Center).Font(APSMenu::Font("Bold", 12)).ColorAndOpacity(APSMenu::Muted)]]
					]
				]
				+ SOverlay::Slot()
				[
					SNew(SChamferedFrame).Color(APSMenu::Cyan).Thickness(1.15f)
				]
			]
			+ SHorizontalBox::Slot().FillWidth(0.62f).Padding(5.0f)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 8.0f)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().FillWidth(1.0f)[SNew(SSearchBox).HintText(LOCTEXT("SearchWorlds", "Search worlds...")).OnTextChanged(this, &SAPSMainMenuRoot::OnWorldSearchChanged)]
					+ SHorizontalBox::Slot().AutoWidth().Padding(8.0f, 0.0f)[SNew(SButton).ButtonStyle(&SecondaryButtonStyle).OnClicked(this, &SAPSMainMenuRoot::CycleWorldSort)[SNew(STextBlock).Text_Lambda([this](){ return GetWorldSortLabel(); }).Font(APSMenu::Font("Bold", 10)).ColorAndOpacity(APSMenu::White)]]
					+ SHorizontalBox::Slot().AutoWidth().Padding(2.0f, 0.0f)[SNew(SButton).ButtonStyle(&SecondaryButtonStyle).OnClicked(this, &SAPSMainMenuRoot::ToggleWorldView)[SNew(STextBlock).Text_Lambda([this](){ return FText::FromString(bCompactWorldList ? TEXT("LIST") : TEXT("GRID")); }).Font(APSMenu::Font("Bold", 10)).ColorAndOpacity(APSMenu::Cyan)]]
				]
				+ SVerticalBox::Slot().FillHeight(1.0f)
				[
					APSMenu::ChamferPanel(SAssignNew(ExistingWorldGridHost, SBox), FMargin(10.0f))
				]
				+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0.0f, 8.0f, 0.0f, 0.0f)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().AutoWidth().Padding(3.0f)[SNew(SBox).MinDesiredWidth(118.0f)[SNew(SButton).ButtonStyle(&SecondaryButtonStyle).ContentPadding(FMargin(12.0f, 7.0f)).OnClicked(this, &SAPSMainMenuRoot::ChangeExistingWorldPage, -1)[SNew(STextBlock).Text(LOCTEXT("PreviousPage", "<<  PREVIOUS")).Justification(ETextJustify::Center).Font(APSMenu::Font("Bold", 10)).ColorAndOpacity(APSMenu::Cyan)]]]
					+ SHorizontalBox::Slot().AutoWidth().Padding(12.0f, 7.0f)[SNew(STextBlock).Text_Lambda([this](){ return FText::AsNumber(ExistingWorldPage + 1); }).Font(APSMenu::Font("Bold", 12)).ColorAndOpacity(APSMenu::White)]
					+ SHorizontalBox::Slot().AutoWidth().Padding(3.0f)[SNew(SBox).MinDesiredWidth(118.0f)[SNew(SButton).ButtonStyle(&SecondaryButtonStyle).ContentPadding(FMargin(12.0f, 7.0f)).OnClicked(this, &SAPSMainMenuRoot::ChangeExistingWorldPage, 1)[SNew(STextBlock).Text(LOCTEXT("NextPage", "NEXT  >>")).Justification(ETextJustify::Center).Font(APSMenu::Font("Bold", 10)).ColorAndOpacity(APSMenu::Cyan)]]]
				]
			]
			+ SHorizontalBox::Slot().FillWidth(0.21f).Padding(5.0f)
			[
				APSMenu::ChamferPanel(SAssignNew(ExistingWorldDetailsHost, SBox), FMargin(16.0f))
			]
		];

	RebuildExistingWorldGrid();
	RebuildExistingWorldDetails();
	BeginExistingWorldMetadataLoad();
	return SNew(SOverlay)
		+ SOverlay::Slot()
		[SNew(SScaleBox).Stretch(EStretch::ScaleToFill)[SNew(SImage).Image(&BackgroundImage).ColorAndOpacity(FLinearColor(0.16f, 0.26f, 0.35f, 0.24f))]]
		+ SOverlay::Slot()
		[SNew(SBorder).BorderImage(FAppStyle::GetBrush("WhiteBrush")).BorderBackgroundColor(FLinearColor(0.0f, 0.010f, 0.020f, 0.52f))]
		+ SOverlay::Slot()[Page];
}

void SAPSMainMenuRoot::LoadExistingWorlds()
{
	TMap<FString, TSharedPtr<FAPSExistingWorldEntry>> CachedEntries;
	const FString PreviouslySelectedSlot = SelectedWorld.IsValid() ? SelectedWorld->SaveFileName : FString();
	for (const TSharedPtr<FAPSExistingWorldEntry>& ExistingEntry : ExistingWorlds)
	{
		if (ExistingEntry.IsValid())
		{
			CachedEntries.Add(ExistingEntry->SaveFileName, ExistingEntry);
		}
	}
	ExistingWorlds.Reset();
	TSet<FString> FavoriteSlots;
	APSMenu::LoadFavoriteSlots(FavoriteSlots);
	TArray<FString> SaveFiles;
	const FString SaveDirectory = FPaths::ProjectSavedDir() / TEXT("SaveGames");
	IFileManager::Get().FindFiles(SaveFiles, *SaveDirectory, TEXT("*.sav"));
	for (const FString& SaveFile : SaveFiles)
	{
		const FString SlotName = FPaths::GetBaseFilename(SaveFile);
		const FFileStatData Stat = IFileManager::Get().GetStatData(*(SaveDirectory / SaveFile));
		TSharedPtr<FAPSExistingWorldEntry> Entry = CachedEntries.FindRef(SlotName);
		const bool bCacheIsCurrent = Entry.IsValid()
			&& Entry->FileTimestamp == Stat.ModificationTime.ToUnixTimestamp()
			&& Entry->FileSizeBytes == Stat.FileSize;
		if (!Entry.IsValid())
		{
			Entry = MakeShared<FAPSExistingWorldEntry>();
		}
		Entry->SaveFileName = SlotName;
		if (!bCacheIsCurrent)
		{
			Entry->DisplayName = SlotName;
			Entry->SystemType = TEXT("FULL-SCALE STAR SYSTEM");
			Entry->StarType = TEXT("GENERATED STAR");
			Entry->PlanetType = TEXT("PERSISTENT WORLD");
			Entry->Habitability = TEXT("UNKNOWN");
			Entry->Environment = TEXT("Legacy world - details load on launch");
			Entry->TotalPlanets = 0;
			Entry->InhabitedPlanets = 0;
			Entry->bMetadataLoaded = false;
		}
		Entry->FileTimestamp = Stat.ModificationTime.ToUnixTimestamp();
		Entry->FileSizeBytes = Stat.FileSize;
		Entry->bFavorite = FavoriteSlots.Contains(SlotName);
		// Sidecars are tiny and contain only browser-facing fields. They let even a
		// 100 MB gameplay save render a complete card without deserializing actors.
		APSMenu::LoadWorldMetadataSidecar(SaveDirectory / (SlotName + TEXT(".apsmeta")), *Entry);
		ExistingWorlds.Add(Entry);
	}
	ExistingWorlds.Sort([](const auto& A, const auto& B) { return A->FileTimestamp > B->FileTimestamp; });
	const TSharedPtr<FAPSExistingWorldEntry>* RestoredSelection = ExistingWorlds.FindByPredicate(
		[&PreviouslySelectedSlot](const TSharedPtr<FAPSExistingWorldEntry>& Entry)
		{
			return Entry.IsValid() && Entry->SaveFileName == PreviouslySelectedSlot;
		});
	SelectedWorld = RestoredSelection ? *RestoredSelection : (ExistingWorlds.Num() > 0 ? ExistingWorlds[0] : nullptr);
}

void SAPSMainMenuRoot::BeginExistingWorldMetadataLoad()
{
	TArray<FString> LightweightSlots;
	for (const TSharedPtr<FAPSExistingWorldEntry>& Entry : ExistingWorlds)
	{
		// Old saves can exceed 100 MB and deserialize on the game thread even when
		// their file read is asynchronous. Keep the browser instant; large legacy
		// worlds remain launchable and receive metadata after a future lightweight save.
		if (Entry.IsValid() && !Entry->bMetadataLoaded
			&& Entry->FileSizeBytes <= 1ll * 1024ll * 1024ll)
		{
			LightweightSlots.Add(Entry->SaveFileName);
		}
	}
	if (AMainMenuController* PC = Controller.Get())
	{
		PC->LoadWorldMetadataAsync(LightweightSlots);
	}
}

void SAPSMainMenuRoot::ApplyExistingWorldMetadata(const FString& SlotName, const UGameSave* Save)
{
	if (!Save) return;
	const TSharedPtr<FAPSExistingWorldEntry>* Found = ExistingWorlds.FindByPredicate(
		[&SlotName](const TSharedPtr<FAPSExistingWorldEntry>& Entry)
		{
			return Entry.IsValid() && Entry->SaveFileName.Equals(SlotName, ESearchCase::IgnoreCase);
		});
	if (!Found || !Found->IsValid()) return;

	FAPSExistingWorldEntry& Entry = *Found->Get();
	Entry.DisplayName = Save->WorldName.IsEmpty() ? Entry.SaveFileName : Save->WorldName;
	Entry.InhabitedPlanets = Save->InhabitedPlanetsDataArray.Num();
	Entry.bMetadataLoaded = true;
	Entry.SystemType = TEXT("GENERATED WORLD");
	if (Save->GeneratedWorldsDataArray.Num() > 0)
	{
		const FGeneratedWorldData& Data = Save->GeneratedWorldsDataArray[0];
		Entry.SystemType = APSMenu::EnumLabel(Data.PlanetarySystemType);
		Entry.StarType = APSMenu::EnumLabel(Data.SpectralClass);
		Entry.PlanetType = APSMenu::EnumLabel(Data.PlanetType);
		Entry.Habitability = APSMenu::EnumLabel(Data.PlanetHabitability);
		Entry.Environment = FString::Printf(TEXT("%s / %.0f KM"), *Entry.PlanetType, Data.PlanetRadius);
		Entry.TotalPlanets = Data.PlanetsAmount;
	}
	APSMenu::WriteWorldMetadataSidecar(
		FPaths::ProjectSavedDir() / TEXT("SaveGames") / (SlotName + TEXT(".apsmeta")), Entry);
	RebuildExistingWorldGrid();
	if (SelectedWorld == *Found) RebuildExistingWorldDetails();
}

void SAPSMainMenuRoot::RebuildExistingWorldGrid()
{
	if (!ExistingWorldGridHost) return;
	TArray<TSharedPtr<FAPSExistingWorldEntry>> Filtered;
	for (const TSharedPtr<FAPSExistingWorldEntry>& Entry : ExistingWorlds)
	{
		if (Entry.IsValid()
			&& (WorldSearch.IsEmpty() || Entry->DisplayName.Contains(WorldSearch, ESearchCase::IgnoreCase))
			&& PassesExistingWorldFilters(*Entry))
		{
			Filtered.Add(Entry);
		}
	}
	if (WorldCollection == EAPSWorldCollection::Recent && Filtered.Num() > 6)
	{
		Filtered.SetNum(6, EAllowShrinking::No);
	}
	if (WorldSortMode == EAPSWorldSortMode::Name)
	{
		Filtered.Sort([](const auto& A, const auto& B){ return A->DisplayName < B->DisplayName; });
	}
	else if (WorldSortMode == EAPSWorldSortMode::SaveSize)
	{
		Filtered.Sort([](const auto& A, const auto& B){ return A->FileSizeBytes > B->FileSizeBytes; });
	}
	else
	{
		Filtered.Sort([](const auto& A, const auto& B){ return A->FileTimestamp > B->FileTimestamp; });
	}
	constexpr int32 ItemsPerPage = 6;
	const int32 MaxPage = FMath::Max(0, FMath::DivideAndRoundUp(Filtered.Num(), ItemsPerPage) - 1);
	ExistingWorldPage = FMath::Clamp(ExistingWorldPage, 0, MaxPage);

	TSharedRef<SUniformGridPanel> Grid = SNew(SUniformGridPanel).SlotPadding(FMargin(5.0f));
	const int32 FirstIndex = ExistingWorldPage * ItemsPerPage;
	const int32 LastIndex = FMath::Min(FirstIndex + ItemsPerPage, Filtered.Num());
	for (int32 SourceIndex = FirstIndex; SourceIndex < LastIndex; ++SourceIndex)
	{
		const int32 VisibleIndex = SourceIndex - FirstIndex;
		const TSharedPtr<FAPSExistingWorldEntry>& Entry = Filtered[SourceIndex];
		const uint32 StableImageIndex = GetTypeHash(Entry->SaveFileName) % 3u;
		const FSlateBrush* Image = StableImageIndex == 0 ? &PlanetImage : (StableImageIndex == 1 ? &GalaxyImage : &SystemImage);
		const bool bSelected = SelectedWorld == Entry;
		const FString LastPlayed = FDateTime::FromUnixTimestamp(Entry->FileTimestamp).ToString(TEXT("%Y-%m-%d  %H:%M"));
		const int32 ColumnCount = bCompactWorldList ? 1 : 3;
		Grid->AddSlot(VisibleIndex % ColumnCount, VisibleIndex / ColumnCount)
		[
			SNew(SButton).ButtonStyle(bSelected ? &PrimaryButtonStyle : &CardButtonStyle).OnClicked(this, &SAPSMainMenuRoot::SelectExistingWorld, Entry).ContentPadding(0.0f)
			[
				SNew(SOverlay)
				+ SOverlay::Slot()
				[
					SNew(SBox).HeightOverride(bCompactWorldList ? 112.0f : 190.0f).Clipping(EWidgetClipping::ClipToBounds)
					[SNew(SScaleBox).Stretch(EStretch::ScaleToFit)[SNew(SImage).Image(Image)]]
				]
				+ SOverlay::Slot().HAlign(HAlign_Right).VAlign(VAlign_Top).Padding(10.0f)
				[
					SNew(SButton).ButtonStyle(&FAppStyle::Get().GetWidgetStyle<FButtonStyle>("NoBorder"))
					.OnClicked(this, &SAPSMainMenuRoot::ToggleWorldFavorite, Entry).ContentPadding(2.0f)
					[SNew(STextBlock).Text(LOCTEXT("FavoriteStar", "*")).Font(APSMenu::Font("Bold", 21)).ColorAndOpacity(Entry->bFavorite ? APSMenu::Amber : APSMenu::White)]
				]
				+ SOverlay::Slot().VAlign(VAlign_Bottom)
				[
					SNew(SBorder).BorderImage(FAppStyle::GetBrush("WhiteBrush")).BorderBackgroundColor(FLinearColor(0.002f, 0.01f, 0.02f, 0.94f)).Padding(12.0f)
					[
						SNew(SVerticalBox)
						+ SVerticalBox::Slot().AutoHeight()[SNew(STextBlock).Text(FText::FromString(Entry->DisplayName)).Font(APSMenu::Font("Bold", 13)).ColorAndOpacity(APSMenu::White)]
						+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 3.0f)[SNew(STextBlock).Text(FText::FromString(Entry->SystemType)).Font(APSMenu::Font("Regular", 10)).ColorAndOpacity(APSMenu::Cyan)]
						+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 4.0f, 0.0f, 0.0f)[SNew(STextBlock).Text(FText::FromString(FString::Printf(TEXT("LAST PLAYED  %s"), *LastPlayed))).Font(APSMenu::Font("Regular", 9)).ColorAndOpacity(APSMenu::Muted)]
					]
				]
				+ SOverlay::Slot()
				[
					SNew(SChamferedFrame)
					.Color(bSelected ? APSMenu::Amber : APSMenu::CyanDim)
					.Thickness(bSelected ? 1.7f : 1.0f)
				]
			]
		];
	}
	ExistingWorldGridHost->SetContent(Grid);
}

void SAPSMainMenuRoot::RebuildExistingWorldDetails()
{
	if (!ExistingWorldDetailsHost) return;
	if (!SelectedWorld)
	{
		ExistingWorldDetailsHost->SetContent(SNew(STextBlock).Text(LOCTEXT("NoWorlds", "NO SAVED WORLDS FOUND")).ColorAndOpacity(APSMenu::Muted));
		return;
	}

	const uint32 StableImageIndex = GetTypeHash(SelectedWorld->SaveFileName) % 3u;
	const FSlateBrush* DetailsImage = StableImageIndex == 0
		? &PlanetImage : (StableImageIndex == 1 ? &GalaxyImage : &SystemImage);
	const auto DetailRow = [](const FText& Label, const FText& Value)
	{
		return SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()
			[SNew(STextBlock).Text(Label).Font(APSMenu::Font("Regular", 11)).ColorAndOpacity(APSMenu::Muted)]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 3.0f, 0.0f, 11.0f)
			[SNew(STextBlock).Text(Value).AutoWrapText(true).Font(APSMenu::Font("Bold", 12)).ColorAndOpacity(APSMenu::White)];
	};
	TSharedRef<SVerticalBox> DetailRows = SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight()[DetailRow(LOCTEXT("DetailSystemType", "SYSTEM TYPE"), FText::FromString(SelectedWorld->SystemType))]
		+ SVerticalBox::Slot().AutoHeight()[DetailRow(LOCTEXT("DetailStarType", "STAR TYPE"), FText::FromString(SelectedWorld->StarType))]
		+ SVerticalBox::Slot().AutoHeight()[DetailRow(LOCTEXT("DetailTotalPlanets", "TOTAL PLANETS"), FText::AsNumber(SelectedWorld->TotalPlanets))]
		+ SVerticalBox::Slot().AutoHeight()[DetailRow(LOCTEXT("DetailInhabited", "INHABITED PLANETS"), FText::AsNumber(SelectedWorld->InhabitedPlanets))]
		+ SVerticalBox::Slot().AutoHeight()[DetailRow(LOCTEXT("DetailHabitability", "HABITABILITY"), FText::FromString(SelectedWorld->Habitability))]
		+ SVerticalBox::Slot().AutoHeight()[DetailRow(LOCTEXT("DetailEnvironment", "ENVIRONMENT"), FText::FromString(SelectedWorld->Environment))]
		+ SVerticalBox::Slot().AutoHeight()[DetailRow(LOCTEXT("DetailSaveSize", "SAVE SIZE"), APSMenu::SaveSizeText(SelectedWorld->FileSizeBytes))];
	if (bShowTechnicalWorldDetails)
	{
		DetailRows->AddSlot().AutoHeight()[DetailRow(LOCTEXT("DetailSlot", "SAVE SLOT"), FText::FromString(SelectedWorld->SaveFileName))];
		DetailRows->AddSlot().AutoHeight()[DetailRow(LOCTEXT("DetailMetadata", "METADATA"), FText::FromString(SelectedWorld->bMetadataLoaded ? TEXT("READY") : TEXT("DEFERRED UNTIL LAUNCH")))];
		DetailRows->AddSlot().AutoHeight()[DetailRow(LOCTEXT("DetailLastPlayed", "LAST PLAYED"), FText::FromString(FDateTime::FromUnixTimestamp(SelectedWorld->FileTimestamp).ToString(TEXT("%Y-%m-%d  %H:%M"))))];
	}

	ExistingWorldDetailsHost->SetContent(
		SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight()
		[
			SNew(SBox).HeightOverride(190.0f).Clipping(EWidgetClipping::ClipToBounds)
			[SNew(SScaleBox).Stretch(EStretch::ScaleToFit)[SNew(SImage).Image(DetailsImage)]]
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 14.0f, 0.0f, 5.0f)
		[SNew(STextBlock).Text(FText::FromString(SelectedWorld->DisplayName)).AutoWrapText(true).Font(APSMenu::Font("Bold", 19)).ColorAndOpacity(APSMenu::White)]
		+ SVerticalBox::Slot().AutoHeight()
		[SNew(STextBlock).Text(FText::FromString(SelectedWorld->SystemType)).AutoWrapText(true).Font(APSMenu::Font("Regular", 12)).ColorAndOpacity(APSMenu::Cyan)]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 12.0f, 0.0f, 10.0f)
		[SNew(SBox).HeightOverride(1.0f)[SNew(SBorder).BorderImage(FAppStyle::GetBrush("WhiteBrush")).BorderBackgroundColor(APSMenu::CyanDim)]]
		+ SVerticalBox::Slot().FillHeight(1.0f)
		[
			SNew(SScrollBox).ScrollBarStyle(&ScrollBarStyle)
			+ SScrollBox::Slot()[DetailRows]
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 12.0f)
		[
			SNew(SButton).ButtonStyle(&PrimaryButtonStyle).OnClicked(this, &SAPSMainMenuRoot::ContinueExistingWorld).ContentPadding(FMargin(20.0f, 14.0f))
			[SNew(STextBlock).Text(LOCTEXT("ContinueWorld", "CONTINUE  >")).Justification(ETextJustify::Center).Font(APSMenu::Font("Bold", 17)).ColorAndOpacity(APSMenu::White)]
		]
		+ SVerticalBox::Slot().AutoHeight()
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(0.0f, 0.0f, 4.0f, 0.0f)
			[SNew(SButton).ButtonStyle(&SecondaryButtonStyle).OnClicked(this, &SAPSMainMenuRoot::ToggleWorldDetails).ContentPadding(FMargin(12.0f, 9.0f))[SNew(STextBlock).Text_Lambda([this](){ return FText::FromString(bShowTechnicalWorldDetails ? TEXT("HIDE DETAILS") : TEXT("WORLD DETAILS")); }).Justification(ETextJustify::Center).Font(APSMenu::Font("Bold", 10)).ColorAndOpacity(APSMenu::Cyan)]]
			+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(4.0f, 0.0f, 0.0f, 0.0f)
			[SNew(SButton).IsEnabled(false).ButtonStyle(&SecondaryButtonStyle).ContentPadding(FMargin(12.0f, 9.0f))[SNew(STextBlock).Text(LOCTEXT("DeleteWorld", "DELETE WORLD")).Justification(ETextJustify::Center).Font(APSMenu::Font("Bold", 10)).ColorAndOpacity(FLinearColor(0.88f, 0.19f, 0.14f, 1.0f))]]
		]);
}

FReply SAPSMainMenuRoot::ChangeExistingWorldPage(int32 Delta)
{
	ExistingWorldPage = FMath::Max(0, ExistingWorldPage + Delta);
	RebuildExistingWorldGrid();
	return FReply::Handled();
}

bool SAPSMainMenuRoot::PassesExistingWorldFilters(const FAPSExistingWorldEntry& Entry) const
{
	if (WorldCollection == EAPSWorldCollection::Favorites && !Entry.bFavorite)
	{
		return false;
	}

	const int32 StarFilter = WorldFilterIndices.FindRef(EAPSWorldFilterKind::StarType);
	if (StarFilter != 0)
	{
		const FString Star = Entry.StarType.ToUpper();
		const bool bKnown = Entry.bMetadataLoaded;
		const bool bHot = Star == TEXT("O") || Star == TEXT("B") || Star == TEXT("A");
		const bool bSolar = Star == TEXT("F") || Star == TEXT("G") || Star == TEXT("K");
		const bool bCool = Star == TEXT("M") || Star == TEXT("L") || Star == TEXT("T") || Star == TEXT("Y");
		if ((StarFilter == 1 && !bHot) || (StarFilter == 2 && !bSolar)
			|| (StarFilter == 3 && !bCool) || (StarFilter == 4 && bKnown))
		{
			return false;
		}
	}

	const int32 WorldTypeFilter = WorldFilterIndices.FindRef(EAPSWorldFilterKind::WorldType);
	if (WorldTypeFilter != 0)
	{
		const FString Type = Entry.SystemType.ToUpper();
		const bool bMulti = Type.Contains(TEXT("MULTI"));
		const bool bSingle = Type.Contains(TEXT("SINGLE"));
		if ((WorldTypeFilter == 1 && !bMulti) || (WorldTypeFilter == 2 && !bSingle)
			|| (WorldTypeFilter == 3 && (bMulti || bSingle)))
		{
			return false;
		}
	}

	const int32 InhabitedFilter = WorldFilterIndices.FindRef(EAPSWorldFilterKind::Inhabited);
	if ((InhabitedFilter == 1 && Entry.InhabitedPlanets <= 0)
		|| (InhabitedFilter == 2 && Entry.InhabitedPlanets > 0))
	{
		return false;
	}

	const int32 EnvironmentFilter = WorldFilterIndices.FindRef(EAPSWorldFilterKind::Environment);
	if (EnvironmentFilter != 0)
	{
		const FString EnvironmentValue = (Entry.Environment + TEXT(" ") + Entry.PlanetType).ToUpper();
		const bool bRocky = EnvironmentValue.Contains(TEXT("ROCK")) || EnvironmentValue.Contains(TEXT("EARTH"));
		const bool bGas = EnvironmentValue.Contains(TEXT("GAS"));
		const bool bIce = EnvironmentValue.Contains(TEXT("ICE")) || EnvironmentValue.Contains(TEXT("FROZEN"));
		if ((EnvironmentFilter == 1 && !bRocky) || (EnvironmentFilter == 2 && !bGas)
			|| (EnvironmentFilter == 3 && !bIce)
			|| (EnvironmentFilter == 4 && (bRocky || bGas || bIce)))
		{
			return false;
		}
	}
	return true;
}

FText SAPSMainMenuRoot::GetWorldCollectionLabel(EAPSWorldCollection Collection) const
{
	switch (Collection)
	{
	case EAPSWorldCollection::MyWorlds: return LOCTEXT("MyWorldsValue", "MY WORLDS");
	case EAPSWorldCollection::Favorites: return LOCTEXT("FavoritesValue", "FAVORITES");
	case EAPSWorldCollection::Recent: return LOCTEXT("RecentValue", "RECENT");
	default: return LOCTEXT("AllWorldsValue", "ALL WORLDS");
	}
}

FText SAPSMainMenuRoot::GetWorldSortLabel() const
{
	switch (WorldSortMode)
	{
	case EAPSWorldSortMode::Name: return LOCTEXT("SortName", "SORT: NAME  v");
	case EAPSWorldSortMode::SaveSize: return LOCTEXT("SortSize", "SORT: SAVE SIZE  v");
	default: return LOCTEXT("SortRecent", "SORT: LAST PLAYED  v");
	}
}

FText SAPSMainMenuRoot::GetWorldFilterLabel(EAPSWorldFilterKind Kind) const
{
	static const TCHAR* StarLabels[] = {TEXT("ANY  v"), TEXT("HOT O/B/A  v"), TEXT("SOLAR F/G/K  v"), TEXT("COOL M/L/T/Y  v"), TEXT("UNKNOWN  v")};
	static const TCHAR* TypeLabels[] = {TEXT("ANY  v"), TEXT("MULTI PLANET  v"), TEXT("SINGLE STAR  v"), TEXT("OTHER  v")};
	static const TCHAR* InhabitedLabels[] = {TEXT("ANY  v"), TEXT("INHABITED  v"), TEXT("UNINHABITED  v")};
	static const TCHAR* EnvironmentLabels[] = {TEXT("ANY  v"), TEXT("ROCKY  v"), TEXT("GAS  v"), TEXT("ICE  v"), TEXT("OTHER  v")};
	const int32 Index = WorldFilterIndices.FindRef(Kind);
	switch (Kind)
	{
	case EAPSWorldFilterKind::StarType: return FText::FromString(StarLabels[FMath::Clamp(Index, 0, UE_ARRAY_COUNT(StarLabels) - 1)]);
	case EAPSWorldFilterKind::WorldType: return FText::FromString(TypeLabels[FMath::Clamp(Index, 0, UE_ARRAY_COUNT(TypeLabels) - 1)]);
	case EAPSWorldFilterKind::Inhabited: return FText::FromString(InhabitedLabels[FMath::Clamp(Index, 0, UE_ARRAY_COUNT(InhabitedLabels) - 1)]);
	default: return FText::FromString(EnvironmentLabels[FMath::Clamp(Index, 0, UE_ARRAY_COUNT(EnvironmentLabels) - 1)]);
	}
}

FReply SAPSMainMenuRoot::SetWorldCollection(EAPSWorldCollection Collection)
{
	WorldCollection = Collection;
	ExistingWorldPage = 0;
	if (!SelectedWorld.IsValid() || !PassesExistingWorldFilters(*SelectedWorld))
	{
		const TSharedPtr<FAPSExistingWorldEntry>* FirstMatch = ExistingWorlds.FindByPredicate(
			[this](const TSharedPtr<FAPSExistingWorldEntry>& Entry)
			{
				return Entry.IsValid() && PassesExistingWorldFilters(*Entry);
			});
		SelectedWorld = FirstMatch ? *FirstMatch : nullptr;
	}
	RebuildExistingWorldGrid();
	RebuildExistingWorldDetails();
	return FReply::Handled();
}

FReply SAPSMainMenuRoot::CycleWorldSort()
{
	WorldSortMode = static_cast<EAPSWorldSortMode>((static_cast<uint8>(WorldSortMode) + 1) % 3);
	ExistingWorldPage = 0;
	RebuildExistingWorldGrid();
	return FReply::Handled();
}

FReply SAPSMainMenuRoot::ToggleWorldView()
{
	bCompactWorldList = !bCompactWorldList;
	ExistingWorldPage = 0;
	RebuildExistingWorldGrid();
	return FReply::Handled();
}

FReply SAPSMainMenuRoot::CycleWorldFilter(EAPSWorldFilterKind Kind)
{
	static const int32 Counts[] = {5, 4, 3, 5};
	int32& Index = WorldFilterIndices.FindOrAdd(Kind);
	Index = (Index + 1) % Counts[static_cast<uint8>(Kind)];
	ExistingWorldPage = 0;
	if (!SelectedWorld.IsValid() || !PassesExistingWorldFilters(*SelectedWorld))
	{
		const TSharedPtr<FAPSExistingWorldEntry>* FirstMatch = ExistingWorlds.FindByPredicate(
			[this](const TSharedPtr<FAPSExistingWorldEntry>& Entry)
			{
				return Entry.IsValid() && PassesExistingWorldFilters(*Entry);
			});
		SelectedWorld = FirstMatch ? *FirstMatch : nullptr;
	}
	RebuildExistingWorldGrid();
	RebuildExistingWorldDetails();
	return FReply::Handled();
}

FReply SAPSMainMenuRoot::ToggleWorldFavorite(TSharedPtr<FAPSExistingWorldEntry> Entry)
{
	if (Entry.IsValid())
	{
		Entry->bFavorite = !Entry->bFavorite;
		APSMenu::SaveFavoriteSlots(ExistingWorlds);
		if (WorldCollection == EAPSWorldCollection::Favorites && !Entry->bFavorite && SelectedWorld == Entry)
		{
			const TSharedPtr<FAPSExistingWorldEntry>* FirstMatch = ExistingWorlds.FindByPredicate(
				[this](const TSharedPtr<FAPSExistingWorldEntry>& Candidate)
				{
					return Candidate.IsValid() && PassesExistingWorldFilters(*Candidate);
				});
			SelectedWorld = FirstMatch ? *FirstMatch : nullptr;
			RebuildExistingWorldDetails();
		}
		RebuildExistingWorldGrid();
	}
	return FReply::Handled();
}

FReply SAPSMainMenuRoot::ToggleWorldDetails()
{
	bShowTechnicalWorldDetails = !bShowTechnicalWorldDetails;
	RebuildExistingWorldDetails();
	return FReply::Handled();
}

void SAPSMainMenuRoot::DiscoverSpawnClassOptions()
{
	if (bSpawnClassOptionsDiscovered)
	{
		return;
	}
	bSpawnClassOptionsDiscovered = true;

	SpawnClassOptions.Add(EAPSStartAssetSlot::Character, TArray<TSoftClassPtr<AActor>>());
	SpawnClassOptions.Add(EAPSStartAssetSlot::Spaceship, TArray<TSoftClassPtr<AActor>>());
	SpawnClassOptions.Add(EAPSStartAssetSlot::SpaceStation, TArray<TSoftClassPtr<AActor>>());
	SpawnClassOptions.Add(EAPSStartAssetSlot::Headquarters, TArray<TSoftClassPtr<AActor>>());
	SpawnClassOptions.Add(EAPSStartAssetSlot::Shipyard, TArray<TSoftClassPtr<AActor>>());
	const FSoftObjectPath ProductionPilotClassPath(
		TEXT("/Game/APS/APS_ALPHA/Blueprints/BP_CustomGravityCharacter.BP_CustomGravityCharacter_C"));

	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
	IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

	const auto FindDerivedClasses = [&AssetRegistry](const UClass* BaseClass)
	{
		TArray<FTopLevelAssetPath> BaseClassPaths;
		BaseClassPaths.Add(BaseClass->GetClassPathName());
		TSet<FTopLevelAssetPath> Result;
		AssetRegistry.GetDerivedClassNames(BaseClassPaths, TSet<FTopLevelAssetPath>(), Result);
		Result.Add(BaseClass->GetClassPathName());
		return Result;
	};

	// Match the GameMode contract: a playable start class may derive from APawn
	// directly or through ACharacter (the authored SinglePlay character does).
	const TSet<FTopLevelAssetPath> CharacterClassPaths = FindDerivedClasses(APawn::StaticClass());
	const TSet<FTopLevelAssetPath> SpaceshipClassPaths = FindDerivedClasses(ASpaceship::StaticClass());
	const TSet<FTopLevelAssetPath> StationClassPaths = FindDerivedClasses(ASpaceStation::StaticClass());
	const TSet<FTopLevelAssetPath> HeadquartersClassPaths = FindDerivedClasses(ASpaceHeadquarters::StaticClass());
	const TSet<FTopLevelAssetPath> ShipyardClassPaths = FindDerivedClasses(ASpaceShipyard::StaticClass());
	TSet<FTopLevelAssetPath> DerivedClassPaths = CharacterClassPaths;
	DerivedClassPaths.Append(SpaceshipClassPaths);
	DerivedClassPaths.Append(StationClassPaths);
	DerivedClassPaths.Append(HeadquartersClassPaths);
	DerivedClassPaths.Append(ShipyardClassPaths);

	FARFilter Filter;
	Filter.PackagePaths.Add(FName(TEXT("/Game/APS")));
	Filter.PackagePaths.Add(FName(TEXT("/Game/APS_PREA")));
	Filter.ClassPaths.Add(UBlueprint::StaticClass()->GetClassPathName());
	Filter.bRecursivePaths = true;
	Filter.bRecursiveClasses = true;

	TArray<FAssetData> BlueprintAssets;
	AssetRegistry.GetAssets(Filter, BlueprintAssets);
	for (const FAssetData& Asset : BlueprintAssets)
	{
		const uint32 ClassFlags = Asset.GetTagValueRef<uint32>(FBlueprintTags::ClassFlags);
		const uint32 RejectedFlags = CLASS_Abstract | CLASS_Deprecated
			| CLASS_NewerVersionExists | CLASS_NotPlaceable;
		if ((ClassFlags & RejectedFlags) != 0)
		{
			continue;
		}
		FString GeneratedClassExportPath;
		if (!Asset.GetTagValue(FBlueprintTags::GeneratedClassPath, GeneratedClassExportPath))
		{
			continue;
		}
		const FString GeneratedClassObjectPath = FPackageName::ExportTextPathToObjectPath(GeneratedClassExportPath);
		const FTopLevelAssetPath GeneratedClassPath(GeneratedClassObjectPath);
		if (!DerivedClassPaths.Contains(GeneratedClassPath))
		{
			continue;
		}

		EAPSStartAssetSlot Slot;
		if (HeadquartersClassPaths.Contains(GeneratedClassPath))
		{
			Slot = EAPSStartAssetSlot::Headquarters;
		}
		else if (ShipyardClassPaths.Contains(GeneratedClassPath))
		{
			Slot = EAPSStartAssetSlot::Shipyard;
		}
		else if (SpaceshipClassPaths.Contains(GeneratedClassPath))
		{
			Slot = EAPSStartAssetSlot::Spaceship;
		}
		else if (StationClassPaths.Contains(GeneratedClassPath))
		{
			Slot = EAPSStartAssetSlot::SpaceStation;
		}
		else
		{
			if (FSoftObjectPath(GeneratedClassObjectPath) != ProductionPilotClassPath)
			{
				continue;
			}
			Slot = EAPSStartAssetSlot::Character;
		}
		SpawnClassOptions.FindOrAdd(Slot).AddUnique(TSoftClassPtr<AActor>(FSoftObjectPath(GeneratedClassObjectPath)));
	}
	// Keep one stable entry even if the asset registry is still discovering files;
	// the async picker load and commit validation remain authoritative.
	SpawnClassOptions.FindOrAdd(EAPSStartAssetSlot::Character).AddUnique(
		TSoftClassPtr<AActor>(ProductionPilotClassPath));

	for (auto& Pair : SpawnClassOptions)
	{
		Pair.Value.Sort([](const TSoftClassPtr<AActor>& A, const TSoftClassPtr<AActor>& B)
		{
			return A.ToSoftObjectPath().GetAssetName() < B.ToSoftObjectPath().GetAssetName();
		});
	}
	SynchronizeSpawnClassOptions();
	UE_LOG(LogTemp, Log,
		TEXT("[APS.Menu] Blueprint spawn catalogue indexed without loading assets: characters=%d ships=%d stations=%d headquarters=%d shipyards=%d"),
		SpawnClassOptions.FindRef(EAPSStartAssetSlot::Character).Num(),
		SpawnClassOptions.FindRef(EAPSStartAssetSlot::Spaceship).Num(),
		SpawnClassOptions.FindRef(EAPSStartAssetSlot::SpaceStation).Num(),
		SpawnClassOptions.FindRef(EAPSStartAssetSlot::Headquarters).Num(),
		SpawnClassOptions.FindRef(EAPSStartAssetSlot::Shipyard).Num());
}

void SAPSMainMenuRoot::SynchronizeSpawnClassOptions()
{
	for (auto& Pair : SpawnClassOptions)
	{
		UClass* CurrentClass = nullptr;
		if (ViewModel.IsValid() && ViewModel->SpawnParameters)
		{
			switch (Pair.Key)
			{
			case EAPSStartAssetSlot::Character: CurrentClass = ViewModel->SpawnParameters->BP_CharacterClass; break;
			case EAPSStartAssetSlot::Spaceship: CurrentClass = ViewModel->SpawnParameters->BP_HomeSpaceship; break;
			case EAPSStartAssetSlot::SpaceStation: CurrentClass = ViewModel->SpawnParameters->BP_HomeSpaceStation; break;
			case EAPSStartAssetSlot::Headquarters: CurrentClass = ViewModel->SpawnParameters->BP_HomeSpaceHeadquarters; break;
			case EAPSStartAssetSlot::Shipyard: CurrentClass = ViewModel->SpawnParameters->BP_HomeSpaceShipyard; break;
			}
		}
		// Slate remains functional even if the legacy picker Blueprint is absent or
		// has an empty array: generator defaults are valid runtime selections and do
		// not require the user to reconnect anything in UMG.
		if (CurrentClass && CurrentClass->IsChildOf(AActor::StaticClass()))
		{
			Pair.Value.AddUnique(TSoftClassPtr<AActor>(CurrentClass));
		}
		int32 InitialIndex = Pair.Value.IndexOfByPredicate([CurrentClass](const TSoftClassPtr<AActor>& Candidate)
		{
			return Candidate.Get() == CurrentClass
				|| (CurrentClass && Candidate.ToSoftObjectPath() == FSoftObjectPath(CurrentClass));
		});
		if (InitialIndex == INDEX_NONE) InitialIndex = 0;
		SpawnClassIndices.Add(Pair.Key, InitialIndex);
		ApplySpawnClassSelection(Pair.Key);
	}
}

void SAPSMainMenuRoot::ApplySpawnClassSelection(EAPSStartAssetSlot Slot)
{
	if (TSharedPtr<FStreamableHandle>* ExistingHandle = SpawnSelectionLoadHandles.Find(Slot))
	{
		if (ExistingHandle->IsValid())
		{
			(*ExistingHandle)->CancelHandle();
		}
	}
	SpawnSelectionLoadHandles.Remove(Slot);
	SpawnSelectionRequestedPaths.Remove(Slot);

	const TArray<TSoftClassPtr<AActor>>* Options = SpawnClassOptions.Find(Slot);
	const int32 Index = SpawnClassIndices.FindRef(Slot);
	if (!Options || !Options->IsValidIndex(Index))
	{
		return;
	}

	const TSoftClassPtr<AActor> SelectedClass = (*Options)[Index];
	if (UClass* LoadedClass = SelectedClass.Get())
	{
		if (ViewModel.IsValid())
		{
			ViewModel->SetSpawnClass(Slot, LoadedClass);
		}
		Invalidate(EInvalidateWidgetReason::Paint);
		return;
	}

	const FSoftObjectPath RequestedPath = SelectedClass.ToSoftObjectPath();
	if (!RequestedPath.IsValid())
	{
		return;
	}
	SpawnSelectionRequestedPaths.Add(Slot, RequestedPath);
	const TSharedPtr<FStreamableHandle> RequestedHandle =
		UAssetManager::GetStreamableManager().RequestAsyncLoad(
		RequestedPath,
		FStreamableDelegate::CreateSP(this, &SAPSMainMenuRoot::OnSpawnClassSelectionLoaded, Slot, RequestedPath));
	// RequestAsyncLoad may invoke its delegate before returning for an already
	// resident class. Only retain the handle if the same request is still active.
	if (SpawnSelectionRequestedPaths.FindRef(Slot) == RequestedPath)
	{
		if (RequestedHandle.IsValid())
		{
			SpawnSelectionLoadHandles.Add(Slot, RequestedHandle);
		}
		else
		{
			// A stale picker entry must not leave the Continue button disabled
			// forever. Keep the previously committed class and surface the failure;
			// retrying the same missing soft path from its callback creates an
			// unbounded async-load loop.
			SpawnSelectionRequestedPaths.Remove(Slot);
			UE_LOG(LogTemp, Error,
				TEXT("[APS.Civilization] Failed to start class load for slot=%d path=%s"),
				static_cast<int32>(Slot), *RequestedPath.ToString());
			Invalidate(EInvalidateWidgetReason::Paint);
		}
	}
}

void SAPSMainMenuRoot::OnSpawnClassSelectionLoaded(EAPSStartAssetSlot Slot, FSoftObjectPath RequestedPath)
{
	const FSoftObjectPath* ActiveRequest = SpawnSelectionRequestedPaths.Find(Slot);
	if (!ActiveRequest || *ActiveRequest != RequestedPath)
	{
		return;
	}
	SpawnSelectionLoadHandles.Remove(Slot);
	SpawnSelectionRequestedPaths.Remove(Slot);
	const TArray<TSoftClassPtr<AActor>>* Options = SpawnClassOptions.Find(Slot);
	const int32 Index = SpawnClassIndices.FindRef(Slot);
	if (!Options || !Options->IsValidIndex(Index)
		|| (*Options)[Index].ToSoftObjectPath() != RequestedPath)
	{
		return;
	}
	UClass* LoadedClass = (*Options)[Index].Get();
	if (!LoadedClass)
	{
		// Loading completed without resolving a class (deleted/corrupt asset).
		// Do not recursively enqueue the identical request; the ViewModel keeps
		// its last valid selection and Commit validation remains authoritative.
		UE_LOG(LogTemp, Error,
			TEXT("[APS.Civilization] Class load resolved no class for slot=%d path=%s"),
			static_cast<int32>(Slot), *RequestedPath.ToString());
		Invalidate(EInvalidateWidgetReason::Paint);
		return;
	}
	if (ViewModel.IsValid())
	{
		ViewModel->SetSpawnClass(Slot, LoadedClass);
	}
	Invalidate(EInvalidateWidgetReason::Paint);
}

FText SAPSMainMenuRoot::GetSpawnClassName(EAPSStartAssetSlot Slot) const
{
	const TArray<TSoftClassPtr<AActor>>* Options = SpawnClassOptions.Find(Slot);
	const int32 Index = SpawnClassIndices.FindRef(Slot);
	return GetSpawnClassOptionName(Slot, Index);
}

FText SAPSMainMenuRoot::GetSpawnClassOptionName(EAPSStartAssetSlot Slot, int32 OptionIndex) const
{
	const TArray<TSoftClassPtr<AActor>>* Options = SpawnClassOptions.Find(Slot);
	if (!Options || !Options->IsValidIndex(OptionIndex)) return LOCTEXT("Unavailable", "NOT CONFIGURED");
	UClass* LoadedClass = (*Options)[OptionIndex].Get();
	if (!LoadedClass)
	{
		FString AssetName = (*Options)[OptionIndex].ToSoftObjectPath().GetAssetName();
		AssetName.RemoveFromEnd(TEXT("_C"));
		return FText::FromString(AssetName.Replace(TEXT("BP_"), TEXT("")));
	}
	const AActor* DefaultActor = LoadedClass->GetDefaultObject<AActor>();
	if (DefaultActor && DefaultActor->Implements<UItemInfoInterface>())
	{
		return IItemInfoInterface::Execute_GetInGameName(DefaultActor);
	}
	return FText::FromString(LoadedClass->GetName().Replace(TEXT("BP_"), TEXT("")));
}

TSharedRef<SWidget> SAPSMainMenuRoot::BuildSpawnCard(EAPSStartAssetSlot Slot, const FText& Label)
{
	const EAPSMenuGlyph SlotGlyph =
		Slot == EAPSStartAssetSlot::Character ? EAPSMenuGlyph::Pilot :
		Slot == EAPSStartAssetSlot::Spaceship ? EAPSMenuGlyph::Ship :
		Slot == EAPSStartAssetSlot::SpaceStation ? EAPSMenuGlyph::Station :
		Slot == EAPSStartAssetSlot::Headquarters ? EAPSMenuGlyph::Headquarters : EAPSMenuGlyph::Shipyard;
	const bool bLockedProductionPilot = Slot == EAPSStartAssetSlot::Character;
	const int32 SlotNumber = static_cast<int32>(Slot) + 1;
	const FText SlotDescription =
		Slot == EAPSStartAssetSlot::Character ? LOCTEXT("PilotCardRole", "CERTIFIED SURFACE & EVA OPERATOR") :
		Slot == EAPSStartAssetSlot::Spaceship ? LOCTEXT("ShipCardRole", "PRIMARY FLEET HULL") :
		Slot == EAPSStartAssetSlot::SpaceStation ? LOCTEXT("StationCardRole", "ORBITAL HABITAT CLASS") :
		Slot == EAPSStartAssetSlot::Headquarters ? LOCTEXT("HeadquartersCardRole", "CIVILIZATION COMMAND NODE") :
		LOCTEXT("ShipyardCardRole", "FLEET CONSTRUCTION NODE");
	TSharedRef<SUniformGridPanel> OptionGrid = SNew(SUniformGridPanel).SlotPadding(FMargin(2.0f));
	const TArray<TSoftClassPtr<AActor>>& Options = SpawnClassOptions.FindRef(Slot);
	for (int32 OptionIndex = 0; OptionIndex < Options.Num(); ++OptionIndex)
	{
		const int32 CapturedIndex = OptionIndex;
		OptionGrid->AddSlot(0, OptionIndex)
		[
			SNew(SButton)
			.ButtonStyle(&CardButtonStyle)
			.ContentPadding(FMargin(5.0f, 4.0f))
			.ButtonColorAndOpacity_Lambda([this, Slot, CapturedIndex]()
			{
				return SpawnClassIndices.FindRef(Slot) == CapturedIndex
					? FLinearColor(0.42f, 0.19f, 0.01f, 1.0f)
					: FLinearColor(0.02f, 0.13f, 0.18f, 0.92f);
			})
			.ToolTipText(FText::FromString(Options[OptionIndex].ToSoftObjectPath().ToString()))
			.OnClicked(this, &SAPSMainMenuRoot::SelectSpawnClass, Slot, OptionIndex)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
				[
					SNew(STextBlock)
					.Text(FText::FromString(FString::Printf(TEXT("%02d"), OptionIndex + 1)))
					.Font(APSMenu::Font("Bold", 8))
					.ColorAndOpacity_Lambda([this, Slot, CapturedIndex]()
					{
						return SpawnClassIndices.FindRef(Slot) == CapturedIndex ? APSMenu::Amber : APSMenu::Cyan;
					})
				]
				+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center).Padding(7.0f, 0.0f)
				[
					SNew(STextBlock)
					.Text_Lambda([this, Slot, CapturedIndex]() { return GetSpawnClassOptionName(Slot, CapturedIndex); })
					.Font(APSMenu::Font("Bold", 9)).ColorAndOpacity(APSMenu::White)
					.OverflowPolicy(ETextOverflowPolicy::Ellipsis)
				]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
				[
					SNew(STextBlock).Text(LOCTEXT("SpawnOptionActive", "ACTIVE"))
					.Font(APSMenu::Font("Bold", 7)).ColorAndOpacity(APSMenu::Success)
					.Visibility_Lambda([this, Slot, CapturedIndex]()
					{
						return SpawnClassIndices.FindRef(Slot) == CapturedIndex
							? EVisibility::Visible : EVisibility::Collapsed;
					})
				]
			]
		];
	}

	return SNew(SOverlay)
		+ SOverlay::Slot()
		[
			SNew(SBorder).BorderImage(&APSMenu::CivCardBrush).Padding(0.0f)
			[
				SNew(SOverlay)
				+ SOverlay::Slot().Padding(11.0f)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight()
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
						[APSMenu::IconBadge(SlotGlyph, bLockedProductionPilot ? APSMenu::Amber : APSMenu::Cyan, 30.0f)]
						+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center).Padding(9.0f, 0.0f)
						[
							SNew(SVerticalBox)
							+ SVerticalBox::Slot().AutoHeight()
							[SNew(STextBlock).Text(Label).Font(APSMenu::Font("Bold", 13)).ColorAndOpacity(APSMenu::White)]
							+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 1.0f, 0.0f, 0.0f)
							[SNew(STextBlock).Text(SlotDescription).Font(APSMenu::Font("Regular", 8)).ColorAndOpacity(APSMenu::Muted).OverflowPolicy(ETextOverflowPolicy::Ellipsis)]
						]
						+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
						[
							SNew(SBorder).BorderImage(&APSMenu::CivStatusBrush).Padding(FMargin(6.0f, 3.0f))
							[
								SNew(STextBlock)
								.Text(bLockedProductionPilot ? LOCTEXT("PilotCardLocked", "LOCKED")
									: FText::FromString(FString::Printf(TEXT("%02d"), SlotNumber)))
								.Font(APSMenu::Font("Bold", 8))
								.ColorAndOpacity(bLockedProductionPilot ? APSMenu::Amber : APSMenu::Cyan)
							]
						]
					]
					+ SVerticalBox::Slot().FillHeight(0.56f).Padding(0.0f, 10.0f, 0.0f, 8.0f)
					[
						SNew(SBorder).BorderImage(&APSMenu::InsetBrush).Padding(2.0f)
						[
							SNew(SOverlay)
							+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center)
							[
								SNew(SBox).WidthOverride(96.0f).HeightOverride(96.0f)
								[SNew(SVectorMenuGlyph).Glyph(SlotGlyph).Color(FLinearColor(0.03f, 0.28f, 0.38f, 0.32f)).StrokeWidth(4.8f)]
							]
							+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center)
							[
								SNew(SBox).WidthOverride(62.0f).HeightOverride(62.0f)
								[SNew(SVectorMenuGlyph).Glyph(SlotGlyph).Color(bLockedProductionPilot ? APSMenu::Amber : APSMenu::Cyan).StrokeWidth(1.9f)]
							]
							+ SOverlay::Slot().HAlign(HAlign_Left).VAlign(VAlign_Bottom).Padding(8.0f)
							[
								SNew(STextBlock).Text(LOCTEXT("SpawnCardLoadout", "LOADOUT CLASS"))
								.Font(APSMenu::Font("Bold", 8)).ColorAndOpacity(APSMenu::Muted)
							]
						]
					]
					+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
					[
						SNew(STextBlock).Text_Lambda([this, Slot]() { return GetSpawnClassName(Slot); })
						.Font(APSMenu::Font("Bold", 14)).ColorAndOpacity(APSMenu::White)
						.Justification(ETextJustify::Center).AutoWrapText(true)
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 7.0f, 0.0f, 5.0f)
					[
						SNew(SHorizontalBox)
						.Visibility(bLockedProductionPilot ? EVisibility::Collapsed : EVisibility::Visible)
						+ SHorizontalBox::Slot().FillWidth(0.28f)
						[SNew(SButton).ButtonStyle(&SecondaryButtonStyle).ContentPadding(FMargin(5.0f, 4.0f)).OnClicked(this, &SAPSMainMenuRoot::CycleSpawnClass, Slot, -1)[SNew(STextBlock).Text(FText::FromString(TEXT("<"))).Justification(ETextJustify::Center).Font(APSMenu::Font("Bold", 11)).ColorAndOpacity(APSMenu::Cyan)]]
						+ SHorizontalBox::Slot().FillWidth(0.44f).HAlign(HAlign_Center).VAlign(VAlign_Center)
						[
							SNew(STextBlock).Text_Lambda([this, Slot]()
							{
								const int32 Count = SpawnClassOptions.FindRef(Slot).Num();
								return FText::FromString(FString::Printf(TEXT("CLASS %02d / %02d"), Count > 0 ? SpawnClassIndices.FindRef(Slot) + 1 : 0, Count));
							}).Justification(ETextJustify::Center).Font(APSMenu::Font("Bold", 8)).ColorAndOpacity(APSMenu::Muted)
						]
						+ SHorizontalBox::Slot().FillWidth(0.28f)
						[SNew(SButton).ButtonStyle(&SecondaryButtonStyle).ContentPadding(FMargin(5.0f, 4.0f)).OnClicked(this, &SAPSMainMenuRoot::CycleSpawnClass, Slot, 1)[SNew(STextBlock).Text(FText::FromString(TEXT(">"))).Justification(ETextJustify::Center).Font(APSMenu::Font("Bold", 11)).ColorAndOpacity(APSMenu::Cyan)]]
					]
					+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0.0f, 5.0f, 0.0f, 8.0f)
					[
						SNew(SBorder).BorderImage(&APSMenu::CivStatusBrush).Padding(FMargin(8.0f, 4.0f))
						.Visibility(bLockedProductionPilot ? EVisibility::Visible : EVisibility::Collapsed)
						[
							SNew(STextBlock).Text(LOCTEXT("ProductionPilotLocked", "VERIFIED GRAVITY PILOT"))
							.Font(APSMenu::Font("Bold", 8)).ColorAndOpacity(APSMenu::Amber)
						]
					]
					+ SVerticalBox::Slot().FillHeight(0.44f)
					[
						SNew(SBox).MaxDesiredHeight(98.0f)
						.Visibility(bLockedProductionPilot ? EVisibility::Collapsed : EVisibility::Visible)
						[
							SNew(SBorder).BorderImage(&APSMenu::InsetBrush).Padding(4.0f)
							[SNew(SScrollBox) + SScrollBox::Slot()[OptionGrid]]
						]
					]
				]
				+ SOverlay::Slot().HAlign(HAlign_Fill).VAlign(VAlign_Top)
				[
					SNew(SBox).HeightOverride(2.0f)
					[SNew(SBorder).BorderImage(FAppStyle::GetBrush("WhiteBrush")).BorderBackgroundColor(bLockedProductionPilot ? APSMenu::Amber : APSMenu::Cyan)]
				]
			]
		]
		+ SOverlay::Slot()
		[
			SNew(SChamferedFrame).Color(APSMenu::Cyan).Thickness(1.15f)
		];
}

TSharedRef<SWidget> SAPSMainMenuRoot::BuildCivilizationPage()
{
	DiscoverSpawnClassOptions();
	const TWeakObjectPtr<UWorldGenerationViewModel> VM = ViewModel;
	CivilizationEditorSection = FMath::Clamp(CivilizationEditorSection, 0, 2);

	const auto EnumControl = [this](const FText& Label, const UEnum* Enum,
		TFunction<int32()> Getter, TFunction<void(int32)> Setter)
	{
		const auto Step = [Enum, Getter, Setter](int32 Direction)
		{
			if (!Enum) return FReply::Handled();
			const int32 Count = FMath::Max(1, Enum->NumEnums() - 1);
			Setter((Getter() + Direction + Count) % Count);
			return FReply::Handled();
		};
		return SNew(SBorder).BorderImage(&APSMenu::CivControlBrush).Padding(FMargin(10.0f, 7.0f))
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(0.44f).VAlign(VAlign_Center)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight()
				[SNew(STextBlock).Text(Label).Font(APSMenu::Font("Bold", 10)).ColorAndOpacity(APSMenu::White)]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 2.0f, 0.0f, 0.0f)
				[SNew(STextBlock).Text(LOCTEXT("EnumControlType", "SELECTED PROFILE")).Font(APSMenu::Font("Regular", 8)).ColorAndOpacity(APSMenu::Muted)]
			]
			+ SHorizontalBox::Slot().FillWidth(0.56f).VAlign(VAlign_Center).Padding(10.0f, 0.0f, 0.0f, 0.0f)
			[
				SNew(SBorder).BorderImage(&APSMenu::InsetBrush).Padding(FMargin(2.0f))
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().AutoWidth()[SNew(SButton).ButtonStyle(&SecondaryButtonStyle).ContentPadding(FMargin(8.0f, 5.0f)).OnClicked_Lambda([Step](){ return Step(-1); })[SNew(STextBlock).Text(FText::FromString(TEXT("<"))).Font(APSMenu::Font("Bold", 10)).ColorAndOpacity(APSMenu::Cyan)]]
					+ SHorizontalBox::Slot().FillWidth(1.0f).HAlign(HAlign_Center).VAlign(VAlign_Center)
					[SNew(STextBlock).Text_Lambda([Enum, Getter](){ return Enum ? Enum->GetDisplayNameTextByValue(Getter()) : FText::FromString(TEXT("--")); }).Font(APSMenu::Font("Bold", 10)).ColorAndOpacity(APSMenu::White).OverflowPolicy(ETextOverflowPolicy::Ellipsis)]
					+ SHorizontalBox::Slot().AutoWidth()[SNew(SButton).ButtonStyle(&SecondaryButtonStyle).ContentPadding(FMargin(8.0f, 5.0f)).OnClicked_Lambda([Step](){ return Step(1); })[SNew(STextBlock).Text(FText::FromString(TEXT(">"))).Font(APSMenu::Font("Bold", 10)).ColorAndOpacity(APSMenu::Cyan)]]
				]
			]
		];
	};

	const auto NumberControl = [](const FText& Label, int32 MinValue, int32 MaxValue, int32 Delta,
		TFunction<int32()> Getter, TFunction<void(int32)> Setter)
	{
		return SNew(SBorder).BorderImage(&APSMenu::CivControlBrush).Padding(FMargin(10.0f, 7.0f))
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(0.58f).VAlign(VAlign_Center)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight()
				[SNew(STextBlock).Text(Label).Font(APSMenu::Font("Bold", 10)).ColorAndOpacity(APSMenu::White)]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 2.0f, 0.0f, 0.0f)
				[SNew(STextBlock).Text(FText::FromString(FString::Printf(TEXT("RANGE  %s - %s"), *FText::AsNumber(MinValue).ToString(), *FText::AsNumber(MaxValue).ToString()))).Font(APSMenu::Font("Regular", 8)).ColorAndOpacity(APSMenu::Muted)]
			]
			+ SHorizontalBox::Slot().FillWidth(0.42f).VAlign(VAlign_Center).Padding(10.0f, 0.0f, 0.0f, 0.0f)
			[
				SNew(SBorder).BorderImage(&APSMenu::InsetBrush).Padding(3.0f)
				[
					SNew(SSpinBox<int32>).MinValue(MinValue).MaxValue(MaxValue).Delta(Delta)
					.MinDesiredWidth(116.0f).Font(APSMenu::Font("Bold", 11))
					.Value_Lambda([Getter](){ return Getter(); })
					.OnValueChanged_Lambda([Setter](int32 Value){ Setter(Value); })
				]
			]
		];
	};

	const auto ReadinessControl = [this](const FText& Label, TFunction<int32()> Getter,
		TFunction<void(int32)> Setter)
	{
		const auto Step = [Getter, Setter](int32 Direction)
		{
			Setter(FMath::Clamp(Getter() + Direction, 0, 20));
			return FReply::Handled();
		};
		return SNew(SBorder).BorderImage(&APSMenu::CivControlBrush).Padding(FMargin(10.0f, 7.0f))
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center)
				[SNew(STextBlock).Text(Label).Font(APSMenu::Font("Bold", 10)).ColorAndOpacity(APSMenu::White)]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
				[SNew(SButton).ButtonStyle(&SecondaryButtonStyle).ContentPadding(FMargin(8.0f, 2.0f)).OnClicked_Lambda([Step](){ return Step(-1); })[SNew(STextBlock).Text(FText::FromString(TEXT("-"))).Font(APSMenu::Font("Bold", 11)).ColorAndOpacity(APSMenu::Cyan)]]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(9.0f, 0.0f)
				[SNew(STextBlock).Text_Lambda([Getter](){ return FText::FromString(FString::Printf(TEXT("%02d / 20"), Getter())); }).Font(APSMenu::Font("Bold", 10)).ColorAndOpacity(APSMenu::Cyan)]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
				[SNew(SButton).ButtonStyle(&SecondaryButtonStyle).ContentPadding(FMargin(8.0f, 2.0f)).OnClicked_Lambda([Step](){ return Step(1); })[SNew(STextBlock).Text(FText::FromString(TEXT("+"))).Font(APSMenu::Font("Bold", 11)).ColorAndOpacity(APSMenu::Cyan)]]
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 7.0f, 0.0f, 0.0f)
			[
				SNew(SBox).HeightOverride(4.0f)
				[
					SNew(SProgressBar)
					.Percent_Lambda([Getter](){ return TOptional<float>(FMath::Clamp(Getter() / 20.0f, 0.0f, 1.0f)); })
					.FillColorAndOpacity(APSMenu::Cyan)
				]
			]
		];
	};

	const auto InfoPanel = [](EAPSMenuGlyph Glyph, const FText& Title,
		const FText& Subtitle, TAttribute<FText> Body)
	{
		return APSMenu::ChamferPanel(
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)[APSMenu::IconBadge(Glyph, APSMenu::Cyan, 30.0f)]
				+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center).Padding(9.0f, 0.0f)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight()[SNew(STextBlock).Text(Title).Font(APSMenu::Font("Bold", 12)).ColorAndOpacity(APSMenu::White)]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 1.0f, 0.0f, 0.0f)[SNew(STextBlock).Text(Subtitle).Font(APSMenu::Font("Regular", 8)).ColorAndOpacity(APSMenu::Muted)]
				]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
				[SNew(STextBlock).Text(LOCTEXT("CivInfoLive", "LIVE")).Font(APSMenu::Font("Bold", 8)).ColorAndOpacity(APSMenu::Success)]
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 9.0f, 0.0f, 0.0f)
			[SNew(SBox).HeightOverride(1.0f)[SNew(SBorder).BorderImage(FAppStyle::GetBrush("WhiteBrush")).BorderBackgroundColor(APSMenu::CyanDim)]]
			+ SVerticalBox::Slot().FillHeight(1.0f).Padding(0.0f, 10.0f, 0.0f, 0.0f)
			[SNew(STextBlock).Text(Body).AutoWrapText(true).Font(APSMenu::Font("Regular", 11)).ColorAndOpacity(APSMenu::White)]
		, FMargin(15.0f), FLinearColor(0.035f, 0.28f, 0.37f, 0.95f), 1.0f);
	};

	const auto PresetButton = [this](const FText& Label, const FText& ToolTip,
		TFunction<bool()> IsActive, TFunction<void()> Apply)
	{
		return SNew(SButton).ButtonStyle(&SecondaryButtonStyle).ToolTipText(ToolTip)
			.ContentPadding(FMargin(8.0f, 7.0f))
			.ButtonColorAndOpacity_Lambda([IsActive]()
			{
				return IsActive() ? FLinearColor(0.45f, 0.18f, 0.01f, 1.0f)
					: FLinearColor(0.015f, 0.09f, 0.12f, 0.96f);
			})
			.OnClicked_Lambda([Apply](){ Apply(); return FReply::Handled(); })
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
				[SNew(STextBlock).Text(Label).Justification(ETextJustify::Center).Font(APSMenu::Font("Bold", 9)).ColorAndOpacity(APSMenu::White)]
				+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0.0f, 2.0f, 0.0f, 0.0f)
				[
					SNew(STextBlock).Text_Lambda([IsActive](){ return IsActive() ? LOCTEXT("PresetActive", "ACTIVE") : LOCTEXT("PresetProfile", "PROFILE"); })
					.Font(APSMenu::Font("Bold", 8)).ColorAndOpacity_Lambda([IsActive](){ return IsActive() ? APSMenu::Amber : APSMenu::Muted; })
				]
			];
	};

	const auto MetricTile = [](const FText& Label, TAttribute<FText> Value,
		const FLinearColor& Accent = APSMenu::Cyan)
	{
		return SNew(SBorder).BorderImage(&APSMenu::CivMetricBrush).Padding(FMargin(8.0f, 7.0f))
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
			[SNew(STextBlock).Text(Value).Font(APSMenu::Font("Bold", 15)).ColorAndOpacity(Accent).Justification(ETextJustify::Center)]
			+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0.0f, 2.0f, 0.0f, 0.0f)
			[SNew(STextBlock).Text(Label).Font(APSMenu::Font("Bold", 8)).ColorAndOpacity(APSMenu::Muted).Justification(ETextJustify::Center)]
		];
	};

	const auto ApplyInfrastructurePreset = [VM](int32 Fleet, int32 Star, int32 Planet,
		int32 Orbital, int32 Ground)
	{
		if (!VM.IsValid() || !VM->SpawnParameters) return;
		USpawnParameters* P = VM->SpawnParameters;
		P->StartingFleetSize = Fleet;
		P->StarOutposts = Star;
		P->PlanetOutposts = Planet;
		P->OrbitalOutposts = Orbital;
		P->GroundOutposts = Ground;
		P->SanitizeForGeneration();
	};
	const auto ApplyDivisionPreset = [VM](int32 Exploration, int32 Industry, int32 Science,
		int32 Civil, int32 Military, int32 Fleet)
	{
		if (!VM.IsValid() || !VM->SpawnParameters) return;
		USpawnParameters* P = VM->SpawnParameters;
		P->ExplorationDivisionLevel = Exploration;
		P->IndustryDivisionLevel = Industry;
		P->ScienceDivisionLevel = Science;
		P->CivilAffairsDivisionLevel = Civil;
		P->MilitaryDivisionLevel = Military;
		P->FleetDivisionLevel = Fleet;
		P->SanitizeForGeneration();
	};

	const auto EditorTab = [this](int32 Index, EAPSMenuGlyph Glyph, const FText& Label)
	{
		return SNew(SButton).ButtonStyle(&SecondaryButtonStyle).ContentPadding(FMargin(6.0f, 7.0f))
		.ButtonColorAndOpacity_Lambda([this, Index]()
		{
			return CivilizationEditorSection == Index
				? FLinearColor(0.38f, 0.16f, 0.01f, 1.0f)
				: FLinearColor(0.015f, 0.08f, 0.11f, 0.95f);
		})
		.OnClicked_Lambda([this, Index]()
		{
			CivilizationEditorSection = Index;
			if (CivilizationEditorSwitcher.IsValid())
			{
				CivilizationEditorSwitcher->SetActiveWidgetIndex(Index);
			}
			return FReply::Handled();
		})
		[
			SNew(SOverlay)
			+ SOverlay::Slot().Padding(2.0f, 0.0f, 2.0f, 3.0f)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
				[SNew(STextBlock).Text(FText::FromString(FString::Printf(TEXT("STEP %02d"), Index + 1))).Font(APSMenu::Font("Bold", 8)).ColorAndOpacity_Lambda([this, Index](){ return CivilizationEditorSection == Index ? APSMenu::Amber : APSMenu::Muted; })]
				+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0.0f, 5.0f, 0.0f, 0.0f)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
					[SNew(SBox).WidthOverride(18.0f).HeightOverride(18.0f)[SNew(SVectorMenuGlyph).Glyph(Glyph).Color(APSMenu::Cyan).StrokeWidth(1.3f)]]
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(5.0f, 0.0f)
					[SNew(STextBlock).Text(Label).Font(APSMenu::Font("Bold", 8)).ColorAndOpacity(APSMenu::White)]
				]
			]
			+ SOverlay::Slot().HAlign(HAlign_Fill).VAlign(VAlign_Bottom)
			[
				SNew(SBox).HeightOverride(2.0f)
				.Visibility_Lambda([this, Index](){ return CivilizationEditorSection == Index ? EVisibility::Visible : EVisibility::Collapsed; })
				[SNew(SBorder).BorderImage(FAppStyle::GetBrush("WhiteBrush")).BorderBackgroundColor(APSMenu::Amber)]
			]
		];
	};

	TSharedRef<SWidget> IdentityEditor = SNew(SScrollBox)
		+ SScrollBox::Slot()
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()[APSMenu::IconSectionHeading(EAPSMenuGlyph::Civilization,
				LOCTEXT("IdentitySetup", "IDENTITY & SOCIETY"), LOCTEXT("IdentitySetupHint", "Define who arrives in the home system."))]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 14.0f, 0.0f, 6.0f)
			[
				SNew(SBorder).BorderImage(&APSMenu::CivControlBrush).Padding(FMargin(10.0f, 7.0f))
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().FillWidth(0.44f).VAlign(VAlign_Center)
					[
						SNew(SVerticalBox)
						+ SVerticalBox::Slot().AutoHeight()[SNew(STextBlock).Text(LOCTEXT("CivilizationName", "CIVILIZATION NAME")).Font(APSMenu::Font("Bold", 10)).ColorAndOpacity(APSMenu::White)]
						+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 2.0f, 0.0f, 0.0f)[SNew(STextBlock).Text(LOCTEXT("CivilizationNameHint", "ARRIVAL REGISTRY")).Font(APSMenu::Font("Regular", 8)).ColorAndOpacity(APSMenu::Muted)]
					]
					+ SHorizontalBox::Slot().FillWidth(0.56f).VAlign(VAlign_Center).Padding(10.0f, 0.0f, 0.0f, 0.0f)
					[SNew(SEditableTextBox).Text_Lambda([VM](){ const USpawnParameters* P=VM.IsValid()?VM->SpawnParameters.Get():nullptr; return P?FText::FromString(P->CivilizationName):FText::GetEmpty(); }).Font(APSMenu::Font("Bold", 11)).SelectAllTextWhenFocused(true).OnTextCommitted_Lambda([VM](const FText& T,ETextCommit::Type){ if(VM.IsValid()&&VM->SpawnParameters) VM->SpawnParameters->CivilizationName=T.ToString(); })]
				]
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 6.0f)[EnumControl(LOCTEXT("Archetype", "ARCHETYPE"), StaticEnum<EAPSCivilizationArchetype>(), [VM](){return VM.IsValid()&&VM->SpawnParameters?static_cast<int32>(VM->SpawnParameters->CivilizationArchetype):0;}, [VM](int32 V){if(VM.IsValid()&&VM->SpawnParameters)VM->SpawnParameters->CivilizationArchetype=static_cast<EAPSCivilizationArchetype>(V);})]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 6.0f)[EnumControl(LOCTEXT("Government", "GOVERNMENT"), StaticEnum<EAPSGovernmentType>(), [VM](){return VM.IsValid()&&VM->SpawnParameters?static_cast<int32>(VM->SpawnParameters->GovernmentType):0;}, [VM](int32 V){if(VM.IsValid()&&VM->SpawnParameters)VM->SpawnParameters->GovernmentType=static_cast<EAPSGovernmentType>(V);})]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 6.0f)[EnumControl(LOCTEXT("Economy", "ECONOMY"), StaticEnum<EAPSEconomicSystem>(), [VM](){return VM.IsValid()&&VM->SpawnParameters?static_cast<int32>(VM->SpawnParameters->EconomicSystem):0;}, [VM](int32 V){if(VM.IsValid()&&VM->SpawnParameters)VM->SpawnParameters->EconomicSystem=static_cast<EAPSEconomicSystem>(V);})]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 6.0f)[EnumControl(LOCTEXT("Society", "SOCIETY"), StaticEnum<EAPSSocietyType>(), [VM](){return VM.IsValid()&&VM->SpawnParameters?static_cast<int32>(VM->SpawnParameters->SocietyType):0;}, [VM](int32 V){if(VM.IsValid()&&VM->SpawnParameters)VM->SpawnParameters->SocietyType=static_cast<EAPSSocietyType>(V);})]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 6.0f)[NumberControl(LOCTEXT("FoundingPopulation", "FOUNDING POPULATION"), 1, 100000000, 1000, [VM](){return VM.IsValid()&&VM->SpawnParameters?VM->SpawnParameters->FoundingPopulation:1;}, [VM](int32 V){if(VM.IsValid()&&VM->SpawnParameters)VM->SpawnParameters->FoundingPopulation=V;})]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 6.0f)[NumberControl(LOCTEXT("StartingCredits", "STARTING CREDITS"), 0, 2000000000, 10000, [VM](){return VM.IsValid()&&VM->SpawnParameters?static_cast<int32>(FMath::Min<int64>(VM->SpawnParameters->StartingCredits, MAX_int32)):0;}, [VM](int32 V){if(VM.IsValid()&&VM->SpawnParameters)VM->SpawnParameters->StartingCredits=V;})]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 6.0f)[NumberControl(LOCTEXT("Technology", "TECHNOLOGY LEVEL"), 1, 10, 1, [VM](){return VM.IsValid()&&VM->SpawnParameters?VM->SpawnParameters->TechnologyLevel:1;}, [VM](int32 V){if(VM.IsValid()&&VM->SpawnParameters)VM->SpawnParameters->TechnologyLevel=V;})]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 6.0f)[EnumControl(LOCTEXT("SpawnPlace", "PILOT START LOCATION"), StaticEnum<ECharSpawnPlace>(), [VM](){return VM.IsValid()&&VM->SpawnParameters?static_cast<int32>(VM->SpawnParameters->CharacterSpawnPlace):0;}, [VM](int32 V){if(VM.IsValid())VM->SetCharacterSpawnPlace(V);})]
			+ SVerticalBox::Slot().AutoHeight()[EnumControl(LOCTEXT("OrbitHeight", "HOME COMPLEX ORBIT"), StaticEnum<EOrbitHeight>(), [VM](){return VM.IsValid()&&VM->SpawnParameters?static_cast<int32>(VM->SpawnParameters->HomeStationOrbitHeight):0;}, [VM](int32 V){if(VM.IsValid())VM->SetStationOrbitHeight(V);})]
		];

	TSharedRef<SWidget> InfrastructureEditor = SNew(SScrollBox)
		+ SScrollBox::Slot()
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()[APSMenu::IconSectionHeading(EAPSMenuGlyph::Infrastructure,
				LOCTEXT("InfrastructureSetup", "PHYSICAL MANIFEST"), LOCTEXT("InfrastructureSetupHint", "Every count creates actors in the generated system."))]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 14.0f, 0.0f, 10.0f)
			[
				SNew(SUniformGridPanel).SlotPadding(FMargin(3.0f))
				+ SUniformGridPanel::Slot(0, 0)[PresetButton(LOCTEXT("PresetScout", "SCOUT"), LOCTEXT("PresetScoutTip", "1 ship and the mandatory home station."), [VM](){const USpawnParameters* P=VM.IsValid()?VM->SpawnParameters.Get():nullptr;return P&&P->StartingFleetSize==1&&P->StarOutposts==0&&P->PlanetOutposts==0&&P->OrbitalOutposts==1&&P->GroundOutposts==0;}, [ApplyInfrastructurePreset](){ApplyInfrastructurePreset(1,0,0,1,0);})]
				+ SUniformGridPanel::Slot(1, 0)[PresetButton(LOCTEXT("PresetFrontier", "FRONTIER"), LOCTEXT("PresetFrontierTip", "A compact expansion-ready settlement."), [VM](){const USpawnParameters* P=VM.IsValid()?VM->SpawnParameters.Get():nullptr;return P&&P->StartingFleetSize==4&&P->StarOutposts==1&&P->PlanetOutposts==2&&P->OrbitalOutposts==2&&P->GroundOutposts==2;}, [ApplyInfrastructurePreset](){ApplyInfrastructurePreset(4,1,2,2,2);})]
				+ SUniformGridPanel::Slot(2, 0)[PresetButton(LOCTEXT("PresetCore", "CORE"), LOCTEXT("PresetCoreTip", "An established system with a task force."), [VM](){const USpawnParameters* P=VM.IsValid()?VM->SpawnParameters.Get():nullptr;return P&&P->StartingFleetSize==10&&P->StarOutposts==3&&P->PlanetOutposts==5&&P->OrbitalOutposts==5&&P->GroundOutposts==8;}, [ApplyInfrastructurePreset](){ApplyInfrastructurePreset(10,3,5,5,8);})]
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 6.0f)[NumberControl(LOCTEXT("FleetSize", "TOTAL FLEET SHIPS"), 1, USpawnParameters::MaxStartingFleetSize, 1, [VM](){return VM.IsValid()&&VM->SpawnParameters?VM->SpawnParameters->StartingFleetSize:1;}, [VM](int32 V){if(VM.IsValid()&&VM->SpawnParameters)VM->SpawnParameters->StartingFleetSize=V;})]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 6.0f)[NumberControl(LOCTEXT("StarOutposts", "STAR OUTPOSTS"), 0, USpawnParameters::MaxInfrastructurePerCategory, 1, [VM](){return VM.IsValid()&&VM->SpawnParameters?VM->SpawnParameters->StarOutposts:0;}, [VM](int32 V){if(VM.IsValid()&&VM->SpawnParameters)VM->SpawnParameters->StarOutposts=V;})]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 6.0f)[NumberControl(LOCTEXT("PlanetOutposts", "PLANET OUTPOSTS"), 0, USpawnParameters::MaxInfrastructurePerCategory, 1, [VM](){return VM.IsValid()&&VM->SpawnParameters?VM->SpawnParameters->PlanetOutposts:0;}, [VM](int32 V){if(VM.IsValid()&&VM->SpawnParameters)VM->SpawnParameters->PlanetOutposts=V;})]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 6.0f)[NumberControl(LOCTEXT("OrbitalOutposts", "TOTAL ORBITAL STATIONS"), 1, USpawnParameters::MaxInfrastructurePerCategory, 1, [VM](){return VM.IsValid()&&VM->SpawnParameters?VM->SpawnParameters->OrbitalOutposts:1;}, [VM](int32 V){if(VM.IsValid()&&VM->SpawnParameters)VM->SpawnParameters->OrbitalOutposts=V;})]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 6.0f)[NumberControl(LOCTEXT("GroundOutposts", "GROUND SETTLEMENTS"), 0, USpawnParameters::MaxInfrastructurePerCategory, 1, [VM](){return VM.IsValid()&&VM->SpawnParameters?VM->SpawnParameters->GroundOutposts:0;}, [VM](int32 V){if(VM.IsValid()&&VM->SpawnParameters)VM->SpawnParameters->GroundOutposts=V;})]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 4.0f, 0.0f, 0.0f)
			[
				SNew(SBorder).BorderImage(&APSMenu::AmberPanelBrush).Padding(10.0f)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight()
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center)
						[SNew(STextBlock).Text(LOCTEXT("DeploymentPlanTitle", "DEPLOYMENT PLAN")).Font(APSMenu::Font("Bold", 9)).ColorAndOpacity(APSMenu::Amber)]
						+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
						[SNew(STextBlock).Text(LOCTEXT("DeploymentPlanPhysical", "PHYSICAL SPAWN")).Font(APSMenu::Font("Bold", 8)).ColorAndOpacity(APSMenu::Success)]
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 8.0f, 0.0f, 0.0f)
					[
						SNew(SUniformGridPanel).SlotPadding(FMargin(2.0f))
						+ SUniformGridPanel::Slot(0,0)[MetricTile(LOCTEXT("MetricActors", "TOTAL ACTORS"),TAttribute<FText>::CreateLambda([VM](){const USpawnParameters* P=VM.IsValid()?VM->SpawnParameters.Get():nullptr;return FText::AsNumber(P?P->GetPlannedPhysicalActorCount():0);}),APSMenu::Amber)]
						+ SUniformGridPanel::Slot(1,0)[MetricTile(LOCTEXT("MetricShips", "FLEET"),TAttribute<FText>::CreateLambda([VM](){const USpawnParameters* P=VM.IsValid()?VM->SpawnParameters.Get():nullptr;return FText::AsNumber(P?P->StartingFleetSize:0);}))]
						+ SUniformGridPanel::Slot(2,0)[MetricTile(LOCTEXT("MetricInfra", "INFRA NODES"),TAttribute<FText>::CreateLambda([VM](){const USpawnParameters* P=VM.IsValid()?VM->SpawnParameters.Get():nullptr;return FText::AsNumber(P?P->GetPlannedInfrastructureActorCount():0);}))]
					]
				]
			]
		];

	TSharedRef<SWidget> DivisionsEditor = SNew(SScrollBox)
		+ SScrollBox::Slot()
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()[APSMenu::IconSectionHeading(EAPSMenuGlyph::Divisions,
				LOCTEXT("DivisionSetup", "DIVISION READINESS"), LOCTEXT("DivisionSetupHint", "Set the starting capability of each command branch."))]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 14.0f, 0.0f, 10.0f)
			[
				SNew(SUniformGridPanel).SlotPadding(FMargin(3.0f))
				+ SUniformGridPanel::Slot(0, 0)[PresetButton(LOCTEXT("PresetCivil", "CIVIL"), LOCTEXT("PresetCivilTip", "Population, logistics and administration."), [VM](){const USpawnParameters* P=VM.IsValid()?VM->SpawnParameters.Get():nullptr;return P&&P->ExplorationDivisionLevel==2&&P->IndustryDivisionLevel==4&&P->ScienceDivisionLevel==3&&P->CivilAffairsDivisionLevel==8&&P->MilitaryDivisionLevel==1&&P->FleetDivisionLevel==2;}, [ApplyDivisionPreset](){ApplyDivisionPreset(2,4,3,8,1,2);})]
				+ SUniformGridPanel::Slot(1, 0)[PresetButton(LOCTEXT("PresetScience", "SCIENCE"), LOCTEXT("PresetScienceTip", "Exploration and research priority."), [VM](){const USpawnParameters* P=VM.IsValid()?VM->SpawnParameters.Get():nullptr;return P&&P->ExplorationDivisionLevel==5&&P->IndustryDivisionLevel==3&&P->ScienceDivisionLevel==10&&P->CivilAffairsDivisionLevel==4&&P->MilitaryDivisionLevel==1&&P->FleetDivisionLevel==3;}, [ApplyDivisionPreset](){ApplyDivisionPreset(5,3,10,4,1,3);})]
				+ SUniformGridPanel::Slot(0, 1)[PresetButton(LOCTEXT("PresetExpansion", "EXPAND"), LOCTEXT("PresetExpansionTip", "Exploration, industry and fleet growth."), [VM](){const USpawnParameters* P=VM.IsValid()?VM->SpawnParameters.Get():nullptr;return P&&P->ExplorationDivisionLevel==8&&P->IndustryDivisionLevel==7&&P->ScienceDivisionLevel==4&&P->CivilAffairsDivisionLevel==5&&P->MilitaryDivisionLevel==4&&P->FleetDivisionLevel==7;}, [ApplyDivisionPreset](){ApplyDivisionPreset(8,7,4,5,4,7);})]
				+ SUniformGridPanel::Slot(1, 1)[PresetButton(LOCTEXT("PresetDefense", "DEFENSE"), LOCTEXT("PresetDefenseTip", "Military and fleet-command readiness."), [VM](){const USpawnParameters* P=VM.IsValid()?VM->SpawnParameters.Get():nullptr;return P&&P->ExplorationDivisionLevel==4&&P->IndustryDivisionLevel==6&&P->ScienceDivisionLevel==3&&P->CivilAffairsDivisionLevel==4&&P->MilitaryDivisionLevel==10&&P->FleetDivisionLevel==10;}, [ApplyDivisionPreset](){ApplyDivisionPreset(4,6,3,4,10,10);})]
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 6.0f)[ReadinessControl(LOCTEXT("ExplorationDivision", "EXPLORATION"), [VM](){return VM.IsValid()&&VM->SpawnParameters?VM->SpawnParameters->ExplorationDivisionLevel:0;}, [VM](int32 V){if(VM.IsValid()&&VM->SpawnParameters)VM->SpawnParameters->ExplorationDivisionLevel=V;})]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 6.0f)[ReadinessControl(LOCTEXT("IndustryDivision", "INDUSTRY"), [VM](){return VM.IsValid()&&VM->SpawnParameters?VM->SpawnParameters->IndustryDivisionLevel:0;}, [VM](int32 V){if(VM.IsValid()&&VM->SpawnParameters)VM->SpawnParameters->IndustryDivisionLevel=V;})]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 6.0f)[ReadinessControl(LOCTEXT("ScienceDivision", "SCIENCE / RESEARCH"), [VM](){return VM.IsValid()&&VM->SpawnParameters?VM->SpawnParameters->ScienceDivisionLevel:0;}, [VM](int32 V){if(VM.IsValid()&&VM->SpawnParameters)VM->SpawnParameters->ScienceDivisionLevel=V;})]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 6.0f)[ReadinessControl(LOCTEXT("CivilDivision", "CIVIL AFFAIRS"), [VM](){return VM.IsValid()&&VM->SpawnParameters?VM->SpawnParameters->CivilAffairsDivisionLevel:0;}, [VM](int32 V){if(VM.IsValid()&&VM->SpawnParameters)VM->SpawnParameters->CivilAffairsDivisionLevel=V;})]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 6.0f)[ReadinessControl(LOCTEXT("MilitaryDivision", "MILITARY"), [VM](){return VM.IsValid()&&VM->SpawnParameters?VM->SpawnParameters->MilitaryDivisionLevel:0;}, [VM](int32 V){if(VM.IsValid()&&VM->SpawnParameters)VM->SpawnParameters->MilitaryDivisionLevel=V;})]
			+ SVerticalBox::Slot().AutoHeight()[ReadinessControl(LOCTEXT("FleetDivision", "FLEET COMMAND"), [VM](){return VM.IsValid()&&VM->SpawnParameters?VM->SpawnParameters->FleetDivisionLevel:0;}, [VM](int32 V){if(VM.IsValid()&&VM->SpawnParameters)VM->SpawnParameters->FleetDivisionLevel=V;})]
		];

	TSharedRef<SWidgetSwitcher> EditorSwitcher = SAssignNew(CivilizationEditorSwitcher, SWidgetSwitcher)
		.WidgetIndex(CivilizationEditorSection)
		+ SWidgetSwitcher::Slot()[IdentityEditor]
		+ SWidgetSwitcher::Slot()[InfrastructureEditor]
		+ SWidgetSwitcher::Slot()[DivisionsEditor];

	TSharedRef<SWidget> Page = SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().Padding(28.0f, 18.0f, 28.0f, 3.0f)[BuildHeader(LOCTEXT("CivParameters", "CIVILIZATION GENERATION"))]
		+ SVerticalBox::Slot().AutoHeight().Padding(32.0f, 2.0f, 32.0f, 7.0f)
		[
			SNew(SBorder).BorderImage(&APSMenu::CivStatusBrush).Padding(FMargin(12.0f, 7.0f))
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
				[SNew(SBox).WidthOverride(18.0f).HeightOverride(18.0f)[SNew(SVectorMenuGlyph).Glyph(EAPSMenuGlyph::Civilization).Color(APSMenu::Cyan).StrokeWidth(1.2f)]]
				+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center).Padding(8.0f, 0.0f)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight()[SNew(STextBlock).Text(LOCTEXT("CivManifestHint", "ARRIVAL ARCHITECT  /  LIVE CIVILIZATION MANIFEST")).Font(APSMenu::Font("Bold", 9)).ColorAndOpacity(APSMenu::White)]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 1.0f, 0.0f, 0.0f)[SNew(STextBlock).Text(LOCTEXT("CivManifestSubHint", "Fleet and infrastructure values create persistent gameplay actors in the generated system.")).Font(APSMenu::Font("Regular", 8)).ColorAndOpacity(APSMenu::Muted)]
				]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Right)
					[
						SNew(STextBlock).Text_Lambda([this](){return SpawnSelectionLoadHandles.IsEmpty()?LOCTEXT("CivManifestReady", "MANIFEST READY"):LOCTEXT("CivManifestResolving", "RESOLVING ASSETS");})
						.Font(APSMenu::Font("Bold", 8)).ColorAndOpacity_Lambda([this](){return SpawnSelectionLoadHandles.IsEmpty()?APSMenu::Success:APSMenu::Amber;})
					]
					+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Right).Padding(0.0f, 1.0f, 0.0f, 0.0f)
					[SNew(STextBlock).Text_Lambda([VM](){const USpawnParameters* P=VM.IsValid()?VM->SpawnParameters.Get():nullptr;return FText::FromString(FString::Printf(TEXT("%d PHYSICAL ACTORS"),P?P->GetPlannedPhysicalActorCount():0));}).Font(APSMenu::Font("Bold", 11)).ColorAndOpacity(APSMenu::Amber)]
				]
			]
		]
		+ SVerticalBox::Slot().FillHeight(1.0f).Padding(26.0f, 8.0f, 26.0f, 6.0f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(0.66f).Padding(5.0f)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight().Padding(4.0f, 0.0f, 4.0f, 6.0f)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center)
					[SNew(STextBlock).Text(LOCTEXT("FoundingRegistryTitle", "FOUNDING ASSET REGISTRY")).Font(APSMenu::Font("Bold", 10)).ColorAndOpacity(APSMenu::White)]
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
					[SNew(STextBlock).Text(LOCTEXT("FoundingRegistryStatus", "5 CLASS SLOTS  /  PILOT VERIFIED")).Font(APSMenu::Font("Bold", 8)).ColorAndOpacity(APSMenu::Success)]
				]
				+ SVerticalBox::Slot().FillHeight(0.60f)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(4.0f)[BuildSpawnCard(EAPSStartAssetSlot::Character, LOCTEXT("Character", "CHARACTER"))]
					+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(4.0f)[BuildSpawnCard(EAPSStartAssetSlot::Spaceship, LOCTEXT("Spaceship", "SPACESHIP"))]
					+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(4.0f)[BuildSpawnCard(EAPSStartAssetSlot::SpaceStation, LOCTEXT("Station", "SPACE STATION"))]
					+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(4.0f)[BuildSpawnCard(EAPSStartAssetSlot::Headquarters, LOCTEXT("HQ", "HEADQUARTERS"))]
					+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(4.0f)[BuildSpawnCard(EAPSStartAssetSlot::Shipyard, LOCTEXT("Shipyard", "SHIPYARD"))]
				]
				+ SVerticalBox::Slot().FillHeight(0.40f).Padding(0.0f, 10.0f, 0.0f, 0.0f)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(4.0f)
					[InfoPanel(EAPSMenuGlyph::Civilization, LOCTEXT("CivilizationStatus", "CIVILIZATION"), LOCTEXT("CivilizationStatusHint", "Identity snapshot"), TAttribute<FText>::CreateLambda([VM](){ const USpawnParameters* P=VM.IsValid()?VM->SpawnParameters.Get():nullptr; return P ? FText::FromString(FString::Printf(TEXT("%s\n%s  /  %s\n%s  /  %s\nPopulation  %s  /  Tech T%d"), *P->CivilizationName, *APSMenu::EnumLabel(P->CivilizationArchetype), *APSMenu::EnumLabel(P->GovernmentType), *APSMenu::EnumLabel(P->EconomicSystem), *APSMenu::EnumLabel(P->SocietyType), *FText::AsNumber(P->FoundingPopulation).ToString(), P->TechnologyLevel)) : FText::GetEmpty(); }))]
					+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(4.0f)
					[InfoPanel(EAPSMenuGlyph::System, LOCTEXT("StarSystemInfo", "HOME SYSTEM"), LOCTEXT("StarSystemInfoHint", "Astronomical handoff"), TAttribute<FText>::CreateLambda([VM](){ const UGeneratedWorld* W=VM.IsValid()?VM->GeneratedWorld.Get():nullptr; return W ? FText::FromString(FString::Printf(TEXT("%s\n%s  /  %s\n%d planets\nHome  %s  /  %.0f KM"), *APSMenu::EnumLabel(W->PlanetarySystemType), *APSMenu::EnumLabel(W->StellarType), *APSMenu::EnumLabel(W->SpectralClass), W->PlanetsAmount, *APSMenu::EnumLabel(W->PlanetType), W->PlanetRadius)) : FText::GetEmpty(); }))]
					+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(4.0f)
					[InfoPanel(EAPSMenuGlyph::Infrastructure, LOCTEXT("Infrastructure", "DEPLOYMENT"), LOCTEXT("InfrastructureHint", "Physical actor manifest"), TAttribute<FText>::CreateLambda([VM](){ const USpawnParameters* P=VM.IsValid()?VM->SpawnParameters.Get():nullptr; return P ? FText::FromString(FString::Printf(TEXT("%d total actors\nFleet  %d ships\nInfrastructure  %d nodes\nPilot  Custom Gravity"), P->GetPlannedPhysicalActorCount(), P->StartingFleetSize, P->GetPlannedInfrastructureActorCount())) : FText::GetEmpty(); }))]
				]
			]
			+ SHorizontalBox::Slot().FillWidth(0.34f).Padding(5.0f)
			[
				APSMenu::ChamferPanel(
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight().Padding(1.0f, 0.0f, 1.0f, 9.0f)
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center)
						[
							SNew(SVerticalBox)
							+ SVerticalBox::Slot().AutoHeight()[SNew(STextBlock).Text(LOCTEXT("CivilizationConsole", "CONFIGURATION CONSOLE")).Font(APSMenu::Font("Bold", 11)).ColorAndOpacity(APSMenu::White)]
							+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 2.0f, 0.0f, 0.0f)[SNew(STextBlock).Text(LOCTEXT("CivilizationConsoleHint", "Tune the colony package before system handoff.")).Font(APSMenu::Font("Regular", 8)).ColorAndOpacity(APSMenu::Muted)]
						]
						+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
						[
							SNew(SBorder).BorderImage(&APSMenu::CivStatusBrush).Padding(FMargin(7.0f, 4.0f))
							[SNew(STextBlock).Text(LOCTEXT("CivilizationConsoleOnline", "ONLINE")).Font(APSMenu::Font("Bold", 8)).ColorAndOpacity(APSMenu::Success)]
						]
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 12.0f)
					[
						SNew(SUniformGridPanel).SlotPadding(FMargin(3.0f))
						+ SUniformGridPanel::Slot(0, 0)[EditorTab(0, EAPSMenuGlyph::Civilization, LOCTEXT("IdentityTab", "IDENTITY"))]
						+ SUniformGridPanel::Slot(1, 0)[EditorTab(1, EAPSMenuGlyph::Infrastructure, LOCTEXT("ManifestTab", "MANIFEST"))]
						+ SUniformGridPanel::Slot(2, 0)[EditorTab(2, EAPSMenuGlyph::Divisions, LOCTEXT("DivisionsTab", "DIVISIONS"))]
					]
					+ SVerticalBox::Slot().FillHeight(1.0f)[EditorSwitcher]
				, FMargin(16.0f), FLinearColor(0.05f, 0.34f, 0.43f, 0.95f), 1.2f)
			]
		]
		+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0.0f, 8.0f, 0.0f, 22.0f)
		[
			SNew(SBox).WidthOverride(500.0f)
			[
				SNew(SButton).ButtonStyle(&PrimaryButtonStyle).OnClicked(this, &SAPSMainMenuRoot::CommitCivilization)
				.IsEnabled_Lambda([this]() { return SpawnSelectionLoadHandles.IsEmpty(); })
				.ContentPadding(FMargin(24.0f, 13.0f))
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
					[
						SNew(STextBlock)
						.Text_Lambda([this]()
						{
							if (!SpawnSelectionLoadHandles.IsEmpty()) return LOCTEXT("LoadingSpawnSelections", "LOADING SELECTED ASSETS...");
							const USpawnParameters* P = ViewModel.IsValid() ? ViewModel->SpawnParameters.Get() : nullptr;
							return P ? FText::FromString(FString::Printf(TEXT("GENERATE CIVILIZATION  /  %d ACTORS  >"), P->GetPlannedPhysicalActorCount()))
								: LOCTEXT("GenerateWorld", "GENERATE CIVILIZATION  >");
						})
						.Justification(ETextJustify::Center).Font(APSMenu::Font("Bold", 18)).ColorAndOpacity(APSMenu::White)
					]
					+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0.0f, 3.0f, 0.0f, 0.0f)
					[SNew(STextBlock).Text(LOCTEXT("GenerateCivilizationSafety", "VALIDATED MANIFEST  /  TRANSACTIONAL SPAWN")).Font(APSMenu::Font("Bold", 8)).ColorAndOpacity(APSMenu::Cyan)]
				]
			]
		];

	return SNew(SOverlay)
		+ SOverlay::Slot()
		[SNew(SScaleBox).Stretch(EStretch::ScaleToFill)[SNew(SImage).Image(&BackgroundImage).ColorAndOpacity(FLinearColor(0.14f, 0.26f, 0.36f, 0.30f))]]
		+ SOverlay::Slot()
		[SNew(SBorder).BorderImage(FAppStyle::GetBrush("WhiteBrush")).BorderBackgroundColor(FLinearColor(0.0f, 0.008f, 0.016f, 0.62f))]
		+ SOverlay::Slot()[Page];
}

void SAPSMainMenuRoot::LoadVisualResources()
{
	APSMenu::DisplayFont = LoadObject<UFont>(nullptr,
		TEXT("/Game/APS/APS_ALPHA/UI/Fonts/Orbitron_Bold_Font.Orbitron_Bold_Font"));
	APSMenu::BodyFont = LoadObject<UFont>(nullptr,
		TEXT("/Game/APS/APS_ALPHA/UI/Fonts/Orbitron_Medium_Font.Orbitron_Medium_Font"));
	if (AMainMenuController* PC = Controller.Get())
	{
		PC->HoldSlateResource(APSMenu::DisplayFont.Get());
		PC->HoldSlateResource(APSMenu::BodyFont.Get());
	}

	auto Load = [this](FSlateBrush& Brush, const TCHAR* Path)
	{
		UTexture2D* Texture = LoadObject<UTexture2D>(nullptr, Path);
		if (AMainMenuController* PC = Controller.Get()) PC->HoldSlateResource(Texture);
		Brush.SetResourceObject(Texture);
		Brush.ImageSize = Texture
			? FVector2D(static_cast<float>(Texture->GetSizeX()), static_cast<float>(Texture->GetSizeY()))
			: FVector2D(640.0f, 420.0f);
		Brush.DrawAs = ESlateBrushDrawType::Image;
	};
	Load(SystemImage, TEXT("/Game/APS/APS_ALPHA/UI/Images/T_System.T_System"));
	Load(PlanetImage, TEXT("/Game/APS/APS_ALPHA/UI/Images/planet_realistic.planet_realistic"));
	Load(GalaxyImage, TEXT("/Game/APS/APS_ALPHA/UI/Images/spiral_galaxy.spiral_galaxy"));
	Load(ClusterImage, TEXT("/Game/APS/APS_ALPHA/UI/Images/I_Cluster.I_Cluster"));
	Load(CivilizationImage, TEXT("/Game/APS/APS_ALPHA/UI/Images/circular_galaxy.circular_galaxy"));
	Load(BackgroundImage, TEXT("/Game/APS/APS_ALPHA/UI/Images/Screenshot_2023.Screenshot_2023"));
}

void SAPSMainMenuRoot::BeginAuxiliaryMenuLoad()
{
	SettingsPanelClass = TSoftClassPtr<UUserWidget>(FSoftObjectPath(
		TEXT("/Game/APS/APS_ALPHA/UI/MainMenu/WBP_SettingsPanel.WBP_SettingsPanel_C")));
	ProfilePanelClass = TSoftClassPtr<UUserWidget>(FSoftObjectPath(
		TEXT("/Game/APS/APS_APOSFERA_SPACETRIPS/MBLS/Widgets/MainMenu/APS_WB_ProfileMenu.APS_WB_ProfileMenu_C")));

	TArray<FSoftObjectPath> Paths;
	Paths.Add(SettingsPanelClass.ToSoftObjectPath());
	Paths.Add(ProfilePanelClass.ToSoftObjectPath());
	AuxiliaryMenuLoadHandle = UAssetManager::GetStreamableManager().RequestAsyncLoad(
		Paths,
		FStreamableDelegate::CreateSP(this, &SAPSMainMenuRoot::OnAuxiliaryMenuLoaded),
		FStreamableManager::AsyncLoadHighPriority);
}

void SAPSMainMenuRoot::OnAuxiliaryMenuLoaded()
{
	if (AMainMenuController* PC = Controller.Get())
	{
		PC->HoldSlateResource(SettingsPanelClass.Get());
		PC->HoldSlateResource(ProfilePanelClass.Get());
	}
	AuxiliaryMenuLoadHandle.Reset();

	if (!ContentHost.IsValid())
	{
		return;
	}
	if (CurrentPage == EAPSMenuPage::Settings)
	{
		ContentHost->SetContent(BuildSettingsPage());
	}
	else if (CurrentPage == EAPSMenuPage::Profile)
	{
		ContentHost->SetContent(BuildProfilePage());
	}
}

FReply SAPSMainMenuRoot::Back()
{
	if (CurrentPage == EAPSMenuPage::Landing) return FReply::Handled();
	EAPSMenuPage TargetPage = EAPSMenuPage::ChoosePath;
	if (CurrentPage == EAPSMenuPage::ChoosePath) TargetPage = EAPSMenuPage::Landing;
	else if (CurrentPage == EAPSMenuPage::Civilization) TargetPage = EAPSMenuPage::AstronomicalGeneration;
	else if (CurrentPage == EAPSMenuPage::Profile || CurrentPage == EAPSMenuPage::Settings)
	{
		TargetPage = PreviousPage == CurrentPage ? EAPSMenuPage::Landing : PreviousPage;
	}
	Navigate(TargetPage);
	return FReply::Handled();
}

FReply SAPSMainMenuRoot::OpenChoosePath() { Navigate(EAPSMenuPage::ChoosePath); return FReply::Handled(); }
FReply SAPSMainMenuRoot::StartSingleGame() { if (AMainMenuController* PC = Controller.Get()) PC->LaunchSingleGame(); return FReply::Handled(); }
FReply SAPSMainMenuRoot::OpenExistingWorlds() { Navigate(EAPSMenuPage::ExistingWorlds); return FReply::Handled(); }
FReply SAPSMainMenuRoot::OpenAstronomicalGeneration(EAstroPreviewFocus Focus, EAPSGenerationRoute Route)
{
	if (ViewModel.IsValid())
	{
		// Establish the route and its requested hierarchy focus before constructing
		// the panel. This prevents a one-frame flash of the previous scope and makes
		// Generate Civilization enter directly at PLANET as designed.
		ViewModel->SetGenerationRoute(Route);
		ViewModel->SetPreviewFocus(Focus);
	}
	Navigate(EAPSMenuPage::AstronomicalGeneration);
	return FReply::Handled();
}
#if WITH_DEV_AUTOMATION_TESTS
void SAPSMainMenuRoot::OpenChoosePathForAutomation()
{
	Navigate(EAPSMenuPage::ChoosePath);
}

void SAPSMainMenuRoot::GetChoosePathDiagnosticsForAutomation(int32& OutCardCount,
	int32& OutProceduralVisualCount, int32& OutStaticTextureResourceCount) const
{
	OutCardCount = 0;
	for (const TWeakPtr<SButton>& WeakButton : ChoosePathCardButtons)
	{
		OutCardCount += WeakButton.IsValid() ? 1 : 0;
	}
	OutProceduralVisualCount = ChoosePathProceduralVisualCount;
	OutStaticTextureResourceCount = ChoosePathStaticTextureResourceCount;
}

bool SAPSMainMenuRoot::FocusChoosePathCardForAutomation(const int32 CardIndex)
{
	if (!ChoosePathCardButtons.IsValidIndex(CardIndex)
		|| !FSlateApplication::IsInitialized())
	{
		return false;
	}
	const TSharedPtr<SButton> Button = ChoosePathCardButtons[CardIndex].Pin();
	return Button.IsValid() && Button->IsEnabled()
		&& FSlateApplication::Get().SetKeyboardFocus(Button, EFocusCause::SetDirectly);
}

bool SAPSMainMenuRoot::HoverChoosePathCardForAutomation(const int32 CardIndex)
{
	if (!ChoosePathCardButtons.IsValidIndex(CardIndex)
		|| !FSlateApplication::IsInitialized())
	{
		return false;
	}
	const TSharedPtr<SButton> Button = ChoosePathCardButtons[CardIndex].Pin();
	if (!Button.IsValid() || !Button->IsEnabled())
	{
		return false;
	}

	FSlateApplication& SlateApplication = FSlateApplication::Get();
	const FVector2D PreviousPosition = SlateApplication.GetCursorPos();
	const FGeometry& Geometry = Button->GetCachedGeometry();
	const FVector2D ScreenPosition = Geometry.LocalToAbsolute(Geometry.GetLocalSize() * 0.5f);
	SlateApplication.SetCursorPos(ScreenPosition);
	const FPointerEvent PointerEvent(
		0, FSlateApplication::CursorPointerIndex, ScreenPosition, PreviousPosition,
		SlateApplication.GetPressedMouseButtons(), EKeys::Invalid, 0.0f,
		FModifierKeysState());
	SlateApplication.ProcessMouseMoveEvent(PointerEvent, true);
	return true;
}

bool SAPSMainMenuRoot::ClearChoosePathCardInteractionsForAutomation()
{
	if (!FSlateApplication::IsInitialized())
	{
		return false;
	}

	const FGeometry& RootGeometry = GetCachedGeometry();
	const FVector2D RootSize = RootGeometry.GetLocalSize();
	if (RootSize.X <= 1.0f || RootSize.Y <= 1.0f)
	{
		return false;
	}

	// Derive the neutral pointer position from the actual laid-out cards instead
	// of assuming a resolution or DPI scale. The midpoint above their top edge is
	// inside the non-interactive title/header band at every supported layout.
	float FirstCardTop = RootSize.Y;
	bool bFoundCard = false;
	for (const TWeakPtr<SButton>& WeakButton : ChoosePathCardButtons)
	{
		const TSharedPtr<SButton> Button = WeakButton.Pin();
		if (!Button.IsValid() || Button->GetCachedGeometry().GetLocalSize().Y <= 1.0f)
		{
			continue;
		}
		const FVector2D CardTopLeft = RootGeometry.AbsoluteToLocal(
			Button->GetCachedGeometry().LocalToAbsolute(FVector2D::ZeroVector));
		FirstCardTop = FMath::Min(FirstCardTop, static_cast<float>(CardTopLeft.Y));
		bFoundCard = true;
	}
	if (!bFoundCard || FirstCardTop <= 4.0f)
	{
		return false;
	}

	FSlateApplication& SlateApplication = FSlateApplication::Get();
	SlateApplication.ClearKeyboardFocus(EFocusCause::Cleared);
	const FVector2D PreviousPosition = SlateApplication.GetCursorPos();
	const FVector2D SafeScreenPosition = RootGeometry.LocalToAbsolute(FVector2D(
		RootSize.X * 0.5f,
		FMath::Clamp(FirstCardTop * 0.5f, 2.0f, FirstCardTop - 2.0f)));
	SlateApplication.SetCursorPos(SafeScreenPosition);
	const FPointerEvent PointerEvent(
		0, FSlateApplication::CursorPointerIndex, SafeScreenPosition, PreviousPosition,
		SlateApplication.GetPressedMouseButtons(), EKeys::Invalid, 0.0f,
		FModifierKeysState());
	SlateApplication.ProcessMouseMoveEvent(PointerEvent, true);
	return true;
}

bool SAPSMainMenuRoot::GetChoosePathCardInteractionForAutomation(const int32 CardIndex,
	bool& bOutHovered, bool& bOutFocused) const
{
	bOutHovered = false;
	bOutFocused = false;
	if (!ChoosePathCardButtons.IsValidIndex(CardIndex))
	{
		return false;
	}
	const TSharedPtr<SButton> Button = ChoosePathCardButtons[CardIndex].Pin();
	if (!Button.IsValid())
	{
		return false;
	}
	bOutHovered = Button->IsHovered();
	bOutFocused = Button->HasKeyboardFocus();
	return true;
}

bool SAPSMainMenuRoot::GetChoosePathCardNormalizedRectForAutomation(
	const int32 CardIndex, FSlateRect& OutRect) const
{
	if (!ChoosePathCardButtons.IsValidIndex(CardIndex))
	{
		return false;
	}
	const TSharedPtr<SButton> Button = ChoosePathCardButtons[CardIndex].Pin();
	if (!Button.IsValid())
	{
		return false;
	}

	const FGeometry& RootGeometry = GetCachedGeometry();
	const FGeometry& CardGeometry = Button->GetCachedGeometry();
	const FVector2D RootSize = RootGeometry.GetLocalSize();
	if (RootSize.X <= 1.0f || RootSize.Y <= 1.0f
		|| CardGeometry.GetLocalSize().X <= 1.0f || CardGeometry.GetLocalSize().Y <= 1.0f)
	{
		return false;
	}
	const FVector2D TopLeft = RootGeometry.AbsoluteToLocal(
		CardGeometry.LocalToAbsolute(FVector2D::ZeroVector));
	const FVector2D BottomRight = RootGeometry.AbsoluteToLocal(
		CardGeometry.LocalToAbsolute(CardGeometry.GetLocalSize()));
	OutRect = FSlateRect(
		TopLeft.X / RootSize.X, TopLeft.Y / RootSize.Y,
		BottomRight.X / RootSize.X, BottomRight.Y / RootSize.Y);
	return FMath::IsFinite(OutRect.Left) && FMath::IsFinite(OutRect.Top)
		&& FMath::IsFinite(OutRect.Right) && FMath::IsFinite(OutRect.Bottom);
}

void SAPSMainMenuRoot::OpenAstronomicalGenerationForAutomation(
	EAstroPreviewFocus Focus, EAPSGenerationRoute Route)
{
	OpenAstronomicalGeneration(Focus, Route);
}

bool SAPSMainMenuRoot::CommitSurfaceControlForAutomation(
	const EAPSGenerationSurfaceControl Control, const double Value)
{
	return WorldGenerationPanel.IsValid()
		&& WorldGenerationPanel->CommitSurfaceControlForAutomation(Control, Value);
}

double SAPSMainMenuRoot::GetSurfaceControlValueForAutomation(
	const EAPSGenerationSurfaceControl Control) const
{
	return WorldGenerationPanel.IsValid()
		? WorldGenerationPanel->GetSurfaceControlValueForAutomation(Control)
		: TNumericLimits<double>::Lowest();
}
#endif
void SAPSMainMenuRoot::ContinueAstronomicalGeneration()
{
	if (!ViewModel.IsValid()) return;
	if (ViewModel->GetGenerationRoute() == EAPSGenerationRoute::Civilization)
	{
		Navigate(EAPSMenuPage::Civilization);
	}
	else
	{
		ViewModel->CommitAndOpenLevel();
	}
}
FReply SAPSMainMenuRoot::OpenCivilization() { Navigate(EAPSMenuPage::Civilization); return FReply::Handled(); }
FReply SAPSMainMenuRoot::OpenProfile() { Navigate(EAPSMenuPage::Profile); return FReply::Handled(); }
FReply SAPSMainMenuRoot::OpenSettings() { Navigate(EAPSMenuPage::Settings); return FReply::Handled(); }
FReply SAPSMainMenuRoot::CommitCivilization() { if (ViewModel.IsValid()) ViewModel->CommitAndOpenLevel(); return FReply::Handled(); }
FReply SAPSMainMenuRoot::ContinueExistingWorld() { if (SelectedWorld.IsValid()) if (AMainMenuController* PC = Controller.Get()) PC->LoadWorldSlot(SelectedWorld->SaveFileName); return FReply::Handled(); }
FReply SAPSMainMenuRoot::SelectExistingWorld(TSharedPtr<FAPSExistingWorldEntry> Entry) { SelectedWorld = Entry; RebuildExistingWorldDetails(); return FReply::Handled(); }
FReply SAPSMainMenuRoot::QuitGame() { if (AMainMenuController* PC = Controller.Get()) UKismetSystemLibrary::QuitGame(PC, PC, EQuitPreference::Quit, false); return FReply::Handled(); }

void SAPSMainMenuRoot::OnWorldSearchChanged(const FText& SearchText)
{
	WorldSearch = SearchText.ToString();
	ExistingWorldPage = 0;
	RebuildExistingWorldGrid();
}

FReply SAPSMainMenuRoot::CycleSpawnClass(EAPSStartAssetSlot Slot, int32 Direction)
{
	const TArray<TSoftClassPtr<AActor>>* Options = SpawnClassOptions.Find(Slot);
	if (!Options || Options->Num() == 0) return FReply::Handled();
	int32& Index = SpawnClassIndices.FindOrAdd(Slot);
	Index = (Index + Direction + Options->Num()) % Options->Num();
	ApplySpawnClassSelection(Slot);
	return FReply::Handled();
}

FReply SAPSMainMenuRoot::SelectSpawnClass(EAPSStartAssetSlot Slot, int32 OptionIndex)
{
	const TArray<TSoftClassPtr<AActor>>* Options = SpawnClassOptions.Find(Slot);
	if (!Options || !Options->IsValidIndex(OptionIndex))
	{
		return FReply::Handled();
	}
	SpawnClassIndices.FindOrAdd(Slot) = OptionIndex;
	ApplySpawnClassSelection(Slot);
	Invalidate(EInvalidateWidgetReason::Paint);
	return FReply::Handled();
}

#undef LOCTEXT_NAMESPACE
