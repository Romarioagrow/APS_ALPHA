#pragma once

#include "APSContinuousPreviewFrame.h"
#include "APSPreviewVisibility.h"
#include "APSStellarOpticalSupport.h"

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
inline double GetFullScalePointRadius(double Distance, double BaseRadius, double PixelTangent,
	double SupportPixels = PointSupportPixels)
{
	if (!FMath::IsFinite(Distance) || Distance <= 0.0
		|| !FMath::IsFinite(PixelTangent) || PixelTangent <= 0.0)
	{
		return BaseRadius;
	}
	const double SafeSupport = FMath::IsFinite(SupportPixels)
		? FMath::Clamp(SupportPixels, PointSupportPixels, APSStellarOpticalSupport::MaximumSupportPixels) : PointSupportPixels;
	return FMath::Max(BaseRadius, Distance * PixelTangent * SafeSupport);
}

inline bool NeedsInstanceUpload(bool bReproject, bool bPreviouslyOccluded, bool bNowOccluded)
{
	return (bReproject && !bNowOccluded) || bPreviouslyOccluded != bNowOccluded;
}

inline bool CanReuseOptics(double PreviousPixelTangent, double CurrentPixelTangent)
{
	return FMath::IsFinite(PreviousPixelTangent) && PreviousPixelTangent > 0.0
		&& FMath::IsFinite(CurrentPixelTangent) && CurrentPixelTangent > 0.0
		&& APSStellarOpticalSupport::MaximumSupportPixels * FMath::Abs(PreviousPixelTangent / CurrentPixelTangent - 1.0)
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

/** Glyph size error a travelling observer may accumulate before a point is re-sized: 0.15 px on the largest
 * (14 px) glyph, ~1% of any glyph. At the 0.05 px optics budget a CRUISE flight at ~1000c re-sized every star
 * within ~70 AU on each pass (10-13 ms); at 0.15 px it is the stars within ~25 AU (30.09). */
inline constexpr double PointSizeErrorPixels = 0.15;

/** A consumed (true 3D) catalogue keeps every centre at its real position, so observer travel changes only the
 * glyph size that holds a point at its pixel support. A point sized at SizedDistanceCm keeps its glyph while that
 * size error stays within PointSizeErrorPixels. The angular test above is for re-projected shells;
 * applied to a consumed catalogue it forced a pass over all 61k instances nearly every frame at interplanetary
 * speed (29.09: 40-80 ms each, 7-12 FPS). */
inline bool CanReusePointSize(double DistanceCm, double SizedDistanceCm)
{
	return SizedDistanceCm > 0.0 && FMath::IsFinite(DistanceCm)
		&& APSStellarOpticalSupport::MaximumSupportPixels * FMath::Abs(DistanceCm - SizedDistanceCm) / SizedDistanceCm
			<= PointSizeErrorPixels;
}

/** How far the observer may still travel before a point sized at SizedDistanceCm (now at DistanceCm) exceeds the
 * CanReusePointSize budget; negative once it has. A distance changes by at most the observer's travel, so no point
 * needs a check until the travel since the last check exceeds the smallest slack of the catalogue. */
inline double PointSizeSlackCm(double DistanceCm, double SizedDistanceCm)
{
	return PointSizeErrorPixels / APSStellarOpticalSupport::MaximumSupportPixels * SizedDistanceCm
		- FMath::Abs(DistanceCm - SizedDistanceCm);
}

/** Native-star selection (which stars get a resolved mesh) tolerates more travel than glyph sizes: refresh it once
 * the observer has moved 2% of the distance to the nearest catalogue point. */
inline bool CanReuseDemand(double ObserverTravelCm, double ClosestPointCm)
{
	return FMath::IsFinite(ObserverTravelCm) && ObserverTravelCm >= 0.0
		&& FMath::IsFinite(ClosestPointCm) && ClosestPointCm > ObserverTravelCm
		&& ObserverTravelCm / (ClosestPointCm - ObserverTravelCm) <= 0.02;
}

/** The list of demand candidates (points of at least 0.3 px, over 2x below the smallest admission radius of 0.65 px)
 * stays complete while no point outside it can grow past 0.65 px: after a travel of ClosestPointCm / 5 every point
 * looks at most 1.25x larger. Only then is the whole catalogue walked again; the CanReuseDemand step re-measures
 * the candidates alone. At the 2% step a CRUISE flight among the ~1 AU-spaced cluster stars walked all 61k points
 * nearly every frame (30.09). */
inline bool CanReuseDemandCandidates(double ObserverTravelCm, double ClosestPointCm)
{
	return FMath::IsFinite(ObserverTravelCm) && ObserverTravelCm >= 0.0
		&& FMath::IsFinite(ClosestPointCm) && ClosestPointCm > ObserverTravelCm
		&& ObserverTravelCm / (ClosestPointCm - ObserverTravelCm) <= 0.25;
}

/** Travel passes of one catalogue source are at least this far apart. A pass that changes a point restarts the
 * source's HISM tree build (~2 ms on the game thread for 25-36k instances) and later its scene proxy (~1 ms); in
 * CRUISE among the cluster stars the size budget ran out every frame, 60-70 rebuilds a second (30.09). Within the
 * interval a glyph can lag its distance by the travel of 0.1 s, e.g. ~10% at 1 AU from a star at 1 AU/s. */
inline constexpr double PointResizeIntervalSeconds = 0.1;

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
	FAPSPreviewProjectedSphere& Sphere, double& OpticalRadiusCm, double SupportPixels = PointSupportPixels)
{
	FAPSContinuousPreviewFrame Frame;
	Frame.FarEnvelopeCm = FarEnvelopeCm;
	if (!FMath::IsFinite(PixelTangent) || PixelTangent <= 0.0
		|| !Frame.ProjectSphere(FromObserverCm, RadiusCm, Sphere)) return false;
	OpticalRadiusCm = GetFullScalePointRadius(Sphere.Center.Size(), Sphere.Radius, PixelTangent, SupportPixels);
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
