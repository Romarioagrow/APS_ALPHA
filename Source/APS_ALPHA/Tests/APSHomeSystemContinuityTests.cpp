#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "APS_ALPHA/Actors/Astro/StarCluster.h"
#include "APS_ALPHA/Core/Enums/StarType.h"
#include "APS_ALPHA/Core/Structs/StarGenerationModel.h"
#include "APS_ALPHA/Core/Structs/StarSystemGenerationModel.h"
#include "APS_ALPHA/Generation/AstroGenerator.h"
#include "APS_ALPHA/Generation/StarGenerator.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Engine/World.h"

namespace APSHomeSystemContinuityTests
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

	bool NearlyEqualColor(const FLinearColor& A, const FLinearColor& B)
	{
		return FMath::IsNearlyEqual(A.R, B.R)
			&& FMath::IsNearlyEqual(A.G, B.G)
			&& FMath::IsNearlyEqual(A.B, B.B)
			&& FMath::IsNearlyEqual(A.A, B.A);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FAPSHomeSystemContinuityTest,
	"APS.Gameplay.Generation.HomeSystemContinuity",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAPSHomeSystemContinuityTest::RunTest(const FString& Parameters)
{
	FClusterStarSystemRecord Record;
	Record.StableId = FGuid(0x10203040, 0x50607080, 0x90A0B0C0, 0xD0E0F001);
	Record.InstanceIndex = 37;
	Record.ClusterLocalLocation = FVector(1.25e8, -8.5e7, 3.75e7);
	Record.bMaterialized = true;
	Record.PrimaryStarModel.SpectralClass = ESpectralClass::A;
	Record.PrimaryStarModel.SpectralSubclass = 0;
	Record.SystemModel.StarSystemType = EStarType::SingleStar;
	Record.SystemModel.AmountOfStars = 1;

	FStarModel MaterializedPrimary;
	MaterializedPrimary.Location = FVector(-9.0e11, 8.0e11, 7.0e11);
	MaterializedPrimary.SpectralClass = ESpectralClass::M;
	MaterializedPrimary.SpectralSubclass = 7;
	MaterializedPrimary.StellarType = EStellarType::Giant;
	MaterializedPrimary.Radius = 4.5f;
	MaterializedPrimary.Luminosity = 72.0f;

	// This deliberately differs from the default/UI single-star choice. It covers
	// the bRandomHomeSystem regression where the selected catalogue record was
	// overwritten with HomeSystemStarType instead of its actual generated model.
	FStarSystemModel MaterializedSystem;
	MaterializedSystem.Location = FVector(-9.0e11, 8.0e11, 7.0e11);
	MaterializedSystem.StableId = FGuid(0xDEADBEEF, 0xABCD1234, 0x55667788, 0x99AABBCC);
	MaterializedSystem.StarSystemType = EStarType::TripleStar;
	MaterializedSystem.AmountOfStars = 3;
	MaterializedSystem.GenerationSeed = 884422;
	MaterializedSystem.PotentialPlanetCount = 9;
	MaterializedSystem.bHasPlanetarySystem = true;

	const FGuid OriginalStableId = Record.StableId;
	const int32 OriginalInstanceIndex = Record.InstanceIndex;
	const FVector OriginalLocalAnchor = Record.ClusterLocalLocation;
	const FLinearColor OldGlyphColor = UStarGenerator::GetStarColor(
		Record.PrimaryStarModel.SpectralClass, Record.PrimaryStarModel.SpectralSubclass);

	AAstroGenerator::SynchronizeHomeClusterRecord(
		Record, MaterializedPrimary, MaterializedSystem);

	TestEqual(TEXT("Stable cluster identity survives materialization"),
		Record.StableId, OriginalStableId);
	TestEqual(TEXT("HISM instance address survives materialization"),
		Record.InstanceIndex, OriginalInstanceIndex);
	TestEqual(TEXT("Cluster-local glyph anchor survives materialization"),
		Record.ClusterLocalLocation, OriginalLocalAnchor);
	TestEqual(TEXT("Primary-star model remains anchored to the HISM record"),
		Record.PrimaryStarModel.Location, OriginalLocalAnchor);
	TestEqual(TEXT("Primary-star spectral class follows the materialized star"),
		Record.PrimaryStarModel.SpectralClass, MaterializedPrimary.SpectralClass);
	TestEqual(TEXT("Primary-star spectral subclass follows the materialized star"),
		Record.PrimaryStarModel.SpectralSubclass, MaterializedPrimary.SpectralSubclass);
	TestTrue(TEXT("Primary-star radius follows the materialized star"),
		FMath::IsNearlyEqual(Record.PrimaryStarModel.Radius, MaterializedPrimary.Radius));
	TestTrue(TEXT("Primary-star luminosity follows the materialized star"),
		FMath::IsNearlyEqual(
			Record.PrimaryStarModel.Luminosity, MaterializedPrimary.Luminosity));
	TestEqual(TEXT("System model remains anchored to the HISM record"),
		Record.SystemModel.Location, OriginalLocalAnchor);
	TestEqual(TEXT("System-model identity follows the stable cluster record"),
		Record.SystemModel.StableId, OriginalStableId);
	TestEqual(TEXT("Actual randomized stellar multiplicity is retained"),
		Record.SystemModel.StarSystemType, EStarType::TripleStar);
	TestEqual(TEXT("Actual randomized star count is retained"),
		Record.SystemModel.AmountOfStars, 3);
	TestEqual(TEXT("Deterministic materialization seed is retained"),
		Record.SystemModel.GenerationSeed, 884422);
	TestEqual(TEXT("Potential planet summary is retained"),
		Record.SystemModel.PotentialPlanetCount, 9);
	TestTrue(TEXT("Planetary-system summary is retained"),
		Record.SystemModel.bHasPlanetarySystem);
	TestTrue(TEXT("Materialized state is not reset while models synchronize"),
		Record.bMaterialized);

	const FLinearColor ExpectedGlyphColor = UStarGenerator::GetStarColor(
		MaterializedPrimary.SpectralClass, MaterializedPrimary.SpectralSubclass);
	const FLinearColor SyncedGlyphColor = UStarGenerator::GetStarColor(
		Record.PrimaryStarModel.SpectralClass, Record.PrimaryStarModel.SpectralSubclass);
	TestTrue(TEXT("Cluster glyph color resolves from the materialized primary star"),
		APSHomeSystemContinuityTests::NearlyEqualColor(
			SyncedGlyphColor, ExpectedGlyphColor));
	TestFalse(TEXT("Cluster glyph no longer keeps its pre-materialization color"),
		APSHomeSystemContinuityTests::NearlyEqualColor(
			SyncedGlyphColor, OldGlyphColor));

	UWorld* World = APSHomeSystemContinuityTests::CreateWorld();
	if (!TestNotNull(TEXT("Anchor test world"), World))
	{
		return false;
	}

	AStarCluster* Cluster = World->SpawnActor<AStarCluster>();
	if (!TestNotNull(TEXT("Anchor test cluster"), Cluster))
	{
		APSHomeSystemContinuityTests::DestroyWorld(World);
		return false;
	}

	Cluster->GenerationSeed = 7321;
	Cluster->SetActorTransform(FTransform(
		FRotator(12.0, 37.0, -8.0), FVector(7.0e9, -4.0e9, 2.0e9), FVector(0.17)));
	const FVector LocalGlyphLocation(450000.0, -275000.0, 125000.0);
	const int32 GlyphIndex = Cluster->StarMeshInstances->AddInstance(
		FTransform(LocalGlyphLocation), false);
	Cluster->RegisterPotentialSystem(
		GlyphIndex, LocalGlyphLocation, MaterializedPrimary, MaterializedSystem);

	const FClusterStarSystemRecord* RegisteredRecord =
		Cluster->FindPotentialSystem(GlyphIndex);
	if (TestNotNull(TEXT("Registered home-system record"), RegisteredRecord))
	{
		FTransform GlyphWorldTransform;
		const bool bHasGlyphTransform = Cluster->StarMeshInstances->GetInstanceTransform(
			GlyphIndex, GlyphWorldTransform, true);
		TestTrue(TEXT("Rendered HISM glyph exposes a world transform"), bHasGlyphTransform);
		if (bHasGlyphTransform)
		{
			const FVector RecordWorldAnchor =
				Cluster->GetPotentialSystemWorldLocation(*RegisteredRecord);
			TestTrue(TEXT("Hierarchy overlay anchor exactly follows the rendered HISM glyph"),
				RecordWorldAnchor.Equals(GlyphWorldTransform.GetLocation(), 0.01));
		}
	}

	APSHomeSystemContinuityTests::DestroyWorld(World);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
