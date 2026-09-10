#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "APS_ALPHA/Actors/Astro/Moon.h"
#include "APS_ALPHA/Actors/Astro/Planet.h"
#include "APS_ALPHA/Actors/Astro/PlanetOrbit.h"
#include "APS_ALPHA/Core/Enums/PlanetType.h"
#include "APS_ALPHA/Core/Enums/PlanetHabitability.h"
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
	Model->GenerationSeed = 884422;
	const int32 AutoPlanetSeed = UGeneratedWorld::ResolveCanonicalSurfaceSeed(
		0, Model->GenerationSeed, TEXT("SYS0/S0/P0"));
	TestTrue(TEXT("Auto surface seed resolves to a persisted positive value"),
		AutoPlanetSeed > 0 && AutoPlanetSeed <= 999983);
	TestEqual(TEXT("Auto surface seed is deterministic for one world/body identity"),
		UGeneratedWorld::ResolveCanonicalSurfaceSeed(
			0, Model->GenerationSeed, TEXT("SYS0/S0/P0")), AutoPlanetSeed);
	TestNotEqual(TEXT("Auto surface seed distinguishes sibling body paths"),
		UGeneratedWorld::ResolveCanonicalSurfaceSeed(
			0, Model->GenerationSeed, TEXT("SYS0/S0/P1")), AutoPlanetSeed);
	TestEqual(TEXT("Explicit surface seed bypasses automatic derivation"),
		UGeneratedWorld::ResolveCanonicalSurfaceSeed(
			31337, Model->GenerationSeed, TEXT("SYS0/S0/P0")), 31337);
	FAPSPreviewBodyEditOverride PlanetA;
	PlanetA.PlanetType = EPlanetType::HighMountain;
	PlanetA.PlanetHabitability = EPlanetHabitability::Habitable;
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
	PlanetA.MoonCount = 3;

	FAPSPreviewBodyEditOverride MoonB;
	MoonB.PlanetType = EPlanetType::Frozen;
	MoonB.PlanetHabitability = EPlanetHabitability::PotentiallyHabitable;
	MoonB.RadiusKm = 920.0;
	MoonB.MoonOrbitRadiusKm = 24000.0;
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
		TestEqual(TEXT("Initial A snapshot keeps habitability independently"),
			InitialASnapshot->PlanetHabitability, EPlanetHabitability::Habitable);
		TestEqual(TEXT("Initial A snapshot owns its edited moon count"),
			InitialASnapshot->MoonCount, 3);
	}

	// Hydrating/editing B adds a second path instead of replacing A's shared state.
	Model->SetPreviewBodyEditOverride(MoonKey, MoonB);
	TestEqual(TEXT("Two independently edited bodies are retained"),
		Model->GetPreviewBodyEditOverrideCount(), 2);
	const FAPSPreviewBodyEditOverride* StoredMoonOverride =
		Model->FindPreviewBodyEditOverride(MoonKey);
	if (TestNotNull(TEXT("Moon orbit edit is retained by stable path"), StoredMoonOverride))
	{
		TestEqual(TEXT("Moon center-radius persists exactly"),
			StoredMoonOverride->MoonOrbitRadiusKm, MoonB.MoonOrbitRadiusKm);
	}

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
	TestEqual(TEXT("Planet A keeps explicit habitability"),
		RebuiltPlanet->PlanetHabitability, EPlanetHabitability::Habitable);
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
		TestEqual(TEXT("Planet A model keeps habitability"),
			RebuiltPlanet->PlanetData.PlanetModel->PlanetHabitability,
			EPlanetHabitability::Habitable);
	}

	TestEqual(TEXT("Moon B keeps its type"), RebuiltMoon->PlanetType, EPlanetType::Frozen);
	TestEqual(TEXT("Moon B keeps explicit habitability"),
		RebuiltMoon->PlanetHabitability, EPlanetHabitability::PotentiallyHabitable);
	TestEqual(TEXT("Moon B generic body data keeps explicit habitability"),
		RebuiltMoon->PlanetData.PlanetHabitability,
		EPlanetHabitability::PotentiallyHabitable);
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
		TestEqual(TEXT("Moon B model keeps habitability"),
			RebuiltMoon->GenerationModel->PlanetHabitability,
			EPlanetHabitability::PotentiallyHabitable);
	}

	// Parent-radius edits are local to the planet. The attached orbit hierarchy is
	// presentation/kinematics state and must not inherit that physical-size edit.
	APlanetOrbit* RebuiltMoonOrbit = World->SpawnActor<APlanetOrbit>();
	if (TestNotNull(TEXT("Rebuilt moon orbit"), RebuiltMoonOrbit))
	{
		RebuiltPlanet->SetActorLocation(FVector(1500.0, -800.0, 320.0));
		RebuiltPlanet->SetActorScale3D(FVector(2.0));
		RebuiltMoonOrbit->SetActorLocation(RebuiltPlanet->GetActorLocation());
		RebuiltMoonOrbit->SetActorRotation(FRotator(17.0, 29.0, -11.0));
		RebuiltMoonOrbit->AttachToActor(
			RebuiltPlanet, FAttachmentTransformRules::KeepWorldTransform);
		RebuiltPlanet->MoonOrbitsList.Add(RebuiltMoonOrbit);

		const FVector OrbitDirection = RebuiltMoonOrbit->GetActorQuat().GetAxisY();
		RebuiltMoon->SetActorLocation(
			RebuiltMoonOrbit->GetActorLocation() + OrbitDirection * 175000.0);
		RebuiltMoon->SetActorScale3D(FVector(0.375));
		RebuiltMoon->AttachToActor(
			RebuiltMoonOrbit, FAttachmentTransformRules::KeepWorldTransform);
		RebuiltMoon->SetParentPlanet(RebuiltPlanet);
		RebuiltPlanet->Moons.AddUnique(RebuiltMoon);
		if (TestTrue(TEXT("Rebuilt planet model exists for moon-orbit propagation"),
			RebuiltPlanet->PlanetData.PlanetModel.IsValid())
			&& TestTrue(TEXT("Rebuilt moon model exists for moon-orbit propagation"),
				RebuiltMoon->GenerationModel.IsValid()))
		{
			FPlanetModel& ParentModel = *RebuiltPlanet->PlanetData.PlanetModel;
			ParentModel.MoonsList.Reset();
			ParentModel.MoonsList.Add(MakeShared<FMoonData>(
				1, 0.25, RebuiltMoon->GenerationModel));
			ParentModel.MoonsListData = ParentModel.GetMoonsData();
			RebuiltPlanet->PlanetData.PlanetModelData = ParentModel;

			const FTransform OrbitRootBeforeOrbitEdit = RebuiltMoonOrbit->GetActorTransform();
			const FVector MoonScaleBeforeOrbitEdit = RebuiltMoon->GetActorScale3D();
			const FVector PlanetScaleBeforeOrbitEdit = RebuiltPlanet->GetActorScale3D();
			TestTrue(TEXT("Moon center-radius override reapplies after hierarchy materialization"),
				AAstroGenerator::ApplyPreviewBodyEditOverrideByKey(
					Model, MoonKey, RebuiltMoon));

			const double ExpectedOrbitRatio =
				MoonB.MoonOrbitRadiusKm / PlanetA.RadiusKm - 1.0;
			const TSharedPtr<FMoonData>& AppliedMoonData = ParentModel.MoonsList[0];
			TestTrue(TEXT("Moon center-radius reaches the parent moon model"),
				FMath::IsNearlyEqual(AppliedMoonData->OrbitRadius, ExpectedOrbitRatio, 1.0e-5));
			TestTrue(TEXT("Moon shared model stores the same orbit ratio"),
				AppliedMoonData->MoonModel.IsValid()
				&& FMath::IsNearlyEqual(
					AppliedMoonData->MoonModel->OrbitDistance, ExpectedOrbitRatio, 1.0e-5));
			TestTrue(TEXT("Moon actor model stores the same orbit ratio"),
				RebuiltMoon->GenerationModel.IsValid()
				&& FMath::IsNearlyEqual(
					RebuiltMoon->GenerationModel->OrbitDistance, ExpectedOrbitRatio, 1.0e-5));
			TestTrue(TEXT("Moon serial snapshot stores the sanitized orbit ratio"),
				FMath::IsNearlyEqual(
					AppliedMoonData->MoonModelData.OrbitDistance, ExpectedOrbitRatio, 1.0e-5)
				&& ParentModel.MoonsListData.IsValidIndex(0)
				&& FMath::IsNearlyEqual(
					ParentModel.MoonsListData[0].OrbitRadius, ExpectedOrbitRatio, 1.0e-5)
				&& FMath::IsNearlyEqual(
					ParentModel.MoonsListData[0].MoonModelData.OrbitDistance,
					ExpectedOrbitRatio, 1.0e-5));
			TestTrue(TEXT("Moon actor reaches the requested center-radius in centimetres"),
				FMath::IsNearlyEqual(FVector::Distance(
					RebuiltMoonOrbit->GetActorLocation(), RebuiltMoon->GetActorLocation()),
					MoonB.MoonOrbitRadiusKm * 100000.0, 1.0));
			TestTrue(TEXT("Moon orbit edit preserves the orbit-root transform"),
				RebuiltMoonOrbit->GetActorTransform().Equals(OrbitRootBeforeOrbitEdit, 0.001));
			TestTrue(TEXT("Moon orbit edit never changes moon visual scale"),
				RebuiltMoon->GetActorScale3D().Equals(MoonScaleBeforeOrbitEdit, 0.001));
			TestTrue(TEXT("Moon orbit edit never changes parent visual scale"),
				RebuiltPlanet->GetActorScale3D().Equals(PlanetScaleBeforeOrbitEdit, 0.001));
			const FVector AppliedOrbitOffset =
				RebuiltMoon->GetActorLocation() - RebuiltMoonOrbit->GetActorLocation();
			TestTrue(TEXT("Edited moon remains on its orbit plane"),
				FMath::Abs(FVector::DotProduct(AppliedOrbitOffset,
					RebuiltMoonOrbit->GetActorQuat().GetAxisZ()))
					<= FMath::Max(AppliedOrbitOffset.Size() * 1.0e-9, 1.0));
		}

		const FTransform MoonOrbitTransformBefore = RebuiltMoonOrbit->GetActorTransform();
		const FTransform MoonTransformBefore = RebuiltMoon->GetActorTransform();
		const double MoonRadiusBefore = RebuiltMoon->RadiusKM;
		const double MoonOrbitRadiusBefore = FVector::Distance(
			RebuiltMoonOrbit->GetActorLocation(), RebuiltMoon->GetActorLocation());
		const FVector PlanetScaleBefore = RebuiltPlanet->GetActorScale3D();
		const double PlanetRadiusBefore = RebuiltPlanet->RadiusKM;

		const double RequestedPlanetRadius = PlanetRadiusBefore * 1.5;
		AAstroGenerator::ApplyPlanetaryBodyRadius(
			*RebuiltPlanet, RequestedPlanetRadius);

		TestEqual(TEXT("Planet stores the exact requested radius"),
			RebuiltPlanet->RadiusKM, RequestedPlanetRadius);
		TestEqual(TEXT("Planet legacy radius mirrors the requested value"),
			RebuiltPlanet->PlanetRadiusKM,
			static_cast<int32>(FMath::RoundToInt(RequestedPlanetRadius)));
		TestTrue(TEXT("Planet radius edit changes only the planet scale"),
			RebuiltPlanet->GetActorScale3D().Equals(PlanetScaleBefore * 1.5, 0.001));
		TestEqual(TEXT("Moon keeps its own model radius after parent edit"),
			RebuiltMoon->RadiusKM, MoonRadiusBefore);
		TestTrue(TEXT("Moon orbit world transform survives parent edit"),
			RebuiltMoonOrbit->GetActorTransform().Equals(MoonOrbitTransformBefore, 0.001));
		TestTrue(TEXT("Moon world transform survives parent edit"),
			RebuiltMoon->GetActorTransform().Equals(MoonTransformBefore, 0.001));
		TestTrue(TEXT("Moon orbit radius survives parent edit"),
			FMath::IsNearlyEqual(FVector::Distance(
				RebuiltMoonOrbit->GetActorLocation(), RebuiltMoon->GetActorLocation()),
				MoonOrbitRadiusBefore, 0.001));

		FAPSPreviewBodyEditOverride ResizedPlanetOverride = PlanetA;
		ResizedPlanetOverride.RadiusKm = 10000.0;
		Model->SetPreviewBodyEditOverride(PlanetKey, ResizedPlanetOverride);
		const FVector MoonScaleBeforePersistentParentEdit = RebuiltMoon->GetActorScale3D();
		TestTrue(TEXT("Persistent planet-radius edit reapplies through the model path"),
			AAstroGenerator::ApplyPreviewBodyEditOverrideByKey(
				Model, PlanetKey, RebuiltPlanet));
		if (RebuiltPlanet->PlanetData.PlanetModel.IsValid()
			&& RebuiltPlanet->PlanetData.PlanetModel->MoonsList.IsValidIndex(0)
			&& RebuiltPlanet->PlanetData.PlanetModel->MoonsList[0].IsValid())
		{
			const FPlanetModel& ResizedParentModel =
				*RebuiltPlanet->PlanetData.PlanetModel;
			const double PersistedMoonCenterKm =
				static_cast<double>(ResizedParentModel.RadiusKM)
					* (1.0 + ResizedParentModel.MoonsList[0]->OrbitRadius);
			TestTrue(TEXT("Planet model resize preserves the moon center-radius"),
				FMath::IsNearlyEqual(
					PersistedMoonCenterKm, MoonB.MoonOrbitRadiusKm, 0.01));
			TestTrue(TEXT("Planet serial resize preserves the moon center-radius"),
				ResizedParentModel.MoonsListData.IsValidIndex(0)
				&& FMath::IsNearlyEqual(
					static_cast<double>(ResizedParentModel.RadiusKM)
						* (1.0 + ResizedParentModel.MoonsListData[0].OrbitRadius),
					MoonB.MoonOrbitRadiusKm, 0.01));
		}
		else
		{
			AddError(TEXT("Planet resize lost the authoritative moon model"));
		}
		TestTrue(TEXT("Persistent parent edit preserves the actor moon center-radius"),
			FMath::IsNearlyEqual(FVector::Distance(
				RebuiltMoonOrbit->GetActorLocation(), RebuiltMoon->GetActorLocation()),
				MoonOrbitRadiusBefore, 1.0));
		TestTrue(TEXT("Persistent parent edit preserves the moon's own visual scale"),
			RebuiltMoon->GetActorScale3D().Equals(
				MoonScaleBeforePersistentParentEdit, 0.001));
	}

	Model->ClearPreviewBodyEditOverrides();
	TestEqual(TEXT("Explicit regeneration can begin a clean edit generation"),
		Model->GetPreviewBodyEditOverrideCount(), 0);

	APSPreviewBodyEditPersistenceTests::DestroyWorld(World);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FAPSPreviewPhysicalRadiusRetentionTest,
	"APS.Gameplay.Generation.PreviewPhysicalRadiusRetention",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAPSPreviewPhysicalRadiusRetentionTest::RunTest(const FString& Parameters)
{
	UWorld* World = APSPreviewBodyEditPersistenceTests::CreateWorld();
	if (!TestNotNull(TEXT("Test world"), World)) return false;
	UGeneratedWorld* Model = NewObject<UGeneratedWorld>();
	APlanet* Planet = World->SpawnActor<APlanet>();
	if (!TestNotNull(TEXT("Physical radius fixture"), Planet))
	{
		APSPreviewBodyEditPersistenceTests::DestroyWorld(World);
		return false;
	}
	for (const double RadiusKm : {12.375, 6371.125, 25359.25, 89999.75})
	{
		Planet->RadiusKM = RadiusKm;
		Planet->PlanetRadiusKM = FMath::RoundToInt(RadiusKm);
		Planet->SetActorScale3D(FVector(3.25));
		const FTransform Before = Planet->GetActorTransform();
		FAPSPreviewBodyEditOverride Override;
		Override.RadiusKm = RadiusKm;
		Model->SetPreviewBodyEditOverride(TEXT("SYS0/S0/P0"), Override);
		for (int32 Repeat = 0; Repeat < 3; ++Repeat)
		{
			TestTrue(TEXT("Retained radius override is applied"), AAstroGenerator::ApplyPreviewBodyEditOverrideByKey(
				Model, TEXT("SYS0/S0/P0"), Planet));
			TestEqual(TEXT("Physical radius survives replay without rounding or terrestrial limits"), Planet->RadiusKM, RadiusKm);
			TestTrue(TEXT("Unchanged radius cannot rescale its actor"), Planet->GetActorTransform().Equals(Before, 0.0));
			TestEqual(TEXT("Generation model retains the same physical radius"),
				static_cast<double>(Planet->PlanetData.PlanetModel->RadiusKM), RadiusKm);
		}
	}
	APSPreviewBodyEditPersistenceTests::DestroyWorld(World);
	return true;
}

#endif
