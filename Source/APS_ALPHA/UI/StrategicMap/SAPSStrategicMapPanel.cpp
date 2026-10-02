#include "SAPSStrategicMapPanel.h"

#include "APSStrategicMapCamera.h"
#include "APSStrategicMapScene.h"
#include "SAPSStrategicMapView.h"
#include "APS_ALPHA/Actors/Astro/APSBodyDesignation.h"
#include "APS_ALPHA/Actors/Astro/Moon.h"
#include "APS_ALPHA/Actors/Astro/Planet.h"
#include "APS_ALPHA/Actors/Astro/Star.h"
#include "APS_ALPHA/Actors/Tech/AutonomousOutpost.h"
#include "APS_ALPHA/Actors/Tech/Colony.h"
#include "APS_ALPHA/Actors/Tech/SpaceHeadquarters.h"
#include "APS_ALPHA/Actors/Tech/SpaceShipyard.h"
#include "APS_ALPHA/Actors/Tech/SpaceStation.h"
#include "APS_ALPHA/Core/Controllers/GravityPlayerController.h"
#include "APS_ALPHA/Gameplay/Expansion/APSInfrastructure.h"
#include "APS_ALPHA/Gameplay/Expansion/APSObjectActions.h"
#include "APS_ALPHA/Gameplay/Expansion/APSStarSystems.h"
#include "APS_ALPHA/Gameplay/Fleet/APSFleetCommand.h"
#include "APS_ALPHA/Generation/AstroGenerator.h"
#include "APS_ALPHA/Pawns/Spaceships/Spaceship.h"
#include "APS_ALPHA/UI/MainMenu/SAPSChamferedOverlay.h"
#include "APS_ALPHA/UI/Style/APSMenuChrome.h"
#include "APS_ALPHA/UI/Style/APSUINumber.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "GameFramework/Pawn.h"
#include "InputCoreTypes.h"
#include "Styling/AppStyle.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SWidgetSwitcher.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SNullWidget.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "APSStrategicMap"

namespace APSStrategicMapPanelLocal
{
	/** What the map keeps between two F10s in one world: the view, the layers, the selection and the tabs. */
	struct FMemory
	{
		TWeakObjectPtr<UWorld> World;
		FAPSStrategicMapCamera::FFocus Focus;
		double Distance{0.0};
		double Yaw{0.0};
		double Pitch{-35.0};
		uint32 LayerMask{0xFFFFFFFFu};
		bool bCleanView{false};
		APSStrategicMap::FSelection Selection;
		int32 Tab{0};
		int32 StarList{0};
		FString Title;
		bool bValid{false};
	};

	FMemory& Memory()
	{
		static FMemory Value;
		return Value;
	}

	FSlateFontInfo Readable(const FName Typeface, const int32 Size)
	{
		return FCoreStyle::GetDefaultFontStyle(Typeface, Size);
	}

	const FSlateBrush* TileBrush()
	{
		static const FSlateRoundedBoxBrush Brush(FLinearColor(0.005f, 0.028f, 0.044f, 0.98f), 6.0f,
			FLinearColor(0.035f, 0.23f, 0.31f, 0.88f), 1.0f);
		return &Brush;
	}

	/**
	 * The menu's chamfered button (the colony terminal's look): a card whose frame lights up on hover or when selected,
	 * dim when it cannot be used.
	 */
	TSharedRef<SWidget> MapButton(const TSharedRef<SWidget>& Content, const FOnClicked& OnClicked, const TAttribute<bool>& IsSelected,
		const FLinearColor& Accent, const TAttribute<bool>& IsEnabled = TAttribute<bool>(true),
		const FMargin& Padding = FMargin(10.0f, 7.0f))
	{
		const TSharedRef<SButton> Button = SNew(SButton)
			.ButtonStyle(FAppStyle::Get(), "NoBorder")
			.ContentPadding(0.0f)
			.IsEnabled(IsEnabled)
			.OnClicked(OnClicked);
		const TWeakPtr<SButton> WeakButton = Button;
		Button->SetContent(
			SNew(SAPSChamferedOverlay)
			+ SOverlay::Slot()
			[
				SNew(SAPSChamferedSurface)
				.Brush(FAppStyle::GetBrush("WhiteBrush"))
				.Tint_Lambda([IsSelected, IsEnabled]()
				{
					if (!IsEnabled.Get(true))
					{
						return FLinearColor(0.02f, 0.05f, 0.065f, 0.92f);
					}
					return IsSelected.Get(false) ? FLinearColor(0.02f, 0.13f, 0.17f, 0.97f) : APSChrome::Panel();
				})
				.ChamferTop(true)
				.ChamferBottom(true)
			]
			+ SOverlay::Slot().Padding(Padding)
			[
				Content
			]
			+ SOverlay::Slot()
			[
				SNew(SAPSChamferedFrame)
				.Thickness(1.0f)
				.Color_Lambda([WeakButton, IsSelected, IsEnabled, Accent]()
				{
					const TSharedPtr<SButton> Pinned = WeakButton.Pin();
					if (!IsEnabled.Get(true))
					{
						return FLinearColor(0.1f, 0.18f, 0.21f, 0.8f);
					}
					return IsSelected.Get(false) || (Pinned && Pinned->IsHovered()) ? Accent : APSChrome::CyanDim();
				})
			]);
		return Button;
	}

	TSharedRef<SWidget> MapChip(const FText& Text, const FLinearColor& Colour)
	{
		return SNew(SBorder)
			.BorderImage(FAppStyle::GetBrush("WhiteBrush"))
			.BorderBackgroundColor(FLinearColor(Colour.R, Colour.G, Colour.B, 0.16f))
			.Padding(FMargin(6.0f, 1.0f))
			[
				SNew(STextBlock).Text(Text).Font(Readable("Bold", 9)).ColorAndOpacity(Colour)
			];
	}

	TSharedRef<SWidget> SectionLabel(const FText& Text, const FLinearColor& Colour)
	{
		return SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.0f, 0.0f, 8.0f, 0.0f)
			[
				SNew(SBox).WidthOverride(3.0f).HeightOverride(13.0f)
				[
					SNew(SBorder).BorderImage(FAppStyle::GetBrush("WhiteBrush")).BorderBackgroundColor(Colour)
				]
			]
			+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center)
			[
				SNew(STextBlock).Text(Text).Font(APSChrome::Font("Bold", 10)).ColorAndOpacity(Colour)
			];
	}

	TSharedRef<SWidget> KeyHint(const FText& Keys, const FText& What)
	{
		return SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
			[
				SNew(SBox).MinDesiredWidth(96.0f).HAlign(HAlign_Left)
				[
					APSChrome::KeyChip(Keys, APSChrome::Cyan())
				]
			]
			+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center).Padding(8.0f, 0.0f, 0.0f, 0.0f)
			[
				SNew(STextBlock).Text(What).Font(APSChrome::Font("Regular", 11)).ColorAndOpacity(APSChrome::Muted())
			];
	}

	TSharedRef<SWidget> Swatch(const FLinearColor& Colour, const FText& Text)
	{
		return SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.0f, 0.0f, 6.0f, 0.0f)
			[
				SNew(SBox).WidthOverride(9.0f).HeightOverride(9.0f)
				[
					SNew(SBorder).BorderImage(FAppStyle::GetBrush("WhiteBrush")).BorderBackgroundColor(Colour)
				]
			]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.0f, 0.0f, 10.0f, 0.0f)
			[
				SNew(STextBlock).Text(Text).Font(Readable("Bold", 9)).ColorAndOpacity(APSChrome::Muted())
			];
	}

	FText AuText(const double Cm)
	{
		FNumberFormattingOptions Options;
		const double Au = Cm / APSStars::AstronomicalUnitCm;
		Options.SetMaximumFractionalDigits(Au >= 100.0 ? 0 : Au >= 10.0 ? 1 : 2);
		Options.SetMinimumFractionalDigits(Au >= 100.0 ? 0 : 1);
		return FText::Format(LOCTEXT("AuValue", "{0} AU"), APSUINumber::Number(Au, &Options));
	}

	EAPSChromeGlyph GlyphOf(const AActor* Actor, const bool bSystem)
	{
		if (bSystem)
		{
			return EAPSChromeGlyph::System;
		}
		if (!Actor)
		{
			return EAPSChromeGlyph::Compass;
		}
		if (Actor->IsA<AStar>()) return EAPSChromeGlyph::Favorite;
		if (Actor->IsA<AMoon>()) return EAPSChromeGlyph::World;
		if (Actor->IsA<APlanet>()) return EAPSChromeGlyph::Planet;
		if (Actor->IsA<ASpaceHeadquarters>()) return EAPSChromeGlyph::Headquarters;
		if (Actor->IsA<ASpaceShipyard>()) return EAPSChromeGlyph::Shipyard;
		if (Actor->IsA<ASpaceStation>()) return EAPSChromeGlyph::Station;
		if (Actor->IsA<AAutonomousOutpost>()) return EAPSChromeGlyph::Infrastructure;
		if (Actor->IsA<AColony>()) return EAPSChromeGlyph::Civilization;
		if (Actor->IsA<ASpaceship>()) return EAPSChromeGlyph::Ship;
		if (Actor->IsA<APawn>()) return EAPSChromeGlyph::Pilot;
		return EAPSChromeGlyph::Compass;
	}
}

void SAPSStrategicMapPanel::Construct(const FArguments& InArgs)
{
	Controller = InArgs._Controller;
	Generator = InArgs._Generator;
	OnClose = InArgs._OnClose;
	AGravityPlayerController* PlayerController = Controller.Get();
	UWorld* World = PlayerController ? PlayerController->GetWorld() : nullptr;

	Scene = MakeShared<FAPSStrategicMapScene>(World, Generator.Get());
	Camera = MakeShared<FAPSStrategicMapCamera>();
	const TWeakPtr<FAPSStrategicMapScene> WeakScene = Scene;
	Camera->SetSystemLocator([WeakScene](const int32 Index, FVector& OutLocation)
	{
		const TSharedPtr<FAPSStrategicMapScene> Pinned = WeakScene.Pin();
		return Pinned.IsValid() && Pinned->LocateSystem(Index, OutLocation);
	});
	const APawn* Pilot = Scene->GetPilot();
	Scene->Update(0.0f, Pilot ? Pilot->GetActorLocation() : FVector::ZeroVector, APSStars::AstronomicalUnitCm);
	TArray<TPair<TWeakObjectPtr<AActor>, double>> Obstacles;
	Scene->GetObstacles(Obstacles);
	Camera->SetObstacles(MoveTemp(Obstacles));
	ObstaclesSerial = Scene->GetObjectsSerial();
	// Rio 02.10 ("HOME SYSTEM shows nothing, the camera falls into the star; the planet is see-through; FPS drops when
	// rotating"): the map's own camera over the live world; the generator's preview presentation is never touched.
	Camera->Begin(PlayerController, Scene->GetReference(), Scene->GetFrameUp());

	SearchStyle = MakeShared<FEditableTextBoxStyle>(FCoreStyle::Get().GetWidgetStyle<FEditableTextBoxStyle>("NormalEditableTextBox"));
	const FSlateRoundedBoxBrush Field(FLinearColor(0.003f, 0.016f, 0.028f, 0.96f), 6.0f, APSChrome::CyanDim(), 1.0f);
	const FSlateRoundedBoxBrush FieldActive(FLinearColor(0.006f, 0.032f, 0.048f, 0.98f), 6.0f, APSChrome::Cyan(), 1.2f);
	SearchStyle->SetBackgroundImageNormal(Field)
		.SetBackgroundImageHovered(FieldActive)
		.SetBackgroundImageFocused(FieldActive)
		.SetBackgroundImageReadOnly(Field)
		.SetForegroundColor(FSlateColor(APSChrome::White()))
		.SetFocusedForegroundColor(FSlateColor(APSChrome::White()))
		.SetBackgroundColor(FSlateColor(FLinearColor::White))
		.SetFont(FCoreStyle::GetDefaultFontStyle("Bold", 12))
		.SetPadding(FMargin(10.0f, 7.0f));

	ChildSlot
	[
		SNew(SBorder)
		.BorderImage(FAppStyle::GetBrush("NoBorder"))
		.Padding(FMargin(16.0f, 14.0f))
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()
			[
				BuildHeader()
			]
			+ SVerticalBox::Slot().FillHeight(1.0f).Padding(0.0f, 12.0f, 0.0f, 0.0f)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth()
				[
					SNew(SBox).WidthOverride(262.0f)
					[
						BuildLeftPanel()
					]
				]
				+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(12.0f, 0.0f)
				[
					SAssignNew(View, SAPSStrategicMapView)
					.Controller(TWeakObjectPtr<APlayerController>(PlayerController))
					.Scene(Scene)
					.Camera(Camera)
					.OnSelect_Lambda([this](const APSStrategicMap::FSelection& Target) { Select(Target, true); })
					.OnFocus_Lambda([this](const APSStrategicMap::FSelection& Target) { FocusOn(Target, TOptional<double>()); })
				]
				+ SHorizontalBox::Slot().AutoWidth()
				[
					SNew(SBox).WidthOverride(392.0f)
					[
						BuildRightPanel()
					]
				]
			]
		]
	];
	// The first flight waits for the view's first paint: only then is the map region known, and with it the framing.
}

SAPSStrategicMapPanel::~SAPSStrategicMapPanel()
{
	if (!bClosing)
	{
		Remember();
	}
	if (Camera.IsValid())
	{
		Camera->End();
	}
}

void SAPSStrategicMapPanel::OpenView()
{
	const APSStrategicMapPanelLocal::FMemory& Saved = APSStrategicMapPanelLocal::Memory();
	UWorld* World = Scene->GetWorld();
	if (Saved.bValid && World && Saved.World.Get() == World)
	{
		// Rio 02.10: the map opens where it was left (the view, the layers, the selection), flying out of the pilot's view.
		Scene->SetLayerMask(Saved.LayerMask);
		Scene->SetCleanView(Saved.bCleanView);
		Scene->Select(Saved.Selection);
		ActiveTab = Saved.Tab;
		StarList = Saved.StarList;
		const bool bFocusAlive = !Saved.Focus.bOnActor || Saved.Focus.Actor.IsValid();
		if (bFocusAlive && Saved.Distance > 0.0)
		{
			Camera->FlyToDistance(Saved.Focus, Saved.Distance, Saved.Pitch, Saved.Yaw);
			FocusTitle = FText::FromString(Saved.Title);
			return;
		}
	}
	// The first time: the star system the pilot is in.
	FAPSStarSystems* Stars = Scene->GetStars();
	const APawn* Pilot = Scene->GetPilot();
	const int32 Containing = Stars && Stars->IsReady() && Pilot ? Stars->FindContaining(Pilot->GetActorLocation()) : INDEX_NONE;
	if (Containing != INDEX_NONE && Containing != Stars->GetHomeIndex())
	{
		FocusOn(APSStrategicMap::FSelection::OfSystem(Containing), -38.0);
		ActivePreset = EPreset::None;
		return;
	}
	ApplyPreset(EPreset::HomeSystem);
}

void SAPSStrategicMapPanel::Remember() const
{
	APSStrategicMapPanelLocal::FMemory& Saved = APSStrategicMapPanelLocal::Memory();
	// Closed before the first flight: the camera still sits at the pilot's view, nothing worth keeping.
	if (!bOpened || !Scene.IsValid() || !Camera.IsValid() || !Camera->IsActive())
	{
		return;
	}
	Saved.World = Scene->GetWorld();
	Saved.Focus = Camera->GetFocus();
	Saved.Distance = Camera->GetTargetDistance();
	Saved.Yaw = Camera->GetTargetYaw();
	Saved.Pitch = Camera->GetTargetPitch();
	Saved.LayerMask = Scene->GetLayerMask();
	Saved.bCleanView = Scene->IsCleanView();
	Saved.Selection = Scene->GetSelection();
	Saved.Tab = ActiveTab;
	Saved.StarList = StarList;
	Saved.Title = FocusTitle.ToString();
	Saved.bValid = true;
}

FReply SAPSStrategicMapPanel::Close()
{
	if (!bClosing)
	{
		Remember();
		bClosing = true;
		OnClose.ExecuteIfBound();
	}
	return FReply::Handled();
}

FReply SAPSStrategicMapPanel::OnPreviewKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent)
{
	// F10 and Esc close the map from anywhere on it, the search box included.
	if (InKeyEvent.GetKey() == EKeys::F10 || InKeyEvent.GetKey() == EKeys::Escape)
	{
		return Close();
	}
	return SCompoundWidget::OnPreviewKeyDown(MyGeometry, InKeyEvent);
}

FReply SAPSStrategicMapPanel::OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent)
{
	if (SearchBox.IsValid() && SearchBox->HasKeyboardFocus())
	{
		return FReply::Unhandled();
	}
	const FKey Key = InKeyEvent.GetKey();
	if (Key == EKeys::F) return ApplyPreset(EPreset::Selection);
	if (Key == EKeys::H) return ApplyPreset(EPreset::HomeSystem);
	if (Key == EKeys::C) return ApplyPreset(EPreset::Cluster);
	if (Key == EKeys::P) return ApplyPreset(EPreset::HomePlanet);
	if (Key == EKeys::M) return ApplyPreset(EPreset::MyShip);
	if (Key == EKeys::L && Scene.IsValid())
	{
		Scene->SetCleanView(!Scene->IsCleanView());
		return FReply::Handled();
	}
	return SCompoundWidget::OnKeyDown(MyGeometry, InKeyEvent);
}

FReply SAPSStrategicMapPanel::ApplyPreset(const EPreset Preset)
{
	if (!Scene.IsValid() || !Camera.IsValid())
	{
		return FReply::Handled();
	}
	switch (Preset)
	{
	case EPreset::Cluster:
	{
		// Home among its neighbours, seen from high over the ecliptic.
		FAPSStrategicMapCamera::FFocus Focus;
		if (AActor* Star = Scene->GetHomeStar())
		{
			Focus.Actor = Star;
			Focus.bOnActor = true;
		}
		else if (const FAPSStarSystems* Stars = Scene->GetStars(); Stars && Stars->GetHomeIndex() != INDEX_NONE)
		{
			Focus.SystemIndex = Stars->GetHomeIndex();
		}
		Focus.RadiusCm = Scene->GetHomeRoomCm() * 0.05;
		Camera->FlyTo(Focus, Scene->GetClusterFrameRadius(), -55.0);
		FocusTitle = LOCTEXT("ClusterTitle", "HOME STAR CLUSTER");
		break;
	}
	case EPreset::HomeSystem:
	{
		// The whole home system in the view: its sphere, not the star (Rio 02.10: "the camera falls into the star").
		FAPSStrategicMapCamera::FFocus Focus;
		AActor* Star = Scene->GetHomeStar();
		if (Star)
		{
			Focus.Actor = Star;
			Focus.bOnActor = true;
			const int32 Index = Scene->FindObject(Star);
			Focus.RadiusCm = Scene->GetObjects().IsValidIndex(Index) ? Scene->GetObjects()[Index].RadiusCm : 0.0;
		}
		Camera->FlyTo(Focus, Scene->GetHomeRoomCm(), -38.0);
		FocusTitle = Star ? FText::Format(LOCTEXT("HomeSystemTitle", "HOME SYSTEM  /  {0}"), Scene->NameOf(
			APSStrategicMap::FSelection::OfActor(Star))) : LOCTEXT("HomeSystemPlain", "HOME SYSTEM");
		break;
	}
	case EPreset::HomePlanet:
		if (APlanet* Planet = Scene->GetHomePlanet())
		{
			FocusOn(APSStrategicMap::FSelection::OfActor(Planet), -22.0);
		}
		break;
	case EPreset::MyShip:
		if (ASpaceship* Ship = Scene->GetMyShip())
		{
			FocusOn(APSStrategicMap::FSelection::OfActor(Ship), -18.0);
		}
		break;
	case EPreset::Selection:
		if (Scene->GetSelection().IsSet())
		{
			FocusOn(Scene->GetSelection(), TOptional<double>());
		}
		break;
	default:
		break;
	}
	ActivePreset = Preset;
	return FReply::Handled();
}

void SAPSStrategicMapPanel::FocusOn(const APSStrategicMap::FSelection& Target, const TOptional<double> Pitch)
{
	if (!Scene.IsValid() || !Camera.IsValid() || !Target.IsSet())
	{
		return;
	}
	FAPSStrategicMapCamera::FFocus Focus;
	if (AActor* Actor = Target.Actor.Get())
	{
		Focus.Actor = Actor;
		Focus.bOnActor = true;
	}
	else
	{
		Focus.SystemIndex = Target.SystemIndex;
	}
	Focus.RadiusCm = Scene->FocusRadius(Target);
	FVector Up;
	double BodyRadius = 0.0;
	FVector Location;
	if (!Pitch.IsSet() && Scene->SurfaceUp(Target, Up, BodyRadius) && Scene->Locate(Target, Location))
	{
		// On the ground (a colony, an anomaly site, a landed ship, the pilot): seen from above it, leaning toward where
		// the camera is now, with some of the land round it; the ecliptic's "down" would look at it through the planet.
		FVector Lean = Camera->GetCameraLocation() - Location;
		Lean = (Lean - Up * FVector::DotProduct(Lean, Up)).GetSafeNormal();
		if (Lean.IsNearlyZero())
		{
			Lean = FVector::CrossProduct(Up, FMath::Abs(Up.Z) < 0.9 ? FVector::UpVector : FVector::ForwardVector).GetSafeNormal();
		}
		const double Frame = FMath::Max(Scene->FrameRadius(Target), FMath::Min(BodyRadius * 0.02, 5000000.0));
		Camera->FlyToFrom(Focus, Frame, (Up * 0.8 + Lean * 0.6).GetSafeNormal());
	}
	else
	{
		Camera->FlyTo(Focus, Scene->FrameRadius(Target), Pitch);
	}
	FocusTitle = Scene->NameOf(Target);
	ActivePreset = EPreset::None;
}

void SAPSStrategicMapPanel::Select(const APSStrategicMap::FSelection& Target, const bool bShowObject)
{
	if (!Scene.IsValid())
	{
		return;
	}
	Scene->Select(Target);
	if (bShowObject && Target.IsSet())
	{
		SetTab(1);
	}
}

FReply SAPSStrategicMapPanel::SetTab(const int32 Tab)
{
	ActiveTab = FMath::Clamp(Tab, 0, 1);
	return FReply::Handled();
}

uint32 SAPSStrategicMapPanel::RuntimeRevision() const
{
	UWorld* World = Scene.IsValid() ? Scene->GetWorld() : nullptr;
	const FAPSFleetCommand* Fleet = APSFleetFind(World);
	const FAPSInfrastructure* Infrastructure = APSInfrastructureFind(World);
	const FAPSStarSystems* Stars = APSStarSystemsFind(World);
	uint32 Hash = GetTypeHash(Fleet ? Fleet->GetRevision() : 0u);
	Hash = HashCombine(Hash, GetTypeHash(Infrastructure ? Infrastructure->GetRevision() : 0u));
	return HashCombine(Hash, GetTypeHash(Stars ? Stars->GetRevision() : 0u));
}

void SAPSStrategicMapPanel::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
	SCompoundWidget::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);
	if (!Scene.IsValid() || !Camera.IsValid())
	{
		return;
	}
	Scene->Update(InDeltaTime, Camera->GetLookLocation(), Camera->GetDistance());
	if (!bOpened)
	{
		OpenWait += InDeltaTime;
		if (Camera->HasRegion() || OpenWait > 0.5f)
		{
			bOpened = true;
			OpenView();
		}
	}
	if (Scene->GetObjectsSerial() != ObstaclesSerial)
	{
		ObstaclesSerial = Scene->GetObjectsSerial();
		TArray<TPair<TWeakObjectPtr<AActor>, double>> Obstacles;
		Scene->GetObstacles(Obstacles);
		Camera->SetObstacles(MoveTemp(Obstacles));
	}
	UpdateStarList(InDeltaTime);
	UpdateObjectPage(InDeltaTime);
}

TSharedRef<SWidget> SAPSStrategicMapPanel::BuildHeader()
{
	namespace Local = APSStrategicMapPanelLocal;
	// The civilization's stocks and what they gain a minute (FAPSInfrastructure).
	const TSharedRef<SHorizontalBox> Stocks = SNew(SHorizontalBox);
	for (int32 Index = 0; Index < static_cast<int32>(APSInfrastructure::EResource::Count); ++Index)
	{
		const APSInfrastructure::EResource Resource = static_cast<APSInfrastructure::EResource>(Index);
		const FLinearColor Colour = APSInfrastructure::ResourceColour(Resource);
		Stocks->AddSlot().AutoWidth().Padding(Index > 0 ? 6.0f : 0.0f, 0.0f, 0.0f, 0.0f)
		[
			SNew(SBorder).BorderImage(Local::TileBrush()).Padding(FMargin(10.0f, 5.0f, 10.0f, 6.0f))
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(STextBlock).Text(APSInfrastructure::ResourceName(Resource)).Font(Local::Readable("Bold", 9))
					.ColorAndOpacity(APSChrome::Muted())
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 2.0f, 0.0f, 0.0f)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Bottom)
					[
						SNew(STextBlock).Font(APSChrome::Font("Bold", 13)).ColorAndOpacity(Colour)
						.Text_Lambda([this, Resource]()
						{
							const FAPSInfrastructure* Infrastructure = APSInfrastructureFind(Scene.IsValid() ? Scene->GetWorld() : nullptr);
							return Infrastructure
								? APSUINumber::Number(FMath::FloorToInt64(Infrastructure->GetStock(Resource)))
								: FText::FromString(TEXT("-"));
						})
					]
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Bottom).Padding(6.0f, 0.0f, 0.0f, 1.0f)
					[
						SNew(STextBlock).Font(Local::Readable("Regular", 10)).ColorAndOpacity(APSChrome::Muted())
						.Text_Lambda([this, Resource]()
						{
							const FAPSInfrastructure* Infrastructure = APSInfrastructureFind(Scene.IsValid() ? Scene->GetWorld() : nullptr);
							if (!Infrastructure)
							{
								return FText::GetEmpty();
							}
							const float Rate = Infrastructure->GetRate(Resource);
							FNumberFormattingOptions Options;
							Options.SetMaximumFractionalDigits(FMath::Abs(Rate) < 10.0f ? 1 : 0);
							return FText::Format(LOCTEXT("RatePerMinute", "{0}{1} / MIN"),
								FText::FromString(Rate < 0.0f ? TEXT("-") : TEXT("+")), APSUINumber::Number(FMath::Abs(Rate), &Options));
						})
					]
				]
			]
		];
	}
	return APSChrome::ChamferPanel(
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
		[
			APSChrome::IconBadge(EAPSChromeGlyph::Compass, APSChrome::Cyan(), 40.0f)
		]
		+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center).Padding(12.0f, 0.0f)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Bottom)
				[
					SNew(STextBlock).Text(LOCTEXT("Title", "STRATEGIC MAP")).Font(APSChrome::Font("Bold", 18))
					.ColorAndOpacity(APSChrome::White())
				]
				+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Bottom).Padding(14.0f, 0.0f, 0.0f, 2.0f)
				[
					SNew(STextBlock).Font(Local::Readable("Bold", 12)).ColorAndOpacity(APSChrome::Cyan())
					.Text_Lambda([this]()
					{
						return FocusTitle.IsEmpty() ? FText::GetEmpty()
							: FText::Format(LOCTEXT("FocusLine", "FOCUS  /  {0}"), FocusTitle);
					})
				]
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 4.0f, 0.0f, 0.0f)
			[
				SNew(STextBlock).Font(APSChrome::Font("Regular", 11)).ColorAndOpacity(APSChrome::Muted())
				.Text(LOCTEXT("HintLine", "RMB DRAG  ORBIT  /  WHEEL  ZOOM  /  MMB OR SHIFT+RMB  PAN  /  CLICK  SELECT  /  DOUBLE-CLICK  FLY TO"))
			]
		]
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
		[
			Stocks
		]
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(14.0f, 0.0f, 0.0f, 0.0f)
		[
			SNew(SBox).MinDesiredWidth(150.0f)
			[
				Local::MapButton(
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center)
					[
						SNew(STextBlock).Text(LOCTEXT("Return", "RETURN")).Font(APSChrome::Font("Bold", 11))
						.ColorAndOpacity(APSChrome::White())
					]
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(10.0f, 0.0f, 0.0f, 0.0f)
					[
						APSChrome::KeyChip(LOCTEXT("ReturnKey", "F10"), APSChrome::Amber())
					],
					FOnClicked::CreateSP(this, &SAPSStrategicMapPanel::Close), TAttribute<bool>(false), APSChrome::Amber())
			]
		],
		FMargin(16.0f, 10.0f), APSChrome::CyanDim());
}

TSharedRef<SWidget> SAPSStrategicMapPanel::PresetButton(const EPreset Preset, const int32 Glyph, const FText& Label,
	const FText& Detail)
{
	namespace Local = APSStrategicMapPanelLocal;
	TAttribute<bool> Enabled(true);
	if (Preset == EPreset::Selection)
	{
		Enabled = TAttribute<bool>::CreateLambda([this]() { return Scene.IsValid() && Scene->GetSelection().IsSet(); });
	}
	return Local::MapButton(
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
		[
			APSChrome::IconBadge(static_cast<EAPSChromeGlyph>(Glyph), APSChrome::Cyan(), 28.0f)
		]
		+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center).Padding(10.0f, 0.0f, 0.0f, 0.0f)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(STextBlock).Text(Label).Font(APSChrome::Font("Bold", 11)).ColorAndOpacity(APSChrome::White())
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 1.0f, 0.0f, 0.0f)
			[
				SNew(STextBlock).Text(Detail).Font(APSChrome::Font("Regular", 11)).ColorAndOpacity(APSChrome::Muted())
				.AutoWrapText(true)
			]
		],
		FOnClicked::CreateSP(this, &SAPSStrategicMapPanel::ApplyPreset, Preset),
		TAttribute<bool>::CreateLambda([this, Preset]() { return ActivePreset == Preset; }), APSChrome::Amber(), Enabled,
		FMargin(8.0f, 6.0f));
}

TSharedRef<SWidget> SAPSStrategicMapPanel::LayerToggle(const APSStrategicMap::ELayer Layer, const FText& Label,
	const FLinearColor& Swatch)
{
	namespace Local = APSStrategicMapPanelLocal;
	const auto IsOn = [this, Layer]() { return Scene.IsValid() && Scene->IsLayerOn(Layer); };
	return Local::MapButton(
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
		[
			SNew(SBox).WidthOverride(12.0f).HeightOverride(12.0f)
			[
				SNew(SBorder).BorderImage(FAppStyle::GetBrush("WhiteBrush"))
				.BorderBackgroundColor_Lambda([IsOn, Swatch]()
				{
					return IsOn() ? FSlateColor(Swatch) : FSlateColor(FLinearColor(0.05f, 0.1f, 0.12f, 1.0f));
				})
			]
		]
		+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center).Padding(10.0f, 0.0f, 0.0f, 0.0f)
		[
			SNew(STextBlock).Text(Label).Font(APSChrome::Font("Bold", 10))
			.ColorAndOpacity_Lambda([IsOn]() { return FSlateColor(IsOn() ? APSChrome::White() : APSChrome::Muted()); })
		]
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
		[
			SNew(STextBlock).Font(Local::Readable("Bold", 9))
			.Text_Lambda([IsOn]() { return IsOn() ? LOCTEXT("LayerOn", "ON") : LOCTEXT("LayerOff", "OFF"); })
			.ColorAndOpacity_Lambda([IsOn]() { return FSlateColor(IsOn() ? APSChrome::Cyan() : APSChrome::Muted()); })
		],
		FOnClicked::CreateLambda([this, Layer]()
		{
			if (Scene.IsValid())
			{
				Scene->SetLayer(Layer, !Scene->IsLayerOn(Layer));
			}
			return FReply::Handled();
		}),
		TAttribute<bool>::CreateLambda(IsOn), Swatch, TAttribute<bool>(true), FMargin(9.0f, 5.0f));
}

TSharedRef<SWidget> SAPSStrategicMapPanel::BuildLeftPanel()
{
	namespace Local = APSStrategicMapPanelLocal;
	using APSStrategicMap::ELayer;
	const TSharedRef<SVerticalBox> Box = SNew(SVerticalBox);
	Box->AddSlot().AutoHeight()
	[
		APSChrome::IconSectionHeading(EAPSChromeGlyph::Compass, LOCTEXT("ViewHeading", "VIEW"),
			LOCTEXT("ViewSubheading", "Where the camera flies"))
	];
	const auto AddPreset = [this, &Box](const EPreset Preset, const EAPSChromeGlyph Glyph, const FText& Label, const FText& Detail)
	{
		Box->AddSlot().AutoHeight().Padding(0.0f, 6.0f, 0.0f, 0.0f)
		[
			PresetButton(Preset, static_cast<int32>(Glyph), Label, Detail)
		];
	};
	AddPreset(EPreset::Cluster, EAPSChromeGlyph::Space, LOCTEXT("PresetCluster", "CLUSTER"),
		LOCTEXT("PresetClusterDetail", "Home among its neighbour stars  /  C"));
	AddPreset(EPreset::HomeSystem, EAPSChromeGlyph::System, LOCTEXT("PresetSystem", "HOME SYSTEM"),
		LOCTEXT("PresetSystemDetail", "Star, planets, moons, orbits  /  H"));
	AddPreset(EPreset::HomePlanet, EAPSChromeGlyph::Planet, LOCTEXT("PresetPlanet", "HOME PLANET"),
		LOCTEXT("PresetPlanetDetail", "Its moons, stations and ships  /  P"));
	AddPreset(EPreset::MyShip, EAPSChromeGlyph::Ship, LOCTEXT("PresetShip", "MY SHIP"),
		LOCTEXT("PresetShipDetail", "The ship you fly, else the flagship  /  M"));
	AddPreset(EPreset::Selection, EAPSChromeGlyph::Favorite, LOCTEXT("PresetSelection", "SELECTION"),
		LOCTEXT("PresetSelectionDetail", "Fly to what is selected  /  F"));

	Box->AddSlot().AutoHeight().Padding(0.0f, 16.0f, 0.0f, 0.0f)
	[
		APSChrome::IconSectionHeading(EAPSChromeGlyph::Collection, LOCTEXT("LayersHeading", "LAYERS"),
			LOCTEXT("LayersSubheading", "What the map draws"))
	];
	// Rio 02.10: one switch for "just space": no labels, rings, orbits or routes; the layers below keep their settings.
	{
		const auto IsClean = [this]() { return Scene.IsValid() && Scene->IsCleanView(); };
		Box->AddSlot().AutoHeight().Padding(0.0f, 5.0f, 0.0f, 0.0f)
		[
			Local::MapButton(
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center)
				[
					SNew(STextBlock).Text(LOCTEXT("CleanView", "ALL MARKS  /  L")).Font(APSChrome::Font("Bold", 10))
					.ColorAndOpacity_Lambda([IsClean]() { return FSlateColor(IsClean() ? APSChrome::Muted() : APSChrome::White()); })
				]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
				[
					SNew(STextBlock).Font(Local::Readable("Bold", 9))
					.Text_Lambda([IsClean]() { return IsClean() ? LOCTEXT("CleanViewHidden", "HIDDEN") : LOCTEXT("CleanViewShown", "SHOWN"); })
					.ColorAndOpacity_Lambda([IsClean]() { return FSlateColor(IsClean() ? APSChrome::Amber() : APSChrome::Cyan()); })
				],
				FOnClicked::CreateLambda([this]()
				{
					if (Scene.IsValid())
					{
						Scene->SetCleanView(!Scene->IsCleanView());
					}
					return FReply::Handled();
				}),
				TAttribute<bool>::CreateLambda(IsClean), APSChrome::Amber(), TAttribute<bool>(true), FMargin(9.0f, 5.0f))
		];
	}
	const auto AddLayer = [this, &Box](const ELayer Layer, const FText& Label, const FLinearColor& Colour)
	{
		Box->AddSlot().AutoHeight().Padding(0.0f, 5.0f, 0.0f, 0.0f)
		[
			LayerToggle(Layer, Label, Colour)
		];
	};
	AddLayer(ELayer::Labels, LOCTEXT("LayerLabels", "LABELS"), APSChrome::White());
	AddLayer(ELayer::Orbits, LOCTEXT("LayerOrbits", "ORBITS"), APSChrome::Cyan());
	AddLayer(ELayer::Routes, LOCTEXT("LayerRoutes", "ROUTES"), APSFleet::DivisionColour(APSFleet::EDivision::Construction));
	AddLayer(ELayer::Network, LOCTEXT("LayerNetwork", "NETWORK"), APSChrome::Success());
	AddLayer(ELayer::Ships, LOCTEXT("LayerShips", "SHIPS"), APSFleet::DivisionColour(APSFleet::EDivision::MainFleet));
	AddLayer(ELayer::Stations, LOCTEXT("LayerStations", "STATIONS"), APSChrome::Amber());
	AddLayer(ELayer::Systems, LOCTEXT("LayerSystems", "STAR SYSTEMS"), APSStars::KnowledgeColour(APSStars::EKnowledge::Scanned));

	// Legend: the colours the view uses.
	Box->AddSlot().AutoHeight().Padding(0.0f, 16.0f, 0.0f, 6.0f)
	[
		Local::SectionLabel(LOCTEXT("LegendShips", "SHIPS BY DIVISION"), APSChrome::Muted())
	];
	{
		const TSharedRef<SVerticalBox> Divisions = SNew(SVerticalBox);
		for (int32 Index = 0; Index < static_cast<int32>(APSFleet::EDivision::Count); ++Index)
		{
			const APSFleet::EDivision Division = static_cast<APSFleet::EDivision>(Index);
			Divisions->AddSlot().AutoHeight().Padding(0.0f, 1.0f)
			[
				Local::Swatch(APSFleet::DivisionColour(Division), APSFleet::DivisionName(Division))
			];
		}
		Box->AddSlot().AutoHeight()[Divisions];
	}
	Box->AddSlot().AutoHeight().Padding(0.0f, 10.0f, 0.0f, 6.0f)
	[
		Local::SectionLabel(LOCTEXT("LegendSystems", "STAR SYSTEMS"), APSChrome::Muted())
	];
	Box->AddSlot().AutoHeight()
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 1.0f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth()[Local::Swatch(APSStars::KnowledgeColour(APSStars::EKnowledge::Catalogued),
				APSStars::KnowledgeName(APSStars::EKnowledge::Catalogued))]
			+ SHorizontalBox::Slot().AutoWidth()[Local::Swatch(APSStars::KnowledgeColour(APSStars::EKnowledge::Scanned),
				APSStars::KnowledgeName(APSStars::EKnowledge::Scanned))]
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 1.0f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth()[Local::Swatch(APSStars::KnowledgeColour(APSStars::EKnowledge::Surveyed),
				APSStars::KnowledgeName(APSStars::EKnowledge::Surveyed))]
			+ SHorizontalBox::Slot().AutoWidth()[Local::Swatch(APSChrome::Success(), LOCTEXT("LegendClaimed", "CLAIMED"))]
			+ SHorizontalBox::Slot().AutoWidth()[Local::Swatch(APSChrome::Amber(), LOCTEXT("LegendHome", "HOME"))]
		]
	];

	Box->AddSlot().AutoHeight().Padding(0.0f, 16.0f, 0.0f, 6.0f)
	[
		Local::SectionLabel(LOCTEXT("ControlsHeading", "CONTROLS"), APSChrome::Muted())
	];
	const auto AddHint = [&Box](const FText& Keys, const FText& What)
	{
		Box->AddSlot().AutoHeight().Padding(0.0f, 2.0f)
		[
			APSStrategicMapPanelLocal::KeyHint(Keys, What)
		];
	};
	AddHint(LOCTEXT("KeyOrbit", "RMB DRAG"), LOCTEXT("KeyOrbitWhat", "Orbit"));
	AddHint(LOCTEXT("KeyZoom", "WHEEL"), LOCTEXT("KeyZoomWhat", "Zoom, Shift faster"));
	AddHint(LOCTEXT("KeyPan", "MMB DRAG"), LOCTEXT("KeyPanWhat", "Pan (or Shift+RMB)"));
	AddHint(LOCTEXT("KeySelect", "CLICK"), LOCTEXT("KeySelectWhat", "Select"));
	AddHint(LOCTEXT("KeyFocus", "DOUBLE-CLICK"), LOCTEXT("KeyFocusWhat", "Fly to it"));
	AddHint(LOCTEXT("KeyClose", "F10 / ESC"), LOCTEXT("KeyCloseWhat", "Back to the game"));

	return APSChrome::ChamferPanel(
		SNew(SScrollBox)
		+ SScrollBox::Slot()
		[
			Box
		],
		FMargin(12.0f), APSChrome::CyanDim());
}

TSharedRef<SWidget> SAPSStrategicMapPanel::BuildRightPanel()
{
	namespace Local = APSStrategicMapPanelLocal;
	const auto Tab = [this](const int32 Index, const EAPSChromeGlyph Glyph, const FText& Label, const TAttribute<FText>& Detail)
	{
		return Local::MapButton(
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
			[
				SNew(SBox).WidthOverride(18.0f).HeightOverride(18.0f)
				[
					SNew(SAPSVectorGlyph).Glyph(Glyph).Color(APSChrome::Cyan()).StrokeWidth(1.35f)
				]
			]
			+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center).Padding(8.0f, 0.0f, 0.0f, 0.0f)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(STextBlock).Text(Label).Font(APSChrome::Font("Bold", 11)).ColorAndOpacity(APSChrome::White())
				]
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(STextBlock).Text(Detail).Font(Local::Readable("Bold", 9)).ColorAndOpacity(APSChrome::Muted())
				]
			],
			FOnClicked::CreateSP(this, &SAPSStrategicMapPanel::SetTab, Index),
			TAttribute<bool>::CreateLambda([this, Index]() { return ActiveTab == Index; }), APSChrome::Amber(),
			TAttribute<bool>(true), FMargin(10.0f, 6.0f));
	};
	return APSChrome::ChamferPanel(
		SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight()
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(1.0f)
			[
				Tab(0, EAPSChromeGlyph::Favorite, LOCTEXT("TabStars", "STARS"), TAttribute<FText>::CreateLambda([this]()
				{
					const FAPSStarSystems* Stars = Scene.IsValid() ? Scene->GetStars() : nullptr;
					return Stars && Stars->IsReady()
						? FText::Format(LOCTEXT("TabStarsDetail", "{0} SYSTEMS"), APSUINumber::Number(Stars->Num()))
						: LOCTEXT("TabStarsWaiting", "CATALOGUE NOT READ");
				}))
			]
			+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(6.0f, 0.0f, 0.0f, 0.0f)
			[
				Tab(1, EAPSChromeGlyph::Compass, LOCTEXT("TabObject", "OBJECT"), TAttribute<FText>::CreateLambda([this]()
				{
					return Scene.IsValid() && Scene->GetSelection().IsSet() ? Scene->NameOf(Scene->GetSelection())
						: LOCTEXT("TabObjectNone", "NOTHING SELECTED");
				}))
			]
		]
		+ SVerticalBox::Slot().FillHeight(1.0f).Padding(0.0f, 10.0f, 0.0f, 0.0f)
		[
			SNew(SWidgetSwitcher)
			.WidgetIndex_Lambda([this]() { return ActiveTab; })
			+ SWidgetSwitcher::Slot()
			[
				BuildStarsTab()
			]
			+ SWidgetSwitcher::Slot()
			[
				BuildObjectTab()
			]
		],
		FMargin(12.0f), APSChrome::CyanDim());
}

TSharedRef<SWidget> SAPSStrategicMapPanel::BuildStarsTab()
{
	namespace Local = APSStrategicMapPanelLocal;
	const auto ListButton = [this](const int32 Index, const FText& Label)
	{
		return Local::MapButton(
			SNew(STextBlock).Text(Label).Font(APSChrome::Font("Bold", 10)).Justification(ETextJustify::Center)
			.ColorAndOpacity_Lambda([this, Index]()
			{
				return FSlateColor(StarList == Index && SearchText.IsEmpty() ? APSChrome::White() : APSChrome::Muted());
			}),
			FOnClicked::CreateLambda([this, Index]()
			{
				StarList = Index;
				bStarListDirty = true;
				if (SearchBox.IsValid() && !SearchText.IsEmpty())
				{
					SearchBox->SetText(FText::GetEmpty());
				}
				return FReply::Handled();
			}),
			TAttribute<bool>::CreateLambda([this, Index]() { return StarList == Index && SearchText.IsEmpty(); }),
			APSChrome::Cyan(), TAttribute<bool>(true), FMargin(6.0f, 6.0f));
	};
	return SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight()
		[
			SAssignNew(SearchBox, SEditableTextBox)
			.Style(SearchStyle.Get())
			.HintText(LOCTEXT("SearchHint", "SEARCH A STAR SYSTEM BY NAME"))
			.SelectAllTextWhenFocused(true)
			.ClearKeyboardFocusOnCommit(false)
			.OnTextChanged_Lambda([this](const FText& Text)
			{
				SearchText = Text.ToString().TrimStartAndEnd();
				bStarListDirty = true;
			})
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 8.0f, 0.0f, 0.0f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(1.0f)[ListButton(0, LOCTEXT("ListNearest", "NEAREST"))]
			+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(5.0f, 0.0f)[ListButton(1, LOCTEXT("ListKnown", "KNOWN"))]
			+ SHorizontalBox::Slot().FillWidth(1.0f)[ListButton(2, LOCTEXT("ListClaimed", "CLAIMED"))]
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 10.0f, 0.0f, 6.0f)
		[
			SNew(STextBlock).Font(Local::Readable("Bold", 10)).ColorAndOpacity(APSChrome::Cyan())
			.Text_Lambda([this]()
			{
				if (!SearchText.IsEmpty())
				{
					return FText::Format(LOCTEXT("ListResults", "{0} FOUND FOR \"{1}\""), APSUINumber::Number(StarListShown),
						FText::FromString(SearchText.ToUpper()));
				}
				const FText Title = StarList == 0 ? LOCTEXT("ListNearestTitle", "NEAREST TO THE VIEW")
					: StarList == 1 ? LOCTEXT("ListKnownTitle", "SCANNED, SURVEYED OR HELD") : LOCTEXT("ListClaimedTitle", "THE CIVILIZATION'S SYSTEMS");
				return FText::Format(LOCTEXT("ListTitle", "{0}  /  {1}"), Title, APSUINumber::Number(StarListShown));
			})
		]
		+ SVerticalBox::Slot().FillHeight(1.0f)
		[
			SNew(SScrollBox)
			+ SScrollBox::Slot()
			[
				SAssignNew(StarRows, SVerticalBox)
			]
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 8.0f, 0.0f, 0.0f)
		[
			SNew(STextBlock).Font(APSChrome::Font("Regular", 11)).ColorAndOpacity(APSChrome::Muted()).AutoWrapText(true)
			.Text(LOCTEXT("StarsFooter", "Distances from home. Click a system for its parameters and orders; probes and surveys make it known, beacons and relays claim it."))
		];
}

namespace APSStrategicMapPanelLocal
{
	/** A star's colour as the eye names it (StarGenerator's class palette): seen from afar, before any survey. */
	FText StarColourName(const FLinearColor& Colour)
	{
		const float Red = FMath::Max(Colour.R, 1.0e-3f);
		const float BlueToRed = Colour.B / Red;
		const float GreenToRed = Colour.G / Red;
		if (FMath::Max3(Colour.R, Colour.G, Colour.B) < 0.05f) return LOCTEXT("StarDark", "DARK OBJECT");
		if (BlueToRed > 1.6f) return LOCTEXT("StarBlue", "BLUE");
		if (BlueToRed > 1.3f) return LOCTEXT("StarBlueWhite", "BLUE-WHITE");
		if (BlueToRed > 0.95f) return LOCTEXT("StarWhite", "WHITE");
		if (BlueToRed > 0.7f) return LOCTEXT("StarYellowWhite", "YELLOW-WHITE");
		if (GreenToRed > 0.75f) return LOCTEXT("StarYellow", "YELLOW");
		if (Colour.R < 0.62f && GreenToRed < 0.5f) return LOCTEXT("StarBrown", "BROWN DWARF");
		if (GreenToRed > 0.45f) return LOCTEXT("StarOrange", "ORANGE");
		return LOCTEXT("StarRed", "RED");
	}
}

TSharedRef<SWidget> SAPSStrategicMapPanel::StarRow(const int32 CatalogueIndex)
{
	namespace Local = APSStrategicMapPanelLocal;
	const FAPSStarSystems* Stars = Scene.IsValid() ? Scene->GetStars() : nullptr;
	const FAPSStarSystemInfo* Info = Stars ? Stars->Get(CatalogueIndex) : nullptr;
	if (!Info)
	{
		return SNullWidget::NullWidget;
	}
	const FAPSStarSystemState State = Stars->GetState(Info->Id);
	const TSharedRef<SHorizontalBox> Chips = SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().AutoWidth()
		[
			Local::MapChip(APSStars::KnowledgeName(State.Knowledge), APSStars::KnowledgeColour(State.Knowledge))
		];
	if (State.bClaimed)
	{
		Chips->AddSlot().AutoWidth().Padding(4.0f, 0.0f, 0.0f, 0.0f)
		[
			Local::MapChip(LOCTEXT("ChipClaimed", "CLAIMED"), APSChrome::Success())
		];
	}
	if (Info->StarCount > 1)
	{
		Chips->AddSlot().AutoWidth().Padding(4.0f, 0.0f, 0.0f, 0.0f)
		[
			Local::MapChip(FText::Format(LOCTEXT("ChipStars", "{0} STARS"), APSUINumber::Number(Info->StarCount)), APSChrome::Muted())
		];
	}
	const FText Distance = Info->bHome ? LOCTEXT("RowHome", "HOME") : Local::AuText(Info->HomeDistanceCm);
	// Rio 02.10 ("what are these dashes and squares"): the colour seen from afar is named; the class shows once surveyed.
	const FText ColourName = Local::StarColourName(Info->Colour);
	const FText Spectral = Info->Spectral.IsEmpty()
		? FText::Format(LOCTEXT("RowClassUnknown", "{0}  /  CLASS UNKNOWN"), ColourName)
		: FText::Format(LOCTEXT("RowClassKnown", "{0}  /  {1}"), FText::FromString(Info->Spectral.ToUpper()), ColourName);
	const FLinearColor StarColour = Info->Colour;
	return Local::MapButton(
		SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight()
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center)
			[
				SNew(STextBlock).Text(FText::FromString(Info->Name)).Font(APSChrome::Font("Bold", 11))
				.ColorAndOpacity_Lambda([this, CatalogueIndex]()
				{
					return FSlateColor(Scene.IsValid() && Scene->GetSelection().SystemIndex == CatalogueIndex
						? APSChrome::Amber() : APSChrome::White());
				})
			]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(8.0f, 0.0f, 0.0f, 0.0f)
			[
				SNew(STextBlock).Text(Distance).Font(APSChrome::Font("Regular", 11)).ColorAndOpacity(APSChrome::Muted())
			]
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 4.0f, 0.0f, 0.0f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
			[
				SNew(SBox).WidthOverride(8.0f).HeightOverride(8.0f)
				[
					SNew(SBorder).BorderImage(FAppStyle::GetBrush("WhiteBrush")).BorderBackgroundColor(StarColour)
				]
			]
			+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center).Padding(6.0f, 0.0f, 0.0f, 0.0f)
			[
				SNew(STextBlock).Text(Spectral).Font(Local::Readable("Bold", 10)).ColorAndOpacity(APSChrome::Muted())
				.OverflowPolicy(ETextOverflowPolicy::Ellipsis).ToolTipText_Lambda([Spectral]()
				{
					return FText::Format(LOCTEXT("RowClassTipFull", "{0}\n{1}"), Spectral, LOCTEXT("RowClassTipLine", "The square and the colour are the star as seen from afar; its spectral class is learnt by a probe, a survey or a visit."));
				})
			]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
			[
				Chips
			]
		],
		FOnClicked::CreateLambda([this, CatalogueIndex]()
		{
			// Rio 02.10: "click a star to see its parameters, build routes": select it, fly there, show its page.
			const APSStrategicMap::FSelection Target = APSStrategicMap::FSelection::OfSystem(CatalogueIndex);
			Select(Target, true);
			FocusOn(Target, TOptional<double>());
			return FReply::Handled();
		}),
		TAttribute<bool>::CreateLambda([this, CatalogueIndex]()
		{
			return Scene.IsValid() && Scene->GetSelection().SystemIndex == CatalogueIndex;
		}),
		APSChrome::Amber(), TAttribute<bool>(true), FMargin(10.0f, 6.0f));
}

void SAPSStrategicMapPanel::UpdateStarList(const float DeltaSeconds)
{
	StarListClock -= DeltaSeconds;
	const FAPSStarSystems* Stars = Scene->GetStars();
	const uint32 Revision = Stars ? HashCombine(Stars->GetRevision(), static_cast<uint32>(Stars->Num())) : 0u;
	bool bRebuild = bStarListDirty || Revision != StarListRevision;
	// The nearest list follows the view, at most twice a second.
	if (!bRebuild && StarList == 0 && SearchText.IsEmpty() && StarListClock <= 0.0f
		&& FVector::Dist(Camera->GetLookLocation(), StarListFocus) > FMath::Max(Camera->GetDistance() * 0.15, 1.0e9))
	{
		bRebuild = true;
	}
	if (bRebuild && (bStarListDirty || StarListClock <= 0.0f))
	{
		const bool bForce = bStarListDirty || Revision != StarListRevision;
		RebuildStarList(bForce);
		bStarListDirty = false;
		StarListRevision = Revision;
		StarListFocus = Camera->GetLookLocation();
		StarListClock = 0.5f;
	}
}

void SAPSStrategicMapPanel::RebuildStarList(const bool bForce)
{
	if (!StarRows.IsValid())
	{
		return;
	}
	FAPSStarSystems* Stars = Scene.IsValid() ? Scene->GetStars() : nullptr;
	if (!Stars || !Stars->IsReady())
	{
		StarRows->ClearChildren();
		StarListShown = 0;
		ListedSystems.Reset();
		StarRows->AddSlot().AutoHeight()
		[
			SNew(STextBlock).Font(APSChrome::Font("Regular", 11)).ColorAndOpacity(APSChrome::Muted()).AutoWrapText(true)
			.Text(LOCTEXT("CatalogueWaiting", "The star catalogue is read once the generated cluster is ready."))
		];
		return;
	}
	TArray<int32> Indices;
	if (!SearchText.IsEmpty())
	{
		Stars->Search(SearchText, 40, Indices);
	}
	else if (StarList == 0)
	{
		Stars->FindNearest(Camera.IsValid() ? Camera->GetLookLocation() : FVector::ZeroVector, 30, Indices);
	}
	else
	{
		Stars->GetKnown(Indices);
		if (StarList == 2)
		{
			Indices.RemoveAll([Stars](const int32 Index)
			{
				const FAPSStarSystemInfo* Info = Stars->Get(Index);
				return !Info || !Stars->IsClaimed(Info->Id);
			});
		}
	}
	// While the view flies the nearest systems mostly stay the same: rebuild the rows only when the list changed.
	if (!bForce && Indices == ListedSystems)
	{
		return;
	}
	ListedSystems = Indices;
	StarRows->ClearChildren();
	for (const int32 Index : Indices)
	{
		StarRows->AddSlot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 5.0f)
		[
			StarRow(Index)
		];
	}
	StarListShown = Indices.Num();
	if (Indices.IsEmpty())
	{
		StarRows->AddSlot().AutoHeight()
		[
			SNew(STextBlock).Font(APSChrome::Font("Regular", 11)).ColorAndOpacity(APSChrome::Muted()).AutoWrapText(true)
			.Text(!SearchText.IsEmpty() ? LOCTEXT("NothingFound", "No system has this in its name.")
				: StarList == 2 ? LOCTEXT("NothingClaimed", "Only home so far: a beacon, relay or administration claims a system.")
				: LOCTEXT("NothingKnown", "Nothing yet: send a probe or survey a system to know it."))
		];
	}
}

TSharedRef<SWidget> SAPSStrategicMapPanel::BuildObjectTab()
{
	return SNew(SScrollBox)
		+ SScrollBox::Slot()
		[
			SAssignNew(ObjectBox, SVerticalBox)
		];
}

TSharedRef<SWidget> SAPSStrategicMapPanel::ActionButton(const TSharedRef<FAPSObjectAction>& Action)
{
	namespace Local = APSStrategicMapPanelLocal;
	const bool bEnabled = Action->bEnabled && static_cast<bool>(Action->Execute);
	const FLinearColor Accent = Action->Colour;
	const TSharedRef<SVerticalBox> Box = SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight()
		[
			Local::MapButton(
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
				[
					SNew(SBox).WidthOverride(3.0f).HeightOverride(14.0f)
					[
						SNew(SBorder).BorderImage(FAppStyle::GetBrush("WhiteBrush"))
						.BorderBackgroundColor(bEnabled ? Accent : FLinearColor(Accent.R, Accent.G, Accent.B, 0.3f))
					]
				]
				+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center).Padding(9.0f, 0.0f, 0.0f, 0.0f)
				[
					SNew(STextBlock).Text(Action->Label).Font(APSChrome::Font("Bold", 10))
					.ColorAndOpacity(bEnabled ? APSChrome::White() : APSChrome::Muted())
				],
				FOnClicked::CreateLambda([this, Action]()
				{
					if (Action->bEnabled && Action->Execute)
					{
						ActionMessage = Action->Execute();
						if (ActionMessage.IsEmpty())
						{
							ActionMessage = FText::Format(LOCTEXT("ActionDone", "{0}: done."), Action->Label);
						}
						if (Scene.IsValid())
						{
							Scene->Invalidate();
						}
						bObjectDirty = true;
					}
					return FReply::Handled();
				}),
				TAttribute<bool>(false), Accent, TAttribute<bool>(bEnabled), FMargin(10.0f, 6.0f))
		];
	if (!Action->Detail.IsEmpty())
	{
		// What it does, or why it cannot be done now.
		Box->AddSlot().AutoHeight().Padding(12.0f, 2.0f, 0.0f, 2.0f)
		[
			SNew(STextBlock).Text(Action->Detail).Font(APSChrome::Font("Regular", 11)).AutoWrapText(true)
			.ColorAndOpacity(bEnabled ? APSChrome::Muted() : FLinearColor(1.0f, 0.66f, 0.46f, 1.0f))
		];
	}
	return Box;
}

void SAPSStrategicMapPanel::UpdateObjectPage(const float DeltaSeconds)
{
	if (Scene->GetSelectionSerial() != ObjectSelectionSerial)
	{
		ActionMessage = FText::GetEmpty();
		bObjectDirty = true;
	}
	if ((bObjectHadTarget && !ObjectTarget.IsValid()) || RuntimeRevision() != ObjectRevision)
	{
		bObjectDirty = true;
	}
	if (bObjectDirty)
	{
		RebuildObjectPage();
		return;
	}
	// Status values change while the page is open (distances, orders): re-read twice a second in place. Actions whose
	// availability changed without a revision (stocks filling up) rebuild the page.
	ObjectFieldsClock -= DeltaSeconds;
	AActor* Target = ObjectTarget.Get();
	UWorld* World = Scene->GetWorld();
	if (ObjectFieldsClock > 0.0f || !Target || !World)
	{
		return;
	}
	ObjectFieldsClock = 0.5f;
	TArray<TPair<FText, FText>> Fresh;
	APSObjectActions::Describe(World, Target, Fresh);
	bool bSameRows = Fresh.Num() == ObjectFields.Num();
	for (int32 Index = 0; bSameRows && Index < Fresh.Num(); ++Index)
	{
		bSameRows = Fresh[Index].Key.EqualTo(ObjectFields[Index].Key);
	}
	if (!bSameRows)
	{
		RebuildObjectPage();
		return;
	}
	ObjectFields = MoveTemp(Fresh);
	TArray<FAPSObjectAction> Gathered;
	APSObjectActions::Gather(World, Target, Gathered);
	bool bSameActions = Gathered.Num() == ObjectActions.Num();
	for (int32 Index = 0; bSameActions && Index < Gathered.Num(); ++Index)
	{
		const FAPSObjectAction& Now = Gathered[Index];
		const FAPSObjectAction& Shown = *ObjectActions[Index];
		bSameActions = Now.Id == Shown.Id && Now.bEnabled == Shown.bEnabled && Now.Detail.EqualTo(Shown.Detail);
	}
	if (!bSameActions)
	{
		RebuildObjectPage();
	}
}

void SAPSStrategicMapPanel::RebuildObjectPage()
{
	namespace Local = APSStrategicMapPanelLocal;
	if (!ObjectBox.IsValid() || !Scene.IsValid())
	{
		return;
	}
	ObjectBox->ClearChildren();
	ObjectActions.Reset();
	ObjectFields.Reset();
	bObjectDirty = false;
	ObjectSelectionSerial = Scene->GetSelectionSerial();
	ObjectRevision = RuntimeRevision();
	ObjectFieldsClock = 0.5f;
	const APSStrategicMap::FSelection Selection = Scene->GetSelection();
	// A star system is acted on through its anchor (spawned on demand by the star systems runtime).
	AActor* Target = Selection.IsSet() ? Scene->ResolveActor(Selection) : nullptr;
	ObjectTarget = Target;
	bObjectHadTarget = Target != nullptr;
	UWorld* World = Scene->GetWorld();
	if (!Target || !World)
	{
		ObjectBox->AddSlot().AutoHeight()
		[
			APSChrome::IconSectionHeading(EAPSChromeGlyph::Compass, LOCTEXT("NothingSelected", "NOTHING SELECTED"),
				LOCTEXT("NothingSelectedHint", "Click an object on the map or a star in the list"))
		];
		ObjectBox->AddSlot().AutoHeight().Padding(0.0f, 10.0f, 0.0f, 0.0f)
		[
			SNew(STextBlock).Font(APSChrome::Font("Regular", 11)).ColorAndOpacity(APSChrome::Muted()).AutoWrapText(true)
			.Text(LOCTEXT("NothingSelectedBody", "Stars, planets, moons, stations, outposts, the colony, anomaly sites, ships and every star system of the cluster can be selected. A double click flies to it."))
		];
		return;
	}

	const FString Designation = APSBodyDesignation::Of(Target);
	const FText Name = Scene->NameOf(Selection);
	ObjectBox->AddSlot().AutoHeight()
	[
		APSChrome::IconSectionHeading(Local::GlyphOf(Target, Selection.IsSystem()),
			Designation.IsEmpty() ? Name : FText::Format(LOCTEXT("NameDesignation", "{0}  {1}"), Name, FText::FromString(Designation)),
			APSObjectActions::KindOf(Target))
	];
	ObjectBox->AddSlot().AutoHeight().Padding(0.0f, 10.0f, 0.0f, 0.0f)
	[
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().FillWidth(1.0f)
		[
			Local::MapButton(
				SNew(STextBlock).Text(LOCTEXT("FlyTo", "FLY TO")).Font(APSChrome::Font("Bold", 10)).Justification(ETextJustify::Center)
				.ColorAndOpacity(APSChrome::White()),
				FOnClicked::CreateSP(this, &SAPSStrategicMapPanel::ApplyPreset, EPreset::Selection), TAttribute<bool>(false),
				APSChrome::Amber(), TAttribute<bool>(true), FMargin(8.0f, 6.0f))
		]
		+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(6.0f, 0.0f, 0.0f, 0.0f)
		[
			Local::MapButton(
				SNew(STextBlock).Text(LOCTEXT("BackToList", "STAR LIST")).Font(APSChrome::Font("Bold", 10))
				.Justification(ETextJustify::Center).ColorAndOpacity(APSChrome::White()),
				FOnClicked::CreateSP(this, &SAPSStrategicMapPanel::SetTab, 0), TAttribute<bool>(false), APSChrome::Cyan(),
				TAttribute<bool>(true), FMargin(8.0f, 6.0f))
		]
		+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(6.0f, 0.0f, 0.0f, 0.0f)
		[
			Local::MapButton(
				SNew(STextBlock).Text(LOCTEXT("ClearSelection", "CLEAR")).Font(APSChrome::Font("Bold", 10))
				.Justification(ETextJustify::Center).ColorAndOpacity(APSChrome::White()),
				FOnClicked::CreateLambda([this]()
				{
					Select(APSStrategicMap::FSelection(), false);
					return FReply::Handled();
				}),
				TAttribute<bool>(false), APSChrome::Cyan(), TAttribute<bool>(true), FMargin(8.0f, 6.0f))
		]
	];

	// Status: what is known of it, what stands there, what is under way (APSObjectActions::Describe).
	APSObjectActions::Describe(World, Target, ObjectFields);
	if (!ObjectFields.IsEmpty())
	{
		ObjectBox->AddSlot().AutoHeight().Padding(0.0f, 14.0f, 0.0f, 6.0f)
		[
			Local::SectionLabel(LOCTEXT("StatusHeading", "STATUS"), APSChrome::Cyan())
		];
		for (int32 Index = 0; Index < ObjectFields.Num(); ++Index)
		{
			ObjectBox->AddSlot().AutoHeight().Padding(0.0f, 2.0f)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Top)
				[
					SNew(SBox).WidthOverride(124.0f)
					[
						SNew(STextBlock).Text(ObjectFields[Index].Key).Font(Local::Readable("Bold", 9))
						.ColorAndOpacity(APSChrome::Muted())
					]
				]
				+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Top)
				[
					SNew(STextBlock).Font(APSChrome::Font("Regular", 11)).ColorAndOpacity(APSChrome::White()).AutoWrapText(true)
					.Text_Lambda([this, Index]()
					{
						return ObjectFields.IsValidIndex(Index) ? ObjectFields[Index].Value : FText::GetEmpty();
					})
				]
			];
		}
	}

	// Every action the runtime offers for it, grouped (navigation, fleet, construction by department).
	TArray<FAPSObjectAction> Gathered;
	APSObjectActions::Gather(World, Target, Gathered);
	for (FAPSObjectAction& Action : Gathered)
	{
		ObjectActions.Add(MakeShared<FAPSObjectAction>(MoveTemp(Action)));
	}
	ObjectBox->AddSlot().AutoHeight().Padding(0.0f, 16.0f, 0.0f, 4.0f)
	[
		Local::SectionLabel(LOCTEXT("ActionsHeading", "ORDERS AND ACTIONS"), APSChrome::Amber())
	];
	ObjectBox->AddSlot().AutoHeight().Padding(0.0f, 2.0f, 0.0f, 4.0f)
	[
		SNew(STextBlock).Font(APSChrome::Font("Regular", 11)).ColorAndOpacity(APSChrome::Amber()).AutoWrapText(true)
		.Text_Lambda([this]() { return ActionMessage; })
		.Visibility_Lambda([this]() { return ActionMessage.IsEmpty() ? EVisibility::Collapsed : EVisibility::Visible; })
	];
	if (ObjectActions.IsEmpty())
	{
		ObjectBox->AddSlot().AutoHeight()
		[
			SNew(STextBlock).Font(APSChrome::Font("Regular", 11)).ColorAndOpacity(APSChrome::Muted()).AutoWrapText(true)
			.Text(LOCTEXT("NoActions", "Nothing can be ordered here now."))
		];
		return;
	}
	TArray<FString> Groups;
	for (const TSharedRef<FAPSObjectAction>& Action : ObjectActions)
	{
		Groups.AddUnique(Action->Group.ToString());
	}
	for (const FString& Group : Groups)
	{
		const TSharedRef<FAPSObjectAction>* First = ObjectActions.FindByPredicate([&Group](const TSharedRef<FAPSObjectAction>& Action)
		{
			return Action->Group.ToString() == Group;
		});
		ObjectBox->AddSlot().AutoHeight().Padding(0.0f, 10.0f, 0.0f, 4.0f)
		[
			Local::SectionLabel(FText::FromString(Group), First ? (*First)->Colour : APSChrome::Cyan())
		];
		for (const TSharedRef<FAPSObjectAction>& Action : ObjectActions)
		{
			if (Action->Group.ToString() == Group)
			{
				ObjectBox->AddSlot().AutoHeight().Padding(0.0f, 2.0f)
				[
					ActionButton(Action)
				];
			}
		}
	}
}

#undef LOCTEXT_NAMESPACE
