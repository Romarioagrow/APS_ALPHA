#include "AstroGenerator.h"

#include "APS_ALPHA/Actors/Astro/Galaxy.h"
#include "APS_ALPHA/Actors/Astro/Moon.h"
#include "APS_ALPHA/Actors/Astro/Planet.h"
#include "APS_ALPHA/Actors/Astro/PlanetarySystem.h"
#include "APS_ALPHA/Actors/Astro/Star.h"
#include "APS_ALPHA/Actors/Astro/StarCluster.h"
#include "APS_ALPHA/Actors/Astro/StarSystem.h"
#include "APS_ALPHA/Generation/StarGenerator.h"
#include "APS_ALPHA/Core/Rendering/APSStellarMaterialContract.h"
#include "APS_ALPHA/Core/Rendering/APSPreviewVisibility.h"
#include "Camera/CameraComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/SphereComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineGlobals.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "ProceduralMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "ProfilingDebugging/CsvProfiler.h"

CSV_DEFINE_CATEGORY(APSPreview, true);

namespace
{
// The ordinary menu and its route tests must use the same physical observer.
// Value 0 remains a diagnostic rollback, never a different catalog or generator.
TAutoConsoleVariable<int32> CVarContinuousPreviewFrame(
	TEXT("aps.Preview.ContinuousFrame"), 1,
	TEXT("Use the common physical observer for every generation-menu scope (new preview required)."));

TAutoConsoleVariable<int32> CVarResolvedStarPreparation(
	TEXT("aps.Preview.ResolvedStarPreparation"), 1,
	TEXT("Prepare bounded hidden stellar component capacity during an existing continuous flight; never delay visible stars."));

bool NeedsResolvedStarView(const FAPSContinuousPreviewPoint& Point,
	const FAPSContinuousPreviewFrame& Frame, const FQuat& CameraRotation,
	const double PixelTangent, const double TanHalfHorizontal, const double TanHalfVertical,
	const bool bWasResolved, FAPSPreviewProjectedSphere& Sphere)
{
	if (Point.MaterializedStar.IsValid() || !Point.StarModel.IsValid()
		|| !Frame.ProjectSphere(Point.CenterCm, Point.RadiusCm, Sphere)) return false;
	const double Pixels = Sphere.Radius / FMath::Max(Sphere.Center.Size() * PixelTangent, 1.0e-12);
	if (Pixels < (bWasResolved ? 0.1 : 0.15)) return false;
	// Same physical corona shell and optical guard for both presentation and the
	// capacity forecast. Forecasts never select or relocate a visible object.
	const double GuardedRadius = Sphere.Radius * 1.14 + Sphere.Center.Size() * PixelTangent * 32.0;
	return APSPreviewVisibility::SphereIntersectsView(CameraRotation.UnrotateVector(Sphere.Center),
		GuardedRadius, TanHalfHorizontal, TanHalfVertical);
}

double PhysicalBodyRadiusCm(const AActor* Actor)
{
	if (const AStar* Star = Cast<AStar>(Actor))
	{
		// The legacy UI kilometre field is an integer (up to 5% error for a 10 km
		// compact star). Project the same precise radius used by its physical model.
		const double RadiusKm = Star->RadiusKM > 0.0 ? Star->RadiusKM : Star->StarRadiusKM;
		return FMath::Max(RadiusKm, 0.001) * 1.0e5;
	}
	if (const APlanetaryBody* Body = Cast<APlanetaryBody>(Actor))
	{
		const double RadiusKm = Body->RadiusKM > 0.0 ? Body->RadiusKM : Body->PlanetRadiusKM;
		return FMath::Max(RadiusKm, 0.001) * 1.0e5;
	}
	return 0.0;
}

AStar* PhysicalParentStar(APlanetaryBody* Body)
{
	APlanet* Planet = Cast<APlanet>(Body);
	if (AMoon* Moon = Cast<AMoon>(Body)) Planet = Moon->ParentPlanet;
	return IsValid(Planet) ? Planet->ParentStar : nullptr;
}

void PresentPhysicalMesh(UStaticMeshComponent* Mesh, const FAPSPreviewProjectedSphere& Sphere,
	const FQuat& Rotation)
{
	CSV_SCOPED_TIMING_STAT(APSPreview, PhysicalMesh);
	if (!IsValid(Mesh) || !IsValid(Mesh->GetStaticMesh())) return;
	const FBoxSphereBounds AssetBounds = Mesh->GetStaticMesh()->GetBounds();
	const double AssetRadius = AssetBounds.BoxExtent.GetMax();
	if (AssetRadius <= UE_SMALL_NUMBER) return;
	const double Scale = Sphere.Radius / AssetRadius;
	Mesh->SetAbsolute(true, true, true);
	Mesh->SetWorldTransform(FTransform(Rotation,
		Sphere.Center - Rotation.RotateVector(AssetBounds.Origin * Scale), FVector(Scale)),
		false, nullptr, ETeleportType::TeleportPhysics);
	// SetWorldTransform sets location/rotation before scale. UE's subsequent scale
	// update can retain the previous ComponentToWorld when the absolute delta is
	// below UE_SMALL_NUMBER. Astronomical assets legitimately need such tiny scales;
	// even a large relative radius change can otherwise lag or stick after a flight.
	// Force propagation only when that cache still disagrees with the requested scale.
	if (Mesh->GetComponentScale() != FVector(Scale))
		Mesh->UpdateComponentToWorld(EUpdateTransformFlags::None, ETeleportType::TeleportPhysics);
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Mesh->UpdateBounds();
}
}

bool AAstroGenerator::UsesContinuousPreviewFrame() const
{
	return bIsPreviewGeneration && IsCanonicalStellarProjectionEnabled()
		&& CVarContinuousPreviewFrame.GetValueOnGameThread() != 0;
}

void AAstroGenerator::RememberContinuousPreviewBody(AActor* Body)
{
	if (AStarSystem* System = GetContinuousPreviewOwningSystem(Body))
	{
		SelectedPreviewClusterSystemIndex = INDEX_NONE;
		for (const auto& Entry : ContinuousMaterializedSystems)
			if (Entry.Value == System) { SelectedPreviewClusterSystemIndex = Entry.Key; break; }
	}
	if (APlanetaryBody* Planet = Cast<APlanetaryBody>(Body))
	{
		ContinuousSelectedPlanet = Planet;
		ContinuousSelectedStar = PhysicalParentStar(Planet);
	}
	else if (AStar* Star = Cast<AStar>(Body))
	{
		ContinuousSelectedStar = Star;
	}
}

bool AAstroGenerator::GetContinuousPreviewPhysicalFocus(const EAstroPreviewFocus Focus,
	FVector& CenterCm, double& RadiusCm) const
{
	CenterCm = FVector::ZeroVector;
	RadiusCm = 0.0;
	if ((Focus == EAstroPreviewFocus::Overview || Focus == EAstroPreviewFocus::Galaxy)
		&& IsValid(GeneratedGalaxy))
	{
		const FAPSCanonicalStellarProjectionFrame& Frame = GeneratedGalaxy->CanonicalProjectionFrame;
		CenterCm = Frame.GetCanonicalRootPositionCm(FVector::ZeroVector) - Frame.CanonicalAnchorCm;
		RadiusCm = GeneratedGalaxy->StarCatalog.CatalogHalfExtent.GetAbsMax()
			* Frame.LayerToRootPositionScale * Frame.CanonicalCmPerUnit;
	}
	else if ((Focus == EAstroPreviewFocus::Overview || Focus == EAstroPreviewFocus::StarCluster)
		&& IsValid(GeneratedStarCluster))
	{
		const FAPSCanonicalStellarProjectionFrame& Frame = GeneratedStarCluster->CanonicalProjectionFrame;
		CenterCm = Frame.GetCanonicalRootPositionCm(FVector::ZeroVector) - Frame.CanonicalAnchorCm;
		RadiusCm = Frame.CanonicalHalfExtentUnits * Frame.LayerToRootPositionScale * Frame.CanonicalCmPerUnit;
	}
	else if (Focus == EAstroPreviewFocus::HomeSystem && IsValid(GetContinuousPreviewActiveSystem()))
	{
		const AStarSystem* System = GetContinuousPreviewActiveSystem();
		CenterCm = GetContinuousPreviewSystemCenter(System);
		for (const TWeakObjectPtr<AActor>& WeakBody : ContinuousPreviewBodies)
		{
			if (const AActor* Body = WeakBody.Get(); Body && GetContinuousPreviewOwningSystem(Body) == System)
			{
				RadiusCm = FMath::Max(RadiusCm,
					(Body->GetActorLocation() - System->GetActorLocation()).Size()
					+ PhysicalBodyRadiusCm(Body));
			}
		}
	}
	else
	{
		const AActor* Body = SelectedPreviewBodyActor.Get();
		if (Focus == EAstroPreviewFocus::HomeStar && !Cast<AStar>(Body))
			Body = ContinuousSelectedStar.IsValid() ? ContinuousSelectedStar.Get() : HomeStar;
		if (Focus == EAstroPreviewFocus::HomePlanet && !Cast<APlanetaryBody>(Body))
			Body = ContinuousSelectedPlanet.IsValid() ? ContinuousSelectedPlanet.Get() : HomePlanet;
		if (!IsValid(Body) || !IsValid(GeneratedHomeStarSystem)) return false;
		CenterCm = GetContinuousPreviewPhysicalPosition(Body);
		RadiusCm = PhysicalBodyRadiusCm(Body);
	}
	return !CenterCm.ContainsNaN() && FMath::IsFinite(RadiusCm) && RadiusCm > 0.0;
}

void AAstroGenerator::ClearContinuousPreviewPresentation()
{
	for (const auto& Entry : ContinuousResolvedStarViews)
		for (UStaticMeshComponent* Mesh : {Entry.Value.Photosphere.Get(), Entry.Value.Corona.Get()})
			if (IsValid(Mesh)) Mesh->DestroyComponent();
	ContinuousResolvedStarViews.Reset();
	for (const FAPSContinuousResolvedStarView& View : ContinuousResolvedStarPool)
		for (UStaticMeshComponent* Mesh : {View.Photosphere.Get(), View.Corona.Get()})
			if (IsValid(Mesh)) Mesh->DestroyComponent();
	ContinuousResolvedStarPool.Reset();
	ContinuousResolvedStarAllocations = 0;
	ContinuousResolvedStarReuses = 0;
	ContinuousResolvedStarPreparations = 0;
	ContinuousResolvedForecastStep = ContinuousResolvedForecastCapacity = 0;
	ContinuousResolvedLastPreparationFrame = MAX_uint64;
	for (UInstancedStaticMeshComponent* Component : { ContinuousGalaxyView.Get(), ContinuousClusterView.Get() })
	{
		if (IsValid(Component)) Component->DestroyComponent();
	}
	ContinuousGalaxyView = nullptr;
	ContinuousClusterView = nullptr;
	ContinuousPreviewBodies.Reset();
	ContinuousGalaxyPoints.Reset();
	ContinuousClusterPoints.Reset();
	for (const auto& Entry : ContinuousMaterializedSystems)
		if (IsValid(Entry.Value)) DestroyActorTree(Entry.Value);
	ContinuousMaterializedSystems.Reset();
	ContinuousSystemRecency.Reset();
	ContinuousRetiredBodyRotations.Reset();
	// The observer survives a structural rebuild; actor references do not.
	ContinuousSelectedPlanet.Reset();
	ContinuousSelectedStar.Reset();
}

void AAstroGenerator::EnsureContinuousPreviewPresentation()
{
	if (!UsesContinuousPreviewFrame()) return;
	if (ContinuousPreviewBodies.IsEmpty() && IsValid(GeneratedHomeStarSystem))
	{
		TArray<AActor*> Descendants;
		GeneratedHomeStarSystem->GetAttachedActors(Descendants, true, true);
		for (AActor* Actor : Descendants)
		{
			if (!IsValid(Actor)) continue;
			Actor->SetActorTickEnabled(false);
			if (Cast<AStar>(Actor) || Cast<APlanetaryBody>(Actor)) ContinuousPreviewBodies.Add(Actor);
		}
	}
	const auto CreateView = [this](UHierarchicalInstancedStaticMeshComponent* Source, const FName Name)
		-> UInstancedStaticMeshComponent*
	{
		if (!IsValid(Source) || !IsValid(Source->GetStaticMesh())) return nullptr;
		UInstancedStaticMeshComponent* View = NewObject<UInstancedStaticMeshComponent>(this,
			MakeUniqueObjectName(this, UInstancedStaticMeshComponent::StaticClass(), Name), RF_Transient);
		View->SetupAttachment(GenerationRoot);
		View->SetAbsolute(true, true, true);
		View->SetMobility(EComponentMobility::Movable);
		// The canonical point material is additive, which UE 5.4's Nanite path
		// cannot render. Retain the same fallback contract as Galaxy/StarCluster;
		// a fresh ISM otherwise audits/substitutes the material on every update.
		View->bDisallowNanite = true;
		View->SetForceDisableNanite(true);
		View->SetStaticMesh(Source->GetStaticMesh());
		for (int32 Index = 0; Index < Source->GetNumMaterials(); ++Index) View->SetMaterial(Index, Source->GetMaterial(Index));
		View->SetNumCustomDataFloats(Source->NumCustomDataFloats);
		View->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		View->SetGenerateOverlapEvents(false);
		View->SetCanEverAffectNavigation(false);
		View->SetCastShadow(false);
		View->SetCullDistances(0, 0);
		AddInstanceComponent(View);
		View->RegisterComponent();
		View->SetWorldTransform(FTransform::Identity);
		for (int32 Index = 0; Index < Source->GetInstanceCount(); ++Index)
			View->AddInstance(FTransform(FQuat::Identity, FVector::ZeroVector, FVector::ZeroVector));
		for (int32 Index = 0; Index < Source->GetInstanceCount(); ++Index)
			for (int32 Field = 0; Field < Source->NumCustomDataFloats; ++Field)
			{
				const int32 Address = Index * Source->NumCustomDataFloats + Field;
				if (Source->PerInstanceSMCustomData.IsValidIndex(Address))
					View->SetCustomDataValue(Index, Field, Source->PerInstanceSMCustomData[Address], false);
			}
		return View;
	};
	if (!IsValid(ContinuousGalaxyView) && IsValid(GeneratedGalaxy))
	{
		ContinuousGalaxyView = CreateView(GeneratedGalaxy->StarMeshInstances, TEXT("ContinuousGalaxyView"));
		if (IsValid(ContinuousGalaxyView))
		{
			const FAPSCanonicalStellarProjectionFrame& Frame = GeneratedGalaxy->CanonicalProjectionFrame;
			for (int32 Index = 0; Index < ContinuousGalaxyView->GetInstanceCount(); ++Index)
			{
				FGalaxyCatalogStarRecord Record;
				if (!GeneratedGalaxy->GetRenderedCatalogRecord(Index, Record)) continue;
				FAPSContinuousPreviewPoint& Point = ContinuousGalaxyPoints.AddDefaulted_GetRef();
				Point.CenterCm = Frame.GetCanonicalRootPositionCm(Record.GalaxyLocalLocation) - Frame.CanonicalAnchorCm;
				Point.RadiusCm = APSCanonicalStellarProjection::GetCanonicalStellarRadiusSolar(Record.SpectralClass)
					* APSCanonicalStellarProjection::SolarRadiusCm;
				Point.SourceInstanceIndex = Index;
				Point.StableId = Record.StableId;
			}
		}
	}
	if (!IsValid(ContinuousClusterView) && IsValid(GeneratedStarCluster))
	{
		ContinuousClusterView = CreateView(GeneratedStarCluster->StarMeshInstances, TEXT("ContinuousClusterView"));
		if (IsValid(ContinuousClusterView))
		{
			const double Started = FPlatformTime::Seconds();
			const FAPSCanonicalStellarProjectionFrame& Frame = GeneratedStarCluster->CanonicalProjectionFrame;
			UStarGenerator* Stars = NewObject<UStarGenerator>(this);
			UPlanetarySystemGenerator* Families = NewObject<UPlanetarySystemGenerator>(this);
			const auto AddStarPoint = [this, &Frame, Stars](const FClusterStarSystemRecord& Record,
				const int32 StarIndex, const FVector& CenterCm, const FStarModel& Model, AStar* Actor)
			{
				FAPSContinuousPreviewPoint& Point = ContinuousClusterPoints.AddDefaulted_GetRef();
				Point.CenterCm = CenterCm;
				Point.RadiusCm = Model.RadiusKM * 1.0e5;
				Point.StableId = Record.StableId;
				Point.SystemStarIndex = StarIndex;
				Point.MaterializedStar = Actor;
				Point.StarModel = MakeShared<FStarModel>(Model);
				Point.SourceInstanceIndex = StarIndex == 0 ? Record.InstanceIndex
					: ContinuousClusterView->AddInstance(FTransform(FQuat::Identity, FVector::ZeroVector, FVector::ZeroVector));
				const FLinearColor Color = Stars->GetStarColor(Model.SpectralClass, Model.SpectralSubclass);
				const double VisualRadiusCm = APSCanonicalStellarProjection::GetAppliedVisualRadiusCm(
					EAPSCanonicalStellarProxyLayer::StarCluster, Frame, Model.Radius);
				const double VisualRadiusSolar = APSCanonicalStellarProjection::UnprojectPhysicalRadiusSolar(Frame, VisualRadiusCm);
				// Primary and companion points use the same existing catalog material
				// convention. Slot creation never depends on whether a system was visited.
				const double Emission = UStarGenerator::GetFarStarVisualEmission(Model.Radius,
					Stars->CalculateEmission(Model.Luminosity * 25.0), VisualRadiusSolar);
				ContinuousClusterView->SetCustomDataValue(Point.SourceInstanceIndex, 0, Color.R, false);
				ContinuousClusterView->SetCustomDataValue(Point.SourceInstanceIndex, 1, Color.G, false);
				ContinuousClusterView->SetCustomDataValue(Point.SourceInstanceIndex, 2, Color.B, false);
				ContinuousClusterView->SetCustomDataValue(Point.SourceInstanceIndex, 3, Emission, false);
				if (StarIndex > 0)
					ContinuousClusterView->SetCustomDataValue(Point.SourceInstanceIndex, 4, Point.SourceInstanceIndex * 0.137f, false);
			};
			for (const FClusterStarSystemRecord& Record : GeneratedStarCluster->PotentialStarSystems)
			{
				const FVector SystemCenter = Frame.GetCanonicalRootPositionCm(Record.ClusterLocalLocation) - Frame.CanonicalAnchorCm;
				if (Record.InstanceIndex == PendingHomeClusterInstanceIndex && IsValid(GeneratedHomeStarSystem))
				{
					for (int32 Index = 0; Index < GeneratedHomeStarSystem->GetStars().Num(); ++Index)
					{
						AStar* Star = GeneratedHomeStarSystem->GetStars()[Index];
						const TSharedPtr<FStarModel>* Model = PreviewResolvedStarModels.Find(FString::Printf(TEXT("SYS0/S%d"), Index));
						if (IsValid(Star) && Model && Model->IsValid())
							AddStarPoint(Record, Index, GetContinuousPreviewPhysicalPosition(Star), **Model, Star);
					}
					continue;
				}
				if (Record.SystemModel.AmountOfStars == 1)
				{
					// A single star is already at the system center. No family layout is
					// needed to resolve its immutable physical position or appearance.
					AddStarPoint(Record, 0, SystemCenter, Record.PrimaryStarModel, nullptr);
					continue;
				}
				TArray<FAPSContinuousPreviewStarLayout> Layouts;
				double RadiusCm = 0.0;
				if (!BuildContinuousPreviewSystemLayout(Record, Stars, Families, Layouts, RadiusCm))
				{
					UE_LOG(LogTemp, Error, TEXT("[APS.Preview.Cluster] Cannot resolve catalog star layout %s"), *Record.StableId.ToString());
					continue;
				}
				for (int32 Index = 0; Index < Layouts.Num(); ++Index)
					AddStarPoint(Record, Index, SystemCenter + Layouts[Index].OffsetCm, *Layouts[Index].StarModel, nullptr);
			}
			UE_LOG(LogTemp, Log, TEXT("[APS.Preview.Cluster] Resolved complete star layer: systems=%d stars=%d in %.3fs (actor-free)"),
				GeneratedStarCluster->PotentialStarSystems.Num(), ContinuousClusterPoints.Num(), FPlatformTime::Seconds() - Started);
		}
	}
	if (IsValid(ContinuousClusterView) && IsValid(StarGenerator))
	{
		TArray<AStarSystem*> Systems{GeneratedHomeStarSystem};
		for (const auto& Entry : ContinuousMaterializedSystems) Systems.Add(Entry.Value);
		for (AStarSystem* System : Systems)
		{
			if (IsValid(System))
			{
				for (int32 StarIndex = 0; StarIndex < System->GetStars().Num(); ++StarIndex)
				{
					AStar* Star = System->GetStars()[StarIndex];
					if (!IsValid(Star)) continue;
					if (FAPSContinuousPreviewPoint* Existing = ContinuousClusterPoints.FindByPredicate(
						[System, StarIndex](const FAPSContinuousPreviewPoint& Item)
						{ return Item.StableId == System->StableSystemId && Item.SystemStarIndex == StarIndex; }))
					{
						Existing->MaterializedStar = Star;
						const FString Prefix = System == GeneratedHomeStarSystem ? TEXT("SYS0")
							: TEXT("SYS-") + System->StableSystemId.ToString(EGuidFormats::Digits);
						if (const TSharedPtr<FStarModel>* Model = PreviewResolvedStarModels.Find(FString::Printf(TEXT("%s/S%d"), *Prefix, StarIndex));
							Model && Model->IsValid()) Existing->StarModel = MakeShared<FStarModel>(**Model);
						continue;
					}
					UE_LOG(LogTemp, Error, TEXT("[APS.Preview.Cluster] Materialized star has no pre-existing view slot %s/S%d"),
						*System->StableSystemId.ToString(), StarIndex);
				}
			}
		}
	}
	// Immutable construction-time HISMs remain available for canonical diagnostics.
	// They are not the camera-dependent render layer and must never be moved here.
	for (UHierarchicalInstancedStaticMeshComponent* Source : {
		IsValid(GeneratedGalaxy) ? GeneratedGalaxy->StarMeshInstances : nullptr,
		IsValid(GeneratedStarCluster) ? GeneratedStarCluster->StarMeshInstances : nullptr })
	{
		if (!IsValid(Source)) continue;
		Source->SetVisibility(false, false);
		Source->SetHiddenInGame(true, false);
	}
}

void AAstroGenerator::TrimContinuousPreviewSystemCache(const double PixelTangent)
{
	CSV_SCOPED_TIMING_STAT(APSPreview, TrimCache);
	if (ContinuousMaterializedSystems.Num() <= ContinuousPreviewSystemCacheLimit) return;
	const TArray<int32> Candidates = ContinuousSystemRecency;
	for (const int32 InstanceIndex : Candidates)
	{
		if (ContinuousMaterializedSystems.Num() <= ContinuousPreviewSystemCacheLimit) break;
		AStarSystem* System = ContinuousMaterializedSystems.FindRef(InstanceIndex);
		if (!IsValid(System))
		{
			ContinuousMaterializedSystems.Remove(InstanceIndex);
			ContinuousSystemRecency.Remove(InstanceIndex);
			continue;
		}
		// Never retire the current hierarchy, a worker's input, or a photosphere
		// still resolved on screen. During an interrupted flight the resident count
		// may briefly exceed the target; departure shrinks the old bodies naturally.
		const auto Belongs = [this, System](const AActor* Actor)
			{ return IsValid(Actor) && GetContinuousPreviewOwningSystem(Actor) == System; };
		if (System == GetContinuousPreviewActiveSystem()
			|| Belongs(SelectedPreviewBodyActor.Get()) || Belongs(ActivePreviewWorldScapeBody.Get())
			|| Belongs(ContinuousSelectedStar.Get()) || Belongs(ContinuousSelectedPlanet.Get())
			|| Belongs(PreviewSurfaceBuildBody.Get())) continue;
		bool bResolved = false;
		for (const TWeakObjectPtr<AActor>& Body : ContinuousPreviewBodies)
		{
			if (!Belongs(Body.Get())) continue;
			const double DistanceCm = FVector::Distance(
				GetContinuousPreviewPhysicalPosition(Body.Get()), ContinuousPreviewFrame.ObserverCm);
			if (PhysicalBodyRadiusCm(Body.Get()) >= FMath::Max(DistanceCm, 1.0) * PixelTangent * 0.3)
			{ bResolved = true; break; }
		}
		if (bResolved) continue;

		// Keep the already observed star slots, their exact physical centers and
		// radii, plus their existing ISM/custom-data slots. Re-entry rebinds these
		// same addresses, never adding a second copy of a companion or snapping
		// the primary back to its system barycenter.
		for (FAPSContinuousPreviewPoint& Point : ContinuousClusterPoints)
		{
			if (!Belongs(Point.MaterializedStar.Get())) continue;
			Point.CenterCm = GetContinuousPreviewPhysicalPosition(Point.MaterializedStar.Get());
			Point.RadiusCm = PhysicalBodyRadiusCm(Point.MaterializedStar.Get());
			Point.MaterializedStar.Reset();
		}
		const FString Prefix = TEXT("SYS-") + System->StableSystemId.ToString(EGuidFormats::Digits) + TEXT("/");
		for (auto It = PreviewGlobeProxyStates.CreateIterator(); It; ++It)
		{
			if (!It.Key().StartsWith(Prefix)) continue;
			FAPSPreviewGlobeProxyState& State = It.Value();
			for (UProceduralMeshComponent* Component : {
				State.TerrainA.Get(), State.TerrainB.Get(), State.OceanA.Get(), State.OceanB.Get() })
			{
				if (!IsValid(Component)) continue;
				Component->SetVisibility(false, false);
				Component->SetHiddenInGame(true, false);
				Component->ClearAllMeshSections();
				Component->SetMaterial(0, nullptr);
				if (!State.bUsesDefaultBuffers) Component->DestroyComponent();
			}
			It.RemoveCurrent();
		}
		PendingPreviewGlobeBodies.RemoveAll([&Belongs](const TWeakObjectPtr<APlanetaryBody>& Body)
			{ return !Body.IsValid() || Belongs(Body.Get()); });
		for (auto It = PreviewPlanetPresentationRotations.CreateIterator(); It; ++It)
		{
			if (Belongs(It.Key().Get()))
				ContinuousRetiredBodyRotations.Add(GetPreviewBodyStableKey(It.Key().Get()), It.Value());
			if (!It.Key().IsValid() || Belongs(It.Key().Get())) It.RemoveCurrent();
		}
		for (auto It = PreviewResolvedStarModels.CreateIterator(); It; ++It)
			if (It.Key().StartsWith(Prefix)) It.RemoveCurrent();
		ContinuousPreviewBodies.RemoveAll([&Belongs](const TWeakObjectPtr<AActor>& Body)
			{ return !Body.IsValid() || Belongs(Body.Get()); });
		ContinuousMaterializedSystems.Remove(InstanceIndex);
		ContinuousSystemRecency.Remove(InstanceIndex);
		DestroyActorTree(System);
		UE_LOG(LogTemp, Log, TEXT("[APS.Preview.Cluster] Retired %s: residentRemote=%d retainedStarPoints=%d"),
			*Prefix, ContinuousMaterializedSystems.Num(), ContinuousClusterPoints.Num());
	}
}

bool AAstroGenerator::GetContinuousPreviewClusterLocation(const int32 InstanceIndex, FVector& OutLocation) const
{
	if (!UsesContinuousPreviewFrame() || !bContinuousPreviewInitialized) return false;
	const FAPSContinuousPreviewPoint* Point = ContinuousClusterPoints.FindByPredicate(
		[InstanceIndex](const FAPSContinuousPreviewPoint& Item) { return Item.SourceInstanceIndex == InstanceIndex; });
	return Point && ContinuousPreviewFrame.ProjectPosition(Point->MaterializedStar.IsValid()
		? GetContinuousPreviewPhysicalPosition(Point->MaterializedStar.Get()) : Point->CenterCm, OutLocation);
}

bool AAstroGenerator::ProjectContinuousPreviewWorldPosition(const FVector& PhysicalWorldPosition, FVector& OutLocation,
	const AActor* CoordinateOwner) const
{
	const AStarSystem* System = CoordinateOwner ? GetContinuousPreviewOwningSystem(CoordinateOwner) : GeneratedHomeStarSystem;
	return UsesContinuousPreviewFrame() && bContinuousPreviewInitialized && IsValid(System)
		&& ContinuousPreviewFrame.ProjectPosition(GetContinuousPreviewSystemCenter(System)
			+ (PhysicalWorldPosition - System->GetActorLocation()), OutLocation);
}

UStaticMeshComponent* AAstroGenerator::GetContinuousPreviewResolvedStarMesh(const int32 PointIndex) const
{
	const FAPSContinuousResolvedStarView* View = ContinuousResolvedStarViews.Find(PointIndex);
	return View ? View->Photosphere.Get() : nullptr;
}

bool AAstroGenerator::IsContinuousResolvedStarPoolHidden() const
{
	for (const FAPSContinuousResolvedStarView& View : ContinuousResolvedStarPool)
		for (const UStaticMeshComponent* Mesh : {View.Photosphere.Get(), View.Corona.Get()})
			if (!IsValid(Mesh) || Mesh->IsVisible() || !Mesh->bHiddenInGame) return false;
	return true;
}

FAPSContinuousResolvedStarView AAstroGenerator::AllocateContinuousResolvedStarPair(
	UStaticMeshComponent* Template, UMaterialInterface* SurfaceBase, UMaterialInterface* CoronaBase)
{
	CSV_SCOPED_TIMING_STAT(APSPreview, AllocateResolved);
	const auto CreateMesh = [this, Template](const FName Name, const bool bCorona)
	{
		UStaticMeshComponent* Mesh = NewObject<UStaticMeshComponent>(this,
			MakeUniqueObjectName(this, UStaticMeshComponent::StaticClass(), Name), RF_Transient);
		Mesh->SetupAttachment(GenerationRoot);
		Mesh->SetMobility(EComponentMobility::Movable);
		Mesh->bDisallowNanite = bCorona || Template->bDisallowNanite;
		Mesh->SetForceDisableNanite(bCorona || Template->bForceDisableNanite);
		Mesh->SetStaticMesh(Template->GetStaticMesh());
		Mesh->SetForcedLodModel(Template->ForcedLodModel);
		Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Mesh->SetGenerateOverlapEvents(false);
		Mesh->SetCanEverAffectNavigation(false);
		Mesh->SetCastShadow(!bCorona && Template->CastShadow);
		Mesh->bAffectDynamicIndirectLighting = !bCorona && Template->bAffectDynamicIndirectLighting;
		Mesh->bAffectDistanceFieldLighting = !bCorona && Template->bAffectDistanceFieldLighting;
		Mesh->SetReceivesDecals(!bCorona && Template->bReceivesDecals);
		Mesh->SetTranslucentSortPriority(bCorona ? 2 : Template->TranslucencySortPriority);
		Mesh->SetVisibility(false, false);
		Mesh->SetHiddenInGame(true, false);
		return Mesh;
	};
	FAPSContinuousResolvedStarView Pair = {
		CreateMesh(TEXT("ResolvedCatalogPhotosphere"), false), CreateMesh(TEXT("ResolvedCatalogCorona"), true)};
	Pair.Photosphere->SetMaterial(0, UMaterialInstanceDynamic::Create(SurfaceBase, this));
	Pair.Corona->SetMaterial(0, UMaterialInstanceDynamic::Create(CoronaBase, this));
	Pair.Corona->SetupAttachment(Pair.Photosphere.Get());
	++ContinuousResolvedStarAllocations;
	CSV_CUSTOM_STAT(APSPreview, CreatedPairs, 1, ECsvCustomStatOp::Accumulate);
	return Pair;
}

void AAstroGenerator::PrepareContinuousResolvedStarPool(const double PixelTangent)
{
	// Never run during initial presentation or a synchronous focus command, and
	// never spend the budget twice if multiple input callbacks update one frame.
	if (!bPreviewCameraTransitionActive || PreviewCameraTransitionElapsed <= 0.0f
		|| ContinuousResolvedLastPreparationFrame == GFrameCounter
		|| CVarResolvedStarPreparation.GetValueOnGameThread() == 0) return;
	ContinuousResolvedLastPreparationFrame = GFrameCounter;
	CSV_SCOPED_TIMING_STAT(APSPreview, PrepareResolved);
	const double Deadline = FPlatformTime::Seconds() + 0.001;
	const auto Capacity = [this]() { return ContinuousResolvedStarViews.Num() + ContinuousResolvedStarPool.Num(); };
	if (Capacity() >= ContinuousResolvedStarPoolLimit) return;
	UStaticMeshComponent* Template = IsValid(HomeStar) ? HomeStar->StarMesh : nullptr;
	if (!IsValid(Template) || !IsValid(Template->GetStaticMesh())) return;
	int32 Width = 1280, Height = 720;
	if (APlayerController* Controller = GetWorld()->GetFirstPlayerController()) Controller->GetViewportSize(Width, Height);
	const double TanHalfHorizontal = FMath::Tan(FMath::DegreesToRadians(PreviewCamera->FieldOfView * 0.5));
	const double TanHalfVertical = TanHalfHorizontal * FMath::Max(Height, 1) / FMath::Max(Width, 1);
	constexpr int32 ForecastSamples = 64;
	// Amortize a complete flight forecast over its early frames. Use the wider
	// retention threshold conservatively; predictions affect hidden capacity only.
	for (int32 Probe = 0; Probe < 2 && ContinuousResolvedForecastStep < ForecastSamples
		&& FPlatformTime::Seconds() < Deadline; ++Probe)
	{
		CSV_SCOPED_TIMING_STAT(APSPreview, ForecastResolved);
		const double Alpha = double(++ContinuousResolvedForecastStep) / ForecastSamples;
		const FAPSContinuousPreviewOrbit Orbit = FAPSContinuousPreviewOrbit::Interpolate(
			ContinuousPreviewStartOrbit, ContinuousPreviewTargetOrbit, Alpha);
		const FAPSContinuousPreviewFrame Frame{Orbit.ObserverCm(), 1.0e7 / Orbit.DistanceCm, 1.0e9};
		const FQuat Rotation = (-Orbit.Outward).Rotation().Quaternion();
		int32 Needed = 0;
		for (const FAPSContinuousPreviewPoint& Point : ContinuousClusterPoints)
		{
			FAPSPreviewProjectedSphere Sphere;
			Needed += NeedsResolvedStarView(Point, Frame, Rotation, PixelTangent,
				TanHalfHorizontal, TanHalfVertical, true, Sphere);
		}
		ContinuousResolvedForecastCapacity = FMath::Min(ContinuousResolvedStarPoolLimit,
			FMath::Max(ContinuousResolvedForecastCapacity, Needed));
	}
	if (Capacity() >= ContinuousResolvedForecastCapacity || FPlatformTime::Seconds() >= Deadline) return;
	UMaterial* SurfaceBase = APSStellarMaterialContract::LoadCanonicalBase(APSStellarMaterialContract::ActorBaseObjectPath);
	UMaterial* CoronaBase = APSStellarMaterialContract::LoadCanonicalBase(APSStellarMaterialContract::CoronaBaseObjectPath);
	if (!IsValid(SurfaceBase) || !IsValid(CoronaBase)) return;
	for (int32 Prepared = 0; Prepared < ContinuousResolvedStarPreparePairLimit
		&& Capacity() < ContinuousResolvedForecastCapacity && FPlatformTime::Seconds() < Deadline; ++Prepared)
	{
		FAPSContinuousResolvedStarView Pair = AllocateContinuousResolvedStarPair(Template, SurfaceBase, CoronaBase);
		for (UStaticMeshComponent* Mesh : {Pair.Photosphere.Get(), Pair.Corona.Get()})
		{
			CSV_SCOPED_TIMING_STAT(APSPreview, RegisterPreparedResolved);
			AddInstanceComponent(Mesh);
			Mesh->RegisterComponent();
		}
		ContinuousResolvedStarPool.Add(Pair);
		++ContinuousResolvedStarPreparations;
		CSV_CUSTOM_STAT(APSPreview, PreparedPairs, 1, ECsvCustomStatOp::Accumulate);
	}
}

void AAstroGenerator::PresentContinuousResolvedStars(const double PixelTangent)
{
	CSV_SCOPED_TIMING_STAT(APSPreview, ResolvedStars);
	// Only render components are allocated here: no star actors, families, planets,
	// terrain workers or lights. The same canonical materials and physical sphere
	// are used before and after a normal system visit.
	UStaticMeshComponent* Template = IsValid(HomeStar) ? HomeStar->StarMesh : nullptr;
	if (!IsValid(Template) || !IsValid(Template->GetStaticMesh()) || !IsValid(StarGenerator)) return;
	int32 Width = 1280, Height = 720;
	if (APlayerController* Controller = GetWorld()->GetFirstPlayerController()) Controller->GetViewportSize(Width, Height);
	const double TanHalfHorizontal = FMath::Tan(FMath::DegreesToRadians(PreviewCamera->FieldOfView * 0.5));
	const double TanHalfVertical = TanHalfHorizontal * FMath::Max(Height, 1) / FMath::Max(Width, 1);
	const FQuat CameraRotation = PreviewCamera->GetComponentQuat();
	TArray<TPair<const FAPSContinuousPreviewPoint*, FAPSPreviewProjectedSphere>> Required;
	TSet<int32> RequiredIndices;
	for (const FAPSContinuousPreviewPoint& Point : ContinuousClusterPoints)
	{
		FAPSPreviewProjectedSphere Sphere;
		if (!NeedsResolvedStarView(Point, ContinuousPreviewFrame, CameraRotation, PixelTangent,
			TanHalfHorizontal, TanHalfVertical, ContinuousResolvedStarViews.Contains(Point.SourceInstanceIndex), Sphere)) continue;
		Required.Emplace(&Point, Sphere);
		RequiredIndices.Add(Point.SourceInstanceIndex);
	}
	// Retire the whole outgoing set first. Reuse must not depend on the relative
	// catalog order of entering and leaving stars in the same flight frame.
	for (auto It = ContinuousResolvedStarViews.CreateIterator(); It; ++It)
	{
		if (RequiredIndices.Contains(It.Key())) continue;
		FAPSContinuousResolvedStarView& View = It.Value();
		if (View.Photosphere.IsValid() && View.Corona.IsValid()
			&& ContinuousResolvedStarPool.Num() < ContinuousResolvedStarPoolLimit)
		{
			CSV_SCOPED_TIMING_STAT(APSPreview, PoolResolved);
			CSV_CUSTOM_STAT(APSPreview, PooledPairs, 1, ECsvCustomStatOp::Accumulate);
			for (UStaticMeshComponent* Mesh : {View.Photosphere.Get(), View.Corona.Get()})
			{
				Mesh->SetVisibility(false, false);
				Mesh->SetHiddenInGame(true, false);
			}
			ContinuousResolvedStarPool.Add(View);
		}
		else
		{
			CSV_SCOPED_TIMING_STAT(APSPreview, DestroyResolved);
			CSV_CUSTOM_STAT(APSPreview, DestroyedPairs, 1, ECsvCustomStatOp::Accumulate);
			for (UStaticMeshComponent* Mesh : {View.Photosphere.Get(), View.Corona.Get()})
				if (IsValid(Mesh)) Mesh->DestroyComponent();
		}
		It.RemoveCurrent();
	}
	UMaterial* SurfaceBase = nullptr;
	UMaterial* CoronaBase = nullptr;
	for (const auto& Item : Required)
	{
		const FAPSContinuousPreviewPoint& Point = *Item.Key;
		const FAPSPreviewProjectedSphere& Sphere = Item.Value;
		FAPSContinuousResolvedStarView* View = ContinuousResolvedStarViews.Find(Point.SourceInstanceIndex);
		if (!View)
		{
			CSV_SCOPED_TIMING_STAT(APSPreview, CreateResolved);
			FAPSContinuousResolvedStarView Acquired;
			while (!ContinuousResolvedStarPool.IsEmpty())
			{
				Acquired = ContinuousResolvedStarPool.Pop(EAllowShrinking::No);
				if (Acquired.Photosphere.IsValid() && Acquired.Corona.IsValid()
					&& Cast<UMaterialInstanceDynamic>(Acquired.Photosphere->GetMaterial(0))
					&& Cast<UMaterialInstanceDynamic>(Acquired.Corona->GetMaterial(0))) break;
				for (UStaticMeshComponent* Mesh : {Acquired.Photosphere.Get(), Acquired.Corona.Get()})
					if (IsValid(Mesh)) Mesh->DestroyComponent();
				Acquired = {};
			}
			if (Acquired.Photosphere.IsValid())
			{
				++ContinuousResolvedStarReuses;
				CSV_CUSTOM_STAT(APSPreview, ReusedPairs, 1, ECsvCustomStatOp::Accumulate);
			}
			else
			{
				if (!SurfaceBase) SurfaceBase = APSStellarMaterialContract::LoadCanonicalBase(APSStellarMaterialContract::ActorBaseObjectPath);
				if (!CoronaBase) CoronaBase = APSStellarMaterialContract::LoadCanonicalBase(APSStellarMaterialContract::CoronaBaseObjectPath);
				if (!IsValid(SurfaceBase) || !IsValid(CoronaBase)) continue;
				Acquired = AllocateContinuousResolvedStarPair(Template, SurfaceBase, CoronaBase);
			}
			UStaticMeshComponent* Photosphere = Acquired.Photosphere.Get();
			UStaticMeshComponent* Corona = Acquired.Corona.Get();
			UMaterialInstanceDynamic* SurfaceMaterial = Cast<UMaterialInstanceDynamic>(Photosphere->GetMaterial(0));
			UMaterialInstanceDynamic* CoronaMaterial = Cast<UMaterialInstanceDynamic>(Corona->GetMaterial(0));
			const FAPSStellarMaterialParameters Parameters = StarGenerator->ApplySpectralMaterialParameters(SurfaceMaterial, Point.StarModel);
			AStar::ConfigureStellarPresentationComponents(Photosphere, Corona, CoronaMaterial, nullptr,
				Parameters.Color, Parameters.Emission, Parameters.SurfaceSeed, Point.StarModel->StellarType);
			PresentPhysicalMesh(Photosphere, Sphere, FQuat::Identity);
			Photosphere->SetVisibility(true, false);
			for (UStaticMeshComponent* Mesh : {Photosphere, Corona})
			{
				Mesh->SetHiddenInGame(false, false);
				if (!Mesh->IsRegistered())
				{
					CSV_SCOPED_TIMING_STAT(APSPreview, RegisterResolved);
					AddInstanceComponent(Mesh);
					Mesh->RegisterComponent();
				}
			}
			View = &ContinuousResolvedStarViews.Add(Point.SourceInstanceIndex, {Photosphere, Corona});
		}
		PresentPhysicalMesh(View->Photosphere.Get(), Sphere, FQuat::Identity);
	}
}

void AAstroGenerator::ApplyContinuousPreviewFrame()
{
	if (!UsesContinuousPreviewFrame() || !bContinuousPreviewInitialized || !IsValid(PreviewCamera)) return;
	CSV_SCOPED_TIMING_STAT(APSPreview, ApplyFrame);
	ContinuousPreviewFrame.ObserverCm = ContinuousPreviewOrbit.ObserverCm();
	// One numerical scale for all objects. Target framing stays precision-safe at
	// every zoom while every resolved angular size still equals physical R / D.
	ContinuousPreviewFrame.RenderCmPerPhysicalCm = 1.0e7 / ContinuousPreviewOrbit.DistanceCm;
	ContinuousPreviewFrame.FarEnvelopeCm = 1.0e9;
	PreviewCamera->SetWorldLocationAndRotation(FVector::ZeroVector, (-ContinuousPreviewOrbit.Outward).Rotation());
	ContinuousPreviewFrame.ProjectPosition(ContinuousPreviewOrbit.CenterCm, PreviewOrbitCenter);
	PreviewOrbitDistance = PreviewOrbitCenter.Size();
	const FRotator Direction = ContinuousPreviewOrbit.Outward.Rotation();
	PreviewOrbitYawDegrees = Direction.Yaw;
	PreviewOrbitPitchDegrees = Direction.Pitch;

	int32 ViewWidth = 1280;
	int32 ViewHeight = 720;
	if (APlayerController* Controller = GetWorld()->GetFirstPlayerController()) Controller->GetViewportSize(ViewWidth, ViewHeight);
	// Optical PSF is explicitly separate from physical photosphere size. The existing
	// Gaussian material's conservative sphere contains a ~0.2-radius bright core;
	// a two-pixel support therefore produces a subpixel core, not a two-pixel disc.
	const double PixelTangent = 2.0 * FMath::Tan(FMath::DegreesToRadians(PreviewCamera->FieldOfView * 0.5))
		/ FMath::Max(ViewWidth, 320);
	TrimContinuousPreviewSystemCache(PixelTangent);
	PresentContinuousResolvedStars(PixelTangent);
	const auto PresentCatalog = [this, PixelTangent](UInstancedStaticMeshComponent* View,
		const TArray<FAPSContinuousPreviewPoint>& Points)
	{
		if (!IsValid(View) || !IsValid(View->GetStaticMesh())) return;
		const double MeshRadius = FMath::Max(View->GetStaticMesh()->GetBounds().BoxExtent.GetMax(), 0.001);
		TArray<FTransform> Transforms;
		Transforms.Init(FTransform(FQuat::Identity, FVector::ZeroVector, FVector::ZeroVector), View->GetInstanceCount());
		for (const FAPSContinuousPreviewPoint& Point : Points)
		{
			if (!Transforms.IsValidIndex(Point.SourceInstanceIndex)) continue;
			FAPSPreviewProjectedSphere Sphere;
			if (!ContinuousPreviewFrame.ProjectSphere(Point.CenterCm, Point.RadiusCm, Sphere)) continue;
			double Radius = FMath::Max(Sphere.Radius, Sphere.Center.Size() * PixelTangent * 2.2);
			AStar* MaterializedStar = Point.MaterializedStar.Get();
			if (IsValid(MaterializedStar))
			{
				const FVector StarCenter = GetContinuousPreviewPhysicalPosition(MaterializedStar);
				ContinuousPreviewFrame.ProjectSphere(StarCenter, PhysicalBodyRadiusCm(MaterializedStar), Sphere);
			}
			if (IsValid(MaterializedStar) || (View == ContinuousClusterView
				&& IsValid(GetContinuousPreviewResolvedStarMesh(Point.SourceInstanceIndex))))
			{
				// Optical point handoff depends only on angular resolution, never on
				// whether the physical hierarchy of this star has already been visited.
				const double PixelRadius = Sphere.Radius / FMath::Max(Sphere.Center.Size() * PixelTangent, 1.0e-12);
				const double PointWeight = 1.0 - FMath::Clamp((PixelRadius - 0.3) / 0.7, 0.0, 1.0);
				Radius = Sphere.Center.Size() * PixelTangent * 2.2 * PointWeight;
			}
			Transforms[Point.SourceInstanceIndex] = FTransform(FQuat::Identity, Sphere.Center, FVector(Radius / MeshRadius));
		}
		// UE 5.4 tracks each changed instance and schedules SendRenderInstanceData.
		// Recreating the entire scene proxy here would discard that incremental path
		// on every flight frame; geometry and instance indices are unchanged.
		View->BatchUpdateInstancesTransforms(0, Transforms, true, false, true);
		View->SetVisibility(true, false);
		View->SetHiddenInGame(false, false);
	};
	{
		CSV_SCOPED_TIMING_STAT(APSPreview, GalaxyCatalog);
		PresentCatalog(ContinuousGalaxyView, ContinuousGalaxyPoints);
	}
	{
		CSV_SCOPED_TIMING_STAT(APSPreview, ClusterCatalog);
		PresentCatalog(ContinuousClusterView, ContinuousClusterPoints);
	}

	PreviewBodyPresentationCenters.Reset();
	PreviewBodyPresentationRadii.Reset();
	for (const TWeakObjectPtr<AActor>& WeakBody : ContinuousPreviewBodies)
	{
		AActor* Actor = WeakBody.Get();
		if (!IsValid(Actor) || !IsValid(GeneratedHomeStarSystem)) continue;
		FAPSPreviewProjectedSphere Sphere;
		if (!ContinuousPreviewFrame.ProjectSphere(GetContinuousPreviewPhysicalPosition(Actor),
			PhysicalBodyRadiusCm(Actor), Sphere)) continue;
		PreviewBodyPresentationCenters.Add(Actor, Sphere.Center);
		PreviewBodyPresentationRadii.Add(Actor, Sphere.Radius);
		Actor->SetActorHiddenInGame(false);
		if (AStar* Star = Cast<AStar>(Actor))
		{
			if (IsValid(Star->PlanetarySystemZone))
			{
				Star->PlanetarySystemZone->SetVisibility(false, false);
				Star->PlanetarySystemZone->SetHiddenInGame(true, false);
			}
			PresentPhysicalMesh(Star->StarMesh, Sphere, Star->GetActorQuat());
			if (IsValid(Star->StarMesh))
			{
				Star->StarMesh->SetVisibility(true, false);
				Star->StarMesh->SetHiddenInGame(false, false);
			}
			if (IsValid(Star->CoronaMesh))
			{
				Star->CoronaMesh->SetVisibility(Star->StellarClass != EStellarType::BlackHole
					&& IsValid(Star->CoronaDynamicMaterial), false);
				Star->CoronaMesh->SetHiddenInGame(false, false);
			}
			Star->SyncStellarLightToPresentedBounds();
		}
		else if (APlanetaryBody* Body = Cast<APlanetaryBody>(Actor))
		{
			TInlineComponentArray<UStaticMeshComponent*> Meshes;
			Body->GetComponents(Meshes);
			for (UStaticMeshComponent* Mesh : Meshes)
				PresentPhysicalMesh(Mesh, Sphere, (GetPreviewPlanetPresentationRotation(Body) * Body->GetActorQuat()).GetNormalized());
			const FAPSPreviewGlobeProxyState* State = FindPreviewGlobeProxyState(Body);
			SetPreviewBodyBackingSphereVisible(Body, !State || State->ActiveBuffer == INDEX_NONE);
		}
	}
	if (IsValid(GeneratedHomeStarSystem) && IsValid(GeneratedHomeStarSystem->StarSystemZone))
	{
		GeneratedHomeStarSystem->StarSystemZone->SetVisibility(false, false);
		GeneratedHomeStarSystem->StarSystemZone->SetHiddenInGame(true, false);
	}
	SyncPreviewGlobeProxyTransforms();
	SetPreviewGlobeProxyVisible(true);
	for (const TWeakObjectPtr<AActor>& WeakBody : ContinuousPreviewBodies)
		if (APlanetaryBody* Body = Cast<APlanetaryBody>(WeakBody.Get())) StabilizePreviewAtmosphere(Body);
	HideLegacyPreviewGuideShells();
	SetPreviewGuideShellVisible(PreviewStarInfluenceWireGuide, false);
	SetPreviewGuideShellVisible(PreviewSystemBoundaryWireGuide, false);
	PrepareContinuousResolvedStarPool(PixelTangent);
}

void AAstroGenerator::SetContinuousPreviewFramingTangent(const double Tangent)
{
	if (!FMath::IsFinite(Tangent) || Tangent <= 0.001
		|| FMath::IsNearlyEqual(Tangent, ContinuousPreviewFramingTangent, 1.0e-5)) return;
	const bool bHadLayout = ContinuousPreviewFramingTangent > 0.0;
	ContinuousPreviewFramingTangent = Tangent;
	// Keep the conservative first presentation stationary: receiving Slate's first
	// layout is not a navigation request and must not start another flight or its
	// speculative stellar preparation. Subsequent resizes refit automatic views;
	// manual orbit/zoom still takes precedence. The next focus uses the exact fit.
	if (bHadLayout && UsesContinuousPreviewFrame() && bContinuousPreviewInitialized && bContinuousPreviewAutoFraming)
		StartContinuousPreviewTransition(nullptr);
}

void AAstroGenerator::StartContinuousPreviewTransition(APlayerController* PlayerController, const double DistanceRatio)
{
	EnsureContinuousPreviewPresentation();
	FVector CenterCm;
	double RadiusCm = 0.0;
	if (!IsValid(PreviewCamera) || !GetContinuousPreviewPhysicalFocus(PreviewFocus, CenterCm, RadiusCm)) return;
	int32 ViewWidth = 1280, ViewHeight = 720;
	APlayerController* Controller = PlayerController ? PlayerController : GetWorld()->GetFirstPlayerController();
	if (Controller) Controller->GetViewportSize(ViewWidth, ViewHeight);
	// Until Slate has its first layout, reserve a conservative central region in
	// both axes. Thereafter the real annotation panel supplies the available angle.
	const double FallbackTangent = FMath::Tan(FMath::DegreesToRadians(PreviewCamera->FieldOfView * 0.5))
		* FMath::Min(0.44, 0.60 * FMath::Max(ViewHeight, 1) / FMath::Max(ViewWidth, 1));
	const double FitTangent = FMath::Max(ContinuousPreviewFramingTangent > 0.0
		? ContinuousPreviewFramingTangent : FallbackTangent, 0.001);
	// A sphere's limb subtends asin(R/D), not atan(R/D). One common margin also
	// leaves room for atmosphere/corona without altering any physical body radius.
	const double FrameRatio = 1.20 * FMath::Sqrt(1.0 + 1.0 / FMath::Square(FitTangent));
	if (DistanceRatio <= 0.0) bContinuousPreviewAutoFraming = true;
	FAPSContinuousPreviewOrbit Target = ContinuousPreviewOrbit;
	Target.CenterCm = CenterCm;
	Target.DistanceCm = RadiusCm * (DistanceRatio > 0.0 ? DistanceRatio : FrameRatio);
	if (!Target.IsValid()) return;
	const bool bInitial = !bContinuousPreviewInitialized;
	if (bInitial)
	{
		ContinuousPreviewOrbit = Target;
		bContinuousPreviewInitialized = true;
	}
	const bool bSameTarget = bPreviewCameraTransitionActive
		&& ContinuousPreviewTargetOrbit.CenterCm.Equals(Target.CenterCm, 0.01)
		&& FMath::IsNearlyEqual(ContinuousPreviewTargetOrbit.DistanceCm / Target.DistanceCm, 1.0, 1.0e-9);
	if (!bSameTarget)
	{
		ContinuousPreviewStartOrbit = ContinuousPreviewOrbit;
		ContinuousPreviewTargetOrbit = Target;
		PreviewCameraTransitionElapsed = 0.0f;
		ContinuousResolvedForecastStep = ContinuousResolvedForecastCapacity = 0;
		const double CruiseDistance = FMath::Max(FMath::Max(Target.DistanceCm, ContinuousPreviewOrbit.DistanceCm),
			FVector::Distance(Target.CenterCm, ContinuousPreviewOrbit.CenterCm) * 2.5);
		const double ZoomTravel = FMath::Loge(CruiseDistance / ContinuousPreviewOrbit.DistanceCm)
			+ FMath::Loge(CruiseDistance / Target.DistanceCm);
		PreviewCameraTransitionDuration = FMath::Clamp(static_cast<float>(
			0.55 + ZoomTravel * 0.045), 0.55f, 1.6f);
		bPreviewCameraTransitionActive = !bInitial && (
			!ContinuousPreviewOrbit.CenterCm.Equals(Target.CenterCm, 0.01)
			|| !FMath::IsNearlyEqual(ContinuousPreviewOrbit.DistanceCm / Target.DistanceCm, 1.0, 1.0e-9));
	}
	ApplyContinuousPreviewFrame();
	if (bPreviewCameraTransitionActive) SetActorTickEnabled(true);
	PreviewCamera->SetActive(true);
	if (Controller && Controller->GetViewTarget() != this) Controller->SetViewTarget(this);
	UE_LOG(LogTemp, Log, TEXT("[APS.Preview.Continuous] focus=%s physicalRadius=%.3e physicalDistance=%.3e initial=%d"),
		*UEnum::GetValueAsString(PreviewFocus), RadiusCm, Target.DistanceCm, bInitial);
}

void AAstroGenerator::FocusContinuousPreviewTarget(const EAstroPreviewFocus NewFocus, APlayerController* Controller)
{
	RememberContinuousPreviewBody(SelectedPreviewBodyActor.Get());
	AActor* TargetBody = nullptr;
	if (NewFocus == EAstroPreviewFocus::HomePlanet)
	{
		TargetBody = ContinuousSelectedPlanet.IsValid() ? ContinuousSelectedPlanet.Get() : HomePlanet;
		AStar* SelectedStar = ContinuousSelectedStar.Get();
		if (IsValid(SelectedStar) && PhysicalParentStar(Cast<APlanetaryBody>(TargetBody)) != SelectedStar
			&& IsValid(SelectedStar->PlanetarySystem) && !SelectedStar->PlanetarySystem->PlanetsActorsList.IsEmpty())
			TargetBody = SelectedStar->PlanetarySystem->PlanetsActorsList[0];
	}
	else if (NewFocus == EAstroPreviewFocus::HomeStar)
		TargetBody = ContinuousSelectedStar.IsValid() ? ContinuousSelectedStar.Get() : HomeStar;
	if (NewFocus == EAstroPreviewFocus::HomePlanet && ContinuousSelectedStar.IsValid()
		&& PhysicalParentStar(Cast<APlanetaryBody>(TargetBody)) != ContinuousSelectedStar.Get()) TargetBody = nullptr;
	if ((NewFocus == EAstroPreviewFocus::HomeStar || NewFocus == EAstroPreviewFocus::HomePlanet) && !TargetBody) return;
	if (PreviewFocus == NewFocus && SelectedPreviewBodyActor.Get() == TargetBody && bContinuousPreviewInitialized) return;
	SelectedPreviewBodyActor = TargetBody;
	PreviewFocus = NewFocus;
	RememberContinuousPreviewBody(TargetBody);
	EnsureContinuousPreviewPresentation();
	if (APlanetaryBody* Body = Cast<APlanetaryBody>(TargetBody))
	{
		if (ActivePreviewWorldScapeBody.Get() != Body) SetPreviewWorldScapeBody(Body);
	}
	StartContinuousPreviewTransition(Controller);
}
