#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "APS_ALPHA/Actors/Astro/Moon.h"
#include "APS_ALPHA/Actors/Astro/Planet.h"
#include "APS_ALPHA/Core/Enums/PlanetType.h"
#include "APS_ALPHA/Core/Model/GeneratedWorld.h"
#include "APS_ALPHA/Core/Structs/MoonGenerationModel.h"
#include "APS_ALPHA/Core/Structs/PlanetGenerationModel.h"
#include "APS_ALPHA/Generation/AstroGenerator.h"
#include "Engine/World.h"

namespace APSPreviewBodyEditPersistenceTests
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
		return UWorld::CreateWorld(
			EWorldType::Game, false, NAME_None, nullptr, false,
			ERHIFeatureLevel::Num, &Values);
	}

	void DestroyWorld(UWorld*& World)
	{
		if (World)
		{
			World->DestroyWorld(false);
			World = nullptr;
		}
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FAPSPreviewBodyEditPersistenceTest,
	"APS.Gameplay.Generation.PreviewBodyEditPersistence",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAPSPreviewBodyEditPersistenceTest::RunTest(const FString& Parameters)
{
	UWorld* World = APSPreviewBodyEditPersistenceTests::CreateWorld();
	if (!TestNotNull(TEXT("Test world"), World))
	{
		return false;
	}

	UGeneratedWorld* Model = NewObject<UGeneratedWorld>();
	FAPSPreviewBodyEditOverride PlanetA;
	PlanetA.PlanetType = EPlanetType::HighMountain;
	PlanetA.RadiusKm = 8123.0;
	PlanetA.SurfaceSeed = 10101;
	PlanetA.SurfaceFeatureScale = 1.75;
	PlanetA.SurfaceReliefScale = 1.9;
	PlanetA.SurfaceLandCoverageScale = 0.55;
	PlanetA.SurfaceMountainScale = 1.8;
	PlanetA.SurfaceCraterScale = 0.35;
	PlanetA.SurfaceRoughnessScale = 1.45;
	PlanetA.AtmosphereHeight = 240.0;
	PlanetA.AtmosphereOpacity = 2.5;

	FAPSPreviewBodyEditOverride MoonB;
	MoonB.PlanetType = EPlanetType::Frozen;
	MoonB.RadiusKm = 920.0;
	MoonB.SurfaceSeed = 20202;
	MoonB.SurfaceFeatureScale = 0.7;
	MoonB.SurfaceReliefScale = 1.25;
	MoonB.SurfaceLandCoverageScale = 0.4;
	MoonB.SurfaceMountainScale = 0.6;
	MoonB.SurfaceCraterScale = 1.95;
	MoonB.SurfaceRoughnessScale = 1.8;
	MoonB.AtmosphereHeight = 12.0;
	MoonB.AtmosphereOpacity = 0.4;

	const FString PlanetKey(TEXT("SYS0/S0/P0"));
	const FString MoonKey(TEXT("SYS0/S0/P1/M0"));
	// Focus switch contract: snapshot the current/initial A before the shared UI
	// buffer is hydrated with B. A does not need a prior slider edit to be retained.
	Model->SetPreviewBodyEditOverride(PlanetKey, PlanetA);
	TestEqual(TEXT("Initial A is snapshotted before focusing B"),
		Model->GetPreviewBodyEditOverrideCount(), 1);
	const FAPSPreviewBodyEditOverride* InitialASnapshot =
		Model->FindPreviewBodyEditOverride(PlanetKey);
	if (TestNotNull(TEXT("Initial A snapshot"), InitialASnapshot))
	{
		TestEqual(TEXT("Initial A snapshot keeps its own seed"),
			InitialASnapshot->SurfaceSeed, 10101);
	}

	// Hydrating/editing B adds a second path instead of replacing A's shared state.
	Model->SetPreviewBodyEditOverride(MoonKey, MoonB);
	TestEqual(TEXT("Two independently edited bodies are retained"),
		Model->GetPreviewBodyEditOverrideCount(), 2);

	// These actors represent a fresh hierarchy after an ordinary structural
	// preview rebuild; no mutable actor pointer from the edited hierarchy survives.
	APlanet* RebuiltPlanet = World->SpawnActor<APlanet>();
	AMoon* RebuiltMoon = World->SpawnActor<AMoon>();
	if (!TestNotNull(TEXT("Rebuilt planet"), RebuiltPlanet)
		|| !TestNotNull(TEXT("Rebuilt moon"), RebuiltMoon))
	{
		APSPreviewBodyEditPersistenceTests::DestroyWorld(World);
		return false;
	}

	TestTrue(TEXT("Planet A override reapplies to its stable path"),
		AAstroGenerator::ApplyPreviewBodyEditOverrideByKey(Model, PlanetKey, RebuiltPlanet));
	TestTrue(TEXT("Moon B override reapplies to its stable path"),
		AAstroGenerator::ApplyPreviewBodyEditOverrideByKey(Model, MoonKey, RebuiltMoon));
	TestFalse(TEXT("A missing hierarchy path cannot leak another body's values"),
		AAstroGenerator::ApplyPreviewBodyEditOverrideByKey(
			Model, TEXT("SYS0/S0/P9"), RebuiltPlanet));

	TestEqual(TEXT("Planet A keeps its type"), RebuiltPlanet->PlanetType, EPlanetType::HighMountain);
	TestEqual(TEXT("Planet A keeps its radius"), RebuiltPlanet->RadiusKM, 8123.0);
	TestEqual(TEXT("Planet A keeps its seed"), RebuiltPlanet->WorldScapeSeed, 10101);
	TestEqual(TEXT("Planet A keeps feature scale"), RebuiltPlanet->SurfaceFeatureScale, 1.75);
	TestEqual(TEXT("Planet A keeps relief"), RebuiltPlanet->SurfaceReliefScale, 1.9);
	TestEqual(TEXT("Planet A keeps land coverage"), RebuiltPlanet->SurfaceLandCoverageScale, 0.55);
	TestEqual(TEXT("Planet A keeps mountains"), RebuiltPlanet->SurfaceMountainScale, 1.8);
	TestEqual(TEXT("Planet A keeps craters"), RebuiltPlanet->SurfaceCraterScale, 0.35);
	TestEqual(TEXT("Planet A keeps roughness"), RebuiltPlanet->SurfaceRoughnessScale, 1.45);
	TestTrue(TEXT("Planet A generation model is retained"),
		RebuiltPlanet->PlanetData.PlanetModel.IsValid());
	if (RebuiltPlanet->PlanetData.PlanetModel.IsValid())
	{
		TestEqual(TEXT("Planet A model keeps its seed"),
			RebuiltPlanet->PlanetData.PlanetModel->SurfaceSeed, 10101);
		TestEqual(TEXT("Planet A model keeps its surface type"),
			RebuiltPlanet->PlanetData.PlanetModel->PlanetType, EPlanetType::HighMountain);
	}

	TestEqual(TEXT("Moon B keeps its type"), RebuiltMoon->PlanetType, EPlanetType::Frozen);
	TestEqual(TEXT("Moon B keeps its radius"), RebuiltMoon->RadiusKM, 920.0);
	TestEqual(TEXT("Moon B keeps its seed"), RebuiltMoon->WorldScapeSeed, 20202);
	TestEqual(TEXT("Moon B keeps crater scale"), RebuiltMoon->SurfaceCraterScale, 1.95);
	TestTrue(TEXT("Moon B generation model is retained"), RebuiltMoon->GenerationModel.IsValid());
	if (RebuiltMoon->GenerationModel.IsValid())
	{
		TestEqual(TEXT("Moon B model keeps its seed"),
			RebuiltMoon->GenerationModel->SurfaceSeed, 20202);
		TestEqual(TEXT("Moon B model keeps its surface type"),
			RebuiltMoon->GenerationModel->PlanetType, EPlanetType::Frozen);
	}

	Model->ClearPreviewBodyEditOverrides();
	TestEqual(TEXT("Explicit regeneration can begin a clean edit generation"),
		Model->GetPreviewBodyEditOverrideCount(), 0);

	APSPreviewBodyEditPersistenceTests::DestroyWorld(World);
	return true;
}

#endif
