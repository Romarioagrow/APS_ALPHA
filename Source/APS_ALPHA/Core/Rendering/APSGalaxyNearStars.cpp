#include "APSGalaxyNearStars.h"

#include "APSGalaxyGpuStars.h"
#include "APSStellarMaterialContract.h"
#include "APS_ALPHA/Actors/Astro/Galaxy.h"
#include "APS_ALPHA/Actors/Astro/StarSystem.h"
#include "APS_ALPHA/Core/Rendering/APSCanonicalStellarProjection.h"
#include "APS_ALPHA/Generation/StarGenerator.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "HAL/IConsoleManager.h"
#include "HAL/PlatformTime.h"
#include "MaterialShared.h"
#include "Materials/Material.h"

namespace
{
	TAutoConsoleVariable<int32> CVarNearStars(TEXT("aps.Stars.GalaxyNearStars"), 1,
		TEXT("Rio 03.10: 1 draws the galaxy's GPU-only stars near the camera that are at least aps.Stars.ResolvePixels in ")
		TEXT("radius as crisp photospheres (a point grows into its star on approach). 0: they stay GPU points."));
	TAutoConsoleVariable<int32> CVarNearCount(TEXT("aps.Stars.GalaxyNearCount"), 256,
		TEXT("How many of the nearest drawn catalogue stars aps.Stars.GalaxyNearStars looks at (the farther ones are points)."));

	struct FNearState
	{
		TWeakObjectPtr<AGalaxy> Galaxy;
		TWeakObjectPtr<UInstancedStaticMeshComponent> Mesh;
		/** Catalogue index -> instance, and back (a removal swaps the last instance into the hole). */
		TMap<int64, int32> InstanceOf;
		TArray<int64> Keys;
		TSet<int64> Materialized;
		double NextQuerySeconds{0.0};
		int32 LoggedCount{INDEX_NONE};
	};
	TMap<TWeakObjectPtr<const UWorld>, FNearState> GStates;

	double ResolvePixels()
	{
		static IConsoleVariable* Variable = IConsoleManager::Get().FindConsoleVariable(TEXT("aps.Stars.ResolvePixels"));
		return Variable ? FMath::Max(static_cast<double>(Variable->GetFloat()), 1.0) : 1.0;
	}

	bool SurfaceReady(UMaterial* Surface, const UWorld* World)
	{
		if (!World || !APSStellarMaterialContract::HasExactBase(Surface, APSStellarMaterialContract::ActorBaseObjectPath))
		{
			return false;
		}
		const FMaterialResource* Resource = Surface->GetMaterialResource(World->GetFeatureLevel());
		return Resource && Resource->GetGameThreadShaderMap() != nullptr;
	}

	void ClearInstances(FNearState& State)
	{
		if (UInstancedStaticMeshComponent* Mesh = State.Mesh.Get(); IsValid(Mesh) && Mesh->GetInstanceCount() > 0)
		{
			Mesh->ClearInstances();
		}
		State.InstanceOf.Reset();
		State.Keys.Reset();
	}

	void DestroyMesh(FNearState& State)
	{
		ClearInstances(State);
		if (UInstancedStaticMeshComponent* Mesh = State.Mesh.Get(); IsValid(Mesh))
		{
			Mesh->DestroyComponent();
		}
		State.Mesh.Reset();
	}

	/** One photosphere component in the galaxy catalogue component's own frame (origin shifts move it with it). */
	UInstancedStaticMeshComponent* EnsureMesh(FNearState& State, AGalaxy& Galaxy)
	{
		if (UInstancedStaticMeshComponent* Existing = State.Mesh.Get(); IsValid(Existing)) return Existing;
		UHierarchicalInstancedStaticMeshComponent* Source = Galaxy.StarMeshInstances;
		UMaterial* Surface = APSStellarMaterialContract::LoadCanonicalBase(APSStellarMaterialContract::ActorBaseObjectPath);
		if (!IsValid(Source) || !IsValid(Source->GetStaticMesh()) || !SurfaceReady(Surface, Galaxy.GetWorld())) return nullptr;
		UInstancedStaticMeshComponent* Mesh = NewObject<UInstancedStaticMeshComponent>(&Galaxy, NAME_None, RF_Transient);
		Mesh->SetupAttachment(Source);
		Mesh->SetMobility(EComponentMobility::Movable);
		Mesh->bDisallowNanite = true;
		Mesh->SetForceDisableNanite(true);
		Mesh->SetStaticMesh(Source->GetStaticMesh());
		Mesh->SetForcedLodModel(1);
		for (int32 Slot = 0; Slot < FMath::Max(Mesh->GetNumMaterials(), 1); ++Slot) Mesh->SetMaterial(Slot, Surface);
		Mesh->SetNumCustomDataFloats(6);
		Mesh->SetRemoveSwap();
		Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Mesh->bDisableCollision = true;
		Mesh->SetGenerateOverlapEvents(false);
		Mesh->SetCanEverAffectNavigation(false);
		Mesh->SetCastShadow(false);
		Mesh->bAffectDynamicIndirectLighting = false;
		Mesh->bAffectDistanceFieldLighting = false;
		Mesh->SetReceivesDecals(false);
		Galaxy.AddInstanceComponent(Mesh);
		Mesh->RegisterComponent();
		State.Mesh = Mesh;
		return Mesh;
	}

	void RemoveInstance(FNearState& State, const int64 CatalogIndex)
	{
		int32 Instance = INDEX_NONE;
		if (!State.InstanceOf.RemoveAndCopyValue(CatalogIndex, Instance) || !State.Keys.IsValidIndex(Instance)) return;
		UInstancedStaticMeshComponent* Mesh = State.Mesh.Get();
		const int32 Last = State.Keys.Num() - 1;
		if (IsValid(Mesh) && Mesh->GetInstanceCount() == State.Keys.Num()) Mesh->RemoveInstance(Instance);
		if (Instance != Last)
		{
			State.Keys[Instance] = State.Keys[Last];
			State.InstanceOf.Add(State.Keys[Instance], Instance);
		}
		State.Keys.Pop(EAllowShrinking::No);
	}

	bool AddInstance(FNearState& State, AGalaxy& Galaxy, const FGalaxyCatalogStarRecord& Record, const double RadiusCm)
	{
		UInstancedStaticMeshComponent* Mesh = EnsureMesh(State, Galaxy);
		UStarGenerator* Stars = GetMutableDefault<UStarGenerator>();
		if (!Mesh || !IsValid(Mesh->GetStaticMesh()) || !IsValid(Galaxy.StarMeshInstances)) return false;
		const FBoxSphereBounds MeshBounds = Mesh->GetStaticMesh()->GetBounds();
		const double ComponentScale = Galaxy.StarMeshInstances->GetComponentScale().GetAbsMax();
		const double Scale = ComponentScale > 0.0 && MeshBounds.BoxExtent.GetMax() > 0.0
			? RadiusCm / ComponentScale / MeshBounds.BoxExtent.GetMax() : 0.0;
		if (!FMath::IsFinite(Scale) || Scale <= 0.0) return false;
		const FVector Centre = Galaxy.CanonicalProjectionFrame.ProjectCanonicalUnits(Record.GalaxyLocalLocation);
		const int32 Instance = Mesh->AddInstance(FTransform(FQuat::Identity, Centre - MeshBounds.Origin * Scale, FVector(Scale)), false);
		if (Instance == INDEX_NONE) return false;
		// The catalogue preset of the resolved catalogue stars (APSGameplayNativeStars' galaxy branch).
		const FLinearColor Color = UStarGenerator::GetStarColor(Record.SpectralClass, Record.SpectralSubclass);
		const float Data[6] = {Color.R, Color.G, Color.B,
			static_cast<float>(Stars->CalculateEmission(static_cast<float>(
				APSCanonicalStellarProjection::GetCanonicalStellarLuminositySolar(Record.SpectralClass)
				* FMath::Clamp(static_cast<double>(Record.RadiusScale) * Record.RadiusScale, 1.0e-4, 1.0e4) * 25.0))),
			static_cast<float>(Record.GenerationSeed & 0xffff) / 65535.0f, 0.0f};
		Mesh->SetCustomData(Instance, TArrayView<const float>(Data, 6), false);
		if (State.Keys.Num() <= Instance) State.Keys.SetNum(Instance + 1);
		State.Keys[Instance] = Record.CatalogIndex;
		State.InstanceOf.Add(Record.CatalogIndex, Instance);
		return true;
	}
}

namespace APSGalaxyNearStars
{
	void Update(UWorld* World, const FVector& Camera, const double PixelTangent, const bool bDaylightHidden)
	{
		if (!World)
		{
			return;
		}
		FNearState& State = GStates.FindOrAdd(TWeakObjectPtr<const UWorld>(World));
		AGalaxy* Galaxy = APSGalaxyGpuStars::GetIndexedGalaxy(World);
		if (State.Galaxy.Get() != Galaxy)
		{
			DestroyMesh(State);
			State.Galaxy = Galaxy;
		}
		if (CVarNearStars.GetValueOnGameThread() == 0 || !Galaxy || bDaylightHidden || !(PixelTangent > 0.0))
		{
			ClearInstances(State);
			return;
		}
		const double Now = FPlatformTime::Seconds();
		if (Now < State.NextQuerySeconds)
		{
			return;
		}
		State.NextQuerySeconds = Now + 0.1;
		TArray<APSGalaxyGpuStars::FNearStar> Stars;
		if (!APSGalaxyGpuStars::FindNearStars(World, Camera, FMath::Clamp(CVarNearCount.GetValueOnGameThread(), 1, 4096),
			1.0e30, Stars))
		{
			return;
		}
		// The GPU layer hides its points inside the home and the materialized systems: so are their photospheres.
		TArray<FVector4> Hidden;
		for (TActorIterator<AStarSystem> It(World); It; ++It)
		{
			if (IsValid(*It) && It->StarSystemRadius > 0.0)
			{
				Hidden.Add(FVector4(It->GetActorLocation(),
					It->StarSystemRadius * APSCanonicalStellarProjection::SystemProxyExclusionPadding));
			}
		}
		const int32 Prefix = IsValid(Galaxy->StarMeshInstances) ? Galaxy->StarMeshInstances->GetInstanceCount() : 0;
		const double Resolve = ResolvePixels();
		TSet<int64> Wanted;
		for (const APSGalaxyGpuStars::FNearStar& Star : Stars)
		{
			// The galaxy HISM's own stars are the catalogue's resolved tier (APSGameplayNativeStars).
			if (Star.Ordinal < Prefix || State.Materialized.Contains(Star.CatalogIndex)) continue;
			if (Hidden.ContainsByPredicate([&Star](const FVector4& Sphere)
				{
					return FVector::DistSquared(Star.WorldLocation, FVector(Sphere)) <= FMath::Square(Sphere.W);
				})) continue;
			FGalaxyCatalogStarRecord Record;
			if (!Galaxy->StarCatalog.ResolveStar(Star.CatalogIndex, Record)) continue;
			const double RadiusCm = APSCanonicalStellarProjection::GetCanonicalStellarRadiusSolar(Record.SpectralClass)
				* FMath::Max(static_cast<double>(Record.RadiusScale), 0.0) * APSCanonicalStellarProjection::SolarRadiusCm;
			const double PixelRadius = RadiusCm / FMath::Max(Star.DistanceCm * PixelTangent, 1.0e-6);
			const bool bShown = State.InstanceOf.Contains(Star.CatalogIndex);
			if (!(PixelRadius >= (bShown ? Resolve * 0.83 : Resolve))) continue;
			Wanted.Add(Star.CatalogIndex);
			if (!bShown)
			{
				AddInstance(State, *Galaxy, Record, RadiusCm);
			}
		}
		TArray<int64> Gone;
		for (const TPair<int64, int32>& Entry : State.InstanceOf)
		{
			if (!Wanted.Contains(Entry.Key)) Gone.Add(Entry.Key);
		}
		for (const int64 CatalogIndex : Gone)
		{
			RemoveInstance(State, CatalogIndex);
		}
		if (State.InstanceOf.Num() != State.LoggedCount)
		{
			State.LoggedCount = State.InstanceOf.Num();
			UE_LOG(LogTemp, Log, TEXT("[APS.Stars] galaxy near photospheres=%d (of the %d nearest drawn stars)"),
				State.InstanceOf.Num(), Stars.Num());
		}
	}

	void Reset(const UWorld* World)
	{
		if (FNearState* State = GStates.Find(TWeakObjectPtr<const UWorld>(World)))
		{
			DestroyMesh(*State);
			GStates.Remove(TWeakObjectPtr<const UWorld>(World));
		}
	}

	void SetMaterialized(const UWorld* World, const int64 CatalogIndex, const bool bMaterialized)
	{
		FNearState& State = GStates.FindOrAdd(TWeakObjectPtr<const UWorld>(World));
		if (bMaterialized)
		{
			State.Materialized.Add(CatalogIndex);
			RemoveInstance(State, CatalogIndex);
		}
		else
		{
			State.Materialized.Remove(CatalogIndex);
		}
	}
}
