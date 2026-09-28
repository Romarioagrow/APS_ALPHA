#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "APS_ALPHA/Core/World/APSWorldOriginSubsystem.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "HAL/IConsoleManager.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FAPSWorldOriginRebaseTest,
	"APS.World.Origin.Rebase",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAPSWorldOriginRebaseTest::RunTest(const FString& Parameters)
{
	const UWorld::InitializationValues InitializationValues = UWorld::InitializationValues()
		.AllowAudioPlayback(false)
		.RequiresHitProxies(false)
		.CreatePhysicsScene(true)
		.CreateNavigation(false)
		.CreateAISystem(false)
		.ShouldSimulatePhysics(false)
		.SetTransactional(false);
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr, false,
		ERHIFeatureLevel::Num, &InitializationValues);
	if (!TestNotNull(TEXT("Test world"), World))
	{
		return false;
	}
	UAPSWorldOriginSubsystem* Origin = World->GetSubsystem<UAPSWorldOriginSubsystem>();
	if (!TestNotNull(TEXT("World origin subsystem exists in game worlds"), Origin))
	{
		World->DestroyWorld(false);
		return false;
	}

	// A surface start 2 756 km from the headquarters frame, as in Rio's session of 2026-09-28.
	const FVector Spawn(227826047.52, -115538803.20, 103500735.89);
	const FVector NeighbourOffset(15000.0, -2500.0, 800.0);
	AStaticMeshActor* Player = World->SpawnActor<AStaticMeshActor>(Spawn, FRotator::ZeroRotator);
	AStaticMeshActor* Neighbour = World->SpawnActor<AStaticMeshActor>(Spawn + NeighbourOffset, FRotator::ZeroRotator);
	AStaticMeshActor* Child = World->SpawnActor<AStaticMeshActor>(Spawn + NeighbourOffset * 2.0, FRotator::ZeroRotator);
	if (!TestTrue(TEXT("Actors spawned"), Player && Neighbour && Child))
	{
		World->DestroyWorld(false);
		return false;
	}
	Player->GetRootComponent()->SetMobility(EComponentMobility::Movable);
	Neighbour->GetRootComponent()->SetMobility(EComponentMobility::Movable);
	Child->GetRootComponent()->SetMobility(EComponentMobility::Movable);
	Child->AttachToActor(Neighbour, FAttachmentTransformRules::KeepWorldTransform);

	TestTrue(TEXT("Rebase onto the spawn succeeds"), Origin->RebaseOnto(Player->GetActorLocation(), TEXT("test")));
	TestTrue(TEXT("The spawn is now within 1 cm of 0,0,0"), Player->GetActorLocation().Size() < 1.0);
	TestTrue(TEXT("Neighbours keep their offset"),
		Neighbour->GetActorLocation().Equals(NeighbourOffset, 1.0));
	TestTrue(TEXT("Attached actors keep their offset"),
		Child->GetActorLocation().Equals(NeighbourOffset * 2.0, 1.0));
	TestTrue(TEXT("Engine origin holds the shift"), FVector(World->OriginLocation).Equals(Spawn, 1.0));
	TestTrue(TEXT("Saves see the generation frame"), Origin->ToGenerationFrame(Player->GetActorLocation()).Equals(Spawn, 1.0));
	TestTrue(TEXT("Generation frame converts back"),
		Origin->FromGenerationFrame(Spawn + NeighbourOffset).Equals(Neighbour->GetActorLocation(), 1.0));

	// A second shift accumulates on top of the first.
	TestTrue(TEXT("Second rebase succeeds"), Origin->RebaseOnto(Neighbour->GetActorLocation(), TEXT("test 2")));
	TestTrue(TEXT("After the second rebase the neighbour is at 0,0,0"), Neighbour->GetActorLocation().Size() < 1.0);
	TestTrue(TEXT("Origin accumulates"), FVector(World->OriginLocation).Equals(Spawn + NeighbourOffset, 1.0));
	TestTrue(TEXT("Generation frame survives two shifts"),
		Origin->ToGenerationFrame(Player->GetActorLocation()).Equals(Spawn, 1.0));

	// Nothing happens next to the origin, beyond the engine's int32 range, or when switched off.
	TestFalse(TEXT("No shift for a point already near 0,0,0"), Origin->RebaseOnto(FVector(500.0, 0.0, 0.0), TEXT("test near")));
	TestFalse(TEXT("No shift beyond the int32 origin range"),
		Origin->RebaseOnto(FVector(3.0e9, 0.0, 0.0), TEXT("test range")));
	IConsoleVariable* Switch = IConsoleManager::Get().FindConsoleVariable(TEXT("aps.WorldOrigin.RebaseOnSpawn"));
	if (TestNotNull(TEXT("aps.WorldOrigin.RebaseOnSpawn exists"), Switch))
	{
		const int32 Previous = Switch->GetInt();
		Switch->Set(0, ECVF_SetByCode);
		TestFalse(TEXT("Switched off"), Origin->RebaseOnto(FVector(1.0e7, 0.0, 0.0), TEXT("test off")));
		Switch->Set(Previous, ECVF_SetByCode);
	}

	World->DestroyWorld(false);
	return true;
}

#endif
