#include "APSStellarVisualSubsystem.h"

#include "APSGameplayStellarProjection.h"
#include "APS_ALPHA/Actors/Astro/Galaxy.h"
#include "APS_ALPHA/Actors/Astro/StarCluster.h"
#include "APS_ALPHA/Actors/Astro/StarSystem.h"
#include "APS_ALPHA/Actors/Astro/Star.h"
#include "APS_ALPHA/Actors/Astro/PlanetaryBody.h"
#include "APS_ALPHA/Generation/AstroGenerator.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "Async/ParallelFor.h"
#include "ProfilingDebugging/CsvProfiler.h"

CSV_DEFINE_CATEGORY(APSGameplayStars, true);

void UAPSStellarVisualSubsystem::ResetGameplayStellarView()
{
	for (FAPSGameplayStellarLayer& Layer : GameplayStellarLayers)
	{
		if (UInstancedStaticMeshComponent* View = Layer.View.Get()) View->DestroyComponent();
		if (UHierarchicalInstancedStaticMeshComponent* Source = Layer.Source.Get())
		{
			Source->SetVisibility(Layer.bSourceVisible, false);
			Source->SetHiddenInGame(Layer.bSourceHidden, false);
		}
	}
	GameplayStellarLayers.Reset();
	GameplayStellarGenerator.Reset();
	GameplayStellarBuildSerial = 0;
	LastStellarPixelTangent = -1.0;
	ClosestStellarPointCm = 0.0;
	ClosestStellarRenderDistanceCm = 0.0;
	LastStellarOccluders.Reset();
}

void UAPSStellarVisualSubsystem::UpdateGameplayStellarView()
{
	TRACE_CPUPROFILER_EVENT_SCOPE(APS_GameplayStellarView);
	CSV_SCOPED_TIMING_STAT(APSGameplayStars, ObserverView);
	UWorld* World = GetWorld();
	APlayerController* Controller = World ? World->GetFirstPlayerController() : nullptr;
	if (!Controller || !Controller->PlayerCameraManager) return;
	AAstroGenerator* Generator = GameplayStellarGenerator.Get();
	if (!IsValid(Generator))
	{
		if (!GameplayStellarLayers.IsEmpty()) ResetGameplayStellarView();
		for (TActorIterator<AAstroGenerator> It(World); It; ++It)
		{
			// This adapter never participates in the accepted menu presentation.
			if (!It->ActorHasTag(TEXT("WorldGenerationPreview"))
				&& !It->UsesContinuousPreviewFrame()
				&& It->GetCanonicalStellarProjectionDescriptor().bFinalized
				&& It->GetCanonicalStellarProjectionDescriptor().Galaxy.bEnabled
				&& IsValid(It->GetPreviewHomeSystem()))
			{
				Generator = *It;
				GameplayStellarGenerator = Generator;
				break;
			}
		}
	}
	if (!Generator) return;
	const FAPSCanonicalStellarProjectionDescriptor& Descriptor =
		Generator->GetCanonicalStellarProjectionDescriptor();
	AStarSystem* Home = Generator->GetPreviewHomeSystem();
	if (!Descriptor.bFinalized || !IsValid(Home)) { ResetGameplayStellarView(); return; }
	if (GameplayStellarBuildSerial != 0 && GameplayStellarBuildSerial != Descriptor.ProxyBuildSerial)
	{
		ResetGameplayStellarView();
		return;
	}
	if (GameplayStellarLayers.IsEmpty())
	{
		TArray<AActor*> Attached;
		Generator->GetAttachedActors(Attached, true, true);
		const auto AddLayer = [&](UHierarchicalInstancedStaticMeshComponent* Source,
			EAPSCanonicalStellarProxyLayer Kind, const FAPSCanonicalStellarProjectionFrame& Frame)
		{
			if (!IsValid(Source) || !IsValid(Source->GetStaticMesh()) || Source->GetInstanceCount() == 0) return;
			FAPSGameplayStellarLayer Layer;
			Layer.Source = Source;
			Layer.bSourceVisible = Source->IsVisible();
			Layer.bSourceHidden = Source->bHiddenInGame;
			UInstancedStaticMeshComponent* View = NewObject<UInstancedStaticMeshComponent>(Generator,
				NAME_None, RF_Transient);
			View->SetAbsolute(true, true, true);
			View->SetMobility(EComponentMobility::Movable);
			View->bDisallowNanite = true;
			View->SetForceDisableNanite(true);
			// Camera-following transforms must not rescan the complete catalog for bounds.
			View->SetUseConservativeBounds(true);
			View->SetStaticMesh(Source->GetStaticMesh());
			for (int32 Slot = 0; Slot < Source->GetNumMaterials(); ++Slot)
				View->SetMaterial(Slot, Source->GetMaterial(Slot));
			View->SetNumCustomDataFloats(Source->NumCustomDataFloats);
			View->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			View->bDisableCollision = true;
			View->SetGenerateOverlapEvents(false);
			View->SetCanEverAffectNavigation(false);
			View->SetCastShadow(false);
			View->bAffectDynamicIndirectLighting = false;
			View->bAffectDistanceFieldLighting = false;
			View->bEvaluateWorldPositionOffset = false;
			View->bWorldPositionOffsetWritesVelocity = false;
			View->SetReceivesDecals(false);
			View->SetCullDistances(0, 0);
			View->SetVisibility(false, false);
			Generator->AddInstanceComponent(View);
			View->RegisterComponent();
			Layer.View = View;
			Layer.Transforms.Init(FTransform(FQuat::Identity, FVector::ZeroVector, FVector::ZeroVector),
				Source->GetInstanceCount());
			View->AddInstances(Layer.Transforms, false, false);
			for (int32 Index = 0; Index < Source->GetInstanceCount(); ++Index)
			{
				FAPSCanonicalStellarProxyRecord Record;
				if (Generator->GetCanonicalStellarProxyRecord(Kind, Index, Record)
					&& !Record.bSuppressedMaterializedHome)
				{
					FAPSGameplayStellarPoint& Point = Layer.Points.AddDefaulted_GetRef();
					Point.CenterFromHomeCm = Frame.GetCanonicalRootPositionCm(Record.CanonicalPositionUnits)
						- Frame.CanonicalAnchorCm;
					Point.RadiusCm = Record.CanonicalPhysicalRadiusSolar * APSCanonicalStellarProjection::SolarRadiusCm;
					Point.InstanceIndex = Index;
				}
				for (int32 Field = 0; Field < Source->NumCustomDataFloats; ++Field)
				{
					const int32 Address = Index * Source->NumCustomDataFloats + Field;
					if (Source->PerInstanceSMCustomData.IsValidIndex(Address))
						View->SetCustomDataValue(Index, Field, Source->PerInstanceSMCustomData[Address], false);
				}
			}
			GameplayStellarLayers.Add(MoveTemp(Layer));
		};
		for (AActor* Actor : Attached)
		{
			if (AGalaxy* Galaxy = Cast<AGalaxy>(Actor))
				AddLayer(Galaxy->StarMeshInstances, EAPSCanonicalStellarProxyLayer::Galaxy, Descriptor.Galaxy);
			else if (AStarCluster* Cluster = Cast<AStarCluster>(Actor))
				AddLayer(Cluster->StarMeshInstances, EAPSCanonicalStellarProxyLayer::StarCluster, Descriptor.StarCluster);
		}
		if (GameplayStellarLayers.IsEmpty()) return;
		GameplayStellarBuildSerial = Descriptor.ProxyBuildSerial;
		UE_LOG(LogTemp, Log, TEXT("[APS.Gameplay.StellarView] Initialized %d physical observer layers; immutable catalog retained"),
			GameplayStellarLayers.Num());
	}

	FVector Camera;
	FRotator Rotation;
	Controller->GetPlayerViewPoint(Camera, Rotation);
	int32 Width = 0, Height = 0;
	Controller->GetViewportSize(Width, Height);
	const double PixelTangent = 2.0 * FMath::Tan(FMath::DegreesToRadians(
		Controller->PlayerCameraManager->GetFOVAngle() * 0.5)) / FMath::Max(Width, 320);
	const FVector HomeLocation = Home->GetActorLocation();
	TArray<FAPSPreviewOccluder> Occluders;
	TArray<AActor*> Bodies;
	Home->GetAttachedActors(Bodies, true, true);
	for (AActor* Body : Bodies)
	{
		if (!IsValid(Body)) continue;
		FVector Center = Body->GetActorLocation();
		double Radius = 0.0;
		if (AStar* Star = Cast<AStar>(Body))
		{
			// Use the actual opaque silhouette, not the additive corona or an AABB
			// sphere inflated by sqrt(3). Do not resize the accepted primary star.
			if (IsValid(Star->StarMesh) && IsValid(Star->StarMesh->GetStaticMesh()))
			{
				const FBoxSphereBounds Bounds = Star->StarMesh->GetStaticMesh()->GetBounds();
				Center = Star->StarMesh->GetComponentTransform().TransformPosition(Bounds.Origin);
				Radius = Bounds.BoxExtent.GetMax() * Star->StarMesh->GetComponentScale().GetAbsMax();
			}
		}
		else if (APlanetaryBody* Planet = Cast<APlanetaryBody>(Body))
		{
			Radius = FMath::Max(Planet->RadiusKM, 0.0) * 1.0e5;
			// Ground can be below the reference sea-level sphere. It must never turn
			// an observer in a valley into an observer with the entire sky occluded.
			Radius = FMath::Min(Radius, FVector::Distance(Center, Camera) * (1.0 - 1.0e-8));
		}
		if (!FMath::IsFinite(Radius) || Radius <= 0.0) continue;
		FAPSPreviewOccluder& Occluder = Occluders.AddDefaulted_GetRef();
		Occluder.Center = Center - Camera;
		Occluder.Radius = Radius;
	}
	const FVector ObserverFromHome = Camera - HomeLocation;
	// UE 5.4 marks ALL instance transforms dirty on a component translation.
	// Keep the render anchor stationary until its own accumulated screen error
	// reaches the same sub-pixel budget. Never move an entire ISM on a cache hit.
	double AnchorTravelCm = 0.0;
	for (const FAPSGameplayStellarLayer& Layer : GameplayStellarLayers)
		if (const UInstancedStaticMeshComponent* View = Layer.View.Get())
			AnchorTravelCm = FMath::Max(AnchorTravelCm, FVector::Distance(Camera, View->GetComponentLocation()));
	const double TravelCm = FVector::Distance(ObserverFromHome, LastStellarObserverFromHome);
	const bool bOpticsChanged = !APSGameplayStellarProjection::CanReuseOptics(LastStellarPixelTangent, PixelTangent);
	const bool bPhysicalParallax = !APSGameplayStellarProjection::CanReuseProjection(TravelCm, ClosestStellarPointCm, PixelTangent);
	const bool bRenderParallax = !APSGameplayStellarProjection::CanReuseProjection(AnchorTravelCm, ClosestStellarRenderDistanceCm, PixelTangent);
	const bool bReproject = bOpticsChanged || bPhysicalParallax || bRenderParallax;
	const double NearestPointCm = ClosestStellarPointCm - TravelCm;
	const double MaskTravelCm = FVector::Distance(ObserverFromHome, LastStellarMaskObserver);
	const bool bUpdateMask = bReproject
		|| !APSGameplayStellarProjection::CanReuseProjection(MaskTravelCm, NearestPointCm, PixelTangent)
		|| !APSGameplayStellarProjection::CanReuseOcclusion(LastStellarOccluders, Occluders, NearestPointCm, PixelTangent);
	if (!bUpdateMask) return;
	CSV_SCOPED_TIMING_STAT(APSGameplayStars, Refresh);
	CSV_CUSTOM_STAT(APSGameplayStars, MaskRefreshes, 1, ECsvCustomStatOp::Accumulate);
	CSV_CUSTOM_STAT(APSGameplayStars, ProjectionRefreshes, bReproject ? 1 : 0, ECsvCustomStatOp::Accumulate);
	CSV_CUSTOM_STAT(APSGameplayStars, OpticsInvalidations, bOpticsChanged ? 1 : 0, ECsvCustomStatOp::Accumulate);
	CSV_CUSTOM_STAT(APSGameplayStars, PhysicalInvalidations, bPhysicalParallax ? 1 : 0, ECsvCustomStatOp::Accumulate);
	CSV_CUSTOM_STAT(APSGameplayStars, AnchorInvalidations, bRenderParallax ? 1 : 0, ECsvCustomStatOp::Accumulate);
	if (bReproject)
	{
		LastStellarObserverFromHome = ObserverFromHome;
		LastStellarPixelTangent = PixelTangent;
		ClosestStellarPointCm = TNumericLimits<double>::Max();
		ClosestStellarRenderDistanceCm = TNumericLimits<double>::Max();
	}
	LastStellarOccluders = Occluders;
	LastStellarMaskObserver = ObserverFromHome;
	TArray<APSGameplayStellarProjection::FPreparedOccluder> PreparedOccluders;
	for (const FAPSPreviewOccluder& Occluder : Occluders) PreparedOccluders.Emplace(Occluder);
	for (FAPSGameplayStellarLayer& Layer : GameplayStellarLayers)
	{
		UInstancedStaticMeshComponent* View = Layer.View.Get();
		if (!IsValid(View) || !IsValid(View->GetStaticMesh())) continue;
		const double MeshRadius = FMath::Max(View->GetStaticMesh()->GetBounds().BoxExtent.GetMax(), 0.001);
		constexpr int32 ChunkSize = 1024;
		const int32 ChunkCount = FMath::DivideAndRoundUp(Layer.Points.Num(), ChunkSize);
		TArray<double> ClosestInChunk;
		TArray<double> ClosestRenderInChunk;
		TArray<uint8> Dirty;
		Dirty.SetNumZeroed(Layer.Transforms.Num());
		ClosestInChunk.Init(TNumericLimits<double>::Max(), ChunkCount);
		ClosestRenderInChunk.Init(TNumericLimits<double>::Max(), ChunkCount);
		ParallelFor(ChunkCount, [&](int32 Chunk)
		{
			const int32 End = FMath::Min((Chunk + 1) * ChunkSize, Layer.Points.Num());
			for (int32 Index = Chunk * ChunkSize; Index < End; ++Index)
			{
				FAPSGameplayStellarPoint& Point = Layer.Points[Index];
				const FVector Offset = Point.CenterFromHomeCm - ObserverFromHome;
				const double Distance = Offset.Size();
				ClosestInChunk[Chunk] = FMath::Min(ClosestInChunk[Chunk], Distance);
				if (Distance <= 0.0 || !FMath::IsFinite(Distance)) continue;
				const FVector Direction = Offset / Distance;
				bool bOccluded = false;
				for (const auto& Occluder : PreparedOccluders)
					if (Occluder.Occludes(Direction, Distance)) { bOccluded = true; break; }
				if (bReproject)
				{
					FAPSPreviewProjectedSphere Sphere;
					double OpticalRadius = 0.0;
					if (!APSGameplayStellarProjection::Project(Offset, Point.RadiusCm, PixelTangent, Sphere, OpticalRadius)) continue;
					ClosestRenderInChunk[Chunk] = FMath::Min(ClosestRenderInChunk[Chunk], Sphere.Center.Size());
					Point.ProjectedTransform = FTransform(FQuat::Identity, Sphere.Center, FVector(OpticalRadius / MeshRadius));
				}
				// A changed body does not imply a changed stellar transform. Only
				// stars whose visibility actually changes need a renderer update.
				if (APSGameplayStellarProjection::NeedsInstanceUpload(bReproject, Point.bOccluded, bOccluded))
				{
					Layer.Transforms[Point.InstanceIndex] = Point.ProjectedTransform;
					if (bOccluded) Layer.Transforms[Point.InstanceIndex].SetScale3D(FVector::ZeroVector);
					Dirty[Point.InstanceIndex] = 1;
				}
				Point.bOccluded = bOccluded;
			}
		});
		if (bReproject)
		{
			for (double Distance : ClosestInChunk) ClosestStellarPointCm = FMath::Min(ClosestStellarPointCm, Distance);
			for (double Distance : ClosestRenderInChunk) ClosestStellarRenderDistanceCm = FMath::Min(ClosestStellarRenderDistanceCm, Distance);
			View->SetWorldLocation(Camera, false, nullptr, ETeleportType::TeleportPhysics);
		}
		int32 DirtyCount = 0;
		for (uint8 Changed : Dirty) DirtyCount += Changed;
		CSV_CUSTOM_STAT(APSGameplayStars, ChangedInstances, DirtyCount, ECsvCustomStatOp::Accumulate);
		if (DirtyCount > Layer.Transforms.Num() / 8)
		{
			View->BatchUpdateInstancesTransforms(0, Layer.Transforms, false, false, true);
		}
		else
		{
			// Coalesce small visibility deltas; do not upload all 60k instances
			// just because one point crossed a planetary limb.
			int32 Start = 0;
			while (Start < Dirty.Num())
			{
				if (!Dirty[Start]) { ++Start; continue; }
				int32 End = Start + 1;
				while (End < Dirty.Num() && Dirty[End]) ++End;
				View->BatchUpdateInstancesTransforms(Start,
					TArrayView<const FTransform>(Layer.Transforms.GetData() + Start, End - Start), false, false, true);
				Start = End;
			}
		}
		View->SetVisibility(true, false);
		View->SetHiddenInGame(false, false);
		// Hide only after a populated replacement is ready; do not propagate into
		// child actors and never alter source instance transforms or custom data.
		if (UHierarchicalInstancedStaticMeshComponent* Source = Layer.Source.Get())
		{
			Source->SetVisibility(false, false);
			Source->SetHiddenInGame(true, false);
		}
	}
}
