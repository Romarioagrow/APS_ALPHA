#include "SAPSCivilizationOverview.h"

#include "APSDashboardKit.h"
#include "APS_ALPHA/Actors/Astro/Planet.h"
#include "APS_ALPHA/Actors/Astro/PlanetaryBody.h"
#include "APS_ALPHA/Actors/Astro/Star.h"
#include "APS_ALPHA/Core/Enums/CharSpawnPlace.h"
#include "APS_ALPHA/Core/Enums/PlanetType.h"
#include "APS_ALPHA/Core/Enums/StarSpectralClass.h"
#include "APS_ALPHA/Core/Instances/MainGameplayInstance.h"
#include "APS_ALPHA/Core/Model/SpawnParameters.h"
#include "APS_ALPHA/Gameplay/Civilizations/Civilization.h"
#include "APS_ALPHA/Gameplay/Fleet/APSFleetCommand.h"
#include "APS_ALPHA/Generation/AstroGenerator.h"
#include "APS_ALPHA/UI/Style/APSMenuChrome.h"
#include "APS_ALPHA/UI/Style/APSUINumber.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "EngineUtils.h"
#include "Styling/AppStyle.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SLeafWidget.h"
#include "Widgets/SNullWidget.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "APSCivilizationOverview"

static_assert(static_cast<int32>(APSFleet::EDivision::Count) == 4, "SAPSCivilizationOverview::DivisionCount");

namespace APSCivilizationOverviewPrivate
{
	using namespace APSChrome;
	using namespace APSDashboard;

	template <typename TEnum>
	FText EnumText(const TEnum Value)
	{
		const UEnum* Enum = StaticEnum<TEnum>();
		return Enum ? Enum->GetDisplayNameTextByValue(static_cast<int64>(Value)).ToUpper() : FText::GetEmpty();
	}

	const UMainGameplayInstance* GameplayState(const UWorld* World)
	{
		const UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
		return GameInstance ? GameInstance->GetSubsystem<UMainGameplayInstance>() : nullptr;
	}

	const UCivilization* CurrentCivilization(const UWorld* World)
	{
		const UMainGameplayInstance* Gameplay = GameplayState(World);
		return Gameplay ? Gameplay->CurrentCivilization.Get() : nullptr;
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

	FText BodyName(const ACelestialBody* Body)
	{
		return Body && !Body->AstroName.IsNone() ? FText::FromName(Body->AstroName) : LOCTEXT("UnknownBody", "UNKNOWN");
	}

	/** One trait of the civilization in the identity line: the caption over the value. */
	TSharedRef<SWidget> Trait(const FText& Label, const FText& Value)
	{
		return SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()
			[
				Caption(Label)
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 4.0f, 0.0f, 0.0f)
			[
				SNew(STextBlock).Text(Value).Font(Font("Bold", 13)).ColorAndOpacity(Cyan())
			];
	}
}

int32 SAPSCivilizationOverview::FLiveState::ShipCount() const
{
	int32 Count = 0;
	for (const int32 DivisionShips : Ships)
	{
		Count += DivisionShips;
	}
	return Count;
}

int32 SAPSCivilizationOverview::FLiveState::InfrastructureCount() const
{
	return Infrastructure[0] + Infrastructure[1] + Infrastructure[2] + Infrastructure[3];
}

void SAPSCivilizationOverview::Construct(const FArguments& InArgs)
{
	World = InArgs._World;
	OnOpenTab = InArgs._OnOpenTab;
	ReadState();

	ChildSlot
	[
		SNew(SScrollBox)
		+ SScrollBox::Slot().Padding(0.0f, 0.0f, 0.0f, 20.0f)
		[
			BuildHero()
		]
		+ SScrollBox::Slot().Padding(0.0f, 0.0f, 0.0f, 16.0f)
		[
			BuildHeadlines()
		]
		+ SScrollBox::Slot()
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(1.15f).Padding(0.0f, 0.0f, 8.0f, 0.0f)
			[
				BuildHome()
			]
			+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(8.0f, 0.0f)
			[
				BuildExploration()
			]
			+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(8.0f, 0.0f, 0.0f, 0.0f)
			[
				BuildInfrastructure()
			]
		]
	];
}

void SAPSCivilizationOverview::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
	SCompoundWidget::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);
	// The terminal's switcher ticks only the shown tab, so this reads nothing while another tab is open.
	ReadAccumulator += InDeltaTime;
	if (ReadAccumulator >= 0.5f)
	{
		ReadAccumulator = 0.0f;
		ReadState();
	}
}

void SAPSCivilizationOverview::ReadState()
{
	using namespace APSCivilizationOverviewPrivate;
	FLiveState Next;
	UWorld* LiveWorld = World.Get();
	const UCivilization* Civilization = CurrentCivilization(LiveWorld);
	if (Civilization)
	{
		Next.bCivilization = true;
		Next.Population = Civilization->Population;
		Next.Credits = Civilization->Credits;
		Next.TechLevel = Civilization->TechnologyLevel;
		Next.Infrastructure[0] = Civilization->Infrastructure.OrbitalStations;
		Next.Infrastructure[1] = Civilization->Infrastructure.GroundSettlements;
		// Live: construction ships add to it (fleet command).
		Next.Infrastructure[2] = Civilization->Infrastructure.PlanetOutposts;
		Next.Infrastructure[3] = Civilization->Infrastructure.StarOutposts;
	}
	const FAPSFleetCommand* Fleet = APSFleetFind(LiveWorld);
	if (Fleet)
	{
		for (const FAPSFleetUnit& Unit : Fleet->GetUnits())
		{
			if (!Unit.Ship.IsValid())
			{
				continue;
			}
			const int32 Division = FMath::Clamp(static_cast<int32>(Unit.Division), 0, DivisionCount - 1);
			++Next.Ships[Division];
			Next.Busy[Division] += Unit.Order != APSFleet::EOrder::None ? 1 : 0;
		}
	}
	else if (Civilization)
	{
		// No fleet command yet: the founding fleet as the main fleet.
		Next.Ships[static_cast<int32>(APSFleet::EDivision::MainFleet)] = Civilization->FleetSize;
	}
	if (LiveWorld)
	{
		// How much of the system the civilization knows (surveys by fleet command).
		for (TActorIterator<APlanetaryBody> It(LiveWorld); It; ++It)
		{
			++Next.Worlds;
			const APSFleet::ESurvey Survey = Fleet ? Fleet->GetSurvey(*It) : APSFleet::ESurvey::Unknown;
			Next.Surveyed += Survey == APSFleet::ESurvey::Surveyed ? 1 : 0;
			Next.Studied += Survey == APSFleet::ESurvey::Studied ? 1 : 0;
		}
	}
	State = Next;
}

FReply SAPSCivilizationOverview::OpenTab(const int32 Tab)
{
	OnOpenTab.ExecuteIfBound(Tab);
	return FReply::Handled();
}

TSharedRef<SWidget> SAPSCivilizationOverview::BuildHero()
{
	using namespace APSCivilizationOverviewPrivate;
	UWorld* LiveWorld = World.Get();
	const UCivilization* Civilization = CurrentCivilization(LiveWorld);
	const APlanet* Planet = HomePlanet(LiveWorld);
	const AStar* Star = Planet ? Planet->ParentStar : nullptr;
	const FText Name = FText::FromString(Civilization && !Civilization->Name.IsEmpty()
		? Civilization->Name.ToUpper() : FString(TEXT("CIVILIZATION")));
	const FText Line = Planet && Star
		? FText::Format(LOCTEXT("HeroLine", "Home world {0} in the {1} system. The founding parameters of this world:"),
			BodyName(Planet), BodyName(Star))
		: LOCTEXT("HeroLineNoHome", "The founding parameters of this world:");

	// The identity line: four traits side by side, split by thin rules.
	const TSharedRef<SHorizontalBox> Traits = SNew(SHorizontalBox);
	const auto AddTrait = [&Traits](const FText& Label, const FText& Value)
	{
		if (Traits->NumSlots() > 0)
		{
			Traits->AddSlot().AutoWidth().VAlign(VAlign_Center).Padding(22.0f, 0.0f)[VerticalRule(34.0f)];
		}
		Traits->AddSlot().AutoWidth().VAlign(VAlign_Center)[Trait(Label, Value)];
	};
	if (Civilization)
	{
		AddTrait(LOCTEXT("Archetype", "ARCHETYPE"), EnumText(Civilization->Archetype));
		AddTrait(LOCTEXT("Government", "GOVERNMENT"), EnumText(Civilization->Government));
		AddTrait(LOCTEXT("Economy", "ECONOMY"), EnumText(Civilization->Economy));
		AddTrait(LOCTEXT("Society", "SOCIETY"), EnumText(Civilization->Society));
	}
	else
	{
		AddTrait(LOCTEXT("NoCivilization", "CIVILIZATION"), LOCTEXT("NoCivilizationValue", "NOT GENERATED"));
	}

	return SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight()
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
			[
				IconBadge(EAPSChromeGlyph::Civilization, Cyan(), 60.0f)
			]
			+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center).Padding(18.0f, 0.0f, 0.0f, 0.0f)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(STextBlock).Text(Name).Font(Font("Bold", 26)).ColorAndOpacity(White())
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 5.0f, 0.0f, 0.0f)
				[
					SNew(STextBlock).Text(Line).Font(Font("Regular", 12)).ColorAndOpacity(Muted())
				]
			]
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(78.0f, 14.0f, 0.0f, 0.0f)
		[
			Traits
		];
}

TSharedRef<SWidget> SAPSCivilizationOverview::BuildHeadlines()
{
	using namespace APSCivilizationOverviewPrivate;
	// Segments at most 16 wide: ten levels or a dozen ships make a short gauge, not a bar across the card.
	const TSharedRef<SWidget> TechGauge =
			SNew(SAPSDashboardSegments).Height(8.0f).MaxSegment(16.0f)
			.Colours([this](TArray<FLinearColor>& Out)
			{
				// Levels 1..10 (UCivilization clamps them): reached amber, the rest a dim track.
				for (int32 Level = 1; Level <= 10; ++Level)
				{
					Out.Add(Level <= State.TechLevel ? Amber() : CyanDim().CopyWithNewOpacity(0.45f));
				}
			});
	const TSharedRef<SWidget> FleetGauge =
			SNew(SAPSDashboardSegments).Height(8.0f).MaxSegment(16.0f)
			.Colours([this](TArray<FLinearColor>& Out)
			{
				// One segment per ship, grouped and coloured by division as on the divisions tab.
				for (int32 Division = 0; Division < DivisionCount; ++Division)
				{
					const FLinearColor Colour = APSFleet::DivisionColour(static_cast<APSFleet::EDivision>(Division));
					for (int32 Ship = 0; Ship < State.Ships[Division]; ++Ship)
					{
						Out.Add(Colour);
					}
				}
			});

	return SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(0.0f, 0.0f, 8.0f, 0.0f)
		[
			Headline(EAPSChromeGlyph::Profile, Cyan(), LOCTEXT("Population", "POPULATION"),
				TAttribute<FText>::CreateLambda([this]() { return APSUINumber::Number(State.Population); }),
				LOCTEXT("PopulationUnit", "colonists"), SNullWidget::NullWidget,
				LOCTEXT("PopulationHint", "The civilization's people."))
		]
		+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(8.0f, 0.0f)
		[
			Headline(EAPSChromeGlyph::Collection, Amber(), LOCTEXT("Credits", "CREDITS"),
				TAttribute<FText>::CreateLambda([this]() { return APSUINumber::Number(State.Credits); }),
				LOCTEXT("CreditsUnit", "CR"), SNullWidget::NullWidget,
				LOCTEXT("CreditsHint", "The civilization's treasury."))
		]
		+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(8.0f, 0.0f)
		[
			Headline(EAPSChromeGlyph::Settings, Amber(), LOCTEXT("Tech", "TECH LEVEL"),
				TAttribute<FText>::CreateLambda([this]() { return APSUINumber::Number(State.TechLevel); }),
				LOCTEXT("TechUnit", "of 10"), TechGauge,
				LOCTEXT("TechHint", "The civilization's technology, level 1 to 10."))
		]
		+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(8.0f, 0.0f, 0.0f, 0.0f)
		[
			Headline(EAPSChromeGlyph::Fleet, Cyan(), LOCTEXT("Fleet", "FLEET"),
				TAttribute<FText>::CreateLambda([this]() { return APSUINumber::Number(State.ShipCount()); }),
				TAttribute<FText>::CreateLambda([this]()
				{
					return State.ShipCount() == 1 ? LOCTEXT("FleetUnitOne", "ship") : LOCTEXT("FleetUnit", "ships");
				}),
				FleetGauge,
				LOCTEXT("FleetHint", "Every ship of the civilization, coloured by division: main fleet, exploration, science, construction."))
		];
}

TSharedRef<SWidget> SAPSCivilizationOverview::BuildHome()
{
	using namespace APSCivilizationOverviewPrivate;
	UWorld* LiveWorld = World.Get();
	const APlanet* Planet = HomePlanet(LiveWorld);
	const AStar* Star = Planet ? Planet->ParentStar : nullptr;
	const UMainGameplayInstance* Gameplay = GameplayState(LiveWorld);
	const USpawnParameters* Spawn = Gameplay ? Gameplay->SpawnParameters : nullptr;
	const FLinearColor StarTint = StarColour(Star);
	const FLinearColor PlanetTint = BodyColour(Planet);
	const int32 Moons = Planet ? Planet->Moons.Num() : 0;

	FText StarDetail = LOCTEXT("StarDetailUnknown", "Not charted yet");
	if (Star)
	{
		const FText Class = !Star->FullSpectralClass.IsNone() ? FText::FromName(Star->FullSpectralClass)
			: EnumText(Star->SpectralClass);
		StarDetail = Star->SurfaceTemperature > 0
			? FText::Format(LOCTEXT("StarDetail", "Class {0}  ·  {1} K"), Class, APSUINumber::Number(Star->SurfaceTemperature))
			: FText::Format(LOCTEXT("StarDetailClass", "Class {0}"), Class);
	}
	FText PlanetDetail = LOCTEXT("PlanetDetailUnknown", "Not charted yet");
	if (Planet)
	{
		FFormatNamedArguments Arguments;
		Arguments.Add(TEXT("Type"), EnumText(Planet->PlanetType));
		Arguments.Add(TEXT("Radius"), APSUINumber::Number(Planet->PlanetRadiusKM));
		Arguments.Add(TEXT("Moons"), Moons);
		PlanetDetail = FText::Format(LOCTEXT("PlanetDetail",
			"{Type}  ·  radius {Radius} km  ·  {Moons} {Moons}|plural(one=moon,other=moons)"), Arguments);
	}

	const TSharedRef<SWidget> Body =
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
		[
			SNew(SBox).WidthOverride(170.0f).HeightOverride(150.0f)
			[
				SNew(SAPSDashboardOrbit).StarColour(StarTint).BodyColour(PlanetTint).Moons(Moons).HasBody(Planet != nullptr)
			]
		]
		+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center).Padding(18.0f, 0.0f, 0.0f, 0.0f)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()
			[
				Caption(LOCTEXT("HomeStar", "HOME STAR"))
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 3.0f, 0.0f, 0.0f)
			[
				SNew(STextBlock).Text(BodyName(Star)).Font(Font("Bold", 17)).ColorAndOpacity(StarTint)
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 2.0f, 0.0f, 0.0f)
			[
				SNew(STextBlock).Text(StarDetail).Font(Font("Regular", 12)).ColorAndOpacity(Muted())
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 14.0f, 0.0f, 0.0f)
			[
				Caption(LOCTEXT("HomePlanet", "HOME PLANET"))
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 3.0f, 0.0f, 0.0f)
			[
				SNew(STextBlock).Text(BodyName(Planet)).Font(Font("Bold", 17)).ColorAndOpacity(PlanetTint)
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 2.0f, 0.0f, 0.0f)
			[
				SNew(STextBlock).Text(PlanetDetail).Font(Font("Regular", 12)).ColorAndOpacity(Muted()).AutoWrapText(true)
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 14.0f, 0.0f, 0.0f)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
				[
					Caption(LOCTEXT("Start", "START"))
				]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(10.0f, 0.0f, 0.0f, 0.0f)
				[
					SNew(STextBlock)
					.Text(Spawn ? EnumText(Spawn->CharacterSpawnPlace) : LOCTEXT("StartUnknown", "UNKNOWN"))
					.Font(Font("Bold", 11)).ColorAndOpacity(Amber())
				]
			]
		];

	return Card(EAPSChromeGlyph::System, LOCTEXT("HomeTitle", "HOME SYSTEM"),
		LOCTEXT("HomeSubtitle", "Where the civilization began"), Body,
		LinkButton(LOCTEXT("OpenMap", "SYSTEM MAP"),
			FOnClicked::CreateSP(this, &SAPSCivilizationOverview::OpenTab, 1)));
}

TSharedRef<SWidget> SAPSCivilizationOverview::BuildExploration()
{
	using namespace APSCivilizationOverviewPrivate;
	const FLinearColor Unknown = CyanDim().CopyWithNewOpacity(0.45f);

	const TSharedRef<SVerticalBox> Divisions = SNew(SVerticalBox);
	for (int32 Division = 0; Division < DivisionCount; ++Division)
	{
		const APSFleet::EDivision Kind = static_cast<APSFleet::EDivision>(Division);
		const FLinearColor Colour = APSFleet::DivisionColour(Kind);
		Divisions->AddSlot().AutoHeight().Padding(0.0f, Division == 0 ? 0.0f : 7.0f, 0.0f, 0.0f)
		[
			SNew(SHorizontalBox).ToolTipText(APSFleet::DivisionRole(Kind))
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
			[
				SNew(SBox).WidthOverride(10.0f).HeightOverride(10.0f)
				[
					SNew(SBorder).BorderImage(FAppStyle::GetBrush("WhiteBrush")).BorderBackgroundColor(Colour)
				]
			]
			+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center).Padding(10.0f, 0.0f, 0.0f, 0.0f)
			[
				SNew(STextBlock).Text(APSFleet::DivisionName(Kind)).Font(Font("Bold", 10)).ColorAndOpacity(White())
			]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
			[
				SNew(STextBlock).Font(Font("Bold", 13)).ColorAndOpacity(Colour)
				.Text_Lambda([this, Division]() { return APSUINumber::Number(State.Ships[Division]); })
			]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(10.0f, 0.0f, 0.0f, 0.0f)
			[
				SNew(SBox).WidthOverride(92.0f)
				[
					SNew(STextBlock).Font(Font("Regular", 11)).ColorAndOpacity(Muted())
					.Text_Lambda([this, Division]()
					{
						return State.Busy[Division] > 0
							? FText::Format(LOCTEXT("DivisionBusy", "{0} on orders"), State.Busy[Division])
							: State.Ships[Division] > 0 ? LOCTEXT("DivisionIdle", "standing by") : FText::GetEmpty();
					})
				]
			]
		];
	}

	const TSharedRef<SWidget> Body =
		SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight()
		[
			BigValue(TAttribute<FText>::CreateLambda([this]() { return APSUINumber::Number(State.Surveyed + State.Studied); }),
				TAttribute<FText>::CreateLambda([this]()
				{
					return FText::Format(LOCTEXT("WorldsKnown", "of {0} worlds known"), APSUINumber::Number(State.Worlds));
				}),
				Amber(), 26)
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 10.0f, 0.0f, 0.0f)
		[
			SNew(SAPSDashboardSegments).Height(12.0f).MaxSegment(40.0f)
			.Colours([this, Unknown](TArray<FLinearColor>& Out)
			{
				// Studied first, then surveyed, then the worlds still unknown.
				for (int32 Index = 0; Index < State.Worlds; ++Index)
				{
					Out.Add(Index < State.Studied ? Success() : Index < State.Studied + State.Surveyed ? Cyan() : Unknown);
				}
			})
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 9.0f, 0.0f, 0.0f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().Padding(0.0f, 0.0f, 18.0f, 0.0f)
			[
				LegendItem(Success(), LOCTEXT("Studied", "STUDIED"),
					TAttribute<FText>::CreateLambda([this]() { return APSUINumber::Number(State.Studied); }))
			]
			+ SHorizontalBox::Slot().AutoWidth().Padding(0.0f, 0.0f, 18.0f, 0.0f)
			[
				LegendItem(Cyan(), LOCTEXT("Surveyed", "SURVEYED"),
					TAttribute<FText>::CreateLambda([this]() { return APSUINumber::Number(State.Surveyed); }))
			]
			+ SHorizontalBox::Slot().AutoWidth()
			[
				LegendItem(Unknown, LOCTEXT("UnknownWorlds", "UNKNOWN"),
					TAttribute<FText>::CreateLambda([this]()
					{
						return APSUINumber::Number(FMath::Max(0, State.Worlds - State.Surveyed - State.Studied));
					}))
			]
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 20.0f, 0.0f, 8.0f)
		[
			Caption(LOCTEXT("FleetAtWork", "SHIPS BY DIVISION"))
		]
		+ SVerticalBox::Slot().AutoHeight()
		[
			Divisions
		];

	return Card(EAPSChromeGlyph::Compass, LOCTEXT("ExplorationTitle", "EXPLORATION"),
		LOCTEXT("ExplorationSubtitle", "The system's worlds and the ships at work"), Body,
		LinkButton(LOCTEXT("OpenFleet", "FLEET ORDERS"),
			FOnClicked::CreateSP(this, &SAPSCivilizationOverview::OpenTab, 3)));
}

TSharedRef<SWidget> SAPSCivilizationOverview::BuildInfrastructure()
{
	using namespace APSCivilizationOverviewPrivate;
	struct FRow
	{
		EAPSChromeGlyph Glyph;
		FText Label;
		FText Tooltip;
	};
	const FRow Rows[] = {
		{EAPSChromeGlyph::Station, LOCTEXT("OrbitalStations", "ORBITAL STATIONS"),
			LOCTEXT("OrbitalStationsHint", "Stations in orbit founded with the civilization.")},
		{EAPSChromeGlyph::Headquarters, LOCTEXT("GroundSettlements", "GROUND SETTLEMENTS"),
			LOCTEXT("GroundSettlementsHint", "Settlements on the ground founded with the civilization.")},
		{EAPSChromeGlyph::Planet, LOCTEXT("PlanetOutposts", "PLANET OUTPOSTS"),
			LOCTEXT("PlanetOutpostsHint", "Outposts on planets and moons; construction ships add more.")},
		{EAPSChromeGlyph::Favorite, LOCTEXT("StarOutposts", "STAR OUTPOSTS"),
			LOCTEXT("StarOutpostsHint", "Outposts near stars founded with the civilization.")}};

	const TSharedRef<SVerticalBox> List = SNew(SVerticalBox);
	for (int32 Index = 0; Index < static_cast<int32>(UE_ARRAY_COUNT(Rows)); ++Index)
	{
		List->AddSlot().AutoHeight().Padding(0.0f, Index == 0 ? 0.0f : 12.0f, 0.0f, 0.0f)
		[
			SNew(SHorizontalBox).ToolTipText(Rows[Index].Tooltip)
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
			[
				SNew(SBox).WidthOverride(26.0f).HeightOverride(26.0f)
				[
					SNew(SAPSVectorGlyph).Glyph(Rows[Index].Glyph).Color(Cyan()).StrokeWidth(1.5f)
				]
			]
			+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center).Padding(12.0f, 0.0f, 0.0f, 0.0f)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center)
					[
						SNew(STextBlock).Text(Rows[Index].Label).Font(Font("Bold", 10)).ColorAndOpacity(White())
					]
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
					[
						SNew(STextBlock).Font(Font("Bold", 15)).ColorAndOpacity(Cyan())
						.Text_Lambda([this, Index]() { return APSUINumber::Number(State.Infrastructure[Index]); })
					]
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 5.0f, 0.0f, 0.0f)
				[
					// One pip per object, so the four counts compare at a glance.
					SNew(SAPSDashboardSegments).Height(5.0f).MaxSegment(12.0f)
					.Colours([this, Index](TArray<FLinearColor>& Out)
					{
						Out.Init(Cyan().CopyWithNewOpacity(0.85f), FMath::Clamp(State.Infrastructure[Index], 0, 60));
					})
				]
			]
		];
	}

	const TSharedRef<SWidget> Body =
		SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight()
		[
			BigValue(TAttribute<FText>::CreateLambda([this]() { return APSUINumber::Number(State.InfrastructureCount()); }),
				LOCTEXT("InfrastructureUnit", "objects in the network"), Cyan(), 26)
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 16.0f, 0.0f, 0.0f)
		[
			List
		];

	return Card(EAPSChromeGlyph::Infrastructure, LOCTEXT("InfrastructureTitle", "INFRASTRUCTURE"),
		LOCTEXT("InfrastructureSubtitle", "Stations, settlements and outposts"), Body,
		LinkButton(LOCTEXT("OpenInfrastructure", "INFRASTRUCTURE"),
			FOnClicked::CreateSP(this, &SAPSCivilizationOverview::OpenTab, 2)));
}

#undef LOCTEXT_NAMESPACE
