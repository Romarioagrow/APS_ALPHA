#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "APS_ALPHA/Actors/Astro/Moon.h"
#include "APS_ALPHA/Actors/Astro/Planet.h"
#include "APS_ALPHA/Core/Enums/PlanetHabitability.h"
#include "APS_ALPHA/Core/Model/GeneratedWorld.h"
#include "APS_ALPHA/Core/Planetary/APSPlanetHabitability.h"
#include "APS_ALPHA/Core/Structs/MoonGenerationModel.h"
#include "APS_ALPHA/Core/Structs/PlanetGenerationModel.h"
#include "APS_ALPHA/Generation/MoonGenerator.h"
#include "APS_ALPHA/Generation/PlanetGenerator.h"
#include "Engine/World.h"

namespace APSPlanetHabitabilityTests
{
	UWorld* CreateWorld()
	{
		const UWorld::InitializationValues Values = UWorld::InitializationValues()
			.AllowAudioPlayback(false)
			.RequiresHitProxies(false)
			.CreatePhysicsScene(false)
			.CreateNavigation(false)
			.CreateAISystem(false)
			.ShouldSimulatePhysics(false)
			.SetTransactional(false);
		return UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr,
			false, ERHIFeatureLevel::Num, &Values);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FAPSPlanetHabitabilityContractTest,
	"APS.Gameplay.Generation.PlanetHabitability",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAPSPlanetHabitabilityContractTest::RunTest(const FString& Parameters)
{
	using Library = UAPSPlanetHabitabilityLibrary;
	TestEqual(TEXT("temperate terrestrial world resolves habitable"),
		Library::ResolveDefaultHabitability(EPlanetType::Terrestrial,
			EPlanetaryZoneType::HabitableZone, 100.0),
		EPlanetHabitability::Habitable);
	TestEqual(TEXT("temperate ocean world resolves habitable"),
		Library::ResolveDefaultHabitability(EPlanetType::Ocean,
			EPlanetaryZoneType::HabitableZone, 140.0),
		EPlanetHabitability::Habitable);
	TestEqual(TEXT("cold tundra remains only potentially habitable"),
		Library::ResolveDefaultHabitability(EPlanetType::Tundra,
			EPlanetaryZoneType::ColdZone, 80.0),
		EPlanetHabitability::PotentiallyHabitable);
	TestEqual(TEXT("hot ocean is not misclassified from visuals alone"),
		Library::ResolveDefaultHabitability(EPlanetType::Ocean,
			EPlanetaryZoneType::HotZone, 140.0),
		EPlanetHabitability::Uninhabitable);
	TestEqual(TEXT("no atmosphere is uninhabitable"),
		Library::ResolveDefaultHabitability(EPlanetType::Forest,
			EPlanetaryZoneType::HabitableZone, 0.0),
		EPlanetHabitability::Uninhabitable);

	UGeneratedWorld* WorldModel = NewObject<UGeneratedWorld>();
	WorldModel->PlanetType = EPlanetType::Frozen;
	WorldModel->PlanetHabitability = EPlanetHabitability::Habitable;
	const FGeneratedWorldData Saved = WorldModel->SaveWorldData();
	TestEqual(TEXT("save snapshot preserves explicit habitability"),
		Saved.PlanetHabitability, EPlanetHabitability::Habitable);

	UPlanetGenerator* PlanetGenerator = NewObject<UPlanetGenerator>();
	TSharedPtr<FPlanetModel> PlanetModel =
		PlanetGenerator->CreatePlanetModelFromGeneratedWorld(WorldModel);
	if (!TestTrue(TEXT("custom planet model is created"), PlanetModel.IsValid()))
	{
		return false;
	}
	TestEqual(TEXT("custom model does not infer from visual type"),
		PlanetModel->PlanetHabitability, EPlanetHabitability::Habitable);

	UWorld* TestWorld = APSPlanetHabitabilityTests::CreateWorld();
	if (!TestNotNull(TEXT("habitability actor world"), TestWorld))
	{
		return false;
	}
	APlanet* Planet = TestWorld->SpawnActor<APlanet>();
	PlanetGenerator->ApplyModel(Planet, PlanetModel);
	TestEqual(TEXT("planet actor receives habitability"),
		Planet->PlanetHabitability, EPlanetHabitability::Habitable);
	TestEqual(TEXT("planet serial data mirrors habitability"),
		Planet->PlanetData.PlanetHabitability, EPlanetHabitability::Habitable);
	WorldModel->HomePlanet = Planet;
	WorldModel->PlanetHabitability = EPlanetHabitability::PotentiallyHabitable;
	const FGeneratedWorldData SavedWithMoonEditorBuffer = WorldModel->SaveWorldData();
	TestEqual(TEXT("save snapshot uses authoritative home planet instead of selected moon editor buffer"),
		SavedWithMoonEditorBuffer.PlanetHabitability, EPlanetHabitability::Habitable);

	UMoonGenerator* MoonGenerator = NewObject<UMoonGenerator>();
	TSharedPtr<FMoonModel> MoonModel = MakeShared<FMoonModel>();
	MoonModel->Type = EMoonType::Continental;
	MoonModel->PlanetHabitability = EPlanetHabitability::PotentiallyHabitable;
	AMoon* Moon = TestWorld->SpawnActor<AMoon>();
	MoonGenerator->ApplyModel(Moon, MoonModel);
	TestEqual(TEXT("moon actor receives independent habitability"),
		Moon->PlanetHabitability, EPlanetHabitability::PotentiallyHabitable);
	TestEqual(TEXT("moon generic body data mirrors habitability"),
		Moon->PlanetData.PlanetHabitability, EPlanetHabitability::PotentiallyHabitable);

	TestWorld->DestroyWorld(false);
	return true;
}

#endif
