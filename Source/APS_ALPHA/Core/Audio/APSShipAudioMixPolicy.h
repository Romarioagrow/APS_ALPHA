#pragma once

#include "CoreMinimal.h"

// Provisional local-player mix; listening is still required. No assets, input
// bindings, movement, RNG or temporal state. The audio owner smooths transitions.
namespace APSShipAudioMix
{
    inline constexpr float MaximumTotalGain = 1.4f;

    struct FInput
    {
        bool bRunning = false;
        float ForwardDemand = 0.f;
        float LateralDemand = 0.f;
        float VerticalDemand = 0.f;
        float AppliedBoost = 0.f;
        int32 Band = 0; // Maneuver, Flight, Orbital, Cruise, Stellar; no ship dependency.
    };

    struct FOutput
    {
        float IdleGain = 0.f;
        float ForwardGain = 0.f;
        float LiftGain = 0.f;
        float BoostGain = 0.f;
        float WarpGain = 0.f;
        float IdlePitch = 1.f;
        float ForwardPitch = 1.f;
        float LiftPitch = 1.f;
        float BoostPitch = 1.f;
        float WarpPitch = 1.f;

        float TotalGain() const { return IdleGain + ForwardGain + LiftGain + BoostGain + WarpGain; }
    };

    inline FOutput Resolve(FInput Input)
    {
        FOutput Out;
        if (!Input.bRunning) return Out;
        const auto Demand = [](float Value)
        { return FMath::IsFinite(Value) ? FMath::Clamp(Value, -1.f, 1.f) : 0.f; };
        const auto Pitch = [](float Semitones)
        { return FMath::Clamp(FMath::Pow(2.f, Semitones / 12.f), .65f, 1.5f); };
        const float Forward = Demand(Input.ForwardDemand);
        const float Side = FMath::Abs(Demand(Input.LateralDemand));
        const float Vertical = Demand(Input.VerticalDemand);
        const float Boost = FMath::IsFinite(Input.AppliedBoost) ? FMath::Clamp(Input.AppliedBoost, 0.f, 1.f) : 0.f;
        const float Band = static_cast<float>(FMath::Clamp(Input.Band, 0, 4));
        const float Mode = Band * .25f;
        const float Reverse = FMath::Max(-Forward, 0.f);
        const float ForwardLoad = FMath::Clamp(FMath::Max(Forward, 0.f) + .75f * Reverse + .35f * Side, 0.f, 1.f);
        const float LiftLoad = FMath::Abs(Vertical) * (Vertical < 0.f ? .85f : 1.f);
        const float WarpMode = FMath::Max(0.f, (Band - 2.f) * .5f);

        Out.IdleGain = .22f + .025f * Mode;
        Out.ForwardGain = (.56f + .04f * Mode) * ForwardLoad;
        Out.LiftGain = (.45f + .025f * Mode) * LiftLoad;
        Out.BoostGain = .58f * Boost; // A distinct layer even when held at rest.
        Out.WarpGain = .22f * WarpMode * FMath::Max(ForwardLoad, Boost);
        Out.IdlePitch = Pitch(-2.f + .25f * Band);
        Out.ForwardPitch = Pitch(-1.f + .35f * Band + 1.5f * ForwardLoad - Reverse);
        Out.LiftPitch = Pitch(-4.f + .25f * Band + LiftLoad - (Vertical < 0.f ? 1.f : 0.f));
        Out.BoostPitch = Pitch(1.f + .25f * Band + Boost);
        Out.WarpPitch = Pitch(-3.f + .3f * Band + ForwardLoad);

        // Budget simultaneous target gains, not waveform peaks or fade tails.
        // The playback owner must manage overlap and audition the final loudness.
        // A tiny margin keeps float roundoff below the public ceiling.
        const float Total = Out.TotalGain();
        if (Total > MaximumTotalGain)
        {
            const float Scale = (MaximumTotalGain - .00001f) / Total;
            Out.IdleGain *= Scale;
            Out.ForwardGain *= Scale;
            Out.LiftGain *= Scale;
            Out.BoostGain *= Scale;
            Out.WarpGain *= Scale;
        }
        return Out;
    }
}
