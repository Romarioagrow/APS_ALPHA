#include "SAPSColonyTerminal.h"

#include "SAPSCivilizationMap.h"

#include "APS_ALPHA/Actors/Astro/Planet.h"
#include "APS_ALPHA/Actors/Astro/PlanetaryBody.h"
#include "APS_ALPHA/Actors/Astro/Star.h"
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
#include "APS_ALPHA/Generation/AstroGenerator.h"
#include "APS_ALPHA/Generation/PlanetarySurfaceGenerator.h"
#include "PlanetaryAtmosphere.h"
#include "WorldScapeCore/Public/WorldScapeRoot.h"
#include "APS_ALPHA/Pawns/Spaceships/APSShipFlightModel.h"
#include "APS_ALPHA/Pawns/Spaceships/ShipNavigationComponent.h"
#include "APS_ALPHA/Pawns/Spaceships/Spaceship.h"
#include "APS_ALPHA/UI/MainMenu/APSUIThumbnails.h"
#include "APS_ALPHA/UI/Style/APSMenuChrome.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Engine/SceneCapture2D.h"
#include "Engine/Texture2D.h"
#include "Engine/TextureRenderTarget2D.h"
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
						+ SWidgetSwitcher::Slot()[BuildShipyard()],
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
	if (AActor* Camera = PreviewCamera.Get())
	{
		Camera->Destroy();
	}
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
		EAPSChromeGlyph Glyph;
		/** The fleet division of its ships (APSFleet::EDivision), -1 none, -2 fleet command (every ship). */
		int32 FleetDivision;
	};
	const FDivisionCard Cards[] = {
		{LOCTEXT("Exploration", "EXPLORATION"), LOCTEXT("ExplorationRole",
			"Surveys planets, systems and clusters, charts routes and finds anomalies."), EAPSChromeGlyph::Compass,
			static_cast<int32>(APSFleet::EDivision::Exploration)},
		{LOCTEXT("Industry", "INDUSTRY"), LOCTEXT("IndustryRole",
			"Production, mining, construction and the shipyards: everything the colony builds."),
			EAPSChromeGlyph::Infrastructure, static_cast<int32>(APSFleet::EDivision::Construction)},
		{LOCTEXT("Science", "SCIENCE"), LOCTEXT("ScienceRole",
			"Research and new technologies; investigates what exploration finds."), EAPSChromeGlyph::Planet,
			static_cast<int32>(APSFleet::EDivision::Science)},
		{LOCTEXT("CivilAffairs", "CIVIL AFFAIRS"), LOCTEXT("CivilAffairsRole",
			"Claims star systems, runs public services, law and population growth."), EAPSChromeGlyph::Civilization, -1},
		{LOCTEXT("Military", "MILITARY"), LOCTEXT("MilitaryRole",
			"Forces, defence protocols and training."), EAPSChromeGlyph::Lock, static_cast<int32>(APSFleet::EDivision::MainFleet)},
		{LOCTEXT("FleetCommand", "FLEET COMMAND"), LOCTEXT("FleetCommandRole",
			"Deploys, repairs and upgrades the fleet."), EAPSChromeGlyph::Fleet, -2}};
	TSharedRef<SUniformGridPanel> Grid = SNew(SUniformGridPanel).SlotPadding(FMargin(6.0f));
	for (int32 Index = 0; Index < UE_ARRAY_COUNT(Cards); ++Index)
	{
		const FDivisionCard& Card = Cards[Index];
		// Eight pips: the civilization's levels in amber, those its work earned in green.
		TSharedRef<SHorizontalBox> Pips = SNew(SHorizontalBox);
		for (int32 Pip = 0; Pip < 8; ++Pip)
		{
			Pips->AddSlot().AutoWidth().Padding(0.0f, 0.0f, 4.0f, 0.0f)
			[
				SNew(SBox).WidthOverride(18.0f).HeightOverride(6.0f)
				[
					SNew(SBorder).BorderImage(FAppStyle::GetBrush("WhiteBrush"))
					.BorderBackgroundColor_Lambda([this, Index, Pip]()
					{
						int32 Earned = 0;
						const int32 Level = DivisionCardLevel(Index, Earned);
						return FSlateColor(Pip < Level - Earned ? Amber() : Pip < Level ? Success() : CyanDim());
					})
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
						SNew(STextBlock).Font(Font("Bold", 11)).ColorAndOpacity(Amber())
						.Text_Lambda([this, Index]()
						{
							int32 Earned = 0;
							const int32 Level = DivisionCardLevel(Index, Earned);
							return Earned > 0
								? FText::Format(LOCTEXT("DivisionLevelEarned", "LEVEL {0}  (+{1} EARNED)"), FText::AsNumber(Level),
									FText::AsNumber(Earned))
								: FText::Format(LOCTEXT("DivisionLevel", "LEVEL {0}"), FText::AsNumber(Level));
						})
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
				// What the level does in the game now, and how the division grows (Rio, 01.10).
				+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 8.0f, 0.0f, 0.0f)
				[
					SNew(STextBlock).AutoWrapText(true).Font(Font("Bold", 10)).ColorAndOpacity(White())
					.Text_Lambda([this, Index]() { return DivisionCardEffect(Index, false); })
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 3.0f, 0.0f, 0.0f)
				[
					SNew(STextBlock).AutoWrapText(true).Font(Font("Regular", 10)).ColorAndOpacity(Success())
					.Text_Lambda([this, Index]() { return DivisionCardEffect(Index, true); })
				]
				// Its ships in fleet command (K), live.
				+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 6.0f, 0.0f, 0.0f)
				[
					SNew(STextBlock).AutoWrapText(true).Font(Font("Bold", 10)).ColorAndOpacity(Cyan())
					.Visibility(Card.FleetDivision < 0 ? EVisibility::Collapsed : EVisibility::Visible)
					.Text_Lambda([this, FleetDivision = Card.FleetDivision]()
					{
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
				Civilization ? LOCTEXT("DivisionsSubtitleEffects", "Levels from the founding (amber) and earned by work (green); what each changes in the game, live")
				: LOCTEXT("DivisionsNoCivilization", "No civilization in this world yet"))
		]
		+ SScrollBox::Slot()
		[
			Grid
		];
}

int32 SAPSColonyTerminal::DivisionCardLevel(const int32 CardIndex, int32& OutEarned) const
{
	using namespace APSColonyUI;
	OutEarned = 0;
	const UMainGameplayInstance* State = GameplayState(World.Get());
	const UCivilization* Civ = State ? State->CurrentCivilization.Get() : nullptr;
	const FAPSCivilizationDivisions Levels = Civ ? Civ->Divisions : FAPSCivilizationDivisions();
	const FAPSFleetCommand* Fleet = GetFleet();
	switch (CardIndex)
	{
	case 0:
		OutEarned = Fleet ? Fleet->GetEarnedLevel(APSFleet::EDivision::Exploration) : 0;
		return Levels.Exploration + OutEarned;
	case 1:
		OutEarned = Fleet ? Fleet->GetEarnedLevel(APSFleet::EDivision::Construction) : 0;
		return Levels.Industry + OutEarned;
	case 2:
		OutEarned = Fleet ? Fleet->GetEarnedLevel(APSFleet::EDivision::Science) : 0;
		return Levels.Science + OutEarned;
	case 3:
		return Levels.CivilAffairs;
	case 4:
		return Levels.Military;
	default:
		OutEarned = Fleet ? Fleet->GetEarnedFleetCommandLevel() : 0;
		return Levels.FleetCommand + OutEarned;
	}
}

FText SAPSColonyTerminal::DivisionCardEffect(const int32 CardIndex, const bool bGrowth) const
{
	using namespace APSFleet;
	const FAPSFleetCommand* Fleet = GetFleet();
	int32 Earned = 0;
	const int32 Level = DivisionCardLevel(CardIndex, Earned);
	const auto Seconds = [](const double Value) { return FText::AsNumber(FMath::RoundToInt(Value)); };
	const FAPSFleetCommand::FWorkTally Tally = Fleet ? Fleet->GetWorkTally() : FAPSFleetCommand::FWorkTally();
	switch (CardIndex)
	{
	case 0:
		return bGrowth
			? FText::Format(LOCTEXT("ExplorationGrowth", "GROWS: +1 per 3 worlds surveyed, up to +3. Surveyed so far: {0}."),
				FText::AsNumber(Tally.Surveyed))
			: FText::Format(LOCTEXT("ExplorationEffect", "A SURVEY TAKES {0} S AT THE WORLD, {1} S WHERE A STATION STANDS"),
				Seconds(WorkSeconds(EOrder::Survey, EDivision::Exploration, Level)),
				Seconds(WorkSeconds(EOrder::Survey, EDivision::Exploration, Level, true)));
	case 1:
		return bGrowth
			? FText::Format(LOCTEXT("IndustryGrowth", "GROWS: +1 per 3 outposts, stations, shipyards or HQs built, up to +3. Built so far: {0}."),
				FText::AsNumber(Tally.Built))
			: FText::Format(LOCTEXT("IndustryEffect", "OUTPOST {0} S  /  STATION {1} S  /  SHIPYARD {2} S  /  SECTOR HQ {3} S;  SLIPWAYS BUILD {4}% FASTER"),
				Seconds(WorkSeconds(EOrder::BuildOutpost, EDivision::Construction, Level)),
				Seconds(WorkSeconds(EOrder::BuildStation, EDivision::Construction, Level)),
				Seconds(WorkSeconds(EOrder::BuildShipyard, EDivision::Construction, Level)),
				Seconds(WorkSeconds(EOrder::BuildHeadquarters, EDivision::Construction, Level)),
				FText::AsNumber(FMath::RoundToInt(20.0f * FMath::Max(Level, 0))));
	case 2:
		return bGrowth
			? FText::Format(LOCTEXT("ScienceGrowthAnomalies", "GROWS: +1 per 2 worlds studied or anomalies investigated (on foot counts twice), up to +3. So far: {0} studied, {1} from anomalies."),
				FText::AsNumber(Tally.Studied), FText::AsNumber(Tally.Investigated))
			: FText::Format(LOCTEXT("ScienceEffect", "A STUDY (LIFE, GEOLOGY, METALS) TAKES {0} S, {1} S WHERE A STATION STANDS"),
				Seconds(WorkSeconds(EOrder::Survey, EDivision::Science, Level)),
				Seconds(WorkSeconds(EOrder::Survey, EDivision::Science, Level, true)));
	case 3:
		return bGrowth ? LOCTEXT("CivilGrowth", "Set when the civilization was founded.")
			: FText::Format(LOCTEXT("CivilEffect", "COLONY MODULES BUILD {0}% FASTER"),
				FText::AsNumber(15 * FMath::Max(Level, 0)));
	case 4:
		return bGrowth ? LOCTEXT("MilitaryGrowth", "Set when the civilization was founded.")
			: FText::Format(LOCTEXT("MilitaryEffect", "MAIN FLEET SHIPS FLY {0}% FASTER: THE LINE ANSWERS FIRST"),
				FText::AsNumber(15 * FMath::Max(Level, 0)));
	default:
	{
		const int32 Headquarters = Fleet ? Fleet->CountBuiltHeadquarters() : 0;
		return bGrowth
			? FText::Format(LOCTEXT("FleetCommandGrowth", "GROWS: +1 per 4 ships launched from the slipways, up to +3. Launched so far: {0}."),
				FText::AsNumber(Tally.Launched))
			: FText::Format(LOCTEXT("FleetCommandEffect", "EVERY SHIP FLIES {0}% FASTER ({1}% OF IT FROM {2} SECTOR HQS)"),
				FText::AsNumber(10 * (FMath::Max(Level, 0) + Headquarters)), FText::AsNumber(10 * Headquarters),
				FText::AsNumber(Headquarters));
	}
	}
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
							// A studied world's photograph (B1).
							+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0.0f, 0.0f, 0.0f, 8.0f)
							[
								SNew(SBox).WidthOverride(220.0f).HeightOverride(220.0f)
								.Visibility_Lambda([this]()
								{
									return PreviewBrush.IsValid() && PreviewSubject.IsValid() && PreviewSubject == FleetTarget
										? EVisibility::Visible : EVisibility::Collapsed;
								})
								[
									SNew(SImage).Image_Lambda([this]() { return PreviewBrush.Get(); })
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
										return LOCTEXT("TargetHelp", "Click a planet, moon, station or outpost on the map. Double-click a planet for its moons.");
									}
									const FAPSFleetCommand* Fleet = GetFleet();
									if (Target->IsA<APlanetaryBody>() && Fleet)
									{
										using APSFleet::EStructure;
										const int32 Shipyards = Fleet->CountStructures(Target, EStructure::Shipyard);
										const int32 Headquarters = Fleet->CountStructures(Target, EStructure::Headquarters);
										return FText::Format(LOCTEXT("TargetSurveyBuilt", "{0}  /  OUTPOSTS {1} OF 3  /  STATIONS {2}{3}{4}"),
											APSFleet::SurveyName(Fleet->GetSurvey(Target)), FText::AsNumber(Fleet->CountOutposts(Target)),
											FText::AsNumber(Fleet->CountStructures(Target, EStructure::Station)),
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
									const FAPSFleetCommand* Fleet = GetFleet();
									return Fleet ? Fleet->DescribeAnomaly(FleetTarget.Get()) : FText::GetEmpty();
								})
								.Visibility_Lambda([this]()
								{
									const FAPSFleetCommand* Fleet = GetFleet();
									return Fleet && !Fleet->DescribeAnomaly(FleetTarget.Get()).IsEmpty()
										? EVisibility::Visible : EVisibility::Collapsed;
								})
							],
							FMargin(14.0f, 12.0f), CyanDim())
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 14.0f, 0.0f, 0.0f)[OrderButton(APSFleet::EOrder::Move)]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 8.0f, 0.0f, 0.0f)[OrderButton(APSFleet::EOrder::Survey)]
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
						.Text(LOCTEXT("ConstructionHelp", "Construction ships at a surveyed world: an outpost first, then a station; a station opens a shipyard and a sector HQ."))
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 8.0f, 0.0f, 0.0f)[OrderButton(APSFleet::EOrder::BuildOutpost)]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 8.0f, 0.0f, 0.0f)[OrderButton(APSFleet::EOrder::BuildStation)]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 8.0f, 0.0f, 0.0f)[OrderButton(APSFleet::EOrder::BuildShipyard)]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 8.0f, 0.0f, 0.0f)[OrderButton(APSFleet::EOrder::BuildHeadquarters)]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 8.0f, 0.0f, 0.0f)
					[
						SNew(STextBlock).AutoWrapText(true).Font(Font("Regular", 10)).ColorAndOpacity(Muted())
						.Text(LOCTEXT("ConstructionEffects", "A station speeds work at its world up by 25%. A shipyard gets its own slipway in SHIPYARD. Every sector HQ: the fleet flies 10% faster. Megastructures are planned with the module-style buildings."))
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
								FText::AsNumber(Studied), FText::AsNumber(Surveyed), FText::AsNumber(Outposts), FText::AsNumber(Built));
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
		UpdateBodyPreview();
	}
}

void SAPSColonyTerminal::UpdateBodyPreview()
{
	UWorld* LiveWorld = World.Get();
	APlanetaryBody* Body = Cast<APlanetaryBody>(FleetTarget.Get());
	const FAPSFleetCommand* Fleet = GetFleet();
	if (!LiveWorld || !Body || !Fleet || Fleet->GetSurvey(Body) != APSFleet::ESurvey::Studied)
	{
		return;
	}
	if (PreviewSubject.Get() == Body && PreviewBrush.IsValid())
	{
		return;
	}
	if (!PreviewTarget.IsValid())
	{
		UTextureRenderTarget2D* Target = NewObject<UTextureRenderTarget2D>();
		Target->RenderTargetFormat = RTF_RGBA8;
		Target->ClearColor = FLinearColor::Black;
		Target->InitAutoFormat(512, 512);
		Target->UpdateResourceImmediate(true);
		PreviewTarget.Reset(Target);
		PreviewBrush = MakeShared<FSlateBrush>();
		PreviewBrush->SetResourceObject(Target);
		PreviewBrush->ImageSize = FVector2D(512.0, 512.0);
	}
	ASceneCapture2D* Camera = Cast<ASceneCapture2D>(PreviewCamera.Get());
	if (!Camera)
	{
		FActorSpawnParameters Parameters;
		Parameters.ObjectFlags |= RF_Transient;
		Parameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		Camera = LiveWorld->SpawnActor<ASceneCapture2D>(ASceneCapture2D::StaticClass(), FTransform::Identity, Parameters);
		if (!Camera)
		{
			return;
		}
		PreviewCamera = Camera;
	}
	USceneCaptureComponent2D* Capture = Camera->GetCaptureComponent2D();
	Capture->TextureTarget = Cast<UTextureRenderTarget2D>(PreviewTarget.Get());
	Capture->CaptureSource = ESceneCaptureSource::SCS_FinalColorLDR;
	Capture->FOVAngle = 30.0f;
	Capture->bCaptureOnMovement = false;
	Capture->PrimitiveRenderMode = ESceneCapturePrimitiveRenderMode::PRM_UseShowOnlyList;
	// The body and what hangs on it (WorldScape root, atmosphere, clouds); not its moons, stations or ships.
	Capture->ShowOnlyActors.Reset();
	Capture->ShowOnlyActors.Add(Body);
	TArray<AActor*> Attached;
	Body->GetAttachedActors(Attached, true, true);
	// The surface and the air need not hang on the body: its environment generator owns them.
	if (const APlanetarySurfaceGenerator* Environment = Body->PlanetaryEnvironmentGenerator)
	{
		Attached.Add(Environment->WorldScapeRootInstance);
		Attached.Add(Environment->PlanetAtmosphere);
	}
	for (AActor* Each : Attached)
	{
		// Not its orbit line, moons, stations or ships: the world itself.
		if (IsValid(Each) && !Each->IsA<APlanetaryBody>() && !Each->IsA<ATechActor>() && !Each->IsA<ASpaceship>()
			&& !Each->GetClass()->GetName().Contains(TEXT("Orbit")))
		{
			Capture->ShowOnlyActors.AddUnique(Each);
		}
	}
	// From its star's side, a little off the line so the terminator shows; the disc fills most of the frame.
	FVector ToStar = FVector::UpVector;
	double Nearest = TNumericLimits<double>::Max();
	for (TActorIterator<AStar> It(LiveWorld); It; ++It)
	{
		const double Distance = IsValid(*It) ? FVector::DistSquared(It->GetActorLocation(), Body->GetActorLocation())
			: TNumericLimits<double>::Max();
		if (Distance < Nearest)
		{
			Nearest = Distance;
			ToStar = (It->GetActorLocation() - Body->GetActorLocation()).GetSafeNormal();
		}
	}
	FVector Side = FVector::CrossProduct(ToStar, FVector::UpVector).GetSafeNormal();
	Side = Side.IsNearlyZero() ? FVector::ForwardVector : Side;
	const FVector View = (ToStar + Side * 0.5).GetSafeNormal();
	const double Radius = FMath::Max(Body->GetWorldScapeBodyRadiusCm(), 100000.0);
	const double Distance = Radius * 1.25 / FMath::Tan(FMath::DegreesToRadians(Capture->FOVAngle * 0.5));
	Camera->SetActorLocationAndRotation(Body->GetActorLocation() + View * Distance, (-View).Rotation(), false, nullptr,
		ETeleportType::TeleportPhysics);
	// A few frames so the exposure settles, then the picture stays (Tick turns the capture off).
	Capture->bCaptureEveryFrame = true;
	PreviewSubject = Body;
	PreviewFramesLeft = 12;
	UE_LOG(LogTemp, Log, TEXT("[APS.Colony.Terminal] preview of %s: %d actors, %.0f km away"), *Body->GetName(),
		Capture->ShowOnlyActors.Num(), Distance / 100000.0);
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
					FText::AsNumber(Index < 0 ? Entries.Num() : Counts.FindRef(Category)))),
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
			.BorderBackgroundColor(bRecent ? FLinearColor(0.03f, 0.12f, 0.15f, 0.94f) : FLinearColor(0.02f, 0.05f, 0.07f, 0.74f))
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
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().AutoWidth()
						[
							SNew(STextBlock).Text(Style.Label).Font(Font("Bold", 10)).ColorAndOpacity(Style.Colour)
						]
						+ SHorizontalBox::Slot().AutoWidth().Padding(12.0f, 0.0f, 0.0f, 0.0f)
						[
							SNew(STextBlock).Text(Time).Font(Font("Regular", 10)).ColorAndOpacity(Muted())
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
				FText::AsNumber(MaxShown))).Font(Font("Regular", 11)).ColorAndOpacity(Muted())
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
	// The body preview: a few frames of capture, then the picture is kept (B1).
	if (PreviewFramesLeft > 0 && --PreviewFramesLeft == 0)
	{
		if (ASceneCapture2D* Camera = Cast<ASceneCapture2D>(PreviewCamera.Get()))
		{
			Camera->GetCaptureComponent2D()->bCaptureEveryFrame = false;
		}
	}
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
	if (ActiveTab != ETab::Colony && ActiveTab != ETab::Map && ActiveTab != ETab::Fleet && ActiveTab != ETab::Shipyard)
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
	// Civil affairs speeds the colony's works up by 15% a level (APSColonyConstructionSubsystem registers them so).
	const UMainGameplayInstance* State = GameplayState(World.Get());
	const UCivilization* Civ = State ? State->CurrentCivilization.Get() : nullptr;
	const double CivilFactor = 1.0 / (1.0 + 0.15 * FMath::Max(Civ ? Civ->Divisions.CivilAffairs : 0, 0));
	const FText Metrics = FText::Format(LOCTEXT("ModuleMetrics", "{0} x {1} M  /  {2} S"),
		FText::AsNumber(FMath::RoundToInt(Spec.SizeCm.X / 100.0)),
		FText::AsNumber(FMath::RoundToInt(Spec.SizeCm.Y / 100.0)),
		FText::AsNumber(FMath::RoundToInt(Spec.BuildSeconds * CivilFactor)));
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
		Brush->SetResourceObject(Texture);
		Brush->ImageSize = FVector2D(static_cast<float>(Texture->GetSizeX()), static_cast<float>(Texture->GetSizeY()));
		Brush->DrawAs = ESlateBrushDrawType::Image;
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
								FText::AsNumber(Fleet->CountQueued(Yard)), FText::AsNumber(FAPSFleetCommand::ShipyardQueueLimit),
								FText::AsNumber(Fleet->GetLaunchedCount(Yard)));
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
					FText::AsNumber(Count))),
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
				+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
				[
					SNew(SBox).WidthOverride(150.0f).HeightOverride(84.0f)
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
						EnumText(Option.SizeClass).ToUpper(), FText::AsNumber(FMath::RoundToInt(Option.BuildSeconds)),
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
							SNew(SHorizontalBox)
							+ SHorizontalBox::Slot().FillWidth(1.0f)
							[
								SNew(STextBlock).Text(FAPSFleetCommand::DisplayName(Each)).Font(Font("Bold", 11)).ColorAndOpacity(White())
							]
							+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(8.0f, 0.0f, 0.0f, 0.0f)
							[
								SNew(STextBlock).Text(bHome ? LOCTEXT("YardHome", "HOME") : LOCTEXT("YardBuilt", "BUILT"))
								.Font(Font("Bold", 9)).ColorAndOpacity(bHome ? Amber() : Success())
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
											FText::AsNumber(FMath::RoundToInt(Job.Progress * 100.0f)),
											FText::AsNumber(Live->CountQueued(Target)), FText::AsNumber(FAPSFleetCommand::ShipyardQueueLimit));
									}
								}
								return FText::Format(LOCTEXT("YardIdle", "SLIPWAY FREE  /  LAUNCHED {0}"),
									FText::AsNumber(Live->GetLaunchedCount(Target)));
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
							FText::AsNumber(FMath::RoundToInt(Progress() * 100.0f)))
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
