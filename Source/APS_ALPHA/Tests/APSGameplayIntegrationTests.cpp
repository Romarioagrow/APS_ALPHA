#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "APS_ALPHA/Core/Interfaces/VehicleControlling.h"
#include "APS_ALPHA/Core/Model/GeneratedWorld.h"
#include "APS_ALPHA/Actors/Astro/Planet.h"
#include "APS_ALPHA/Core/Enums/PlanetType.h"
#include "APS_ALPHA/Generation/PlanetarySurfaceGenerator.h"
#include "APS_ALPHA/Pawns/Characters/CustomGravityCharacter.h"
#include "APS_ALPHA/Pawns/Spaceships/Spaceship.h"
#include "APS_ALPHA/Pawns/Spaceships/ShipNavigationComponent.h"
#include "APS_ALPHA/UI/MainMenu/WorldGenerationViewModel.h"
#include "Engine/World.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"

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

	UWorldGenerationViewModel* ViewModel = NewObject<UWorldGenerationViewModel>();
	ViewModel->Initialize(GetTransientPackage(), Model);

	TestEqual(TEXT("Initialization guarantees at least one planet"), Model->PlanetsAmount, 1);
	TestEqual(TEXT("Initialization clamps the start planet"), Model->StartPlanetIndex, 1);

	ViewModel->SetPlanetsAmount(4.0);
	ViewModel->SetStartPlanetIndex(12.0);
	ViewModel->SetPlanetRadius(-10.0);
	ViewModel->SetMoonsAmount(-2.0);

	TestEqual(TEXT("Planet amount accepts valid values"), Model->PlanetsAmount, 4);
	TestEqual(TEXT("Start planet stays inside generated planet list"), Model->StartPlanetIndex, 4);
	TestEqual(TEXT("Planet radius remains positive"), Model->PlanetRadius, 1.0);
	TestEqual(TEXT("Moon count cannot be negative"), Model->MoonsAmount, 0);
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

	TestTrue(TEXT("Preload begins outside activation"),
		Planet->GetWorldScapePreloadRadiusCm() > Planet->GetWorldScapeActivationRadiusCm());
	TestTrue(TEXT("Family unload radius exceeds preload radius"),
		Planet->GetWorldScapeUnloadRadiusCm() > Planet->GetWorldScapePreloadRadiusCm());

	Planet->SetWorldScapeStreamingState(EWorldScapeSurfaceState::Preloaded);
	APlanetarySurfaceGenerator* Generator = Planet->PlanetaryEnvironmentGenerator;
	TestEqual(TEXT("Preloaded surface reports its state"), Planet->GetWorldScapeStreamingState(),
		EWorldScapeSurfaceState::Preloaded);
	TestNull(TEXT("Preload does not allocate a runtime WorldScape actor"),
		Generator ? Generator->WorldScapeRootInstance : nullptr);

	Planet->SetWorldScapeStreamingState(EWorldScapeSurfaceState::Active);
	Generator = Planet->PlanetaryEnvironmentGenerator;
	AWorldScapeRoot* Root = Generator ? Generator->WorldScapeRootInstance : nullptr;
	TestTrue(TEXT("Active surface generates"), Planet->IsWorldScapeStreamingActive());
	if (TestNotNull(TEXT("Only activation creates the configured WorldScape root"), Root))
	{
		TestTrue(TEXT("Ocean profile enables the ocean mesh"), Root->bOcean);
		TestNotNull(TEXT("Ocean profile assigns an ocean material"), Root->OceanMaterial.DefaultMaterial);
		TestNotNull(TEXT("Ocean profile assigns terrain noise"), Root->WorldScapeNoise);
	}
	Planet->SetWorldScapeStreamingState(EWorldScapeSurfaceState::FrozenVisible);
	TestEqual(TEXT("Generated sibling remains resident and frozen"), Planet->GetWorldScapeStreamingState(),
		EWorldScapeSurfaceState::FrozenVisible);
	TestTrue(TEXT("Frozen surface keeps generated data"), Root && Root->bGenerateWorldScape && Root->bFreezeGeneration);
	TestFalse(TEXT("Frozen surface stays visible"), Root && Root->IsHidden());
	Planet->SetWorldScapeStreamingState(EWorldScapeSurfaceState::Preloaded);
	TestNull(TEXT("Leaving the nearest body releases its transient root"),
		Generator ? Generator->WorldScapeRootInstance : nullptr);

	Planet->SetWorldScapeStreamingState(EWorldScapeSurfaceState::Unloaded);
	TestNull(TEXT("Leaving the family releases its transient root"),
		Generator ? Generator->WorldScapeRootInstance : nullptr);

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

	const bool bExited = VehicleInterface && VehicleInterface->RequestReleaseVehicleControl();
	TestTrue(TEXT("Unified vehicle interface releases the pilot"), bExited);
	TestEqual(TEXT("Controller returns to the character"), Controller->GetPawn(), static_cast<APawn*>(Character));
	TestNull(TEXT("Spaceship clears its pilot"), Ship->Pilot.Get());
	TestFalse(TEXT("Pilot is detached after exit"), Character->IsAttachedTo(Ship));
	TestTrue(TEXT("Pilot collision is restored"), Character->GetActorEnableCollision());
	TestTrue(TEXT("Pilot ticking is restored"), Character->IsActorTickEnabled());
	TestFalse(TEXT("Pilot visibility is restored"), Character->IsHidden());

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

	APSGameplayIntegrationTests::DestroyTestWorld(World);
	return true;
}

#endif
