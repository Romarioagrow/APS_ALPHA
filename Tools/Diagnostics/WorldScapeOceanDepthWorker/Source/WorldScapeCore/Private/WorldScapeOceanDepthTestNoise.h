#pragma once

#include "WorldScapeNoise/Public/WorldScapeNoiseClass.h"
#include "WorldScapeOceanDepthTestNoise.generated.h"

// Private isolated-host fixture, not an APS terrain replacement or a saved asset.
UCLASS()
class UWorldScapeOceanDepthTestNoise : public UWorldScapeNoiseClass
{
    GENERATED_BODY()
public:
    int32 GroundCalls = 0;
    int32 OceanCalls = 0;

    static double GroundHeight(const DVector& P) { return 500.0 + P.X * .002 - P.Y * .001; }
    static double SeaHeight(const DVector& P) { return 850.0 + P.Y * .0001; }

    virtual FNoiseData GetNoise(CustomNoise Noise, const DVector& P, const DVector& Planet,
        double Scale, double Intensity, double Radius, bool Flat, double Latitude,
        DVector& NoisePosition, FNoiseData Previous, bool Planetary) override
    {
        ++GroundCalls;
        FNoiseData Data;
        Data.Height = GroundHeight(P);
        Data.HeightNormalize = .2f; Data.Temperature = .4f; Data.Humidity = .6f;
        return Data;
    }

    virtual FNoiseData GetOceanNoise(CustomNoise Noise, const DVector& P, const DVector& Planet,
        double Scale, double Intensity, double Radius, bool Flat, double Latitude,
        DVector& NoisePosition, FNoiseData Previous, bool Planetary) override
    {
        ++OceanCalls;
        FNoiseData Data;
        Data.Height = SeaHeight(P);
        Data.HeightNormalize = .3f; Data.Temperature = .5f; Data.Humidity = .7f;
        return Data;
    }
};
