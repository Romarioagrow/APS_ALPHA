#include "SWorldGenerationPanel.h"

#include "APS_ALPHA/Core/Model/GeneratedWorld.h"
#include "APS_ALPHA/UI/MainMenu/WorldGenerationViewModel.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SSpinBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "WorldGenerationPanel"

void SWorldGenerationPanel::Construct(const FArguments& InArgs)
{
	ViewModel = InArgs._ViewModel;
	const FLinearColor Accent(0.18f, 0.78f, 0.95f, 1.0f);
	const FLinearColor Muted(0.58f, 0.66f, 0.72f, 1.0f);

	ChildSlot
	[
		SNew(SBorder)
		.BorderBackgroundColor(FLinearColor(0.015f, 0.025f, 0.04f, 0.94f))
		.Padding(FMargin(28.0f, 24.0f))
		[
			SNew(SBox)
			.WidthOverride(430.0f)
			[
				SNew(SScrollBox)
				+ SScrollBox::Slot()
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 4.0f)
					[
						SNew(STextBlock)
						.Text(LOCTEXT("Title", "GENERATE CIVILIZATION"))
						.ColorAndOpacity(Accent)
						.Font(FCoreStyle::GetDefaultFontStyle("Bold", 22))
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 22.0f)
					[
						SNew(STextBlock)
						.Text(LOCTEXT("Subtitle", "ASTRONOMICAL GENERATION / LIVE SCENE"))
						.ColorAndOpacity(Muted)
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 5.0f)
					[
						SNew(STextBlock).Text(LOCTEXT("GalaxySize", "GALAXY SIZE"))
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 12.0f)
					[
						SNew(SSpinBox<int32>)
						.MinValue(1).MaxValue(100000)
						.Value_Lambda([WeakViewModel = ViewModel]()
						{
							const UWorldGenerationViewModel* VM = WeakViewModel.Get();
							return VM && VM->GeneratedWorld ? VM->GeneratedWorld->GalaxySize : 250;
						})
						.OnValueChanged_Lambda([WeakViewModel = ViewModel](int32 Value)
						{
							if (UWorldGenerationViewModel* VM = WeakViewModel.Get()) VM->SetGalaxySize(Value);
						})
						.OnValueCommitted_Lambda([WeakViewModel = ViewModel](int32 Value, ETextCommit::Type)
						{
							if (UWorldGenerationViewModel* VM = WeakViewModel.Get()) VM->SetGalaxySize(Value);
						})
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 5.0f)
					[
						SNew(STextBlock).Text(LOCTEXT("PlanetRadius", "HOME PLANET RADIUS / KM"))
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 12.0f)
					[
						SNew(SSpinBox<double>)
						.MinValue(100.0).MaxValue(200000.0).Delta(100.0)
						.Value_Lambda([WeakViewModel = ViewModel]()
						{
							const UWorldGenerationViewModel* VM = WeakViewModel.Get();
							return VM && VM->GeneratedWorld ? VM->GeneratedWorld->PlanetRadius : 6750.0;
						})
						.OnValueChanged_Lambda([WeakViewModel = ViewModel](double Value)
						{
							if (UWorldGenerationViewModel* VM = WeakViewModel.Get()) VM->SetPlanetRadius(Value);
						})
						.OnValueCommitted_Lambda([WeakViewModel = ViewModel](double Value, ETextCommit::Type)
						{
							if (UWorldGenerationViewModel* VM = WeakViewModel.Get()) VM->SetPlanetRadius(Value);
						})
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 5.0f)
					[
						SNew(STextBlock).Text(LOCTEXT("Planets", "PLANETS"))
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 12.0f)
					[
						SNew(SSpinBox<int32>)
						.MinValue(1).MaxValue(64)
						.Value_Lambda([WeakViewModel = ViewModel]()
						{
							const UWorldGenerationViewModel* VM = WeakViewModel.Get();
							return VM && VM->GeneratedWorld ? FMath::Max(1, VM->GeneratedWorld->PlanetsAmount) : 1;
						})
						.OnValueChanged_Lambda([WeakViewModel = ViewModel](int32 Value)
						{
							if (UWorldGenerationViewModel* VM = WeakViewModel.Get()) VM->SetPlanetsAmount(Value);
						})
						.OnValueCommitted_Lambda([WeakViewModel = ViewModel](int32 Value, ETextCommit::Type)
						{
							if (UWorldGenerationViewModel* VM = WeakViewModel.Get()) VM->SetPlanetsAmount(Value);
						})
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 5.0f)
					[
						SNew(STextBlock).Text(LOCTEXT("Moons", "MOONS"))
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 24.0f)
					[
						SNew(SSpinBox<int32>)
						.MinValue(0).MaxValue(32)
						.Value_Lambda([WeakViewModel = ViewModel]()
						{
							const UWorldGenerationViewModel* VM = WeakViewModel.Get();
							return VM && VM->GeneratedWorld ? VM->GeneratedWorld->MoonsAmount : 0;
						})
						.OnValueChanged_Lambda([WeakViewModel = ViewModel](int32 Value)
						{
							if (UWorldGenerationViewModel* VM = WeakViewModel.Get()) VM->SetMoonsAmount(Value);
						})
						.OnValueCommitted_Lambda([WeakViewModel = ViewModel](int32 Value, ETextCommit::Type)
						{
							if (UWorldGenerationViewModel* VM = WeakViewModel.Get()) VM->SetMoonsAmount(Value);
						})
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 4.0f, 0.0f, 14.0f)
					[
						SNew(STextBlock)
						.Text(this, &SWorldGenerationPanel::GetPreviewStatus)
						.ColorAndOpacity(Accent)
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 4.0f)
					[
						SNew(SButton)
						.Text(LOCTEXT("Refresh", "REFRESH LIVE SCENE"))
						.OnClicked(this, &SWorldGenerationPanel::RefreshPreview)
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 4.0f)
					[
						SNew(SButton)
						.Text(LOCTEXT("Generate", "CREATE WORLD"))
						.OnClicked(this, &SWorldGenerationPanel::CommitWorld)
					]
				]
			]
		]
	];
}

FText SWorldGenerationPanel::GetPreviewStatus() const
{
	if (const UWorldGenerationViewModel* VM = ViewModel.Get())
	{
		return VM->PreviewStatus;
	}
	return LOCTEXT("NoViewModel", "PREVIEW OFFLINE");
}

FReply SWorldGenerationPanel::CommitWorld()
{
	if (UWorldGenerationViewModel* VM = ViewModel.Get())
	{
		VM->CommitAndOpenLevel();
	}
	return FReply::Handled();
}

FReply SWorldGenerationPanel::RefreshPreview()
{
	if (UWorldGenerationViewModel* VM = ViewModel.Get())
	{
		VM->RequestPreview();
	}
	return FReply::Handled();
}

#undef LOCTEXT_NAMESPACE
