#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "APS_ALPHA/Core/Planetary/APSWorldScapeFoliagePolicy.h"
#include "Engine/StaticMesh.h"
#include "WorldScapeFoliages/Public/WorldScapeFoliagesAsset.h"
#include "WorldScapeFoliages/Public/WorldScapeFoliagesBlueprint.h"
#include "WorldScapeFoliages/Public/WorldScapeFoliagesCluster.h"
#include "WorldScapeFoliages/Public/WorldScapeFoliagesCollection.h"

namespace APSWorldScapeFoliagePolicyTests
{
	FAPSResolvedPlanetSurfaceProfile MakeOptedInProfile()
	{
		FAPSResolvedPlanetSurfaceProfile Profile;
		Profile.PlanetType = EPlanetType::Forest;
		Profile.Archetype = EAPSPlanetSurfaceArchetype::Biosphere;
		Profile.Biomass = 0.9f;
		Profile.Biodiversity = 0.8f;
		Profile.Humidity = 0.8f;
		Profile.Foliage.bEnabled = true;
		Profile.Foliage.Collections.Add(TSoftObjectPtr<UWorldScapeFoliagesCollection>(
			FSoftObjectPath(TEXT("/Game/APS/Tests/DA_Foliage.DA_Foliage"))));
		return Profile;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FAPSWorldScapeFoliageActivationGatesTest,
	"APS.Gameplay.World.PlanetSurface.Foliage.ActivationGates",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAPSWorldScapeFoliageActivationGatesTest::RunTest(const FString& Parameters)
{
	const FAPSPlanetSurfaceArchetypeDefinition NativeBiosphere =
		UAPSPlanetSurfaceProfileResolver::GetNativeDefinition(
			EAPSPlanetSurfaceArchetype::Biosphere);
	TestFalse(TEXT("Native living-world definition remains off"), NativeBiosphere.Foliage.bEnabled);

	FAPSResolvedPlanetSurfaceProfile Profile;
	FAPSFoliageActivationPlan Plan = FAPSWorldScapeFoliagePolicy::BuildActivationPlan(
		Profile, true, false);
	TestFalse(TEXT("Native/default profile is off"), Plan.bEnabled);
	FAPSResolvedPlanetSurfaceProfile AppliedProfile;
	FAPSResolvedPlanetSurfaceProfile RequestedProfile;
	TestFalse(TEXT("Non-foliage profiles retain the existing safe live-edit path"),
		FAPSWorldScapeFoliagePolicy::RequiresFreshRootForProfileTransition(
			AppliedProfile, RequestedProfile, false));
	RequestedProfile.Foliage.bEnabled = true;
	TestTrue(TEXT("Opting into foliage requires a fresh root"),
		FAPSWorldScapeFoliagePolicy::RequiresFreshRootForProfileTransition(
			AppliedProfile, RequestedProfile, false));
	RequestedProfile.Foliage.bEnabled = false;
	AppliedProfile.Foliage.bEnabled = true;
	TestTrue(TEXT("Leaving a foliage profile requires a fresh root"),
		FAPSWorldScapeFoliagePolicy::RequiresFreshRootForProfileTransition(
			AppliedProfile, RequestedProfile, false));
	AppliedProfile.Foliage.bEnabled = false;
	TestTrue(TEXT("An activated foliage root rejects in-place profile mutation"),
		FAPSWorldScapeFoliagePolicy::RequiresFreshRootForProfileTransition(
			AppliedProfile, RequestedProfile, true));

	Profile = APSWorldScapeFoliagePolicyTests::MakeOptedInProfile();
	Plan = FAPSWorldScapeFoliagePolicy::BuildActivationPlan(Profile, false, false);
	TestFalse(TEXT("Profile opt-in cannot bypass the runtime kill switch"), Plan.bEnabled);

	Plan = FAPSWorldScapeFoliagePolicy::BuildActivationPlan(Profile, true, true);
	TestFalse(TEXT("Scaled orbital/menu preview is always vetoed"), Plan.bEnabled);

	Profile.PlanetType = EPlanetType::GasGiant;
	Plan = FAPSWorldScapeFoliagePolicy::BuildActivationPlan(Profile, true, false);
	TestFalse(TEXT("Unsupported gaseous bodies are always vetoed"), Plan.bEnabled);

	Profile = APSWorldScapeFoliagePolicyTests::MakeOptedInProfile();
	Plan = FAPSWorldScapeFoliagePolicy::BuildActivationPlan(Profile, true, false);
	TestTrue(TEXT("Both explicit keys enable a full-scale living profile"), Plan.bEnabled);
	TestEqual(TEXT("Default collection budget is conservative"), Plan.MaxCollections, 1);
	TestEqual(TEXT("Default type budget is conservative"), Plan.MaxTypesPerCollection, 3);
	TestEqual(TEXT("Default per-sector collection budget is conservative"),
		Plan.MaxInstancesPerSectorPerCollection, 128);
	TestEqual(TEXT("Default cluster mesh budget is conservative"),
		Plan.MaxClusterMeshesPerType, 2);
	TestEqual(TEXT("Default sector floor is 120 metres"), Plan.MinSectorSizeCm, 12000.0f);
	const uint32 BaseSignature =
		UAPSPlanetSurfaceProfileResolver::BuildProfileSignature(Profile);
	FAPSResolvedPlanetSurfaceProfile SignatureVariant = Profile;
	SignatureVariant.Biodiversity += 0.01f;
	TestNotEqual(TEXT("Biodiversity participates in foliage profile identity"),
		BaseSignature,
		UAPSPlanetSurfaceProfileResolver::BuildProfileSignature(SignatureVariant));
	SignatureVariant = Profile;
	SignatureVariant.Foliage.MaxInstancesPerSectorPerCollection++;
	TestNotEqual(TEXT("Foliage budgets participate in profile identity"),
		BaseSignature,
		UAPSPlanetSurfaceProfileResolver::BuildProfileSignature(SignatureVariant));
	SignatureVariant = Profile;
	SignatureVariant.Foliage.Collections.Add(
		TSoftObjectPtr<UWorldScapeFoliagesCollection>(
			FSoftObjectPath(TEXT("/Game/APS/Tests/DA_Foliage_B.DA_Foliage_B"))));
	const uint32 OrderedCollectionsSignature =
		UAPSPlanetSurfaceProfileResolver::BuildProfileSignature(SignatureVariant);
	Swap(SignatureVariant.Foliage.Collections[0], SignatureVariant.Foliage.Collections[1]);
	TestNotEqual(TEXT("Ordered collection paths participate in profile identity"),
		OrderedCollectionsSignature,
		UAPSPlanetSurfaceProfileResolver::BuildProfileSignature(SignatureVariant));

	Profile.Foliage.MaxCollections = 99;
	Profile.Foliage.MaxTypesPerCollection = 99;
	Profile.Foliage.MaxInstancesPerSectorPerCollection = 100000;
	Profile.Foliage.MaxClusterMeshesPerType = 99;
	Profile.Foliage.MinSectorSizeCm = 1.0f;
	Profile.Foliage.MaxCullDistanceMultiplier = 99.0f;
	Plan = FAPSWorldScapeFoliagePolicy::BuildActivationPlan(Profile, true, false);
	TestEqual(TEXT("Collection budget has a hard ceiling"), Plan.MaxCollections, 2);
	TestEqual(TEXT("Type budget has a hard ceiling"), Plan.MaxTypesPerCollection, 6);
	TestEqual(TEXT("Instance budget has a hard ceiling"),
		Plan.MaxInstancesPerSectorPerCollection, 512);
	TestEqual(TEXT("Cluster mesh budget has a hard ceiling"), Plan.MaxClusterMeshesPerType, 4);
	TestEqual(TEXT("Sector size has a hard floor"), Plan.MinSectorSizeCm, 2000.0f);
	TestEqual(TEXT("Cull multiplier has a hard ceiling"), Plan.MaxCullDistanceMultiplier, 1.5f);

	Profile.Biomass = Profile.Foliage.MinimumBiomass;
	Profile.Biodiversity = 0.0f;
	Plan = FAPSWorldScapeFoliagePolicy::BuildActivationPlan(Profile, true, false);
	TestFalse(TEXT("Insufficient habitat signal allocates no foliage"), Plan.bEnabled);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FAPSWorldScapeFoliageTransientBudgetTest,
	"APS.Gameplay.World.PlanetSurface.Foliage.TransientHISMBudget",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAPSWorldScapeFoliageTransientBudgetTest::RunTest(const FString& Parameters)
{
	UWorldScapeFoliagesCollection* Source =
		NewObject<UWorldScapeFoliagesCollection>(GetTransientPackage());
	UStaticMesh* Mesh = NewObject<UStaticMesh>(GetTransientPackage());
	if (!TestNotNull(TEXT("Source collection"), Source)
		|| !TestNotNull(TEXT("Test mesh"), Mesh))
	{
		return false;
	}

	UWorldScapeFoliagesAsset* Asset = NewObject<UWorldScapeFoliagesAsset>(Source);
	Asset->StaticMesh = Mesh;
	Asset->FoliagesCount = 1000.0f;
	Asset->FoliageSectorSize = 1000.0;
	Asset->bUsePoissonDisc = true;
	Asset->bCollision = true;
	Asset->Is_NaniteMesh = true;
	Asset->bCastShadows = true;
	Asset->FoliageCullDistanceMultiplier = 4.0f;
	Source->FoliageList.Add(Asset);

	UWorldScapeFoliagesBlueprint* ActorEntry =
		NewObject<UWorldScapeFoliagesBlueprint>(Source);
	ActorEntry->FoliagesCount = 1000.0f;
	Source->FoliageList.Add(ActorEntry);
	UWorldScapeFoliagesAsset* ActorModeAsset =
		NewObject<UWorldScapeFoliagesAsset>(Source);
	ActorModeAsset->StaticMesh = Mesh;
	ActorModeAsset->FoliagesCount = 1000.0f;
	ActorModeAsset->bSpawnActorInstead = true;
	Source->FoliageList.Add(ActorModeAsset);

	UWorldScapeFoliagesCluster* Cluster = NewObject<UWorldScapeFoliagesCluster>(Source);
	Cluster->FoliagesCount = 1000.0f;
	for (int32 Index = 0; Index < 2; ++Index)
	{
		FWorldScapeFoliagesClusterUnit Unit;
		Unit.StaticMesh = Mesh;
		Unit.Is_NaniteMesh = true;
		Unit.bCollision = true;
		Unit.bCastShadows = true;
		Unit.ClusterMin = 3;
		Unit.ClusterMax = 10;
		Unit.FoliageCullDistanceMultiplier = 3.0f;
		Cluster->FoliagesClusterUnitList.Add(Unit);
	}
	Source->FoliageList.Add(Cluster);

	FAPSFoliageActivationPlan Plan;
	Plan.bEnabled = true;
	Plan.HabitatDensityScale = 1.0f;
	Plan.MaxCollections = 1;
	Plan.MaxTypesPerCollection = 4;
	Plan.MaxInstancesPerSectorPerCollection = 40;
	Plan.MaxClusterMeshesPerType = 2;
	Plan.MinSectorSizeCm = 8000.0f;
	Plan.MaxCullDistanceMultiplier = 0.8f;
	Plan.bUseNoiseMask = true;
	Plan.bCastShadows = false;

	TArray<UWorldScapeFoliagesCollection*> Sources;
	Sources.Add(Source);
	TArray<UWorldScapeFoliagesCollection*> Budgeted;
	const int32 BuiltCount = FAPSWorldScapeFoliagePolicy::BuildBudgetedCollections(
		GetTransientPackage(), Sources, Plan, Budgeted);
	if (!TestEqual(TEXT("One supported collection is cloned"), BuiltCount, 1)
		|| !TestTrue(TEXT("Cloned collection is present"), Budgeted.IsValidIndex(0)))
	{
		return false;
	}

	UWorldScapeFoliagesCollection* Copy = Budgeted[0];
	TestNotEqual(TEXT("Authored collection is never mutated in place"), Copy, Source);
	TestTrue(TEXT("Runtime collection is transient"), Copy->HasAnyFlags(RF_Transient));
	TestEqual(TEXT("Actor-mode and Blueprint foliage are rejected"), Copy->FoliageList.Num(), 2);
	TestTrue(TEXT("Authored asset retains collision"), Asset->bCollision);
	TestTrue(TEXT("Authored asset retains its ISM flag"), Asset->Is_NaniteMesh);
	TestTrue(TEXT("Authored asset retains Poisson sampling"), Asset->bUsePoissonDisc);

	UWorldScapeFoliagesAsset* BudgetedAsset = nullptr;
	UWorldScapeFoliagesCluster* BudgetedCluster = nullptr;
	for (UWorldScapeFoliagesInterface* Entry : Copy->FoliageList)
	{
		if (Entry->GetClass() == UWorldScapeFoliagesAsset::StaticClass())
		{
			BudgetedAsset = static_cast<UWorldScapeFoliagesAsset*>(Entry);
		}
		else if (Entry->GetClass() == UWorldScapeFoliagesCluster::StaticClass())
		{
			BudgetedCluster = static_cast<UWorldScapeFoliagesCluster*>(Entry);
		}
	}

	if (TestNotNull(TEXT("Budgeted mesh entry"), BudgetedAsset))
	{
		TestFalse(TEXT("Runtime mesh entry disables collision"), BudgetedAsset->bCollision);
		TestFalse(TEXT("Runtime mesh entry forces WorldScape HISM"), BudgetedAsset->Is_NaniteMesh);
		TestFalse(TEXT("Runtime mesh entry disables Poisson sampling"), BudgetedAsset->bUsePoissonDisc);
		TestFalse(TEXT("Runtime mesh entry disables shadows by profile"), BudgetedAsset->bCastShadows);
		TestTrue(TEXT("Runtime mesh entry uses the APS foliage mask"),
			BudgetedAsset->bUseFoliageNoiseMask);
		TestTrue(TEXT("Runtime mesh entry respects the type budget"),
			BudgetedAsset->FoliagesCount <= 20.0f);
		TestTrue(TEXT("Runtime sector size respects the floor"),
			BudgetedAsset->FoliageSectorSize >= 8000.0);
		TestTrue(TEXT("Runtime cull multiplier respects the ceiling"),
			BudgetedAsset->FoliageCullDistanceMultiplier <= 0.8f);
	}

	if (TestNotNull(TEXT("Budgeted cluster entry"), BudgetedCluster))
	{
		int32 Expansion = 0;
		for (const FWorldScapeFoliagesClusterUnit& Unit :
			BudgetedCluster->FoliagesClusterUnitList)
		{
			Expansion += Unit.ClusterMax;
			TestFalse(TEXT("Cluster unit disables collision"), Unit.bCollision);
			TestFalse(TEXT("Cluster unit forces WorldScape HISM"), Unit.Is_NaniteMesh);
			TestFalse(TEXT("Cluster unit disables shadows by profile"), Unit.bCastShadows);
		}
		TestTrue(TEXT("Cluster-expanded instances respect the type budget"),
			BudgetedCluster->FoliagesCount * static_cast<float>(Expansion) <= 20.0f);
	}

	FAPSFoliageActivationPlan InvalidPlan = Plan;
	InvalidPlan.HabitatDensityScale = -1.0f;
	InvalidPlan.MaxCollections = MIN_int32;
	InvalidPlan.MaxTypesPerCollection = MIN_int32;
	InvalidPlan.MaxInstancesPerSectorPerCollection = MIN_int32;
	TArray<UWorldScapeFoliagesCollection*> InvalidOutput;
	TestEqual(TEXT("Invalid public budget input is rejected without allocation"),
		FAPSWorldScapeFoliagePolicy::BuildBudgetedCollections(
			GetTransientPackage(), Sources, InvalidPlan, InvalidOutput),
		0);
	TestTrue(TEXT("Rejected public budget input leaves no transient collections"),
		InvalidOutput.IsEmpty());

	FAPSFoliageActivationPlan OversizedPlan = Plan;
	OversizedPlan.HabitatDensityScale = 5.0f;
	OversizedPlan.MaxCollections = MAX_int32;
	OversizedPlan.MaxTypesPerCollection = MAX_int32;
	OversizedPlan.MaxInstancesPerSectorPerCollection = MAX_int32;
	OversizedPlan.MaxClusterMeshesPerType = MAX_int32;
	OversizedPlan.MinSectorSizeCm = -1000.0f;
	OversizedPlan.MaxCullDistanceMultiplier = 1000.0f;
	TArray<UWorldScapeFoliagesCollection*> NormalizedOutput;
	if (TestEqual(TEXT("Oversized public budget input is normalized"),
		FAPSWorldScapeFoliagePolicy::BuildBudgetedCollections(
			GetTransientPackage(), Sources, OversizedPlan, NormalizedOutput),
		1)
		&& TestTrue(TEXT("Normalized collection is present"), NormalizedOutput.IsValidIndex(0)))
	{
		for (UWorldScapeFoliagesInterface* Entry : NormalizedOutput[0]->FoliageList)
		{
			TestTrue(TEXT("Normalized entry uses the hard sector floor"),
				Entry->FoliageSectorSize >= 2000.0);
			TestTrue(TEXT("Normalized entry count cannot exceed the hard collection cap"),
				Entry->FoliagesCount <= 512.0f);
			if (const UWorldScapeFoliagesAsset* NormalizedAsset =
				Cast<UWorldScapeFoliagesAsset>(Entry))
			{
				TestTrue(TEXT("Normalized asset cull distance has a hard ceiling"),
					NormalizedAsset->FoliageCullDistanceMultiplier <= 1.5f);
			}
		}
	}

	return true;
}

#endif
