#include "APSPlanetSurfacePlacementResolver.h"

#include "APS_ALPHA/Actors/Astro/PlanetaryBody.h"
#include "APS_ALPHA/Generation/PlanetarySurfaceGenerator.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/World.h"
#include "Math/RotationMatrix.h"

DEFINE_LOG_CATEGORY_STATIC(LogAPSPlanetSurfacePlacement, Log, All);

namespace APSPlanetSurfacePlacement
{
	constexpr int32 RingDirections = 12;
	constexpr int32 HeadingCount = 4;
	constexpr int32 GlobalDirections = 96;
	constexpr int32 RouteProbeCount = 9;
	constexpr double RouteHalfWidthCm = 150.0;
	constexpr double TraceHalfLengthCm = 5000.0;

	struct FSample
	{
		FVector2D Offset;
		double HeightCm{0.0};
	};

	struct FMetrics
	{
		double MinHeightCm{TNumericLimits<double>::Max()};
		double MaxHeightCm{-TNumericLimits<double>::Max()};
		double MinDryMarginCm{TNumericLimits<double>::Max()};
		double MaxSlope{0.0};
		bool bFinite{true};
	};

	struct FCandidate
	{
		FVector BaseUp{FVector::ZeroVector};
		FVector PadUp{FVector::ZeroVector};
		FVector Forward{FVector::ZeroVector};
		FVector Right{FVector::ZeroVector};
		FMetrics Base;
		FMetrics Pad;
		FMetrics Route;
		double Score{-TNumericLimits<double>::Max()};
		int32 Ordinal{INDEX_NONE};
		bool bValid{false};
	};

	FVector OffsetDirection(
		const FVector& Up, const FVector& Forward, const FVector& Right,
		const FVector2D OffsetCm, const double RadiusCm)
	{
		return (Up + Forward * (OffsetCm.X / RadiusCm)
			+ Right * (OffsetCm.Y / RadiusCm)).GetSafeNormal();
	}

	double HeightAt(
		AWorldScapeRoot* Root, const FVector& Center, const double RadiusCm,
		const FVector& Direction)
	{
		return Root->GetGroundHeight(Center + Direction * RadiusCm, false);
	}

	FMetrics Measure(
		AWorldScapeRoot* Root, const FVector& Center, const double RadiusCm,
		const FVector& Up, const FVector& Forward, const FVector& Right,
		const TArray<FVector2D>& Offsets, const bool bHasLiquid,
		const double OceanHeightCm)
	{
		FMetrics Result;
		TArray<FSample> Samples;
		Samples.Reserve(Offsets.Num());
		for (const FVector2D Offset : Offsets)
		{
			const double HeightCm = HeightAt(Root, Center, RadiusCm,
				OffsetDirection(Up, Forward, Right, Offset, RadiusCm));
			if (!FMath::IsFinite(HeightCm))
			{
				Result.bFinite = false;
				return Result;
			}
			Samples.Add({Offset, HeightCm});
			Result.MinHeightCm = FMath::Min(Result.MinHeightCm, HeightCm);
			Result.MaxHeightCm = FMath::Max(Result.MaxHeightCm, HeightCm);
			Result.MinDryMarginCm = FMath::Min(Result.MinDryMarginCm,
				bHasLiquid ? HeightCm - OceanHeightCm : TNumericLimits<double>::Max());
		}
		for (int32 A = 0; A < Samples.Num(); ++A)
		{
			for (int32 B = A + 1; B < Samples.Num(); ++B)
			{
				const double DistanceCm = FVector2D::Distance(
					Samples[A].Offset, Samples[B].Offset);
				if (DistanceCm >= 100.0)
				{
					Result.MaxSlope = FMath::Max(Result.MaxSlope,
						FMath::Abs(Samples[A].HeightCm - Samples[B].HeightCm)
						/ DistanceCm);
				}
			}
		}
		return Result;
	}

	TArray<FVector2D> RectangleOffsets(const FVector2D SizeCm)
	{
		const FVector2D Half = SizeCm * 0.5;
		return {FVector2D::ZeroVector,
			{Half.X, Half.Y}, {Half.X, -Half.Y},
			{-Half.X, Half.Y}, {-Half.X, -Half.Y},
			{Half.X, 0.0}, {-Half.X, 0.0},
			{0.0, Half.Y}, {0.0, -Half.Y}};
	}

	TArray<FVector2D> DiscOffsets(const double RadiusCm)
	{
		TArray<FVector2D> Result;
		Result.Reserve(RingDirections + 1);
		Result.Add(FVector2D::ZeroVector);
		for (int32 Index = 0; Index < RingDirections; ++Index)
		{
			const double Angle = UE_TWO_PI * static_cast<double>(Index)
				/ static_cast<double>(RingDirections);
			Result.Add({FMath::Cos(Angle) * RadiusCm,
				FMath::Sin(Angle) * RadiusCm});
		}
		return Result;
	}

	FMetrics MeasureRoute(
		AWorldScapeRoot* Root, const FVector& Center, const double RadiusCm,
		const FVector& Up, const FVector& Forward, const FVector& Right,
		const double StartCm, const double EndCm, const bool bHasLiquid,
		const double OceanHeightCm)
	{
		TArray<FVector2D> Offsets;
		Offsets.Reserve(RouteProbeCount * 3);
		for (int32 Probe = 0; Probe < RouteProbeCount; ++Probe)
		{
			const double Alpha = static_cast<double>(Probe)
				/ static_cast<double>(RouteProbeCount - 1);
			const double Along = FMath::Lerp(StartCm, EndCm, Alpha);
			Offsets.Add({Along, -RouteHalfWidthCm});
			Offsets.Add({Along, 0.0});
			Offsets.Add({Along, RouteHalfWidthCm});
		}
		return Measure(Root, Center, RadiusCm, Up, Forward, Right,
			Offsets, bHasLiquid, OceanHeightCm);
	}

	bool ValidateRequest(
		const FAPSCivilizationFootprintRequest& Request, FString& OutReason)
	{
		if (Request.BaseSizeCm.X <= 0.0 || Request.BaseSizeCm.Y <= 0.0
			|| Request.PadDiameterCm <= 0.0 || Request.SeparationCm <= 0.0)
		{
			OutReason = TEXT("Footprint dimensions must be positive");
			return false;
		}
		const double MinimumSeparation = Request.BaseSizeCm.X * 0.5
			+ Request.PadDiameterCm * 0.5 + 300.0;
		if (Request.SeparationCm <= MinimumSeparation)
		{
			OutReason = TEXT("Base and pad overlap the route corridor");
			return false;
		}
		if (Request.MaximumStructureSlope <= 0.0
			|| Request.MaximumRouteSlope <= 0.0
			|| Request.MinimumDryMarginCm < 0.0
			|| Request.SurfaceClearanceCm < 0.0)
		{
			OutReason = TEXT("Slope or clearance limits are invalid");
			return false;
		}
		return true;
	}

	bool TraceMatchesHeight(
		UWorld* World, const APlanetaryBody* Body,
		const APlanetarySurfaceGenerator* Surface, const FVector& Center,
		const double RadiusCm, const FVector& Direction, const double HeightCm,
		const double ToleranceCm)
	{
		if (!World)
		{
			return false;
		}
		const FVector Expected = Center + Direction * (RadiusCm + HeightCm);
		FCollisionQueryParams Params(SCENE_QUERY_STAT(APSCivilizationFootprint), false);
		Params.AddIgnoredActor(Body);
		Params.AddIgnoredActor(Surface);
		FHitResult Hit;
		return World->LineTraceSingleByChannel(Hit,
			Expected + Direction * TraceHalfLengthCm,
			Expected - Direction * TraceHalfLengthCm, ECC_Visibility, Params)
			&& FVector::Distance(Hit.ImpactPoint, Expected) <= ToleranceCm;
	}

	FName AnchorTag(const int64 PlacementKey, const TCHAR* Role)
	{
		return FName(*FString::Printf(
			TEXT("APS.Surface.Placement.%lld.%s"), PlacementKey, Role));
	}
}

uint32 UAPSPlanetSurfacePlacementResolver::BuildPlacementKey(
	const int32 ManifestSeed, const int32 SurfaceSeed,
	const int32 PlanetTypeValue, const int32 PlanetRadiusKm)
{
	uint32 Hash = HashCombine(GetTypeHash(ManifestSeed), GetTypeHash(SurfaceSeed));
	Hash = HashCombine(Hash, GetTypeHash(PlanetTypeValue));
	Hash = HashCombine(Hash, GetTypeHash(PlanetRadiusKm));
	return Hash != 0 ? Hash : 1u;
}

void UAPSPlanetSurfacePlacementResolver::BuildSeedFrame(
	const uint32 PlacementKey, FVector& OutOutward, FVector& OutForward)
{
	FRandomStream Stream(static_cast<int32>(PlacementKey));
	OutOutward = Stream.VRand().GetSafeNormal(
		UE_DOUBLE_SMALL_NUMBER, FVector::UpVector);
	FVector TangentA;
	FVector TangentB;
	OutOutward.FindBestAxisVectors(TangentA, TangentB);
	const double Heading = Stream.FRandRange(0.0, UE_TWO_PI);
	OutForward = (TangentA * FMath::Cos(Heading)
		+ TangentB * FMath::Sin(Heading)).GetSafeNormal(
			UE_DOUBLE_SMALL_NUMBER, TangentA);
}

bool UAPSPlanetSurfacePlacementResolver::TryResolveCivilizationFootprint(
	APlanetaryBody* HomeBody, const FAPSCivilizationFootprintRequest& Request,
	FAPSCivilizationFootprintResult& OutResult)
{
	using namespace APSPlanetSurfacePlacement;
	OutResult = FAPSCivilizationFootprintResult{};
	if (!IsValid(HomeBody))
	{
		OutResult.FailureReason = TEXT("Home body is invalid");
		return false;
	}
	if (!ValidateRequest(Request, OutResult.FailureReason))
	{
		return false;
	}

	const uint32 PlacementKey = BuildPlacementKey(Request.ManifestSeed,
		HomeBody->WorldScapeSeed, static_cast<int32>(HomeBody->PlanetType),
		HomeBody->PlanetRadiusKM);
	OutResult.PlacementKey = static_cast<int64>(PlacementKey);
	HomeBody->SetWorldScapeStreamingState(EWorldScapeSurfaceState::Active);
	APlanetarySurfaceGenerator* Surface = HomeBody->PlanetaryEnvironmentGenerator;
	AWorldScapeRoot* Root = IsValid(Surface) ? Surface->WorldScapeRootInstance : nullptr;
	if (!IsValid(Surface) || !IsValid(Root)
		|| !Surface->IsSurfaceProfileCurrent(HomeBody)
		|| !IsValid(Root->WorldScapeNoise) || Root->PlanetScale <= 0.0)
	{
		OutResult.FailureReason = TEXT("Canonical WorldScape profile is not ready");
		return false;
	}

	const FVector Center = Root->GetActorLocation();
	const double RadiusCm = Root->PlanetScale;
	const bool bHasLiquid = Surface->ResolvedSurfaceProfile.LiquidType
		!= EAPSPlanetLiquidType::None;
	const double OceanHeightCm = bHasLiquid
		? static_cast<double>(Surface->ResolvedSurfaceProfile.OceanLevel)
			* Root->NoiseIntensity
		: -TNumericLimits<double>::Max();
	FVector PreferredUp;
	FVector SeedForward;
	BuildSeedFrame(PlacementKey, PreferredUp, SeedForward);
	FVector TangentA;
	FVector TangentB;
	PreferredUp.FindBestAxisVectors(TangentA, TangentB);
	const double SeedPhase = FMath::Atan2(
		FVector::DotProduct(SeedForward, TangentB),
		FVector::DotProduct(SeedForward, TangentA));

	TArray<FVector> Directions;
	Directions.Reserve(1 + RingDirections * 5 + GlobalDirections);
	Directions.Add(PreferredUp);
	constexpr double SearchRadiiCm[] = {
		10000.0, 25000.0, 50000.0, 100000.0, 500000.0};
	for (const double SearchRadiusCm : SearchRadiiCm)
	{
		const double Angle = FMath::Clamp(SearchRadiusCm / RadiusCm, 1.0e-6, 0.35);
		for (int32 Index = 0; Index < RingDirections; ++Index)
		{
			const double Azimuth = SeedPhase + UE_TWO_PI * Index / RingDirections;
			const FVector RingTangent = TangentA * FMath::Cos(Azimuth)
				+ TangentB * FMath::Sin(Azimuth);
			Directions.Add((PreferredUp * FMath::Cos(Angle)
				+ RingTangent * FMath::Sin(Angle)).GetSafeNormal());
		}
	}
	const double GoldenAngle = UE_PI * (3.0 - FMath::Sqrt(5.0));
	for (int32 Index = 0; Index < GlobalDirections; ++Index)
	{
		const int32 Rotated = (Index
			+ static_cast<int32>(PlacementKey % GlobalDirections)) % GlobalDirections;
		const double Z = 1.0 - 2.0 * (Rotated + 0.5) / GlobalDirections;
		const double Ring = FMath::Sqrt(FMath::Max(0.0, 1.0 - Z * Z));
		const double Azimuth = SeedPhase + GoldenAngle * Rotated;
		Directions.Add({Ring * FMath::Cos(Azimuth),
			Ring * FMath::Sin(Azimuth), Z});
	}

	const TArray<FVector2D> BaseOffsets = RectangleOffsets(Request.BaseSizeCm);
	const double PadRadiusCm = Request.PadDiameterCm * 0.5;
	const TArray<FVector2D> PadOffsets = DiscOffsets(PadRadiusCm);
	const double RouteStartCm = Request.BaseSizeCm.X * 0.5 + 150.0;
	const double RouteEndCm = Request.SeparationCm - PadRadiusCm - 150.0;
	FCandidate Best;
	for (int32 DirectionIndex = 0; DirectionIndex < Directions.Num(); ++DirectionIndex)
	{
		const FVector BaseUp = Directions[DirectionIndex].GetSafeNormal();
		FVector AxisA;
		FVector AxisB;
		BaseUp.FindBestAxisVectors(AxisA, AxisB);
		for (int32 HeadingIndex = 0; HeadingIndex < HeadingCount; ++HeadingIndex)
		{
			const double Heading = SeedPhase + UE_TWO_PI * HeadingIndex / HeadingCount;
			const FVector Forward = (AxisA * FMath::Cos(Heading)
				+ AxisB * FMath::Sin(Heading)).GetSafeNormal();
			const FVector Right = FVector::CrossProduct(BaseUp, Forward).GetSafeNormal();
			const FVector PadUp = OffsetDirection(BaseUp, Forward, Right,
				{Request.SeparationCm, 0.0}, RadiusCm);
			const FVector PadForward = FVector::VectorPlaneProject(
				Forward, PadUp).GetSafeNormal(UE_DOUBLE_SMALL_NUMBER, Forward);
			const FVector PadRight = FVector::CrossProduct(
				PadUp, PadForward).GetSafeNormal();

			FCandidate Candidate;
			Candidate.BaseUp = BaseUp;
			Candidate.PadUp = PadUp;
			Candidate.Forward = Forward;
			Candidate.Right = Right;
			Candidate.Ordinal = DirectionIndex * HeadingCount + HeadingIndex;
			Candidate.Base = Measure(Root, Center, RadiusCm, BaseUp, Forward, Right,
				BaseOffsets, bHasLiquid, OceanHeightCm);
			Candidate.Pad = Measure(Root, Center, RadiusCm, PadUp, PadForward, PadRight,
				PadOffsets, bHasLiquid, OceanHeightCm);
			Candidate.Route = MeasureRoute(Root, Center, RadiusCm,
				BaseUp, Forward, Right, RouteStartCm, RouteEndCm,
				bHasLiquid, OceanHeightCm);
			if (!Candidate.Base.bFinite || !Candidate.Pad.bFinite
				|| !Candidate.Route.bFinite)
			{
				continue;
			}
			const double DryMargin = FMath::Min3(Candidate.Base.MinDryMarginCm,
				Candidate.Pad.MinDryMarginCm, Candidate.Route.MinDryMarginCm);
			Candidate.bValid = (!bHasLiquid || DryMargin >= Request.MinimumDryMarginCm)
				&& Candidate.Base.MaxSlope <= Request.MaximumStructureSlope
				&& Candidate.Pad.MaxSlope <= Request.MaximumStructureSlope
				&& Candidate.Route.MaxSlope <= Request.MaximumRouteSlope;
			if (!Candidate.bValid)
			{
				continue;
			}
			const double MaxDeviation = FMath::Max3(
				Candidate.Base.MaxHeightCm - Candidate.Base.MinHeightCm,
				Candidate.Pad.MaxHeightCm - Candidate.Pad.MinHeightCm,
				Candidate.Route.MaxHeightCm - Candidate.Route.MinHeightCm);
			Candidate.Score = FVector::DotProduct(BaseUp, PreferredUp) * 4.0
				- FMath::Max3(Candidate.Base.MaxSlope, Candidate.Pad.MaxSlope,
					Candidate.Route.MaxSlope) * 20.0
				- MaxDeviation / 5000.0
				+ (bHasLiquid ? FMath::Clamp(DryMargin / 10000.0, 0.0, 2.0) : 1.0);
			if (Candidate.Score > Best.Score)
			{
				Best = Candidate;
			}
		}
	}
	if (!Best.bValid)
	{
		OutResult.FailureReason = TEXT(
			"No deterministic dry footprint satisfies structure and route slopes");
		return false;
	}

	OutResult.bTerrainResolved = true;
	OutResult.bDry = true;
	OutResult.bSlopeValid = true;
	OutResult.bWalkableRoute = true;
	OutResult.Outward = Best.BaseUp;
	OutResult.Forward = Best.Forward;
	OutResult.Right = Best.Right;
	OutResult.CandidateOrdinal = Best.Ordinal;
	OutResult.MinimumDryMarginCm = bHasLiquid ? FMath::Min3(
		Best.Base.MinDryMarginCm, Best.Pad.MinDryMarginCm,
		Best.Route.MinDryMarginCm) : TNumericLimits<double>::Max();
	OutResult.BaseMaximumSlope = Best.Base.MaxSlope;
	OutResult.PadMaximumSlope = Best.Pad.MaxSlope;
	OutResult.RouteMaximumSlope = Best.Route.MaxSlope;
	OutResult.MaximumTerrainDeviationCm = FMath::Max3(
		Best.Base.MaxHeightCm - Best.Base.MinHeightCm,
		Best.Pad.MaxHeightCm - Best.Pad.MinHeightCm,
		Best.Route.MaxHeightCm - Best.Route.MinHeightCm);

	const FVector PadForward = FVector::VectorPlaneProject(
		Best.Forward, Best.PadUp).GetSafeNormal(
			UE_DOUBLE_SMALL_NUMBER, Best.Forward);
	const FVector BaseLocation = Center + Best.BaseUp
		* (RadiusCm + Best.Base.MaxHeightCm + Request.SurfaceClearanceCm);
	const FVector PadLocation = Center + Best.PadUp
		* (RadiusCm + Best.Pad.MaxHeightCm + Request.SurfaceClearanceCm);
	OutResult.BaseTransform = FTransform(
		FRotationMatrix::MakeFromXZ(Best.Forward, Best.BaseUp).ToQuat(), BaseLocation);
	OutResult.PadTransform = FTransform(
		FRotationMatrix::MakeFromXZ(PadForward, Best.PadUp).ToQuat(), PadLocation);

	UWorldScapeLod* Lod0 = Root->WorldScapeLod.IsValidIndex(0)
		? Root->WorldScapeLod[0] : nullptr;
	OutResult.Lod0AnchorDistanceCm = IsValid(Lod0)
		? FVector::Distance(BaseLocation - Center, Lod0->RelativePosition.ToFVector())
		: TNumericLimits<double>::Max();
	const double Lod0CoverageCm = FMath::Max(
		static_cast<double>(Root->TriangleSize) * Root->LodResolution * 0.5,
		static_cast<double>(Root->TriangleSize));
	OutResult.bLod0Ready = HomeBody->bWorldScapeSurfaceReady
		&& IsValid(Lod0) && IsValid(Lod0->Mesh) && Lod0->Mesh->IsRegistered()
		&& Root->WorldScapeLodInGeneration.Num() == 0;

	const bool bCollisionMeshReady = Root->bGenerateCollision
		&& Root->CollisionLods.ContainsByPredicate([](const UWorldScapeLod* Lod)
		{
			return IsValid(Lod) && IsValid(Lod->Mesh) && Lod->Mesh->IsRegistered()
				&& Lod->Mesh->GetCollisionEnabled() != ECollisionEnabled::NoCollision;
		});
	const double CollisionToleranceCm = FMath::Max(250.0,
		static_cast<double>(Root->CollisionTriangleSize) * 2.0);
	const FVector RouteMidUp = OffsetDirection(Best.BaseUp, Best.Forward, Best.Right,
		{(RouteStartCm + RouteEndCm) * 0.5, 0.0}, RadiusCm);
	const double RouteMidHeight = HeightAt(Root, Center, RadiusCm, RouteMidUp);
	OutResult.bCollisionReady = bCollisionMeshReady
		&& Root->WorldScapeLodInGeneration.Num() == 0
		&& TraceMatchesHeight(HomeBody->GetWorld(), HomeBody, Surface, Center,
			RadiusCm, Best.BaseUp, Best.Base.MaxHeightCm, CollisionToleranceCm)
		&& TraceMatchesHeight(HomeBody->GetWorld(), HomeBody, Surface, Center,
			RadiusCm, Best.PadUp, Best.Pad.MaxHeightCm, CollisionToleranceCm)
		&& TraceMatchesHeight(HomeBody->GetWorld(), HomeBody, Surface, Center,
			RadiusCm, RouteMidUp, RouteMidHeight, CollisionToleranceCm);
	OutResult.bFoliageClearanceApplied = false;
	OutResult.bReadyForMaterialization = OutResult.bTerrainResolved
		&& OutResult.bDry && OutResult.bSlopeValid && OutResult.bWalkableRoute
		&& OutResult.bLod0Ready && OutResult.bCollisionReady;
	if (!OutResult.bReadyForMaterialization)
	{
		OutResult.FailureReason = FString::Printf(
			TEXT("Resolved site is waiting for LOD0/collision (lod0=%d collision=%d)"),
			OutResult.bLod0Ready ? 1 : 0, OutResult.bCollisionReady ? 1 : 0);
		return false;
	}

	UE_LOG(LogAPSPlanetSurfacePlacement, Log,
		TEXT("[APS.Surface.Placement] body=%s key=%lld candidate=%d base=%s pad=%s dryMargin=%.1f slopes=[%.4f,%.4f,%.4f] lod0Distance=%.1f lod0Coverage=%.1f foliageClearance=0"),
		*GetNameSafe(HomeBody), OutResult.PlacementKey, OutResult.CandidateOrdinal,
		*BaseLocation.ToCompactString(), *PadLocation.ToCompactString(),
		OutResult.MinimumDryMarginCm, OutResult.BaseMaximumSlope,
		OutResult.PadMaximumSlope, OutResult.RouteMaximumSlope,
		OutResult.Lod0AnchorDistanceCm, Lod0CoverageCm);
	return true;
}

bool UAPSPlanetSurfacePlacementResolver::RequestPlacementAnchors(
	APlanetaryBody* HomeBody,
	const FAPSCivilizationFootprintResult& ResolvedPlacement)
{
	using namespace APSPlanetSurfacePlacement;
	if (!IsValid(HomeBody) || !ResolvedPlacement.bTerrainResolved
		|| ResolvedPlacement.PlacementKey == 0)
	{
		return false;
	}
	HomeBody->SetWorldScapeStreamingState(EWorldScapeSurfaceState::Active);
	APlanetarySurfaceGenerator* Surface = HomeBody->PlanetaryEnvironmentGenerator;
	AWorldScapeRoot* Root = IsValid(Surface) ? Surface->WorldScapeRootInstance : nullptr;
	UWorld* World = HomeBody->GetWorld();
	if (!IsValid(Root) || !World)
	{
		return false;
	}

	const FVector BaseLocation = ResolvedPlacement.BaseTransform.GetLocation();
	const FVector PadLocation = ResolvedPlacement.PadTransform.GetLocation();
	const FVector Locations[] = {
		BaseLocation, PadLocation, (BaseLocation + PadLocation) * 0.5};
	const TCHAR* Roles[] = {TEXT("Base"), TEXT("Pad"), TEXT("Route")};
	for (int32 RoleIndex = 0; RoleIndex < UE_ARRAY_COUNT(Locations); ++RoleIndex)
	{
		const FName Tag = AnchorTag(ResolvedPlacement.PlacementKey, Roles[RoleIndex]);
		AActor* Anchor = nullptr;
		for (AActor* Existing : Root->CollisionDependantActor)
		{
			if (IsValid(Existing) && Existing->ActorHasTag(Tag))
			{
				Anchor = Existing;
				break;
			}
		}
		if (!IsValid(Anchor))
		{
			FActorSpawnParameters SpawnParameters;
			SpawnParameters.ObjectFlags |= RF_Transient;
			SpawnParameters.SpawnCollisionHandlingOverride =
				ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			Anchor = World->SpawnActor<AActor>(AActor::StaticClass(),
				FTransform::Identity, SpawnParameters);
			if (!IsValid(Anchor))
			{
				ReleasePlacementAnchors(HomeBody, ResolvedPlacement.PlacementKey);
				return false;
			}
			USceneComponent* SceneRoot = NewObject<USceneComponent>(
				Anchor, USceneComponent::StaticClass(), NAME_None, RF_Transient);
			Anchor->SetRootComponent(SceneRoot);
			SceneRoot->RegisterComponent();
			Anchor->Tags.Add(Tag);
			Anchor->SetActorHiddenInGame(true);
			Anchor->SetActorEnableCollision(false);
			Anchor->SetActorTickEnabled(false);
		}
		Anchor->SetActorLocation(Locations[RoleIndex], false, nullptr,
			ETeleportType::TeleportPhysics);
		Root->CollisionDependantActor.AddUnique(Anchor);
	}
	Root->bGenerateCollision = true;
	Root->SetActorTickEnabled(true);
	return true;
}

void UAPSPlanetSurfacePlacementResolver::ReleasePlacementAnchors(
	APlanetaryBody* HomeBody, const int64 PlacementKey)
{
	if (!IsValid(HomeBody) || PlacementKey == 0)
	{
		return;
	}
	APlanetarySurfaceGenerator* Surface = HomeBody->PlanetaryEnvironmentGenerator;
	AWorldScapeRoot* Root = IsValid(Surface) ? Surface->WorldScapeRootInstance : nullptr;
	if (!IsValid(Root))
	{
		return;
	}
	const FString Prefix = FString::Printf(
		TEXT("APS.Surface.Placement.%lld."), PlacementKey);
	for (int32 Index = Root->CollisionDependantActor.Num() - 1; Index >= 0; --Index)
	{
		AActor* Actor = Root->CollisionDependantActor[Index];
		const bool bOwnedAnchor = IsValid(Actor) && Actor->HasAnyFlags(RF_Transient)
			&& Actor->Tags.ContainsByPredicate([&Prefix](const FName Tag)
			{
				return Tag.ToString().StartsWith(Prefix);
			});
		if (bOwnedAnchor)
		{
			Root->CollisionDependantActor.RemoveAtSwap(Index, 1, false);
			Actor->Destroy();
		}
	}
}
