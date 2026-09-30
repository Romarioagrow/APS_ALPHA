#include "APSSpawnPlacementSubsystem.h"

#include "APS_ALPHA/Actors/Astro/PlanetaryBody.h"
#include "APS_ALPHA/Generation/PlanetarySurfaceGenerator.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Math/RotationMatrix.h"

DEFINE_LOG_CATEGORY_STATIC(LogAPSSpawnPlacement, Log, All);

namespace APSSpawnPlacement
{
	/** Directions around the anchor in degrees from its front axis: the sides first, the front last. */
	constexpr double DirectionOrderDegrees[] = {90.0, -90.0, 135.0, -135.0, 45.0, -45.0, 180.0, 112.5, -112.5,
		157.5, -157.5, 67.5, -67.5, 22.5, -22.5, 0.0};
	constexpr int32 RingCount = 4;
	constexpr double EdgeSearchStepCm = 25.0;
	/** Half width of the walk kept free from the anchor to each route target. */
	constexpr double RouteHalfWidthCm = 600.0;
	/** Footprint samples reach this far past the structure's edge. */
	constexpr double FootprintMarginCm = 100.0;
	/** Height pairs closer than this carry no slope signal. */
	constexpr double MinimumSlopeRunCm = 100.0;
	/** The support plane sits this far above the highest footprint sample. */
	constexpr double SupportClearanceCm = 10.0;
	/** The foundation reaches this far below the lowest footprint sample. */
	constexpr double FoundationBuryCm = 40.0;
	/** A boom reaches this far into the station it connects to. */
	constexpr double BoomOverlapCm = 150.0;
	/** An orbital structure closes in until its front face is this far from the station's hull. */
	constexpr double OrbitHullGapCm = 500.0;

	const FName SurfaceNotLoaded(TEXT("APS.Spawn.SurfaceNotLoaded"));
	const FName InvalidRequest(TEXT("APS.Spawn.InvalidRequest"));
	const FName Crowded(TEXT("APS.Spawn.Crowded"));
	const FName TooSteep(TEXT("APS.Spawn.TooSteep"));
	const FName Flooded(TEXT("APS.Spawn.Flooded"));

	/** The plane structures stand in around an anchor, and the anchor's footprint rectangle in its own axes. */
	struct FAnchorFrame
	{
		FVector Origin{FVector::ZeroVector};
		FVector Forward{FVector::ForwardVector};
		FVector Right{FVector::RightVector};
		FVector Up{FVector::UpVector};
		FVector2D RectCenter{FVector2D::ZeroVector};
		FVector2D RectHalf{FVector2D::ZeroVector};
		/** The footprint's middle above the origin along Up: orbital structures ring the station at that height. */
		double MidHeight{0.0};
	};

	struct FCircle
	{
		FVector2D Center{FVector2D::ZeroVector};
		double Radius{0.0};
	};

	FVector2D ToFrame(const FAnchorFrame& Frame, const FVector& WorldLocation)
	{
		const FVector Delta = WorldLocation - Frame.Origin;
		return FVector2D(FVector::DotProduct(Delta, Frame.Forward), FVector::DotProduct(Delta, Frame.Right));
	}

	FVector BoxCorner(const FBox& Box, const int32 Corner)
	{
		return FVector((Corner & 1) ? Box.Max.X : Box.Min.X, (Corner & 2) ? Box.Max.Y : Box.Min.Y,
			(Corner & 4) ? Box.Max.Z : Box.Min.Z);
	}

	bool BuildAnchorFrame(const AActor* Anchor, const FVector& Up, FAnchorFrame& OutFrame)
	{
		const FTransform Transform = Anchor->GetActorTransform();
		OutFrame.Origin = Transform.GetLocation();
		OutFrame.Up = Up.GetSafeNormal();
		OutFrame.Forward = FVector::VectorPlaneProject(Transform.GetUnitAxis(EAxis::X), OutFrame.Up).GetSafeNormal();
		if (OutFrame.Forward.IsNearlyZero())
		{
			OutFrame.Forward = FVector::VectorPlaneProject(Transform.GetUnitAxis(EAxis::Y), OutFrame.Up).GetSafeNormal();
		}
		OutFrame.Right = FVector::CrossProduct(OutFrame.Up, OutFrame.Forward).GetSafeNormal();
		if (OutFrame.Up.IsNearlyZero() || OutFrame.Forward.IsNearlyZero() || OutFrame.Right.IsNearlyZero())
		{
			return false;
		}
		const FBox Local = UAPSSpawnPlacementSubsystem::VisualLocalBounds(Anchor);
		if (!Local.IsValid)
		{
			// An anchor without visible meshes still anchors, as a point.
			return true;
		}
		FBox2D Rect(ForceInit);
		double MinHeight = TNumericLimits<double>::Max();
		double MaxHeight = -TNumericLimits<double>::Max();
		for (int32 Corner = 0; Corner < 8; ++Corner)
		{
			const FVector WorldCorner = Transform.TransformPosition(BoxCorner(Local, Corner));
			Rect += ToFrame(OutFrame, WorldCorner);
			const double Height = FVector::DotProduct(WorldCorner - OutFrame.Origin, OutFrame.Up);
			MinHeight = FMath::Min(MinHeight, Height);
			MaxHeight = FMath::Max(MaxHeight, Height);
		}
		OutFrame.RectCenter = Rect.GetCenter();
		OutFrame.RectHalf = Rect.GetExtent();
		OutFrame.MidHeight = 0.5 * (MinHeight + MaxHeight);
		return true;
	}

	/** An obstacle's footprint as a circle in the anchor plane, from its own visible bounds. */
	FCircle ObstacleCircle(const FAnchorFrame& Frame, const AActor* Obstacle)
	{
		FCircle Circle;
		const FBox Local = UAPSSpawnPlacementSubsystem::VisualLocalBounds(Obstacle);
		if (Local.IsValid)
		{
			const FTransform Transform = Obstacle->GetActorTransform();
			Circle.Center = ToFrame(Frame, Transform.TransformPosition(Local.GetCenter()));
			for (int32 Corner = 0; Corner < 8; ++Corner)
			{
				Circle.Radius = FMath::Max(Circle.Radius, FVector2D::Distance(
					ToFrame(Frame, Transform.TransformPosition(BoxCorner(Local, Corner))), Circle.Center));
			}
			return Circle;
		}
		FVector Origin;
		FVector Extent;
		Obstacle->GetActorBounds(true, Origin, Extent);
		Circle.Center = ToFrame(Frame, Origin);
		Circle.Radius = Extent.Size();
		return Circle;
	}

	void CollectObstacles(const FAPSSpawnRequest& Request, const FAnchorFrame& Frame, TArray<FCircle>& OutCircles,
		TArray<FVector2D>& OutRoutes)
	{
		for (const TObjectPtr<AActor>& Obstacle : Request.Obstacles)
		{
			if (IsValid(Obstacle) && Obstacle != Request.Anchor)
			{
				OutCircles.Add(ObstacleCircle(Frame, Obstacle));
			}
		}
		for (const TObjectPtr<AActor>& Target : Request.RouteTargets)
		{
			if (IsValid(Target))
			{
				OutRoutes.Add(ToFrame(Frame, Target->GetActorLocation()));
			}
		}
	}

	bool IsClear(const FVector2D& Candidate, const double FootprintRadius, const double Spacing,
		const FVector2D& RouteStart, const TArray<FCircle>& Circles, const TArray<FVector2D>& Routes)
	{
		for (const FCircle& Circle : Circles)
		{
			if (FVector2D::Distance(Candidate, Circle.Center) < Circle.Radius + FootprintRadius + Spacing)
			{
				return false;
			}
		}
		for (const FVector2D& Route : Routes)
		{
			if (UAPSSpawnPlacementSubsystem::DistanceToSegment(Candidate, RouteStart, Route)
				< FootprintRadius + RouteHalfWidthCm)
			{
				return false;
			}
		}
		return true;
	}
}

double UAPSSpawnPlacementSubsystem::DistanceToRectangle(const FVector2D& Point, const FVector2D& RectCenter,
	const FVector2D& RectHalfSize)
{
	const FVector2D Delta = (Point - RectCenter).GetAbs() - RectHalfSize;
	return FVector2D(FMath::Max(Delta.X, 0.0), FMath::Max(Delta.Y, 0.0)).Size();
}

double UAPSSpawnPlacementSubsystem::DistanceToSegment(const FVector2D& Point, const FVector2D& Start,
	const FVector2D& End)
{
	const FVector2D Segment = End - Start;
	const double LengthSquared = Segment.SizeSquared();
	const double Alpha = LengthSquared > UE_SMALL_NUMBER
		? FMath::Clamp(FVector2D::DotProduct(Point - Start, Segment) / LengthSquared, 0.0, 1.0)
		: 0.0;
	return FVector2D::Distance(Point, Start + Segment * Alpha);
}

double UAPSSpawnPlacementSubsystem::MeasureGroundGap(const AActor* Structure, APlanetaryBody* Body,
	FBox& OutLocalFootprint, bool& bOutRound) const
{
	OutLocalFootprint = FBox(ForceInit);
	bOutRound = false;
	APlanetarySurfaceGenerator* Surface = IsValid(Body) ? Body->PlanetaryEnvironmentGenerator : nullptr;
	AWorldScapeRoot* Root = IsValid(Surface) ? Surface->WorldScapeRootInstance : nullptr;
	if (!IsValid(Structure) || !IsValid(Root) || !IsValid(Root->WorldScapeNoise) || Root->PlanetScale <= 0.0
		|| !Surface->IsSurfaceProfileCurrent(Body))
	{
		return -1.0;
	}
	// The largest visible part carries the structure: its bottom face is the underside to fill beneath.
	const FTransform Transform = Structure->GetActorTransform();
	const FTransform WorldToActor = Transform.Inverse();
	double LargestArea = 0.0;
	Structure->ForEachComponent<UStaticMeshComponent>(false, [&](const UStaticMeshComponent* Mesh)
	{
		if (!Mesh->IsRegistered() || !Mesh->IsVisible() || !Mesh->GetStaticMesh())
		{
			return;
		}
		const FBox Local = Mesh->CalcBounds(Mesh->GetComponentTransform() * WorldToActor).GetBox();
		const double Area = Local.GetExtent().X * Local.GetExtent().Y;
		if (Area > LargestArea)
		{
			LargestArea = Area;
			OutLocalFootprint = Local;
			bOutRound = Mesh->GetStaticMesh()->GetName().Contains(TEXT("Cylinder"));
		}
	});
	if (!OutLocalFootprint.IsValid)
	{
		return -1.0;
	}
	const FVector Center = Root->GetActorLocation();
	const double RadiusCm = Root->PlanetScale;
	const FVector LocalCenter = OutLocalFootprint.GetCenter();
	const FVector LocalExtent = OutLocalFootprint.GetExtent();
	double Gap = -TNumericLimits<double>::Max();
	const auto Sample = [&](const double U, const double V)
	{
		const FVector World = Transform.TransformPosition(FVector(LocalCenter.X + U * LocalExtent.X,
			LocalCenter.Y + V * LocalExtent.Y, OutLocalFootprint.Min.Z));
		const FVector Direction = (World - Center).GetSafeNormal();
		const double GroundRadius = RadiusCm + Root->GetGroundHeight(Center + Direction * RadiusCm, false);
		if (FMath::IsFinite(GroundRadius))
		{
			Gap = FMath::Max(Gap, (World - Center).Size() - GroundRadius);
		}
	};
	for (int32 I = -2; I <= 2; ++I)
	{
		for (int32 J = -2; J <= 2; ++J)
		{
			const double U = I * 0.5;
			const double V = J * 0.5;
			if (!bOutRound || U * U + V * V <= 1.0001)
			{
				Sample(U, V);
			}
		}
	}
	if (bOutRound)
	{
		for (int32 Index = 0; Index < 8; ++Index)
		{
			const double Angle = UE_TWO_PI * (Index + 0.5) / 8.0;
			Sample(FMath::Cos(Angle), FMath::Sin(Angle));
		}
	}
	return Gap;
}

FBox UAPSSpawnPlacementSubsystem::VisualLocalBounds(const AActor* Actor)
{
	FBox Box(ForceInit);
	if (!IsValid(Actor))
	{
		return Box;
	}
	const FTransform WorldToActor = Actor->GetActorTransform().Inverse();
	Actor->ForEachComponent<UStaticMeshComponent>(false, [&Box, &WorldToActor](const UStaticMeshComponent* Mesh)
	{
		if (Mesh->IsRegistered() && Mesh->IsVisible() && Mesh->GetStaticMesh())
		{
			Box += Mesh->CalcBounds(Mesh->GetComponentTransform() * WorldToActor).GetBox();
		}
	});
	return Box;
}

void UAPSSpawnPlacementSubsystem::BuildRingCandidates(const FVector2D& RectCenter, const FVector2D& RectHalfSize,
	const double FootprintRadiusCm, const double SpacingCm, const double MaximumReachCm, const int32 Seed,
	TArray<FVector2D>& OutCandidates)
{
	using namespace APSSpawnPlacement;
	OutCandidates.Reset();
	constexpr int32 DirectionCount = UE_ARRAY_COUNT(DirectionOrderDegrees);
	const double Clearance = FMath::Max(0.0, SpacingCm) + FMath::Max(0.0, FootprintRadiusCm);
	const double RingStep = FMath::Max(2.0 * FootprintRadiusCm + SpacingCm, 100.0);
	const double SearchLimit = RectHalfSize.Size() + Clearance + EdgeSearchStepCm;
	FVector2D Directions[DirectionCount];
	double EdgeDistances[DirectionCount];
	for (int32 Index = 0; Index < DirectionCount; ++Index)
	{
		const int32 Rotated = ((Index + Seed) % DirectionCount + DirectionCount) % DirectionCount;
		const double Radians = FMath::DegreesToRadians(DirectionOrderDegrees[Rotated]);
		Directions[Index] = FVector2D(FMath::Cos(Radians), FMath::Sin(Radians));
		// Out from the rectangle's centre until the footprint circle clears the rectangle by the spacing.
		double Distance = 0.0;
		while (Distance < SearchLimit
			&& DistanceToRectangle(RectCenter + Directions[Index] * Distance, RectCenter, RectHalfSize) < Clearance)
		{
			Distance += EdgeSearchStepCm;
		}
		EdgeDistances[Index] = Distance;
	}
	for (int32 Ring = 0; Ring < RingCount; ++Ring)
	{
		const double RingOffset = RingStep * Ring;
		if (RingOffset > MaximumReachCm)
		{
			break;
		}
		for (int32 Index = 0; Index < DirectionCount; ++Index)
		{
			OutCandidates.Add(RectCenter + Directions[Index] * (EdgeDistances[Index] + RingOffset));
		}
	}
}

bool UAPSSpawnPlacementSubsystem::ResolvePlacement(const FAPSSpawnRequest& Request,
	FAPSSpawnPlacement& OutPlacement) const
{
	OutPlacement = FAPSSpawnPlacement{};
	if (!IsValid(Request.Anchor) || Request.SizeCm.X <= 0.0 || Request.SizeCm.Y <= 0.0 || Request.SizeCm.Z <= 0.0
		|| Request.SpacingCm < 0.0 || Request.MaximumReachCm < 0.0)
	{
		OutPlacement.FailureCode = APSSpawnPlacement::InvalidRequest;
		return false;
	}
	return Request.Site == EAPSSpawnSite::Surface
		? ResolveSurface(Request, OutPlacement)
		: ResolveOrbit(Request, OutPlacement);
}

bool UAPSSpawnPlacementSubsystem::ResolveSurface(const FAPSSpawnRequest& Request,
	FAPSSpawnPlacement& OutPlacement) const
{
	using namespace APSSpawnPlacement;
	APlanetaryBody* Body = Request.Body;
	APlanetarySurfaceGenerator* Surface = IsValid(Body) ? Body->PlanetaryEnvironmentGenerator : nullptr;
	AWorldScapeRoot* Root = IsValid(Surface) ? Surface->WorldScapeRootInstance : nullptr;
	if (!IsValid(Root) || !IsValid(Root->WorldScapeNoise) || Root->PlanetScale <= 0.0
		|| !Surface->IsSurfaceProfileCurrent(Body))
	{
		OutPlacement.FailureCode = SurfaceNotLoaded;
		return false;
	}
	const FVector Center = Root->GetActorLocation();
	const double RadiusCm = Root->PlanetScale;
	const bool bHasLiquid = Surface->ResolvedSurfaceProfile.LiquidType != EAPSPlanetLiquidType::None;
	const double OceanHeightCm = bHasLiquid
		? static_cast<double>(Surface->ResolvedSurfaceProfile.OceanLevel) * Root->NoiseIntensity
		: -TNumericLimits<double>::Max();
	const FVector AnchorLocation = Request.Anchor->GetActorLocation();
	FAnchorFrame Frame;
	if (!BuildAnchorFrame(Request.Anchor, AnchorLocation - Center, Frame))
	{
		OutPlacement.FailureCode = InvalidRequest;
		return false;
	}
	OutPlacement.bSiteReady = true;

	TArray<FCircle> Circles;
	TArray<FVector2D> Routes;
	CollectObstacles(Request, Frame, Circles, Routes);
	const FVector2D Half(Request.SizeCm.X * 0.5, Request.SizeCm.Y * 0.5);
	const double FootprintRadius = Half.Size();
	TArray<FVector2D> Candidates;
	BuildRingCandidates(Frame.RectCenter, Frame.RectHalf, FootprintRadius, Request.SpacingCm,
		Request.MaximumReachCm, Request.Seed, Candidates);

	const FVector2D SampleHalf = Half + FVector2D(FootprintMarginCm);
	const FVector2D SampleOffsets[] = {
		FVector2D::ZeroVector,
		FVector2D(SampleHalf.X, SampleHalf.Y), FVector2D(SampleHalf.X, -SampleHalf.Y),
		FVector2D(-SampleHalf.X, SampleHalf.Y), FVector2D(-SampleHalf.X, -SampleHalf.Y),
		FVector2D(SampleHalf.X, 0.0), FVector2D(-SampleHalf.X, 0.0),
		FVector2D(0.0, SampleHalf.Y), FVector2D(0.0, -SampleHalf.Y)};
	constexpr int32 SampleCount = UE_ARRAY_COUNT(SampleOffsets);
	int32 Blocked = 0;
	int32 Steep = 0;
	int32 Wet = 0;
	for (const FVector2D& Offset : Candidates)
	{
		++OutPlacement.CandidatesTried;
		if (!IsClear(Offset, FootprintRadius, Request.SpacingCm, Frame.RectCenter, Circles, Routes))
		{
			++Blocked;
			continue;
		}
		const FVector Up = (Frame.Up * RadiusCm + Frame.Forward * Offset.X + Frame.Right * Offset.Y).GetSafeNormal();
		const FVector GroundPoint = Center + Up * RadiusCm;
		// The structure's front faces the anchor.
		FVector Forward = FVector::VectorPlaneProject(AnchorLocation - GroundPoint, Up).GetSafeNormal();
		if (Forward.IsNearlyZero())
		{
			Forward = FVector::VectorPlaneProject(Frame.Forward, Up).GetSafeNormal();
		}
		const FVector Right = FVector::CrossProduct(Up, Forward).GetSafeNormal();

		double Heights[SampleCount];
		double MinHeight = TNumericLimits<double>::Max();
		double MaxHeight = -TNumericLimits<double>::Max();
		bool bFinite = true;
		for (int32 Index = 0; Index < SampleCount; ++Index)
		{
			const FVector Direction = (Up * RadiusCm + Forward * SampleOffsets[Index].X
				+ Right * SampleOffsets[Index].Y).GetSafeNormal();
			Heights[Index] = Root->GetGroundHeight(Center + Direction * RadiusCm, false);
			if (!FMath::IsFinite(Heights[Index]))
			{
				bFinite = false;
				break;
			}
			MinHeight = FMath::Min(MinHeight, Heights[Index]);
			MaxHeight = FMath::Max(MaxHeight, Heights[Index]);
		}
		if (!bFinite)
		{
			++Steep;
			continue;
		}
		double Slope = 0.0;
		for (int32 A = 0; A < SampleCount; ++A)
		{
			for (int32 B = A + 1; B < SampleCount; ++B)
			{
				const double Run = FVector2D::Distance(SampleOffsets[A], SampleOffsets[B]);
				if (Run >= MinimumSlopeRunCm)
				{
					Slope = FMath::Max(Slope, FMath::Abs(Heights[A] - Heights[B]) / Run);
				}
			}
		}
		if (Slope > Request.MaximumSlope)
		{
			++Steep;
			continue;
		}
		if (bHasLiquid && MinHeight - OceanHeightCm < Request.MinimumDryMarginCm)
		{
			++Wet;
			continue;
		}
		OutPlacement.Transform = FTransform(FRotationMatrix::MakeFromXZ(Forward, Up).ToQuat(),
			Center + Up * (RadiusCm + MaxHeight + SupportClearanceCm));
		OutPlacement.AttachParent = Body;
		OutPlacement.Slope = Slope;
		OutPlacement.FoundationDepthCm = MaxHeight - MinHeight + SupportClearanceCm + FoundationBuryCm;
		OutPlacement.bValid = true;
		UE_LOG(LogAPSSpawnPlacement, Log,
			TEXT("[APS.Spawn] surface site beside %s: candidate %d, %.0f m from its centre, slope %.3f, foundation %.0f cm"),
			*GetNameSafe(Request.Anchor), OutPlacement.CandidatesTried,
			FVector2D::Distance(Offset, Frame.RectCenter) * 0.01, Slope, OutPlacement.FoundationDepthCm);
		return true;
	}
	OutPlacement.FailureCode = Steep >= FMath::Max(Blocked, Wet) ? TooSteep : (Wet > Blocked ? Flooded : Crowded);
	UE_LOG(LogAPSSpawnPlacement, Log,
		TEXT("[APS.Spawn] surface: no site beside %s (%d candidates: %d crowded, %d steep, %d flooded)"),
		*GetNameSafe(Request.Anchor), OutPlacement.CandidatesTried, Blocked, Steep, Wet);
	return false;
}

bool UAPSSpawnPlacementSubsystem::ResolveOrbit(const FAPSSpawnRequest& Request,
	FAPSSpawnPlacement& OutPlacement) const
{
	using namespace APSSpawnPlacement;
	FAnchorFrame Frame;
	if (!BuildAnchorFrame(Request.Anchor, Request.Anchor->GetActorUpVector(), Frame))
	{
		OutPlacement.FailureCode = InvalidRequest;
		return false;
	}
	OutPlacement.bSiteReady = true;

	TArray<FCircle> Circles;
	TArray<FVector2D> Routes;
	CollectObstacles(Request, Frame, Circles, Routes);
	const FVector2D Half(Request.SizeCm.X * 0.5, Request.SizeCm.Y * 0.5);
	const double FootprintRadius = Half.Size();
	TArray<FVector2D> Candidates;
	BuildRingCandidates(Frame.RectCenter, Frame.RectHalf, FootprintRadius, Request.SpacingCm,
		Request.MaximumReachCm, Request.Seed, Candidates);
	const FVector AnchorMiddle = Frame.Origin + Frame.Forward * Frame.RectCenter.X
		+ Frame.Right * Frame.RectCenter.Y + Frame.Up * Frame.MidHeight;
	// The first clear candidate whose rays find no hull (a gap between the station's arms) stays a fallback: it hangs
	// on a long boom at the edge of the station's box.
	FAPSSpawnPlacement Fallback;
	FCollisionQueryParams HullQuery(SCENE_QUERY_STAT(APSSpawnOrbitHull), true);
	for (const FVector2D& Offset : Candidates)
	{
		++OutPlacement.CandidatesTried;
		if (!IsClear(Offset, FootprintRadius, Request.SpacingCm, Frame.RectCenter, Circles, Routes))
		{
			continue;
		}
		FVector Middle = Frame.Origin + Frame.Forward * Offset.X + Frame.Right * Offset.Y + Frame.Up * Frame.MidHeight;
		const FVector Forward = (AnchorMiddle - Middle).GetSafeNormal();
		if (Forward.IsNearlyZero())
		{
			continue;
		}
		const FQuat Rotation = FRotationMatrix::MakeFromXZ(Forward, Frame.Up).ToQuat();
		// Rays over the structure's front face toward the station: the nearest hull along the way it would travel.
		const FVector Side = FVector::CrossProduct(Frame.Up, Forward).GetSafeNormal();
		const double Reach = FVector::Distance(Middle, AnchorMiddle);
		double HullDistance = TNumericLimits<double>::Max();
		double CenterHullDistance = TNumericLimits<double>::Max();
		for (const double Across : {0.0, -1.0, 1.0})
		{
			for (const double Rise : {0.0, -0.45, 0.45})
			{
				const FVector Start = Middle + Forward * Half.X + Side * (Across * Half.Y)
					+ Frame.Up * (Rise * Request.SizeCm.Z);
				FHitResult Hit;
				if (Request.Anchor->ActorLineTraceSingle(Hit, Start, Start + Forward * Reach, ECC_Visibility, HullQuery))
				{
					HullDistance = FMath::Min(HullDistance, static_cast<double>(Hit.Distance));
					if (Across == 0.0 && Rise == 0.0)
					{
						CenterHullDistance = Hit.Distance;
					}
				}
			}
		}
		if (HullDistance == TNumericLimits<double>::Max())
		{
			if (!Fallback.bValid)
			{
				const FVector2D Outward = (Offset - Frame.RectCenter).GetSafeNormal();
				const double EdgeFromCenter = FMath::Min(
					FMath::Abs(Outward.X) > UE_SMALL_NUMBER ? Frame.RectHalf.X / FMath::Abs(Outward.X) : UE_BIG_NUMBER,
					FMath::Abs(Outward.Y) > UE_SMALL_NUMBER ? Frame.RectHalf.Y / FMath::Abs(Outward.Y) : UE_BIG_NUMBER);
				Fallback.GapToAnchorCm = FMath::Max(0.0,
					FVector2D::Distance(Offset, Frame.RectCenter) - EdgeFromCenter - Half.X) + BoomOverlapCm;
				Fallback.Transform = FTransform(Rotation, Middle - Frame.Up * (Request.SizeCm.Z * 0.5));
				Fallback.bValid = true;
			}
			continue;
		}
		// Close in on the hull, still clear of everything already bolted on.
		const double Travel = FMath::Max(0.0, HullDistance - OrbitHullGapCm);
		Middle += Forward * Travel;
		if (!IsClear(ToFrame(Frame, Middle), FootprintRadius, Request.SpacingCm, Frame.RectCenter, Circles, Routes))
		{
			continue;
		}
		OutPlacement.GapToAnchorCm = (CenterHullDistance < TNumericLimits<double>::Max()
			? CenterHullDistance - Travel : OrbitHullGapCm) + BoomOverlapCm;
		OutPlacement.Transform = FTransform(Rotation, Middle - Frame.Up * (Request.SizeCm.Z * 0.5));
		OutPlacement.AttachParent = Request.Anchor;
		OutPlacement.bValid = true;
		UE_LOG(LogAPSSpawnPlacement, Log,
			TEXT("[APS.Spawn] orbit site on the hull of %s: candidate %d, closed in %.0f m, boom %.0f m"),
			*GetNameSafe(Request.Anchor), OutPlacement.CandidatesTried, Travel * 0.01,
			OutPlacement.GapToAnchorCm * 0.01);
		return true;
	}
	if (Fallback.bValid)
	{
		const int32 Tried = OutPlacement.CandidatesTried;
		OutPlacement = Fallback;
		OutPlacement.CandidatesTried = Tried;
		OutPlacement.AttachParent = Request.Anchor;
		OutPlacement.bSiteReady = true;
		OutPlacement.bValid = true;
		UE_LOG(LogAPSSpawnPlacement, Log,
			TEXT("[APS.Spawn] orbit site beside %s: no hull along any free approach, boom %.0f m to its box"),
			*GetNameSafe(Request.Anchor), OutPlacement.GapToAnchorCm * 0.01);
		return true;
	}
	OutPlacement.FailureCode = Crowded;
	UE_LOG(LogAPSSpawnPlacement, Log, TEXT("[APS.Spawn] orbit: no room beside %s (%d candidates)"),
		*GetNameSafe(Request.Anchor), OutPlacement.CandidatesTried);
	return false;
}

AActor* UAPSSpawnPlacementSubsystem::SpawnAtPlacement(TSubclassOf<AActor> ActorClass,
	const FAPSSpawnPlacement& Placement, TFunctionRef<void(AActor*)> BeforeFinish) const
{
	UWorld* World = GetWorld();
	if (!World || !ActorClass || !Placement.bValid)
	{
		return nullptr;
	}
	AActor* Actor = World->SpawnActorDeferred<AActor>(ActorClass, Placement.Transform, nullptr, nullptr,
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (!Actor)
	{
		return nullptr;
	}
	BeforeFinish(Actor);
	Actor->FinishSpawning(Placement.Transform);
	if (AActor* Parent = Placement.AttachParent.Get(); IsValid(Parent))
	{
		Actor->AttachToActor(Parent, FAttachmentTransformRules::KeepWorldTransform);
	}
	return Actor;
}
