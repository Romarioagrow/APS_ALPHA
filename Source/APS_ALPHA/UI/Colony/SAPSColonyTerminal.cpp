#include "SAPSColonyTerminal.h"
#include "APS_ALPHA/UI/Style/APSUITheme.h"
#include "APS_ALPHA/UI/Style/APSUINumber.h"

#include "SAPSCivilizationMap.h"
#include "SAPSCivilizationOverview.h"
#include "SAPSPilotDashboard.h"
#include "SAPSDivisionsPanel.h"
#include "SAPSInfrastructurePanel.h"
#include "SAPSSystemScheme.h"
#include "SAPSStarMapPanel.h"
#include "SAPSSurfaceMap.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "APS_ALPHA/Pawns/Characters/CustomGravityCharacter.h"
#include "Widgets/Layout/SGridPanel.h"
#include "APS_ALPHA/Actors/Astro/APSBodyDesignation.h"
#include "APS_ALPHA/Actors/Astro/Moon.h"

#include "APS_ALPHA/Actors/Astro/Planet.h"
#include "APS_ALPHA/Actors/Astro/PlanetaryBody.h"
#include "APS_ALPHA/Actors/Astro/Star.h"
#include "APS_ALPHA/Actors/Tech/SpaceHeadquarters.h"
#include "APS_ALPHA/Actors/Tech/SpaceShipyard.h"
#include "APS_ALPHA/Core/Instances/MainGameplayInstance.h"
#include "APS_ALPHA/Core/Interfaces/ItemInfoInterface.h"
#include "APS_ALPHA/Core/Model/SpawnParameters.h"
#include "APS_ALPHA/Gameplay/Civilizations/APSCivilizationIdentityComponent.h"
#include "APS_ALPHA/Gameplay/Colony/APSColonyConstructionSubsystem.h"
#include "APS_ALPHA/Gameplay/Colony/APSColonyModule.h"
#include "APS_ALPHA/Gameplay/Colony/APSColonyModuleCatalogue.h"
#include "APS_ALPHA/Gameplay/Civilizations/APSCivilizationJournalSubsystem.h"
#include "APS_ALPHA/Gameplay/Civilizations/APSCivilizationMaterializationSubsystem.h"
#include "APS_ALPHA/Gameplay/Civilizations/Civilization.h"
#include "APS_ALPHA/Gameplay/Expansion/APSInfrastructure.h"
#include "APS_ALPHA/Gameplay/Expansion/APSObjectActions.h"
#include "APS_ALPHA/Generation/AstroGenerator.h"
#include "APS_ALPHA/Pawns/Spaceships/APSShipFlightModel.h"
#include "APS_ALPHA/Pawns/Spaceships/ShipNavigationComponent.h"
#include "APS_ALPHA/Pawns/Spaceships/Spaceship.h"
#include "APS_ALPHA/UI/MainMenu/APSUIThumbnails.h"
#include "APS_ALPHA/UI/Style/APSMenuChrome.h"
#include "Engine/Texture2D.h"
#include "EngineUtils.h"
#include "Framework/Application/SlateApplication.h"
#include "GameFramework/PlayerController.h"
#include "InputCoreTypes.h"
#include "Styling/AppStyle.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScaleBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SWidgetSwitcher.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SUniformGridPanel.h"
#include "Widgets/Layout/SWrapBox.h"
#include "Widgets/SOverlay.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "APSColonyTerminal"

namespace APSColonyUI
{
	using namespace APSChrome;

	/** Registered with the module, so -ExecCmds at start-up can set it (B1 test captures). */
	TAutoConsoleVariable<FString> CVarPreviewTest(TEXT("aps.Colony.PreviewTest"), TEXT(""),
		TEXT("Tests: picks the studied planet or moon whose name contains this text as the fleet target (B1 preview)."));

	/** Rio 06.10 (audit: a clicked terminal button kept keyboard focus, so Space/Enter repeated it and Tab/K/F10 could miss
	 * the terminal). Read when a button is built; the terminal is rebuilt on every open. Other panels look it up by name. */
	TAutoConsoleVariable<int32> CVarTerminalButtonsNoFocus(TEXT("aps.UI.TerminalButtonsNoFocus"), 1,
		TEXT("1: terminal buttons never take keyboard focus (a click no longer makes Space/Enter repeat it, and Tab/K/F10 always reach the terminal). 0: focusable buttons as before."));

	template <typename TEnum>
	FText EnumText(const TEnum Value)
	{
		const UEnum* Enum = StaticEnum<TEnum>();
		return Enum ? Enum->GetDisplayNameTextByValue(static_cast<int64>(Value)) : FText::GetEmpty();
	}

	const UMainGameplayInstance* GameplayState(const UWorld* World)
	{
		const UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
		return GameInstance ? GameInstance->GetSubsystem<UMainGameplayInstance>() : nullptr;
	}

	const APlanet* HomePlanet(UWorld* World)
	{
		if (World)
		{
			for (TActorIterator<AAstroGenerator> It(World); It; ++It)
			{
				if (IsValid(*It) && IsValid(It->HomePlanet))
				{
					return It->HomePlanet;
				}
			}
		}
		return nullptr;
	}

	/**
	 * The menu's chamfered button: a card whose frame lights up on hover or when selected. Content is laid out by the
	 * caller (a glyph badge and a label for tabs, a label and a key chip for actions).
	 */
	TSharedRef<SWidget> ChromeButton(TSharedRef<SWidget> Content, FOnClicked OnClicked, TAttribute<bool> IsSelected,
		const FLinearColor& Accent)
	{
		// The button exists before its content, so the frame can watch its hover state.
		const TSharedRef<SButton> Button = SNew(SButton)
			.ButtonStyle(FAppStyle::Get(), "NoBorder")
			.ContentPadding(0.0f)
			// Rio 06.10 (audit: focus stayed on the clicked button): aps.UI.TerminalButtonsNoFocus, 0 = focusable as before.
			.IsFocusable(CVarTerminalButtonsNoFocus.GetValueOnGameThread() == 0)
			.OnClicked(OnClicked);
		const TWeakPtr<SButton> WeakButton = Button;
		Button->SetContent(
			SNew(SAPSChamferedOverlay)
			+ SOverlay::Slot()
			[
				SNew(SAPSChamferedSurface)
				.Brush(FAppStyle::GetBrush("WhiteBrush"))
				.Tint_Lambda([IsSelected]()
				{
					return IsSelected.Get(false) ? APSUITheme::Retint(FLinearColor(0.02f, 0.13f, 0.17f, 0.97f)) : Panel();
				})
				.ChamferTop(true)
				.ChamferBottom(true)
			]
			+ SOverlay::Slot().Padding(FMargin(14.0f, 9.0f))
			[
				Content
			]
			+ SOverlay::Slot()
			[
				SNew(SAPSChamferedFrame)
				.Thickness(1.0f)
				.Color_Lambda([WeakButton, IsSelected, Accent]()
				{
					const TSharedPtr<SButton> Pinned = WeakButton.Pin();
					return IsSelected.Get(false) || (Pinned && Pinned->IsHovered()) ? Accent : CyanDim();
				})
			]);
		return Button;
	}

	/** A label alone (chips, CLOSE, BACK TO THE SYSTEM): centred both ways in the button by its capitals (Rio 03.10:
	 * "everywhere the text strictly centred by height and width"). A label that brings its own render transform (a
	 * symbol such as "<", shifted by SymbolCenterShift) keeps it. */
	TSharedRef<SWidget> ChromeButton(const TSharedRef<STextBlock>& Label, FOnClicked OnClicked, TAttribute<bool> IsSelected,
		const FLinearColor& Accent)
	{
		Label->SetJustification(ETextJustify::Center);
		if (!Label->GetRenderTransform().IsSet())
		{
			Label->SetRenderTransform(CapsCenterShift(Label->GetFont()));
		}
		return ChromeButton(SNew(SBox).HAlign(HAlign_Center).VAlign(VAlign_Center)[Label], MoveTemp(OnClicked),
			MoveTemp(IsSelected), Accent);
	}

	/** The one obvious action of a card or row: a filled amber button with dark text; dim when unavailable
	 * (Rio, 30.09: "it is unclear what to click, everything blends"). */
	TSharedRef<SWidget> PrimaryButton(const FText& Label, FOnClicked OnClicked, TAttribute<bool> CanClick)
	{
		const TSharedRef<SButton> Button = SNew(SButton)
			.ButtonStyle(FAppStyle::Get(), "NoBorder")
			.ContentPadding(0.0f)
			.IsEnabled(CanClick)
			// Rio 06.10 (audit: Space after SET COURSE repeated the order): aps.UI.TerminalButtonsNoFocus, 0 = focusable as before.
			.IsFocusable(CVarTerminalButtonsNoFocus.GetValueOnGameThread() == 0)
			.OnClicked(OnClicked);
		const TWeakPtr<SButton> WeakButton = Button;
		Button->SetContent(
			SNew(SAPSChamferedOverlay)
			+ SOverlay::Slot()
			[
				SNew(SAPSChamferedSurface)
				.Brush(FAppStyle::GetBrush("WhiteBrush"))
				.Tint_Lambda([WeakButton, CanClick]()
				{
					if (!CanClick.Get(true))
					{
						return APSUITheme::Retint(FLinearColor(0.07f, 0.13f, 0.16f, 0.95f));
					}
					const TSharedPtr<SButton> Pinned = WeakButton.Pin();
					return Pinned && Pinned->IsHovered() ? APSChrome::AmberBright() : Amber();
				})
				.ChamferTop(true)
				.ChamferBottom(true)
			]
			+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center).Padding(FMargin(14.0f, 8.0f))
			[
				SNew(STextBlock).Text(Label).Font(Font("Bold", 11))
				.Justification(ETextJustify::Center).RenderTransform(CapsCenterShift(Font("Bold", 11)))
				.ColorAndOpacity_Lambda([CanClick]()
				{
					return FSlateColor(CanClick.Get(true) ? APSChrome::OnAmber() : Muted());
				})
			]);
		return Button;
	}

	/** A line glyph per module, from the menu's set. */
	EAPSChromeGlyph ModuleGlyph(const FName ModuleId)
	{
		static const TMap<FName, EAPSChromeGlyph> Glyphs = {
			{TEXT("Habitat"), EAPSChromeGlyph::Headquarters},
			{TEXT("Greenhouse"), EAPSChromeGlyph::Planet},
			{TEXT("Storage"), EAPSChromeGlyph::Collection},
			{TEXT("Floodlight"), EAPSChromeGlyph::Favorite},
			{TEXT("SolarArray"), EAPSChromeGlyph::System},
			{TEXT("CommsMast"), EAPSChromeGlyph::Compass},
			{TEXT("SolarWing"), EAPSChromeGlyph::System},
			{TEXT("CargoPods"), EAPSChromeGlyph::Collection},
			{TEXT("NavBeacon"), EAPSChromeGlyph::Compass}};
		const EAPSChromeGlyph* Glyph = Glyphs.Find(ModuleId);
		return Glyph ? *Glyph : EAPSChromeGlyph::Infrastructure;
	}

	/** In-game name of a ship, else its class name without the blueprint decoration. */
	FText ShipName(const ASpaceship* Ship)
	{
		if (Ship->GetClass()->ImplementsInterface(UItemInfoInterface::StaticClass()))
		{
			const FText Name = IItemInfoInterface::Execute_GetInGameName(Ship);
			if (!Name.IsEmpty())
			{
				return FText::FromString(Name.ToString().ToUpper());
			}
		}
		FString Name = Ship->GetClass()->GetName();
		Name.RemoveFromStart(TEXT("BP_"));
		Name.RemoveFromEnd(TEXT("_C"));
		Name.ReplaceInline(TEXT("_"), TEXT(" "));
		return FText::FromString(Name.ToUpper());
	}

	FString FormatSpeed(const double MetersPerSecond)
	{
		constexpr double LightSpeed = 299792458.0;
		if (MetersPerSecond >= LightSpeed * 0.1)
		{
			return FString::Printf(TEXT("%.1f c"), MetersPerSecond / LightSpeed);
		}
		return MetersPerSecond >= 1000.0 ? FString::Printf(TEXT("%.1f km/s"), MetersPerSecond / 1000.0)
			: FString::Printf(TEXT("%.0f m/s"), MetersPerSecond);
	}

	FString PawnDistance(const UWorld* World, const AActor* Actor)
	{
		const APlayerController* Controller = World ? World->GetFirstPlayerController() : nullptr;
		const APawn* Pawn = Controller ? Controller->GetPawn() : nullptr;
		return Pawn && Actor
			? UShipNavigationComponent::FormatDistance(FVector::Distance(Pawn->GetActorLocation(), Actor->GetActorLocation()))
			: FString(TEXT("?"));
	}
}

namespace APSColonyUI
{
	/** How a journal category reads: its glyph, colour and label (Rio, 01.10: "the journal is hard to read"). */
	struct FJournalStyle
	{
		EAPSChromeGlyph Glyph{EAPSChromeGlyph::Recent};
		FLinearColor Colour{FLinearColor::White};
		FText Label;
	};

	FJournalStyle JournalStyle(const FName Category)
	{
		if (Category == TEXT("Objective"))
		{
			return {EAPSChromeGlyph::Favorite, Amber(), LOCTEXT("JournalObjective", "OBJECTIVE")};
		}
		if (Category == TEXT("Fleet"))
		{
			return {EAPSChromeGlyph::Fleet, Cyan(), LOCTEXT("JournalFleet", "FLEET")};
		}
		if (Category == TEXT("Colony"))
		{
			return {EAPSChromeGlyph::Headquarters, FLinearColor(0.36f, 1.0f, 0.58f), LOCTEXT("JournalColony", "COLONY")};
		}
		if (Category == TEXT("Build"))
		{
			return {EAPSChromeGlyph::Infrastructure, FLinearColor(1.0f, 0.62f, 0.32f), LOCTEXT("JournalBuild", "CONSTRUCTION")};
		}
		if (Category == TEXT("Flight"))
		{
			return {EAPSChromeGlyph::Ship, FLinearColor(0.56f, 0.78f, 1.0f), LOCTEXT("JournalFlight", "FLIGHT")};
		}
		if (Category == TEXT("Navigation"))
		{
			return {EAPSChromeGlyph::Compass, FLinearColor(0.56f, 0.78f, 1.0f), LOCTEXT("JournalNavigation", "NAVIGATION")};
		}
		if (Category == TEXT("Start"))
		{
			return {EAPSChromeGlyph::Civilization, White(), LOCTEXT("JournalStart", "START")};
		}
		return {EAPSChromeGlyph::Recent, Muted(), FText::FromString(Category.ToString().ToUpper())};
	}

	/** Chips read best with the player's goals first, then what the fleet and the colony did. */
	int32 JournalRank(const FName Category)
	{
		static const FName Ordered[] = {TEXT("Objective"), TEXT("Fleet"), TEXT("Colony"), TEXT("Build"), TEXT("Flight"),
			TEXT("Navigation"), TEXT("Start")};
		for (int32 Index = 0; Index < static_cast<int32>(UE_ARRAY_COUNT(Ordered)); ++Index)
		{
			if (Ordered[Index] == Category)
			{
				return Index;
			}
		}
		return static_cast<int32>(UE_ARRAY_COUNT(Ordered));
	}
}

void SAPSColonyTerminal::Construct(const FArguments& InArgs)
{
	using namespace APSColonyUI;
	World = InArgs._World;
	OnClose = InArgs._OnClose;
	OnOpenMap = InArgs._OnOpenMap;
	CacheColonyActors();
	// The headquarters is the default base (Rio, 29.09); a pilot standing at the surface base builds there.
	if (const UAPSColonyConstructionSubsystem* Construction = GetConstruction())
	{
		const AActor* Base = Construction->GetSiteAnchor(EAPSSpawnSite::Surface);
		const APlayerController* Controller = World.IsValid() ? World->GetFirstPlayerController() : nullptr;
		const APawn* Pawn = Controller ? Controller->GetPawn() : nullptr;
		if (Base && Pawn && FVector::Dist(Base->GetActorLocation(), Pawn->GetActorLocation()) < 300000.0)
		{
			SelectedSite = EAPSSpawnSite::Surface;
		}
	}
	const UMainGameplayInstance* State = GameplayState(World.Get());
	const UCivilization* Civilization = State ? State->CurrentCivilization.Get() : nullptr;
	const FString CivilizationName = Civilization ? Civilization->Name
		: (State && State->SpawnParameters ? State->SpawnParameters->CivilizationName : FString(TEXT("CIVILIZATION")));

	const auto TabButton = [this](const EAPSChromeGlyph Glyph, const FText& Label, const FText& Details, const ETab Tab)
	{
		return ChromeButton(
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
			[
				IconBadge(Glyph, Cyan(), 32.0f)
			]
			+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center).Padding(10.0f, 0.0f, 0.0f, 0.0f)
			[
				// Rio 03.10: the title sits by its capitals, so the two lines centre on the badge.
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(STextBlock).Text(Label).Font(Font("Bold", 12)).ColorAndOpacity(White())
					.RenderTransform(CapsCenterShift(Font("Bold", 12)))
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 2.0f, 0.0f, 0.0f)
				[
					SNew(STextBlock).Text(Details).Font(Font("Regular", 10)).ColorAndOpacity(Muted())
				]
			],
			FOnClicked::CreateLambda([this, Tab]()
			{
				// Rio 05.10 (star map): MAP reopens the map mode used last (stars, system, scheme or surface).
				return SelectTab(Tab == ETab::Map ? LastMapTab : Tab);
			}),
			TAttribute<bool>::CreateLambda([this, Tab]() { return ActiveTab == Tab || (Tab == ETab::Map && IsMapTab()); }),
			Cyan());
	};
	const auto ActionButton = [](const FText& Label, const FText& Key, FOnClicked OnClicked, const FLinearColor& Accent)
	{
		return SNew(SBox).WidthOverride(240.0f)
		[
			ChromeButton(
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center)
				[
					SNew(STextBlock).Text(Label).Font(Font("Bold", 11)).ColorAndOpacity(Accent)
					.RenderTransform(CapsCenterShift(Font("Bold", 11)))
				]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(8.0f, 0.0f, 0.0f, 0.0f)
				[
					KeyChip(Key, Muted())
				],
				OnClicked, TAttribute<bool>(false), Accent)
		];
	};

	ChildSlot
	[
		SNew(SBorder).BorderImage(FAppStyle::GetBrush("WhiteBrush"))
		.BorderBackgroundColor(Scrim()).Padding(FMargin(32.0f, 26.0f))
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
				[
					IconBadge(EAPSChromeGlyph::Civilization, Cyan(), 48.0f)
				]
				+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center).Padding(14.0f, 0.0f)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight()
					[
						SNew(STextBlock)
						.Text(FText::Format(LOCTEXT("Title", "CIVILIZATION COMMAND  //  {0}"),
							FText::FromString(CivilizationName.ToUpper())))
						.Font(Font("Bold", 22)).ColorAndOpacity(White())
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 3.0f, 0.0f, 0.0f)
					[
						SNew(STextBlock).Text(LOCTEXT("Subtitle", "LIVE CIVILIZATION DATA  /  TAB OR ESC TO CLOSE  /  K FLEET ORDERS"))
						.Font(Font("Bold", 9)).ColorAndOpacity(Cyan())
					]
				]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(8.0f, 0.0f)
				[
					ActionButton(LOCTEXT("Map", "STRATEGIC MAP"), LOCTEXT("MapKey", "F10"),
						FOnClicked::CreateSP(this, &SAPSColonyTerminal::OpenMap), Cyan())
				]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
				[
					ActionButton(LOCTEXT("Close", "RETURN TO GAME"), LOCTEXT("CloseKey", "TAB"),
						FOnClicked::CreateSP(this, &SAPSColonyTerminal::Close), Amber())
				]
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 14.0f, 0.0f, 0.0f)
			[
				SNew(SBox).HeightOverride(1.0f)
				[
					SNew(SBorder).BorderImage(FAppStyle::GetBrush("WhiteBrush")).BorderBackgroundColor(CyanDim())
				]
			]
			+ SVerticalBox::Slot().FillHeight(1.0f).Padding(0.0f, 16.0f, 0.0f, 0.0f)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().Padding(0.0f, 0.0f, 16.0f, 0.0f)
				[
					SNew(SBox).WidthOverride(290.0f)
					[
						SNew(SVerticalBox)
						+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 8.0f)
						[
							TabButton(EAPSChromeGlyph::Civilization, LOCTEXT("Overview", "OVERVIEW"),
								LOCTEXT("OverviewDetails", "Civilization and home"), ETab::Overview)
						]
						+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 8.0f)
						[
							TabButton(EAPSChromeGlyph::Pilot, LOCTEXT("PilotTab", "PILOT"),
								LOCTEXT("PilotDetails", "Status, surroundings, current tasks"), ETab::Pilot)
						]
						+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 8.0f)
						[
							// One map with modes (Rio, 02.10): the system map, the system scheme and the surface.
							TabButton(EAPSChromeGlyph::Compass, LOCTEXT("MapTab", "MAP"),
								LOCTEXT("MapDetails", "Stars, system, scheme, surface"), ETab::Map)
						]
						+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 8.0f)
						[
							// Rio 02.10: fleet orders back in the main menu, before the infrastructure.
							TabButton(EAPSChromeGlyph::Fleet, LOCTEXT("FleetTab", "FLEET ORDERS"),
								LOCTEXT("FleetTabDetails", "Pick ships, give orders (K)"), ETab::Fleet)
						]
						+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 8.0f)
						[
							TabButton(EAPSChromeGlyph::Infrastructure, LOCTEXT("Colony", "INFRASTRUCTURE"),
								LOCTEXT("ColonyDetails", "Network, construction, object pages"), ETab::Colony)
						]
						+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 8.0f)
						[
							TabButton(EAPSChromeGlyph::Ship, LOCTEXT("ShipyardTab", "SHIPYARD"),
								LOCTEXT("ShipyardDetails", "Build ships for the fleet"), ETab::Shipyard)
						]
						+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 8.0f)
						[
							TabButton(EAPSChromeGlyph::Divisions, LOCTEXT("Divisions", "DIVISIONS"),
								LOCTEXT("DivisionsDetails", "Exploration, industry, science, fleet"), ETab::Divisions)
						]
						+ SVerticalBox::Slot().AutoHeight()
						[
							TabButton(EAPSChromeGlyph::Recent, LOCTEXT("Journal", "JOURNAL"),
								LOCTEXT("JournalDetails", "What the civilization did"), ETab::Journal)
						]
					]
				]
				+ SHorizontalBox::Slot().FillWidth(1.0f)
				[
					ChamferPanel(
						SAssignNew(Switcher, SWidgetSwitcher)
						+ SWidgetSwitcher::Slot()[BuildOverview()]
						+ SWidgetSwitcher::Slot()[BuildMap()]
						+ SWidgetSwitcher::Slot()[BuildColony()]
						+ SWidgetSwitcher::Slot()[BuildFleet()]
						+ SWidgetSwitcher::Slot()[BuildDivisions()]
						+ SWidgetSwitcher::Slot()[BuildJournal()]
						+ SWidgetSwitcher::Slot()[BuildShipyard()]
						+ SWidgetSwitcher::Slot()[BuildScheme()]
						+ SWidgetSwitcher::Slot()[BuildPilot()]
						+ SWidgetSwitcher::Slot()[BuildSurface()]
						+ SWidgetSwitcher::Slot()[BuildStars()],
						FMargin(22.0f, 18.0f), CyanDim())
				]
			]
		]
	];

	if (UWorld* LiveWorld = World.Get())
	{
		if (UAPSCivilizationJournalSubsystem* Journal = LiveWorld->GetSubsystem<UAPSCivilizationJournalSubsystem>())
		{
			JournalHandle = Journal->OnEntryAdded().AddSP(this, &SAPSColonyTerminal::HandleJournalEntry);
		}
	}
	RebuildJournal();
	RebuildCatalogue();
	RefreshConstruction();
}

SAPSColonyTerminal::~SAPSColonyTerminal()
{
	if (UWorld* LiveWorld = World.Get())
	{
		if (UAPSCivilizationJournalSubsystem* Journal = LiveWorld->GetSubsystem<UAPSCivilizationJournalSubsystem>())
		{
			Journal->OnEntryAdded().Remove(JournalHandle);
		}
	}
}

TSharedRef<SWidget> SAPSColonyTerminal::BuildOverview()
{
	// Rio 04.10 ("unreadable, ugly: improve all of it"): headline numbers with gauges and three cards that lead on to
	// the map, fleet orders and infrastructure tabs, instead of sixteen equal tiles (SAPSCivilizationOverview).
	return SNew(SAPSCivilizationOverview).World(World)
		.OnOpenTab(FAPSOverviewOpenTab::CreateSP(this, &SAPSColonyTerminal::ShowTab));
}

TSharedRef<SWidget> SAPSColonyTerminal::BuildDivisions()
{
	// Rio 02.10 ("still very unreadable: turn the text into visual parameters"): the cards are their own widget,
	// level rings, stat chips, growth bars and hull icons over the same live values (SAPSDivisionsPanel).
	return SNew(SAPSDivisionsPanel).World(World);
}

TSharedRef<SWidget> SAPSColonyTerminal::BuildColony()
{
	using namespace APSColonyUI;
	const auto StateText = [this]()
	{
		const UAPSCivilizationMaterializationSubsystem* Colony = World.IsValid()
			? World->GetSubsystem<UAPSCivilizationMaterializationSubsystem>() : nullptr;
		if (!Colony)
		{
			return LOCTEXT("NoColony", "NO COLONY SYSTEM");
		}
		switch (Colony->GetRuntimeManifest().MaterializationState)
		{
		case EAPSCivilizationMaterializationState::Materialized: return LOCTEXT("Materialized", "FOUNDED");
		case EAPSCivilizationMaterializationState::LoadedFromSave: return LOCTEXT("Loaded", "RESTORED");
		case EAPSCivilizationMaterializationState::Blocked: return LOCTEXT("Blocked", "NO SAFE SITE");
		default: return LOCTEXT("Pending", "SEARCHING FOR A SITE");
		}
	};
	const auto ObjectCard = [this](const EAPSChromeGlyph Glyph, const FText& Label, const int32 RoleIndex)
	{
		// Rio 02.10 ("each row clickable into its object page"): the card opens the actor's page in this tab.
		return ChromeButton(
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
			[
				IconBadge(Glyph, Cyan(), 38.0f)
			]
			+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center).Padding(12.0f, 0.0f)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(STextBlock).Text(Label).Font(Font("Bold", 13)).ColorAndOpacity(White())
					.RenderTransform(CapsCenterShift(Font("Bold", 13)))
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 3.0f, 0.0f, 0.0f)
				[
					SNew(STextBlock).Text_Lambda([this, RoleIndex]() { return DescribeColonyActor(RoleIndex); })
					.Font(Font("Regular", 11)).ColorAndOpacity(Muted())
				]
			]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Text_Lambda([this, RoleIndex]()
				{
					return ColonyActors[RoleIndex].IsValid() ? LOCTEXT("Present", "PRESENT") : LOCTEXT("Missing", "NOT YET");
				})
				.ColorAndOpacity_Lambda([this, RoleIndex]()
				{
					return FSlateColor(ColonyActors[RoleIndex].IsValid() ? Success() : Amber());
				})
				.Font(Font("Bold", 11)).RenderTransform(CapsCenterShift(Font("Bold", 11)))
			],
			FOnClicked::CreateLambda([this, RoleIndex]()
			{
				OpenObjectWindow(ColonyActors[RoleIndex].Get());
				return FReply::Handled();
			}),
			TAttribute<bool>(false), Cyan());
	};

	const TSharedRef<SWidget> Holdings = SNew(SScrollBox)
		+ SScrollBox::Slot().Padding(0.0f, 0.0f, 0.0f, 12.0f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(1.0f)
			[
				IconSectionHeading(EAPSChromeGlyph::Headquarters, LOCTEXT("ColonySection", "HOME COLONY"),
					LOCTEXT("ColonySubtitle", "Placed on the WorldScape terrain by the surface resolver"))
			]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Top).Padding(12.0f, 6.0f, 0.0f, 0.0f)
			[
				SNew(STextBlock).Text_Lambda(StateText).Font(Font("Bold", 12)).ColorAndOpacity(Amber())
			]
		]
		+ SScrollBox::Slot().Padding(0.0f, 0.0f, 0.0f, 8.0f)
		[
			ObjectCard(EAPSChromeGlyph::Infrastructure, LOCTEXT("Base", "BASE MODULE"), 0)
		]
		+ SScrollBox::Slot().Padding(0.0f, 0.0f, 0.0f, 8.0f)
		[
			ObjectCard(EAPSChromeGlyph::Station, LOCTEXT("Pad", "LANDING PAD"), 1)
		]
		+ SScrollBox::Slot()
		[
			ObjectCard(EAPSChromeGlyph::Ship, LOCTEXT("Ship", "HOME SHIP"), 2)
		]
		// C3 (Rio, 02.10): the colony is one kind of object; stations, shipyards, headquarters and outposts follow.
		+ SScrollBox::Slot().Padding(0.0f, 24.0f, 0.0f, 0.0f)
		[
			IconSectionHeading(EAPSChromeGlyph::Station, LOCTEXT("OrbitalSection", "STATIONS, SHIPYARDS, OUTPOSTS"),
				LOCTEXT("OrbitalSubtitle", "Everything the civilization holds, generated and built: a row opens its page"))
		]
		+ SScrollBox::Slot().Padding(0.0f, 10.0f, 0.0f, 0.0f)
		[
			SAssignNew(InfrastructureList, SVerticalBox)
		]
		// Construction lives with the colony: one civilization menu (Rio, 30.09).
		+ SScrollBox::Slot().Padding(0.0f, 28.0f, 0.0f, 0.0f)
		[
			BuildConstruction()
		];

	// Rio 02.10 ("every kind of object may have its own options"): the terminal's own screens for an object, after the
	// runtime's actions on its page: fleet orders aimed at it, its surface, its place on the system map, a shipyard's
	// slipway, a ship picked for orders, the colony's modules.
	const auto ScreenActions = [this](AActor* Object, TArray<FAPSObjectAction>& OutActions)
	{
		if (!Object)
		{
			return;
		}
		const TWeakObjectPtr<AActor> Weak = Object;
		const auto Add = [&OutActions](const FName Id, const FText& Label, const FText& Detail, TFunction<FText()> Execute)
		{
			FAPSObjectAction& Action = OutActions.AddDefaulted_GetRef();
			Action.Id = Id;
			Action.Label = Label;
			Action.Detail = Detail;
			Action.Group = LOCTEXT("ScreensGroup", "OPEN IN THE TERMINAL");
			Action.Colour = Cyan();
			Action.Execute = MoveTemp(Execute);
		};
		// On foot the course goes to the home ship, as the map's SET COURSE does; a pilot has the navigation actions.
		const APlayerController* Controller = World.IsValid() ? World->GetFirstPlayerController() : nullptr;
		if (!(Controller && Cast<ASpaceship>(Controller->GetPawn())) && GetCourseShip() && !Object->IsA<ASpaceship>())
		{
			FAPSObjectAction& Course = OutActions.AddDefaulted_GetRef();
			Course.Id = TEXT("Screen.HomeShipCourse");
			Course.Label = LOCTEXT("ScreenCourse", "COURSE FOR THE HOME SHIP");
			Course.Detail = LOCTEXT("ScreenCourseDetail", "On foot: the home ship's navigation marks it; board it and fly.");
			Course.Group = LOCTEXT("ScreenCourseGroup", "NAVIGATION");
			Course.Colour = FLinearColor(0.30f, 0.80f, 1.00f, 1.0f);
			Course.Execute = [this, Weak]()
			{
				ASpaceship* CourseShip = GetCourseShip();
				AActor* Target = Weak.Get();
				return Target && CourseShip && CourseShip->ShipNavigation && CourseShip->ShipNavigation->SetCourse(Target->GetPathName())
					? FText::Format(LOCTEXT("ScreenCourseSet", "COURSE SET: {0}"), FAPSFleetCommand::DisplayName(Target))
					: LOCTEXT("ScreenCourseRefused", "This object is not charted for navigation.");
			};
		}
		if (Object->IsA<APlanetaryBody>())
		{
			Add(TEXT("Screen.FleetOrders"), LOCTEXT("ScreenFleet", "FLEET ORDERS HERE"),
				LOCTEXT("ScreenFleetDetail", "The fleet orders map with this world as the target: pick ships and give the order."),
				[this, Weak]()
				{
					FleetTarget = Weak;
					FleetMessage = FText::GetEmpty();
					SelectTab(ETab::Fleet);
					if (FleetMap.IsValid() && FleetTarget.IsValid())
					{
						FleetMap->SelectById(FleetTarget->GetPathName());
					}
					UpdateBodyPreview();
					return FText::GetEmpty();
				});
			Add(TEXT("Screen.Surface"), LOCTEXT("ScreenSurface", "SURFACE MAP"),
				LOCTEXT("ScreenSurfaceDetail", "The world's surface map with what stands on it and above it."),
				[this, Weak]()
				{
					if (SurfaceMap.IsValid())
					{
						SurfaceMap->SetBody(Cast<APlanetaryBody>(Weak.Get()));
					}
					SelectTab(ETab::Surface);
					return FText::GetEmpty();
				});
		}
		// The system map shows the home system: its star, worlds and ships, and what orbits or stands on its worlds.
		if (Object->IsA<APlanetaryBody>() || Object->IsA<AStar>() || Object->IsA<ASpaceship>()
			|| (Object->IsA<ATechActor>() && APSInfrastructureUI::BodyOf(Object)))
		{
			Add(TEXT("Screen.SystemMap"), LOCTEXT("ScreenMap", "SHOW ON THE SYSTEM MAP"),
				LOCTEXT("ScreenMapDetail", "The system map with this object picked: a planet in the system's view, anything else in its planet's."),
				[this, Weak]()
				{
					SelectTab(ETab::Map);
					AActor* Target = Weak.Get();
					if (!Map.IsValid() || !Target)
					{
						return FText::GetEmpty();
					}
					// The planet whose local view holds it: a moon's, an orbiting structure's or ship's (none for a planet or star).
					AActor* Planet = nullptr;
					if (!Target->IsA<APlanet>() && !Target->IsA<AStar>())
					{
						APlanetaryBody* Body = Cast<APlanetaryBody>(Target);
						if (!Body)
						{
							Body = APSInfrastructureUI::BodyOf(Target);
						}
						const AMoon* Moon = Cast<AMoon>(Body);
						Planet = Moon ? static_cast<AActor*>(Moon->ParentPlanet) : static_cast<AActor*>(Body);
					}
					Map->Focus(Planet);
					Map->SelectById(Target->GetPathName());
					MapListSignature.Reset();
					RefreshMap();
					return FText::GetEmpty();
				});
		}
		if (ASpaceShipyard* Yard = Cast<ASpaceShipyard>(Object))
		{
			const TWeakObjectPtr<ASpaceShipyard> WeakYard = Yard;
			Add(TEXT("Screen.Shipyard"), LOCTEXT("ScreenYard", "OPEN ITS SLIPWAY"),
				LOCTEXT("ScreenYardDetail", "SHIPYARD with this yard picked: order ships built here."),
				[this, WeakYard]()
				{
					SelectedYard = WeakYard;
					SelectTab(ETab::Shipyard);
					return FText::GetEmpty();
				});
		}
		if (ASpaceship* Ship = Cast<ASpaceship>(Object); Ship && GetFleet() && GetFleet()->FindUnit(Ship))
		{
			const TWeakObjectPtr<ASpaceship> WeakShip = Ship;
			Add(TEXT("Screen.PickShip"), LOCTEXT("ScreenPick", "PICK FOR ORDERS"),
				LOCTEXT("ScreenPickDetail", "FLEET ORDERS with this ship picked."),
				[this, WeakShip]()
				{
					PickedUnits.Reset();
					if (WeakShip.IsValid())
					{
						PickedUnits.Add(WeakShip);
					}
					SelectTab(ETab::Fleet);
					return FText::GetEmpty();
				});
		}
		if (Object == ColonyActors[0].Get() || Object == ColonyActors[1].Get() || Object->IsA<ASpaceHeadquarters>())
		{
			Add(TEXT("Screen.ColonyModules"), LOCTEXT("ScreenModules", "COLONY MODULES"),
				LOCTEXT("ScreenModulesDetail", "HOLDINGS AND COLONY: modules for the headquarters or the surface base."),
				[this]()
				{
					if (InfrastructurePanel.IsValid())
					{
						InfrastructurePanel->ShowSection(SAPSInfrastructurePanel::ESection::Holdings);
					}
					return FText::GetEmpty();
				});
		}
	};
	// Rio 02.10 ("greatly expand the infrastructure ...; now it is just a list and it is unclear what to do with it"): the
	// tab is the infrastructure panel (stocks, network map, construction catalogue, object pages); these lists and the
	// colony's modules are its HOLDINGS section.
	return SAssignNew(InfrastructurePanel, SAPSInfrastructurePanel)
		.World(World)
		.Holdings(Holdings)
		.ExtraActions(ScreenActions);
}

TSharedRef<SWidget> SAPSColonyTerminal::BuildFleet()
{
	using namespace APSColonyUI;
	// Division filter chips with live counts: ALL, then the four divisions in their colours.
	TSharedRef<SWrapBox> Filters = SNew(SWrapBox).UseAllottedSize(true).InnerSlotPadding(FVector2D(6.0f, 6.0f));
	for (int32 Filter = -1; Filter < static_cast<int32>(APSFleet::EDivision::Count); ++Filter)
	{
		const FLinearColor Accent = Filter < 0 ? Cyan() : APSFleet::DivisionColour(static_cast<APSFleet::EDivision>(Filter));
		Filters->AddSlot()
		[
			ChromeButton(
				SNew(STextBlock).Font(Font("Bold", 9)).ColorAndOpacity(Accent)
				.Text_Lambda([this, Filter]()
				{
					int32 Count = 0;
					if (const FAPSFleetCommand* Fleet = GetFleet())
					{
						for (const FAPSFleetUnit& Unit : Fleet->GetUnits())
						{
							Count += Filter < 0 || static_cast<int32>(Unit.Division) == Filter ? 1 : 0;
						}
					}
					return FText::Format(LOCTEXT("FilterChip", "{0}  {1}"), Filter < 0 ? LOCTEXT("FilterAll", "ALL")
						: APSFleet::DivisionName(static_cast<APSFleet::EDivision>(Filter)), APSUINumber::Number(Count));
				}),
				FOnClicked::CreateSP(this, &SAPSColonyTerminal::SetDivisionFilter, Filter),
				TAttribute<bool>::CreateLambda([this, Filter]() { return DivisionFilter == Filter; }), Accent)
		];
	}

	// One order button, dim with the reason underneath when no picked ship can take it.
	const auto OrderButton = [this](const APSFleet::EOrder Order)
	{
		return SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()
			[
				PrimaryButton(APSFleet::OrderName(Order), FOnClicked::CreateSP(this, &SAPSColonyTerminal::GiveOrder, Order),
					TAttribute<bool>::CreateLambda([this, Order]() { return OrderRefusal(Order).IsEmpty(); }))
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(2.0f, 3.0f, 0.0f, 0.0f)
			[
				SNew(STextBlock).AutoWrapText(true).Font(Font("Regular", 10)).ColorAndOpacity(Muted())
				.Text_Lambda([this, Order]() { return OrderRefusal(Order); })
				.Visibility_Lambda([this, Order]()
				{
					return GetPickedShips().IsEmpty() || OrderRefusal(Order).IsEmpty() ? EVisibility::Collapsed : EVisibility::Visible;
				})
			];
	};

	// Rio 02.10: fleet orders are their own tab now, without the map's mode bar.
	return SNew(SVerticalBox)
		+ SVerticalBox::Slot().FillHeight(1.0f)
		[
	SNew(SHorizontalBox)
		// Units: filter, pick-all, the cards.
		+ SHorizontalBox::Slot().AutoWidth().Padding(0.0f, 0.0f, 16.0f, 0.0f)
		[
			SNew(SBox).WidthOverride(350.0f)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight()
				[
					IconSectionHeading(EAPSChromeGlyph::Fleet, LOCTEXT("FleetSection", "FLEET COMMAND"),
						LOCTEXT("FleetSubtitle", "Pick ships, pick a world on the map, give the order"))
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 12.0f, 0.0f, 0.0f)
				[
					Filters
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 10.0f, 0.0f, 8.0f)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().AutoWidth()
					[
						ChromeButton(SNew(STextBlock).Text(LOCTEXT("PickAll", "PICK ALL SHOWN")).Font(Font("Bold", 9))
							.ColorAndOpacity(Cyan()), FOnClicked::CreateSP(this, &SAPSColonyTerminal::PickAllShown),
							TAttribute<bool>(false), Cyan())
					]
					+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center).Padding(10.0f, 0.0f, 0.0f, 0.0f)
					[
						SNew(STextBlock).Font(Font("Bold", 10)).ColorAndOpacity(Amber())
						.RenderTransform(CapsCenterShift(Font("Bold", 10)))
						.Text_Lambda([this]()
						{
							return FText::Format(LOCTEXT("PickedCount", "{0} PICKED  /  CTRL ADDS"), APSUINumber::Number(GetPickedShips().Num()));
						})
					]
				]
				+ SVerticalBox::Slot().FillHeight(1.0f)
				[
					SNew(SScrollBox)
					+ SScrollBox::Slot()
					[
						SAssignNew(FleetList, SVerticalBox)
					]
				]
			]
		]
		// The map: the ships in their divisions' colours, the orders under way, the worlds and what is known of them.
		+ SHorizontalBox::Slot().FillWidth(1.0f)
		[
			SNew(SOverlay)
			+ SOverlay::Slot()
			[
				SAssignNew(FleetMap, SAPSCivilizationMap)
				.World(World)
				.Visibility_Lambda([this]() { return bFleetStars ? EVisibility::Collapsed : EVisibility::Visible; })
				.OnSelectionChanged_Lambda([this]() { HandleFleetMapSelection(); })
			]
			// Rio 05.10 (star map): the target among the stars around (the star scheme's picker): a click aims the order at a
			// star system, a double click shows a system that stands on the system map.
			+ SOverlay::Slot()
			[
				SAssignNew(FleetStars, SAPSStarScheme)
				.World(World)
				.Style(SAPSStarScheme::EStyle::Picker)
				.Visibility_Lambda([this]() { return bFleetStars ? EVisibility::Visible : EVisibility::Collapsed; })
				.OnPicked_Lambda([this](const FGuid& SystemId)
				{
					FAPSStarSystems* Stars = APSStarSystemsFind(World.Get());
					if (AActor* Anchor = Stars ? Stars->GetAnchor(SystemId) : nullptr)
					{
						FleetTarget = Anchor;
						FleetMessage = FText::GetEmpty();
						RefreshFleet(false);
						UpdateBodyPreview();
					}
				})
				.OnOpened_Lambda([this](const FGuid& SystemId)
				{
					FAPSStarSystems* Stars = APSStarSystemsFind(World.Get());
					if (Stars && APSStarMap::FindStarActor(World.Get(), *Stars, Stars->IndexOf(SystemId)))
					{
						bFleetStars = false;
						if (FleetMap.IsValid())
						{
							FleetMap->Focus(nullptr);
						}
						return;
					}
					bFleetMessageIsError = true;
					FleetMessage = LOCTEXT("FleetStarAway", "Its worlds show on the system map while the system stands: fly there in person.");
				})
			]
			// The view's name and, in a planet's local view, the way back to the system.
			+ SOverlay::Slot().HAlign(HAlign_Left).VAlign(VAlign_Top).Padding(8.0f)
			[
				SNew(SVerticalBox)
				// Rio 05.10 (star map): what the target is picked on: the system (worlds, stations) or the stars around.
				+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 8.0f)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(2.0f, 0.0f, 10.0f, 0.0f)
					[
						SNew(STextBlock).Text(LOCTEXT("FleetTargetOn", "TARGET")).Font(Font("Regular", 9)).ColorAndOpacity(Muted())
					]
					+ SHorizontalBox::Slot().AutoWidth().Padding(0.0f, 0.0f, 6.0f, 0.0f)
					[
						ChromeButton(SNew(STextBlock).Text(LOCTEXT("FleetTargetSystem", "SYSTEM")).Font(Font("Bold", 10))
							.ColorAndOpacity_Lambda([this]() { return FSlateColor(bFleetStars ? Muted() : Cyan()); }),
							FOnClicked::CreateLambda([this]() { bFleetStars = false; return FReply::Handled(); }),
							TAttribute<bool>::CreateLambda([this]() { return !bFleetStars; }), Cyan())
					]
					+ SHorizontalBox::Slot().AutoWidth().Padding(0.0f, 0.0f, 6.0f, 0.0f)
					[
						ChromeButton(SNew(STextBlock).Text(LOCTEXT("FleetTargetStars", "STARS")).Font(Font("Bold", 10))
							.ColorAndOpacity_Lambda([this]() { return FSlateColor(bFleetStars ? Cyan() : Muted()); }),
							FOnClicked::CreateLambda([this]()
							{
								bFleetStars = true;
								RefreshFleet(false);
								return FReply::Handled();
							}),
							TAttribute<bool>::CreateLambda([this]() { return bFleetStars; }), Cyan())
					]
					+ SHorizontalBox::Slot().AutoWidth()
					[
						SNew(SBox).Visibility_Lambda([this]() { return bFleetStars ? EVisibility::Visible : EVisibility::Collapsed; })
						[
							APSStarMapUI::LegendToggle()
						]
					]
				]
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(STextBlock).Font(Font("Bold", 11)).ColorAndOpacity(Cyan())
					.Text_Lambda([this]()
					{
						return bFleetStars ? (FleetStars.IsValid() ? FleetStars->GetTitle() : FText::GetEmpty())
							: FleetMap.IsValid() ? FleetMap->GetViewTitle() : FText::GetEmpty();
					})
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 6.0f, 0.0f, 0.0f)
				[
					SNew(SBox)
					.Visibility_Lambda([this]()
					{
						return !bFleetStars && FleetMap.IsValid() && FleetMap->IsLocalView() ? EVisibility::Visible : EVisibility::Collapsed;
					})
					[
						ChromeButton(SNew(STextBlock).Text(LOCTEXT("FleetSystemView", "BACK TO THE SYSTEM")).Font(Font("Bold", 9))
							.ColorAndOpacity(Cyan()),
							FOnClicked::CreateLambda([this]()
							{
								if (FleetMap.IsValid())
								{
									FleetMap->Focus(nullptr);
								}
								return FReply::Handled();
							}), TAttribute<bool>(false), Cyan())
					]
				]
			]
			+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center).Padding(24.0f)
			[
				SNew(SBox)
				.Visibility_Lambda([this]() { return bConstructionOpen ? EVisibility::Visible : EVisibility::Collapsed; })
				[
					BuildConstructionCatalogue()
				]
			]
		]
		// Orders: the pick, the target and what the civilization knows of it, the order buttons.
		+ SHorizontalBox::Slot().AutoWidth().Padding(16.0f, 0.0f, 0.0f, 0.0f)
		[
			SNew(SBox).WidthOverride(320.0f)
			[
				SNew(SScrollBox)
				+ SScrollBox::Slot()
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight()
					[
						SNew(STextBlock).Text(LOCTEXT("OrdersTitle", "ORDERS")).Font(Font("Bold", 14)).ColorAndOpacity(White())
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 4.0f, 0.0f, 0.0f)
					[
						SNew(STextBlock).AutoWrapText(true).Font(Font("Bold", 10)).ColorAndOpacity(Cyan())
						.Text_Lambda([this]()
						{
							const FAPSFleetCommand* Fleet = GetFleet();
							TArray<FString> Signs;
							for (const ASpaceship* Ship : GetPickedShips())
							{
								const FAPSFleetUnit* Unit = Fleet ? Fleet->FindUnit(Ship) : nullptr;
								Signs.Add(Unit ? Unit->CallSign : Ship->GetName());
							}
							return Signs.IsEmpty() ? LOCTEXT("NoPick", "No ships picked: click a card or a ship on the map.")
								: FText::Format(LOCTEXT("Picked", "SHIPS: {0}"), FText::FromString(FString::Join(Signs, TEXT(", "))));
						})
					]
						+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 12.0f, 0.0f, 0.0f)
					[
						ChamferPanel(
							SNew(SVerticalBox)
							// The surveyed world's globe (B1; Rio 02.10: "remove the black background"): its own surface on a
							// transparent background, coarse while only surveyed; drag turns it.
							+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0.0f, 0.0f, 0.0f, 8.0f)
							[
								SNew(SBox).WidthOverride(220.0f).HeightOverride(220.0f)
								.Visibility_Lambda([this]()
								{
									const FAPSFleetCommand* Fleet = GetFleet();
									const APlanetaryBody* Target = Cast<APlanetaryBody>(FleetTarget.Get());
									return Target && Fleet && Fleet->GetSurvey(Target) != APSFleet::ESurvey::Unknown && OrdersGlobe.IsValid()
										&& OrdersGlobe->GetBody() == Target && OrdersGlobe->HasSurface() ? EVisibility::Visible : EVisibility::Collapsed;
								})
								[
									SAssignNew(OrdersGlobe, SAPSSurfaceMap).World(World).GlobeOnly(true)
								]
							]
							+ SVerticalBox::Slot().AutoHeight()
							[
								SNew(STextBlock).Font(Font("Bold", 13)).ColorAndOpacity(White())
								.Text_Lambda([this]()
								{
									const AActor* Target = FleetTarget.Get();
									const SAPSCivilizationMap::FObject* Object = FleetMap.IsValid() ? FleetMap->GetSelected() : nullptr;
									return !Target ? LOCTEXT("NoTarget", "NO TARGET")
										: Object && Object->Actor.Get() == Target ? Object->Name
										: FAPSFleetCommand::DisplayName(Target);
								})
							]
							+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 3.0f, 0.0f, 0.0f)
							[
								SNew(STextBlock).AutoWrapText(true).Font(Font("Bold", 10)).ColorAndOpacity(Cyan())
								.Text_Lambda([this]()
								{
									const AActor* Target = FleetTarget.Get();
									if (!Target)
									{
										return bFleetStars ? LOCTEXT("TargetHelpStars", "Click a star on the map: the order goes to its system.")
											: LOCTEXT("TargetHelp", "Click a planet, moon, station or outpost on the map. Double-click a planet for its moons.");
									}
									// Rio 05.10 (star map): a star system picked among the stars.
									FText SystemKind, SystemBody, SystemAnomaly;
									if (APSStarMapUI::DescribeSystemTarget(World.Get(), Target, SystemKind, SystemBody, SystemAnomaly))
									{
										return SystemKind;
									}
									const FAPSFleetCommand* Fleet = GetFleet();
									if (Target->IsA<APlanetaryBody>() && Fleet)
									{
										using APSFleet::EStructure;
										const int32 Shipyards = Fleet->CountStructures(Target, EStructure::Shipyard);
										const int32 Headquarters = Fleet->CountStructures(Target, EStructure::Headquarters);
										return FText::Format(LOCTEXT("TargetSurveyBuilt", "{0}  /  OUTPOSTS {1} OF 3  /  STATIONS {2}{3}{4}"),
											APSFleet::SurveyName(Fleet->GetSurvey(Target)), APSUINumber::Number(Fleet->CountOutposts(Target)),
											APSUINumber::Number(Fleet->CountStructures(Target, EStructure::Station)),
											Shipyards > 0 ? LOCTEXT("TargetShipyard", "  /  SHIPYARD") : FText::GetEmpty(),
											Headquarters > 0 ? LOCTEXT("TargetHeadquarters", "  /  HQ") : FText::GetEmpty());
									}
									return LOCTEXT("TargetStation", "STATION OR OUTPOST: ships can fly there");
								})
							]
							+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 6.0f, 0.0f, 0.0f)
							[
								SNew(STextBlock).AutoWrapText(true).Font(Font("Regular", 11)).ColorAndOpacity(Muted())
								.Text_Lambda([this]()
								{
									FText SystemKind, SystemBody, SystemAnomaly;
									if (APSStarMapUI::DescribeSystemTarget(World.Get(), FleetTarget.Get(), SystemKind, SystemBody, SystemAnomaly))
									{
										return SystemBody;
									}
									const FAPSFleetCommand* Fleet = GetFleet();
									const FAPSFleetBodyRecord* Record = Fleet ? Fleet->FindBody(FleetTarget.Get()) : nullptr;
									if (!Record || Record->Findings.IsEmpty())
									{
										return FleetTarget.IsValid() && FleetTarget->IsA<APlanetaryBody>()
											? LOCTEXT("NoFindings", "No data yet: send an exploration or science ship to survey it.")
											: FText::GetEmpty();
									}
									return FText::Join(FText::FromString(TEXT("\n")), Record->Findings);
								})
							]
							// The world's anomaly, when the fleet knows of one (Rio, 01.10).
							+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 6.0f, 0.0f, 0.0f)
							[
								SNew(STextBlock).AutoWrapText(true).Font(Font("Bold", 11)).ColorAndOpacity(Amber())
								.Text_Lambda([this]()
								{
									// Rio 05.10 (star map): a star system's anomaly as the star systems know it.
									FText SystemKind, SystemBody, SystemAnomaly;
									if (APSStarMapUI::DescribeSystemTarget(World.Get(), FleetTarget.Get(), SystemKind, SystemBody, SystemAnomaly))
									{
										return SystemAnomaly;
									}
									const FAPSFleetCommand* Fleet = GetFleet();
									return Fleet ? Fleet->DescribeAnomaly(FleetTarget.Get()) : FText::GetEmpty();
								})
								.Visibility_Lambda([this]()
								{
									FText SystemKind, SystemBody, SystemAnomaly;
									if (APSStarMapUI::DescribeSystemTarget(World.Get(), FleetTarget.Get(), SystemKind, SystemBody, SystemAnomaly))
									{
										return SystemAnomaly.IsEmpty() ? EVisibility::Collapsed : EVisibility::Visible;
									}
									const FAPSFleetCommand* Fleet = GetFleet();
									return Fleet && !Fleet->DescribeAnomaly(FleetTarget.Get()).IsEmpty()
										? EVisibility::Visible : EVisibility::Collapsed;
								})
							],
							FMargin(14.0f, 12.0f), CyanDim())
					]
					// Rio 05.10 (star map): each picked ship's way to a star system target: from where, how far, about when.
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 10.0f, 0.0f, 0.0f)
					[
						SNew(SAPSStarRoutes)
						.World(World)
						.Ships([this]() { return GetPickedShips(); })
						.Target([this]() { return FleetTarget.Get(); })
						.Visibility_Lambda([this]()
						{
							return APSStarMapUI::SystemOf(FleetTarget.Get()).IsValid() && !PickedUnits.IsEmpty()
								? EVisibility::Visible : EVisibility::Collapsed;
						})
					]
					// Rio 02.10 ("OPEN should open a full page"): the target's object page in INFRASTRUCTURE, with every
					// action for it (construction from the catalogue included).
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 8.0f, 0.0f, 0.0f)
					[
						SNew(SBox)
						.Visibility_Lambda([this]() { return FleetTarget.IsValid() ? EVisibility::Visible : EVisibility::Collapsed; })
						[
							ChromeButton(SNew(STextBlock).Text(LOCTEXT("FleetOpenTarget", "OPEN THE TARGET'S PAGE")).Justification(ETextJustify::Center)
								.Font(Font("Bold", 10)).ColorAndOpacity(Cyan()),
								FOnClicked::CreateLambda([this]()
								{
									OpenObjectWindow(FleetTarget.Get());
									return FReply::Handled();
								}), TAttribute<bool>(false), Cyan())
						]
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 14.0f, 0.0f, 0.0f)[OrderButton(APSFleet::EOrder::Move)]
					// Rio 05.10 (star map): a star system target takes its own orders (a probe, a survey of the system) in
					// place of a world's survey.
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 8.0f, 0.0f, 0.0f)
					[
						SNew(SBox)
						.Visibility_Lambda([this]()
						{
							return APSStarMapUI::SystemOf(FleetTarget.Get()).IsValid() ? EVisibility::Collapsed : EVisibility::Visible;
						})
						[
							OrderButton(APSFleet::EOrder::Survey)
						]
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 8.0f, 0.0f, 0.0f)
					[
						SNew(SBox)
						.Visibility_Lambda([this]()
						{
							return APSStarMapUI::SystemOf(FleetTarget.Get()).IsValid() ? EVisibility::Visible : EVisibility::Collapsed;
						})
						[
							OrderButton(APSFleet::EOrder::Probe)
						]
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 8.0f, 0.0f, 0.0f)
					[
						SNew(SBox)
						.Visibility_Lambda([this]()
						{
							return APSStarMapUI::SystemOf(FleetTarget.Get()).IsValid() ? EVisibility::Visible : EVisibility::Collapsed;
						})
						[
							OrderButton(APSFleet::EOrder::SurveySystem)
						]
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 8.0f, 0.0f, 0.0f)[OrderButton(APSFleet::EOrder::Expedition)]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 8.0f, 0.0f, 0.0f)[OrderButton(APSFleet::EOrder::Return)]
					// Construction (Rio, 01.10: "after an outpost, a headquarters, a station, infrastructure"): what a
					// construction ship raises at a surveyed world, each step opening the next.
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 18.0f, 0.0f, 0.0f)
					[
						SNew(STextBlock).Text(LOCTEXT("ConstructionOrders", "CONSTRUCTION")).Font(Font("Bold", 12))
						.ColorAndOpacity(APSFleet::DivisionColour(APSFleet::EDivision::Construction))
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 2.0f, 0.0f, 0.0f)
					[
						SNew(STextBlock).AutoWrapText(true).Font(Font("Regular", 10)).ColorAndOpacity(Muted())
						.Text(LOCTEXT("ConstructionHelp", "Construction ships raise outposts, stations, shipyards and sector HQs at a surveyed world."))
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 8.0f, 0.0f, 0.0f)
					[
						PrimaryButton(LOCTEXT("OpenConstruction", "CONSTRUCTION..."),
							FOnClicked::CreateLambda([this]() { bConstructionOpen = !bConstructionOpen; return FReply::Handled(); }),
							TAttribute<bool>::CreateLambda([this]() { return FleetTarget.IsValid(); }))
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 10.0f, 0.0f, 0.0f)
					[
						ChromeButton(SNew(STextBlock).Text(LOCTEXT("CancelOrders", "CANCEL ORDERS")).Justification(ETextJustify::Center)
							.Font(Font("Bold", 10)).ColorAndOpacity(Amber()),
							FOnClicked::CreateSP(this, &SAPSColonyTerminal::CancelPickedOrders), TAttribute<bool>(false), Amber())
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 10.0f, 0.0f, 0.0f)
					[
						SNew(STextBlock).AutoWrapText(true).Font(Font("Bold", 10))
						.Text_Lambda([this]() { return FleetMessage; })
						.ColorAndOpacity_Lambda([this]() { return FSlateColor(bFleetMessageIsError ? Amber() : Success()); })
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 16.0f, 0.0f, 0.0f)
					[
						SNew(STextBlock).AutoWrapText(true).Font(Font("Regular", 10)).ColorAndOpacity(Muted())
						.Text_Lambda([this]()
						{
							const FAPSFleetCommand* Fleet = GetFleet();
							if (!Fleet)
							{
								return LOCTEXT("NoFleetCommand", "Fleet command is not running in this world.");
							}
							int32 Surveyed = 0, Studied = 0, Outposts = 0, Built = 0;
							for (const FAPSFleetBodyRecord& Record : Fleet->GetBodies())
							{
								Surveyed += Record.Survey == APSFleet::ESurvey::Surveyed ? 1 : 0;
								Studied += Record.Survey == APSFleet::ESurvey::Studied ? 1 : 0;
								Outposts += Fleet->CountOutposts(Record.Body.Get());
							}
							for (const FAPSFleetStructure& Structure : Fleet->GetStructures())
							{
								Built += Structure.bBuilt && Structure.Actor.IsValid() ? 1 : 0;
							}
							return FText::Format(LOCTEXT("KnownWorldsBuilt", "KNOWN WORLDS: {0} studied, {1} surveyed  /  OUTPOSTS BUILT: {2}  /  STATIONS, SHIPYARDS AND HQS BUILT: {3}"),
								APSUINumber::Number(Studied), APSUINumber::Number(Surveyed), APSUINumber::Number(Outposts), APSUINumber::Number(Built));
						})
					]
				]
			]
		]
		];
}

FAPSFleetCommand* SAPSColonyTerminal::GetFleet() const
{
	return APSFleetFind(World.Get());
}

TArray<ASpaceship*> SAPSColonyTerminal::GetPickedShips() const
{
	TArray<ASpaceship*> Ships;
	for (const TWeakObjectPtr<ASpaceship>& Ship : PickedUnits)
	{
		if (Ship.IsValid())
		{
			Ships.Add(Ship.Get());
		}
	}
	return Ships;
}

void SAPSColonyTerminal::RefreshFleet(const bool bForceRebuild)
{
	using namespace APSColonyUI;
	UpdateBodyPreview();
	if (FleetMap.IsValid())
	{
		FleetMap->Refresh();
		if (UWorld* LiveWorld = World.Get(); LiveWorld && !bFleetMapFocused)
		{
			bFleetMapFocused = true;
			for (TActorIterator<AAstroGenerator> It(LiveWorld); It; ++It)
			{
				if (IsValid(*It) && IsValid(It->HomePlanet))
				{
					FleetMap->Focus(It->HomePlanet);
					break;
				}
			}
		}
		TArray<TWeakObjectPtr<AActor>> Highlighted;
		for (const TWeakObjectPtr<ASpaceship>& Ship : PickedUnits)
		{
			Highlighted.Add(Ship.Get());
		}
		FleetMap->SetHighlightedShips(Highlighted);
		// The order target wears the course brackets on this map.
		FleetMap->SetCourseTargetId(FleetTarget.IsValid() ? FleetTarget->GetPathName() : FString());
	}
	// Rio 05.10 (star map): the STARS map marks the target system, where the picked ships are and their ways there.
	if (FleetStars.IsValid())
	{
		FleetStars->SetSelected(APSStarMapUI::SystemOf(FleetTarget.Get()));
		FleetStars->SetHighlightedShips(PickedUnits);
		FleetStars->SetPlannedRoutes(APSStarMapUI::PlannedRoutes(World.Get(), GetPickedShips(), FleetTarget.Get()));
	}
	if (!FleetList.IsValid())
	{
		return;
	}
	const FAPSFleetCommand* Fleet = GetFleet();
	// Cards change only with the units, their divisions or the filter; their text is read live.
	FString Signature = FString::Printf(TEXT("%d|"), DivisionFilter);
	if (Fleet)
	{
		for (const FAPSFleetUnit& Unit : Fleet->GetUnits())
		{
			Signature += FString::Printf(TEXT("%s:%d;"), *Unit.CallSign, static_cast<int32>(Unit.Division));
		}
	}
	if (!bForceRebuild && Signature == FleetSignature)
	{
		return;
	}
	FleetSignature = Signature;
	FleetList->ClearChildren();
	if (!Fleet || Fleet->GetUnits().IsEmpty())
	{
		FleetList->AddSlot().AutoHeight()
		[
			SNew(STextBlock).AutoWrapText(true).Font(Font("Regular", 11)).ColorAndOpacity(Muted())
			.Text(Fleet ? LOCTEXT("NoFleet", "No ships of the civilization in this world yet.")
				: LOCTEXT("NoFleetCommandList", "Fleet command is not running in this world."))
		];
		return;
	}
	for (const FAPSFleetUnit& Unit : Fleet->GetUnits())
	{
		if (DivisionFilter < 0 || static_cast<int32>(Unit.Division) == DivisionFilter)
		{
			FleetList->AddSlot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 6.0f)
			[
				BuildUnitCard(Unit.Ship)
			];
		}
	}
}

TSharedRef<SWidget> SAPSColonyTerminal::BuildUnitCard(const TWeakObjectPtr<ASpaceship> Ship)
{
	using namespace APSColonyUI;
	const auto Unit = [this, Ship]() -> const FAPSFleetUnit*
	{
		const FAPSFleetCommand* Fleet = GetFleet();
		return Fleet ? Fleet->FindUnit(Ship.Get()) : nullptr;
	};
	const auto DivisionColour = [Unit]()
	{
		const FAPSFleetUnit* Found = Unit();
		return Found ? APSFleet::DivisionColour(Found->Division) : Muted();
	};
	return ChromeButton(
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().AutoWidth().Padding(0.0f, 0.0f, 10.0f, 0.0f)
		[
			SNew(SBox).WidthOverride(4.0f)
			[
				SNew(SBorder).BorderImage(FAppStyle::GetBrush("WhiteBrush"))
				.BorderBackgroundColor_Lambda([DivisionColour]() { return FSlateColor(DivisionColour()); })
			]
		]
		+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(STextBlock).Font(Font("Bold", 12)).ColorAndOpacity(White()).RenderTransform(CapsCenterShift(Font("Bold", 12)))
				.Text_Lambda([Unit, Ship]()
				{
					const FAPSFleetUnit* Found = Unit();
					return Found && Ship.IsValid() ? FText::FromString(Found->CallSign) : LOCTEXT("CardGone", "GONE");
				})
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 3.0f, 0.0f, 0.0f)
			[
				SNew(STextBlock).AutoWrapText(true).Font(Font("Bold", 9))
				.Text_Lambda([this, Unit]()
				{
					const FAPSFleetUnit* Found = Unit();
					const FAPSFleetCommand* Fleet = GetFleet();
					return Found && Fleet ? Fleet->DescribeState(*Found) : FText::GetEmpty();
				})
				.ColorAndOpacity_Lambda([Unit]()
				{
					const FAPSFleetUnit* Found = Unit();
					return FSlateColor(!Found || Found->Order == APSFleet::EOrder::None ? Muted()
						: Found->Phase == APSFleet::EPhase::Working ? Amber()
						: Found->Phase == APSFleet::EPhase::Holding ? Success() : Cyan());
				})
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 2.0f, 0.0f, 0.0f)
			[
				SNew(STextBlock).Font(Font("Regular", 10)).ColorAndOpacity(Muted())
				.Text_Lambda([this, Ship]()
				{
					const ASpaceship* Live = Ship.Get();
					return Live ? FText::Format(LOCTEXT("CardDetail", "CLASS {0}  /  {1} away"),
						FText::FromString(Live->GetSizeClassName()), FText::FromString(APSColonyUI::PawnDistance(World.Get(), Live)))
						: FText::GetEmpty();
				})
			]
		]
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(8.0f, 0.0f, 0.0f, 0.0f)
		[
			// The division, one click to the next.
			ChromeButton(
				SNew(STextBlock).Font(Font("Bold", 8))
				.Text_Lambda([Unit]()
				{
					const FAPSFleetUnit* Found = Unit();
					return Found ? APSFleet::DivisionName(Found->Division) : FText::GetEmpty();
				})
				.ColorAndOpacity_Lambda([DivisionColour]() { return FSlateColor(DivisionColour()); }),
				FOnClicked::CreateSP(this, &SAPSColonyTerminal::CycleUnitDivision, Ship), TAttribute<bool>(false),
				Cyan())
		],
		FOnClicked::CreateSP(this, &SAPSColonyTerminal::PickUnit, Ship),
		TAttribute<bool>::CreateLambda([this, Ship]() { return PickedUnits.Contains(Ship); }),
		Cyan());
}

FReply SAPSColonyTerminal::PickUnit(const TWeakObjectPtr<ASpaceship> Ship)
{
	if (!Ship.IsValid())
	{
		return FReply::Handled();
	}
	// Ctrl or Shift adds or removes; a plain click picks only this ship (again: none).
	const FModifierKeysState Modifiers = FSlateApplication::Get().GetModifierKeys();
	if (Modifiers.IsControlDown() || Modifiers.IsShiftDown())
	{
		if (PickedUnits.Remove(Ship) == 0)
		{
			PickedUnits.Add(Ship);
		}
	}
	else
	{
		const bool bOnlyThis = PickedUnits.Num() == 1 && PickedUnits[0] == Ship;
		PickedUnits.Reset();
		if (!bOnlyThis)
		{
			PickedUnits.Add(Ship);
		}
	}
	FleetMessage = FText::GetEmpty();
	RefreshFleet(false);
	return FReply::Handled();
}

FReply SAPSColonyTerminal::CycleUnitDivision(const TWeakObjectPtr<ASpaceship> Ship)
{
	FAPSFleetCommand* Fleet = GetFleet();
	if (const FAPSFleetUnit* Unit = Fleet ? Fleet->FindUnit(Ship.Get()) : nullptr)
	{
		Fleet->SetDivision(Ship.Get(), APSFleet::NextDivision(Unit->Division));
		RefreshFleet(false);
	}
	return FReply::Handled();
}

FReply SAPSColonyTerminal::SetDivisionFilter(const int32 Filter)
{
	DivisionFilter = Filter;
	RefreshFleet(true);
	return FReply::Handled();
}

FReply SAPSColonyTerminal::PickAllShown()
{
	PickedUnits.Reset();
	if (const FAPSFleetCommand* Fleet = GetFleet())
	{
		for (const FAPSFleetUnit& Unit : Fleet->GetUnits())
		{
			if ((DivisionFilter < 0 || static_cast<int32>(Unit.Division) == DivisionFilter) && Unit.Ship.IsValid()
				&& !Unit.Ship->HasPilot())
			{
				PickedUnits.Add(Unit.Ship);
			}
		}
	}
	RefreshFleet(false);
	return FReply::Handled();
}

void SAPSColonyTerminal::HandleFleetMapSelection()
{
	const SAPSCivilizationMap::FObject* Selected = FleetMap.IsValid() ? FleetMap->GetSelected() : nullptr;
	if (!Selected || !Selected->Actor.IsValid())
	{
		return;
	}
	// A ship on the map is picked like its card; anything else becomes the target.
	if (Selected->Kind == SAPSCivilizationMap::EKind::Ship)
	{
		if (ASpaceship* Ship = Cast<ASpaceship>(Selected->Actor.Get()))
		{
			PickUnit(Ship);
		}
		return;
	}
	if (Selected->Kind != SAPSCivilizationMap::EKind::Pilot && Selected->Kind != SAPSCivilizationMap::EKind::Star)
	{
		FleetTarget = Selected->Actor;
		FleetMessage = FText::GetEmpty();
		RefreshFleet(false);
		UpdateBodyPreview();
	}
}

void SAPSColonyTerminal::UpdateBodyPreview()
{
	if (!OrdersGlobe.IsValid())
	{
		return;
	}
	// Only a surveyed world shows its globe; the globe reads the survey level itself (coarse until studied).
	APlanetaryBody* Target = Cast<APlanetaryBody>(FleetTarget.Get());
	const FAPSFleetCommand* Fleet = GetFleet();
	if (!Target || !Fleet || Fleet->GetSurvey(Target) == APSFleet::ESurvey::Unknown)
	{
		Target = nullptr;
	}
	if (OrdersGlobe->GetBody() != Target)
	{
		OrdersGlobe->SetBody(Target);
	}
	else if (Target)
	{
		OrdersGlobe->RefreshMarkers();
	}
}

FText SAPSColonyTerminal::OrderRefusal(const APSFleet::EOrder Order) const
{
	const FAPSFleetCommand* Fleet = GetFleet();
	if (!Fleet)
	{
		return LOCTEXT("RefuseNoFleet", "Fleet command is not running in this world.");
	}
	const TArray<ASpaceship*> Ships = GetPickedShips();
	if (Ships.IsEmpty())
	{
		return LOCTEXT("RefuseNoPick", "Pick ships first.");
	}
	FText First;
	for (const ASpaceship* Ship : Ships)
	{
		// The piloted ship takes MOVE as its autopilot (Rio 02.10).
		if (Order == APSFleet::EOrder::Move && Ship->HasPilot() && FleetTarget.IsValid())
		{
			return FText::GetEmpty();
		}
		const FText Refusal = Fleet->CheckOrder(Ship, Order, FleetTarget.Get());
		if (Refusal.IsEmpty())
		{
			return FText::GetEmpty();
		}
		if (First.IsEmpty())
		{
			const FAPSFleetUnit* Unit = Fleet->FindUnit(Ship);
			First = Ships.Num() > 1 && Unit
				? FText::Format(LOCTEXT("RefuseUnit", "{0}: {1}"), FText::FromString(Unit->CallSign), Refusal) : Refusal;
		}
	}
	return First;
}

FReply SAPSColonyTerminal::GiveOrder(const APSFleet::EOrder Order)
{
	FAPSFleetCommand* Fleet = GetFleet();
	if (!Fleet)
	{
		return FReply::Handled();
	}
	FText Refusal;
	TArray<ASpaceship*> Ships = GetPickedShips();
	// MOVE for the piloted ship engages its autopilot; the crewless ones take the order as before (Rio 02.10).
	bool bAutopilot = false;
	if (Order == APSFleet::EOrder::Move && FleetTarget.IsValid())
	{
		for (int32 Index = Ships.Num() - 1; Index >= 0; --Index)
		{
			if (Ships[Index]->HasPilot() && Ships[Index]->FlightModel)
			{
				Ships[Index]->FlightModel->EngageAutopilot(FleetTarget.Get());
				Ships.RemoveAt(Index);
				bAutopilot = true;
			}
		}
	}
	const int32 Issued = (Ships.IsEmpty() ? 0 : Fleet->IssueOrder(Ships, Order, FleetTarget.Get(), Refusal)) + (bAutopilot ? 1 : 0);
	bFleetMessageIsError = Issued == 0;
	FleetMessage = Issued == 0 ? Refusal : FText::Format(LOCTEXT("OrderGiven", "ORDER GIVEN: {0} ({1} ships){2}"),
		APSFleet::OrderName(Order), APSUINumber::Number(Issued), Refusal.IsEmpty() ? FText::GetEmpty()
			: FText::Format(LOCTEXT("OrderPartly", "\nNot all: {0}"), Refusal));
	RefreshFleet(false);
	return FReply::Handled();
}

FReply SAPSColonyTerminal::CancelPickedOrders()
{
	if (FAPSFleetCommand* Fleet = GetFleet())
	{
		for (const ASpaceship* Ship : GetPickedShips())
		{
			Fleet->CancelOrder(Ship);
		}
		FleetMessage = LOCTEXT("OrdersCancelled", "Orders cancelled: the ships hold where they are.");
		bFleetMessageIsError = false;
	}
	return FReply::Handled();
}

TSharedRef<SWidget> SAPSColonyTerminal::BuildMapModes()
{
	using namespace APSColonyUI;
	const auto Mode = [this](const FText& Label, const ETab Tab, const EAPSChromeGlyph Glyph)
	{
		return ChromeButton(
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
			[
				SNew(SBox).WidthOverride(18.0f).HeightOverride(18.0f)
				[SNew(SAPSVectorGlyph).Glyph(Glyph).Color(Cyan()).StrokeWidth(1.25f)]
			]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(8.0f, 0.0f, 0.0f, 0.0f)
			[
				SNew(STextBlock).Text(Label).Font(Font("Bold", 10)).ColorAndOpacity(White())
				.RenderTransform(CapsCenterShift(Font("Bold", 10)))
			],
			FOnClicked::CreateSP(this, &SAPSColonyTerminal::SelectTab, Tab),
			TAttribute<bool>::CreateLambda([this, Tab]() { return ActiveTab == Tab; }), Cyan());
	};
	return SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().AutoWidth().Padding(0.0f, 0.0f, 8.0f, 0.0f)
		[
			// Rio 05.10: the STAR level first, the system below it.
			Mode(LOCTEXT("ModeStars", "STAR MAP"), ETab::Stars, EAPSChromeGlyph::Stars)
		]
		+ SHorizontalBox::Slot().AutoWidth().Padding(0.0f, 0.0f, 8.0f, 0.0f)
		[
			// "STRATEGIC MAP" is the F10 universe view (Rio 02.10); this mode is the system from above.
			Mode(LOCTEXT("ModeStrategic", "SYSTEM MAP"), ETab::Map, EAPSChromeGlyph::Compass)
		]
		+ SHorizontalBox::Slot().AutoWidth().Padding(0.0f, 0.0f, 8.0f, 0.0f)
		[
			Mode(LOCTEXT("ModeScheme", "SYSTEM SCHEME"), ETab::Scheme, EAPSChromeGlyph::System)
		]
		+ SHorizontalBox::Slot().AutoWidth()
		[
			Mode(LOCTEXT("ModeSurface", "SURFACE"), ETab::Surface, EAPSChromeGlyph::Planet)
		];
}

TSharedRef<SWidget> SAPSColonyTerminal::BuildScheme()
{
	using namespace APSColonyUI;
	// 0 designation and name, 1 kind and type, 2 size.
	const auto PickedText = [this](const int32 Field) -> FText
	{
		const AActor* Picked = Scheme.IsValid() ? Scheme->GetPicked() : nullptr;
		if (!Picked)
		{
			return Field == 0 ? LOCTEXT("SchemeNothing", "NOTHING PICKED")
				: Field == 1 ? LOCTEXT("SchemePick", "Click a star, planet or moon in the scheme.") : FText::GetEmpty();
		}
		const FString Designation = APSBodyDesignation::Of(Picked);
		if (const AStar* Star = Cast<AStar>(Picked))
		{
			const double RadiusKm = Star->RadiusKM > 0.0 ? Star->RadiusKM : static_cast<double>(Star->StarRadiusKM);
			return Field == 0 ? FText::FromString(FString::Printf(TEXT("%s  %s"), *Designation, *Star->AstroName.ToString().ToUpper()))
				: Field == 1 ? FText::Format(LOCTEXT("SchemeStarKind", "STAR  /  {0}"), FText::FromName(Star->FullSpectralName))
				: FText::Format(LOCTEXT("SchemeStarSize", "Radius {0} km, {1} solar radii"), APSUINumber::Number(FMath::RoundToInt(RadiusKm)),
					APSUINumber::Number(RadiusKm / 695700.0, &FNumberFormattingOptions().SetMaximumFractionalDigits(2)));
		}
		if (const APlanetaryBody* Body = Cast<APlanetaryBody>(Picked))
		{
			return Field == 0 ? FText::FromString(FString::Printf(TEXT("%s  %s"), *Designation, *Body->AstroName.ToString().ToUpper()))
				: Field == 1 ? FText::Format(LOCTEXT("SchemeBodyKind", "{0}  /  {1}"),
					Picked->IsA<AMoon>() ? LOCTEXT("SchemeMoon", "MOON") : LOCTEXT("SchemePlanet", "PLANET"),
					FText::FromString(UEnum::GetDisplayValueAsText(Body->PlanetType).ToString().ToUpper()))
				: FText::Format(LOCTEXT("SchemeBodySize", "Radius {0} km, {1} Earth radii"), APSUINumber::Number(Body->PlanetRadiusKM),
					APSUINumber::Number(Body->PlanetRadiusKM / 6371.0, &FNumberFormattingOptions().SetMaximumFractionalDigits(2)));
		}
		return FText::GetEmpty();
	};
	const auto SetCourse = [this]()
	{
		const AActor* Picked = Scheme.IsValid() ? Scheme->GetPicked() : nullptr;
		ASpaceship* Ship = GetCourseShip();
		bCourseIsError = true;
		if (!Picked)
		{
			CourseMessage = LOCTEXT("SchemeCoursePick", "Pick a body in the scheme first.");
		}
		else if (!Ship || !Ship->ShipNavigation)
		{
			CourseMessage = LOCTEXT("SchemeCourseNoShip", "No ship to set a course for.");
		}
		else if (!Ship->ShipNavigation->SetCourse(Picked->GetPathName()))
		{
			CourseMessage = LOCTEXT("SchemeCourseNotCharted", "This body is not charted for navigation.");
		}
		else
		{
			CourseMessage = FText::Format(LOCTEXT("SchemeCourseSet", "COURSE SET: {0}"),
				FText::FromString(APSBodyDesignation::Of(Picked)));
			bCourseIsError = false;
		}
		return FReply::Handled();
	};
	return SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 12.0f)
		[
			BuildMapModes()
		]
		+ SVerticalBox::Slot().FillHeight(1.0f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(1.0f)
			[
				SAssignNew(Scheme, SAPSSystemScheme)
				.World(World)
				.OnPicked_Lambda([this](AActor*) { CourseMessage = FText::GetEmpty(); })
			]
			+ SHorizontalBox::Slot().AutoWidth().Padding(18.0f, 0.0f, 0.0f, 0.0f)
			[
				SNew(SBox).WidthOverride(330.0f)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight()
					[
						IconSectionHeading(EAPSChromeGlyph::System, LOCTEXT("SchemeSection", "SYSTEM SCHEME"),
							LOCTEXT("SchemeSubtitle", "Stars, planets and moons in order, sizes to scale"))
					]
					// Rio 05.10 (star map): a system opened from the star map leads back to it.
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 10.0f, 0.0f, 0.0f)
					[
						SNew(SBox)
						.Visibility_Lambda([this]() { return Scheme.IsValid() && Scheme->IsPinned() ? EVisibility::Visible : EVisibility::Collapsed; })
						[
							ChromeButton(SNew(STextBlock).Text(LOCTEXT("SchemeBackToStars", "<  BACK TO THE STAR MAP")).Font(Font("Bold", 10))
								.ColorAndOpacity(Cyan()),
								FOnClicked::CreateLambda([this]() { return SelectTab(ETab::Stars); }), TAttribute<bool>(false), Cyan())
						]
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 14.0f, 0.0f, 0.0f)
					[
						ChamferPanel(
							SNew(SVerticalBox)
							+ SVerticalBox::Slot().AutoHeight()
							[
								SNew(STextBlock).Text_Lambda([PickedText]() { return PickedText(0); })
								.Font(Font("Bold", 14)).ColorAndOpacity(White()).AutoWrapText(true)
							]
							+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 3.0f, 0.0f, 0.0f)
							[
								SNew(STextBlock).Text_Lambda([PickedText]() { return PickedText(1); }).AutoWrapText(true)
								.Font(Font("Bold", 10)).ColorAndOpacity(Cyan())
							]
							+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 6.0f, 0.0f, 0.0f)
							[
								SNew(STextBlock).Text_Lambda([PickedText]() { return PickedText(2); }).AutoWrapText(true)
								.Font(Font("Regular", 11)).ColorAndOpacity(Muted())
							],
							FMargin(14.0f, 12.0f), CyanDim())
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 10.0f, 0.0f, 0.0f)
					[
						PrimaryButton(LOCTEXT("SchemeSetCourse", "SET COURSE"), FOnClicked::CreateLambda(SetCourse),
							TAttribute<bool>::CreateLambda([this]() { return Scheme.IsValid() && Scheme->GetPicked() && GetCourseShip(); }))
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 8.0f, 0.0f, 0.0f)
					[
						SNew(STextBlock).Text_Lambda([this]() { return CourseMessage; }).AutoWrapText(true)
						.ColorAndOpacity_Lambda([this]() { return FSlateColor(bCourseIsError ? Amber() : Success()); })
						.Font(Font("Bold", 10))
					]
				]
			]
		];
}

TSharedRef<SWidget> SAPSColonyTerminal::BuildStars()
{
	// Rio 05.10 (star map): the STAR level with its card, actions and list (SAPSStarMapPanel); a system that stands opens
	// its scheme, FLEET ORDERS takes a system as the target.
	return SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 12.0f)
		[
			BuildMapModes()
		]
		+ SVerticalBox::Slot().FillHeight(1.0f)
		[
			SAssignNew(StarMapPanel, SAPSStarMapPanel)
			.World(World)
			.CourseShip([this]() { return GetCourseShip(); })
			.OnOpenScheme_Lambda([this](AActor* Star) { OpenSchemeOf(Star); })
			.OnFleetOrders_Lambda([this](const FGuid& SystemId) { OrderToSystem(SystemId); })
		];
}

void SAPSColonyTerminal::OpenSchemeOf(AActor* Star)
{
	if (!Scheme.IsValid() || !Star)
	{
		return;
	}
	Scheme->ShowSystem(Star);
	bKeepSchemePin = true;
	SelectTab(ETab::Scheme);
}

void SAPSColonyTerminal::OrderToSystem(const FGuid& SystemId)
{
	FAPSStarSystems* Stars = APSStarSystemsFind(World.Get());
	AActor* Anchor = Stars ? Stars->GetAnchor(SystemId) : nullptr;
	if (!Anchor)
	{
		return;
	}
	FleetTarget = Anchor;
	FleetMessage = FText::GetEmpty();
	bFleetStars = true;
	if (FleetStars.IsValid())
	{
		FleetStars->SetSelected(SystemId);
	}
	SelectTab(ETab::Fleet);
	UpdateBodyPreview();
}

void SAPSColonyTerminal::RefreshInfrastructure(const bool bForceRebuild)
{
	using namespace APSColonyUI;
	if (!InfrastructureList.IsValid())
	{
		return;
	}
	struct FRow
	{
		EAPSChromeGlyph Glyph;
		FText Kind;
		FText Name;
		FText Where;
		bool bBuilt;
		/** Rio 02.10 ("each row clickable into its object page"). */
		TWeakObjectPtr<AActor> Actor;
		FLinearColor Accent;
	};
	TArray<FRow> Rows;
	const FAPSFleetCommand* Fleet = GetFleet();
	const FAPSInfrastructure* Infrastructure = APSInfrastructureFind(World.Get());
	const auto Where = [](const APlanetaryBody* Body)
	{
		if (!Body) return LOCTEXT("InfraDeepSpace", "deep space");
		const FString Designation = APSBodyDesignation::Of(Body);
		return FText::FromString(FString::Printf(TEXT("%s%s%s"), *Designation, Designation.IsEmpty() ? TEXT("") : TEXT("  "),
			*Body->AstroName.ToString().ToUpper()));
	};
	if (Fleet)
	{
		for (const FAPSFleetStructure& Structure : Fleet->GetStructures())
		{
			AActor* Actor = Structure.Actor.Get();
			// The catalogue's structures follow with their own types.
			if (!Actor || (Infrastructure && Infrastructure->FindByActor(Actor))) continue;
			const bool bYard = Structure.Kind == APSFleet::EStructure::Shipyard;
			const bool bHq = Structure.Kind == APSFleet::EStructure::Headquarters;
			Rows.Add({bYard ? EAPSChromeGlyph::Shipyard : bHq ? EAPSChromeGlyph::Headquarters : EAPSChromeGlyph::Station,
				bYard ? LOCTEXT("InfraShipyard", "SHIPYARD") : bHq ? LOCTEXT("InfraHq", "HEADQUARTERS") : LOCTEXT("InfraStation", "STATION"),
				FAPSFleetCommand::DisplayName(Actor), Where(Structure.Body.Get()), Structure.bBuilt, Actor, Cyan()});
		}
		for (const FAPSFleetBodyRecord& Record : Fleet->GetBodies())
		{
			for (const TWeakObjectPtr<AActor>& Outpost : Record.Outposts)
			{
				if (Outpost.IsValid())
				{
					Rows.Add({EAPSChromeGlyph::Infrastructure, LOCTEXT("InfraOutpost", "OUTPOST"),
						FAPSFleetCommand::DisplayName(Outpost.Get()), Where(Record.Body.Get()), true, Outpost, Amber()});
				}
			}
		}
	}
	// Rio 02.10: what construction ships raised from the infrastructure catalogue, by type, department and place.
	if (Infrastructure)
	{
		for (const FAPSBuiltStructure& Built : Infrastructure->GetStructures())
		{
			const APSInfrastructure::FType* Type = APSInfrastructure::Find(Built.Type);
			AActor* Actor = Built.Actor.Get();
			if (!Type || !Actor) continue;
			const AActor* Site = Actor->GetAttachParentActor();
			const APlanetaryBody* Body = Cast<APlanetaryBody>(Site);
			Rows.Add({APSInfrastructureUI::CategoryGlyph(Type->Category),
				FText::Format(LOCTEXT("InfraCatalogueKind", "{0}  /  {1}"), APSInfrastructure::CategoryName(Type->Category),
					APSInfrastructure::DepartmentName(Type->Department)),
				FAPSFleetCommand::DisplayName(Actor), Body ? Where(Body) : APSObjectActions::NameOf(Site), true, Actor,
				APSInfrastructure::DepartmentColour(Type->Department)});
		}
	}
	FString Signature;
	for (const FRow& Row : Rows)
	{
		Signature += Row.Kind.ToString() + Row.Name.ToString() + Row.Where.ToString() + (Row.bBuilt ? TEXT("+") : TEXT("-"));
	}
	if (!bForceRebuild && Signature == InfrastructureSignature)
	{
		return;
	}
	InfrastructureSignature = Signature;
	InfrastructureList->ClearChildren();
	if (Rows.IsEmpty())
	{
		InfrastructureList->AddSlot().AutoHeight()
		[
			SNew(STextBlock).Text(LOCTEXT("InfraNone", "No stations, shipyards or outposts yet: construction ships build them (MAP, FLEET ORDERS)."))
			.AutoWrapText(true).Font(Font("Regular", 11)).ColorAndOpacity(Muted())
		];
		return;
	}
	for (const FRow& Row : Rows)
	{
		const TWeakObjectPtr<AActor> Actor = Row.Actor;
		InfrastructureList->AddSlot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 8.0f)
		[
			ChromeButton(
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
				[
					IconBadge(Row.Glyph, Row.Accent, 34.0f)
				]
				+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center).Padding(12.0f, 0.0f)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight()
					[
						SNew(STextBlock).Text(Row.Name).Font(Font("Bold", 12)).ColorAndOpacity(White())
						.RenderTransform(CapsCenterShift(Font("Bold", 12)))
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 3.0f, 0.0f, 0.0f)
					[
						SNew(STextBlock).Text(FText::Format(LOCTEXT("InfraWhere", "{0}  /  at {1}"), Row.Kind, Row.Where))
						.Font(Font("Regular", 10)).ColorAndOpacity(Muted())
					]
				]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
				[
					SNew(STextBlock).Text(Row.bBuilt ? LOCTEXT("InfraBuilt", "BUILT BY THE FLEET") : LOCTEXT("InfraFounded", "FOUNDED"))
					.Font(Font("Bold", 10)).ColorAndOpacity(Row.bBuilt ? Success() : Cyan())
					.RenderTransform(CapsCenterShift(Font("Bold", 10)))
				],
				FOnClicked::CreateLambda([this, Actor]()
				{
					OpenObjectWindow(Actor.Get());
					return FReply::Handled();
				}),
				TAttribute<bool>(false), Row.Accent)
		];
	}
}

TSharedRef<SWidget> SAPSColonyTerminal::BuildConstructionCatalogue()
{
	using namespace APSColonyUI;
	struct FEntry
	{
		APSFleet::EOrder Order;
		EAPSChromeGlyph Glyph;
		FText Name;
		FText Needs;
		FText Gives;
	};
	const FEntry Entries[] = {
		{APSFleet::EOrder::BuildOutpost, EAPSChromeGlyph::Infrastructure, LOCTEXT("CatOutpost", "OUTPOST"),
			LOCTEXT("CatOutpostNeeds", "A surveyed world and a construction ship."),
			LOCTEXT("CatOutpostGives", "A foothold in orbit: the world joins the civilization's map.")},
		{APSFleet::EOrder::BuildStation, EAPSChromeGlyph::Station, LOCTEXT("CatStation", "STATION"),
			LOCTEXT("CatStationNeeds", "An outpost at the world."),
			LOCTEXT("CatStationGives", "Work at this world goes 25% faster; opens shipyards and HQs.")},
		{APSFleet::EOrder::BuildShipyard, EAPSChromeGlyph::Shipyard, LOCTEXT("CatShipyard", "SHIPYARD"),
			LOCTEXT("CatShipyardNeeds", "A station at the world."),
			LOCTEXT("CatShipyardGives", "Its own slipway in SHIPYARD: ships are built here as well.")},
		{APSFleet::EOrder::BuildHeadquarters, EAPSChromeGlyph::Headquarters, LOCTEXT("CatHq", "SECTOR HQ"),
			LOCTEXT("CatHqNeeds", "A station at the world."),
			LOCTEXT("CatHqGives", "Every sector HQ makes the whole fleet fly 10% faster.")},
	};
	TSharedRef<SGridPanel> Grid = SNew(SGridPanel).FillColumn(0, 1.0f).FillColumn(1, 1.0f);
	for (int32 Index = 0; Index < UE_ARRAY_COUNT(Entries); ++Index)
	{
		const FEntry& Entry = Entries[Index];
		const APSFleet::EOrder Order = Entry.Order;
		Grid->AddSlot(Index % 2, Index / 2).Padding(6.0f)
		[
			SNew(SBox).WidthOverride(300.0f)
			[
				ChamferPanel(
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight()
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)[IconBadge(Entry.Glyph, Cyan(), 40.0f)]
						+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center).Padding(12.0f, 0.0f, 0.0f, 0.0f)
						[SNew(STextBlock).Text(Entry.Name).Font(Font("Bold", 14)).ColorAndOpacity(White())]
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 10.0f, 0.0f, 0.0f)
					[SNew(STextBlock).Text(LOCTEXT("CatNeeds", "NEEDS")).Font(Font("Bold", 8)).ColorAndOpacity(Muted())]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 2.0f, 0.0f, 0.0f)
					[SNew(STextBlock).Text(Entry.Needs).AutoWrapText(true).Font(Font("Regular", 10)).ColorAndOpacity(White())]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 8.0f, 0.0f, 0.0f)
					[SNew(STextBlock).Text(LOCTEXT("CatGives", "GIVES")).Font(Font("Bold", 8)).ColorAndOpacity(Muted())]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 2.0f, 0.0f, 0.0f)
					[SNew(STextBlock).Text(Entry.Gives).AutoWrapText(true).Font(Font("Regular", 10)).ColorAndOpacity(Cyan())]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 10.0f, 0.0f, 0.0f)
					[
						// Why the picked ships cannot raise it here, or that they can.
						SNew(STextBlock).AutoWrapText(true).Font(Font("Bold", 10))
						.Text_Lambda([this, Order]()
						{
							const FText Refusal = OrderRefusal(Order);
							return GetPickedShips().IsEmpty() ? LOCTEXT("CatPickShips", "Pick a construction ship first.")
								: Refusal.IsEmpty() ? LOCTEXT("CatReady", "READY TO ORDER") : Refusal;
						})
						.ColorAndOpacity_Lambda([this, Order]()
						{
							return FSlateColor(!GetPickedShips().IsEmpty() && OrderRefusal(Order).IsEmpty() ? Success() : Amber());
						})
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 10.0f, 0.0f, 0.0f)
					[
						PrimaryButton(LOCTEXT("CatOrder", "ORDER"),
							FOnClicked::CreateLambda([this, Order]()
							{
								GiveOrder(Order);
								bConstructionOpen = bFleetMessageIsError;
								return FReply::Handled();
							}),
							TAttribute<bool>::CreateLambda([this, Order]() { return !GetPickedShips().IsEmpty() && OrderRefusal(Order).IsEmpty(); }))
					],
					FMargin(16.0f, 14.0f), CyanDim())
			]
		];
	}
	return SNew(SBorder).BorderImage(FAppStyle::GetBrush("WhiteBrush")).BorderBackgroundColor(Panel())
		.Padding(FMargin(18.0f, 16.0f))
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center)
				[
					SNew(STextBlock).Font(Font("Bold", 16)).ColorAndOpacity(White())
					.Text_Lambda([this]()
					{
						const AActor* Target = FleetTarget.Get();
						const FString Designation = APSBodyDesignation::Of(Target);
						return FText::Format(LOCTEXT("CatTitle", "CONSTRUCTION AT {0}{1}"), FText::FromString(Designation.IsEmpty()
							? FString() : Designation + TEXT("  ")), Target ? FAPSFleetCommand::DisplayName(Target) : LOCTEXT("CatNoTarget", "..."));
					})
				]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
				[
					ChromeButton(SNew(STextBlock).Text(LOCTEXT("CatClose", "CLOSE")).Font(Font("Bold", 10)).ColorAndOpacity(Amber()),
						FOnClicked::CreateLambda([this]() { bConstructionOpen = false; return FReply::Handled(); }),
						TAttribute<bool>(false), Amber())
				]
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 4.0f, 0.0f, 10.0f)
			[
				SNew(STextBlock).Font(Font("Regular", 10)).ColorAndOpacity(Muted())
				.Text(LOCTEXT("CatSubtitle", "An outpost first, then a station; a station opens a shipyard and a sector HQ. Pick construction ships in the list on the left."))
			]
			+ SVerticalBox::Slot().AutoHeight()
			[
				Grid
			]
		];
}

void SAPSColonyTerminal::OpenObjectWindow(AActor* Actor)
{
	// Rio 02.10 ("I don't like this little popup: OPEN should open a full page in the INFRASTRUCTURE screen"): every OPEN
	// lands on the object's page there; the window over the map stays closed.
	ObjectWindowActor = Actor;
	bObjectWindowOpen = false;
	if (!Actor || !InfrastructurePanel.IsValid())
	{
		return;
	}
	SelectTab(ETab::Colony);
	InfrastructurePanel->ShowObject(Actor);
}

void SAPSColonyTerminal::RebuildObjectWindow()
{
	using namespace APSColonyUI;
	if (!ObjectWindowBox.IsValid())
	{
		return;
	}
	ObjectWindowBox->ClearChildren();
	AActor* Actor = ObjectWindowActor.Get();
	if (!Actor)
	{
		bObjectWindowOpen = false;
		return;
	}
	const FAPSFleetCommand* Fleet = GetFleet();
	const APlanetaryBody* Body = Cast<APlanetaryBody>(Actor);
	const AStar* Star = Cast<AStar>(Actor);
	ASpaceship* Ship = Cast<ASpaceship>(Actor);
	const FAPSFleetStructure* Structure = nullptr;
	if (Fleet)
	{
		for (const FAPSFleetStructure& Candidate : Fleet->GetStructures())
		{
			if (Candidate.Actor.Get() == Actor)
			{
				Structure = &Candidate;
				break;
			}
		}
	}
	const bool bColony = Actor == ColonyActors[0].Get() || Actor == ColonyActors[1].Get();
	const FString Designation = APSBodyDesignation::Of(Actor);
	const auto TypeName = [](const EPlanetType Type)
	{
		FString Name = UEnum::GetDisplayValueAsText(Type).ToString().ToUpper();
		Name.RemoveFromEnd(TEXT(" PLANET"));
		return FText::FromString(Name);
	};

	// What it is.
	EAPSChromeGlyph Glyph = EAPSChromeGlyph::Planet;
	FText Kind = LOCTEXT("ObjObject", "OBJECT");
	if (Star) { Glyph = EAPSChromeGlyph::System; Kind = LOCTEXT("ObjStar", "STAR"); }
	else if (Body) { Glyph = EAPSChromeGlyph::Planet; Kind = Actor->IsA<AMoon>() ? LOCTEXT("ObjMoon", "MOON") : LOCTEXT("ObjPlanet", "PLANET"); }
	else if (Ship) { Glyph = EAPSChromeGlyph::Ship; Kind = LOCTEXT("ObjShip", "SHIP"); }
	else if (Structure)
	{
		const bool bYard = Structure->Kind == APSFleet::EStructure::Shipyard;
		const bool bHq = Structure->Kind == APSFleet::EStructure::Headquarters;
		Glyph = bYard ? EAPSChromeGlyph::Shipyard : bHq ? EAPSChromeGlyph::Headquarters : EAPSChromeGlyph::Station;
		Kind = bYard ? LOCTEXT("ObjYard", "SHIPYARD") : bHq ? LOCTEXT("ObjHq", "HEADQUARTERS") : LOCTEXT("ObjStation", "STATION");
	}
	else if (bColony) { Glyph = EAPSChromeGlyph::Infrastructure; Kind = LOCTEXT("ObjColony", "HOME COLONY"); }
	const FText Title = Star || Body
		? FText::FromString(Star ? Star->AstroName.ToString().ToUpper() : Body->AstroName.ToString().ToUpper())
		: FAPSFleetCommand::DisplayName(Actor);

	ObjectWindowBox->AddSlot().AutoHeight()
	[
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)[IconBadge(Glyph, Cyan(), 44.0f)]
		+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center).Padding(12.0f, 0.0f)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(STextBlock).Font(Font("Bold", 10)).ColorAndOpacity(Cyan())
				.Text(FText::Format(LOCTEXT("ObjKindLine", "{0}{1}"), Kind,
					Designation.IsEmpty() ? FText::GetEmpty() : FText::FromString(TEXT("  ") + Designation)))
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 2.0f, 0.0f, 0.0f)
			[SNew(STextBlock).Text(Title).Font(Font("Bold", 18)).ColorAndOpacity(White())]
		]
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Top)
		[
			ChromeButton(SNew(STextBlock).Text(LOCTEXT("ObjClose", "CLOSE")).Font(Font("Bold", 10)).ColorAndOpacity(Amber()),
				FOnClicked::CreateLambda([this]() { bObjectWindowOpen = false; return FReply::Handled(); }),
				TAttribute<bool>(false), Amber())
		]
	];

	// Facts: label and value rows.
	TSharedRef<SVerticalBox> Facts = SNew(SVerticalBox);
	const auto Fact = [&Facts](const FText& Label, const FText& Value)
	{
		if (Value.IsEmpty()) return;
		Facts->AddSlot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 6.0f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Top)
			[SNew(SBox).WidthOverride(130.0f)[SNew(STextBlock).Text(Label).Font(Font("Bold", 9)).ColorAndOpacity(Muted())]]
			+ SHorizontalBox::Slot().FillWidth(1.0f)
			[SNew(STextBlock).Text(Value).AutoWrapText(true).Font(Font("Bold", 11)).ColorAndOpacity(White())]
		];
	};
	Fact(LOCTEXT("ObjDistance", "DISTANCE"), FText::FromString(PawnDistance(World.Get(), Actor)));
	if (Star)
	{
		Fact(LOCTEXT("ObjSpectrum", "SPECTRUM"), FText::FromName(Star->FullSpectralName));
		Fact(LOCTEXT("ObjStarRadius", "RADIUS"), FText::Format(LOCTEXT("ObjStarRadiusValue", "{0} km"),
			APSUINumber::Number(FMath::RoundToInt(Star->RadiusKM))));
	}
	if (Body)
	{
		Fact(LOCTEXT("ObjType", "TYPE"), TypeName(Body->PlanetType));
		Fact(LOCTEXT("ObjRadius", "RADIUS"), FText::Format(LOCTEXT("ObjRadiusValue", "{0} km"), APSUINumber::Number(Body->PlanetRadiusKM)));
		if (Fleet)
		{
			const APSFleet::ESurvey Survey = Fleet->GetSurvey(Body);
			Fact(LOCTEXT("ObjSurvey", "KNOWN"), Survey == APSFleet::ESurvey::Studied ? LOCTEXT("ObjStudied", "Studied")
				: Survey == APSFleet::ESurvey::Surveyed ? LOCTEXT("ObjSurveyed", "Surveyed") : LOCTEXT("ObjUnknown", "Not surveyed yet"));
			if (const FAPSFleetBodyRecord* Record = Fleet->FindBody(Body); Record && !Record->Findings.IsEmpty())
			{
				Fact(LOCTEXT("ObjFindings", "FINDINGS"), FText::Join(FText::FromString(TEXT("\n")), Record->Findings));
			}
			Fact(LOCTEXT("ObjAnomaly", "ANOMALY"), Fleet->DescribeAnomaly(Body));
			int32 Stations = 0, Yards = 0, Hqs = 0;
			for (const FAPSFleetStructure& Candidate : Fleet->GetStructures())
			{
				if (Candidate.Body.Get() != Body || !Candidate.Actor.IsValid()) continue;
				Stations += Candidate.Kind == APSFleet::EStructure::Station ? 1 : 0;
				Yards += Candidate.Kind == APSFleet::EStructure::Shipyard ? 1 : 0;
				Hqs += Candidate.Kind == APSFleet::EStructure::Headquarters ? 1 : 0;
			}
			Fact(LOCTEXT("ObjHeld", "HELD HERE"), FText::Format(
				LOCTEXT("ObjHeldValue", "{0} outposts, {1} stations, {2} shipyards, {3} HQs"),
				APSUINumber::Number(Fleet->CountOutposts(Body)), APSUINumber::Number(Stations), APSUINumber::Number(Yards), APSUINumber::Number(Hqs)));
		}
	}
	if (Ship && Fleet)
	{
		if (const FAPSFleetUnit* Unit = Fleet->FindUnit(Ship))
		{
			Fact(LOCTEXT("ObjCallSign", "CALL SIGN"), FText::FromString(Unit->CallSign));
			Fact(LOCTEXT("ObjDivision", "DIVISION"), APSFleet::DivisionName(Unit->Division));
			Fact(LOCTEXT("ObjOrder", "ORDER"), APSFleet::OrderName(Unit->Order));
			Fact(LOCTEXT("ObjSpeed", "SPEED"), FText::Format(LOCTEXT("ObjSpeedValue", "{0} km/s"),
				APSUINumber::Number(Unit->Speed / 100000.0, &FNumberFormattingOptions().SetMaximumFractionalDigits(1))));
		}
	}
	if (Structure)
	{
		Fact(LOCTEXT("ObjAt", "AT"), Structure->Body.IsValid() ? FText::FromString(FString::Printf(TEXT("%s  %s"),
			*APSBodyDesignation::Of(Structure->Body.Get()), *Structure->Body->AstroName.ToString().ToUpper())) : FText::GetEmpty());
		Fact(LOCTEXT("ObjOrigin", "ORIGIN"), Structure->bBuilt ? LOCTEXT("ObjBuilt", "Built by the fleet") : LOCTEXT("ObjFounded", "Founded with the civilization"));
	}
	ObjectWindowBox->AddSlot().AutoHeight().Padding(0.0f, 14.0f, 0.0f, 0.0f)[Facts];

	// What can be done.
	TSharedRef<SWrapBox> Actions = SNew(SWrapBox).UseAllottedSize(true).InnerSlotPadding(FVector2D(8.0f, 8.0f));
	const auto Action = [&Actions](const FText& Label, FOnClicked OnClicked)
	{
		Actions->AddSlot()
		[
			SNew(SBox).MinDesiredWidth(150.0f)
			[
				PrimaryButton(Label, OnClicked, TAttribute<bool>(true))
			]
		];
	};
	if (!Ship && GetCourseShip())
	{
		Action(LOCTEXT("ObjCourse", "SET COURSE"), FOnClicked::CreateLambda([this]()
		{
			AActor* Target = ObjectWindowActor.Get();
			ASpaceship* CourseShip = GetCourseShip();
			bCourseIsError = !(Target && CourseShip && CourseShip->ShipNavigation
				&& CourseShip->ShipNavigation->SetCourse(Target->GetPathName()));
			CourseMessage = bCourseIsError ? LOCTEXT("ObjCourseRefused", "This object is not charted for navigation.")
				: FText::Format(LOCTEXT("ObjCourseSet", "COURSE SET: {0}"), FAPSFleetCommand::DisplayName(Target));
			return FReply::Handled();
		}));
	}
	if (Body)
	{
		// Fleet orders with this world as the target; construction opens its catalogue there.
		const auto ToFleet = [this](const bool bConstruction)
		{
			return FOnClicked::CreateLambda([this, bConstruction]()
			{
				FleetTarget = ObjectWindowActor;
				FleetMessage = FText::GetEmpty();
				bObjectWindowOpen = false;
				SelectTab(ETab::Fleet);
				if (FleetMap.IsValid() && FleetTarget.IsValid()) FleetMap->SelectById(FleetTarget->GetPathName());
				UpdateBodyPreview();
				bConstructionOpen = bConstruction;
				return FReply::Handled();
			});
		};
		Action(LOCTEXT("ObjFleet", "FLEET ORDERS HERE"), ToFleet(false));
		Action(LOCTEXT("ObjConstruct", "CONSTRUCTION..."), ToFleet(true));
		Action(LOCTEXT("ObjSurface", "SURFACE MAP"), FOnClicked::CreateLambda([this]()
		{
			bObjectWindowOpen = false;
			if (SurfaceMap.IsValid()) SurfaceMap->SetBody(Cast<APlanetaryBody>(ObjectWindowActor.Get()));
			SelectTab(ETab::Surface);
			return FReply::Handled();
		}));
	}
	if (Structure && Structure->Kind == APSFleet::EStructure::Shipyard)
	{
		Action(LOCTEXT("ObjOpenYard", "OPEN SHIPYARD"), FOnClicked::CreateLambda([this]()
		{
			SelectedYard = Cast<ASpaceShipyard>(ObjectWindowActor.Get());
			bObjectWindowOpen = false;
			SelectTab(ETab::Shipyard);
			return FReply::Handled();
		}));
	}
	if (bColony || (Structure && Structure->Kind == APSFleet::EStructure::Headquarters))
	{
		Action(LOCTEXT("ObjInfrastructure", "INFRASTRUCTURE"), FOnClicked::CreateLambda([this]()
		{
			bObjectWindowOpen = false;
			SelectTab(ETab::Colony);
			return FReply::Handled();
		}));
	}
	if (Ship && Fleet && Fleet->FindUnit(Ship))
	{
		Action(LOCTEXT("ObjPickShip", "PICK FOR ORDERS"), FOnClicked::CreateLambda([this]()
		{
			if (ASpaceship* Picked = Cast<ASpaceship>(ObjectWindowActor.Get()))
			{
				PickedUnits.Reset();
				PickedUnits.Add(Picked);
			}
			bObjectWindowOpen = false;
			SelectTab(ETab::Fleet);
			return FReply::Handled();
		}));
	}
	ObjectWindowBox->AddSlot().AutoHeight().Padding(0.0f, 14.0f, 0.0f, 0.0f)[Actions];
	ObjectWindowBox->AddSlot().AutoHeight().Padding(0.0f, 8.0f, 0.0f, 0.0f)
	[
		SNew(STextBlock).Text_Lambda([this]() { return CourseMessage; }).AutoWrapText(true).Font(Font("Bold", 10))
		.ColorAndOpacity_Lambda([this]() { return FSlateColor(bCourseIsError ? Amber() : Success()); })
	];
}

void SAPSColonyTerminal::ShowTestOverlay(const int32 Overlay)
{
	bObjectWindowOpen = false;
	bConstructionOpen = false;
	// Rio 05.10 (star map): 3 is FLEET ORDERS with the target picked among the stars.
	bFleetStars = Overlay == 3;
	if (Overlay == 3)
	{
		SelectTab(ETab::Fleet);
	}
	if (Overlay == 1)
	{
		SelectTab(ETab::Map);
		if (const APlanet* Home = APSColonyUI::HomePlanet(World.Get()))
		{
			OpenObjectWindow(const_cast<APlanet*>(Home));
		}
	}
	else if (Overlay == 2)
	{
		SelectTab(ETab::Fleet);
		bConstructionOpen = true;
	}
}

TSharedRef<SWidget> SAPSColonyTerminal::BuildSurface()
{
	using namespace APSColonyUI;
	const auto Title = [this]() -> FText
	{
		const APlanetaryBody* Body = SurfaceMap.IsValid() ? SurfaceMap->GetBody() : nullptr;
		if (!Body) return LOCTEXT("SurfaceNoBody", "NO WORLD");
		const FString Designation = APSBodyDesignation::Of(Body);
		return FText::FromString(FString::Printf(TEXT("%s%s%s"), *Designation, Designation.IsEmpty() ? TEXT("") : TEXT("  "),
			*Body->AstroName.ToString().ToUpper()));
	};
	const auto Kind = [this]() -> FText
	{
		const APlanetaryBody* Body = SurfaceMap.IsValid() ? SurfaceMap->GetBody() : nullptr;
		if (!Body) return FText::GetEmpty();
		FString Type = UEnum::GetDisplayValueAsText(Body->PlanetType).ToString().ToUpper();
		Type.RemoveFromEnd(TEXT(" PLANET"));
		return FText::Format(LOCTEXT("SurfaceKind", "{0}  /  {1}  /  radius {2} km"),
			Body->IsA<AMoon>() ? LOCTEXT("SurfaceMoon", "MOON") : LOCTEXT("SurfacePlanet", "PLANET"), FText::FromString(Type),
			APSUINumber::Number(Body->PlanetRadiusKM));
	};
	const auto StepButton = [this](const FText& Label, const int32 Step)
	{
		// "<" and ">" sit on the symbols' middle line, not the capitals'.
		return ChromeButton(SNew(STextBlock).Text(Label).Font(Font("Bold", 11)).ColorAndOpacity(White())
				.RenderTransform(SymbolCenterShift(Font("Bold", 11))),
			FOnClicked::CreateSP(this, &SAPSColonyTerminal::StepSurfaceBody, Step), TAttribute<bool>(false), Cyan());
	};
	// Rio 02.10: the surface's looks - as it is, realistic, geology, anomalies.
	const auto ModeButton = [this](const SAPSSurfaceMap::EMode InMode)
	{
		return ChromeButton(SNew(STextBlock).Text(SAPSSurfaceMap::ModeName(InMode)).Font(Font("Bold", 10)).ColorAndOpacity(White())
				.Justification(ETextJustify::Center),
			FOnClicked::CreateLambda([this, InMode]()
			{
				if (SurfaceMap.IsValid()) SurfaceMap->SetMode(InMode);
				return FReply::Handled();
			}),
			TAttribute<bool>::CreateLambda([this, InMode]() { return SurfaceMap.IsValid() && SurfaceMap->GetMode() == InMode; }),
			Cyan());
	};
	return SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 12.0f)
		[
			BuildMapModes()
		]
		+ SVerticalBox::Slot().FillHeight(1.0f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(1.0f)
			[
				SAssignNew(SurfaceMap, SAPSSurfaceMap).World(World)
				// A double click on a marker opens its page in INFRASTRUCTURE.
				.OnOpenObject_Lambda([this](AActor* Actor) { OpenObjectWindow(Actor); })
			]
			+ SHorizontalBox::Slot().AutoWidth().Padding(18.0f, 0.0f, 0.0f, 0.0f)
			[
				SNew(SBox).WidthOverride(330.0f)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight()
					[
						IconSectionHeading(EAPSChromeGlyph::Planet, LOCTEXT("SurfaceSection", "SURFACE MAP"),
							LOCTEXT("SurfaceSubtitle", "The world from its own terrain profile, with what stands on it and above it"))
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 14.0f, 0.0f, 0.0f)
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().AutoWidth()[StepButton(LOCTEXT("SurfacePrevious", "<"), -1)]
						+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center).Padding(10.0f, 0.0f)
						[
							SNew(STextBlock).Text_Lambda(Title).Font(Font("Bold", 13)).ColorAndOpacity(White()).Justification(ETextJustify::Center)
						]
						+ SHorizontalBox::Slot().AutoWidth()[StepButton(LOCTEXT("SurfaceNext", ">"), 1)]
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 10.0f, 0.0f, 0.0f)
					[
						SNew(SVerticalBox)
						+ SVerticalBox::Slot().AutoHeight()
						[
							SNew(SHorizontalBox)
							+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(0.0f, 0.0f, 3.0f, 3.0f)[ModeButton(SAPSSurfaceMap::EMode::Terrain)]
							+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(3.0f, 0.0f, 0.0f, 3.0f)[ModeButton(SAPSSurfaceMap::EMode::Realistic)]
						]
						+ SVerticalBox::Slot().AutoHeight()
						[
							SNew(SHorizontalBox)
							+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(0.0f, 3.0f, 3.0f, 0.0f)[ModeButton(SAPSSurfaceMap::EMode::Geology)]
							+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(3.0f, 3.0f, 0.0f, 0.0f)[ModeButton(SAPSSurfaceMap::EMode::Scan)]
						]
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 10.0f, 0.0f, 0.0f)
					[
						ChamferPanel(
							SNew(SVerticalBox)
							+ SVerticalBox::Slot().AutoHeight()
							[
								SNew(STextBlock).Text_Lambda(Kind).Font(Font("Bold", 10)).ColorAndOpacity(Cyan()).AutoWrapText(true)
							]
							+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 6.0f, 0.0f, 0.0f)
							[
								SNew(STextBlock).AutoWrapText(true).Font(Font("Regular", 11)).ColorAndOpacity(Muted())
								.Text_Lambda([this]() { return SurfaceMap.IsValid() ? SurfaceMap->GetStatusText() : FText::GetEmpty(); })
							]
							+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 12.0f, 0.0f, 0.0f)
							[
								SNew(STextBlock).Text(LOCTEXT("SurfacePilot", "PILOT")).Font(Font("Bold", 9)).ColorAndOpacity(Muted())
							]
							+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 2.0f, 0.0f, 0.0f)
							[
								SNew(STextBlock).AutoWrapText(true).Font(Font("Bold", 11)).ColorAndOpacity(Amber())
								.Text_Lambda([this]() { return SurfaceMap.IsValid() ? SurfaceMap->GetPilotText() : FText::GetEmpty(); })
							]
							+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 12.0f, 0.0f, 0.0f)
							[
								SNew(STextBlock).Text(LOCTEXT("SurfacePicked", "PICKED")).Font(Font("Bold", 9)).ColorAndOpacity(Muted())
							]
							+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 2.0f, 0.0f, 0.0f)
							[
								SNew(STextBlock).AutoWrapText(true).Font(Font("Bold", 11)).ColorAndOpacity(White())
								.Text_Lambda([this]() { return SurfaceMap.IsValid() ? SurfaceMap->GetSelectionText() : FText::GetEmpty(); })
							]
							+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 12.0f, 0.0f, 0.0f)
							[
								SNew(STextBlock).Text(LOCTEXT("SurfaceMarkers", "ON THE MAP")).Font(Font("Bold", 9)).ColorAndOpacity(Muted())
							]
							+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 2.0f, 0.0f, 0.0f)
							[
								SNew(STextBlock).AutoWrapText(true).Font(Font("Regular", 11)).ColorAndOpacity(White())
								.Text_Lambda([this]() { return SurfaceMap.IsValid() ? SurfaceMap->GetLegendText() : FText::GetEmpty(); })
							],
							FMargin(14.0f, 12.0f), CyanDim())
					]
				]
			]
		];
}

FReply SAPSColonyTerminal::StepSurfaceBody(const int32 Step)
{
	if (!SurfaceMap.IsValid() || !World.IsValid()) return FReply::Handled();
	// The worlds of the shown world's system (else the nearest one), in designation order.
	const APlanetaryBody* Current = SurfaceMap->GetBody();
	if (!Current) Current = SAPSSurfaceMap::FindDefaultBody(World.Get());
	const auto StarOf = [](const APlanetaryBody* Body) -> const AStar*
	{
		const APlanet* Planet = Cast<APlanet>(Body);
		if (const AMoon* Moon = Cast<AMoon>(Body)) Planet = Moon->ParentPlanet;
		return Planet ? Planet->ParentStar : nullptr;
	};
	const AStar* Star = Current ? StarOf(Current) : nullptr;
	TArray<APlanetaryBody*> Bodies;
	for (TActorIterator<APlanetaryBody> It(World.Get()); It; ++It)
	{
		if (IsValid(*It) && (!Star || StarOf(*It) == Star)) Bodies.Add(*It);
	}
	if (Bodies.IsEmpty()) return FReply::Handled();
	Bodies.Sort([](const APlanetaryBody& A, const APlanetaryBody& B)
	{
		return APSBodyDesignation::Of(&A) < APSBodyDesignation::Of(&B);
	});
	const int32 Index = Bodies.IndexOfByKey(Current);
	const int32 Next = Index == INDEX_NONE ? 0 : (Index + Step + Bodies.Num()) % Bodies.Num();
	SurfaceMap->SetBody(Bodies[Next]);
	return FReply::Handled();
}

TSharedRef<SWidget> SAPSColonyTerminal::BuildPilot()
{
	// Rio 04.10 ("like the overview dashboard: the character's status, the main parameters, what goes on around"):
	// headline numbers, where the pilot is, what is near and what to do now (SAPSPilotDashboard).
	return SNew(SAPSPilotDashboard).World(World)
		.CourseShip([this]() { return GetCourseShip(); })
		.ColonyActor([this](const int32 Role)
		{
			return Role >= 0 && Role < static_cast<int32>(UE_ARRAY_COUNT(ColonyActors)) ? ColonyActors[Role].Get() : nullptr;
		})
		.OnOpenTab(FAPSOverviewOpenTab::CreateSP(this, &SAPSColonyTerminal::ShowTab));
}

TSharedRef<SWidget> SAPSColonyTerminal::BuildJournal()
{
	return SNew(SScrollBox) + SScrollBox::Slot()[SAssignNew(JournalList, SVerticalBox)];
}

void SAPSColonyTerminal::RebuildJournal()
{
	using namespace APSColonyUI;
	if (!JournalList.IsValid())
	{
		return;
	}
	JournalList->ClearChildren();
	JournalList->AddSlot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 10.0f)
	[
		IconSectionHeading(EAPSChromeGlyph::Recent, LOCTEXT("JournalSection", "CIVILIZATION JOURNAL"),
			LOCTEXT("JournalSubtitle", "Newest first, game time since the world began"))
	];
	const UAPSCivilizationJournalSubsystem* Journal = World.IsValid()
		? World->GetSubsystem<UAPSCivilizationJournalSubsystem>() : nullptr;
	if (!Journal || Journal->GetEntries().IsEmpty())
	{
		JournalList->AddSlot().AutoHeight()
		[
			SNew(STextBlock).Text(LOCTEXT("EmptyJournal", "Nothing recorded yet."))
			.Font(Font("Regular", 13)).ColorAndOpacity(Muted())
		];
		return;
	}
	const TArray<FAPSCivilizationJournalEntry>& Entries = Journal->GetEntries();

	// Category chips with their counts, ALL first.
	TArray<FName> Categories;
	TMap<FName, int32> Counts;
	for (const FAPSCivilizationJournalEntry& Entry : Entries)
	{
		Categories.AddUnique(Entry.Category);
		++Counts.FindOrAdd(Entry.Category);
	}
	Categories.Sort([](const FName& A, const FName& B)
	{
		const int32 RankA = JournalRank(A);
		const int32 RankB = JournalRank(B);
		return RankA != RankB ? RankA < RankB : A.LexicalLess(B);
	});
	if (!JournalFilter.IsNone() && !Counts.Contains(JournalFilter))
	{
		JournalFilter = NAME_None;
	}
	const TSharedRef<SWrapBox> Filters = SNew(SWrapBox).UseAllottedSize(true).InnerSlotPadding(FVector2D(6.0f, 6.0f));
	for (int32 Index = -1; Index < Categories.Num(); ++Index)
	{
		const FName Category = Index < 0 ? FName(NAME_None) : Categories[Index];
		const FJournalStyle Style = JournalStyle(Category);
		const FLinearColor Accent = Index < 0 ? Cyan() : Style.Colour;
		Filters->AddSlot()
		[
			ChromeButton(
				SNew(STextBlock).Font(Font("Bold", 10)).ColorAndOpacity(Accent)
				.Text(FText::Format(LOCTEXT("JournalChip", "{0}  {1}"), Index < 0 ? LOCTEXT("JournalAll", "ALL") : Style.Label,
					APSUINumber::Number(Index < 0 ? Entries.Num() : Counts.FindRef(Category)))),
				FOnClicked::CreateSP(this, &SAPSColonyTerminal::SetJournalFilter, Category),
				TAttribute<bool>::CreateLambda([this, Category]() { return JournalFilter == Category; }), Accent)
		];
	}
	JournalList->AddSlot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 12.0f)[Filters];

	// Newest first, like a ship's log; the last minute's entries stand out.
	constexpr int32 MaxShown = 200;
	const double Now = World.IsValid() ? World->GetTimeSeconds() : 0.0;
	int32 Shown = 0;
	for (int32 Index = Entries.Num() - 1; Index >= 0 && Shown < MaxShown; --Index)
	{
		const FAPSCivilizationJournalEntry& Entry = Entries[Index];
		if (!JournalFilter.IsNone() && Entry.Category != JournalFilter)
		{
			continue;
		}
		++Shown;
		const FJournalStyle Style = JournalStyle(Entry.Category);
		const int32 Seconds = FMath::FloorToInt(Entry.WorldSeconds);
		const FText Time = FText::FromString(FString::Printf(TEXT("T+%02d:%02d:%02d"),
			Seconds / 3600, (Seconds / 60) % 60, Seconds % 60));
		const bool bRecent = Entry.WorldSeconds <= Now && Now - Entry.WorldSeconds < 60.0;
		JournalList->AddSlot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 6.0f)
		[
			SNew(SBorder).BorderImage(FAppStyle::GetBrush("WhiteBrush")).Padding(0.0f)
			// One background for every row: the lighter rows of the last minute read as a selection (Rio 02.10);
			// fresh entries carry a NEW tag beside their time instead.
			.BorderBackgroundColor(APSUITheme::Retint(FLinearColor(0.02f, 0.06f, 0.08f, 0.82f)))
			[
				SNew(SHorizontalBox)
				// The category's colour down the left edge.
				+ SHorizontalBox::Slot().AutoWidth()
				[
					SNew(SBox).WidthOverride(3.0f)
					[
						SNew(SBorder).BorderImage(FAppStyle::GetBrush("WhiteBrush")).BorderBackgroundColor(Style.Colour)
					]
				]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Top).Padding(10.0f, 9.0f, 10.0f, 9.0f)
				[
					IconBadge(Style.Glyph, Style.Colour, 30.0f)
				]
				+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(0.0f, 8.0f, 14.0f, 10.0f)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight()
					[
						// Rio 03.10: the category (display face), its time and NEW (engine face) on one middle line.
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
						[
							SNew(STextBlock).Text(Style.Label).Font(Font("Bold", 10)).ColorAndOpacity(Style.Colour)
							.RenderTransform(CapsCenterShift(Font("Bold", 10)))
						]
						+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(12.0f, 0.0f, 0.0f, 0.0f)
						[
							SNew(STextBlock).Text(Time).Font(Font("Regular", 10)).ColorAndOpacity(Muted())
							.RenderTransform(CapsCenterShift(Font("Regular", 10)))
						]
						+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(10.0f, 0.0f, 0.0f, 0.0f)
						[
							SNew(STextBlock).Text(LOCTEXT("JournalNew", "NEW")).Font(APSUITheme::BodyFont("Bold", 10))
							.RenderTransform(CapsCenterShift(APSUITheme::BodyFont("Bold", 10)))
							.ColorAndOpacity(Success()).Visibility(bRecent ? EVisibility::Visible : EVisibility::Collapsed)
						]
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 4.0f, 0.0f, 0.0f)
					[
						SNew(STextBlock).Text(Entry.Text).AutoWrapText(true).Font(Font("Regular", 14))
						.LineHeightPercentage(1.12f).ColorAndOpacity(White())
					]
				]
			]
		];
	}
	if (Shown >= MaxShown)
	{
		JournalList->AddSlot().AutoHeight().Padding(0.0f, 6.0f, 0.0f, 0.0f)
		[
			SNew(STextBlock).Text(FText::Format(LOCTEXT("JournalOlder", "The latest {0} are shown; older entries stay in the save."),
				APSUINumber::Number(MaxShown))).Font(Font("Regular", 11)).ColorAndOpacity(Muted())
		];
	}
}

void SAPSColonyTerminal::HandleJournalEntry(const FAPSCivilizationJournalEntry& Entry)
{
	// The cards are rebuilt where they are seen; switching to the tab rebuilds them anyway.
	if (ActiveTab == ETab::Journal)
	{
		RebuildJournal();
	}
}

FReply SAPSColonyTerminal::SetJournalFilter(const FName Category)
{
	JournalFilter = Category;
	RebuildJournal();
	return FReply::Handled();
}

void SAPSColonyTerminal::CacheColonyActors()
{
	UWorld* LiveWorld = World.Get();
	if (!LiveWorld)
	{
		return;
	}
	for (TActorIterator<AActor> It(LiveWorld); It; ++It)
	{
		if (!IsValid(*It) || !It->ActorHasTag(TEXT("APS.Civilization.Materialized")))
		{
			continue;
		}
		if (const UAPSCivilizationIdentityComponent* Identity = It->FindComponentByClass<UAPSCivilizationIdentityComponent>())
		{
			const int32 RoleIndex = static_cast<int32>(Identity->Role);
			if (RoleIndex >= 0 && RoleIndex < UE_ARRAY_COUNT(ColonyActors))
			{
				ColonyActors[RoleIndex] = *It;
			}
		}
	}
}

FText SAPSColonyTerminal::DescribeColonyActor(const int32 RoleIndex) const
{
	const AActor* Actor = ColonyActors[RoleIndex].Get();
	if (!Actor)
	{
		return LOCTEXT("NotMaterialized", "Not materialized in this world yet");
	}
	const APlayerController* Controller = World.IsValid() ? World->GetFirstPlayerController() : nullptr;
	const APawn* Pawn = Controller ? Controller->GetPawn() : nullptr;
	const FString Distance = Pawn
		? UShipNavigationComponent::FormatDistance(FVector::Distance(Pawn->GetActorLocation(), Actor->GetActorLocation()))
		: FString(TEXT("?"));
	FText State = FText::GetEmpty();
	if (Actor->ActorHasTag(TEXT("APS.Civilization.SurfaceParked")))
	{
		State = LOCTEXT("Parked", "  /  parked at the colony");
	}
	else if (Actor->ActorHasTag(TEXT("APS.Civilization.InService")))
	{
		State = LOCTEXT("InService", "  /  in service");
	}
	return FText::Format(LOCTEXT("ActorDetails", "{0}  /  {1} away{2}"), FText::FromString(Actor->GetName()),
		FText::FromString(Distance), State);
}

FReply SAPSColonyTerminal::SelectTab(const ETab Tab)
{
	const ETab Previous = ActiveTab;
	ActiveTab = Tab;
	if (Switcher.IsValid())
	{
		Switcher->SetActiveWidgetIndex(static_cast<int32>(Tab));
	}
	if (IsMapTab())
	{
		LastMapTab = Tab;
	}
	if (Tab == ETab::Journal)
	{
		RebuildJournal();
	}
	if (Tab == ETab::Colony)
	{
		QueueSignature.Reset();
		BuiltCount = -1;
		RefreshConstruction();
		RefreshInfrastructure(true);
	}
	if (Tab == ETab::Scheme && Scheme.IsValid())
	{
		// Rio 05.10 (star map): the mode button shows the player's own system again; the star map's drill-down keeps its pin.
		if (bKeepSchemePin)
		{
			Scheme->Refresh();
		}
		else
		{
			Scheme->ShowSystem(nullptr);
		}
		bKeepSchemePin = false;
	}
	if (Tab == ETab::Surface && SurfaceMap.IsValid())
	{
		// Rio 02.10: SURFACE opens the world picked where the player came from (scheme, map or fleet orders).
		APlanetaryBody* Wanted = nullptr;
		if (Previous == ETab::Scheme && Scheme.IsValid())
		{
			Wanted = Cast<APlanetaryBody>(Scheme->GetPicked());
		}
		else if (Previous == ETab::Map && Map.IsValid())
		{
			if (const SAPSCivilizationMap::FObject* Selected = Map->GetSelected())
			{
				Wanted = Cast<APlanetaryBody>(Selected->Actor.Get());
			}
		}
		else if (Previous == ETab::Fleet)
		{
			Wanted = Cast<APlanetaryBody>(FleetTarget.Get());
		}
		if (Wanted)
		{
			SurfaceMap->SetBody(Wanted);
		}
		else if (!SurfaceMap->GetBody())
		{
			SurfaceMap->SetBody(SAPSSurfaceMap::FindDefaultBody(World.Get()));
		}
		SurfaceMap->RefreshMarkers();
	}
	if (Tab == ETab::Map)
	{
		MapListSignature.Reset();
		RefreshMap();
	}
	if (Tab == ETab::Fleet)
	{
		RefreshFleet(true);
	}
	if (Tab == ETab::Shipyard)
	{
		RebuildShipyardCatalogue();
		RefreshShipyard(true);
	}
	return FReply::Handled();
}

FReply SAPSColonyTerminal::Close()
{
	OnClose.ExecuteIfBound();
	return FReply::Handled();
}

FReply SAPSColonyTerminal::OpenMap()
{
	OnOpenMap.ExecuteIfBound();
	return FReply::Handled();
}

TOptional<FReply> SAPSColonyTerminal::HandleTerminalHotKey(const FKey& Key)
{
	if (Key == EKeys::Tab)
	{
		return Close();
	}
	// K: fleet command, and again to close it (the same key opens it in the game).
	if (Key == EKeys::K)
	{
		return ActiveTab == ETab::Fleet ? Close() : SelectTab(ETab::Fleet);
	}
	if (Key == EKeys::F10)
	{
		return OpenMap();
	}
	return TOptional<FReply>();
}

FReply SAPSColonyTerminal::OnKeyDown(const FGeometry& Geometry, const FKeyEvent& Event)
{
	const FKey Key = Event.GetKey();
	if (Key == EKeys::Escape)
	{
		return Close();
	}
	if (TOptional<FReply> HotKeyReply = HandleTerminalHotKey(Key))
	{
		return HotKeyReply.GetValue();
	}
	return SCompoundWidget::OnKeyDown(Geometry, Event);
}

FReply SAPSColonyTerminal::OnPreviewKeyDown(const FGeometry& Geometry, const FKeyEvent& Event)
{
	// Rio 06.10 (audit: Tab after clicking a terminal tab moved Slate's focus instead of closing): with the CVar on, the
	// terminal's hot keys win over any focused child; 0 leaves them to OnKeyDown as before.
	if (APSColonyUI::CVarTerminalButtonsNoFocus.GetValueOnGameThread() != 0)
	{
		if (TOptional<FReply> HotKeyReply = HandleTerminalHotKey(Event.GetKey()))
		{
			return HotKeyReply.GetValue();
		}
	}
	return SCompoundWidget::OnPreviewKeyDown(Geometry, Event);
}

void SAPSColonyTerminal::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
	SCompoundWidget::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);
	if (!FleetTarget.IsValid())
	{
		const FString Wanted = APSColonyUI::CVarPreviewTest.GetValueOnGameThread();
		const FAPSFleetCommand* Fleet = GetFleet();
		if (!Wanted.IsEmpty() && Fleet && World.IsValid())
		{
			for (TActorIterator<APlanetaryBody> It(World.Get()); It; ++It)
			{
				if (IsValid(*It) && Fleet->GetSurvey(*It) == APSFleet::ESurvey::Studied
					&& FAPSFleetCommand::DisplayName(*It).ToString().Contains(Wanted))
				{
					FleetTarget = *It;
					UpdateBodyPreview();
					break;
				}
			}
		}
	}
	if (ActiveTab != ETab::Colony && ActiveTab != ETab::Map && ActiveTab != ETab::Fleet && ActiveTab != ETab::Shipyard
		&& ActiveTab != ETab::Scheme && ActiveTab != ETab::Surface)
	{
		return;
	}
	RefreshAccumulator += InDeltaTime;
	// The construction queue shows progress, the maps follow moving ships.
	if (RefreshAccumulator >= (ActiveTab == ETab::Colony ? 0.2f : 0.5f))
	{
		RefreshAccumulator = 0.0f;
		if (ActiveTab == ETab::Map)
		{
			RefreshMap();
		}
		else if (ActiveTab == ETab::Fleet)
		{
			RefreshFleet(false);
		}
		else if (ActiveTab == ETab::Shipyard)
		{
			RefreshShipyard(false);
		}
		else if (ActiveTab == ETab::Surface && SurfaceMap.IsValid())
		{
			SurfaceMap->RefreshMarkers();
		}
		else if (ActiveTab == ETab::Scheme)
		{
			// Bodies do not change size; a slow re-read follows a newly selected system.
			if (Scheme.IsValid()) Scheme->Refresh();
		}
		else
		{
			RefreshConstruction();
			RefreshInfrastructure(false);
		}
	}
}

ASpaceship* SAPSColonyTerminal::GetCourseShip() const
{
	const APlayerController* Controller = World.IsValid() ? World->GetFirstPlayerController() : nullptr;
	if (ASpaceship* Piloted = Controller ? Cast<ASpaceship>(Controller->GetPawn()) : nullptr)
	{
		return Piloted;
	}
	return Cast<ASpaceship>(ColonyActors[2].Get());
}

TSharedRef<SWidget> SAPSColonyTerminal::BuildMap()
{
	using namespace APSColonyUI;
	// 0 the selected object's name, 1 its kind, 2 its distance (and whether it is ours).
	const auto SelectedText = [this](const int32 Field) -> FText
	{
		const SAPSCivilizationMap::FObject* Selected = Map.IsValid() ? Map->GetSelected() : nullptr;
		if (!Selected)
		{
			return Field == 0 ? LOCTEXT("MapNothing", "NOTHING SELECTED")
				: Field == 1 ? LOCTEXT("MapPick", "Click an object on the map or in the list below; double-click a planet for its moons, stations and ships.")
				: FText::GetEmpty();
		}
		if (Field == 0)
		{
			return Selected->Name;
		}
		if (Field == 1)
		{
			return Selected->Detail;
		}
		return FText::Format(LOCTEXT("MapDistance", "{0} away{1}"),
			FText::FromString(PawnDistance(World.Get(), Selected->Actor.Get())),
			Selected->bOwn ? LOCTEXT("MapOwn", "  /  ours") : FText::GetEmpty());
	};
	const auto CanSetCourse = [this]()
	{
		const SAPSCivilizationMap::FObject* Selected = Map.IsValid() ? Map->GetSelected() : nullptr;
		return Selected && Selected->Kind != SAPSCivilizationMap::EKind::Pilot
			&& Selected->Kind != SAPSCivilizationMap::EKind::Ship && GetCourseShip() != nullptr;
	};
	return SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 12.0f)
		[
			BuildMapModes()
		]
		+ SVerticalBox::Slot().FillHeight(1.0f)
		[
	SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().FillWidth(1.0f)
		[
			SNew(SOverlay)
			+ SOverlay::Slot()
			[
				SAssignNew(Map, SAPSCivilizationMap)
				.World(World)
				.OnSelectionChanged_Lambda([this]()
				{
					CourseMessage = FText::GetEmpty();
					// An open object window follows the pick.
					if (bObjectWindowOpen)
					{
						const SAPSCivilizationMap::FObject* Picked = Map.IsValid() ? Map->GetSelected() : nullptr;
						OpenObjectWindow(Picked ? Picked->Actor.Get() : nullptr);
					}
				})
			]
			+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center).Padding(24.0f)
			[
				SNew(SBox).WidthOverride(560.0f)
				.Visibility_Lambda([this]() { return bObjectWindowOpen ? EVisibility::Visible : EVisibility::Collapsed; })
				[
					SNew(SBorder).BorderImage(FAppStyle::GetBrush("WhiteBrush")).BorderBackgroundColor(Panel())
					.Padding(FMargin(18.0f, 16.0f))
					[
						SAssignNew(ObjectWindowBox, SVerticalBox)
					]
				]
			]
		]
		+ SHorizontalBox::Slot().AutoWidth().Padding(18.0f, 0.0f, 0.0f, 0.0f)
		[
			SNew(SBox).WidthOverride(330.0f)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight()
				[
					IconSectionHeading(EAPSChromeGlyph::Compass, LOCTEXT("MapSection", "MAP"),
						LOCTEXT("MapSubtitle", "Your ships, stations and colony, and the course"))
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 12.0f, 0.0f, 0.0f)
				[
					SNew(STextBlock)
					.Text_Lambda([this]() { return Map.IsValid() ? Map->GetViewTitle() : FText::GetEmpty(); })
					.Font(Font("Bold", 12)).ColorAndOpacity(Cyan())
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 8.0f, 0.0f, 0.0f)
				[
					SNew(SBox)
					.Visibility_Lambda([this]()
					{
						return Map.IsValid() && Map->IsLocalView() ? EVisibility::Visible : EVisibility::Collapsed;
					})
					[
						ChromeButton(
							SNew(STextBlock).Text(LOCTEXT("SystemView", "BACK TO THE SYSTEM")).Justification(ETextJustify::Center)
							.Font(Font("Bold", 10)).ColorAndOpacity(Cyan()),
							FOnClicked::CreateSP(this, &SAPSColonyTerminal::ShowSystemView), TAttribute<bool>(false), Cyan())
					]
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 14.0f, 0.0f, 0.0f)
				[
					ChamferPanel(
						SNew(SVerticalBox)
						+ SVerticalBox::Slot().AutoHeight()
						[
							SNew(STextBlock).Text_Lambda([SelectedText]() { return SelectedText(0); })
							.Font(Font("Bold", 14)).ColorAndOpacity(White())
						]
						+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 3.0f, 0.0f, 0.0f)
						[
							SNew(STextBlock).Text_Lambda([SelectedText]() { return SelectedText(1); }).AutoWrapText(true)
							.Font(Font("Bold", 10)).ColorAndOpacity(Cyan())
						]
						+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 6.0f, 0.0f, 0.0f)
						[
							SNew(STextBlock).Text_Lambda([SelectedText]() { return SelectedText(2); })
							.Font(Font("Regular", 11)).ColorAndOpacity(Muted())
						],
						FMargin(14.0f, 12.0f), CyanDim())
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 10.0f, 0.0f, 0.0f)
				[
					PrimaryButton(LOCTEXT("MapSetCourse", "SET COURSE"),
						FOnClicked::CreateSP(this, &SAPSColonyTerminal::SetCourseToSelected),
						TAttribute<bool>::CreateLambda(CanSetCourse))
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 8.0f, 0.0f, 0.0f)
				[
					PrimaryButton(LOCTEXT("MapOpenObject", "OPEN"),
						FOnClicked::CreateLambda([this]()
						{
							const SAPSCivilizationMap::FObject* Picked = Map.IsValid() ? Map->GetSelected() : nullptr;
							OpenObjectWindow(Picked ? Picked->Actor.Get() : nullptr);
							return FReply::Handled();
						}),
						TAttribute<bool>::CreateLambda([this]()
						{
							const SAPSCivilizationMap::FObject* Picked = Map.IsValid() ? Map->GetSelected() : nullptr;
							return Picked && Picked->Actor.IsValid();
						}))
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 8.0f, 0.0f, 0.0f)
				[
					SNew(SBox)
					.Visibility_Lambda([this]()
					{
						const SAPSCivilizationMap::FObject* Selected = Map.IsValid() ? Map->GetSelected() : nullptr;
						return Selected && Selected->Kind == SAPSCivilizationMap::EKind::Planet && !Map->IsLocalView()
							? EVisibility::Visible : EVisibility::Collapsed;
					})
					[
						ChromeButton(
							SNew(STextBlock).Text(LOCTEXT("OpenLocalView", "OPEN THE PLANET'S VIEW")).Justification(ETextJustify::Center)
							.Font(Font("Bold", 10)).ColorAndOpacity(Cyan()),
							FOnClicked::CreateSP(this, &SAPSColonyTerminal::OpenLocalView), TAttribute<bool>(false), Cyan())
					]
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 8.0f, 0.0f, 0.0f)
				[
					SNew(STextBlock).Text_Lambda([this]() { return CourseMessage; }).AutoWrapText(true)
					.ColorAndOpacity_Lambda([this]() { return FSlateColor(bCourseIsError ? Amber() : Success()); })
					.Font(Font("Bold", 10))
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 18.0f, 0.0f, 6.0f)
				[
					SNew(STextBlock).Text(LOCTEXT("MapInView", "IN THIS VIEW")).Font(Font("Bold", 10)).ColorAndOpacity(Muted())
				]
				+ SVerticalBox::Slot().FillHeight(1.0f)
				[
					SNew(SScrollBox)
					+ SScrollBox::Slot()
					[
						SAssignNew(MapList, SVerticalBox)
					]
				]
			]
		]
		];
}

void SAPSColonyTerminal::RefreshMap()
{
	using namespace APSColonyUI;
	if (!Map.IsValid())
	{
		return;
	}
	Map->Refresh();
	const ASpaceship* CourseShip = GetCourseShip();
	const FShipNavigationContact* Course = CourseShip && CourseShip->ShipNavigation
		? CourseShip->ShipNavigation->GetSelectedContact() : nullptr;
	Map->SetCourseTargetId(Course ? Course->StableId : FString());
	if (!MapList.IsValid())
	{
		return;
	}
	TArray<const SAPSCivilizationMap::FObject*> Shown;
	Map->GetShownObjects(Shown);
	FString Signature;
	for (const SAPSCivilizationMap::FObject* Object : Shown)
	{
		Signature += Object->StableId + TEXT(";");
	}
	if (Signature == MapListSignature)
	{
		return;
	}
	MapListSignature = Signature;
	MapList->ClearChildren();
	// Rio 02.10 ("hard to find the objects you need: they are only text... icons, readable labels: where the HQ is, where
	// a station, where just a building"): the list goes by what matters under headings (command, fleet, stations,
	// settlements and sites, worlds), each row with the object's real look (its baked thumbnail) or its glyph.
	const auto GroupOf = [](const SAPSCivilizationMap::FObject& Object) -> int32
	{
		switch (Object.Kind)
		{
		case SAPSCivilizationMap::EKind::Colony: return 0;
		case SAPSCivilizationMap::EKind::Station:
			return Object.Actor.IsValid() && Object.Actor->IsA<ASpaceHeadquarters>() ? 0 : 2;
		case SAPSCivilizationMap::EKind::Ship: return 1;
		case SAPSCivilizationMap::EKind::Settlement:
		case SAPSCivilizationMap::EKind::Outpost: return 3;
		default: return 4;
		}
	};
	Shown.StableSort([&GroupOf](const SAPSCivilizationMap::FObject& A, const SAPSCivilizationMap::FObject& B)
	{
		return GroupOf(A) < GroupOf(B);
	});
	const FText GroupHeadings[] = {LOCTEXT("MapGroupCommand", "COMMAND"), LOCTEXT("MapGroupFleet", "FLEET"),
		LOCTEXT("MapGroupStations", "STATIONS"), LOCTEXT("MapGroupSites", "SETTLEMENTS AND SITES"),
		LOCTEXT("MapGroupWorlds", "WORLDS")};
	int32 ShownGroup = INDEX_NONE;
	for (const SAPSCivilizationMap::FObject* Object : Shown)
	{
		if (Object->Kind == SAPSCivilizationMap::EKind::Pilot)
		{
			continue;
		}
		const int32 Group = GroupOf(*Object);
		if (Group != ShownGroup)
		{
			MapList->AddSlot().AutoHeight().Padding(2.0f, ShownGroup == INDEX_NONE ? 0.0f : 9.0f, 0.0f, 4.0f)
			[
				SNew(STextBlock).Text(GroupHeadings[Group]).Font(Font("Bold", 9))
				.ColorAndOpacity(Group == 0 ? Amber() : Muted())
			];
			ShownGroup = Group;
		}
		const FString StableId = Object->StableId;
		const TWeakObjectPtr<AActor> Actor = Object->Actor;
		const FSlateBrush* Snapshot = Actor.IsValid() ? APSUIThumbnails::FindBrush(Actor->GetClass()) : nullptr;
		const EAPSChromeGlyph Glyph = Object->Kind == SAPSCivilizationMap::EKind::Star ? EAPSChromeGlyph::System
			: Object->Kind == SAPSCivilizationMap::EKind::Planet || Object->Kind == SAPSCivilizationMap::EKind::Moon
				? EAPSChromeGlyph::Planet
			: Object->Kind == SAPSCivilizationMap::EKind::Colony ? EAPSChromeGlyph::Civilization
			: Object->Kind == SAPSCivilizationMap::EKind::Settlement ? EAPSChromeGlyph::Infrastructure
			: Object->Kind == SAPSCivilizationMap::EKind::Outpost ? EAPSChromeGlyph::Compass
			: APSInfrastructureUI::GlyphOf(Actor.Get());
		const TSharedRef<SWidget> Icon = Snapshot
			// A thumbnail may be wide (a long hull): fit it, never stretch it.
			? StaticCastSharedRef<SWidget>(SNew(SScaleBox).Stretch(EStretch::ScaleToFit)[SNew(SImage).Image(Snapshot)])
			: StaticCastSharedRef<SWidget>(SNew(SBox).Padding(8.0f)
				[
					SNew(SAPSVectorGlyph).Glyph(Glyph).Color(Object->Color).StrokeWidth(1.8f)
				]);
		MapList->AddSlot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 5.0f)
		[
			ChromeButton(
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.0f, 0.0f, 9.0f, 0.0f)
				[
					SNew(SBox).WidthOverride(42.0f).HeightOverride(42.0f)
					[
						Icon
					]
				]
				+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight()
					[
						SNew(STextBlock).Text(Object->Name).Font(Font("Bold", 10))
						.ColorAndOpacity(FSlateColor(Object->Color)).RenderTransform(CapsCenterShift(Font("Bold", 10)))
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 1.0f, 0.0f, 0.0f)
					[
						SNew(STextBlock).Text(Object->Detail).Font(Font("Regular", 10)).ColorAndOpacity(Muted())
					]
				]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(8.0f, 0.0f, 0.0f, 0.0f)
				[
					SNew(STextBlock)
					.Text_Lambda([this, Actor]()
					{
						return FText::FromString(APSColonyUI::PawnDistance(World.Get(), Actor.Get()));
					})
					.Font(Font("Bold", 10)).ColorAndOpacity(Amber()).RenderTransform(CapsCenterShift(Font("Bold", 10)))
				],
				FOnClicked::CreateLambda([this, StableId]()
				{
					if (Map.IsValid())
					{
						Map->SelectById(StableId);
					}
					return FReply::Handled();
				}),
				TAttribute<bool>::CreateLambda([this, StableId]()
				{
					const SAPSCivilizationMap::FObject* Selected = Map.IsValid() ? Map->GetSelected() : nullptr;
					return Selected && Selected->StableId == StableId;
				}),
				Cyan())
		];
	}
}

FReply SAPSColonyTerminal::SetCourseToSelected()
{
	const SAPSCivilizationMap::FObject* Target = Map.IsValid() ? Map->GetSelected() : nullptr;
	ASpaceship* Ship = GetCourseShip();
	bCourseIsError = true;
	if (!Target)
	{
		CourseMessage = LOCTEXT("CoursePick", "Pick an object on the map first.");
	}
	else if (!Ship || !Ship->ShipNavigation)
	{
		CourseMessage = LOCTEXT("CourseNoShip", "No ship to set a course for.");
	}
	else if (!Ship->ShipNavigation->SetCourse(Target->StableId))
	{
		CourseMessage = FText::Format(LOCTEXT("CourseNotCharted", "{0} is not charted for navigation."), Target->Name);
	}
	else
	{
		CourseMessage = FText::Format(LOCTEXT("CourseSet", "COURSE SET: {0}"), Target->Name);
		bCourseIsError = false;
		UAPSCivilizationJournalSubsystem::Post(World.Get(), TEXT("Navigation"), FText::Format(
			LOCTEXT("CourseJournal", "Course set for {0}: {1}."), FText::FromString(Ship->GetName()), Target->Name));
	}
	return FReply::Handled();
}

FReply SAPSColonyTerminal::ShowSystemView()
{
	if (Map.IsValid())
	{
		Map->Focus(nullptr);
		MapListSignature.Reset();
		RefreshMap();
	}
	return FReply::Handled();
}

FReply SAPSColonyTerminal::OpenLocalView()
{
	const SAPSCivilizationMap::FObject* Selected = Map.IsValid() ? Map->GetSelected() : nullptr;
	if (Selected && Selected->Kind == SAPSCivilizationMap::EKind::Planet)
	{
		Map->Focus(Selected->Actor.Get());
		MapListSignature.Reset();
		RefreshMap();
	}
	return FReply::Handled();
}

UAPSColonyConstructionSubsystem* SAPSColonyTerminal::GetConstruction() const
{
	UWorld* LiveWorld = World.Get();
	return LiveWorld ? LiveWorld->GetSubsystem<UAPSColonyConstructionSubsystem>() : nullptr;
}

TSharedRef<SWidget> SAPSColonyTerminal::BuildConstruction()
{
	using namespace APSColonyUI;
	const auto SiteButton = [this](const EAPSSpawnSite Site, const EAPSChromeGlyph Glyph, const FText& Label)
	{
		return SNew(SBox).WidthOverride(236.0f)
		[
			ChromeButton(
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
				[
					IconBadge(Glyph, Cyan(), 28.0f)
				]
				+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center).Padding(10.0f, 0.0f, 0.0f, 0.0f)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight()
					[
						SNew(STextBlock).Text(Label).Font(Font("Bold", 11)).ColorAndOpacity(White())
						.RenderTransform(CapsCenterShift(Font("Bold", 11)))
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 2.0f, 0.0f, 0.0f)
					[
						SNew(STextBlock)
						.Text_Lambda([this, Site]()
						{
							const UAPSColonyConstructionSubsystem* Construction = GetConstruction();
							return Construction && Construction->IsSiteReady(Site)
								? LOCTEXT("SiteReady", "READY FOR ORDERS") : LOCTEXT("SiteNotReady", "NOT AVAILABLE");
						})
						.ColorAndOpacity_Lambda([this, Site]()
						{
							const UAPSColonyConstructionSubsystem* Construction = GetConstruction();
							return FSlateColor(Construction && Construction->IsSiteReady(Site) ? Success() : Amber());
						})
						.Font(Font("Bold", 9))
					]
				],
				FOnClicked::CreateSP(this, &SAPSColonyTerminal::SelectSite, Site),
				TAttribute<bool>::CreateLambda([this, Site]() { return SelectedSite == Site; }),
				Cyan())
		];
	};

	return SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 12.0f)
		[
			IconSectionHeading(EAPSChromeGlyph::Infrastructure, LOCTEXT("ConstructionSection", "CONSTRUCTION"),
				LOCTEXT("ConstructionSubtitle",
					"Order a module: the production queue builds it and the crew sets it beside the site"))
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 14.0f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().Padding(0.0f, 0.0f, 10.0f, 0.0f)
			[
				SiteButton(EAPSSpawnSite::Orbit, EAPSChromeGlyph::Headquarters, LOCTEXT("OrbitSiteButton", "HEADQUARTERS"))
			]
			+ SHorizontalBox::Slot().AutoWidth().Padding(0.0f, 0.0f, 16.0f, 0.0f)
			[
				SiteButton(EAPSSpawnSite::Surface, EAPSChromeGlyph::Planet, LOCTEXT("SurfaceSiteButton", "SURFACE BASE"))
			]
			+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(STextBlock).Text_Lambda([this]() { return DescribeSite(); })
					.Font(Font("Regular", 11)).ColorAndOpacity(Muted()).AutoWrapText(true)
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 4.0f, 0.0f, 0.0f)
				[
					SNew(STextBlock).Text_Lambda([this]() { return StatusMessage; })
					.ColorAndOpacity_Lambda([this]() { return FSlateColor(bStatusIsError ? Amber() : Success()); })
					.Font(Font("Bold", 10)).AutoWrapText(true)
				]
			]
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 20.0f)
		[
			SAssignNew(CatalogueBox, SVerticalBox)
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 10.0f)
		[
			IconSectionHeading(EAPSChromeGlyph::Recent, LOCTEXT("QueueSection", "BUILD QUEUE"),
				LOCTEXT("QueueSubtitle", "Orders at this site: first in, first built"))
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 20.0f)
		[
			SAssignNew(QueueBox, SVerticalBox)
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 10.0f)
		[
			IconSectionHeading(EAPSChromeGlyph::Collection, LOCTEXT("BuiltSection", "STANDING MODULES"),
				LOCTEXT("BuiltSubtitle", "Built this session; keeping them in saves comes with the save handoff"))
		]
		+ SVerticalBox::Slot().AutoHeight()
		[
			SAssignNew(BuiltBox, SVerticalBox)
		];
}

TSharedRef<SWidget> SAPSColonyTerminal::BuildModuleCard(const FAPSColonyModuleSpec& Spec)
{
	using namespace APSColonyUI;
	const FName ModuleId = Spec.Id;
	const EAPSSpawnSite Site = Spec.Site;
	// Civil affairs speeds the colony's works up by 15% a level (APSColonyConstructionSubsystem registers them so).
	const UMainGameplayInstance* State = GameplayState(World.Get());
	const UCivilization* Civ = State ? State->CurrentCivilization.Get() : nullptr;
	const double CivilFactor = 1.0 / (1.0 + 0.15 * FMath::Max(Civ ? Civ->Divisions.CivilAffairs : 0, 0));
	const FText Metrics = FText::Format(LOCTEXT("ModuleMetrics", "{0} x {1} M  /  {2} S"),
		APSUINumber::Number(FMath::RoundToInt(Spec.SizeCm.X / 100.0)),
		APSUINumber::Number(FMath::RoundToInt(Spec.SizeCm.Y / 100.0)),
		APSUINumber::Number(FMath::RoundToInt(Spec.BuildSeconds * CivilFactor)));
	const auto CanOrder = [this, Site]()
	{
		const UAPSColonyConstructionSubsystem* Construction = GetConstruction();
		return Construction && Construction->IsSiteReady(Site);
	};
	return ChamferPanel(
		SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight()
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
			[
				IconBadge(ModuleGlyph(ModuleId), Cyan(), 34.0f)
			]
			+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center).Padding(10.0f, 0.0f, 0.0f, 0.0f)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(STextBlock).Text(Spec.Name).Font(Font("Bold", 12)).ColorAndOpacity(White())
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 2.0f, 0.0f, 0.0f)
				[
					SNew(STextBlock).Text(Metrics).Font(Font("Bold", 9)).ColorAndOpacity(Cyan())
				]
			]
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 8.0f, 0.0f, 10.0f)
		[
			SNew(SBox).MinDesiredHeight(46.0f)
			[
				SNew(STextBlock).Text(Spec.Description).AutoWrapText(true).Font(Font("Regular", 11))
				.ColorAndOpacity(Muted())
			]
		]
		+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Right)
		[
			SNew(SBox).WidthOverride(140.0f)
			[
				PrimaryButton(LOCTEXT("Build", "BUILD"),
					FOnClicked::CreateSP(this, &SAPSColonyTerminal::OrderModule, ModuleId),
					TAttribute<bool>::CreateLambda(CanOrder))
			]
		],
		FMargin(14.0f, 12.0f), CyanDim());
}

FText SAPSColonyTerminal::DescribeSite() const
{
	const UAPSColonyConstructionSubsystem* Construction = GetConstruction();
	const AActor* Anchor = Construction ? Construction->GetSiteAnchor(SelectedSite) : nullptr;
	if (!Anchor)
	{
		return SelectedSite == EAPSSpawnSite::Surface
			? LOCTEXT("NoSurfaceSite", "The colony base is not founded in this world yet.")
			: LOCTEXT("NoOrbitSite", "No headquarters in this world.");
	}
	return FText::Format(LOCTEXT("SiteDetails", "{0}  /  {1} away. Modules stand beside it and move with it."),
		FText::FromString(Anchor->GetName()), FText::FromString(APSColonyUI::PawnDistance(World.Get(), Anchor)));
}

FReply SAPSColonyTerminal::SelectSite(const EAPSSpawnSite Site)
{
	if (SelectedSite != Site)
	{
		SelectedSite = Site;
		StatusMessage = FText::GetEmpty();
		RebuildCatalogue();
		QueueSignature.Reset();
		BuiltCount = -1;
		RefreshConstruction();
	}
	return FReply::Handled();
}

FReply SAPSColonyTerminal::OrderModule(const FName ModuleId)
{
	UAPSColonyConstructionSubsystem* Construction = GetConstruction();
	const FAPSColonyModuleSpec* Spec = FAPSColonyModuleCatalogue::Find(ModuleId);
	FText Failure;
	if (Construction && Spec && Construction->RequestBuild(ModuleId, Failure))
	{
		StatusMessage = FText::Format(LOCTEXT("OrderedStatus", "ORDERED: {0}"), Spec->Name);
		bStatusIsError = false;
		QueueSignature.Reset();
		RefreshConstruction();
	}
	else
	{
		StatusMessage = Failure.IsEmpty()
			? LOCTEXT("NoConstruction", "Construction is not available in this world.") : Failure;
		bStatusIsError = true;
	}
	return FReply::Handled();
}

void SAPSColonyTerminal::RebuildCatalogue()
{
	if (!CatalogueBox.IsValid())
	{
		return;
	}
	CatalogueBox->ClearChildren();
	const TSharedRef<SUniformGridPanel> Grid = SNew(SUniformGridPanel).SlotPadding(FMargin(5.0f));
	int32 Cell = 0;
	for (const FAPSColonyModuleSpec& Spec : FAPSColonyModuleCatalogue::Get())
	{
		if (Spec.Site == SelectedSite)
		{
			Grid->AddSlot(Cell % 3, Cell / 3)[BuildModuleCard(Spec)];
			++Cell;
		}
	}
	CatalogueBox->AddSlot().AutoHeight()[Grid];
}

float SAPSColonyTerminal::JobProgress(const FGuid& JobId) const
{
	const float* Progress = JobProgressById.Find(JobId);
	return Progress ? FMath::Clamp(*Progress, 0.0f, 1.0f) : 0.0f;
}

void SAPSColonyTerminal::RefreshConstruction()
{
	UAPSColonyConstructionSubsystem* Construction = GetConstruction();
	FAPSProductionSnapshot Snapshot;
	const bool bHasSnapshot = Construction && Construction->GetSiteSnapshot(SelectedSite, Snapshot);
	JobProgressById.Reset();
	FString Signature = FString::Printf(TEXT("%d|"), static_cast<int32>(SelectedSite));
	if (bHasSnapshot)
	{
		for (const FAPSProductionJobSnapshot& Job : Snapshot.Jobs)
		{
			JobProgressById.Add(Job.JobId, static_cast<float>(Job.ProgressNormalized));
			Signature += FString::Printf(TEXT("%s:%d:%s;"), *Job.JobId.ToString(), static_cast<int32>(Job.State),
				*Construction->GetJobNote(Job.JobId).ToString());
		}
	}
	if (Signature != QueueSignature)
	{
		QueueSignature = Signature;
		RebuildQueue(bHasSnapshot ? &Snapshot : nullptr);
	}
	TArray<AAPSColonyModule*> Modules;
	if (Construction)
	{
		Construction->GetBuiltModules(SelectedSite, Modules);
	}
	if (Modules.Num() != BuiltCount)
	{
		BuiltCount = Modules.Num();
		RebuildBuilt(Modules);
	}
}

void SAPSColonyTerminal::RebuildQueue(const FAPSProductionSnapshot* Snapshot)
{
	using namespace APSColonyUI;
	if (!QueueBox.IsValid())
	{
		return;
	}
	QueueBox->ClearChildren();
	// Active orders in queue order, then the last few finished ones, newest first.
	TArray<const FAPSProductionJobSnapshot*> Rows;
	if (Snapshot)
	{
		TArray<const FAPSProductionJobSnapshot*> Finished;
		for (const FAPSProductionJobSnapshot& Job : Snapshot->Jobs)
		{
			const bool bFinished = Job.State == EAPSProductionJobState::Succeeded
				|| Job.State == EAPSProductionJobState::Failed || Job.State == EAPSProductionJobState::Cancelled;
			(bFinished ? Finished : Rows).Add(&Job);
		}
		for (int32 Index = Finished.Num() - 1; Index >= 0 && Index >= Finished.Num() - 4; --Index)
		{
			Rows.Add(Finished[Index]);
		}
	}
	if (Rows.IsEmpty())
	{
		QueueBox->AddSlot().AutoHeight()
		[
			SNew(STextBlock).Text(LOCTEXT("NoOrders", "No orders at this site.")).Font(Font("Regular", 11))
			.ColorAndOpacity(Muted())
		];
		return;
	}
	const UAPSColonyConstructionSubsystem* Construction = GetConstruction();
	for (const FAPSProductionJobSnapshot* Job : Rows)
	{
		const FAPSColonyModuleSpec* Spec = FAPSColonyModuleCatalogue::Find(
			FAPSColonyModuleCatalogue::ModuleIdFromDefinition(Job->DefinitionId));
		FText State;
		FLinearColor StateColor = Cyan();
		switch (Job->State)
		{
		case EAPSProductionJobState::Queued:
			State = FText::Format(LOCTEXT("QueuedState", "QUEUED  /  #{0}"), APSUINumber::Number(Job->QueuePosition + 1));
			break;
		case EAPSProductionJobState::InProgress:
			State = LOCTEXT("BuildingState", "BUILDING");
			StateColor = Amber();
			break;
		case EAPSProductionJobState::AwaitingMaterialization:
		{
			const FText Note = Construction ? Construction->GetJobNote(Job->JobId) : FText::GetEmpty();
			State = Note.IsEmpty() ? LOCTEXT("PlacingState", "PLACING") : FText::FromString(Note.ToString().ToUpper());
			StateColor = Amber();
			break;
		}
		case EAPSProductionJobState::Succeeded:
			State = LOCTEXT("BuiltState", "BUILT");
			StateColor = Success();
			break;
		case EAPSProductionJobState::Failed:
			State = FText::Format(LOCTEXT("FailedState", "FAILED  /  {0}"),
				UAPSColonyConstructionSubsystem::DescribeFailure(Job->StatusReason.Key));
			StateColor = Amber();
			break;
		default:
			State = LOCTEXT("CancelledState", "CANCELLED");
			StateColor = Muted();
			break;
		}
		const FGuid JobId = Job->JobId;
		QueueBox->AddSlot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 8.0f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.0f, 0.0f, 12.0f, 0.0f)
			[
				SNew(SBox).WidthOverride(190.0f)
				[
					SNew(STextBlock).Text(Spec ? Spec->Name : FText::FromName(Job->DefinitionId.PrimaryAssetName))
					.Font(Font("Bold", 11)).ColorAndOpacity(White()).RenderTransform(CapsCenterShift(Font("Bold", 11)))
				]
			]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.0f, 0.0f, 12.0f, 0.0f)
			[
				SNew(SBox).WidthOverride(290.0f)
				[
					SNew(STextBlock).Text(State).Font(Font("Bold", 10)).ColorAndOpacity(StateColor)
					.RenderTransform(CapsCenterShift(Font("Bold", 10)))
				]
			]
			+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center)
			[
				SNew(SBox).HeightOverride(6.0f)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().FillWidth(TAttribute<float>::CreateLambda([this, JobId]()
					{
						return JobProgress(JobId);
					}))
					[
						SNew(SBorder).BorderImage(FAppStyle::GetBrush("WhiteBrush")).BorderBackgroundColor(StateColor)
					]
					+ SHorizontalBox::Slot().FillWidth(TAttribute<float>::CreateLambda([this, JobId]()
					{
						return 1.0f - JobProgress(JobId);
					}))
					[
						SNew(SBorder).BorderImage(FAppStyle::GetBrush("WhiteBrush")).BorderBackgroundColor(PanelSoft())
					]
				]
			]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(10.0f, 0.0f, 0.0f, 0.0f)
			[
				SNew(SBox).WidthOverride(44.0f)
				[
					SNew(STextBlock)
					.Text_Lambda([this, JobId]()
					{
						return FText::FromString(FString::Printf(TEXT("%d%%"),
							FMath::RoundToInt(JobProgress(JobId) * 100.0f)));
					})
					.Font(Font("Bold", 10)).ColorAndOpacity(Muted()).RenderTransform(CapsCenterShift(Font("Bold", 10)))
				]
			]
		];
	}
}

void SAPSColonyTerminal::RebuildBuilt(const TArray<AAPSColonyModule*>& Modules)
{
	using namespace APSColonyUI;
	if (!BuiltBox.IsValid())
	{
		return;
	}
	BuiltBox->ClearChildren();
	if (Modules.IsEmpty())
	{
		BuiltBox->AddSlot().AutoHeight()
		[
			SNew(STextBlock).Text(LOCTEXT("NothingBuilt", "Nothing built at this site yet.")).Font(Font("Regular", 11))
			.ColorAndOpacity(Muted())
		];
		return;
	}
	for (AAPSColonyModule* Module : Modules)
	{
		const FAPSColonyModuleSpec* Spec = FAPSColonyModuleCatalogue::Find(Module->GetModuleId());
		const TWeakObjectPtr<AAPSColonyModule> WeakModule = Module;
		BuiltBox->AddSlot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 8.0f)
		[
			ChamferPanel(
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
				[
					IconBadge(ModuleGlyph(Module->GetModuleId()), Success(), 30.0f)
				]
				+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center).Padding(12.0f, 0.0f)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight()
					[
						SNew(STextBlock).Text(Spec ? Spec->Name : FText::FromName(Module->GetModuleId()))
						.Font(Font("Bold", 12)).ColorAndOpacity(White())
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 3.0f, 0.0f, 0.0f)
					[
						SNew(STextBlock)
						.Text_Lambda([this, WeakModule]()
						{
							const AAPSColonyModule* Live = WeakModule.Get();
							return Live
								? FText::Format(LOCTEXT("ModuleDetails", "#{0}  /  {1} away"),
									FText::FromString(Live->GetStableId().ToString().Left(8)),
									FText::FromString(PawnDistance(World.Get(), Live)))
								: LOCTEXT("ModuleGone", "No longer standing");
						})
						.Font(Font("Regular", 11)).ColorAndOpacity(Muted())
					]
				]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
				[
					SNew(STextBlock).Text(LOCTEXT("Standing", "STANDING")).Font(Font("Bold", 11)).ColorAndOpacity(Success())
					.RenderTransform(CapsCenterShift(Font("Bold", 11)))
				],
				FMargin(14.0f, 9.0f), CyanDim())
		];
	}
}

const FSlateBrush* SAPSColonyTerminal::ShipThumbnail(const TSubclassOf<ASpaceship> ShipClass)
{
	if (!ShipClass)
	{
		return nullptr;
	}
	const FString Package = ShipClass->GetOutermost()->GetName();
	if (const TSharedPtr<FSlateBrush>* Found = ShipThumbnails.Find(Package))
	{
		return Found->Get();
	}
	TSharedPtr<FSlateBrush> Brush;
	if (UTexture2D* Texture = LoadObject<UTexture2D>(nullptr, *APSUIThumbnails::TexturePathForBlueprintPackage(Package),
		nullptr, LOAD_NoWarn | LOAD_Quiet))
	{
		ShipThumbnailTextures.Emplace(Texture);
		Brush = MakeShared<FSlateBrush>();
		// Cropped to the hull, so the card's box fills with the ship (Rio 05.10: "the ship icons are too small").
		APSUIThumbnails::InitBrush(*Brush, Texture);
	}
	ShipThumbnails.Add(Package, Brush);
	return Brush.Get();
}

TSharedRef<SWidget> SAPSColonyTerminal::BuildShipyard()
{
	using namespace APSColonyUI;
	return SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().FillWidth(1.0f)
		[
			SNew(SScrollBox)
			+ SScrollBox::Slot().Padding(0.0f, 0.0f, 0.0f, 10.0f)
			[
				IconSectionHeading(EAPSChromeGlyph::Ship, LOCTEXT("ShipyardSection", "SHIPYARD"),
					LOCTEXT("ShipyardSubtitle", "Pick a shipyard on the right, then a ship: it launches there and joins FLEET COMMAND"))
			]
			+ SScrollBox::Slot()
			[
				SAssignNew(ShipyardCatalogueBox, SVerticalBox)
			]
		]
		+ SHorizontalBox::Slot().AutoWidth().Padding(16.0f, 0.0f, 0.0f, 0.0f)
		[
			SNew(SBox).WidthOverride(340.0f)
			[
				SNew(SScrollBox)
				+ SScrollBox::Slot()
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight()
					[
						SNew(STextBlock).Text(LOCTEXT("ShipyardsTitle", "SHIPYARDS")).Font(Font("Bold", 14)).ColorAndOpacity(White())
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 4.0f, 0.0f, 10.0f)
					[
						SNew(STextBlock).AutoWrapText(true).Font(Font("Regular", 10)).ColorAndOpacity(Muted())
						.Text(LOCTEXT("ShipyardsHelp", "Every shipyard builds one ship at a time, all of them at once. More shipyards: FLEET COMMAND, a construction ship, a world with a station, BUILD SHIPYARD."))
					]
					+ SVerticalBox::Slot().AutoHeight()
					[
						SAssignNew(ShipyardYardsBox, SVerticalBox)
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 14.0f, 0.0f, 0.0f)
					[
						SNew(STextBlock).AutoWrapText(true).Font(Font("Bold", 13)).ColorAndOpacity(White())
						.Text_Lambda([this]()
						{
							const ASpaceShipyard* Yard = GetSelectedYard();
							return Yard ? FText::Format(LOCTEXT("SlipwayOf", "SLIPWAY  /  {0}"), FAPSFleetCommand::DisplayName(Yard))
								: LOCTEXT("SlipwayTitle", "SLIPWAY");
						})
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 4.0f, 0.0f, 10.0f)
					[
						SNew(STextBlock).AutoWrapText(true).Font(Font("Regular", 10))
						.Text_Lambda([this]()
						{
							const FAPSFleetCommand* Fleet = GetFleet();
							const ASpaceShipyard* Yard = GetSelectedYard();
							if (!Fleet || !Yard)
							{
								return LOCTEXT("SlipwayNoShipyard", "No shipyard in this system: ships cannot be launched.");
							}
							return FText::Format(LOCTEXT("SlipwayState", "{0} of {1} queued. Launched here: {2}."),
								APSUINumber::Number(Fleet->CountQueued(Yard)), APSUINumber::Number(FAPSFleetCommand::ShipyardQueueLimit),
								APSUINumber::Number(Fleet->GetLaunchedCount(Yard)));
						})
						.ColorAndOpacity_Lambda([this]() { return FSlateColor(bShipyardPresent ? Muted() : Amber()); })
					]
					+ SVerticalBox::Slot().AutoHeight()
					[
						SAssignNew(ShipyardQueueBox, SVerticalBox)
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 10.0f, 0.0f, 0.0f)
					[
						SNew(STextBlock).AutoWrapText(true).Font(Font("Bold", 10))
						.Text_Lambda([this]() { return ShipyardMessage; })
						.ColorAndOpacity_Lambda([this]() { return FSlateColor(bShipyardMessageIsError ? Amber() : Success()); })
					]
				]
			]
		];
}

void SAPSColonyTerminal::RebuildShipyardCatalogue()
{
	using namespace APSColonyUI;
	if (!ShipyardCatalogueBox.IsValid())
	{
		return;
	}
	ShipyardOptions.Reset();
	if (const FAPSFleetCommand* Fleet = GetFleet())
	{
		Fleet->GetShipyardOptions(ShipyardOptions);
	}
	ShipyardCatalogueBox->ClearChildren();
	if (ShipyardOptions.IsEmpty())
	{
		ShipyardCatalogueBox->AddSlot().AutoHeight()
		[
			SNew(STextBlock).AutoWrapText(true).Font(Font("Regular", 12)).ColorAndOpacity(Muted())
			.Text(LOCTEXT("ShipyardNoCatalogue", "No ship catalogue in this world: the shipyard has nothing to build."))
		];
		return;
	}
	// Class chips with their counts: ALL, then every class the catalogue has, smallest first.
	TArray<ESpaceshipSizeClass> Classes;
	for (const FAPSShipyardOption& Option : ShipyardOptions)
	{
		Classes.AddUnique(Option.SizeClass);
	}
	const TSharedRef<SWrapBox> Filters = SNew(SWrapBox).UseAllottedSize(true).InnerSlotPadding(FVector2D(6.0f, 6.0f));
	for (int32 Index = -1; Index < Classes.Num(); ++Index)
	{
		const int32 Filter = Index < 0 ? -1 : static_cast<int32>(Classes[Index]);
		int32 Count = 0;
		for (const FAPSShipyardOption& Option : ShipyardOptions)
		{
			Count += Filter < 0 || static_cast<int32>(Option.SizeClass) == Filter ? 1 : 0;
		}
		Filters->AddSlot()
		[
			ChromeButton(
				SNew(STextBlock).Font(Font("Bold", 9)).ColorAndOpacity(Cyan())
				.Text(FText::Format(LOCTEXT("ShipyardClassChip", "{0}  {1}"), Index < 0 ? LOCTEXT("ShipyardClassAll", "ALL")
					: FText::Format(LOCTEXT("ShipyardClassName", "CLASS {0}"), EnumText(Classes[Index]).ToUpper()),
					APSUINumber::Number(Count))),
				FOnClicked::CreateSP(this, &SAPSColonyTerminal::SetShipyardClassFilter, Filter),
				TAttribute<bool>::CreateLambda([this, Filter]() { return ShipyardClassFilter == Filter; }), Cyan())
		];
	}
	ShipyardCatalogueBox->AddSlot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 10.0f)[Filters];

	constexpr int32 Columns = 4;
	const TSharedRef<SUniformGridPanel> Grid = SNew(SUniformGridPanel).SlotPadding(FMargin(5.0f));
	int32 Shown = 0;
	for (int32 Index = 0; Index < ShipyardOptions.Num(); ++Index)
	{
		const FAPSShipyardOption& Option = ShipyardOptions[Index];
		if (ShipyardClassFilter >= 0 && static_cast<int32>(Option.SizeClass) != ShipyardClassFilter)
		{
			continue;
		}
		const FSlateBrush* Thumbnail = ShipThumbnail(Option.ShipClass);
		Grid->AddSlot(Shown % Columns, Shown / Columns)
		[
			ChamferPanel(
				SNew(SVerticalBox)
				// Rio 05.10 ("the ship icons are too small"): the cropped icon fits the card's whole width, this high.
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(SBox).WidthOverride(300.0f).HeightOverride(112.0f)
					[
						Thumbnail
							? StaticCastSharedRef<SWidget>(SNew(SScaleBox).Stretch(EStretch::ScaleToFit)[SNew(SImage).Image(Thumbnail)])
							: StaticCastSharedRef<SWidget>(SNew(SBox).HAlign(HAlign_Center).VAlign(VAlign_Center)
								[IconBadge(EAPSChromeGlyph::Ship, Cyan(), 44.0f)])
					]
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 6.0f, 0.0f, 0.0f)
				[
					SNew(STextBlock).Text(Option.Name).Font(Font("Bold", 11)).ColorAndOpacity(White())
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 2.0f, 0.0f, 0.0f)
				[
					SNew(STextBlock).AutoWrapText(true)
					.Text(FText::Format(LOCTEXT("ShipyardOptionDetail", "CLASS {0}  /  {1} S  /  {2}"),
						EnumText(Option.SizeClass).ToUpper(), APSUINumber::Number(FMath::RoundToInt(Option.BuildSeconds)),
						APSFleet::DivisionName(APSFleet::DefaultDivision(Option.SizeClass, false))))
					.Font(Font("Bold", 9)).ColorAndOpacity(Cyan())
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 8.0f, 0.0f, 0.0f)
				[
					PrimaryButton(LOCTEXT("ShipyardBuild", "BUILD"),
						FOnClicked::CreateSP(this, &SAPSColonyTerminal::OrderShipyardShip, Index),
						TAttribute<bool>::CreateLambda([this]()
						{
							const FAPSFleetCommand* Fleet = GetFleet();
							const ASpaceShipyard* Yard = GetSelectedYard();
							return Fleet && Yard && Fleet->CountQueued(Yard) < FAPSFleetCommand::ShipyardQueueLimit;
						}))
				],
				FMargin(10.0f, 8.0f), CyanDim())
		];
		++Shown;
	}
	// A filter showing fewer than four keeps the cards their width.
	for (int32 Pad = Shown; Pad < Columns; ++Pad)
	{
		Grid->AddSlot(Pad, 0)[SNew(SBox)];
	}
	ShipyardCatalogueBox->AddSlot().AutoHeight()[Grid];
}

ASpaceShipyard* SAPSColonyTerminal::GetSelectedYard() const
{
	if (SelectedYard.IsValid())
	{
		return SelectedYard.Get();
	}
	const FAPSFleetCommand* Fleet = GetFleet();
	return Fleet ? Fleet->FindShipyard() : nullptr;
}

FReply SAPSColonyTerminal::SelectShipyard(const TWeakObjectPtr<ASpaceShipyard> Yard)
{
	SelectedYard = Yard;
	ShipyardMessage = FText::GetEmpty();
	RefreshShipyard(true);
	return FReply::Handled();
}

FReply SAPSColonyTerminal::SetShipyardClassFilter(const int32 Filter)
{
	ShipyardClassFilter = Filter;
	RebuildShipyardCatalogue();
	return FReply::Handled();
}

void SAPSColonyTerminal::RefreshShipyard(const bool bForceRebuild)
{
	using namespace APSColonyUI;
	const FAPSFleetCommand* Fleet = GetFleet();
	TArray<ASpaceShipyard*> Yards;
	if (Fleet)
	{
		Fleet->GetShipyards(Yards);
	}
	if (!SelectedYard.IsValid() || !Yards.Contains(SelectedYard.Get()))
	{
		SelectedYard = Yards.IsEmpty() ? nullptr : Yards[0];
	}
	ASpaceShipyard* Yard = SelectedYard.Get();
	bShipyardPresent = Yard != nullptr;

	// The shipyards: rebuilt when the set or the pick changes; what each builds is read live.
	if (ShipyardYardsBox.IsValid())
	{
		FString YardsSignature = GetNameSafe(Yard);
		for (const ASpaceShipyard* Each : Yards)
		{
			YardsSignature += TEXT(";") + Each->GetName();
		}
		if (bForceRebuild || YardsSignature != ShipyardYardsSignature)
		{
			ShipyardYardsSignature = YardsSignature;
			ShipyardYardsBox->ClearChildren();
			if (Yards.IsEmpty())
			{
				ShipyardYardsBox->AddSlot().AutoHeight()
				[
					SNew(STextBlock).AutoWrapText(true).Font(Font("Regular", 11)).ColorAndOpacity(Amber())
					.Text(LOCTEXT("NoShipyards", "No shipyard yet: a construction ship can build one at a world with a station."))
				];
			}
			for (ASpaceShipyard* Each : Yards)
			{
				const TWeakObjectPtr<ASpaceShipyard> Weak = Each;
				const FText Orbit = FAPSFleetCommand::DisplayName(FAPSFleetCommand::OrbitedBody(Each));
				const bool bHome = !Each->ActorHasTag(TEXT("APS.Fleet.Structure"));
				ShipyardYardsBox->AddSlot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 6.0f)
				[
					ChromeButton(
						SNew(SVerticalBox)
						+ SVerticalBox::Slot().AutoHeight()
						[
							// The name (display face) and its tag (engine face) on one middle line (Rio 03.10).
							SNew(SHorizontalBox)
							+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center)
							[
								SNew(STextBlock).Text(FAPSFleetCommand::DisplayName(Each)).Font(Font("Bold", 11)).ColorAndOpacity(White())
								.RenderTransform(CapsCenterShift(Font("Bold", 11)))
							]
							+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(8.0f, 0.0f, 0.0f, 0.0f)
							[
								SNew(STextBlock).Text(bHome ? LOCTEXT("YardHome", "HOME") : LOCTEXT("YardBuilt", "BUILT"))
								.Font(Font("Bold", 9)).ColorAndOpacity(bHome ? Amber() : Success())
								.RenderTransform(CapsCenterShift(Font("Bold", 9)))
							]
						]
						+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 2.0f, 0.0f, 0.0f)
						[
							SNew(STextBlock).Font(Font("Bold", 9)).ColorAndOpacity(Cyan())
							.Text(Orbit.IsEmpty() ? LOCTEXT("YardNoOrbit", "IN OPEN SPACE")
								: FText::Format(LOCTEXT("YardOrbit", "ORBIT  {0}"), Orbit))
						]
						+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 3.0f, 0.0f, 0.0f)
						[
							SNew(STextBlock).AutoWrapText(true).Font(Font("Regular", 10)).ColorAndOpacity(Muted())
							.Text_Lambda([this, Weak]()
							{
								const FAPSFleetCommand* Live = GetFleet();
								const ASpaceShipyard* Target = Weak.Get();
								if (!Live || !Target)
								{
									return FText::GetEmpty();
								}
								for (const FAPSShipyardJob& Job : Live->GetShipyardQueue())
								{
									if (Job.Yard.Get() == Target)
									{
										return FText::Format(LOCTEXT("YardBusy", "BUILDING {0}  {1}%  /  QUEUE {2} OF {3}"), Job.Name,
											APSUINumber::Number(FMath::RoundToInt(Job.Progress * 100.0f)),
											APSUINumber::Number(Live->CountQueued(Target)), APSUINumber::Number(FAPSFleetCommand::ShipyardQueueLimit));
									}
								}
								return FText::Format(LOCTEXT("YardIdle", "SLIPWAY FREE  /  LAUNCHED {0}"),
									APSUINumber::Number(Live->GetLaunchedCount(Target)));
							})
						],
						FOnClicked::CreateSP(this, &SAPSColonyTerminal::SelectShipyard, Weak),
						TAttribute<bool>::CreateLambda([this, Weak]() { return SelectedYard == Weak; }), Amber())
				];
			}
		}
	}

	if (!ShipyardQueueBox.IsValid())
	{
		return;
	}
	// The picked shipyard's slipway.
	static const TArray<FAPSShipyardJob> NoJobs;
	const TArray<FAPSShipyardJob>& Queue = Fleet ? Fleet->GetShipyardQueue() : NoJobs;
	TArray<int32> Jobs;
	FString Signature = GetNameSafe(Yard);
	for (int32 Index = 0; Index < Queue.Num(); ++Index)
	{
		if (Yard && Queue[Index].Yard.Get() == Yard)
		{
			Jobs.Add(Index);
			Signature += TEXT(";") + Queue[Index].Name.ToString();
		}
	}
	if (!bForceRebuild && Signature == ShipyardSignature)
	{
		return;
	}
	ShipyardSignature = Signature;
	ShipyardQueueBox->ClearChildren();
	if (Jobs.IsEmpty())
	{
		ShipyardQueueBox->AddSlot().AutoHeight()
		[
			SNew(STextBlock).Text(LOCTEXT("SlipwayEmpty", "The slipway is free: pick a ship on the left.")).AutoWrapText(true)
			.Font(Font("Regular", 11)).ColorAndOpacity(Muted())
		];
		return;
	}
	const TWeakObjectPtr<ASpaceShipyard> WeakYard = Yard;
	for (int32 Position = 0; Position < Jobs.Num(); ++Position)
	{
		const FAPSShipyardJob& Job = Queue[Jobs[Position]];
		// Read live: the queue shifts as ships launch, so find this shipyard's job at this position again.
		const auto Progress = [this, WeakYard, Position]()
		{
			const FAPSFleetCommand* Live = GetFleet();
			int32 Seen = 0;
			for (const FAPSShipyardJob& Each : Live ? Live->GetShipyardQueue() : NoJobs)
			{
				if (Each.Yard == WeakYard && Seen++ == Position)
				{
					return Each.Progress;
				}
			}
			return 0.0f;
		};
		ShipyardQueueBox->AddSlot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 8.0f)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(STextBlock).Font(Font("Bold", 11)).ColorAndOpacity(White())
				.Text(FText::Format(LOCTEXT("SlipwayJob", "{0}  /  CLASS {1}"), Job.Name, EnumText(Job.SizeClass).ToUpper()))
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 2.0f, 0.0f, 0.0f)
			[
				SNew(STextBlock).Font(Font("Bold", 9)).ColorAndOpacity(Position == 0 ? Amber() : Muted())
				.Text_Lambda([Progress, Position]()
				{
					return Position == 0
						? FText::Format(LOCTEXT("SlipwayBuilding", "ON THE SLIPWAY  /  {0}%"),
							APSUINumber::Number(FMath::RoundToInt(Progress() * 100.0f)))
						: LOCTEXT("SlipwayQueued", "QUEUED");
				})
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 4.0f, 0.0f, 0.0f)
			[
				SNew(SBox).HeightOverride(6.0f)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().FillWidth(TAttribute<float>::CreateLambda([Progress]() { return Progress(); }))
					[
						SNew(SBorder).BorderImage(FAppStyle::GetBrush("WhiteBrush")).BorderBackgroundColor(Amber())
					]
					+ SHorizontalBox::Slot().FillWidth(TAttribute<float>::CreateLambda([Progress]() { return 1.0f - Progress(); }))
					[
						SNew(SBorder).BorderImage(FAppStyle::GetBrush("WhiteBrush")).BorderBackgroundColor(CyanDim())
					]
				]
			]
		];
	}
}

FReply SAPSColonyTerminal::OrderShipyardShip(const int32 OptionIndex)
{
	FAPSFleetCommand* Fleet = GetFleet();
	if (!Fleet || !ShipyardOptions.IsValidIndex(OptionIndex))
	{
		return FReply::Handled();
	}
	ASpaceShipyard* Yard = GetSelectedYard();
	const FText Refusal = Fleet->OrderShip(ShipyardOptions[OptionIndex], Yard);
	bShipyardMessageIsError = !Refusal.IsEmpty();
	ShipyardMessage = Refusal.IsEmpty()
		? FText::Format(LOCTEXT("ShipyardOrdered", "LAID DOWN AT {0}: {1}"), FAPSFleetCommand::DisplayName(Yard),
			ShipyardOptions[OptionIndex].Name)
		: Refusal;
	RefreshShipyard(true);
	return FReply::Handled();
}

#undef LOCTEXT_NAMESPACE
