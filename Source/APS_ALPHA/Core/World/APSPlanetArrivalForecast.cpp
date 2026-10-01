#include "APSPlanetArrivalForecast.h"

APSPlanetArrivalForecast::FResult APSPlanetArrivalForecast::Evaluate(const FVector& RelativeNow,
	const FVector& RelativeBefore, const double StepSeconds, const double BodyRadiusCm)
{
	FResult Result;
	Result.DistanceCm = RelativeNow.Size();
	if (!(StepSeconds > UE_DOUBLE_SMALL_NUMBER) || RelativeNow.ContainsNaN() || RelativeBefore.ContainsNaN()
		|| !FMath::IsFinite(Result.DistanceCm) || Result.DistanceCm <= UE_DOUBLE_SMALL_NUMBER)
	{
		return Result;
	}
	const FVector Velocity = (RelativeNow - RelativeBefore) / StepSeconds;
	// Negative while the observer closes in.
	const double Along = FVector::DotProduct(RelativeNow, Velocity);
	const double SpeedSquared = Velocity.SizeSquared();
	if (Along >= 0.0 || !(SpeedSquared > UE_DOUBLE_SMALL_NUMBER) || !FMath::IsFinite(SpeedSquared))
	{
		return Result;
	}
	Result.ClosingSpeedCmPerSecond = -Along / Result.DistanceCm;
	Result.MissDistanceCm = (RelativeNow + Velocity * (-Along / SpeedSquared)).Size();
	Result.SecondsToSurface = FMath::Max(Result.DistanceCm - BodyRadiusCm, 0.0) / Result.ClosingSpeedCmPerSecond;
	return Result;
}

bool APSPlanetArrivalForecast::IsArriving(const FResult& Result, const double LeadSeconds, const double ReachCm,
	const bool bCurrent)
{
	if (!(LeadSeconds > 0.0) || !FMath::IsFinite(Result.SecondsToSurface)
		|| Result.SecondsToSurface == TNumericLimits<double>::Max())
	{
		return false;
	}
	const double Loosen = bCurrent ? 1.5 : 1.0;
	return Result.SecondsToSurface <= LeadSeconds * Loosen
		&& Result.MissDistanceCm <= FMath::Max(ReachCm, 0.1 * Result.DistanceCm) * Loosen;
}
