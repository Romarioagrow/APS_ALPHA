#include "APSPlanetReliefField.h"
#include "APSWorldScapePlanetNoise.h"
#include "APS_ALPHA/Core/Planetary/APSWorldScapeSurfaceEnvelope.h"
#include "Async/ParallelFor.h"

namespace APSPlanetReliefField
{
    bool DirectionAt(int32 Face, int32 X, int32 Y, int32 Resolution,
        FIntVector& Address, FVector3d& Direction)
    {
        if (Face < 0 || Face >= 6 || Resolution < MinimumResolution || Resolution > MaximumResolution
            || X < 0 || X > Resolution || Y < 0 || Y > Resolution) return false;
        const int32 U = 2 * X - Resolution, V = 2 * Y - Resolution;
        const FIntVector Addresses[] = {
            {Resolution, U, V}, {-Resolution, -U, V}, {-U, Resolution, V},
            {U, -Resolution, V}, {U, V, Resolution}, {-U, V, -Resolution}};
        Address = Addresses[Face];
        Direction = FVector3d(Address.X, Address.Y, Address.Z).GetSafeNormal();
        return !Direction.ContainsNaN() && !Direction.IsNearlyZero();
    }

    bool EvaluateNormal(const FVector3d& Direction, double RadiusCm, double HalfStepCm,
        TFunctionRef<bool(const FVector3d&, double&)> Height, FVector3f& OutNormal)
    {
        if (Direction.ContainsNaN() || !FMath::IsFinite(RadiusCm) || RadiusCm <= 0.0
            || !FMath::IsFinite(HalfStepCm) || HalfStepCm <= 0.0 || HalfStepCm > RadiusCm / MinimumResolution
            || !FMath::IsNearlyEqual(Direction.SizeSquared(), 1.0, 1.e-10)) return false;
        const double Angle = HalfStepCm / RadiusCm;
        const double Sin = FMath::Sin(Angle), Cos = FMath::Cos(Angle);
        if (!FMath::IsFinite(Sin) || Sin <= 0.0) return false;
        FVector3d Derivatives[3] = {FVector3d::ZeroVector, FVector3d::ZeroVector, FVector3d::ZeroVector};
        for (int32 Axis = 0; Axis < 3; ++Axis)
        {
            FVector3d E = FVector3d::ZeroVector; E[Axis] = 1.0;
            const FVector3d Projected = E - Direction * Direction[Axis];
            const double Length = Projected.Size();
            if (Length <= 1.e-12) continue; // Its contribution vanishes continuously at an axis pole.
            const FVector3d T = Projected / Length;
            const FVector3d Plus = Direction * Cos + T * Sin, Minus = Direction * Cos - T * Sin;
            double HPlus = 0.0, HMinus = 0.0;
            if (!Height(Plus, HPlus) || !Height(Minus, HMinus)
                || !FMath::IsFinite(HPlus) || !FMath::IsFinite(HMinus)
                || RadiusCm + HPlus <= 0.0 || RadiusCm + HMinus <= 0.0) return false;
            // Divide positions by R before the cross products; preserve double
            // precision without squaring astronomical centimetre magnitudes.
            Derivatives[Axis] = (Plus * (1.0 + HPlus / RadiusCm)
                - Minus * (1.0 + HMinus / RadiusCm)) * (Length / (2.0 * Sin));
        }
        FVector3d Normal = FVector3d::CrossProduct(Derivatives[1], Derivatives[2]) * Direction.X
            + FVector3d::CrossProduct(Derivatives[2], Derivatives[0]) * Direction.Y
            + FVector3d::CrossProduct(Derivatives[0], Derivatives[1]) * Direction.Z;
        if (Normal.ContainsNaN() || !Normal.Normalize()) return false;
        if (FVector3d::DotProduct(Normal, Direction) < 0.0) Normal = -Normal;
        const FVector3f Result(Normal);
        if (Result.ContainsNaN()) return false;
        OutNormal = Result;
        return true;
    }

    bool Build(const APSClosedGlobeMesh::FSamplingFrame& Frame, const FBuildOptions& Options,
        FFieldData& Out, FString& Error)
    {
        Error.Reset();
        const int32 N = Options.FaceResolution;
        const double Sea = double(Frame.Profile.OceanLevel) * Frame.NoiseIntensity;
        if (!Out.PlanetNormals.IsEmpty() || Out.Metadata.FaceResolution != 0 || Out.Metadata.InputSignature != 0)
        { Error = TEXT("Relief field output must be empty"); return false; }
        if (N < MinimumResolution || N > MaximumResolution || Options.InputSignature == 0
            || !FMath::IsFinite(Frame.Radius) || Frame.Radius <= 0.0
            || !FMath::IsFinite(Frame.NoiseScale) || Frame.NoiseScale < 1.0
            || !FMath::IsFinite(Frame.NoiseIntensity) || Frame.NoiseIntensity < 0.0
            || !FMath::IsFinite(Frame.OceanHeight) || !FMath::IsFinite(Sea)
            || !FMath::IsFinite(Frame.PresentationScale) || Frame.PresentationScale != 1.0
            || !FMath::IsFinite(Options.DerivativeHalfStepCm) || Options.DerivativeHalfStepCm <= 0.0
            || Options.DerivativeHalfStepCm > Frame.Radius / N)
        { Error = TEXT("Relief field needs a full-scale finite snapshot, nonzero identity, resolution16..256 and explicit half-step in (0,R/N]"); return false; }
        const int32 Count = 6 * (N + 1) * (N + 1); // <=396294, including raw seam duplicates.
        TMap<FIntVector, int32> Welded;
        TArray<FVector3d> Directions;
        TArray<int32> Addresses;
        Welded.Reserve(6 * N * N + 2); Directions.Reserve(6 * N * N + 2); Addresses.Reserve(Count);
        for (int32 Face = 0; Face < 6; ++Face)
            for (int32 Y = 0; Y <= N; ++Y)
                for (int32 X = 0; X <= N; ++X)
                {
                    FIntVector Key; FVector3d D;
                    if (!DirectionAt(Face, X, Y, N, Key, D))
                    { Error = TEXT("Relief cube address is invalid"); return false; }
                    if (const int32* Existing = Welded.Find(Key)) Addresses.Add(*Existing);
                    else { const int32 Index = Directions.Add(D); Welded.Add(Key, Index); Addresses.Add(Index); }
                }
        TArray<FVector3f> Normals; Normals.SetNumUninitialized(Directions.Num());
        TArray<uint8> Valid; Valid.Init(1, 6);
        const APSClosedGlobeMesh::FSamplingFrame Snapshot = Frame;
        ParallelFor(6, [&](int32 Worker)
        {
            CustomNoise Noise = Snapshot.SeededNoise;
            const auto Height = [&](const FVector3d& D, double& H)
            {
                H = UAPSWorldScapePlanetNoise::SampleHeightResolvedProfile(Snapshot.Profile, Noise,
                    DVector(D * Snapshot.Radius), DVector(0.0, 0.0, 0.0), Snapshot.NoiseScale,
                    Snapshot.NoiseIntensity, Snapshot.Radius, D.Z, Snapshot.bCoastalReliefCandidate);
                H = APSWorldScapeSurfaceEnvelope::Height(H, Sea, Snapshot.bApplyNativeLavaEnvelope);
                return FMath::IsFinite(H);
            };
            for (int32 I = Directions.Num() * Worker / 6; I < Directions.Num() * (Worker + 1) / 6; ++I)
                if (!EvaluateNormal(Directions[I], Snapshot.Radius, Options.DerivativeHalfStepCm, Height, Normals[I]))
                { Valid[Worker] = 0; break; }
        });
        for (uint8 V : Valid) if (!V)
        { Error = TEXT("Relief height sample or derivative is nonfinite/degenerate; no field published"); return false; }
        FFieldData Result;
        FMetadata& M = Result.Metadata;
        M.FaceResolution = N; M.InputSignature = Options.InputSignature;
        M.PlanetType = Frame.Profile.PlanetType; M.TerrainSeed = Frame.Profile.TerrainSeed; M.BiomeSeed = Frame.Profile.BiomeSeed;
        M.RadiusCm = Frame.Radius; M.NoiseScale = Frame.NoiseScale; M.NoiseIntensity = Frame.NoiseIntensity;
        M.PresentationScale = Frame.PresentationScale; M.OceanHeightCm = Frame.OceanHeight; M.EnvelopeSeaHeightCm = Sea;
        M.DerivativeHalfStepCm = Options.DerivativeHalfStepCm; M.DerivativeHalfAngleRadians = Options.DerivativeHalfStepCm / Frame.Radius;
        M.MaximumTexelArcRadians = 2.0 / N; M.MaximumTexelArcCm = Frame.Radius * M.MaximumTexelArcRadians;
        M.UniqueDirections = Directions.Num(); M.MaximumHeightSamples = int64(Directions.Num()) * 6;
        M.bCoastalReliefCandidate = Frame.bCoastalReliefCandidate; M.bNativeLavaEnvelope = Frame.bApplyNativeLavaEnvelope;
        Result.PlanetNormals.Reserve(Count);
        for (int32 Index : Addresses) Result.PlanetNormals.Add(Normals[Index]);
        Out = MoveTemp(Result);
        return true;
    }
}
