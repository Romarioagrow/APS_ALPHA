#include "APSWorldScapeFoliagePolicy.h"
#include "APSPlanetFoliagePrototype.h"
#include "APSPlanetSurfaceScatter.h"
#include "APSPlanetScatterMaterial.h"
#include "APSFoliageLeafMaterial.h"
#include "APSFoliageCollisionComponent.h"
#include "APSFoliageExclusionComponent.h"
#include "Materials/MaterialInstanceDynamic.h"

#include "HAL/IConsoleManager.h"
#include "Engine/World.h"
#include "Engine/StaticMesh.h"
#include "UObject/UObjectGlobals.h"
#include "WorldScapeCore/Public/WorldScapeRoot.h"
#include "WorldScapeFoliages/Public/WorldScapeFoliagesAsset.h"
#include "WorldScapeFoliages/Public/WorldScapeFoliagesBlueprint.h"
#include "WorldScapeFoliages/Public/WorldScapeFoliagesCluster.h"
#include "WorldScapeFoliages/Public/WorldScapeFoliagesCollection.h"

namespace APSWorldScapeFoliage
{
	struct FWorldReservation
	{
		TWeakObjectPtr<UWorld> World;
		TWeakObjectPtr<AWorldScapeRoot> Root;
	};
	// Game-thread only; weak references never prolong root/world lifetime. Do not
	// release a slot merely because Destroy() made the actor pending-kill: plugin
	// queues/components may not yet be reclaimed. Serial validity also survives
	// the GC mark phase without dereferencing an unreachable actor.
	TArray<FWorldReservation> WorldReservations;

	void PruneWorldReservations()
	{
		check(IsInGameThread());
		WorldReservations.RemoveAll([](const FWorldReservation& Entry)
		{
			return !Entry.Root.IsValid(true, true) || !Entry.World.IsValid(true, true);
		});
	}

	bool HasReservation(const AWorldScapeRoot* Root)
	{
		PruneWorldReservations();
		return WorldReservations.ContainsByPredicate([Root](const FWorldReservation& Entry)
		{
			return Entry.Root.Get() == Root;
		});
	}

	constexpr int32 MaximumCollections = 2;
	constexpr int32 MaximumTypesPerCollection = 6;
	constexpr int32 MaximumInstancesPerSectorPerCollection = 512;
	constexpr int32 MaximumClusterMeshesPerType = 4;
	constexpr float MinimumSectorSizeCm = 2000.0f;
	constexpr float MaximumSectorSizeCm = 100000.0f;
	constexpr float MinimumCullDistanceMultiplier = 0.1f;
	constexpr float MaximumCullDistanceMultiplier = 1.5f;
	constexpr double MaximumAuthoredDensityWeight = 1000000.0;

	TAutoConsoleVariable<int32> CVarEnable(
		TEXT("aps.WorldScapeFoliage.Enable"),
		2,
		TEXT("Enables experimental, profile-driven WorldScape foliage on fresh full-scale gameplay roots.\n")
		TEXT("0: disabled; 1: explicit authored/prototype opt-in; 2(default): validated generated palettes only.\n")
		TEXT("Mode2 never replaces authored collections or manual planets; fresh roots only."),
		ECVF_Default);
	TAutoConsoleVariable<int32> CVarPrototype(
		TEXT("aps.WorldScapeFoliage.Prototype"), 0,
		TEXT("Explicit sparse test palettes on fresh gameplay roots with empty, disabled foliage profiles.\n")
		TEXT("Requires aps.WorldScapeFoliage.Enable=1. Never replaces an authored palette.\n")
		TEXT("0: disabled (default); 1: use separately baked diagnostic palettes, mineral entries need no biomass."),
		ECVF_Default);

	TAutoConsoleVariable<int32> CVarSurfaceScatter(
		TEXT("aps.WorldScapeFoliage.SurfaceScatter"), 2,
		TEXT("Five-mesh surface palettes, fresh generated roots only; requires baked V2 assets and bounded seed integration.\n")
		TEXT("0: sparse V1 fallback; 1: all-family diagnostic opt-in; 2(default): rendered type/habitat subset.\n")
		TEXT("Preserves authored/manual/preview roots and master-off; fresh roots only."), ECVF_Default);

	TAutoConsoleVariable<int32> CVarOasisTreeAttempts(
		TEXT("aps.WorldScapeFoliage.OasisTreeAttempts"), 3,
		TEXT("Fresh owned Oasis scatter only: bounded tree candidate attempts, climate mask unchanged.\n")
		TEXT("3: original fallback; 12: published sparse Oasis project budget; clamped to16 and unused collection budget. No live hot-swap."), ECVF_Default);

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
		if (!IsValid(Entry)
			|| !FMath::IsFinite(Entry->FoliagesCount)
			|| Entry->FoliagesCount <= 0.0f)
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

	double GetAuthoredDensityWeight(const UWorldScapeFoliagesInterface* Entry)
	{
		return IsValid(Entry) && FMath::IsFinite(Entry->FoliagesCount)
			? FMath::Clamp(
				static_cast<double>(Entry->FoliagesCount),
				1.0,
				MaximumAuthoredDensityWeight)
			: 1.0;
	}

	double ResolveSectorSizeCm(
		const double AuthoredSectorSizeCm,
		const FAPSFoliageActivationPlan& Plan)
	{
		const double FiniteSectorSizeCm = FMath::IsFinite(AuthoredSectorSizeCm)
			? AuthoredSectorSizeCm
			: static_cast<double>(Plan.MinSectorSizeCm);
		return FMath::Clamp(
			FiniteSectorSizeCm,
			static_cast<double>(Plan.MinSectorSizeCm),
			static_cast<double>(MaximumSectorSizeCm));
	}

	float ResolveCullDistanceMultiplier(
		const float AuthoredMultiplier,
		const FAPSFoliageActivationPlan& Plan)
	{
		const float FiniteMultiplier = FMath::IsFinite(AuthoredMultiplier)
			? AuthoredMultiplier
			: MinimumCullDistanceMultiplier;
		return FMath::Clamp(
			FiniteMultiplier,
			MinimumCullDistanceMultiplier,
			Plan.MaxCullDistanceMultiplier);
	}

	TArray<int32> BuildPerTypeBudgets(
		const TArray<UWorldScapeFoliagesInterface*>& Entries,
		const int32 HabitatBudget)
	{
		TArray<int32> Budgets;
		if (Entries.IsEmpty() || HabitatBudget < Entries.Num())
		{
			return Budgets;
		}

		Budgets.Init(1, Entries.Num());
		const int32 RemainingBudget = HabitatBudget - Entries.Num();
		if (RemainingBudget <= 0)
		{
			return Budgets;
		}

		TArray<double> Weights;
		TArray<double> Fractions;
		TArray<int32> FractionOrder;
		Weights.Reserve(Entries.Num());
		Fractions.Init(0.0, Entries.Num());
		FractionOrder.Reserve(Entries.Num());
		double TotalWeight = 0.0;
		for (int32 Index = 0; Index < Entries.Num(); ++Index)
		{
			const double Weight = GetAuthoredDensityWeight(Entries[Index]);
			Weights.Add(Weight);
			TotalWeight += Weight;
			FractionOrder.Add(Index);
		}

		int32 DistributedBudget = 0;
		for (int32 Index = 0; Index < Entries.Num(); ++Index)
		{
			const double ExactShare = static_cast<double>(RemainingBudget)
				* Weights[Index] / TotalWeight;
			const int32 WholeShare = FMath::FloorToInt32(ExactShare);
			Budgets[Index] += WholeShare;
			DistributedBudget += WholeShare;
			Fractions[Index] = ExactShare - static_cast<double>(WholeShare);
		}

		FractionOrder.Sort([&Fractions](const int32 Left, const int32 Right)
		{
			if (!FMath::IsNearlyEqual(Fractions[Left], Fractions[Right]))
			{
				return Fractions[Left] > Fractions[Right];
			}
			return Left < Right;
		});
		const int32 Remainder = RemainingBudget - DistributedBudget;
		for (int32 Index = 0; Index < Remainder; ++Index)
		{
			++Budgets[FractionOrder[Index]];
		}
		return Budgets;
	}

	void ApplyCommonBudget(
		UWorldScapeFoliagesInterface* Entry,
		const FAPSFoliageActivationPlan& Plan,
		int32 InstanceCount)
	{
		Entry->bGenerateOnServer = false;
		Entry->FoliageSectorSize = ResolveSectorSizeCm(
			Entry->FoliageSectorSize, Plan);
		Entry->bUsePoissonDisc = false;
		Entry->PoissonDiscDensityVariation = 0.0f;
		Entry->FoliagesCount = static_cast<float>(FMath::Max(InstanceCount, 1));
		if (!Plan.bPreserveEntryNoiseMask) Entry->bUseFoliageNoiseMask = Plan.bUseNoiseMask;
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
		if (Plan.bSurfaceScatterPalette && IsValid(Asset->StaticMesh))
		{
			const auto Bounds = Asset->StaticMesh->GetBounds();
			const double Height = Bounds.BoxExtent.Z * 2.;
			if (Height > 0 && FMath::IsFinite(Height) && FMath::IsFinite(Bounds.Origin.Z)
				&& FMath::IsFinite(Asset->MinScale) && FMath::IsFinite(Asset->MaxScale)
				&& Asset->MinScale > 0 && Asset->MaxScale >= Asset->MinScale)
				Asset->Offset = FVector(0, 0, APSPlanetSurfaceScatter::GroundOffsetCm(
					Bounds.Origin.Z - Bounds.BoxExtent.Z, Height, Asset->MinScale, Asset->MaxScale,
					Asset->bUseFoliageNoiseMask));
		}
		Asset->FoliageCullDistanceMultiplier = ResolveCullDistanceMultiplier(
			Asset->FoliageCullDistanceMultiplier, Plan);
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
			Unit.FoliageCullDistanceMultiplier = ResolveCullDistanceMultiplier(
				Unit.FoliageCullDistanceMultiplier, Plan);
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
	bool bScaledOrbitalPreview,
	bool bPrototypeOptIn)
{
	FAPSFoliageActivationPlan Plan;
	// Prototype mode supplies an explicit temporary palette only when there is
	// no authored choice to override. It does not change the profile, its habitat
	// signal, signature or serialized data. Existing callers keep the old gate.
	const bool bPrototype = bPrototypeOptIn && !Profile.Foliage.bEnabled
		&& Profile.Foliage.Collections.IsEmpty();
	const FAPSPlanetFoliageProfile PrototypeSettings = bPrototype
		? APSPlanetFoliagePrototype::Settings(Profile.PlanetType) : FAPSPlanetFoliageProfile{};
	const FAPSPlanetFoliageProfile& Settings = bPrototype ? PrototypeSettings : Profile.Foliage;
	const bool bRequiresHabitat = !bPrototype || APSPlanetFoliagePrototype::Recipe(Profile.PlanetType).bBiological;
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
		|| (bRequiresHabitat && HabitatSignal <= MinimumBiomass))
	{
		return Plan;
	}

	const float HabitatRange = FMath::Max(1.0f - MinimumBiomass, UE_SMALL_NUMBER);
	const float BiomassResponse = FMath::Clamp(
		(HabitatSignal - MinimumBiomass) / HabitatRange, 0.0f, 1.0f);
	const float HumidityResponse = FMath::Lerp(
		0.55f, 1.0f, FMath::Clamp(Profile.Humidity, 0.0f, 1.0f));
	Plan.HabitatDensityScale = bRequiresHabitat
		? FMath::Clamp(BiomassResponse * HumidityResponse, 0.0f, 1.0f) : 1.0f;
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
	Plan.Collections = Settings.Collections;
	Plan.bPrototypePalette = bPrototype;
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

FAPSFoliageActivationPlan FAPSWorldScapeFoliagePolicy::BuildRuntimeActivationPlan(
	const FAPSResolvedPlanetSurfaceProfile& Profile, int32 RuntimeMode,
	bool bScaledOrbitalPreview, bool bPrototypeOptIn, bool bManualPlanet, bool bSurfaceScatterOptIn)
{
	// Automatic scatter fills only unconfigured profiles. Authored palettes must
	// retain their existing mode-1 admission when the global trial is enabled.
	if (bSurfaceScatterOptIn && !Profile.Foliage.bEnabled && Profile.Foliage.Collections.IsEmpty())
	{
		if ((RuntimeMode != 1 && RuntimeMode != 2) || bScaledOrbitalPreview || bManualPlanet) return {};
		const auto S = APSPlanetSurfaceScatter::Settings(Profile.PlanetType, APSPlanetSurfaceScatter::HasHabitat(Profile));
		if (!S.bEnabled) return {};
		FAPSFoliageActivationPlan P;
		P.bEnabled = true; P.HabitatDensityScale = 1;
		P.MaxCollections = S.MaxCollections; P.MaxTypesPerCollection = S.MaxTypesPerCollection;
		P.MaxInstancesPerSectorPerCollection = S.MaxInstancesPerSectorPerCollection;
		P.MaxClusterMeshesPerType = S.MaxClusterMeshesPerType;
		P.MinSectorSizeCm = S.MinSectorSizeCm; P.MaxCullDistanceMultiplier = S.MaxCullDistanceMultiplier;
		P.bUseNoiseMask = false; P.bPreserveEntryNoiseMask = true; P.bCastShadows = false;
		P.Collections = S.Collections; P.bSurfaceScatterPalette = true;
		FAPSFoliageActivationPlan Normalized;
		return APSWorldScapeFoliage::NormalizePlan(P, Normalized) ? Normalized : FAPSFoliageActivationPlan{};
	}
	if (RuntimeMode == 2)
	{
		const bool bEligible = !bManualPlanet && !Profile.Foliage.bEnabled
			&& Profile.Foliage.Collections.IsEmpty()
			&& (Profile.PlanetType == EPlanetType::Frozen || Profile.PlanetType == EPlanetType::Forest);
		return BuildActivationPlan(Profile, bEligible, bScaledOrbitalPreview, true);
	}
	return BuildActivationPlan(Profile, RuntimeMode == 1, bScaledOrbitalPreview, bPrototypeOptIn);
}

bool FAPSWorldScapeFoliagePolicy::IsRuntimeOptInEnabled()
{
	return APSWorldScapeFoliage::CVarEnable.GetValueOnGameThread() > 0;
}

bool FAPSWorldScapeFoliagePolicy::HasPendingNativeWorker(const AWorldScapeRoot* Root)
{
	check(IsInGameThread());
	// The worker clears FoliageGenerationInProgress before returning from DoWork.
	// Only FAsyncTask's completion fence proves it no longer uses the root/queues.
	// Never cancel/delete/join it here; native EndPlay retains ownership.
	return IsValid(Root) && Root->FoliageGenerationThreadPool
		&& !Root->FoliageGenerationThreadPool->IsDone();
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
	// Allocate across EVERY permitted collection. Screen-distance culling does
	// not release retained sectors, HISM components or queued transforms. The
	// extra 27 sectors account for one batch before the old sectors are retired.
	const int32 SectorEnvelope = PeakSectorEnvelopePerType * SafePlan.MaxCollections;
	const int32 CollectionInstanceCap = FMath::Min(
		SafePlan.MaxInstancesPerSectorPerCollection, MaximumPeakInstancesPerRoot / SectorEnvelope);
	const int32 CollectionMeshSlots = MaximumPeakMeshComponentsPerRoot / SectorEnvelope;
	const int32 HabitatBudget = FMath::Clamp(
		FMath::FloorToInt(static_cast<float>(SafePlan.MaxInstancesPerSectorPerCollection)
			* SafePlan.HabitatDensityScale), 1, CollectionInstanceCap);

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
		const int32 TypeLimit = FMath::Min3(SafePlan.MaxTypesPerCollection, CollectionMeshSlots, HabitatBudget);
		SupportedEntries.Reserve(TypeLimit);
		for (UWorldScapeFoliagesInterface* Entry : SourceCollection->FoliageList)
		{
			if (SupportedEntries.Num() >= TypeLimit)
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

		// Preserve the authored palette's relative density instead of flattening every
		// mesh type to the same cap. One slot per supported type keeps rare accent
		// meshes viable; the remainder is apportioned deterministically by authored
		// FoliagesCount and never exceeds the collection budget.
		const TArray<int32> PerTypeBudgets =
			APSWorldScapeFoliage::BuildPerTypeBudgets(SupportedEntries, HabitatBudget);
		if (PerTypeBudgets.Num() != SupportedEntries.Num())
		{
			continue;
		}

		int32 RemainingMeshSlots = CollectionMeshSlots;
		for (int32 EntryIndex = 0; EntryIndex < SupportedEntries.Num(); ++EntryIndex)
		{
			UWorldScapeFoliagesInterface* SourceEntry = SupportedEntries[EntryIndex];
			const int32 BudgetPerType = PerTypeBudgets[EntryIndex];
			// Authored data assets carry Standalone/Public. Retaining those flags on
			// transient duplicates can keep a retired palette alive in editor GC.
			// Mask them at duplication (including duplicated inner objects), not on
			// the source and not merely by adding RF_Transient afterwards.
			UWorldScapeFoliagesInterface* Entry =
				Cast<UWorldScapeFoliagesInterface>(StaticDuplicateObject(SourceEntry, Collection,
					NAME_None, RF_AllFlags & ~(RF_Public | RF_Standalone)));
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
				--RemainingMeshSlots;
			}
			else if (Entry->GetClass() == UWorldScapeFoliagesCluster::StaticClass())
			{
				// A cluster creates one component per retained unit, even if some
				// units emit no instances. Reserve one slot for each remaining type.
				FAPSFoliageActivationPlan ClusterPlan = SafePlan;
				ClusterPlan.MaxClusterMeshesPerType = FMath::Min(
					SafePlan.MaxClusterMeshesPerType,
					RemainingMeshSlots - (SupportedEntries.Num() - EntryIndex - 1));
				auto* Cluster = static_cast<UWorldScapeFoliagesCluster*>(Entry);
				const int32 ClusterExpansion = APSWorldScapeFoliage::SanitizeCluster(
					Cluster, ClusterPlan, BudgetPerType);
				if (ClusterExpansion <= 0)
				{
					continue;
				}
				MaximumSpawnGroups = FMath::Max(BudgetPerType / ClusterExpansion, 1);
				RemainingMeshSlots -= Cluster->FoliagesClusterUnitList.Num();
			}
			else
			{
				continue;
			}

			const int32 AuthoredSpawnGroups = FMath::Max(
				FMath::FloorToInt32(
					APSWorldScapeFoliage::GetAuthoredDensityWeight(SourceEntry)),
				1);
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

int32 FAPSWorldScapeFoliagePolicy::GetReservedRootCount(const UWorld* World)
{
	APSWorldScapeFoliage::PruneWorldReservations();
	if (!IsValid(World)) return 0;
	int32 Count = 0;
	for (const auto& Entry : APSWorldScapeFoliage::WorldReservations)
	{
		if (Entry.World.Get() == World) ++Count;
	}
	return Count;
}

int32 FAPSWorldScapeFoliagePolicy::ApplyToFreshOwnedRuntimeRoot(
	AWorldScapeRoot* Root,
	const FAPSResolvedPlanetSurfaceProfile& Profile,
	bool bScaledOrbitalPreview, bool bManualPlanet)
{
	if (!IsValid(Root))
	{
		return 0;
	}
	check(IsInGameThread());
	// Defend the fresh-root contract before clearing arrays that a worker can
	// reference. A repeated call must neither release its lease nor hot-swap data.
	if (APSWorldScapeFoliage::HasReservation(Root)
		|| Root->FoliageGenerationThreadPool != nullptr
		|| !Root->FoliageDataList.IsEmpty() || !Root->FoliageDataToSpawn.IsEmpty())
	{
		UE_LOG(LogTemp, Display, TEXT("APS_FOLIAGE_ADMISSION root=%s existingStatePreserved=1"), *Root->GetPathName());
		return Root->Foliages.Num();
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

	const FAPSFoliageActivationPlan Plan = BuildRuntimeActivationPlan(
		Profile, APSWorldScapeFoliage::CVarEnable.GetValueOnGameThread(), bScaledOrbitalPreview,
		APSWorldScapeFoliage::CVarPrototype.GetValueOnGameThread() == 1, bManualPlanet,
		APSPlanetSurfaceScatter::UsesScatter(Profile, APSWorldScapeFoliage::CVarSurfaceScatter.GetValueOnGameThread()));
	if (!Plan.bEnabled)
	{
		return 0;
	}
	if (Plan.bSurfaceScatterPalette)
	{
		const auto* Revision = IConsoleManager::Get().FindConsoleVariable(TEXT("worldscape.APS.FoliageSeedRevision"));
		if (!Revision || Revision->GetInt() != 2)
		{
			UE_LOG(LogTemp, Warning, TEXT("APS_FOLIAGE_ADMISSION denied=MissingScatterSeedV2; no silent repetitive fallback"));
			return 0;
		}
		// Immutable for this root's lifetime. Native worker's corrected stream is
		// restricted to these owned palettes; old/authored distributions are kept.
		Root->Tags.AddUnique(FName(TEXT("APS.SurfaceScatter.V2")));
	}
	UWorld* World = Root->GetWorld();
	if (!IsValid(World) || GetReservedRootCount(World) >= MaximumAdmittedRootsPerWorld)
	{
		UE_LOG(LogTemp, Display, TEXT("APS_FOLIAGE_ADMISSION root=%s denied=WorldBudget collectionsLoaded=0; fresh-root retry only"),
			*Root->GetPathName());
		return 0;
	}

	Root->WPO_FoliageDisabledDistance = FMath::Clamp(
		FMath::RoundToInt(Plan.MinSectorSizeCm * 2.0f), 10000, 100000);
	Root->FoliageMinimuMSpawnChanceToNotIgnore = FMath::Max(
		Root->FoliageMinimuMSpawnChanceToNotIgnore, 0.02f);

	TArray<UWorldScapeFoliagesCollection*> Sources;
	Sources.Reserve(Plan.MaxCollections);
	for (const TSoftObjectPtr<UWorldScapeFoliagesCollection>& SoftCollection : Plan.Collections)
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
	if (Plan.bSurfaceScatterPalette && Profile.PlanetType == EPlanetType::Oasis
		&& APSPlanetSurfaceScatter::HasHabitat(Profile))
	{
		for (auto* Collection : BudgetedCollections)
		{
			UWorldScapeFoliagesAsset* Tree = nullptr;
			int32 Total = 0;
			bool bOwnedStaticPalette = Collection && Collection->FoliageList.Num() == 5;
			if (!bOwnedStaticPalette) continue;
			for (auto* Item : Collection->FoliageList)
			{
				auto* Asset = Cast<UWorldScapeFoliagesAsset>(Item);
				if (!Asset || !IsValid(Asset->StaticMesh) || !FMath::IsFinite(Asset->FoliagesCount)
					|| Asset->FoliagesCount < 1 || Asset->FoliagesCount > 64
					|| !Asset->StaticMesh->GetPathName().StartsWith(FString(APSPlanetSurfaceScatter::Root)+TEXT("/")))
				{ bOwnedStaticPalette = false; break; }
				Total += FMath::CeilToInt(Asset->FoliagesCount);
				if (Asset->StaticMesh->GetName() == TEXT("SM_APS_Scatter_TreeA") && Asset->bUseFoliageNoiseMask) Tree = Asset;
			}
			if (!bOwnedStaticPalette || !Tree) continue;
			const int32 Before = FMath::CeilToInt(Tree->FoliagesCount);
			const int32 After = APSPlanetSurfaceScatter::BoundedOasisTreeAttempts(Before, Total - Before,
				APSWorldScapeFoliage::CVarOasisTreeAttempts.GetValueOnGameThread(), Plan.MaxInstancesPerSectorPerCollection);
			Tree->FoliagesCount = After;
			UE_LOG(LogTemp, Display, TEXT("APS_FOLIAGE_OASIS_TREE_BUDGET before=%d after=%d total=%d cap=%d climateMask=unchanged; transient owned palette only"),
				Before, After, Total - Before + After, Plan.MaxInstancesPerSectorPerCollection);
		}
	}
	if (Plan.bSurfaceScatterPalette)
	{
		// Only owned scatter entries. Do not mutate meshes, saved palettes,
		// authored assets, or accepted terrain. Share MIDs within each root.
		TMap<int32, UMaterialInstanceDynamic*> Minerals;
		TMap<FString, UMaterialInstanceDynamic*> Leaves;
		for (auto* Collection : BudgetedCollections)
			for (auto* Item : Collection->FoliageList)
			{
				auto* Entry = Cast<UWorldScapeFoliagesAsset>(Item);
				if (!Entry || !IsValid(Entry->StaticMesh)
					|| !Entry->StaticMesh->GetPathName().StartsWith(FString(APSPlanetSurfaceScatter::Root)+TEXT("/"))) continue;
				for (int32 I=0; I<5; ++I)
				{
					if (Entry->StaticMesh->GetName() != FString(TEXT("SM_APS_Scatter_"))+APSPlanetScatterMaterial::Shapes[I]) continue;
					auto*& MID=Minerals.FindOrAdd(I);
					if (!MID)
					{
						auto* Template=LoadObject<UMaterialInterface>(nullptr,*APSPlanetScatterMaterial::Path(I));
						if (!Template)
						{ UE_LOG(LogTemp,Warning,TEXT("APS_FOLIAGE_ADMISSION denied=MissingScatterMaterial")); return 0; }
						MID=UMaterialInstanceDynamic::Create(Template,Root);
						if (!MID) return 0;
						MID->SetVectorParameterValue(TEXT("RockTint"),APSPlanetScatterMaterial::Tint(Profile.PlanetType));
					}
					Entry->OverrideMaterial.Init(MID,Entry->StaticMesh->GetStaticMaterials().Num());
				}
				// TreeB's old parent is absent from the vendor package; its authored
				// atlas is the same tree_Color used by the intact fixed leaf master.
				// Preserve each role's atlas (including billboard UVs), not a generic
				// replacement texture. This affects owned scatter entries only.
				for (auto& Slot : Entry->OverrideMaterial)
				{
					const FString Path=GetPathNameSafe(Slot);
					const bool BrokenLeaf=Path==TEXT("/WorldScape/Ressources/Mesh/Tree/MI_Grass_Leaf1.MI_Grass_Leaf1");
					const bool Billboard=Path==TEXT("/WorldScape/Ressources/Mesh/Tree/MI_LeafLOD.MI_LeafLOD");
					// Owned temperate/cold/dry cards share the vendor ObjectRadius
					// fade that erased the accepted Terrestrial near grass. Apply
					// the same atlas-preserving fix to every admitted role and LOD,
					// not only one planet type. Publication/habitat gates stay intact.
					const bool Grass = APSFoliageLeafMaterial::NeedsOwnedGrassFade(
						Entry->StaticMesh->GetName(), Path);
					if (!BrokenLeaf && !Billboard && !Grass) continue;
					auto*& MID=Leaves.FindOrAdd(Path);
					if (!MID)
					{
						UTexture* Color=nullptr; UTexture* Normal=nullptr;
						Slot->GetTextureParameterValue(FMaterialParameterInfo(BrokenLeaf?TEXT("TextureBaseColor"):TEXT("MW_TextureBaseColor")),Color);
						Slot->GetTextureParameterValue(FMaterialParameterInfo(BrokenLeaf?TEXT("TextureNormal"):TEXT("MW_TextureNormal")),Normal);
						auto* Template=LoadObject<UMaterialInterface>(nullptr,APSFoliageLeafMaterial::TemplatePath);
						if (!Color || !Normal || !Template)
						{ UE_LOG(LogTemp,Warning,TEXT("APS_FOLIAGE_ADMISSION denied=MissingScatterLeafTexture")); return 0; }
						MID=UMaterialInstanceDynamic::Create(Template,Root);
						if (!MID) return 0;
						MID->SetTextureParameterValue(TEXT("MW_TextureBaseColor"),Color);
						MID->SetTextureParameterValue(TEXT("MW_TextureNormal"),Normal);
					}
					Slot=MID;
				}
			}
	}
	Root->Foliages = MoveTemp(BudgetedCollections);
	if (!Root->Foliages.IsEmpty())
	{
		// Loading soft assets above can execute user code. Recheck before committing
		// so a re-entrant admission cannot consume the same last world slot twice.
		if (GetReservedRootCount(World) >= MaximumAdmittedRootsPerWorld)
		{
			Root->Foliages.Reset();
			return 0;
		}
		APSWorldScapeFoliage::WorldReservations.Add({ World, Root });
		UE_LOG(LogTemp, Display, TEXT("APS_FOLIAGE_ADMISSION root=%s reservedRoots=%d/%d potentialInstancesPerWorld=%d potentialComponentsPerWorld=%d; admission envelope, not measured memory"),
			*Root->GetPathName(), GetReservedRootCount(World), MaximumAdmittedRootsPerWorld,
			MaximumPeakInstancesPerWorld, MaximumPeakMeshComponentsPerWorld);
	}
	Root->bGenerateFoliages = !Root->Foliages.IsEmpty();
	if (Root->bGenerateFoliages && Plan.bSurfaceScatterPalette)
	{
		auto* Exclusion = NewObject<UAPSFoliageExclusionComponent>(Root, NAME_None, RF_Transient);
		Root->AddInstanceComponent(Exclusion);
		Exclusion->RegisterComponent();
		Exclusion->AddTickPrerequisiteActor(Root);
		auto* Collision = NewObject<UAPSFoliageCollisionComponent>(Root, NAME_None, RF_Transient);
		Root->AddInstanceComponent(Collision);
		Collision->RegisterComponent();
		Collision->AddTickPrerequisiteComponent(Exclusion);
	}
	return Root->Foliages.Num();
}
