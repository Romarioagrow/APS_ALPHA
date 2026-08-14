#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "APS_ALPHA/Actors/Astro/Planet.h"
#include "APS_ALPHA/Core/World/APSPlanetSurfacePlacementResolver.h"
#include "APS_ALPHA/Generation/PlanetarySurfaceGenerator.h"
#include "Engine/World.h"

namespace APSPlanetSurfacePlacementResolverTests
{
	UWorld* CreateTestWorld()
	{
		const UWorld::InitializationValues InitializationValues = UWorld::InitializationValues()
			.AllowAudioPlayback(false)
			.RequiresHitProxies(false)
			.CreatePhysicsScene(true)
			.CreateNavigation(false)
			.CreateAISystem(false)
			.ShouldSimulatePhysics(false)
			.SetTransactional(false);

		return UWorld::CreateWorld(
			EWorldType::Game,
			false,
			NAME_None,
			nullptr,
			false,
			ERHIFeatureLevel::Num,
			&InitializationValues);
	}

	void DestroyTestWorld(UWorld*& World)
	{
		if (World)
		{
			World->DestroyWorld(false);
			World = nullptr;
		}
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FAPSPlanetSurfacePlacementDeterminismTest,
	"APS.Surface.Placement.DeterministicContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAPSPlanetSurfacePlacementDeterminismTest::RunTest(const FString& Parameters)
{
	const FAPSCivilizationFootprintRequest Defaults;
	TestEqual(TEXT("Default base length is 80m"), Defaults.BaseSizeCm.X, 8000.0);
	TestEqual(TEXT("Default base width is 55m"), Defaults.BaseSizeCm.Y, 5500.0);
	TestEqual(TEXT("Default pad diameter is 90m"), Defaults.PadDiameterCm, 9000.0);
	TestEqual(TEXT("Default separation is 140m"), Defaults.SeparationCm, 14000.0);

	const uint32 KeyA = UAPSPlanetSurfacePlacementResolver::BuildPlacementKey(
		7788, 1337, 4, 6378);
	const uint32 KeyB = UAPSPlanetSurfacePlacementResolver::BuildPlacementKey(
		7788, 1337, 4, 6378);
	const uint32 ChangedManifestKey =
		UAPSPlanetSurfacePlacementResolver::BuildPlacementKey(7789, 1337, 4, 6378);
	const uint32 ChangedSurfaceKey =
		UAPSPlanetSurfacePlacementResolver::BuildPlacementKey(7788, 1338, 4, 6378);
	TestEqual(TEXT("Identical canonical inputs preserve the placement key"), KeyA, KeyB);
	TestNotEqual(TEXT("Manifest seed changes the placement key"), KeyA, ChangedManifestKey);
	TestNotEqual(TEXT("Surface seed changes the placement key"), KeyA, ChangedSurfaceKey);

	FVector OutwardA;
	FVector ForwardA;
	FVector OutwardB;
	FVector ForwardB;
	UAPSPlanetSurfacePlacementResolver::BuildSeedFrame(KeyA, OutwardA, ForwardA);
	UAPSPlanetSurfacePlacementResolver::BuildSeedFrame(KeyA, OutwardB, ForwardB);
	TestTrue(TEXT("Seed frame outward is normalized"), OutwardA.IsUnit(1.0e-6));
	TestTrue(TEXT("Seed frame forward is normalized"), ForwardA.IsUnit(1.0e-6));
	TestTrue(TEXT("Seed frame is tangent"),
		FMath::Abs(FVector::DotProduct(OutwardA, ForwardA)) <= 1.0e-6);
	TestTrue(TEXT("Seed frame outward is deterministic"), OutwardA.Equals(OutwardB, 1.0e-9));
	TestTrue(TEXT("Seed frame forward is deterministic"), ForwardA.Equals(ForwardB, 1.0e-9));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FAPSPlanetSurfacePlacementReadyActivePreservationTest,
	"APS.Surface.Placement.ReadyActivePreservation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAPSPlanetSurfacePlacementReadyActivePreservationTest::RunTest(const FString& Parameters)
{
	UWorld* World = APSPlanetSurfacePlacementResolverTests::CreateTestWorld();
	if (!TestNotNull(TEXT("Test world"), World))
	{
		return false;
	}

	APlanet* HomeBody = World->SpawnActor<APlanet>();
	if (!TestNotNull(TEXT("Home planet"), HomeBody))
	{
		APSPlanetSurfacePlacementResolverTests::DestroyTestWorld(World);
		return false;
	}
	HomeBody->PlanetType = EPlanetType::Desert;
	HomeBody->RadiusKM = 1000.0;
	HomeBody->PlanetRadiusKM = 1000;
	HomeBody->WorldScapePresentationScale = 0.001337;
	HomeBody->SetWorldScapeStreamingState(EWorldScapeSurfaceState::Active);

	APlanetarySurfaceGenerator* Surface = HomeBody->PlanetaryEnvironmentGenerator;
	AWorldScapeRoot* Root = IsValid(Surface) ? Surface->WorldScapeRootInstance : nullptr;
	if (!TestNotNull(TEXT("Active surface generator"), Surface)
		|| !TestNotNull(TEXT("Active WorldScape root"), Root))
	{
		APSPlanetSurfacePlacementResolverTests::DestroyTestWorld(World);
		return false;
	}

	UWorldScapeLod* RetainedLodSentinel =
		NewObject<UWorldScapeLod>(Root, NAME_None, RF_Transient);
	Root->WorldScapeLod.Add(RetainedLodSentinel);
	Root->bGenerateWorldScape = true;
	Root->bFreezeGeneration = true;
	Root->SetActorHiddenInGame(false);
	HomeBody->bWorldScapeSurfaceReady = true;

	auto TestReadyRootPreserved = [this, HomeBody, Root, RetainedLodSentinel](const TCHAR* Stage)
	{
		bool bPreserved = true;
		bPreserved &= TestEqual(
			*FString::Printf(TEXT("%s preserves the active state"), Stage),
			HomeBody->GetWorldScapeStreamingState(), EWorldScapeSurfaceState::Active);
		bPreserved &= TestEqual(
			*FString::Printf(TEXT("%s preserves the root pointer"), Stage),
			HomeBody->PlanetaryEnvironmentGenerator->WorldScapeRootInstance, Root);
		bPreserved &= TestTrue(
			*FString::Printf(TEXT("%s preserves the ready latch"), Stage),
			HomeBody->bWorldScapeSurfaceReady);
		bPreserved &= TestFalse(
			*FString::Printf(TEXT("%s keeps the root visible"), Stage), Root->IsHidden());
		bPreserved &= TestTrue(
			*FString::Printf(TEXT("%s retains the resident LOD sentinel"), Stage),
			Root->WorldScapeLod.Contains(RetainedLodSentinel));
		return bPreserved;
	};

	TestTrue(TEXT("Ready frozen Active surface starts semantically active"),
		HomeBody->IsWorldScapeStreamingActive());
	TestTrue(TEXT("Initial ready root invariants"),
		TestReadyRootPreserved(TEXT("Initial state")));

	FAPSCivilizationFootprintRequest Request;
	Request.ManifestSeed = 7788;
	for (int32 PollIndex = 0; PollIndex < 3; ++PollIndex)
	{
		FAPSCivilizationFootprintResult PollResult;
		UAPSPlanetSurfacePlacementResolver::TryResolveCivilizationFootprint(
			HomeBody, Request, PollResult);
		TestTrue(
			*FString::Printf(TEXT("Footprint poll %d preserves the ready root"), PollIndex),
			TestReadyRootPreserved(TEXT("Footprint poll")));
	}

	FAPSCivilizationFootprintResult AnchorRequest;
	AnchorRequest.PlacementKey = static_cast<int64>(
		UAPSPlanetSurfacePlacementResolver::BuildPlacementKey(
			Request.ManifestSeed, HomeBody->WorldScapeSeed,
			static_cast<int32>(HomeBody->PlanetType), HomeBody->PlanetRadiusKM));
	AnchorRequest.bTerrainResolved = true;
	AnchorRequest.BaseTransform.SetLocation(Root->GetActorLocation() + FVector(1000.0, 0.0, 0.0));
	AnchorRequest.PadTransform.SetLocation(Root->GetActorLocation() + FVector(15000.0, 0.0, 0.0));
	const int32 InitialCollisionAnchorCount = Root->CollisionDependantActor.Num();
	for (int32 PollIndex = 0; PollIndex < 3; ++PollIndex)
	{
		TestTrue(
			*FString::Printf(TEXT("Placement anchor request %d succeeds"), PollIndex),
			UAPSPlanetSurfacePlacementResolver::RequestPlacementAnchors(
				HomeBody, AnchorRequest));
		TestTrue(
			*FString::Printf(TEXT("Placement anchor poll %d preserves the ready root"), PollIndex),
			TestReadyRootPreserved(TEXT("Placement anchor poll")));
	}

	UAPSPlanetSurfacePlacementResolver::ReleasePlacementAnchors(
		HomeBody, AnchorRequest.PlacementKey);
	TestEqual(TEXT("Release removes only the placement-owned anchors"),
		Root->CollisionDependantActor.Num(), InitialCollisionAnchorCount);
	TestTrue(TEXT("Anchor release preserves the ready root"),
		TestReadyRootPreserved(TEXT("Anchor release")));

	APSPlanetSurfacePlacementResolverTests::DestroyTestWorld(World);
	return true;
}

#endif
