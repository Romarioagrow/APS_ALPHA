#include "SAPSMainMenuRoot.h"

#include "APS_ALPHA/Core/Controllers/MainMenuController.h"
#include "APS_ALPHA/Core/Enums/CharSpawnPlace.h"
#include "APS_ALPHA/Core/Enums/OrbitHeight.h"
#include "APS_ALPHA/Core/Interfaces/ItemInfoInterface.h"
#include "APS_ALPHA/Core/Model/SpawnParameters.h"
#include "APS_ALPHA/Core/Model/GeneratedWorld.h"
#include "APS_ALPHA/Core/Saves/GameSave.h"
#include "APS_ALPHA/Core/Saves/GeneratedWorldData.h"
#include "APS_ALPHA/Pawns/Base/ControlledPawn.h"
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
#include "HAL/FileManager.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/Paths.h"
#include "Modules/ModuleManager.h"
#include "Rendering/DrawElements.h"
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
		Recent
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
			const float Cut = FMath::Clamp(FMath::Min(Size.X, Size.Y) * 0.025f, 8.0f, 18.0f);
			const TArray<FVector2D> Points = {
				FVector2D(Cut, 0.5f), FVector2D(Size.X - Cut, 0.5f),
				FVector2D(Size.X - 0.5f, Cut), FVector2D(Size.X - 0.5f, Size.Y - Cut),
				FVector2D(Size.X - Cut, Size.Y - 0.5f), FVector2D(Cut, Size.Y - 0.5f),
				FVector2D(0.5f, Size.Y - Cut), FVector2D(0.5f, Cut), FVector2D(Cut, 0.5f)
			};
			FSlateDrawElement::MakeLines(OutDrawElements, LayerId, Geometry.ToPaintGeometry(),
				Points, ESlateDrawEffect::None, Color, true, Thickness);
			return LayerId;
		}

	private:
		FLinearColor Color{FLinearColor::White};
		float Thickness{1.0f};
	};

	/** A real filled chamfered polygon. Unlike a rectangular SBorder with a
	 * chamfer outline on top, this leaves the cut corners genuinely empty. */
	class SChamferedSurface final : public SLeafWidget
	{
	public:
		SLATE_BEGIN_ARGS(SChamferedSurface) {}
			SLATE_ARGUMENT(const FSlateBrush*, Brush)
			SLATE_ARGUMENT(FLinearColor, Tint)
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
			const FColor VertexColor = Tint.ToFColor(true);
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
		FLinearColor Tint{FLinearColor::White};
		bool bChamferTop{true};
		bool bChamferBottom{true};
	};
}

namespace APSMenu
{
	const FLinearColor Background(0.002f, 0.009f, 0.017f, 0.22f);
	const FLinearColor Panel(0.004f, 0.025f, 0.041f, 0.94f);
	const FLinearColor PanelSoft(0.012f, 0.055f, 0.083f, 0.88f);
	const FLinearColor Cyan(0.12f, 0.82f, 1.0f, 1.0f);
	const FLinearColor CyanDim(0.05f, 0.30f, 0.42f, 1.0f);
	const FLinearColor Amber(1.0f, 0.55f, 0.04f, 1.0f);
	const FLinearColor White(0.92f, 0.97f, 1.0f, 1.0f);
	const FLinearColor Muted(0.48f, 0.62f, 0.70f, 1.0f);
	TWeakObjectPtr<UFont> DisplayFont;
	TWeakObjectPtr<UFont> BodyFont;
	const FSlateRoundedBoxBrush PanelBrush(Panel, 10.0f, CyanDim, 1.0f);
	const FSlateRoundedBoxBrush PanelSoftBrush(PanelSoft, 9.0f, CyanDim, 1.0f);
	const FSlateRoundedBoxBrush InsetBrush(FLinearColor(0.001f, 0.012f, 0.022f, 0.96f), 6.0f, FLinearColor(0.04f, 0.22f, 0.31f, 1.0f), 1.0f);
	const FSlateRoundedBoxBrush CyanBadgeBrush(FLinearColor(0.01f, 0.07f, 0.10f, 0.98f), 18.0f, Cyan, 1.25f);
	const FSlateRoundedBoxBrush AmberPanelBrush(FLinearColor(0.11f, 0.045f, 0.002f, 0.96f), 9.0f, Amber, 1.4f);

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
		Metadata.GetString(TEXT("APSWorld"), TEXT("Environment"), Entry.Environment);
		int64 Value = 0;
		if (Metadata.GetInt64(TEXT("APSWorld"), TEXT("TotalPlanets"), Value))
		{
			Entry.TotalPlanets = FMath::Max<int32>(0, static_cast<int32>(Value));
		}
		if (Metadata.GetInt64(TEXT("APSWorld"), TEXT("InhabitedPlanets"), Value))
		{
			Entry.InhabitedPlanets = FMath::Max<int32>(0, static_cast<int32>(Value));
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

	switch (CurrentPage)
	{
	case EAPSMenuPage::Landing: ContentHost->SetContent(BuildLandingPage()); break;
	case EAPSMenuPage::ChoosePath: ContentHost->SetContent(BuildChoosePathPage()); break;
	case EAPSMenuPage::ExistingWorlds: ContentHost->SetContent(BuildExistingWorldsPage()); break;
	case EAPSMenuPage::AstronomicalGeneration:
		ContentHost->SetContent(
			SNew(SWorldGenerationPanel)
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
	const FSlateBrush* Image, const FLinearColor& Accent, FSimpleDelegate Action, bool bLarge, bool bEnabled)
{
	const FString UpperTitle = Title.ToString().ToUpper();
	const FText Glyph = FText::FromString(
		UpperTitle.Contains(TEXT("START")) ? TEXT("*") :
		UpperTitle.Contains(TEXT("VISIT")) ? TEXT("W") :
		UpperTitle.Contains(TEXT("CIVILIZATION")) ? TEXT("C") :
		UpperTitle.Contains(TEXT("SPACE")) ? TEXT("S") :
		UpperTitle.Contains(TEXT("PLANET")) ? TEXT("P") : TEXT("X"));
	return SNew(SButton)
		.IsEnabled(bEnabled)
		.ButtonStyle(&FAppStyle::Get().GetWidgetStyle<FButtonStyle>("NoBorder"))
		.OnClicked_Lambda([Action]() mutable { Action.ExecuteIfBound(); return FReply::Handled(); })
		.ContentPadding(0.0f)
		[
			SNew(SOverlay)
			+ SOverlay::Slot()
			[
				SNew(SChamferedSurface)
				.Brush(Image)
				.Tint(bEnabled ? FLinearColor::White : FLinearColor(0.18f, 0.22f, 0.25f, 0.42f))
				.ChamferTop(true)
				.ChamferBottom(true)
			]
			+ SOverlay::Slot().VAlign(VAlign_Bottom)
			[
				SNew(SBox).HeightOverride(bLarge ? 218.0f : 132.0f)
				[
				SNew(SOverlay)
				+ SOverlay::Slot()
				[
					SNew(SChamferedSurface)
					.Brush(FAppStyle::GetBrush("WhiteBrush"))
					.Tint(FLinearColor(0.001f, 0.010f, 0.018f, 0.94f))
					.ChamferTop(false)
					.ChamferBottom(true)
				]
				+ SOverlay::Slot().Padding(FMargin(bLarge ? 30.0f : 22.0f, bLarge ? 24.0f : 16.0f))
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight()
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
						[APSMenu::Badge(Glyph, bEnabled ? Accent : APSMenu::Muted, bLarge ? 44.0f : 34.0f)]
						+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center).Padding(12.0f, 0.0f)
						[SNew(STextBlock).Text(Title).Font(APSMenu::Font("Bold", bLarge ? 28 : 17)).ColorAndOpacity(bEnabled ? APSMenu::White : APSMenu::Muted)]
						+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
						[SNew(STextBlock).Text(FText::FromString(bEnabled ? TEXT(">") : TEXT("LOCK"))).Font(APSMenu::Font("Bold", bEnabled ? 22 : 9)).ColorAndOpacity(bEnabled ? Accent : APSMenu::Muted)]
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 7.0f, 0.0f, 0.0f)
					[SNew(STextBlock).Text(Description).AutoWrapText(true).Font(APSMenu::Font("Regular", bLarge ? 15 : 12)).ColorAndOpacity(bEnabled ? Accent : APSMenu::Muted)]
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
				.Color(bEnabled ? Accent : APSMenu::Muted)
				.Thickness(bLarge ? 1.8f : 1.2f)
			]
		];
}

TSharedRef<SWidget> SAPSMainMenuRoot::BuildChoosePathPage()
{
	TSharedRef<SWidget> Foreground = SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().Padding(30.0f, 20.0f, 30.0f, 10.0f)[BuildHeader(LOCTEXT("ChoosePath", "CHOOSE YOUR PATH"))]
		+ SVerticalBox::Slot().FillHeight(1.0f).Padding(58.0f, 18.0f, 58.0f, 46.0f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(0.31f).Padding(7.0f)
			[
				BuildPathCard(LOCTEXT("StartGame", "START SINGLE GAME"),
					LOCTEXT("StartGameDesc", "Begin a new journey through the live full-scale world."), &SystemImage,
					APSMenu::Amber, FSimpleDelegate::CreateLambda([this]() { StartSingleGame(); }), true)
			]
			+ SHorizontalBox::Slot().FillWidth(0.69f).Padding(7.0f)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().FillHeight(0.5f).Padding(0.0f, 0.0f, 0.0f, 6.0f)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(6.0f, 0.0f)
					[BuildPathCard(LOCTEXT("VisitWorld", "VISIT EXISTING WORLD"), LOCTEXT("VisitDesc", "Explore worlds you have already created."), &PlanetImage, APSMenu::Cyan, FSimpleDelegate::CreateLambda([this]() { OpenExistingWorlds(); }))]
					+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(6.0f, 0.0f)
					[BuildPathCard(LOCTEXT("GenCiv", "GENERATE CIVILIZATION"), LOCTEXT("GenCivDesc", "Create a civilization and shape its astronomical home."), &CivilizationImage, APSMenu::Cyan, FSimpleDelegate::CreateLambda([this]() { OpenAstronomicalGeneration(EAstroPreviewFocus::HomeSystem, EAPSGenerationRoute::Civilization); }))]
					+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(6.0f, 0.0f)
					[BuildPathCard(LOCTEXT("GenSpace", "GENERATE SPACE"), LOCTEXT("GenSpaceDesc", "Procedurally generate stellar systems and clusters."), &GalaxyImage, APSMenu::Cyan, FSimpleDelegate::CreateLambda([this]() { OpenAstronomicalGeneration(EAstroPreviewFocus::StarCluster, EAPSGenerationRoute::Space); }))]
				]
				+ SVerticalBox::Slot().FillHeight(0.5f).Padding(0.0f, 6.0f, 0.0f, 0.0f)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(6.0f, 0.0f)
					[BuildPathCard(LOCTEXT("CreatePlanet", "CREATE PLANET"), LOCTEXT("PlanetDesc", "Design a planet with atmosphere, terrain and moons."), &PlanetImage, APSMenu::Cyan, FSimpleDelegate::CreateLambda([this]() { OpenAstronomicalGeneration(EAstroPreviewFocus::HomePlanet, EAPSGenerationRoute::Planet); }))]
					+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(6.0f, 0.0f)
					[BuildPathCard(LOCTEXT("Story", "STORY MODE"), LOCTEXT("StoryDesc", "Unravel the deeper story of Aposfera.  COMING SOON"), &GalaxyImage, APSMenu::Muted, FSimpleDelegate(), false, false)]
				]
			]
		];

	return SNew(SOverlay)
		+ SOverlay::Slot()
		[SNew(SScaleBox).Stretch(EStretch::ScaleToFill)[SNew(SImage).Image(&BackgroundImage).ColorAndOpacity(FLinearColor(0.22f, 0.34f, 0.44f, 0.32f))]]
		+ SOverlay::Slot()
		[SNew(SBorder).BorderImage(FAppStyle::GetBrush("WhiteBrush")).BorderBackgroundColor(FLinearColor(0.0f, 0.012f, 0.024f, 0.44f))]
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

	const TSet<FTopLevelAssetPath> CharacterClassPaths = FindDerivedClasses(AControlledPawn::StaticClass());
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
			Slot = EAPSStartAssetSlot::Character;
		}
		SpawnClassOptions.FindOrAdd(Slot).AddUnique(TSoftClassPtr<AActor>(FSoftObjectPath(GeneratedClassObjectPath)));
	}

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
		RefreshSpawnClassBrush(Pair.Key);
	}
}

void SAPSMainMenuRoot::ApplySpawnClassSelection(EAPSStartAssetSlot Slot)
{
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
		RefreshSpawnClassBrush(Slot);
		Invalidate(EInvalidateWidgetReason::Paint);
		return;
	}

	const FSoftObjectPath RequestedPath = SelectedClass.ToSoftObjectPath();
	if (!RequestedPath.IsValid())
	{
		return;
	}
	SpawnSelectionLoadHandles.Add(Slot, UAssetManager::GetStreamableManager().RequestAsyncLoad(
		RequestedPath,
		FStreamableDelegate::CreateSP(this, &SAPSMainMenuRoot::OnSpawnClassSelectionLoaded, Slot, RequestedPath)));
}

void SAPSMainMenuRoot::OnSpawnClassSelectionLoaded(EAPSStartAssetSlot Slot, FSoftObjectPath RequestedPath)
{
	SpawnSelectionLoadHandles.Remove(Slot);
	const TArray<TSoftClassPtr<AActor>>* Options = SpawnClassOptions.Find(Slot);
	const int32 Index = SpawnClassIndices.FindRef(Slot);
	if (!Options || !Options->IsValidIndex(Index)
		|| (*Options)[Index].ToSoftObjectPath() != RequestedPath)
	{
		return;
	}
	ApplySpawnClassSelection(Slot);
}

void SAPSMainMenuRoot::RefreshSpawnClassBrush(EAPSStartAssetSlot Slot)
{
	FSlateBrush& Brush = SpawnClassBrushes.FindOrAdd(Slot);
	Brush.SetResourceObject(nullptr);
	Brush.DrawAs = ESlateBrushDrawType::NoDrawType;
	const TArray<TSoftClassPtr<AActor>>* Options = SpawnClassOptions.Find(Slot);
	const int32 Index = SpawnClassIndices.FindRef(Slot);
	if (!Options || !Options->IsValidIndex(Index)) return;
	UClass* LoadedClass = (*Options)[Index].Get();
	if (!LoadedClass) return;
	const AActor* DefaultActor = LoadedClass->GetDefaultObject<AActor>();
	if (!DefaultActor || !DefaultActor->Implements<UItemInfoInterface>()) return;
	if (UTexture2D* Texture = IItemInfoInterface::Execute_GetAvatarPicture(DefaultActor))
	{
		if (AMainMenuController* PC = Controller.Get()) PC->HoldSlateResource(Texture);
		Brush.SetResourceObject(Texture);
		Brush.ImageSize = FVector2D(256.0f, 180.0f);
		Brush.DrawAs = ESlateBrushDrawType::Image;
	}
}

const FSlateBrush* SAPSMainMenuRoot::GetSpawnClassBrush(EAPSStartAssetSlot Slot) const
{
	return SpawnClassBrushes.Find(Slot);
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
	const FText SlotGlyph = FText::FromString(
		Slot == EAPSStartAssetSlot::Character ? TEXT("PILOT") :
		Slot == EAPSStartAssetSlot::Spaceship ? TEXT("SHIP") :
		Slot == EAPSStartAssetSlot::SpaceStation ? TEXT("STATION") :
		Slot == EAPSStartAssetSlot::Headquarters ? TEXT("HQ") : TEXT("YARD"));
	TSharedRef<SUniformGridPanel> OptionGrid = SNew(SUniformGridPanel).SlotPadding(FMargin(2.0f));
	const TArray<TSoftClassPtr<AActor>>& Options = SpawnClassOptions.FindRef(Slot);
	for (int32 OptionIndex = 0; OptionIndex < Options.Num(); ++OptionIndex)
	{
		const int32 CapturedIndex = OptionIndex;
		OptionGrid->AddSlot(OptionIndex % 2, OptionIndex / 2)
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
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(STextBlock)
					.Text(FText::FromString(FString::Printf(TEXT("%02d"), OptionIndex + 1)))
					.Font(APSMenu::Font("Bold", 7))
					.ColorAndOpacity_Lambda([this, Slot, CapturedIndex]()
					{
						return SpawnClassIndices.FindRef(Slot) == CapturedIndex ? APSMenu::Amber : APSMenu::Cyan;
					})
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 2.0f)
				[
					SNew(STextBlock)
					.Text_Lambda([this, Slot, CapturedIndex]() { return GetSpawnClassOptionName(Slot, CapturedIndex); })
					.Font(APSMenu::Font("Bold", 8)).ColorAndOpacity(APSMenu::White)
					.AutoWrapText(true)
				]
			]
		];
	}

	return SNew(SOverlay)
		+ SOverlay::Slot()
		[
			SNew(SBorder).BorderImage(&APSMenu::PanelBrush).Padding(10.0f)
			[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0.0f, 2.0f, 0.0f, 8.0f)
			[SNew(STextBlock).Text(Label).Font(APSMenu::Font("Bold", 15)).ColorAndOpacity(APSMenu::Cyan)]
			+ SVerticalBox::Slot().FillHeight(0.58f).Padding(2.0f, 4.0f)
			[
				SNew(SBorder).BorderImage(&APSMenu::InsetBrush).Padding(2.0f)
				[
					SNew(SOverlay)
					+ SOverlay::Slot()
					[
						SNew(SBox).Clipping(EWidgetClipping::ClipToBounds)
						[SNew(SScaleBox).Stretch(EStretch::ScaleToFit)[SNew(SImage).Image_Lambda([this, Slot]() { return GetSpawnClassBrush(Slot); })]]
					]
					+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center)
					[
						SNew(STextBlock).Text(SlotGlyph).Font(APSMenu::Font("Bold", 22)).ColorAndOpacity(FLinearColor(0.20f, 0.62f, 0.78f, 0.42f))
						.Visibility_Lambda([this, Slot]()
						{
							const FSlateBrush* Brush = GetSpawnClassBrush(Slot);
							return Brush && Brush->GetResourceObject() ? EVisibility::Collapsed : EVisibility::Visible;
						})
					]
				]
			]
			+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0.0f, 9.0f)
			[SNew(STextBlock).Text_Lambda([this, Slot]() { return GetSpawnClassName(Slot); }).Font(APSMenu::Font("Bold", 14)).ColorAndOpacity(APSMenu::White).Justification(ETextJustify::Center)]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 3.0f, 0.0f, 5.0f)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().FillWidth(0.32f)[SNew(SButton).ButtonStyle(&SecondaryButtonStyle).OnClicked(this, &SAPSMainMenuRoot::CycleSpawnClass, Slot, -1)[SNew(STextBlock).Text(FText::FromString(TEXT("<"))).Justification(ETextJustify::Center).ColorAndOpacity(APSMenu::Cyan)]]
				+ SHorizontalBox::Slot().FillWidth(0.36f).HAlign(HAlign_Center).VAlign(VAlign_Center)
				[
					SNew(STextBlock).Text_Lambda([this, Slot]()
					{
						const int32 Count = SpawnClassOptions.FindRef(Slot).Num();
						return FText::FromString(FString::Printf(TEXT("%02d / %02d"), Count > 0 ? SpawnClassIndices.FindRef(Slot) + 1 : 0, Count));
					}).Font(APSMenu::Font("Regular", 8)).ColorAndOpacity(APSMenu::Muted)
				]
				+ SHorizontalBox::Slot().FillWidth(0.32f)[SNew(SButton).ButtonStyle(&SecondaryButtonStyle).OnClicked(this, &SAPSMainMenuRoot::CycleSpawnClass, Slot, 1)[SNew(STextBlock).Text(FText::FromString(TEXT(">"))).Justification(ETextJustify::Center).ColorAndOpacity(APSMenu::Cyan)]]
			]
			+ SVerticalBox::Slot().FillHeight(0.42f)
			[
				SNew(SBorder).BorderImage(&APSMenu::InsetBrush).Padding(3.0f)
				[
					SNew(SScrollBox)
					+ SScrollBox::Slot()[OptionGrid]
				]
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
		return SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()[SNew(STextBlock).Text(Label).Font(APSMenu::Font("Regular", 10)).ColorAndOpacity(APSMenu::Muted)]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 3.0f, 0.0f, 9.0f)
			[
				SNew(SBorder).BorderImage(&APSMenu::InsetBrush).Padding(FMargin(2.0f))
				[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth()[SNew(SButton).ButtonStyle(&SecondaryButtonStyle).OnClicked_Lambda([Step](){ return Step(-1); })[SNew(STextBlock).Text(FText::FromString(TEXT("<"))).ColorAndOpacity(APSMenu::Cyan)]]
				+ SHorizontalBox::Slot().FillWidth(1.0f).HAlign(HAlign_Center).VAlign(VAlign_Center)
				[SNew(STextBlock).Text_Lambda([Enum, Getter](){ return Enum ? Enum->GetDisplayNameTextByValue(Getter()) : FText::FromString(TEXT("--")); }).Font(APSMenu::Font("Bold", 10)).ColorAndOpacity(APSMenu::White)]
				+ SHorizontalBox::Slot().AutoWidth()[SNew(SButton).ButtonStyle(&SecondaryButtonStyle).OnClicked_Lambda([Step](){ return Step(1); })[SNew(STextBlock).Text(FText::FromString(TEXT(">"))).ColorAndOpacity(APSMenu::Cyan)]]
				]
			];
	};

	const auto NumberControl = [](const FText& Label, int32 MinValue, int32 MaxValue, int32 Delta,
		TFunction<int32()> Getter, TFunction<void(int32)> Setter)
	{
		return SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()[SNew(STextBlock).Text(Label).Font(APSMenu::Font("Regular", 10)).ColorAndOpacity(APSMenu::Muted)]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 3.0f, 0.0f, 9.0f)
			[
				SNew(SBorder).BorderImage(&APSMenu::InsetBrush).Padding(2.0f)
				[
				SNew(SSpinBox<int32>).MinValue(MinValue).MaxValue(MaxValue).Delta(Delta)
				.Value_Lambda([Getter](){ return Getter(); })
				.OnValueChanged_Lambda([Setter](int32 Value){ Setter(Value); })
				]
			];
	};

	const auto InfoPanel = [](const FText& Glyph, const FText& Title, TAttribute<FText> Body)
	{
		return APSMenu::ChamferPanel(
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()[APSMenu::SectionHeading(Glyph, Title)]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 10.0f, 0.0f, 0.0f)[SNew(STextBlock).Text(Body).AutoWrapText(true).Font(APSMenu::Font("Regular", 10)).ColorAndOpacity(APSMenu::White)]
		, FMargin(16.0f));
	};

	TSharedRef<SWidget> Page = SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().Padding(28.0f, 18.0f, 28.0f, 6.0f)[BuildHeader(LOCTEXT("CivParameters", "CIVILIZATION PARAMETERS"))]
		+ SVerticalBox::Slot().FillHeight(1.0f).Padding(34.0f, 10.0f, 34.0f, 8.0f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(0.74f).Padding(5.0f)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().FillHeight(0.56f)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(4.0f)[BuildSpawnCard(EAPSStartAssetSlot::Character, LOCTEXT("Character", "CHARACTER"))]
					+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(4.0f)[BuildSpawnCard(EAPSStartAssetSlot::Spaceship, LOCTEXT("Spaceship", "SPACESHIP"))]
					+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(4.0f)[BuildSpawnCard(EAPSStartAssetSlot::SpaceStation, LOCTEXT("Station", "SPACE STATION"))]
					+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(4.0f)[BuildSpawnCard(EAPSStartAssetSlot::Headquarters, LOCTEXT("HQ", "HEADQUARTERS"))]
					+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(4.0f)[BuildSpawnCard(EAPSStartAssetSlot::Shipyard, LOCTEXT("Shipyard", "SHIPYARD"))]
				]
				+ SVerticalBox::Slot().FillHeight(0.44f).Padding(0.0f, 10.0f, 0.0f, 0.0f)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(4.0f)
					[InfoPanel(LOCTEXT("CivGlyph", "CIV"), LOCTEXT("CivilizationStatus", "CIVILIZATION STATUS"), TAttribute<FText>::CreateLambda([VM](){ const USpawnParameters* P=VM.IsValid()?VM->SpawnParameters.Get():nullptr; return P ? FText::FromString(FString::Printf(TEXT("Government: %s\nEconomy: %s\nSociety: %s\nPopulation: %s"), *APSMenu::EnumLabel(P->GovernmentType), *APSMenu::EnumLabel(P->EconomicSystem), *APSMenu::EnumLabel(P->SocietyType), *FText::AsNumber(P->FoundingPopulation).ToString())) : FText::GetEmpty(); }))]
					+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(4.0f)
					[InfoPanel(LOCTEXT("SystemGlyph", "SYS"), LOCTEXT("StarSystemInfo", "STAR SYSTEM INFO"), TAttribute<FText>::CreateLambda([VM](){ const UGeneratedWorld* W=VM.IsValid()?VM->GeneratedWorld.Get():nullptr; return W ? FText::FromString(FString::Printf(TEXT("System: %s\nStar: %s / %s\nPlanets: %d\nHome planet: %s / %.0f KM"), *APSMenu::EnumLabel(W->PlanetarySystemType), *APSMenu::EnumLabel(W->StellarType), *APSMenu::EnumLabel(W->SpectralClass), W->PlanetsAmount, *APSMenu::EnumLabel(W->PlanetType), W->PlanetRadius)) : FText::GetEmpty(); }))]
					+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(4.0f)
					[InfoPanel(LOCTEXT("InfraGlyph", "INF"), LOCTEXT("Infrastructure", "INFRASTRUCTURE"), TAttribute<FText>::CreateLambda([VM](){ const USpawnParameters* P=VM.IsValid()?VM->SpawnParameters.Get():nullptr; return P ? FText::FromString(FString::Printf(TEXT("Star outposts: %d\nPlanet outposts: %d\nOrbital stations: %d\nGround settlements: %d\nStarting fleet: %d"), P->StarOutposts, P->PlanetOutposts, P->OrbitalOutposts, P->GroundOutposts, P->StartingFleetSize)) : FText::GetEmpty(); }))]
					+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(4.0f)
					[InfoPanel(LOCTEXT("DivisionGlyph", "DIV"), LOCTEXT("Divisions", "DIVISIONS"), TAttribute<FText>::CreateLambda([VM](){ const USpawnParameters* P=VM.IsValid()?VM->SpawnParameters.Get():nullptr; return P ? FText::FromString(FString::Printf(TEXT("Exploration     Lv.%d\nIndustry          Lv.%d\nScience           Lv.%d\nCivil Affairs    Lv.%d\nMilitary           Lv.%d\nFleet                Lv.%d"), P->ExplorationDivisionLevel, P->IndustryDivisionLevel, P->ScienceDivisionLevel, P->CivilAffairsDivisionLevel, P->MilitaryDivisionLevel, P->FleetDivisionLevel)) : FText::GetEmpty(); }))]
				]
			]
			+ SHorizontalBox::Slot().FillWidth(0.26f).Padding(5.0f)
			[
				APSMenu::ChamferPanel(
					SNew(SScrollBox)
					+ SScrollBox::Slot()
					[
						SNew(SVerticalBox)
						+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 12.0f)[APSMenu::SectionHeading(LOCTEXT("SpawnGlyph", "CIV"), LOCTEXT("SpawnParameters", "CIVILIZATION & SPAWN"))]
						+ SVerticalBox::Slot().AutoHeight()[SNew(STextBlock).Text(LOCTEXT("CivilizationName", "CIVILIZATION NAME")).Font(APSMenu::Font("Regular", 10)).ColorAndOpacity(APSMenu::Muted)]
						+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 4.0f, 0.0f, 10.0f)[SNew(SEditableTextBox).Text_Lambda([VM](){ const USpawnParameters* P=VM.IsValid()?VM->SpawnParameters.Get():nullptr; return P?FText::FromString(P->CivilizationName):FText::GetEmpty(); }).OnTextCommitted_Lambda([VM](const FText& T,ETextCommit::Type){ if(VM.IsValid()&&VM->SpawnParameters) VM->SpawnParameters->CivilizationName=T.ToString(); })]
						+ SVerticalBox::Slot().AutoHeight()[EnumControl(LOCTEXT("Archetype", "ARCHETYPE"), StaticEnum<EAPSCivilizationArchetype>(), [VM](){return VM.IsValid()&&VM->SpawnParameters?static_cast<int32>(VM->SpawnParameters->CivilizationArchetype):0;}, [VM](int32 V){if(VM.IsValid()&&VM->SpawnParameters)VM->SpawnParameters->CivilizationArchetype=static_cast<EAPSCivilizationArchetype>(V);})]
						+ SVerticalBox::Slot().AutoHeight()[EnumControl(LOCTEXT("Government", "GOVERNMENT"), StaticEnum<EAPSGovernmentType>(), [VM](){return VM.IsValid()&&VM->SpawnParameters?static_cast<int32>(VM->SpawnParameters->GovernmentType):0;}, [VM](int32 V){if(VM.IsValid()&&VM->SpawnParameters)VM->SpawnParameters->GovernmentType=static_cast<EAPSGovernmentType>(V);})]
						+ SVerticalBox::Slot().AutoHeight()[EnumControl(LOCTEXT("Economy", "ECONOMY"), StaticEnum<EAPSEconomicSystem>(), [VM](){return VM.IsValid()&&VM->SpawnParameters?static_cast<int32>(VM->SpawnParameters->EconomicSystem):0;}, [VM](int32 V){if(VM.IsValid()&&VM->SpawnParameters)VM->SpawnParameters->EconomicSystem=static_cast<EAPSEconomicSystem>(V);})]
						+ SVerticalBox::Slot().AutoHeight()[EnumControl(LOCTEXT("Society", "SOCIETY"), StaticEnum<EAPSSocietyType>(), [VM](){return VM.IsValid()&&VM->SpawnParameters?static_cast<int32>(VM->SpawnParameters->SocietyType):0;}, [VM](int32 V){if(VM.IsValid()&&VM->SpawnParameters)VM->SpawnParameters->SocietyType=static_cast<EAPSSocietyType>(V);})]
						+ SVerticalBox::Slot().AutoHeight()[NumberControl(LOCTEXT("FoundingPopulation", "FOUNDING POPULATION"), 1, 100000000, 1000, [VM](){return VM.IsValid()&&VM->SpawnParameters?VM->SpawnParameters->FoundingPopulation:1;}, [VM](int32 V){if(VM.IsValid()&&VM->SpawnParameters)VM->SpawnParameters->FoundingPopulation=V;})]
						+ SVerticalBox::Slot().AutoHeight()[NumberControl(LOCTEXT("StartingCredits", "STARTING CREDITS"), 0, 2000000000, 10000, [VM](){return VM.IsValid()&&VM->SpawnParameters?static_cast<int32>(FMath::Min<int64>(VM->SpawnParameters->StartingCredits, MAX_int32)):0;}, [VM](int32 V){if(VM.IsValid()&&VM->SpawnParameters)VM->SpawnParameters->StartingCredits=V;})]
						+ SVerticalBox::Slot().AutoHeight()[NumberControl(LOCTEXT("Technology", "TECHNOLOGY LEVEL"), 1, 10, 1, [VM](){return VM.IsValid()&&VM->SpawnParameters?VM->SpawnParameters->TechnologyLevel:1;}, [VM](int32 V){if(VM.IsValid()&&VM->SpawnParameters)VM->SpawnParameters->TechnologyLevel=V;})]
						+ SVerticalBox::Slot().AutoHeight()[EnumControl(LOCTEXT("SpawnPlace", "START LOCATION"), StaticEnum<ECharSpawnPlace>(), [VM](){return VM.IsValid()&&VM->SpawnParameters?static_cast<int32>(VM->SpawnParameters->CharacterSpawnPlace):0;}, [VM](int32 V){if(VM.IsValid())VM->SetCharacterSpawnPlace(V);})]
						+ SVerticalBox::Slot().AutoHeight()[EnumControl(LOCTEXT("OrbitHeight", "HOME ORBIT"), StaticEnum<EOrbitHeight>(), [VM](){return VM.IsValid()&&VM->SpawnParameters?static_cast<int32>(VM->SpawnParameters->HomeStationOrbitHeight):0;}, [VM](int32 V){if(VM.IsValid())VM->SetStationOrbitHeight(V);})]
						+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 12.0f, 0.0f, 8.0f)[APSMenu::SectionHeading(LOCTEXT("StartingInfraGlyph", "INF"), LOCTEXT("InfrastructureSetup", "STARTING INFRASTRUCTURE"))]
						+ SVerticalBox::Slot().AutoHeight()[NumberControl(LOCTEXT("FleetSize", "STARTING FLEET"), 0, 1000, 1, [VM](){return VM.IsValid()&&VM->SpawnParameters?VM->SpawnParameters->StartingFleetSize:0;}, [VM](int32 V){if(VM.IsValid()&&VM->SpawnParameters)VM->SpawnParameters->StartingFleetSize=V;})]
						+ SVerticalBox::Slot().AutoHeight()[NumberControl(LOCTEXT("StarOutposts", "STAR OUTPOSTS"), 0, 100, 1, [VM](){return VM.IsValid()&&VM->SpawnParameters?VM->SpawnParameters->StarOutposts:0;}, [VM](int32 V){if(VM.IsValid()&&VM->SpawnParameters)VM->SpawnParameters->StarOutposts=V;})]
						+ SVerticalBox::Slot().AutoHeight()[NumberControl(LOCTEXT("PlanetOutposts", "PLANET OUTPOSTS"), 0, 100, 1, [VM](){return VM.IsValid()&&VM->SpawnParameters?VM->SpawnParameters->PlanetOutposts:0;}, [VM](int32 V){if(VM.IsValid()&&VM->SpawnParameters)VM->SpawnParameters->PlanetOutposts=V;})]
						+ SVerticalBox::Slot().AutoHeight()[NumberControl(LOCTEXT("OrbitalOutposts", "ORBITAL STATIONS"), 0, 100, 1, [VM](){return VM.IsValid()&&VM->SpawnParameters?VM->SpawnParameters->OrbitalOutposts:0;}, [VM](int32 V){if(VM.IsValid()&&VM->SpawnParameters)VM->SpawnParameters->OrbitalOutposts=V;})]
						+ SVerticalBox::Slot().AutoHeight()[NumberControl(LOCTEXT("GroundOutposts", "GROUND SETTLEMENTS"), 0, 100, 1, [VM](){return VM.IsValid()&&VM->SpawnParameters?VM->SpawnParameters->GroundOutposts:0;}, [VM](int32 V){if(VM.IsValid()&&VM->SpawnParameters)VM->SpawnParameters->GroundOutposts=V;})]
						+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 12.0f, 0.0f, 8.0f)[APSMenu::SectionHeading(LOCTEXT("DivisionSetupGlyph", "DIV"), LOCTEXT("DivisionSetup", "DIVISION LEVELS"))]
						+ SVerticalBox::Slot().AutoHeight()[NumberControl(LOCTEXT("ExplorationDivision", "EXPLORATION"), 0, 20, 1, [VM](){return VM.IsValid()&&VM->SpawnParameters?VM->SpawnParameters->ExplorationDivisionLevel:0;}, [VM](int32 V){if(VM.IsValid()&&VM->SpawnParameters)VM->SpawnParameters->ExplorationDivisionLevel=V;})]
						+ SVerticalBox::Slot().AutoHeight()[NumberControl(LOCTEXT("IndustryDivision", "INDUSTRY"), 0, 20, 1, [VM](){return VM.IsValid()&&VM->SpawnParameters?VM->SpawnParameters->IndustryDivisionLevel:0;}, [VM](int32 V){if(VM.IsValid()&&VM->SpawnParameters)VM->SpawnParameters->IndustryDivisionLevel=V;})]
						+ SVerticalBox::Slot().AutoHeight()[NumberControl(LOCTEXT("ScienceDivision", "SCIENCE / RESEARCH"), 0, 20, 1, [VM](){return VM.IsValid()&&VM->SpawnParameters?VM->SpawnParameters->ScienceDivisionLevel:0;}, [VM](int32 V){if(VM.IsValid()&&VM->SpawnParameters)VM->SpawnParameters->ScienceDivisionLevel=V;})]
						+ SVerticalBox::Slot().AutoHeight()[NumberControl(LOCTEXT("CivilDivision", "CIVIL AFFAIRS"), 0, 20, 1, [VM](){return VM.IsValid()&&VM->SpawnParameters?VM->SpawnParameters->CivilAffairsDivisionLevel:0;}, [VM](int32 V){if(VM.IsValid()&&VM->SpawnParameters)VM->SpawnParameters->CivilAffairsDivisionLevel=V;})]
						+ SVerticalBox::Slot().AutoHeight()[NumberControl(LOCTEXT("MilitaryDivision", "MILITARY"), 0, 20, 1, [VM](){return VM.IsValid()&&VM->SpawnParameters?VM->SpawnParameters->MilitaryDivisionLevel:0;}, [VM](int32 V){if(VM.IsValid()&&VM->SpawnParameters)VM->SpawnParameters->MilitaryDivisionLevel=V;})]
						+ SVerticalBox::Slot().AutoHeight()[NumberControl(LOCTEXT("FleetDivision", "FLEET COMMAND"), 0, 20, 1, [VM](){return VM.IsValid()&&VM->SpawnParameters?VM->SpawnParameters->FleetDivisionLevel:0;}, [VM](int32 V){if(VM.IsValid()&&VM->SpawnParameters)VM->SpawnParameters->FleetDivisionLevel=V;})]
					]
				, FMargin(16.0f))
			]
		]
		+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0.0f, 8.0f, 0.0f, 24.0f)
		[
			SNew(SBox).WidthOverride(440.0f)
			[
				SNew(SButton).ButtonStyle(&PrimaryButtonStyle).OnClicked(this, &SAPSMainMenuRoot::CommitCivilization)
				.IsEnabled_Lambda([this]() { return SpawnSelectionLoadHandles.IsEmpty(); })
				.ContentPadding(FMargin(24.0f, 16.0f))
				[
					SNew(STextBlock)
					.Text_Lambda([this]()
					{
						return SpawnSelectionLoadHandles.IsEmpty()
							? LOCTEXT("GenerateWorld", "GENERATE WORLD  >")
							: LOCTEXT("LoadingSpawnSelections", "LOADING SELECTIONS...");
					})
					.Justification(ETextJustify::Center).Font(APSMenu::Font("Bold", 19)).ColorAndOpacity(APSMenu::White)
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
	if (ViewModel.IsValid()) ViewModel->SetGenerationRoute(Route);
	Navigate(EAPSMenuPage::AstronomicalGeneration);
	if (ViewModel.IsValid()) ViewModel->SetPreviewFocus(Focus);
	return FReply::Handled();
}
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
