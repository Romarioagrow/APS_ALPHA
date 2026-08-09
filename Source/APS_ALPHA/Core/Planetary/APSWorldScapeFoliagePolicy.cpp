#include "APSWorldScapeFoliagePolicy.h"

#include "HAL/IConsoleManager.h"
#include "UObject/UObjectGlobals.h"
#include "WorldScapeCore/Public/WorldScapeRoot.h"
#include "WorldScapeFoliages/Public/WorldScapeFoliagesAsset.h"
#include "WorldScapeFoliages/Public/WorldScapeFoliagesBlueprint.h"
#include "WorldScapeFoliages/Public/WorldScapeFoliagesCluster.h"
#include "WorldScapeFoliages/Public/WorldScapeFoliagesCollection.h"

namespace APSWorldScapeFoliage
{
	constexpr int32 MaximumCollections = 2;
	constexpr int32 MaximumTypesPerCollection = 6;
	constexpr int32 MaximumInstancesPerSectorPerCollection = 512;
	constexpr int32 MaximumClusterMeshesPerType = 4;
	constexpr float MinimumSectorSizeCm = 2000.0f;
	constexpr float MaximumSectorSizeCm = 100000.0f;
	constexpr float MinimumCullDistanceMultiplier = 0.1f;
	constexpr float MaximumCullDistanceMultiplier = 1.5f;

	TAutoConsoleVariable<int32> CVarEnable(
		TEXT("aps.WorldScapeFoliage.Enable"),
		0,
		TEXT("Enables experimental, profile-driven WorldScape foliage on fresh full-scale gameplay roots.\n")
		TEXT("0: disabled (default)\n")
		TEXT("1: allow profiles with Foliage.bEnabled and configured collections"),
		ECVF_Default);

	bool NormalizePlan(
		const FAPSFoliageActivationPlan& Input,
		FAPSFoliageActivationPlan& Output)
	{
		Output = FAPSFoliageActivationPlan{};
		if (!Input.bEnabled
			|| !FMath::IsFinite(Input.HabitatDensityScale)
			|| !FMath::IsFinite(Input.MinSectorSizeCm)
			|| !FMath::IsFinite(Input.MaxCullDistanceMultiplier))
		{
			return false;
		}

		Output = Input;
		Output.HabitatDensityScale = FMath::Clamp(Input.HabitatDensityScale, 0.0f, 1.0f);
		if (Output.HabitatDensityScale <= UE_SMALL_NUMBER)
		{
			Output = FAPSFoliageActivationPlan{};
			return false;
		}
		Output.MaxCollections = FMath::Clamp(Input.MaxCollections, 1, MaximumCollections);
		Output.MaxTypesPerCollection = FMath::Clamp(
			Input.MaxTypesPerCollection, 1, MaximumTypesPerCollection);
		Output.MaxInstancesPerSectorPerCollection = FMath::Clamp(
			Input.MaxInstancesPerSectorPerCollection,
			16,
			MaximumInstancesPerSectorPerCollection);
		Output.MaxClusterMeshesPerType = FMath::Clamp(
			Input.MaxClusterMeshesPerType, 1, MaximumClusterMeshesPerType);
		Output.MinSectorSizeCm = FMath::Clamp(
			Input.MinSectorSizeCm, MinimumSectorSizeCm, MaximumSectorSizeCm);
		Output.MaxCullDistanceMultiplier = FMath::Clamp(
			Input.MaxCullDistanceMultiplier,
			MinimumCullDistanceMultiplier,
			MaximumCullDistanceMultiplier);
		Output.bEnabled = true;
		return true;
	}

	bool IsSupportedMeshEntry(const UWorldScapeFoliagesInterface* Entry)
	{
		if (!IsValid(Entry) || Entry->FoliagesCount <= 0.0f)
		{
			return false;
		}

		if (Entry->GetClass() == UWorldScapeFoliagesAsset::StaticClass())
		{
			const UWorldScapeFoliagesAsset* Asset =
				static_cast<const UWorldScapeFoliagesAsset*>(Entry);
			return !Asset->bSpawnActorInstead && IsValid(Asset->StaticMesh);
		}

		if (Entry->GetClass() == UWorldScapeFoliagesCluster::StaticClass())
		{
			const UWorldScapeFoliagesCluster* Cluster =
				static_cast<const UWorldScapeFoliagesCluster*>(Entry);
			for (const FWorldScapeFoliagesClusterUnit& Unit : Cluster->FoliagesClusterUnitList)
			{
				if (IsValid(Unit.StaticMesh))
				{
					return true;
				}
			}
		}

		// WorldScape 5.4 treats Blueprint foliage as actor spawning. Its root also
		// dispatches by exact class, so derived/custom entries are not a safe HISM path.
		return false;
	}

	void ApplyCommonBudget(
		UWorldScapeFoliagesInterface* Entry,
		const FAPSFoliageActivationPlan& Plan,
		int32 InstanceCount)
	{
		Entry->bGenerateOnServer = false;
		Entry->FoliageSectorSize = FMath::Max(
			Entry->FoliageSectorSize, static_cast<double>(Plan.MinSectorSizeCm));
		Entry->bUsePoissonDisc = false;
		Entry->PoissonDiscDensityVariation = 0.0f;
		Entry->FoliagesCount = static_cast<float>(FMath::Max(InstanceCount, 1));
		Entry->bUseFoliageNoiseMask = Plan.bUseNoiseMask;
	}

	void SanitizeAsset(
		UWorldScapeFoliagesAsset* Asset,
		const FAPSFoliageActivationPlan& Plan)
	{
		Asset->bSpawnActorInstead = false;
		Asset->ObjectToSpawn = nullptr;
		Asset->bCollision = false;
		// WorldScape 5.4 creates a HISM only when this compatibility flag is false.
		Asset->Is_NaniteMesh = false;
		Asset->bCastShadows = Plan.bCastShadows;
		Asset->bCastFarShadow = false;
		Asset->bCastDistanceFieldIndirectShadow = false;
		Asset->bAffectDynamicIndirectLighting = false;
		Asset->bAffectDistanceFieldLighting = false;
		Asset->bNeverDistanceCull = false;
		Asset->FoliageCullDistanceMultiplier = FMath::Clamp(
			Asset->FoliageCullDistanceMultiplier,
			0.1f,
			Plan.MaxCullDistanceMultiplier);
	}

	int32 SanitizeCluster(
		UWorldScapeFoliagesCluster* Cluster,
		const FAPSFoliageActivationPlan& Plan,
		int32 BudgetPerType)
	{
		TArray<FWorldScapeFoliagesClusterUnit> Units;
		const int32 MaxUnits = FMath::Min3(
			Plan.MaxClusterMeshesPerType,
			Cluster->FoliagesClusterUnitList.Num(),
			FMath::Max(BudgetPerType, 1));
		Units.Reserve(MaxUnits);

		for (const FWorldScapeFoliagesClusterUnit& SourceUnit : Cluster->FoliagesClusterUnitList)
		{
			if (Units.Num() >= MaxUnits)
			{
				break;
			}
			if (!IsValid(SourceUnit.StaticMesh))
			{
				continue;
			}

			FWorldScapeFoliagesClusterUnit Unit = SourceUnit;
			Unit.Is_NaniteMesh = false;
			Unit.bCollision = false;
			Unit.bGenerateOnServer = false;
			Unit.bCastShadows = Plan.bCastShadows;
			Unit.bCastFarShadow = false;
			Unit.bCastDistanceFieldIndirectShadow = false;
			Unit.bAffectDynamicIndirectLighting = false;
			Unit.bAffectDistanceFieldLighting = false;
			Unit.bNeverDistanceCull = false;
			Unit.FoliageCullDistanceMultiplier = FMath::Clamp(
				Unit.FoliageCullDistanceMultiplier,
				0.1f,
				Plan.MaxCullDistanceMultiplier);
			Units.Add(MoveTemp(Unit));
		}

		if (Units.IsEmpty())
		{
			Cluster->FoliagesClusterUnitList.Reset();
			return 0;
		}

		const int32 MaxExpansionPerUnit = FMath::Max(BudgetPerType / Units.Num(), 1);
		int32 EstimatedClusterExpansion = 0;
		for (FWorldScapeFoliagesClusterUnit& Unit : Units)
		{
			Unit.ClusterMax = FMath::Clamp(Unit.ClusterMax, 1, MaxExpansionPerUnit);
			Unit.ClusterMin = FMath::Clamp(Unit.ClusterMin, 1, Unit.ClusterMax);
			EstimatedClusterExpansion += Unit.ClusterMax;
		}
		Cluster->FoliagesClusterUnitList = MoveTemp(Units);
		return EstimatedClusterExpansion;
	}
}

FAPSFoliageActivationPlan FAPSWorldScapeFoliagePolicy::BuildActivationPlan(
	const FAPSResolvedPlanetSurfaceProfile& Profile,
	bool bRuntimeOptIn,
	bool bScaledOrbitalPreview)
{
	FAPSFoliageActivationPlan Plan;
	const FAPSPlanetFoliageProfile& Settings = Profile.Foliage;
	const bool bHasCollection = Settings.Collections.ContainsByPredicate(
		[](const TSoftObjectPtr<UWorldScapeFoliagesCollection>& Collection)
		{
			return !Collection.IsNull();
		});

	const float HabitatSignal = FMath::Max(Profile.Biomass, Profile.Biodiversity * 0.75f);
	const float MinimumBiomass = FMath::Clamp(Settings.MinimumBiomass, 0.0f, 1.0f);
	if (!bRuntimeOptIn
		|| bScaledOrbitalPreview
		|| !Settings.bEnabled
		|| !bHasCollection
		|| !UAPSPlanetSurfaceProfileResolver::SupportsWorldScape(Profile.PlanetType)
		|| HabitatSignal <= MinimumBiomass)
	{
		return Plan;
	}

	const float HabitatRange = FMath::Max(1.0f - MinimumBiomass, UE_SMALL_NUMBER);
	const float BiomassResponse = FMath::Clamp(
		(HabitatSignal - MinimumBiomass) / HabitatRange, 0.0f, 1.0f);
	const float HumidityResponse = FMath::Lerp(
		0.55f, 1.0f, FMath::Clamp(Profile.Humidity, 0.0f, 1.0f));
	Plan.HabitatDensityScale = FMath::Clamp(BiomassResponse * HumidityResponse, 0.0f, 1.0f);
	if (Plan.HabitatDensityScale <= UE_SMALL_NUMBER)
	{
		return FAPSFoliageActivationPlan{};
	}

	Plan.MaxCollections = FMath::Clamp(
		Settings.MaxCollections, 1, APSWorldScapeFoliage::MaximumCollections);
	Plan.MaxTypesPerCollection = FMath::Clamp(
		Settings.MaxTypesPerCollection, 1, APSWorldScapeFoliage::MaximumTypesPerCollection);
	Plan.MaxInstancesPerSectorPerCollection = FMath::Clamp(
		Settings.MaxInstancesPerSectorPerCollection,
		16,
		APSWorldScapeFoliage::MaximumInstancesPerSectorPerCollection);
	Plan.MaxClusterMeshesPerType = FMath::Clamp(
		Settings.MaxClusterMeshesPerType,
		1,
		APSWorldScapeFoliage::MaximumClusterMeshesPerType);
	Plan.MinSectorSizeCm = FMath::Clamp(
		Settings.MinSectorSizeCm,
		APSWorldScapeFoliage::MinimumSectorSizeCm,
		APSWorldScapeFoliage::MaximumSectorSizeCm);
	Plan.MaxCullDistanceMultiplier = FMath::Clamp(
		Settings.MaxCullDistanceMultiplier,
		APSWorldScapeFoliage::MinimumCullDistanceMultiplier,
		APSWorldScapeFoliage::MaximumCullDistanceMultiplier);
	Plan.bUseNoiseMask = Settings.bUseNoiseMask;
	Plan.bCastShadows = Settings.bCastShadows;
	Plan.bEnabled = true;
	FAPSFoliageActivationPlan NormalizedPlan;
	return APSWorldScapeFoliage::NormalizePlan(Plan, NormalizedPlan)
		? NormalizedPlan
		: FAPSFoliageActivationPlan{};
}

bool FAPSWorldScapeFoliagePolicy::RequiresFreshRootForProfileTransition(
	const FAPSResolvedPlanetSurfaceProfile& AppliedProfile,
	const FAPSResolvedPlanetSurfaceProfile& RequestedProfile,
	bool bRootHasActivatedFoliage)
{
	return bRootHasActivatedFoliage
		|| AppliedProfile.Foliage.bEnabled
		|| RequestedProfile.Foliage.bEnabled;
}

bool FAPSWorldScapeFoliagePolicy::IsRuntimeOptInEnabled()
{
	return APSWorldScapeFoliage::CVarEnable.GetValueOnGameThread() > 0;
}

int32 FAPSWorldScapeFoliagePolicy::BuildBudgetedCollections(
	UObject* Outer,
	const TArray<UWorldScapeFoliagesCollection*>& SourceCollections,
	const FAPSFoliageActivationPlan& Plan,
	TArray<UWorldScapeFoliagesCollection*>& OutCollections)
{
	OutCollections.Reset();
	FAPSFoliageActivationPlan SafePlan;
	if (!IsValid(Outer) || !APSWorldScapeFoliage::NormalizePlan(Plan, SafePlan))
	{
		return 0;
	}

	for (UWorldScapeFoliagesCollection* SourceCollection : SourceCollections)
	{
		if (OutCollections.Num() >= SafePlan.MaxCollections)
		{
			break;
		}
		if (!IsValid(SourceCollection))
		{
			continue;
		}

		TArray<UWorldScapeFoliagesInterface*> SupportedEntries;
		SupportedEntries.Reserve(SafePlan.MaxTypesPerCollection);
		for (UWorldScapeFoliagesInterface* Entry : SourceCollection->FoliageList)
		{
			if (SupportedEntries.Num() >= SafePlan.MaxTypesPerCollection)
			{
				break;
			}
			if (APSWorldScapeFoliage::IsSupportedMeshEntry(Entry))
			{
				SupportedEntries.Add(Entry);
			}
		}
		if (SupportedEntries.IsEmpty())
		{
			continue;
		}

		UWorldScapeFoliagesCollection* Collection =
			NewObject<UWorldScapeFoliagesCollection>(Outer, NAME_None, RF_Transient);
		if (!IsValid(Collection))
		{
			continue;
		}
		Collection->Elevation = SourceCollection->Elevation;
		Collection->Temperature = SourceCollection->Temperature;
		Collection->Humidity = SourceCollection->Humidity;
		Collection->Slope = SourceCollection->Slope;
		Collection->SpawnInWater = SourceCollection->SpawnInWater;
		Collection->FoliageLayer = SourceCollection->FoliageLayer;
		Collection->SpawnOnlyInVolume = SourceCollection->SpawnOnlyInVolume;

		const int32 HabitatBudget = FMath::Clamp(
			FMath::FloorToInt(
				static_cast<float>(SafePlan.MaxInstancesPerSectorPerCollection)
				* SafePlan.HabitatDensityScale),
			1,
			SafePlan.MaxInstancesPerSectorPerCollection);
		const int32 BudgetPerType = FMath::Max(
			HabitatBudget / SupportedEntries.Num(), 1);

		for (UWorldScapeFoliagesInterface* SourceEntry : SupportedEntries)
		{
			UWorldScapeFoliagesInterface* Entry =
				DuplicateObject<UWorldScapeFoliagesInterface>(SourceEntry, Collection);
			if (!IsValid(Entry))
			{
				continue;
			}
			Entry->SetFlags(RF_Transient);

			int32 MaximumSpawnGroups = BudgetPerType;
			if (Entry->GetClass() == UWorldScapeFoliagesAsset::StaticClass())
			{
				APSWorldScapeFoliage::SanitizeAsset(
					static_cast<UWorldScapeFoliagesAsset*>(Entry), SafePlan);
			}
			else if (Entry->GetClass() == UWorldScapeFoliagesCluster::StaticClass())
			{
				const int32 ClusterExpansion = APSWorldScapeFoliage::SanitizeCluster(
					static_cast<UWorldScapeFoliagesCluster*>(Entry), SafePlan, BudgetPerType);
				if (ClusterExpansion <= 0)
				{
					continue;
				}
				MaximumSpawnGroups = FMath::Max(BudgetPerType / ClusterExpansion, 1);
			}
			else
			{
				continue;
			}

			const int32 AuthoredSpawnGroups = FMath::Max(
				FMath::FloorToInt(SourceEntry->FoliagesCount), 1);
			APSWorldScapeFoliage::ApplyCommonBudget(
				Entry, SafePlan, FMath::Min(AuthoredSpawnGroups, MaximumSpawnGroups));
			Collection->FoliageList.Add(Entry);
		}

		if (!Collection->FoliageList.IsEmpty())
		{
			OutCollections.Add(Collection);
		}
	}

	return OutCollections.Num();
}

int32 FAPSWorldScapeFoliagePolicy::ApplyToFreshOwnedRuntimeRoot(
	AWorldScapeRoot* Root,
	const FAPSResolvedPlanetSurfaceProfile& Profile,
	bool bScaledOrbitalPreview)
{
	if (!IsValid(Root))
	{
		return 0;
	}

	// WorldScape's constructor defaults this to true. Establish the APS invariant
	// before loading even one optional asset.
	Root->bGenerateFoliages = false;
	Root->Foliages.Reset();
	Root->DisableFoliageDedicatedServer = true;
	Root->Foliage_ForceDisabledCollision = true;
	Root->FoliageCollisionPooling_Enabled = false;
	Root->FoliageMeshIsOccluder = false;
	Root->FoliageRenderTreatAsBackGroundForOcclusion = true;
	// This foundation is visual-only. Reject dedicated servers before resolving
	// soft collections so their mesh/material dependency graphs are never loaded.
	if (Root->GetNetMode() == NM_DedicatedServer)
	{
		return 0;
	}

	const FAPSFoliageActivationPlan Plan = BuildActivationPlan(
		Profile, IsRuntimeOptInEnabled(), bScaledOrbitalPreview);
	if (!Plan.bEnabled)
	{
		return 0;
	}

	Root->WPO_FoliageDisabledDistance = FMath::Clamp(
		FMath::RoundToInt(Plan.MinSectorSizeCm * 2.0f), 10000, 100000);
	Root->FoliageMinimuMSpawnChanceToNotIgnore = FMath::Max(
		Root->FoliageMinimuMSpawnChanceToNotIgnore, 0.02f);

	TArray<UWorldScapeFoliagesCollection*> Sources;
	Sources.Reserve(Plan.MaxCollections);
	for (const TSoftObjectPtr<UWorldScapeFoliagesCollection>& SoftCollection : Profile.Foliage.Collections)
	{
		if (Sources.Num() >= Plan.MaxCollections)
		{
			break;
		}
		if (UWorldScapeFoliagesCollection* Collection = SoftCollection.LoadSynchronous())
		{
			Sources.Add(Collection);
		}
	}

	TArray<UWorldScapeFoliagesCollection*> BudgetedCollections;
	BuildBudgetedCollections(Root, Sources, Plan, BudgetedCollections);
	Root->Foliages = MoveTemp(BudgetedCollections);
	Root->bGenerateFoliages = !Root->Foliages.IsEmpty();
	return Root->Foliages.Num();
}
