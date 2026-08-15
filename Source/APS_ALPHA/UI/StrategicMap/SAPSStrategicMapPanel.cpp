#include "SAPSStrategicMapPanel.h"

#include "APS_ALPHA/Actors/Astro/Galaxy.h"
#include "APS_ALPHA/Actors/Astro/Planet.h"
#include "APS_ALPHA/Actors/Astro/Star.h"
#include "APS_ALPHA/Actors/Astro/StarCluster.h"
#include "APS_ALPHA/Actors/Astro/StarSystem.h"
#include "APS_ALPHA/Core/Controllers/GravityPlayerController.h"
#include "APS_ALPHA/Core/Model/GeneratedWorld.h"
#include "APS_ALPHA/UI/Style/APSUIActionButton.h"
#include "APS_ALPHA/UI/Style/APSUIComponents.h"
#include "APS_ALPHA/UI/Style/APSUIStyle.h"
#include "InputCoreTypes.h"
#include "Styling/AppStyle.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "APSStrategicMap"

namespace APSMapUI
{
	const FAPSUIColorPalette& Palette()
	{
		static const FAPSUIColorPalette Value = FAPSUIStyle::GetPalette(FAPSUIStyle::GetRecommendedDisplayProfile());
		return Value;
	}

	FSlateFontInfo Font(const FName Typeface, const int32 Size)
	{
		return Typeface == TEXT("Regular")
			? FAPSUIStyle::BodyFont(Typeface, Size)
			: FAPSUIStyle::DisplayFont(Typeface, Size);
	}
}

void SAPSStrategicMapPanel::Construct(const FArguments& InArgs)
{
	Controller = InArgs._Controller;
	Generator = InArgs._Generator;
	OnClose = InArgs._OnClose;
	using namespace APSMapUI;
	const FAPSUIColorPalette& Colors = Palette();
	const FAPSUILayoutMetrics& Layout = FAPSUIStyle::Metrics();
	const auto GetWorld = [this]() -> const UGeneratedWorld*
	{
		const AAstroGenerator* Astro = Generator.Get();
		return Astro ? Astro->GetGeneratedWorldModel() : nullptr;
	};

	const auto FocusButton = [this](const FText& Glyph, const FText& Text, const FText& Details,
		const int32 Depth, const EAstroPreviewFocus FocusValue)
	{
		return SNew(SAPSUIHierarchyCard)
			.Glyph(Glyph)
			.Label(Text)
			.Details(Details)
			.Depth(Depth)
			.OnClicked(this, &SAPSStrategicMapPanel::Focus, static_cast<uint8>(FocusValue));
	};

	ChildSlot
	[
		SNew(SBorder).BorderImage(FAppStyle::GetBrush("WhiteBrush"))
		.BorderBackgroundColor(FLinearColor::Transparent).Padding(Layout.Space5)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().FillWidth(1.0f)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight()
					[
						SNew(STextBlock).Text(LOCTEXT("Title", "STRATEGIC MAP // LIVE UNIVERSE"))
						.Font(Font("Bold", 23)).ColorAndOpacity(Colors.TextPrimary)
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, Layout.Space1, 0.0f, 0.0f)
					[
						SNew(STextBlock).Text(LOCTEXT("Subtitle", "FULL-SCALE HIERARCHY  /  READ-ONLY  /  F10 TO CLOSE"))
						.Font(Font("Regular", 10)).ColorAndOpacity(Colors.FocusCyan)
					]
				]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
				[
					SNew(SBox).WidthOverride(170.0f)
					[
						SNew(SAPSUIActionButton)
						.Label(LOCTEXT("Close", "RETURN TO GAME"))
						.Shortcut(LOCTEXT("CloseShortcut", "F10"))
						.OnClicked(this, &SAPSStrategicMapPanel::Close)
					]
				]
			]
			+ SVerticalBox::Slot().FillHeight(1.0f).Padding(0.0f, Layout.Space4)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().Padding(0.0f, 0.0f, Layout.Space3, 0.0f)
				[
					SNew(SBox).WidthOverride(270.0f)
					[
						SNew(SBorder).BorderImage(FAppStyle::GetBrush("WhiteBrush"))
						.BorderBackgroundColor(Colors.Panel).Padding(Layout.Space3)
						[
							SNew(SVerticalBox)
							+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, Layout.Space2)
							[
								SNew(STextBlock).Text(LOCTEXT("HierarchyTitle", "CANONICAL HIERARCHY"))
								.Font(Font("Bold", 10)).ColorAndOpacity(Colors.TextSecondary)
							]
							+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, Layout.Space1)
							[
								FocusButton(LOCTEXT("OverviewGlyph", "ALL"), LOCTEXT("Overview", "UNIVERSE OVERVIEW"),
									LOCTEXT("OverviewDetails", "Full canonical universe"), 0, EAstroPreviewFocus::Overview)
							]
							+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, Layout.Space1)
							[
								FocusButton(LOCTEXT("ClusterGlyph", "CLS"), LOCTEXT("Cluster", "STAR CLUSTER"),
									LOCTEXT("ClusterDetails", "Generated home cluster"), 1, EAstroPreviewFocus::StarCluster)
							]
							+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, Layout.Space1)
							[
								FocusButton(LOCTEXT("GalaxyGlyph", "GAL"), LOCTEXT("Galaxy", "GALAXY"),
									LOCTEXT("GalaxyDetails", "Canonical home galaxy"), 2, EAstroPreviewFocus::Galaxy)
							]
							+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, Layout.Space1)
							[
								FocusButton(LOCTEXT("SystemGlyph", "SYS"), LOCTEXT("System", "HOME SYSTEM"),
									LOCTEXT("SystemDetails", "Planets and satellites"), 3, EAstroPreviewFocus::HomeSystem)
							]
							+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, Layout.Space1)
							[
								FocusButton(LOCTEXT("StarGlyph", "STR"), LOCTEXT("Star", "HOME STAR"),
									LOCTEXT("StarDetails", "Primary stellar body"), 4, EAstroPreviewFocus::HomeStar)
							]
							+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, Layout.Space1)
							[
								FocusButton(LOCTEXT("PlanetGlyph", "PLN"), LOCTEXT("Planet", "HOME PLANET"),
									LOCTEXT("PlanetDetails", "Planet and satellite group"), 4, EAstroPreviewFocus::HomePlanet)
							]
						]
					]
				]
				+ SHorizontalBox::Slot().FillWidth(1.0f)
				[
					SNew(SBorder).BorderImage(FAppStyle::GetBrush("WhiteBrush"))
					.BorderBackgroundColor(FLinearColor::Transparent).Padding(Layout.Space5)
					[
						SNew(STextBlock).Text(LOCTEXT("Help", "RMB DRAG  //  ORBIT\nMOUSE WHEEL  //  SCALE\nDOUBLE CLICK OBJECT  //  FOCUS"))
						.Justification(ETextJustify::Center).Font(Font("Bold", 11)).ColorAndOpacity(Colors.TextSecondary)
					]
				]
				+ SHorizontalBox::Slot().AutoWidth().Padding(Layout.Space3, 0.0f, 0.0f, 0.0f)
				[
					SNew(SBox).WidthOverride(300.0f)
					[
						SNew(SBorder).BorderImage(FAppStyle::GetBrush("WhiteBrush"))
						.BorderBackgroundColor(Colors.Panel).Padding(Layout.Space4)
						[
							SNew(SVerticalBox)
							+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, Layout.Space2)
							[
								SNew(STextBlock).Text(LOCTEXT("LiveModelTitle", "LIVE MODEL"))
								.Font(Font("Bold", 10)).ColorAndOpacity(Colors.FocusCyan)
							]
							+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, Layout.Space1)
							[
								SNew(SAPSUIFactChip)
								.Label(LOCTEXT("HomeStarLabel", "HOME STAR"))
								.Value_Lambda([GetWorld]()
								{
									const UGeneratedWorld* World = GetWorld();
									return World ? FText::FromName(World->HomeStarName) : LOCTEXT("UnavailableStar", "NOT AVAILABLE");
								})
							]
							+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, Layout.Space1)
							[
								SNew(SAPSUIFactChip)
								.Label(LOCTEXT("SpectrumLabel", "SPECTRUM / TEMPERATURE"))
								.Value_Lambda([GetWorld]()
								{
									const UGeneratedWorld* World = GetWorld();
									return World
										? FText::Format(LOCTEXT("SpectrumValue", "{0} / {1} K"),
											FText::FromName(World->FullSpectralName), FText::AsNumber(World->HomeStarTemperature))
										: LOCTEXT("UnavailableSpectrum", "NOT AVAILABLE");
								})
							]
							+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, Layout.Space1)
							[
								SNew(SAPSUIFactChip)
								.Label(LOCTEXT("HomePlanetLabel", "HOME PLANET"))
								.Value_Lambda([GetWorld]()
								{
									const UGeneratedWorld* World = GetWorld();
									return World
										? FText::Format(LOCTEXT("PlanetValue", "{0} / {1} KM"),
											FText::FromName(World->HomePlanetName), FText::AsNumber(FMath::RoundToInt(World->PlanetRadius)))
										: LOCTEXT("UnavailablePlanet", "NOT AVAILABLE");
								})
							]
							+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, Layout.Space1)
							[
								SNew(SAPSUIFactChip)
								.Label(LOCTEXT("SystemLabel", "PLANETS / SATELLITES"))
								.Value_Lambda([GetWorld]()
								{
									const UGeneratedWorld* World = GetWorld();
									return World
										? FText::Format(LOCTEXT("SystemValue", "{0} PLANETS / {1} MOONS"),
											FText::AsNumber(World->PlanetsAmount), FText::AsNumber(World->MoonsAmount))
										: LOCTEXT("UnavailableSystem", "NOT AVAILABLE");
								})
							]
							+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, Layout.Space1)
							[
								SNew(SAPSUIFactChip)
								.Label(LOCTEXT("ClusterLabel", "HOME CLUSTER"))
								.Value_Lambda([GetWorld]()
								{
									const UGeneratedWorld* World = GetWorld();
									return World
										? FText::Format(LOCTEXT("ClusterValue", "{0} STARS"), FText::AsNumber(World->StarsAmount))
										: LOCTEXT("UnavailableCluster", "NOT AVAILABLE");
								})
							]
						]
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
	{
		if (AAstroGenerator* Astro = Generator.Get()) Astro->BeginPreviewCameraOrbit();
		return FReply::Handled().CaptureMouse(SharedThis(this));
	}
	return FReply::Unhandled();
}

FReply SAPSStrategicMapPanel::OnMouseButtonUp(const FGeometry&, const FPointerEvent&)
{
	if (HasMouseCapture())
	{
		if (AAstroGenerator* Astro = Generator.Get()) Astro->EndPreviewCameraOrbit();
		return FReply::Handled().ReleaseMouseCapture();
	}
	return FReply::Unhandled();
}

void SAPSStrategicMapPanel::OnMouseCaptureLost(const FCaptureLostEvent& CaptureLostEvent)
{
	if (AAstroGenerator* Astro = Generator.Get()) Astro->EndPreviewCameraOrbit();
	SCompoundWidget::OnMouseCaptureLost(CaptureLostEvent);
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
