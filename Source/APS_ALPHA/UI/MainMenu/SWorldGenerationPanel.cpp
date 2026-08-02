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
#include "APS_ALPHA/UI/MainMenu/WorldGenerationViewModel.h"
#include "InputCoreTypes.h"
#include "Styling/AppStyle.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SSpinBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "WorldGenerationPanel"

namespace APSGenerationUI
{
	const FLinearColor Background(0.002f, 0.010f, 0.018f, 0.54f);
	const FLinearColor Panel(0.008f, 0.030f, 0.047f, 0.91f);
	const FLinearColor Cyan(0.12f, 0.82f, 1.0f, 1.0f);
	const FLinearColor CyanDim(0.05f, 0.28f, 0.39f, 1.0f);
	const FLinearColor Amber(1.0f, 0.56f, 0.04f, 1.0f);
	const FLinearColor White(0.92f, 0.97f, 1.0f, 1.0f);
	const FLinearColor Muted(0.46f, 0.61f, 0.69f, 1.0f);

	FSlateFontInfo Font(const FName Typeface, int32 Size)
	{
		return FCoreStyle::GetDefaultFontStyle(Typeface, Size);
	}

	FButtonStyle MakeButtonStyle(const FLinearColor& Outline, const FLinearColor& Fill)
	{
		return FButtonStyle()
			.SetNormal(FSlateRoundedBoxBrush(Fill, 6.0f, Outline, 1.0f))
			.SetHovered(FSlateRoundedBoxBrush(FLinearColor(Fill.R + 0.025f, Fill.G + 0.05f, Fill.B + 0.07f, 0.98f), 6.0f, Cyan, 1.5f))
			.SetPressed(FSlateRoundedBoxBrush(FLinearColor(0.02f, 0.14f, 0.20f, 1.0f), 6.0f, Cyan, 1.5f));
	}

	TSharedRef<SWidget> SectionTitle(const FText& Text)
	{
		return SNew(STextBlock).Text(Text).Font(Font("Bold", 16)).ColorAndOpacity(Cyan);
	}

	template <typename TEnum, typename TGetter>
	TSharedRef<SWidget> EnumRow(const FText& Label, TWeakObjectPtr<UWorldGenerationViewModel> ViewModel, TGetter Getter)
	{
		const UEnum* Enum = StaticEnum<TEnum>();
		return SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(STextBlock).Text(Label).Font(Font("Regular", 11)).ColorAndOpacity(Muted)
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 3.0f, 0.0f, 8.0f)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth()
				[
					SNew(SButton).Text(FText::FromString(TEXT("<"))).ContentPadding(FMargin(9.0f, 4.0f))
					.OnClicked_Lambda([ViewModel, Getter, Enum]()
					{
						if (UWorldGenerationViewModel* VM = ViewModel.Get(); VM && VM->GeneratedWorld && Enum)
						{
							const int32 Count = FMath::Max(1, Enum->NumEnums() - 1);
							const int32 Current = static_cast<int32>(Getter(VM->GeneratedWorld));
							VM->SetEnumValue(Enum, (Current - 1 + Count) % Count);
						}
						return FReply::Handled();
					})
				]
				+ SHorizontalBox::Slot().FillWidth(1.0f).HAlign(HAlign_Center).VAlign(VAlign_Center)
				[
					SNew(STextBlock)
					.Text_Lambda([ViewModel, Getter, Enum]()
					{
						if (const UWorldGenerationViewModel* VM = ViewModel.Get(); VM && VM->GeneratedWorld && Enum)
						{
							return Enum->GetDisplayNameTextByValue(static_cast<int64>(Getter(VM->GeneratedWorld)));
						}
						return FText::FromString(TEXT("--"));
					})
					.Font(Font("Bold", 11)).ColorAndOpacity(White)
				]
				+ SHorizontalBox::Slot().AutoWidth()
				[
					SNew(SButton).Text(FText::FromString(TEXT(">"))).ContentPadding(FMargin(9.0f, 4.0f))
					.OnClicked_Lambda([ViewModel, Getter, Enum]()
					{
						if (UWorldGenerationViewModel* VM = ViewModel.Get(); VM && VM->GeneratedWorld && Enum)
						{
							const int32 Count = FMath::Max(1, Enum->NumEnums() - 1);
							const int32 Current = static_cast<int32>(Getter(VM->GeneratedWorld));
							VM->SetEnumValue(Enum, (Current + 1) % Count);
						}
						return FReply::Handled();
					})
				]
			];
	}

	template <typename TValue, typename TGetter, typename TSetter>
	TSharedRef<SWidget> NumberRow(const FText& Label, TValue Min, TValue Max, TValue Delta,
		TWeakObjectPtr<UWorldGenerationViewModel> ViewModel, TGetter Getter, TSetter Setter)
	{
		return SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(STextBlock).Text(Label).Font(Font("Regular", 11)).ColorAndOpacity(Muted)
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 3.0f, 0.0f, 9.0f)
			[
				SNew(SSpinBox<TValue>)
				.MinValue(Min).MaxValue(Max).Delta(Delta)
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
				SNew(SBorder)
				.BorderImage(FAppStyle::GetBrush("WhiteBrush"))
				.BorderBackgroundColor(FLinearColor(0.0f, 0.025f, 0.045f, 0.08f))
				.Padding(16.0f)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
					[
						SNew(STextBlock).Text(LOCTEXT("LiveScene", "LIVE FULL-SCALE SCENE"))
						.Font(Font("Bold", 13)).ColorAndOpacity(Cyan)
					]
					+ SVerticalBox::Slot().FillHeight(1.0f).VAlign(VAlign_Center).HAlign(HAlign_Center)
					[
						SNew(STextBlock).Text(LOCTEXT("PreviewHelp", "DRAG RMB TO ORBIT  /  WHEEL TO ZOOM\nSELECT A LEVEL BELOW TO FOCUS"))
						.Justification(ETextJustify::Center).Font(Font("Regular", 10)).ColorAndOpacity(Muted)
					]
				]
			];
		}

		virtual FReply OnMouseButtonDown(const FGeometry&, const FPointerEvent& Event) override
		{
			if (Event.GetEffectingButton() == EKeys::RightMouseButton || Event.GetEffectingButton() == EKeys::MiddleMouseButton)
			{
				return FReply::Handled().CaptureMouse(SharedThis(this));
			}
			return FReply::Unhandled();
		}

		virtual FReply OnMouseButtonUp(const FGeometry&, const FPointerEvent& Event) override
		{
			if (HasMouseCapture()) return FReply::Handled().ReleaseMouseCapture();
			return FReply::Unhandled();
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
}

void SWorldGenerationPanel::Construct(const FArguments& InArgs)
{
	ViewModel = InArgs._ViewModel;
	OnBack = InArgs._OnBack;
	OnContinue = InArgs._OnContinue;

	using namespace APSGenerationUI;
	const TWeakObjectPtr<UWorldGenerationViewModel> VM = ViewModel;

	const TSharedRef<SWidget> LeftControls = SNew(SScrollBox)
		+ SScrollBox::Slot()
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 12.0f)[SectionTitle(LOCTEXT("Astro", "ASTRO GENERATION"))]
			+ SVerticalBox::Slot().AutoHeight()[EnumRow<EAstroGenerationLevel>(LOCTEXT("Level", "GENERATION LEVEL"), VM, [](const UGeneratedWorld* W){ return W->AstroGenerationLevel; })]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 12.0f, 0.0f, 10.0f)[SectionTitle(LOCTEXT("Cluster", "STAR CLUSTER"))]
			+ SVerticalBox::Slot().AutoHeight()[EnumRow<EStarClusterSize>(LOCTEXT("ClusterSize", "SIZE"), VM, [](const UGeneratedWorld* W){ return W->StarClusterSize; })]
			+ SVerticalBox::Slot().AutoHeight()[EnumRow<EStarClusterType>(LOCTEXT("ClusterType", "TYPE / PRESET FAMILY"), VM, [](const UGeneratedWorld* W){ return W->StarClusterType; })]
			+ SVerticalBox::Slot().AutoHeight()[EnumRow<EStarClusterPopulation>(LOCTEXT("Population", "POPULATION"), VM, [](const UGeneratedWorld* W){ return W->StarClusterPopulation; })]
			+ SVerticalBox::Slot().AutoHeight()[EnumRow<EStarClusterComposition>(LOCTEXT("Composition", "COMPOSITION"), VM, [](const UGeneratedWorld* W){ return W->StarClusterComposition; })]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 12.0f, 0.0f, 10.0f)[SectionTitle(LOCTEXT("Galaxy", "GALAXY"))]
			+ SVerticalBox::Slot().AutoHeight()[EnumRow<EGalaxyType>(LOCTEXT("GalaxyType", "TYPE / PRESET"), VM, [](const UGeneratedWorld* W){ return W->GalaxyType; })]
			+ SVerticalBox::Slot().AutoHeight()[EnumRow<EGalaxyClass>(LOCTEXT("GalaxyClass", "CLASS"), VM, [](const UGeneratedWorld* W){ return W->GalaxyClass; })]
			+ SVerticalBox::Slot().AutoHeight()[NumberRow<int32>(LOCTEXT("GalaxySize", "SIZE"), 1, 100000, 1, VM, [](const UGeneratedWorld* W){ return W->GalaxySize; }, [](UWorldGenerationViewModel* V, int32 X){ V->SetGalaxySize(X); })]
			+ SVerticalBox::Slot().AutoHeight()[NumberRow<int32>(LOCTEXT("StarCount", "STAR COUNT"), 1, 10000000, 100, VM, [](const UGeneratedWorld* W){ return W->GalaxyStarCount; }, [](UWorldGenerationViewModel* V, int32 X){ V->SetGalaxyStarCount(X); })]
			+ SVerticalBox::Slot().AutoHeight()[NumberRow<double>(LOCTEXT("Density", "STAR DENSITY"), 0.01, 1000.0, 0.1, VM, [](const UGeneratedWorld* W){ return W->GalaxyStarDensity; }, [](UWorldGenerationViewModel* V, double X){ V->SetGalaxyStarDensity(X); })]
		];

	const TSharedRef<SWidget> RightControls = SNew(SScrollBox)
		+ SScrollBox::Slot()
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 12.0f)[SectionTitle(LOCTEXT("HomeStarSystem", "HOME STAR SYSTEM"))]
			+ SVerticalBox::Slot().AutoHeight()[EnumRow<EStellarType>(LOCTEXT("StellarType", "STELLAR TYPE"), VM, [](const UGeneratedWorld* W){ return W->StellarType; })]
			+ SVerticalBox::Slot().AutoHeight()[EnumRow<ESpectralClass>(LOCTEXT("Spectral", "SPECTRAL CLASS"), VM, [](const UGeneratedWorld* W){ return W->SpectralClass; })]
			+ SVerticalBox::Slot().AutoHeight()[EnumRow<EPlanetType>(LOCTEXT("PlanetType", "HOME PLANET TYPE"), VM, [](const UGeneratedWorld* W){ return W->PlanetType; })]
			+ SVerticalBox::Slot().AutoHeight()[NumberRow<double>(LOCTEXT("Radius", "PLANET RADIUS / KM"), 100.0, 200000.0, 100.0, VM, [](const UGeneratedWorld* W){ return W->PlanetRadius; }, [](UWorldGenerationViewModel* V, double X){ V->SetPlanetRadius(X); })]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 12.0f, 0.0f, 10.0f)[SectionTitle(LOCTEXT("SystemLayout", "SYSTEM LAYOUT"))]
			+ SVerticalBox::Slot().AutoHeight()[EnumRow<EPlanetarySystemType>(LOCTEXT("SystemType", "PLANETARY TYPE"), VM, [](const UGeneratedWorld* W){ return W->PlanetarySystemType; })]
			+ SVerticalBox::Slot().AutoHeight()[EnumRow<EOrbitDistributionType>(LOCTEXT("Distribution", "ORBIT DISTRIBUTION"), VM, [](const UGeneratedWorld* W){ return W->OrbitDistributionType; })]
			+ SVerticalBox::Slot().AutoHeight()[NumberRow<int32>(LOCTEXT("Planets", "PLANETS AMOUNT"), 1, 64, 1, VM, [](const UGeneratedWorld* W){ return FMath::Max(1, W->PlanetsAmount); }, [](UWorldGenerationViewModel* V, int32 X){ V->SetPlanetsAmount(X); })]
			+ SVerticalBox::Slot().AutoHeight()[NumberRow<int32>(LOCTEXT("Moons", "MOONS AMOUNT"), 0, 32, 1, VM, [](const UGeneratedWorld* W){ return W->MoonsAmount; }, [](UWorldGenerationViewModel* V, int32 X){ V->SetMoonsAmount(X); })]
			+ SVerticalBox::Slot().AutoHeight()[NumberRow<int32>(LOCTEXT("StartIndex", "START PLANET INDEX"), 1, 64, 1, VM, [](const UGeneratedWorld* W){ return FMath::Max(1, W->StartPlanetIndex); }, [](UWorldGenerationViewModel* V, int32 X){ V->SetStartPlanetIndex(X); })]
		];

	const auto FocusButton = [this](const FText& Label, EAstroPreviewFocus Focus)
	{
		return SNew(SButton)
			.ContentPadding(FMargin(10.0f, 6.0f))
			.OnClicked(this, &SWorldGenerationPanel::FocusPreview, static_cast<uint8>(Focus))
			[SNew(STextBlock).Text(Label).Font(APSGenerationUI::Font("Bold", 10)).ColorAndOpacity(APSGenerationUI::White)];
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
					SNew(SButton).OnClicked(this, &SWorldGenerationPanel::GoBack).ContentPadding(FMargin(15.0f, 8.0f))
					[SNew(STextBlock).Text(LOCTEXT("Back", "<  BACK")).Font(Font("Bold", 12)).ColorAndOpacity(White)]
				]
				+ SHorizontalBox::Slot().FillWidth(1.0f).HAlign(HAlign_Center)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)[SNew(STextBlock).Text(LOCTEXT("Title", "APOSFERA")).Font(Font("Bold", 32)).ColorAndOpacity(White)]
					+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)[SNew(STextBlock).Text(LOCTEXT("Subtitle", "ASTRONOMICAL GENERATION")).Font(Font("Bold", 14)).ColorAndOpacity(Cyan)]
				]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
				[
					SNew(SButton).OnClicked(this, &SWorldGenerationPanel::RefreshPreview).ContentPadding(FMargin(14.0f, 8.0f))
					[SNew(STextBlock).Text(LOCTEXT("Refresh", "REGENERATE")).Font(Font("Bold", 11)).ColorAndOpacity(Cyan)]
				]
			]
			+ SVerticalBox::Slot().FillHeight(1.0f).Padding(0.0f, 14.0f)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().FillWidth(0.25f).Padding(0.0f, 0.0f, 10.0f, 0.0f)
				[
					SNew(SBorder).BorderImage(FAppStyle::GetBrush("WhiteBrush")).BorderBackgroundColor(Panel).Padding(16.0f)[LeftControls]
				]
				+ SHorizontalBox::Slot().FillWidth(0.50f).Padding(6.0f, 0.0f)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().FillHeight(1.0f)[SNew(SPreviewInteractionSurface).ViewModel(VM)]
					+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0.0f, 10.0f)
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().AutoWidth().Padding(2.0f)[FocusButton(LOCTEXT("FocusOverview", "OVERVIEW"), EAstroPreviewFocus::Overview)]
						+ SHorizontalBox::Slot().AutoWidth().Padding(2.0f)[FocusButton(LOCTEXT("FocusCluster", "CLUSTER"), EAstroPreviewFocus::StarCluster)]
						+ SHorizontalBox::Slot().AutoWidth().Padding(2.0f)[FocusButton(LOCTEXT("FocusGalaxy", "GALAXY"), EAstroPreviewFocus::Galaxy)]
						+ SHorizontalBox::Slot().AutoWidth().Padding(2.0f)[FocusButton(LOCTEXT("FocusSystem", "SYSTEM"), EAstroPreviewFocus::HomeSystem)]
						+ SHorizontalBox::Slot().AutoWidth().Padding(2.0f)[FocusButton(LOCTEXT("FocusStar", "STAR"), EAstroPreviewFocus::HomeStar)]
						+ SHorizontalBox::Slot().AutoWidth().Padding(2.0f)[FocusButton(LOCTEXT("FocusPlanet", "PLANET"), EAstroPreviewFocus::HomePlanet)]
					]
				]
				+ SHorizontalBox::Slot().FillWidth(0.25f).Padding(10.0f, 0.0f, 0.0f, 0.0f)
				[
					SNew(SBorder).BorderImage(FAppStyle::GetBrush("WhiteBrush")).BorderBackgroundColor(Panel).Padding(16.0f)[RightControls]
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
					SNew(SButton).OnClicked(this, &SWorldGenerationPanel::CommitWorld).ContentPadding(FMargin(52.0f, 13.0f))
					[SNew(STextBlock).Text(LOCTEXT("Continue", "CONTINUE TO CIVILIZATION")).Font(Font("Bold", 14)).ColorAndOpacity(Amber)]
				]
			]
		]
	];

	if (UWorldGenerationViewModel* MutableVM = ViewModel.Get())
	{
		MutableVM->RequestPreview();
	}
}

FText SWorldGenerationPanel::GetPreviewStatus() const
{
	if (const UWorldGenerationViewModel* VM = ViewModel.Get()) return VM->PreviewStatus;
	return LOCTEXT("NoViewModel", "PREVIEW OFFLINE");
}

FReply SWorldGenerationPanel::CommitWorld()
{
	if (OnContinue.IsBound()) OnContinue.Execute();
	else if (UWorldGenerationViewModel* VM = ViewModel.Get()) VM->CommitAndOpenLevel();
	return FReply::Handled();
}

FReply SWorldGenerationPanel::RefreshPreview()
{
	if (UWorldGenerationViewModel* VM = ViewModel.Get()) VM->RequestPreview();
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

#undef LOCTEXT_NAMESPACE
