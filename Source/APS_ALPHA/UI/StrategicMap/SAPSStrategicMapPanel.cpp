#include "SAPSStrategicMapPanel.h"

#include "APS_ALPHA/Actors/Astro/Galaxy.h"
#include "APS_ALPHA/Actors/Astro/Planet.h"
#include "APS_ALPHA/Actors/Astro/Star.h"
#include "APS_ALPHA/Actors/Astro/StarCluster.h"
#include "APS_ALPHA/Actors/Astro/StarSystem.h"
#include "APS_ALPHA/Core/Controllers/GravityPlayerController.h"
#include "APS_ALPHA/Core/Model/GeneratedWorld.h"
#include "InputCoreTypes.h"
#include "Styling/AppStyle.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "APSStrategicMap"

namespace APSMapUI
{
	const FLinearColor Cyan(0.12f, 0.82f, 1.0f, 1.0f);
	const FLinearColor White(0.92f, 0.97f, 1.0f, 1.0f);
	const FLinearColor Muted(0.46f, 0.61f, 0.69f, 1.0f);
	FSlateFontInfo Font(const FName Typeface, int32 Size) { return FCoreStyle::GetDefaultFontStyle(Typeface, Size); }
}

void SAPSStrategicMapPanel::Construct(const FArguments& InArgs)
{
	Controller = InArgs._Controller;
	Generator = InArgs._Generator;
	OnClose = InArgs._OnClose;
	using namespace APSMapUI;

	const auto FocusButton = [this](const FText& Text, EAstroPreviewFocus FocusValue)
	{
		return SNew(SButton).OnClicked(this, &SAPSStrategicMapPanel::Focus, static_cast<uint8>(FocusValue))
			.ContentPadding(FMargin(14.0f, 9.0f))
			[SNew(STextBlock).Text(Text).Font(Font("Bold", 11)).ColorAndOpacity(White)];
	};

	ChildSlot
	[
		SNew(SBorder).BorderImage(FAppStyle::GetBrush("WhiteBrush"))
		.BorderBackgroundColor(FLinearColor(0.002f, 0.01f, 0.02f, 0.38f)).Padding(22.0f)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().FillWidth(1.0f)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight()[SNew(STextBlock).Text(LOCTEXT("Title", "STRATEGIC MAP // LIVE UNIVERSE")).Font(Font("Bold", 23)).ColorAndOpacity(White)]
					+ SVerticalBox::Slot().AutoHeight()[SNew(STextBlock).Text(LOCTEXT("Subtitle", "FULL-SCALE HIERARCHY  /  F10 TO CLOSE")).Font(Font("Regular", 10)).ColorAndOpacity(Cyan)]
				]
				+ SHorizontalBox::Slot().AutoWidth()[SNew(SButton).OnClicked(this, &SAPSStrategicMapPanel::Close).ContentPadding(FMargin(18.0f, 9.0f))[SNew(STextBlock).Text(LOCTEXT("Close", "CLOSE  X")).ColorAndOpacity(White)]]
			]
			+ SVerticalBox::Slot().FillHeight(1.0f).Padding(0.0f, 16.0f)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().Padding(0.0f, 0.0f, 12.0f, 0.0f)
				[
					SNew(SBox).WidthOverride(210.0f)
					[
						SNew(SBorder).BorderImage(FAppStyle::GetBrush("WhiteBrush")).BorderBackgroundColor(FLinearColor(0.005f, 0.035f, 0.055f, 0.92f)).Padding(14.0f)
						[
							SNew(SVerticalBox)
							+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 4.0f)[FocusButton(LOCTEXT("Overview", "UNIVERSE OVERVIEW"), EAstroPreviewFocus::Overview)]
							+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 4.0f)[FocusButton(LOCTEXT("Cluster", "STAR CLUSTER"), EAstroPreviewFocus::StarCluster)]
							+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 4.0f)[FocusButton(LOCTEXT("Galaxy", "GALAXY"), EAstroPreviewFocus::Galaxy)]
							+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 4.0f)[FocusButton(LOCTEXT("System", "HOME SYSTEM"), EAstroPreviewFocus::HomeSystem)]
							+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 4.0f)[FocusButton(LOCTEXT("Star", "HOME STAR"), EAstroPreviewFocus::HomeStar)]
							+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 4.0f)[FocusButton(LOCTEXT("Planet", "HOME PLANET"), EAstroPreviewFocus::HomePlanet)]
						]
					]
				]
				+ SHorizontalBox::Slot().FillWidth(1.0f)
				[
					SNew(SBorder).BorderImage(FAppStyle::GetBrush("WhiteBrush")).BorderBackgroundColor(FLinearColor(0.0f, 0.02f, 0.035f, 0.07f)).Padding(20.0f)
					[
						SNew(STextBlock).Text(LOCTEXT("Help", "RMB DRAG  //  ORBIT\nMOUSE WHEEL  //  SCALE\nDOUBLE CLICK OBJECT  //  FOCUS"))
						.Justification(ETextJustify::Center).Font(Font("Bold", 11)).ColorAndOpacity(Muted)
					]
				]
				+ SHorizontalBox::Slot().AutoWidth().Padding(12.0f, 0.0f, 0.0f, 0.0f)
				[
					SNew(SBox).WidthOverride(260.0f)
					[
						SNew(SBorder).BorderImage(FAppStyle::GetBrush("WhiteBrush")).BorderBackgroundColor(FLinearColor(0.005f, 0.035f, 0.055f, 0.92f)).Padding(16.0f)
						[SNew(STextBlock).Text(this, &SAPSStrategicMapPanel::GetWorldSummary).AutoWrapText(true).Font(Font("Regular", 11)).ColorAndOpacity(White)]
					]
				]
			]
		]
	];
}

FReply SAPSStrategicMapPanel::OnKeyDown(const FGeometry&, const FKeyEvent& Event)
{
	if (Event.GetKey() == EKeys::F10 || Event.GetKey() == EKeys::Escape) return Close();
	return FReply::Unhandled();
}

FReply SAPSStrategicMapPanel::OnMouseButtonDown(const FGeometry&, const FPointerEvent& Event)
{
	if (Event.GetEffectingButton() == EKeys::RightMouseButton || Event.GetEffectingButton() == EKeys::MiddleMouseButton)
		return FReply::Handled().CaptureMouse(SharedThis(this));
	return FReply::Unhandled();
}

FReply SAPSStrategicMapPanel::OnMouseButtonUp(const FGeometry&, const FPointerEvent&)
{
	if (HasMouseCapture()) return FReply::Handled().ReleaseMouseCapture();
	return FReply::Unhandled();
}

FReply SAPSStrategicMapPanel::OnMouseMove(const FGeometry&, const FPointerEvent& Event)
{
	if (HasMouseCapture())
	{
		if (AAstroGenerator* Astro = Generator.Get()) Astro->OrbitPreviewCamera(Event.GetCursorDelta());
		return FReply::Handled();
	}
	return FReply::Unhandled();
}

FReply SAPSStrategicMapPanel::OnMouseWheel(const FGeometry&, const FPointerEvent& Event)
{
	if (AAstroGenerator* Astro = Generator.Get()) Astro->ZoomPreviewCamera(Event.GetWheelDelta());
	return FReply::Handled();
}

FReply SAPSStrategicMapPanel::OnMouseButtonDoubleClick(const FGeometry&, const FPointerEvent& Event)
{
	if (Event.GetEffectingButton() == EKeys::LeftMouseButton && FocusUnderCursor()) return FReply::Handled();
	return FReply::Unhandled();
}

FReply SAPSStrategicMapPanel::Close()
{
	OnClose.ExecuteIfBound();
	return FReply::Handled();
}

FReply SAPSStrategicMapPanel::Focus(uint8 FocusValue)
{
	if (AAstroGenerator* Astro = Generator.Get()) Astro->FocusPreviewTarget(static_cast<EAstroPreviewFocus>(FocusValue), Controller.Get());
	return FReply::Handled();
}

bool SAPSStrategicMapPanel::FocusUnderCursor()
{
	AGravityPlayerController* PC = Controller.Get();
	UWorld* World = PC ? PC->GetWorld() : nullptr;
	if (!PC || !World) return false;
	float X = 0.0f, Y = 0.0f;
	FVector Origin, Direction;
	if (!PC->GetMousePosition(X, Y) || !PC->DeprojectScreenPositionToWorld(X, Y, Origin, Direction)) return false;
	FHitResult Hit;
	if (!World->LineTraceSingleByChannel(Hit, Origin, Origin + Direction * 1.0e15, ECC_Visibility)) return false;
	for (AActor* Candidate = Hit.GetActor(); IsValid(Candidate); Candidate = Candidate->GetAttachParentActor())
	{
		if (Candidate->IsA<APlanet>()) { Focus(static_cast<uint8>(EAstroPreviewFocus::HomePlanet)); return true; }
		if (Candidate->IsA<AStar>()) { Focus(static_cast<uint8>(EAstroPreviewFocus::HomeStar)); return true; }
		if (Candidate->IsA<AStarSystem>()) { Focus(static_cast<uint8>(EAstroPreviewFocus::HomeSystem)); return true; }
		if (Candidate->IsA<AGalaxy>()) { Focus(static_cast<uint8>(EAstroPreviewFocus::Galaxy)); return true; }
		if (Candidate->IsA<AStarCluster>()) { Focus(static_cast<uint8>(EAstroPreviewFocus::StarCluster)); return true; }
	}
	return false;
}

FText SAPSStrategicMapPanel::GetWorldSummary() const
{
	const AAstroGenerator* Astro = Generator.Get();
	const UGeneratedWorld* World = Astro ? Astro->GetGeneratedWorldModel() : nullptr;
	if (!World) return LOCTEXT("NoModel", "LIVE WORLD MODEL\n\nGenerator data is not available yet.");
	return FText::FromString(FString::Printf(TEXT("LIVE WORLD MODEL\n\nHOME STAR\n%s\n%s / %d K\n\nHOME PLANET\n%s\n%.0f KM\n\nSYSTEM\n%d PLANETS / %d MOONS\n\nCLUSTER\n%d STARS"),
		*World->HomeStarName.ToString(), *World->FullSpectralName.ToString(), World->HomeStarTemperature,
		*World->HomePlanetName.ToString(), World->PlanetRadius, World->PlanetsAmount, World->MoonsAmount, World->StarsAmount));
}

#undef LOCTEXT_NAMESPACE
