#pragma once

#include "CoreMinimal.h"

namespace APSWorldScapeReadinessPolicy
{
    inline bool NeedsInitialPayloadValidation(bool bPublished, bool bWorkersInFlight)
    {
        return !bPublished && !bWorkersInFlight;
    }

    // Compare the visible vertices to THIS patch's analytic height field. A real
    // flat basin is valid; an undisplaced sphere standing in for relief is not.
    struct FHeightAgreement
    {
        double MinRender = TNumericLimits<double>::Max();
        double MaxRender = -TNumericLimits<double>::Max();
        double MinExpected = TNumericLimits<double>::Max();
        double MaxExpected = -TNumericLimits<double>::Max();
        double MaxError = 0.0;
        int32 Samples = 0;
        bool bFinite = true;

        void Add(double RenderCm, double ExpectedCm)
        {
            if (!FMath::IsFinite(RenderCm) || !FMath::IsFinite(ExpectedCm))
            {
                bFinite = false;
                return;
            }
            MinRender = FMath::Min(MinRender, RenderCm);
            MaxRender = FMath::Max(MaxRender, RenderCm);
            MinExpected = FMath::Min(MinExpected, ExpectedCm);
            MaxExpected = FMath::Max(MaxExpected, ExpectedCm);
            MaxError = FMath::Max(MaxError, FMath::Abs(RenderCm - ExpectedCm));
            ++Samples;
        }

        bool IsReady() const
        {
            if (!bFinite || Samples < 3 || !FMath::IsFinite(MaxError) || MaxError > 250.0)
                return false; // Preserve the existing 2.5 m absolute error ceiling.
            const double ExpectedRelief = MaxExpected - MinExpected;
            const double RenderRelief = MaxRender - MinRender;
            const double RequiredRelief = FMath::Min(100.0, ExpectedRelief * 0.5);
            return FMath::IsFinite(ExpectedRelief) && FMath::IsFinite(RenderRelief)
                && RenderRelief + 2.0 >= RequiredRelief;
        }
    };
}
