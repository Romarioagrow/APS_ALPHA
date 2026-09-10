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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FAPSPlanetSurfacePlacementIncrementalTest,
	"APS.Surface.Placement.BoundedSearchAndCache",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAPSPlanetSurfacePlacementIncrementalTest::RunTest(const FString& Parameters)
{
	using namespace APSPlanetSurfacePlacementResolverTests;
	UWorld* World = CreateTestWorld();
	if (!TestNotNull(TEXT("Test world"), World))
	{
		return false;
	}
	APlanet* HomeBody = World->SpawnActor<APlanet>();
	if (!TestNotNull(TEXT("Home body"), HomeBody))
	{
		DestroyTestWorld(World);
		return false;
	}
	HomeBody->PlanetType = EPlanetType::Desert;
	HomeBody->RadiusKM = 1000.0;
	HomeBody->PlanetRadiusKM = 1000;
	HomeBody->WorldScapePresentationScale = 0.001337;
	HomeBody->SetWorldScapeStreamingState(EWorldScapeSurfaceState::Active);
	APlanetarySurfaceGenerator* Surface = HomeBody->PlanetaryEnvironmentGenerator;
	AWorldScapeRoot* Root = IsValid(Surface) ? Surface->WorldScapeRootInstance : nullptr;
	if (!TestNotNull(TEXT("Canonical surface"), Surface)
		|| !TestNotNull(TEXT("Canonical root"), Root)
		|| !TestTrue(TEXT("Canonical profile current"), Surface->IsSurfaceProfileCurrent(HomeBody)))
	{
		DestroyTestWorld(World);
		return false;
	}
	Root->bFreezeGeneration = true;
	Root->bGenerateCollision = false;
	FAPSCivilizationFootprintRequest Request;
	Request.ManifestSeed = 7788;
	Request.MaximumStructureSlope = 1.0e9;
	Request.MaximumRouteSlope = 1.0e9;
	Request.MinimumDryMarginCm = 0.0;
	FAPSCivilizationFootprintResult Synchronous;
	UAPSPlanetSurfacePlacementResolver::TryResolveCivilizationFootprint(HomeBody, Request, Synchronous);
	TestTrue(TEXT("Loose dry fixture produces a terrain candidate"), Synchronous.bTerrainResolved);
	TestFalse(TEXT("Missing collision never permits materialization"), Synchronous.bReadyForMaterialization);

	FAPSCivilizationFootprintSearch Search;
	FAPSCivilizationFootprintResult Result;
	UAPSPlanetSurfacePlacementResolver::AdvanceCivilizationFootprint(HomeBody, Request, Search, Result, 1, 1.0);
	TestEqual(TEXT("First call evaluates only one candidate"), Search.GetEvaluatedCandidateCount(), 1);
	TestTrue(TEXT("Search continues across frames"), Search.IsSearching());
	TestFalse(TEXT("Partial best is not exposed as safe terrain"), Result.bTerrainResolved);
	UAPSPlanetSurfacePlacementResolver::AdvanceCivilizationFootprint(HomeBody, Request, Search, Result, 100, 0.0);
	TestEqual(TEXT("Expired time budget yields after one atomic candidate"), Search.GetEvaluatedCandidateCount(), 2);
	UAPSPlanetSurfacePlacementResolver::AdvanceCivilizationFootprint(HomeBody, Request, Search, Result, 0, 1.0);
	TestEqual(TEXT("Zero candidate budget does no terrain work"), Search.GetEvaluatedCandidateCount(), 2);
	for (int32 Slice = 0; Slice < 630 && Search.IsSearching(); ++Slice)
	{
		const int32 Before = Search.GetEvaluatedCandidateCount();
		UAPSPlanetSurfacePlacementResolver::AdvanceCivilizationFootprint(HomeBody, Request, Search, Result, 7, 1.0);
		TestTrue(TEXT("Candidate budget respected"), Search.GetEvaluatedCandidateCount() - Before <= 7);
	}
	TestFalse(TEXT("Search completes"), Search.IsSearching());
	TestEqual(TEXT("All original candidates evaluated exactly once"), Search.GetEvaluatedCandidateCount(), 628);
	TestEqual(TEXT("Sliced and synchronous searches choose identical ordinal"), Result.CandidateOrdinal, Synchronous.CandidateOrdinal);
	TestTrue(TEXT("Sliced search preserves base transform"), Result.BaseTransform.Equals(Synchronous.BaseTransform, 0.001));
	TestTrue(TEXT("Sliced search preserves pad transform"), Result.PadTransform.Equals(Synchronous.PadTransform, 0.001));
	for (int32 Poll = 0; Poll < 5; ++Poll)
	{
		UAPSPlanetSurfacePlacementResolver::AdvanceCivilizationFootprint(HomeBody, Request, Search, Result);
		TestEqual(TEXT("Collision retry does not repeat terrain search"), Search.GetEvaluatedCandidateCount(), 628);
		TestFalse(TEXT("Cached terrain never bypasses live collision gating"), Result.bReadyForMaterialization);
	}
	const FVector OldBase = Result.BaseTransform.GetLocation();
	const FVector Shift(100000.0, -50000.0, 20000.0);
	Root->SetActorLocation(Root->GetActorLocation() + Shift);
	UAPSPlanetSurfacePlacementResolver::AdvanceCivilizationFootprint(HomeBody, Request, Search, Result);
	TestEqual(TEXT("Root translation retains planet-relative search"), Search.GetEvaluatedCandidateCount(), 628);
	TestTrue(TEXT("Cached site follows live root translation"), Result.BaseTransform.GetLocation().Equals(OldBase + Shift, 0.01));

	Request.PadDiameterCm += 100.0;
	UAPSPlanetSurfacePlacementResolver::AdvanceCivilizationFootprint(HomeBody, Request, Search, Result, 1, 1.0);
	TestEqual(TEXT("Changed footprint invalidates cached search"), Search.GetEvaluatedCandidateCount(), 1);
	++Surface->AppliedSurfaceProfileSignature;
	UAPSPlanetSurfacePlacementResolver::AdvanceCivilizationFootprint(HomeBody, Request, Search, Result, 1, 1.0);
	TestEqual(TEXT("Stale canonical profile discards pending work"), Search.GetEvaluatedCandidateCount(), 0);
	TestFalse(TEXT("Missing canonical profile waits instead of searching every frame"), Search.IsSearching());
	--Surface->AppliedSurfaceProfileSignature;
	UAPSPlanetSurfacePlacementResolver::AdvanceCivilizationFootprint(HomeBody, Request, Search, Result, 2, 1.0);
	Root->NoiseIntensity += 100.0f;
	UAPSPlanetSurfacePlacementResolver::AdvanceCivilizationFootprint(HomeBody, Request, Search, Result, 1, 1.0);
	TestEqual(TEXT("Changed terrain noise invalidates accumulated scores"), Search.GetEvaluatedCandidateCount(), 1);

	// A deliberately submerged fixture checks the negative-result cache too.
	Surface->ResolvedSurfaceProfile.LiquidType = EAPSPlanetLiquidType::Water;
	Surface->ResolvedSurfaceProfile.OceanLevel = 1.0e9f;
	UAPSPlanetSurfacePlacementResolver::AdvanceCivilizationFootprint(HomeBody, Request, Search, Result, 628, 10.0);
	TestFalse(TEXT("Submerged fixture has no safe terrain"), Result.bTerrainResolved);
	TestFalse(TEXT("No-site result is a completed search"), Search.IsSearching());
	for (int32 Poll = 0; Poll < 5; ++Poll)
	{
		UAPSPlanetSurfacePlacementResolver::AdvanceCivilizationFootprint(HomeBody, Request, Search, Result, 1, 1.0);
		TestEqual(TEXT("No-site retry does not restart expensive scan"), Search.GetEvaluatedCandidateCount(), 628);
		TestFalse(TEXT("No-site cache does not authorize materialization"), Result.bReadyForMaterialization);
	}
	Search.Reset();
	TestEqual(TEXT("Reset releases per-world search state"), Search.GetEvaluatedCandidateCount(), 0);
	DestroyTestWorld(World);
	return true;
}

#endif
