#include "../../../Source/APS_ALPHA/Editor/APSPlanetReliefNormalCode.h"
#include <cstdio>
#include <cstdint>

int main()
{
    using namespace APSPlanetReliefNormalCode;
    if (!ReferenceContractsPass()) return 1;
    uint32_t State = 173;
    const auto Scalar = [&]() { State = State * 1664525u + 1013904223u;
        return double(State) / 4294967295.0 * 2.0 - 1.0; };
    const auto Unit = [&]() { FVector3 V{Scalar(), Scalar(), Scalar()};
        return Scale(V, 1.0 / std::sqrt(Dot(V, V))); };
    double MaximumUnitError = 0.0, MaximumResidualError = 0.0;
    for (int I = 0; I < 5000; ++I)
    {
        const FVector3 Base = Unit(), Target = Unit(), Detail = Unit();
        if (Dot(Base, Target) < -.9999) continue;
        const FVector3 Result = EvaluateReference(Detail, Base, Target, 1.0, 1.0);
        const double UnitError = std::abs(Dot(Result, Result) - 1.0);
        const double ResidualError = std::abs(Dot(Result, Target) - Dot(Detail, Base));
        if (UnitError > MaximumUnitError) MaximumUnitError = UnitError;
        if (ResidualError > MaximumResidualError) MaximumResidualError = ResidualError;
        if (!Finite(Result) || UnitError > 1.e-10 || ResidualError > 1.e-10) return 2;
        const FVector3 Unchanged = EvaluateReference(Detail, Base, Target, 1.0, 0.0);
        if (Unchanged.X != Detail.X || Unchanged.Y != Detail.Y || Unchanged.Z != Detail.Z) return 3;
    }
    std::printf("PASS portable normal transport: 5000 rotations, near identity; maxUnitError=%.3g maxResidualError=%.3g\n",
        MaximumUnitError, MaximumResidualError);
    return 0;
}
