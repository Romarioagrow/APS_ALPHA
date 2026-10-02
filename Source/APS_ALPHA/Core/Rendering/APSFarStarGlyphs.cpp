#include "APSFarStarGlyphs.h"

#include "APSStellarOpticalSupport.h"
#include "APS_ALPHA/Actors/Astro/Star.h"
#include "APS_ALPHA/Actors/Astro/StarCluster.h"
#include "APS_ALPHA/Actors/Astro/StarSystem.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "HAL/IConsoleManager.h"

namespace APSFarStarGlyphsLocal
{
	TAutoConsoleVariable<float> CVarGlyphPixels(TEXT("aps.Stars.FarGlyphPixels"), 2.2f,
		TEXT("A materialized star (the home sun, a visited system's) whose disc is smaller than this many pixels keeps its ")
		TEXT("catalogue glyph (core and rays) so it stays visible far away. 0: off."));

	struct FGlyph
	{
		TWeakObjectPtr<AStar> Star;
		TWeakObjectPtr<AActor> Actor;
		TWeakObjectPtr<UInstancedStaticMeshComponent> Mesh;
		int32 Index{INDEX_NONE};
		/** The carrier radius the instance was last sized at. */
		double Carrier{0.0};
	};
	TMap<TWeakObjectPtr<const UWorld>, TArray<FGlyph>> GGlyphs;

	void Destroy(FGlyph& Glyph)
	{
		if (AActor* Actor = Glyph.Actor.Get())
		{
			Actor->Destroy();
		}
		Glyph.Actor.Reset();
		Glyph.Mesh.Reset();
	}

	/** A one-instance copy of the catalogue point: its mesh, its material and its row of per-star data. */
	bool Create(UWorld& World, FGlyph& Glyph, const UHierarchicalInstancedStaticMeshComponent& Source, const AStar& Star)
	{
		// The stellar view widens the catalogue's data to the optics' layout first; until then there is nothing to copy.
		const int32 Stride = Source.NumCustomDataFloats;
		const int32 Address = Glyph.Index * Stride;
		if (Stride < APSStellarOpticalSupport::RequiredStride || !Source.PerInstanceSMCustomData.IsValidIndex(Address + Stride - 1))
		{
			return false;
		}
		FActorSpawnParameters Parameters;
		Parameters.ObjectFlags |= RF_Transient;
		Parameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		AActor* Actor = World.SpawnActor<AActor>(AActor::StaticClass(), FTransform(Star.GetActorLocation()), Parameters);
		if (!Actor)
		{
			return false;
		}
		UInstancedStaticMeshComponent* Mesh = NewObject<UInstancedStaticMeshComponent>(Actor, TEXT("FarStarGlyph"), RF_Transient);
		Mesh->SetMobility(EComponentMobility::Movable);
		// Set up as the catalogue points are (APSStarRenderStabilitySubsystem): Nanite cannot draw the additive stellar
		// point material in UE 5.4 (02.10 run: the glyph went to Nanite, was not drawn and warned nine times a frame),
		// no wind offset on a point, never culled by distance, the catalogue's LOD.
		Mesh->bDisallowNanite = true;
		Mesh->SetForceDisableNanite(true);
		Mesh->bEvaluateWorldPositionOffset = false;
		Mesh->bWorldPositionOffsetWritesVelocity = false;
		Mesh->bUseAsOccluder = false;
		Mesh->bNeverDistanceCull = true;
		Mesh->SetCullDistances(0, 0);
		Mesh->SetForcedLodModel(Source.ForcedLodModel);
		Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Mesh->SetGenerateOverlapEvents(false);
		Mesh->SetCanEverAffectNavigation(false);
		Mesh->SetCastShadow(false);
		Mesh->bAffectDistanceFieldLighting = false;
		Mesh->bAffectDynamicIndirectLighting = false;
		Mesh->SetReceivesDecals(false);
		Mesh->SetStaticMesh(Source.GetStaticMesh());
		Mesh->SetMaterial(0, Source.GetMaterial(0));
		Mesh->SetTranslucentSortPriority(Source.TranslucencySortPriority);
		// The instance and its data before the render state, so the proxy is made once, complete.
		Mesh->SetNumCustomDataFloats(Stride);
		Mesh->AddInstance(FTransform::Identity, false);
		Mesh->SetCustomData(0, TArrayView<const float>(Source.PerInstanceSMCustomData.GetData() + Address, Stride), false);
		Actor->SetRootComponent(Mesh);
		Actor->AddInstanceComponent(Mesh);
		Mesh->RegisterComponent();
#if WITH_EDITOR
		Actor->SetActorLabel(TEXT("FarStarGlyph ") + Star.GetName());
#endif
		Glyph.Actor = Actor;
		Glyph.Mesh = Mesh;
		Glyph.Carrier = 0.0;
		UE_LOG(LogTemp, Log, TEXT("[APS.Stars] far glyph for %s (%s), catalogue point %d"), *Star.AstroName.ToString(),
			*Star.GetName(), Glyph.Index);
		return true;
	}
}

void APSFarStarGlyphs::Update(UWorld* World, const TArray<AActor*>& Attached, const FVector& Camera, const double PixelTangent,
	const bool bDaylightHidden)
{
	using namespace APSFarStarGlyphsLocal;
	if (!World || !FMath::IsFinite(PixelTangent) || PixelTangent <= 0.0)
	{
		return;
	}
	for (auto It = GGlyphs.CreateIterator(); It; ++It)
	{
		if (!It.Key().IsValid())
		{
			It.RemoveCurrent();
		}
	}
	TArray<FGlyph>& Glyphs = GGlyphs.FindOrAdd(World);
	const AStarCluster* Cluster = nullptr;
	for (const AActor* Actor : Attached)
	{
		if ((Cluster = Cast<AStarCluster>(Actor)) != nullptr)
		{
			break;
		}
	}
	const UHierarchicalInstancedStaticMeshComponent* Source = IsValid(Cluster) ? Cluster->StarMeshInstances : nullptr;
	const float GlyphPixels = CVarGlyphPixels.GetValueOnGameThread();
	if (GlyphPixels <= 0.0f || !IsValid(Source) || !IsValid(Source->GetStaticMesh()))
	{
		for (FGlyph& Glyph : Glyphs) Destroy(Glyph);
		Glyphs.Reset();
		return;
	}

	// The stars that stand materialized now, each with its catalogue point.
	TSet<const AStar*> Present;
	for (const FClusterStarSystemRecord& Record : Cluster->PotentialStarSystems)
	{
		const AStarSystem* System = Record.bMaterialized ? Record.MaterializedSystem.Get() : nullptr;
		AStar* Star = System ? System->MainStar : nullptr;
		if (!IsValid(Star) || Record.InstanceIndex == INDEX_NONE)
		{
			continue;
		}
		Present.Add(Star);
		if (!Glyphs.ContainsByPredicate([Star](const FGlyph& Glyph) { return Glyph.Star.Get() == Star; }))
		{
			FGlyph& Glyph = Glyphs.AddDefaulted_GetRef();
			Glyph.Star = Star;
			Glyph.Index = Record.InstanceIndex;
		}
	}
	Glyphs.RemoveAll([&Present](FGlyph& Glyph)
	{
		const bool bGone = !Glyph.Star.IsValid() || !Present.Contains(Glyph.Star.Get());
		if (bGone) Destroy(Glyph);
		return bGone;
	});

	const double MeshRadius = FMath::Max(Source->GetStaticMesh()->GetBounds().BoxExtent.GetMax(), 0.001);
	for (FGlyph& Glyph : Glyphs)
	{
		const AStar* Star = Glyph.Star.Get();
		const double RadiusCm = FMath::Max(static_cast<double>(Star->StarRadiusKM), 1.0) * 100000.0;
		const double PixelWorldRadius = FVector::Distance(Camera, Star->GetActorLocation()) * PixelTangent;
		const double ApparentPixels = PixelWorldRadius > 0.0 ? RadiusCm / PixelWorldRadius : TNumericLimits<double>::Max();
		// The glyph while the disc is smaller than it; from there on the sphere itself (photosphere and corona).
		const bool bShow = !bDaylightHidden && ApparentPixels < GlyphPixels && !Star->IsHidden();
		if (!bShow)
		{
			if (UInstancedStaticMeshComponent* Mesh = Glyph.Mesh.Get(); Mesh && Mesh->IsVisible())
			{
				Mesh->SetVisibility(false);
			}
			continue;
		}
		if (!Glyph.Mesh.IsValid() && !Create(*World, Glyph, *Source, *Star))
		{
			continue;
		}
		UInstancedStaticMeshComponent* Mesh = Glyph.Mesh.Get();
		// Sized as the stellar view sizes catalogue points: a compact core of at least a couple of pixels, rays for a
		// bright star, from the point's own data.
		const APSStellarOpticalSupport::FProfile Profile = APSStellarOpticalSupport::Select(Source->PerInstanceSMCustomData,
			Source->NumCustomDataFloats, Glyph.Index);
		const double Core = APSStellarOpticalSupport::CoreRadius(RadiusCm, PixelWorldRadius);
		const double Carrier = APSStellarOpticalSupport::CarrierRadius(RadiusCm, PixelWorldRadius, Profile);
		// Data and size reach the GPU through the instance's own updates (no render state rebuild); the size only when it
		// moved by a percent, a hundredth of a pixel at these sizes.
		APSStellarOpticalSupport::Publish(Mesh, 0, APSStellarOpticalSupport::CoreScale(Core, Carrier),
			APSStellarOpticalSupport::ResolvedRayStrength(Profile, RadiusCm, PixelWorldRadius));
		if (!Glyph.Actor->GetActorLocation().Equals(Star->GetActorLocation(), 1.0))
		{
			Glyph.Actor->SetActorLocation(Star->GetActorLocation(), false, nullptr, ETeleportType::TeleportPhysics);
		}
		if (!FMath::IsNearlyEqual(Glyph.Carrier, Carrier, Carrier * 0.01))
		{
			Glyph.Carrier = Carrier;
			Mesh->UpdateInstanceTransform(0, FTransform(FQuat::Identity, FVector::ZeroVector, FVector(Carrier / MeshRadius)),
				false, false, true);
		}
		if (!Mesh->IsVisible())
		{
			Mesh->SetVisibility(true);
		}
	}
}

void APSFarStarGlyphs::Reset(const UWorld* World)
{
	using namespace APSFarStarGlyphsLocal;
	if (TArray<FGlyph>* Glyphs = GGlyphs.Find(World))
	{
		for (FGlyph& Glyph : *Glyphs) Destroy(Glyph);
		GGlyphs.Remove(World);
	}
}
