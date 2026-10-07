#pragma once
#include "CoreMinimal.h"

namespace APSWorldScapeFlightPolicy
{
    // Physical centimetres, relative to the body's centre. A common origin shift
    // cancels before velocity is measured; pawn velocity is unreliable for kinematic ships.
    inline bool WantsTransit(const FVector& RelativePosition, const FVector& RelativeVelocity,
        double Radius, bool bTransit)
    {
        if (RelativePosition.ContainsNaN() || RelativeVelocity.ContainsNaN()
            || !FMath::IsFinite(Radius) || Radius <= 0) return false;
        const double Distance = RelativePosition.Size();
        if (!FMath::IsFinite(Distance) || Distance <= Radius) return false;
        const double Height = Distance - Radius;
        const double NearHeight = FMath::Max(200.e5, Radius * 0.20);
        const double FarHeight = FMath::Max(500.e5, Radius * 0.35);
        if (Height <= NearHeight) return false; // Speed never removes nearby ground.
        const double SpeedSquared = RelativeVelocity.SizeSquared();
        if (!FMath::IsFinite(SpeedSquared)) return false;
        // Predict the closest point along the next eight seconds, including a
        // tangential approach. Start rebuilding before the inner boundary is crossed.
        const double ClosestTime = SpeedSquared > 1.0
            ? FMath::Clamp(-FVector::DotProduct(RelativePosition, RelativeVelocity) / SpeedSquared, 0.0, 8.0)
            : 0.0;
        if ((RelativePosition + RelativeVelocity * ClosestTime).Size() - Radius <= NearHeight)
            return false;
        if (bTransit) return true; // Distance hysteresis survives braking in high orbit.
        const double RadialSpeed = FVector::DotProduct(RelativePosition / Distance, RelativeVelocity);
        return Height >= FarHeight && (RadialSpeed >= 50000.0 || Height >= Radius);
    }
}
