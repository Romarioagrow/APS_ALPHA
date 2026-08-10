#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "APS_ALPHA/Core/Planetary/APSWorldScapeFoliagePolicy.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "HAL/IConsoleManager.h"
#include "WorldScapeCore/Public/WorldScapeRoot.h"
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

#endif
