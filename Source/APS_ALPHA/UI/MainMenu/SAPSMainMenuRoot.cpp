#include "SAPSMainMenuRoot.h"
#include "Widgets/Layout/SGridPanel.h"
#include "APS_ALPHA/Core/Audio/APSAudioSubsystem.h"
#include "APS_ALPHA/UI/Settings/SAPSAudioSettings.h"
#include "APS_ALPHA/UI/Settings/SAPSSettingsPage.h"
#include "APS_ALPHA/UI/Settings/APSAudioSettingsBridge.h"
#include "Engine/World.h"
#include "APS_ALPHA/UI/Style/APSUINumber.h"
#include "SAPSChamferedOverlay.h"

#include "APS_ALPHA/Core/Controllers/MainMenuController.h"
#include "APS_ALPHA/Core/Instances/MainGameplayInstance.h"
#include "APS_ALPHA/UI/MainMenu/APSStartAssetFilter.h"
#include "APS_ALPHA/UI/MainMenu/APSUIThumbnails.h"
#include "APS_ALPHA/UI/MainMenu/SAPSWorldSchemePreview.h"
#include "APS_ALPHA/UI/Style/APSMenuChrome.h"
#include "APS_ALPHA/UI/Style/APSUITheme.h"
#include "Brushes/SlateColorBrush.h"
#include "Brushes/SlateImageBrush.h"
#include "Containers/Ticker.h"
#include "UnrealClient.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "APS_ALPHA/Core/Loading/APSAuthoredLevelLaunchSubsystem.h"
#include "APS_ALPHA/Core/Enums/CharSpawnPlace.h"
#include "APS_ALPHA/Core/Enums/OrbitHeight.h"
#include "APS_ALPHA/Core/Interfaces/ItemInfoInterface.h"
#include "APS_ALPHA/Core/Model/SpawnParameters.h"
#include "APS_ALPHA/Core/Model/GeneratedWorld.h"
#include "APS_ALPHA/Core/Saves/GameSave.h"
#include "APS_ALPHA/Core/Saves/GeneratedWorldData.h"
#include "APS_ALPHA/Pawns/Characters/CustomGravityCharacter.h"
#include "APS_ALPHA/Pawns/Spaceships/Spaceship.h"
#include "APS_ALPHA/Actors/Tech/SpaceHeadquarters.h"
#include "APS_ALPHA/Actors/Tech/SpaceShipyard.h"
#include "APS_ALPHA/Actors/Tech/SpaceStation.h"
#include "APS_ALPHA/UI/MainMenu/SWorldGenerationPanel.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Blueprint/BlueprintSupport.h"
#include "Engine/Blueprint.h"
#include "Engine/Engine.h"
#include "Engine/Texture2D.h"
#include "Engine/Font.h"
#include "Engine/AssetManager.h"
#include "Engine/GameInstance.h"
#include "Engine/StreamableManager.h"
#include "Blueprint/UserWidget.h"
#include "Framework/Application/SlateApplication.h"
#include "HAL/IConsoleManager.h"
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
#include "Styling/StyleDefaults.h"
#include "Widgets/Layout/SWrapBox.h"
#include "Widgets/SNullWidget.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SSearchBox.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Input/SMenuAnchor.h"
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
#include "Widgets/SOverlay.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "APSMainMenuRoot"

namespace
{
	UAPSAuthoredLevelLaunchSubsystem* GetAuthoredLaunch(const AMainMenuController* Controller)
	{
		return Controller && Controller->GetGameInstance()
			? Controller->GetGameInstance()->GetSubsystem<UAPSAuthoredLevelLaunchSubsystem>()
			: nullptr;
	}

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
				{
					// Head and shoulders spanning -0.76..0.80 of the radius: centred on the box (Rio 03.10: the old
					// full-ellipse body hung 1.3 radii down and the person sat low in its badge).
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

	// One fixed cut in Slate units keeps adjacent cards geometrically identical.
	// Deriving it from each card's height made large and compact neighbours expose
	// different corner wedges even when their perimeter stroke had the same width.
	// Shared with astronomical panels and the full child-content stencil.

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

	/** Rio 06.10, the Observatory menu: dark bands fading in from the top and the bottom edge keep the title and the
	 * button row readable over the galaxy, which stays clear in the middle. */
	class SObservatoryVeil final : public SLeafWidget
	{
	public:
		SLATE_BEGIN_ARGS(SObservatoryVeil) : _LeftShade(false) {}
			/** NEW WORLD: also a band from the left edge under the page's text. */
			SLATE_ARGUMENT(bool, LeftShade)
		SLATE_END_ARGS()

		void Construct(const FArguments& InArgs)
		{
			bLeftShade = InArgs._LeftShade;
			SetVisibility(EVisibility::HitTestInvisible);
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
			if (Size.X <= 1.0f || Size.Y <= 1.0f)
			{
				return LayerId;
			}
			// Piecewise strips fade from the edge inwards without an opaque panel between the player and the scene.
			constexpr int32 Bands = 14;
			// Rio 06.10: Classic keeps its deep teal; the other themes darken with their own darkest tone.
			const FLinearColor VeilTone = APSUITheme::Current() == EAPSUITheme::Classic
				? FLinearColor(0.002f, 0.012f, 0.020f, 1.0f) : APSUITheme::Palette().InsetFill.CopyWithNewOpacity(1.0f);
			const auto Veil = [&](const double Height, const float MaxAlpha, const bool bFromBottom)
			{
				for (int32 Band = 0; Band < Bands; ++Band)
				{
					const double T0 = static_cast<double>(Band) / Bands;
					const double T1 = static_cast<double>(Band + 1) / Bands;
					const float Alpha = FMath::Square(1.0f - static_cast<float>(T0)) * MaxAlpha;
					const double Top = bFromBottom ? Size.Y - Height * T1 : Height * T0;
					FSlateDrawElement::MakeBox(OutDrawElements, LayerId,
						Geometry.ToPaintGeometry(FVector2D(Size.X, Height * (T1 - T0) + 1.0),
							FSlateLayoutTransform(FVector2D(0.0, Top))),
						FAppStyle::GetBrush("WhiteBrush"), ESlateDrawEffect::None,
						VeilTone.CopyWithNewOpacity(Alpha));
				}
			};
			Veil(Size.Y * 0.24, 0.55f, false);
			Veil(Size.Y * 0.30, 0.70f, true);
			if (bLeftShade)
			{
				const double Width = Size.X * 0.50;
				for (int32 Band = 0; Band < Bands; ++Band)
				{
					const double T0 = static_cast<double>(Band) / Bands;
					const double T1 = static_cast<double>(Band + 1) / Bands;
					const float Alpha = FMath::Square(1.0f - static_cast<float>(T0)) * 0.78f;
					FSlateDrawElement::MakeBox(OutDrawElements, LayerId,
						Geometry.ToPaintGeometry(FVector2D(Width * (T1 - T0) + 1.0, Size.Y),
							FSlateLayoutTransform(FVector2D(Width * T0, 0.0))),
						FAppStyle::GetBrush("WhiteBrush"), ESlateDrawEffect::None,
						VeilTone.CopyWithNewOpacity(Alpha));
				}
			}
			return LayerId;
		}

	private:
		bool bLeftShade{false};
	};

	/**
	 * Rio 06.10, NEW WORLD's picture of the picked path ("instead of this circle we should render something nice"):
	 * CIVILIZATION is a home system with its planets on their orbits and the home world ringed; SPACE is a dense star
	 * cluster turning slowly. Code-drawn, about six hundred boxes and lines at 30 frames a second, no textures.
	 */
	class SNewWorldVisual final : public SLeafWidget
	{
	public:
		SLATE_BEGIN_ARGS(SNewWorldVisual) {}
			SLATE_ATTRIBUTE(EAPSNewWorldPath, Path)
		SLATE_END_ARGS()

		void Construct(const FArguments& InArgs)
		{
			Path = InArgs._Path;
			SetVisibility(EVisibility::HitTestInvisible);
			// The cluster's stars: a fixed seed, a soft gaussian ball, a few warm and a few blue.
			FRandomStream Stream(20261006);
			const auto Gauss = [&Stream]()
			{
				const float U = FMath::Max(Stream.GetFraction(), 1.0e-4f);
				return FMath::Sqrt(-2.0f * FMath::Loge(U)) * FMath::Cos(UE_TWO_PI * Stream.GetFraction());
			};
			for (int32 Index = 0; Index < 520; ++Index)
			{
				ClusterStars.Add(FVector3f(Gauss(), Gauss(), Gauss()) * 0.33f);
				const float Kind = Stream.GetFraction();
				ClusterColours.Add(Kind < 0.18f ? FLinearColor(1.0f, 0.72f, 0.52f) : Kind < 0.45f ? FLinearColor(0.66f, 0.80f, 1.0f)
					: FLinearColor(0.93f, 0.96f, 1.0f));
				ClusterSizes.Add(Stream.GetFraction() < 0.92f ? 1.6f : 2.8f);
			}
			RegisterActiveTimer(1.0f / 30.0f, FWidgetActiveTimerDelegate::CreateSP(this, &SNewWorldVisual::Animate));
		}

		virtual FVector2D ComputeDesiredSize(float) const override
		{
			return FVector2D(480.0f, 320.0f);
		}

		virtual int32 OnPaint(const FPaintArgs&, const FGeometry& Geometry, const FSlateRect&,
			FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool) const override
		{
			const FVector2D Size = Geometry.GetLocalSize();
			if (Size.X < 40.0f || Size.Y < 40.0f)
			{
				return LayerId;
			}
			static const FSlateRoundedBoxBrush Disc(FLinearColor::White);
			const FLinearColor Tint = InWidgetStyle.GetColorAndOpacityTint();
			const float Line = Geometry.GetAccumulatedLayoutTransform().GetScale();
			const FVector2D Centre = Size * 0.5;
			// The drawing keeps a 1.6:1 frame whatever the slot's shape.
			const float Unit = static_cast<float>(FMath::Min(Size.X / 1.6, Size.Y));
			const bool bClassicTheme = APSUITheme::Current() == EAPSUITheme::Classic;
			const FLinearColor Cyan = bClassicTheme ? FLinearColor(0.055f, 0.67f, 0.84f, 1.0f) : APSUITheme::Palette().Highlight;
			const FLinearColor Amber = bClassicTheme ? FLinearColor(0.89f, 0.46f, 0.012f, 1.0f) : APSUITheme::Palette().Action;
			const auto DiscAt = [&](const FVector2D& At, const float Radius, const FLinearColor& Colour, const int32 Layer)
			{
				FSlateDrawElement::MakeBox(OutDrawElements, Layer, Geometry.ToPaintGeometry(FVector2D(Radius * 2.0f),
					FSlateLayoutTransform(At - FVector2D(Radius))), &Disc, ESlateDrawEffect::None, Colour * Tint);
			};
			const auto Ellipse = [&](const FVector2D& At, const float RadiusX, const float RadiusY, const float Rotation,
				const FLinearColor& Colour, const bool bDashed, const int32 Layer)
			{
				constexpr int32 Steps = 96;
				const float CosR = FMath::Cos(Rotation);
				const float SinR = FMath::Sin(Rotation);
				const auto Point = [&](const int32 Step)
				{
					const float Angle = UE_TWO_PI * static_cast<float>(Step) / Steps;
					const FVector2D P(FMath::Cos(Angle) * RadiusX, FMath::Sin(Angle) * RadiusY);
					return At + FVector2D(P.X * CosR - P.Y * SinR, P.X * SinR + P.Y * CosR);
				};
				if (bDashed)
				{
					for (int32 Step = 0; Step < Steps; Step += 2)
					{
						FSlateDrawElement::MakeLines(OutDrawElements, Layer, Geometry.ToPaintGeometry(),
							TArray<FVector2D>{Point(Step), Point(Step + 1)}, ESlateDrawEffect::None, Colour * Tint, true, Line);
					}
					return;
				}
				TArray<FVector2D> Points;
				Points.Reserve(Steps + 1);
				for (int32 Step = 0; Step <= Steps; ++Step)
				{
					Points.Add(Point(Step));
				}
				FSlateDrawElement::MakeLines(OutDrawElements, Layer, Geometry.ToPaintGeometry(), Points,
					ESlateDrawEffect::None, Colour * Tint, true, Line);
			};

			if (Path.Get(EAPSNewWorldPath::Civilization) == EAPSNewWorldPath::Space)
			{
				// A soft core, then the stars turning about the vertical axis, nearer ones a little larger.
				DiscAt(Centre, Unit * 0.30f, FLinearColor(1.0f, 0.95f, 0.88f, 0.035f), LayerId);
				DiscAt(Centre, Unit * 0.17f, FLinearColor(1.0f, 0.95f, 0.88f, 0.07f), LayerId);
				DiscAt(Centre, Unit * 0.07f, FLinearColor(1.0f, 0.97f, 0.92f, 0.14f), LayerId);
				const float Turn = Seconds * 0.09f;
				const float CosT = FMath::Cos(Turn);
				const float SinT = FMath::Sin(Turn);
				constexpr float Tilt = 0.35f;
				for (int32 Index = 0; Index < ClusterStars.Num(); ++Index)
				{
					const FVector3f& Star = ClusterStars[Index];
					const float X = Star.X * CosT + Star.Z * SinT;
					const float Z = -Star.X * SinT + Star.Z * CosT;
					const float Y = Star.Y * FMath::Cos(Tilt) - Z * FMath::Sin(Tilt);
					const float Depth = Z * FMath::Cos(Tilt) + Star.Y * FMath::Sin(Tilt);
					const float Radius = ClusterSizes[Index] * FMath::Clamp(1.0f + Depth * 0.6f, 0.6f, 1.5f) * 0.5f;
					DiscAt(Centre + FVector2D(X, Y) * Unit, Radius, ClusterColours[Index].CopyWithNewOpacity(0.85f), LayerId + 1);
				}
				// Corner brackets frame the cluster, as the map marks a target.
				const float Half = Unit * 0.46f;
				const float Arm = Unit * 0.07f;
				for (const FVector2D Corner : {FVector2D(-1.0, -1.0), FVector2D(1.0, -1.0), FVector2D(-1.0, 1.0), FVector2D(1.0, 1.0)})
				{
					const FVector2D At = Centre + Corner * Half;
					FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 2, Geometry.ToPaintGeometry(),
						TArray<FVector2D>{At - FVector2D(Corner.X * Arm, 0.0), At, At - FVector2D(0.0, Corner.Y * Arm)},
						ESlateDrawEffect::None, Amber * Tint, true, 2.0f * Line);
				}
				return LayerId + 2;
			}

			// The home system: three orbits, the outer one dashed, and the star with two faint shells.
			constexpr float Rotation = -0.14f;
			const float Orbits[3] = {Unit * 0.30f, Unit * 0.50f, Unit * 0.74f};
			const float Speeds[3] = {0.34f, 0.2f, 0.11f};
			const float Phases[3] = {0.6f, 2.5f, 4.3f};
			const float Sizes[3] = {Unit * 0.017f, Unit * 0.026f, Unit * 0.022f};
			const FLinearColor Worlds[3] = {FLinearColor(0.55f, 0.75f, 1.0f), FLinearColor(0.36f, 0.86f, 0.5f), FLinearColor(0.94f, 0.76f, 0.56f)};
			for (int32 Orbit = 0; Orbit < 3; ++Orbit)
			{
				Ellipse(Centre, Orbits[Orbit], Orbits[Orbit] * 0.36f, Rotation, Cyan.CopyWithNewOpacity(0.55f - 0.12f * Orbit), Orbit == 2, LayerId);
			}
			DiscAt(Centre, Unit * 0.10f, FLinearColor(1.0f, 0.88f, 0.70f, 0.10f), LayerId + 1);
			DiscAt(Centre, Unit * 0.066f, FLinearColor(1.0f, 0.90f, 0.75f, 0.22f), LayerId + 1);
			DiscAt(Centre, Unit * 0.042f, FLinearColor(1.0f, 0.96f, 0.88f, 1.0f), LayerId + 2);
			const float CosR = FMath::Cos(Rotation);
			const float SinR = FMath::Sin(Rotation);
			for (int32 Orbit = 0; Orbit < 3; ++Orbit)
			{
				const float Angle = Phases[Orbit] + Seconds * Speeds[Orbit];
				const FVector2D P(FMath::Cos(Angle) * Orbits[Orbit], FMath::Sin(Angle) * Orbits[Orbit] * 0.36f);
				const FVector2D At = Centre + FVector2D(P.X * CosR - P.Y * SinR, P.X * SinR + P.Y * CosR);
				DiscAt(At, Sizes[Orbit], Worlds[Orbit], LayerId + 2);
				if (Orbit == 1)
				{
					// The home world: an amber ring and a leader to the caption in the top-left corner.
					Ellipse(At, Sizes[Orbit] * 1.9f, Sizes[Orbit] * 1.9f, 0.0f, Amber, false, LayerId + 2);
					const FVector2D Caption(Size.X * 0.06, Size.Y * 0.16);
					FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 1, Geometry.ToPaintGeometry(),
						TArray<FVector2D>{Caption, At}, ESlateDrawEffect::None, Amber.CopyWithNewOpacity(0.6f) * Tint, true, Line);
				}
			}
			return LayerId + 2;
		}

	private:
		EActiveTimerReturnType Animate(double, float DeltaTime)
		{
			Seconds = FMath::Fmod(Seconds + DeltaTime, 4096.0f);
			Invalidate(EInvalidateWidgetReason::Paint);
			return EActiveTimerReturnType::Continue;
		}

		TAttribute<EAPSNewWorldPath> Path;
		TArray<FVector3f> ClusterStars;
		TArray<FLinearColor> ClusterColours;
		TArray<float> ClusterSizes;
		float Seconds{0.0f};
	};
}

namespace APSMenu
{
	// Landing and Choose Path sit over the real generated astronomical scene.
	// Keep only a faint readability veil here; the old turquoise wash hid every
	// small star in the live background.
	FLinearColor SRGB(uint8 R, uint8 G, uint8 B, uint8 A = 255)
	{
		return FLinearColor::FromSRGBColor(FColor(R, G, B, A));
	}
	// Rio 06.10: the palette is the active interface theme's (APSUITheme), copied in by ApplyTheme() when the menu
	// opens and after every theme switch; the values below are Classic, the look until 06.10.
	static FLinearColor Background = SRGB(2, 7, 11, 20);
	static FLinearColor Panel = SRGB(8, 32, 42, 242);
	static FLinearColor PanelSoft = SRGB(6, 19, 26, 232);
	static FLinearColor Cyan = SRGB(67, 214, 236);
	static FLinearColor CyanDim = SRGB(27, 83, 96, 178);
	static FLinearColor Amber = SRGB(242, 181, 29);
	static FLinearColor White = SRGB(234, 246, 248);
	static FLinearColor Muted = SRGB(88, 114, 122);
	static FLinearColor Success = SRGB(100, 214, 166);
	static FLinearColor CivPanel = SRGB(6, 19, 26, 249);
	static FLinearColor CivRaised = SRGB(12, 41, 52, 251);
	static FLinearColor CivControl = SRGB(8, 32, 42, 252);
	// Rio 03.10 (world browser: "tiny dim grey labels"): the in-game chrome's lighter secondary grey, readable on the
	// dark panels, and a muted red for the one destructive action.
	static FLinearColor Readable = SRGB(170, 194, 202);
	/** Footnotes: dimmer than Readable, still legible (an old card's "System recorded on next save"). */
	static FLinearColor Quiet = SRGB(124, 150, 158);
	static FLinearColor Danger = SRGB(222, 108, 92);
	static FLinearColor DangerBright = SRGB(255, 142, 120);
	/** Fill of a selected (action-framed) card, chip or toggle. */
	static FLinearColor ActionFillStrong = FLinearColor(0.42f, 0.19f, 0.01f, 1.0f);
	/** Fill of an idle highlight-framed chip or toggle. */
	static FLinearColor ControlFill = FLinearColor(0.015f, 0.09f, 0.12f, 0.96f);
	/** Brighter quiet frame of a feature panel. */
	static FLinearColor FrameBright = FLinearColor(0.035f, 0.28f, 0.37f, 0.95f);
	/** Fill of the full-page shades behind pages (alpha set at the call). */
	static FLinearColor PageShade = FLinearColor(0.0f, 0.008f, 0.016f, 1.0f);
	/** Tint of the menu's background image (alpha set at the call). */
	static FLinearColor ImageTint = FLinearColor(0.14f, 0.26f, 0.36f, 1.0f);
	// Rio 06.10: the landing chips' and NEW WORLD cards' fills and frames.
	static FLinearColor ChipRest = SRGB(3, 24, 34, 236);
	static FLinearColor ChipLit = SRGB(6, 50, 63, 246);
	static FLinearColor ChipDisabled = SRGB(4, 14, 20, 200);
	static FLinearColor DisabledFrame = SRGB(40, 58, 64, 160);
	static FLinearColor CardSelectedFill = SRGB(48, 34, 4, 236);
	static FLinearColor PlateTint = SRGB(2, 14, 20, 218);
	static FLinearColor SplashFill = SRGB(2, 6, 10);
	static FLinearColor ActionBright = SRGB(255, 208, 82);
	static FLinearColor ActionPeak = SRGB(255, 226, 140);
	static FLinearColor OnAction = SRGB(4, 18, 26);
	static FLinearColor OnActionCode = SRGB(90, 61, 0);
	/** Rio 06.10: the start-up title screen (SAPSMainMenuRoot::TickSplash). */
	TAutoConsoleVariable<float> CVarSplashTimeout(TEXT("aps.Menu.SplashTimeout"), 8.0f,
		TEXT("Seconds the start-up title screen waits at most before the main menu opens; 0 skips it."));
	constexpr float CardPerimeterThickness = 1.0f;
	/** RECENT: the worlds saved last (Rio 03.10). */
	constexpr int32 RecentWorldCount = 6;
	TWeakObjectPtr<UFont> DisplayFont;
	TWeakObjectPtr<UFont> BodyFont;
	static FSlateRoundedBoxBrush PanelBrush(Panel, 10.0f, CyanDim, 1.0f);
	static FSlateRoundedBoxBrush PanelSoftBrush(PanelSoft, 9.0f, CyanDim, 1.0f);
	static FSlateRoundedBoxBrush InsetBrush(FLinearColor(0.001f, 0.012f, 0.022f, 0.96f), 6.0f, FLinearColor(0.04f, 0.22f, 0.31f, 1.0f), 1.0f);
	static FSlateRoundedBoxBrush CyanBadgeBrush(FLinearColor(0.01f, 0.07f, 0.10f, 0.98f), 18.0f, Cyan, 1.25f);
	static FSlateRoundedBoxBrush AmberPanelBrush(FLinearColor(0.11f, 0.045f, 0.002f, 0.96f), 9.0f, Amber, 1.4f);
	static FSlateRoundedBoxBrush CivControlBrush(CivControl, 6.0f, FLinearColor(0.025f, 0.18f, 0.25f, 0.95f), 1.0f);
	static FSlateRoundedBoxBrush CivMetricBrush(FLinearColor(0.005f, 0.028f, 0.044f, 0.98f), 6.0f,
		FLinearColor(0.035f, 0.23f, 0.31f, 0.88f), 1.0f);
	static FSlateRoundedBoxBrush CivStatusBrush(FLinearColor(0.004f, 0.04f, 0.06f, 0.96f), 5.0f,
		FLinearColor(0.06f, 0.45f, 0.57f, 0.95f), 1.0f);
	static uint32 AppliedThemeRevision = 0;
	/** Rio 06.10: an outline alone in the action colour (a start card's image frame). */
	static FSlateRoundedBoxBrush OutlineBrush(FLinearColor::Transparent, 6.0f, SRGB(242, 181, 29), 2.0f);
	/** Rio 06.10: a progress bar's track (the default style's trough and blue fill ignored the palette). */
	static FSlateColorBrush ProgressTrackBrush(FLinearColor(0.035f, 0.23f, 0.31f, 0.5f));

	/** Rio 06.10: copies the active theme into the palette and brushes above. Brushes are referenced by pointer, so
	 * what is on screen recolours at once; colours passed by value need the page rebuilt (SAPSMainMenuRoot does). */
	void ApplyTheme()
	{
		if (AppliedThemeRevision == APSUITheme::Revision())
		{
			return;
		}
		AppliedThemeRevision = APSUITheme::Revision();
		const FAPSUIThemePalette& P = APSUITheme::Palette();
		const bool bClassic = APSUITheme::Current() == EAPSUITheme::Classic;
		Background = APSUITheme::Fade(P.Scrim, 20.0f / 150.0f);
		Panel = P.Panel;
		PanelSoft = P.PanelSoft;
		Cyan = P.Highlight;
		CyanDim = P.Frame;
		Amber = P.Action;
		White = P.Text;
		Muted = P.TextDim;
		Success = P.Success;
		CivPanel = P.PanelSoft.CopyWithNewOpacity(249.0f / 255.0f);
		CivRaised = P.Raised;
		CivControl = P.Panel.CopyWithNewOpacity(252.0f / 255.0f);
		Readable = P.TextSoft;
		Quiet = P.TextQuiet;
		Danger = P.Danger;
		DangerBright = P.DangerBright;
		ActionFillStrong = bClassic ? FLinearColor(0.42f, 0.19f, 0.01f, 1.0f)
			: FMath::Lerp(P.ActionFill, P.Action, 0.38f).CopyWithNewOpacity(1.0f);
		ControlFill = bClassic ? FLinearColor(0.015f, 0.09f, 0.12f, 0.96f) : P.Raised.CopyWithNewOpacity(0.96f);
		FrameBright = bClassic ? FLinearColor(0.035f, 0.28f, 0.37f, 0.95f)
			: FMath::Lerp(P.Frame, P.Highlight, 0.18f).CopyWithNewOpacity(0.95f);
		PageShade = bClassic ? FLinearColor(0.0f, 0.008f, 0.016f, 1.0f) : P.InsetFill.CopyWithNewOpacity(1.0f);
		ImageTint = bClassic ? FLinearColor(0.14f, 0.26f, 0.36f, 1.0f) : FLinearColor(0.20f, 0.20f, 0.21f, 1.0f);
		ChipRest = bClassic ? SRGB(3, 24, 34, 236) : FMath::Lerp(P.Panel, P.Raised, 0.3f).CopyWithNewOpacity(236.0f / 255.0f);
		ChipLit = bClassic ? SRGB(6, 50, 63, 246) : FMath::Lerp(P.Raised, P.Highlight, 0.06f).CopyWithNewOpacity(246.0f / 255.0f);
		ChipDisabled = bClassic ? SRGB(4, 14, 20, 200) : P.PanelSoft.CopyWithNewOpacity(200.0f / 255.0f);
		DisabledFrame = bClassic ? SRGB(40, 58, 64, 160) : APSUITheme::Fade(P.Frame, 0.6f);
		CardSelectedFill = bClassic ? SRGB(48, 34, 4, 236)
			: FMath::Lerp(P.ActionFill, P.Action, 0.25f).CopyWithNewOpacity(236.0f / 255.0f);
		PlateTint = bClassic ? SRGB(2, 14, 20, 218) : P.PanelSoft.CopyWithNewOpacity(218.0f / 255.0f);
		SplashFill = bClassic ? SRGB(2, 6, 10) : P.InsetFill.CopyWithNewOpacity(1.0f);
		ActionBright = P.ActionBright;
		ActionPeak = P.ActionPeak;
		OnAction = P.OnAction;
		OnActionCode = P.OnActionCode;

		const FLinearColor StatusFill = bClassic ? FLinearColor(0.004f, 0.04f, 0.06f, 0.96f)
			: FMath::Lerp(P.InsetFill, P.HighlightFill, 0.6f).CopyWithNewOpacity(0.96f);
		const FLinearColor StatusFrame = bClassic ? FLinearColor(0.06f, 0.45f, 0.57f, 0.95f)
			: FMath::Lerp(P.InsetFrame, P.Highlight, 0.35f).CopyWithNewOpacity(0.95f);
		const FLinearColor ControlFrame = bClassic ? FLinearColor(0.025f, 0.18f, 0.25f, 0.95f)
			: FMath::Lerp(P.InsetFrame, P.Highlight, 0.08f).CopyWithNewOpacity(0.95f);
		const FLinearColor MetricFill = bClassic ? FLinearColor(0.005f, 0.028f, 0.044f, 0.98f)
			: FMath::Lerp(P.InsetFill, P.Panel, 0.35f).CopyWithNewOpacity(0.98f);
		const FLinearColor MetricFrame = bClassic ? FLinearColor(0.035f, 0.23f, 0.31f, 0.88f)
			: APSUITheme::Fade(P.InsetFrame, 0.88f);
		PanelBrush = FSlateRoundedBoxBrush(Panel, 10.0f, CyanDim, 1.0f);
		PanelSoftBrush = FSlateRoundedBoxBrush(PanelSoft, 9.0f, CyanDim, 1.0f);
		InsetBrush = FSlateRoundedBoxBrush(P.InsetFill, 6.0f, P.InsetFrame, 1.0f);
		CyanBadgeBrush = FSlateRoundedBoxBrush(P.HighlightFill, 18.0f, Cyan, 1.25f);
		AmberPanelBrush = FSlateRoundedBoxBrush(P.ActionFill, 9.0f, Amber, 1.4f);
		CivControlBrush = FSlateRoundedBoxBrush(CivControl, 6.0f, ControlFrame, 1.0f);
		CivMetricBrush = FSlateRoundedBoxBrush(MetricFill, 6.0f, MetricFrame, 1.0f);
		CivStatusBrush = FSlateRoundedBoxBrush(StatusFill, 5.0f, StatusFrame, 1.0f);
		ProgressTrackBrush = FSlateColorBrush(APSUITheme::Fade(CyanDim, 0.7f));
		OutlineBrush = FSlateRoundedBoxBrush(FLinearColor::Transparent, 6.0f, Amber, 2.0f);
	}

	/** Rio 06.10: a Classic chrome tone (dark teal fills, teal frames, pale cyan) in the active theme: the same
	 * luminance in the hue of the theme's panels. Classic returns it unchanged. */
	FLinearColor Retint(const FLinearColor& ClassicTone)
	{
		return APSUITheme::Retint(ClassicTone);
	}

	/** The same for a Classic amber tone (a selected toggle's tint): the theme's action colour at that luminance. */
	FLinearColor RetintAction(const FLinearColor& ClassicTone)
	{
		return APSUITheme::RetintAction(ClassicTone);
	}

	TSharedRef<SWidget> ChamferPanel(TSharedRef<SWidget> Content, const FMargin& Padding,
		const FLinearColor& Accent = CyanDim, float Thickness = 1.0f)
	{
		return SNew(SAPSChamferedOverlay)
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
		// Rio 06.10: Chakra Petch for bold labels and titles, Exo 2 for text (aps.UI.LegacyFonts 1: Orbitron, Roboto).
		if (!APSUITheme::UsesLegacyFonts())
		{
			return Typeface == TEXT("Bold") ? APSUITheme::DisplayFont(Typeface, Size) : APSUITheme::BodyFont(Typeface, Size);
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

	/** Rio 03.10 ("the text strictly centred by height and width"): an upper-case label sits by its capitals, not by
	 * Slate's line box, which holds Orbitron's capitals 0.11 em high (APSChrome::CapsCenterOffset). */
	TOptional<FSlateRenderTransform> CapsShift(const FName Typeface, const int32 Size)
	{
		return APSChrome::CapsCenterShift(Font(Typeface, Size));
	}

	/** For a label of symbols alone ("<", ">", "+", "-", "v"), which sit lower than capitals. */
	TOptional<FSlateRenderTransform> SymbolShift(const FName Typeface, const int32 Size)
	{
		return APSChrome::SymbolCenterShift(Font(Typeface, Size));
	}

	/** Button padding for a label in this font: 16 each side and the same air above and below at every size. */
	FMargin ButtonPadding(const FName Typeface, const int32 Size, const float Horizontal = 16.0f)
	{
		return APSChrome::ButtonPadding(Font(Typeface, Size), Horizontal);
	}

	TSharedRef<SWidget> Badge(const FText& Glyph, const FLinearColor& Accent, float Size = 34.0f)
	{
		return SNew(SBox).WidthOverride(Size).HeightOverride(Size)
		.HAlign(HAlign_Fill).VAlign(VAlign_Fill)
		[
			SNew(SBorder).BorderImage(&CyanBadgeBrush).BorderBackgroundColor(Accent).Padding(1.0f)
			[
				SNew(SBorder).BorderImage(&InsetBrush).Padding(0.0f)
				.HAlign(HAlign_Center).VAlign(VAlign_Center)
				[
					SNew(STextBlock).Text(Glyph).Justification(ETextJustify::Center)
					.Font(Font("Bold", FMath::RoundToInt(Size * 0.34f))).ColorAndOpacity(Accent)
					.RenderTransform(CapsShift("Bold", FMath::RoundToInt(Size * 0.34f)))
				]
			]
		];
	}

	TSharedRef<SWidget> IconBadge(EAPSMenuGlyph Glyph, const FLinearColor& Accent, float Size = 34.0f)
	{
		return SNew(SBox).WidthOverride(Size).HeightOverride(Size)
		.HAlign(HAlign_Fill).VAlign(VAlign_Fill)
		[
			SNew(SBorder).BorderImage(&CyanBadgeBrush).BorderBackgroundColor(Accent).Padding(1.0f)
			[
				SNew(SBorder).BorderImage(&InsetBrush).Padding(FMath::Max(4.0f, Size * 0.19f))
				.HAlign(HAlign_Fill).VAlign(VAlign_Fill)
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
		return FText::FromString(MB >= 1024.0 ? FString::Printf(TEXT("%.1f GB"), MB / 1024.0)
			: MB >= 1.0 ? FString::Printf(TEXT("%.1f MB"), MB)
			: FString::Printf(TEXT("%.0f KB"), static_cast<double>(Bytes) / 1024.0));
	}

	/** A modal dialog's action: caps label centred both ways, even padding (Rio 03.10). */
	TSharedRef<SWidget> DialogButton(const FButtonStyle* Style, const TAttribute<FText>& Label,
		const TAttribute<FSlateColor>& Color, const FOnClicked& Action, const TAttribute<bool>& Enabled = true,
		const float MinWidth = 150.0f)
	{
		const FSlateFontInfo LabelFont = Font("Bold", 13);
		return SNew(SBox).MinDesiredWidth(MinWidth)
		[
			SNew(SButton).ButtonStyle(Style).ContentPadding(APSChrome::ButtonPadding(LabelFont, 18.0f))
			.HAlign(HAlign_Center).VAlign(VAlign_Center).IsEnabled(Enabled).OnClicked(Action)
			[APSChrome::CenteredLabel(Label, LabelFont, Color)]
		];
	}

	bool LoadWorldMetadataSidecar(const FString& MetadataPath, FAPSExistingWorldEntry& Entry)
	{
		if (!IFileManager::Get().FileExists(*MetadataPath))
		{
			return false;
		}

		FConfigFile Metadata;
		APSWorldBrowserMetadata::ReadSidecar(MetadataPath, Metadata);
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
		// Version 2 adds the home system the game read from the live hierarchy (Rio 03.10); older records keep false.
		Entry.bSystemRecorded = APSWorldBrowserMetadata::ReadSystemRecord(Metadata, Entry.System);
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

	/** Saved/SaveGames holds more than worlds: the MBLS settings panel keeps its audio volumes in
	 * GameSettings_SaveSlot.sav (BP_SG_GameSettings). World slots read "<Name> Cluster 1630" and never end in
	 * "_SaveSlot", so such saves are neither listed nor deletable as worlds (Rio 03.10). */
	bool IsWorldSaveSlot(const FString& SlotName)
	{
		return !SlotName.IsEmpty() && !SlotName.EndsWith(TEXT("_SaveSlot"), ESearchCase::IgnoreCase);
	}

	/** "NO PLANETS", "1 PLANET", "7 PLANETS". */
	FText PlanetCountText(const int32 Count)
	{
		if (Count <= 0)
		{
			return LOCTEXT("NoPlanetsCount", "NO PLANETS");
		}
		return Count == 1 ? LOCTEXT("OnePlanetCount", "1 PLANET")
			: FText::Format(LOCTEXT("PlanetsCount", "{0} PLANETS"), APSUINumber::Number(Count));
	}

	/** The home star's class. Even a version 1 record holds the menu's home star setting, which the generator applies. */
	FString StarClassOf(const FAPSExistingWorldEntry& Entry)
	{
		return APSWorldScheme::StarClassFromLabel(Entry.bSystemRecorded ? Entry.System.StarClassLabel : Entry.StarType);
	}

	/** "G YELLOW DWARF", "K ORANGE GIANT", "BLACK HOLE". */
	FText StarText(const FAPSExistingWorldEntry& Entry)
	{
		return FText::FromString(APSWorldScheme::DescribeStar(StarClassOf(Entry), Entry.System.StellarType));
	}

	FText UpperText(const FString& Value)
	{
		return FText::FromString(Value.ToUpper());
	}

	const FAPSWorldPlanetRecord* HomeRecord(const FAPSExistingWorldEntry& Entry)
	{
		return Entry.bSystemRecorded && Entry.System.Planets.IsValidIndex(Entry.System.HomePlanetIndex)
			? &Entry.System.Planets[Entry.System.HomePlanetIndex] : nullptr;
	}

	/** The card chip (Rio 03.10: "1 PLANETS / G - Yellow" -> "1 PLANET  •  G YELLOW DWARF"); empty until recorded. */
	FText WorldChipText(const FAPSExistingWorldEntry& Entry)
	{
		return Entry.bSystemRecorded
			? FText::Format(LOCTEXT("WorldChip", "{0}  •  {1}"), PlanetCountText(Entry.TotalPlanets), StarText(Entry))
			: FText::GetEmpty();
	}

	/** The card's second line: the home world once recorded, else when its record arrives. */
	FText WorldCardLine(const FAPSExistingWorldEntry& Entry)
	{
		if (!Entry.bMetadataLoaded)
		{
			return LOCTEXT("CardLegacy", "Details load on launch");
		}
		if (!Entry.bSystemRecorded)
		{
			return LOCTEXT("CardUnrecorded", "System recorded on next save");
		}
		const FAPSWorldPlanetRecord* Home = HomeRecord(Entry);
		return FText::Format(LOCTEXT("CardHome", "HOME  •  {0}"), UpperText(Home ? Home->Type : Entry.PlanetType));
	}

	/** File times are UTC; the player reads his local time ("03 OCT 2026  01:17"). */
	FText LocalTimeText(const int64 UnixUtc, const TCHAR* Format)
	{
		const FTimespan Offset = FTimespan::FromMinutes(FMath::RoundToDouble(
			(FDateTime::Now() - FDateTime::UtcNow()).GetTotalMinutes()));
		return FText::FromString((FDateTime::FromUnixTimestamp(UnixUtc) + Offset).ToFormattedString(Format).ToUpper());
	}

	/** What one world's scheme is drawn from: the recorded system, else only its star and its slot name. */
	FAPSWorldSchemeInput SchemeInput(const FAPSExistingWorldEntry& Entry)
	{
		FAPSWorldSchemeInput Input;
		Input.Key = Entry.SaveFileName;
		Input.StarClass = StarClassOf(Entry);
		Input.bRecorded = Entry.bSystemRecorded;
		if (Entry.bSystemRecorded)
		{
			Input.StellarType = Entry.System.StellarType;
			Input.StarCount = Entry.System.StarCount;
			Input.HomeIndex = Entry.System.HomePlanetIndex;
			for (const FAPSWorldPlanetRecord& Planet : Entry.System.Planets)
			{
				FAPSWorldSchemePlanet& Body = Input.Planets.AddDefaulted_GetRef();
				Body.Type = Planet.Type;
				Body.Orbit = Planet.OrbitAu;
				Body.RadiusKm = Planet.RadiusKm;
				Body.Moons = Planet.Moons;
				Body.Star = Planet.Star;
				Body.bInhabited = Planet.bInhabited;
			}
		}
		return Input;
	}

	/** Rio 03.10 (filters): a field an older record cannot vouch for files the world here, never under a guess. */
	const TCHAR* const UnrecordedFilterKey = TEXT("UNRECORDED");

	/** The world's option in one filter: a star class, a system type, inhabited or not, the home world's type. */
	FString WorldFilterKey(const FAPSExistingWorldEntry& Entry, const EAPSWorldFilterKind Kind)
	{
		switch (Kind)
		{
		case EAPSWorldFilterKind::StarType:
			{
				// The star class holds even in a version 1 record: the generator applies the menu's home-star class.
				if (Entry.bSystemRecorded)
				{
					const FString Stellar = Entry.System.StellarType.ToUpper().Replace(TEXT(" "), TEXT(""));
					if (Stellar.Contains(TEXT("WHITEDWARF"))) return TEXT("WD");
					if (Stellar.Contains(TEXT("BLACKHOLE"))) return TEXT("BH");
					if (Stellar.Contains(TEXT("NEUTRON")) || Stellar.Contains(TEXT("PULSAR"))) return TEXT("NS");
					if (Stellar.Contains(TEXT("PROTO"))) return TEXT("PS");
				}
				const FString Class = Entry.bMetadataLoaded ? StarClassOf(Entry) : FString();
				return Class.IsEmpty() ? FString(UnrecordedFilterKey) : Class;
			}
		case EAPSWorldFilterKind::WorldType:
			{
				if (!Entry.bSystemRecorded) return UnrecordedFilterKey;
				// Display names ("Gas Giant  System") and bare enum names ("GasGiantsSystem") alike.
				const FString Type = Entry.SystemType.ToUpper().Replace(TEXT(" "), TEXT(""));
				if (Type.Contains(TEXT("MULTI"))) return TEXT("MULTI PLANET");
				if (Type.Contains(TEXT("SINGLE"))) return TEXT("SINGLE PLANET");
				if (Type.Contains(TEXT("HABITABLE"))) return TEXT("HABITABLE ZONE");
				if (Type.Contains(TEXT("GAS"))) return TEXT("GAS GIANTS");
				if (Type.Contains(TEXT("NOPLANET"))) return TEXT("NO PLANETS");
				return TEXT("OTHER");
			}
		case EAPSWorldFilterKind::Inhabited:
			// A version 1 count says nothing: every world, colonised or not, recorded its home planet (often again and again).
			if (!Entry.bSystemRecorded) return UnrecordedFilterKey;
			return Entry.InhabitedPlanets > 0 ? TEXT("INHABITED") : TEXT("UNINHABITED");
		default:
			{
				if (!Entry.bSystemRecorded) return UnrecordedFilterKey;
				const FAPSWorldPlanetRecord* Home = HomeRecord(Entry);
				FString Type = (Home ? Home->Type : Entry.PlanetType).ToUpper().TrimStartAndEnd();
				Type.RemoveFromEnd(TEXT(" PLANET"));
				return Type.IsEmpty() ? FString(TEXT("OTHER")) : Type;
			}
		}
	}

	/** Star classes hot to cold, then the remnants; other filters read alphabetically. UNRECORDED always last. */
	int32 WorldFilterOrder(const EAPSWorldFilterKind Kind, const FString& Key)
	{
		static const TCHAR* const StarOrder[] = {TEXT("O"), TEXT("B"), TEXT("A"), TEXT("F"), TEXT("G"), TEXT("K"), TEXT("M"),
			TEXT("L"), TEXT("T"), TEXT("Y"), TEXT("WD"), TEXT("NS"), TEXT("PS"), TEXT("BH")};
		static const TCHAR* const TypeOrder[] = {TEXT("MULTI PLANET"), TEXT("SINGLE PLANET"), TEXT("HABITABLE ZONE"),
			TEXT("GAS GIANTS"), TEXT("NO PLANETS"), TEXT("OTHER")};
		static const TCHAR* const InhabitedOrder[] = {TEXT("INHABITED"), TEXT("UNINHABITED")};
		if (Key == UnrecordedFilterKey)
		{
			return 1000;
		}
		const auto Find = [&Key](const TCHAR* const* Order, const int32 Count)
		{
			for (int32 Index = 0; Index < Count; ++Index)
			{
				if (Key == Order[Index]) return Index;
			}
			return 500;
		};
		switch (Kind)
		{
		case EAPSWorldFilterKind::StarType: return Find(StarOrder, static_cast<int32>(UE_ARRAY_COUNT(StarOrder)));
		case EAPSWorldFilterKind::WorldType: return Find(TypeOrder, static_cast<int32>(UE_ARRAY_COUNT(TypeOrder)));
		case EAPSWorldFilterKind::Inhabited: return Find(InhabitedOrder, static_cast<int32>(UE_ARRAY_COUNT(InhabitedOrder)));
		default: return 500;
		}
	}

	FText WorldFilterOptionLabel(const EAPSWorldFilterKind Kind, const FString& Key)
	{
		if (Key.IsEmpty())
		{
			return LOCTEXT("FilterAny", "ANY");
		}
		if (Key == UnrecordedFilterKey)
		{
			return LOCTEXT("FilterUnrecorded", "UNRECORDED");
		}
		if (Kind == EAPSWorldFilterKind::StarType)
		{
			if (Key == TEXT("O")) return LOCTEXT("FilterStarO", "O BLUE");
			if (Key == TEXT("B")) return LOCTEXT("FilterStarB", "B BLUE-WHITE");
			if (Key == TEXT("A")) return LOCTEXT("FilterStarA", "A WHITE");
			if (Key == TEXT("F")) return LOCTEXT("FilterStarF", "F YELLOW-WHITE");
			if (Key == TEXT("G")) return LOCTEXT("FilterStarG", "G YELLOW");
			if (Key == TEXT("K")) return LOCTEXT("FilterStarK", "K ORANGE");
			if (Key == TEXT("M")) return LOCTEXT("FilterStarM", "M RED");
			if (Key == TEXT("L") || Key == TEXT("T") || Key == TEXT("Y"))
			{
				return FText::Format(LOCTEXT("FilterStarBrown", "{0} BROWN DWARF"), FText::FromString(Key));
			}
			if (Key == TEXT("WD")) return LOCTEXT("FilterStarWhiteDwarf", "WHITE DWARF");
			if (Key == TEXT("NS")) return LOCTEXT("FilterStarNeutron", "NEUTRON STAR");
			if (Key == TEXT("PS")) return LOCTEXT("FilterStarProto", "PROTOSTAR");
			if (Key == TEXT("BH")) return LOCTEXT("FilterStarBlackHole", "BLACK HOLE");
		}
		return FText::FromString(Key);
	}
}

SAPSMainMenuRoot::SAPSMainMenuRoot()
{
	APSMenu::ApplyTheme();
	BuildButtonStyles();
	ScrollBarStyle = FAppStyle::Get().GetWidgetStyle<FScrollBarStyle>("ScrollBar");
}

SAPSMainMenuRoot::~SAPSMainMenuRoot()
{
	APSUITheme::OnChanged().Remove(ThemeChangedHandle);
}

void SAPSMainMenuRoot::BuildButtonStyles()
{
	// Classic keeps the exact pre-06.10 fills; the other themes derive theirs from the palette.
	const bool bClassic = APSUITheme::Current() == EAPSUITheme::Classic;
	const FAPSUIThemePalette& P = APSUITheme::Palette();
	const auto Pick = [bClassic](const FLinearColor& ClassicColour, const FLinearColor& Themed)
	{
		return bClassic ? ClassicColour : Themed;
	};
	const FLinearColor PrimaryRest = Pick(APSMenu::SRGB(74, 48, 4, 246), FMath::Lerp(P.ActionFill, P.Action, 0.30f).CopyWithNewOpacity(246.0f / 255.0f));
	const FLinearColor PrimaryHover = Pick(APSMenu::SRGB(106, 70, 5, 252), FMath::Lerp(P.ActionFill, P.Action, 0.45f).CopyWithNewOpacity(252.0f / 255.0f));
	const FLinearColor PrimaryDown = Pick(APSMenu::SRGB(128, 82, 4), FMath::Lerp(P.ActionFill, P.Action, 0.60f).CopyWithNewOpacity(1.0f));
	const FLinearColor SecondaryRest = Pick(APSMenu::SRGB(8, 32, 42, 232), P.Panel.CopyWithNewOpacity(232.0f / 255.0f));
	const FLinearColor SecondaryHover = Pick(APSMenu::SRGB(12, 52, 65, 246), FMath::Lerp(P.Raised, P.Highlight, 0.05f).CopyWithNewOpacity(246.0f / 255.0f));
	const FLinearColor SecondaryDown = Pick(APSMenu::SRGB(13, 67, 82), FMath::Lerp(P.Raised, P.Highlight, 0.10f).CopyWithNewOpacity(1.0f));
	PrimaryButtonStyle = FButtonStyle()
		.SetNormal(FSlateRoundedBoxBrush(PrimaryRest, 8.0f, APSMenu::Amber, 1.5f))
		.SetHovered(FSlateRoundedBoxBrush(PrimaryHover, 8.0f, Pick(APSMenu::SRGB(255, 208, 82), P.ActionBright), 2.0f))
		.SetPressed(FSlateRoundedBoxBrush(PrimaryDown, 8.0f, APSMenu::Amber, 2.0f))
		.SetNormalPadding(FMargin(2.0f)).SetPressedPadding(FMargin(2.0f, 3.0f, 2.0f, 1.0f));
	SecondaryButtonStyle = FButtonStyle()
		.SetNormal(FSlateRoundedBoxBrush(SecondaryRest, 7.0f, APSMenu::CyanDim, 1.0f))
		.SetHovered(FSlateRoundedBoxBrush(SecondaryHover, 7.0f, APSMenu::Cyan, 1.5f))
		.SetPressed(FSlateRoundedBoxBrush(SecondaryDown, 7.0f, APSMenu::Cyan, 1.5f));
	CardButtonStyle = SecondaryButtonStyle;
	const FLinearColor DisabledFill = Pick(FLinearColor(0.015f, 0.025f, 0.035f, 0.82f), P.PanelSoft.CopyWithNewOpacity(0.82f));
	const FLinearColor DisabledFrame = Pick(FLinearColor(0.20f, 0.25f, 0.28f), APSUITheme::Fade(P.Frame, 0.8f));
	DisabledCardButtonStyle = FButtonStyle()
		.SetNormal(FSlateRoundedBoxBrush(DisabledFill, 8.0f, DisabledFrame, 1.0f))
		.SetHovered(FSlateRoundedBoxBrush(DisabledFill, 8.0f, DisabledFrame, 1.0f));
	// Rio 03.10: DELETE WORLD looks secondary until the pointer is on it; the confirmation's DELETE is red at rest.
	const FSlateRoundedBoxBrush DangerHovered(APSMenu::SRGB(58, 14, 11, 246), 7.0f, APSMenu::Danger, 1.5f);
	const FSlateRoundedBoxBrush DangerPressed(APSMenu::SRGB(84, 20, 15), 7.0f, APSMenu::DangerBright, 1.5f);
	const FSlateRoundedBoxBrush DangerDisabled(Pick(APSMenu::SRGB(6, 18, 24, 200), P.PanelSoft.CopyWithNewOpacity(200.0f / 255.0f)),
		7.0f, Pick(APSMenu::SRGB(40, 58, 64, 160), APSUITheme::Fade(P.Frame, 0.6f)), 1.0f);
	DangerButtonStyle = FButtonStyle(SecondaryButtonStyle)
		.SetHovered(DangerHovered).SetPressed(DangerPressed).SetDisabled(DangerDisabled);
	DangerConfirmButtonStyle = FButtonStyle(SecondaryButtonStyle)
		.SetNormal(FSlateRoundedBoxBrush(APSMenu::SRGB(40, 10, 8, 240), 7.0f, APSMenu::SRGB(150, 58, 48), 1.2f))
		.SetHovered(DangerHovered).SetPressed(DangerPressed).SetDisabled(DangerDisabled);
	DropdownOptionStyle = FButtonStyle()
		.SetHovered(FSlateRoundedBoxBrush(SecondaryHover, 5.0f))
		.SetPressed(FSlateRoundedBoxBrush(SecondaryDown, 5.0f));
}

namespace APSMenuChecks
{
	/** The menu on screen, for the check commands below. */
	TWeakPtr<SAPSMainMenuRoot> GActiveRoot;

	FAutoConsoleCommand OpenCommand(TEXT("aps.Menu.Open"),
		TEXT("aps.Menu.Open <Landing|NewWorld|Worlds|Settings|Profile> [Video|Graphics|Audio|Interface]: opens a menu page (checks)."),
		FConsoleCommandWithArgsDelegate::CreateLambda([](const TArray<FString>& Args)
		{
			const TSharedPtr<SAPSMainMenuRoot> Root = GActiveRoot.Pin();
			const bool bOpened = Root.IsValid() && Args.Num() > 0 && Root->OpenPageByName(Args[0], Args.Num() > 1 ? Args[1] : FString());
			UE_LOG(LogTemp, Log, TEXT("[APS.Menu] Open %s %s"), Args.Num() > 0 ? *Args[0] : TEXT("?"), bOpened ? TEXT("done") : TEXT("failed"));
		}));

	/**
	 * aps.Menu.ThemeShots <label> [quit]: every theme through the landing, NEW WORLD and the four SETTINGS tabs, one
	 * screenshot each with the UI (Saved/Screenshots/MenuThemes/<label>_<theme>_<page>.png). Rio 06.10: "check that the
	 * sizes are fine and readable" in every theme.
	 */
	struct FThemeShots
	{
		FString Label;
		bool bQuit{false};
		int32 Step{0};
		double NextAt{0.0};
		bool bShotPending{false};
		FTSTicker::FDelegateHandle Ticker;
	};
	TUniquePtr<FThemeShots> GShots;

	bool TickThemeShots(float)
	{
		static const TCHAR* Pages[][2] = {
			{TEXT("Landing"), TEXT("")}, {TEXT("NewWorld"), TEXT("")}, {TEXT("Settings"), TEXT("Video")},
			{TEXT("Settings"), TEXT("Graphics")}, {TEXT("Settings"), TEXT("Audio")}, {TEXT("Settings"), TEXT("Interface")}};
		constexpr int32 PageCount = UE_ARRAY_COUNT(Pages);
		constexpr int32 ThemeCount = static_cast<int32>(EAPSUITheme::Count);
		FThemeShots* Shots = GShots.Get();
		const TSharedPtr<SAPSMainMenuRoot> Root = GActiveRoot.Pin();
		if (!Shots || !Root.IsValid())
		{
			return false;
		}
		const double Now = FPlatformTime::Seconds();
		if (Now < Shots->NextAt)
		{
			return true;
		}
		if (Shots->bShotPending)
		{
			// The page had its seconds to build and draw: shoot it, then move on.
			const int32 Theme = Shots->Step / PageCount;
			const int32 Page = Shots->Step % PageCount;
			const FString Name = FString::Printf(TEXT("%s_%s_%s%s"), *Shots->Label,
				*APSUITheme::DisplayName(static_cast<EAPSUITheme>(Theme)).ToString().ToLower(), Pages[Page][0], Pages[Page][1]);
			FScreenshotRequest::RequestScreenshot(FPaths::ScreenShotDir() / TEXT("MenuThemes") / Name + TEXT(".png"), true, false);
			UE_LOG(LogTemp, Log, TEXT("[APS.MenuThemes] shot %s"), *Name);
			Shots->bShotPending = false;
			++Shots->Step;
			Shots->NextAt = Now + 0.6;
			return true;
		}
		if (Shots->Step >= PageCount * ThemeCount)
		{
			UE_LOG(LogTemp, Log, TEXT("[APS.MenuThemes] done"));
			APSUITheme::SetCurrent(EAPSUITheme::Obsidian);
			if (Shots->bQuit)
			{
				FPlatformMisc::RequestExit(false, TEXT("aps.Menu.ThemeShots"));
			}
			GShots.Reset();
			return false;
		}
		const EAPSUITheme Theme = static_cast<EAPSUITheme>(Shots->Step / PageCount);
		if (APSUITheme::Current() != Theme)
		{
			APSUITheme::SetCurrent(Theme);
		}
		const int32 Page = Shots->Step % PageCount;
		Root->OpenPageByName(Pages[Page][0], Pages[Page][1]);
		Shots->bShotPending = true;
		Shots->NextAt = Now + 2.0;
		return true;
	}

	FAutoConsoleCommand ThemeShotsCommand(TEXT("aps.Menu.ThemeShots"),
		TEXT("aps.Menu.ThemeShots <label> [quit]: screenshots of the landing, NEW WORLD and SETTINGS in every interface theme."),
		FConsoleCommandWithArgsDelegate::CreateLambda([](const TArray<FString>& Args)
		{
			if (GShots.IsValid() || !GActiveRoot.IsValid())
			{
				UE_LOG(LogTemp, Warning, TEXT("[APS.MenuThemes] busy or no menu"));
				return;
			}
			GShots = MakeUnique<FThemeShots>();
			GShots->Label = Args.Num() > 0 ? Args[0] : FString(TEXT("themes"));
			GShots->bQuit = Args.Contains(TEXT("quit"));
			GShots->NextAt = FPlatformTime::Seconds() + 3.0;
			GShots->Ticker = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateStatic(&TickThemeShots), 0.1f);
			UE_LOG(LogTemp, Log, TEXT("[APS.MenuThemes] start %s"), *GShots->Label);
		}));
}

TSharedRef<SWidget> SAPSMainMenuRoot::BuildAuthoredMapGrid()
{
	// Rio 06.10 ("the Single Play button is stretched; it should be a grid, with preview pictures, the map's name and
	// description, a real render instead of an icon"): one card per authored map; a click plays it. The preview is a
	// render of the map saved as Content/Slate/MapPreviews/<map>.png (loaded by Slate from the file, no asset).
	const FString PreviewPath = FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir() / TEXT("Slate/MapPreviews/StartLocation.png"));
	if (!MapPreviewBrush.IsValid() && FPaths::FileExists(PreviewPath))
	{
		MapPreviewBrush = MakeShared<FSlateImageBrush>(FName(*PreviewPath), FVector2D(1600.0f, 900.0f));
	}
	const auto MapCard = [this](const FText& Title, const FText& Description, const FText& Place, const FSlateBrush* Preview,
		const bool bPlayable) -> TSharedRef<SWidget>
	{
		const TSharedRef<SButton> Button = SNew(SButton)
			.ButtonStyle(FAppStyle::Get(), "NoBorder")
			.ContentPadding(0.0f)
			.IsEnabled(bPlayable)
			.OnClicked(this, &SAPSMainMenuRoot::StartSingleGame);
		const TWeakPtr<SButton> Weak = Button;
		const auto Lit = [Weak]()
		{
			const TSharedPtr<SButton> Pinned = Weak.Pin();
			return Pinned.IsValid() && Pinned->IsHovered() && Pinned->IsEnabled();
		};
		const TSharedRef<SWidget> Picture = Preview
			? StaticCastSharedRef<SWidget>(SNew(SScaleBox).Stretch(EStretch::ScaleToFill)[SNew(SImage).Image(Preview)])
			: StaticCastSharedRef<SWidget>(
				SNew(SBorder).BorderImage(FAppStyle::GetBrush("WhiteBrush")).BorderBackgroundColor(APSMenu::PageShade)
				.HAlign(HAlign_Center).VAlign(VAlign_Center)
				[
					SNew(SBox).WidthOverride(54.0f).HeightOverride(54.0f)
					[SNew(SVectorMenuGlyph).Glyph(bPlayable ? EAPSMenuGlyph::Planet : EAPSMenuGlyph::Lock).Color(APSMenu::CyanDim).StrokeWidth(1.6f)]
				]);
		Button->SetContent(
			SNew(SBox).HeightOverride(330.0f)
			[
				SNew(SAPSChamferedOverlay)
				+ SOverlay::Slot()
				[
					SNew(SChamferedSurface).Brush(FAppStyle::GetBrush("WhiteBrush")).ChamferTop(true).ChamferBottom(true)
					.Tint_Lambda([Lit, bPlayable]() { return !bPlayable ? APSMenu::ChipDisabled : Lit() ? APSMenu::ChipLit : APSMenu::ChipRest; })
				]
				+ SOverlay::Slot()
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight()
					[
						SNew(SBox).HeightOverride(196.0f)
						[
							SNew(SOverlay)
							+ SOverlay::Slot()[Picture]
							+ SOverlay::Slot().HAlign(HAlign_Left).VAlign(VAlign_Top).Padding(FMargin(14.0f, 12.0f))
							[
								SNew(SBorder).BorderImage(&APSMenu::InsetBrush).Padding(FMargin(8.0f, 3.0f))
								.Visibility(bPlayable ? EVisibility::HitTestInvisible : EVisibility::Collapsed)
								[
									SNew(STextBlock).Text(LOCTEXT("MapAuthoredTag", "AUTHORED")).Font(APSMenu::Font("Bold", 10))
									.ColorAndOpacity(APSMenu::Readable).RenderTransform(APSMenu::CapsShift("Bold", 10))
								]
							]
						]
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(FMargin(18.0f, 14.0f, 18.0f, 0.0f))
					[
						SNew(STextBlock).Text(Title).Font(APSMenu::Font("Bold", 18)).ColorAndOpacity(bPlayable ? APSMenu::White : APSMenu::Readable)
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(FMargin(18.0f, 4.0f, 18.0f, 0.0f))
					[
						SNew(STextBlock).Text(Description).Font(APSMenu::Font("Regular", 14)).ColorAndOpacity(APSMenu::Readable)
						.AutoWrapText(true)
					]
					+ SVerticalBox::Slot().FillHeight(1.0f)
					+ SVerticalBox::Slot().AutoHeight().Padding(FMargin(18.0f, 0.0f, 18.0f, 14.0f))
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center)
						[
							SNew(STextBlock).Text(Place).Font(APSMenu::Font("Bold", 11)).ColorAndOpacity(APSMenu::Quiet)
						]
						+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
						[
							SNew(STextBlock).Text(LOCTEXT("MapPlay", "PLAY  >")).Font(APSMenu::Font("Bold", 14))
							.Visibility(bPlayable ? EVisibility::HitTestInvisible : EVisibility::Collapsed)
							.ColorAndOpacity_Lambda([Lit]() { return FSlateColor(Lit() ? APSMenu::ActionBright : APSMenu::Amber); })
							.RenderTransform(APSMenu::CapsShift("Bold", 14))
						]
					]
				]
				+ SOverlay::Slot()
				[
					SNew(SChamferedFrame).Thickness(1.0f)
					.Color_Lambda([Lit, bPlayable]() { return !bPlayable ? APSMenu::DisabledFrame : Lit() ? APSMenu::Cyan : APSMenu::CyanDim; })
				]
			]);
		return Button;
	};

	return SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight()
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center)
			[SNew(STextBlock).Text(LOCTEXT("NewWorldMaps", "AUTHORED MAPS")).Font(APSMenu::Font("Bold", 12)).ColorAndOpacity(APSMenu::Cyan)]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
			[SNew(STextBlock).Text(LOCTEXT("NewWorldMapsCount", "1 MAP")).Font(APSMenu::Font("Bold", 11)).ColorAndOpacity(APSMenu::Quiet)]
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 14.0f, 0.0f, 0.0f)
		[
			SNew(SUniformGridPanel).SlotPadding(FMargin(8.0f))
			+ SUniformGridPanel::Slot(0, 0)
			[
				MapCard(LOCTEXT("NewWorldMapStart", "START LOCATION"),
					LOCTEXT("NewWorldMapStartDesc", "The Alpha map: a set start with ships ready to fly."),
					LOCTEXT("NewWorldMapStartPlace", "ALPHA"), MapPreviewBrush.Get(), true)
			]
			+ SUniformGridPanel::Slot(1, 0)
			[
				MapCard(LOCTEXT("NewWorldMapsSoon", "MORE MAPS"),
					LOCTEXT("NewWorldMapsSoonDesc", "New maps arrive with the next versions."),
					FText::GetEmpty(), nullptr, false)
			]
		];
}

bool SAPSMainMenuRoot::OpenPageByName(const FString& Page, const FString& Tab)
{
	EAPSMenuPage Target;
	if (Page.Equals(TEXT("Landing"), ESearchCase::IgnoreCase)) Target = EAPSMenuPage::Landing;
	else if (Page.Equals(TEXT("NewWorld"), ESearchCase::IgnoreCase)) Target = EAPSMenuPage::ChoosePath;
	else if (Page.Equals(TEXT("Worlds"), ESearchCase::IgnoreCase)) Target = EAPSMenuPage::ExistingWorlds;
	else if (Page.Equals(TEXT("Settings"), ESearchCase::IgnoreCase)) Target = EAPSMenuPage::Settings;
	else if (Page.Equals(TEXT("Profile"), ESearchCase::IgnoreCase)) Target = EAPSMenuPage::Profile;
	else return false;
	if (Target == EAPSMenuPage::Settings && !Tab.IsEmpty())
	{
		static const TCHAR* Tabs[] = {TEXT("Video"), TEXT("Graphics"), TEXT("Audio"), TEXT("Interface")};
		for (int32 Index = 0; Index < UE_ARRAY_COUNT(Tabs); ++Index)
		{
			if (Tab.Equals(Tabs[Index], ESearchCase::IgnoreCase))
			{
				SettingsTab = static_cast<EAPSSettingsTab>(Index);
			}
		}
		if (CurrentPage == EAPSMenuPage::Settings)
		{
			// Same page, another tab: build it again on that tab.
			bHasBuiltCurrentPage = false;
		}
	}
	Navigate(Target);
	return true;
}

void SAPSMainMenuRoot::HandleThemeChanged()
{
	APSMenu::ApplyTheme();
	BuildButtonStyles();
	RebuildCurrentPage();
}

void SAPSMainMenuRoot::RebuildCurrentPage()
{
	// The generation and civilization pages hold live editing state; they pick the theme up when next opened.
	if (!ContentHost.IsValid() || CurrentPage == EAPSMenuPage::AstronomicalGeneration
		|| CurrentPage == EAPSMenuPage::Civilization)
	{
		return;
	}
	bHasBuiltCurrentPage = false;
	const EAPSMenuPage Page = CurrentPage;
	const EAPSMenuPage Before = PreviousPage;
	Navigate(Page);
	PreviousPage = Before;
}

void SAPSMainMenuRoot::Construct(const FArguments& InArgs)
{
	Controller = InArgs._Controller;
	ViewModel = InArgs._ViewModel;
	ThemeChangedHandle = APSUITheme::OnChanged().AddSP(this, &SAPSMainMenuRoot::HandleThemeChanged);
	APSMenuChecks::GActiveRoot = StaticCastSharedRef<SAPSMainMenuRoot>(AsShared());
	LoadVisualResources();
	BeginAuxiliaryMenuLoad();

	ChildSlot
	[
		SNew(SOverlay)
		+ SOverlay::Slot()
		[
			SNew(SBorder)
			.BorderImage(FAppStyle::GetBrush("WhiteBrush"))
			.BorderBackgroundColor(APSMenu::Background)
			.Padding(0.0f)
			.IsEnabled_Lambda([this]()
			{
				const UAPSAuthoredLevelLaunchSubsystem* Launch = GetAuthoredLaunch(Controller.Get());
				return !Launch || !Launch->IsDialogVisible();
			})
			[SAssignNew(ContentHost, SBox)]
		]
		+ SOverlay::Slot()
		[
			SNew(SBorder)
			.BorderImage(FAppStyle::GetBrush("WhiteBrush"))
			.BorderBackgroundColor(APSMenu::Retint(FLinearColor(0.005f, 0.015f, 0.025f, 0.96f)))
			.HAlign(HAlign_Center).VAlign(VAlign_Center).Padding(32.0f)
			.Visibility_Lambda([this]()
			{
				const UAPSAuthoredLevelLaunchSubsystem* Launch = GetAuthoredLaunch(Controller.Get());
				return Launch && Launch->IsDialogVisible() ? EVisibility::Visible : EVisibility::Collapsed;
			})
			[
				SNew(SBox).WidthOverride(580.0f)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 16.0f)
					[
						SNew(STextBlock).Text(LOCTEXT("SinglePlayPreparation", "SINGLE GAME"))
						.Font(APSMenu::Font("Bold", 24)).ColorAndOpacity(APSMenu::Cyan)
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 16.0f)
					[
						SNew(STextBlock).AutoWrapText(true)
						.Font(APSMenu::Font("Regular", 17)).ColorAndOpacity(APSMenu::Muted)
						.Text_Lambda([this]()
						{
							const UAPSAuthoredLevelLaunchSubsystem* Launch = GetAuthoredLaunch(Controller.Get());
							return Launch ? Launch->GetStatusText() : FText::GetEmpty();
						})
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 24.0f)
					[
						SNew(STextBlock).AutoWrapText(true)
						.Text(LOCTEXT("SinglePlayColdCacheHint", "First launch prepares level assets in the background. You can cancel without changing your world."))
						.Font(APSMenu::Font("Regular", 14)).ColorAndOpacity(APSMenu::Muted)
					]
					+ SVerticalBox::Slot().AutoHeight()
					[
						SNew(SButton).ButtonStyle(&SecondaryButtonStyle).ContentPadding(APSMenu::ButtonPadding("Bold", 13))
						.HAlign(HAlign_Center).VAlign(VAlign_Center)
						.IsEnabled_Lambda([this]()
						{
							const UAPSAuthoredLevelLaunchSubsystem* Launch = GetAuthoredLaunch(Controller.Get());
							return Launch && !Launch->IsOpening();
						})
						.OnClicked_Lambda([this]()
						{
							if (UAPSAuthoredLevelLaunchSubsystem* Launch = GetAuthoredLaunch(Controller.Get())) Launch->CancelLaunch();
							return FReply::Handled();
						})
						[
							SNew(STextBlock).Text(LOCTEXT("CancelSinglePlayLaunch", "BACK TO MENU"))
							.Justification(ETextJustify::Center).Font(APSMenu::Font("Bold", 13)).ColorAndOpacity(APSMenu::White)
							.RenderTransform(APSMenu::CapsShift("Bold", 13))
						]
					]
				]
			]
		]
		+ SOverlay::Slot()
		[
			// Rio 06.10: the start-up title screen over everything; TickSplash lifts it once the start-up is over.
			SAssignNew(Splash, SBorder)
			.BorderImage(FAppStyle::GetBrush("WhiteBrush"))
			.BorderBackgroundColor(APSMenu::SplashFill)
			.HAlign(HAlign_Center).VAlign(VAlign_Center)
			.Visibility(GIsAutomationTesting || APSMenu::CVarSplashTimeout.GetValueOnGameThread() <= 0.0f
				? EVisibility::Collapsed : EVisibility::Visible)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
				[
					SNew(STextBlock).Text(LOCTEXT("SplashBrand", "A P O S F E R A")).Font(APSMenu::Font("Bold", 58)).ColorAndOpacity(APSMenu::White)
				]
				+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0.0f, 10.0f, 0.0f, 0.0f)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
					[SNew(SBox).WidthOverride(110.0f).HeightOverride(1.0f)[SNew(SBorder).BorderImage(FAppStyle::GetBrush("WhiteBrush")).BorderBackgroundColor(APSMenu::Cyan)]]
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(18.0f, 0.0f)
					[SNew(STextBlock).Text(LOCTEXT("SplashSubtitle", "S P A C E T R I P S")).Font(APSMenu::Font("Bold", 15)).ColorAndOpacity(APSMenu::Cyan)]
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
					[SNew(SBox).WidthOverride(110.0f).HeightOverride(1.0f)[SNew(SBorder).BorderImage(FAppStyle::GetBrush("WhiteBrush")).BorderBackgroundColor(APSMenu::Cyan)]]
				]
				+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0.0f, 56.0f, 0.0f, 0.0f)
				[
					SNew(STextBlock).Text(LOCTEXT("SplashLoading", "L O A D I N G")).Font(APSMenu::Font("Bold", 11)).ColorAndOpacity(APSMenu::Readable)
				]
			]
		]
	];
	if (IsSplashVisible())
	{
		RegisterActiveTimer(0.0f, FWidgetActiveTimerDelegate::CreateSP(this, &SAPSMainMenuRoot::TickSplash));
	}
	Navigate(EAPSMenuPage::Landing);
}

void SAPSMainMenuRoot::Navigate(EAPSMenuPage NewPage)
{
	if (bHasBuiltCurrentPage && CurrentPage == NewPage && ContentHost.IsValid())
	{
		if (NewPage == EAPSMenuPage::ExistingWorlds)
		{
			RefreshExistingWorlds();
		}
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
		PendingDeleteWorld.Reset();
		PendingBulkDelete.Reset();
		WorldFilterAnchors.Reset();
	}
	PreviousPage = CurrentPage;
	CurrentPage = NewPage;
	if (CurrentPage != EAPSMenuPage::Landing && IsSplashVisible())
	{
		// Any other page (an automation run, a direct open) never waits under the start-up title screen.
		Splash->SetVisibility(EVisibility::Collapsed);
	}
	else if ((CurrentPage == EAPSMenuPage::Landing || CurrentPage == EAPSMenuPage::ChoosePath) && PreviousPage != CurrentPage)
	{
		// The landing and NEW WORLD pages' letter keys need the root's focus: after a page with text fields, and after
		// the clicked button that opened the page was rebuilt away.
		FSlateApplication::Get().SetKeyboardFocus(AsShared(), EFocusCause::SetDirectly);
	}
	if (!ContentHost.IsValid()) return;
	if (CurrentPage != EAPSMenuPage::AstronomicalGeneration)
	{
		WorldGenerationPanel.Reset();
	}
	if ((CurrentPage == EAPSMenuPage::Landing || CurrentPage == EAPSMenuPage::ChoosePath)
		&& ViewModel.IsValid())
	{
		if (ViewModel->GetGenerationRoute() == EAPSGenerationRoute::Planet)
		{
			// Do not leave the editor's body-only recipe armed while the player is in
			// the route selector. The decorative hero itself never consumes this model.
			ViewModel->SetGenerationRoute(EAPSGenerationRoute::Space);
		}
		// Landing and Choose Path share one cheap, deterministic HISM composition.
		// It is presentation-only and explicitly replaced by the normal generator
		// before Astronomical Generation is constructed.
		ViewModel->PresentMainMenuHeroGalaxy();
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
				.HAlign(HAlign_Center).VAlign(VAlign_Center)
				[
					SNew(STextBlock).Text(LOCTEXT("Back", "<   BACK"))
					.Justification(ETextJustify::Center).OverflowPolicy(ETextOverflowPolicy::Ellipsis)
					.Font(APSMenu::Font("Bold", 15)).ColorAndOpacity(APSMenu::White)
					.RenderTransform(APSMenu::CapsShift("Bold", 15))
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
			// Rio 06.10: no "SPACETRIPS GENERATION" line under the brand; the section names the page.
			+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0.0f, 6.0f, 0.0f, 0.0f)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
				[SNew(SBox).WidthOverride(92.0f).HeightOverride(1.0f)[SNew(SBorder).BorderImage(FAppStyle::GetBrush("WhiteBrush")).BorderBackgroundColor(APSMenu::Cyan)]]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(18.0f, 0.0f)
				[SNew(STextBlock).Text(SectionTitle).Font(APSMenu::Font("Bold", 18)).ColorAndOpacity(APSMenu::Cyan).RenderTransform(APSMenu::CapsShift("Bold", 18))]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
				[SNew(SBox).WidthOverride(92.0f).HeightOverride(1.0f)[SNew(SBorder).BorderImage(FAppStyle::GetBrush("WhiteBrush")).BorderBackgroundColor(APSMenu::Cyan)]]
			]
		]
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
		[
			SNew(SBox).WidthOverride(190.0f).HeightOverride(58.0f)
			.VAlign(VAlign_Center)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(5.0f)
				[
					SNew(SBox).WidthOverride(52.0f).HeightOverride(52.0f)
					[
						SNew(SButton).ButtonStyle(&SecondaryButtonStyle).ContentPadding(0.0f)
						.HAlign(HAlign_Center).VAlign(VAlign_Center)
						.OnClicked(this, &SAPSMainMenuRoot::OpenProfile).ToolTipText(LOCTEXT("ProfileTip", "PLAYER PROFILE"))
						[APSMenu::IconBadge(EAPSMenuGlyph::Profile, APSMenu::Cyan, 38.0f)]
					]
				]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(5.0f)
				[
					SNew(SBox).WidthOverride(52.0f).HeightOverride(52.0f)
					[
						SNew(SButton).ButtonStyle(&SecondaryButtonStyle).ContentPadding(0.0f)
						.HAlign(HAlign_Center).VAlign(VAlign_Center)
						.OnClicked(this, &SAPSMainMenuRoot::OpenSettings).ToolTipText(LOCTEXT("SettingsTip", "SETTINGS"))
						[APSMenu::IconBadge(EAPSMenuGlyph::Settings, APSMenu::Cyan, 38.0f)]
					]
				]
			]
		];
}

TSharedRef<SWidget> SAPSMainMenuRoot::BuildLandingPage()
{
	// Rio 05.10-06.10, the "Observatory" (Docs/Design/MAIN_MENU_OBSERVATORY.md): the galaxy fills the screen, the title
	// sits over it as on the generation page, and the actions are one row of chamfered chips along the bottom, each with
	// its key. CONTINUE opens the newest world and the plate above the row names it. World sidecars are tiny, so the scan
	// is cheap enough to run whenever this page is built.
	LoadExistingWorlds();
	LatestWorld = ExistingWorlds.IsEmpty() ? nullptr : ExistingWorlds[0];
	const bool bHasWorld = LatestWorld.IsValid();

	const auto MenuChip = [](const FText& Code, const FText& Label, const FOnClicked& Action, const bool bPrimary,
		const bool bEnabled, const FText& Tooltip, TSharedPtr<SButton>* OutButton) -> TSharedRef<SWidget>
	{
		const TSharedRef<SButton> Button = SNew(SButton)
			.ButtonStyle(FAppStyle::Get(), "NoBorder")
			.ContentPadding(0.0f)
			.IsEnabled(bEnabled)
			.ToolTipText(Tooltip)
			.OnClicked(Action);
		if (OutButton)
		{
			*OutButton = Button;
		}
		const TWeakPtr<SButton> WeakButton = Button;
		const auto Lit = [WeakButton]()
		{
			const TSharedPtr<SButton> Pinned = WeakButton.Pin();
			return Pinned.IsValid() && Pinned->IsHovered();
		};
		const FLinearColor CodeColour = !bEnabled ? APSMenu::Muted : bPrimary ? APSMenu::OnActionCode : APSMenu::Cyan;
		const FLinearColor LabelColour = !bEnabled ? APSMenu::Muted : bPrimary ? APSMenu::OnAction : APSMenu::White;
		Button->SetContent(
			SNew(SBox).HeightOverride(52.0f)
			[
				SNew(SOverlay)
				+ SOverlay::Slot()
				[
					SNew(SChamferedSurface)
					.Brush(FAppStyle::GetBrush("WhiteBrush"))
					.Tint_Lambda([Lit, bPrimary, bEnabled]()
					{
						if (!bEnabled)
						{
							return APSMenu::ChipDisabled;
						}
						if (bPrimary)
						{
							return Lit() ? APSMenu::ActionBright : APSMenu::Amber;
						}
						return Lit() ? APSMenu::ChipLit : APSMenu::ChipRest;
					})
					.ChamferTop(true)
					.ChamferBottom(true)
				]
				+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center).Padding(FMargin(24.0f, 0.0f))
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.0f, 0.0f, 11.0f, 0.0f)
					[
						SNew(STextBlock).Text(Code).Font(APSMenu::Font("Bold", 13)).ColorAndOpacity(CodeColour)
						.RenderTransform(APSMenu::CapsShift("Bold", 13))
					]
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
					[
						SNew(STextBlock).Text(Label).Font(APSMenu::Font("Bold", 15)).ColorAndOpacity(LabelColour)
						.RenderTransform(APSMenu::CapsShift("Bold", 15))
					]
				]
				+ SOverlay::Slot()
				[
					SNew(SChamferedFrame).Thickness(1.0f)
					.Color_Lambda([Lit, bPrimary, bEnabled]()
					{
						if (!bEnabled)
						{
							return APSMenu::DisabledFrame;
						}
						if (bPrimary)
						{
							return Lit() ? APSMenu::ActionPeak : APSMenu::ActionBright;
						}
						return Lit() ? APSMenu::Cyan : APSMenu::CyanDim;
					})
				]
			]);
		return Button;
	};

	TSharedPtr<SButton> ContinueButton;
	const TSharedRef<SHorizontalBox> Chips = SNew(SHorizontalBox);
	const auto AddChip = [&Chips](const TSharedRef<SWidget>& ChipWidget)
	{
		Chips->AddSlot().AutoWidth().Padding(5.0f, 0.0f)[ChipWidget];
	};
	if (bHasWorld)
	{
		AddChip(MenuChip(LOCTEXT("LandingContinueKey", "C"), LOCTEXT("LandingContinue", "CONTINUE"),
			FOnClicked::CreateSP(this, &SAPSMainMenuRoot::ContinueLatestWorld), true, true,
			LOCTEXT("LandingContinueHint", "Open your newest world"), &ContinueButton));
	}
	// Without a world NEW WORLD is the one obvious action.
	AddChip(MenuChip(LOCTEXT("LandingNewWorldKey", "N"), LOCTEXT("LandingNewWorld", "NEW WORLD"),
		FOnClicked::CreateSP(this, &SAPSMainMenuRoot::OpenChoosePath), !bHasWorld, true,
		LOCTEXT("LandingNewWorldHint", "Start a new world: a civilization, open space or an authored map"), nullptr));
	// Rio 06.10: MULTIPLAYER stays in the row, inactive.
	AddChip(MenuChip(LOCTEXT("LandingMultiplayerKey", "M"), LOCTEXT("LandingMultiplayer", "MULTIPLAYER"), FOnClicked(),
		false, false, LOCTEXT("LandingMultiplayerHint", "Multiplayer is coming later"), nullptr));
	AddChip(MenuChip(LOCTEXT("LandingWorldsKey", "W"), LOCTEXT("LandingWorlds", "WORLDS"),
		FOnClicked::CreateSP(this, &SAPSMainMenuRoot::OpenExistingWorlds), false, true,
		LOCTEXT("LandingWorldsHint", "Your saved worlds"), nullptr));
	AddChip(MenuChip(LOCTEXT("LandingProfileKey", "P"), LOCTEXT("LandingProfile", "PROFILE"),
		FOnClicked::CreateSP(this, &SAPSMainMenuRoot::OpenProfile), false, true,
		LOCTEXT("LandingProfileHint", "The pilot's profile"), nullptr));
	AddChip(MenuChip(LOCTEXT("LandingSettingsKey", "S"), LOCTEXT("LandingSettings", "SETTINGS"),
		FOnClicked::CreateSP(this, &SAPSMainMenuRoot::OpenSettings), false, true,
		LOCTEXT("LandingSettingsHint", "Graphics, sound and controls"), nullptr));
	AddChip(MenuChip(LOCTEXT("LandingQuitKey", "Q"), LOCTEXT("LandingQuit", "QUIT"),
		FOnClicked::CreateSP(this, &SAPSMainMenuRoot::QuitGame), false, true,
		LOCTEXT("LandingQuitHint", "Leave the game (the Q key twice)"), nullptr));

	// LAST SESSION: the newest world's name and, from a version 2 sidecar, its home planet and star. Its frame lights
	// with CONTINUE's hover, so it reads as that button's caption.
	FText Where = FText::GetEmpty();
	FText WorldName = FText::GetEmpty();
	if (bHasWorld)
	{
		const FAPSExistingWorldEntry& Latest = *LatestWorld;
		WorldName = FText::FromString(Latest.DisplayName.ToUpper());
		if (Latest.bSystemRecorded && !Latest.System.HomePlanetName.IsEmpty())
		{
			Where = Latest.System.HomeStarName.IsEmpty() ? FText::FromString(Latest.System.HomePlanetName)
				: FText::Format(LOCTEXT("LandingWhere", "{0}, {1} system"), FText::FromString(Latest.System.HomePlanetName),
					FText::FromString(Latest.System.HomeStarName));
		}
	}
	const TWeakPtr<SButton> WeakContinue = ContinueButton;
	const TSharedRef<SWidget> Plate =
		SNew(SBox).HeightOverride(40.0f).Visibility(bHasWorld ? EVisibility::Visible : EVisibility::Collapsed)
		[
			SNew(SOverlay)
			+ SOverlay::Slot()
			[
				SNew(SChamferedSurface).Brush(FAppStyle::GetBrush("WhiteBrush")).Tint(APSMenu::PlateTint)
				.ChamferTop(true).ChamferBottom(true)
			]
			+ SOverlay::Slot().VAlign(VAlign_Center).Padding(FMargin(18.0f, 0.0f))
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
				[
					SNew(STextBlock).Text(LOCTEXT("LandingLastSession", "LAST SESSION")).Font(APSMenu::Font("Bold", 11))
					.ColorAndOpacity(APSMenu::Amber).RenderTransform(APSMenu::CapsShift("Bold", 11))
				]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(14.0f, 0.0f)
				[
					SNew(SBox).WidthOverride(1.0f).HeightOverride(16.0f)
					[SNew(SBorder).BorderImage(FAppStyle::GetBrush("WhiteBrush")).BorderBackgroundColor(APSMenu::CyanDim)]
				]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
				[
					SNew(STextBlock).Text(WorldName).Font(APSMenu::Font("Bold", 13)).ColorAndOpacity(APSMenu::White)
					.RenderTransform(APSMenu::CapsShift("Bold", 13))
				]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(12.0f, 0.0f, 0.0f, 0.0f)
				[
					SNew(STextBlock).Text(Where).Font(APSMenu::Font("Regular", 14)).ColorAndOpacity(APSMenu::Readable)
					.Visibility(Where.IsEmpty() ? EVisibility::Collapsed : EVisibility::Visible)
				]
			]
			+ SOverlay::Slot()
			[
				SNew(SChamferedFrame).Thickness(1.0f)
				.Color_Lambda([WeakContinue]()
				{
					const TSharedPtr<SButton> Pinned = WeakContinue.Pin();
					return Pinned.IsValid() && Pinned->IsHovered() ? APSMenu::Amber : APSMenu::CyanDim;
				})
			]
		];

	return SNew(SOverlay)
		+ SOverlay::Slot()
		[SNew(SObservatoryVeil)]
		+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Top).Padding(0.0f, 52.0f, 0.0f, 0.0f)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
			[
				SNew(STextBlock).Text(LOCTEXT("LandingBrand", "A P O S F E R A")).Font(APSMenu::Font("Bold", 58)).ColorAndOpacity(APSMenu::White)
			]
			+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0.0f, 10.0f, 0.0f, 0.0f)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
				[SNew(SBox).WidthOverride(110.0f).HeightOverride(1.0f)[SNew(SBorder).BorderImage(FAppStyle::GetBrush("WhiteBrush")).BorderBackgroundColor(APSMenu::Cyan)]]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(18.0f, 0.0f)
				[SNew(STextBlock).Text(LOCTEXT("LandingSubtitle", "S P A C E T R I P S")).Font(APSMenu::Font("Bold", 15)).ColorAndOpacity(APSMenu::Cyan)]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
				[SNew(SBox).WidthOverride(110.0f).HeightOverride(1.0f)[SNew(SBorder).BorderImage(FAppStyle::GetBrush("WhiteBrush")).BorderBackgroundColor(APSMenu::Cyan)]]
			]
		]
		+ SOverlay::Slot().HAlign(HAlign_Left).VAlign(VAlign_Top).Padding(40.0f, 48.0f)
		[
			SNew(STextBlock).Text(LOCTEXT("LandingLive", "+  LIVE GALAXY")).Font(APSMenu::Font("Bold", 10))
			.ColorAndOpacity(APSUITheme::Current() == EAPSUITheme::Classic
				? FLinearColor(0.20f, 0.90f, 0.55f, 0.82f) : APSUITheme::Fade(APSMenu::Readable, 0.82f))
		]
		+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Bottom).Padding(0.0f, 0.0f, 0.0f, 30.0f)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0.0f, 0.0f, 0.0f, 18.0f)
			[
				Plate
			]
			+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
			[
				Chips
			]
			+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0.0f, 16.0f, 0.0f, 0.0f)
			[
				// Rio 06.10: no key hint under the buttons; the line only answers the first Q.
				SNew(STextBlock).Font(APSMenu::Font("Regular", 13)).ColorAndOpacity(APSMenu::Readable)
				.Text(LOCTEXT("LandingQuitArmed", "Press Q again to quit"))
				.Visibility_Lambda([this]() { return FPlatformTime::Seconds() < QuitArmedUntil ? EVisibility::HitTestInvisible : EVisibility::Hidden; })
			]
		];
}

FReply SAPSMainMenuRoot::ContinueLatestWorld()
{
	if (LatestWorld.IsValid())
	{
		if (AMainMenuController* PC = Controller.Get())
		{
			PC->LoadWorldSlot(LatestWorld->SaveFileName);
		}
	}
	return FReply::Handled();
}

FReply SAPSMainMenuRoot::OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent)
{
	// Rio 06.10: Esc closes the civilization page's class grid.
	if (bSpawnPickerOpen && CurrentPage == EAPSMenuPage::Civilization && InKeyEvent.GetKey() == EKeys::Escape)
	{
		CloseSpawnPicker();
		return FReply::Handled();
	}
	// The Observatory's keys, on the landing page only and never under the splash or the authored-map dialog.
	const UAPSAuthoredLevelLaunchSubsystem* Launch = GetAuthoredLaunch(Controller.Get());
	if (CurrentPage == EAPSMenuPage::Landing && !InKeyEvent.IsRepeat() && !IsSplashVisible()
		&& !(Launch && Launch->IsDialogVisible()))
	{
		const FKey Key = InKeyEvent.GetKey();
		if (Key == EKeys::C && LatestWorld.IsValid())
		{
			return ContinueLatestWorld();
		}
		if (Key == EKeys::Enter)
		{
			return LatestWorld.IsValid() ? ContinueLatestWorld() : OpenChoosePath();
		}
		if (Key == EKeys::N)
		{
			return OpenChoosePath();
		}
		if (Key == EKeys::W)
		{
			return OpenExistingWorlds();
		}
		if (Key == EKeys::P)
		{
			return OpenProfile();
		}
		if (Key == EKeys::S)
		{
			return OpenSettings();
		}
		if (Key == EKeys::Q)
		{
			// A stray Q never quits: the second one within two seconds does (the hint line says so meanwhile).
			const double Now = FPlatformTime::Seconds();
			if (Now < QuitArmedUntil)
			{
				return QuitGame();
			}
			QuitArmedUntil = Now + 2.0;
			return FReply::Handled();
		}
	}
	if (CurrentPage == EAPSMenuPage::ChoosePath && !InKeyEvent.IsRepeat() && !(Launch && Launch->IsDialogVisible()))
	{
		const FKey Key = InKeyEvent.GetKey();
		if (Key == EKeys::G)
		{
			return PickNewWorldPath(EAPSNewWorldPath::SingleGame);
		}
		if (Key == EKeys::C)
		{
			return PickNewWorldPath(EAPSNewWorldPath::Civilization);
		}
		if (Key == EKeys::S)
		{
			return PickNewWorldPath(EAPSNewWorldPath::Space);
		}
		if (Key == EKeys::Enter)
		{
			return RunNewWorldPath();
		}
		if (Key == EKeys::Escape || Key == EKeys::B)
		{
			return Back();
		}
	}
	return SCompoundWidget::OnKeyDown(MyGeometry, InKeyEvent);
}

bool SAPSMainMenuRoot::IsSplashVisible() const
{
	return Splash.IsValid() && Splash->GetVisibility() != EVisibility::Collapsed;
}

EActiveTimerReturnType SAPSMainMenuRoot::TickSplash(const double InCurrentTime, const float InDeltaTime)
{
	if (!IsSplashVisible())
	{
		return EActiveTimerReturnType::Stop;
	}
	if (SplashShownAt < 0.0)
	{
		SplashShownAt = InCurrentTime;
	}
	const double Shown = InCurrentTime - SplashShownAt;
	if (SplashLiftStarted < 0.0)
	{
		// Rio 06.10: a still title screen while the start-up loads; the menu opens once that is over, in one fade. Over
		// means ten calm frames in a row after a second (the start-up hitches are behind), or the timeout in any case.
		SplashCalmFrames = InDeltaTime < 0.05f ? SplashCalmFrames + 1 : 0;
		if ((Shown >= 1.0 && SplashCalmFrames >= 10) || Shown >= APSMenu::CVarSplashTimeout.GetValueOnGameThread())
		{
			SplashLiftStarted = InCurrentTime;
			UE_LOG(LogTemp, Log, TEXT("[APS.Menu] Splash lifts after %.2f s (calm frames %d)"), Shown, SplashCalmFrames);
		}
		return EActiveTimerReturnType::Continue;
	}
	const float Opacity = 1.0f - FMath::Clamp(static_cast<float>((InCurrentTime - SplashLiftStarted) / 0.5), 0.0f, 1.0f);
	Splash->SetRenderOpacity(Opacity);
	if (Opacity <= 0.0f)
	{
		Splash->SetVisibility(EVisibility::Collapsed);
		return EActiveTimerReturnType::Stop;
	}
	return EActiveTimerReturnType::Continue;
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
		[SNew(SScaleBox).Stretch(EStretch::ScaleToFill)[SNew(SImage).Image(&BackgroundImage).ColorAndOpacity(APSMenu::Retint(FLinearColor(0.10f, 0.20f, 0.30f, 0.36f)))]]
		+ SOverlay::Slot()
		[SNew(SBorder).BorderImage(FAppStyle::GetBrush("WhiteBrush")).BorderBackgroundColor(APSMenu::Retint(FLinearColor(0.0f, 0.008f, 0.016f, 0.78f)))]
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
	// Rio 06.10: one native page in the interface theme instead of the MBLS template panel with our audio block
	// under it (the two overlapped, and the template's sliders drove its own sound classes, not the game's).
	return SNew(SOverlay)
		+ SOverlay::Slot()
		[SNew(SScaleBox).Stretch(EStretch::ScaleToFill)[SNew(SImage).Image(&BackgroundImage).ColorAndOpacity(APSMenu::Retint(FLinearColor(0.10f, 0.20f, 0.30f, 0.36f)))]]
		+ SOverlay::Slot()
		[SNew(SBorder).BorderImage(FAppStyle::GetBrush("WhiteBrush")).BorderBackgroundColor(APSMenu::Retint(FLinearColor(0.0f, 0.008f, 0.016f, 0.78f)))]
		+ SOverlay::Slot().Padding(24.0f)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()[BuildHeader(LOCTEXT("SettingsTitle", "SETTINGS"))]
			+ SVerticalBox::Slot().FillHeight(1.0f).Padding(0.0f, 18.0f, 0.0f, 0.0f)
			[
				SNew(SAPSSettingsPage)
				.World(Controller.IsValid() ? Controller->GetWorld() : nullptr)
				.InitialTab(SettingsTab)
				.OnTabChanged_Lambda([this](const EAPSSettingsTab Tab) { SettingsTab = Tab; })
			]
		];
}

namespace
{
	/** NEW WORLD's left block for each path. */
	struct FNewWorldCopy
	{
		FText Kicker;
		FText Title;
		FText Description;
		/** The amber button's label; empty: no button (SINGLE GAME plays from its list, PLANET is not ready). */
		FText Action;
	};

	FNewWorldCopy NewWorldCopy(const EAPSNewWorldPath Path)
	{
		switch (Path)
		{
		case EAPSNewWorldPath::SingleGame:
			return {LOCTEXT("NewWorldSingleKicker", "AUTHORED"), LOCTEXT("NewWorldSingleTitle", "SINGLE GAME"),
				LOCTEXT("NewWorldSingleDesc", "Authored maps with a set start. Pick one from the list to play it."), FText::GetEmpty()};
		case EAPSNewWorldPath::Space:
			return {LOCTEXT("NewWorldSpaceKicker", "EXPLORATION"), LOCTEXT("NewWorldSpaceTitle", "DEEP SPACE"),
				LOCTEXT("NewWorldSpaceDesc", "Roll stellar systems and clusters, then spawn among them and fly."),
				LOCTEXT("NewWorldSpaceAction", "GENERATE SPACE")};
		case EAPSNewWorldPath::Planet:
			return {LOCTEXT("NewWorldPlanetKicker", "LABORATORY"), LOCTEXT("NewWorldPlanetTitle", "CREATE PLANET"),
				LOCTEXT("NewWorldPlanetDesc", "Design a planet with atmosphere, terrain and moons. Planet Lab is coming later."),
				FText::GetEmpty()};
		default:
			return {LOCTEXT("NewWorldCivKicker", "CREATION"), LOCTEXT("NewWorldCivTitle", "CREATE YOUR CIVILIZATION"),
				LOCTEXT("NewWorldCivDesc", "Set up a civilization and shape its astronomical home."),
				LOCTEXT("NewWorldCivAction", "SET UP CIVILIZATION")};
		}
	}
}

TSharedRef<SWidget> SAPSMainMenuRoot::BuildChoosePathPage()
{
#if WITH_DEV_AUTOMATION_TESTS
	ChoosePathCardButtons.Reset();
	ChoosePathProceduralVisualCount = 0;
	ChoosePathStaticTextureResourceCount = 0;
#endif
	// Rio 06.10, NEW WORLD (Docs/Design/MAIN_MENU_OBSERVATORY.md §3.4; layout from Codex's concept, look from the
	// Observatory): the picked path on the left with its action, its picture on the right, the four paths as chamfered
	// cards with their keys along the bottom. MY WORLDS lives on the landing page (WORLDS); PLANET waits for Planet Lab;
	// SINGLE GAME is a plain list of the authored maps, one for now.
	if (NewWorldPath == EAPSNewWorldPath::Planet)
	{
		NewWorldPath = EAPSNewWorldPath::Civilization;
	}

	const auto Card = [this](const EAPSNewWorldPath Path, const EAPSMenuGlyph Glyph, const FText& Code, const FText& Label,
		const FText& Sub, const bool bEnabled, const FText& Tip) -> TSharedRef<SWidget>
	{
		const TSharedRef<SButton> Button = SNew(SButton)
			.ButtonStyle(FAppStyle::Get(), "NoBorder")
			.ContentPadding(0.0f)
			.IsEnabled(bEnabled)
			.IsFocusable(bEnabled)
			.ToolTipText(Tip)
			.OnClicked(FOnClicked::CreateSP(this, &SAPSMainMenuRoot::PickNewWorldPath, Path));
#if WITH_DEV_AUTOMATION_TESTS
		ChoosePathCardButtons.Add(Button);
		++ChoosePathProceduralVisualCount;
#endif
		const TWeakPtr<SButton> WeakButton = Button;
		const auto Lit = [WeakButton]()
		{
			const TSharedPtr<SButton> Pinned = WeakButton.Pin();
			return Pinned.IsValid() && (Pinned->IsHovered() || Pinned->HasKeyboardFocus());
		};
		const auto Picked = [this, Path]() { return NewWorldPath == Path; };
		const FLinearColor Accent = bEnabled ? APSMenu::Cyan : APSMenu::Quiet;
		Button->SetContent(
			SNew(SBox).HeightOverride(112.0f)
			[
				SNew(SOverlay)
				+ SOverlay::Slot()
				[
					SNew(SChamferedSurface)
					.Brush(FAppStyle::GetBrush("WhiteBrush"))
					.Tint_Lambda([Lit, Picked, bEnabled]()
					{
						if (!bEnabled)
						{
							return APSMenu::ChipDisabled;
						}
						if (Picked())
						{
							return APSMenu::CardSelectedFill;
						}
						return Lit() ? APSMenu::ChipLit : APSMenu::ChipRest;
					})
					.ChamferTop(true)
					.ChamferBottom(true)
				]
				+ SOverlay::Slot().Padding(FMargin(20.0f, 16.0f))
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight()
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
						[
							SNew(SBox).WidthOverride(28.0f).HeightOverride(28.0f)
							[SNew(SVectorMenuGlyph).Glyph(Glyph).Color(Accent).StrokeWidth(1.6f)]
						]
						+ SHorizontalBox::Slot().FillWidth(1.0f)
						+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
						[
							SNew(SBorder).BorderImage(&APSMenu::InsetBrush).Padding(FMargin(8.0f, 3.0f))
							[
								SNew(STextBlock).Text(Code).Font(APSMenu::Font("Bold", 11)).ColorAndOpacity(Accent)
								.RenderTransform(APSMenu::CapsShift("Bold", 11))
							]
						]
					]
					+ SVerticalBox::Slot().FillHeight(1.0f)
					+ SVerticalBox::Slot().AutoHeight()
					[
						SNew(STextBlock).Text(Label).Font(APSMenu::Font("Bold", 17))
						.ColorAndOpacity(bEnabled ? APSMenu::White : APSMenu::Readable)
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 3.0f, 0.0f, 0.0f)
					[
						SNew(STextBlock).Text(Sub).Font(APSMenu::Font("Regular", 16)).ColorAndOpacity(APSMenu::Readable)
					]
				]
				+ SOverlay::Slot()
				[
					SNew(SChamferedFrame).Thickness(1.0f)
					.Color_Lambda([Lit, Picked, bEnabled]()
					{
						if (!bEnabled)
						{
							return APSMenu::DisabledFrame;
						}
						if (Picked())
						{
							return APSMenu::Amber;
						}
						return Lit() ? APSMenu::Cyan : APSMenu::CyanDim;
					})
				]
			]);
		return Button;
	};

	// The picked path: kicker, title, description and the amber action.
	FSlateFontInfo KickerFont = APSMenu::Font("Bold", 13);
	KickerFont.LetterSpacing = 300;
	const TSharedRef<SButton> ActionButton = SNew(SButton)
		.ButtonStyle(FAppStyle::Get(), "NoBorder")
		.ContentPadding(0.0f)
		.OnClicked(this, &SAPSMainMenuRoot::RunNewWorldPath)
		.Visibility_Lambda([this]() { return NewWorldCopy(NewWorldPath).Action.IsEmpty() ? EVisibility::Collapsed : EVisibility::Visible; });
	const TWeakPtr<SButton> WeakAction = ActionButton;
	ActionButton->SetContent(
		SNew(SBox).HeightOverride(60.0f)
		[
			SNew(SOverlay)
			+ SOverlay::Slot()
			[
				SNew(SChamferedSurface).Brush(FAppStyle::GetBrush("WhiteBrush"))
				.Tint_Lambda([WeakAction]()
				{
					const TSharedPtr<SButton> Pinned = WeakAction.Pin();
					return Pinned.IsValid() && Pinned->IsHovered() ? APSMenu::ActionBright : APSMenu::Amber;
				})
				.ChamferTop(true).ChamferBottom(true)
			]
			+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center).Padding(FMargin(30.0f, 0.0f))
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
				[
					SNew(STextBlock).Font(APSMenu::Font("Bold", 16)).ColorAndOpacity(APSMenu::OnAction)
					.RenderTransform(APSMenu::CapsShift("Bold", 16))
					.Text_Lambda([this]() { return NewWorldCopy(NewWorldPath).Action; })
				]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(22.0f, 0.0f, 0.0f, 0.0f)
				[
					SNew(STextBlock).Text(FText::FromString(TEXT(">"))).Font(APSMenu::Font("Bold", 18))
					.ColorAndOpacity(APSMenu::OnAction).RenderTransform(APSMenu::SymbolShift("Bold", 18))
				]
			]
			+ SOverlay::Slot()
			[
				SNew(SChamferedFrame).Thickness(1.0f).Color(APSMenu::ActionBright)
			]
		]);
	const TSharedRef<SWidget> Left =
		SNew(SBox).WidthOverride(600.0f)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(STextBlock).Font(KickerFont).ColorAndOpacity(APSMenu::Cyan)
				.Text_Lambda([this]() { return NewWorldCopy(NewWorldPath).Kicker; })
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 14.0f, 0.0f, 0.0f)
			[
				SNew(STextBlock).Font(APSMenu::Font("Bold", 44)).ColorAndOpacity(APSMenu::White).AutoWrapText(true)
				.Text_Lambda([this]() { return NewWorldCopy(NewWorldPath).Title; })
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 16.0f, 0.0f, 0.0f)
			[
				SNew(STextBlock).Font(APSMenu::Font("Regular", 18)).ColorAndOpacity(APSMenu::Readable).AutoWrapText(true)
				.Text_Lambda([this]() { return NewWorldCopy(NewWorldPath).Description; })
			]
			+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Left).Padding(0.0f, 30.0f, 0.0f, 0.0f)
			[
				ActionButton
			]
		];

	// The picture of the path, or SINGLE GAME's list of authored maps.
	// Rio 06.10: no home-system diagram for CIVILIZATION ("flat; its stars merge with the galaxy; the caption stands still
	// while the system turns"): the live galaxy behind the page is the picture until the menu's new galaxy (APS DEV).
	const auto IsPicture = [this]() { return NewWorldPath == EAPSNewWorldPath::Space; };
	const TSharedRef<SWidget> Right =
		SNew(SOverlay)
		+ SOverlay::Slot()
		[
			SNew(SBox).Visibility_Lambda([IsPicture]() { return IsPicture() ? EVisibility::HitTestInvisible : EVisibility::Collapsed; })
			[
				SNew(SNewWorldVisual).Path_Lambda([this]() { return NewWorldPath; })
			]
		]
		+ SOverlay::Slot().HAlign(HAlign_Left).VAlign(VAlign_Top).Padding(FMargin(0.0f, 24.0f, 0.0f, 0.0f))
		[
			SNew(SVerticalBox).Visibility_Lambda([IsPicture]() { return IsPicture() ? EVisibility::HitTestInvisible : EVisibility::Collapsed; })
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(STextBlock).Font(APSMenu::Font("Bold", 14)).ColorAndOpacity(APSMenu::Amber)
				.Text_Lambda([this]()
				{
					return NewWorldPath == EAPSNewWorldPath::Space ? LOCTEXT("NewWorldSpaceCaption", "STAR CLUSTER")
						: LOCTEXT("NewWorldCivCaption", "YOUR HOME WORLD");
				})
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 4.0f, 0.0f, 0.0f)
			[
				SNew(STextBlock).Font(APSMenu::Font("Regular", 14)).ColorAndOpacity(APSMenu::Readable)
				.Text_Lambda([this]()
				{
					return NewWorldPath == EAPSNewWorldPath::Space ? LOCTEXT("NewWorldSpaceCaptionSub", "Your start is rolled among these stars")
						: LOCTEXT("NewWorldCivCaptionSub", "Its star, planets and moons are yours to shape");
				})
			]
		]
		+ SOverlay::Slot().VAlign(VAlign_Center).Padding(FMargin(20.0f, 0.0f))
		[
			SNew(SBox)
			.Visibility_Lambda([this]() { return NewWorldPath == EAPSNewWorldPath::SingleGame ? EVisibility::Visible : EVisibility::Collapsed; })
			[
				BuildAuthoredMapGrid()
			]
		];

	const TSharedRef<SHorizontalBox> Cards = SNew(SHorizontalBox);
	Cards->AddSlot().FillWidth(1.0f).Padding(8.0f, 0.0f)
	[Card(EAPSNewWorldPath::SingleGame, EAPSMenuGlyph::Ship, LOCTEXT("NewWorldSingleKey", "G"), LOCTEXT("NewWorldSingleCard", "SINGLE GAME"),
		LOCTEXT("NewWorldSingleSub", "Authored maps"), true, LOCTEXT("NewWorldSingleTip", "Play a hand-made map"))];
	Cards->AddSlot().FillWidth(1.0f).Padding(8.0f, 0.0f)
	[Card(EAPSNewWorldPath::Civilization, EAPSMenuGlyph::Civilization, LOCTEXT("NewWorldCivKey", "C"), LOCTEXT("NewWorldCivCard", "CIVILIZATION"),
		LOCTEXT("NewWorldCivSub", "Create civilization"), true, LOCTEXT("NewWorldCivTip", "Generate a civilization"))];
	Cards->AddSlot().FillWidth(1.0f).Padding(8.0f, 0.0f)
	[Card(EAPSNewWorldPath::Space, EAPSMenuGlyph::Space, LOCTEXT("NewWorldSpaceKey", "S"), LOCTEXT("NewWorldSpaceCard", "SPACE"),
		LOCTEXT("NewWorldSpaceSub", "Fly the deep space"), true, LOCTEXT("NewWorldSpaceTip", "Generate space"))];
	Cards->AddSlot().FillWidth(1.0f).Padding(8.0f, 0.0f)
	[Card(EAPSNewWorldPath::Planet, EAPSMenuGlyph::Planet, LOCTEXT("NewWorldPlanetKey", "P"), LOCTEXT("NewWorldPlanetCard", "PLANET"),
		LOCTEXT("NewWorldPlanetSub", "Coming soon"), false, LOCTEXT("PlanetLabSoon", "Planet Lab is coming later"))];

	return SNew(SOverlay)
		+ SOverlay::Slot()
		[SNew(SObservatoryVeil).LeftShade(true)]
		+ SOverlay::Slot()
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight().Padding(28.0f, 16.0f, 28.0f, 8.0f)
			[
				BuildHeader(LOCTEXT("NewWorldSection", "NEW WORLD"))
			]
			+ SVerticalBox::Slot().FillHeight(1.0f).Padding(90.0f, 10.0f, 60.0f, 10.0f)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().FillWidth(0.46f).VAlign(VAlign_Center)
				[
					Left
				]
				+ SHorizontalBox::Slot().FillWidth(0.54f).Padding(30.0f, 0.0f, 0.0f, 0.0f)
				[
					Right
				]
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(82.0f, 0.0f, 82.0f, 0.0f)
			[
				Cards
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(90.0f, 18.0f, 90.0f, 28.0f)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center)
				[
					SNew(STextBlock).Text(LOCTEXT("NewWorldHint", "Choose your path   /   G, C or S picks a card, Enter starts, Esc goes back"))
					.Font(APSMenu::Font("Regular", 14)).ColorAndOpacity(APSMenu::Cyan)
				]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
				[
					SNew(SBox).WidthOverride(16.0f).HeightOverride(16.0f)
					[SNew(SVectorMenuGlyph).Glyph(EAPSMenuGlyph::Lock).Color(APSMenu::Readable).StrokeWidth(1.4f)]
				]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(9.0f, 0.0f, 0.0f, 0.0f)
				[
					SNew(STextBlock).Text(LOCTEXT("NewWorldStory", "STORY MODE   ·   IN DEVELOPMENT")).Font(APSMenu::Font("Bold", 11))
					.ColorAndOpacity(APSMenu::Readable).RenderTransform(APSMenu::CapsShift("Bold", 11))
				]
			]
		];
}

FReply SAPSMainMenuRoot::PickNewWorldPath(const EAPSNewWorldPath Path)
{
	if (Path != EAPSNewWorldPath::Planet)
	{
		NewWorldPath = Path;
	}
	return FReply::Handled();
}

FReply SAPSMainMenuRoot::RunNewWorldPath()
{
	switch (NewWorldPath)
	{
	case EAPSNewWorldPath::SingleGame:
		return StartSingleGame();
	case EAPSNewWorldPath::Space:
		return OpenAstronomicalGeneration(EAstroPreviewFocus::StarCluster, EAPSGenerationRoute::Space);
	case EAPSNewWorldPath::Civilization:
		return OpenAstronomicalGeneration(EAstroPreviewFocus::HomePlanet, EAPSGenerationRoute::Civilization);
	default:
		return FReply::Handled();
	}
}

TSharedRef<SWidget> SAPSMainMenuRoot::BuildExistingWorldsPage()
{
	// A new page starts with an empty search box, so with no search; collection, sort and filters persist.
	WorldSearch.Reset();
	ExistingWorldPage = 0;
	PendingDeleteWorld.Reset();
	PendingBulkDelete.Reset();
	WorldBrowserNotice = FText::GetEmpty();
	WorldFilterAnchors.Reset();
	LoadExistingWorlds();
	const FButtonStyle* NoBorderStyle = &FAppStyle::Get().GetWidgetStyle<FButtonStyle>("NoBorder");

	const auto NavRow = [this](const FText& Label, EAPSWorldCollection Collection, TFunction<int32()> CountGetter,
		const FText& ToolTip)
	{
		const FText Glyph = FText::FromString(Label.ToString().Left(1));
		return SNew(SButton)
			.ButtonStyle(&SecondaryButtonStyle)
			.ToolTipText(ToolTip)
			.ButtonColorAndOpacity_Lambda([this, Collection]()
			{
				return WorldCollection == Collection
					? APSMenu::RetintAction(FLinearColor(0.72f, 0.32f, 0.03f, 1.0f)) : FLinearColor::White;
			})
			.ContentPadding(APSMenu::ButtonPadding("Bold", 13, 14.0f))
			.OnClicked(this, &SAPSMainMenuRoot::SetWorldCollection, Collection)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.0f, 0.0f, 10.0f, 0.0f)
				[APSMenu::Badge(Glyph, APSMenu::Cyan, 26.0f)]
				+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center)
				[
					SNew(STextBlock).Text(Label).Font(APSMenu::Font("Bold", 13)).RenderTransform(APSMenu::CapsShift("Bold", 13))
					.ColorAndOpacity_Lambda([this, Collection](){ return WorldCollection == Collection ? APSMenu::Amber : APSMenu::White; })
				]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
				[
					SNew(STextBlock).Text_Lambda([CountGetter](){ return APSUINumber::Number(CountGetter()); })
					.Font(APSMenu::Font("Bold", 12)).ColorAndOpacity(APSMenu::Readable).RenderTransform(APSMenu::CapsShift("Bold", 12))
				]
			];
	};

	// Rio 03.10 ("the filters are broken"): a label over a dropdown whose options come from the scanned worlds.
	const auto FilterRow = [this](const FText& Label, EAPSWorldFilterKind Kind)
	{
		const TSharedRef<SMenuAnchor> Anchor = SNew(SMenuAnchor)
			.Placement(MenuPlacement_ComboBox)
			.Method(EPopupMethod::UseCurrentWindow)
			.OnGetMenuContent(FOnGetContent::CreateSP(this, &SAPSMainMenuRoot::BuildWorldFilterMenu, Kind));
		Anchor->SetContent(
			SNew(SButton).ButtonStyle(&SecondaryButtonStyle)
			.ContentPadding(APSMenu::ButtonPadding("Bold", 11, 12.0f))
			.OnClicked(this, &SAPSMainMenuRoot::ToggleWorldFilterMenu, Kind)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center)
				[
					SNew(STextBlock).Text_Lambda([this, Kind]() { return GetWorldFilterLabel(Kind); })
					.Font(APSMenu::Font("Bold", 11)).OverflowPolicy(ETextOverflowPolicy::Ellipsis)
					.RenderTransform(APSMenu::CapsShift("Bold", 11))
					.ColorAndOpacity_Lambda([this, Kind]() { return WorldFilterChoices.Contains(Kind) ? APSMenu::Amber : APSMenu::White; })
				]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(8.0f, 0.0f, 0.0f, 0.0f)
				[
					SNew(STextBlock).Text(FText::FromString(TEXT("v"))).Font(APSMenu::Font("Bold", 11))
					.ColorAndOpacity(APSMenu::Cyan).RenderTransform(APSMenu::SymbolShift("Bold", 11))
				]
			]);
		WorldFilterAnchors.Add(Kind, Anchor);
		return SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight().Padding(2.0f, 0.0f, 0.0f, 5.0f)
			[SNew(STextBlock).Text(Label).Font(APSMenu::Font("Bold", 11)).ColorAndOpacity(APSMenu::Readable)]
			+ SVerticalBox::Slot().AutoHeight()[Anchor];
	};

	const auto TopBarButton = [this](const TAttribute<FText>& Label, const FOnClicked& Action, const FLinearColor& Color)
	{
		return SNew(SButton).ButtonStyle(&SecondaryButtonStyle)
			.ContentPadding(APSMenu::ButtonPadding("Bold", 10))
			.HAlign(HAlign_Center).VAlign(VAlign_Center)
			.OnClicked(Action)
			[
				SNew(STextBlock).Text(Label).Justification(ETextJustify::Center)
				.Font(APSMenu::Font("Bold", 10)).ColorAndOpacity(Color).RenderTransform(APSMenu::CapsShift("Bold", 10))
			];
	};
	const auto PageButton = [this](const FText& Label, const int32 Delta)
	{
		return SNew(SBox).MinDesiredWidth(128.0f)
		[
			SNew(SButton).ButtonStyle(&SecondaryButtonStyle)
			.ContentPadding(APSMenu::ButtonPadding("Bold", 10))
			.HAlign(HAlign_Center).VAlign(VAlign_Center)
			.OnClicked(this, &SAPSMainMenuRoot::ChangeExistingWorldPage, Delta)
			[
				SNew(STextBlock).Text(Label).Justification(ETextJustify::Center)
				.Font(APSMenu::Font("Bold", 10)).ColorAndOpacity(APSMenu::Cyan).RenderTransform(APSMenu::CapsShift("Bold", 10))
			]
		];
	};

	// DELETE ALL: whatever the list shows now (Rio 03.10), behind a confirmation that arms after a countdown.
	TSharedRef<SButton> DeleteAllButton = SNew(SButton).ButtonStyle(&DangerButtonStyle)
		.ContentPadding(APSMenu::ButtonPadding("Bold", 11))
		.HAlign(HAlign_Center).VAlign(VAlign_Center)
		.IsEnabled_Lambda([this]() { return ListedWorlds.Num() > 0; })
		.ToolTipText(LOCTEXT("DeleteAllTip", "Delete every world the list shows now: the collection, the search and the filters decide which."))
		.OnClicked(this, &SAPSMainMenuRoot::RequestDeleteListedWorlds);
	const TWeakPtr<SButton> WeakDeleteAll = DeleteAllButton;
	DeleteAllButton->SetContent(
		SNew(STextBlock).Justification(ETextJustify::Center)
		.Font(APSMenu::Font("Bold", 11)).RenderTransform(APSMenu::CapsShift("Bold", 11))
		.Text_Lambda([this]()
		{
			if (ListedWorlds.IsEmpty())
			{
				return LOCTEXT("DeleteAllNone", "DELETE ALL");
			}
			return HasWorldNarrowing()
				? FText::Format(LOCTEXT("DeleteShown", "DELETE {0} SHOWN"), APSUINumber::Number(ListedWorlds.Num()))
				: FText::Format(LOCTEXT("DeleteAllCount", "DELETE ALL {0}"), APSUINumber::Number(ListedWorlds.Num()));
		})
		.ColorAndOpacity_Lambda([this, WeakDeleteAll]()
		{
			const TSharedPtr<SButton> Button = WeakDeleteAll.Pin();
			return ListedWorlds.IsEmpty() ? APSMenu::Muted
				: Button.IsValid() && Button->IsHovered() ? APSMenu::DangerBright : APSMenu::Danger;
		}));

	TSharedRef<SWidget> Page = SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().Padding(30.0f, 18.0f, 30.0f, 8.0f)[BuildHeader(LOCTEXT("VisitExisting", "VISIT EXISTING WORLD"))]
		+ SVerticalBox::Slot().FillHeight(1.0f).Padding(24.0f, 8.0f, 24.0f, 24.0f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(0.17f).Padding(5.0f)
			[
				SNew(SAPSChamferedOverlay)
				+ SOverlay::Slot()
				[
					SNew(SBorder).BorderImage(&APSMenu::PanelBrush).Padding(14.0f)
					[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 5.0f)
					[NavRow(LOCTEXT("AllWorlds", "ALL WORLDS"), EAPSWorldCollection::All, [this](){ return ExistingWorlds.Num(); },
						LOCTEXT("AllWorldsTip", "Every world save."))]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 5.0f)
					[NavRow(LOCTEXT("Favorites", "FAVORITES"), EAPSWorldCollection::Favorites,
						[this](){ int32 Count=0; for(const TSharedPtr<FAPSExistingWorldEntry>& E:ExistingWorlds){ if(E.IsValid()&&E->bFavorite){++Count;} } return Count; },
						LOCTEXT("FavoritesTip", "Worlds marked with the star on their card."))]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 5.0f)
					[NavRow(LOCTEXT("Recent", "RECENT"), EAPSWorldCollection::Recent, [this](){ return FMath::Min(APSMenu::RecentWorldCount, ExistingWorlds.Num()); },
						LOCTEXT("RecentTip", "The six worlds saved last."))]
					+ SVerticalBox::Slot().AutoHeight().Padding(2.0f, 18.0f, 0.0f, 8.0f)
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center)
						[SNew(STextBlock).Text(LOCTEXT("Filters", "FILTERS")).Font(APSMenu::Font("Bold", 12)).ColorAndOpacity(APSMenu::Cyan).RenderTransform(APSMenu::CapsShift("Bold", 12))]
						+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
						[
							SNew(SButton).ButtonStyle(NoBorderStyle).ContentPadding(FMargin(8.0f, 4.0f))
							.Visibility_Lambda([this]() { return WorldFilterChoices.Num() > 0 ? EVisibility::Visible : EVisibility::Hidden; })
							.ToolTipText(LOCTEXT("ClearFiltersTip", "Show every star, world type and environment again."))
							.OnClicked(this, &SAPSMainMenuRoot::ClearWorldFilters)
							[SNew(STextBlock).Text(LOCTEXT("ClearFilters", "CLEAR")).Font(APSMenu::Font("Bold", 10)).ColorAndOpacity(APSMenu::Amber).RenderTransform(APSMenu::CapsShift("Bold", 10))]
						]
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 10.0f)[FilterRow(LOCTEXT("StarTypeFilter", "STAR TYPE"), EAPSWorldFilterKind::StarType)]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 10.0f)[FilterRow(LOCTEXT("WorldTypeFilter", "WORLD TYPE"), EAPSWorldFilterKind::WorldType)]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 10.0f)[FilterRow(LOCTEXT("InhabitedFilter", "INHABITED"), EAPSWorldFilterKind::Inhabited)]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 10.0f)[FilterRow(LOCTEXT("EnvironmentFilter", "ENVIRONMENT"), EAPSWorldFilterKind::Environment)]
					+ SVerticalBox::Slot().AutoHeight().Padding(2.0f, 4.0f, 0.0f, 0.0f)
					[
						SNew(STextBlock)
						.Text_Lambda([this]()
						{
							const int32 Total = ExistingWorlds.Num();
							const FText TotalText = Total == 1 ? LOCTEXT("OneWorldCount", "1 WORLD")
								: FText::Format(LOCTEXT("WorldsCount", "{0} WORLDS"), APSUINumber::Number(Total));
							return HasWorldNarrowing()
								? FText::Format(LOCTEXT("WorldsShownCount", "{0} OF {1}"), APSUINumber::Number(ListedWorlds.Num()), TotalText)
								: TotalText;
						})
						.Font(APSMenu::Font("Bold", 12))
						.ColorAndOpacity_Lambda([this]() { return HasWorldNarrowing() ? APSMenu::Amber : APSMenu::Readable; })
					]
					+ SVerticalBox::Slot().FillHeight(1.0f)
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 8.0f)[DeleteAllButton]
					+ SVerticalBox::Slot().AutoHeight()
					[
						SNew(SButton).IsEnabled(false).ToolTipText(LOCTEXT("ImportSaveHint", "Place .sav files in Saved/SaveGames; the browser discovers them without blocking."))
						.ButtonStyle(&SecondaryButtonStyle).ContentPadding(APSMenu::ButtonPadding("Bold", 11)).HAlign(HAlign_Center).VAlign(VAlign_Center)
						[SNew(STextBlock).Text(LOCTEXT("ImportSave", "IMPORT SAVE")).Justification(ETextJustify::Center).Font(APSMenu::Font("Bold", 11)).ColorAndOpacity(APSMenu::Muted).RenderTransform(APSMenu::CapsShift("Bold", 11))]
					]
					]
				]
				+ SOverlay::Slot()
				[
					SNew(SChamferedFrame).Color(APSMenu::Cyan).Thickness(APSMenu::CardPerimeterThickness)
				]
			]
			+ SHorizontalBox::Slot().FillWidth(0.62f).Padding(5.0f)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 8.0f)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center)
					[SNew(SSearchBox).HintText(LOCTEXT("SearchWorlds", "Search worlds...")).OnTextChanged(this, &SAPSMainMenuRoot::OnWorldSearchChanged)]
					+ SHorizontalBox::Slot().AutoWidth().Padding(8.0f, 0.0f, 0.0f, 0.0f)
					[TopBarButton(LOCTEXT("RefreshWorlds", "REFRESH"), FOnClicked::CreateLambda([this](){ RefreshExistingWorlds(); return FReply::Handled(); }), APSMenu::Cyan)]
					+ SHorizontalBox::Slot().AutoWidth().Padding(8.0f, 0.0f, 0.0f, 0.0f)
					[TopBarButton(TAttribute<FText>::CreateLambda([this](){ return GetWorldSortLabel(); }), FOnClicked::CreateSP(this, &SAPSMainMenuRoot::CycleWorldSort), APSMenu::White)]
					+ SHorizontalBox::Slot().AutoWidth().Padding(8.0f, 0.0f, 0.0f, 0.0f)
					[TopBarButton(TAttribute<FText>::CreateLambda([this](){ return FText::FromString(bCompactWorldList ? TEXT("LIST") : TEXT("GRID")); }), FOnClicked::CreateSP(this, &SAPSMainMenuRoot::ToggleWorldView), APSMenu::Cyan)]
				]
				+ SVerticalBox::Slot().FillHeight(1.0f)
				[
					APSMenu::ChamferPanel(SAssignNew(ExistingWorldGridHost, SBox), FMargin(10.0f))
				]
				+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0.0f, 8.0f, 0.0f, 0.0f)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(3.0f)[PageButton(LOCTEXT("PreviousPage", "<<  PREVIOUS"), -1)]
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(14.0f, 0.0f)
					[
						SNew(STextBlock)
						.Text_Lambda([this]()
						{
							const int32 Pages = FMath::Max(1, FMath::DivideAndRoundUp(ListedWorlds.Num(), 6));
							return FText::Format(LOCTEXT("PageOfPages", "{0} / {1}"), APSUINumber::Number(ExistingWorldPage + 1), APSUINumber::Number(Pages));
						})
						.Font(APSMenu::Font("Bold", 12)).ColorAndOpacity(APSMenu::White).RenderTransform(APSMenu::CapsShift("Bold", 12))
					]
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(3.0f)[PageButton(LOCTEXT("NextPage", "NEXT  >>"), 1)]
				]
			]
			+ SHorizontalBox::Slot().FillWidth(0.21f).Padding(5.0f)
			[
				APSMenu::ChamferPanel(SAssignNew(ExistingWorldDetailsHost, SBox), FMargin(16.0f))
			]
		];

	ApplyExistingWorldView();
	BeginExistingWorldMetadataLoad();
	return SNew(SOverlay)
		+ SOverlay::Slot()
		[SNew(SScaleBox).Stretch(EStretch::ScaleToFill)[SNew(SImage).Image(&BackgroundImage).ColorAndOpacity(APSMenu::Retint(FLinearColor(0.16f, 0.26f, 0.35f, 0.24f)))]]
		+ SOverlay::Slot()
		[SNew(SBorder).BorderImage(FAppStyle::GetBrush("WhiteBrush")).BorderBackgroundColor(APSMenu::Retint(FLinearColor(0.0f, 0.010f, 0.020f, 0.52f)))]
		+ SOverlay::Slot()[Page]
		+ SOverlay::Slot()[BuildDeleteWorldDialog()]
		+ SOverlay::Slot()[BuildDeleteAllDialog()];
}

TSharedRef<SWidget> SAPSMainMenuRoot::BuildDeleteWorldDialog()
{
	using APSMenu::DialogButton;
	// A dimmed backdrop over the whole browser: a click beside the dialog cancels, one inside it does not.
	return SNew(SBorder)
		.BorderImage(FAppStyle::GetBrush("WhiteBrush"))
		.BorderBackgroundColor(APSMenu::Retint(FLinearColor(0.0f, 0.004f, 0.010f, 0.80f)))
		.Visibility_Lambda([this]() { return PendingDeleteWorld.IsValid() ? EVisibility::Visible : EVisibility::Collapsed; })
		.OnMouseButtonDown_Lambda([this](const FGeometry&, const FPointerEvent&)
		{
			PendingDeleteWorld.Reset();
			return FReply::Handled();
		})
		.HAlign(HAlign_Center).VAlign(VAlign_Center).Padding(32.0f)
		[
			SNew(SBox).WidthOverride(640.0f)
			[
				SNew(SBorder).BorderImage(FStyleDefaults::GetNoBrush()).Padding(0.0f)
				.OnMouseButtonDown_Lambda([](const FGeometry&, const FPointerEvent&) { return FReply::Handled(); })
				[
					APSMenu::ChamferPanel(
						SNew(SVerticalBox)
						+ SVerticalBox::Slot().AutoHeight()
						[
							SNew(STextBlock)
							.Text_Lambda([this]()
							{
								return FText::Format(LOCTEXT("DeleteWorldQuestion", "DELETE WORLD “{0}”?"),
									FText::FromString(PendingDeleteWorld.IsValid() ? PendingDeleteWorld->DisplayName : FString()));
							})
							.AutoWrapText(true).Font(APSMenu::Font("Bold", 18)).ColorAndOpacity(APSMenu::White)
						]
						+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 12.0f, 0.0f, 0.0f)
						[
							SNew(STextBlock).AutoWrapText(true)
							.Text(LOCTEXT("DeleteWorldWarning", "Its save files will be removed. This cannot be undone."))
							.Font(APSMenu::Font("Regular", 14)).ColorAndOpacity(APSMenu::Readable)
						]
						+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 8.0f, 0.0f, 0.0f)
						[
							// Display names repeat ("Mevelex Cluster"); the slot names the exact files.
							SNew(STextBlock).AutoWrapText(true)
							.Text_Lambda([this]()
							{
								return PendingDeleteWorld.IsValid()
									? FText::Format(LOCTEXT("DeleteWorldFiles", "{0}.sav  +  .apsmeta   •   {1}"),
										FText::FromString(PendingDeleteWorld->SaveFileName),
										APSMenu::SaveSizeText(PendingDeleteWorld->FileSizeBytes))
									: FText::GetEmpty();
							})
							.Font(APSMenu::Font("Regular", 12)).ColorAndOpacity(APSMenu::Danger)
						]
						+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Right).Padding(0.0f, 22.0f, 0.0f, 0.0f)
						[
							SNew(SHorizontalBox)
							+ SHorizontalBox::Slot().AutoWidth().Padding(0.0f, 0.0f, 10.0f, 0.0f)
							[DialogButton(&SecondaryButtonStyle, LOCTEXT("DeleteWorldCancel", "CANCEL"), APSMenu::White,
								FOnClicked::CreateSP(this, &SAPSMainMenuRoot::CancelDeleteExistingWorld))]
							+ SHorizontalBox::Slot().AutoWidth()
							[DialogButton(&DangerConfirmButtonStyle, LOCTEXT("DeleteWorldConfirm", "DELETE"), APSMenu::DangerBright,
								FOnClicked::CreateSP(this, &SAPSMainMenuRoot::ConfirmDeleteExistingWorld))]
						]
					, FMargin(28.0f, 24.0f), APSMenu::Danger, 1.2f)
				]
			]
		];
}

TSharedRef<SWidget> SAPSMainMenuRoot::BuildDeleteAllDialog()
{
	using APSMenu::DialogButton;
	// Rio 03.10: DELETE ALL removes every world the collection, search and filters list. The dialog names the count and
	// the size, and its DELETE ALL arms only after a 3 s countdown, so wiping the list always takes a second, deliberate
	// click.
	const auto SecondsToArm = [this]()
	{
		return FMath::Max(0.0, BulkDeleteArmTime - FSlateApplication::Get().GetCurrentTime());
	};
	return SNew(SBorder)
		.BorderImage(FAppStyle::GetBrush("WhiteBrush"))
		.BorderBackgroundColor(APSMenu::Retint(FLinearColor(0.0f, 0.004f, 0.010f, 0.80f)))
		.Visibility_Lambda([this]() { return PendingBulkDelete.IsEmpty() ? EVisibility::Collapsed : EVisibility::Visible; })
		.OnMouseButtonDown_Lambda([this](const FGeometry&, const FPointerEvent&)
		{
			PendingBulkDelete.Reset();
			return FReply::Handled();
		})
		.HAlign(HAlign_Center).VAlign(VAlign_Center).Padding(32.0f)
		[
			SNew(SBox).WidthOverride(680.0f)
			[
				SNew(SBorder).BorderImage(FStyleDefaults::GetNoBrush()).Padding(0.0f)
				.OnMouseButtonDown_Lambda([](const FGeometry&, const FPointerEvent&) { return FReply::Handled(); })
				[
					APSMenu::ChamferPanel(
						SNew(SVerticalBox)
						+ SVerticalBox::Slot().AutoHeight()
						[
							SNew(STextBlock)
							.Text_Lambda([this]()
							{
								return PendingBulkDelete.Num() == 1
									? LOCTEXT("DeleteAllQuestionOne", "DELETE 1 WORLD?")
									: FText::Format(LOCTEXT("DeleteAllQuestion", "DELETE {0} WORLDS?"),
										APSUINumber::Number(PendingBulkDelete.Num()));
							})
							.AutoWrapText(true).Font(APSMenu::Font("Bold", 18)).ColorAndOpacity(APSMenu::White)
						]
						+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 12.0f, 0.0f, 0.0f)
						[
							SNew(STextBlock).AutoWrapText(true)
							.Text_Lambda([this]()
							{
								int64 Bytes = 0;
								for (const TSharedPtr<FAPSExistingWorldEntry>& Entry : PendingBulkDelete)
								{
									Bytes += Entry.IsValid() ? Entry->FileSizeBytes : 0;
								}
								return FText::Format(PendingBulkDelete.Num() == 1
									? LOCTEXT("DeleteAllWarningOne", "Its save file and record will be removed (≈{0}). This cannot be undone.")
									: LOCTEXT("DeleteAllWarning", "All their save files and records will be removed (≈{0}). This cannot be undone."),
									APSMenu::SaveSizeText(Bytes));
							})
							.Font(APSMenu::Font("Regular", 14)).ColorAndOpacity(APSMenu::Readable)
						]
						+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 8.0f, 0.0f, 0.0f)
						[
							// The exact slots (display names repeat), and the worlds a running game keeps.
							SNew(STextBlock).AutoWrapText(true)
							.Text_Lambda([this]()
							{
								TArray<FString> Names;
								int32 Kept = 0;
								for (const TSharedPtr<FAPSExistingWorldEntry>& Entry : PendingBulkDelete)
								{
									if (!Entry.IsValid())
									{
										continue;
									}
									if (!CanDeleteExistingWorld(*Entry))
									{
										++Kept;
									}
									else if (Names.Num() < 3)
									{
										Names.Add(Entry->SaveFileName);
									}
								}
								const int32 More = PendingBulkDelete.Num() - Kept - Names.Num();
								FText Line = More > 0
									? FText::Format(LOCTEXT("DeleteAllNamesMore", "{0} and {1} more"),
										FText::FromString(FString::Join(Names, TEXT(", "))), APSUINumber::Number(More))
									: FText::FromString(FString::Join(Names, TEXT(", ")));
								if (Kept > 0)
								{
									Line = FText::Format(LOCTEXT("DeleteAllKept", "{0}   •   {1} open in a running game will be kept"),
										Line, APSUINumber::Number(Kept));
								}
								return Line;
							})
							.Font(APSMenu::Font("Regular", 12)).ColorAndOpacity(APSMenu::Danger)
						]
						+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Right).Padding(0.0f, 22.0f, 0.0f, 0.0f)
						[
							SNew(SHorizontalBox)
							+ SHorizontalBox::Slot().AutoWidth().Padding(0.0f, 0.0f, 10.0f, 0.0f)
							[DialogButton(&SecondaryButtonStyle, LOCTEXT("DeleteAllCancel", "CANCEL"), APSMenu::White,
								FOnClicked::CreateSP(this, &SAPSMainMenuRoot::CancelDeleteListedWorlds))]
							+ SHorizontalBox::Slot().AutoWidth()
							[
								DialogButton(&DangerConfirmButtonStyle,
									TAttribute<FText>::CreateLambda([SecondsToArm]()
									{
										const int32 Seconds = FMath::CeilToInt32(SecondsToArm());
										return Seconds > 0
											? FText::Format(LOCTEXT("DeleteAllArming", "DELETE ALL ({0})"), APSUINumber::Number(Seconds))
											: LOCTEXT("DeleteAllConfirm", "DELETE ALL");
									}),
									TAttribute<FSlateColor>::CreateLambda([SecondsToArm]()
									{
										return FSlateColor(SecondsToArm() > 0.0 ? APSMenu::SRGB(150, 112, 106) : APSMenu::DangerBright);
									}),
									FOnClicked::CreateSP(this, &SAPSMainMenuRoot::ConfirmDeleteListedWorlds),
									TAttribute<bool>::CreateLambda([SecondsToArm]() { return SecondsToArm() <= 0.0; }),
									// Wide enough for the countdown and the armed label alike: nothing shifts when it arms.
									210.0f)
							]
						]
					, FMargin(28.0f, 24.0f), APSMenu::Danger, 1.2f)
				]
			]
		];
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
	SaveFiles.Sort();
	int32 SkippedSlots = 0;
	for (const FString& SaveFile : SaveFiles)
	{
		const FString SlotName = FPaths::GetBaseFilename(SaveFile);
		if (!APSMenu::IsWorldSaveSlot(SlotName))
		{
			++SkippedSlots;
			continue;
		}
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
			Entry->bSystemRecorded = false;
			Entry->System = FAPSWorldSystemRecord();
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
	// Otherwise the newest world the collection, search and filters let through.
	const TSharedPtr<FAPSExistingWorldEntry>* FirstListed = RestoredSelection ? nullptr : ExistingWorlds.FindByPredicate(
		[this](const TSharedPtr<FAPSExistingWorldEntry>& Entry) { return Entry.IsValid() && PassesExistingWorldFilters(*Entry); });
	SelectedWorld = RestoredSelection ? *RestoredSelection : (FirstListed ? *FirstListed : nullptr);
	ExistingWorldDirectoryFingerprint = ComputeExistingWorldDirectoryFingerprint();
	UE_LOG(LogTemp, Log, TEXT("[APS.WorldBrowser] Scanned directory=%s saves=%d skippedNonWorld=%d"),
		*SaveDirectory, ExistingWorlds.Num(), SkippedSlots);
}

uint32 SAPSMainMenuRoot::ComputeExistingWorldDirectoryFingerprint() const
{
	const FString SaveDirectory = FPaths::ProjectSavedDir() / TEXT("SaveGames");
	TArray<FString> SaveFiles;
	IFileManager::Get().FindFiles(SaveFiles, *SaveDirectory, TEXT("*.sav"));
	// Settings saves change with every volume slider; only world slots refresh the browser.
	SaveFiles.RemoveAll([](const FString& SaveFile) { return !APSMenu::IsWorldSaveSlot(FPaths::GetBaseFilename(SaveFile)); });
	SaveFiles.Sort();
	uint32 Fingerprint = GetTypeHash(SaveFiles.Num());
	for (const FString& SaveFile : SaveFiles)
	{
		const FFileStatData Stat = IFileManager::Get().GetStatData(*(SaveDirectory / SaveFile));
		Fingerprint = HashCombineFast(Fingerprint, GetTypeHash(SaveFile));
		Fingerprint = HashCombineFast(Fingerprint, GetTypeHash(Stat.FileSize));
		Fingerprint = HashCombineFast(Fingerprint,
			GetTypeHash(Stat.ModificationTime.ToUnixTimestamp()));
	}
	return Fingerprint;
}

void SAPSMainMenuRoot::RefreshExistingWorlds()
{
	if (CurrentPage != EAPSMenuPage::ExistingWorlds)
	{
		return;
	}
	if (AMainMenuController* PC = Controller.Get())
	{
		PC->CancelWorldMetadataLoad();
	}
	// Collection, search and filters survive the rescan; only what they list may stay selected.
	LoadExistingWorlds();
	ApplyExistingWorldView();
	BeginExistingWorldMetadataLoad();
}

void SAPSMainMenuRoot::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime,
	const float InDeltaTime)
{
	SCompoundWidget::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);
	if (AMainMenuController* PC = Controller.Get())
	{
		if (UAPSAudioSubsystem* Audio = PC->GetWorld()->GetSubsystem<UAPSAudioSubsystem>())
		{
			Audio->ApplyButtonSounds(PrimaryButtonStyle);
			Audio->ApplyButtonSounds(SecondaryButtonStyle);
			Audio->ApplyButtonSounds(CardButtonStyle);
			Audio->ApplyButtonSounds(DangerButtonStyle);
			Audio->ApplyButtonSounds(DangerConfirmButtonStyle);
			Audio->ApplyButtonSounds(DropdownOptionStyle);
		}
	}
	if (CurrentPage != EAPSMenuPage::ExistingWorlds
		|| InCurrentTime < NextExistingWorldRefreshTime)
	{
		return;
	}
	NextExistingWorldRefreshTime = InCurrentTime + 1.0;
	if (ComputeExistingWorldDirectoryFingerprint() != ExistingWorldDirectoryFingerprint)
	{
		UE_LOG(LogTemp, Log, TEXT("[APS.WorldBrowser] Save directory changed; refreshing"));
		RefreshExistingWorlds();
	}
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
	// The record can move the selected world out of the filters; the view then drops the selection.
	if (SelectedWorld == *Found)
	{
		ApplyExistingWorldView();
	}
	else
	{
		RebuildExistingWorldGrid();
	}
}

void SAPSMainMenuRoot::RebuildExistingWorldGrid()
{
	if (!ExistingWorldGridHost) return;
	RebuildWorldFilterOptions();
	TArray<TSharedPtr<FAPSExistingWorldEntry>> Filtered;
	for (const TSharedPtr<FAPSExistingWorldEntry>& Entry : ExistingWorlds)
	{
		if (Entry.IsValid() && PassesExistingWorldFilters(*Entry))
		{
			Filtered.Add(Entry);
		}
	}
	if (WorldCollection == EAPSWorldCollection::Recent && Filtered.Num() > APSMenu::RecentWorldCount)
	{
		Filtered.SetNum(APSMenu::RecentWorldCount, EAllowShrinking::No);
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
	ListedWorlds = Filtered;
	constexpr int32 ItemsPerPage = 6;
	const int32 MaxPage = FMath::Max(0, FMath::DivideAndRoundUp(Filtered.Num(), ItemsPerPage) - 1);
	ExistingWorldPage = FMath::Clamp(ExistingWorldPage, 0, MaxPage);

	TSharedRef<SUniformGridPanel> Grid = SNew(SUniformGridPanel).SlotPadding(FMargin(5.0f));
	const int32 FirstIndex = ExistingWorldPage * ItemsPerPage;
	const int32 LastIndex = FMath::Min(FirstIndex + ItemsPerPage, Filtered.Num());
	const FButtonStyle* NoBorderStyle = &FAppStyle::Get().GetWidgetStyle<FButtonStyle>("NoBorder");
	const FSlateBrush* WhiteBrush = FAppStyle::GetBrush("WhiteBrush");
	for (int32 SourceIndex = FirstIndex; SourceIndex < LastIndex; ++SourceIndex)
	{
		const int32 VisibleIndex = SourceIndex - FirstIndex;
		const TSharedPtr<FAPSExistingWorldEntry>& Entry = Filtered[SourceIndex];
		const int32 ColumnCount = bCompactWorldList ? 1 : 3;
		// Rio 03.10: the world's own scheme instead of the stock picture, and captions that read.
		const FText Chip = APSMenu::WorldChipText(*Entry);
		const TSharedRef<SWidget> Scheme = SNew(SAPSWorldSchemePreview).Input(APSMenu::SchemeInput(*Entry));
		const TSharedRef<SWidget> Caption = SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(STextBlock).Text(FText::FromString(Entry->DisplayName)).OverflowPolicy(ETextOverflowPolicy::Ellipsis)
				.Font(APSMenu::Font("Bold", 15)).ColorAndOpacity(APSMenu::White)
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 5.0f, 0.0f, 0.0f)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center)
				[
					// An old record's note is a quiet footnote, not a caption (Rio 03.10).
					SNew(STextBlock).Text(APSMenu::WorldCardLine(*Entry)).OverflowPolicy(ETextOverflowPolicy::Ellipsis)
					.Font(Entry->bSystemRecorded ? APSMenu::Font("Bold", 10) : APSMenu::Font("Regular", 11))
					.ColorAndOpacity(Entry->bSystemRecorded ? APSMenu::Cyan : APSMenu::Quiet)
				]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(10.0f, 0.0f, 0.0f, 0.0f)
				[
					SNew(STextBlock).Text(APSMenu::LocalTimeText(Entry->FileTimestamp, TEXT("%d %b %Y")))
					.Font(APSMenu::Font("Regular", 11)).ColorAndOpacity(APSMenu::Readable)
				]
			];
		TSharedPtr<SWidget> Body;
		if (bCompactWorldList)
		{
			Body = SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth()[SNew(SBox).WidthOverride(250.0f)[Scheme]]
				+ SHorizontalBox::Slot().AutoWidth()
				[SNew(SBox).WidthOverride(1.0f)[SNew(SBorder).BorderImage(WhiteBrush).BorderBackgroundColor(APSMenu::CyanDim)]]
				+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center).Padding(18.0f, 8.0f, 54.0f, 8.0f)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight()[Caption]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 6.0f, 0.0f, 0.0f)
					[
						SNew(STextBlock).Text(Chip).Visibility(Chip.IsEmpty() ? EVisibility::Collapsed : EVisibility::Visible)
						.Font(APSMenu::Font("Bold", 10)).ColorAndOpacity(APSMenu::White)
					]
				];
		}
		else
		{
			Body = SNew(SVerticalBox)
				+ SVerticalBox::Slot().FillHeight(1.0f)[Scheme]
				+ SVerticalBox::Slot().AutoHeight()
				[SNew(SBox).HeightOverride(1.0f)[SNew(SBorder).BorderImage(WhiteBrush).BorderBackgroundColor(APSMenu::CyanDim)]]
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(SBorder).BorderImage(WhiteBrush).BorderBackgroundColor(APSMenu::Retint(FLinearColor(0.001f, 0.008f, 0.016f, 0.97f)))
					.Padding(FMargin(14.0f, 9.0f, 14.0f, 11.0f))
					[Caption]
				];
		}

		TSharedRef<SButton> Card = SNew(SButton).ButtonStyle(NoBorderStyle)
			.OnClicked(this, &SAPSMainMenuRoot::SelectExistingWorld, Entry).ContentPadding(0.0f);
		const TWeakPtr<SButton> WeakCard = Card;
		const TSharedPtr<FAPSExistingWorldEntry> CardEntry = Entry;
		Card->SetContent(
			SNew(SAPSChamferedOverlay)
			+ SOverlay::Slot()
			[SNew(SBox).HeightOverride(bCompactWorldList ? 112.0f : 218.0f)[Body.ToSharedRef()]]
			+ SOverlay::Slot().HAlign(HAlign_Left).VAlign(VAlign_Top).Padding(10.0f)
			[
				SNew(SBox).MaxDesiredWidth(300.0f)
				.Visibility(bCompactWorldList || Chip.IsEmpty() ? EVisibility::Collapsed : EVisibility::HitTestInvisible)
				[
					SNew(SBorder).BorderImage(&APSMenu::InsetBrush).Padding(FMargin(10.0f, 6.0f))
					.HAlign(HAlign_Center).VAlign(VAlign_Center)
					[
						SNew(STextBlock).Text(Chip).OverflowPolicy(ETextOverflowPolicy::Ellipsis)
						.Font(APSMenu::Font("Bold", 10)).ColorAndOpacity(APSMenu::White)
						.RenderTransform(APSMenu::CapsShift("Bold", 10))
					]
				]
			]
			+ SOverlay::Slot().HAlign(HAlign_Right).VAlign(VAlign_Top).Padding(6.0f)
			[
				// The code-native star: the font has no "★" glyph.
				SNew(SButton).ButtonStyle(NoBorderStyle).ContentPadding(5.0f)
				.OnClicked(this, &SAPSMainMenuRoot::ToggleWorldFavorite, Entry)
				.ToolTipText(Entry->bFavorite ? LOCTEXT("UnfavoriteWorld", "Remove from favorites")
					: LOCTEXT("FavoriteWorld", "Add to favorites"))
				[
					SNew(SBox).WidthOverride(20.0f).HeightOverride(20.0f)
					[
						SNew(SVectorMenuGlyph).Glyph(EAPSMenuGlyph::Favorite)
						.Color(Entry->bFavorite ? APSMenu::Amber : APSMenu::Retint(FLinearColor(0.80f, 0.88f, 0.90f, 0.70f)))
						.StrokeWidth(Entry->bFavorite ? 2.0f : 1.4f)
					]
				]
			]
			+ SOverlay::Slot()
			[
				SNew(SChamferedFrame)
				.Color_Lambda([this, WeakCard, CardEntry]()
				{
					if (SelectedWorld == CardEntry)
					{
						return APSMenu::Amber;
					}
					const TSharedPtr<SButton> Button = WeakCard.Pin();
					return Button.IsValid() && Button->IsHovered() ? APSMenu::Cyan : APSMenu::CyanDim;
				})
				.Thickness(APSMenu::CardPerimeterThickness)
			]);
		Grid->AddSlot(VisibleIndex % ColumnCount, VisibleIndex / ColumnCount)[Card];
	}
	ExistingWorldGridHost->SetContent(Grid);
}

void SAPSMainMenuRoot::RebuildExistingWorldDetails()
{
	if (!ExistingWorldDetailsHost) return;
	const FSlateBrush* WhiteBrush = FAppStyle::GetBrush("WhiteBrush");
	if (!SelectedWorld)
	{
		const bool bAnyWorld = ExistingWorlds.Num() > 0;
		ExistingWorldDetailsHost->SetContent(
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()
			[
				APSMenu::IconSectionHeading(EAPSMenuGlyph::World,
					bAnyWorld ? LOCTEXT("SelectWorldTitle", "SELECT A WORLD") : LOCTEXT("NoWorlds", "NO SAVED WORLDS FOUND"),
					bAnyWorld ? LOCTEXT("SelectWorldHint", "Choose a card to see its system.")
						: LOCTEXT("NoWorldsHint", "Generated worlds appear here once saved."))
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 14.0f, 0.0f, 0.0f)
			[
				SNew(STextBlock).Text(WorldBrowserNotice).AutoWrapText(true)
				.Visibility(WorldBrowserNotice.IsEmpty() ? EVisibility::Collapsed : EVisibility::Visible)
				.Font(APSMenu::Font("Bold", 12)).ColorAndOpacity(APSMenu::Amber)
			]);
		return;
	}

	const FAPSExistingWorldEntry& Entry = *SelectedWorld;
	const bool bRecorded = Entry.bSystemRecorded;
	FText DeleteReason;
	const bool bCanDelete = CanDeleteExistingWorld(Entry, &DeleteReason);

	// Rio 03.10 ("tiny dim grey labels on a teal block"): short sections of tiles, a readable label over a large value,
	// as in the deployment panel. Long values take the whole row.
	struct FWorldTile
	{
		FText Label;
		FText Value;
		int32 Span;
		FLinearColor Color;
	};
	const auto Section = [WhiteBrush](EAPSMenuGlyph Glyph, const FText& Title, const TArray<FWorldTile>& Tiles)
	{
		const TSharedRef<SGridPanel> Tiled = SNew(SGridPanel).FillColumn(0, 1.0f).FillColumn(1, 1.0f);
		int32 Column = 0;
		int32 Row = 0;
		for (const FWorldTile& Tile : Tiles)
		{
			if (Column + Tile.Span > 2)
			{
				Column = 0;
				++Row;
			}
			Tiled->AddSlot(Column, Row).ColumnSpan(Tile.Span).Padding(0.0f, 0.0f, 12.0f, 12.0f)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight()
				[SNew(STextBlock).Text(Tile.Label).Font(APSMenu::Font("Bold", 11)).ColorAndOpacity(APSMenu::Readable)]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 4.0f, 0.0f, 0.0f)
				[SNew(STextBlock).Text(Tile.Value).AutoWrapText(true).Font(APSMenu::Font("Bold", 14)).ColorAndOpacity(Tile.Color)]
			];
			Column += Tile.Span;
		}
		return SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 10.0f)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
				[SNew(SBox).WidthOverride(18.0f).HeightOverride(18.0f)[SNew(SVectorMenuGlyph).Glyph(Glyph).Color(APSMenu::Cyan).StrokeWidth(1.3f)]]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(8.0f, 0.0f)
				[SNew(STextBlock).Text(Title).Font(APSMenu::Font("Bold", 12)).ColorAndOpacity(APSMenu::Cyan)]
				+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center)
				[SNew(SBox).HeightOverride(1.0f)[SNew(SBorder).BorderImage(WhiteBrush).BorderBackgroundColor(APSMenu::CyanDim)]]
			]
			+ SVerticalBox::Slot().AutoHeight()[Tiled];
	};

	TSharedRef<SVerticalBox> Sections = SNew(SVerticalBox);
	if (bRecorded)
	{
		int32 Moons = 0;
		for (const FAPSWorldPlanetRecord& Planet : Entry.System.Planets)
		{
			Moons += Planet.Moons;
		}
		const FLinearColor StarValueColor = FMath::Lerp(
			APSWorldScheme::StarColor(APSMenu::StarClassOf(Entry)), FLinearColor::White, 0.35f);
		TArray<FWorldTile> SystemTiles;
		SystemTiles.Add({LOCTEXT("DetailStar", "STAR"), APSMenu::StarText(Entry), 2, StarValueColor});
		SystemTiles.Add({LOCTEXT("DetailPlanets", "PLANETS"), APSUINumber::Number(Entry.TotalPlanets), 1, APSMenu::White});
		SystemTiles.Add(Entry.System.StarCount > 1
			? FWorldTile{LOCTEXT("DetailStars", "STARS"), APSUINumber::Number(Entry.System.StarCount), 1, APSMenu::White}
			: FWorldTile{LOCTEXT("DetailMoons", "MOONS"), APSUINumber::Number(Moons), 1, APSMenu::White});
		Sections->AddSlot().AutoHeight()[Section(EAPSMenuGlyph::System, LOCTEXT("DetailSystemSection", "SYSTEM"), SystemTiles)];

		const FAPSWorldPlanetRecord* Home = APSMenu::HomeRecord(Entry);
		const FText HomeType = APSMenu::UpperText(Home ? Home->Type : Entry.PlanetType);
		TArray<FWorldTile> ColonyTiles;
		ColonyTiles.Add({LOCTEXT("DetailHomeWorld", "HOME WORLD"), Entry.System.HomePlanetName.IsEmpty() ? HomeType
			: FText::Format(LOCTEXT("DetailHomeNamed", "{0}  •  {1}"), APSMenu::UpperText(Entry.System.HomePlanetName), HomeType),
			2, APSMenu::White});
		ColonyTiles.Add({LOCTEXT("DetailRadius", "RADIUS"), Home && Home->RadiusKm > 0
			? FText::Format(LOCTEXT("DetailRadiusKm", "{0} KM"), APSUINumber::Number(Home->RadiusKm))
			: LOCTEXT("DetailRadiusUnknown", "--"), 1, APSMenu::White});
		ColonyTiles.Add({LOCTEXT("DetailInhabited", "INHABITED"), APSUINumber::Number(Entry.InhabitedPlanets), 1,
			Entry.InhabitedPlanets > 0 ? APSMenu::Amber : APSMenu::White});
		ColonyTiles.Add({LOCTEXT("DetailHabitability", "HABITABILITY"), APSMenu::UpperText(Entry.Habitability), 2, APSMenu::White});
		Sections->AddSlot().AutoHeight().Padding(0.0f, 6.0f, 0.0f, 0.0f)
		[Section(EAPSMenuGlyph::Civilization, LOCTEXT("DetailColonySection", "COLONY"), ColonyTiles)];
	}
	else
	{
		// Rio 03.10 (save audit): an older record only echoes the menu's editor buffer; say so instead of showing it.
		Sections->AddSlot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 16.0f)
		[
			SNew(SBorder).BorderImage(&APSMenu::CivStatusBrush).Padding(FMargin(14.0f, 12.0f))
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(STextBlock).AutoWrapText(true)
					.Text(Entry.bMetadataLoaded ? LOCTEXT("SystemUnrecordedTitle", "SYSTEM NOT RECORDED YET")
						: LOCTEXT("SystemLegacyTitle", "NO WORLD RECORD YET"))
					.Font(APSMenu::Font("Bold", 12)).ColorAndOpacity(APSMenu::Amber)
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 6.0f, 0.0f, 0.0f)
				[
					SNew(STextBlock).AutoWrapText(true)
					.Text(LOCTEXT("SystemUnrecordedBody", "This save predates full system records. Continue the world: its star, planets and home world are recorded the next time it is saved."))
					.Font(APSMenu::Font("Regular", 12)).ColorAndOpacity(APSMenu::Readable)
				]
			]
		];
	}
	TArray<FWorldTile> SaveTiles;
	SaveTiles.Add({LOCTEXT("DetailLastPlayed", "LAST PLAYED"),
		APSMenu::LocalTimeText(Entry.FileTimestamp, TEXT("%d %b %Y  %H:%M")), 2, APSMenu::White});
	SaveTiles.Add({LOCTEXT("DetailSaveSize", "SAVE SIZE"), APSMenu::SaveSizeText(Entry.FileSizeBytes), 1, APSMenu::White});
	if (bShowTechnicalWorldDetails)
	{
		SaveTiles.Add({LOCTEXT("DetailRecord", "RECORD"), bRecorded ? LOCTEXT("DetailRecordSystem", "FULL SYSTEM")
			: Entry.bMetadataLoaded ? LOCTEXT("DetailRecordBasic", "BASIC") : LOCTEXT("DetailRecordNone", "NONE"), 1, APSMenu::White});
		SaveTiles.Add({LOCTEXT("DetailSlot", "SAVE SLOT"), FText::FromString(Entry.SaveFileName), 2, APSMenu::Readable});
	}
	Sections->AddSlot().AutoHeight().Padding(0.0f, 6.0f, 0.0f, 0.0f)
	[Section(EAPSMenuGlyph::Recent, LOCTEXT("DetailSaveSection", "SAVE"), SaveTiles)];

	// DELETE WORLD: secondary at rest, muted red under the pointer (Rio 03.10: "make it possible to delete the world").
	TSharedRef<SButton> DeleteButton = SNew(SButton).ButtonStyle(&DangerButtonStyle)
		.IsEnabled(bCanDelete)
		.ToolTipText(bCanDelete ? LOCTEXT("DeleteWorldTip", "Delete this world's save files.") : DeleteReason)
		.OnClicked(this, &SAPSMainMenuRoot::RequestDeleteExistingWorld)
		.ContentPadding(APSMenu::ButtonPadding("Bold", 11, 14.0f))
		.HAlign(HAlign_Center).VAlign(VAlign_Center);
	const TWeakPtr<SButton> WeakDelete = DeleteButton;
	DeleteButton->SetContent(
		SNew(STextBlock).Text(LOCTEXT("DeleteWorld", "DELETE WORLD")).Justification(ETextJustify::Center)
		.Font(APSMenu::Font("Bold", 11)).RenderTransform(APSMenu::CapsShift("Bold", 11))
		.ColorAndOpacity_Lambda([WeakDelete, bCanDelete]()
		{
			const TSharedPtr<SButton> Button = WeakDelete.Pin();
			return !bCanDelete ? APSMenu::Muted
				: Button.IsValid() && Button->IsHovered() ? APSMenu::DangerBright : APSMenu::Danger;
		}));

	// Only a recorded system has a type to name; the notice below speaks for older records.
	const FText Subtitle = bRecorded ? APSMenu::UpperText(Entry.SystemType) : FText::GetEmpty();
	ExistingWorldDetailsHost->SetContent(
		SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight()
		[
			SNew(SBox).HeightOverride(176.0f)
			[
				SNew(SAPSChamferedOverlay)
				+ SOverlay::Slot()
				[
					// Rio 03.10: the picture alone, no caption or badges over it.
					SNew(SAPSWorldSchemePreview).Input(APSMenu::SchemeInput(Entry)).Centred(true)
				]
				+ SOverlay::Slot()
				[SNew(SChamferedFrame).Color(APSMenu::CyanDim).Thickness(APSMenu::CardPerimeterThickness)]
			]
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 14.0f, 0.0f, 3.0f)
		[SNew(STextBlock).Text(FText::FromString(Entry.DisplayName)).AutoWrapText(true).Font(APSMenu::Font("Bold", 20)).ColorAndOpacity(APSMenu::White)]
		+ SVerticalBox::Slot().AutoHeight()
		[
			SNew(STextBlock).Text(Subtitle).AutoWrapText(true).Font(APSMenu::Font("Bold", 11)).ColorAndOpacity(APSMenu::Cyan)
			.Visibility(Subtitle.IsEmpty() ? EVisibility::Collapsed : EVisibility::Visible)
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 12.0f, 0.0f, 12.0f)
		[SNew(SBox).HeightOverride(1.0f)[SNew(SBorder).BorderImage(WhiteBrush).BorderBackgroundColor(APSMenu::CyanDim)]]
		+ SVerticalBox::Slot().FillHeight(1.0f)
		[
			SNew(SScrollBox).ScrollBarStyle(&ScrollBarStyle)
			+ SScrollBox::Slot()[Sections]
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 10.0f, 0.0f, 0.0f)
		[
			// A failed deletion explains itself here, beside the world that stayed.
			SNew(STextBlock).Text(WorldBrowserNotice).AutoWrapText(true)
			.Visibility(WorldBrowserNotice.IsEmpty() ? EVisibility::Collapsed : EVisibility::Visible)
			.Font(APSMenu::Font("Regular", 12)).ColorAndOpacity(APSMenu::Danger)
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 12.0f, 0.0f, 8.0f)
		[
			SNew(SButton).ButtonStyle(&PrimaryButtonStyle).OnClicked(this, &SAPSMainMenuRoot::ContinueExistingWorld)
			.ContentPadding(APSMenu::ButtonPadding("Bold", 17, 20.0f)).HAlign(HAlign_Center).VAlign(VAlign_Center)
			[SNew(STextBlock).Text(LOCTEXT("ContinueWorld", "CONTINUE  >")).Justification(ETextJustify::Center).Font(APSMenu::Font("Bold", 17)).ColorAndOpacity(APSMenu::White).RenderTransform(APSMenu::CapsShift("Bold", 17))]
		]
		+ SVerticalBox::Slot().AutoHeight()
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(0.0f, 0.0f, 4.0f, 0.0f)
			[
				SNew(SButton).ButtonStyle(&SecondaryButtonStyle).OnClicked(this, &SAPSMainMenuRoot::ToggleWorldDetails)
				.ContentPadding(APSMenu::ButtonPadding("Bold", 11, 14.0f)).HAlign(HAlign_Center).VAlign(VAlign_Center)
				[
					SNew(STextBlock).Text_Lambda([this]()
					{
						return bShowTechnicalWorldDetails ? LOCTEXT("HideWorldDetails", "HIDE DETAILS") : LOCTEXT("ShowWorldDetails", "WORLD DETAILS");
					})
					.Justification(ETextJustify::Center).Font(APSMenu::Font("Bold", 11)).ColorAndOpacity(APSMenu::Cyan)
					.RenderTransform(APSMenu::CapsShift("Bold", 11))
				]
			]
			+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(4.0f, 0.0f, 0.0f, 0.0f)
			[DeleteButton]
		]);
}

bool SAPSMainMenuRoot::IsWorldSlotInUse(const FString& SlotName) const
{
	const AMainMenuController* PC = Controller.Get();
	const UWorld* MenuWorld = PC ? PC->GetWorld() : nullptr;
	const auto IsSlot = [&SlotName](const UMainGameplayInstance* Gameplay)
	{
		return Gameplay && FPaths::GetBaseFilename(Gameplay->SaveSlotName).Equals(SlotName, ESearchCase::IgnoreCase);
	};
	// This game: CONTINUE has handed the slot to a level that is opening now. The replay flag alone could be left over
	// from a replay that failed, which must not lock that broken save out of deletion.
	if (const UGameInstance* GameInstance = PC ? PC->GetGameInstance() : nullptr)
	{
		const UMainGameplayInstance* Gameplay = GameInstance->GetSubsystem<UMainGameplayInstance>();
		const FWorldContext* MenuContext = GEngine && MenuWorld ? GEngine->GetWorldContextFromWorld(MenuWorld) : nullptr;
		if (Gameplay && Gameplay->bPendingSavedWorldReplay && IsSlot(Gameplay)
			&& MenuContext && !MenuContext->TravelURL.IsEmpty())
		{
			return true;
		}
	}
	// Another game world of this process (a second PIE instance) playing the slot: it would save it again on exit.
	if (GEngine)
	{
		for (const FWorldContext& Context : GEngine->GetWorldContexts())
		{
			const UWorld* OtherWorld = Context.World();
			if (!OtherWorld || OtherWorld == MenuWorld || !OtherWorld->IsGameWorld())
			{
				continue;
			}
			const UGameInstance* OtherInstance = OtherWorld->GetGameInstance();
			const APlayerController* Player = OtherWorld->GetFirstPlayerController();
			if (OtherInstance && Player && !Player->IsA<AMainMenuController>()
				&& IsSlot(OtherInstance->GetSubsystem<UMainGameplayInstance>()))
			{
				return true;
			}
		}
	}
	return false;
}

bool SAPSMainMenuRoot::CanDeleteExistingWorld(const FAPSExistingWorldEntry& Entry, FText* OutReason) const
{
	FText Reason;
	if (!APSMenu::IsWorldSaveSlot(Entry.SaveFileName))
	{
		Reason = LOCTEXT("DeleteNotWorld", "This save is not a world.");
	}
	else if (IsWorldSlotInUse(Entry.SaveFileName))
	{
		Reason = LOCTEXT("DeleteWorldInUse", "This world is open in a running game.");
	}
	if (OutReason)
	{
		*OutReason = Reason;
	}
	return Reason.IsEmpty();
}

FReply SAPSMainMenuRoot::RequestDeleteExistingWorld()
{
	if (SelectedWorld.IsValid() && CanDeleteExistingWorld(*SelectedWorld))
	{
		PendingBulkDelete.Reset();
		PendingDeleteWorld = SelectedWorld;
	}
	return FReply::Handled();
}

FReply SAPSMainMenuRoot::CancelDeleteExistingWorld()
{
	PendingDeleteWorld.Reset();
	return FReply::Handled();
}

FReply SAPSMainMenuRoot::ConfirmDeleteExistingWorld()
{
	const TSharedPtr<FAPSExistingWorldEntry> Target = PendingDeleteWorld;
	PendingDeleteWorld.Reset();
	FText Reason;
	if (!Target.IsValid() || !ExistingWorlds.Contains(Target) || !CanDeleteExistingWorld(*Target, &Reason))
	{
		WorldBrowserNotice = Reason;
		RebuildExistingWorldDetails();
		return FReply::Handled();
	}
	// A queued metadata read of this slot must not race its removal.
	if (AMainMenuController* PC = Controller.Get())
	{
		PC->CancelWorldMetadataLoad();
	}
	FString Failure;
	if (!DeleteWorldFiles(*Target, Failure, true))
	{
		WorldBrowserNotice = FText::Format(LOCTEXT("DeleteWorldFailed", "“{0}” could not be deleted. {1}"),
			FText::FromString(Target->DisplayName), FText::FromString(Failure));
		RefreshExistingWorlds();
		return FReply::Handled();
	}
	WorldBrowserNotice = FText::Format(LOCTEXT("DeletedWorldNotice", "“{0}” was deleted."),
		FText::FromString(Target->DisplayName));
	// Rescan so the list, the counts and the directory fingerprint follow the disk; favourites keep only what is left;
	// nothing stays selected.
	LoadExistingWorlds();
	APSMenu::SaveFavoriteSlots(ExistingWorlds);
	SelectedWorld.Reset();
	ApplyExistingWorldView();
	BeginExistingWorldMetadataLoad();
	return FReply::Handled();
}

bool SAPSMainMenuRoot::DeleteWorldFiles(const FAPSExistingWorldEntry& Entry, FString& OutFailure, const bool bLogEach)
{
	FText Reason;
	if (!CanDeleteExistingWorld(Entry, &Reason))
	{
		OutFailure = Reason.ToString();
		return false;
	}
	const FString SlotName = Entry.SaveFileName;
	const bool bHadSave = UGameplayStatics::DoesSaveGameExist(SlotName, 0);
	const bool bSaveDeleted = !bHadSave || UGameplayStatics::DeleteGameInSlot(SlotName, 0);
	const FString SidecarPath = APSWorldBrowserMetadata::SidecarPath(SlotName);
	const bool bSidecarDeleted = !IFileManager::Get().FileExists(*SidecarPath)
		|| IFileManager::Get().Delete(*SidecarPath, false, true, true);
	if (bLogEach || !bSaveDeleted || !bSidecarDeleted)
	{
		UE_LOG(LogTemp, Log, TEXT("[APS.WorldBrowser] Deleted slot=%s save=%s sidecar=%s"), *SlotName,
			bSaveDeleted ? (bHadSave ? TEXT("deleted") : TEXT("missing")) : TEXT("FAILED"),
			bSidecarDeleted ? TEXT("deleted/missing") : TEXT("FAILED"));
	}
	if (!bSaveDeleted)
	{
		OutFailure = TEXT("The save file is locked or read-only.");
		return false;
	}
	// A sidecar left behind is harmless: the browser lists .sav files only.
	return true;
}

FReply SAPSMainMenuRoot::RequestDeleteListedWorlds()
{
	if (!ListedWorlds.IsEmpty())
	{
		PendingDeleteWorld.Reset();
		PendingBulkDelete = ListedWorlds;
		// The confirmation's DELETE ALL arms after a countdown: a stray double click cannot wipe the list.
		BulkDeleteArmTime = FSlateApplication::Get().GetCurrentTime() + 3.0;
	}
	return FReply::Handled();
}

FReply SAPSMainMenuRoot::CancelDeleteListedWorlds()
{
	PendingBulkDelete.Reset();
	return FReply::Handled();
}

FReply SAPSMainMenuRoot::ConfirmDeleteListedWorlds()
{
	if (PendingBulkDelete.IsEmpty() || FSlateApplication::Get().GetCurrentTime() < BulkDeleteArmTime)
	{
		return FReply::Handled();
	}
	const TArray<TSharedPtr<FAPSExistingWorldEntry>> Targets = MoveTemp(PendingBulkDelete);
	PendingBulkDelete.Reset();
	if (AMainMenuController* PC = Controller.Get())
	{
		PC->CancelWorldMetadataLoad();
	}
	int32 Deleted = 0;
	TArray<FString> FailedSlots;
	TArray<FString> FailedReasons;
	for (const TSharedPtr<FAPSExistingWorldEntry>& Target : Targets)
	{
		FString Failure;
		if (Target.IsValid() && DeleteWorldFiles(*Target, Failure, false))
		{
			++Deleted;
		}
		else if (Target.IsValid())
		{
			FailedSlots.Add(Target->SaveFileName);
			FailedReasons.Add(Failure);
		}
	}
	UE_LOG(LogTemp, Log, TEXT("[APS.WorldBrowser] Deleted all: %d slots, %d failed"), Deleted, FailedSlots.Num());
	for (int32 Index = 0; Index < FailedSlots.Num(); ++Index)
	{
		UE_LOG(LogTemp, Warning, TEXT("[APS.WorldBrowser] Not deleted: slot=%s - %s"), *FailedSlots[Index], *FailedReasons[Index]);
	}
	const FText DeletedText = Deleted == 1
		? LOCTEXT("DeletedAllOne", "1 world was deleted.")
		: FText::Format(LOCTEXT("DeletedAllNotice", "{0} worlds were deleted."), APSUINumber::Number(Deleted));
	if (FailedSlots.IsEmpty())
	{
		WorldBrowserNotice = DeletedText;
	}
	else
	{
		// The first few slots in the notice; the log has every one with its reason.
		const TArray<FString> Shown(FailedSlots.GetData(), FMath::Min(FailedSlots.Num(), 4));
		WorldBrowserNotice = FText::Format(LOCTEXT("DeletedAllFailedNotice", "{0} Not deleted ({1}): {2}{3}"),
			DeletedText, APSUINumber::Number(FailedSlots.Num()), FText::FromString(FString::Join(Shown, TEXT(", "))),
			FText::FromString(FailedSlots.Num() > Shown.Num() ? TEXT(", ...") : TEXT("")));
	}
	// One rescan for the whole batch; favourites keep only the worlds that are left.
	LoadExistingWorlds();
	APSMenu::SaveFavoriteSlots(ExistingWorlds);
	SelectedWorld.Reset();
	ExistingWorldPage = 0;
	ApplyExistingWorldView();
	BeginExistingWorldMetadataLoad();
	return FReply::Handled();
}

FReply SAPSMainMenuRoot::ChangeExistingWorldPage(int32 Delta)
{
	ExistingWorldPage = FMath::Max(0, ExistingWorldPage + Delta);
	RebuildExistingWorldGrid();
	return FReply::Handled();
}

bool SAPSMainMenuRoot::PassesWorldCollectionAndSearch(const FAPSExistingWorldEntry& Entry) const
{
	// RECENT keeps the six newest of what passes; the grid cuts the list, so the check here is the same as ALL.
	if (WorldCollection == EAPSWorldCollection::Favorites && !Entry.bFavorite)
	{
		return false;
	}
	return WorldSearch.IsEmpty() || Entry.DisplayName.Contains(WorldSearch, ESearchCase::IgnoreCase)
		|| Entry.SaveFileName.Contains(WorldSearch, ESearchCase::IgnoreCase);
}

bool SAPSMainMenuRoot::MatchesWorldFilter(const FAPSExistingWorldEntry& Entry, const EAPSWorldFilterKind Kind) const
{
	const FString* Choice = WorldFilterChoices.Find(Kind);
	return !Choice || Choice->IsEmpty() || APSMenu::WorldFilterKey(Entry, Kind) == *Choice;
}

bool SAPSMainMenuRoot::PassesExistingWorldFilters(const FAPSExistingWorldEntry& Entry) const
{
	return PassesWorldCollectionAndSearch(Entry)
		&& MatchesWorldFilter(Entry, EAPSWorldFilterKind::StarType)
		&& MatchesWorldFilter(Entry, EAPSWorldFilterKind::WorldType)
		&& MatchesWorldFilter(Entry, EAPSWorldFilterKind::Inhabited)
		&& MatchesWorldFilter(Entry, EAPSWorldFilterKind::Environment);
}

bool SAPSMainMenuRoot::HasWorldNarrowing() const
{
	return WorldCollection != EAPSWorldCollection::All || !WorldSearch.IsEmpty() || WorldFilterChoices.Num() > 0;
}

void SAPSMainMenuRoot::RebuildWorldFilterOptions()
{
	// Each filter's options count the worlds the other filters (and the collection and search) let through, so a
	// number always says how many worlds picking that option shows.
	constexpr EAPSWorldFilterKind Kinds[] = {EAPSWorldFilterKind::StarType, EAPSWorldFilterKind::WorldType,
		EAPSWorldFilterKind::Inhabited, EAPSWorldFilterKind::Environment};
	constexpr int32 KindCount = static_cast<int32>(UE_ARRAY_COUNT(Kinds));
	TMap<FString, int32> Counts[KindCount];
	int32 AnyCounts[KindCount] = {};
	for (const TSharedPtr<FAPSExistingWorldEntry>& Entry : ExistingWorlds)
	{
		if (!Entry.IsValid() || !PassesWorldCollectionAndSearch(*Entry))
		{
			continue;
		}
		FString Keys[KindCount];
		bool Passes[KindCount];
		for (int32 KindIndex = 0; KindIndex < KindCount; ++KindIndex)
		{
			Keys[KindIndex] = APSMenu::WorldFilterKey(*Entry, Kinds[KindIndex]);
			const FString* Choice = WorldFilterChoices.Find(Kinds[KindIndex]);
			Passes[KindIndex] = !Choice || Keys[KindIndex] == *Choice;
		}
		for (int32 KindIndex = 0; KindIndex < KindCount; ++KindIndex)
		{
			bool bOthersPass = true;
			for (int32 Other = 0; Other < KindCount; ++Other)
			{
				bOthersPass &= Other == KindIndex || Passes[Other];
			}
			if (bOthersPass)
			{
				++Counts[KindIndex].FindOrAdd(Keys[KindIndex]);
				++AnyCounts[KindIndex];
			}
		}
	}
	for (int32 KindIndex = 0; KindIndex < KindCount; ++KindIndex)
	{
		const EAPSWorldFilterKind Kind = Kinds[KindIndex];
		// The current choice stays listed even when nothing matches it any more, so it can be read and undone.
		if (const FString* Choice = WorldFilterChoices.Find(Kind))
		{
			Counts[KindIndex].FindOrAdd(*Choice);
		}
		if (WorldCollection == EAPSWorldCollection::Recent)
		{
			// RECENT shows the newest few of whatever passes, so no option shows more than that.
			AnyCounts[KindIndex] = FMath::Min(AnyCounts[KindIndex], APSMenu::RecentWorldCount);
			for (TPair<FString, int32>& Count : Counts[KindIndex])
			{
				Count.Value = FMath::Min(Count.Value, APSMenu::RecentWorldCount);
			}
		}
		TArray<FString> Keys;
		Counts[KindIndex].GetKeys(Keys);
		Keys.Sort([Kind](const FString& A, const FString& B)
		{
			const int32 OrderA = APSMenu::WorldFilterOrder(Kind, A);
			const int32 OrderB = APSMenu::WorldFilterOrder(Kind, B);
			return OrderA != OrderB ? OrderA < OrderB : A < B;
		});
		TArray<FAPSWorldFilterOption>& Options = WorldFilterOptions.FindOrAdd(Kind);
		Options.Reset();
		Options.Add({FString(), APSMenu::WorldFilterOptionLabel(Kind, FString()), AnyCounts[KindIndex]});
		for (const FString& Key : Keys)
		{
			Options.Add({Key, APSMenu::WorldFilterOptionLabel(Kind, Key), Counts[KindIndex].FindRef(Key)});
		}
	}
}

void SAPSMainMenuRoot::ApplyExistingWorldView()
{
	RebuildExistingWorldGrid();
	// Rio 03.10: a world the list no longer shows is no longer selected.
	if (SelectedWorld.IsValid() && !ListedWorlds.Contains(SelectedWorld))
	{
		SelectedWorld.Reset();
	}
	RebuildExistingWorldDetails();
}

TSharedRef<SWidget> SAPSMainMenuRoot::BuildWorldFilterMenu(const EAPSWorldFilterKind Kind)
{
	const FString* Choice = WorldFilterChoices.Find(Kind);
	const FString Current = Choice ? *Choice : FString();
	TSharedRef<SVerticalBox> Rows = SNew(SVerticalBox);
	for (const FAPSWorldFilterOption& Option : WorldFilterOptions.FindRef(Kind))
	{
		const bool bChosen = Option.Key == Current;
		Rows->AddSlot().AutoHeight()
		[
			SNew(SButton).ButtonStyle(&DropdownOptionStyle)
			.ContentPadding(APSMenu::ButtonPadding("Bold", 11, 12.0f))
			.OnClicked(this, &SAPSMainMenuRoot::SelectWorldFilter, Kind, Option.Key)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center)
				[
					SNew(STextBlock).Text(Option.Label).Font(APSMenu::Font("Bold", 11))
					.ColorAndOpacity(bChosen ? APSMenu::Amber : (Option.Count > 0 ? APSMenu::White : APSMenu::Muted))
					.RenderTransform(APSMenu::CapsShift("Bold", 11))
				]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(16.0f, 0.0f, 0.0f, 0.0f)
				[
					SNew(STextBlock).Text(APSUINumber::Number(Option.Count)).Font(APSMenu::Font("Bold", 11))
					.ColorAndOpacity(bChosen ? APSMenu::Amber : APSMenu::Readable)
					.RenderTransform(APSMenu::CapsShift("Bold", 11))
				]
			]
		];
	}
	return SNew(SBorder).BorderImage(&APSMenu::PanelBrush).Padding(6.0f)
	[
		SNew(SBox).MinDesiredWidth(240.0f).MaxDesiredHeight(420.0f)
		[
			SNew(SScrollBox).ScrollBarStyle(&ScrollBarStyle)
			+ SScrollBox::Slot()[Rows]
		]
	];
}

FReply SAPSMainMenuRoot::ToggleWorldFilterMenu(const EAPSWorldFilterKind Kind)
{
	if (const TSharedPtr<SMenuAnchor>* Anchor = WorldFilterAnchors.Find(Kind); Anchor && Anchor->IsValid())
	{
		(*Anchor)->SetIsOpen(!(*Anchor)->IsOpen());
	}
	return FReply::Handled();
}

FReply SAPSMainMenuRoot::SelectWorldFilter(const EAPSWorldFilterKind Kind, FString Key)
{
	if (const TSharedPtr<SMenuAnchor>* Anchor = WorldFilterAnchors.Find(Kind); Anchor && Anchor->IsValid())
	{
		(*Anchor)->SetIsOpen(false);
	}
	if (Key.IsEmpty())
	{
		WorldFilterChoices.Remove(Kind);
	}
	else
	{
		WorldFilterChoices.Add(Kind, MoveTemp(Key));
	}
	ExistingWorldPage = 0;
	ApplyExistingWorldView();
	return FReply::Handled();
}

FReply SAPSMainMenuRoot::ClearWorldFilters()
{
	WorldFilterChoices.Reset();
	ExistingWorldPage = 0;
	ApplyExistingWorldView();
	return FReply::Handled();
}

FText SAPSMainMenuRoot::GetWorldCollectionLabel(EAPSWorldCollection Collection) const
{
	switch (Collection)
	{
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
	// "ANY", or the choice and how many worlds it shows: "G YELLOW (12)".
	const FString* Choice = WorldFilterChoices.Find(Kind);
	if (!Choice || Choice->IsEmpty())
	{
		return APSMenu::WorldFilterOptionLabel(Kind, FString());
	}
	int32 Count = 0;
	for (const FAPSWorldFilterOption& Option : WorldFilterOptions.FindRef(Kind))
	{
		if (Option.Key == *Choice)
		{
			Count = Option.Count;
			break;
		}
	}
	return FText::Format(LOCTEXT("FilterChoiceCount", "{0} ({1})"), APSMenu::WorldFilterOptionLabel(Kind, *Choice),
		APSUINumber::Number(Count));
}

FReply SAPSMainMenuRoot::SetWorldCollection(EAPSWorldCollection Collection)
{
	WorldCollection = Collection;
	ExistingWorldPage = 0;
	ApplyExistingWorldView();
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

FReply SAPSMainMenuRoot::ToggleWorldFavorite(TSharedPtr<FAPSExistingWorldEntry> Entry)
{
	if (Entry.IsValid())
	{
		// The star on a card: saved at once (Saved/Config/APSWorldBrowser.ini); FAVORITES counts it live.
		Entry->bFavorite = !Entry->bFavorite;
		APSMenu::SaveFavoriteSlots(ExistingWorlds);
		if (WorldCollection == EAPSWorldCollection::Favorites)
		{
			ApplyExistingWorldView();
		}
		else
		{
			RebuildExistingWorldGrid();
		}
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
	// Pilots built on the production pilot's native class inherit its generated-world movement/camera contract.
	const TSet<FTopLevelAssetPath> PilotClassPaths = FindDerivedClasses(ACustomGravityCharacter::StaticClass());
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

	// Basic rule: a start asset must reference a mesh, directly or through a parent Blueprint, so
	// empty and placeholder Blueprints never reach the pickers. The filter asset adds hand-picked
	// exclusions and the Blueprints the thumbnail bake found to render nothing.
	const UAPSStartAssetFilter* StartAssetFilter = LoadObject<UAPSStartAssetFilter>(
		nullptr, APSUIThumbnails::StartAssetFilterPath, nullptr, LOAD_NoWarn | LOAD_Quiet);
	const TSet<FTopLevelAssetPath> MeshClassPaths = {
		UStaticMesh::StaticClass()->GetClassPathName(), USkeletalMesh::StaticClass()->GetClassPathName()};
	const FTopLevelAssetPath BlueprintClassPath = UBlueprint::StaticClass()->GetClassPathName();
	TMap<FName, bool> MeshReferenceCache;
	TFunction<bool(FName)> ReferencesMesh = [&](FName PackageName) -> bool
	{
		if (const bool* Cached = MeshReferenceCache.Find(PackageName))
		{
			return *Cached;
		}
		MeshReferenceCache.Add(PackageName, false);
		TArray<FName> Dependencies;
		AssetRegistry.GetDependencies(PackageName, Dependencies, UE::AssetRegistry::EDependencyCategory::Package,
			UE::AssetRegistry::EDependencyQuery::Hard);
		for (const FName Dependency : Dependencies)
		{
			TArray<FAssetData> DependencyAssets;
			AssetRegistry.GetAssetsByPackageName(Dependency, DependencyAssets, true);
			for (const FAssetData& DependencyAsset : DependencyAssets)
			{
				if (MeshClassPaths.Contains(DependencyAsset.AssetClassPath)
					|| (DependencyAsset.AssetClassPath == BlueprintClassPath && ReferencesMesh(Dependency)))
				{
					MeshReferenceCache.Add(PackageName, true);
					return true;
				}
			}
		}
		return false;
	};
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
			if (FSoftObjectPath(GeneratedClassObjectPath) != ProductionPilotClassPath
				&& !(PilotClassPaths.Contains(GeneratedClassPath)
					&& GeneratedClassObjectPath.StartsWith(TEXT("/Game/APS/APS_ALPHA/"))))
			{
				continue;
			}
			Slot = EAPSStartAssetSlot::Character;
		}
		if (Slot != EAPSStartAssetSlot::Character
			&& ((StartAssetFilter && StartAssetFilter->Hides(FSoftObjectPath(GeneratedClassObjectPath)))
				|| !ReferencesMesh(Asset.PackageName)))
		{
			HiddenStartClasses.Add(FSoftObjectPath(GeneratedClassObjectPath));
			continue;
		}
		SpawnClassOptions.FindOrAdd(Slot).AddUnique(TSoftClassPtr<AActor>(FSoftObjectPath(GeneratedClassObjectPath)));
	}
	// Legacy copies (/Game/APS/Core, APS_PREA) share Blueprint names with the current APS_ALPHA ones;
	// list each name once and prefer the APS_ALPHA Blueprint.
	const auto IsPreferredStartAsset = [](const FSoftObjectPath& Path)
	{
		return Path.GetLongPackageName().StartsWith(TEXT("/Game/APS/APS_ALPHA/"));
	};
	for (auto& Pair : SpawnClassOptions)
	{
		TMap<FString, int32> IndexByName;
		TArray<TSoftClassPtr<AActor>> Unique;
		for (const TSoftClassPtr<AActor>& Option : Pair.Value)
		{
			const FSoftObjectPath Path = Option.ToSoftObjectPath();
			if (const int32* Existing = IndexByName.Find(Path.GetAssetName()))
			{
				const FSoftObjectPath ExistingPath = Unique[*Existing].ToSoftObjectPath();
				const bool bReplace = IsPreferredStartAsset(Path) && !IsPreferredStartAsset(ExistingPath);
				const FSoftObjectPath& Dropped = bReplace ? ExistingPath : Path;
				HiddenStartClasses.Add(Dropped);
				DuplicateStartClasses.Add(Dropped, bReplace ? Path : ExistingPath);
				if (bReplace)
				{
					Unique[*Existing] = Option;
				}
				continue;
			}
			IndexByName.Add(Path.GetAssetName(), Unique.Num());
			Unique.Add(Option);
		}
		Pair.Value = MoveTemp(Unique);
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
		TEXT("[APS.Menu] Blueprint spawn catalogue indexed without loading assets: characters=%d ships=%d stations=%d headquarters=%d shipyards=%d hidden=%d"),
		SpawnClassOptions.FindRef(EAPSStartAssetSlot::Character).Num(),
		SpawnClassOptions.FindRef(EAPSStartAssetSlot::Spaceship).Num(),
		SpawnClassOptions.FindRef(EAPSStartAssetSlot::SpaceStation).Num(),
		SpawnClassOptions.FindRef(EAPSStartAssetSlot::Headquarters).Num(),
		SpawnClassOptions.FindRef(EAPSStartAssetSlot::Shipyard).Num(),
		HiddenStartClasses.Num());
}

namespace APSMenuStartDefaults
{
	/** Rio 02.10: until the player picks, the menu starts with the newest pieces: the ranger pilot, the S P3 cargo ship,
	 * station R2 and headquarters Alpha. The shipyard stays the generator's. Loaded like any pick (asynchronously). */
	FSoftObjectPath For(const EAPSStartAssetSlot Slot)
	{
		switch (Slot)
		{
		case EAPSStartAssetSlot::Character:
			return FSoftObjectPath(TEXT("/Game/APS/APS_ALPHA/Blueprints/BP_CustomGravityCharacter_SpeedModes.BP_CustomGravityCharacter_SpeedModes_C"));
		case EAPSStartAssetSlot::Spaceship:
			return FSoftObjectPath(TEXT("/Game/APS/APS_ALPHA/Core/Spaceships/S/BP_Spaceship_S_P3_01.BP_Spaceship_S_P3_01_C"));
		case EAPSStartAssetSlot::SpaceStation:
			return FSoftObjectPath(TEXT("/Game/APS/APS_ALPHA/Core/Stations/BP_SpaceStation_R2.BP_SpaceStation_R2_C"));
		case EAPSStartAssetSlot::Headquarters:
			return FSoftObjectPath(TEXT("/Game/APS/APS_ALPHA/Core/SpaceInfrastructure/BP_SpaceHeadquarters_Alpha.BP_SpaceHeadquarters_Alpha_C"));
		default:
			return FSoftObjectPath();
		}
	}
}

namespace APSMenuClassNames
{
	FText BaseName(const TSoftClassPtr<AActor>& Option);
}

void SAPSMainMenuRoot::SynchronizeSpawnClassOptions()
{
	SpawnClassCaptions.Reset();
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
		// A hidden default (for example a mesh-less generator shipyard) gives way to the first visible
		// option below instead of coming back as an empty, pre-selected entry.
		if (CurrentClass && CurrentClass->IsChildOf(AActor::StaticClass())
			&& !HiddenStartClasses.Contains(FSoftObjectPath(CurrentClass)))
		{
			Pair.Value.AddUnique(TSoftClassPtr<AActor>(CurrentClass));
		}
		FSoftObjectPath CurrentPath = CurrentClass ? FSoftObjectPath(CurrentClass) : FSoftObjectPath();
		if (const FSoftObjectPath* Twin = DuplicateStartClasses.Find(CurrentPath))
		{
			CurrentPath = *Twin;
		}
		int32 InitialIndex = Pair.Value.IndexOfByPredicate([CurrentClass, &CurrentPath](const TSoftClassPtr<AActor>& Candidate)
		{
			return Candidate.Get() == CurrentClass
				|| (CurrentPath.IsValid() && Candidate.ToSoftObjectPath() == CurrentPath);
		});
		if (InitialIndex == INDEX_NONE) InitialIndex = 0;
		if (ViewModel.IsValid() && ViewModel->IsSpawnSlotDefaulted(Pair.Key))
		{
			const FSoftObjectPath Preferred = APSMenuStartDefaults::For(Pair.Key);
			const int32 PreferredIndex = Preferred.IsValid() ? Pair.Value.IndexOfByPredicate(
				[&Preferred](const TSoftClassPtr<AActor>& Candidate) { return Candidate.ToSoftObjectPath() == Preferred; })
				: INDEX_NONE;
			if (PreferredIndex != INDEX_NONE)
			{
				InitialIndex = PreferredIndex;
			}
		}
		SpawnClassIndices.Add(Pair.Key, InitialIndex);
		ApplySpawnClassSelection(Pair.Key);
	}
	RefreshSpawnThumbnails();
}

void SAPSMainMenuRoot::RefreshSpawnThumbnails()
{
	SpawnClassThumbnails.Reset();
	for (const auto& Pair : SpawnClassOptions)
	{
		TArray<TSharedPtr<FSlateBrush>>& Brushes = SpawnClassThumbnails.Add(Pair.Key);
		for (const TSoftClassPtr<AActor>& Option : Pair.Value)
		{
			const FString PackageName = Option.ToSoftObjectPath().GetLongPackageName();
			const FName Key(*PackageName);
			if (const TSharedPtr<FSlateBrush>* Cached = SpawnThumbnailBrushCache.Find(Key))
			{
				Brushes.Add(*Cached);
				continue;
			}
			TSharedPtr<FSlateBrush> Brush;
			const FString TexturePath = APSUIThumbnails::TexturePathForBlueprintPackage(PackageName);
			if (UTexture2D* Texture = LoadObject<UTexture2D>(nullptr, *TexturePath, nullptr,
				LOAD_NoWarn | LOAD_Quiet))
			{
				if (AMainMenuController* PC = Controller.Get())
				{
					PC->HoldSlateResource(Texture);
				}
				Brush = MakeShared<FSlateBrush>();
				// Cropped to the object, so the card's picture fills with it (Rio 05.10).
				APSUIThumbnails::InitBrush(*Brush, Texture);
			}
			SpawnThumbnailBrushCache.Add(Key, Brush);
			Brushes.Add(Brush);
		}
	}
}

const FSlateBrush* SAPSMainMenuRoot::GetSpawnClassThumbnail(EAPSStartAssetSlot Slot, int32 OptionIndex) const
{
	const TArray<TSharedPtr<FSlateBrush>>* Brushes = SpawnClassThumbnails.Find(Slot);
	return Brushes && Brushes->IsValidIndex(OptionIndex) ? (*Brushes)[OptionIndex].Get() : nullptr;
}

const FSlateBrush* SAPSMainMenuRoot::GetSelectedSpawnThumbnail(EAPSStartAssetSlot Slot) const
{
	return GetSpawnClassThumbnail(Slot, SpawnClassIndices.FindRef(Slot));
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
	// Remembers its in-game name for the next session; this session's captions stay as they are.
	APSMenuClassNames::BaseName((*Options)[Index]);
	Invalidate(EInvalidateWidgetReason::Paint);
}

FText SAPSMainMenuRoot::GetSpawnClassName(EAPSStartAssetSlot Slot) const
{
	const TArray<TSoftClassPtr<AActor>>* Options = SpawnClassOptions.Find(Slot);
	const int32 Index = SpawnClassIndices.FindRef(Slot);
	return GetSpawnClassOptionName(Slot, Index);
}

namespace APSMenuClassNames
{
	/** In-game names of start classes seen loaded, kept across sessions (GameUserSettings). */
	const TCHAR* const Section = TEXT("APS.Menu.ClassNames");

	TMap<FString, FString>& Remembered()
	{
		static TMap<FString, FString> Names;
		static bool bRead = false;
		if (!bRead && GConfig)
		{
			bRead = true;
			if (const FConfigSection* Stored = GConfig->GetSection(Section, false, GGameUserSettingsIni))
			{
				for (const auto& Pair : *Stored) Names.Add(Pair.Key.ToString(), Pair.Value.GetValue());
			}
		}
		return Names;
	}

	/**
	 * Rio 02.10 ("the names keep changing, at least the headquarters"): an option read its in-game name only while
	 * its class happened to be loaded and its asset name otherwise, so the list changed with what was loaded. The
	 * in-game name, once seen, is remembered; until then the asset name stands in.
	 */
	/** Rio 02.10: names given in the menu where the in-game ones say nothing (both pilots read the same). */
	const FString* Curated(const FString& Path)
	{
		static const TMap<FString, FString> Names = {
			{TEXT("/Game/APS/APS_ALPHA/Blueprints/BP_CustomGravityCharacter.BP_CustomGravityCharacter_C"),
				TEXT("PILOT  /  STANDARD")},
			{TEXT("/Game/APS/APS_ALPHA/Blueprints/BP_CustomGravityCharacter_SpeedModes.BP_CustomGravityCharacter_SpeedModes_C"),
				TEXT("PILOT  /  RANGER")}};
		return Names.Find(Path);
	}

	FText BaseName(const TSoftClassPtr<AActor>& Option)
	{
		const FString Path = Option.ToSoftObjectPath().ToString();
		if (const FString* Name = Curated(Path))
		{
			return FText::FromString(*Name);
		}
		if (const UClass* Loaded = Option.Get())
		{
			const AActor* Default = Loaded->GetDefaultObject<AActor>();
			const FText InGameName = Default && Default->Implements<UItemInfoInterface>()
				? IItemInfoInterface::Execute_GetInGameName(Default) : FText::GetEmpty();
			if (!InGameName.IsEmptyOrWhitespace())
			{
				const FString Name = InGameName.ToString();
				if (Remembered().FindRef(Path) != Name && GConfig)
				{
					Remembered().Add(Path, Name);
					GConfig->SetString(Section, *Path, *Name, GGameUserSettingsIni);
					GConfig->Flush(false, GGameUserSettingsIni);
				}
				return InGameName;
			}
		}
		if (const FString* Name = Remembered().Find(Path))
		{
			return FText::FromString(*Name);
		}
		FString AssetName = Option.ToSoftObjectPath().GetAssetName();
		AssetName.RemoveFromEnd(TEXT("_C"));
		return FText::FromString(AssetName.Replace(TEXT("BP_"), TEXT("")));
	}
}

FText SAPSMainMenuRoot::GetSpawnClassOptionName(EAPSStartAssetSlot Slot, int32 OptionIndex) const
{
	const TArray<TSoftClassPtr<AActor>>* Options = SpawnClassOptions.Find(Slot);
	if (!Options || !Options->IsValidIndex(OptionIndex)) return LOCTEXT("Unavailable", "NOT CONFIGURED");
	TArray<FText>& Captions = SpawnClassCaptions.FindOrAdd(Slot);
	if (Captions.Num() != Options->Num())
	{
		// Rio 02.10 ("the names under the list change"): a name read while its class happened to be loaded renamed the
		// option, and its twins with it, as picks loaded classes. A slot's captions are now taken at once and kept.
		TArray<FText> Names;
		for (const TSoftClassPtr<AActor>& Option : *Options)
		{
			Names.Add(APSMenuClassNames::BaseName(Option));
		}
		Captions.Reset(Options->Num());
		for (int32 Index = 0; Index < Options->Num(); ++Index)
		{
			// Rio 02.10: three headquarters all read "HQ X-1". Options sharing a name show their variant (the asset's suffix).
			bool bShared = false;
			for (int32 Other = 0; Other < Names.Num() && !bShared; ++Other)
			{
				bShared = Other != Index && Names[Other].EqualTo(Names[Index]);
			}
			if (!bShared)
			{
				Captions.Add(Names[Index]);
				continue;
			}
			FString Variant = (*Options)[Index].ToSoftObjectPath().GetAssetName();
			Variant.RemoveFromEnd(TEXT("_C"));
			Variant.RemoveFromStart(TEXT("BP_"));
			FString Head, Tail;
			const FString Suffix = Variant.Split(TEXT("_"), &Head, &Tail, ESearchCase::IgnoreCase, ESearchDir::FromEnd)
				? Tail.ToUpper() : FString(TEXT("ORIGINAL"));
			Captions.Add(FText::Format(LOCTEXT("SpawnClassVariant", "{0}  /  {1}"), Names[Index], FText::FromString(Suffix)));
		}
	}
	return Captions[OptionIndex];
}

namespace
{
	bool IsStationCard(EAPSStartAssetSlot Slot)
	{
		return Slot == EAPSStartAssetSlot::SpaceStation || Slot == EAPSStartAssetSlot::Headquarters
			|| Slot == EAPSStartAssetSlot::Shipyard;
	}

	EAPSStartStation StartStationForCard(EAPSStartAssetSlot Slot)
	{
		return Slot == EAPSStartAssetSlot::Headquarters ? EAPSStartStation::Headquarters
			: Slot == EAPSStartAssetSlot::Shipyard ? EAPSStartStation::Shipyard : EAPSStartStation::HomeStation;
	}
}

bool SAPSMainMenuRoot::IsStartHere(EAPSStartAssetSlot Slot) const
{
	const USpawnParameters* Parameters = ViewModel.IsValid() ? ViewModel->SpawnParameters.Get() : nullptr;
	return Parameters && IsStationCard(Slot) && Parameters->CharacterSpawnPlace == ECharSpawnPlace::PlanetOrbit
		&& Parameters->StartStation == StartStationForCard(Slot);
}

FReply SAPSMainMenuRoot::SetStartHere(EAPSStartAssetSlot Slot)
{
	if (ViewModel.IsValid() && IsStationCard(Slot))
	{
		ViewModel->SetCharacterSpawnPlace(static_cast<int32>(ECharSpawnPlace::PlanetOrbit));
		ViewModel->SetStartStation(static_cast<int32>(StartStationForCard(Slot)));
	}
	return FReply::Handled();
}

TSharedRef<SWidget> SAPSMainMenuRoot::BuildSpawnCard(EAPSStartAssetSlot Slot, const FText& Label)
{
	const EAPSMenuGlyph SlotGlyph =
		Slot == EAPSStartAssetSlot::Character ? EAPSMenuGlyph::Pilot :
		Slot == EAPSStartAssetSlot::Spaceship ? EAPSMenuGlyph::Ship :
		Slot == EAPSStartAssetSlot::SpaceStation ? EAPSMenuGlyph::Station :
		Slot == EAPSStartAssetSlot::Headquarters ? EAPSMenuGlyph::Headquarters : EAPSMenuGlyph::Shipyard;
	// With a single certified pilot the card stays a locked badge; derived pilots make it a normal picker.
	const bool bLockedProductionPilot = Slot == EAPSStartAssetSlot::Character
		&& SpawnClassOptions.FindRef(Slot).Num() <= 1;
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
			.ContentPadding(FMargin(12.0f, 7.0f))
			.ButtonColorAndOpacity_Lambda([this, Slot, CapturedIndex]()
			{
				return SpawnClassIndices.FindRef(Slot) == CapturedIndex
					? APSMenu::RetintAction(FLinearColor(0.42f, 0.19f, 0.01f, 1.0f))
					: APSMenu::Retint(FLinearColor(0.02f, 0.13f, 0.18f, 0.92f));
			})
			.ToolTipText(FText::FromString(Options[OptionIndex].ToSoftObjectPath().ToString()))
			.OnClicked(this, &SAPSMainMenuRoot::SelectSpawnClass, Slot, OptionIndex)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
				[
					SNew(STextBlock)
					.Text(FText::FromString(FString::Printf(TEXT("%02d"), OptionIndex + 1)))
					.Font(APSMenu::Font("Bold", 9)).RenderTransform(APSMenu::CapsShift("Bold", 9))
					.ColorAndOpacity_Lambda([this, Slot, CapturedIndex]()
					{
						return SpawnClassIndices.FindRef(Slot) == CapturedIndex ? APSMenu::Amber : APSMenu::Cyan;
					})
				]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(6.0f, 0.0f, 0.0f, 0.0f)
				[
					SNew(SBox).WidthOverride(24.0f).HeightOverride(24.0f)
					.Visibility_Lambda([this, Slot, CapturedIndex]()
					{
						return GetSpawnClassThumbnail(Slot, CapturedIndex)
							? EVisibility::HitTestInvisible : EVisibility::Collapsed;
					})
					// Rio 05.10: icons may be wide (a long hull baked with -Wide); fit, never stretch.
					[SNew(SScaleBox).Stretch(EStretch::ScaleToFit)
						[SNew(SImage).Image_Lambda([this, Slot, CapturedIndex]() { return GetSpawnClassThumbnail(Slot, CapturedIndex); })]]
				]
				+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center).Padding(7.0f, 0.0f)
				[
					SNew(STextBlock)
					.Text_Lambda([this, Slot, CapturedIndex]() { return GetSpawnClassOptionName(Slot, CapturedIndex); })
					.Font(APSMenu::Font("Bold", 11)).ColorAndOpacity(APSMenu::White)
					.OverflowPolicy(ETextOverflowPolicy::Ellipsis).RenderTransform(APSMenu::CapsShift("Bold", 11))
				]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
				[
					SNew(STextBlock).Text(LOCTEXT("SpawnOptionActive", "ACTIVE"))
					.Font(APSMenu::Font("Bold", 8)).ColorAndOpacity(APSMenu::Success).RenderTransform(APSMenu::CapsShift("Bold", 8))
					.Visibility_Lambda([this, Slot, CapturedIndex]()
					{
						return SpawnClassIndices.FindRef(Slot) == CapturedIndex
							? EVisibility::Visible : EVisibility::Collapsed;
					})
				]
			]
		];
	}

	return SNew(SAPSChamferedOverlay)
		+ SOverlay::Slot()
		[
			SNew(SOverlay)
			+ SOverlay::Slot()
			[
				SNew(SChamferedSurface)
				.Brush(FAppStyle::GetBrush("WhiteBrush"))
				.Tint(APSMenu::CivRaised)
				.ChamferTop(true)
				.ChamferBottom(true)
			]
			+ SOverlay::Slot()
			[
				SNew(SOverlay)
				+ SOverlay::Slot().Padding(14.0f)
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
							[SNew(STextBlock).Text(Label).Font(APSMenu::Font("Bold", 15)).ColorAndOpacity(APSMenu::White)]
							+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 1.0f, 0.0f, 0.0f)
							[SNew(STextBlock).Text(SlotDescription).Font(APSMenu::Font("Regular", 9)).ColorAndOpacity(APSMenu::Muted).OverflowPolicy(ETextOverflowPolicy::Ellipsis)]
						]
						+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
						[
							SNew(SBorder).BorderImage(&APSMenu::CivStatusBrush).Padding(FMargin(8.0f, 4.0f))
							.HAlign(HAlign_Center).VAlign(VAlign_Center)
							[
								SNew(STextBlock)
								.Text(bLockedProductionPilot ? LOCTEXT("PilotCardLocked", "LOCKED")
									: FText::FromString(FString::Printf(TEXT("%02d"), SlotNumber)))
								.Font(APSMenu::Font("Bold", 8)).RenderTransform(APSMenu::CapsShift("Bold", 8))
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
								SNew(SBox).WidthOverride(118.0f).HeightOverride(118.0f)
								.Visibility_Lambda([this, Slot]() { return GetSelectedSpawnThumbnail(Slot) ? EVisibility::Collapsed : EVisibility::Visible; })
								[SNew(SVectorMenuGlyph).Glyph(SlotGlyph).Color(APSMenu::Retint(FLinearColor(0.03f, 0.28f, 0.38f, 0.32f))).StrokeWidth(4.8f)]
							]
							+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center)
							[
								SNew(SBox).WidthOverride(76.0f).HeightOverride(76.0f)
								.Visibility_Lambda([this, Slot]() { return GetSelectedSpawnThumbnail(Slot) ? EVisibility::Collapsed : EVisibility::Visible; })
								[SNew(SVectorMenuGlyph).Glyph(SlotGlyph).Color(bLockedProductionPilot ? APSMenu::Amber : APSMenu::Cyan).StrokeWidth(1.9f)]
							]
							// Baked thumbnail of the selected Blueprint, as in the editor's content browser.
							+ SOverlay::Slot().Padding(6.0f)
							[
								SNew(SScaleBox).Stretch(EStretch::ScaleToFit)
								.Visibility_Lambda([this, Slot]() { return GetSelectedSpawnThumbnail(Slot) ? EVisibility::HitTestInvisible : EVisibility::Collapsed; })
								[SNew(SImage).Image_Lambda([this, Slot]() { return GetSelectedSpawnThumbnail(Slot); })]
							]
							+ SOverlay::Slot().HAlign(HAlign_Left).VAlign(VAlign_Bottom).Padding(8.0f)
							[
								SNew(STextBlock).Text(LOCTEXT("SpawnCardLoadout", "LOADOUT CLASS"))
								.Font(APSMenu::Font("Bold", 8)).ColorAndOpacity(APSMenu::Muted)
							]
							// Rio 06.10: the start is chosen in the console (START); the card the pilot starts on shows it here
							// instead of a START HERE button: the image framed in the action colour and a label in its corner.
							+ SOverlay::Slot().HAlign(HAlign_Right).VAlign(VAlign_Bottom).Padding(8.0f)
							[
								SNew(STextBlock).Text(LOCTEXT("SpawnCardStartsHere", "START HERE"))
								.Font(APSMenu::Font("Bold", 8)).ColorAndOpacity_Lambda([]() { return FSlateColor(APSMenu::ActionBright); })
								.Visibility_Lambda([this, Slot]() { return IsStartHere(Slot) ? EVisibility::HitTestInvisible : EVisibility::Collapsed; })
							]
							+ SOverlay::Slot().Padding(-2.0f)
							[
								// A rounded box's outline ignores the border's tint: ApplyTheme draws it in the action colour.
								SNew(SBorder).BorderImage(&APSMenu::OutlineBrush).Padding(0.0f)
								.Visibility_Lambda([this, Slot]() { return IsStartHere(Slot) ? EVisibility::HitTestInvisible : EVisibility::Collapsed; })
							]
						]
					]
					+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
					[
						SNew(STextBlock).Text_Lambda([this, Slot]() { return GetSpawnClassName(Slot); })
						.Font(APSMenu::Font("Bold", 16)).ColorAndOpacity(APSMenu::White)
						.Justification(ETextJustify::Center).OverflowPolicy(ETextOverflowPolicy::Ellipsis)
					]
					// Every card keeps the same rows (hidden where unused) so the images line up at one height.
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 7.0f, 0.0f, 5.0f)
					[
						SNew(SOverlay)
						+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center)
						[
							SNew(SBorder).BorderImage(&APSMenu::CivStatusBrush).Padding(FMargin(12.0f, 5.0f))
							.HAlign(HAlign_Center).VAlign(VAlign_Center)
							.Visibility(bLockedProductionPilot ? EVisibility::Visible : EVisibility::Collapsed)
							[
								SNew(STextBlock).Text(LOCTEXT("ProductionPilotLocked", "VERIFIED GRAVITY PILOT"))
								.Font(APSMenu::Font("Bold", 8)).ColorAndOpacity(APSMenu::Amber).RenderTransform(APSMenu::CapsShift("Bold", 8))
							]
						]
						+ SOverlay::Slot()
						[
						SNew(SHorizontalBox)
						.Visibility(bLockedProductionPilot ? EVisibility::Hidden : EVisibility::Visible)
						+ SHorizontalBox::Slot().FillWidth(0.28f)
						[SNew(SButton).ButtonStyle(&SecondaryButtonStyle).ContentPadding(APSMenu::ButtonPadding("Bold", 11, 14.0f))
						.HAlign(HAlign_Center).VAlign(VAlign_Center).OnClicked(this, &SAPSMainMenuRoot::CycleSpawnClass, Slot, -1)
						[SNew(STextBlock).Text(FText::FromString(TEXT("<"))).Justification(ETextJustify::Center).Font(APSMenu::Font("Bold", 11)).ColorAndOpacity(APSMenu::Cyan).RenderTransform(APSMenu::SymbolShift("Bold", 11))]]
						// Rio 06.10: the class counter opens every class of this card as a grid of large cards.
						+ SHorizontalBox::Slot().FillWidth(0.44f).Padding(4.0f, 0.0f)
						[
							SNew(SButton).ButtonStyle(&SecondaryButtonStyle).ContentPadding(APSMenu::ButtonPadding("Bold", 9, 6.0f))
							.HAlign(HAlign_Center).VAlign(VAlign_Center)
							.ToolTipText(LOCTEXT("SpawnPickerTip", "Show every class as cards"))
							.OnClicked_Lambda([this, Slot]() { OpenSpawnPicker(Slot); return FReply::Handled(); })
							[
								SNew(STextBlock).Text_Lambda([this, Slot]()
								{
									const int32 Count = SpawnClassOptions.FindRef(Slot).Num();
									return FText::FromString(FString::Printf(TEXT("CLASS %02d / %02d"), Count > 0 ? SpawnClassIndices.FindRef(Slot) + 1 : 0, Count));
								}).Justification(ETextJustify::Center).Font(APSMenu::Font("Bold", 9)).ColorAndOpacity(APSMenu::Readable)
								.RenderTransform(APSMenu::CapsShift("Bold", 9))
							]
						]
						+ SHorizontalBox::Slot().FillWidth(0.28f)
						[SNew(SButton).ButtonStyle(&SecondaryButtonStyle).ContentPadding(APSMenu::ButtonPadding("Bold", 11, 14.0f))
						.HAlign(HAlign_Center).VAlign(VAlign_Center).OnClicked(this, &SAPSMainMenuRoot::CycleSpawnClass, Slot, 1)
						[SNew(STextBlock).Text(FText::FromString(TEXT(">"))).Justification(ETextJustify::Center).Font(APSMenu::Font("Bold", 11)).ColorAndOpacity(APSMenu::Cyan).RenderTransform(APSMenu::SymbolShift("Bold", 11))]]
						]
					]
					+ SVerticalBox::Slot().FillHeight(0.44f).Padding(0.0f, 4.0f, 0.0f, 0.0f)
					[
						SNew(SBox).MaxDesiredHeight(200.0f)
						.Visibility(bLockedProductionPilot ? EVisibility::Hidden : EVisibility::Visible)
						[
							SNew(SBorder).BorderImage(&APSMenu::InsetBrush).Padding(4.0f)
							[SNew(SScrollBox) + SScrollBox::Slot()[OptionGrid]]
						]
					]
				]
			]
		]
		+ SOverlay::Slot()
		[
			SNew(SChamferedFrame)
			.Color(bLockedProductionPilot ? APSMenu::Amber : APSMenu::Cyan)
			.Thickness(APSMenu::CardPerimeterThickness)
		];
}

TSharedRef<SWidget> SAPSMainMenuRoot::BuildCivilizationPage()
{
	DiscoverSpawnClassOptions();
	bSpawnPickerOpen = false;
	const TWeakObjectPtr<UWorldGenerationViewModel> VM = ViewModel;
	CivilizationEditorSection = FMath::Clamp(CivilizationEditorSection, 0, 3);

	const auto EnumControl = [this](const FText& Label, const UEnum* Enum,
		TFunction<int32()> Getter, TFunction<void(int32)> Setter, TFunction<bool(int32)> IsAvailable = nullptr)
	{
		const auto Step = [Enum, Getter, Setter, IsAvailable](int32 Direction)
		{
			if (!Enum) return FReply::Handled();
			const int32 Count = FMath::Max(1, Enum->NumEnums() - 1);
			// Unavailable values (a lunar start without moons) are stepped over, not selected.
			int32 Value = Getter();
			for (int32 Tries = 0; Tries < Count; ++Tries)
			{
				Value = (Value + Direction + Count) % Count;
				if (!IsAvailable || IsAvailable(Value))
				{
					Setter(Value);
					break;
				}
			}
			return FReply::Handled();
		};
		return SNew(SBorder).BorderImage(&APSMenu::CivControlBrush).Padding(FMargin(10.0f, 7.0f))
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(0.40f).VAlign(VAlign_Center)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight()
				[SNew(STextBlock).Text(Label).Font(APSMenu::Font("Bold", 11)).ColorAndOpacity(APSMenu::White)]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 2.0f, 0.0f, 0.0f)
				[SNew(STextBlock).Text(LOCTEXT("EnumControlType", "SELECTED PROFILE")).Font(APSMenu::Font("Regular", 9)).ColorAndOpacity(APSMenu::Muted)]
			]
			+ SHorizontalBox::Slot().FillWidth(0.60f).VAlign(VAlign_Center).Padding(10.0f, 0.0f, 0.0f, 0.0f)
			[
				SNew(SBorder).BorderImage(&APSMenu::InsetBrush).Padding(FMargin(2.0f))
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().AutoWidth()[SNew(SBox).WidthOverride(40.0f).HeightOverride(40.0f)[SNew(SButton).ButtonStyle(&SecondaryButtonStyle).ContentPadding(0.0f).HAlign(HAlign_Center).VAlign(VAlign_Center).OnClicked_Lambda([Step](){ return Step(-1); })[SNew(STextBlock).Text(FText::FromString(TEXT("<"))).Justification(ETextJustify::Center).Font(APSMenu::Font("Bold", 10)).ColorAndOpacity(APSMenu::Cyan).RenderTransform(APSMenu::SymbolShift("Bold", 10))]]]
					+ SHorizontalBox::Slot().FillWidth(1.0f).HAlign(HAlign_Center).VAlign(VAlign_Center).Padding(8.0f, 0.0f)
					[SNew(STextBlock).Text_Lambda([Enum, Getter](){ return Enum ? Enum->GetDisplayNameTextByValue(Getter()) : FText::FromString(TEXT("--")); }).Justification(ETextJustify::Center).Font(APSMenu::Font("Bold", 11)).ColorAndOpacity(APSMenu::White).OverflowPolicy(ETextOverflowPolicy::Ellipsis).RenderTransform(APSMenu::CapsShift("Bold", 11))]
					+ SHorizontalBox::Slot().AutoWidth()[SNew(SBox).WidthOverride(40.0f).HeightOverride(40.0f)[SNew(SButton).ButtonStyle(&SecondaryButtonStyle).ContentPadding(0.0f).HAlign(HAlign_Center).VAlign(VAlign_Center).OnClicked_Lambda([Step](){ return Step(1); })[SNew(STextBlock).Text(FText::FromString(TEXT(">"))).Justification(ETextJustify::Center).Font(APSMenu::Font("Bold", 10)).ColorAndOpacity(APSMenu::Cyan).RenderTransform(APSMenu::SymbolShift("Bold", 10))]]]
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
				[SNew(STextBlock).Text(Label).Font(APSMenu::Font("Bold", 11)).ColorAndOpacity(APSMenu::White)]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 2.0f, 0.0f, 0.0f)
				[SNew(STextBlock).Text(FText::FromString(FString::Printf(TEXT("RANGE  %s - %s"), *APSUINumber::Number(MinValue).ToString(), *APSUINumber::Number(MaxValue).ToString()))).Font(APSMenu::Font("Regular", 9)).ColorAndOpacity(APSMenu::Muted)]
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
				[SNew(STextBlock).Text(Label).Font(APSMenu::Font("Bold", 11)).ColorAndOpacity(APSMenu::White)]
				// Rio 03.10: square steppers, the sign centred in each.
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
				[SNew(SBox).WidthOverride(30.0f).HeightOverride(30.0f)[SNew(SButton).ButtonStyle(&SecondaryButtonStyle).ContentPadding(0.0f).HAlign(HAlign_Center).VAlign(VAlign_Center).OnClicked_Lambda([Step](){ return Step(-1); })[SNew(STextBlock).Text(FText::FromString(TEXT("-"))).Justification(ETextJustify::Center).Font(APSMenu::Font("Bold", 11)).ColorAndOpacity(APSMenu::Cyan).RenderTransform(APSMenu::SymbolShift("Bold", 11))]]]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(10.0f, 0.0f)
				[SNew(STextBlock).Text_Lambda([Getter](){ return FText::FromString(FString::Printf(TEXT("%02d / 20"), Getter())); }).Font(APSMenu::Font("Bold", 10)).ColorAndOpacity(APSMenu::Cyan).RenderTransform(APSMenu::CapsShift("Bold", 10))]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
				[SNew(SBox).WidthOverride(30.0f).HeightOverride(30.0f)[SNew(SButton).ButtonStyle(&SecondaryButtonStyle).ContentPadding(0.0f).HAlign(HAlign_Center).VAlign(VAlign_Center).OnClicked_Lambda([Step](){ return Step(1); })[SNew(STextBlock).Text(FText::FromString(TEXT("+"))).Justification(ETextJustify::Center).Font(APSMenu::Font("Bold", 11)).ColorAndOpacity(APSMenu::Cyan).RenderTransform(APSMenu::SymbolShift("Bold", 11))]]]
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 7.0f, 0.0f, 0.0f)
			[
				SNew(SBox).HeightOverride(4.0f)
				[
					SNew(SProgressBar)
					.BackgroundImage(&APSMenu::ProgressTrackBrush)
					.FillImage(FAppStyle::GetBrush("WhiteBrush"))
					.Percent_Lambda([Getter](){ return TOptional<float>(FMath::Clamp(Getter() / 20.0f, 0.0f, 1.0f)); })
					.FillColorAndOpacity_Lambda([]() { return FSlateColor(APSMenu::Cyan); })
				]
			]
		];
	};

	// Rio 02.10 ("unreadable UX"): a headline value, then tiles with a small label over a large value, like LIVE MODEL.
	struct FInfoTile
	{
		FText Label;
		int32 Span{1};
		TFunction<FText()> Value;
	};
	const auto InfoPanel = [](EAPSMenuGlyph Glyph, const FText& Title,
		const FText& Subtitle, TAttribute<FText> Headline, TArray<FInfoTile> Tiles)
	{
		const TSharedRef<SGridPanel> Grid = SNew(SGridPanel).FillColumn(0, 1.0f).FillColumn(1, 1.0f).FillColumn(2, 1.0f);
		int32 Column = 0;
		int32 Row = 0;
		for (FInfoTile& Tile : Tiles)
		{
			if (Column + Tile.Span > 3)
			{
				Column = 0;
				++Row;
			}
			Grid->AddSlot(Column, Row).ColumnSpan(Tile.Span).Padding(0.0f, 0.0f, 14.0f, 12.0f)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight()
				[SNew(STextBlock).Text(Tile.Label).Font(APSMenu::Font("Bold", 9)).ColorAndOpacity(APSMenu::Muted)]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 3.0f, 0.0f, 0.0f)
				[SNew(STextBlock).Text_Lambda(MoveTemp(Tile.Value)).AutoWrapText(true).Font(APSMenu::Font("Bold", 13)).ColorAndOpacity(APSMenu::White)]
			];
			Column += Tile.Span;
		}
		return APSMenu::ChamferPanel(
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)[APSMenu::IconBadge(Glyph, APSMenu::Cyan, 30.0f)]
				+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center).Padding(9.0f, 0.0f)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight()[SNew(STextBlock).Text(Title).Font(APSMenu::Font("Bold", 14)).ColorAndOpacity(APSMenu::White)]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 1.0f, 0.0f, 0.0f)[SNew(STextBlock).Text(Subtitle).Font(APSMenu::Font("Regular", 9)).ColorAndOpacity(APSMenu::Muted)]
				]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
				[SNew(STextBlock).Text(LOCTEXT("CivInfoLive", "LIVE")).Font(APSMenu::Font("Bold", 8)).ColorAndOpacity(APSMenu::Success)]
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 9.0f, 0.0f, 0.0f)
			[SNew(SBox).HeightOverride(1.0f)[SNew(SBorder).BorderImage(FAppStyle::GetBrush("WhiteBrush")).BorderBackgroundColor(APSMenu::CyanDim)]]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 10.0f, 0.0f, 0.0f)
			[SNew(STextBlock).Text(Headline).AutoWrapText(true).Font(APSMenu::Font("Bold", 16)).ColorAndOpacity(APSMenu::Cyan)]
			+ SVerticalBox::Slot().FillHeight(1.0f).Padding(0.0f, 12.0f, 0.0f, 0.0f)
			[Grid]
		, FMargin(15.0f), APSMenu::Retint(FLinearColor(0.035f, 0.28f, 0.37f, 0.95f)), 1.0f);
	};
	// The tiles read the live spawn parameters and generated world.
	const auto Spawn = [VM]() -> const USpawnParameters* { return VM.IsValid() ? VM->SpawnParameters.Get() : nullptr; };
	const auto Generated = [VM]() -> const UGeneratedWorld* { return VM.IsValid() ? VM->GeneratedWorld.Get() : nullptr; };
	const auto Upper = [](const FString& Value) { return FText::FromString(Value.ToUpper()); };

	const auto PresetButton = [this](const FText& Label, const FText& ToolTip,
		TFunction<bool()> IsActive, TFunction<void()> Apply)
	{
		return SNew(SButton).ButtonStyle(&SecondaryButtonStyle).ToolTipText(ToolTip)
			.ContentPadding(FMargin(14.0f, 8.0f))
			.HAlign(HAlign_Center).VAlign(VAlign_Center)
			.ButtonColorAndOpacity_Lambda([IsActive]()
			{
				return IsActive() ? APSMenu::RetintAction(FLinearColor(0.45f, 0.18f, 0.01f, 1.0f))
					: APSMenu::Retint(FLinearColor(0.015f, 0.09f, 0.12f, 0.96f));
			})
			.OnClicked_Lambda([Apply](){ Apply(); return FReply::Handled(); })
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
				[SNew(STextBlock).Text(Label).Justification(ETextJustify::Center).Font(APSMenu::Font("Bold", 9)).ColorAndOpacity(APSMenu::White).RenderTransform(APSMenu::CapsShift("Bold", 9))]
				+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0.0f, 2.0f, 0.0f, 0.0f)
				[
					SNew(STextBlock).Text_Lambda([IsActive](){ return IsActive() ? LOCTEXT("PresetActive", "ACTIVE") : LOCTEXT("PresetProfile", "PROFILE"); })
					.Justification(ETextJustify::Center).RenderTransform(APSMenu::CapsShift("Bold", 8))
					.Font(APSMenu::Font("Bold", 8)).ColorAndOpacity_Lambda([IsActive](){ return IsActive() ? APSMenu::Amber : APSMenu::Muted; })
				]
			];
	};

	const auto MetricTile = [](const FText& Label, TAttribute<FText> Value,
		const FLinearColor& Accent = APSMenu::Cyan)
	{
		return SNew(SBorder).BorderImage(&APSMenu::CivMetricBrush).Padding(FMargin(14.0f, 8.0f))
		.HAlign(HAlign_Center).VAlign(VAlign_Center)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
			[SNew(STextBlock).Text(Value).Font(APSMenu::Font("Bold", 15)).ColorAndOpacity(Accent).Justification(ETextJustify::Center).RenderTransform(APSMenu::CapsShift("Bold", 15))]
			+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0.0f, 2.0f, 0.0f, 0.0f)
			[SNew(STextBlock).Text(Label).Font(APSMenu::Font("Bold", 8)).ColorAndOpacity(APSMenu::Muted).Justification(ETextJustify::Center).RenderTransform(APSMenu::CapsShift("Bold", 8))]
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
		return SNew(SButton).ButtonStyle(&SecondaryButtonStyle).ContentPadding(FMargin(12.0f, 8.0f))
		.HAlign(HAlign_Fill).VAlign(VAlign_Center)
		.ButtonColorAndOpacity_Lambda([this, Index]()
		{
			return CivilizationEditorSection == Index
				? APSMenu::RetintAction(FLinearColor(0.38f, 0.16f, 0.01f, 1.0f))
				: APSMenu::Retint(FLinearColor(0.015f, 0.08f, 0.11f, 0.95f));
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
				[SNew(STextBlock).Text(FText::FromString(FString::Printf(TEXT("STEP %02d"), Index + 1))).Font(APSMenu::Font("Bold", 8)).RenderTransform(APSMenu::CapsShift("Bold", 8)).ColorAndOpacity_Lambda([this, Index](){ return CivilizationEditorSection == Index ? APSMenu::Amber : APSMenu::Muted; })]
				+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0.0f, 5.0f, 0.0f, 0.0f)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
					[SNew(SBox).WidthOverride(18.0f).HeightOverride(18.0f)[SNew(SVectorMenuGlyph).Glyph(Glyph).Color(APSMenu::Cyan).StrokeWidth(1.3f)]]
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(5.0f, 0.0f)
					[SNew(STextBlock).Text(Label).Font(APSMenu::Font("Bold", 8)).ColorAndOpacity(APSMenu::White).RenderTransform(APSMenu::CapsShift("Bold", 8))]
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
		];

	// STEP 04: where the pilot starts, on which station for orbital starts, and the complex orbit.
	TSharedRef<SWidget> StartEditor = SNew(SScrollBox)
		+ SScrollBox::Slot()
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()[APSMenu::IconSectionHeading(EAPSMenuGlyph::Station,
				LOCTEXT("StartSetup", "ARRIVAL POINT"), LOCTEXT("StartSetupHint", "Where the pilot wakes up in the home system."))]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 14.0f, 0.0f, 6.0f)[EnumControl(LOCTEXT("SpawnPlace", "PILOT START LOCATION"), StaticEnum<ECharSpawnPlace>(), [VM](){return VM.IsValid()&&VM->SpawnParameters?static_cast<int32>(VM->SpawnParameters->CharacterSpawnPlace):0;}, [VM](int32 V){if(VM.IsValid())VM->SetCharacterSpawnPlace(V);}, [VM](int32 V){return !VM.IsValid()||VM->IsCharacterSpawnPlaceAvailable(V);})]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 6.0f)
			[
				SNew(STextBlock)
				.Visibility_Lambda([VM](){return VM.IsValid()&&VM->GetHomePlanetMoonCount()==0?EVisibility::Visible:EVisibility::Collapsed;})
				.Text_Lambda([VM]()
				{
					const bool bLunarChosen = VM.IsValid() && VM->SpawnParameters
						&& !VM->IsCharacterSpawnPlaceAvailable(static_cast<int32>(VM->SpawnParameters->CharacterSpawnPlace));
					return bLunarChosen
						? LOCTEXT("NoMoonsChosen", "THE HOME PLANET HAS NO MOONS: THE PILOT WILL START AT THE HEADQUARTERS")
						: LOCTEXT("NoMoons", "LUNAR STARTS NEED A MOON AROUND THE HOME PLANET");
				})
				.Font(APSMenu::Font("Regular", 9)).ColorAndOpacity(APSMenu::Muted).AutoWrapText(true)
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 6.0f)
			[
				SNew(SBox)
				.IsEnabled_Lambda([VM](){return VM.IsValid()&&VM->SpawnParameters&&VM->SpawnParameters->CharacterSpawnPlace==ECharSpawnPlace::PlanetOrbit;})
				.ToolTipText(LOCTEXT("StartStationTip", "Used when the pilot starts in planet orbit."))
				[EnumControl(LOCTEXT("StartStation", "START STATION"), StaticEnum<EAPSStartStation>(), [VM](){return VM.IsValid()&&VM->SpawnParameters?static_cast<int32>(VM->SpawnParameters->StartStation):0;}, [VM](int32 V){if(VM.IsValid())VM->SetStartStation(V);})]
			]
			+ SVerticalBox::Slot().AutoHeight()[EnumControl(LOCTEXT("OrbitHeight", "HOME COMPLEX ORBIT"), StaticEnum<EOrbitHeight>(), [VM](){return VM.IsValid()&&VM->SpawnParameters?static_cast<int32>(VM->SpawnParameters->HomeStationOrbitHeight):0;}, [VM](int32 V){if(VM.IsValid())VM->SetStationOrbitHeight(V);})]
		];

	// STEP 05 (Rio 02.10, C19: "for the ground start, a screen of its own to choose the base colony, the ground vehicles,
	// the launch pad"): the colony on the home world as it is founded. It stands in every start; on the ground the pilot
	// wakes up in it.
	const auto VehicleToggle = [this, VM](const FText& Label, const FText& ToolTip, const int32 Bit)
	{
		const auto IsOn = [VM, Bit]()
		{
			const USpawnParameters* P = VM.IsValid() ? VM->SpawnParameters.Get() : nullptr;
			return P && (P->GroundVehicleMask & Bit) != 0;
		};
		return SNew(SButton).ButtonStyle(&SecondaryButtonStyle).ToolTipText(ToolTip).ContentPadding(FMargin(14.0f, 8.0f))
			.HAlign(HAlign_Center).VAlign(VAlign_Center)
			.ButtonColorAndOpacity_Lambda([IsOn]()
			{
				return IsOn() ? APSMenu::RetintAction(FLinearColor(0.45f, 0.18f, 0.01f, 1.0f)) : APSMenu::Retint(FLinearColor(0.015f, 0.09f, 0.12f, 0.96f));
			})
			.OnClicked_Lambda([VM, Bit]()
			{
				if (VM.IsValid() && VM->SpawnParameters)
				{
					VM->SpawnParameters->GroundVehicleMask ^= Bit;
				}
				return FReply::Handled();
			})
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
				[SNew(STextBlock).Text(Label).Justification(ETextJustify::Center).Font(APSMenu::Font("Bold", 9)).ColorAndOpacity(APSMenu::White).RenderTransform(APSMenu::CapsShift("Bold", 9))]
				+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0.0f, 2.0f, 0.0f, 0.0f)
				[
					SNew(STextBlock).Text_Lambda([IsOn](){ return IsOn() ? LOCTEXT("VehicleOn", "AT THE COLONY") : LOCTEXT("VehicleOff", "NONE"); })
					.Justification(ETextJustify::Center).RenderTransform(APSMenu::CapsShift("Bold", 8))
					.Font(APSMenu::Font("Bold", 8)).ColorAndOpacity_Lambda([IsOn](){ return IsOn() ? APSMenu::Amber : APSMenu::Muted; })
				]
			];
	};
	const auto PackageText = [VM]()
	{
		const USpawnParameters* P = VM.IsValid() ? VM->SpawnParameters.Get() : nullptr;
		if (!P) return FText::GetEmpty();
		switch (P->ColonyStartPackage)
		{
		case EAPSColonyStartPackage::Standard:
			return LOCTEXT("PackageStandard", "The base, and the colony orders a habitat, a solar array and storage on founding.");
		case EAPSColonyStartPackage::Settlement:
			return LOCTEXT("PackageSettlement", "A working colony: habitat, solar array, storage, greenhouse, comms mast and floodlight, built over the first minutes.");
		case EAPSColonyStartPackage::Outpost:
		default:
			return LOCTEXT("PackageOutpost", "The base alone: the first structures are yours to build.");
		}
	};
	TSharedRef<SWidget> GroundEditor = SNew(SScrollBox)
		+ SScrollBox::Slot()
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()[APSMenu::IconSectionHeading(EAPSMenuGlyph::Planet,
				LOCTEXT("GroundSetup", "GROUND START"), LOCTEXT("GroundSetupHint", "The colony on the home world as it is founded."))]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 14.0f, 0.0f, 4.0f)[EnumControl(LOCTEXT("ColonyPackage", "BASE COLONY"), StaticEnum<EAPSColonyStartPackage>(), [VM](){return VM.IsValid()&&VM->SpawnParameters?static_cast<int32>(VM->SpawnParameters->ColonyStartPackage):0;}, [VM](int32 V){if(VM.IsValid()&&VM->SpawnParameters)VM->SpawnParameters->ColonyStartPackage=static_cast<EAPSColonyStartPackage>(V);})]
			+ SVerticalBox::Slot().AutoHeight().Padding(2.0f, 0.0f, 2.0f, 8.0f)
			[SNew(STextBlock).Text_Lambda(PackageText).Font(APSMenu::Font("Regular", 9)).ColorAndOpacity(APSMenu::Muted).AutoWrapText(true)]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 8.0f)[EnumControl(LOCTEXT("LaunchPad", "LAUNCH PAD"), StaticEnum<EAPSLaunchPadStart>(), [VM](){return VM.IsValid()&&VM->SpawnParameters?static_cast<int32>(VM->SpawnParameters->LaunchPadStart):0;}, [VM](int32 V){if(VM.IsValid()&&VM->SpawnParameters)VM->SpawnParameters->LaunchPadStart=static_cast<EAPSLaunchPadStart>(V);})]
			+ SVerticalBox::Slot().AutoHeight().Padding(2.0f, 4.0f, 0.0f, 6.0f)
			[SNew(STextBlock).Text(LOCTEXT("GroundVehicles", "GROUND VEHICLES")).Font(APSMenu::Font("Bold", 11)).ColorAndOpacity(APSMenu::White)]
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(SUniformGridPanel).SlotPadding(FMargin(3.0f))
				+ SUniformGridPanel::Slot(0, 0)[VehicleToggle(LOCTEXT("VehicleRover", "ROVER"), LOCTEXT("VehicleRoverTip", "Wheels, 25-35 m/s over the ground."), 1)]
				+ SUniformGridPanel::Slot(1, 0)[VehicleToggle(LOCTEXT("VehicleHover", "HOVER"), LOCTEXT("VehicleHoverTip", "Floats over the ground and water, twice as fast."), 2)]
				+ SUniformGridPanel::Slot(2, 0)[VehicleToggle(LOCTEXT("VehicleDrone", "DRONE"), LOCTEXT("VehicleDroneTip", "Flies up to the edge of the atmosphere."), 4)]
			]
		];

	TSharedRef<SWidget> InfrastructureEditor = SNew(SScrollBox)
		+ SScrollBox::Slot()
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()[APSMenu::IconSectionHeading(EAPSMenuGlyph::Infrastructure,
				LOCTEXT("InfrastructureSetup", "PHYSICAL MANIFEST"), LOCTEXT("InfrastructureSetupHint", "Everything counted here arrives in the generated system."))]
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
						+ SUniformGridPanel::Slot(0,0)[MetricTile(LOCTEXT("MetricUnits", "UNITS"),TAttribute<FText>::CreateLambda([VM](){const USpawnParameters* P=VM.IsValid()?VM->SpawnParameters.Get():nullptr;return APSUINumber::Number(P?P->GetPlannedUnitCount():0);}),APSMenu::Amber)]
						+ SUniformGridPanel::Slot(1,0)[MetricTile(LOCTEXT("MetricShips", "FLEET"),TAttribute<FText>::CreateLambda([VM](){const USpawnParameters* P=VM.IsValid()?VM->SpawnParameters.Get():nullptr;return APSUINumber::Number(P?P->StartingFleetSize:0);}))]
						+ SUniformGridPanel::Slot(2,0)[MetricTile(LOCTEXT("MetricStructures", "STRUCTURES"),TAttribute<FText>::CreateLambda([VM](){const USpawnParameters* P=VM.IsValid()?VM->SpawnParameters.Get():nullptr;return APSUINumber::Number(P?P->GetPlannedStructureCount():0);}))]
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
		+ SWidgetSwitcher::Slot()[DivisionsEditor]
		+ SWidgetSwitcher::Slot()[StartEditor]
		+ SWidgetSwitcher::Slot()[GroundEditor];

	TSharedRef<SWidget> Page = SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().Padding(28.0f, 18.0f, 28.0f, 3.0f)[BuildHeader(LOCTEXT("CivParameters", "CIVILIZATION GENERATION"))]
		+ SVerticalBox::Slot().FillHeight(1.0f).Padding(26.0f, 4.0f, 26.0f, 6.0f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(0.66f).Padding(5.0f)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight().Padding(4.0f, 0.0f, 4.0f, 6.0f)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center)
					[SNew(STextBlock).Text(LOCTEXT("FoundingRegistryTitle", "FOUNDING ASSET REGISTRY")).Font(APSMenu::Font("Bold", 12)).ColorAndOpacity(APSMenu::White)]
					// Rio 06.10: the manifest's state and count, moved here from the strip above the page.
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
					[
						SNew(STextBlock).Font(APSMenu::Font("Bold", 10))
						.Text_Lambda([this, VM]()
						{
							const USpawnParameters* P = VM.IsValid() ? VM->SpawnParameters.Get() : nullptr;
							return FText::Format(LOCTEXT("CivManifestLine", "{0}   ·   {1} UNITS  /  {2} STRUCTURES"),
								SpawnSelectionLoadHandles.IsEmpty() ? LOCTEXT("CivManifestReady", "MANIFEST READY") : LOCTEXT("CivManifestResolving", "RESOLVING ASSETS"),
								P ? P->GetPlannedUnitCount() : 0, P ? P->GetPlannedStructureCount() : 0);
						})
						.ColorAndOpacity_Lambda([this]() { return FSlateColor(SpawnSelectionLoadHandles.IsEmpty() ? APSMenu::Readable : APSMenu::Amber); })
					]
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
					[InfoPanel(EAPSMenuGlyph::Civilization, LOCTEXT("CivilizationStatus", "CIVILIZATION"), LOCTEXT("CivilizationStatusHint", "Identity snapshot"),
						TAttribute<FText>::CreateLambda([Spawn, Upper]() { const USpawnParameters* P = Spawn(); return P ? Upper(P->CivilizationName) : FText::GetEmpty(); }),
						{
							{LOCTEXT("CivStance", "STANCE"), 1, [Spawn, Upper]() { const USpawnParameters* P = Spawn(); return P ? Upper(APSMenu::EnumLabel(P->CivilizationArchetype)) : FText::GetEmpty(); }},
							{LOCTEXT("CivGovernment", "GOVERNMENT"), 1, [Spawn, Upper]() { const USpawnParameters* P = Spawn(); return P ? Upper(APSMenu::EnumLabel(P->GovernmentType)) : FText::GetEmpty(); }},
							{LOCTEXT("CivEconomy", "ECONOMY"), 1, [Spawn, Upper]() { const USpawnParameters* P = Spawn(); return P ? Upper(APSMenu::EnumLabel(P->EconomicSystem)) : FText::GetEmpty(); }},
							{LOCTEXT("CivSociety", "SOCIETY"), 1, [Spawn, Upper]() { const USpawnParameters* P = Spawn(); return P ? Upper(APSMenu::EnumLabel(P->SocietyType)) : FText::GetEmpty(); }},
							{LOCTEXT("CivPopulation", "POPULATION"), 1, [Spawn]() { const USpawnParameters* P = Spawn(); return P ? APSUINumber::Number(P->FoundingPopulation) : FText::GetEmpty(); }},
							{LOCTEXT("CivTech", "TECH LEVEL"), 1, [Spawn]() { const USpawnParameters* P = Spawn(); return P ? FText::FromString(FString::Printf(TEXT("T%d"), P->TechnologyLevel)) : FText::GetEmpty(); }}
						})]
					+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(4.0f)
					[InfoPanel(EAPSMenuGlyph::System, LOCTEXT("StarSystemInfo", "HOME SYSTEM"), LOCTEXT("StarSystemInfoHint", "Astronomical handoff"),
						TAttribute<FText>::CreateLambda([Generated, Upper]() { const UGeneratedWorld* W = Generated(); return W ? Upper(APSMenu::EnumLabel(W->PlanetarySystemType)) : FText::GetEmpty(); }),
						{
							{LOCTEXT("SysStar", "STAR"), 1, [Generated, Upper]() { const UGeneratedWorld* W = Generated(); return W ? Upper(APSMenu::EnumLabel(W->StellarType)) : FText::GetEmpty(); }},
							{LOCTEXT("SysSpectrum", "SPECTRUM"), 1, [Generated, Upper]() { const UGeneratedWorld* W = Generated(); return W ? Upper(APSMenu::EnumLabel(W->SpectralClass)) : FText::GetEmpty(); }},
							{LOCTEXT("SysPlanets", "PLANETS"), 1, [Generated]() { const UGeneratedWorld* W = Generated(); return W ? APSUINumber::Number(W->PlanetsAmount) : FText::GetEmpty(); }},
							{LOCTEXT("SysHome", "HOME WORLD"), 1, [Generated, Upper]() { const UGeneratedWorld* W = Generated(); return W ? Upper(APSMenu::EnumLabel(W->PlanetType)) : FText::GetEmpty(); }},
							{LOCTEXT("SysRadius", "RADIUS"), 1, [Generated]() { const UGeneratedWorld* W = Generated(); return W ? FText::Format(LOCTEXT("SysRadiusKm", "{0} KM"), APSUINumber::Number(FMath::RoundToInt(W->PlanetRadius))) : FText::GetEmpty(); }},
							{LOCTEXT("SysMoons", "MOONS"), 1, [Generated]() { const UGeneratedWorld* W = Generated(); return W ? APSUINumber::Number(W->MoonsAmount) : FText::GetEmpty(); }}
						})]
					+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(4.0f)
					[InfoPanel(EAPSMenuGlyph::Infrastructure, LOCTEXT("Infrastructure", "DEPLOYMENT"), LOCTEXT("InfrastructureHint", "Arrival manifest"),
						TAttribute<FText>::CreateLambda([Spawn]() { const USpawnParameters* P = Spawn(); return P ? FText::Format(LOCTEXT("DeployUnits", "{0} UNITS  /  {1} STRUCTURES"), APSUINumber::Number(P->GetPlannedUnitCount()), APSUINumber::Number(P->GetPlannedStructureCount())) : FText::GetEmpty(); }),
						{
							{LOCTEXT("DeployFleet", "FLEET"), 1, [Spawn]() { const USpawnParameters* P = Spawn(); return P ? FText::Format(LOCTEXT("DeployShips", "{0} SHIPS"), APSUINumber::Number(P->StartingFleetSize)) : FText::GetEmpty(); }},
							{LOCTEXT("DeployInfrastructure", "INFRASTRUCTURE"), 1, [Spawn]() { const USpawnParameters* P = Spawn(); return P ? FText::Format(LOCTEXT("DeployNodes", "{0} NODES"), APSUINumber::Number(P->GetPlannedInfrastructureActorCount())) : FText::GetEmpty(); }},
							{LOCTEXT("DeployPilot", "PILOT"), 1, [this]()
							{
								// The chosen pilot's name ("PILOT  /  RANGER" reads RANGER here).
								FString Name = GetSpawnClassName(EAPSStartAssetSlot::Character).ToString();
								Name.RemoveFromStart(TEXT("PILOT  /  "));
								return FText::FromString(Name);
							}},
							{LOCTEXT("DeployStart", "START"), 3, [Spawn, Upper]()
							{
								const USpawnParameters* P = Spawn();
								if (!P) return FText::GetEmpty();
								FString Start = StaticEnum<ECharSpawnPlace>()->GetDisplayNameTextByValue(static_cast<int64>(P->CharacterSpawnPlace)).ToString();
								if (P->CharacterSpawnPlace == ECharSpawnPlace::PlanetOrbit)
								{
									Start += TEXT("  /  ") + StaticEnum<EAPSStartStation>()->GetDisplayNameTextByValue(static_cast<int64>(P->StartStation)).ToString();
								}
								else if (P->CharacterSpawnPlace == ECharSpawnPlace::PlanetSurface)
								{
									// C19: the ground start names its colony package.
									Start += TEXT("  /  ") + StaticEnum<EAPSColonyStartPackage>()->GetDisplayNameTextByValue(static_cast<int64>(P->ColonyStartPackage)).ToString()
										+ TEXT(" COLONY");
								}
								return Upper(Start);
							}}
						})]
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
							+ SVerticalBox::Slot().AutoHeight()[SNew(STextBlock).Text(LOCTEXT("CivilizationConsole", "CONFIGURATION CONSOLE")).Font(APSMenu::Font("Bold", 13)).ColorAndOpacity(APSMenu::White)]
							+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 2.0f, 0.0f, 0.0f)[SNew(STextBlock).Text(LOCTEXT("CivilizationConsoleHint", "Tune the colony package before system handoff.")).Font(APSMenu::Font("Regular", 10)).ColorAndOpacity(APSMenu::Muted)]
						]
						+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
						[
							SNew(SBorder).BorderImage(&APSMenu::CivStatusBrush).Padding(FMargin(10.0f, 5.0f))
							.HAlign(HAlign_Center).VAlign(VAlign_Center)
							[SNew(STextBlock).Text(LOCTEXT("CivilizationConsoleOnline", "ONLINE")).Font(APSMenu::Font("Bold", 8)).ColorAndOpacity(APSMenu::Success).RenderTransform(APSMenu::CapsShift("Bold", 8))]
						]
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 12.0f)
					[
						SNew(SUniformGridPanel).SlotPadding(FMargin(3.0f))
						+ SUniformGridPanel::Slot(0, 0)[EditorTab(0, EAPSMenuGlyph::Civilization, LOCTEXT("IdentityTab", "IDENTITY"))]
						+ SUniformGridPanel::Slot(1, 0)[EditorTab(1, EAPSMenuGlyph::Infrastructure, LOCTEXT("ManifestTab", "MANIFEST"))]
						+ SUniformGridPanel::Slot(2, 0)[EditorTab(2, EAPSMenuGlyph::Divisions, LOCTEXT("DivisionsTab", "DIVISIONS"))]
						+ SUniformGridPanel::Slot(3, 0)[EditorTab(3, EAPSMenuGlyph::Station, LOCTEXT("StartTab", "START"))]
						+ SUniformGridPanel::Slot(4, 0)[EditorTab(4, EAPSMenuGlyph::Planet, LOCTEXT("GroundTab", "GROUND"))]
					]
					+ SVerticalBox::Slot().FillHeight(1.0f)[EditorSwitcher]
				, FMargin(16.0f), APSMenu::Retint(FLinearColor(0.05f, 0.34f, 0.43f, 0.95f)), 1.2f)
			]
		]
		+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0.0f, 8.0f, 0.0f, 22.0f)
		[
			SNew(SBox).WidthOverride(500.0f)
			[
				SNew(SButton).ButtonStyle(&PrimaryButtonStyle).OnClicked(this, &SAPSMainMenuRoot::CommitCivilization)
				.IsEnabled_Lambda([this]() { return SpawnSelectionLoadHandles.IsEmpty(); })
				.ContentPadding(APSMenu::ButtonPadding("Bold", 18, 24.0f))
				.HAlign(HAlign_Center).VAlign(VAlign_Center)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
					[
						SNew(STextBlock)
						.Text_Lambda([this]()
						{
							if (!SpawnSelectionLoadHandles.IsEmpty()) return LOCTEXT("LoadingSpawnSelections", "LOADING SELECTED ASSETS...");
							// Rio 02.10: the start button says what it does, no counts of actors and the like.
							return LOCTEXT("GenerateWorld", "GENERATE CIVILIZATION  >");
						})
						.Justification(ETextJustify::Center).Font(APSMenu::Font("Bold", 18)).ColorAndOpacity(APSMenu::White)
						.RenderTransform(APSMenu::CapsShift("Bold", 18))
					]
				]
			]
		];

	return SNew(SOverlay)
		+ SOverlay::Slot()
		[SNew(SScaleBox).Stretch(EStretch::ScaleToFill)[SNew(SImage).Image(&BackgroundImage).ColorAndOpacity(APSMenu::Retint(FLinearColor(0.14f, 0.26f, 0.36f, 0.30f)))]]
		+ SOverlay::Slot()
		[SNew(SBorder).BorderImage(FAppStyle::GetBrush("WhiteBrush")).BorderBackgroundColor(APSMenu::Retint(FLinearColor(0.0f, 0.008f, 0.016f, 0.62f)))]
		+ SOverlay::Slot()[Page]
		+ SOverlay::Slot()
		[
			SAssignNew(SpawnPickerHost, SBox)
			.Visibility_Lambda([this]() { return bSpawnPickerOpen ? EVisibility::Visible : EVisibility::Collapsed; })
		];
}

void SAPSMainMenuRoot::OpenSpawnPicker(const EAPSStartAssetSlot Slot)
{
	if (!SpawnPickerHost.IsValid() || SpawnClassOptions.FindRef(Slot).Num() == 0)
	{
		return;
	}
	SpawnPickerHost->SetContent(BuildSpawnPicker(Slot));
	bSpawnPickerOpen = true;
	FSlateApplication::Get().SetKeyboardFocus(AsShared(), EFocusCause::SetDirectly);
}

void SAPSMainMenuRoot::CloseSpawnPicker()
{
	bSpawnPickerOpen = false;
	if (SpawnPickerHost.IsValid())
	{
		SpawnPickerHost->SetContent(SNullWidget::NullWidget);
	}
}

TSharedRef<SWidget> SAPSMainMenuRoot::BuildSpawnPicker(const EAPSStartAssetSlot Slot)
{
	const FText SlotName =
		Slot == EAPSStartAssetSlot::Character ? LOCTEXT("Character", "CHARACTER") :
		Slot == EAPSStartAssetSlot::Spaceship ? LOCTEXT("Spaceship", "SPACESHIP") :
		Slot == EAPSStartAssetSlot::SpaceStation ? LOCTEXT("Station", "SPACE STATION") :
		Slot == EAPSStartAssetSlot::Headquarters ? LOCTEXT("HQ", "HEADQUARTERS") : LOCTEXT("Shipyard", "SHIPYARD");
	const EAPSMenuGlyph SlotGlyph =
		Slot == EAPSStartAssetSlot::Character ? EAPSMenuGlyph::Pilot :
		Slot == EAPSStartAssetSlot::Spaceship ? EAPSMenuGlyph::Ship :
		Slot == EAPSStartAssetSlot::SpaceStation ? EAPSMenuGlyph::Station :
		Slot == EAPSStartAssetSlot::Headquarters ? EAPSMenuGlyph::Headquarters : EAPSMenuGlyph::Shipyard;
	const TSharedRef<SWrapBox> Grid = SNew(SWrapBox).UseAllottedSize(true).InnerSlotPadding(FVector2D(14.0f, 14.0f));
	const int32 Count = SpawnClassOptions.FindRef(Slot).Num();
	for (int32 OptionIndex = 0; OptionIndex < Count; ++OptionIndex)
	{
		const TSharedRef<SButton> Button = SNew(SButton)
			.ButtonStyle(FAppStyle::Get(), "NoBorder")
			.ContentPadding(0.0f)
			.ToolTipText(FText::FromString(SpawnClassOptions.FindRef(Slot)[OptionIndex].ToSoftObjectPath().ToString()))
			.OnClicked_Lambda([this, Slot, OptionIndex]()
			{
				SelectSpawnClass(Slot, OptionIndex);
				CloseSpawnPicker();
				return FReply::Handled();
			});
		const TWeakPtr<SButton> Weak = Button;
		const auto Lit = [Weak]()
		{
			const TSharedPtr<SButton> Pinned = Weak.Pin();
			return Pinned.IsValid() && Pinned->IsHovered();
		};
		const auto Picked = [this, Slot, OptionIndex]() { return SpawnClassIndices.FindRef(Slot) == OptionIndex; };
		Button->SetContent(
			SNew(SBox).WidthOverride(232.0f).HeightOverride(236.0f)
			[
				SNew(SAPSChamferedOverlay)
				+ SOverlay::Slot()
				[
					SNew(SChamferedSurface).Brush(FAppStyle::GetBrush("WhiteBrush")).ChamferTop(true).ChamferBottom(true)
					.Tint_Lambda([Lit, Picked]() { return Picked() ? APSMenu::CardSelectedFill : Lit() ? APSMenu::ChipLit : APSMenu::ChipRest; })
				]
				+ SOverlay::Slot().Padding(FMargin(12.0f, 12.0f, 12.0f, 10.0f))
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().FillHeight(1.0f)
					[
						SNew(SBorder).BorderImage(&APSMenu::InsetBrush).Padding(6.0f)
						[
							SNew(SOverlay)
							+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center)
							[
								SNew(SBox).WidthOverride(64.0f).HeightOverride(64.0f)
								.Visibility_Lambda([this, Slot, OptionIndex]() { return GetSpawnClassThumbnail(Slot, OptionIndex) ? EVisibility::Collapsed : EVisibility::Visible; })
								[SNew(SVectorMenuGlyph).Glyph(SlotGlyph).Color(APSMenu::CyanDim).StrokeWidth(1.6f)]
							]
							+ SOverlay::Slot()
							[
								SNew(SScaleBox).Stretch(EStretch::ScaleToFit)
								.Visibility_Lambda([this, Slot, OptionIndex]() { return GetSpawnClassThumbnail(Slot, OptionIndex) ? EVisibility::HitTestInvisible : EVisibility::Collapsed; })
								[SNew(SImage).Image_Lambda([this, Slot, OptionIndex]() { return GetSpawnClassThumbnail(Slot, OptionIndex); })]
							]
						]
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 9.0f, 0.0f, 0.0f)
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.0f, 0.0f, 8.0f, 0.0f)
						[
							SNew(STextBlock).Text(FText::FromString(FString::Printf(TEXT("%02d"), OptionIndex + 1)))
							.Font(APSMenu::Font("Bold", 10)).ColorAndOpacity_Lambda([Picked]() { return FSlateColor(Picked() ? APSMenu::ActionBright : APSMenu::Readable); })
						]
						+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center)
						[
							SNew(STextBlock).Text_Lambda([this, Slot, OptionIndex]() { return GetSpawnClassOptionName(Slot, OptionIndex); })
							.Font(APSMenu::Font("Bold", 12)).ColorAndOpacity(APSMenu::White).OverflowPolicy(ETextOverflowPolicy::Ellipsis)
						]
						+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(6.0f, 0.0f, 0.0f, 0.0f)
						[
							SNew(STextBlock).Text(LOCTEXT("SpawnOptionActive", "ACTIVE")).Font(APSMenu::Font("Bold", 8))
							.ColorAndOpacity_Lambda([]() { return FSlateColor(APSMenu::ActionBright); })
							.Visibility_Lambda([Picked]() { return Picked() ? EVisibility::Visible : EVisibility::Collapsed; })
						]
					]
				]
				+ SOverlay::Slot()
				[
					SNew(SChamferedFrame).Thickness(1.0f)
					.Color_Lambda([Lit, Picked]() { return Picked() ? APSMenu::Amber : Lit() ? APSMenu::Cyan : APSMenu::CyanDim; })
				]
			]);
		Grid->AddSlot()[Button];
	}

	const TSharedRef<SWidget> Panel = APSMenu::ChamferPanel(
		SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight()
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(STextBlock).Text(FText::Format(LOCTEXT("SpawnPickerTitle", "{0}  ·  CHOOSE A CLASS"), SlotName))
					.Font(APSMenu::Font("Bold", 20)).ColorAndOpacity(APSMenu::White)
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 4.0f, 0.0f, 0.0f)
				[
					SNew(STextBlock).Text(FText::Format(LOCTEXT("SpawnPickerHint", "{0} classes. A click picks one; Esc closes."), FText::AsNumber(Count)))
					.Font(APSMenu::Font("Regular", 14)).ColorAndOpacity(APSMenu::Readable)
				]
			]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
			[
				SNew(SButton).ButtonStyle(&SecondaryButtonStyle).ContentPadding(APSMenu::ButtonPadding("Bold", 13))
				.OnClicked_Lambda([this]() { CloseSpawnPicker(); return FReply::Handled(); })
				[
					SNew(STextBlock).Text(LOCTEXT("SpawnPickerClose", "CLOSE")).Font(APSMenu::Font("Bold", 13)).ColorAndOpacity(APSMenu::White)
					.RenderTransform(APSMenu::CapsShift("Bold", 13))
				]
			]
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 14.0f)
		[SNew(SBox).HeightOverride(1.0f)[SNew(SBorder).BorderImage(FAppStyle::GetBrush("WhiteBrush")).BorderBackgroundColor(APSMenu::CyanDim)]]
		+ SVerticalBox::Slot().FillHeight(1.0f)
		[
			SNew(SScrollBox) + SScrollBox::Slot()[Grid]
		],
		FMargin(26.0f, 22.0f), APSMenu::CyanDim);

	return SNew(SBorder)
		.BorderImage(FAppStyle::GetBrush("WhiteBrush"))
		.BorderBackgroundColor(APSUITheme::Fade(APSMenu::PageShade, 0.82f))
		.OnMouseButtonDown_Lambda([this](const FGeometry&, const FPointerEvent&)
		{
			// A click beside the panel closes it; the panel's own clicks never reach here.
			CloseSpawnPicker();
			return FReply::Handled();
		})
		.HAlign(HAlign_Center).VAlign(VAlign_Center).Padding(FMargin(60.0f, 50.0f))
		[
			SNew(SBox).MaxDesiredWidth(1290.0f).MaxDesiredHeight(860.0f)
			[
				SNew(SBorder).BorderImage(FStyleDefaults::GetNoBrush()).Padding(0.0f)
				.OnMouseButtonDown_Lambda([](const FGeometry&, const FPointerEvent&) { return FReply::Handled(); })
				[Panel]
			]
		];
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
	// The world browser paints each world's own scheme (SAPSWorldSchemePreview, Rio 03.10); its five stock pictures
	// under UI/Images/WorldBrowser are no longer loaded.
}

void SAPSMainMenuRoot::BeginAuxiliaryMenuLoad()
{
	// Rio 06.10: SETTINGS is native (SAPSSettingsPage); only the profile panel is still a Blueprint.
	ProfilePanelClass = TSoftClassPtr<UUserWidget>(FSoftObjectPath(
		TEXT("/Game/APS/APS_APOSFERA_SPACETRIPS/MBLS/Widgets/MainMenu/APS_WB_ProfileMenu.APS_WB_ProfileMenu_C")));

	TArray<FSoftObjectPath> Paths;
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
		PC->HoldSlateResource(ProfilePanelClass.Get());
	}
	AuxiliaryMenuLoadHandle.Reset();

	if (!ContentHost.IsValid())
	{
		return;
	}
	if (CurrentPage == EAPSMenuPage::Profile)
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
		ViewModel->DismissMainMenuHeroGalaxy();
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
FReply SAPSMainMenuRoot::SelectExistingWorld(TSharedPtr<FAPSExistingWorldEntry> Entry) { SelectedWorld = Entry; WorldBrowserNotice = FText::GetEmpty(); RebuildExistingWorldDetails(); return FReply::Handled(); }
FReply SAPSMainMenuRoot::QuitGame() { if (AMainMenuController* PC = Controller.Get()) UKismetSystemLibrary::QuitGame(PC, PC, EQuitPreference::Quit, false); return FReply::Handled(); }

void SAPSMainMenuRoot::OnWorldSearchChanged(const FText& SearchText)
{
	WorldSearch = SearchText.ToString();
	ExistingWorldPage = 0;
	ApplyExistingWorldView();
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
