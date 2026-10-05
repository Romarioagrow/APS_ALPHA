#pragma once

#include "CoreMinimal.h"
#include "APS_ALPHA/Generation/APSPlanetReliefField.h"
#include "APS_ALPHA/Generation/APSWorldScapePlanetNoise.h"
#include "APS_ALPHA/Core/Planetary/APSWorldScapeSurfaceEnvelope.h"
#include "Async/ParallelFor.h"
#include "Engine/Texture2D.h"
#include "HAL/PlatformTime.h"
#include "Math/Float16Color.h"
#include "TextureResource.h"

// Isolated static-chart diagnostic. No cache, material binding, saves or waits.
// Samples the captured height field, NOT a substitute GPU noise field. Neither
// finite differences nor this mip chain prove a band-limited slope/mask signal.
namespace APSCanonicalReliefChart
{
    constexpr int32 MaximumResolution = 1024;
    constexpr double MinimumSpacingCm = 750.0, MaximumSpacingCm = 1500.0;

    struct FMetadata
    {
        FVector3d Center = FVector3d::ZeroVector, AxisU = FVector3d::ZeroVector,
            AxisV = FVector3d::ZeroVector;
        int32 Resolution = 0;
        double RadiusCm = 0.0, TexelSpacingCm = 0.0, DerivativeHalfStepCm = 0.0;
        double ChartWidthCm = 0.0, MaximumTexelArcRadians = 0.0;
        double NoiseScale = 0.0, NoiseIntensity = 0.0, EnvelopeSeaHeightCm = 0.0;
        int32 TerrainSeed = 0, BiomeSeed = 0;
        EPlanetType PlanetType = EPlanetType::Unknown;
        bool bCoastalReliefCandidate = false, bNativeLavaEnvelope = false;
        bool bProvenBandLimited = false; // Always false; not an acceptance flag.
        int64 MaximumHeightSamples = 0, ActualHeightSamples = 0;
        double PreparationWallSeconds = 0.0; // Joined CPU preparation, not game/GPU performance.
        // Caller retains full captured-profile/noise identity; these labels are
        // useful provenance, not an inferred hash of CustomNoise internals.
    };
    struct FMip { int32 Size = 0; TArray<FFloat16Color> Pixels; };
    struct FData
    {
        FMetadata Metadata;
        TArray<FMip> Mips; // Row-major +U right, +V down. Signed planet normal RGB, A=1.
        int64 ByteCount = 0;
    };

    namespace Private
    {
        inline bool GeometryValid(const FMetadata& M)
        {
            return M.Resolution >= 1 && M.Resolution <= MaximumResolution
                && FMath::IsPowerOfTwo(M.Resolution)
                && FMath::IsFinite(M.RadiusCm) && M.RadiusCm > 0.0
                && FMath::IsFinite(M.TexelSpacingCm) && M.TexelSpacingCm >= MinimumSpacingCm
                && M.TexelSpacingCm <= MaximumSpacingCm
                && FMath::IsFinite(M.DerivativeHalfStepCm) && M.DerivativeHalfStepCm > 0.0
                && M.DerivativeHalfStepCm <= M.RadiusCm / APSPlanetReliefField::MinimumResolution
                && !M.Center.ContainsNaN() && !M.AxisU.ContainsNaN() && !M.AxisV.ContainsNaN()
                && FMath::IsNearlyEqual(M.Center.SizeSquared(), 1.0, 1.e-8)
                && FMath::IsNearlyEqual(M.AxisU.SizeSquared(), 1.0, 1.e-8)
                && FMath::IsNearlyEqual(M.AxisV.SizeSquared(), 1.0, 1.e-8)
                && FMath::Abs(FVector3d::DotProduct(M.Center, M.AxisU)) <= 1.e-8
                && FMath::Abs(FVector3d::DotProduct(M.Center, M.AxisV)) <= 1.e-8
                && FMath::Abs(FVector3d::DotProduct(M.AxisU, M.AxisV)) <= 1.e-8
                && FVector3d::CrossProduct(M.AxisU, M.AxisV).Equals(M.Center, 1.e-8);
        }
        inline FVector3d Direction(const FMetadata& M, double U, double V)
        {
            const double Width = M.Resolution * M.TexelSpacingCm;
            return (M.Center + (M.AxisU * ((U - .5) * Width)
                + M.AxisV * ((V - .5) * Width)) / M.RadiusCm).GetSafeNormal();
        }
    }

    // Mip texel centres have the SAME extent/orientation as the base chart.
    inline bool TexelDirection(const FMetadata& M, int32 X, int32 Y, int32 MipSize, FVector3d& Out)
    {
        if (!Private::GeometryValid(M) || MipSize < 1 || MipSize > M.Resolution
            || !FMath::IsPowerOfTwo(MipSize) || X < 0 || Y < 0 || X >= MipSize || Y >= MipSize) return false;
        const FVector3d D = Private::Direction(M, (X + .5) / MipSize, (Y + .5) / MipSize);
        if (D.ContainsNaN() || !FMath::IsNearlyEqual(D.SizeSquared(), 1.0, 1.e-10)) return false;
        Out = D; return true;
    }
    inline bool DirectionToUV(const FMetadata& M, const FVector3d& D, FVector2d& Out)
    {
        if (!Private::GeometryValid(M) || D.ContainsNaN()
            || !FMath::IsNearlyEqual(D.SizeSquared(), 1.0, 1.e-8)) return false;
        const double Denominator = FVector3d::DotProduct(D, M.Center);
        if (!FMath::IsFinite(Denominator) || Denominator <= 1.e-12) return false;
        const double Factor = (M.RadiusCm / (M.Resolution * M.TexelSpacingCm)) / Denominator;
        const FVector2d UV(.5 + FVector3d::DotProduct(D, M.AxisU) * Factor,
            .5 + FVector3d::DotProduct(D, M.AxisV) * Factor);
        if (UV.ContainsNaN() || UV.X < 0.0 || UV.X > 1.0 || UV.Y < 0.0 || UV.Y > 1.0) return false;
        Out = UV; return true;
    }

    // Small pure orientation/inverse contract; caller may run without textures.
    inline bool OrientationContractsPass()
    {
        FMetadata M; M.Resolution = 8; M.RadiusCm = 128089600.0;
        M.TexelSpacingCm = 750.0; M.DerivativeHalfStepCm = 1500.0;
        M.Center = FVector3d(1.0, 2.0, 3.0).GetSafeNormal();
        M.AxisU = FVector3d::CrossProduct(FVector3d(0.0, 0.0, 1.0), M.Center).GetSafeNormal();
        M.AxisV = FVector3d::CrossProduct(M.Center, M.AxisU);
        FVector2d UV;
        if (!DirectionToUV(M, M.Center, UV) || !UV.Equals(FVector2d(.5, .5), 1.e-10)
            || DirectionToUV(M, -M.Center, UV)) return false;
        for (int32 Size = 8; Size >= 1; Size /= 2)
            for (int32 Y = 0; Y < Size; ++Y) for (int32 X = 0; X < Size; ++X)
            {
                FVector3d D;
                if (!TexelDirection(M, X, Y, Size, D) || !DirectionToUV(M, D, UV)
                    || !UV.Equals(FVector2d((X + .5) / Size, (Y + .5) / Size), 1.e-9)) return false;
            }
        M.AxisV = -M.AxisV; // Reject a mirrored/inconsistent material chart frame.
        return !Private::GeometryValid(M);
    }

    // Pure CPU value snapshot only. No UObject queries in workers. Empty output
    // is required; ANY error leaves it unchanged. At most 6*N*N height calls.
    inline bool Build(const APSClosedGlobeMesh::FSamplingFrame& Frame,
        const FVector3d& Center, const FVector3d& AxisU, const FVector3d& AxisV,
        int32 Resolution, double TexelSpacingCm, double DerivativeHalfStepCm,
        FData& Out, FString& Error)
    {
        Error.Reset(); const double Started = FPlatformTime::Seconds();
        if (!Out.Mips.IsEmpty() || Out.ByteCount != 0 || Out.Metadata.Resolution != 0)
        { Error = TEXT("Canonical chart requires empty output"); return false; }
        FData Result; FMetadata& M = Result.Metadata;
        M.Center = Center; M.AxisU = AxisU; M.AxisV = AxisV; M.Resolution = Resolution;
        M.RadiusCm = Frame.Radius; M.TexelSpacingCm = TexelSpacingCm;
        M.DerivativeHalfStepCm = DerivativeHalfStepCm;
        const double Sea = double(Frame.Profile.OceanLevel) * Frame.NoiseIntensity;
        if (!Private::GeometryValid(M) || Frame.PresentationScale != 1.0
            || !FMath::IsFinite(Frame.NoiseScale) || Frame.NoiseScale < 1.0
            || !FMath::IsFinite(Frame.NoiseIntensity) || Frame.NoiseIntensity < 0.0
            || !FMath::IsFinite(Frame.OceanHeight) || !FMath::IsFinite(Sea))
        { Error = TEXT("Canonical chart needs full-scale finite frame, orthonormal right-handed C/U/V, power2 N<=1024, spacing7.5..15m and half-step in (0,R/16]"); return false; }
        M.ChartWidthCm = Resolution * TexelSpacingCm;
        M.MaximumTexelArcRadians = TexelSpacingCm / Frame.Radius;
        M.NoiseScale = Frame.NoiseScale; M.NoiseIntensity = Frame.NoiseIntensity; M.EnvelopeSeaHeightCm = Sea;
        M.TerrainSeed = Frame.Profile.TerrainSeed; M.BiomeSeed = Frame.Profile.BiomeSeed;
        M.PlanetType = Frame.Profile.PlanetType; M.bCoastalReliefCandidate = Frame.bCoastalReliefCandidate;
        M.bNativeLavaEnvelope = Frame.bApplyNativeLavaEnvelope;
        M.MaximumHeightSamples = int64(6) * Resolution * Resolution;
        const APSClosedGlobeMesh::FSamplingFrame Snapshot = Frame;
        TArray<FVector3d> Sums; Sums.SetNumUninitialized(Resolution * Resolution);
        TArray<uint8> Valid; Valid.Init(1, Resolution);
        TArray<int64> Calls; Calls.SetNumZeroed(Resolution);
        ParallelFor(Resolution, [&](int32 Y)
        {
            CustomNoise Noise = Snapshot.SeededNoise;
            const auto Height = [&](const FVector3d& D, double& H)
            {
                ++Calls[Y];
                H = UAPSWorldScapePlanetNoise::SampleHeightResolvedProfile(Snapshot.Profile, Noise,
                    DVector(D * Snapshot.Radius), DVector(0.0, 0.0, 0.0), Snapshot.NoiseScale,
                    Snapshot.NoiseIntensity, Snapshot.Radius, D.Z, Snapshot.bCoastalReliefCandidate);
                H = APSWorldScapeSurfaceEnvelope::Height(H, Sea, Snapshot.bApplyNativeLavaEnvelope);
                return FMath::IsFinite(H);
            };
            for (int32 X = 0; X < Resolution; ++X)
            {
                const FVector3d D = Private::Direction(M, (X + .5) / Resolution, (Y + .5) / Resolution);
                FVector3f Normal;
                if (!APSPlanetReliefField::EvaluateNormal(D, Snapshot.Radius, DerivativeHalfStepCm, Height, Normal)
                    || FVector3d::DotProduct(FVector3d(Normal), D) <= 0.0)
                { Valid[Y] = 0; break; }
                // Equal chart-UV box weights. Keep vector sums (not repeatedly
                // renormalised mip normals) for every coarser level.
                Sums[Y * Resolution + X] = FVector3d(Normal);
            }
        });
        for (int32 Y = 0; Y < Resolution; ++Y)
        {
            if (!Valid[Y]) { Error = FString::Printf(TEXT("Canonical chart nonfinite/degenerate sample at row%d; no chart published"), Y); return false; }
            M.ActualHeightSamples += Calls[Y];
        }
        if (M.ActualHeightSamples > M.MaximumHeightSamples)
        { Error = TEXT("Canonical chart height-call budget exceeded"); return false; }
        for (int32 Size = Resolution;; Size /= 2)
        {
            FMip Mip; Mip.Size = Size; Mip.Pixels.Reserve(Size * Size);
            for (int32 Y = 0; Y < Size; ++Y) for (int32 X = 0; X < Size; ++X)
            {
                const FVector3d Sum = Sums[Y * Size + X]; const double Length = Sum.Size();
                const FVector3d D = Private::Direction(M, (X + .5) / Size, (Y + .5) / Size);
                if (Sum.ContainsNaN() || !FMath::IsFinite(Length) || Length < 1.e-12
                    || FVector3d::DotProduct(Sum, D) <= 0.0)
                { Error = TEXT("Canonical chart mip vector mean is degenerate/non-outward"); return false; }
                const FVector3d N = Sum / Length;
                Mip.Pixels.Emplace(FLinearColor(float(N.X), float(N.Y), float(N.Z), 1.0f));
            }
            Result.ByteCount += int64(Mip.Pixels.Num()) * sizeof(FFloat16Color);
            Result.Mips.Add(MoveTemp(Mip));
            if (Size == 1) break;
            const int32 Next = Size / 2; TArray<FVector3d> NextSums; NextSums.SetNumZeroed(Next * Next);
            for (int32 Y = 0; Y < Next; ++Y) for (int32 X = 0; X < Next; ++X)
                for (int32 DY = 0; DY < 2; ++DY) for (int32 DX = 0; DX < 2; ++DX)
                    NextSums[Y * Next + X] += Sums[(2 * Y + DY) * Size + 2 * X + DX];
            Sums = MoveTemp(NextSums);
        }
        M.PreparationWallSeconds = FPlatformTime::Seconds() - Started;
        Out = MoveTemp(Result); return true;
    }

    // GT only. Caller must retain texture immediately (UPROPERTY/strong pointer/
    // MID parameter) and wait asynchronously for render readiness before use.
    // Texture address clamp is NOT chart validity: material must gate its UVs.
    inline UTexture2D* CreateTransient(const FData& Data, FString& Error)
    {
        Error.Reset(); const FMetadata& M = Data.Metadata;
        if (!IsInGameThread() || !Private::GeometryValid(M) || Data.Mips.IsEmpty()
            || M.bProvenBandLimited || M.ChartWidthCm != M.Resolution * M.TexelSpacingCm
            || M.MaximumTexelArcRadians != M.TexelSpacingCm / M.RadiusCm
            || !FMath::IsFinite(M.PreparationWallSeconds) || M.PreparationWallSeconds < 0.0
            || M.MaximumHeightSamples != int64(6) * M.Resolution * M.Resolution
            || M.ActualHeightSamples <= 0 || M.ActualHeightSamples > M.MaximumHeightSamples)
        { Error = TEXT("Canonical chart upload requires GT and validated diagnostic data"); return nullptr; }
        int32 Size = M.Resolution; int64 Bytes = 0;
        for (const FMip& Mip : Data.Mips)
        {
            if (Size < 1 || Mip.Size != Size || Mip.Pixels.Num() != Size * Size)
            { Error = TEXT("Canonical chart mip dimensions/count changed"); return nullptr; }
            for (const FFloat16Color& P : Mip.Pixels)
            {
                const FLinearColor C = P.GetFloats();
                const double L2 = double(C.R) * C.R + double(C.G) * C.G + double(C.B) * C.B;
                if (!FMath::IsFinite(L2) || !FMath::IsNearlyEqual(L2, 1.0, .003) || C.A != 1.0f)
                { Error = TEXT("Canonical chart upload has invalid signed unit-normal payload"); return nullptr; }
            }
            Bytes += int64(Mip.Pixels.Num()) * sizeof(FFloat16Color); Size /= 2;
        }
        if (Size != 0 || Bytes != Data.ByteCount)
        { Error = TEXT("Canonical chart mip chain/byte count changed"); return nullptr; }
        static_assert(sizeof(FFloat16Color) == 8, "PF_FloatRGBA requires four half floats");
        UTexture2D* Texture = UTexture2D::CreateTransient(M.Resolution, M.Resolution, PF_FloatRGBA);
        if (!Texture) { Error = TEXT("Canonical chart Texture2D allocation failed"); return nullptr; }
        Texture->SRGB = false; Texture->NeverStream = true; Texture->Filter = TF_Trilinear;
        Texture->AddressX = TA_Clamp; Texture->AddressY = TA_Clamp;
        Texture->CompressionSettings = TC_HDR; Texture->LODBias = 0;
#if WITH_EDITORONLY_DATA
        Texture->MipGenSettings = TMGS_LeaveExistingMips;
#endif
        FTexturePlatformData* Platform = Texture->GetPlatformData();
        if (!Platform || Platform->Mips.Num() != 1)
        { Error = TEXT("Canonical chart transient platform-data contract changed"); return nullptr; }
        for (int32 I = 0; I < Data.Mips.Num(); ++I)
        {
            const FMip& Source = Data.Mips[I];
            if (I) Platform->Mips.Add(new FTexture2DMipMap(Source.Size, Source.Size, 1));
            FTexture2DMipMap& Mip = Platform->Mips[I];
            const int64 SizeBytes = int64(Source.Pixels.Num()) * sizeof(FFloat16Color);
            Mip.BulkData.Lock(LOCK_READ_WRITE);
            void* Destination = Mip.BulkData.Realloc(SizeBytes);
            FMemory::Memcpy(Destination, Source.Pixels.GetData(), SizeBytes); Mip.BulkData.Unlock();
        }
        // No Source art: supplied platform mips remain the upload source.
        Texture->UpdateResource(); return Texture; // Enqueued, NOT render-ready.
    }
}
