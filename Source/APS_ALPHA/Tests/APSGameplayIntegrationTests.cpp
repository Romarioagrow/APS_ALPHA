#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "APS_ALPHA/Core/Interfaces/VehicleControlling.h"
#include "APS_ALPHA/Core/Model/GeneratedWorld.h"
#include "APS_ALPHA/Actors/Astro/Galaxy.h"
#include "APS_ALPHA/Actors/Astro/Moon.h"
#include "APS_ALPHA/Actors/Astro/Planet.h"
#include "APS_ALPHA/Actors/Astro/StarCluster.h"
#include "APS_ALPHA/Actors/Astro/StarSystem.h"
#include "APS_ALPHA/Core/Enums/PlanetType.h"
#include "APS_ALPHA/Core/Enums/StarClusterType.h"
#include "APS_ALPHA/Core/Structs/StarGenerationModel.h"
#include "APS_ALPHA/Core/Structs/StarSystemGenerationModel.h"
#include "APS_ALPHA/Generation/PlanetarySurfaceGenerator.h"
#include "APS_ALPHA/Generation/APSWorldScapePlanetNoise.h"
#include "APS_ALPHA/Generation/PlanetaryProceduralGenerator.h"
#include "APS_ALPHA/Generation/PlanetGenerator.h"
#include "APS_ALPHA/Generation/MoonGenerator.h"
#include "APS_ALPHA/Generation/StarGenerator.h"
#include "APS_ALPHA/Generation/StarClusterGenerator.h"
#include "APS_ALPHA/Generation/StarSystemGenerator.h"
#include "APS_ALPHA/Pawns/Characters/CustomGravityCharacter.h"
#include "APS_ALPHA/Pawns/Spaceships/Spaceship.h"
#include "APS_ALPHA/Pawns/Spaceships/ShipNavigationComponent.h"
#include "APS_ALPHA/UI/MainMenu/WorldGenerationViewModel.h"
#include "Engine/World.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Components/SpotLightComponent.h"
#include "Components/PointLightComponent.h"

namespace APSGameplayIntegrationTests
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
	FAPSGenerationViewModelConstraintsTest,
	"APS.Gameplay.Generation.ViewModelConstraints",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAPSGenerationViewModelConstraintsTest::RunTest(const FString& Parameters)
{
	UGeneratedWorld* Model = NewObject<UGeneratedWorld>();
	Model->PlanetsAmount = 0;
	Model->StartPlanetIndex = 99;
	Model->PlanetSurfaceSeed = -5;
	Model->SurfaceFeatureScale = 99.0;
	Model->SurfaceReliefScale = 0.0;
	Model->SurfaceLandCoverageScale = 99.0;
	Model->SurfaceMountainScale = -1.0;
	Model->SurfaceCraterScale = 99.0;
	Model->SurfaceRoughnessScale = 0.0;

	UWorldGenerationViewModel* ViewModel = NewObject<UWorldGenerationViewModel>();
	ViewModel->Initialize(GetTransientPackage(), Model);

	TestEqual(TEXT("Initialization guarantees at least one planet"), Model->PlanetsAmount, 1);
	TestEqual(TEXT("Initialization clamps the start planet"), Model->StartPlanetIndex, 1);
	TestEqual(TEXT("Surface seed cannot be negative"), Model->PlanetSurfaceSeed, 0);
	TestEqual(TEXT("Surface feature scale is bounded"), Model->SurfaceFeatureScale, 4.0);
	TestEqual(TEXT("Surface relief scale is bounded"), Model->SurfaceReliefScale, 0.25);
	TestEqual(TEXT("Surface land coverage scale is bounded"), Model->SurfaceLandCoverageScale, 2.0);
	TestEqual(TEXT("Surface mountain scale is bounded"), Model->SurfaceMountainScale, 0.0);
	TestEqual(TEXT("Surface crater scale is bounded"), Model->SurfaceCraterScale, 2.0);
	TestEqual(TEXT("Surface roughness scale is bounded"), Model->SurfaceRoughnessScale, 0.25);

	ViewModel->SetPlanetsAmount(4.0);
	ViewModel->SetStartPlanetIndex(12.0);
	ViewModel->SetPlanetRadius(-10.0);
	ViewModel->SetMoonsAmount(-2.0);

	TestEqual(TEXT("Planet amount accepts valid values"), Model->PlanetsAmount, 4);
	TestEqual(TEXT("Start planet stays inside generated planet list"), Model->StartPlanetIndex, 4);
	TestEqual(TEXT("Planet radius respects the preview's 100 km minimum"), Model->PlanetRadius, 100.0);
	TestEqual(TEXT("Moon count cannot be negative"), Model->MoonsAmount, 0);

	Model->PlanetType = EPlanetType::HighMountain;
	Model->PlanetSurfaceSeed = 7788;
	Model->SurfaceFeatureScale = 1.35;
	Model->SurfaceReliefScale = 1.45;
	Model->SurfaceLandCoverageScale = 0.8;
	Model->SurfaceMountainScale = 1.7;
	Model->SurfaceCraterScale = 0.4;
	Model->SurfaceRoughnessScale = 1.2;
	UPlanetGenerator* PlanetGenerator = NewObject<UPlanetGenerator>();
	const TSharedPtr<FPlanetModel> PlanetModel = PlanetGenerator->CreatePlanetModelFromGeneratedWorld(Model);
	if (TestTrue(TEXT("UI model creates a planet-generation model"), PlanetModel.IsValid()))
	{
		TestEqual(TEXT("Selected EPlanetType reaches the surface pipeline"), PlanetModel->PlanetType,
			EPlanetType::HighMountain);
		TestEqual(TEXT("Surface seed reaches the body model"), PlanetModel->SurfaceSeed, 7788);
		TestEqual(TEXT("Feature multiplier reaches the body model"), PlanetModel->SurfaceFeatureScale, 1.35);
		TestEqual(TEXT("Relief multiplier reaches the body model"), PlanetModel->SurfaceReliefScale, 1.45);
		TestEqual(TEXT("Land multiplier reaches the body model"), PlanetModel->SurfaceLandCoverageScale, 0.8);
		TestEqual(TEXT("Mountain multiplier reaches the body model"), PlanetModel->SurfaceMountainScale, 1.7);
		TestEqual(TEXT("Crater multiplier reaches the body model"), PlanetModel->SurfaceCraterScale, 0.4);
		TestEqual(TEXT("Roughness multiplier reaches the body model"), PlanetModel->SurfaceRoughnessScale, 1.2);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FAPSWorldScapeFamilyLifecycleTest,
	"APS.Gameplay.World.WorldScapeFamilyLifecycle",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAPSWorldScapeFamilyLifecycleTest::RunTest(const FString& Parameters)
{
	UWorld* World = APSGameplayIntegrationTests::CreateTestWorld();
	if (!TestNotNull(TEXT("Test world"), World))
	{
		return false;
	}

	APlanet* Planet = World->SpawnActor<APlanet>();
	if (!TestNotNull(TEXT("Streamed planet"), Planet))
	{
		APSGameplayIntegrationTests::DestroyTestWorld(World);
		return false;
	}
	Planet->PlanetType = EPlanetType::Ocean;
	Planet->RadiusKM = 1000.0;
	Planet->PlanetRadiusKM = 1000;
	// Menu previews deliberately compress a physical planet. WorldScape 5.4
	// compares this float against an int32 previous value, so the configured
	// amplitude must remain integral or the plugin regenerates every frame.
	Planet->WorldScapePresentationScale = 0.001337;

	TestTrue(TEXT("Preload begins outside activation"),
		Planet->GetWorldScapePreloadRadiusCm() > Planet->GetWorldScapeActivationRadiusCm());
	TestTrue(TEXT("Family unload radius exceeds preload radius"),
		Planet->GetWorldScapeUnloadRadiusCm() > Planet->GetWorldScapePreloadRadiusCm());

	Planet->SetWorldScapeStreamingState(EWorldScapeSurfaceState::Preloaded);
	APlanetarySurfaceGenerator* Generator = Planet->PlanetaryEnvironmentGenerator;
	TestEqual(TEXT("Preloaded surface reports its state"), Planet->GetWorldScapeStreamingState(),
		EWorldScapeSurfaceState::Preloaded);
	AWorldScapeRoot* PreloadedRoot = Generator ? Generator->WorldScapeRootInstance : nullptr;
	UWorldScapeLod* RetainedLodSentinel = nullptr;
	if (TestNotNull(TEXT("Preload allocates a configured WorldScape root"), PreloadedRoot))
	{
		TestTrue(TEXT("Preloaded root remains frozen"), PreloadedRoot->bFreezeGeneration);
		TestFalse(TEXT("Preloaded root does not generate chunks"), PreloadedRoot->bGenerateWorldScape);
		TestTrue(TEXT("Preloaded root remains hidden"), PreloadedRoot->IsHidden());
		TestTrue(TEXT("Preview-scaled noise amplitude cannot trigger per-frame regeneration"),
			FMath::IsNearlyEqual(PreloadedRoot->NoiseIntensity,
				static_cast<float>(FMath::RoundToInt(PreloadedRoot->NoiseIntensity))));
		// A resident family's completed LOD storage is authoritative until an actual
		// profile change requests a drain/rebuild. Activation itself must not invoke
		// WorldScape's destructive WS_ForceRegenerate path.
		RetainedLodSentinel = NewObject<UWorldScapeLod>(PreloadedRoot, NAME_None, RF_Transient);
		PreloadedRoot->WorldScapeLod.Add(RetainedLodSentinel);
	}

	Planet->SetWorldScapeStreamingState(EWorldScapeSurfaceState::Active);
	Generator = Planet->PlanetaryEnvironmentGenerator;
	AWorldScapeRoot* Root = Generator ? Generator->WorldScapeRootInstance : nullptr;
	TestTrue(TEXT("Active surface generates"), Planet->IsWorldScapeStreamingActive());
	TestEqual(TEXT("Activation reuses the preloaded WorldScape root"), Root, PreloadedRoot);
	TestTrue(TEXT("Worker-free activation preserves the resident LOD set"),
		Root && Root->WorldScapeLod.Contains(RetainedLodSentinel));
	if (TestNotNull(TEXT("Activation owns a configured WorldScape root"), Root))
	{
		TestTrue(TEXT("Active root generates runtime collision"), Root->bGenerateCollision);
		TestTrue(TEXT("Active root generates collision for possessed pawns"),
			Root->bGenerateCollisionForAllPlayer);
#if WITH_EDITOR
		TestTrue(TEXT("Active root keeps editor-preview collision enabled"),
			Root->bGenerateCollisionInEditor);
		TestFalse(TEXT("Active root collision remains dynamically centred"),
			Root->bStaticCollisionInEditor);
#endif
		TestTrue(TEXT("Ocean profile enables the ocean mesh"), Root->bOcean);
		TestNotNull(TEXT("Ocean profile assigns an ocean material"), Root->OceanMaterial.DefaultMaterial);
		TestNotNull(TEXT("Ocean profile assigns terrain noise"), Root->WorldScapeNoise);
		Root->WorldScapeLodInGeneration.Add(nullptr, false);
		Planet->bWorldScapeSurfaceReady = true;
		TestTrue(TEXT("In-flight workers retain a previously ready surface"),
			Planet->RefreshWorldScapeSurfaceVisibility());
		TestFalse(TEXT("Incremental generation does not restore the fallback globe"), Root->IsHidden());
		Root->WorldScapeLodInGeneration.Empty();
		Planet->bWorldScapeSurfaceReady = true;
		TestTrue(TEXT("Ready latch survives a transiently incomplete resident LOD set"),
			Planet->RefreshWorldScapeSurfaceVisibility());

		// The synthetic root above never received a genuinely complete payload. Reset
		// the test-only readiness injection before exercising the unresolved freeze
		// path below; production invalidation does this in profile/root/state setters.
		Planet->bWorldScapeSurfaceReady = false;
		Root->SetActorHiddenInGame(true);
	}

	// A live body edit can arrive while WorldScape still owns a Lod result buffer.
	// The entire old profile must remain immutable until that batch drains; changing
	// its noise/material/regen properties early lets GenerateBaseMesh destroy the Lod
	// while LodGenerationThread is still inside UWorldScapeLod::SetData.
	if (Root && Generator)
	{
		const uint32 PreviousSignature = Generator->AppliedSurfaceProfileSignature;
		UWorldScapeNoiseClass* PreviousNoise = Root->WorldScapeNoise;
		auto* PreviousTerrain = Root->TerrainMaterial.DefaultMaterial;
		Root->WorldScapeLodInGeneration.Add(nullptr, false);
		Planet->PlanetType = EPlanetType::Desert;
		++Planet->WorldScapeSeed;
		Generator->ApplySurfaceProfile(Planet);

		TestTrue(TEXT("In-flight live edit queues a deferred surface profile"),
			Generator->IsSurfaceProfileApplyPending());
		TestEqual(TEXT("Queued edit retains the worker's profile signature"),
			Generator->AppliedSurfaceProfileSignature, PreviousSignature);
		TestEqual(TEXT("Queued edit retains the worker's noise object"),
			Root->WorldScapeNoise, PreviousNoise);
		TestEqual(TEXT("Queued edit retains the worker's terrain material"),
			Root->TerrainMaterial.DefaultMaterial, PreviousTerrain);
		TestTrue(TEXT("Deferred reprofile freezes the WorldScape producer"),
			Root->bGenerateWorldScape && Root->bFreezeGeneration);
		TestFalse(TEXT("Deferred reprofile disables the WorldScape actor tick"),
			Root->IsActorTickEnabled());
		Planet->PlanetType = EPlanetType::Frozen;
		++Planet->WorldScapeSeed;
		Generator->ApplySurfaceProfile(Planet);
		TestEqual(TEXT("Repeated queued edit still retains the worker's noise object"),
			Root->WorldScapeNoise, PreviousNoise);

		// An Active request during the drain records the continuation but must not
		// unfreeze the producer before the profile can be swapped atomically.
		Generator->SpawnWorldScapeRoot();
		TestTrue(TEXT("Active continuation remains frozen until drain completes"),
			Root->bFreezeGeneration);
		Root->WorldScapeLodInGeneration.Empty();
		Generator->Tick(0.0f);

		TestFalse(TEXT("Drained live edit clears the deferred profile state"),
			Generator->IsSurfaceProfileApplyPending());
		TestTrue(TEXT("Drained live edit applies the latest resolver profile"),
			Generator->IsSurfaceProfileCurrent(Planet));
		TestEqual(TEXT("Latest queued body edit wins after one drain"),
			Generator->ResolvedSurfaceProfile.PlanetType, EPlanetType::Frozen);
		TestNotEqual(TEXT("Drained live edit changes the applied signature"),
			Generator->AppliedSurfaceProfileSignature, PreviousSignature);
		TestTrue(TEXT("Drained profile installs a new per-body noise object"),
			Root->WorldScapeNoise == Generator->ResolvedNoiseInstance
			&& Root->WorldScapeNoise != PreviousNoise);
		TestTrue(TEXT("Drained profile keeps resolver terrain ownership"),
			Root->TerrainMaterial.DefaultMaterial
				== Generator->ResolvedTerrainMaterialInstance);
		TestTrue(TEXT("Active continuation resumes generation after atomic apply"),
			Root->bGenerateWorldScape && !Root->bFreezeGeneration
			&& Root->IsActorTickEnabled());
	}
	Planet->SetWorldScapeStreamingState(EWorldScapeSurfaceState::FrozenVisible);
	TestEqual(TEXT("Generated sibling remains resident and frozen"), Planet->GetWorldScapeStreamingState(),
		EWorldScapeSurfaceState::FrozenVisible);
	TestTrue(TEXT("Frozen surface keeps generated data"), Root && Root->bGenerateWorldScape && Root->bFreezeGeneration);
	// This synthetic test world never runs a completed WorldScape noise worker.
	// Atomic hand-off must therefore keep the unresolved root hidden and retain the
	// fallback globe even after it is frozen; exposing it would reproduce the torn
	// white/black shell seen in the menu and gameplay during first entry.
	TestTrue(TEXT("Frozen unresolved surface stays hidden behind fallback"), Root && Root->IsHidden());
	TestFalse(TEXT("Frozen unresolved surface is not reported ready"), Planet->bWorldScapeSurfaceReady);
	Planet->SetWorldScapeStreamingState(EWorldScapeSurfaceState::Preloaded);
	TestEqual(TEXT("Leaving the nearest body retains the family root"),
		Generator ? Generator->WorldScapeRootInstance : nullptr, Root);
	TestTrue(TEXT("Retained family root is hidden while only preloaded"), Root && Root->IsHidden());

	// A family can leave the retention zone while the plugin's worker still owns a
	// LOD result buffer. The root must disappear visually at once but remain alive
	// until the generation map drains; destroying it immediately reproduces the
	// reported LodGenerationThread::DoWork -> UWorldScapeLod::SetData crash.
	if (Root)
	{
		Root->WorldScapeLodInGeneration.Add(nullptr, false);
	}
	Planet->SetWorldScapeStreamingState(EWorldScapeSurfaceState::Unloaded);
	TestEqual(TEXT("In-flight unload retains the WorldScape root safely"),
		Generator ? Generator->WorldScapeRootInstance : nullptr, Root);
	TestTrue(TEXT("In-flight unload hides the retained root"), Root && Root->IsHidden());
	if (Root)
	{
		Root->WorldScapeLodInGeneration.Empty();
	}
	if (Generator)
	{
		Generator->Tick(0.0f);
	}
	TestNull(TEXT("Drained family releases its transient root"),
		Generator ? Generator->WorldScapeRootInstance : nullptr);

	APSGameplayIntegrationTests::DestroyTestWorld(World);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FAPSMoonSurfaceResolverPipelineTest,
	"APS.Gameplay.World.MoonSurfaceResolverPipeline",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAPSMoonSurfaceResolverPipelineTest::RunTest(const FString& Parameters)
{
	UWorld* World = APSGameplayIntegrationTests::CreateTestWorld();
	if (!TestNotNull(TEXT("Moon surface test world"), World))
	{
		return false;
	}

	AMoon* Moon = World->SpawnActor<AMoon>();
	if (!TestNotNull(TEXT("Editable solid moon"), Moon))
	{
		APSGameplayIntegrationTests::DestroyTestWorld(World);
		return false;
	}
	Moon->PlanetType = EPlanetType::Frozen;
	Moon->RadiusKM = 1737.4;
	Moon->PlanetRadiusKM = 1737;
	Moon->WorldScapeSeed = 67319;
	Moon->SurfaceFeatureScale = 1.25;
	Moon->SurfaceReliefScale = 1.40;
	Moon->SurfaceLandCoverageScale = 0.85;
	Moon->SurfaceMountainScale = 0.70;
	Moon->SurfaceCraterScale = 1.30;
	Moon->SurfaceRoughnessScale = 1.15;

	Moon->SetWorldScapeStreamingState(EWorldScapeSurfaceState::Preloaded);
	APlanetarySurfaceGenerator* Generator = Moon->PlanetaryEnvironmentGenerator;
	AWorldScapeRoot* Root = Generator ? Generator->WorldScapeRootInstance : nullptr;
	if (TestNotNull(TEXT("Moon resolver allocates a preloaded WorldScape root"), Root)
		&& TestNotNull(TEXT("Moon owns a surface generator"), Generator))
	{
		TestTrue(TEXT("Moon profile is current after preload"),
			Generator->IsSurfaceProfileCurrent(Moon));
		TestEqual(TEXT("Moon keeps its selected EPlanetType subtype"),
			Generator->ResolvedSurfaceProfile.PlanetType, EPlanetType::Frozen);
		TestNotNull(TEXT("Moon uses an individual resolver-created noise instance"),
			Cast<UAPSWorldScapePlanetNoise>(Generator->ResolvedNoiseInstance));
		TestTrue(TEXT("WorldScape root uses the resolver noise without a legacy overwrite"),
			Root->WorldScapeNoise == Generator->ResolvedNoiseInstance);
		TestTrue(TEXT("WorldScape root uses the resolver terrain material"),
			Root->TerrainMaterial.DefaultMaterial == Generator->ResolvedTerrainMaterialInstance);

		const uint32 InitialSignature = Generator->AppliedSurfaceProfileSignature;
		const float InitialMountains = Generator->ResolvedSurfaceProfile.MountainStrength;
		Moon->SurfaceMountainScale = 1.80;
		Generator->GenerateWorldscapeSurfaceByModel(World, Moon);
		TestTrue(TEXT("Legacy moon entry point reapplies the resolver after an edit"),
			Generator->IsSurfaceProfileCurrent(Moon));
		TestNotEqual(TEXT("Moon surface edit changes the applied signature"),
			Generator->AppliedSurfaceProfileSignature, InitialSignature);
		TestTrue(TEXT("Moon mountain control changes the resolved terrain"),
			Generator->ResolvedSurfaceProfile.MountainStrength > InitialMountains);
		TestNotNull(TEXT("Moon edit still uses the custom per-body noise class"),
			Cast<UAPSWorldScapePlanetNoise>(Generator->ResolvedNoiseInstance));
	}

	Moon->SetWorldScapeStreamingState(EWorldScapeSurfaceState::Unloaded);
	if (Generator)
	{
		Generator->Tick(0.0f);
	}
	APSGameplayIntegrationTests::DestroyTestWorld(World);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FAPSCharacterGravityStateTest,
	"APS.Gameplay.Character.GravityStateTransitions",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAPSCharacterGravityStateTest::RunTest(const FString& Parameters)
{
	UWorld* World = APSGameplayIntegrationTests::CreateTestWorld();
	if (!TestNotNull(TEXT("Test world"), World))
	{
		return false;
	}

	ACustomGravityCharacter* Character = World->SpawnActor<ACustomGravityCharacter>();
	if (!TestNotNull(TEXT("Custom gravity character"), Character))
	{
		APSGameplayIntegrationTests::DestroyTestWorld(World);
		return false;
	}

	Character->SetZeroGravityEnabled(true);
	TestTrue(TEXT("Character enters zero-G state"), Character->bIsZeroG);
	TestEqual(TEXT("Zero-G uses flying movement"), Character->GetCharacterMovement()->MovementMode, MOVE_Flying);
	TestEqual(TEXT("Zero-G disables gravity scale"), Character->GetCharacterMovement()->GravityScale, 0.0f);

	Character->SetCustomGravityDirection(FVector(0.0, 0.0, -3.0));
	TestFalse(TEXT("Custom gravity exits zero-G"), Character->bIsZeroG);
	TestTrue(TEXT("Custom gravity direction is normalized"),
		Character->GetCurrentGravityDirection().Equals(FVector::DownVector, KINDA_SMALL_NUMBER));
	TestEqual(TEXT("Leaving zero-G begins with compensated gravity"),
		Character->GetCharacterMovement()->GravityScale, 0.0f);
	Character->Tick(1.0f);
	TestEqual(TEXT("Gravity capture blend restores full gravity"),
		Character->GetCharacterMovement()->GravityScale, 1.0f);

	APSGameplayIntegrationTests::DestroyTestWorld(World);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FAPSVehicleControlRoundTripTest,
	"APS.Gameplay.Vehicle.ControlRoundTrip",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAPSVehicleControlRoundTripTest::RunTest(const FString& Parameters)
{
	UWorld* World = APSGameplayIntegrationTests::CreateTestWorld();
	if (!TestNotNull(TEXT("Test world"), World))
	{
		return false;
	}

	APlayerController* Controller = World->SpawnActor<APlayerController>();
	ACustomGravityCharacter* Character = World->SpawnActor<ACustomGravityCharacter>(
		FVector::ZeroVector, FRotator::ZeroRotator);
	ASpaceship* Ship = World->SpawnActor<ASpaceship>(FVector(1000.0, 0.0, 0.0), FRotator::ZeroRotator);

	if (!TestNotNull(TEXT("Player controller"), Controller)
		|| !TestNotNull(TEXT("Pilot character"), Character)
		|| !TestNotNull(TEXT("Spaceship"), Ship))
	{
		APSGameplayIntegrationTests::DestroyTestWorld(World);
		return false;
	}

	Controller->Possess(Character);
	TestEqual(TEXT("Controller starts on character"), Controller->GetPawn(), static_cast<APawn*>(Character));
	TestEqual(TEXT("Character points back to its controller"), Character->GetController(), static_cast<AController*>(Controller));
	TestTrue(TEXT("Spaceship exposes the unified vehicle interface"),
		Ship->GetClass()->ImplementsInterface(UVehicleControlling::StaticClass()));
	TestTrue(TEXT("Native vehicle eligibility accepts the pilot"),
		Ship->CanRequestVehicleControl(Character));
	TestTrue(TEXT("Uncontrolled fleet ship keeps its fill light disabled"),
		Ship->PilotFillPointLight && !Ship->PilotFillPointLight->IsVisible());
	TestTrue(TEXT("Ship fill light is isolated from world geometry"),
		Ship->PilotFillPointLight
		&& !Ship->PilotFillPointLight->LightingChannels.bChannel0
		&& !Ship->PilotFillPointLight->LightingChannels.bChannel1
		&& Ship->PilotFillPointLight->LightingChannels.bChannel2);
	IVehicleControlling* VehicleInterface = Cast<IVehicleControlling>(Ship);
	TestNotNull(TEXT("Native vehicle interface address is available"), VehicleInterface);
	TestTrue(TEXT("Interface vehicle eligibility accepts the pilot"),
		VehicleInterface && VehicleInterface->CanRequestVehicleControl(Character));

	const bool bEntered = VehicleInterface && VehicleInterface->RequestVehicleControl(Character);
	TestTrue(TEXT("Unified vehicle interface accepts the pilot"), bEntered);
	TestEqual(TEXT("Controller possesses the spaceship"), Controller->GetPawn(), static_cast<APawn*>(Ship));
	TestEqual(TEXT("Spaceship remembers its pilot"), Ship->Pilot.Get(), static_cast<APawn*>(Character));
	TestTrue(TEXT("Pilot is attached to the ship seat"), Character->IsAttachedTo(Ship));
	TestTrue(TEXT("Pilot is hidden while no seated animation is configured"), Character->IsHidden());
	TestTrue(TEXT("Controlled ship enables its camera-side fill light"),
		Ship->PilotFillPointLight && Ship->PilotFillPointLight->IsVisible());

	const bool bExited = VehicleInterface && VehicleInterface->RequestReleaseVehicleControl();
	TestTrue(TEXT("Unified vehicle interface releases the pilot"), bExited);
	TestEqual(TEXT("Controller returns to the character"), Controller->GetPawn(), static_cast<APawn*>(Character));
	TestNull(TEXT("Spaceship clears its pilot"), Ship->Pilot.Get());
	TestFalse(TEXT("Pilot is detached after exit"), Character->IsAttachedTo(Ship));
	TestTrue(TEXT("Pilot collision is restored"), Character->GetActorEnableCollision());
	TestTrue(TEXT("Pilot ticking is restored"), Character->IsActorTickEnabled());
	TestFalse(TEXT("Pilot visibility is restored"), Character->IsHidden());
	TestTrue(TEXT("Released ship disables its camera-side fill light"),
		Ship->PilotFillPointLight && !Ship->PilotFillPointLight->IsVisible());

	APSGameplayIntegrationTests::DestroyTestWorld(World);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FAPSFlightModeEngineMappingTest,
	"APS.Gameplay.Vehicle.FlightModeEngineMapping",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAPSFlightModeEngineMappingTest::RunTest(const FString& Parameters)
{
	UWorld* World = APSGameplayIntegrationTests::CreateTestWorld();
	if (!TestNotNull(TEXT("Test world"), World))
	{
		return false;
	}

	ASpaceship* Ship = World->SpawnActor<ASpaceship>();
	if (!TestNotNull(TEXT("Spaceship"), Ship)
		|| !TestNotNull(TEXT("Onboard computer"), Ship ? Ship->OnboardComputer : nullptr))
	{
		APSGameplayIntegrationTests::DestroyTestWorld(World);
		return false;
	}

	USpaceshipOnboardComputer* Computer = Ship->OnboardComputer;
	Computer->SpaceshipHull = Ship->SpaceshipHull;

	UStaticMesh* TestHullMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (TestNotNull(TEXT("Engine test hull mesh"), TestHullMesh))
	{
		Ship->SpaceshipHull->SetStaticMesh(TestHullMesh);
		Ship->SpaceshipHull->SetWorldScale3D(FVector(30.0, 10.0, 5.0));
		Ship->RefreshInteractionGeometry();

		TestTrue(TEXT("Ship derives an interaction radius from hull bounds"),
			Ship->SphereCollisionComponent->GetUnscaledSphereRadius() > 1000.0f);
		TestFalse(TEXT("Ship derives a pilot seat when no socket or Blueprint override exists"),
			Ship->PilotChair->GetRelativeLocation().IsNearlyZero());
		TestFalse(TEXT("Ship derives a safe exit when no socket or Blueprint override exists"),
			Ship->PilotExitPoint->GetRelativeLocation().Equals(FVector(0.0, -200.0, 100.0), 0.1));

		const FTransform ExplicitSeatOverride(FRotator(0.0, 15.0, 0.0), FVector(123.0, 456.0, 789.0));
		Ship->PilotChair->SetRelativeTransform(ExplicitSeatOverride);
		Ship->RefreshInteractionGeometry();
		TestTrue(TEXT("Automatic setup preserves an explicit ship-specific seat override"),
			Ship->PilotChair->GetRelativeTransform().Equals(ExplicitSeatOverride, 0.1));
	}

	Computer->FlightSystem.CurrentFlightMode = EFlightMode::Station;
	Computer->ApplyEngineModeForCurrentFlightMode();
	TestEqual(TEXT("Station flight uses physical impulse mode"),
		Computer->EngineSystem.CurrentEngineMode, EEngineMode::Impulse);

	Computer->FlightSystem.CurrentFlightMode = EFlightMode::Interplanetary;
	Computer->ApplyEngineModeForCurrentFlightMode();
	TestEqual(TEXT("Interplanetary flight uses space-wrap mode"),
		Computer->EngineSystem.CurrentEngineMode, EEngineMode::SpaceWrap);

	Computer->IncreaseFlightMode();
	TestEqual(TEXT("Flight scale advances to stellar"),
		Computer->FlightSystem.CurrentFlightMode, EFlightMode::Stellar);
	TestEqual(TEXT("Stellar flight remains in space-wrap mode"),
		Computer->EngineSystem.CurrentEngineMode, EEngineMode::SpaceWrap);

	Computer->IncreaseFlightMode();
	TestEqual(TEXT("Flight scale advances to interstellar"),
		Computer->FlightSystem.CurrentFlightMode, EFlightMode::Interstellar);
	TestEqual(TEXT("Interstellar flight uses offset mode"),
		Computer->EngineSystem.CurrentEngineMode, EEngineMode::Offset);

	Computer->IncreaseFlightMode();
	TestEqual(TEXT("Flight scale advances to intergalaxy"),
		Computer->FlightSystem.CurrentFlightMode, EFlightMode::Intergalaxy);
	TestEqual(TEXT("Intergalaxy flight keeps offset mode"),
		Computer->EngineSystem.CurrentEngineMode, EEngineMode::Offset);

	Computer->FlightSystem.CurrentFlightMode = EFlightMode::Basic;
	Computer->ApplyEngineModeForCurrentFlightMode();
	TestEqual(TEXT("Basic flight restores physical impulse mode"),
		Computer->EngineSystem.CurrentEngineMode, EEngineMode::Impulse);

	Ship->bGenerateSimpleHullCollision = true;
	Ship->SimpleCollisionSliceCount = 5;
	Ship->RebuildSimpleHullCollision();
	TestEqual(TEXT("Generated hull uses a bounded number of simple collision slices"),
		Ship->GetGeneratedCollisionCount(), 5);
	TestEqual(TEXT("Imported mesh collision is disabled behind the proxy hull"),
		Ship->SpaceshipHull->GetCollisionEnabled(), ECollisionEnabled::NoCollision);

	APSGameplayIntegrationTests::DestroyTestWorld(World);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FAPSShipClassPresetTest,
	"APS.Gameplay.Vehicle.ShipClassPresets",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAPSShipClassPresetTest::RunTest(const FString& Parameters)
{
	const FSpaceshipClassPreset XXS = ASpaceship::GetPresetForSizeClass(ESpaceshipSizeClass::XXS);
	const FSpaceshipClassPreset Medium = ASpaceship::GetPresetForSizeClass(ESpaceshipSizeClass::M);
	const FSpaceshipClassPreset Titan = ASpaceship::GetPresetForSizeClass(ESpaceshipSizeClass::Titan);

	TestTrue(TEXT("Small ships accelerate faster than medium ships"),
		XXS.ImpulseAcceleration > Medium.ImpulseAcceleration);
	TestTrue(TEXT("Medium ships support space-wrap"), Medium.bSupportsSpaceWrap);
	TestTrue(TEXT("Medium ships support offset travel"), Medium.bSupportsOffset);
	TestFalse(TEXT("XXS ships cannot use strategic travel engines"),
		XXS.bSupportsSpaceWrap || XXS.bSupportsOffset);
	TestFalse(TEXT("Titan ships use stable kinematic impulse movement"), Titan.bUsesPhysicalImpulse);
	TestTrue(TEXT("Titan ships turn slower than medium ships"), Titan.RotationSpeed < Medium.RotationSpeed);
	TestTrue(TEXT("Titan ships build angular velocity slower than medium ships"),
		Titan.AngularAcceleration < Medium.AngularAcceleration);
	TestTrue(TEXT("XXS ships build angular velocity faster than medium ships"),
		XXS.AngularAcceleration > Medium.AngularAcceleration);

	TestEqual(TEXT("20 metre hull is XXS"),
		ASpaceship::InferSizeClassFromLength(2000.0), ESpaceshipSizeClass::XXS);
	TestEqual(TEXT("60 metre generated hull is S"),
		ASpaceship::InferSizeClassFromLength(6000.0), ESpaceshipSizeClass::S);
	TestEqual(TEXT("Full-scale capital hull is T"),
		ASpaceship::InferSizeClassFromLength(25000000.0), ESpaceshipSizeClass::Titan);

	TestFalse(TEXT("Null mesh never becomes a runtime ship"), ASpaceship::IsGeneratedShipMeshAsset(nullptr));
	UStaticMesh* GeneratedShipMesh = LoadObject<UStaticMesh>(nullptr,
		TEXT("/Game/APS/APS_ALPHA/Assets/AI_Shpis/Pack_1/01/e5b238288cac3d44ede823f8f809396c.e5b238288cac3d44ede823f8f809396c"));
	if (TestNotNull(TEXT("Whitelisted generated ship mesh exists"), GeneratedShipMesh))
	{
		TestTrue(TEXT("AI_Shpis assets are whitelisted"),
			ASpaceship::IsGeneratedShipMeshAsset(GeneratedShipMesh));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FAPSShipDriveEnvironmentTest,
	"APS.Gameplay.Vehicle.DriveEnvironmentAndSteering",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAPSShipDriveEnvironmentTest::RunTest(const FString& Parameters)
{
	UWorld* World = APSGameplayIntegrationTests::CreateTestWorld();
	if (!TestNotNull(TEXT("Test world"), World))
	{
		return false;
	}

	ASpaceship* Ship = World->SpawnActor<ASpaceship>();
	if (!TestNotNull(TEXT("Spaceship"), Ship))
	{
		APSGameplayIntegrationTests::DestroyTestWorld(World);
		return false;
	}

	Ship->SizeClass = ESpaceshipSizeClass::M;
	Ship->ActiveClassPreset = ASpaceship::GetPresetForSizeClass(ESpaceshipSizeClass::M);
	Ship->CurrentFlightEnvironment = EShipFlightEnvironment::DeepSpace;
	TestTrue(TEXT("Every spaceworthy ship can select orbital power in deep space"),
		Ship->CanUseDriveMode(EShipDriveMode::Orbital));
	Ship->CurrentFlightEnvironment = EShipFlightEnvironment::GravityWell;
	TestTrue(TEXT("Gravity well does not remove orbital power from the player"),
		Ship->CanUseDriveMode(EShipDriveMode::Orbital));
	Ship->CurrentFlightEnvironment = EShipFlightEnvironment::Atmosphere;
	TestTrue(TEXT("Atmosphere permits landing power"), Ship->CanUseDriveMode(EShipDriveMode::Landing));
	TestTrue(TEXT("Atmosphere permits explicit exit-atmosphere power"),
		Ship->CanUseDriveMode(EShipDriveMode::Orbital));
	Ship->SelectedDriveMode = EShipDriveMode::Orbital;
	TestEqual(TEXT("Orbital power is labelled as exit atmosphere while atmospheric"),
		Ship->GetDriveModeName(), FString(TEXT("EXIT ATMOSPHERE")));

	TestEqual(TEXT("Landing mode has a precise low speed limit"),
		ASpaceship::GetDriveSpeedScale(EShipDriveMode::Landing), 0.05);
	TestEqual(TEXT("Local mode supports nearby travel"),
		ASpaceship::GetDriveSpeedScale(EShipDriveMode::Local), 1.0);
	TestEqual(TEXT("Orbital mode reaches escape-scale speed"),
		ASpaceship::GetDriveSpeedScale(EShipDriveMode::Orbital), 32.0);
	TestEqual(TEXT("Interplanetary mode continues beyond orbital flight"),
		ASpaceship::GetDriveSpeedScale(EShipDriveMode::Interplanetary), 256.0);
	TestTrue(TEXT("Orbital mode accelerates substantially faster than local mode"),
		ASpaceship::GetDriveAccelerationScale(EShipDriveMode::Orbital)
		> ASpaceship::GetDriveAccelerationScale(EShipDriveMode::Local));
	TestTrue(TEXT("Atmosphere exit has a non-negotiable acceleration floor"),
		ASpaceship::GetMinimumDriveAcceleration(EShipDriveMode::Orbital) >= 500000.0);
	TestTrue(TEXT("Space-wrap is substantially faster than impulse at the same power step"),
		ASpaceship::GetEngineSpeedMultiplier(EEngineMode::SpaceWrap)
		> ASpaceship::GetEngineSpeedMultiplier(EEngineMode::Impulse));
	TestTrue(TEXT("Offset is substantially faster than space-wrap at the same power step"),
		ASpaceship::GetEngineSpeedMultiplier(EEngineMode::Offset)
		> ASpaceship::GetEngineSpeedMultiplier(EEngineMode::SpaceWrap));
	TestNotNull(TEXT("Every ship owns a native navigation component"), Ship->ShipNavigation);
	TestEqual(TEXT("Navigation formats local distances in kilometres"),
		UShipNavigationComponent::FormatDistance(150000.0), FString(TEXT("1.5 km")));
	TestTrue(TEXT("Flight assist compensates gravity by default"), Ship->bFlightAssistCompensatesGravity);
	Ship->SelectedDriveMode = EShipDriveMode::Landing;
	Ship->IncreaseFlightMode();
	TestEqual(TEXT("Right-shift progression advances landing to local even in atmosphere"),
		Ship->SelectedDriveMode, EShipDriveMode::Local);
	Ship->IncreaseFlightMode();
	TestEqual(TEXT("Second progression selects exit-atmosphere power"),
		Ship->SelectedDriveMode, EShipDriveMode::Orbital);
	Ship->IncreaseFlightMode();
	TestEqual(TEXT("Third progression continues to interplanetary impulse flight"),
		Ship->SelectedDriveMode, EShipDriveMode::Interplanetary);
	TestEqual(TEXT("Power progression does not silently switch the selected engine"),
		Ship->OnboardComputer->EngineSystem.CurrentEngineMode, EEngineMode::Impulse);
	Ship->IncreaseFlightMode();
	TestEqual(TEXT("Fourth progression selects the fifth power step"),
		Ship->SelectedDriveMode, EShipDriveMode::Stellar);
	Ship->IncreaseFlightMode();
	TestEqual(TEXT("Fifth progression selects the sixth power step"),
		Ship->SelectedDriveMode, EShipDriveMode::Interstellar);
	TestEqual(TEXT("Sixth impulse power remains on impulse"),
		Ship->OnboardComputer->EngineSystem.CurrentEngineMode, EEngineMode::Impulse);
	Ship->SelectSpaceWrapEngine();
	TestEqual(TEXT("Key 2 selects the space-wrap engine transition"),
		Ship->SelectedEngineMode, EEngineMode::SpaceWrap);
	Ship->SelectOffsetEngine();
	TestEqual(TEXT("Key 3 selects the offset engine transition"),
		Ship->SelectedEngineMode, EEngineMode::Offset);

	Ship->ActiveClassPreset = ASpaceship::GetPresetForSizeClass(ESpaceshipSizeClass::XXS);
	TestTrue(TEXT("Small ships retain all six impulse power steps"),
		Ship->CanUseDriveMode(EShipDriveMode::Interstellar));
	TestFalse(TEXT("Small ships do not expose unsupported space-wrap engines"),
		Ship->CanUseEngineMode(EEngineMode::SpaceWrap));
	TestFalse(TEXT("Small ships do not expose unsupported offset engines"),
		Ship->CanUseEngineMode(EEngineMode::Offset));
	Ship->ActiveClassPreset = ASpaceship::GetPresetForSizeClass(ESpaceshipSizeClass::M);

	Ship->SwitchEngines();
	Ship->ThrustYaw(1.0f);
	Ship->Tick(1.0f / 60.0f);
	const double FirstFrameYawRate = FMath::Abs(Ship->GetCurrentAngularVelocityDegrees().Y);
	TestTrue(TEXT("Steering starts building angular velocity"), FirstFrameYawRate > 0.0);
	TestTrue(TEXT("A single mouse frame cannot instantly reach maximum turn rate"),
		FirstFrameYawRate < Ship->ActiveClassPreset.RotationSpeed);

	Ship->ThrustYaw(0.0f);
	Ship->Tick(1.0f / 60.0f);
	TestTrue(TEXT("Released steering begins damping angular velocity"),
		FMath::Abs(Ship->GetCurrentAngularVelocityDegrees().Y) < FirstFrameYawRate);

	// Engine backends must exchange momentum as one transaction. A stale
	// kinematic/physics velocity here produces the visible forward/backward kick
	// that used to occur halfway through every 1/2/3 engine transition.
	Ship->SelectImpulseEngine();
	Ship->Tick(1.0f);
	const FVector TestCruiseVelocity(123400.0, -5000.0, 1200.0);
	Ship->SpaceshipHull->SetPhysicsLinearVelocity(TestCruiseVelocity);
	const double SpeedBeforeHandoff = Ship->GetShipSpeedMetersPerSecond();
	Ship->SelectSpaceWrapEngine();
	Ship->Tick(0.5f);
	TestTrue(TEXT("Physics-to-kinematic engine handoff preserves speed"),
		FMath::IsNearlyEqual(Ship->GetShipSpeedMetersPerSecond(), SpeedBeforeHandoff, 0.1));
	Ship->SelectImpulseEngine();
	Ship->Tick(0.5f);
	TestTrue(TEXT("Kinematic-to-physics engine handoff preserves speed"),
		FMath::IsNearlyEqual(Ship->GetShipSpeedMetersPerSecond(), SpeedBeforeHandoff, 0.1));

	APSGameplayIntegrationTests::DestroyTestWorld(World);
	return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FAPSClusterSystemDataTest,
	"APS.Gameplay.Generation.ClusterSystemData",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAPSClusterSystemDataTest::RunTest(const FString& Parameters)
{
	const FLinearColor OStar = UStarGenerator::GetStarColor(ESpectralClass::O, 0);
	const FLinearColor TStar = UStarGenerator::GetStarColor(ESpectralClass::T, 5);
	TestTrue(TEXT("O stars stay blue rather than crossing through magenta"),
		OStar.B > OStar.R && OStar.G > OStar.R);
	TestTrue(TEXT("T dwarfs use a red-brown palette rather than magenta"),
		TStar.R > TStar.B && TStar.G >= TStar.B);
	const double SmallStarVisualRadius = UStarGenerator::GetFarStarVisualRadius(0.4);
	const double SmallStarVisualEmission = UStarGenerator::GetFarStarVisualEmission(0.4, 100.0);
	TestEqual(TEXT("Small far-star proxies receive a stable minimum radius"),
		SmallStarVisualRadius, 1.0);
	TestTrue(TEXT("Expanded far-star proxies preserve luminous area instead of saturating"),
		FMath::IsNearlyEqual(SmallStarVisualEmission, 16.0));
	TestEqual(TEXT("Sun-sized far-star proxies keep their physical visual radius"),
		UStarGenerator::GetFarStarVisualRadius(1.2), 1.2);
	TestEqual(TEXT("Sun-sized far-star proxies keep their physical emission"),
		UStarGenerator::GetFarStarVisualEmission(1.2, 100.0), 100.0);

	FStarModel PrimaryStar;
	PrimaryStar.SpectralClass = ESpectralClass::G;
	PrimaryStar.SpectralSubclass = 2;
	PrimaryStar.Luminosity = 1.0f;
	UStarSystemGenerator* Generator = NewObject<UStarSystemGenerator>();
	FStarSystemModel FirstSystem;
	FStarSystemModel SecondSystem;
	Generator->GeneratePotentialStarSystemModel(FirstSystem, PrimaryStar, 424242);
	Generator->GeneratePotentialStarSystemModel(SecondSystem, PrimaryStar, 424242);
	TestEqual(TEXT("Potential-system multiplicity is deterministic"),
		FirstSystem.AmountOfStars, SecondSystem.AmountOfStars);
	TestEqual(TEXT("Potential planet count is deterministic"),
		FirstSystem.PotentialPlanetCount, SecondSystem.PotentialPlanetCount);
	TestTrue(TEXT("G-star potential planet count remains bounded"),
		FirstSystem.PotentialPlanetCount >= 0 && FirstSystem.PotentialPlanetCount <= 12);

	UWorld* World = APSGameplayIntegrationTests::CreateTestWorld();
	AStarCluster* Cluster = World->SpawnActor<AStarCluster>();
	Cluster->StarMeshInstances->bUseTranslatedInstanceSpace = false;
	Cluster->StarMeshInstances->bAutoRebuildTreeOnInstanceChanges = true;
	Cluster->StarMeshInstances->bEvaluateWorldPositionOffset = true;
	Cluster->FinalizeGeneratedInstances();
	TestTrue(TEXT("Finalized star instances use translated large-world space"),
		Cluster->StarMeshInstances->bUseTranslatedInstanceSpace);
	TestFalse(TEXT("Finalized star instances cannot restore animated WPO"),
		Cluster->StarMeshInstances->bEvaluateWorldPositionOffset);
	TestFalse(TEXT("Finalized star instances rebuild only as one explicit batch"),
		Cluster->StarMeshInstances->bAutoRebuildTreeOnInstanceChanges);
	Cluster->GenerationSeed = 77;
	UStarClusterGenerator* ClusterGenerator = NewObject<UStarClusterGenerator>();
	TSharedPtr<FStarModel> FormationStar = MakeShared<FStarModel>(PrimaryStar);
	FormationStar->Radius = 1.0;
	Cluster->ClusterBounds = FVector(160000.0, 160000.0, 18000.0);
	Cluster->StarAmount = 1001;
	Cluster->ClusterType = EStarClusterType::RingArc;
	FBox RingBounds(EForceInit::ForceInit);
	for (int32 Index = 0; Index < Cluster->StarAmount; ++Index)
	{
		RingBounds += ClusterGenerator->CalculateStarPosition(Index, Cluster, FormationStar);
	}
	TestTrue(TEXT("Ring/Arc formation bounds stay centred on the cluster origin"),
		RingBounds.IsValid && RingBounds.GetCenter().Size() < 160000.0 * 100.0 * 0.025);

	Cluster->ClusterBounds = FVector(160000.0, 160000.0, 25000.0);
	Cluster->ClusterType = EStarClusterType::Nebula;
	FVector NebulaSum = FVector::ZeroVector;
	bool bNebulaInsideBounds = true;
	for (int32 Index = 0; Index < Cluster->StarAmount; ++Index)
	{
		const FVector Position = ClusterGenerator->CalculateStarPosition(Index, Cluster, FormationStar);
		NebulaSum += Position;
		bNebulaInsideBounds &= FMath::Abs(Position.X) <= Cluster->ClusterBounds.X * 50.0 + 1.0;
		bNebulaInsideBounds &= FMath::Abs(Position.Y) <= Cluster->ClusterBounds.Y * 50.0 + 1.0;
		bNebulaInsideBounds &= FMath::Abs(Position.Z) <= Cluster->ClusterBounds.Z * 50.0 + 1.0;
	}
	TestTrue(TEXT("Nebula samples stay inside their logical cluster bounds"), bNebulaInsideBounds);
	TestTrue(TEXT("Nebula antipodal sampling stays centred independently of render budget"),
		(NebulaSum / Cluster->StarAmount).Size() < 1.0);

	Cluster->RegisterPotentialSystem(0, FVector(100.0, 200.0, 300.0),
		PrimaryStar, FirstSystem);
	const FClusterStarSystemRecord* Record = Cluster->FindPotentialSystem(0);
	TestNotNull(TEXT("HISM instance owns a lightweight system record"), Record);
	if (Record)
	{
		TestTrue(TEXT("Cluster system gets a valid stable identity"), Record->StableId.IsValid());
		TestEqual(TEXT("Cluster system retains its instance index"), Record->InstanceIndex, 0);
		TestEqual(TEXT("Cluster system retains local full-scale position"),
			Record->ClusterLocalLocation, FVector(100.0, 200.0, 300.0));

		AStarSystem* MaterializedSystem = World->SpawnActor<AStarSystem>();
		Generator->ApplyModel(MaterializedSystem, MakeShared<FStarSystemModel>(Record->SystemModel));
		TestEqual(TEXT("Materialized actor retains the HISM system identity"),
			MaterializedSystem->StableSystemId, Record->StableId);
		TestEqual(TEXT("Materialized actor retains the deterministic seed"),
			MaterializedSystem->GenerationSeed, Record->SystemModel.GenerationSeed);
		TestTrue(TEXT("Actor knows it was materialized from a cluster record"),
			MaterializedSystem->bMaterializedFromCluster);
	}
	APSGameplayIntegrationTests::DestroyTestWorld(World);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FAPSGalaxyCatalogVolumeTest,
	"APS.Gameplay.Generation.GalaxyCatalogVolume",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAPSGalaxyCatalogVolumeTest::RunTest(const FString& Parameters)
{
	FGalaxyCatalogDescriptor Catalog;
	Catalog.GenerationSeed = 271828;
	Catalog.ModeledStarCount = 100000000;
	Catalog.GalaxySize = 250;
	Catalog.StarDensity = 10.0;
	Catalog.GalaxyType = EGalaxyType::Spiral;
	Catalog.GalaxyClass = EGalaxyClass::Sc;
	const double CatalogRadius = Catalog.GalaxySize * 50000.0;

	constexpr int32 SampleCount = 2048;
	FVector SampleSum = FVector::ZeroVector;
	double MaximumAbsoluteZ = 0.0;
	bool bAllSamplesResolved = true;
	bool bAllSamplesInsideCatalog = true;
	for (int32 SampleIndex = 0; SampleIndex < SampleCount; ++SampleIndex)
	{
		const int64 CatalogIndex = Catalog.ModeledStarCount * SampleIndex / SampleCount;
		FGalaxyCatalogStarRecord Record;
		if (!Catalog.ResolveStar(CatalogIndex, Record))
		{
			bAllSamplesResolved = false;
			continue;
		}
		SampleSum += Record.GalaxyLocalLocation;
		MaximumAbsoluteZ = FMath::Max(MaximumAbsoluteZ, FMath::Abs(Record.GalaxyLocalLocation.Z));
		bAllSamplesInsideCatalog &= Record.GalaxyLocalLocation.Size() <= CatalogRadius * 1.001;
	}

	TestTrue(TEXT("Indexed galaxy samples resolve"), bAllSamplesResolved);
	TestTrue(TEXT("Galaxy samples remain inside the logical catalog radius"), bAllSamplesInsideCatalog);
	const FVector SampleCenter = SampleSum / SampleCount;
	TestTrue(TEXT("Galaxy sample remains centered around the home cluster origin"),
		SampleCenter.Size() < CatalogRadius * 0.08);
	TestTrue(TEXT("Disk galaxy retains a visible three-dimensional stellar halo"),
		MaximumAbsoluteZ > CatalogRadius * 0.50);

	FGalaxyCatalogStarRecord FirstResolve;
	FGalaxyCatalogStarRecord SecondResolve;
	TestTrue(TEXT("Stable catalog record resolves first time"), Catalog.ResolveStar(1234567, FirstResolve));
	TestTrue(TEXT("Stable catalog record resolves second time"), Catalog.ResolveStar(1234567, SecondResolve));
	TestEqual(TEXT("Galaxy catalog location is deterministic"),
		FirstResolve.GalaxyLocalLocation, SecondResolve.GalaxyLocalLocation);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FAPSPlanetaryOrbitIsolationTest,
	"APS.Gameplay.Generation.PlanetaryOrbitIsolation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAPSPlanetaryOrbitIsolationTest::RunTest(const FString& Parameters)
{
	UPlanetarySystemGenerator* SystemGenerator = NewObject<UPlanetarySystemGenerator>();
	UPlanetGenerator* PlanetGenerator = NewObject<UPlanetGenerator>();
	UMoonGenerator* MoonGenerator = NewObject<UMoonGenerator>();
	TSharedPtr<FStarModel> StarModel = MakeShared<FStarModel>();
	StarModel->Mass = 1.0;
	StarModel->Radius = 1.0;
	StarModel->Luminosity = 1.0;
	StarModel->SurfaceTemperature = 5772.0;
	StarModel->StellarType = EStellarType::MainSequence;

	const auto GenerateThreePlanetSystem = [&]()
	{
		TSharedPtr<FPlanetarySystemModel> Model = MakeShared<FPlanetarySystemModel>();
		Model->AmountOfPlanets = 3;
		Model->PlanetarySystemType = EPlanetarySystemType::MultiPlanetSystem;
		Model->OrbitDistributionType = EOrbitDistributionType::Uniform;
		SystemGenerator->GenerateCustomPlanetarySystemModel(
			Model, StarModel, PlanetGenerator, MoonGenerator);
		return Model;
	};

	const TSharedPtr<FPlanetarySystemModel> FirstSystem = GenerateThreePlanetSystem();
	const TSharedPtr<FPlanetarySystemModel> SecondSystem = GenerateThreePlanetSystem();
	TestEqual(TEXT("First star receives exactly its requested planets"), FirstSystem->PlanetsList.Num(), 3);
	TestEqual(TEXT("Second star does not inherit the first star's orbit scratch data"),
		SecondSystem->PlanetsList.Num(), 3);
	return true;
}

#endif
