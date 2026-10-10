#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "APS_ALPHA/Core/Model/APSWorldRules.h"
#include "APS_ALPHA/Core/Model/GeneratedWorld.h"
#include "APS_ALPHA/Core/Worlds/APSAuthoredWorlds.h"
#include "APS_ALPHA/Gameplay/Colony/APSColonyModuleCatalogue.h"
#include "APS_ALPHA/Gameplay/Expansion/APSInfrastructure.h"
#include "APS_ALPHA/Gameplay/Origins/APSOrigins.h"
#include "APS_ALPHA/UI/MainMenu/APSWorldRoll.h"
#include "HAL/FileManager.h"
#include "UObject/Package.h"

// Rio 07–09.10 (ORIGIN_TASK_CARDS T-01, T-02, T-21): the rules of a world, the origin table and the authored worlds,
// checked without a world. The first test is the SANDBOX guarantee: a fresh model reads as today's game.

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FAPSWorldRulesDefaultsTest,
	"APS.Origin.WorldRules.DefaultsAreToday",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAPSWorldRulesDefaultsTest::RunTest(const FString& Parameters)
{
	TestTrue(TEXT("no model reads as today"), APSWorldRules::Of(nullptr).IsToday());

	UGeneratedWorld* World = NewObject<UGeneratedWorld>(GetTransientPackage());
	TestTrue(TEXT("a fresh model reads as today"), APSWorldRules::Of(World).IsToday());
	TestTrue(TEXT("today describes empty, so old descriptors hold"), APSWorldRules::Describe(APSWorldRules::Of(World)).IsEmpty());

	APSWorldRules::ApplyPreset(*World, APSWorldRules::EMode::Origin);
	const APSWorldRules::FRules Origin = APSWorldRules::Of(World);
	TestTrue(TEXT("ORIGIN: ladder"), Origin.Reach == APSWorldRules::EReach::Ladder);
	TestTrue(TEXT("ORIGIN: unknown sky"), Origin.Knowledge == APSWorldRules::EKnowledge::Unknown);
	TestTrue(TEXT("ORIGIN: traces"), Origin.Others == APSWorldRules::EOthers::Traces);
	TestTrue(TEXT("ORIGIN: an origin is picked"), Origin.Origin == APSWorldRules::EOrigin::Ark);
	TestTrue(TEXT("ORIGIN: civilization goals"), Origin.Goals == APSWorldRules::EGoals::Civilization);
	TestFalse(TEXT("ORIGIN is not today"), Origin.IsToday());

	APSWorldRules::ApplyPreset(*World, APSWorldRules::EMode::SpaceTrips);
	const APSWorldRules::FRules Trips = APSWorldRules::Of(World);
	TestTrue(TEXT("SPACE TRIPS: no goals"), Trips.Goals == APSWorldRules::EGoals::None);
	TestTrue(TEXT("SPACE TRIPS: open reach"), Trips.Reach == APSWorldRules::EReach::Open);
	TestTrue(TEXT("SPACE TRIPS: no origin"), Trips.Origin == APSWorldRules::EOrigin::None);

	APSWorldRules::ApplyPreset(*World, APSWorldRules::EMode::Sandbox);
	TestTrue(TEXT("SANDBOX puts today back"), APSWorldRules::Of(World).IsToday());

	// Out-of-range bytes (a save from a later version) fold to zero instead of reading garbage.
	World->RulesMode = 200;
	World->RulesOrigin = 77;
	TestTrue(TEXT("unknown bytes read as today"), APSWorldRules::Of(World).Mode == APSWorldRules::EMode::Sandbox
		&& APSWorldRules::Of(World).Origin == APSWorldRules::EOrigin::None);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FAPSOriginsDefinitionsTest,
	"APS.Origin.Definitions.FourRecords",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAPSOriginsDefinitionsTest::RunTest(const FString& Parameters)
{
	const TArray<APSOrigins::FDefinition>& All = APSOrigins::All();
	TestEqual(TEXT("four origins"), All.Num(), 4);
	for (const APSOrigins::FDefinition& Definition : All)
	{
		TestFalse(FString::Printf(TEXT("%s has a name"), Definition.Key), Definition.Name.IsEmpty());
		TestFalse(FString::Printf(TEXT("%s has a description"), Definition.Key), Definition.Description.IsEmpty());
		// Every stock appears once in the order (APSInfrastructure::EResource 0..4).
		uint8 Seen = 0;
		for (const uint8 Resource : Definition.ResourceOrder)
		{
			TestTrue(FString::Printf(TEXT("%s: resource %d in range"), Definition.Key, Resource), Resource < 5);
			Seen |= static_cast<uint8>(1u << Resource);
		}
		TestEqual(FString::Printf(TEXT("%s: resource order is a permutation"), Definition.Key), static_cast<int32>(Seen), 31);
		TestEqual(FString::Printf(TEXT("%s: energy opens first"), Definition.Key), static_cast<int32>(Definition.ResourceOrder[0]), 2);
		TestTrue(FString::Printf(TEXT("%s is found by id"), Definition.Key), APSOrigins::Find(Definition.Id) == &Definition);
	}
	const APSOrigins::FDefinition* Exodus = APSOrigins::Find(APSWorldRules::EOrigin::Exodus);
	TestTrue(TEXT("EXODUS waits for the galaxy modes"), Exodus && !Exodus->bAvailable);
	TestTrue(TEXT("ARK is available"), APSOrigins::Find(APSWorldRules::EOrigin::Ark) && APSOrigins::Find(APSWorldRules::EOrigin::Ark)->bAvailable);
	TestNull(TEXT("no definition for none"), APSOrigins::Find(APSWorldRules::EOrigin::None));

	TestEqual(TEXT("metals token"), APSProgressionTokens::ResourceToken(0), APSProgressionTokens::ResMetals());
	TestEqual(TEXT("energy has no token"), APSProgressionTokens::ResourceToken(2), FName(NAME_None));
	TestFalse(TEXT("launch token named"), APSProgressionTokens::Launch().IsNone());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FAPSAuthoredWorldsLocksTest,
	"APS.Origin.AuthoredWorlds.Locks",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAPSAuthoredWorldsLocksTest::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("galaxy scope"), static_cast<int32>(APSAuthoredWorlds::LockBitsOf(APSWorldRoll::EScope::GalaxyOnly)), static_cast<int32>(APSWorldRules::LockGalaxy));
	TestEqual(TEXT("cluster scope"), static_cast<int32>(APSAuthoredWorlds::LockBitsOf(APSWorldRoll::EScope::ClusterOnly)), static_cast<int32>(APSWorldRules::LockCluster));
	TestEqual(TEXT("star scope"), static_cast<int32>(APSAuthoredWorlds::LockBitsOf(APSWorldRoll::EScope::StarOnly)), static_cast<int32>(APSWorldRules::LockStar));
	TestEqual(TEXT("body scope"), static_cast<int32>(APSAuthoredWorlds::LockBitsOf(APSWorldRoll::EScope::BodyOnly)), static_cast<int32>(APSWorldRules::LockBody));
	TestEqual(TEXT("planet-only scope"), static_cast<int32>(APSAuthoredWorlds::LockBitsOf(APSWorldRoll::EScope::PlanetOnly)), static_cast<int32>(APSWorldRules::LockBody));
	TestEqual(TEXT("system scope"), static_cast<int32>(APSAuthoredWorlds::LockBitsOf(APSWorldRoll::EScope::System)),
		static_cast<int32>(APSWorldRules::LockHomeSystem | APSWorldRules::LockStar | APSWorldRules::LockBody));

	UGeneratedWorld* World = NewObject<UGeneratedWorld>(GetTransientPackage());
	TestFalse(TEXT("a rolled world is never locked"), APSAuthoredWorlds::IsScopeLocked(*World, APSWorldRoll::EScope::System));
	World->AuthoredLocks = APSWorldRules::LockAll;
	TestFalse(TEXT("locks without an id do nothing"), APSAuthoredWorlds::IsScopeLocked(*World, APSWorldRoll::EScope::System));
	World->AuthoredWorldId = TEXT("test");
	World->AuthoredLocks = APSWorldRules::LockGalaxy;
	TestTrue(TEXT("galaxy locked"), APSAuthoredWorlds::IsScopeLocked(*World, APSWorldRoll::EScope::GalaxyOnly));
	TestFalse(TEXT("system open"), APSAuthoredWorlds::IsScopeLocked(*World, APSWorldRoll::EScope::System));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FAPSAuthoredWorldsCatalogueTest,
	"APS.Origin.AuthoredWorlds.CatalogueApplies",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAPSAuthoredWorldsCatalogueTest::RunTest(const FString& Parameters)
{
	if (!IFileManager::Get().DirectoryExists(*APSAuthoredWorlds::Root()))
	{
		AddWarning(FString::Printf(TEXT("no authored worlds under %s; nothing to check"), *APSAuthoredWorlds::Root()));
		return true;
	}
	const TArray<APSAuthoredWorlds::FCard>& Cards = APSAuthoredWorlds::Catalogue(true);
	TestTrue(TEXT("the ten authored worlds are listed"), Cards.Num() >= 10);
	const int32 DefaultSeed = GetDefault<UGeneratedWorld>()->GenerationSeed;
	TSet<int32> Seeds;
	for (const APSAuthoredWorlds::FCard& Card : Cards)
	{
		TestTrue(FString::Printf(TEXT("%s has world.json"), *Card.Id), Card.bHasWorld);
		TestFalse(FString::Printf(TEXT("%s has a name"), *Card.Id), Card.Name.IsEmpty());
		if (!Card.bHasWorld)
		{
			continue;
		}
		UGeneratedWorld* World = NewObject<UGeneratedWorld>(GetTransientPackage());
		FString Failure;
		TestTrue(FString::Printf(TEXT("%s applies: %s"), *Card.Id, *Failure), APSAuthoredWorlds::Apply(Card, *World, nullptr, &Failure));
		TestEqual(FString::Printf(TEXT("%s writes its id"), *Card.Id), World->AuthoredWorldId, Card.Id);
		TestEqual(FString::Printf(TEXT("%s writes its locks"), *Card.Id), static_cast<int32>(World->AuthoredLocks), static_cast<int32>(Card.Locks));
		TestNotEqual(FString::Printf(TEXT("%s pins its own seed"), *Card.Id), World->GenerationSeed, DefaultSeed);
		TestFalse(FString::Printf(TEXT("%s pins the home system"), *Card.Id), World->bRandomHomeSystem);
		TestTrue(FString::Printf(TEXT("%s: a distinct seed"), *Card.Id), !Seeds.Contains(World->GenerationSeed));
		Seeds.Add(World->GenerationSeed);
		TestTrue(FString::Printf(TEXT("%s locks what it says"), *Card.Id),
			APSAuthoredWorlds::IsScopeLocked(*World, APSWorldRoll::EScope::System) == ((Card.Locks & (APSWorldRules::LockHomeSystem | APSWorldRules::LockStar | APSWorldRules::LockBody)) != 0));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FAPSLadderCatalogueTest,
	"APS.Origin.Ladder.CatalogueTokens",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAPSLadderCatalogueTest::RunTest(const FString& Parameters)
{
	using namespace APSInfrastructure;
	// T-07: the ground yard the LAUNCH opens.
	const FType* Yard = Find(FName(TEXT("LaunchYard")));
	TestNotNull(TEXT("LAUNCH YARD listed"), Yard);
	if (Yard)
	{
		TestEqual(TEXT("LAUNCH YARD needs the LAUNCH"), Yard->RequiresToken, APSProgressionTokens::Launch());
		TestTrue(TEXT("LAUNCH YARD stands on the ground"), Yard->Placement == EPlacement::Surface);
		TestTrue(TEXT("LAUNCH YARD is a shipyard"), Yard->Visual == EVisual::Shipyard);
	}
	// T-08: the openers of the closed stocks, and every closed stock has one.
	struct FOpener
	{
		const TCHAR* Id;
		FName Token;
	};
	const FOpener Openers[] = {
		{TEXT("MiningOutpost"), APSProgressionTokens::ResMetals()},
		{TEXT("GasHarvester"), APSProgressionTokens::ResVolatiles()},
		{TEXT("ResearchStation"), APSProgressionTokens::ResResearch()},
		{TEXT("SurveyBeacon"), APSProgressionTokens::ResInfluence()},
		{TEXT("AdministrationHub"), APSProgressionTokens::ResInfluence()},
		{TEXT("SolarCollector"), FName(NAME_None)}};
	for (const FOpener& Opener : Openers)
	{
		const FType* Type = Find(FName(Opener.Id));
		TestNotNull(Opener.Id, Type);
		if (Type)
		{
			TestEqual(FString::Printf(TEXT("%s opens %s"), Opener.Id, *Opener.Token.ToString()),
				FAPSInfrastructure::OpensResourceToken(*Type), Opener.Token);
		}
	}
	for (int32 Resource = 0; Resource < static_cast<int32>(EResource::Count); ++Resource)
	{
		const FName Token = APSProgressionTokens::ResourceToken(Resource);
		if (Token.IsNone())
		{
			continue;
		}
		bool bOpener = false;
		for (const FType& Type : Types())
		{
			bOpener |= FAPSInfrastructure::OpensResourceToken(Type) == Token;
		}
		TestTrue(FString::Printf(TEXT("%s has an opener"), *Token.ToString()), bOpener);
		TestFalse(FString::Printf(TEXT("%s has a title"), *Token.ToString()), APSProgressionTokens::Title(Token).IsEmpty());
		TestFalse(FString::Printf(TEXT("%s has a journal line"), *Token.ToString()), APSProgressionTokens::Opened(Token).IsEmpty());
	}
	// T-06: the fabrication bay opens the rover and feeds metals.
	const FAPSColonyModuleSpec* Fab = FAPSColonyModuleCatalogue::Find(FName(TEXT("Fab")));
	TestNotNull(TEXT("FABRICATION BAY listed"), Fab);
	if (Fab)
	{
		TestEqual(TEXT("the bay opens the rover"), Fab->UnlocksToken, APSProgressionTokens::VehiclesRover());
		TestTrue(TEXT("the bay yields metals"), Fab->YieldPerMinute[0] > 0.0f);
	}
	return true;
}

#endif
