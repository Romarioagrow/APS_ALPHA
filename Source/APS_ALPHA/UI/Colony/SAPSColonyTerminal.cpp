#include "SAPSColonyTerminal.h"

#include "SAPSCivilizationMap.h"

#include "APS_ALPHA/Actors/Astro/Planet.h"
#include "APS_ALPHA/Actors/Astro/PlanetaryBody.h"
#include "APS_ALPHA/Actors/Astro/Star.h"
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
#include "APS_ALPHA/Generation/AstroGenerator.h"
#include "APS_ALPHA/Pawns/Spaceships/APSShipFlightModel.h"
#include "APS_ALPHA/Pawns/Spaceships/ShipNavigationComponent.h"
#include "APS_ALPHA/Pawns/Spaceships/Spaceship.h"
#include "APS_ALPHA/UI/Style/APSMenuChrome.h"
#include "EngineUtils.h"
#include "Framework/Application/SlateApplication.h"
#include "GameFramework/PlayerController.h"
#include "InputCoreTypes.h"
#include "Styling/AppStyle.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
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
					return IsSelected.Get(false) ? FLinearColor(0.02f, 0.13f, 0.17f, 0.97f) : Panel();
				})
				.ChamferTop(true)
				.ChamferBottom(true)
			]
			+ SOverlay::Slot().Padding(FMargin(12.0f, 9.0f))
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

	/** The one obvious action of a card or row: a filled amber button with dark text; dim when unavailable
	 * (Rio, 30.09: "it is unclear what to click, everything blends"). */
	TSharedRef<SWidget> PrimaryButton(const FText& Label, FOnClicked OnClicked, TAttribute<bool> CanClick)
	{
		const TSharedRef<SButton> Button = SNew(SButton)
			.ButtonStyle(FAppStyle::Get(), "NoBorder")
			.ContentPadding(0.0f)
			.IsEnabled(CanClick)
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
						return FLinearColor(0.07f, 0.13f, 0.16f, 0.95f);
					}
					const TSharedPtr<SButton> Pinned = WeakButton.Pin();
					return Pinned && Pinned->IsHovered() ? FLinearColor(1.0f, 0.83f, 0.38f, 1.0f) : Amber();
				})
				.ChamferTop(true)
				.ChamferBottom(true)
			]
			+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center).Padding(FMargin(14.0f, 8.0f))
			[
				SNew(STextBlock).Text(Label).Font(Font("Bold", 11))
				.ColorAndOpacity_Lambda([CanClick]()
				{
					return FSlateColor(CanClick.Get(true) ? FLinearColor(0.02f, 0.05f, 0.07f, 1.0f) : Muted());
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
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(STextBlock).Text(Label).Font(Font("Bold", 12)).ColorAndOpacity(White())
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 2.0f, 0.0f, 0.0f)
				[
					SNew(STextBlock).Text(Details).Font(Font("Regular", 10)).ColorAndOpacity(Muted())
				]
			],
			FOnClicked::CreateSP(this, &SAPSColonyTerminal::SelectTab, Tab),
			TAttribute<bool>::CreateLambda([this, Tab]() { return ActiveTab == Tab; }),
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
						SNew(STextBlock).Text(LOCTEXT("Subtitle", "LIVE CIVILIZATION DATA  /  TAB OR ESC TO CLOSE  /  K FLEET COMMAND"))
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
							TabButton(EAPSChromeGlyph::Compass, LOCTEXT("MapTab", "MAP"),
								LOCTEXT("MapDetails", "Ships, stations, colony, course"), ETab::Map)
						]
						+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 8.0f)
						[
							TabButton(EAPSChromeGlyph::Headquarters, LOCTEXT("Colony", "COLONY"),
								LOCTEXT("ColonyDetails", "Base, landing pad, construction"), ETab::Colony)
						]
						+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 8.0f)
						[
							TabButton(EAPSChromeGlyph::Fleet, LOCTEXT("FleetTab", "FLEET COMMAND"),
								LOCTEXT("FleetDetails", "Ships, divisions and orders  (K)"), ETab::Fleet)
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
						+ SWidgetSwitcher::Slot()[BuildJournal()],
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
	using namespace APSColonyUI;
	const UMainGameplayInstance* State = GameplayState(World.Get());
	const UCivilization* Civilization = State ? State->CurrentCivilization.Get() : nullptr;
	const USpawnParameters* Spawn = State ? State->SpawnParameters : nullptr;
	const APlanet* Planet = HomePlanet(World.Get());
	const AStar* Star = Planet ? Planet->ParentStar : nullptr;

	TSharedRef<SUniformGridPanel> CivilizationGrid = SNew(SUniformGridPanel).SlotPadding(FMargin(4.0f));
	int32 Cell = 0;
	const auto AddTile = [&CivilizationGrid, &Cell](const FText& Label, const FText& Value, const FLinearColor& Accent)
	{
		CivilizationGrid->AddSlot(Cell % 3, Cell / 3)[MetricTile(Label, Value, Accent)];
		++Cell;
	};
	if (Civilization)
	{
		AddTile(LOCTEXT("Archetype", "ARCHETYPE"), EnumText(Civilization->Archetype).ToUpper(), Cyan());
		AddTile(LOCTEXT("Government", "GOVERNMENT"), EnumText(Civilization->Government).ToUpper(), Cyan());
		AddTile(LOCTEXT("Economy", "ECONOMY"), EnumText(Civilization->Economy).ToUpper(), Cyan());
		AddTile(LOCTEXT("Society", "SOCIETY"), EnumText(Civilization->Society).ToUpper(), Cyan());
		AddTile(LOCTEXT("Population", "POPULATION"), FText::AsNumber(Civilization->Population), Amber());
		AddTile(LOCTEXT("Credits", "CREDITS"), FText::AsNumber(Civilization->Credits), Amber());
		AddTile(LOCTEXT("Tech", "TECH LEVEL"), FText::AsNumber(Civilization->TechnologyLevel), Cyan());
		AddTile(LOCTEXT("Fleet", "FLEET"), FText::AsNumber(Civilization->FleetSize), Cyan());
	}
	else
	{
		AddTile(LOCTEXT("NoCivilization", "CIVILIZATION"), LOCTEXT("NoCivilizationValue", "NOT GENERATED"), Muted());
	}

	TSharedRef<SUniformGridPanel> HomeGrid = SNew(SUniformGridPanel).SlotPadding(FMargin(4.0f));
	HomeGrid->AddSlot(0, 0)[MetricTile(LOCTEXT("Star", "HOME STAR"),
		Star && !Star->AstroName.IsNone() ? FText::FromName(Star->AstroName) : LOCTEXT("Unknown", "UNKNOWN"), Amber())];
	HomeGrid->AddSlot(1, 0)[MetricTile(LOCTEXT("Planet", "HOME PLANET"),
		Planet && !Planet->AstroName.IsNone() ? FText::FromName(Planet->AstroName) : LOCTEXT("Unknown2", "UNKNOWN"), Cyan())];
	TSharedRef<SUniformGridPanel> InfrastructureGrid = SNew(SUniformGridPanel).SlotPadding(FMargin(4.0f));
	const FAPSCivilizationInfrastructure Infrastructure = Civilization
		? Civilization->Infrastructure : FAPSCivilizationInfrastructure();
	InfrastructureGrid->AddSlot(0, 0)[MetricTile(LOCTEXT("OrbitalStations", "ORBITAL STATIONS"),
		FText::AsNumber(Infrastructure.OrbitalStations), Cyan())];
	InfrastructureGrid->AddSlot(1, 0)[MetricTile(LOCTEXT("GroundSettlements", "GROUND SETTLEMENTS"),
		FText::AsNumber(Infrastructure.GroundSettlements), Cyan())];
	InfrastructureGrid->AddSlot(2, 0)[MetricTile(LOCTEXT("PlanetOutposts", "PLANET OUTPOSTS"),
		TAttribute<FText>::CreateLambda([this]()
		{
			// Live: construction ships add to it (fleet command).
			const UMainGameplayInstance* LiveState = GameplayState(World.Get());
			const UCivilization* Live = LiveState ? LiveState->CurrentCivilization.Get() : nullptr;
			return FText::AsNumber(Live ? Live->Infrastructure.PlanetOutposts : 0);
		}), Cyan())];
	InfrastructureGrid->AddSlot(0, 1)[MetricTile(LOCTEXT("StarOutposts", "STAR OUTPOSTS"),
		FText::AsNumber(Infrastructure.StarOutposts), Cyan())];
	HomeGrid->AddSlot(2, 0)[MetricTile(LOCTEXT("Start", "START"),
		Spawn ? EnumText(Spawn->CharacterSpawnPlace).ToUpper() : LOCTEXT("Unknown3", "UNKNOWN"), Cyan())];
	// How much of the system the civilization knows (surveys by fleet command).
	HomeGrid->AddSlot(0, 1)[MetricTile(LOCTEXT("WorldsSurveyed", "WORLDS SURVEYED"),
		TAttribute<FText>::CreateLambda([this]()
		{
			int32 Worlds = 0, Known = 0;
			const FAPSFleetCommand* Fleet = GetFleet();
			if (UWorld* LiveWorld = World.Get())
			{
				for (TActorIterator<APlanetaryBody> It(LiveWorld); It; ++It)
				{
					++Worlds;
					Known += Fleet && Fleet->GetSurvey(*It) != APSFleet::ESurvey::Unknown ? 1 : 0;
				}
			}
			return FText::Format(LOCTEXT("WorldsSurveyedValue", "{0} OF {1}"), FText::AsNumber(Known), FText::AsNumber(Worlds));
		}), Amber())];

	return SNew(SScrollBox)
		+ SScrollBox::Slot().Padding(0.0f, 0.0f, 0.0f, 12.0f)
		[
			IconSectionHeading(EAPSChromeGlyph::Civilization,
				FText::FromString(Civilization ? Civilization->Name.ToUpper() : FString(TEXT("CIVILIZATION"))),
				LOCTEXT("CivilizationSubtitle", "Founding parameters of this world"))
		]
		+ SScrollBox::Slot().Padding(0.0f, 0.0f, 0.0f, 22.0f)
		[
			CivilizationGrid
		]
		+ SScrollBox::Slot().Padding(0.0f, 0.0f, 0.0f, 12.0f)
		[
			IconSectionHeading(EAPSChromeGlyph::System, LOCTEXT("HomeSection", "HOME"),
				LOCTEXT("HomeSubtitle", "Star, planet and the start of this session"))
		]
		+ SScrollBox::Slot().Padding(0.0f, 0.0f, 0.0f, 22.0f)
		[
			HomeGrid
		]
		+ SScrollBox::Slot().Padding(0.0f, 0.0f, 0.0f, 12.0f)
		[
			IconSectionHeading(EAPSChromeGlyph::Infrastructure, LOCTEXT("InfrastructureSection", "INFRASTRUCTURE"),
				LOCTEXT("InfrastructureSubtitle", "Stations, settlements and outposts founded with the civilization"))
		]
		+ SScrollBox::Slot()
		[
			InfrastructureGrid
		];
}

TSharedRef<SWidget> SAPSColonyTerminal::BuildDivisions()
{
	using namespace APSColonyUI;
	const UMainGameplayInstance* State = GameplayState(World.Get());
	const UCivilization* Civilization = State ? State->CurrentCivilization.Get() : nullptr;
	struct FDivisionCard
	{
		FText Name;
		FText Role;
		int32 Level;
		EAPSChromeGlyph Glyph;
		/** The fleet division of its ships (APSFleet::EDivision), -1 none, -2 fleet command (every ship's speed). */
		int32 FleetDivision;
	};
	const FAPSCivilizationDivisions Levels = Civilization ? Civilization->Divisions : FAPSCivilizationDivisions();
	const FDivisionCard Cards[] = {
		{LOCTEXT("Exploration", "EXPLORATION"), LOCTEXT("ExplorationRole",
			"Surveys planets, systems and clusters, charts routes and finds anomalies."), Levels.Exploration, EAPSChromeGlyph::Compass,
			static_cast<int32>(APSFleet::EDivision::Exploration)},
		{LOCTEXT("Industry", "INDUSTRY"), LOCTEXT("IndustryRole",
			"Production, mining, construction and the shipyards: everything the colony builds."), Levels.Industry,
			EAPSChromeGlyph::Infrastructure, static_cast<int32>(APSFleet::EDivision::Construction)},
		{LOCTEXT("Science", "SCIENCE"), LOCTEXT("ScienceRole",
			"Research and new technologies; investigates what exploration finds."), Levels.Science, EAPSChromeGlyph::Planet,
			static_cast<int32>(APSFleet::EDivision::Science)},
		{LOCTEXT("CivilAffairs", "CIVIL AFFAIRS"), LOCTEXT("CivilAffairsRole",
			"Claims star systems, runs public services, law and population growth."), Levels.CivilAffairs,
			EAPSChromeGlyph::Civilization, -1},
		{LOCTEXT("Military", "MILITARY"), LOCTEXT("MilitaryRole",
			"Forces, defence protocols and training."), Levels.Military, EAPSChromeGlyph::Lock,
			static_cast<int32>(APSFleet::EDivision::MainFleet)},
		{LOCTEXT("FleetCommand", "FLEET COMMAND"), LOCTEXT("FleetCommandRole",
			"Deploys, repairs and upgrades the fleet."), Levels.FleetCommand, EAPSChromeGlyph::Fleet, -2}};
	TSharedRef<SUniformGridPanel> Grid = SNew(SUniformGridPanel).SlotPadding(FMargin(6.0f));
	for (int32 Index = 0; Index < UE_ARRAY_COUNT(Cards); ++Index)
	{
		const FDivisionCard& Card = Cards[Index];
		TSharedRef<SHorizontalBox> Pips = SNew(SHorizontalBox);
		for (int32 Pip = 0; Pip < 5; ++Pip)
		{
			Pips->AddSlot().AutoWidth().Padding(0.0f, 0.0f, 4.0f, 0.0f)
			[
				SNew(SBox).WidthOverride(22.0f).HeightOverride(6.0f)
				[
					SNew(SBorder).BorderImage(FAppStyle::GetBrush("WhiteBrush"))
					.BorderBackgroundColor(Pip < Card.Level ? Amber() : CyanDim())
				]
			];
		}
		Grid->AddSlot(Index % 2, Index / 2)
		[
			ChamferPanel(
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
					[
						IconBadge(Card.Glyph, Cyan(), 34.0f)
					]
					+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center).Padding(10.0f, 0.0f)
					[
						SNew(STextBlock).Text(Card.Name).Font(Font("Bold", 13)).ColorAndOpacity(White())
					]
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
					[
						SNew(STextBlock).Text(FText::Format(LOCTEXT("DivisionLevel", "LEVEL {0}"), FText::AsNumber(Card.Level)))
						.Font(Font("Bold", 11)).ColorAndOpacity(Amber())
					]
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 10.0f, 0.0f, 0.0f)
				[
					Pips
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 10.0f, 0.0f, 0.0f)
				[
					SNew(STextBlock).Text(Card.Role).AutoWrapText(true).Font(Font("Regular", 11)).ColorAndOpacity(Muted())
				]
				// Its ships in fleet command (K), live.
				+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 8.0f, 0.0f, 0.0f)
				[
					SNew(STextBlock).AutoWrapText(true).Font(Font("Bold", 10)).ColorAndOpacity(Cyan())
					.Visibility(Card.FleetDivision == -1 ? EVisibility::Collapsed : EVisibility::Visible)
					.Text_Lambda([this, FleetDivision = Card.FleetDivision, Level = Card.Level]()
					{
						if (FleetDivision == -2)
						{
							return FText::Format(LOCTEXT("FleetSpeedBonus", "EVERY SHIP FLIES {0}% FASTER"),
								FText::AsNumber(10 * FMath::Max(Level, 0)));
						}
						int32 Ships = 0, UnderOrders = 0;
						if (const FAPSFleetCommand* Fleet = GetFleet())
						{
							for (const FAPSFleetUnit& Unit : Fleet->GetUnits())
							{
								if (static_cast<int32>(Unit.Division) == FleetDivision)
								{
									++Ships;
									UnderOrders += Unit.Order != APSFleet::EOrder::None ? 1 : 0;
								}
							}
						}
						return FText::Format(LOCTEXT("DivisionShips", "SHIPS {0}  /  UNDER ORDERS {1}"), FText::AsNumber(Ships),
							FText::AsNumber(UnderOrders));
					})
				],
				FMargin(16.0f, 14.0f), CyanDim())
		];
	}
	return SNew(SScrollBox)
		+ SScrollBox::Slot().Padding(0.0f, 0.0f, 0.0f, 12.0f)
		[
			IconSectionHeading(EAPSChromeGlyph::Divisions, LOCTEXT("DivisionsSection", "DIVISIONS"),
				Civilization ? LOCTEXT("DivisionsSubtitle", "Levels set when the civilization was founded; ships and orders: FLEET COMMAND (K)")
				: LOCTEXT("DivisionsNoCivilization", "No civilization in this world yet"))
		]
		+ SScrollBox::Slot()
		[
			Grid
		];
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
		return ChamferPanel(
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
				.Font(Font("Bold", 11))
			],
			FMargin(14.0f, 11.0f), CyanDim());
	};

	return SNew(SScrollBox)
		+ SScrollBox::Slot().Padding(0.0f, 0.0f, 0.0f, 12.0f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(1.0f)
			[
				IconSectionHeading(EAPSChromeGlyph::Headquarters, LOCTEXT("ColonySection", "COLONY ON THE HOME PLANET"),
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
		// Construction lives with the colony: one civilization menu (Rio, 30.09).
		+ SScrollBox::Slot().Padding(0.0f, 28.0f, 0.0f, 0.0f)
		[
			BuildConstruction()
		];
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
						: APSFleet::DivisionName(static_cast<APSFleet::EDivision>(Filter)), FText::AsNumber(Count));
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

	return SNew(SHorizontalBox)
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
						.Text_Lambda([this]()
						{
							return FText::Format(LOCTEXT("PickedCount", "{0} PICKED  /  CTRL ADDS"), FText::AsNumber(GetPickedShips().Num()));
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
				.OnSelectionChanged_Lambda([this]() { HandleFleetMapSelection(); })
			]
			// The view's name and, in a planet's local view, the way back to the system.
			+ SOverlay::Slot().HAlign(HAlign_Left).VAlign(VAlign_Top).Padding(8.0f)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(STextBlock).Font(Font("Bold", 11)).ColorAndOpacity(Cyan())
					.Text_Lambda([this]() { return FleetMap.IsValid() ? FleetMap->GetViewTitle() : FText::GetEmpty(); })
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 6.0f, 0.0f, 0.0f)
				[
					SNew(SBox)
					.Visibility_Lambda([this]()
					{
						return FleetMap.IsValid() && FleetMap->IsLocalView() ? EVisibility::Visible : EVisibility::Collapsed;
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
							+ SVerticalBox::Slot().AutoHeight()
							[
								SNew(STextBlock).Font(Font("Bold", 13)).ColorAndOpacity(White())
								.Text_Lambda([this]()
								{
									const AActor* Target = FleetTarget.Get();
									const SAPSCivilizationMap::FObject* Object = FleetMap.IsValid() ? FleetMap->GetSelected() : nullptr;
									return !Target ? LOCTEXT("NoTarget", "NO TARGET")
										: Object && Object->Actor.Get() == Target ? Object->Name
										: FText::FromString(Target->GetName());
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
										return LOCTEXT("TargetHelp", "Click a planet, moon, station or outpost on the map. Double-click a planet for its moons.");
									}
									const FAPSFleetCommand* Fleet = GetFleet();
									if (Target->IsA<APlanetaryBody>() && Fleet)
									{
										return FText::Format(LOCTEXT("TargetSurvey", "{0}  /  OUTPOSTS {1} OF 3"),
											APSFleet::SurveyName(Fleet->GetSurvey(Target)), FText::AsNumber(Fleet->CountOutposts(Target)));
									}
									return LOCTEXT("TargetStation", "STATION OR OUTPOST: ships can fly there");
								})
							]
							+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 6.0f, 0.0f, 0.0f)
							[
								SNew(STextBlock).AutoWrapText(true).Font(Font("Regular", 11)).ColorAndOpacity(Muted())
								.Text_Lambda([this]()
								{
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
							],
							FMargin(14.0f, 12.0f), CyanDim())
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 14.0f, 0.0f, 0.0f)[OrderButton(APSFleet::EOrder::Move)]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 8.0f, 0.0f, 0.0f)[OrderButton(APSFleet::EOrder::Survey)]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 8.0f, 0.0f, 0.0f)[OrderButton(APSFleet::EOrder::BuildOutpost)]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 8.0f, 0.0f, 0.0f)[OrderButton(APSFleet::EOrder::Return)]
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
							int32 Surveyed = 0, Studied = 0, Outposts = 0;
							for (const FAPSFleetBodyRecord& Record : Fleet->GetBodies())
							{
								Surveyed += Record.Survey == APSFleet::ESurvey::Surveyed ? 1 : 0;
								Studied += Record.Survey == APSFleet::ESurvey::Studied ? 1 : 0;
								Outposts += Fleet->CountOutposts(Record.Body.Get());
							}
							return FText::Format(LOCTEXT("KnownWorlds", "KNOWN WORLDS: {0} studied, {1} surveyed  /  OUTPOSTS BUILT: {2}"),
								FText::AsNumber(Studied), FText::AsNumber(Surveyed), FText::AsNumber(Outposts));
						})
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
				SNew(STextBlock).Font(Font("Bold", 12)).ColorAndOpacity(White())
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
	const int32 Issued = Fleet->IssueOrder(GetPickedShips(), Order, FleetTarget.Get(), Refusal);
	bFleetMessageIsError = Issued == 0;
	FleetMessage = Issued == 0 ? Refusal : FText::Format(LOCTEXT("OrderGiven", "ORDER GIVEN: {0} ({1} ships){2}"),
		APSFleet::OrderName(Order), FText::AsNumber(Issued), Refusal.IsEmpty() ? FText::GetEmpty()
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
	JournalList->AddSlot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 12.0f)
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
			.Font(Font("Regular", 12)).ColorAndOpacity(Muted())
		];
		return;
	}
	// Newest first: the journal reads like a ship's log.
	const TArray<FAPSCivilizationJournalEntry>& Entries = Journal->GetEntries();
	for (int32 Index = Entries.Num() - 1; Index >= 0; --Index)
	{
		const FAPSCivilizationJournalEntry& Entry = Entries[Index];
		const int32 Seconds = FMath::FloorToInt(Entry.WorldSeconds);
		const FText Time = FText::FromString(FString::Printf(TEXT("T+%02d:%02d:%02d"),
			Seconds / 3600, (Seconds / 60) % 60, Seconds % 60));
		JournalList->AddSlot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 6.0f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Top).Padding(0.0f, 0.0f, 12.0f, 0.0f)
			[
				SNew(SBox).WidthOverride(96.0f)
				[
					SNew(STextBlock).Text(Time).Font(Font("Bold", 10)).ColorAndOpacity(Muted())
				]
			]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Top).Padding(0.0f, 0.0f, 12.0f, 0.0f)
			[
				SNew(SBox).WidthOverride(92.0f)
				[
					SNew(STextBlock).Text(FText::FromName(Entry.Category).ToUpper()).Font(Font("Bold", 10))
					.ColorAndOpacity(Cyan())
				]
			]
			+ SHorizontalBox::Slot().FillWidth(1.0f)
			[
				SNew(STextBlock).Text(Entry.Text).AutoWrapText(true).Font(Font("Regular", 12)).ColorAndOpacity(White())
			]
		];
	}
}

void SAPSColonyTerminal::HandleJournalEntry(const FAPSCivilizationJournalEntry& Entry)
{
	RebuildJournal();
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
	ActiveTab = Tab;
	if (Switcher.IsValid())
	{
		Switcher->SetActiveWidgetIndex(static_cast<int32>(Tab));
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

FReply SAPSColonyTerminal::OnKeyDown(const FGeometry& Geometry, const FKeyEvent& Event)
{
	const FKey Key = Event.GetKey();
	if (Key == EKeys::Tab || Key == EKeys::Escape)
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
	return SCompoundWidget::OnKeyDown(Geometry, Event);
}

void SAPSColonyTerminal::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
	SCompoundWidget::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);
	if (ActiveTab != ETab::Colony && ActiveTab != ETab::Map && ActiveTab != ETab::Fleet)
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
		else
		{
			RefreshConstruction();
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
	return SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().FillWidth(1.0f)
		[
			SAssignNew(Map, SAPSCivilizationMap)
			.World(World)
			.OnSelectionChanged_Lambda([this]()
			{
				CourseMessage = FText::GetEmpty();
			})
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
	for (const SAPSCivilizationMap::FObject* Object : Shown)
	{
		if (Object->Kind == SAPSCivilizationMap::EKind::Pilot)
		{
			continue;
		}
		const FString StableId = Object->StableId;
		const TWeakObjectPtr<AActor> Actor = Object->Actor;
		MapList->AddSlot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 5.0f)
		[
			ChromeButton(
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight()
					[
						SNew(STextBlock).Text(Object->Name).Font(Font("Bold", 10))
						.ColorAndOpacity(FSlateColor(Object->Color))
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
					.Font(Font("Bold", 10)).ColorAndOpacity(Amber())
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
	const FText Metrics = FText::Format(LOCTEXT("ModuleMetrics", "{0} x {1} M  /  {2} S"),
		FText::AsNumber(FMath::RoundToInt(Spec.SizeCm.X / 100.0)),
		FText::AsNumber(FMath::RoundToInt(Spec.SizeCm.Y / 100.0)),
		FText::AsNumber(FMath::RoundToInt(Spec.BuildSeconds)));
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
			State = FText::Format(LOCTEXT("QueuedState", "QUEUED  /  #{0}"), FText::AsNumber(Job->QueuePosition + 1));
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
					.Font(Font("Bold", 11)).ColorAndOpacity(White())
				]
			]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.0f, 0.0f, 12.0f, 0.0f)
			[
				SNew(SBox).WidthOverride(290.0f)
				[
					SNew(STextBlock).Text(State).Font(Font("Bold", 10)).ColorAndOpacity(StateColor)
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
					.Font(Font("Bold", 10)).ColorAndOpacity(Muted())
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
				],
				FMargin(14.0f, 9.0f), CyanDim())
		];
	}
}

#undef LOCTEXT_NAMESPACE
