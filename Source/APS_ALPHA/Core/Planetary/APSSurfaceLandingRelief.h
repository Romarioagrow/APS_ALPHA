#pragma once
#include "CoreMinimal.h"

// Hero composition is a preference, not a requirement to make a new world
// playable. Recovery keeps the existing oceanic non-flat relief floor; all
// dry-ground, daylight, slope and cooked-collision gates remain separate.
namespace APSSurfaceLandingRelief
{
struct FRequirements { double NearCm; double MidCm; double FarCm; };
inline FRequirements Gentle() { return {15.0, 150.0, 350.0}; }
inline FRequirements Preferred(bool Oceanic)
{
    return Oceanic ? Gentle() : FRequirements{300.0, 1500.0, 2500.0};
}
inline bool CanRecover(bool FoundPreferred, bool Oceanic, bool NewDaylitStart)
{
    return !FoundPreferred && !Oceanic && NewDaylitStart;
}
inline bool HasRelief(const FRequirements& R, double Near, double Mid, double Far)
{
    return FMath::IsFinite(Near) && FMath::IsFinite(Mid) && FMath::IsFinite(Far)
        && Near >= R.NearCm && Mid >= R.MidCm && Far >= R.FarCm;
}
}
