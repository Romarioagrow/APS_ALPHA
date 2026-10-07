#pragma once

#include "CoreMinimal.h"

/** Physical preview distances: close-up belongs to the body, zoom-out to its family. */
struct FAPSPreviewCameraBounds
{
	double MinimumCm{1.0};
	double MaximumCm{30.0};

	// Planet close-ups zoom the distance above the surface, not the enormous
	// centre distance. Offset=0 preserves the stellar/system/transition law.
	static double ApplyWheel(const double DistanceCm, const double SurfaceOffsetCm,
		const double WheelDelta, const double MinimumCm, const double MaximumCm)
	{
		if (!FMath::IsFinite(DistanceCm) || !FMath::IsFinite(WheelDelta)
			|| !FMath::IsFinite(SurfaceOffsetCm) || SurfaceOffsetCm < 0.0
			|| !FMath::IsFinite(MinimumCm) || !FMath::IsFinite(MaximumCm)
			|| MinimumCm <= SurfaceOffsetCm || MaximumCm < MinimumCm) return DistanceCm;
		const double Clearance = FMath::Max(DistanceCm - SurfaceOffsetCm, MinimumCm - SurfaceOffsetCm);
		return FMath::Clamp(SurfaceOffsetCm + Clearance * FMath::Pow(0.82, FMath::Clamp(WheelDelta, -1024.0, 1024.0)),
			MinimumCm, MaximumCm);
	}

	static double WheelDeltaBetween(const double FromCm, const double ToCm, const double SurfaceOffsetCm)
	{
		if (!FMath::IsFinite(FromCm) || !FMath::IsFinite(ToCm) || !FMath::IsFinite(SurfaceOffsetCm)
			|| SurfaceOffsetCm < 0.0 || FromCm <= SurfaceOffsetCm || ToCm <= SurfaceOffsetCm) return 0.0;
		return FMath::Loge((ToCm - SurfaceOffsetCm) / (FromCm - SurfaceOffsetCm)) / FMath::Loge(0.82);
	}

	static FAPSPreviewCameraBounds Calculate(const double BodyRadiusCm,
		const double FamilyEnvelopeCm, const double FitTangent, const double MinimumRatio)
	{
		FAPSPreviewCameraBounds Result;
		if (!FMath::IsFinite(BodyRadiusCm) || BodyRadiusCm <= 0.0) return Result;
		const double Envelope = FMath::IsFinite(FamilyEnvelopeCm)
			? FMath::Max(FamilyEnvelopeCm, BodyRadiusCm) : BodyRadiusCm;
		const double Tangent = FMath::IsFinite(FitTangent) ? FMath::Max(FitTangent, 0.001) : 0.001;
		const double FrameRatio = 1.20 * FMath::Sqrt(1.0 + 1.0 / FMath::Square(Tangent));
		Result.MinimumCm = FMath::Max(1.0, BodyRadiusCm * MinimumRatio);
		Result.MaximumCm = FMath::Max3(Result.MinimumCm,
			BodyRadiusCm * 30.0, Envelope * FrameRatio * 1.25);
		return Result;
	}
};
