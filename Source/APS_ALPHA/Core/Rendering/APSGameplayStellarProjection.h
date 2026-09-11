#pragma once

#include "APSContinuousPreviewFrame.h"
#include "APSPreviewVisibility.h"

namespace APSGameplayStellarProjection
{
// Match the accepted menu's Gaussian support. The bright core is smaller than
// this sphere; this is optical sampling, not an enlarged physical stellar radius.
inline constexpr double PointSupportPixels = 2.2;
inline constexpr double FarEnvelopeCm = 1.0e9;
// Invalidation is based on accumulated screen error, not an arbitrary frame rate.
// The component anchor also remains still until its rendered parallax exceeds
// this bound: moving an ordinary ISM invalidates every instance in UE 5.4.
inline constexpr double ReprojectionErrorPixels = 0.05;

// Keep the accepted Gaussian point resolvable at its actual 3D distance. All
// lengths use the same frame; only the glyph radius changes, never its centre.
inline double GetFullScalePointRadius(double Distance, double BaseRadius, double PixelTangent)
{
	if (!FMath::IsFinite(Distance) || Distance <= 0.0
		|| !FMath::IsFinite(PixelTangent) || PixelTangent <= 0.0)
	{
		return BaseRadius;
	}
	return FMath::Max(BaseRadius, Distance * PixelTangent * PointSupportPixels);
}

inline bool NeedsInstanceUpload(bool bReproject, bool bPreviouslyOccluded, bool bNowOccluded)
{
	return (bReproject && !bNowOccluded) || bPreviouslyOccluded != bNowOccluded;
}

inline bool CanReuseOptics(double PreviousPixelTangent, double CurrentPixelTangent)
{
	return FMath::IsFinite(PreviousPixelTangent) && PreviousPixelTangent > 0.0
		&& FMath::IsFinite(CurrentPixelTangent) && CurrentPixelTangent > 0.0
		&& PointSupportPixels * FMath::Abs(PreviousPixelTangent / CurrentPixelTangent - 1.0)
			<= ReprojectionErrorPixels;
}

inline bool CanReuseProjection(double ObserverTravelCm, double ClosestPointCm, double PixelTangent)
{
	return FMath::IsFinite(ObserverTravelCm) && ObserverTravelCm >= 0.0
		&& FMath::IsFinite(ClosestPointCm) && ClosestPointCm > ObserverTravelCm
		&& FMath::IsFinite(PixelTangent) && PixelTangent > 0.0
		&& ObserverTravelCm / (ClosestPointCm - ObserverTravelCm)
			<= PixelTangent * ReprojectionErrorPixels;
}

/** Only distant points behind both body silhouettes may reuse an occlusion mask.
 * Nearby/foreground points fall back to exact recomputation on body movement. */
inline bool CanReuseOcclusion(const TArray<FAPSPreviewOccluder>& Previous,
	const TArray<FAPSPreviewOccluder>& Current, double NearestPointCm, double PixelTangent)
{
	if (Previous.Num() != Current.Num()) return false;
	for (int32 Index = 0; Index < Current.Num(); ++Index)
	{
		const FAPSPreviewOccluder& A = Previous[Index];
		const FAPSPreviewOccluder& B = Current[Index];
		if (A.Center == B.Center && A.Radius == B.Radius) continue;
		const double DA = A.Center.Size(), DB = B.Center.Size();
		if (DA <= A.Radius || DB <= B.Radius
			|| FMath::Max(DA + A.Radius, DB + B.Radius) >= NearestPointCm) return false;
		const double DirectionChange = (A.Center / DA - B.Center / DB).Size();
		const double LimbChange = FMath::Abs(FMath::Asin(FMath::Clamp(A.Radius / DA, 0.0, 1.0))
			- FMath::Asin(FMath::Clamp(B.Radius / DB, 0.0, 1.0)));
		if (DirectionChange + LimbChange > PixelTangent * ReprojectionErrorPixels) return false;
	}
	return true;
}

/** Normalize each body once per refresh instead of once for every catalog point. */
struct FPreparedOccluder
{
	FVector Direction;
	double DistanceCm;
	double CosHalfAngleSquared;
	bool bInside;

	explicit FPreparedOccluder(const FAPSPreviewOccluder& Body)
	{
		DistanceCm = Body.Center.Size();
		bInside = DistanceCm <= Body.Radius;
		Direction = bInside ? FVector::ZeroVector : Body.Center / DistanceCm;
		CosHalfAngleSquared = bInside ? 0.0 : 1.0 - FMath::Square(Body.Radius / DistanceCm);
	}

	bool Occludes(const FVector& PointDirection, double PointDistanceCm) const
	{
		if (bInside) return true;
		const double Along = FVector::DotProduct(Direction, PointDirection);
		const double Discriminant = Along * Along - CosHalfAngleSquared;
		return Along > 0.0 && Discriminant >= 0.0
			&& DistanceCm * (Along - FMath::Sqrt(Discriminant)) < PointDistanceCm * (1.0 - 1.0e-10);
	}
};

inline bool Project(const FVector& FromObserverCm, double RadiusCm, double PixelTangent,
	FAPSPreviewProjectedSphere& Sphere, double& OpticalRadiusCm)
{
	FAPSContinuousPreviewFrame Frame;
	Frame.FarEnvelopeCm = FarEnvelopeCm;
	if (!FMath::IsFinite(PixelTangent) || PixelTangent <= 0.0
		|| !Frame.ProjectSphere(FromObserverCm, RadiusCm, Sphere)) return false;
	OpticalRadiusCm = FMath::Max(Sphere.Radius,
		Sphere.Center.Size() * PixelTangent * PointSupportPixels);
	return FMath::IsFinite(OpticalRadiusCm);
}

/** Test in physical camera space BEFORE compression. A bounded optical proxy
 * must not move a background star in front of an opaque local-system body. */
inline bool IsOccluded(const FVector& FromObserverCm,
	const TArray<FAPSPreviewOccluder>& Occluders)
{
	for (const FAPSPreviewOccluder& Occluder : Occluders)
		if (Occluder.Occludes(FromObserverCm)) return true;
	return false;
}
}
