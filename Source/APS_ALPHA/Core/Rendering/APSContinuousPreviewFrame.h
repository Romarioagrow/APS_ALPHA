#pragma once

#include "CoreMinimal.h"

/** A sphere in render coordinates, derived from one physical observer. */
struct FAPSPreviewProjectedSphere
{
	FVector Center{FVector::ZeroVector};
	double Radius{0.0};
	double PhysicalDistanceCm{0.0};
	double AppliedScale{1.0};
};

/**
 * Shared observer-relative presentation for every astronomical scope.
 *
 * Canonical positions and radii are physical centimetres. Subtraction happens in
 * double precision before render conversion. Distant centers can be compressed
 * radially into a finite render envelope; their radii receive exactly the same
 * factor, preserving direction and angular radius. This is a view transform, not
 * a modification of the generated world or a per-scope object-size policy.
 */
struct FAPSContinuousPreviewFrame
{
	FVector ObserverCm{FVector::ZeroVector};
	double RenderCmPerPhysicalCm{1.0};
	double FarEnvelopeCm{1.0e9};

	bool IsValid() const
	{
		return !ObserverCm.ContainsNaN()
			&& FMath::IsFinite(RenderCmPerPhysicalCm) && RenderCmPerPhysicalCm > 0.0
			&& FMath::IsFinite(FarEnvelopeCm) && FarEnvelopeCm > 0.0;
	}

	bool ProjectSphere(const FVector& CanonicalCenterCm, const double PhysicalRadiusCm,
		FAPSPreviewProjectedSphere& Out) const
	{
		Out = {};
		if (!IsValid() || CanonicalCenterCm.ContainsNaN()
			|| !FMath::IsFinite(PhysicalRadiusCm) || PhysicalRadiusCm < 0.0)
		{
			return false;
		}
		const FVector Offset = CanonicalCenterCm - ObserverCm;
		const double Distance = Offset.Size();
		const double ScaledDistance = Distance * RenderCmPerPhysicalCm;
		if (!FMath::IsFinite(Distance) || !FMath::IsFinite(ScaledDistance)) return false;
		// Continuous at every distance; no hard far-shell crossing or scope boundary.
		const double Scale = RenderCmPerPhysicalCm / (1.0 + ScaledDistance / FarEnvelopeCm);
		Out.Center = Offset * Scale;
		Out.Radius = PhysicalRadiusCm * Scale;
		Out.PhysicalDistanceCm = Distance;
		Out.AppliedScale = Scale;
		return !Out.Center.ContainsNaN() && FMath::IsFinite(Out.Radius)
			&& FMath::IsFinite(Scale) && Scale > 0.0;
	}

	bool ProjectPosition(const FVector& CanonicalCenterCm, FVector& OutCenter) const
	{
		FAPSPreviewProjectedSphere Sphere;
		const bool bProjected = ProjectSphere(CanonicalCenterCm, 0.0, Sphere);
		OutCenter = Sphere.Center;
		return bProjected;
	}
};

/** Camera orbit remains in canonical space, including during interrupted moves. */
struct FAPSContinuousPreviewOrbit
{
	FVector CenterCm{FVector::ZeroVector};
	FVector Outward{FVector(1.0, 1.0, -0.45).GetSafeNormal()};
	double DistanceCm{1.0};

	FVector ObserverCm() const { return CenterCm + Outward * DistanceCm; }

	bool IsValid() const
	{
		return !CenterCm.ContainsNaN() && !Outward.ContainsNaN()
			&& Outward.IsNormalized() && FMath::IsFinite(DistanceCm) && DistanceCm > 0.0
			&& !ObserverCm().ContainsNaN();
	}

	static FAPSContinuousPreviewOrbit Interpolate(const FAPSContinuousPreviewOrbit& From,
		const FAPSContinuousPreviewOrbit& To, const double Alpha)
	{
		if (Alpha <= 0.0) return From;
		if (Alpha >= 1.0) return To;
		const double CenterTravel = FVector::Distance(From.CenterCm, To.CenterCm);
		if (CenterTravel > FMath::Max(From.DistanceCm, To.DistanceCm) * 0.5)
		{
			// Two close-up targets can be separated by an entire planetary orbit.
			// Pull back far enough to connect them visibly before approaching the
			// destination; a close-up lateral pan would eject both bodies immediately.
			FAPSContinuousPreviewOrbit Cruise;
			Cruise.CenterCm = (From.CenterCm + To.CenterCm) * 0.5;
			Cruise.DistanceCm = FMath::Max(FMath::Max(From.DistanceCm, To.DistanceCm), CenterTravel * 2.5);
			Cruise.Outward = FQuat::Slerp(FQuat::Identity,
				FQuat::FindBetweenNormals(From.Outward, To.Outward), 0.5)
				.RotateVector(From.Outward).GetSafeNormal();
			// Each leg spans at most half CenterTravel and has a 2.5x envelope,
			// so neither recursive call can enter this branch again.
			return Alpha < 0.5 ? Interpolate(From, Cruise, Alpha * 2.0)
				: Interpolate(Cruise, To, (Alpha - 0.5) * 2.0);
		}
		const double T = FMath::Clamp(Alpha, 0.0, 1.0);
		const double SmoothT = T * T * (3.0 - 2.0 * T);
		FAPSContinuousPreviewOrbit Result;
		Result.DistanceCm = FMath::Exp(FMath::Lerp(
			FMath::Loge(FMath::Max(From.DistanceCm, 1.0e-12)),
			FMath::Loge(FMath::Max(To.DistanceCm, 1.0e-12)), SmoothT));
		// Couple translation to zoom. Linear center interpolation with logarithmic
		// distance would leave the camera far off target at intermediate deep scales.
		const double DistanceDelta = From.DistanceCm - To.DistanceCm;
		const double CenterAlpha = FMath::Abs(DistanceDelta)
			> FMath::Max(From.DistanceCm, To.DistanceCm) * 1.0e-8
			? FMath::Clamp((From.DistanceCm - Result.DistanceCm) / DistanceDelta, 0.0, 1.0)
			: SmoothT;
		Result.CenterCm = FMath::Lerp(From.CenterCm, To.CenterCm, CenterAlpha);
		const FQuat DirectionRotation = FQuat::FindBetweenNormals(From.Outward, To.Outward);
		Result.Outward = FQuat::Slerp(FQuat::Identity, DirectionRotation, SmoothT)
			.RotateVector(From.Outward).GetSafeNormal();
		return Result;
	}
};
