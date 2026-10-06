#include "APSFarStarGlyphs.h"

#include "APSGalaxyGpuStars.h"
#include "APSGameplayStarAppearance.h"
#include "APSStellarOpticalSupport.h"
#include "APS_ALPHA/Actors/Astro/Galaxy.h"
#include "APS_ALPHA/Actors/Astro/Star.h"
#include "APS_ALPHA/Actors/Astro/StarCluster.h"
#include "APS_ALPHA/Actors/Astro/StarSystem.h"
#include "APS_ALPHA/Core/Rendering/APSCanonicalStellarProjection.h"
#include "APS_ALPHA/Core/World/APSRealScale.h"
#include "APS_ALPHA/Core/World/APSWorldOriginSubsystem.h"
#include "APS_ALPHA/Gameplay/Expansion/APSStarSystems.h"
#include "APS_ALPHA/Gameplay/Expansion/APSSystemMaterializer.h"
#include "APS_ALPHA/Generation/APSGalaxyMorphology.h"
#include "APS_ALPHA/Generation/StarGenerator.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "HAL/IConsoleManager.h"
#include "HAL/PlatformTime.h"

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
		/** Rio 05.10 (real scale, stage 2): a galaxy star's own row of per-star data (empty: the catalogue point's, Index). */
		TArray<float> Row;
		/** Rio 05.10 night: a galaxy star's glyph logs its hand-over to the sphere (shown last frame, next periodic line). */
		bool bGalaxy{false};
		bool bShown{false};
		double NextLogSeconds{0.0};
	};
	TMap<TWeakObjectPtr<const UWorld>, TArray<FGlyph>> GGlyphs;

	/**
	 * Rio 05.10 (real scale, stage 2): the row a galaxy star's glyph draws with, made as the cluster's rows are made
	 * (AAstroGenerator::ComposeCanonicalStellarProjection): its colour, its light at the real distances, seed and gain.
	 */
	TArray<float> MakeGalaxyRow(const AStarCluster& Cluster, const FGalaxyCatalogStarRecord& Record, const int32 Stride)
	{
		TArray<float> Row;
		Row.Init(0.0f, FMath::Max(Stride, 0));
		if (Stride < APSStellarOpticalSupport::RequiredStride)
		{
			return Row;
		}
		const double RadiusSolar = APSCanonicalStellarProjection::GetCanonicalStellarRadiusSolar(Record.SpectralClass)
			* FMath::Max(static_cast<double>(Record.RadiusScale), 0.0);
		const double Luminosity = APSCanonicalStellarProjection::GetCanonicalStellarLuminositySolar(Record.SpectralClass)
			* APSGalaxyMorphology::GetRadiusScaleLuminosity(Record.RadiusScale);
		const FAPSCanonicalStellarProjectionFrame& Frame = Cluster.CanonicalProjectionFrame;
		const double AppliedRadiusCm = APSCanonicalStellarProjection::GetAppliedVisualRadiusCm(
			EAPSCanonicalStellarProxyLayer::StarCluster, Frame, RadiusSolar);
		const double AppliedRadiusSolar = APSCanonicalStellarProjection::GetLegacyLayoutRadiusSolar(Frame,
			APSCanonicalStellarProjection::UnprojectPhysicalRadiusSolar(Frame, AppliedRadiusCm));
		const double Emission = UStarGenerator::GetFarStarVisualEmission(RadiusSolar,
			GetMutableDefault<UStarGenerator>()->CalculateEmission(static_cast<float>(Luminosity * 25.0)), AppliedRadiusSolar);
		const FLinearColor Color = UStarGenerator::GetStarColor(Record.SpectralClass, Record.SpectralSubclass);
		Row[0] = Color.R;
		Row[1] = Color.G;
		Row[2] = Color.B;
		Row[3] = static_cast<float>(Emission);
		Row[4] = static_cast<float>(Record.GenerationSeed & 0xffff) / 65535.0f;
		Row[6] = APSGameplayStarAppearance::GetLuminosityGain(Luminosity);
		Row[APSStellarOpticalSupport::CoreScaleIndex] = 1.0f;
		return Row;
	}

	/**
	 * Rio 05.10 (real scale, stage 2): a REAL SCALE galaxy star stands up as a system a quarter of a light year out, where
	 * its sphere is far below a pixel; its catalogue point (ISM prefix) is suppressed and its GPU point hidden by the
	 * system's sphere: its glyph stays, from its record.
	 * Rio 05.10 night: only the fallback now. A star the GPU layer draws keeps its approach point (APSGalaxyGpuStars::
	 * UpdateApproachPoints: the same photometry as before, the exact place, a crossfade into this sphere at the same disc
	 * size), so the glyph is made only for a star of the ISM prefix, without a GPU layer, or with aps.Stars.ApproachPoint 0.
	 */
	void AddGalaxyGlyph(UWorld& World, const AStarCluster& Cluster, const int32 Stride, TArray<FGlyph>& Glyphs,
		TSet<const AStar*>& Present)
	{
		const FAPSStarSystems* Systems = APSStarSystemsFind(&World);
		const FAPSSystemMaterializer* Materializer = Systems ? Systems->GetMaterializer() : nullptr;
		const FAPSStarSystemInfo* Info = Materializer ? Systems->Get(Materializer->GetActiveIndex()) : nullptr;
		const AGalaxy* Galaxy = APSGalaxyGpuStars::GetIndexedGalaxy(&World);
		FGalaxyCatalogStarRecord Record;
		if (!Info || Info->GalaxyIndex == INDEX_NONE || !Galaxy || !Galaxy->StarCatalog.ResolveStar(Info->GalaxyIndex, Record)
			|| APSGalaxyGpuStars::DrawsApproachPoint(&World, Info->GalaxyIndex))
		{
			return;
		}
		// The system's star stands at its catalogue point (APSSystemMaterializer's MaterializeGalaxyStar). Rio 06.10 (still
		// ship): the catalogue's places are the sky's; a star actor stands at its sky place (SkyPlace: a system riding with
		// the sky where it is, any other at its world place + the sky offset).
		AStar* Star = nullptr;
		double BestSquared = TNumericLimits<double>::Max();
		for (TActorIterator<AStar> It(&World); It; ++It)
		{
			const double DistanceSquared = FVector::DistSquared(UAPSWorldOriginSubsystem::SkyPlace(**It), Info->Location);
			if (IsValid(*It) && DistanceSquared < BestSquared)
			{
				BestSquared = DistanceSquared;
				Star = *It;
			}
		}
		if (!Star || BestSquared > FMath::Square(FMath::Max(Info->StarRadiusCm, 1.0e10) * 3.0))
		{
			return;
		}
		Present.Add(Star);
		if (!Glyphs.ContainsByPredicate([Star](const FGlyph& Glyph) { return Glyph.Star.Get() == Star; }))
		{
			FGlyph& Glyph = Glyphs.AddDefaulted_GetRef();
			Glyph.Star = Star;
			Glyph.Row = MakeGalaxyRow(Cluster, Record, Stride);
			Glyph.bGalaxy = true;
			// Rio 05.10 night (with aps.Stars.ApproachPoint): a star of the galaxy's ISM prefix keeps the row its own catalogue
			// point drew with (made in the galaxy's frame), not one made again in the cluster's: the same light at the hand-over.
			const UHierarchicalInstancedStaticMeshComponent* GalaxyPoints = Galaxy->StarMeshInstances;
			const int32 Instance = Galaxy->RenderedCatalogIndices.Find(Info->GalaxyIndex);
			if (APSGalaxyGpuStars::AreApproachPointsActive(&World) && IsValid(GalaxyPoints) && Instance != INDEX_NONE
				&& GalaxyPoints->NumCustomDataFloats == Stride
				&& GalaxyPoints->PerInstanceSMCustomData.IsValidIndex((Instance + 1) * Stride - 1))
			{
				Glyph.Row = TArray<float>(GalaxyPoints->PerInstanceSMCustomData.GetData() + Instance * Stride, Stride);
				UE_LOG(LogTemp, Log, TEXT("[APS.Stars] far glyph for %s takes its ISM point's own row (instance %d)"),
					*Info->Name, Instance);
			}
		}
	}

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
		// Rio 05.10 (real scale, stage 2): a galaxy star's glyph brings its own row (MakeGalaxyRow).
		const bool bOwnRow = !Glyph.Row.IsEmpty();
		if (Stride < APSStellarOpticalSupport::RequiredStride || (bOwnRow ? Glyph.Row.Num() != Stride
			: !Source.PerInstanceSMCustomData.IsValidIndex(Address + Stride - 1)))
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
		Mesh->SetCustomData(0, bOwnRow ? TArrayView<const float>(Glyph.Row)
			: TArrayView<const float>(Source.PerInstanceSMCustomData.GetData() + Address, Stride), false);
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
	// Rio 05.10 night: the GPU stars near the camera in a REAL SCALE world first (their approach points), so the galaxy
	// glyph below knows whether its star still needs it. After PresentGameplayFrame, in the same frame.
	APSGalaxyGpuStars::UpdateApproachPoints(World, Camera, PixelTangent);
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
	// Rio 05.10 (real scale, stage 2): and the galaxy star that stands materialized in a REAL SCALE world.
	if (APSRealScale::IsActive(World))
	{
		AddGalaxyGlyph(*World, *Cluster, Source->NumCustomDataFloats, Glyphs, Present);
	}
	Glyphs.RemoveAll([&Present](FGlyph& Glyph)
	{
		const bool bGone = !Glyph.Star.IsValid() || !Present.Contains(Glyph.Star.Get());
		if (bGone) Destroy(Glyph);
		return bGone;
	});

	const double MeshRadius = FMath::Max(Source->GetStaticMesh()->GetBounds().BoxExtent.GetMax(), 0.001);
	// Rio 06.10 (still ship): while a fast REAL SCALE ship owes its travel only the sky moves; a glyph stands where the sky
	// has its star (SkyPlace) and is sized from there.
	const double NowSeconds = FPlatformTime::Seconds();
	for (FGlyph& Glyph : Glyphs)
	{
		const AStar* Star = Glyph.Star.Get();
		const FVector StarInSky = UAPSWorldOriginSubsystem::SkyPlace(*Star);
		const double RadiusCm = FMath::Max(static_cast<double>(Star->StarRadiusKM), 1.0) * 100000.0;
		const double PixelWorldRadius = FVector::Distance(Camera, StarInSky) * PixelTangent;
		const double ApparentPixels = PixelWorldRadius > 0.0 ? RadiusCm / PixelWorldRadius : TNumericLimits<double>::Max();
		// The glyph while the disc is smaller than it; from there on the sphere itself (photosphere and corona).
		const bool bShow = !bDaylightHidden && ApparentPixels < GlyphPixels && !Star->IsHidden();
		// Rio 05.10 night: a galaxy star's glyph (the fallback without an approach point) logs its hand-over to the sphere.
		if (Glyph.bGalaxy && bShow != Glyph.bShown)
		{
			UE_LOG(LogTemp, Log, TEXT("[APS.Stars] far glyph for %s: %s at %.4g AU (disc %.3f px, threshold %.2f px)"),
				*Star->AstroName.ToString(), bShow ? TEXT("sphere -> glyph") : TEXT("glyph -> sphere"),
				PixelWorldRadius / PixelTangent / 1.495978707e13, ApparentPixels, GlyphPixels);
			Glyph.bShown = bShow;
		}
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
		// A galaxy star's glyph (Rio 05.10, real scale) reads its own row.
		const APSStellarOpticalSupport::FProfile Profile = Glyph.Row.IsEmpty()
			? APSStellarOpticalSupport::Select(Source->PerInstanceSMCustomData, Source->NumCustomDataFloats, Glyph.Index)
			: APSStellarOpticalSupport::Select(Glyph.Row, Glyph.Row.Num(), 0);
		const double Core = APSStellarOpticalSupport::CoreRadius(RadiusCm, PixelWorldRadius);
		const double Carrier = APSStellarOpticalSupport::CarrierRadius(RadiusCm, PixelWorldRadius, Profile);
		// Data and size reach the GPU through the instance's own updates (no render state rebuild); the size only when it
		// moved by a percent, a hundredth of a pixel at these sizes.
		APSStellarOpticalSupport::Publish(Mesh, 0, APSStellarOpticalSupport::CoreScale(Core, Carrier),
			APSStellarOpticalSupport::ResolvedRayStrength(Profile, RadiusCm, PixelWorldRadius));
		if (Glyph.bGalaxy && NowSeconds >= Glyph.NextLogSeconds)
		{
			Glyph.NextLogSeconds = NowSeconds + 0.5;
			UE_LOG(LogTemp, Log, TEXT("[APS.Stars] far glyph for %s (fallback): %.4g AU, disc %.3f px, core %.2f px, carrier %.2f px, emission %.4g"),
				*Star->AstroName.ToString(), PixelWorldRadius / PixelTangent / 1.495978707e13, ApparentPixels,
				Core / PixelWorldRadius, Carrier / PixelWorldRadius, Glyph.Row.IsValidIndex(3) ? Glyph.Row[3] : -1.0f);
		}
		if (!Glyph.Actor->GetActorLocation().Equals(StarInSky, 1.0))
		{
			Glyph.Actor->SetActorLocation(StarInSky, false, nullptr, ETeleportType::TeleportPhysics);
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
