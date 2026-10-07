#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "APS_ALPHA/Core/Planetary/APSWorldScapeFoliagePolicy.h"
#include "APS_ALPHA/Core/Planetary/APSPlanetFoliagePrototype.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "HAL/IConsoleManager.h"
#include "WorldScapeCore/Public/WorldScapeRoot.h"
#include "WorldScapeCommon/Public/WorldScapeHelper.h"
#include "WorldScapeFoliages/Public/WorldScapeFoliagesAsset.h"
#include "WorldScapeFoliages/Public/WorldScapeFoliagesBlueprint.h"
#include "WorldScapeFoliages/Public/WorldScapeFoliagesCluster.h"
#include "WorldScapeFoliages/Public/WorldScapeFoliagesCollection.h"

namespace APSWorldScapeFoliagePolicyTests
{
	class FScopedFoliageRuntimeOptIn
	{
	public:
		FScopedFoliageRuntimeOptIn()
		{
			Variable = IConsoleManager::Get().FindConsoleVariable(
				TEXT("aps.WorldScapeFoliage.Enable"));
			if (Variable)
			{
				SavedValue = Variable->GetString();
				SavedFlags = Variable->GetFlags();
				OverrideFlags = static_cast<EConsoleVariableFlags>(
					(SavedFlags & ECVF_SetByMask) | ECVF_Set_SetOnly_Unsafe);
			}
		}

		~FScopedFoliageRuntimeOptIn()
		{
			if (Variable)
			{
				Variable->Set(*SavedValue, OverrideFlags);
			}
		}

		bool IsValid() const { return Variable != nullptr; }
		void Set(const int32 Value) const
		{
			if (Variable)
			{
				Variable->Set(Value, OverrideFlags);
			}
		}

	private:
		IConsoleVariable* Variable{nullptr};
		FString SavedValue;
		EConsoleVariableFlags SavedFlags{ECVF_Default};
		EConsoleVariableFlags OverrideFlags{ECVF_Set_SetOnly_Unsafe};
	};

	UWorld* CreateTestWorld()
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

	void DestroyTestWorld(UWorld*& World)
	{
		if (World)
		{
			World->DestroyWorld(false);
			World = nullptr;
		}
	}

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
	FAPSWorldScapeFoliageCatalogDefaultsTest,
	"APS.Gameplay.World.PlanetSurface.Foliage.CatalogDefaultsOff",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAPSWorldScapeFoliageCatalogDefaultsTest::RunTest(const FString& Parameters)
{
	const EAPSPlanetSurfaceArchetype NativeArchetypes[] =
	{
		EAPSPlanetSurfaceArchetype::Rocky,
		EAPSPlanetSurfaceArchetype::Temperate,
		EAPSPlanetSurfaceArchetype::Oceanic,
		EAPSPlanetSurfaceArchetype::Biosphere,
		EAPSPlanetSurfaceArchetype::Desert,
		EAPSPlanetSurfaceArchetype::Cryogenic,
		EAPSPlanetSurfaceArchetype::Magmatic,
		EAPSPlanetSurfaceArchetype::Metallic,
		EAPSPlanetSurfaceArchetype::ExoticChemical,
	};
	for (const EAPSPlanetSurfaceArchetype Archetype : NativeArchetypes)
	{
		const FAPSPlanetSurfaceArchetypeDefinition Definition =
			UAPSPlanetSurfaceProfileResolver::GetNativeDefinition(Archetype);
		const FString Context = UEnum::GetValueAsString(Archetype);
		TestFalse(*FString::Printf(TEXT("Native %s foliage remains disabled"), *Context),
			Definition.Foliage.bEnabled);
		if (!Definition.Foliage.Collections.IsEmpty())
		{
			AddWarning(FString::Printf(
				TEXT("Native %s now preauthors disabled foliage collections; default-off safety is unchanged"),
				*Context));
		}
	}

	UAPSPlanetSurfaceCatalog* Catalog = LoadObject<UAPSPlanetSurfaceCatalog>(nullptr,
		TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/DA_PlanetSurfaceCatalog.DA_PlanetSurfaceCatalog"));
	if (!TestNotNull(TEXT("Production planet-surface catalog"), Catalog))
	{
		return false;
	}
	TestTrue(TEXT("Production catalog contains explicit archetype definitions"),
		!Catalog->Archetypes.IsEmpty());
	for (const TPair<EAPSPlanetSurfaceArchetype,
		FAPSPlanetSurfaceArchetypeDefinition>& Entry : Catalog->Archetypes)
	{
		const FString Context = UEnum::GetValueAsString(Entry.Key);
		TestFalse(*FString::Printf(TEXT("Catalog %s foliage remains disabled"), *Context),
			Entry.Value.Foliage.bEnabled);
		if (!Entry.Value.Foliage.Collections.IsEmpty())
		{
			AddWarning(FString::Printf(
				TEXT("Catalog %s now preauthors disabled foliage collections; current empty baseline changed"),
				*Context));
		}
	}
	return true;
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
	// Authored palettes may be malformed or migrated from a much larger world.
	// Runtime copies must still honor the absolute sector/cull budget.
	Asset->FoliageSectorSize = 1000000.0;
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
	Cluster->FoliagesCount = 100.0f;
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
	Source->FoliageLayer = 3;
	Source->Temperature.MinValue = 0.2f;
	Source->Temperature.MaxValue = 0.8f;
	Source->Humidity.MinValue = 0.35f;
	Source->Humidity.MaxValue = 0.95f;

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
	Asset->SetFlags(RF_Public | RF_Standalone);
	Cluster->SetFlags(RF_Public | RF_Standalone);
	const int32 BuiltCount = FAPSWorldScapeFoliagePolicy::BuildBudgetedCollections(
		GetTransientPackage(), Sources, Plan, Budgeted);
	TestTrue(TEXT("Authored data-asset ownership flags are not changed"),
		Asset->HasAllFlags(RF_Public | RF_Standalone) && Cluster->HasAllFlags(RF_Public | RF_Standalone));
	// These test-only source objects must not become permanent editor roots.
	Asset->ClearFlags(RF_Public | RF_Standalone);
	Cluster->ClearFlags(RF_Public | RF_Standalone);
	if (!TestEqual(TEXT("One supported collection is cloned"), BuiltCount, 1)
		|| !TestTrue(TEXT("Cloned collection is present"), Budgeted.IsValidIndex(0)))
	{
		return false;
	}

	UWorldScapeFoliagesCollection* Copy = Budgeted[0];
	TestNotEqual(TEXT("Authored collection is never mutated in place"), Copy, Source);
	TestTrue(TEXT("Runtime collection is transient"), Copy->HasAnyFlags(RF_Transient));
	TestEqual(TEXT("Actor-mode and Blueprint foliage are rejected"), Copy->FoliageList.Num(), 2);
	TestFalse(TEXT("Preset mesh palette keeps its temperature biome gate"),
		Copy->Temperature.notEqual(Source->Temperature));
	TestFalse(TEXT("Preset mesh palette keeps its humidity biome gate"),
		Copy->Humidity.notEqual(Source->Humidity));
	TestEqual(TEXT("Preset mesh palette keeps its authored foliage layer"),
		Copy->FoliageLayer, Source->FoliageLayer);
	TestTrue(TEXT("Authored asset retains collision"), Asset->bCollision);
	TestTrue(TEXT("Authored asset retains its ISM flag"), Asset->Is_NaniteMesh);
	TestTrue(TEXT("Authored asset retains Poisson sampling"), Asset->bUsePoissonDisc);

	UWorldScapeFoliagesAsset* BudgetedAsset = nullptr;
	UWorldScapeFoliagesCluster* BudgetedCluster = nullptr;
	for (UWorldScapeFoliagesInterface* Entry : Copy->FoliageList)
	{
		TestTrue(TEXT("Every runtime entry is transient"), Entry->HasAnyFlags(RF_Transient));
		TestFalse(TEXT("Runtime copies cannot retain standalone/public asset ownership"),
			Entry->HasAnyFlags(RF_Public | RF_Standalone));
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
		TestTrue(TEXT("Runtime mesh entry respects the collection budget"),
			BudgetedAsset->FoliagesCount <= 40.0f);
		TestEqual(TEXT("Runtime sector size respects the hard ceiling"),
			BudgetedAsset->FoliageSectorSize, 100000.0);
		TestTrue(TEXT("Runtime cull multiplier respects the ceiling"),
			BudgetedAsset->FoliageCullDistanceMultiplier <= 0.8f);
		TestEqual(TEXT("The dominant authored mesh receives its proportional density budget"),
			BudgetedAsset->FoliagesCount, 36.0f);
		TestEqual(TEXT("Mesh palette identity survives transient sanitization"),
			BudgetedAsset->StaticMesh, Mesh);
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
			BudgetedCluster->FoliagesCount * static_cast<float>(Expansion) <= 4.0f);
		TestEqual(TEXT("Rare cluster palette receives the remaining weighted budget"),
			BudgetedCluster->FoliagesCount * static_cast<float>(Expansion), 4.0f);
		if (!BudgetedCluster->FoliagesClusterUnitList.IsEmpty())
		{
			TestEqual(TEXT("Cluster mesh palette identity survives transient sanitization"),
				BudgetedCluster->FoliagesClusterUnitList[0].StaticMesh, Mesh);
		}
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
			TestTrue(TEXT("Normalized entry uses the hard sector ceiling"),
				Entry->FoliageSectorSize <= 100000.0);
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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FAPSWorldScapeFoliageFreshRootApplicationTest,
	"APS.Gameplay.World.PlanetSurface.Foliage.FreshRootApplication",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAPSWorldScapeFoliageFreshRootApplicationTest::RunTest(const FString& Parameters)
{
	using namespace APSWorldScapeFoliagePolicyTests;
	FScopedFoliageRuntimeOptIn RuntimeOptIn;
	if (!TestTrue(TEXT("Foliage runtime kill switch is registered"), RuntimeOptIn.IsValid()))
	{
		return false;
	}

	UWorld* World = CreateTestWorld();
	if (!TestNotNull(TEXT("Fresh-root foliage test world"), World))
	{
		return false;
	}

	RuntimeOptIn.Set(0);
	AWorldScapeRoot* DisabledRoot = World->SpawnActor<AWorldScapeRoot>();
	if (!TestNotNull(TEXT("Default-off WorldScape root"), DisabledRoot))
	{
		DestroyTestWorld(World);
		return false;
	}
	DisabledRoot->bGenerateFoliages = true;
	DisabledRoot->DisableFoliageDedicatedServer = false;
	DisabledRoot->Foliage_ForceDisabledCollision = false;
	DisabledRoot->FoliageCollisionPooling_Enabled = true;
	DisabledRoot->FoliageMeshIsOccluder = true;
	DisabledRoot->FoliageRenderTreatAsBackGroundForOcclusion = false;
	const FAPSResolvedPlanetSurfaceProfile OptedInProfile = MakeOptedInProfile();
	TestEqual(TEXT("Runtime kill switch allocates no root collections"),
		FAPSWorldScapeFoliagePolicy::ApplyToFreshOwnedRuntimeRoot(
			DisabledRoot, OptedInProfile, false), 0);
	TestFalse(TEXT("Default-off root cannot generate foliage"),
		DisabledRoot->bGenerateFoliages);
	TestTrue(TEXT("Default-off root clears authored foliage state"),
		DisabledRoot->Foliages.IsEmpty());
	TestTrue(TEXT("Fresh-root policy disables foliage on dedicated servers"),
		DisabledRoot->DisableFoliageDedicatedServer);
	TestTrue(TEXT("Fresh-root policy forces foliage collision off"),
		DisabledRoot->Foliage_ForceDisabledCollision);
	TestFalse(TEXT("Fresh-root policy disables collision pooling"),
		DisabledRoot->FoliageCollisionPooling_Enabled);
	TestFalse(TEXT("Fresh-root policy disables foliage occluders"),
		DisabledRoot->FoliageMeshIsOccluder);
	TestTrue(TEXT("Fresh-root policy treats foliage as background for occlusion"),
		DisabledRoot->FoliageRenderTreatAsBackGroundForOcclusion);

	UWorldScapeFoliagesCollection* Source =
		NewObject<UWorldScapeFoliagesCollection>(GetTransientPackage());
	UStaticMesh* Mesh = NewObject<UStaticMesh>(Source);
	UWorldScapeFoliagesAsset* SourceAsset =
		NewObject<UWorldScapeFoliagesAsset>(Source);
	if (!TestNotNull(TEXT("Transient root-application collection"), Source)
		|| !TestNotNull(TEXT("Transient root-application mesh"), Mesh)
		|| !TestNotNull(TEXT("Transient root-application asset"), SourceAsset))
	{
		DestroyTestWorld(World);
		return false;
	}
	SourceAsset->StaticMesh = Mesh;
	SourceAsset->FoliagesCount = 1000.0f;
	SourceAsset->FoliageSectorSize = 1000.0;
	SourceAsset->bCollision = true;
	SourceAsset->Is_NaniteMesh = true;
	SourceAsset->bCastShadows = true;
	Source->FoliageList.Add(SourceAsset);

	FAPSResolvedPlanetSurfaceProfile TransientProfile = MakeOptedInProfile();
	TransientProfile.Foliage.Collections.Reset();
	TransientProfile.Foliage.Collections.Add(
		TSoftObjectPtr<UWorldScapeFoliagesCollection>(Source));
	RuntimeOptIn.Set(1);
	AWorldScapeRoot* EnabledRoot = World->SpawnActor<AWorldScapeRoot>();
	if (!TestNotNull(TEXT("Explicitly enabled WorldScape root"), EnabledRoot))
	{
		DestroyTestWorld(World);
		return false;
	}

	TestEqual(TEXT("Explicit opt-in installs one transient root collection"),
		FAPSWorldScapeFoliagePolicy::ApplyToFreshOwnedRuntimeRoot(
			EnabledRoot, TransientProfile, false), 1);
	TestTrue(TEXT("Explicit opt-in enables foliage only after a collection is sanitized"),
		EnabledRoot->bGenerateFoliages);
	TestEqual(TEXT("Profile sector budget drives the root WPO/LOD cutoff"),
		EnabledRoot->WPO_FoliageDisabledDistance, 24000);
	if (TestEqual(TEXT("Enabled root owns one budgeted collection"),
		EnabledRoot->Foliages.Num(), 1))
	{
		UWorldScapeFoliagesCollection* BudgetedCollection = EnabledRoot->Foliages[0];
		TestNotEqual(TEXT("Enabled root never retains the authored collection"),
			BudgetedCollection, Source);
		TestTrue(TEXT("Enabled root collection is transient"),
			BudgetedCollection->HasAnyFlags(RF_Transient));
		if (TestEqual(TEXT("Enabled root retains one supported mesh type"),
			BudgetedCollection->FoliageList.Num(), 1))
		{
			const UWorldScapeFoliagesAsset* BudgetedAsset =
				Cast<UWorldScapeFoliagesAsset>(BudgetedCollection->FoliageList[0]);
			if (TestNotNull(TEXT("Enabled root owns a budgeted mesh entry"), BudgetedAsset))
			{
				TestFalse(TEXT("Root mesh entry disables collision"), BudgetedAsset->bCollision);
				TestFalse(TEXT("Root mesh entry forces the WorldScape HISM path"),
					BudgetedAsset->Is_NaniteMesh);
				TestFalse(TEXT("Root mesh entry disables shadows by default"),
					BudgetedAsset->bCastShadows);
				TestTrue(TEXT("Root mesh entry respects the profile instance budget"),
					BudgetedAsset->FoliagesCount
						<= TransientProfile.Foliage.MaxInstancesPerSectorPerCollection);
			}
		}
	}

	DestroyTestWorld(World);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FAPSWorldScapeFoliageAggregateBudgetTest,
	"APS.Gameplay.World.PlanetSurface.Foliage.AggregateAdmissionBudget",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAPSWorldScapeFoliageAggregateBudgetTest::RunTest(const FString& Parameters)
{
	using Policy = FAPSWorldScapeFoliagePolicy;
	UStaticMesh* Mesh = NewObject<UStaticMesh>(GetTransientPackage());
	for (int32 Collections = 1; Collections <= 2; ++Collections)
	{
		for (int32 Kind = 0; Kind < 3; ++Kind)
		{
			TArray<UWorldScapeFoliagesCollection*> Sources;
			for (int32 C = 0; C < 3; ++C)
			{
				auto* Source = NewObject<UWorldScapeFoliagesCollection>(GetTransientPackage());
				for (int32 T = 0; T < 8; ++T)
				{
					if (Kind == 1 || (Kind == 2 && T % 2 == 0))
					{
						auto* Cluster = NewObject<UWorldScapeFoliagesCluster>(Source);
						Cluster->FoliagesCount = 10000.0f;
						for (int32 U = 0; U < 8; ++U)
						{
							FWorldScapeFoliagesClusterUnit Unit;
							Unit.StaticMesh = Mesh; Unit.ClusterMin = 99; Unit.ClusterMax = 999;
							Cluster->FoliagesClusterUnitList.Add(Unit);
						}
						Source->FoliageList.Add(Cluster);
					}
					else
					{
						auto* Asset = NewObject<UWorldScapeFoliagesAsset>(Source);
						Asset->StaticMesh = Mesh; Asset->FoliagesCount = 10000.0f;
						Source->FoliageList.Add(Asset);
					}
				}
				Sources.Add(Source);
			}
			FAPSFoliageActivationPlan Plan;
			Plan.bEnabled = true; Plan.HabitatDensityScale = 1.0f;
			Plan.MaxCollections = Collections; Plan.MaxTypesPerCollection = 6;
			Plan.MaxInstancesPerSectorPerCollection = 512; Plan.MaxClusterMeshesPerType = 4;
			Plan.MinSectorSizeCm = 2000.0f; Plan.MaxCullDistanceMultiplier = 1.5f;
			TArray<UWorldScapeFoliagesCollection*> Result;
			TestEqual(TEXT("Both admitted collections retain a nonempty palette"),
				Policy::BuildBudgetedCollections(GetTransientPackage(), Sources, Plan, Result), Collections);
			int64 InstancesPerSectorSum = 0, ComponentSlotsSum = 0;
			for (const auto* Collection : Result)
			{
				for (const auto* Entry : Collection->FoliageList)
				{
					int32 Expansion = 1, Slots = 1;
					if (const auto* Cluster = Cast<UWorldScapeFoliagesCluster>(Entry))
					{
						Expansion = 0; Slots = Cluster->FoliagesClusterUnitList.Num();
						for (const auto& Unit : Cluster->FoliagesClusterUnitList) Expansion += Unit.ClusterMax;
					}
					TestTrue(TEXT("Every retained type has a positive spawn/component allocation"),
						Entry->FoliagesCount >= 1 && Expansion >= 1 && Slots >= 1);
					InstancesPerSectorSum += FMath::CeilToInt64(Entry->FoliagesCount) * Expansion;
					ComponentSlotsSum += Slots;
				}
			}
			TestTrue(TEXT("All types, collections and cluster expansions fit the instance envelope"),
				InstancesPerSectorSum > 0 && InstancesPerSectorSum * Policy::PeakSectorEnvelopePerType <= Policy::MaximumPeakInstancesPerRoot);
			TestTrue(TEXT("Even empty cluster units fit the HISM component envelope"),
				ComponentSlotsSum > 0 && ComponentSlotsSum * Policy::PeakSectorEnvelopePerType <= Policy::MaximumPeakMeshComponentsPerRoot);
			TestEqual(TEXT("Authored type list is untouched"), Sources[0]->FoliageList.Num(), 8);
			TestEqual(TEXT("Authored density is untouched"), Sources[0]->FoliageList[0]->FoliagesCount, 10000.0f);
			Plan.HabitatDensityScale = 0.001f;
			TestEqual(TEXT("Low density retains a bounded first type rather than dropping the entire palette"),
				Policy::BuildBudgetedCollections(GetTransientPackage(), Sources, Plan, Result), Collections);
			for (const auto* Collection : Result)
			{
				TestEqual(TEXT("One viable type fits the one-instance collection budget"), Collection->FoliageList.Num(), 1);
			}
		}
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FAPSWorldScapeFoliageSectorEnvelopeTest,
	"APS.Gameplay.World.PlanetSurface.Foliage.PluginSectorEnvelope",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAPSWorldScapeFoliageSectorEnvelopeTest::RunTest(const FString& Parameters)
{
	using namespace APSWorldScapeFoliagePolicyTests;
	UWorld* World = CreateTestWorld();
	if (!TestNotNull(TEXT("Sector contract world"), World)) return false;
	auto* Root = World->SpawnActor<AWorldScapeRoot>();
	if (!TestNotNull(TEXT("Sector contract root"), Root)) { DestroyTestWorld(World); return false; }
	Root->bGenerateFoliages = false; Root->bFlatWorld = false;
	constexpr double Size = 12000.0;
	TestEqual(TEXT("Plugin generation ring remains 27 sectors"),
		Root->GetSurroundingFoliageSector(DVector(0), Size).Num(), FAPSWorldScapeFoliagePolicy::QueuedSectorEnvelopePerType);
	int32 Retained = 0;
	for (int32 X = -5; X <= 5; ++X)
		for (int32 Y = -5; Y <= 5; ++Y)
			for (int32 Z = -5; Z <= 5; ++Z)
				if (WorldScapeHelper::IsPointInCube(DVector(0), DVector(X * Size, Y * Size, Z * Size), Size * 4)) ++Retained;
	TestEqual(TEXT("Inclusive retention cube is 729 sectors, not 27"), Retained,
		FAPSWorldScapeFoliagePolicy::RetainedSectorEnvelopePerType);
	DestroyTestWorld(World);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FAPSWorldScapeFoliageWorldAdmissionTest,
	"APS.Gameplay.World.PlanetSurface.Foliage.WorldAdmissionBudget",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAPSWorldScapeFoliageWorldAdmissionTest::RunTest(const FString& Parameters)
{
	using namespace APSWorldScapeFoliagePolicyTests;
	using Policy = FAPSWorldScapeFoliagePolicy;
	FScopedFoliageRuntimeOptIn RuntimeOptIn;
	if (!TestTrue(TEXT("Admission kill switch registered"), RuntimeOptIn.IsValid())) return false;
	RuntimeOptIn.Set(1);
	UWorld* World = CreateTestWorld();
	UWorld* OtherWorld = CreateTestWorld();
	if (!TestNotNull(TEXT("Admission world"), World) || !TestNotNull(TEXT("Independent world"), OtherWorld))
	{
		DestroyTestWorld(World); DestroyTestWorld(OtherWorld); return false;
	}
	auto* Source = NewObject<UWorldScapeFoliagesCollection>(GetTransientPackage());
	auto* SourceAsset = NewObject<UWorldScapeFoliagesAsset>(Source);
	SourceAsset->StaticMesh = NewObject<UStaticMesh>(Source);
	SourceAsset->FoliagesCount = 1000.0f;
	Source->FoliageList.Add(SourceAsset);
	auto Profile = MakeOptedInProfile();
	Profile.Foliage.Collections.Reset();
	Profile.Foliage.Collections.Add(TSoftObjectPtr<UWorldScapeFoliagesCollection>(Source));
	TArray<AWorldScapeRoot*> Roots;
	for (int32 I = 0; I < Policy::MaximumAdmittedRootsPerWorld + 1; ++I)
	{
		auto* Root = World->SpawnActor<AWorldScapeRoot>();
		if (!TestNotNull(TEXT("Admission candidate"), Root)) break;
		Roots.Add(Root);
		const int32 Expected = I < Policy::MaximumAdmittedRootsPerWorld ? 1 : 0;
		TestEqual(TEXT("Only bounded roots receive collections"),
			Policy::ApplyToFreshOwnedRuntimeRoot(Root, Profile, false), Expected);
		TestEqual(TEXT("Only admitted roots generate"), Root->bGenerateFoliages, Expected != 0);
	}
	TestEqual(TEXT("World reservations stay bounded"), Policy::GetReservedRootCount(World),
		Policy::MaximumAdmittedRootsPerWorld);
	if (Roots.Num() > Policy::MaximumAdmittedRootsPerWorld
		&& TestFalse(TEXT("First admission produced a collection before reuse test"), Roots[0]->Foliages.IsEmpty()))
	{
		// The fake soft path must NOT be loaded when the world is full. Unexpected
		// load failures would fail this automation, proving denial happens too late.
		TestEqual(TEXT("Budget rejects before resolving optional dependencies"),
			Policy::ApplyToFreshOwnedRuntimeRoot(Roots.Last(), MakeOptedInProfile(), false), 0);
		auto* FirstCollection = Roots[0]->Foliages[0];
		RuntimeOptIn.Set(0);
		TestEqual(TEXT("Reapplication is non-destructive even with kill switch off"),
			Policy::ApplyToFreshOwnedRuntimeRoot(Roots[0], Profile, false), 1);
		TestEqual(TEXT("A live collection is never swapped"), Roots[0]->Foliages[0], FirstCollection);
		RuntimeOptIn.Set(1);
		TestTrue(TEXT("Retire an admitted actor"), Roots[0]->Destroy());
		TestEqual(TEXT("Pending-kill root retains its reservation until actual reclamation"),
			Policy::GetReservedRootCount(World), Policy::MaximumAdmittedRootsPerWorld);
		TestEqual(TEXT("Retirement cannot double-spend an outstanding slot"),
			Policy::ApplyToFreshOwnedRuntimeRoot(Roots.Last(), Profile, false), 0);
	}
	if (auto* OtherRoot = OtherWorld->SpawnActor<AWorldScapeRoot>())
	{
		TestEqual(TEXT("Separate PIE/game world has a separate budget"),
			Policy::ApplyToFreshOwnedRuntimeRoot(OtherRoot, Profile, false), 1);
		TestEqual(TEXT("Separate world reservation count"), Policy::GetReservedRootCount(OtherWorld), 1);
	}
	else AddError(TEXT("Could not spawn independent-world root"));
	TestEqual(TEXT("Source density remains authored"), SourceAsset->FoliagesCount, 1000.0f);
	DestroyTestWorld(World); DestroyTestWorld(OtherWorld);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FAPSWorldScapeFoliagePrototypeGatesTest,
	"APS.Gameplay.World.PlanetSurface.Foliage.PrototypePaletteGates",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAPSWorldScapeFoliagePrototypeGatesTest::RunTest(const FString& Parameters)
{
	using Policy = FAPSWorldScapeFoliagePolicy;
	FAPSResolvedPlanetSurfaceProfile Barren;
	Barren.PlanetType = EPlanetType::Frozen;
	Barren.Archetype = EAPSPlanetSurfaceArchetype::Cryogenic;
	Barren.Biomass = 0; Barren.Biodiversity = 0; Barren.Humidity = 0;
	const uint32 OriginalSignature = UAPSPlanetSurfaceProfileResolver::BuildProfileSignature(Barren);
	TestFalse(TEXT("Normal barren profile remains empty"), Policy::BuildActivationPlan(Barren, true, false).bEnabled);
	TestFalse(TEXT("Prototype alone cannot bypass the global key"), Policy::BuildActivationPlan(Barren, false, false, true).bEnabled);
	TestFalse(TEXT("Prototype cannot run in scaled preview"), Policy::BuildActivationPlan(Barren, true, true, true).bEnabled);
	const auto Minerals = Policy::BuildActivationPlan(Barren, true, false, true);
	TestTrue(TEXT("Explicit mineral palette needs no biological habitat"), Minerals.bEnabled && Minerals.bPrototypePalette);
	TestEqual(TEXT("Mineral density is not multiplied by nonexistent biomass"), Minerals.HabitatDensityScale, 1.0f);
	TestFalse(TEXT("Mineral placement does not use the biological noise mask"), Minerals.bUseNoiseMask);
	TestFalse(TEXT("Prototype shadows remain off"), Minerals.bCastShadows);
	TestEqual(TEXT("Prototype has exactly one collection"), Minerals.Collections.Num(), 1);
	TestEqual(TEXT("Prototype sector cap"), Minerals.MaxInstancesPerSectorPerCollection, 16);
	if (Minerals.Collections.Num() == 1)
	{
		TestEqual(TEXT("Runtime palette path matches the separately baked preset"),
			Minerals.Collections[0].ToSoftObjectPath().ToString(),
			FString(TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/Diagnostics/FoliagePrototype20260929V1/FC_APS_Proto_Frozen.FC_APS_Proto_Frozen")));
	}
	TestEqual(TEXT("Prototype selection never changes the input profile signature"),
		UAPSPlanetSurfaceProfileResolver::BuildProfileSignature(Barren), OriginalSignature);
	TestEqual(TEXT("No fake biomass is written"), Barren.Biomass, 0.0f);
	TestTrue(TEXT("Authored source remains disabled and empty"), !Barren.Foliage.bEnabled && Barren.Foliage.Collections.IsEmpty());

	FAPSResolvedPlanetSurfaceProfile Living = Barren;
	Living.PlanetType = EPlanetType::Forest;
	Living.Archetype = EAPSPlanetSurfaceArchetype::Biosphere;
	TestFalse(TEXT("Tree prototypes cannot bypass biomass gate"), Policy::BuildActivationPlan(Living, true, false, true).bEnabled);
	Living.Biomass = 0.8f; Living.Biodiversity = 0.7f; Living.Humidity = 0.75f;
	const auto Trees = Policy::BuildActivationPlan(Living, true, false, true);
	TestTrue(TEXT("Living prototype uses habitat and mask"), Trees.bEnabled && Trees.bUseNoiseMask);
	TestTrue(TEXT("Biological density remains habitat-modulated"), Trees.HabitatDensityScale > 0 && Trees.HabitatDensityScale < 1);

	auto Authored = APSWorldScapeFoliagePolicyTests::MakeOptedInProfile();
	const auto AuthoredPlan = Policy::BuildActivationPlan(Authored, true, false, true);
	TestTrue(TEXT("Authored opt-in still works"), AuthoredPlan.bEnabled);
	TestFalse(TEXT("Prototype never replaces an authored palette"), AuthoredPlan.bPrototypePalette);
	TestTrue(TEXT("Authored collection references are preserved"), AuthoredPlan.Collections == Authored.Foliage.Collections);
	Authored.Foliage.bEnabled = false;
	TestFalse(TEXT("An authored disabled palette stays disabled"), Policy::BuildActivationPlan(Authored, true, false, true).bEnabled);
	Authored.Foliage.bEnabled = true; Authored.Biomass = 0; Authored.Biodiversity = 0;
	TestFalse(TEXT("Prototype flag cannot exempt an authored biological palette from habitat"),
		Policy::BuildActivationPlan(Authored, true, false, true).bEnabled);

	TSet<FString> PalettePaths;
	for (uint8 Value = 0; Value <= APSPlanetTypes::LastValue; ++Value)
	{
		auto Profile = Living; Profile.PlanetType = static_cast<EPlanetType>(Value);
		const bool Supported = UAPSPlanetSurfaceProfileResolver::SupportsWorldScape(Profile.PlanetType);
		const auto Plan = Policy::BuildActivationPlan(Profile, true, false, true);
		TestEqual(TEXT("Every supported surface type gets an explicit test recipe; giants stay off"), Plan.bEnabled, Supported);
		if (Supported && TestEqual(TEXT("Each prototype has one recipe"), Plan.Collections.Num(), 1))
		{
			const FString Path = Plan.Collections[0].ToSoftObjectPath().ToString();
			TestFalse(TEXT("Each type owns a separately replaceable palette"), PalettePaths.Contains(Path));
			PalettePaths.Add(Path);
		}
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAPSFoliageValidatedRuntimeTest,
    "APS.Gameplay.World.PlanetSurface.Foliage.ValidatedRuntimeScope",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAPSFoliageValidatedRuntimeTest::RunTest(const FString& Parameters)
{
    using P = FAPSWorldScapeFoliagePolicy;
    FAPSResolvedPlanetSurfaceProfile Profile;
    Profile.Biomass = Profile.Biodiversity = 1.0f;
    for (uint8 V = 0; V <= APSPlanetTypes::LastValue; ++V)
    {
        Profile.PlanetType = static_cast<EPlanetType>(V);
        const bool Expected = Profile.PlanetType == EPlanetType::Frozen || Profile.PlanetType == EPlanetType::Forest;
        TestEqual(TEXT("Automatic rollout only validated two types"), P::BuildRuntimeActivationPlan(Profile,2,false,false,false).bEnabled, Expected);
        TestFalse(TEXT("Master-off always wins"), P::BuildRuntimeActivationPlan(Profile,0,false,true,false).bEnabled);
        TestFalse(TEXT("Manual planets retain previous default"), P::BuildRuntimeActivationPlan(Profile,2,false,true,true).bEnabled);
        TestFalse(TEXT("No orbital/menu foliage"), P::BuildRuntimeActivationPlan(Profile,2,true,true,false).bEnabled);
        TestFalse(TEXT("Unknown mode fails closed"), P::BuildRuntimeActivationPlan(Profile,3,false,true,false).bEnabled);
    }
    Profile.PlanetType = EPlanetType::Frozen;
    Profile.Foliage.bEnabled = true;
    TestFalse(TEXT("Authored enable is not changed by automatic rollout"), P::BuildRuntimeActivationPlan(Profile,2,false,false,false).bEnabled);
    Profile.Foliage.bEnabled = false;
    Profile.Foliage.Collections = APSPlanetFoliagePrototype::Settings(EPlanetType::Frozen).Collections;
    TestFalse(TEXT("Authored collection remains untouched even when disabled"), P::BuildRuntimeActivationPlan(Profile,2,false,false,false).bEnabled);
    Profile.Foliage.Collections.Reset();
    Profile.Biomass = Profile.Biodiversity = 0.0f;
    TestTrue(TEXT("Mineral rock needs no biosphere"), P::BuildRuntimeActivationPlan(Profile,2,false,false,false).bEnabled);
    Profile.PlanetType = EPlanetType::Forest;
    TestFalse(TEXT("Automatic vegetation still respects habitat"), P::BuildRuntimeActivationPlan(Profile,2,false,false,false).bEnabled);
    return true;
}

#endif
