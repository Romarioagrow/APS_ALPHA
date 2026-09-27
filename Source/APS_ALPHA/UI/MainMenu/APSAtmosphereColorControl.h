#pragma once

#include "CoreMinimal.h"

/** UI-only HSV view of linear Rayleigh coefficients; never a serialized tint. */
namespace APSAtmosphereColorControl
{
    inline bool SameBits(const FLinearColor& A, const FLinearColor& B)
    {
        return FMemory::Memcmp(&A, &B, sizeof(FLinearColor)) == 0;
    }

    struct FState
    {
        FLinearColor Source = FLinearColor::Black;
        float Hue = 0.0f;
        float Saturation = 0.0f;
        float Strength = 0.0f;
        bool bValid = true;
        bool bHasSource = false;

        void Read(const FLinearColor& Coefficients)
        {
            if (bHasSource && SameBits(Source, Coefficients)) return;
            Source = Coefficients;
            bHasSource = true;
            bValid = FMath::IsFinite(Source.R) && FMath::IsFinite(Source.G)
                && FMath::IsFinite(Source.B) && Source.R >= 0.0f
                && Source.G >= 0.0f && Source.B >= 0.0f;
            Hue = Saturation = Strength = 0.0f;
            if (!bValid) return; // Display safely, never repair the model by reading it.
            Strength = FMath::Max3(Source.R, Source.G, Source.B);
            if (Strength > 0.0f)
            {
                const FLinearColor Unit(Source.R / Strength, Source.G / Strength,
                    Source.B / Strength, 1.0f);
                const FLinearColor HSV = Unit.LinearRGBToHSV();
                Hue = HSV.R;
                Saturation = HSV.G;
            }
        }

        FLinearColor Swatch() const
        {
            if (!bValid || Strength <= 0.0f) return FLinearColor(0, 0, 0, 1);
            return FLinearColor(Hue, Saturation, 1.0f, 1.0f).HSVToLinearRGB();
        }

        bool Commit(FLinearColor& Coefficients)
        {
            if (!bValid) return false;
            // A hue/saturation edit at zero stores UI intent only, even for -0.
            if (Strength == 0.0f && FMath::Max3(Source.R, Source.G, Source.B) == 0.0f)
                return false;
            const FLinearColor Unit = FLinearColor(Hue, Saturation, 1.0f, 1.0f).HSVToLinearRGB();
            const FLinearColor Result(Unit.R * Strength, Unit.G * Strength,
                Unit.B * Strength, Source.A);
            if (SameBits(Result, Coefficients)) return false;
            Coefficients = Result;
            Source = Result; // Keep the chosen hue even when saturation is zero.
            return true;
        }

        bool SetHue(FLinearColor& Coefficients, float Value)
        {
            Read(Coefficients);
            if (!bValid || !FMath::IsFinite(Value)) return false;
            const float NewHue = FMath::Fmod(FMath::Clamp(Value, 0.0f, 360.0f), 360.0f);
            if (NewHue == Hue) return false;
            Hue = NewHue;
            return Commit(Coefficients);
        }

        bool SetSaturation(FLinearColor& Coefficients, float Value)
        {
            Read(Coefficients);
            if (!bValid || !FMath::IsFinite(Value)) return false;
            const float NewSaturation = FMath::Clamp(Value, 0.0f, 1.0f);
            if (NewSaturation == Saturation) return false;
            Saturation = NewSaturation;
            return Commit(Coefficients);
        }

        bool SetStrength(FLinearColor& Coefficients, float Value)
        {
            Read(Coefficients);
            if (!bValid || !FMath::IsFinite(Value)) return false;
            if (Value == Strength) return false;
            // Only an explicit strength edit uses the existing UI coefficient range.
            // Read and hue/saturation edits preserve legacy strengths above 64.
            Strength = FMath::Clamp(Value, 0.0f, 64.0f);
            return Commit(Coefficients);
        }
    };
}
