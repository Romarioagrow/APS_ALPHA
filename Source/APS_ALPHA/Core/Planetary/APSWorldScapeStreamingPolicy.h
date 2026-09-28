#pragma once

#include "CoreMinimal.h"

namespace APSWorldScapeStreamingPolicy
{
    // Retention is a preference, not an absolute lock on an overlapping family.
    // Evaluate every eligible candidate before selecting; infinity/NaN cannot win.
    inline double Score(double CenterDistanceCm, double BodyRadiusCm,
        double EligibilityRadiusCm, bool bCurrent, double CurrentBias)
    {
        if (!FMath::IsFinite(CenterDistanceCm) || !FMath::IsFinite(BodyRadiusCm)
            || !FMath::IsFinite(EligibilityRadiusCm) || !FMath::IsFinite(CurrentBias)
            || CenterDistanceCm < 0.0 || BodyRadiusCm <= 0.0
            || EligibilityRadiusCm <= 0.0 || CenterDistanceCm > EligibilityRadiusCm)
            return TNumericLimits<double>::Max();
        return FMath::Max(0.0, CenterDistanceCm - BodyRadiusCm)
            * (bCurrent ? FMath::Clamp(CurrentBias, 0.01, 1.0) : 1.0);
    }

    inline bool Prefer(double CandidateScore, bool bCurrent, const FString& Key,
        double BestScore, bool bBestCurrent, const FString& BestKey)
    {
        if (!FMath::IsFinite(CandidateScore) || CandidateScore == TNumericLimits<double>::Max())
            return false;
        if (CandidateScore != BestScore) return CandidateScore < BestScore;
        if (bCurrent != bBestCurrent) return bCurrent;
        return BestKey.IsEmpty() || Key.Compare(BestKey, ESearchCase::CaseSensitive) < 0;
    }
}
