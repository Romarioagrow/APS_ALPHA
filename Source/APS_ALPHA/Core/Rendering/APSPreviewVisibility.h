#pragma once

#include "CoreMinimal.h"

/** Camera-relative opaque body used by menu annotations, never a gravity zone. */
struct FAPSPreviewOccluder
{
	FVector Center{FVector::ZeroVector};
	double Radius{0.0};

	bool Occludes(const FVector& Point) const
	{
		const double Distance = Center.Size();
		if (!FMath::IsFinite(Distance) || !FMath::IsFinite(Radius) || Radius <= 0.0) return false;
		if (Distance <= Radius) return true;
		const FVector C = Center / Distance;
		const FVector P = Point / Distance;
		const double R = Radius / Distance;
		const double Along = FVector::DotProduct(C, P);
		const double LengthSquared = P.SizeSquared();
		if (Along <= 0.0 || LengthSquared <= UE_DOUBLE_SMALL_NUMBER) return false;
		// The near ray/sphere intersection must precede the annotation. This
		// distinguishes an orbit in front of a body from one behind its silhouette.
		const double Discriminant = Along * Along - (1.0 - R * R) * LengthSquared;
		return Discriminant >= 0.0
			&& (Along - FMath::Sqrt(Discriminant)) < LengthSquared * (1.0 - 1.0e-10);
	}
};

namespace APSPreviewVisibility
{
/** Conservative sphere/frustum test in camera axes (X forward, Y right, Z up).
 * The caller includes any corona/optical guard in Radius. A center outside the
 * rectangle is NOT sufficient to discard a disc intersecting the frame edge.
 */
inline bool SphereIntersectsView(const FVector& Center, const double Radius,
	const double TanHalfHorizontalFov, const double TanHalfVerticalFov)
{
	if (Center.ContainsNaN() || !FMath::IsFinite(Radius) || Radius < 0.0
		|| !FMath::IsFinite(TanHalfHorizontalFov) || !FMath::IsFinite(TanHalfVerticalFov)
		|| TanHalfHorizontalFov <= 0.0 || TanHalfVerticalFov <= 0.0) return false;
	return Center.X + Radius >= 0.0
		&& FMath::Abs(Center.Y) <= Center.X * TanHalfHorizontalFov
			+ Radius * FMath::Sqrt(1.0 + FMath::Square(TanHalfHorizontalFov))
		&& FMath::Abs(Center.Z) <= Center.X * TanHalfVerticalFov
			+ Radius * FMath::Sqrt(1.0 + FMath::Square(TanHalfVerticalFov));
}

using FVisibilityCuts = TArray<double, TInlineAllocator<32>>;

inline void AddUnitQuadraticRoots(double A, double B, double C, FVisibilityCuts& Cuts)
{
	const double Scale = FMath::Max3(FMath::Abs(A), FMath::Abs(B), FMath::Abs(C));
	if (!FMath::IsFinite(Scale) || Scale <= 1.0e-30) return;
	A /= Scale; B /= Scale; C /= Scale;
	const auto Add = [&Cuts](const double T)
	{
		if (FMath::IsFinite(T) && T > 0.0 && T < 1.0) Cuts.Add(T);
	};
	if (FMath::Abs(A) < 1.0e-14)
	{
		if (FMath::Abs(B) > 1.0e-14) Add(-C / B);
		return;
	}
	const double Discriminant = B * B - 4.0 * A * C;
	if (Discriminant < 0.0) return;
	const double Q = -0.5 * (B + (B >= 0.0 ? 1.0 : -1.0) * FMath::Sqrt(Discriminant));
	Add(Q / A);
	if (FMath::Abs(Q) > 1.0e-30) Add(C / Q);
}

/** Exact visible parameter intervals of a straight camera-relative segment.
 * Split at shadow-cone and sphere boundaries, not only at sampled endpoints:
 * two visible endpoints may still have an entire planet between them.
 */
inline void VisibleIntervals(const FVector& From, const FVector& To,
	const TArray<FAPSPreviewOccluder>& Occluders, TArray<FVector2D>& OutIntervals)
{
	OutIntervals.Reset();
	if (From.ContainsNaN() || To.ContainsNaN()) return;
	FVisibilityCuts Cuts{0.0, 1.0};
	for (const FAPSPreviewOccluder& Sphere : Occluders)
	{
		const double Distance = Sphere.Center.Size();
		if (!FMath::IsFinite(Distance) || !FMath::IsFinite(Sphere.Radius) || Sphere.Radius <= 0.0) continue;
		if (Distance <= Sphere.Radius) return;
		const FVector C = Sphere.Center / Distance;
		const FVector A = From / Distance;
		const FVector D = (To - From) / Distance;
		const double R = Sphere.Radius / Distance;
		const double CA = FVector::DotProduct(C, A);
		const double CD = FVector::DotProduct(C, D);
		const double K = 1.0 - R * R;
		AddUnitQuadraticRoots(CD * CD - K * D.SizeSquared(),
			2.0 * (CA * CD - K * FVector::DotProduct(A, D)), CA * CA - K * A.SizeSquared(), Cuts);
		const FVector Relative = A - C;
		AddUnitQuadraticRoots(D.SizeSquared(), 2.0 * FVector::DotProduct(Relative, D),
			Relative.SizeSquared() - R * R, Cuts);
		if (FMath::Abs(CD) > 1.0e-14)
		{
			const double T = -CA / CD;
			if (T > 0.0 && T < 1.0) Cuts.Add(T);
		}
	}
	Cuts.Sort();
	for (int32 Index = 1; Index < Cuts.Num(); ++Index)
	{
		const double Begin = Cuts[Index - 1];
		const double End = Cuts[Index];
		if (End - Begin <= 1.0e-12) continue;
		const FVector Midpoint = FMath::Lerp(From, To, (Begin + End) * 0.5);
		if (Occluders.ContainsByPredicate([&](const FAPSPreviewOccluder& Sphere) { return Sphere.Occludes(Midpoint); })) continue;
		if (!OutIntervals.IsEmpty() && FMath::IsNearlyEqual(OutIntervals.Last().Y, Begin, 1.0e-12))
			OutIntervals.Last().Y = End;
		else OutIntervals.Emplace(Begin, End);
	}
}

/** Clip a projected guide segment to the actual central panel (DPI-local units). */
inline bool ClipToPanel(FVector2D& From, FVector2D& To, const FVector2D& Size)
{
	if (From.ContainsNaN() || To.ContainsNaN() || Size.X <= 0.0 || Size.Y <= 0.0) return false;
	const FVector2D Delta = To - From;
	double Begin = 0.0;
	double End = 1.0;
	for (int32 Axis = 0; Axis < 2; ++Axis)
	{
		if (FMath::Abs(Delta[Axis]) < 1.0e-12)
		{
			if (From[Axis] < 0.0 || From[Axis] > Size[Axis]) return false;
			continue;
		}
		double A = -From[Axis] / Delta[Axis];
		double B = (Size[Axis] - From[Axis]) / Delta[Axis];
		if (A > B) Swap(A, B);
		Begin = FMath::Max(Begin, A);
		End = FMath::Min(End, B);
		if (Begin > End) return false;
	}
	To = From + Delta * End;
	From += Delta * Begin;
	return true;
}
}
