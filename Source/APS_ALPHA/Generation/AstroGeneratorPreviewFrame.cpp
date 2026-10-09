#include "AstroGenerator.h"
#include "APS_ALPHA/Generation/APSPreviewClusterSpacing.h"

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
#include "APS_ALPHA/Core/Rendering/APSPreviewCameraBounds.h"
#include "APS_ALPHA/Core/Rendering/APSStellarOpticalSupport.h"
#include "APS_ALPHA/Core/Rendering/APSStellarViewOptics.h"
#include "APS_ALPHA/Core/Rendering/APSGalaxyGpuStars.h"
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
#include "Async/ParallelFor.h"
#include <atomic>

CSV_DEFINE_CATEGORY(APSPreview, true);

namespace
{
// The ordinary menu and its route tests must use the same physical observer.
// Value 0 remains a diagnostic rollback, never a different catalog or generator.
TAutoConsoleVariable<int32> CVarContinuousPreviewFrame(
	TEXT("aps.Preview.ContinuousFrame"), 1,
	TEXT("Use the common physical observer for every generation-menu scope (new preview required)."));

// Rio 06.10 ("120 FPS on the galaxy, 90 while I turn it"): every moving frame the catalogue views re-sent their
// instances, and an ISM without conservative bounds walks all of them for its bounds each time. The gameplay
// stellar view already runs this way (APSGameplayStellarView). 0 restores the old views (new preview required).
TAutoConsoleVariable<int32> CVarPreviewViewConservativeBounds(
	TEXT("aps.Preview.ViewConservativeBounds"), 1,
	TEXT("Generation-menu catalogue views keep conservative bounds and skip lighting/WPO work they never use."));

// Rio 02.10: FPS fell hard while the cluster camera turned. Each orbit step re-sent every catalogue point to the GPU.
TAutoConsoleVariable<int32> CVarCatalogDeltaUpload(
	TEXT("aps.Preview.CatalogDeltaUpload"), 1,
	TEXT("1: a preview frame re-sends only the catalogue points that moved by more than 0.15 px or resized by 0.4%."));

namespace APSPreviewCatalogDelta
{
	/** The transforms last sent per catalogue view (the menu has one galaxy and one cluster view). */
	TMap<TWeakObjectPtr<UInstancedStaticMeshComponent>, TArray<FTransform>> Sent;

	/** Everything a catalogue frame depends on: an identical key gives identical points, so the frame is skipped. */
	struct FFrameKey
	{
		FVector Observer{FVector::ZeroVector};
		double Scale{0.0};
		double PixelTangent{0.0};
		uint64 Resolved{0};
		uint64 Materialized{0};
		int32 Points{0};
		int32 Instances{0};
		/** The live ray settings: changing one re-publishes every point. */
		double Optics{0.0};
		/** Rio 03.10: the home system's exclusion sphere (centre, radius); a system edit re-publishes every point. */
		FVector4 Exclusion{0.0, 0.0, 0.0, 0.0};
		/** Rio 03.10 (fast path): view direction, frustum tangents and the fast-path settings decide culling and caps. */
		FQuat ViewRotation{FQuat::Identity};
		FVector2D ViewTangents{FVector2D::ZeroVector};
		double Policy{0.0};
		bool operator==(const FFrameKey& Other) const
		{
			return Observer == Other.Observer && Scale == Other.Scale && PixelTangent == Other.PixelTangent
				&& Resolved == Other.Resolved && Materialized == Other.Materialized && Points == Other.Points
				&& Instances == Other.Instances && Optics == Other.Optics && Exclusion == Other.Exclusion
				&& ViewRotation == Other.ViewRotation && ViewTangents == Other.ViewTangents && Policy == Other.Policy;
		}
	};
	TMap<TWeakObjectPtr<UInstancedStaticMeshComponent>, FFrameKey> Presented;

	/** Sends the frame's transforms; with the delta rule only the changed points. Returns the count sent. */
	int32 Upload(UInstancedStaticMeshComponent* View, const TArray<FTransform>& Transforms, const double PixelTangent)
	{
		for (auto It = Sent.CreateIterator(); It; ++It)
		{
			if (!It.Key().IsValid()) It.RemoveCurrent();
		}
		TArray<FTransform>& Last = Sent.FindOrAdd(View);
		if (CVarCatalogDeltaUpload.GetValueOnGameThread() == 0 || Last.Num() != Transforms.Num()
			|| View->GetInstanceCount() != Transforms.Num())
		{
			View->BatchUpdateInstancesTransforms(0, Transforms, true, false, true);
			Last = Transforms;
			return Transforms.Num();
		}
		TArray<int32> ChangedIndices;
		for (int32 Index = 0; Index < Transforms.Num(); ++Index)
		{
			const FTransform& New = Transforms[Index];
			FTransform& Old = Last[Index];
			// The camera sits at the origin: a point's pixel size is its distance times the pixel tangent.
			const double Pixel = FMath::Max(New.GetLocation().Size(), 1.0) * PixelTangent;
			const double OldScale = Old.GetScale3D().X;
			const double NewScale = New.GetScale3D().X;
			const bool bResized = (OldScale == 0.0) != (NewScale == 0.0)
				|| FMath::Abs(NewScale - OldScale) > FMath::Max(OldScale, NewScale) * 0.004;
			const bool bMoved = NewScale != 0.0
				&& FVector::DistSquared(Old.GetLocation(), New.GetLocation()) > FMath::Square(Pixel * 0.15);
			if (!bResized && !bMoved) continue;
			ChangedIndices.Add(Index);
		}
		// While the camera orbits nearly every point moves: one batch costs far less than per-instance updates.
		if (ChangedIndices.Num() * 4 > Transforms.Num())
		{
			View->BatchUpdateInstancesTransforms(0, Transforms, true, false, true);
			Last = Transforms;
			return Transforms.Num();
		}
		for (const int32 Index : ChangedIndices)
		{
			View->UpdateInstanceTransform(Index, Transforms[Index], true, false, true);
			Last[Index] = Transforms[Index];
		}
		return ChangedIndices.Num();
	}
}

TAutoConsoleVariable<int32> CVarResolvedStarPreparation(
	TEXT("aps.Preview.ResolvedStarPreparation"), 1,
	TEXT("Prepare bounded hidden stellar component capacity during an existing continuous flight; never delay visible stars."));

/**
 * Rio 03.10 (crash "InStateBucketId < (1 << 14) - 1" in NaniteMaterials.h after GALAXY SIZE 1: 26,821 catalogue stars
 * resolved at once, 3.6 s in one frame; ~10 fps with hundreds of giants on screen): at most this many catalogue stars
 * are drawn as photosphere + corona pairs, the largest on screen first. The rest stay optical points of the same size.
 * Each pair owns its own MIDs (one Nanite material bin each), so the cap also bounds Nanite's material buckets.
 */
TAutoConsoleVariable<int32> CVarResolvedStarCap(
	TEXT("aps.Preview.ResolvedStarCap"), 48,
	TEXT("Most catalogue stars drawn as photosphere + corona pairs at once (largest on screen first; the rest stay points)."));

TAutoConsoleVariable<float> CVarFarStarDetail(
	TEXT("aps.Preview.FarStarDetail"), 0.2f,
	TEXT("Rio 04.10 evening (\"far stars: no prominences or sunspots, a smoother surface\"): share of the surface variation, ")
	TEXT("granulation and spots a neighbour star of the menu preview keeps (1 = the full close-up surface). Applies to ")
	TEXT("stars resolved after the change."));

/** Pairs newly shown per applied frame (taken from the pool or allocated); the rest resolve over the next frames. */
constexpr int32 ResolvedStarRevealsPerFrame = 16;
/** After the first few reveals of a frame, further ones only while this much of the frame is left (seconds). */
constexpr double ResolvedStarRevealSeconds = 0.003;

int32 GetResolvedStarCap()
{
	return FMath::Clamp(CVarResolvedStarCap.GetValueOnGameThread(), 0, AAstroGenerator::ContinuousResolvedStarPoolLimit);
}

// Rio 03.10 04:30 ("30-40 fps while the cluster/galaxy camera moves, ~10 fps among giants"): the serial catalogue pass
// projected every point, then BatchUpdateInstancesTransforms rebuilt each matrix from an FTransform on the game thread.
TAutoConsoleVariable<int32> CVarFastCatalog(
	TEXT("aps.Preview.FastCatalog"), 1,
	TEXT("1: catalogue points are projected in one parallel pass and sent as precomputed instance matrices "
		"(BatchUpdateInstancesData). 0: the previous serial path. On by default since gate g2 (03.10): same frames, "
		"galaxy orbit 26.7 -> 16.5 ms and cluster orbit 30.6 -> 19.6 ms in a 50k-star galaxy with a Colossal cluster."));
TAutoConsoleVariable<int32> CVarCatalogCull(
	TEXT("aps.Preview.CatalogCull"), 1,
	TEXT("With aps.Preview.FastCatalog: 1 zero-scales catalogue points outside the view (32 px guard), so they cost nothing "
		"until they return. 0: every point is presented."));
TAutoConsoleVariable<float> CVarMaxPointPixels(
	TEXT("aps.Preview.MaxPointPixels"), 64.0f,
	TEXT("With aps.Preview.FastCatalog: largest radius in pixels of a star drawn as an optical point (stars beyond the "
		"resolved-star cap); 0 = unlimited."));
TAutoConsoleVariable<int32> CVarParallelResolve(
	TEXT("aps.Preview.ParallelResolve"), 1,
	TEXT("1: the per-frame resolved-star test and its flight forecast run in parallel. 0: the previous serial loops."));

/** The fast path's per-view state: what was sent last and this frame's matrices, one entry per instance. */
namespace APSPreviewCatalogFast
{
	enum : uint8 { MovedBit = 1, OpticsBit = 2, DeferredBit = 4, ExcludedBit = 8 };

	struct FState
	{
		const void* PointsData = nullptr;
		int32 PointsNum = -1;
		TArray<int32> InstanceToPoint;
		TArray<FInstancedStaticMeshInstanceData> Pending;
		/** (render centre, scale) last sent and computed this frame; scale 0 = hidden. */
		TArray<FVector4> Last;
		TArray<FVector4> Next;
		TArray<float> Core;
		TArray<float> Ray;
		TArray<uint8> Flags;
	};
	TMap<TWeakObjectPtr<UInstancedStaticMeshComponent>, FState> States;

	/** FTransform(FQuat::Identity, Centre, FVector(Scale)).ToMatrixWithScale(), without the quaternion maths. */
	inline FMatrix PointMatrix(const FVector4& Point)
	{
		return FMatrix(FPlane(Point.W, 0.0, 0.0, 0.0), FPlane(0.0, Point.W, 0.0, 0.0), FPlane(0.0, 0.0, Point.W, 0.0),
			FPlane(Point.X, Point.Y, Point.Z, 1.0));
	}

	/** The serial path's delta rule: moved by more than 0.15 px, resized by 0.4%, or shown/hidden. */
	inline bool HasChanged(const FVector4& Old, const FVector4& New, const double PixelTangent)
	{
		const bool bResized = (Old.W == 0.0) != (New.W == 0.0)
			|| FMath::Abs(New.W - Old.W) > FMath::Max(Old.W, New.W) * 0.004;
		const FVector NewCenter(New.X, New.Y, New.Z);
		const double Pixel = FMath::Max(NewCenter.Size(), 1.0) * PixelTangent;
		return bResized || (New.W != 0.0
			&& FVector::DistSquared(FVector(Old.X, Old.Y, Old.Z), NewCenter) > FMath::Square(Pixel * 0.15));
	}
}

/** Half-angle tangents of the real view (letterbox, FOV axis, off-centre projection); never narrower than the camera. */
void GetPreviewViewTangents(APlayerController* Controller, const double FieldOfViewDegrees, const int32 Width,
	const int32 Height, double& OutTanHalfHorizontal, double& OutTanHalfVertical)
{
	OutTanHalfHorizontal = FMath::Tan(FMath::DegreesToRadians(FieldOfViewDegrees * 0.5));
	OutTanHalfVertical = OutTanHalfHorizontal * FMath::Max(Height, 1) / FMath::Max(Width, 1);
	ULocalPlayer* Player = IsValid(Controller) ? Controller->GetLocalPlayer() : nullptr;
	FViewport* Viewport = IsValid(Player) && IsValid(Player->ViewportClient) ? Player->ViewportClient->Viewport : nullptr;
	FSceneViewProjectionData Projection;
	if (!Viewport || !Player->GetProjectionData(Viewport, Projection) || !Projection.IsPerspectiveProjection()) return;
	const FMatrix& Matrix = Projection.ProjectionMatrix;
	const double X = FMath::Abs(Matrix.M[0][0]);
	const double Y = FMath::Abs(Matrix.M[1][1]);
	if (!FMath::IsFinite(X) || !FMath::IsFinite(Y) || X <= UE_SMALL_NUMBER || Y <= UE_SMALL_NUMBER) return;
	OutTanHalfHorizontal = FMath::Max(OutTanHalfHorizontal, (1.0 + FMath::Abs(Matrix.M[2][0])) / X);
	OutTanHalfVertical = FMath::Max(OutTanHalfVertical, (1.0 + FMath::Abs(Matrix.M[2][1])) / Y);
}

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
	// Rio 03.10 (fps among giants): SetAbsolute always re-propagates the transform; once absolute it is redundant.
	if (!Mesh->IsUsingAbsoluteLocation() || !Mesh->IsUsingAbsoluteRotation() || !Mesh->IsUsingAbsoluteScale())
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

/**
 * Rio 03.10 ("galaxy POPULATION / COMPOSITION: nothing really changes"): a far point is a carrier of a few pixels, so a
 * giant's radius never showed. Per catalogue view and instance, its carrier grows with the POPULATION size factor,
 * gently: giants about 2x, super/hypergiants 2.4x, dwarfs down to 0.6x (and an additive point's light with its area).
 */
TMap<TWeakObjectPtr<const UInstancedStaticMeshComponent>, TArray<float>> PointDisplayScales;

float PopulationPointScale(const float RadiusScale)
{
	return FMath::IsNearlyEqual(RadiusScale, 1.0f) ? 1.0f
		: FMath::Clamp(FMath::Pow(FMath::Max(RadiusScale, 1.0e-4f), 0.3f), 0.6f, 2.4f);
}

TAutoConsoleVariable<float> CVarNeighbourRadiusShare(
	TEXT("aps.Preview.NeighbourRadiusShare"), 0.2f,
	TEXT("Rio 03.10 (\"the stars all stuck into each other\"): a cluster star's drawn radius is capped at this share of the "
		"distance to its nearest neighbour. 0.2 keeps at least 60% of that distance clear between two neighbours; the old "
		"0.45 let giant populations nearly touch. A new preview applies a change."));

/**
 * Rio 03.10 ("Supercluster: stars run into each other, they must not overlap"): a dense core can hold stars closer
 * than their own radii at this compressed scale. Cap each point's drawn radius at a share (aps.Preview.NeighbourRadiusShare)
 * of the distance to its nearest neighbour; positions and star data stay untouched. An X-sorted sweep, scanning both
 * ways while dx < best.
 */
void LimitPointRadiiToNeighbourSpacing(TArray<FAPSContinuousPreviewPoint>& Points)
{
	const int32 Count = Points.Num();
	if (Count < 2) return;
	const double Share = FMath::Clamp(static_cast<double>(CVarNeighbourRadiusShare.GetValueOnGameThread()), 0.01, 0.45);
	TArray<int32> Order;
	Order.SetNumUninitialized(Count);
	for (int32 Index = 0; Index < Count; ++Index) Order[Index] = Index;
	Order.Sort([&Points](const int32 A, const int32 B) { return Points[A].CenterCm.X < Points[B].CenterCm.X; });
	int32 Limited = 0;
	for (int32 Rank = 0; Rank < Count; ++Rank)
	{
		FAPSContinuousPreviewPoint& Point = Points[Order[Rank]];
		double BestSquared = TNumericLimits<double>::Max();
		for (int32 Next = Rank + 1; Next < Count; ++Next)
		{
			const FVector& Other = Points[Order[Next]].CenterCm;
			if (FMath::Square(Other.X - Point.CenterCm.X) >= BestSquared) break;
			const double DistanceSquared = FVector::DistSquared(Point.CenterCm, Other);
			if (DistanceSquared > 0.0) BestSquared = FMath::Min(BestSquared, DistanceSquared);
		}
		for (int32 Previous = Rank - 1; Previous >= 0; --Previous)
		{
			const FVector& Other = Points[Order[Previous]].CenterCm;
			if (FMath::Square(Point.CenterCm.X - Other.X) >= BestSquared) break;
			const double DistanceSquared = FVector::DistSquared(Point.CenterCm, Other);
			if (DistanceSquared > 0.0) BestSquared = FMath::Min(BestSquared, DistanceSquared);
		}
		if (BestSquared < TNumericLimits<double>::Max())
		{
			const double Limit = Share * FMath::Sqrt(BestSquared);
			if (Point.RadiusCm > Limit)
			{
				Point.RadiusCm = Limit;
				++Limited;
			}
		}
	}
	UE_LOG(LogTemp, Log, TEXT("[APS.Preview.Cluster] Neighbour spacing: %d of %d star points drawn smaller (at most %.0f%% of the nearest distance)"),
		Limited, Count, Share * 100.0);
}

TAutoConsoleVariable<float> CVarClusterSpacingPixels(
	TEXT("aps.Preview.ClusterSpacingPixels"), 3.0f,
	TEXT("Rio 03.10 (dense cores: \"close, but not on top of each other\"): least distance, in pixels at the CLUSTER framing "
		"distance, between the menu's live cluster systems. Presentation only, the catalogue never moves. 0 = off. "
		"A new preview applies a change."));

/** Per generator: live cluster system (InstanceIndex) -> presentation offset in cm; only moved systems are stored. */
TMap<TWeakObjectPtr<const AAstroGenerator>, TMap<int32, FVector>> ClusterSpacingOffsets;

/**
 * Rio 03.10 ("the points in the centre stick onto each other"): spaces system centres for presentation. First a shell
 * density cap around the cluster centre (dense Plummer/globular cores: systems keep their direction and distance order,
 * no shell holds more than one system per 5 Separation^3), then up to six relaxation steps on a fixed-size spatial hash
 * push any pair still closer than Separation apart (half each; a fixed system such as home takes nothing; one step moves
 * a system at most half the separation). Deterministic for one sample. Returns one offset per centre.
 */
TArray<FVector> SpaceSystemCentres(const TArray<FVector>& Centres, const TArray<uint8>& Fixed, const FVector& ClusterCenter,
	const double Separation)
{
	const int32 Count = Centres.Num();
	TArray<FVector> Positions = Centres;
	TArray<FVector> Offsets;
	Offsets.SetNumZeroed(Count);
	if (Count < 2 || Fixed.Num() != Count || !FMath::IsFinite(Separation) || Separation <= 0.0) return Offsets;
	const auto FixedDirection = [](const uint32 Seed)
	{
		const uint32 Hash = HashCombineFast(Seed, 0x9e3779b9u);
		return FVector(double(Hash & 1023u) - 511.5, double((Hash >> 10) & 1023u) - 511.5,
			double((Hash >> 20) & 1023u) - 511.5).GetSafeNormal();
	};

	TArray<int32> Order;
	Order.Reserve(Count);
	for (int32 Index = 0; Index < Count; ++Index)
		if (!Fixed[Index]) Order.Add(Index);
	Order.Sort([&Centres, &ClusterCenter](const int32 A, const int32 B)
	{
		const double DistanceA = FVector::DistSquared(Centres[A], ClusterCenter);
		const double DistanceB = FVector::DistSquared(Centres[B], ClusterCenter);
		return DistanceA < DistanceB || (DistanceA == DistanceB && A < B);
	});
	const double VolumePerSystem = 5.0 * Separation * Separation * Separation;
	double Volume = 0.0;
	for (const int32 Index : Order)
	{
		const FVector Offset = Centres[Index] - ClusterCenter;
		const double Radius = Offset.Size();
		Volume = FMath::Max(4.0 / 3.0 * UE_DOUBLE_PI * Radius * Radius * Radius, Volume + VolumePerSystem);
		const double NewRadius = FMath::Pow(3.0 * Volume / (4.0 * UE_DOUBLE_PI), 1.0 / 3.0);
		if (NewRadius <= Radius * (1.0 + 1.0e-9)) continue;
		Positions[Index] = ClusterCenter + (Radius > Separation * 1.0e-6
			? Offset * (NewRadius / Radius) : FixedDirection(uint32(Index)) * NewRadius);
	}

	constexpr int32 TableBits = 18;
	constexpr uint32 TableMask = (1u << TableBits) - 1u;
	TArray<int32> Heads;
	Heads.SetNumUninitialized(1 << TableBits);
	TArray<int32> Next;
	Next.SetNumUninitialized(Count);
	TArray<FIntVector> CellOf;
	CellOf.SetNumUninitialized(Count);
	TArray<FVector> Push;
	Push.SetNumUninitialized(Count);
	const double InverseSeparation = 1.0 / Separation;
	const auto Bucket = [](const FIntVector& Cell)
	{
		return (uint32(Cell.X) * 73856093u ^ uint32(Cell.Y) * 19349663u ^ uint32(Cell.Z) * 83492791u) & TableMask;
	};
	for (int32 Step = 0; Step < 6; ++Step)
	{
		FMemory::Memset(Heads.GetData(), 0xff, Heads.Num() * sizeof(int32));
		for (int32 Index = Count - 1; Index >= 0; --Index)
		{
			const FVector Scaled = Positions[Index] * InverseSeparation;
			CellOf[Index] = FIntVector(FMath::FloorToInt32(Scaled.X), FMath::FloorToInt32(Scaled.Y), FMath::FloorToInt32(Scaled.Z));
			const uint32 Slot = Bucket(CellOf[Index]);
			Next[Index] = Heads[Slot];
			Heads[Slot] = Index;
		}
		FMemory::Memzero(Push.GetData(), Count * sizeof(FVector));
		int32 Crowded = 0;
		for (int32 Index = 0; Index < Count; ++Index)
		{
			for (int32 Z = -1; Z <= 1; ++Z)
			for (int32 Y = -1; Y <= 1; ++Y)
			for (int32 X = -1; X <= 1; ++X)
			{
				const FIntVector Cell = CellOf[Index] + FIntVector(X, Y, Z);
				for (int32 Other = Heads[Bucket(Cell)]; Other != INDEX_NONE; Other = Next[Other])
				{
					// Each pair once; a shared hash slot of another cell is skipped.
					if (Other <= Index || CellOf[Other] != Cell || (Fixed[Index] && Fixed[Other])) continue;
					FVector Delta = Positions[Index] - Positions[Other];
					const double DistanceSquared = Delta.SizeSquared();
					if (DistanceSquared >= Separation * Separation) continue;
					++Crowded;
					const double Distance = FMath::Sqrt(DistanceSquared);
					const FVector Direction = Distance > Separation * 1.0e-6
						? Delta / Distance : FixedDirection(HashCombineFast(GetTypeHash(Index), GetTypeHash(Other)));
					const double Deficit = Separation - Distance;
					const double ShareIndex = Fixed[Index] ? 0.0 : Fixed[Other] ? 1.0 : 0.5;
					Push[Index] += Direction * (Deficit * ShareIndex);
					Push[Other] -= Direction * (Deficit * (1.0 - ShareIndex));
				}
			}
		}
		if (Crowded == 0) break;
		for (int32 Index = 0; Index < Count; ++Index)
		{
			const double Length = Push[Index].Size();
			if (Length > 0.0) Positions[Index] += Push[Index] * (FMath::Min(Length, Separation * 0.5) / Length);
		}
	}
	for (int32 Index = 0; Index < Count; ++Index) Offsets[Index] = Fixed[Index] ? FVector::ZeroVector : Positions[Index] - Centres[Index];
	return Offsets;
}

/** Rio 08.10 (STAR BRIGHTNESS): the name of the material instances it puts on a catalogue view; only those are ever replaced. */
const TCHAR* const PreviewStarBrightnessMaterialName = TEXT("APSPreviewStarBrightness");

/**
 * Rio 08.10 (STAR BRIGHTNESS, a menu viewing aid): a catalogue view's points through their material's point visibility
 * (GameplayPointVisibility, the gameplay daylight term: it scales the whole point and ray signal and is 1 by default). Below
 * 1 the view draws a material instance of its own with that value; at 1 and above its original material is put back, so the
 * accepted look is drawn by the very same material (the shared material clamps the term at 1: no brightening here).
 */
void ApplyPreviewStarBrightness(UInstancedStaticMeshComponent* View, const float Brightness)
{
	if (!IsValid(View)) return;
	static const FName VisibilityParameter(TEXT("GameplayPointVisibility"));
	const float Visibility = FMath::IsFinite(Brightness) ? FMath::Clamp(Brightness, 0.0f, 1.0f) : 1.0f;
	for (int32 Slot = 0; Slot < View->GetNumMaterials(); ++Slot)
	{
		UMaterialInterface* Current = View->GetMaterial(Slot);
		UMaterialInstanceDynamic* Own = Cast<UMaterialInstanceDynamic>(Current);
		if (Own && Own->GetFName().GetPlainNameString() != PreviewStarBrightnessMaterialName) Own = nullptr;
		if (Visibility >= 1.0f)
		{
			if (Own && IsValid(Own->Parent)) View->SetMaterial(Slot, Own->Parent);
			continue;
		}
		if (!Own)
		{
			if (!IsValid(Current)) continue;
			Own = UMaterialInstanceDynamic::Create(Current, View,
				MakeUniqueObjectName(View, UMaterialInstanceDynamic::StaticClass(), FName(PreviewStarBrightnessMaterialName)));
			if (!IsValid(Own)) continue;
			View->SetMaterial(Slot, Own);
		}
		Own->SetScalarParameterValue(VisibilityParameter, Visibility);
	}
}
}

FVector APSPreviewClusterSpacing::GetOffsetCm(const AAstroGenerator* Generator, const int32 InstanceIndex)
{
	const TMap<int32, FVector>* Offsets = ClusterSpacingOffsets.Find(TWeakObjectPtr<const AAstroGenerator>(Generator));
	const FVector* Offset = Offsets ? Offsets->Find(InstanceIndex) : nullptr;
	return Offset ? *Offset : FVector::ZeroVector;
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
		// Rio 03.10 ("SIZE must show"): CLUSTER frames every size against its type's Giant extent, so a Tiny
		// cluster reads small and a Colossal one fills past the frame. Historic clusters have exactly that extent.
		// Rio 04.10 evening ("on CLUSTER the camera goes too far"): halfway in scale between its own extent and the
		// Giant one, so a smaller cluster still reads smaller but no longer sits small in an empty frame.
		const double GiantHalfExtentUnits = Focus == EAstroPreviewFocus::StarCluster && IsValid(StarClusterGenerator)
			? UStarClusterGenerator::GetLogicalHalfExtent(StarClusterGenerator->GetStarClusterBoundsByRange(
				GeneratedStarCluster->ClusterType), GeneratedStarCluster->ClusterType)
			: Frame.CanonicalHalfExtentUnits;
		const double FramedHalfExtentUnits = GiantHalfExtentUnits > 0.0 && Frame.CanonicalHalfExtentUnits > 0.0
			? FMath::Sqrt(GiantHalfExtentUnits * Frame.CanonicalHalfExtentUnits) : GiantHalfExtentUnits;
		RadiusCm = FramedHalfExtentUnits * Frame.LayerToRootPositionScale * Frame.CanonicalCmPerUnit;
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

bool AAstroGenerator::GetPreviewFocusPhysicalDistance(const EAstroPreviewFocus Focus, double& OutDistanceCm) const
{
	OutDistanceCm = 0.0;
	FVector CenterCm;
	double RadiusCm = 0.0;
	if (!UsesContinuousPreviewFrame() || !ContinuousPreviewFrame.IsValid()
		|| !GetContinuousPreviewPhysicalFocus(Focus, CenterCm, RadiusCm))
	{
		return false;
	}
	OutDistanceCm = FVector::Distance(CenterCm, ContinuousPreviewFrame.ObserverCm);
	return FMath::IsFinite(OutDistanceCm) && OutDistanceCm > 0.0;
}

double AAstroGenerator::GetContinuousPreviewFocusEnvelopeCm() const
{
	FVector CenterCm;
	double RadiusCm = 0.0;
	if (!GetContinuousPreviewPhysicalFocus(PreviewFocus, CenterCm, RadiusCm)) return 0.0;
	if (PreviewFocus != EAstroPreviewFocus::HomePlanet) return RadiusCm;
	const AActor* Selected = SelectedPreviewBodyActor.Get();
	if (!Cast<APlanetaryBody>(Selected))
		Selected = ContinuousSelectedPlanet.IsValid() ? ContinuousSelectedPlanet.Get() : HomePlanet;
	const APlanet* Planet = Cast<APlanet>(Selected);
	if (!IsValid(Planet)) return RadiusCm; // A moon's close-up does not own its parent's orbit.
	double EnvelopeCm = RadiusCm;
	for (const AMoon* Moon : Planet->Moons)
	{
		if (!IsValid(Moon)) continue;
		EnvelopeCm = FMath::Max(EnvelopeCm,
			FVector::Distance(Planet->GetActorLocation(), Moon->GetActorLocation()) + PhysicalBodyRadiusCm(Moon));
	}
	// Read the current model too: edited orbital radii are authoritative even before
	// the deferred presentation refresh has moved a satellite's actor.
	if (const TSharedPtr<FPlanetModel>& Model = Planet->PlanetData.PlanetModel; Model)
	{
		for (const TSharedPtr<FMoonData>& Moon : Model->MoonsList)
		{
			if (!Moon || !Moon->MoonModel || !FMath::IsFinite(Moon->OrbitRadius)) continue;
			const double MoonRadiusCm = FMath::Max(static_cast<double>(Moon->MoonModel->RadiusKM),
				Moon->MoonModel->Radius * 6371.0) * 1.0e5;
			const double OrbitEnvelopeCm = RadiusCm * FMath::Max(1.0 + Moon->OrbitRadius, 1.0) + MoonRadiusCm;
			if (FMath::IsFinite(OrbitEnvelopeCm)) EnvelopeCm = FMath::Max(EnvelopeCm, OrbitEnvelopeCm);
		}
	}
	return EnvelopeCm;
}

bool AAstroGenerator::GetContinuousPreviewZoomLimits(double& MinDistanceCm, double& MaxDistanceCm) const
{
	FVector CenterCm;
	double RadiusCm = 0.0;
	if (!IsValid(PreviewCamera) || !GetContinuousPreviewPhysicalFocus(PreviewFocus, CenterCm, RadiusCm)) return false;
	const double FallbackTangent = FMath::Tan(FMath::DegreesToRadians(PreviewCamera->FieldOfView * 0.5)) * 0.25;
	const double FitTangent = ContinuousPreviewFramingTangent > 0.0 ? ContinuousPreviewFramingTangent : FallbackTangent;
	const double MinRatio = PreviewFocus == EAstroPreviewFocus::HomePlanet ? 1.01
		: PreviewFocus == EAstroPreviewFocus::HomeStar ? 1.2 : 0.01;
	const FAPSPreviewCameraBounds Bounds = FAPSPreviewCameraBounds::Calculate(
		RadiusCm, GetContinuousPreviewFocusEnvelopeCm(), FitTangent, MinRatio);
	MinDistanceCm = Bounds.MinimumCm;
	MaxDistanceCm = Bounds.MaximumCm;
	return true;
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
	ClusterSpacingOffsets.Remove(TWeakObjectPtr<const AAstroGenerator>(this));
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
		if (CVarPreviewViewConservativeBounds.GetValueOnGameThread() != 0)
		{
			// Camera-following transforms must not rescan the complete catalogue for bounds (as the gameplay view).
			View->SetUseConservativeBounds(true);
			View->bAffectDynamicIndirectLighting = false;
			View->bAffectDistanceFieldLighting = false;
			View->bEvaluateWorldPositionOffset = false;
			View->bWorldPositionOffsetWritesVelocity = false;
			View->SetReceivesDecals(false);
		}
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
		APSStellarOpticalSupport::EnsureLayout(View);
		// Rio 08.10 (STAR BRIGHTNESS): a rebuilt view keeps the menu's brightness (1: its material stays untouched).
		ApplyPreviewStarBrightness(View, PreviewStarBrightness);
		return View;
	};
	if (!IsValid(ContinuousGalaxyView) && IsValid(GeneratedGalaxy))
	{
		ContinuousGalaxyView = CreateView(GeneratedGalaxy->StarMeshInstances, TEXT("ContinuousGalaxyView"));
		if (IsValid(ContinuousGalaxyView))
		{
			const FAPSCanonicalStellarProjectionFrame& Frame = GeneratedGalaxy->CanonicalProjectionFrame;
			for (auto It = PointDisplayScales.CreateIterator(); It; ++It)
				if (!It.Key().IsValid()) It.RemoveCurrent();
			TArray<float>& DisplayScales = PointDisplayScales.FindOrAdd(
				TWeakObjectPtr<const UInstancedStaticMeshComponent>(ContinuousGalaxyView.Get()));
			DisplayScales.Init(1.0f, ContinuousGalaxyView->GetInstanceCount());
			for (int32 Index = 0; Index < ContinuousGalaxyView->GetInstanceCount(); ++Index)
			{
				FGalaxyCatalogStarRecord Record;
				if (!GeneratedGalaxy->GetRenderedCatalogRecord(Index, Record)) continue;
				DisplayScales[Index] = PopulationPointScale(Record.RadiusScale);
				FAPSContinuousPreviewPoint& Point = ContinuousGalaxyPoints.AddDefaulted_GetRef();
				Point.CenterCm = Frame.GetCanonicalRootPositionCm(Record.GalaxyLocalLocation) - Frame.CanonicalAnchorCm;
				Point.RadiusCm = APSCanonicalStellarProjection::GetCanonicalStellarRadiusSolar(Record.SpectralClass)
					* APSCanonicalStellarProjection::SolarRadiusCm * Record.RadiusScale; // Rio 03.10: galaxy POPULATION.
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
				// Rio 05.10 (real scale experiment): the same light at real distances (factor 1 when OFF).
				const double VisualRadiusSolar = APSCanonicalStellarProjection::GetLegacyLayoutRadiusSolar(Frame,
					APSCanonicalStellarProjection::UnprojectPhysicalRadiusSolar(Frame, VisualRadiusCm));
				// Primary and companion points use the same existing catalog material
				// convention. Slot creation never depends on whether a system was visited.
				const double Emission = UStarGenerator::GetFarStarVisualEmission(Model.Radius,
					Stars->CalculateEmission(Model.Luminosity * 25.0), VisualRadiusSolar);
				ContinuousClusterView->SetCustomDataValue(Point.SourceInstanceIndex, 0, Color.R, false);
				ContinuousClusterView->SetCustomDataValue(Point.SourceInstanceIndex, 1, Color.G, false);
				ContinuousClusterView->SetCustomDataValue(Point.SourceInstanceIndex, 2, Color.B, false);
				ContinuousClusterView->SetCustomDataValue(Point.SourceInstanceIndex, 3, Emission, false);
				// Companions can be appended after the view's stride upgrade. Newly
				// allocated rows otherwise start with a zero luminosity/core ratio.
				ContinuousClusterView->SetCustomDataValue(Point.SourceInstanceIndex, 6, 1.0f, false);
				ContinuousClusterView->SetCustomDataValue(Point.SourceInstanceIndex,
					APSStellarOpticalSupport::CoreScaleIndex, 1.0f, false);
				if (StarIndex > 0)
					ContinuousClusterView->SetCustomDataValue(Point.SourceInstanceIndex, 4, Point.SourceInstanceIndex * 0.137f, false);
			};
			// Rio 03.10 (dense cores, APSPreviewClusterSpacing.h): presentation-only spacing of the live systems, sized so
			// neighbours stay aps.Preview.ClusterSpacingPixels apart at the CLUSTER framing distance. On by default (Rio 03.10,
			// "everything stuck together" in a giant-rich cluster).
			TMap<int32, FVector>& SpacingOffsets = ClusterSpacingOffsets.FindOrAdd(TWeakObjectPtr<const AAstroGenerator>(this));
			SpacingOffsets.Reset();
			const double SpacingPixels = FMath::Max(static_cast<double>(CVarClusterSpacingPixels.GetValueOnGameThread()), 0.0);
			FVector FramedCenter;
			double FramedRadius = 0.0;
			if (SpacingPixels > 0.0 && IsValid(PreviewCamera)
				&& GetContinuousPreviewPhysicalFocus(EAstroPreviewFocus::StarCluster, FramedCenter, FramedRadius))
			{
				int32 Width = 1280, Height = 720;
				APlayerController* Controller = GetWorld()->GetFirstPlayerController();
				if (Controller) Controller->GetViewportSize(Width, Height);
				const double TanHalf = FMath::Tan(FMath::DegreesToRadians(PreviewCamera->FieldOfView * 0.5));
				const double FitTangent = FMath::Max(ContinuousPreviewFramingTangent > 0.0 ? ContinuousPreviewFramingTangent
					: TanHalf * FMath::Min(0.44, 0.60 * FMath::Max(Height, 1) / FMath::Max(Width, 1)), 0.001);
				const double FrameDistance = FramedRadius * 1.20 * FMath::Sqrt(1.0 + 1.0 / FMath::Square(FitTangent));
				const double Separation = SpacingPixels * FrameDistance
					* APSStellarViewOptics::PixelTangent(Controller, 2.0 * TanHalf / FMath::Max(Width, 320));
				TArray<FVector> Centres;
				TArray<uint8> FixedCentres;
				for (const FClusterStarSystemRecord& Record : GeneratedStarCluster->PotentialStarSystems)
				{
					Centres.Add(Frame.GetCanonicalRootPositionCm(Record.ClusterLocalLocation) - Frame.CanonicalAnchorCm);
					FixedCentres.Add(Record.InstanceIndex == PendingHomeClusterInstanceIndex ? 1 : 0);
				}
				const double SpacingStarted = FPlatformTime::Seconds();
				const TArray<FVector> Spaced = SpaceSystemCentres(Centres, FixedCentres,
					Frame.GetCanonicalRootPositionCm(FVector::ZeroVector) - Frame.CanonicalAnchorCm, Separation);
				for (int32 Index = 0; Index < Spaced.Num(); ++Index)
					if (!Spaced[Index].IsNearlyZero(Separation * 1.0e-3))
						SpacingOffsets.Add(GeneratedStarCluster->PotentialStarSystems[Index].InstanceIndex, Spaced[Index]);
				UE_LOG(LogTemp, Log, TEXT("[APS.Preview.Cluster] Spacing %.1f px (%.3e cm at the CLUSTER frame): %d of %d systems moved apart in %.3fs"),
					SpacingPixels, Separation, SpacingOffsets.Num(), Centres.Num(), FPlatformTime::Seconds() - SpacingStarted);
			}
			for (const FClusterStarSystemRecord& Record : GeneratedStarCluster->PotentialStarSystems)
			{
				const FVector SystemCenter = Frame.GetCanonicalRootPositionCm(Record.ClusterLocalLocation) - Frame.CanonicalAnchorCm
					+ APSPreviewClusterSpacing::GetOffsetCm(this, Record.InstanceIndex);
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
			LimitPointRadiiToNeighbourSpacing(ContinuousClusterPoints);
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
	// Rio 03.10: never prepare more pairs than may ever be shown at once.
	const int32 PairCap = GetResolvedStarCap();
	if (Capacity() >= PairCap) return;
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
		if (CVarParallelResolve.GetValueOnGameThread() != 0 && ContinuousClusterPoints.Num() > 1024)
		{
			// Rio 03.10 04:30: the forecast probe in parallel; materialized stars never need a pair, so they are skipped.
			std::atomic<int32> Shared{0};
			ParallelFor(TEXT("APS.Preview.ResolvedForecast"), ContinuousClusterPoints.Num(), 1024, [&](const int32 Index)
			{
				const FAPSContinuousPreviewPoint& Point = ContinuousClusterPoints[Index];
				if (Shared.load(std::memory_order_relaxed) >= PairCap || !Point.MaterializedStar.IsExplicitlyNull()) return;
				FAPSPreviewProjectedSphere Sphere;
				if (NeedsResolvedStarView(Point, Frame, Rotation, PixelTangent, TanHalfHorizontal, TanHalfVertical, true, Sphere))
					Shared.fetch_add(1, std::memory_order_relaxed);
			});
			Needed = FMath::Min(Shared.load(), PairCap);
		}
		else
		{
			for (const FAPSContinuousPreviewPoint& Point : ContinuousClusterPoints)
			{
				FAPSPreviewProjectedSphere Sphere;
				Needed += NeedsResolvedStarView(Point, Frame, Rotation, PixelTangent,
					TanHalfHorizontal, TanHalfVertical, true, Sphere);
				if (Needed >= PairCap) break;
			}
		}
		ContinuousResolvedForecastCapacity = FMath::Min(PairCap,
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
	// Rio 04.10 evening ("stars still spawn inside the system: hide the ones inside its sphere"): the catalogue points inside
	// the home system's extent were hidden (ApplyContinuousPreviewFrame), but a near one still resolved into a photosphere
	// pair there. The same sphere: the farthest home body plus its radius, with the system exclusion padding.
	FVector HomeCenterCm = FVector::ZeroVector;
	double HomeRadiusCm = 0.0;
	const FGuid HomeId = IsValid(GeneratedHomeStarSystem) ? GeneratedHomeStarSystem->StableSystemId : FGuid();
	if (IsValid(GeneratedHomeStarSystem))
	{
		HomeCenterCm = GetContinuousPreviewSystemCenter(GeneratedHomeStarSystem);
		for (const TWeakObjectPtr<AActor>& WeakBody : ContinuousPreviewBodies)
		{
			const AActor* Body = WeakBody.Get();
			if (!IsValid(Body) || GetContinuousPreviewOwningSystem(Body) != GeneratedHomeStarSystem) continue;
			HomeRadiusCm = FMath::Max(HomeRadiusCm,
				(Body->GetActorLocation() - GeneratedHomeStarSystem->GetActorLocation()).Size() + PhysicalBodyRadiusCm(Body));
		}
		HomeRadiusCm *= APSCanonicalStellarProjection::SystemProxyExclusionPadding;
	}
	const auto InsideHome = [&HomeCenterCm, HomeRadiusCm, &HomeId](const FAPSContinuousPreviewPoint& Point)
	{
		return HomeRadiusCm > 0.0 && Point.StableId != HomeId
			&& FVector::DistSquared(Point.CenterCm, HomeCenterCm) <= FMath::Square(HomeRadiusCm + Point.RadiusCm);
	};
	// Rio 03.10 04:30 (fps while moving): the test runs in parallel over every cluster point; only its candidates and the
	// few materialized stars are tested again here, in catalogue order, so the result equals the serial loop.
	TArray<uint8> Candidates;
	if (CVarParallelResolve.GetValueOnGameThread() != 0 && ContinuousClusterPoints.Num() > 1024)
	{
		Candidates.SetNumZeroed(ContinuousClusterPoints.Num());
		const FAPSContinuousPreviewFrame Frame = ContinuousPreviewFrame;
		ParallelFor(TEXT("APS.Preview.ResolvedTest"), ContinuousClusterPoints.Num(), 1024, [&](const int32 Index)
		{
			const FAPSContinuousPreviewPoint& Point = ContinuousClusterPoints[Index];
			if (!Point.MaterializedStar.IsExplicitlyNull()) { Candidates[Index] = 1; return; }
			if (InsideHome(Point)) return;
			FAPSPreviewProjectedSphere Sphere;
			Candidates[Index] = NeedsResolvedStarView(Point, Frame, CameraRotation, PixelTangent, TanHalfHorizontal,
				TanHalfVertical, ContinuousResolvedStarViews.Contains(Point.SourceInstanceIndex), Sphere) ? 1 : 0;
		});
	}
	for (int32 Index = 0; Index < ContinuousClusterPoints.Num(); ++Index)
	{
		if (!Candidates.IsEmpty() && !Candidates[Index]) continue;
		const FAPSContinuousPreviewPoint& Point = ContinuousClusterPoints[Index];
		if (InsideHome(Point)) continue;
		FAPSPreviewProjectedSphere Sphere;
		if (!NeedsResolvedStarView(Point, ContinuousPreviewFrame, CameraRotation, PixelTangent,
			TanHalfHorizontal, TanHalfVertical, ContinuousResolvedStarViews.Contains(Point.SourceInstanceIndex), Sphere)) continue;
		Required.Emplace(&Point, Sphere);
		RequiredIndices.Add(Point.SourceInstanceIndex);
	}
	// Rio 03.10 (crash + ~10 fps among giants): keep only the largest on screen. A star that already has a pair keeps it
	// unless another one is clearly larger, so stars near the cap do not flicker between point and sphere.
	const int32 PairCap = GetResolvedStarCap();
	if (Required.Num() > FMath::Min(PairCap, ResolvedStarRevealsPerFrame))
	{
		TArray<TPair<double, int32>> Ranked;
		Ranked.Reserve(Required.Num());
		for (int32 Index = 0; Index < Required.Num(); ++Index)
		{
			const FAPSPreviewProjectedSphere& Sphere = Required[Index].Value;
			const double Pixels = Sphere.Radius / FMath::Max(Sphere.Center.Size() * PixelTangent, 1.0e-12);
			Ranked.Emplace(ContinuousResolvedStarViews.Contains(Required[Index].Key->SourceInstanceIndex)
				? Pixels * 1.25 : Pixels, Index);
		}
		Ranked.Sort([](const TPair<double, int32>& A, const TPair<double, int32>& B) { return A.Key > B.Key; });
		TArray<TPair<const FAPSContinuousPreviewPoint*, FAPSPreviewProjectedSphere>> Kept;
		const int32 KeptCount = FMath::Min(PairCap, Required.Num());
		Kept.Reserve(KeptCount);
		RequiredIndices.Reset();
		for (int32 Index = 0; Index < KeptCount; ++Index)
		{
			Kept.Add(Required[Ranked[Index].Value]);
			RequiredIndices.Add(Kept.Last().Key->SourceInstanceIndex);
		}
		Required = MoveTemp(Kept);
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
	int32 Reveals = 0;
	const double RevealStarted = FPlatformTime::Seconds();
	for (const auto& Item : Required)
	{
		const FAPSContinuousPreviewPoint& Point = *Item.Key;
		const FAPSPreviewProjectedSphere& Sphere = Item.Value;
		FAPSContinuousResolvedStarView* View = ContinuousResolvedStarViews.Find(Point.SourceInstanceIndex);
		if (!View)
		{
			// Rio 03.10 ("apply 3617 ms: resolved 3603"): a bounded number of new pairs per frame, largest first; the
			// rest stay optical points of the same size and resolve over the next frames.
			if (Reveals >= ResolvedStarRevealsPerFrame
				|| (Reveals >= 4 && FPlatformTime::Seconds() - RevealStarted > ResolvedStarRevealSeconds)) continue;
			++Reveals;
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
			// A neighbour star is a far disc: it keeps only a share of the close-up surface detail.
			if (const float Detail = FMath::Clamp(CVarFarStarDetail.GetValueOnGameThread(), 0.0f, 1.0f); Detail < 1.0f)
			{
				for (const TCHAR* Name : {TEXT("SurfaceVariation"), TEXT("GranulationStrength"), TEXT("SpotStrength")})
				{
					float Value = 0.0f;
					if (SurfaceMaterial->GetScalarParameterValue(FMaterialParameterInfo(Name), Value))
					{
						SurfaceMaterial->SetScalarParameterValue(Name, Value * Detail);
					}
				}
			}
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
	const double TimeStart = FPlatformTime::Seconds();
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
	APlayerController* Controller = GetWorld()->GetFirstPlayerController();
	if (Controller) Controller->GetViewportSize(ViewWidth, ViewHeight);
	// Optical PSF is explicitly separate from physical photosphere size. The existing
	// Gaussian material's conservative sphere contains a ~0.2-radius bright core;
	// a two-pixel support therefore produces a subpixel core, not a two-pixel disc.
	const double PixelTangent = APSStellarViewOptics::PixelTangent(Controller,
		2.0 * FMath::Tan(FMath::DegreesToRadians(PreviewCamera->FieldOfView * 0.5))
			/ FMath::Max(ViewWidth, 320));
	TrimContinuousPreviewSystemCache(PixelTangent);
	PresentContinuousResolvedStars(PixelTangent);
	const double TimeResolved = FPlatformTime::Seconds();
	// Rio 03.10 ("after every change ... if cluster stars fall inside the system, remove them too"): catalogue points
	// inside the home system's current extent are hidden. Measured from the live bodies every applied frame, so
	// planet, orbit and star-size edits and REGENERATE all update it; the home's own stars are drawn by their actors.
	FVector HomeExclusionCenterCm = FVector::ZeroVector;
	double HomeExclusionRadiusCm = 0.0;
	if (IsValid(GeneratedHomeStarSystem))
	{
		HomeExclusionCenterCm = GetContinuousPreviewSystemCenter(GeneratedHomeStarSystem);
		for (const TWeakObjectPtr<AActor>& WeakBody : ContinuousPreviewBodies)
		{
			const AActor* Body = WeakBody.Get();
			if (!IsValid(Body) || GetContinuousPreviewOwningSystem(Body) != GeneratedHomeStarSystem) continue;
			HomeExclusionRadiusCm = FMath::Max(HomeExclusionRadiusCm,
				(Body->GetActorLocation() - GeneratedHomeStarSystem->GetActorLocation()).Size() + PhysicalBodyRadiusCm(Body));
		}
		HomeExclusionRadiusCm *= APSCanonicalStellarProjection::SystemProxyExclusionPadding;
	}
	const FGuid HomeSystemId = IsValid(GeneratedHomeStarSystem) ? GeneratedHomeStarSystem->StableSystemId : FGuid();
	// Rio 03.10 04:30 (fps while moving): the fast path's inputs, read once on the game thread for the parallel pass.
	const bool bFastCatalog = CVarFastCatalog.GetValueOnGameThread() != 0;
	const bool bCatalogCull = bFastCatalog && CVarCatalogCull.GetValueOnGameThread() != 0;
	// Both paths: since the 48-pair cap, stars beyond it are drawn as points and must not flood the screen.
	const double MaxPointPixels = FMath::Max(static_cast<double>(CVarMaxPointPixels.GetValueOnGameThread()), 0.0);
	double TanHalfHorizontal = 1.0;
	double TanHalfVertical = 1.0;
	GetPreviewViewTangents(Controller, PreviewCamera->FieldOfView, ViewWidth, ViewHeight, TanHalfHorizontal, TanHalfVertical);
	const FQuat CameraRotation = PreviewCamera->GetComponentQuat();
	const int32 RayRule = APSStellarOpticalSupport::RayRuleSetting();
	const double RayBrightness = APSStellarOpticalSupport::RayBrightnessSetting();
	const double RaySize = APSStellarOpticalSupport::RaySizeSetting();
	const auto PresentCatalog = [this, PixelTangent, HomeExclusionCenterCm, HomeExclusionRadiusCm, HomeSystemId,
		bFastCatalog, bCatalogCull, MaxPointPixels, TanHalfHorizontal, TanHalfVertical, CameraRotation,
		RayRule, RayBrightness, RaySize](
		UInstancedStaticMeshComponent* View, const TArray<FAPSContinuousPreviewPoint>& Points)
	{
		if (!IsValid(View) || !IsValid(View->GetStaticMesh())) return;
		if (!APSStellarOpticalSupport::EnsureLayout(View)) return;
		// Rio 03.10: the galaxy POPULATION's point sizes (PopulationPointScale), null for the cluster view.
		const TArray<float>* DisplayScales = PointDisplayScales.Find(TWeakObjectPtr<const UInstancedStaticMeshComponent>(View));
		APSPreviewCatalogDelta::FFrameKey Key;
		Key.Exclusion = FVector4(HomeExclusionCenterCm, HomeExclusionRadiusCm);
		Key.Observer = ContinuousPreviewFrame.ObserverCm;
		Key.Scale = ContinuousPreviewFrame.RenderCmPerPhysicalCm;
		Key.PixelTangent = PixelTangent;
		for (const auto& Pair : ContinuousResolvedStarViews)
			Key.Resolved += (uint64(uint32(Pair.Key)) + 1) * 0x9E3779B97F4A7C15ull;
		for (const auto& Pair : ContinuousMaterializedSystems)
			Key.Materialized += (uint64(uint32(Pair.Key)) + 1) * 0xC2B2AE3D27D4EB4Full + (IsValid(Pair.Value) ? 1 : 0);
		Key.Points = Points.Num();
		Key.Instances = View->GetInstanceCount();
		Key.Optics = APSStellarOpticalSupport::RayRuleSetting() + 10.0 * APSStellarOpticalSupport::RayBrightnessSetting()
			+ 1000.0 * APSStellarOpticalSupport::RaySizeSetting();
		Key.ViewRotation = bCatalogCull ? CameraRotation : FQuat::Identity;
		Key.ViewTangents = bCatalogCull ? FVector2D(TanHalfHorizontal, TanHalfVertical) : FVector2D::ZeroVector;
		Key.Policy = (bFastCatalog ? 1.0 : 0.0) + (bCatalogCull ? 2.0 : 0.0) + 4.0 * MaxPointPixels;
		if (CVarCatalogDeltaUpload.GetValueOnGameThread() != 0)
		{
			APSPreviewCatalogDelta::FFrameKey& LastKey = APSPreviewCatalogDelta::Presented.FindOrAdd(View);
			if (LastKey == Key)
			{
				View->SetVisibility(true, false);
				View->SetHiddenInGame(false, false);
				return;
			}
			LastKey = Key;
		}
		const double MeshRadius = FMath::Max(View->GetStaticMesh()->GetBounds().BoxExtent.GetMax(), 0.001);
		int32 Excluded = 0;
		int32 Sent = 0;
		if (bFastCatalog && View->GetComponentTransform().Equals(FTransform::Identity, 1.0e-6)
			&& View->PerInstanceSMData.Num() == View->GetInstanceCount())
		{
			// Rio 03.10 04:30 (30-40 fps while orbiting, galaxy 50k at 32 ms): one parallel pass computes every instance's
			// matrix, optics and change flag; the game thread then sends only the changed rows as precomputed matrices.
			// Points outside the view are zero-scaled and stay unsent until they return. Materialized stars (a handful)
			// keep the serial evaluation because they read live actors. Optics equal the serial path below.
			using namespace APSPreviewCatalogFast;
			APSPreviewCatalogDelta::Sent.Remove(View);
			for (auto It = States.CreateIterator(); It; ++It)
				if (!It.Key().IsValid()) It.RemoveCurrent();
			FState& State = States.FindOrAdd(View);
			const int32 Count = View->GetInstanceCount();
			if (State.PointsData != Points.GetData() || State.PointsNum != Points.Num() || State.InstanceToPoint.Num() != Count)
			{
				State.InstanceToPoint.Init(INDEX_NONE, Count);
				for (int32 PointIndex = 0; PointIndex < Points.Num(); ++PointIndex)
					if (State.InstanceToPoint.IsValidIndex(Points[PointIndex].SourceInstanceIndex))
						State.InstanceToPoint[Points[PointIndex].SourceInstanceIndex] = PointIndex;
				State.PointsData = Points.GetData();
				State.PointsNum = Points.Num();
				State.Last.Reset();
			}
			const bool bFull = State.Last.Num() != Count;
			State.Pending.SetNumUninitialized(Count);
			State.Next.SetNumUninitialized(Count);
			State.Core.SetNumUninitialized(Count);
			State.Ray.SetNumUninitialized(Count);
			State.Flags.SetNumUninitialized(Count);
			TBitArray<> Handoff(false, Count);
			if (View == ContinuousClusterView)
				for (const auto& Pair : ContinuousResolvedStarViews)
					if (Handoff.IsValidIndex(Pair.Key) && IsValid(Pair.Value.Photosphere.Get())) Handoff[Pair.Key] = true;
			const FAPSContinuousPreviewFrame Frame = ContinuousPreviewFrame;
			const int32 Stride = View->NumCustomDataFloats;
			const TArray<float>& CustomData = View->PerInstanceSMCustomData;
			const FMatrix ZeroMatrix(FPlane(0.0, 0.0, 0.0, 0.0), FPlane(0.0, 0.0, 0.0, 0.0),
				FPlane(0.0, 0.0, 0.0, 0.0), FPlane(0.0, 0.0, 0.0, 1.0));
			// bSerial: the game-thread pass for materialized stars, which also reads the live actor's sphere.
			const auto Evaluate = [&](const int32 Index, const bool bSerial) -> uint8
			{
				uint8 Flags = 0;
				FVector4 Out(0.0, 0.0, 0.0, 0.0);
				const int32 PointIndex = State.InstanceToPoint[Index];
				if (PointIndex != INDEX_NONE)
				{
					const FAPSContinuousPreviewPoint& Point = Points[PointIndex];
					if (!bSerial && !Point.MaterializedStar.IsExplicitlyNull()) return DeferredBit;
					FAPSPreviewProjectedSphere Sphere;
					if (HomeExclusionRadiusCm > 0.0 && Point.StableId != HomeSystemId
						&& FVector::DistSquared(Point.CenterCm, HomeExclusionCenterCm)
							<= FMath::Square(HomeExclusionRadiusCm + Point.RadiusCm))
					{
						Flags |= ExcludedBit; // Stays zero-scaled: a foreign star never sits inside the home system.
					}
					else if (Frame.ProjectSphere(Point.CenterCm, Point.RadiusCm, Sphere))
					{
						double PixelWorldRadius = Sphere.Center.Size() * PixelTangent;
						AStar* MaterializedStar = bSerial ? Point.MaterializedStar.Get() : nullptr;
						const bool bMaterialized = IsValid(MaterializedStar);
						if (bMaterialized || !bCatalogCull || APSPreviewVisibility::SphereIntersectsView(
							CameraRotation.UnrotateVector(Sphere.Center), Sphere.Radius * 1.14 + PixelWorldRadius * 32.0,
							TanHalfHorizontal, TanHalfVertical))
						{
							const auto Profile = APSStellarOpticalSupport::Select(CustomData, Stride, Index,
								RayRule, RayBrightness, RaySize);
							double CoreRadius = APSStellarOpticalSupport::CoreRadius(Sphere.Radius, PixelWorldRadius);
							double Radius = APSStellarOpticalSupport::CarrierRadius(Sphere.Radius, PixelWorldRadius, Profile);
							float RayStrength = APSStellarOpticalSupport::ResolvedRayStrength(Profile, Sphere.Radius, PixelWorldRadius);
							if (DisplayScales && DisplayScales->IsValidIndex(Index))
							{
								CoreRadius *= (*DisplayScales)[Index];
								Radius *= (*DisplayScales)[Index];
							}
							if (bMaterialized)
								Frame.ProjectSphere(GetContinuousPreviewPhysicalPosition(MaterializedStar),
									PhysicalBodyRadiusCm(MaterializedStar), Sphere);
							if (bMaterialized || Handoff[Index])
							{
								PixelWorldRadius = Sphere.Center.Size() * PixelTangent;
								const double PixelRadius = Sphere.Radius / FMath::Max(PixelWorldRadius, 1.0e-12);
								const double PointWeight = 1.0 - FMath::Clamp((PixelRadius - 0.3) / 0.7, 0.0, 1.0);
								CoreRadius = PixelWorldRadius * APSStellarOpticalSupport::CompactSupportPixels * PointWeight;
								Radius = PixelWorldRadius * Profile.SupportPixels * PointWeight;
								RayStrength *= float(PointWeight);
							}
							else if (MaxPointPixels > 0.0 && Radius > PixelWorldRadius * MaxPointPixels)
							{
								// A star beyond the resolved-star cap: its point never floods the screen.
								const double Shrink = PixelWorldRadius * MaxPointPixels / Radius;
								CoreRadius *= Shrink;
								Radius *= Shrink;
							}
							const float CoreScale = APSStellarOpticalSupport::CoreScale(CoreRadius, Radius);
							State.Core[Index] = CoreScale;
							State.Ray[Index] = RayStrength;
							const int64 Address = int64(Index) * Stride;
							if (Address + APSStellarOpticalSupport::RayStrengthIndex < CustomData.Num()
								&& (!FMath::IsNearlyEqual(CustomData[Address + APSStellarOpticalSupport::CoreScaleIndex], CoreScale, 1.0e-5f)
									|| !FMath::IsNearlyEqual(CustomData[Address + APSStellarOpticalSupport::RayStrengthIndex], RayStrength, 1.0e-5f)))
								Flags |= OpticsBit;
							Out = FVector4(Sphere.Center, Radius / MeshRadius);
						}
					}
				}
				State.Next[Index] = Out;
				State.Pending[Index].Transform = Out.W != 0.0 ? PointMatrix(Out) : ZeroMatrix;
				if (bFull || HasChanged(State.Last[Index], Out, PixelTangent)) Flags |= MovedBit;
				return Flags;
			};
			ParallelFor(TEXT("APS.Preview.CatalogPoints"), Count, 1024,
				[&State, &Evaluate](const int32 Index) { State.Flags[Index] = Evaluate(Index, false); });
			int32 MovedCount = 0;
			for (int32 Index = 0; Index < Count; ++Index)
			{
				uint8& Flags = State.Flags[Index];
				if (Flags & DeferredBit) Flags = Evaluate(Index, true);
				if (Flags & ExcludedBit) ++Excluded;
				if (Flags & OpticsBit)
				{
					View->SetCustomDataValue(Index, APSStellarOpticalSupport::CoreScaleIndex, State.Core[Index], false);
					View->SetCustomDataValue(Index, APSStellarOpticalSupport::RayStrengthIndex, State.Ray[Index], false);
				}
				MovedCount += (Flags & MovedBit) ? 1 : 0;
			}
			if (Count > 0 && (bFull || MovedCount * 4 > Count))
			{
				// While the camera orbits nearly every visible point moves: one call for every row.
				View->BatchUpdateInstancesData(0, Count, State.Pending.GetData(), false, true);
				Swap(State.Last, State.Next);
				Sent = Count;
			}
			else
			{
				for (int32 Index = 0; Index < Count;)
				{
					if (!(State.Flags[Index] & MovedBit)) { ++Index; continue; }
					int32 End = Index + 1;
					while (End < Count && (State.Flags[End] & MovedBit)) ++End;
					View->BatchUpdateInstancesData(Index, End - Index, State.Pending.GetData() + Index, false, true);
					for (int32 Run = Index; Run < End; ++Run) State.Last[Run] = State.Next[Run];
					Index = End;
				}
				Sent = MovedCount;
			}
		}
		else
		{
		APSPreviewCatalogFast::States.Remove(View);
		TArray<FTransform> Transforms;
		Transforms.Init(FTransform(FQuat::Identity, FVector::ZeroVector, FVector::ZeroVector), View->GetInstanceCount());
		for (const FAPSContinuousPreviewPoint& Point : Points)
		{
			if (!Transforms.IsValidIndex(Point.SourceInstanceIndex)) continue;
			if (HomeExclusionRadiusCm > 0.0 && Point.StableId != HomeSystemId
				&& FVector::Distance(Point.CenterCm, HomeExclusionCenterCm) - Point.RadiusCm <= HomeExclusionRadiusCm)
			{
				++Excluded; // Stays zero-scaled: a foreign star never sits inside the home system.
				continue;
			}
			FAPSPreviewProjectedSphere Sphere;
			if (!ContinuousPreviewFrame.ProjectSphere(Point.CenterCm, Point.RadiusCm, Sphere)) continue;
			const auto Profile = APSStellarOpticalSupport::Select(View->PerInstanceSMCustomData,
				View->NumCustomDataFloats, Point.SourceInstanceIndex);
			double PixelWorldRadius = Sphere.Center.Size() * PixelTangent;
			double CoreRadius = APSStellarOpticalSupport::CoreRadius(Sphere.Radius, PixelWorldRadius);
			double Radius = APSStellarOpticalSupport::CarrierRadius(Sphere.Radius, PixelWorldRadius, Profile);
			float RayStrength = APSStellarOpticalSupport::ResolvedRayStrength(Profile, Sphere.Radius, PixelWorldRadius);
			if (DisplayScales && DisplayScales->IsValidIndex(Point.SourceInstanceIndex))
			{
				CoreRadius *= (*DisplayScales)[Point.SourceInstanceIndex];
				Radius *= (*DisplayScales)[Point.SourceInstanceIndex];
			}
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
				PixelWorldRadius = Sphere.Center.Size() * PixelTangent;
				CoreRadius = PixelWorldRadius * APSStellarOpticalSupport::CompactSupportPixels * PointWeight;
				Radius = PixelWorldRadius * Profile.SupportPixels * PointWeight;
				RayStrength *= float(PointWeight);
			}
			else if (MaxPointPixels > 0.0 && Radius > PixelWorldRadius * MaxPointPixels)
			{
				// Rio 03.10: a star beyond the resolved-star cap keeps a point of at most MaxPointPixels radius.
				const double Shrink = PixelWorldRadius * MaxPointPixels / Radius;
				CoreRadius *= Shrink;
				Radius *= Shrink;
			}
			APSStellarOpticalSupport::Publish(View, Point.SourceInstanceIndex,
				APSStellarOpticalSupport::CoreScale(CoreRadius, Radius), RayStrength);
			Transforms[Point.SourceInstanceIndex] = FTransform(FQuat::Identity, Sphere.Center, FVector(Radius / MeshRadius));
		}
		// UE 5.4 tracks each changed instance and schedules SendRenderInstanceData.
		// Recreating the entire scene proxy here would discard that incremental path
		// on every flight frame; geometry and instance indices are unchanged.
		Sent = APSPreviewCatalogDelta::Upload(View, Transforms, PixelTangent);
		}
		CSV_CUSTOM_STAT(APSPreview, CatalogPointsSent, Sent, ECsvCustomStatOp::Accumulate);
		View->SetVisibility(true, false);
		View->SetHiddenInGame(false, false);
		// Evidence only when the home extent changes, never per moving frame.
		static TMap<TWeakObjectPtr<UInstancedStaticMeshComponent>, FVector4> LoggedExclusion;
		const FVector4* LastExclusion = LoggedExclusion.Find(View);
		if (!LastExclusion || *LastExclusion != Key.Exclusion)
		{
			LoggedExclusion.Add(View, Key.Exclusion);
			UE_LOG(LogTemp, Log, TEXT("[APS.Preview.Exclusion] %s: %d catalogue stars hidden inside the home system (radius %.3e cm)"),
				*View->GetName(), Excluded, HomeExclusionRadiusCm);
		}
	};
	{
		CSV_SCOPED_TIMING_STAT(APSPreview, GalaxyCatalog);
		PresentCatalog(ContinuousGalaxyView, ContinuousGalaxyPoints);
		// Rio 03.10 (galaxy phase 3): GPU points + glow beyond the ISM prefix; inert while aps.Stars.GpuPoints/GalaxyGlow are 0.
		// Rio 04.10 evening ("the stars stay; only the glow drops on the system, star and planet screens").
		// Rio: "smoothly, exponentially, together with the camera": the share follows the camera's flight to the screen.
		using APSGalaxyGpuStars::EMenuGlowScope;
		const EAstroPreviewFocus GpuFocus = GetCurrentPreviewFocus();
		const float FlightAlpha = bPreviewCameraTransitionActive && PreviewCameraTransitionDuration > 0.0f
			? FMath::Clamp(PreviewCameraTransitionElapsed / PreviewCameraTransitionDuration, 0.0f, 1.0f) : 1.0f;
		APSGalaxyGpuStars::SetMenuGlowScope(GpuFocus == EAstroPreviewFocus::HomePlanet ? EMenuGlowScope::Planet
			: GpuFocus == EAstroPreviewFocus::HomeStar ? EMenuGlowScope::Star
			: GpuFocus == EAstroPreviewFocus::HomeSystem ? EMenuGlowScope::System : EMenuGlowScope::Far, FlightAlpha);
		APSGalaxyGpuStars::PresentContinuousFrame(GeneratedGalaxy, ContinuousPreviewFrame, HomeExclusionCenterCm,
			HomeExclusionRadiusCm);
	}
	const double TimeGalaxy = FPlatformTime::Seconds();
	{
		CSV_SCOPED_TIMING_STAT(APSPreview, ClusterCatalog);
		PresentCatalog(ContinuousClusterView, ContinuousClusterPoints);
	}
	const double TimeCluster = FPlatformTime::Seconds();

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
				// Continuous preview returns before the legacy presentation policy.
				// Its generated emissive sphere is not an occluder for the separate
				// preview directional light: that self-shadow appears as a false
				// circular boundary on PLANET. Only this preview's materialized
				// stars are touched; authored/gameplay stars and body shadows keep
				// their existing policy.
				if (Star->StarMesh->CastShadow)
				{
					Star->StarMesh->SetCastShadow(false);
					UE_LOG(LogTemp, Log, TEXT("[APS.Preview.Continuous] Disabled emitter self-shadow: %s"),
						*Star->GetName());
				}
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
	const double TimeBodies = FPlatformTime::Seconds();
	if (IsValid(GeneratedHomeStarSystem) && IsValid(GeneratedHomeStarSystem->StarSystemZone))
	{
		GeneratedHomeStarSystem->StarSystemZone->SetVisibility(false, false);
		GeneratedHomeStarSystem->StarSystemZone->SetHiddenInGame(true, false);
	}
	SyncPreviewGlobeProxyTransforms();
	SetPreviewGlobeProxyVisible(true);
	const double TimeGlobes = FPlatformTime::Seconds();
	for (const TWeakObjectPtr<AActor>& WeakBody : ContinuousPreviewBodies)
		if (APlanetaryBody* Body = Cast<APlanetaryBody>(WeakBody.Get())) StabilizePreviewAtmosphere(Body);
	const double TimeAtmospheres = FPlatformTime::Seconds();
	HideLegacyPreviewGuideShells();
	SetPreviewGuideShellVisible(PreviewStarInfluenceWireGuide, false);
	SetPreviewGuideShellVisible(PreviewSystemBoundaryWireGuide, false);
	PrepareContinuousResolvedStarPool(PixelTangent);
	// Rio 02.10 (freezes while turning the cluster): name the stage when one apply costs a visible share of a frame.
	static uint64 AppliesFrame = 0;
	static int32 AppliesThisFrame = 0;
	AppliesThisFrame = AppliesFrame == GFrameCounter ? AppliesThisFrame + 1 : 1;
	AppliesFrame = GFrameCounter;
	const double TimeEnd = FPlatformTime::Seconds();
	static double LastSlowApplyLog = 0.0;
	if ((TimeEnd - TimeStart) * 1000.0 > 6.0 && TimeEnd - LastSlowApplyLog > 0.5)
	{
		LastSlowApplyLog = TimeEnd;
		const auto Ms = [](const double From, const double To) { return (To - From) * 1000.0; };
		UE_LOG(LogTemp, Warning, TEXT("[APS.Preview.Slow] apply %.1f ms: resolved %.1f (%d pairs, fast=%d), galaxy %.1f (%d), cluster %.1f (%d), bodies %.1f (%d), globes %.1f, atmospheres %.1f, rest %.1f; applies this frame %d, focus=%s"),
			Ms(TimeStart, TimeEnd), Ms(TimeStart, TimeResolved), ContinuousResolvedStarViews.Num(), bFastCatalog ? 1 : 0,
			Ms(TimeResolved, TimeGalaxy), ContinuousGalaxyPoints.Num(),
			Ms(TimeGalaxy, TimeCluster), ContinuousClusterPoints.Num(), Ms(TimeCluster, TimeBodies), ContinuousPreviewBodies.Num(),
			Ms(TimeBodies, TimeGlobes), Ms(TimeGlobes, TimeAtmospheres), Ms(TimeAtmospheres, TimeEnd), AppliesThisFrame,
			*UEnum::GetValueAsString(PreviewFocus));
	}
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

void AAstroGenerator::SetPreviewStarBrightness(const float Brightness)
{
	const float NewBrightness = FMath::IsFinite(Brightness) ? FMath::Clamp(Brightness, 0.05f, 4.0f) : 1.0f;
	if (NewBrightness == PreviewStarBrightness) return;
	PreviewStarBrightness = NewBrightness;
	ApplyPreviewStarBrightness(ContinuousGalaxyView, PreviewStarBrightness);
	ApplyPreviewStarBrightness(ContinuousClusterView, PreviewStarBrightness);
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
	// Rio 04.10 evening ("GALAXY and CLUSTER leave too much space, 20% closer, but it must fit"): their spheres are
	// catalogue envelopes the stars never fill, so they frame without the margin.
	const double FrameMargin = PreviewFocus == EAstroPreviewFocus::Galaxy || PreviewFocus == EAstroPreviewFocus::StarCluster
		? 1.0 : 1.20;
	const double FrameRatio = FrameMargin * FMath::Sqrt(1.0 + 1.0 / FMath::Square(FitTangent));
	if (DistanceRatio <= 0.0) bContinuousPreviewAutoFraming = true;
	FAPSContinuousPreviewOrbit Target = ContinuousPreviewOrbit;
	Target.CenterCm = CenterCm;
	Target.DistanceCm = RadiusCm * (DistanceRatio > 0.0 ? DistanceRatio : FrameRatio);
	if (DistanceRatio > 0.0)
	{
		double MinimumDistance, MaximumDistance;
		if (GetContinuousPreviewZoomLimits(MinimumDistance, MaximumDistance))
			Target.DistanceCm = FMath::Clamp(Target.DistanceCm, MinimumDistance, MaximumDistance);
	}
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
