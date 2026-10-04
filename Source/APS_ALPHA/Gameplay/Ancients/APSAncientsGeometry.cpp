#include "APSAncientsGeometry.h"

#include "Math/RandomStream.h"
#include "Math/RotationMatrix.h"

namespace APSAncientsGeometryLocal
{
	using namespace APSAncients;

	/** UV tiles of 10 m (the Builders' stone carries no texture yet; the UVs keep the mesh valid for one). */
	constexpr double UVTileCm = 1000.0;

	int32 StreamSeed(const uint32 Seed)
	{
		return static_cast<int32>(Seed & 0x7fffffffu);
	}

	FQuat TurnAboutUp(const double Radians)
	{
		return FQuat(FVector::UpVector, Radians);
	}

	FVector HorizontalAxis(const double Heading)
	{
		return FVector(FMath::Cos(Heading), FMath::Sin(Heading), 0.0);
	}

	/** The lowest ground under a rectangle (its centre and corners) in the site's plane. */
	double LowestUnder(const FAPSAncientGround& Ground, const FVector2D& Centre, const FVector2D& Half, const double YawRadians)
	{
		const FVector2D AxisX(FMath::Cos(YawRadians), FMath::Sin(YawRadians));
		const FVector2D AxisY(-AxisX.Y, AxisX.X);
		double Lowest = Ground.Z(Centre.X, Centre.Y);
		for (int32 Corner = 0; Corner < 4; ++Corner)
		{
			const double SignX = (Corner & 1) != 0 ? 1.0 : -1.0;
			const double SignY = (Corner & 2) != 0 ? 1.0 : -1.0;
			const FVector2D Point = Centre + AxisX * (Half.X * SignX) + AxisY * (Half.Y * SignY);
			Lowest = FMath::Min(Lowest, Ground.Z(Point.X, Point.Y));
		}
		return Lowest;
	}

	/** The lowest and highest ground round a circle: its centre and eight points of its rim. */
	void RangeRound(const FAPSAncientGround& Ground, const FVector2D& Centre, const double Radius, double& OutLow, double& OutHigh)
	{
		OutLow = OutHigh = Ground.Z(Centre.X, Centre.Y);
		for (int32 Index = 0; Index < 8; ++Index)
		{
			const double Angle = UE_TWO_PI * Index / 8.0;
			const double Height = Ground.Z(Centre.X + FMath::Cos(Angle) * Radius, Centre.Y + FMath::Sin(Angle) * Radius);
			OutLow = FMath::Min(OutLow, Height);
			OutHigh = FMath::Max(OutHigh, Height);
		}
	}

	/**
	 * A ring of standing stones (the monolith's and the circle's): each seated on its own ground, a few leaning, some
	 * broken short, some fallen outward and lying half in the ground; every third one carries a glyph band on its inner
	 * face. Every random draw happens for every stone, so the shape never depends on the ground. Returns the stones' tops
	 * (zero for fallen ones) for lintels.
	 */
	void StoneRing(FRandomStream& Stream, const FAPSAncientGround& Ground, FAPSAncientShape& Out, const int32 Count,
		const double Radius, const FVector& MinHalf, const FVector& MaxHalf, const double Bury, const float FallenShare,
		const float BrokenShare, const double GlyphScale, TArray<FVector>& OutTops)
	{
		OutTops.Reset();
		for (int32 Index = 0; Index < Count; ++Index)
		{
			const double Angle = UE_TWO_PI * Index / Count + Stream.FRandRange(-0.07, 0.07);
			FVector Half(Stream.FRandRange(MinHalf.X, MaxHalf.X), Stream.FRandRange(MinHalf.Y, MaxHalf.Y),
				Stream.FRandRange(MinHalf.Z, MaxHalf.Z));
			const float State = Stream.FRand();
			const double TiltHeading = Stream.FRandRange(0.0, UE_TWO_PI);
			const double TiltDegrees = Stream.FRandRange(0.0, 6.0);
			const double FallYaw = Stream.FRandRange(-0.4, 0.4);
			const FVector2D Radial(FMath::Cos(Angle), FMath::Sin(Angle));
			const FVector2D Where = Radial * Radius;
			// The thin side (X) faces the centre, so the broad face looks inward.
			const FQuat Facing = TurnAboutUp(Angle);
			if (State < FallenShare + BrokenShare)
			{
				Half.Z *= 0.45;
			}
			if (State < FallenShare)
			{
				// Fallen outward: its long axis along the ground, lying on its broad face, half in the ground.
				const FVector2D Lying = Radial * (Radius + Half.Z * 0.6);
				const FQuat Fallen = TurnAboutUp(Angle + FallYaw) * FQuat(FVector::RightVector, FMath::DegreesToRadians(86.0));
				Out.Stone.Box(FVector(Lying.X, Lying.Y, Ground.Z(Lying.X, Lying.Y) + Half.X * 0.3), Half, Fallen);
				OutTops.Add(FVector::ZeroVector);
				continue;
			}
			const FQuat Turn = FQuat(HorizontalAxis(TiltHeading), FMath::DegreesToRadians(TiltDegrees)) * Facing;
			const double Bottom = LowestUnder(Ground, Where, FVector2D(Half.X, Half.Y), Angle) - Bury;
			const FVector Centre(Where.X, Where.Y, Bottom + Half.Z);
			Out.Stone.Box(Centre, Half, Turn);
			OutTops.Add(Centre + Turn.RotateVector(FVector(0.0, 0.0, Half.Z)));
			if (Index % 3 == 0)
			{
				Out.Glow.Box(Centre + Turn.RotateVector(FVector(-(Half.X + GlyphScale), 0.0, Half.Z * 0.35)),
					FVector(GlyphScale * 1.5, Half.Y * 0.55, Half.Z * 0.05), Turn);
			}
		}
	}

	/**
	 * THE NEEDLE: a leaning obelisk 2.4-4.2 km tall (scaled down for the large variant) on a dais, with a turned capstone,
	 * a bright crown band and a pyramidion, glyph seams down its faces and an eye near the top; a ring of standing stones
	 * round it and a processional path of slabs with small lights leading to its front.
	 */
	void BuildMonolith(const FSiteSpec& Spec, const FAPSAncientGround* Ground, FAPSAncientShape& Out)
	{
		FRandomStream Stream(StreamSeed(Spec.Seed));
		const double Scale = Spec.Scale;
		const double Height = Stream.FRandRange(2400.0, 4200.0) * 100.0 * Scale;
		const double Width = Height * Stream.FRandRange(0.065, 0.085);
		const double Depth = Width * Stream.FRandRange(0.55, 0.72);
		const double LeanDegrees = Stream.FRandRange(1.5, 5.0);
		const double LeanHeading = Stream.FRandRange(0.0, UE_TWO_PI);
		const int32 StoneCount = Stream.RandRange(9, 14);
		const double RingRadius = Width * Stream.FRandRange(3.6, 4.6) + 10000.0 * Scale;
		const double DaisRadius = Width * 2.2;
		Out.StoneColour = FLinearColor(0.016f, 0.017f, 0.021f);
		Out.Metrics.SizeCm = Height;
		Out.Metrics.HeightCm = Height * 1.16;
		Out.Metrics.FootprintCm = RingRadius + 3000.0 * Scale;
		Out.Metrics.NavHeightCm = Out.Metrics.HeightCm + 150000.0;
		Out.Metrics.OnFootCm = DaisRadius + FMath::Max(25000.0 * Scale, 8000.0);
		Out.Metrics.PartCount = StoneCount;
		if (!Ground)
		{
			return;
		}

		const FQuat Lean(HorizontalAxis(LeanHeading + UE_HALF_PI), FMath::DegreesToRadians(LeanDegrees));
		const FQuat Diamond = TurnAboutUp(UE_PI * 0.25);
		const auto Along = [&Lean](const double X, const double Y, const double Z) { return Lean.RotateVector(FVector(X, Y, Z)); };
		// The shaft, from below the lowest ground under it to its top.
		const double Bury = FMath::Max(Height * 0.04, 8000.0 * Scale);
		const double Base = LowestUnder(*Ground, FVector2D::ZeroVector, FVector2D(Width * 0.5, Depth * 0.5), 0.0) - Bury;
		Out.Stone.Box(Along(0.0, 0.0, (Base + Height) * 0.5), FVector(Width * 0.5, Depth * 0.5, (Height - Base) * 0.5), Lean);
		// The capstone turned 45 degrees, the crown band that burns at night, the pyramidion.
		Out.Stone.Box(Along(0.0, 0.0, Height * 1.03), FVector(Width * 0.36, Depth * 0.36, Height * 0.035), Lean * Diamond);
		Out.Beacon.Box(Along(0.0, 0.0, Height * 1.0685), FVector(Width * 0.38, Depth * 0.38, Height * 0.0035), Lean * Diamond);
		Out.Stone.Frustum(Along(0.0, 0.0, Height * 1.117), Lean * Diamond, Depth * 0.36 * UE_SQRT_2, 0.0, Height * 0.045, 4);
		// Glyph seams down both broad faces and the eye near the top of the front face.
		for (const double SideY : {-1.0, 1.0})
		{
			for (const double SideX : {-1.0, 1.0})
			{
				Out.Glow.Box(Along(SideX * Width * 0.22, SideY * Depth * 0.504, Height * 0.47),
					FVector(Width * 0.025, Depth * 0.012, Height * 0.40), Lean);
			}
		}
		Out.Glow.Box(Along(Width * 0.505, 0.0, Height * 0.83), FVector(Width * 0.03, Depth * 0.16, Depth * 0.16),
			Lean * FQuat(FVector::ForwardVector, UE_PI * 0.25));

		// The dais: above the ground everywhere round its rim, half in it.
		double DaisLow = 0.0;
		double DaisHigh = 0.0;
		RangeRound(*Ground, FVector2D::ZeroVector, DaisRadius, DaisLow, DaisHigh);
		const double DaisBottom = DaisLow - 1500.0 * Scale;
		const double DaisTop = FMath::Max3(Ground->Z(0.0, 0.0) + 3000.0 * Scale, DaisBottom + 3000.0 * Scale, DaisHigh + 400.0 * Scale);
		Out.Stone.Frustum(FVector(0.0, 0.0, (DaisTop + DaisBottom) * 0.5), TurnAboutUp(UE_PI / 8.0), DaisRadius, DaisRadius * 0.94,
			(DaisTop - DaisBottom) * 0.5, 8);

		// The ring of standing stones, 50-95 m tall.
		TArray<FVector> Tops;
		StoneRing(Stream, *Ground, Out, StoneCount, RingRadius, FVector(700.0, 1200.0, 5000.0) * Scale,
			FVector(1100.0, 2000.0, 9500.0) * Scale, 1200.0 * Scale, 0.18f, 0.18f, 100.0 * Scale, Tops);

		// The processional path to the front face, every other slab lit on both sides.
		constexpr int32 Slabs = 7;
		for (int32 Index = 0; Index < Slabs; ++Index)
		{
			const double X = FMath::Lerp(RingRadius * 1.35, DaisRadius * 1.08, static_cast<double>(Index) / (Slabs - 1));
			const double SlabZ = Ground->Z(X, 0.0);
			Out.Stone.Box(FVector(X, 0.0, SlabZ - 100.0 * Scale), FVector(1400.0, 2300.0, 300.0) * Scale);
			if (Index % 2 == 0)
			{
				for (const double Side : {-1.0, 1.0})
				{
					Out.Glow.Box(FVector(X, Side * 2800.0 * Scale, SlabZ + 150.0 * Scale), FVector(250.0 * Scale));
				}
			}
		}
	}

	/**
	 * THE BROKEN RING: a ring 3-5 km across standing on its edge, its centre 40-60% of its radius above the ground so its
	 * lowest arc is buried, an arc of four to seven segments broken out and lying beneath the gap, two loose segments
	 * missing elsewhere; footings where it meets the ground, a glowing seam on the inner face of every standing segment, a
	 * beacon on the highest one and an altar under its centre.
	 */
	void BuildBrokenRing(const FSiteSpec& Spec, const FAPSAncientGround* Ground, FAPSAncientShape& Out)
	{
		FRandomStream Stream(StreamSeed(Spec.Seed));
		const double Scale = Spec.Scale;
		const double Radius = Stream.FRandRange(1500.0, 2600.0) * 100.0 * Scale;
		const double Thickness = Radius * 0.075;
		const double Depth = Radius * 0.06;
		const double Rise = Radius * Stream.FRandRange(0.42, 0.60);
		constexpr int32 Segments = 44;
		const int32 BreakStart = Stream.RandRange(4, 12);
		const int32 BreakLength = Stream.RandRange(4, 7);
		const int32 LooseA = Stream.RandRange(0, Segments - 1);
		const int32 LooseB = Stream.RandRange(0, Segments - 1);
		const double Span = FMath::Sqrt(FMath::Max(Radius * Radius - Rise * Rise, 0.0));
		const double AltarRadius = Radius * 0.09;
		Out.StoneColour = FLinearColor(0.34f, 0.35f, 0.37f);
		Out.Metrics.SizeCm = Radius * 2.0;
		Out.Metrics.HeightCm = Rise + Radius + Thickness * 0.5;
		Out.Metrics.FootprintCm = FMath::Max(Span + Thickness * 2.0, Radius * 0.8);
		Out.Metrics.NavHeightCm = Out.Metrics.HeightCm + 150000.0;
		Out.Metrics.OnFootCm = AltarRadius + 25000.0 * Scale;
		Out.Metrics.PartCount = Segments;
		if (!Ground)
		{
			return;
		}

		const double CentreZ = Ground->Z(0.0, 0.0) + Rise;
		const double Chord = 2.0 * Radius * FMath::Sin(UE_PI / Segments) * 1.04;
		const auto IsMissing = [&](const int32 Index)
		{
			return (Index >= BreakStart && Index < BreakStart + BreakLength) || Index == LooseA || Index == LooseB;
		};
		const auto RadialOf = [](const double Theta) { return FVector(FMath::Sin(Theta), 0.0, FMath::Cos(Theta)); };
		int32 TopIndex = INDEX_NONE;
		double TopZ = -TNumericLimits<double>::Max();
		for (int32 Index = 0; Index < Segments; ++Index)
		{
			const double Theta = UE_TWO_PI * (Index + 0.5) / Segments;
			const FVector Radial = RadialOf(Theta);
			const FVector Tangent(FMath::Cos(Theta), 0.0, -FMath::Sin(Theta));
			const FVector Centre = FVector(0.0, 0.0, CentreZ) + Radial * Radius;
			const double Floor = Ground->Z(Centre.X, 0.0);
			// Missing ones, and the lowest arc wholly under the ground.
			if (IsMissing(Index) || Centre.Z + Thickness * 0.5 < Floor - 2000.0 * Scale)
			{
				continue;
			}
			const FQuat Turn = FRotationMatrix::MakeFromXZ(Tangent, Radial).ToQuat();
			Out.Stone.Box(Centre, FVector(Chord * 0.5, Depth * 0.5, Thickness * 0.5), Turn);
			if (Centre.Z - Thickness * 0.5 > Floor)
			{
				Out.Glow.Box(Centre - Radial * (Thickness * 0.5 + 60.0 * Scale), FVector(Chord * 0.42, Depth * 0.14, 80.0 * Scale), Turn);
			}
			if (Centre.Z > TopZ)
			{
				TopZ = Centre.Z;
				TopIndex = Index;
			}
		}
		if (TopIndex != INDEX_NONE)
		{
			const double Theta = UE_TWO_PI * (TopIndex + 0.5) / Segments;
			const FVector Radial = RadialOf(Theta);
			const FVector Tangent(FMath::Cos(Theta), 0.0, -FMath::Sin(Theta));
			Out.Beacon.Box(FVector(0.0, 0.0, CentreZ) + Radial * (Radius + Thickness * 0.5 + 80.0 * Scale),
				FVector(Chord * 0.35, Depth * 0.35, 100.0 * Scale), FRotationMatrix::MakeFromXZ(Tangent, Radial).ToQuat());
		}
		// The fallen arc: each segment of the break lies tumbled beneath its gap, half in the ground.
		for (int32 Index = BreakStart; Index < BreakStart + BreakLength; ++Index)
		{
			const double Theta = UE_TWO_PI * (Index + 0.5) / Segments;
			const double X = Radius * FMath::Sin(Theta) * Stream.FRandRange(0.7, 1.25) + Radius * Stream.FRandRange(-0.08, 0.08);
			const double Y = Stream.FRandRange(-1.0, 1.0) * Depth * 5.0;
			const FQuat Tumble = FRotator(Stream.FRandRange(-25.0, 25.0), Stream.FRandRange(0.0, 360.0),
				Stream.FRandRange(-40.0, 40.0)).Quaternion();
			// A gap that was under the ground anyway leaves nothing to fall.
			if (CentreZ + Radius * FMath::Cos(Theta) + Thickness * 0.5 < Ground->Z(Radius * FMath::Sin(Theta), 0.0))
			{
				continue;
			}
			Out.Stone.Box(FVector(X, Y, Ground->Z(X, Y) + Thickness * 0.2), FVector(Chord * 0.5, Depth * 0.5, Thickness * 0.5), Tumble);
		}
		// Footings where the ring goes into the ground.
		for (const double Side : {-1.0, 1.0})
		{
			const double X = Side * Span;
			Out.Stone.Box(FVector(X, 0.0, Ground->Z(X, 0.0) - Thickness * 0.2), FVector(Thickness * 1.3, Depth * 1.25, Thickness * 0.9));
		}
		// The altar under the ring's centre, its top glowing.
		double AltarLow = 0.0;
		double AltarHigh = 0.0;
		RangeRound(*Ground, FVector2D::ZeroVector, AltarRadius, AltarLow, AltarHigh);
		const double AltarBottom = AltarLow - 1000.0 * Scale;
		const double AltarTop = FMath::Max(AltarHigh + 1500.0 * Scale, AltarBottom + 2500.0 * Scale);
		Out.Stone.Frustum(FVector(0.0, 0.0, (AltarTop + AltarBottom) * 0.5), FQuat::Identity, AltarRadius, AltarRadius * 0.83,
			(AltarTop - AltarBottom) * 0.5, 6);
		Out.Glow.Frustum(FVector(0.0, 0.0, AltarTop + 60.0 * Scale), FQuat::Identity, AltarRadius * 0.55, AltarRadius * 0.55,
			60.0 * Scale, 6);
	}

	/**
	 * THE SPIRE FOREST: 18-30 stepped towers on a jittered grid 2.2-3.4 km wide, taller toward the centre, round one spire
	 * 1.8-2.6 km tall (scaled down for the large variant). Each tower stands in tiers that narrow upward, each on its own
	 * ground, with a pyramid tip; the taller ones carry a glowing ledge, the central one a beacon at its tip.
	 */
	void BuildSpireField(const FSiteSpec& Spec, const FAPSAncientGround* Ground, FAPSAncientShape& Out)
	{
		FRandomStream Stream(StreamSeed(Spec.Seed));
		const double Scale = Spec.Scale;
		const double HalfSize = Stream.FRandRange(1100.0, 1700.0) * 100.0 * Scale;
		const int32 Count = Stream.RandRange(18, 30);
		const double Tallest = Stream.FRandRange(1800.0, 2600.0) * 100.0 * Scale;
		const int32 Cells = FMath::CeilToInt(FMath::Sqrt(static_cast<double>(Count + 4)));
		const double Cell = 2.0 * HalfSize / Cells;
		Out.StoneColour = FLinearColor(0.085f, 0.064f, 0.05f);
		Out.Metrics.SizeCm = Tallest;
		Out.Metrics.HeightCm = Tallest * 1.1;
		Out.Metrics.FootprintCm = HalfSize * 1.45;
		Out.Metrics.NavHeightCm = Out.Metrics.HeightCm + 120000.0;
		Out.Metrics.OnFootCm = Tallest * 0.04 + 25000.0 * Scale;
		Out.Metrics.PartCount = Count;
		if (!Ground)
		{
			return;
		}

		struct FSpire
		{
			FVector2D At;
			double Height;
			double Width;
			int32 Tiers;
			double Yaw;
		};
		// The cells in an order shuffled from the seed; the cells round the centre stay free for the tallest spire.
		TArray<FIntPoint> Order;
		for (int32 GridX = 0; GridX < Cells; ++GridX)
		{
			for (int32 GridY = 0; GridY < Cells; ++GridY)
			{
				Order.Add(FIntPoint(GridX, GridY));
			}
		}
		for (int32 Index = Order.Num() - 1; Index > 0; --Index)
		{
			Order.Swap(Index, Stream.RandRange(0, Index));
		}
		TArray<FSpire> Spires;
		Spires.Add({FVector2D::ZeroVector, Tallest, Tallest * 0.07, 5, Stream.FRandRange(0.0, UE_HALF_PI)});
		for (const FIntPoint& CellAt : Order)
		{
			if (Spires.Num() > Count)
			{
				break;
			}
			const FVector2D Jitter(Stream.FRandRange(-0.3, 0.3), Stream.FRandRange(-0.3, 0.3));
			const double Variation = Stream.FRandRange(0.75, 1.2);
			const int32 Tiers = Stream.RandRange(3, 5);
			const double SpireYaw = Stream.FRandRange(0.0, UE_HALF_PI);
			const FVector2D At(-HalfSize + Cell * (CellAt.X + 0.5 + Jitter.X), -HalfSize + Cell * (CellAt.Y + 0.5 + Jitter.Y));
			if (At.Size() < Cell * 1.1)
			{
				continue;
			}
			const double Reach = FMath::Clamp(1.0 - At.Size() / (HalfSize * 1.45), 0.0, 1.0);
			const double SpireHeight = Tallest * FMath::Clamp(0.2 + 0.6 * FMath::Pow(Reach, 1.4), 0.12, 0.85) * Variation;
			Spires.Add({At, SpireHeight, FMath::Max(SpireHeight * 0.075, 3500.0 * Scale), Tiers, SpireYaw});
		}

		for (int32 Index = 0; Index < Spires.Num(); ++Index)
		{
			const FSpire& Spire = Spires[Index];
			const FQuat Turn = TurnAboutUp(Spire.Yaw);
			const double Low = LowestUnder(*Ground, Spire.At, FVector2D(Spire.Width * 0.5), Spire.Yaw);
			const double Bottom = Low - (2000.0 * Scale + Spire.Height * 0.02);
			const double Total = Low + Spire.Height - Bottom;
			double WeightSum = 0.0;
			for (int32 Tier = 0; Tier < Spire.Tiers; ++Tier)
			{
				WeightSum += 1.0 / (Tier + 1.3);
			}
			const bool bLit = Index == 0 || Spire.Height > Tallest * 0.45;
			double Z = Bottom;
			double TierWidth = Spire.Width;
			double LastWidth = TierWidth;
			for (int32 Tier = 0; Tier < Spire.Tiers; ++Tier)
			{
				const double TierHeight = Total * (1.0 / (Tier + 1.3)) / WeightSum;
				Out.Stone.Box(FVector(Spire.At.X, Spire.At.Y, Z + TierHeight * 0.5), FVector(TierWidth * 0.5, TierWidth * 0.5, TierHeight * 0.5), Turn);
				Z += TierHeight;
				if (Tier == 0 && bLit)
				{
					// A glowing ledge between the first two tiers.
					Out.Glow.Box(FVector(Spire.At.X, Spire.At.Y, Z), FVector(TierWidth * 0.46, TierWidth * 0.46, 120.0 * Scale), Turn);
				}
				LastWidth = TierWidth;
				TierWidth *= 0.83;
			}
			const double TipHalf = Spire.Height * 0.04;
			Out.Stone.Frustum(FVector(Spire.At.X, Spire.At.Y, Z + TipHalf), Turn * TurnAboutUp(UE_PI * 0.25), LastWidth * 0.5 * UE_SQRT_2,
				0.0, TipHalf, 4);
			if (Index == 0)
			{
				Out.Beacon.Box(FVector(Spire.At.X, Spire.At.Y, Z + TipHalf * 2.0 + Spire.Width * 0.06), FVector(Spire.Width * 0.06), Turn);
			}
		}
	}

	/**
	 * THE SUNKEN CUBE: a black cube 0.7-1.3 km on a side (scaled), its body diagonal tilted 8-30 degrees off the vertical,
	 * sunk until one corner stands 30-55% of an edge above the ground over the site's centre; the three edges that meet at
	 * that corner glow, the corner burns, and broken-off shards lie half buried round it.
	 */
	void BuildSunkenCube(const FSiteSpec& Spec, const FAPSAncientGround* Ground, FAPSAncientShape& Out)
	{
		FRandomStream Stream(StreamSeed(Spec.Seed));
		const double Scale = Spec.Scale;
		const double Edge = Stream.FRandRange(700.0, 1300.0) * 100.0 * Scale;
		const double TiltDegrees = Stream.FRandRange(8.0, 30.0);
		const double TiltHeading = Stream.FRandRange(0.0, UE_TWO_PI);
		const double CubeYaw = Stream.FRandRange(0.0, UE_TWO_PI);
		const double Exposed = Edge * Stream.FRandRange(0.30, 0.55);
		const int32 ShardCount = Stream.RandRange(4, 6);
		Out.StoneColour = FLinearColor(0.02f, 0.02f, 0.024f);
		Out.Metrics.SizeCm = Edge;
		Out.Metrics.HeightCm = Exposed;
		// The cube's corner stands over the centre, the cube itself up to half an edge aside, the shards out to 1.6 edges.
		Out.Metrics.FootprintCm = Edge * 1.8;
		// High enough that a fleet slot 1.5 km round the navigation point stays above the exposed corner.
		Out.Metrics.NavHeightCm = FMath::Max(Exposed, 30000.0) + 180000.0;
		Out.Metrics.OnFootCm = Edge * 0.25 + 30000.0 * Scale;
		Out.Metrics.PartCount = ShardCount;
		if (!Ground)
		{
			return;
		}

		const FQuat Diagonal = FQuat::FindBetweenNormals(FVector(1.0, 1.0, 1.0).GetSafeNormal(), FVector::UpVector);
		const FQuat Turn = TurnAboutUp(CubeYaw) * FQuat(HorizontalAxis(TiltHeading), FMath::DegreesToRadians(TiltDegrees)) * Diagonal;
		const FVector Corner = Turn.RotateVector(FVector(Edge * 0.5));
		const FVector Centre(-Corner.X, -Corner.Y, Ground->Z(0.0, 0.0) + Exposed - Corner.Z);
		Out.Stone.Box(Centre, FVector(Edge * 0.5), Turn);
		// The three edges meeting at the exposed corner, a little proud of the faces.
		const double Seam = Edge * 0.007;
		for (int32 Axis = 0; Axis < 3; ++Axis)
		{
			FVector Direction = FVector::ZeroVector;
			Direction[Axis] = 1.0;
			const FVector Local = FVector(Edge * 0.5) - Direction * (Edge * 0.5) + (FVector(1.0) - Direction) * (Edge * 0.004);
			FVector Half(Seam);
			Half[Axis] = Edge * 0.49;
			Out.Glow.Box(Centre + Turn.RotateVector(Local), Half, Turn);
		}
		Out.Beacon.Box(Centre + Corner + FVector::UpVector * (Edge * 0.012), FVector(Edge * 0.02), Turn);
		// Shards broken off, half buried round it.
		for (int32 Index = 0; Index < ShardCount; ++Index)
		{
			const double ShardSize = Edge * Stream.FRandRange(0.08, 0.22);
			const double Angle = Stream.FRandRange(0.0, UE_TWO_PI);
			const double Distance = Edge * Stream.FRandRange(0.8, 1.6);
			const FQuat Tumble = FRotator(Stream.FRandRange(-60.0, 60.0), Stream.FRandRange(0.0, 360.0),
				Stream.FRandRange(-60.0, 60.0)).Quaternion();
			const double Sink = Stream.FRandRange(-0.1, 0.35);
			const FVector2D At = FVector2D(FMath::Cos(Angle), FMath::Sin(Angle)) * Distance + FVector2D(Centre.X, Centre.Y);
			Out.Stone.Box(FVector(At.X, At.Y, Ground->Z(At.X, At.Y) + ShardSize * Sink), FVector(ShardSize * 0.5), Tumble);
		}
	}

	/**
	 * THE STONE CIRCLE (walkable): 9-13 standing stones 4-7 m tall in a ring 28-48 m across (scaled), one pair bridged
	 * by a lintel, round a plinth with a glowing glass disc; a tablet leans beside the plinth.
	 */
	void BuildStoneCircle(const FSiteSpec& Spec, const FAPSAncientGround* Ground, FAPSAncientShape& Out)
	{
		FRandomStream Stream(StreamSeed(Spec.Seed));
		const double Scale = Spec.Scale;
		const double Radius = Stream.FRandRange(1400.0, 2400.0) * Scale;
		const int32 StoneCount = Stream.RandRange(9, 13);
		const int32 LintelAt = Stream.RandRange(0, StoneCount - 1);
		Out.StoneColour = FLinearColor(0.14f, 0.135f, 0.125f);
		Out.Metrics.SizeCm = Radius * 2.0;
		Out.Metrics.HeightCm = 750.0 * Scale;
		Out.Metrics.FootprintCm = Radius + 600.0 * Scale;
		Out.Metrics.NavHeightCm = 200000.0;
		Out.Metrics.OnFootCm = Radius + 800.0 * Scale;
		Out.Metrics.PartCount = StoneCount;
		if (!Ground)
		{
			return;
		}

		TArray<FVector> Tops;
		StoneRing(Stream, *Ground, Out, StoneCount, Radius, FVector(35.0, 60.0, 220.0) * Scale, FVector(55.0, 95.0, 360.0) * Scale,
			60.0 * Scale, 0.15f, 0.1f, 3.0 * Scale, Tops);
		// A lintel across two neighbours that both stand: a trilithon.
		const int32 Next = (LintelAt + 1) % StoneCount;
		if (Tops.IsValidIndex(LintelAt) && Tops.IsValidIndex(Next) && !Tops[LintelAt].IsZero() && !Tops[Next].IsZero())
		{
			const FVector Span = Tops[Next] - Tops[LintelAt];
			Out.Stone.Box((Tops[LintelAt] + Tops[Next]) * 0.5 + FVector(0.0, 0.0, 35.0 * Scale),
				FVector(Span.Size() * 0.5 + 50.0 * Scale, 45.0 * Scale, 35.0 * Scale), FRotationMatrix::MakeFromX(Span).ToQuat());
		}
		// The plinth and its glass disc, the brightest thing here.
		double PlinthLow = 0.0;
		double PlinthHigh = 0.0;
		RangeRound(*Ground, FVector2D::ZeroVector, 260.0 * Scale, PlinthLow, PlinthHigh);
		const double PlinthBottom = PlinthLow - 40.0 * Scale;
		const double PlinthTop = FMath::Max(PlinthHigh + 90.0 * Scale, PlinthBottom + 110.0 * Scale);
		Out.Stone.Frustum(FVector(0.0, 0.0, (PlinthTop + PlinthBottom) * 0.5), TurnAboutUp(UE_PI / 8.0), 260.0 * Scale, 235.0 * Scale,
			(PlinthTop - PlinthBottom) * 0.5, 8);
		Out.Beacon.Frustum(FVector(0.0, 0.0, PlinthTop + 4.0 * Scale), FQuat::Identity, 190.0 * Scale, 190.0 * Scale, 4.0 * Scale, 16);
		// The tablet leaning by the plinth.
		const double TabletX = 420.0 * Scale;
		Out.Stone.Box(FVector(TabletX, 0.0, Ground->Z(TabletX, 0.0) + 110.0 * Scale), FVector(18.0, 120.0, 160.0) * Scale,
			FQuat(FVector::RightVector, FMath::DegreesToRadians(-14.0)));
	}

	/**
	 * THE QUIET HULL: a hull 2.2-4.2 km long (scaled) along X, broken behind its middle: the front section with its nose,
	 * a gap with debris, the rear section knocked 3-7 degrees askew with its flared engine bell and fins; three rings of
	 * segments on spokes, some segments gone. Glowing slits run along the front section, the broken end glows faintly from
	 * inside and a bright point marks the nose.
	 */
	void BuildDerelict(const FSiteSpec& Spec, const FAPSAncientGround* Ground, FAPSAncientShape& Out)
	{
		FRandomStream Stream(StreamSeed(Spec.Seed));
		const double Scale = Spec.Scale;
		const double Length = Stream.FRandRange(2200.0, 4200.0) * 100.0 * Scale;
		const double Hull = Length * Stream.FRandRange(0.055, 0.075);
		const double RearTilt = Stream.FRandRange(3.0, 7.0);
		Out.StoneColour = FLinearColor(0.045f, 0.05f, 0.055f);
		Out.GlowColour = FLinearColor(0.2f, 0.75f, 1.0f);
		Out.Metrics.SizeCm = Length;
		Out.Metrics.HeightCm = Hull * 2.0;
		Out.Metrics.FootprintCm = Length * 0.55;
		// A fleet slot 1.5 km round the navigation point stays clear of the hull and its rings from any side.
		Out.Metrics.NavHeightCm = 180000.0 + Hull * 2.0;
		Out.Metrics.OnFootCm = 300000.0;
		Out.Metrics.PartCount = 3;
		if (!Ground)
		{
			return;
		}

		// The frustums' own Z runs along the hull.
		const FQuat AxisTurn = FQuat::FindBetweenNormals(FVector::UpVector, FVector::ForwardVector);
		const FQuat Rear(FVector::RightVector, FMath::DegreesToRadians(RearTilt));
		const FVector RearOffset(0.0, 0.0, Hull * 0.25);
		const auto RearPoint = [&](const FVector& Point) { return Rear.RotateVector(Point) + RearOffset; };
		Out.Stone.Frustum(FVector(Length * 0.18, 0.0, 0.0), AxisTurn, Hull, Hull * 0.93, Length * 0.26, 16, true);
		Out.Stone.Frustum(FVector(Length * 0.51, 0.0, 0.0), AxisTurn, Hull * 0.93, Hull * 0.18, Length * 0.07, 16, true);
		Out.Stone.Frustum(RearPoint(FVector(-Length * 0.33, 0.0, 0.0)), Rear * AxisTurn, Hull * 0.82, Hull * 0.9, Length * 0.17, 16, true);
		Out.Stone.Frustum(RearPoint(FVector(-Length * 0.54, 0.0, 0.0)), Rear * AxisTurn, Hull * 1.15, Hull * 0.6, Length * 0.04, 16, true);

		// Three rings of segments on four spokes each; the rear one goes askew with the rear section.
		const double RingRadius = Hull * 1.75;
		constexpr int32 RingCount = 3;
		const double RingStations[RingCount] = {Length * 0.30, Length * -0.02, Length * -0.30};
		constexpr int32 RingSegments = 14;
		for (int32 RingIndex = 0; RingIndex < RingCount; ++RingIndex)
		{
			const bool bRearRing = RingIndex == 2;
			for (int32 Segment = 0; Segment < RingSegments; ++Segment)
			{
				const bool bGone = Stream.FRand() < 0.15f;
				const double Phi = UE_TWO_PI * (Segment + 0.5) / RingSegments;
				const FVector Radial(0.0, FMath::Cos(Phi), FMath::Sin(Phi));
				if (bGone)
				{
					continue;
				}
				const FVector Centre = FVector(RingStations[RingIndex], 0.0, 0.0) + Radial * RingRadius;
				const FQuat Turn = FRotationMatrix::MakeFromXZ(FVector::ForwardVector, Radial).ToQuat();
				Out.Stone.Box(bRearRing ? RearPoint(Centre) : Centre,
					FVector(Hull * 0.10, RingRadius * FMath::Sin(UE_PI / RingSegments) * 1.04, Hull * 0.10), bRearRing ? Rear * Turn : Turn);
			}
			for (int32 Spoke = 0; Spoke < 4; ++Spoke)
			{
				const double Phi = UE_HALF_PI * Spoke + UE_PI * 0.25;
				const FVector Radial(0.0, FMath::Cos(Phi), FMath::Sin(Phi));
				const FVector Centre = FVector(RingStations[RingIndex], 0.0, 0.0) + Radial * ((Hull + RingRadius) * 0.5);
				const FQuat Turn = FRotationMatrix::MakeFromXZ(FVector::ForwardVector, Radial).ToQuat();
				Out.Stone.Box(bRearRing ? RearPoint(Centre) : Centre, FVector(Hull * 0.05, Hull * 0.05, (RingRadius - Hull) * 0.5),
					bRearRing ? Rear * Turn : Turn);
			}
		}
		// Fins at the rear.
		for (int32 Fin = 0; Fin < 4; ++Fin)
		{
			const double Phi = UE_HALF_PI * Fin + UE_PI * 0.25;
			const FVector Radial(0.0, FMath::Cos(Phi), FMath::Sin(Phi));
			const FQuat Turn = Rear * FRotationMatrix::MakeFromXZ(FVector::ForwardVector, Radial).ToQuat();
			Out.Stone.Box(RearPoint(FVector(-Length * 0.40, 0.0, 0.0) + Radial * (Hull * 1.7)), FVector(Length * 0.10, Hull * 0.05, Hull * 0.9), Turn);
		}
		// Debris drifting in the break.
		for (int32 Index = 0; Index < 10; ++Index)
		{
			const double AlongHull = Stream.FRandRange(-0.17, -0.07) * Length;
			const double Spread = Stream.FRandRange(0.0, 2.2) * Hull;
			const double Around = Stream.FRandRange(0.0, UE_TWO_PI);
			const double PieceSize = Stream.FRandRange(1500.0, 9000.0) * Scale;
			const FQuat Tumble = FRotator(Stream.FRandRange(-90.0, 90.0), Stream.FRandRange(0.0, 360.0),
				Stream.FRandRange(-180.0, 180.0)).Quaternion();
			Out.Stone.Box(FVector(AlongHull, FMath::Cos(Around) * Spread, FMath::Sin(Around) * Spread),
				FVector(PieceSize * 0.5, PieceSize * 0.35, PieceSize * 0.25), Tumble);
		}
		// Slits along the front section (on the faces between the hull's edges), the glowing break and the nose light.
		for (int32 Slit = 0; Slit < 4; ++Slit)
		{
			const double Phi = UE_HALF_PI * Slit + UE_PI / 16.0;
			const FVector Radial(0.0, FMath::Cos(Phi), FMath::Sin(Phi));
			Out.Glow.Box(FVector(Length * 0.20, 0.0, 0.0) + Radial * (Hull * 0.975), FVector(Length * 0.18, Hull * 0.035, Hull * 0.035),
				FRotationMatrix::MakeFromXZ(FVector::ForwardVector, Radial).ToQuat());
		}
		Out.Glow.Frustum(FVector(-Length * 0.08 - 50.0 * Scale, 0.0, 0.0), AxisTurn, Hull * 0.82, Hull * 0.82, 40.0 * Scale, 16);
		Out.Beacon.Box(FVector(Length * 0.585, 0.0, 0.0), FVector(Hull * 0.12), FQuat::Identity);
	}
}

void FAPSAncientMesh::AddVertex(const FVector& Position, const FVector& Normal, const FVector& UAxis)
{
	using APSAncientsGeometryLocal::UVTileCm;
	const FVector VAxis = FVector::CrossProduct(Normal, UAxis);
	Vertices.Add(Position);
	Normals.Add(Normal);
	UVs.Add(FVector2D(FVector::DotProduct(Position, UAxis) / UVTileCm, FVector::DotProduct(Position, VAxis) / UVTileCm));
	Tangents.Add(FProcMeshTangent(UAxis, false));
}

void FAPSAncientMesh::Quad(const FVector& A, const FVector& B, const FVector& C, const FVector& D, const FVector& Normal,
	const FVector& UAxis)
{
	QuadSmooth(A, B, C, D, Normal, Normal, Normal, Normal, UAxis);
}

void FAPSAncientMesh::QuadSmooth(const FVector& A, const FVector& B, const FVector& C, const FVector& D, const FVector& NormalA,
	const FVector& NormalB, const FVector& NormalC, const FVector& NormalD, const FVector& UAxis)
{
	const int32 First = Vertices.Num();
	AddVertex(A, NormalA, UAxis);
	AddVertex(B, NormalB, UAxis);
	AddVertex(C, NormalC, UAxis);
	AddVertex(D, NormalD, UAxis);
	// The engine's front faces wind so that (B - A) x (D - A) points away from the normal, as the engine's own box does
	// (UKismetProceduralMeshLibrary::GenerateBoxMesh); the corners may come round either way.
	const FVector Facing = NormalA + NormalB + NormalC + NormalD;
	if (FVector::DotProduct(FVector::CrossProduct(B - A, D - A), Facing) <= 0.0)
	{
		Triangles.Append({First, First + 1, First + 3, First + 1, First + 2, First + 3});
	}
	else
	{
		Triangles.Append({First, First + 3, First + 1, First + 1, First + 3, First + 2});
	}
}

void FAPSAncientMesh::Tri(const FVector& A, const FVector& B, const FVector& C, const FVector& Normal, const FVector& UAxis)
{
	const int32 First = Vertices.Num();
	AddVertex(A, Normal, UAxis);
	AddVertex(B, Normal, UAxis);
	AddVertex(C, Normal, UAxis);
	if (FVector::DotProduct(FVector::CrossProduct(B - A, C - A), Normal) <= 0.0)
	{
		Triangles.Append({First, First + 1, First + 2});
	}
	else
	{
		Triangles.Append({First, First + 2, First + 1});
	}
}

void FAPSAncientMesh::Box(const FVector& Centre, const FVector& HalfSize, const FQuat& Rotation)
{
	const FVector X = Rotation.GetAxisX();
	const FVector Y = Rotation.GetAxisY();
	const FVector Z = Rotation.GetAxisZ();
	const FVector Ex = X * HalfSize.X;
	const FVector Ey = Y * HalfSize.Y;
	const FVector Ez = Z * HalfSize.Z;
	const FVector& C = Centre;
	Quad(C + Ez - Ex + Ey, C + Ez + Ex + Ey, C + Ez + Ex - Ey, C + Ez - Ex - Ey, Z, X);
	Quad(C - Ez - Ex - Ey, C - Ez + Ex - Ey, C - Ez + Ex + Ey, C - Ez - Ex + Ey, -Z, X);
	Quad(C + Ex - Ey - Ez, C + Ex + Ey - Ez, C + Ex + Ey + Ez, C + Ex - Ey + Ez, X, Y);
	Quad(C - Ex + Ey - Ez, C - Ex - Ey - Ez, C - Ex - Ey + Ez, C - Ex + Ey + Ez, -X, Y);
	Quad(C + Ey + Ex - Ez, C + Ey - Ex - Ez, C + Ey - Ex + Ez, C + Ey + Ex + Ez, Y, X);
	Quad(C - Ey - Ex - Ez, C - Ey + Ex - Ez, C - Ey + Ex + Ez, C - Ey - Ex + Ez, -Y, X);
}

void FAPSAncientMesh::Frustum(const FVector& Centre, const FQuat& Rotation, const double BottomRadius, const double TopRadius,
	const double HalfHeight, const int32 Sides, const bool bSmooth)
{
	const int32 Count = FMath::Clamp(Sides, 3, 64);
	const FVector AxisX = Rotation.GetAxisX();
	const FVector AxisY = Rotation.GetAxisY();
	const FVector AxisZ = Rotation.GetAxisZ();
	const FVector BottomCentre = Centre - AxisZ * HalfHeight;
	const FVector TopCentre = Centre + AxisZ * HalfHeight;
	const bool bCone = TopRadius <= UE_KINDA_SMALL_NUMBER;
	TArray<FVector, TInlineAllocator<65>> Directions;
	for (int32 Index = 0; Index <= Count; ++Index)
	{
		const double Angle = UE_TWO_PI * (Index % Count) / Count;
		Directions.Add(AxisX * FMath::Cos(Angle) + AxisY * FMath::Sin(Angle));
	}
	// Outward along the slant: the radial direction tipped by the narrowing.
	const auto SideNormal = [&](const FVector& Radial)
	{
		return (Radial * (2.0 * HalfHeight) + AxisZ * (BottomRadius - TopRadius)).GetSafeNormal();
	};
	for (int32 Index = 0; Index < Count; ++Index)
	{
		const FVector& D0 = Directions[Index];
		const FVector& D1 = Directions[Index + 1];
		const FVector B0 = BottomCentre + D0 * BottomRadius;
		const FVector B1 = BottomCentre + D1 * BottomRadius;
		const FVector Tangent = (D1 - D0).GetSafeNormal();
		const FVector Middle = (D0 + D1).GetSafeNormal();
		if (bCone)
		{
			Tri(B0, B1, TopCentre, SideNormal(Middle), Tangent);
			continue;
		}
		const FVector T0 = TopCentre + D0 * TopRadius;
		const FVector T1 = TopCentre + D1 * TopRadius;
		if (bSmooth)
		{
			QuadSmooth(B0, B1, T1, T0, SideNormal(D0), SideNormal(D1), SideNormal(D1), SideNormal(D0), Tangent);
		}
		else
		{
			Quad(B0, B1, T1, T0, SideNormal(Middle), Tangent);
		}
	}
	for (int32 Index = 0; Index < Count; ++Index)
	{
		if (BottomRadius > UE_KINDA_SMALL_NUMBER)
		{
			Tri(BottomCentre, BottomCentre + Directions[Index + 1] * BottomRadius, BottomCentre + Directions[Index] * BottomRadius,
				-AxisZ, AxisX);
		}
		if (!bCone)
		{
			Tri(TopCentre, TopCentre + Directions[Index] * TopRadius, TopCentre + Directions[Index + 1] * TopRadius, AxisZ, AxisX);
		}
	}
}

void APSAncientsGeometry::Build(const APSAncients::FSiteSpec& Spec, const FAPSAncientGround* Ground, FAPSAncientShape& Out)
{
	using namespace APSAncientsGeometryLocal;
	Out = FAPSAncientShape();
	switch (Spec.Kind)
	{
	case EKind::Monolith: BuildMonolith(Spec, Ground, Out); break;
	case EKind::BrokenRing: BuildBrokenRing(Spec, Ground, Out); break;
	case EKind::SpireField: BuildSpireField(Spec, Ground, Out); break;
	case EKind::SunkenCube: BuildSunkenCube(Spec, Ground, Out); break;
	case EKind::StoneCircle: BuildStoneCircle(Spec, Ground, Out); break;
	case EKind::Derelict: BuildDerelict(Spec, Ground, Out); break;
	default: break;
	}
}

double APSAncientsGeometry::MaxSlope(const APSAncients::EKind Kind)
{
	using APSAncients::EKind;
	switch (Kind)
	{
	case EKind::Monolith: return 0.22;
	case EKind::BrokenRing: return 0.18;
	case EKind::SpireField: return 0.25;
	case EKind::SunkenCube: return 0.35;
	case EKind::StoneCircle: return 0.20;
	default: return 1.0;
	}
}
