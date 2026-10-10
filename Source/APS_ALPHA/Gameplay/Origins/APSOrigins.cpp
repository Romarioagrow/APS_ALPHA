#include "APSOrigins.h"

#include "APS_ALPHA/Core/Enums/CharSpawnPlace.h"

#include "APS_ALPHA/Gameplay/Civilizations/APSCivilizationJournalSubsystem.h"
#include "APS_ALPHA/Gameplay/Expansion/APSInfrastructure.h"
#include "APS_ALPHA/Gameplay/Expansion/APSMissions.h"
#include "Engine/World.h"
#include "HAL/IConsoleManager.h"

#define LOCTEXT_NAMESPACE "APSOrigins"

namespace APSOrigins
{
	const TArray<FDefinition>& All()
	{
		// Resource order: APSInfrastructure::EResource { Metals 0, Volatiles 1, Energy 2, Research 3, Influence 4 }.
		static const TArray<FDefinition> Definitions = []()
		{
			TArray<FDefinition> Out;
			{
				FDefinition D;
				D.Id = APSWorldRules::EOrigin::Ark;
				D.Key = TEXT("Ark");
				D.Name = LOCTEXT("ArkName", "ARK · LANDING");
				D.Subtitle = LOCTEXT("ArkSub", "The ark is down and will never fly again");
				D.Description = LOCTEXT("ArkDesc", "The ark is down. It will never fly again. Power it, walk out, and make this ground yours.");
				D.StartPlace = ECharSpawnPlace::PlanetSurface;
				const uint8 Order[5] = {2, 0, 1, 3, 4};
				FMemory::Memcpy(D.ResourceOrder, Order, sizeof(Order));
				D.OnboardingRoute = TEXT("APS.Onboarding.Origin.Ark");
				Out.Add(D);
			}
			{
				FDefinition D;
				D.Id = APSWorldRules::EOrigin::Adrift;
				D.Key = TEXT("Adrift");
				D.Name = LOCTEXT("AdriftName", "ADRIFT · THE LONG NIGHT");
				D.Subtitle = LOCTEXT("AdriftSub", "A ship, a dying reactor, no surface on record");
				D.Description = LOCTEXT("AdriftDesc", "Reactor at eleven percent and no surface on record. Find a world before the lights go.");
				D.StartPlace = ECharSpawnPlace::SpaceShip;
				const uint8 Order[5] = {2, 1, 0, 3, 4};
				FMemory::Memcpy(D.ResourceOrder, Order, sizeof(Order));
				D.OnboardingRoute = TEXT("APS.Onboarding.Origin.Adrift");
				Out.Add(D);
			}
			{
				FDefinition D;
				D.Id = APSWorldRules::EOrigin::UnderTheRing;
				D.Key = TEXT("UnderTheRing");
				D.Name = LOCTEXT("RingName", "UNDER THE RING");
				D.Subtitle = LOCTEXT("RingSub", "A moon under a giant, and a ring that is not yours");
				D.Description = LOCTEXT("RingDesc", "A moon under a giant, and over the horizon a ring that is not yours. Its tower has stairs the right size for you.");
				D.StartPlace = ECharSpawnPlace::MoonSurface;
				const uint8 Order[5] = {2, 1, 0, 3, 4};
				FMemory::Memcpy(D.ResourceOrder, Order, sizeof(Order));
				D.OnboardingRoute = TEXT("APS.Onboarding.Origin.Ring");
				Out.Add(D);
			}
			{
				FDefinition D;
				D.Id = APSWorldRules::EOrigin::Exodus;
				D.Key = TEXT("Exodus");
				D.Name = LOCTEXT("ExodusName", "EXODUS · THE LONG VOYAGE");
				D.Subtitle = LOCTEXT("ExodusSub", "Hundreds of generations between the galaxies");
				D.Description = LOCTEXT("ExodusDesc", "Hundreds of generations between the galaxies, scanning every star for a world that breathes. Nothing yet.");
				D.StartPlace = ECharSpawnPlace::SpaceShip;
				const uint8 Order[5] = {2, 1, 0, 3, 4};
				FMemory::Memcpy(D.ResourceOrder, Order, sizeof(Order));
				D.OnboardingRoute = TEXT("APS.Onboarding.Origin.Exodus");
				// Waits for the galaxy modes of the generator (concept §2.1, EXODUS dependencies).
				D.bAvailable = false;
				Out.Add(D);
			}
			return Out;
		}();
		return Definitions;
	}

	const FDefinition* Find(const APSWorldRules::EOrigin Id)
	{
		return All().FindByPredicate([Id](const FDefinition& Definition) { return Definition.Id == Id; });
	}
}

namespace APSProgressionTokens
{
	FName Launch() { static const FName Name(TEXT("LAUNCH")); return Name; }
	FName StellarDrive() { static const FName Name(TEXT("STELLAR_DRIVE")); return Name; }
	FName GalaxyHulls() { static const FName Name(TEXT("GALAXY_HULLS")); return Name; }
	FName VehiclesRover() { static const FName Name(TEXT("VEHICLES_ROVER")); return Name; }
	FName VehiclesHover() { static const FName Name(TEXT("VEHICLES_HOVER")); return Name; }
	FName VehiclesDrone() { static const FName Name(TEXT("VEHICLES_DRONE")); return Name; }
	FName ResMetals() { static const FName Name(TEXT("RES_METALS")); return Name; }
	FName ResVolatiles() { static const FName Name(TEXT("RES_VOLATILES")); return Name; }
	FName ResResearch() { static const FName Name(TEXT("RES_RESEARCH")); return Name; }
	FName ResInfluence() { static const FName Name(TEXT("RES_INFLUENCE")); return Name; }
	FName TabInfrastructure() { static const FName Name(TEXT("TAB_INFRASTRUCTURE")); return Name; }
	FName TabFleet() { static const FName Name(TEXT("TAB_FLEET")); return Name; }
	FName TabMap() { static const FName Name(TEXT("TAB_MAP_F10")); return Name; }

	FName ResourceToken(const int32 Resource)
	{
		switch (Resource)
		{
		case 0: return ResMetals();
		case 1: return ResVolatiles();
		case 3: return ResResearch();
		case 4: return ResInfluence();
		default: return NAME_None; // Energy (2) is open from the first minute; others out of range.
		}
	}

	FText Title(const FName Token)
	{
		if (Token == Launch()) return LOCTEXT("TitleLaunch", "THE LAUNCH");
		if (Token == StellarDrive()) return LOCTEXT("TitleStellarDrive", "THE STELLAR DRIVE");
		if (Token == GalaxyHulls()) return LOCTEXT("TitleGalaxyHulls", "THE GALAXY HULLS");
		if (Token == VehiclesRover()) return LOCTEXT("TitleRover", "THE ROVER");
		if (Token == VehiclesHover()) return LOCTEXT("TitleHover", "THE HOVER");
		if (Token == VehiclesDrone()) return LOCTEXT("TitleDrone", "THE DRONE");
		if (Token == ResMetals()) return LOCTEXT("TitleMetals", "METALS");
		if (Token == ResVolatiles()) return LOCTEXT("TitleVolatiles", "VOLATILES");
		if (Token == ResResearch()) return LOCTEXT("TitleResearch", "RESEARCH");
		if (Token == ResInfluence()) return LOCTEXT("TitleInfluence", "INFLUENCE");
		if (Token == TabInfrastructure()) return LOCTEXT("TitleTabInfrastructure", "THE INFRASTRUCTURE TAB");
		if (Token == TabFleet()) return LOCTEXT("TitleTabFleet", "THE FLEET TAB");
		if (Token == TabMap()) return LOCTEXT("TitleTabMap", "THE MAP");
		return FText::FromString(Token.ToString().Replace(TEXT("_"), TEXT(" ")));
	}

	FText Opened(const FName Token)
	{
		if (Token == Launch()) return LOCTEXT("OpenedLaunch", "THE LAUNCH: the stone's last line is a launch window. A LAUNCH YARD can be raised and the first hull laid down.");
		if (Token == StellarDrive()) return LOCTEXT("OpenedStellarDrive", "THE STELLAR DRIVE: the hull's drive core answers ours. M-class hulls with a stellar drive can be laid down.");
		if (Token == GalaxyHulls()) return LOCTEXT("OpenedGalaxyHulls", "THE GALAXY HULLS: the road ends in a shipwright's chart. L-class hulls and larger can be laid down.");
		if (Token == VehiclesRover()) return LOCTEXT("OpenedRover", "THE ROVER: the bay's printer runs through the night. The first rover rolls out at dawn.");
		if (Token == VehiclesHover()) return LOCTEXT("OpenedHover", "THE HOVER: lift coils, printed and balanced. A hover waits by the bay.");
		if (Token == VehiclesDrone()) return LOCTEXT("OpenedDrone", "THE DRONE: a scout that flies. It lifts from the bay's roof.");
		if (Token == ResMetals()) return LOCTEXT("OpenedMetals", "METALS: the outpost's crushers run and the stock counts from now on.");
		if (Token == ResVolatiles()) return LOCTEXT("OpenedVolatiles", "VOLATILES: the harvester's scoops come back full. Fuel and air count from now on.");
		if (Token == ResResearch()) return LOCTEXT("OpenedResearch", "RESEARCH: the station's first paper. Knowledge counts from now on.");
		if (Token == ResInfluence()) return LOCTEXT("OpenedInfluence", "INFLUENCE: the claim is heard. A voice among the stars counts from now on.");
		return FText::Format(LOCTEXT("OpenedToken", "{0} is open."), Title(Token));
	}

	bool Grant(const UWorld* World, const FName Token, const FName Category)
	{
		FAPSMissionBoard* Board = Token.IsNone() ? nullptr : APSMissionsFind(World);
		if (!Board || Board->IsUnlocked(Token))
		{
			return false;
		}
		Board->Unlock(Token);
		if (FAPSInfrastructure* Infrastructure = APSInfrastructureFind(World))
		{
			Infrastructure->RefreshRates();
		}
		UAPSCivilizationJournalSubsystem::Post(World, Category, Opened(Token));
		UE_LOG(LogTemp, Log, TEXT("[APS.Origin] %s opened (%s)"), *Token.ToString(), *Category.ToString());
		return true;
	}
}

namespace APSOriginsConsole
{
	// Test helpers for the ladder: open a token by hand, list what is open.
	FAutoConsoleCommandWithWorldAndArgs GrantCommand(TEXT("aps.Origin.Grant"),
		TEXT("Opens a token of the ORIGIN ladder by hand: aps.Origin.Grant LAUNCH | STELLAR_DRIVE | GALAXY_HULLS | VEHICLES_ROVER | ")
		TEXT("VEHICLES_HOVER | VEHICLES_DRONE | RES_METALS | RES_VOLATILES | RES_RESEARCH | RES_INFLUENCE | all."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			if (!World || Args.Num() == 0)
			{
				UE_LOG(LogTemp, Warning, TEXT("[APS.Origin] aps.Origin.Grant <token|all>"));
				return;
			}
			using namespace APSProgressionTokens;
			const FName Wanted(*Args[0].ToUpper());
			const FName All[] = {Launch(), StellarDrive(), GalaxyHulls(), VehiclesRover(), VehiclesHover(), VehiclesDrone(),
				ResMetals(), ResVolatiles(), ResResearch(), ResInfluence(), TabInfrastructure(), TabFleet(), TabMap()};
			int32 Opened = 0;
			for (const FName& Token : All)
			{
				if (Wanted == TEXT("ALL") || Token == Wanted)
				{
					Opened += Grant(World, Token, TEXT("Console")) ? 1 : 0;
				}
			}
			UE_LOG(LogTemp, Log, TEXT("[APS.Origin] aps.Origin.Grant %s: %d opened"), *Args[0], Opened);
		}));

	FAutoConsoleCommandWithWorldAndArgs TokensCommand(TEXT("aps.Origin.Tokens"),
		TEXT("Logs which tokens of the ORIGIN ladder are open in this world."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>&, UWorld* World)
		{
			using namespace APSProgressionTokens;
			const FAPSMissionBoard* Board = APSMissionsFind(World);
			const FName All[] = {Launch(), StellarDrive(), GalaxyHulls(), VehiclesRover(), VehiclesHover(), VehiclesDrone(),
				ResMetals(), ResVolatiles(), ResResearch(), ResInfluence(), TabInfrastructure(), TabFleet(), TabMap()};
			FString Line;
			for (const FName& Token : All)
			{
				Line += FString::Printf(TEXT(" %s=%s"), *Token.ToString(), Board && Board->IsUnlocked(Token) ? TEXT("open") : TEXT("-"));
			}
			UE_LOG(LogTemp, Log, TEXT("[APS.Origin] ladder %s, tokens:%s"), APSWorldRules::IsLadder(World) ? TEXT("on") : TEXT("off"), *Line);
		}));
}

#undef LOCTEXT_NAMESPACE
