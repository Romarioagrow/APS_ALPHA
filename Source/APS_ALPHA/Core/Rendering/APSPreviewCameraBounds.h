#pragma once

#include "CoreMinimal.h"

/** Physical preview distances: close-up belongs to the body, zoom-out to its family. */
struct FAPSPreviewCameraBounds
{
	double MinimumCm{1.0};
	double MaximumCm{30.0};

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
