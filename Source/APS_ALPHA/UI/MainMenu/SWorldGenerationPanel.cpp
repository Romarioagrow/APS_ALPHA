#include "SWorldGenerationPanel.h"

#include "APS_ALPHA/Core/Enums/AstroGenerationLevel.h"
#include "APS_ALPHA/Core/Enums/GalaxyClass.h"
#include "APS_ALPHA/Core/Enums/GalaxyType.h"
#include "APS_ALPHA/Core/Enums/OrbitDistributionType.h"
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
#include "APS_ALPHA/Actors/Astro/Star.h"
#include "APS_ALPHA/UI/MainMenu/WorldGenerationViewModel.h"
#include "Engine/Font.h"
#include "Engine/GameViewportClient.h"
#include "Engine/Engine.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/PlayerController.h"
#include "InputCoreTypes.h"
#include "Math/RotationMatrix.h"
#include "Rendering/DrawElements.h"
#include "Styling/AppStyle.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SSlider.h"
#include "Widgets/Input/SSpinBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
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
		SSlider::Construct(SSlider::FArguments()
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
					FLinearColor(DrawColor.R, DrawColor.G, DrawColor.B, 0.22f), true, 1.0f);
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
			const float Cut = FMath::Clamp(FMath::Min(Size.X, Size.Y) * 0.025f, 9.0f, 18.0f);
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

	// The astronomical preview is the real level viewport. Keep the root chrome
	// fully transparent so no Slate wash/vignette changes the scene exposure or
	// hides small galaxy/cluster instances in the middle of the screen.
	const FLinearColor Background(0.0f, 0.0f, 0.0f, 0.0f);
	const FLinearColor Panel(0.002f, 0.014f, 0.024f, 0.93f);
	const FLinearColor Cyan(0.12f, 0.82f, 1.0f, 1.0f);
	const FLinearColor CyanDim(0.035f, 0.23f, 0.32f, 1.0f);
	const FLinearColor Amber(1.0f, 0.56f, 0.04f, 1.0f);
	const FLinearColor White(0.92f, 0.97f, 1.0f, 1.0f);
	const FLinearColor SecondaryText(0.58f, 0.72f, 0.78f, 1.0f);
	TWeakObjectPtr<UFont> DisplayFont;
	TWeakObjectPtr<UFont> BodyFont;
	const FSlateRoundedBoxBrush PanelBrush(Panel, 10.0f, CyanDim, 1.0f);
	const FSlateRoundedBoxBrush ControlBrush(FLinearColor(0.003f, 0.016f, 0.028f, 0.94f), 6.0f, CyanDim, 1.0f);
	const FSlateRoundedBoxBrush BadgeBrush(FLinearColor(0.005f, 0.045f, 0.070f, 0.98f), 16.0f, Cyan, 1.0f);
	const FLinearColor HierarchyRowFill(0.003f, 0.022f, 0.038f, 0.94f);
	const FLinearColor HierarchyHoverFill(0.010f, 0.075f, 0.105f, 0.98f);
	const FLinearColor HierarchyPressedFill(0.015f, 0.115f, 0.155f, 1.0f);
	const FLinearColor SelectedFill(Amber.R, Amber.G, Amber.B, 0.12f);

	FSlateFontInfo Font(const FName Typeface, int32 Size)
	{
		if (!DisplayFont.IsValid())
		{
			DisplayFont = LoadObject<UFont>(nullptr, TEXT("/Game/APS/APS_ALPHA/UI/Fonts/Orbitron_Bold_Font.Orbitron_Bold_Font"));
			BodyFont = LoadObject<UFont>(nullptr, TEXT("/Game/APS/APS_ALPHA/UI/Fonts/Orbitron_Medium_Font.Orbitron_Medium_Font"));
		}
		if (UFont* FontObject = (Typeface == TEXT("Bold") ? DisplayFont.Get() : BodyFont.Get()))
		{
			return FSlateFontInfo(FontObject, Size, Typeface);
		}
		return FCoreStyle::GetDefaultFontStyle(Typeface, Size);
	}

	FSlateFontInfo ReadableFont(const FName Typeface, int32 Size)
	{
		// Reserve the display face for branding and compact technical headings.
		// Long descriptions, generated names and metadata need a neutral UI face.
		return FCoreStyle::GetDefaultFontStyle(Typeface, Size);
	}

	FText CompactLabel(const FText& Text, int32 MaxCharacters)
	{
		const FString Source = Text.ToString();
		if (Source.Len() <= MaxCharacters || MaxCharacters < 4)
		{
			return Text;
		}
		return FText::FromString(Source.Left(MaxCharacters - 3).TrimEnd() + TEXT("..."));
	}

	FButtonStyle MakeButtonStyle(const FLinearColor& Outline, const FLinearColor& Fill,
		const FLinearColor& ActiveOutline, const FLinearColor& PressedFill)
	{
		return FButtonStyle()
			.SetNormal(FSlateRoundedBoxBrush(Fill, 6.0f, Outline, 1.0f))
			.SetHovered(FSlateRoundedBoxBrush(FLinearColor(Fill.R + 0.025f, Fill.G + 0.05f, Fill.B + 0.07f, 0.98f), 6.0f, ActiveOutline, 1.5f))
			.SetPressed(FSlateRoundedBoxBrush(PressedFill, 6.0f, ActiveOutline, 1.5f));
	}

	const FButtonStyle SecondaryButton = MakeButtonStyle(
		CyanDim, FLinearColor(0.003f, 0.022f, 0.038f, 0.94f), Cyan,
		FLinearColor(0.02f, 0.14f, 0.20f, 1.0f));
	const FButtonStyle PrimaryButton = MakeButtonStyle(
		Amber, FLinearColor(0.30f, 0.12f, 0.004f, 0.96f),
		FLinearColor(1.0f, 0.76f, 0.18f, 1.0f), FLinearColor(0.52f, 0.22f, 0.006f, 1.0f));
	const FButtonStyle HierarchyButton = FButtonStyle()
		.SetNormal(FSlateRoundedBoxBrush(HierarchyRowFill, 5.0f, CyanDim, 1.0f))
		.SetHovered(FSlateRoundedBoxBrush(HierarchyHoverFill, 5.0f, Cyan, 1.25f))
		.SetPressed(FSlateRoundedBoxBrush(HierarchyPressedFill, 5.0f, Cyan, 1.5f))
		.SetDisabled(FSlateRoundedBoxBrush(
			FLinearColor(HierarchyRowFill.R, HierarchyRowFill.G, HierarchyRowFill.B, 0.55f),
			5.0f, FLinearColor(CyanDim.R, CyanDim.G, CyanDim.B, 0.55f), 1.0f));

	TSharedRef<SWidget> ChamferPanel(TSharedRef<SWidget> Content)
	{
		return SNew(SOverlay)
			+ SOverlay::Slot()
			[
				SNew(SBorder).BorderImage(&PanelBrush).Padding(16.0f)[Content]
			]
			+ SOverlay::Slot()
			[
				SNew(SGenerationChamferedFrame).Color(Cyan).Thickness(1.15f)
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
			Values.Add(Enum->GetValueByIndex(Index));
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
			return {EPlanetType::Rocky, EPlanetType::Dwarf};
		case static_cast<int32>(EAPSPlanetSurfaceArchetype::Temperate):
			return {EPlanetType::Terrestrial, EPlanetType::Pangea, EPlanetType::Nordic,
				EPlanetType::SuperEarth, EPlanetType::HighMountain};
		case static_cast<int32>(EAPSPlanetSurfaceArchetype::Oceanic):
			return {EPlanetType::Ocean, EPlanetType::Water, EPlanetType::Archipelago};
		case static_cast<int32>(EAPSPlanetSurfaceArchetype::Biosphere):
			return {EPlanetType::Forest, EPlanetType::Oasis};
		case static_cast<int32>(EAPSPlanetSurfaceArchetype::Desert):
			return {EPlanetType::Greenhouse, EPlanetType::Desert, EPlanetType::Sand};
		case static_cast<int32>(EAPSPlanetSurfaceArchetype::Cryogenic):
			return {EPlanetType::Ice, EPlanetType::Frozen, EPlanetType::Tundra, EPlanetType::Rogue};
		case static_cast<int32>(EAPSPlanetSurfaceArchetype::Magmatic):
			return {EPlanetType::Volcanic, EPlanetType::Melted, EPlanetType::Lava};
		case static_cast<int32>(EAPSPlanetSurfaceArchetype::Metallic):
			return {EPlanetType::Metal, EPlanetType::Metallic, EPlanetType::Carbon};
		case static_cast<int32>(EAPSPlanetSurfaceArchetype::ExoticChemical):
			return {EPlanetType::Ammonia, EPlanetType::Exoplanet};
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
						SNew(SBox).MinDesiredWidth(36.0f).MinDesiredHeight(36.0f)
						[
							SNew(SButton).Text(FText::FromString(TEXT("<")))
							.ContentPadding(FMargin(9.0f, 4.0f)).ButtonStyle(&SecondaryButton)
							.OnClicked_Lambda([ViewModel, Stepper]()
							{
								if (UWorldGenerationViewModel* VM = ViewModel.Get()) Stepper(VM, -1);
								return FReply::Handled();
							})
						]
					]
					+ SHorizontalBox::Slot().FillWidth(1.0f).HAlign(HAlign_Center).VAlign(VAlign_Center)
					[
						SNew(STextBlock).Text_Lambda(ValueText).ToolTipText_Lambda(ValueText)
						.OverflowPolicy(ETextOverflowPolicy::Ellipsis)
						.Font(Font("Bold", 11)).ColorAndOpacity(White)
					]
					+ SHorizontalBox::Slot().AutoWidth()
					[
						SNew(SBox).MinDesiredWidth(36.0f).MinDesiredHeight(36.0f)
						[
							SNew(SButton).Text(FText::FromString(TEXT(">")))
							.ContentPadding(FMargin(9.0f, 4.0f)).ButtonStyle(&SecondaryButton)
							.OnClicked_Lambda([ViewModel, Stepper]()
							{
								if (UWorldGenerationViewModel* VM = ViewModel.Get()) Stepper(VM, 1);
								return FReply::Handled();
							})
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
				[SNew(SBorder).BorderImage(&BadgeBrush).Padding(0.0f)[SNew(STextBlock).Text(Glyph).Justification(ETextJustify::Center).Font(Font("Bold", 8)).ColorAndOpacity(Cyan)]]
			]
			+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center).Padding(10.0f, 0.0f)
			[SNew(STextBlock).Text(Text).Font(Font("Bold", 16)).ColorAndOpacity(Cyan)]
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 8.0f, 0.0f, 0.0f)
		[SNew(SBox).HeightOverride(1.0f)[SNew(SBorder).BorderImage(FAppStyle::GetBrush("WhiteBrush")).BorderBackgroundColor(CyanDim)]];
	}

	template <typename TEnum, typename TGetter>
	TSharedRef<SWidget> EnumRow(const FText& Label, TWeakObjectPtr<UWorldGenerationViewModel> ViewModel, TGetter Getter)
	{
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
					SNew(SBox).MinDesiredWidth(36.0f).MinDesiredHeight(36.0f)
					[
						SNew(SButton).Text(FText::FromString(TEXT("<"))).ContentPadding(FMargin(9.0f, 4.0f))
						.ButtonStyle(&SecondaryButton)
						.OnClicked_Lambda([ViewModel, Getter, Enum]()
						{
							if (UWorldGenerationViewModel* VM = ViewModel.Get(); VM && VM->GeneratedWorld && Enum)
							{
								const TArray<int64> Values = GetSelectableEnumValues(Enum);
								if (!Values.IsEmpty())
								{
									const int64 Current = static_cast<int64>(Getter(VM->GeneratedWorld));
									const int32 CurrentIndex = Values.IndexOfByKey(Current);
									const int32 NewIndex = CurrentIndex == INDEX_NONE
										? Values.Num() - 1 : (CurrentIndex - 1 + Values.Num()) % Values.Num();
									VM->SetEnumValue(Enum, static_cast<int32>(Values[NewIndex]));
								}
							}
							return FReply::Handled();
						})
					]
				]
				+ SHorizontalBox::Slot().FillWidth(1.0f).HAlign(HAlign_Center).VAlign(VAlign_Center)
				[
					SNew(STextBlock)
					.Text_Lambda(ValueText).ToolTipText_Lambda(ValueText)
					.OverflowPolicy(ETextOverflowPolicy::Ellipsis)
					.Font(Font("Bold", 11)).ColorAndOpacity(White)
				]
				+ SHorizontalBox::Slot().AutoWidth()
				[
					SNew(SBox).MinDesiredWidth(36.0f).MinDesiredHeight(36.0f)
					[
						SNew(SButton).Text(FText::FromString(TEXT(">"))).ContentPadding(FMargin(9.0f, 4.0f))
						.ButtonStyle(&SecondaryButton)
						.OnClicked_Lambda([ViewModel, Getter, Enum]()
						{
							if (UWorldGenerationViewModel* VM = ViewModel.Get(); VM && VM->GeneratedWorld && Enum)
							{
								const TArray<int64> Values = GetSelectableEnumValues(Enum);
								if (!Values.IsEmpty())
								{
									const int64 Current = static_cast<int64>(Getter(VM->GeneratedWorld));
									const int32 CurrentIndex = Values.IndexOfByKey(Current);
									const int32 NewIndex = CurrentIndex == INDEX_NONE
										? 0 : (CurrentIndex + 1) % Values.Num();
									VM->SetEnumValue(Enum, static_cast<int32>(Values[NewIndex]));
								}
							}
							return FReply::Handled();
						})
					]
				]
				]
			];
	}

	template <typename TValue, typename TGetter, typename TSetter>
	TSharedRef<SWidget> NumberRow(const FText& Label, TValue Min, TValue Max, TValue Delta,
		TWeakObjectPtr<UWorldGenerationViewModel> ViewModel, TGetter Getter, TSetter Setter,
		TSharedPtr<SAPSGenerationRangeSlider>* OutSlider = nullptr)
	{
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
					SAssignNew(Slider, SAPSGenerationRangeSlider)
					.MinValue(static_cast<float>(Min))
					.MaxValue(static_cast<float>(Max))
					.StepSize(static_cast<float>(Delta))
					.SliderBarColor(FSlateColor(CyanDim))
					.SliderHandleColor(FSlateColor(Cyan))
					.Value_Lambda([ViewModel, Getter]()
					{
						if (const UWorldGenerationViewModel* VM = ViewModel.Get(); VM && VM->GeneratedWorld)
						{
							return static_cast<float>(Getter(VM->GeneratedWorld));
						}
						return 0.0f;
					})
					.OnValueChanged_Lambda([ViewModel, Setter](const float Value)
					{
						if (UWorldGenerationViewModel* VM = ViewModel.Get())
						{
							if constexpr (TIsIntegral<TValue>::Value)
							{
								Setter(VM, static_cast<TValue>(FMath::RoundToInt(Value)));
							}
							else
							{
								Setter(VM, static_cast<TValue>(Value));
							}
						}
					})
				]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(7.0f, 0.0f, 0.0f, 0.0f)
				[
					SNew(SBox).WidthOverride(104.0f)
					[
						SNew(SSpinBox<TValue>)
						.MinValue(Min).MaxValue(Max)
						.Delta(Delta).EnableSlider(false).MinDesiredWidth(96.0f)
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
		}

		virtual FVector2D ComputeDesiredSize(float) const override
		{
			return FVector2D(100.0f, 100.0f);
		}

		virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry,
			const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements,
			int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override
		{
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
			const FGeometry ViewportGeometry = ViewportWidget->GetCachedGeometry();
			const FVector2D ViewportLocalSize = ViewportGeometry.GetLocalSize();
			const FVector2D PanelSize = AllottedGeometry.GetLocalSize();

			const auto ProjectToPanel = [&](const FVector& WorldPosition, FVector2D& OutPanelPosition)
			{
				FVector2D ScreenPosition;
				if (!PC->ProjectWorldLocationToScreen(WorldPosition, ScreenPosition, true))
				{
					return false;
				}
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
							FSlateDrawElement::MakeLines(OutDrawElements, LayerId,
								AllottedGeometry.ToPaintGeometry(), Segment, ESlateDrawEffect::None,
								ScopeColor, true, 0.85f);
						}
						Segment.Reset();
					}
				}
				if (Segment.Num() > 1)
				{
					FSlateDrawElement::MakeLines(OutDrawElements, LayerId,
						AllottedGeometry.ToPaintGeometry(), Segment, ESlateDrawEffect::None,
						ScopeColor, true, 0.85f);
				}
			};

			if (Focus == EAstroPreviewFocus::Galaxy)
			{
				DrawScopeRing(Focus, 1.0, FLinearColor(0.52f, 0.32f, 1.0f, 0.34f));
			}
			else if (Focus == EAstroPreviewFocus::StarCluster)
			{
				DrawScopeRing(Focus, 1.0, FLinearColor(Cyan.R, Cyan.G, Cyan.B, 0.28f));
			}
			AActor* SelectedBody = VM->GetSelectedPreviewBody();
			if (CachedPreviewRevision != VM->PreviewRevision
				|| CachedPreviewFocus != Focus
				|| CachedSelectedBody.Get() != SelectedBody)
			{
				CachedEntries.Reset();
				VM->GetPreviewBodyEntries(CachedEntries);
				for (FAPSPreviewBodyEntry& Entry : CachedEntries)
				{
					Entry.Label = CompactLabel(Entry.Label, 23);
					Entry.Details = CompactLabel(Entry.Details, 29);
				}
				CachedPreviewRevision = VM->PreviewRevision;
				CachedPreviewFocus = Focus;
				CachedSelectedBody = SelectedBody;
			}
			const TArray<FAPSPreviewBodyEntry>& Entries = CachedEntries;
			if (Entries.IsEmpty())
			{
				return LayerId;
			}

			// World-space orbit planes are sampled and projected every paint. They remain
			// locked to the generated actors while the preview camera orbits and zooms.
			// Keep curves visibly round at 1440p while retaining a strict per-orbit cap.
			// Even a twenty-planet SYSTEM stays below 1,300 world projections per paint.
			const TArray<FVector2D>& OrbitCircle = Focus == EAstroPreviewFocus::HomeSystem
				? UnitCircleSamples<64>() : UnitCircleSamples<80>();
			TArray<FVector2D> OrbitSegment;
			OrbitSegment.Reserve(OrbitCircle.Num());
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
				OrbitSegment.Reset();
				for (const FVector2D& UnitPoint : OrbitCircle)
				{
					FVector2D Point;
					if (ProjectToPanel(Center + (AxisX * UnitPoint.X + AxisY * UnitPoint.Y) * Radius, Point)
						&& Point.X > -PanelSize.X && Point.X < PanelSize.X * 2.0f
						&& Point.Y > -PanelSize.Y && Point.Y < PanelSize.Y * 2.0f)
					{
						OrbitSegment.Add(Point);
					}
					else if (OrbitSegment.Num() > 1)
					{
						FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 1,
							AllottedGeometry.ToPaintGeometry(), OrbitSegment, ESlateDrawEffect::None,
							Entry.Depth > 1 ? FLinearColor(0.36f, 0.65f, 1.0f, 0.22f)
								: FLinearColor(Cyan.R, Cyan.G, Cyan.B, 0.30f), true, Entry.Depth > 1 ? 0.65f : 1.0f);
						OrbitSegment.Reset();
					}
				}
				if (OrbitSegment.Num() > 1)
				{
					FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 1,
						AllottedGeometry.ToPaintGeometry(), OrbitSegment, ESlateDrawEffect::None,
						Entry.Depth > 1 ? FLinearColor(0.36f, 0.65f, 1.0f, 0.22f)
							: FLinearColor(Cyan.R, Cyan.G, Cyan.B, 0.30f), true, Entry.Depth > 1 ? 0.65f : 1.0f);
				}
			}

			TArray<FSlateRect> OccupiedLabels;
			const FVector2D LabelSize(160.0f, 36.0f);
			FVector ViewLocation = FVector::ZeroVector;
			FRotator ViewRotation = FRotator::ZeroRotator;
			PC->GetPlayerViewPoint(ViewLocation, ViewRotation);
			const FQuat ViewQuaternion = ViewRotation.Quaternion();
			const FVector CameraRight = ViewQuaternion.GetRightVector();
			const FVector CameraUp = ViewQuaternion.GetUpVector();
			const auto OverlapsExistingLabel = [&OccupiedLabels](const FSlateRect& Candidate)
			{
				for (const FSlateRect& Existing : OccupiedLabels)
				{
					if (FSlateRect::DoRectanglesIntersect(Candidate, Existing))
					{
						return true;
					}
				}
				return false;
			};

			// The overflow rack is a finite screen-space grid. Enumerating columns
			// left/right toward the centre is deterministic, checks labels already
			// placed near bodies, and never wraps onto an occupied slot.
			constexpr float RackGap = 3.0f;
			const int32 RackRows = FMath::Max(1,
				FMath::FloorToInt((PanelSize.Y - 8.0f + RackGap) / (LabelSize.Y + RackGap)));
			const int32 RackColumns = FMath::Max(1,
				FMath::FloorToInt((PanelSize.X - 8.0f + RackGap) / (LabelSize.X + RackGap)));
			const int32 RackCapacity = RackRows * RackColumns;
			int32 NextRackSlot = 0;
			const auto TryPlaceInRack = [&](FVector2D& OutPosition)
			{
				for (int32 Offset = 0; Offset < RackCapacity; ++Offset)
				{
					const int32 Slot = (NextRackSlot + Offset) % RackCapacity;
					const int32 SequenceColumn = Slot / RackRows;
					const int32 Column = SequenceColumn % 2 == 0
						? SequenceColumn / 2
						: RackColumns - 1 - SequenceColumn / 2;
					if (Column < 0 || Column >= RackColumns)
					{
						continue;
					}
					const int32 Row = Slot % RackRows;
					const FVector2D CandidatePosition(
						4.0f + Column * (LabelSize.X + RackGap),
						4.0f + Row * (LabelSize.Y + RackGap));
					const FSlateRect Candidate(CandidatePosition.X, CandidatePosition.Y,
						CandidatePosition.X + LabelSize.X, CandidatePosition.Y + LabelSize.Y);
					if (!OverlapsExistingLabel(Candidate))
					{
						OutPosition = CandidatePosition;
						OccupiedLabels.Add(Candidate);
						NextRackSlot = (Slot + 1) % RackCapacity;
						return true;
					}
				}
				return false;
			};
			for (const FAPSPreviewBodyEntry& Entry : Entries)
			{
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
				const FVector BodyCenter = Entry.bHasExplicitWorldAnchor
					? Entry.ExplicitWorldAnchor : PresentationLocation(Body);
				const bool bProjected = ProjectToPanel(BodyCenter, Anchor);
				const bool bAnchorInside = bProjected
					&& Anchor.X >= 0.0f && Anchor.X <= PanelSize.X
					&& Anchor.Y >= 0.0f && Anchor.Y <= PanelSize.Y;
				if (!bAnchorInside)
				{
					// Object inspection stays intentionally local. SYSTEM, however, promises
					// one marker per planet, so an off-screen/behind-camera body enters the
					// rack with a leader anchored to the nearest panel edge.
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

				// Screen-space body discs provide a stable and cheap occlusion test.
				// This avoids visibility traces for every label on every Slate paint,
				// while still hiding a moon/planet marker behind a nearer celestial body.
				bool bOccluded = false;
				if (bAnchorInside && Focus != EAstroPreviewFocus::HomeSystem)
				{
					const double TargetDistance = FVector::Distance(ViewLocation, BodyCenter);
					for (const FAPSPreviewBodyEntry& OccluderEntry : Entries)
					{
						const AActor* Occluder = OccluderEntry.Actor.Get();
						const FVector OccluderPresentationCenter = PresentationLocation(Occluder);
						if (!Occluder || Occluder == Body
							|| FVector::Distance(ViewLocation, OccluderPresentationCenter) >= TargetDistance)
						{
							continue;
						}
						// Actor bounds also include physical gravity/safe-zone components at the
						// invariant data address. In PLANET a moon mesh can be presentation-spread,
						// so build the screen disc from rendered meshes at the published centre.
						const FVector OccluderCenter = OccluderPresentationCenter;
						double OccluderRadius = 0.0;
						TInlineComponentArray<UStaticMeshComponent*> OccluderMeshes;
						Occluder->GetComponents(OccluderMeshes);
						for (UStaticMeshComponent* OccluderMesh : OccluderMeshes)
						{
							if (!IsValid(OccluderMesh)) continue;
							OccluderMesh->UpdateBounds();
							OccluderRadius = FMath::Max(OccluderRadius,
								static_cast<double>(OccluderMesh->Bounds.SphereRadius));
						}
						FVector2D OccluderScreen;
						FVector2D OccluderEdgeScreen;
						if (OccluderRadius > UE_SMALL_NUMBER
							&& ProjectToPanel(OccluderCenter, OccluderScreen)
							&& ProjectToPanel(OccluderCenter + CameraRight * OccluderRadius, OccluderEdgeScreen))
						{
							const double RadiusPixels = FVector2D::Distance(OccluderScreen, OccluderEdgeScreen);
							if (RadiusPixels > 2.0
								&& FVector2D::Distance(Anchor, OccluderScreen) < RadiusPixels * 0.86)
							{
								bOccluded = true;
								break;
							}
						}
					}
				}
				if (bOccluded)
				{
					continue;
				}

				FVector2D LabelPosition(
					FMath::Clamp(Anchor.X - LabelSize.X * 0.5f, 4.0f, FMath::Max(4.0f, PanelSize.X - LabelSize.X - 4.0f)),
					FMath::Clamp(Anchor.Y - LabelSize.Y - 34.0f, 4.0f, FMath::Max(4.0f, PanelSize.Y - LabelSize.Y - 4.0f)));
				bool bPlacedLabel = false;
				const float OriginalLabelY = LabelPosition.Y;
				for (int32 Attempt = 0; bAnchorInside && Attempt < 13; ++Attempt)
				{
					if (Attempt > 0)
					{
						const int32 Lane = (Attempt + 1) / 2;
						const float Direction = Attempt % 2 == 1 ? 1.0f : -1.0f;
						LabelPosition.Y = FMath::Clamp(
							OriginalLabelY + Direction * Lane * (LabelSize.Y + 5.0f),
							4.0f, FMath::Max(4.0f, PanelSize.Y - LabelSize.Y - 4.0f));
					}
					const FSlateRect Candidate(LabelPosition.X, LabelPosition.Y,
						LabelPosition.X + LabelSize.X, LabelPosition.Y + LabelSize.Y);
					if (!OverlapsExistingLabel(Candidate))
					{
						OccupiedLabels.Add(Candidate);
						bPlacedLabel = true;
						break;
					}
				}
				if (!bPlacedLabel)
				{
					// If every finite slot is occupied, stop at the deterministic screen
					// capacity instead of wrapping and drawing labels over one another.
					if (!TryPlaceInRack(LabelPosition))
					{
						continue;
					}
				}

				const bool bSelected = Body && SelectedBody == Body;
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
				if (bSelected)
				{
					MarkerColor = Amber;
				}
				const FVector2D PoleEnd(LabelPosition.X + LabelSize.X * 0.5f, LabelPosition.Y + LabelSize.Y);
				FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 2, AllottedGeometry.ToPaintGeometry(),
					TArray<FVector2D>{Anchor, PoleEnd}, ESlateDrawEffect::None,
					FLinearColor(MarkerColor.R, MarkerColor.G, MarkerColor.B, 0.72f), true, 1.0f);
				FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 3, AllottedGeometry.ToPaintGeometry(),
					TArray<FVector2D>{Anchor - FVector2D(4.0f, 0.0f), Anchor + FVector2D(4.0f, 0.0f)},
					ESlateDrawEffect::None, MarkerColor, true, 1.0f);
				FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 3, AllottedGeometry.ToPaintGeometry(),
					TArray<FVector2D>{Anchor - FVector2D(0.0f, 4.0f), Anchor + FVector2D(0.0f, 4.0f)},
					ESlateDrawEffect::None, MarkerColor, true, 1.0f);
				FSlateDrawElement::MakeBox(OutDrawElements, LayerId + 3,
					AllottedGeometry.ToPaintGeometry(LabelSize, FSlateLayoutTransform(LabelPosition)),
					FAppStyle::GetBrush("WhiteBrush"), ESlateDrawEffect::None,
					FLinearColor(0.002f, 0.014f, 0.026f, 0.96f));
				FSlateDrawElement::MakeBox(OutDrawElements, LayerId + 4,
					AllottedGeometry.ToPaintGeometry(FVector2D(3.0f, LabelSize.Y), FSlateLayoutTransform(LabelPosition)),
					FAppStyle::GetBrush("WhiteBrush"), ESlateDrawEffect::None, MarkerColor);
				if (bSelected)
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
				FSlateDrawElement::MakeText(OutDrawElements, LayerId + 5,
					AllottedGeometry.ToPaintGeometry(FVector2D(LabelSize.X - 12.0f, 16.0f),
						FSlateLayoutTransform(LabelPosition + FVector2D(7.0f, 4.0f))),
					Entry.Label, ReadableFont("Bold", 10),
					ESlateDrawEffect::None, bSelected ? Amber : White);
				FSlateDrawElement::MakeText(OutDrawElements, LayerId + 5,
					AllottedGeometry.ToPaintGeometry(FVector2D(LabelSize.X - 12.0f, 12.0f),
						FSlateLayoutTransform(LabelPosition + FVector2D(7.0f, 21.0f))),
					Entry.Details, ReadableFont("Regular", 8),
					ESlateDrawEffect::None, SecondaryText);
			}

			return LayerId + 5;
		}

	private:
		TWeakObjectPtr<UWorldGenerationViewModel> ViewModel;
		mutable TArray<FAPSPreviewBodyEntry> CachedEntries;
		mutable TWeakObjectPtr<AActor> CachedSelectedBody;
		mutable int32 CachedPreviewRevision{MIN_int32};
		mutable EAstroPreviewFocus CachedPreviewFocus{EAstroPreviewFocus::Overview};
	};
}

void SWorldGenerationPanel::Construct(const FArguments& InArgs)
{
	ViewModel = InArgs._ViewModel;
	OnBack = InArgs._OnBack;
	OnContinue = InArgs._OnContinue;

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
				.ButtonColorAndOpacity_Lambda([VM, Getter]()
				{
					return VM.IsValid() && VM->GeneratedWorld && Getter(VM->GeneratedWorld)
						? FLinearColor(0.0f, 0.52f, 0.68f, 1.0f) : FLinearColor(0.18f, 0.23f, 0.27f, 1.0f);
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
					.Font(Font("Bold", 10)).ColorAndOpacity(Cyan)
				]
			]
		];
	};

	const TSharedRef<SWidget> OverviewControls = SNew(SScrollBox) + SScrollBox::Slot()
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 12.0f)[SectionTitle(LOCTEXT("Astro", "ASTRO GENERATION"))]
		+ SVerticalBox::Slot().AutoHeight()[EnumRow<EAstroGenerationLevel>(LOCTEXT("Level", "GENERATION LEVEL"), VM, [](const UGeneratedWorld* W){ return W->AstroGenerationLevel; })]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 5.0f)[BoolRow(LOCTEXT("FullScale", "FULL-SCALE WORLD"), [](const UGeneratedWorld* W){return W->bGenerateFullScaledWorld;}, [](UGeneratedWorld* W, bool V){W->bGenerateFullScaledWorld=V;})]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 5.0f)[BoolRow(LOCTEXT("HomeSystemEnabled", "GENERATE HOME SYSTEM"), [](const UGeneratedWorld* W){return W->bGenerateHomeSystem;}, [](UGeneratedWorld* W, bool V){W->bGenerateHomeSystem=V;})]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 5.0f)[BoolRow(LOCTEXT("StartWithHomePlanet", "START WITH HOME PLANET"), [](const UGeneratedWorld* W){return W->bStartWithHomePlanet;}, [](UGeneratedWorld* W, bool V){W->bStartWithHomePlanet=V;})]
	];

	const TSharedRef<SWidget> GalaxyControls = SNew(SScrollBox) + SScrollBox::Slot()
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 12.0f)[SectionTitle(LOCTEXT("Galaxy", "GALAXY"))]
		+ SVerticalBox::Slot().AutoHeight()[EnumRow<EGalaxyType>(LOCTEXT("GalaxyType", "TYPE / PRESET"), VM, [](const UGeneratedWorld* W){ return W->GalaxyType; })]
		+ SVerticalBox::Slot().AutoHeight()[EnumRow<EGalaxyClass>(LOCTEXT("GalaxyClass", "CLASS"), VM, [](const UGeneratedWorld* W){ return W->GalaxyClass; })]
		+ SVerticalBox::Slot().AutoHeight()[NumberRow<int32>(LOCTEXT("GalaxySize", "SIZE"), 1, 100000, 1, VM, [](const UGeneratedWorld* W){ return W->GalaxySize; }, [](UWorldGenerationViewModel* V, int32 X){ V->SetGalaxySize(X); })]
		+ SVerticalBox::Slot().AutoHeight()[NumberRow<int32>(LOCTEXT("StarCount", "MODELED STAR COUNT"), 1, 1000000000, 100000, VM, [](const UGeneratedWorld* W){ return W->GalaxyStarCount; }, [](UWorldGenerationViewModel* V, int32 X){ V->SetGalaxyStarCount(X); })]
		+ SVerticalBox::Slot().AutoHeight()[NumberRow<double>(LOCTEXT("Density", "STAR DENSITY"), 0.01, 1000.0, 0.1, VM, [](const UGeneratedWorld* W){ return W->GalaxyStarDensity; }, [](UWorldGenerationViewModel* V, double X){ V->SetGalaxyStarDensity(X); })]
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
		+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 12.0f)[SectionTitle(LOCTEXT("HomeStarSystem", "STAR SYSTEM"))]
		+ SVerticalBox::Slot().AutoHeight()[EnumRow<EStarType>(LOCTEXT("StarType", "STAR SYSTEM TYPE"), VM, [](const UGeneratedWorld* W){ return W->StarType; })]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 12.0f, 0.0f, 10.0f)[SectionTitle(LOCTEXT("SystemLayout", "SYSTEM LAYOUT"))]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 5.0f)[BoolRow(LOCTEXT("RandomHomeSystem", "RANDOM HOME SYSTEM"), [](const UGeneratedWorld* W){return W->bRandomHomeSystem;}, [](UGeneratedWorld* W, bool V){W->bRandomHomeSystem=V;})]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 5.0f)[BoolRow(LOCTEXT("RandomSystemType", "RANDOM SYSTEM TYPE"), [](const UGeneratedWorld* W){return W->bRandomHomeSystemType;}, [](UGeneratedWorld* W, bool V){W->bRandomHomeSystemType=V;})]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 5.0f)[BoolRow(LOCTEXT("RandomHomeStar", "RANDOM HOME STAR"), [](const UGeneratedWorld* W){return W->bRandomHomeStar;}, [](UGeneratedWorld* W, bool V){W->bRandomHomeStar=V;})]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 5.0f)[BoolRow(LOCTEXT("RandomStartPlanet", "RANDOM START PLANET"), [](const UGeneratedWorld* W){return W->bRandomStartPlanetNumber;}, [](UGeneratedWorld* W, bool V){W->bRandomStartPlanetNumber=V;})]
		+ SVerticalBox::Slot().AutoHeight()[EnumRow<EPlanetarySystemType>(LOCTEXT("SystemType", "PLANETARY TYPE"), VM, [](const UGeneratedWorld* W){ return W->PlanetarySystemType; })]
		+ SVerticalBox::Slot().AutoHeight()[EnumRow<EOrbitDistributionType>(LOCTEXT("Distribution", "ORBIT DISTRIBUTION"), VM, [](const UGeneratedWorld* W){ return W->OrbitDistributionType; })]
		+ SVerticalBox::Slot().AutoHeight()[NumberRow<int32>(LOCTEXT("Planets", "PLANETS / STAR"), 1, 20, 1, VM, [](const UGeneratedWorld* W){ return FMath::Clamp(W->PlanetsAmount, 1, 20); }, [](UWorldGenerationViewModel* V, int32 X){ V->SetPlanetsAmount(X); })]
		+ SVerticalBox::Slot().AutoHeight()[NumberRow<int32>(LOCTEXT("Moons", "HOME PLANET MOONS"), 0, 10, 1, VM, [](const UGeneratedWorld* W){ return FMath::Clamp(W->MoonsAmount, 0, 10); }, [](UWorldGenerationViewModel* V, int32 X){ V->SetMoonsAmount(X); })]
		+ SVerticalBox::Slot().AutoHeight()[NumberRow<int32>(LOCTEXT("StartIndex", "START PLANET INDEX"), 1, 20, 1, VM, [](const UGeneratedWorld* W){ return FMath::Clamp(W->StartPlanetIndex, 1, FMath::Max(1, W->PlanetsAmount)); }, [](UWorldGenerationViewModel* V, int32 X){ V->SetStartPlanetIndex(X); })]
	];

	const TSharedRef<SWidget> StarControls = SNew(SScrollBox) + SScrollBox::Slot()
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 12.0f)[SectionTitle(LOCTEXT("SelectedStar", "SELECTED STAR"))]
		+ SVerticalBox::Slot().AutoHeight()[EnumRow<EStellarType>(LOCTEXT("StellarType", "STELLAR TYPE"), VM, [](const UGeneratedWorld* W){ return W->StellarType; })]
		+ SVerticalBox::Slot().AutoHeight()[EnumRow<ESpectralClass>(LOCTEXT("Spectral", "SPECTRAL CLASS"), VM, [](const UGeneratedWorld* W){ return W->SpectralClass; })]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 14.0f, 0.0f, 0.0f)
		[
			SNew(STextBlock)
			.Text(LOCTEXT("StarScopeHint", "STELLAR CHANGES UPDATE THIS STAR AND ITS SAFE ORBIT CLEARANCE WITHOUT REGENERATING THE PARENT SYSTEM."))
			.AutoWrapText(true).Font(ReadableFont("Regular", 10)).ColorAndOpacity(SecondaryText)
		]
	];

	const TSharedRef<SWidget> PlanetControls = SNew(SScrollBox) + SScrollBox::Slot()
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 12.0f)[SectionTitle(LOCTEXT("Planet", "SELECTED PLANET"))]
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
					constexpr int32 FamilyCount = GasGiantSurfaceFamilyIndex + 1;
					const int32 Current = GetSurfaceFamilyIndex(V->GeneratedWorld->PlanetType);
					const int32 Next = (Current + (Direction < 0 ? -1 : 1) + FamilyCount) % FamilyCount;
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
					const TArray<EPlanetType> Presets = GetSurfacePresetsForFamily(
						GetSurfaceFamilyIndex(V->GeneratedWorld->PlanetType));
					if (Presets.IsEmpty()) return;
					const int32 Current = Presets.IndexOfByKey(V->GeneratedWorld->PlanetType);
					const int32 Base = Current == INDEX_NONE ? 0 : Current;
					const int32 Next = (Base + (Direction < 0 ? -1 : 1) + Presets.Num()) % Presets.Num();
					V->SetEnumValue(StaticEnum<EPlanetType>(), static_cast<int32>(Presets[Next]));
				})
		]
		+ SVerticalBox::Slot().AutoHeight()[NumberRow<double>(LOCTEXT("Radius", "PLANET RADIUS / KM"), 100.0, 20000.0, 100.0, VM, [](const UGeneratedWorld* W){ return FMath::Clamp(W->PlanetRadius, 100.0, 20000.0); }, [](UWorldGenerationViewModel* V, double X){ V->SetPlanetRadius(X); })]
		+ SVerticalBox::Slot().AutoHeight()[NumberRow<int32>(LOCTEXT("PlanetMoons", "MOONS AMOUNT"), 0, 10, 1, VM, [](const UGeneratedWorld* W){ return FMath::Clamp(W->MoonsAmount, 0, 10); }, [](UWorldGenerationViewModel* V, int32 X){ V->SetMoonsAmount(X); })]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 12.0f, 0.0f, 10.0f)[SectionTitle(LOCTEXT("PlanetSurface", "PLANET SURFACE"))]
		+ SVerticalBox::Slot().AutoHeight()
		[
			SNew(SVerticalBox)
			.IsEnabled_Lambda([VM]()
			{
				return VM.IsValid() && VM->GeneratedWorld
					&& UAPSPlanetSurfaceProfileResolver::SupportsWorldScape(VM->GeneratedWorld->PlanetType);
			})
		+ SVerticalBox::Slot().AutoHeight()[NumberRow<int32>(LOCTEXT("SurfaceSeed", "SURFACE SEED"), 0, 999983, 1, VM, [](const UGeneratedWorld* W){ return FMath::Clamp(W->PlanetSurfaceSeed, 0, 999983); }, [](UWorldGenerationViewModel* V, int32 X){ V->SetPlanetSurfaceSeed(X); }, &SurfaceControlSliders.FindOrAdd(EAPSGenerationSurfaceControl::Seed))]
		+ SVerticalBox::Slot().AutoHeight()[NumberRow<double>(LOCTEXT("SurfaceFeatureScale", "FEATURE SCALE"), 0.25, 4.0, 0.05, VM, [](const UGeneratedWorld* W){ return W->SurfaceFeatureScale; }, [](UWorldGenerationViewModel* V, double X){ V->SetSurfaceFeatureScale(X); }, &SurfaceControlSliders.FindOrAdd(EAPSGenerationSurfaceControl::FeatureScale))]
		+ SVerticalBox::Slot().AutoHeight()[NumberRow<double>(LOCTEXT("SurfaceReliefScale", "RELIEF"), 0.25, 2.5, 0.05, VM, [](const UGeneratedWorld* W){ return W->SurfaceReliefScale; }, [](UWorldGenerationViewModel* V, double X){ V->SetSurfaceReliefScale(X); }, &SurfaceControlSliders.FindOrAdd(EAPSGenerationSurfaceControl::ReliefScale))]
		+ SVerticalBox::Slot().AutoHeight()[NumberRow<double>(LOCTEXT("SurfaceLandScale", "LAND COVERAGE"), 0.25, 2.0, 0.05, VM, [](const UGeneratedWorld* W){ return W->SurfaceLandCoverageScale; }, [](UWorldGenerationViewModel* V, double X){ V->SetSurfaceLandCoverageScale(X); }, &SurfaceControlSliders.FindOrAdd(EAPSGenerationSurfaceControl::LandCoverage))]
		+ SVerticalBox::Slot().AutoHeight()[NumberRow<double>(LOCTEXT("SurfaceMountainScale", "MOUNTAINS"), 0.0, 2.0, 0.05, VM, [](const UGeneratedWorld* W){ return W->SurfaceMountainScale; }, [](UWorldGenerationViewModel* V, double X){ V->SetSurfaceMountainScale(X); }, &SurfaceControlSliders.FindOrAdd(EAPSGenerationSurfaceControl::Mountains))]
		+ SVerticalBox::Slot().AutoHeight()[NumberRow<double>(LOCTEXT("SurfaceCraterScale", "CRATERS"), 0.0, 2.0, 0.05, VM, [](const UGeneratedWorld* W){ return W->SurfaceCraterScale; }, [](UWorldGenerationViewModel* V, double X){ V->SetSurfaceCraterScale(X); }, &SurfaceControlSliders.FindOrAdd(EAPSGenerationSurfaceControl::Craters))]
		+ SVerticalBox::Slot().AutoHeight()[NumberRow<double>(LOCTEXT("SurfaceRoughnessScale", "ROUGHNESS"), 0.25, 2.0, 0.05, VM, [](const UGeneratedWorld* W){ return W->SurfaceRoughnessScale; }, [](UWorldGenerationViewModel* V, double X){ V->SetSurfaceRoughnessScale(X); }, &SurfaceControlSliders.FindOrAdd(EAPSGenerationSurfaceControl::Roughness))]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 5.0f, 0.0f, 0.0f)
		[
			SNew(STextBlock)
			.Text(LOCTEXT("SurfaceSolidHint", "SOLID PLANETS ONLY — GAS GIANTS KEEP THEIR ASTRONOMICAL MATERIAL."))
			.AutoWrapText(true).Font(ReadableFont("Regular", 9)).ColorAndOpacity(SecondaryText)
		]
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 12.0f, 0.0f, 10.0f)[SectionTitle(LOCTEXT("Atmosphere", "PLANET ATMOSPHERE"))]
		+ SVerticalBox::Slot().AutoHeight()[NumberRow<double>(LOCTEXT("AtmosphereHeight", "HEIGHT / KM"), 0.0, 2000.0, 5.0, VM, [](const UGeneratedWorld* W){ return W->AtmosphereHeight; }, [](UWorldGenerationViewModel* V, double X){ if(V->GeneratedWorld){V->GeneratedWorld->AtmosphereHeight=X; V->RefreshPlanetAppearancePreview(false);} })]
		+ SVerticalBox::Slot().AutoHeight()[NumberRow<double>(LOCTEXT("AtmosphereOpacity", "OPACITY"), 0.0, 40.0, 0.25, VM, [](const UGeneratedWorld* W){ return W->AtmosphereOpacity; }, [](UWorldGenerationViewModel* V, double X){ if(V->GeneratedWorld){V->GeneratedWorld->AtmosphereOpacity=X; V->RefreshPlanetAppearancePreview(false);} })]
		+ SVerticalBox::Slot().AutoHeight()[NumberRow<double>(LOCTEXT("AtmosphereMulti", "MULTI SCATTERING"), 0.0, 10.0, 0.05, VM, [](const UGeneratedWorld* W){ return W->AtmosphereMultiScattering; }, [](UWorldGenerationViewModel* V, double X){ if(V->GeneratedWorld){V->GeneratedWorld->AtmosphereMultiScattering=X; V->RefreshPlanetAppearancePreview(false);} })]
		+ SVerticalBox::Slot().AutoHeight()[NumberRow<double>(LOCTEXT("AtmosphereRayleigh", "RAYLEIGH SCATTERING"), 0.0, 64.0, 0.25, VM, [](const UGeneratedWorld* W){ return W->AtmosphereRayleighScattering; }, [](UWorldGenerationViewModel* V, double X){ if(V->GeneratedWorld){V->GeneratedWorld->AtmosphereRayleighScattering=X; V->RefreshPlanetAppearancePreview(false);} })]
		+ SVerticalBox::Slot().AutoHeight()[NumberRow<double>(LOCTEXT("AtmosphereColorR", "COLOR / RED"), 0.0, 64.0, 0.1, VM, [](const UGeneratedWorld* W){ return static_cast<double>(W->AtmosphereColor.R); }, [](UWorldGenerationViewModel* V, double X){ if(V->GeneratedWorld){V->GeneratedWorld->AtmosphereColor.R=static_cast<float>(X); V->RefreshPlanetAppearancePreview(false);} })]
		+ SVerticalBox::Slot().AutoHeight()[NumberRow<double>(LOCTEXT("AtmosphereColorG", "COLOR / GREEN"), 0.0, 64.0, 0.1, VM, [](const UGeneratedWorld* W){ return static_cast<double>(W->AtmosphereColor.G); }, [](UWorldGenerationViewModel* V, double X){ if(V->GeneratedWorld){V->GeneratedWorld->AtmosphereColor.G=static_cast<float>(X); V->RefreshPlanetAppearancePreview(false);} })]
		+ SVerticalBox::Slot().AutoHeight()[NumberRow<double>(LOCTEXT("AtmosphereColorB", "COLOR / BLUE"), 0.0, 64.0, 0.1, VM, [](const UGeneratedWorld* W){ return static_cast<double>(W->AtmosphereColor.B); }, [](UWorldGenerationViewModel* V, double X){ if(V->GeneratedWorld){V->GeneratedWorld->AtmosphereColor.B=static_cast<float>(X); V->RefreshPlanetAppearancePreview(false);} })]
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
				SNew(SButton).ButtonStyle(&SecondaryButton).ContentPadding(FMargin(9.0f, 4.0f))
				.OnClicked(this, &SWorldGenerationPanel::FocusPreviewUp)
				.IsEnabled_Lambda([VM]() { return VM.IsValid() && VM->GetPreviewFocus() != EAstroPreviewFocus::Overview; })
				[SNew(STextBlock).Text(LOCTEXT("Up", "^  UP")).Font(Font("Bold", 8)).ColorAndOpacity(White)]
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
				SNew(STextBlock).AutoWrapText(true)
				.Font(ReadableFont("Regular", 10)).ColorAndOpacity(White)
				.Text_Lambda([VM]() { return VM.IsValid() ? VM->GetPreviewScopeSummary() : FText::GetEmpty(); })
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
					? FLinearColor(0.32f, 0.13f, 0.005f, 1.0f) : FLinearColor::White;
			})
			.ContentPadding(FMargin(12.0f, 8.0f))
			.OnClicked(this, &SWorldGenerationPanel::FocusPreview, static_cast<uint8>(Focus))
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.0f, 0.0f, 7.0f, 0.0f)
				[SNew(STextBlock).Text(Glyph).Font(APSGenerationUI::Font("Bold", 9)).ColorAndOpacity(APSGenerationUI::Cyan)]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
				[SNew(STextBlock).Text(Label).Font(APSGenerationUI::Font("Bold", 10)).ColorAndOpacity(APSGenerationUI::White)]
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
					[SNew(STextBlock).Text(LOCTEXT("Back", "<  BACK")).Font(Font("Bold", 12)).ColorAndOpacity(White)]
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
					[SNew(STextBlock).Text(LOCTEXT("Refresh", "REGENERATE")).Font(Font("Bold", 11)).ColorAndOpacity(Cyan)]
				]
			]
			+ SVerticalBox::Slot().FillHeight(1.0f).Padding(0.0f, 14.0f)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().FillWidth(0.25f).Padding(0.0f, 0.0f, 10.0f, 0.0f)
				[
					ChamferPanel(ControlsSwitcher)
				]
				+ SHorizontalBox::Slot().FillWidth(0.50f).Padding(6.0f, 0.0f)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().FillHeight(1.0f)
					[
					SNew(SOverlay)
						+ SOverlay::Slot()[SNew(SPreviewInteractionSurface).ViewModel(VM)]
						+ SOverlay::Slot()[SNew(SPreviewSystemOverlay).ViewModel(VM)]
						+ SOverlay::Slot().HAlign(HAlign_Left).VAlign(VAlign_Top).Padding(10.0f)
						[SNew(STextBlock).Text(LOCTEXT("PreviewCornerTL", "+  LIVE FULL-SCALE PREVIEW")).Font(Font("Bold", 9)).ColorAndOpacity(FLinearColor(0.20f, 0.90f, 0.55f, 0.82f))]
						+ SOverlay::Slot().HAlign(HAlign_Right).VAlign(VAlign_Bottom).Padding(10.0f)
						[SNew(STextBlock).Text(LOCTEXT("PreviewCornerBR", "FULL SCALE  +")).Font(Font("Bold", 8)).ColorAndOpacity(FLinearColor(Cyan.R, Cyan.G, Cyan.B, 0.55f))]
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
				+ SHorizontalBox::Slot().FillWidth(0.25f).Padding(10.0f, 0.0f, 0.0f, 0.0f)
				[
					ChamferPanel(ContextPanel)
				]
			]
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center)
				[
					SNew(STextBlock).Text(this, &SWorldGenerationPanel::GetPreviewStatus).Font(Font("Bold", 11)).ColorAndOpacity(Cyan)
				]
				+ SHorizontalBox::Slot().AutoWidth()
				[
					SNew(SButton).ButtonStyle(&PrimaryButton).OnClicked(this, &SWorldGenerationPanel::CommitWorld)
					.IsEnabled_Lambda([VM]() { return VM.IsValid() && VM->bPreviewReady; })
					.ContentPadding(FMargin(52.0f, 13.0f))
					[SNew(STextBlock).Text(this, &SWorldGenerationPanel::GetContinueLabel).Font(Font("Bold", 14)).ColorAndOpacity(White)]
				]
			]
		]
	];

	if (UWorldGenerationViewModel* MutableVM = ViewModel.Get())
	{
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
	if (UWorldGenerationViewModel* VM = ViewModel.Get()) VM->SetPreviewFocus(static_cast<EAstroPreviewFocus>(FocusValue));
	return FReply::Handled();
}

FReply SWorldGenerationPanel::FocusPreviewUp()
{
	if (UWorldGenerationViewModel* VM = ViewModel.Get())
	{
		EAstroPreviewFocus ParentFocus = EAstroPreviewFocus::Overview;
		switch (VM->GetPreviewFocus())
		{
		case EAstroPreviewFocus::HomePlanet:
		case EAstroPreviewFocus::HomeStar:
			ParentFocus = EAstroPreviewFocus::HomeSystem;
			break;
		case EAstroPreviewFocus::HomeSystem:
			ParentFocus = EAstroPreviewFocus::StarCluster;
			break;
		case EAstroPreviewFocus::StarCluster:
			ParentFocus = EAstroPreviewFocus::Galaxy;
			break;
		case EAstroPreviewFocus::Galaxy:
		default:
			ParentFocus = EAstroPreviewFocus::Overview;
			break;
		}
		VM->SetPreviewFocus(ParentFocus);
	}
	return FReply::Handled();
}

FReply SWorldGenerationPanel::FocusPreviewBody(TWeakObjectPtr<AActor> BodyActor)
{
	if (UWorldGenerationViewModel* VM = ViewModel.Get())
	{
		VM->FocusPreviewBody(BodyActor);
	}
	return FReply::Handled();
}

FReply SWorldGenerationPanel::FocusPreviewHierarchyEntry(
	TWeakObjectPtr<AActor> BodyActor, int32 ClusterSystemInstanceIndex, int32 PreviewFocusValue)
{
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

	TArray<FAPSPreviewBodyEntry> Entries;
	VM->GetPreviewBodyEntries(Entries);
	uint32 Signature = HashCombineFast(GetTypeHash(VM->PreviewRevision), GetTypeHash(Entries.Num()));
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

	for (int32 EntryIndex = 0; EntryIndex < Entries.Num(); ++EntryIndex)
	{
		const FAPSPreviewBodyEntry& Entry = Entries[EntryIndex];
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
		BodyHierarchyBox->AddSlot().AutoHeight().Padding(0.0f, 2.0f)
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
					.BorderBackgroundColor_Lambda([IsSelected]()
					{
						return IsSelected() ? SelectedFill : FLinearColor::Transparent;
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
					+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center)
					[
						SNew(SVerticalBox)
						+ SVerticalBox::Slot().AutoHeight()
						[
							SNew(STextBlock).Text(Entry.Label).Font(ReadableFont("Bold", 10))
							.ColorAndOpacity(White).OverflowPolicy(ETextOverflowPolicy::Ellipsis)
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
							SNew(SBorder).BorderImage(&ControlBrush).Padding(0.0f)
							[
								SNew(STextBlock).Text(FText::AsNumber(ImmediateChildCount))
								.Justification(ETextJustify::Center).Font(Font("Bold", 8)).ColorAndOpacity(Cyan)
							]
						]
					]
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
					[
						SNew(STextBlock).Text(bCanFocus ? FText::FromString(TEXT(">")) : FText::GetEmpty())
						.Font(Font("Bold", 9)).ColorAndOpacity_Lambda([IsSelected]()
						{
							return IsSelected() ? Amber : Cyan;
						})
					]
				]
				+ SOverlay::Slot().HAlign(HAlign_Left)
				[
					SNew(SBox).WidthOverride(3.0f)
					.Visibility_Lambda([IsSelected]()
					{
						return IsSelected() ? EVisibility::HitTestInvisible : EVisibility::Collapsed;
					})
					[SNew(SBorder).BorderImage(FAppStyle::GetBrush("WhiteBrush")).BorderBackgroundColor(Amber)]
				]
			]
		];
	}
}

#undef LOCTEXT_NAMESPACE
