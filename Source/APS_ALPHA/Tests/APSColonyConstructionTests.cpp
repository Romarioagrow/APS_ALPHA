#include "APS_ALPHA/Gameplay/Civilizations/APSCivilizationStarterActors.h"
#include "APS_ALPHA/Gameplay/Colony/APSColonyConstructionSubsystem.h"
#include "APS_ALPHA/Gameplay/Colony/APSColonyModule.h"
#include "APS_ALPHA/Gameplay/Colony/APSColonyModuleCatalogue.h"
#include "APS_ALPHA/Gameplay/Production/APSProductionEventSubsystem.h"
#include "APS_ALPHA/Gameplay/Production/APSProductionSubsystem.h"
#include "APS_ALPHA/Gameplay/Quests/APSEarlyAccessOnboardingDefinition.h"
#include "APS_ALPHA/Gameplay/Spawn/APSSpawnPlacementSubsystem.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAPSColonyModuleCatalogueTest,
	"APS.Colony.Construction.Catalogue",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAPSColonyModuleCatalogueTest::RunTest(const FString& Parameters)
{
	const TArray<FAPSColonyModuleSpec>& Catalogue = FAPSColonyModuleCatalogue::Get();
	TSet<FName> Ids;
	int32 SiteCounts[2] = {0, 0};
	for (const FAPSColonyModuleSpec& Spec : Catalogue)
	{
		TestFalse(*FString::Printf(TEXT("%s id is unique"), *Spec.Id.ToString()), Ids.Contains(Spec.Id));
		Ids.Add(Spec.Id);
		++SiteCounts[static_cast<int32>(Spec.Site)];
		TestTrue(*FString::Printf(TEXT("%s has a footprint, a height and a build time"), *Spec.Id.ToString()),
			Spec.SizeCm.X > 0.0 && Spec.SizeCm.Y > 0.0 && Spec.SizeCm.Z > 0.0 && Spec.BuildSeconds > 0.0);
		TestTrue(*FString::Printf(TEXT("%s has parts"), *Spec.Id.ToString()), Spec.Parts.Num() > 0);
		TestFalse(*FString::Printf(TEXT("%s has a name"), *Spec.Id.ToString()), Spec.Name.IsEmpty());
		FString Reason;
		const FAPSProductionDefinition Definition = FAPSColonyModuleCatalogue::MakeDefinition(Spec);
		const bool bValidDefinition = Definition.IsStructurallyValid(&Reason);
		TestTrue(*FString::Printf(TEXT("%s is a valid Building definition: %s"), *Spec.Id.ToString(), *Reason),
			bValidDefinition && Definition.Domain == EAPSProductionDomain::Building);
		TestEqual(*FString::Printf(TEXT("%s definition id round-trips"), *Spec.Id.ToString()),
			FAPSColonyModuleCatalogue::ModuleIdFromDefinition(Definition.DefinitionId), Spec.Id);
		TestTrue(*FString::Printf(TEXT("%s is found by id"), *Spec.Id.ToString()),
			FAPSColonyModuleCatalogue::Find(Spec.Id) == &Spec);
	}
	TestTrue(TEXT("The surface base offers several modules"), SiteCounts[0] >= 3);
	TestTrue(TEXT("The headquarters offers several modules"), SiteCounts[1] >= 3);
	TestEqual(TEXT("Test build times scale"),
		FAPSColonyModuleCatalogue::MakeDefinition(Catalogue[0], 0.5).DurationSeconds, Catalogue[0].BuildSeconds * 0.5);

	// The onboarding "build your first structure" objective listens to the verb production publishes.
	const UAPSEarlyAccessOnboardingDefinition* Onboarding =
		NewObject<UAPSEarlyAccessOnboardingDefinition>(GetTransientPackage());
	const FAPSQuestObjectiveNodeDefinition* BuildNode = Onboarding->Nodes.FindByPredicate(
		[](const FAPSQuestObjectiveNodeDefinition& Node)
		{
			return Node.NodeId == FAPSEarlyAccessOnboardingContract::StructurePlacedNode;
		});
	TestTrue(TEXT("Onboarding build objective waits for APS.Building.Build"),
		BuildNode && BuildNode->Trigger.Verb == FName(TEXT("APS.Building.Build")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAPSSpawnRingPlacementTest,
	"APS.Colony.Construction.RingPlacement",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAPSSpawnRingPlacementTest::RunTest(const FString& Parameters)
{
	// The 80 x 55 m colony base and a 14 x 9 m module.
	const FVector2D RectHalf(4000.0, 2750.0);
	const double Radius = FVector2D(700.0, 450.0).Size();
	const double Spacing = 800.0;
	TArray<FVector2D> Candidates;
	UAPSSpawnPlacementSubsystem::BuildRingCandidates(FVector2D::ZeroVector, RectHalf, Radius, Spacing, 12000.0, 0,
		Candidates);
	TestTrue(TEXT("Candidates exist"), Candidates.Num() >= 16);
	bool bAllClear = true;
	for (const FVector2D& Candidate : Candidates)
	{
		bAllClear &= UAPSSpawnPlacementSubsystem::DistanceToRectangle(Candidate, FVector2D::ZeroVector, RectHalf)
			>= Spacing + Radius - 1.0;
	}
	TestTrue(TEXT("Every candidate clears the base by the spacing"), bAllClear);
	TestTrue(TEXT("The first candidate stands at the base's side"),
		FMath::Abs(Candidates[0].X) < 1.0 && Candidates[0].Y > RectHalf.Y);
	TestTrue(TEXT("The front (toward the pad) comes last in a ring"),
		Candidates[15].X > RectHalf.X && FMath::Abs(Candidates[15].Y) < 1.0);
	TestTrue(TEXT("The next ring lies farther out"),
		Candidates.Num() > 16 && Candidates[16].Size() > Candidates[0].Size() + Radius);
	TestEqual(TEXT("Point to segment"), UAPSSpawnPlacementSubsystem::DistanceToSegment(
		FVector2D(5.0, 3.0), FVector2D::ZeroVector, FVector2D(10.0, 0.0)), 3.0);
	TestEqual(TEXT("Point past a segment's end"), UAPSSpawnPlacementSubsystem::DistanceToSegment(
		FVector2D(14.0, 3.0), FVector2D::ZeroVector, FVector2D(10.0, 0.0)), 5.0);
	TestEqual(TEXT("Inside a rectangle"), UAPSSpawnPlacementSubsystem::DistanceToRectangle(
		FVector2D(10.0, 10.0), FVector2D::ZeroVector, RectHalf), 0.0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAPSColonyOrbitConstructionTest,
	"APS.Colony.Construction.OrbitFlow",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAPSColonyOrbitConstructionTest::RunTest(const FString& Parameters)
{
	const UWorld::InitializationValues InitializationValues = UWorld::InitializationValues()
		.AllowAudioPlayback(false)
		.RequiresHitProxies(false)
		.CreatePhysicsScene(false)
		.CreateNavigation(false)
		.CreateAISystem(false)
		.ShouldSimulatePhysics(false)
		.SetTransactional(false);
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr, false, ERHIFeatureLevel::Num,
		&InitializationValues);
	if (!TestNotNull(TEXT("Isolated construction test world exists"), World))
	{
		return false;
	}
	FWorldContext& WorldContext = GEngine->CreateNewWorldContext(EWorldType::Game);
	WorldContext.SetCurrentWorld(World);
	const auto DestroyTestWorld = [World]()
	{
		GEngine->DestroyWorldContext(World);
		World->DestroyWorld(false);
	};

	// A station stand-in with real meshes: the native base placeholder, 80 x 55 m.
	AActor* Station = World->SpawnActor<AAPSCivilizationBaseModule>(FVector(0.0, 0.0, 100000.0), FRotator::ZeroRotator);
	World->InitializeActorsForPlay(FURL());
	World->BeginPlay();
	UAPSColonyConstructionSubsystem* Construction = World->GetSubsystem<UAPSColonyConstructionSubsystem>();
	UAPSProductionSubsystem* Production = World->GetSubsystem<UAPSProductionSubsystem>();
	UAPSProductionEventSubsystem* Events = World->GetSubsystem<UAPSProductionEventSubsystem>();
	if (!TestNotNull(TEXT("Station exists"), Station) || !TestNotNull(TEXT("Construction exists"), Construction)
		|| !TestNotNull(TEXT("Production exists"), Production) || !TestNotNull(TEXT("Production events exist"), Events))
	{
		DestroyTestWorld();
		return false;
	}
	TArray<FAPSProductionEvent> Published;
	const FDelegateHandle EventHandle = Events->OnEventPublished().AddLambda(
		[&Published](const FAPSProductionEvent& Event) { Published.Add(Event); });

	const FGuid CivilizationId = FGuid::NewGuid();
	TestTrue(TEXT("The orbit site registers on the station"),
		Construction->RegisterSite(EAPSSpawnSite::Orbit, Station, nullptr, CivilizationId));
	TestTrue(TEXT("The orbit site takes orders"), Construction->IsSiteReady(EAPSSpawnSite::Orbit));
	FText Failure;
	TestFalse(TEXT("A surface module is refused without a surface base"),
		Construction->RequestBuild(TEXT("Habitat"), Station, Failure));
	TestFalse(TEXT("An unknown module is refused"), Construction->RequestBuild(TEXT("NoSuchModule"), Station, Failure));

	const auto BuildOne = [&](const FName ModuleId) -> AAPSColonyModule*
	{
		FText OrderFailure;
		if (!Construction->RequestBuild(ModuleId, Station, OrderFailure))
		{
			AddError(FString::Printf(TEXT("%s order refused: %s"), *ModuleId.ToString(), *OrderFailure.ToString()));
			return nullptr;
		}
		Production->AdvanceProduction(1000.0);
		Construction->MaterializeFinishedJobs();
		TArray<AAPSColonyModule*> Modules;
		Construction->GetBuiltModules(EAPSSpawnSite::Orbit, Modules);
		AAPSColonyModule* Last = Modules.IsEmpty() ? nullptr : Modules.Last();
		return Last && Last->GetModuleId() == ModuleId ? Last : nullptr;
	};

	AAPSColonyModule* Beacon = BuildOne(TEXT("NavBeacon"));
	AAPSColonyModule* Pods = BuildOne(TEXT("CargoPods"));
	if (TestNotNull(TEXT("The beacon stands"), Beacon) && TestNotNull(TEXT("The cargo pods stand"), Pods))
	{
		TestTrue(TEXT("Modules are bolted to the station"),
			Beacon->GetAttachParentActor() == Station && Pods->GetAttachParentActor() == Station);
		TestTrue(TEXT("Modules have parts"), Beacon->GetPartCount() > 3 && Pods->GetPartCount() > 3);
		TestTrue(TEXT("Modules carry the civilization"), Beacon->GetOwnerCivilizationId() == CivilizationId);
		const double BeaconToPods = FVector::Distance(Beacon->GetActorLocation(), Pods->GetActorLocation());
		TestTrue(*FString::Printf(TEXT("The second module keeps clear of the first (%.0f cm)"), BeaconToPods),
			BeaconToPods > 1500.0);
		const FVector Local = Station->GetActorTransform().InverseTransformPosition(Beacon->GetActorLocation());
		TestTrue(*FString::Printf(TEXT("The beacon stands outside the station's footprint (%s)"), *Local.ToString()),
			FMath::Abs(Local.X) > 4000.0 || FMath::Abs(Local.Y) > 2750.0);
		TestTrue(TEXT("The beacon's front faces the station"), FVector::DotProduct(
			Beacon->GetActorForwardVector(), (Station->GetActorLocation() - Beacon->GetActorLocation()).GetSafeNormal2D())
			> 0.9);
	}
	FAPSProductionSnapshot Snapshot;
	TestTrue(TEXT("The orbit queue is readable"), Construction->GetSiteSnapshot(EAPSSpawnSite::Orbit, Snapshot));
	TestEqual(TEXT("Both orders are jobs"), Snapshot.Jobs.Num(), 2);
	for (const FAPSProductionJobSnapshot& Job : Snapshot.Jobs)
	{
		TestEqual(TEXT("Each job resolved as built"), Job.State, EAPSProductionJobState::Succeeded);
	}
	const int32 BuiltEvents = Published.FilterByPredicate([](const FAPSProductionEvent& Event)
	{
		return Event.Verb == FName(TEXT("APS.Building.Build")) && Event.Result == EAPSProductionEventResult::Succeeded;
	}).Num();
	TestEqual(TEXT("Production published one committed build per module"), BuiltEvents, 2);

	Events->OnEventPublished().Remove(EventHandle);
	DestroyTestWorld();
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
