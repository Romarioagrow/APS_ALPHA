#pragma once
#include "CoreMinimal.h"
#include "APSPlanetCloudSettings.generated.h"

/** Multipliers of the resolved climate, not replacements for pressure/chemistry.
 * Tagged saves without this property retain the automatic defaults. */
USTRUCT(BlueprintType)
struct FAPSPlanetCloudSettings
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Clouds")
    double CoverageScale = .5;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Clouds")
    double DensityScale = 1.;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Clouds")
    double FeatureScale = 1.;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Clouds")
    double AltitudeScale = 1.;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Clouds")
    double WindScale = 1.;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Clouds")
    double StormScale = 1.;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Clouds")
    int32 SeedOffset = 0;

    FAPSPlanetCloudSettings Sanitized() const
    {
        FAPSPlanetCloudSettings S = *this;
        const auto Safe=[](double X,double Lo,double Hi)
        { return FMath::IsFinite(X) ? FMath::Clamp(X,Lo,Hi) : 1.; };
        S.CoverageScale=FMath::IsFinite(S.CoverageScale) ? FMath::Clamp(S.CoverageScale,0.,2.) : .5;
        S.DensityScale=Safe(S.DensityScale,0.,2.);
        S.FeatureScale=Safe(S.FeatureScale,.25,3.);
        S.AltitudeScale=Safe(S.AltitudeScale,.25,2.);
        S.WindScale=Safe(S.WindScale,0.,3.);
        S.StormScale=Safe(S.StormScale,0.,2.);
        S.SeedOffset=FMath::Clamp(S.SeedOffset,0,999983);
        return S;
    }
};
