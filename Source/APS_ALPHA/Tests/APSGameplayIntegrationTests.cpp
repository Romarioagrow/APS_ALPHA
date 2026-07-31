#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "APS_ALPHA/Core/Interfaces/VehicleControlling.h"
#include "APS_ALPHA/Core/Model/GeneratedWorld.h"
#include "APS_ALPHA/Pawns/Characters/CustomGravityCharacter.h"
#include "APS_ALPHA/Pawns/Spaceships/Spaceship.h"
#include "APS_ALPHA/UI/MainMenu/WorldGenerationViewModel.h"
#include "Engine/World.h"
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
	TestEqual(TEXT("Leaving zero-G restores gravity scale"), Character->GetCharacterMovement()->GravityScale, 1.0f);

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

	const bool bExited = VehicleInterface && VehicleInterface->RequestReleaseVehicleControl();
	TestTrue(TEXT("Unified vehicle interface releases the pilot"), bExited);
	TestEqual(TEXT("Controller returns to the character"), Controller->GetPawn(), static_cast<APawn*>(Character));
	TestNull(TEXT("Spaceship clears its pilot"), Ship->Pilot.Get());
	TestFalse(TEXT("Pilot is detached after exit"), Character->IsAttachedTo(Ship));
	TestTrue(TEXT("Pilot collision is restored"), Character->GetActorEnableCollision());
	TestTrue(TEXT("Pilot ticking is restored"), Character->IsActorTickEnabled());

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

	APSGameplayIntegrationTests::DestroyTestWorld(World);
	return true;
}

#endif
