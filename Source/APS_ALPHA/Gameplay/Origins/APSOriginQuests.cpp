#include "APSOriginQuests.h"

#include "APS_ALPHA/Actors/Astro/Planet.h"
#include "APS_ALPHA/Actors/Astro/PlanetaryBody.h"
#include "APS_ALPHA/Actors/Astro/Star.h"
#include "APS_ALPHA/Actors/Tech/Colony.h"
#include "APS_ALPHA/Gameplay/Civilizations/APSCivilizationJournalSubsystem.h"
#include "APS_ALPHA/Gameplay/Colony/APSColonyConstructionSubsystem.h"
#include "APS_ALPHA/Gameplay/Expansion/APSInfrastructure.h"
#include "APS_ALPHA/Gameplay/Expansion/APSMissions.h"
#include "APS_ALPHA/Gameplay/Fleet/APSFleetCommand.h"
#include "APS_ALPHA/Gameplay/Origins/APSOrigins.h"
#include "APS_ALPHA/Generation/AstroGenerator.h"
#include "APS_ALPHA/Pawns/Spaceships/Spaceship.h"
#include "APS_ALPHA/UI/Colony/APSColonyTerminalSubsystem.h"
#include "APS_ALPHA/UI/Colony/APSMissionTracker.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"

#define LOCTEXT_NAMESPACE "APSOriginQuests"

namespace APSOriginQuestsLocal
{
	using namespace APSOriginQuests;

	TAutoConsoleVariable<int32> CVarStory(TEXT("aps.Origin.Story"), 1,
		TEXT("1: the origin's story chain runs in a ladder world (journal lines, HUD notices, the ORIGIN page). 0: off."));
	TAutoConsoleVariable<float> CVarCheckSeconds(TEXT("aps.Origin.StoryCheckSeconds"), 1.0f,
		TEXT("How often the current step of the origin's story is checked, seconds."));

	const FName JournalCategory(TEXT("Origin"));

	struct FState
	{
		TWeakObjectPtr<UWorld> World;
		float Clock{0.0f};
		/** The step whose story was told this session (a load tells the current one again, once). */
		int32 Announced{-1};
	};

	TArray<FState>& States()
	{
		static TArray<FState> Entries;
		return Entries;
	}

	FState& StateFor(UWorld* World)
	{
		TArray<FState>& Entries = States();
		for (FState& Entry : Entries)
		{
			if (Entry.World.Get() == World)
			{
				return Entry;
			}
		}
		Entries.RemoveAll([](const FState& Entry) { return !Entry.World.IsValid(); });
		FState& Added = Entries.AddDefaulted_GetRef();
		Added.World = World;
		return Added;
	}

	FStep Make(const TCHAR* Id, const FText& Title, const FText& Story, const FText& Objective, const FText& Done,
		const ETrigger Trigger, const FName Param, const float Number, const uint8 Rung, const FText& Opens = FText::GetEmpty())
	{
		FStep Step;
		Step.Id = Id;
		Step.Title = Title;
		Step.Story = Story;
		Step.Objective = Objective;
		Step.Done = Done;
		Step.Opens = Opens;
		Step.Trigger = Trigger;
		Step.Param = Param;
		Step.Number = Number;
		Step.Rung = Rung;
		return Step;
	}

	// The steps every origin shares once the colony stands on the ground (concept section 4.1-4.3).
	void AddGroundSteps(TArray<FStep>& Steps, const bool bWithYard)
	{
		Steps.Add(Make(TEXT("Light"), LOCTEXT("LightTitle", "LIGHT"),
			LOCTEXT("LightStory", "Stores: energy only, and the Ark's cells are flat. First task: a solar array."),
			LOCTEXT("LightObjective", "Build mode (B): raise a SOLAR ARRAY beside the base."),
			LOCTEXT("LightDone", "Solar array online. Energy is the only currency we have."),
			ETrigger::ModuleBuilt, FName(TEXT("SolarArray")), 0.0f, 0));
		Steps.Add(Make(TEXT("Shelter"), LOCTEXT("ShelterTitle", "SHELTER"),
			LOCTEXT("ShelterStory", "The hold is cold and the ramp is open. Somewhere to sleep that is not the hold."),
			LOCTEXT("ShelterObjective", "Build mode (B): a HABITAT MODULE."),
			LOCTEXT("ShelterDone", "Habitat sealed. A place to sleep that is not the hold."),
			ETrigger::ModuleBuilt, FName(TEXT("Habitat")), 0.0f, 0));
		Steps.Add(Make(TEXT("Ears"), LOCTEXT("EarsTitle", "EARS"),
			LOCTEXT("EarsStory", "The deep radar needs a mast. Whatever the ground hides, it will draw."),
			LOCTEXT("EarsObjective", "Build mode (B): a COMMS MAST."),
			LOCTEXT("EarsDone", "Comms mast raised. The deep radar wakes."),
			ETrigger::ModuleBuilt, FName(TEXT("CommsMast")), 0.0f, 0));
		Steps.Add(Make(TEXT("Signal"), LOCTEXT("SignalTitle", "THE SIGNAL"),
			LOCTEXT("SignalStory", "The mast listens. The first pass of the radar takes a little while."),
			LOCTEXT("SignalObjective", "Wait for the deep radar: it draws what the ground hides (the JOURNAL tells it)."),
			LOCTEXT("SignalDone", "SIGNAL: straight edges on the radar, too regular for rock. A shape on the horizon."),
			ETrigger::AncientsChain, FName(TEXT("Echoes")), 0.0f, 1));
		Steps.Add(Make(TEXT("Wheels"), LOCTEXT("WheelsTitle", "WHEELS"),
			LOCTEXT("WheelsStory", "Whatever it is, it is far. A fabrication bay prints a rover; we stop walking today."),
			LOCTEXT("WheelsObjective", "Build mode (B): a FABRICATION BAY. The rover rolls out when it stands."),
			LOCTEXT("WheelsDone", "Fabricator running. First job: a rover. We stop walking today."),
			ETrigger::ModuleBuilt, FName(TEXT("Fab")), 0.0f, 1, LOCTEXT("WheelsOpens", "THE ROVER")));
		Steps.Add(Make(TEXT("Shape"), LOCTEXT("ShapeTitle", "A SHAPE ON THE HORIZON"),
			LOCTEXT("ShapeStory", "Take the rover. It is visible from the ridge, and at night the top of it glows."),
			LOCTEXT("ShapeObjective", "Drive to the structure and study it to its foot: the ECHOES chain in the JOURNAL."),
			LOCTEXT("ShapeDone", "What the Builders knew of lifting stone to orbit is ours now. The fabricator can build a launch yard."),
			ETrigger::Token, APSProgressionTokens::Launch(), 0.0f, 1, LOCTEXT("ShapeOpens", "THE LAUNCH")));
		if (bWithYard)
		{
			Steps.Add(Make(TEXT("Yard"), LOCTEXT("YardTitle", "THE YARD"),
				LOCTEXT("YardStory", "A yard on the ground, beside the pad. The Ark will never fly again; something else will."),
				LOCTEXT("YardObjective", "CONSTRUCTION: raise the LAUNCH YARD (a construction order, or build mode)."),
				LOCTEXT("YardDone", "Launch yard complete. The first hull can be laid down."),
				ETrigger::StructureBuilt, FName(TEXT("LaunchYard")), 0.0f, 2));
			Steps.Add(Make(TEXT("Hull"), LOCTEXT("HullTitle", "THE FIRST HULL"),
				LOCTEXT("HullStory", "The yard asks for metals and energy, then time. Class S: enough for the system."),
				LOCTEXT("HullObjective", "SHIPYARD: order a class S hull at the launch yard and wait for it."),
				LOCTEXT("HullDone", "Hull complete on the pad. Class S. She has no name yet."),
				ETrigger::ShipsLaunched, NAME_None, 1.0f, 2));
			Steps.Add(Make(TEXT("Liftoff"), LOCTEXT("LiftoffTitle", "LIFT-OFF"),
				LOCTEXT("LiftoffStory", "Board her. Below: one ramp, a few modules, a mast. Take her up."),
				LOCTEXT("LiftoffObjective", "Fly the hull sixty kilometres above the colony."),
				LOCTEXT("LiftoffDone", "Orbit. The planet turns under us, and it is ours."),
				ETrigger::InShipAboveKm, NAME_None, 60.0f, 2));
		}
		Steps.Add(Make(TEXT("ColdReturn"), LOCTEXT("ColdTitle", "A COLD RETURN"),
			LOCTEXT("ColdStory", "In orbit the radar reaches further. Something up there is not a moon."),
			LOCTEXT("ColdObjective", "Listen from orbit: the radar draws a hull that is not ours (the JOURNAL tells it)."),
			LOCTEXT("ColdDone", "A COLD RETURN: a hull of the Builders in a high orbit. Dock with it when you are ready."),
			ETrigger::AncientsChain, FName(TEXT("QuietHull")), 0.0f, 2));
		Steps.Add(Make(TEXT("Drive"), LOCTEXT("DriveTitle", "THE STELLAR DRIVE"),
			LOCTEXT("DriveStory", "The quiet hull keeps a drive that crosses the dark between suns. Wake it."),
			LOCTEXT("DriveObjective", "Board the quiet hull and finish its chain (QUIET HULL in the JOURNAL)."),
			LOCTEXT("DriveDone", "The hull's drive core answers ours. The neighbouring suns are within reach."),
			ETrigger::Token, APSProgressionTokens::StellarDrive(), 0.0f, 2, LOCTEXT("DriveOpens", "THE STELLAR DRIVE")));
	}

	TArray<FChain> Build()
	{
		TArray<FChain> Chains;
		{
			FChain& Chain = Chains.AddDefaulted_GetRef();
			Chain.Origin = APSWorldRules::EOrigin::Ark;
			Chain.Key = TEXT("Ark");
			Chain.Name = LOCTEXT("ArkChain", "LANDING");
			Chain.Steps.Add(Make(TEXT("Wake"), LOCTEXT("ArkWakeTitle", "POWER THE ARK"),
				LOCTEXT("ArkWakeStory", "ARK: power 4 %. Drive: dead. Crew: one. One console answers."),
				LOCTEXT("ArkWakeObjective", "Open the console (Tab)."),
				LOCTEXT("ArkWakeDone", "Main bus restored. Lights in the bridge and the hold. The hull is cold but whole."),
				ETrigger::TerminalOpened, NAME_None, 0.0f, 0));
			Chain.Steps.Add(Make(TEXT("Ground"), LOCTEXT("ArkGroundTitle", "FIRST GROUND"),
				LOCTEXT("ArkGroundStory", "Atmosphere breathable. Gravity holds. Nobody has stood on this ground before."),
				LOCTEXT("ArkGroundObjective", "Walk out: forty metres from the Ark."),
				LOCTEXT("ArkGroundDone", "Ramp down. Nobody has stood on this ground before. Now it has a footprint."),
				ETrigger::OnFootAway, NAME_None, 40.0f, 0));
			AddGroundSteps(Chain.Steps, true);
		}
		{
			FChain& Chain = Chains.AddDefaulted_GetRef();
			Chain.Origin = APSWorldRules::EOrigin::Adrift;
			Chain.Key = TEXT("Adrift");
			Chain.Name = LOCTEXT("AdriftChain", "THE LONG NIGHT");
			Chain.Steps.Add(Make(TEXT("Drift"), LOCTEXT("DriftTitle", "DRIFT"),
				LOCTEXT("DriftStory", "Drift. Reactor 11 %. No surface on record. Only the navigation answers."),
				LOCTEXT("DriftObjective", "Open the console (Tab): the system map shows worlds without data."),
				LOCTEXT("DriftDone", "Five worlds on the map and nothing known of any of them. Pick the nearest."),
				ETrigger::TerminalOpened, NAME_None, 0.0f, 0));
			Chain.Steps.Add(Make(TEXT("NotHere"), LOCTEXT("NotHereTitle", "NOT HERE"),
				LOCTEXT("NotHereStory", "Every flight burns the reactor down. Survey the nearest worlds and move on."),
				LOCTEXT("NotHereObjective", "Survey three worlds: fly close, or order SURVEY from FLEET ORDERS."),
				LOCTEXT("NotHereDone", "Rock, ice, no air. Not here. One more on the map is worth the fuel."),
				ETrigger::WorldsSurveyed, NAME_None, 3.0f, 0));
			Chain.Steps.Add(Make(TEXT("Down"), LOCTEXT("DownTitle", "DOWN"),
				LOCTEXT("DownStory", "One world answers: air we can breathe. Land on it and walk out."),
				LOCTEXT("DownObjective", "Land on the living world and step out of the ship."),
				LOCTEXT("DownDone", "Down. The ship is the Ark now."),
				ETrigger::OnHomeGround, NAME_None, 0.0f, 0));
			AddGroundSteps(Chain.Steps, false);
		}
		{
			FChain& Chain = Chains.AddDefaulted_GetRef();
			Chain.Origin = APSWorldRules::EOrigin::UnderTheRing;
			Chain.Key = TEXT("Ring");
			Chain.Name = LOCTEXT("RingChain", "THE RING");
			Chain.Steps.Add(Make(TEXT("Wake"), LOCTEXT("RingWakeTitle", "UNDER THE GIANT"),
				LOCTEXT("RingWakeStory", "Ark down. The giant fills the sky. Over the horizon: a thin arc that is not a cloud."),
				LOCTEXT("RingWakeObjective", "Open the console (Tab)."),
				LOCTEXT("RingWakeDone", "Main bus restored. The giant's light comes in blue through the hold."),
				ETrigger::TerminalOpened, NAME_None, 0.0f, 0));
			Chain.Steps.Add(Make(TEXT("Ground"), LOCTEXT("RingGroundTitle", "FIRST GROUND"),
				LOCTEXT("RingGroundStory", "Thin air, low gravity, and the ring over the horizon. Walk out and look up."),
				LOCTEXT("RingGroundObjective", "Walk out: forty metres from the Ark."),
				LOCTEXT("RingGroundDone", "Ramp down. The arc over the horizon has edges too straight for a ring of ice."),
				ETrigger::OnFootAway, NAME_None, 40.0f, 0));
			AddGroundSteps(Chain.Steps, true);
			// The giant gives first (concept 4.5): volatiles before metal, from a harvester in its clouds.
			Chain.Steps.Insert(Make(TEXT("Giant"), LOCTEXT("GiantTitle", "THE GIANT GIVES"),
				LOCTEXT("GiantStory", "The giant's upper clouds are fuel and air. A harvester in its orbit opens them."),
				LOCTEXT("GiantObjective", "CONSTRUCTION: a GAS HARVESTER at the giant (a construction order)."),
				LOCTEXT("GiantDone", "Volatiles flowing from the giant. Ships can be fuelled now; metal will come from the moon."),
				ETrigger::Token, APSProgressionTokens::ResVolatiles(), 0.0f, 2, LOCTEXT("GiantOpens", "VOLATILES")),
				Chain.Steps.Num() - 2);
		}
		return Chains;
	}

	const TArray<FChain>& Chains()
	{
		static const TArray<FChain> Built = Build();
		return Built;
	}

	AAstroGenerator* Generator(UWorld* World)
	{
		for (TActorIterator<AAstroGenerator> It(World); It; ++It)
		{
			if (IsValid(*It))
			{
				return *It;
			}
		}
		return nullptr;
	}

	APawn* Pilot(UWorld* World)
	{
		const APlayerController* Controller = World ? World->GetFirstPlayerController() : nullptr;
		return Controller ? Controller->GetPawn() : nullptr;
	}

	/** The pilot's height over the home world's surface, km; negative when there is no pilot or no home world. */
	double HeightOverHomeKm(UWorld* World, const APawn* Pawn)
	{
		const AAstroGenerator* Gen = Generator(World);
		const APlanetaryBody* Home = Gen ? Gen->HomePlanet : nullptr;
		if (!Pawn || !IsValid(Home))
		{
			return -1.0;
		}
		const double RadiusCm = FMath::Max(Home->GetWorldScapeBodyRadiusCm(), 1.0);
		return (FVector::Dist(Pawn->GetActorLocation(), Home->GetActorLocation()) - RadiusCm) / 100000.0;
	}

	bool AncientsChainOnBoard(const FAPSMissionBoard& Board, const FName Key)
	{
		const FString Prefix = FString::Printf(TEXT("Ancients.%s."), *Key.ToString());
		for (const FAPSMission& Mission : Board.GetMissions())
		{
			if (Mission.Template.ToString().StartsWith(Prefix))
			{
				return true;
			}
		}
		FAPSMissionSaveData Data;
		Board.CaptureSave(Data);
		for (const FName& Template : Data.CompletedTemplates)
		{
			if (Template.ToString().StartsWith(Prefix))
			{
				return true;
			}
		}
		return false;
	}

	bool Met(UWorld* World, const FAPSMissionBoard& Board, const FStep& Step)
	{
		APawn* Pawn = Pilot(World);
		switch (Step.Trigger)
		{
		case ETrigger::TerminalOpened:
		{
			const UAPSColonyTerminalSubsystem* Terminal = World->GetSubsystem<UAPSColonyTerminalSubsystem>();
			return Terminal && Terminal->IsTerminalOpen();
		}
		case ETrigger::OnFootAway:
		{
			if (!Pawn || !Pawn->IsA<ACharacter>())
			{
				return false;
			}
			double NearestCm = TNumericLimits<double>::Max();
			for (TActorIterator<AColony> It(World); It; ++It)
			{
				if (IsValid(*It))
				{
					NearestCm = FMath::Min(NearestCm, FVector::Dist(Pawn->GetActorLocation(), It->GetActorLocation()));
				}
			}
			// No colony actor to measure from (a bare start): the step asks for nothing more than being on foot.
			return NearestCm == TNumericLimits<double>::Max() || NearestCm >= Step.Number * 100.0;
		}
		case ETrigger::ModuleBuilt:
		{
			const UAPSColonyConstructionSubsystem* Colony = World->GetSubsystem<UAPSColonyConstructionSubsystem>();
			return Colony && Colony->CountBuilt(Step.Param) > 0;
		}
		case ETrigger::Token:
			return Board.IsUnlocked(Step.Param);
		case ETrigger::StructureBuilt:
		{
			const FAPSInfrastructure* Infrastructure = APSInfrastructureFind(World);
			if (!Infrastructure)
			{
				return false;
			}
			for (const FAPSBuiltStructure& Structure : Infrastructure->GetStructures())
			{
				if (Structure.Type == Step.Param)
				{
					return true;
				}
			}
			return false;
		}
		case ETrigger::AncientsChain:
			return AncientsChainOnBoard(Board, Step.Param);
		case ETrigger::ShipsLaunched:
		{
			const FAPSFleetCommand* Fleet = APSFleetFind(World);
			return Fleet && Fleet->GetLaunchedCount() >= FMath::RoundToInt(Step.Number);
		}
		case ETrigger::InShipAboveKm:
			return Pawn && Pawn->IsA<ASpaceship>() && HeightOverHomeKm(World, Pawn) >= Step.Number;
		case ETrigger::WorldsSurveyed:
		{
			const FAPSFleetCommand* Fleet = APSFleetFind(World);
			const AAstroGenerator* Gen = Generator(World);
			const AStar* Star = Gen ? Gen->HomeStar : nullptr;
			if (!Fleet || !IsValid(Star))
			{
				return false;
			}
			int32 Surveyed = 0;
			for (const APlanet* Planet : Star->Planets)
			{
				if (IsValid(Planet) && Fleet->GetSurvey(Planet) >= APSFleet::ESurvey::Surveyed)
				{
					++Surveyed;
				}
			}
			return Surveyed >= FMath::RoundToInt(Step.Number);
		}
		case ETrigger::OnHomeGround:
		{
			const double Height = HeightOverHomeKm(World, Pawn);
			return Pawn && Pawn->IsA<ACharacter>() && Height >= -1.0 && Height < 2.0;
		}
		default:
			return false;
		}
	}

	FText Kicker(const FChain& Chain, const int32 Index, const bool bDone)
	{
		return FText::Format(bDone ? LOCTEXT("KickerDone", "ORIGIN  ·  STEP {0} OF {1} DONE") : LOCTEXT("KickerOpen", "ORIGIN  ·  STEP {0} OF {1}"),
			FText::AsNumber(Index + 1), FText::AsNumber(Chain.Steps.Num()));
	}

	void TellOpen(UWorld* World, const FChain& Chain, const int32 Index)
	{
		const FStep& Step = Chain.Steps[Index];
		UAPSCivilizationJournalSubsystem::Post(World, JournalCategory, FText::Format(LOCTEXT("JournalOpen", "{0}: {1} Objective: {2}"),
			Step.Title, Step.Story, Step.Objective));
		APSMissionTracker::PushNotice(World, Kicker(Chain, Index, false), Step.Title, Step.Story, Step.Objective, FLinearColor(0.96f, 0.64f, 0.18f));
		UE_LOG(LogTemp, Log, TEXT("[APS.Origin] %s step %d/%d open: %s"), Chain.Key, Index + 1, Chain.Steps.Num(), Step.Id);
	}

	void TellDone(UWorld* World, const FChain& Chain, const int32 Index)
	{
		const FStep& Step = Chain.Steps[Index];
		UAPSCivilizationJournalSubsystem::Post(World, JournalCategory, Step.Done);
		const FText Next = Chain.Steps.IsValidIndex(Index + 1)
			? FText::Format(LOCTEXT("NextStep", "Next: {0}."), Chain.Steps[Index + 1].Title)
			: LOCTEXT("ChainDone", "The origin's story is told. The ladder goes on: the system, the cluster, the galaxy.");
		APSMissionTracker::PushNotice(World, Kicker(Chain, Index, true), Step.Title, Step.Done, Next, FLinearColor(0.37f, 0.82f, 0.54f));
		UE_LOG(LogTemp, Log, TEXT("[APS.Origin] %s step %d/%d done: %s"), Chain.Key, Index + 1, Chain.Steps.Num(), Step.Id);
	}

	FAutoConsoleCommandWithWorld AdvanceCommand(TEXT("aps.Origin.Advance"),
		TEXT("Test: completes the current step of the origin's story by hand."),
		FConsoleCommandWithWorldDelegate::CreateLambda([](UWorld* World)
		{
			UE_LOG(LogTemp, Log, TEXT("[APS.Origin] aps.Origin.Advance: %s"), APSOriginQuests::Advance(World) ? TEXT("advanced") : TEXT("nothing to advance"));
		}));

	// APS DEV 13.10: "aps.Origin.Story" is the variable above; a command under the same name is a Fatal in the console
	// manager at static initialization (the packaged game exited with CrashDuringStaticInit before writing a log).
	FAutoConsoleCommandWithWorld StatusCommand(TEXT("aps.Origin.StoryStatus"),
		TEXT("Logs the origin's story: the chain, the steps done, the current objective."),
		FConsoleCommandWithWorldDelegate::CreateLambda([](UWorld* World)
		{
			const FView V = APSOriginQuests::View(World);
			if (!V.IsValid())
			{
				UE_LOG(LogTemp, Log, TEXT("[APS.Origin] no origin story in this world (ladder %s)"), APSWorldRules::IsLadder(World) ? TEXT("on") : TEXT("off"));
				return;
			}
			UE_LOG(LogTemp, Log, TEXT("[APS.Origin] %s: %d/%d done%s"), V.Chain->Key, V.Done, V.Chain->Steps.Num(),
				V.Current() ? *FString::Printf(TEXT(", now %s: %s"), V.Current()->Id, *V.Current()->Objective.ToString()) : TEXT(", complete"));
		}));
}

const APSOriginQuests::FChain* APSOriginQuests::ChainOf(const APSWorldRules::EOrigin Origin)
{
	return APSOriginQuestsLocal::Chains().FindByPredicate([Origin](const FChain& Chain) { return Chain.Origin == Origin; });
}

FName APSOriginQuests::StepToken(const FChain& Chain, const int32 Index)
{
	return FName(*FString::Printf(TEXT("ORIGIN_%s_%d"), Chain.Key, Index + 1));
}

FText APSOriginQuests::RungName(const int32 Rung)
{
	switch (Rung)
	{
	case 0: return LOCTEXT("Rung0", "THE ARK");
	case 1: return LOCTEXT("Rung1", "THE SURFACE");
	case 2: return LOCTEXT("Rung2", "THE SYSTEM");
	case 3: return LOCTEXT("Rung3", "THE CLUSTER");
	default: return LOCTEXT("Rung4", "THE GALAXY");
	}
}

APSOriginQuests::FView APSOriginQuests::View(const UWorld* World)
{
	FView V;
	if (!World || !APSWorldRules::IsLadder(World))
	{
		return V;
	}
	V.Chain = ChainOf(APSWorldRules::OfGame(World).Origin);
	const FAPSMissionBoard* Board = APSMissionsFind(World);
	if (!V.Chain || !Board)
	{
		return V;
	}
	while (V.Done < V.Chain->Steps.Num() && Board->IsUnlocked(StepToken(*V.Chain, V.Done)))
	{
		++V.Done;
	}
	return V;
}

void APSOriginQuests::Tick(UWorld* World, const float DeltaSeconds)
{
	using namespace APSOriginQuestsLocal;
	if (!World || !World->IsGameWorld() || CVarStory.GetValueOnGameThread() == 0 || !APSWorldRules::IsLadder(World))
	{
		return;
	}
	FState& State = StateFor(World);
	State.Clock += DeltaSeconds;
	if (State.Clock < FMath::Max(CVarCheckSeconds.GetValueOnGameThread(), 0.2f))
	{
		return;
	}
	State.Clock = 0.0f;
	const FView V = View(World);
	FAPSMissionBoard* Board = APSMissionsFind(World);
	if (!V.IsValid() || !Board || V.IsComplete())
	{
		return;
	}
	if (State.Announced != V.Done)
	{
		State.Announced = V.Done;
		TellOpen(World, *V.Chain, V.Done);
		return;
	}
	if (Met(World, *Board, V.Chain->Steps[V.Done]))
	{
		Board->Unlock(StepToken(*V.Chain, V.Done));
		TellDone(World, *V.Chain, V.Done);
	}
}

bool APSOriginQuests::Advance(UWorld* World)
{
	using namespace APSOriginQuestsLocal;
	const FView V = View(World);
	FAPSMissionBoard* Board = APSMissionsFind(World);
	if (!V.IsValid() || !Board || V.IsComplete())
	{
		return false;
	}
	Board->Unlock(StepToken(*V.Chain, V.Done));
	TellDone(World, *V.Chain, V.Done);
	return true;
}

#undef LOCTEXT_NAMESPACE
