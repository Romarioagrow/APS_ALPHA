#pragma once
#include "CoreMinimal.h"
#include <cmath>

// Uniform-only, palette-relative shallow-water response. No material selection,
// geometry, exposure, normal or endpoint colour changes happen here.
namespace APSWaterDepthPalette
{
    inline constexpr float DefaultRelativeBudget = 0.35f;
    inline constexpr float DefaultHalfDepthM = 20.0f;

    inline double Luminance(const FLinearColor& C)
    {
        return double(C.R) * 0.2126 + double(C.G) * 0.7152 + double(C.B) * 0.0722;
    }

    inline bool ResolveStrength(const FLinearColor& Deep, const FLinearColor& Shallow,
        float MaxRelativeLuminanceChange, float& OutStrength)
    {
        OutStrength = 0.0f;
        const auto Valid = [](const FLinearColor& C)
        {
            return FMath::IsFinite(C.R) && FMath::IsFinite(C.G) && FMath::IsFinite(C.B)
                && C.R >= 0.0f && C.G >= 0.0f && C.B >= 0.0f;
        };
        if (!Valid(Deep) || !Valid(Shallow) || !FMath::IsFinite(MaxRelativeLuminanceChange)
            || MaxRelativeLuminanceChange < 0.0f || MaxRelativeLuminanceChange > 1.0f) return false;
        const double A = Luminance(Deep), B = Luminance(Shallow);
        const double Contrast = FMath::Abs(B - A), Floor = FMath::Min(A, B);
        // Equal luminance must not allow an unrestricted hue change. A black
        // endpoint cannot support a relative-light budget either.
        if (Contrast <= 1.e-12 || Floor <= 0.0 || MaxRelativeLuminanceChange == 0.0f) return true;
        const double Limit = FMath::Min(1.0, double(MaxRelativeLuminanceChange) * Floor / Contrast);
        OutStrength = float(Limit);
        if (double(OutStrength) > Limit) OutStrength = std::nextafter(OutStrength, 0.0f);
        // For alpha = legacy + (1-legacy)*strength*shallowDepth and inputs in
        // [0,1], |delta Y| <= budget*min(A,B) <= budget*Ybase. This is only the
        // linear palette term, NOT a bound on lighting, tone mapping or pixels.
        return true;
    }
}
