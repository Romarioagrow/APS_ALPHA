#pragma once
#include "APSPlanetCloudWeather.h"

// V29-only presentation policy. Callers must explicitly select its candidate
// material; the accepted climate resolver and its single-layer result stay intact.
namespace APSPlanetCloudLayers
{
struct FDeck
{
    float BottomKm = 0;
    float ThicknessKm = 0;
    float Coverage = 0;
    float Density = 0;
};

struct FStack
{
    bool Enabled = false;
    FDeck Low, Middle, High;

    float BoundsBottomKm() const { return Enabled ? Low.BottomKm : 0.f; }
    float BoundsTopKm() const { return Enabled ? High.BottomKm + High.ThicknessKm : 0.f; }
};

inline FStack Resolve(const APSPlanetCloudWeather::FWeather& Base, double AtmosphereKm)
{
    using APSPlanetCloudWeather::ECondensate;
    FStack Stack;
    if (!Base.Enabled || (Base.Condensate != ECondensate::Water && Base.Condensate != ECondensate::Ice)
        || !FMath::IsFinite(AtmosphereKm) || AtmosphereKm <= 0.
        || !FMath::IsFinite(Base.BottomKm) || Base.BottomKm <= 0.f
        || !FMath::IsFinite(Base.ThicknessKm) || Base.ThicknessKm <= 0.f
        || !FMath::IsFinite(Base.Coverage) || Base.Coverage <= 0.f
        || !FMath::IsFinite(Base.Density) || Base.Density <= 0.f)
        return Stack;

    // Physical kilometres, deliberately independent of planet radius. Altitude
    // and climate controls have already been resolved into Base. These are
    // sea-level shells, not a promise to clear every mountain; scene depth clips
    // opaque terrain and ships in the common volume integral.
    const float T = Base.ThicknessKm;
    const float LowThickness = .8f * T;
    const float MiddleThickness = 1.5f * T;
    const float HighThickness = FMath::Clamp(.25f * T, .25f, .8f);
    const float MiddleBottom = Base.BottomKm + LowThickness + FMath::Max(.5f, .35f * T);
    const float HighBottom = MiddleBottom + MiddleThickness + FMath::Max(.8f, .6f * T);
    const float Top = HighBottom + HighThickness;
    // A thin atmosphere uses the unchanged single layer. Compressing all three
    // into the remaining height would merge decks or manufacture dense sheets.
    if (LowThickness < .2f || MiddleThickness < .4f || HighThickness >= LowThickness
        || !FMath::IsFinite(Top) || double(Top) > .85 * AtmosphereKm
        || Base.BottomKm + LowThickness <= Base.BottomKm
        || MiddleBottom <= Base.BottomKm + LowThickness
        || MiddleBottom + MiddleThickness <= MiddleBottom
        || HighBottom <= MiddleBottom + MiddleThickness || Top <= HighBottom)
        return Stack;

    const float Coverage = FMath::Clamp(Base.Coverage, 0.f, 1.f);
    const float Density = FMath::Clamp(Base.Density, 0.f, 3.f);
    const auto Deck = [T, Coverage, Density](float Bottom, float Thickness, float CoverageFactor, float MassShare)
    {
        FDeck Result;
        Result.BottomKm = Bottom;
        Result.ThicknessKm = Thickness;
        Result.Coverage = Coverage * CoverageFactor;
        // Repartition nominal density * thickness, rather than tripling the
        // climate's optical column. Coverage/shape still determine actual mass.
        // Bound each shape factor even for independently constructed FWeather.
        Result.Density = Density * FMath::Clamp(MassShare * T / Thickness, 0.f, 1.f);
        return Result;
    };
    Stack.Low = Deck(Base.BottomKm, LowThickness, .90f, .32f);
    Stack.Middle = Deck(MiddleBottom, MiddleThickness, 1.f, .60f);
    Stack.High = Deck(HighBottom, HighThickness, .55f, .08f);
    for (const auto* Layer : { &Stack.Low, &Stack.Middle, &Stack.High })
        if (Layer->Coverage <= 0.f || Layer->Density <= 0.f) return {};
    Stack.Enabled = true;
    return Stack;
}
}
