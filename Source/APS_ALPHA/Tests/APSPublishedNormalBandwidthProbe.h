#pragma once

#if WITH_DEV_AUTOMATION_TESTS
#include "CoreMinimal.h"
#include "APS_ALPHA/Generation/APSNativeGlobeSnapshot.h"
#include "APS_ALPHA/Generation/APSPlanetReliefField.h"
#include "APS_ALPHA/Core/Planetary/APSWorldScapeSurfaceEnvelope.h"
#include "Misc/Crc.h"
#include "WorldScapeRoot.h"
#include "WorldScapeMeshComponent.h"

// Call once after the caller's settled hold. GT-published CPU copies only;
// no worker LodSize/RelativePosition/Normals, mesh copy, material or asset writes.
// Finite differences at four fixed physical steps are NOT band-limited normals.
namespace APSPublishedNormalBandwidthProbe
{
    namespace Private
    {
        struct FStats
        {
            int32 Count = 0;
            double Min = TNumericLimits<double>::Max(), Max = -TNumericLimits<double>::Max();
            double Sum = 0.0, SumSquared = 0.0;
            void Add(double V)
            { ++Count; Min = FMath::Min(Min, V); Max = FMath::Max(Max, V); Sum += V; SumSquared += V * V; }
            FString Text() const
            { return FString::Printf(TEXT("n=%d min=%.6f max=%.6f mean=%.6f rms=%.6f"),
                Count, Min, Max, Sum / Count, FMath::Sqrt(SumSquared / Count)); }
        };
        struct FSample { FVector3d Direction, Normal; double HeightCm = 0.0; int32 Index = 0; };
        struct FLod
        {
            TArray<FSample> Samples;
            FString MeshPath;
            int32 Vertices = 0, Triangles = 0;
            uint32 SampleHash = 0;
            FStats HeightError, NativeSlope, ReferenceSlope[4], NativeError[4];
        };
        inline double Angle(const FVector3d& A, const FVector3d& B)
        { return FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(FVector3d::DotProduct(A, B), -1.0, 1.0))); }
        template<class T> inline void Hash(uint32& H, const T& Value)
        { H = HashCombineFast(H, FCrc::MemCrc32(&Value, sizeof(Value))); }
    }

    inline bool Capture(AWorldScapeRoot* Root, FString& Error)
    {
        Error.Reset();
        const auto Fail = [&](const FString& Why)
        {
            Error = Why;
            UE_LOG(LogTemp, Error, TEXT("[APS.NormalBandwidth] complete=0 noAggregateResult=1 reason=%s"), *Error);
            return false;
        };
        if (!IsInGameThread() || !IsValid(Root) || !Root->GetWorld())
            return Fail(TEXT("Requires valid root/world on game thread"));
        auto* Body = Cast<APlanetaryBody>(Root->GetOwner());
        auto* Generator = IsValid(Body) ? Body->PlanetaryEnvironmentGenerator : nullptr;
        if (!IsValid(Generator) || Generator->WorldScapeRootInstance != Root || Body->GetWorld() != Root->GetWorld())
            return Fail(TEXT("Root owner/body/factory identity mismatch"));
        if (!Body->bWorldScapeSurfaceReady || Root->IsHidden() || !Root->WorldScapeLodInGeneration.IsEmpty())
            return Fail(TEXT("Requires ready body, visible root and zero GT-tracked incomplete LOD jobs after settled hold"));
        APSClosedGlobeMesh::FSamplingFrame Frame;
        APSClosedGlobeMesh::FBuildOptions Options;
        uint32 Signature = 0;
        if (!APSNativeGlobeSnapshot::Capture(Body, Generator, Frame, Options, Signature, Error))
            return Fail(TEXT("Native snapshot refused: ") + Error);
        const double Sea = double(Frame.Profile.OceanLevel) * Frame.NoiseIntensity;
        const FTransform RootFrame = Root->GetActorTransform();
        if (!Signature || RootFrame.ContainsNaN() || !RootFrame.GetRotation().IsNormalized()
            || !RootFrame.GetScale3D().Equals(FVector::OneVector, 1.e-9)
            || !FMath::IsFinite(Frame.Radius) || Frame.Radius <= 0.0
            || !FMath::IsFinite(Frame.NoiseScale) || Frame.NoiseScale < 1.0
            || !FMath::IsFinite(Frame.NoiseIntensity) || Frame.NoiseIntensity < 0.0
            || !FMath::IsFinite(Frame.OceanHeight) || !FMath::IsFinite(Sea) || Frame.PresentationScale != 1.0)
            return Fail(TEXT("Nonfinite/invalid physical radius, noise, sea or unit root frame"));
        constexpr double HalfStepsCm[] = {1500.0, 6000.0, 30000.0, 120000.0};
        if (HalfStepsCm[3] > Frame.Radius / APSPlanetReliefField::MinimumResolution)
            return Fail(TEXT("Radius is too small for the fixed 1200m derivative half-step"));

        Private::FLod Lods[3];
        // Copy at most 96 selected values before doing any height calculations.
        for (int32 L = 0; L < 3; ++L)
        {
            auto* Lod = Root->WorldScapeLod.IsValidIndex(L) ? Root->WorldScapeLod[L] : nullptr;
            auto* Mesh = IsValid(Lod) ? Lod->Mesh : nullptr;
            if (!IsValid(Mesh) || Mesh->GetWorld() != Root->GetWorld() || Mesh->GetOwner() != Root
                || !Mesh->IsRegistered() || !Mesh->IsVisible() || Mesh->bHiddenInGame)
                return Fail(FString::Printf(TEXT("LOD%d missing/unpublished/hidden/foreign mesh"), L));
            const auto* Section = Mesh->GetProcMeshSection(0);
            if (!Section || !Section->bSectionVisible || Section->PlanetVertexBuffer.Num() < 3
                || Section->PlanetIndexBuffer.Num() < 3 || Section->PlanetIndexBuffer.Num() % 3 != 0)
                return Fail(FString::Printf(TEXT("LOD%d main section is absent, hidden or malformed"), L));
            const FTransform MeshFrame = Mesh->GetComponentTransform();
            const FVector Scale = MeshFrame.GetScale3D();
            if (MeshFrame.ContainsNaN() || !MeshFrame.GetRotation().IsNormalized()
                || FMath::Abs(Scale.X) < UE_SMALL_NUMBER || FMath::Abs(Scale.Y) < UE_SMALL_NUMBER
                || FMath::Abs(Scale.Z) < UE_SMALL_NUMBER)
                return Fail(FString::Printf(TEXT("LOD%d mesh frame is nonfinite/singular"), L));
            const FVector InverseScale(1.0 / Scale.X, 1.0 / Scale.Y, 1.0 / Scale.Z);
            auto& S = Lods[L]; S.MeshPath = Mesh->GetPathName();
            S.Vertices = Section->PlanetVertexBuffer.Num(); S.Triangles = Section->PlanetIndexBuffer.Num() / 3;
            const int32 Count = FMath::Min(32, S.Vertices); S.Samples.Reserve(Count);
            for (int32 I = 0; I < Count; ++I)
            {
                // For N>=Count this spans [0,N-1] with strictly increasing unique indices.
                const int32 Index = int32(int64(I) * (S.Vertices - 1) / (Count - 1));
                const auto& V = Section->PlanetVertexBuffer[Index];
                if (V.Position.ContainsNaN() || V.Normal.ContainsNaN() || V.Normal.IsNearlyZero())
                    return Fail(FString::Printf(TEXT("LOD%d vertex%d has invalid position/normal"), L, Index));
                const FVector3d Ecef = RootFrame.InverseTransformPositionNoScale(MeshFrame.TransformPosition(V.Position));
                // Inverse transpose for mesh normal, then inverse ROOT ROTATION (no translation).
                const FVector WorldNormal = MeshFrame.GetRotation().RotateVector(V.Normal * InverseScale).GetSafeNormal();
                const FVector3d Native = RootFrame.InverseTransformVectorNoScale(WorldNormal).GetSafeNormal();
                const double Radius = Ecef.Size();
                if (Ecef.ContainsNaN() || Native.ContainsNaN() || Native.IsNearlyZero()
                    || !FMath::IsFinite(Radius) || Radius <= 0.0)
                    return Fail(FString::Printf(TEXT("LOD%d vertex%d has invalid transformed ECEF sample"), L, Index));
                Private::FSample VOut; VOut.Index = Index; VOut.Direction = Ecef / Radius;
                VOut.Normal = Native; VOut.HeightCm = Radius - Frame.Radius; S.Samples.Add(VOut);
                Private::Hash(S.SampleHash, Index); Private::Hash(S.SampleHash, Ecef); Private::Hash(S.SampleHash, Native);
            }
        }

        CustomNoise Noise = Frame.SeededNoise; // Value-only snapshot; never call root GetGroundHeight/GetNoise.
        int32 HeightCalls = 0, Samples = 0;
        const auto Height = [&](const FVector3d& D, double& H)
        {
            if (++HeightCalls > 2400) return false;
            H = UAPSWorldScapePlanetNoise::SampleHeightResolvedProfile(Frame.Profile, Noise,
                DVector(D * Frame.Radius), DVector(0.0, 0.0, 0.0), Frame.NoiseScale,
                Frame.NoiseIntensity, Frame.Radius, D.Z, Frame.bCoastalReliefCandidate);
            H = APSWorldScapeSurfaceEnvelope::Height(H, Sea, Frame.bApplyNativeLavaEnvelope);
            return FMath::IsFinite(H) && Frame.Radius + H > 0.0;
        };
        for (int32 L = 0; L < 3; ++L)
            for (const auto& V : Lods[L].Samples)
            {
                double H = 0.0;
                if (!Height(V.Direction, H))
                    return Fail(FString::Printf(TEXT("LOD%d vertex%d canonical height invalid or evaluation budget exceeded"), L, V.Index));
                auto& S = Lods[L]; S.HeightError.Add(V.HeightCm - H);
                S.NativeSlope.Add(Private::Angle(V.Normal, V.Direction)); ++Samples;
                for (int32 Step = 0; Step < 4; ++Step)
                {
                    FVector3f Reference;
                    if (!APSPlanetReliefField::EvaluateNormal(V.Direction, Frame.Radius, HalfStepsCm[Step], Height, Reference))
                        return Fail(FString::Printf(TEXT("LOD%d vertex%d halfStepM=%.0f canonical derivative invalid; heightCalls=%d"),
                            L, V.Index, HalfStepsCm[Step] / 100.0, HeightCalls));
                    const FVector3d N = FVector3d(Reference).GetSafeNormal();
                    if (N.ContainsNaN() || N.IsNearlyZero()) return Fail(TEXT("Canonical derivative returned invalid unit normal"));
                    S.ReferenceSlope[Step].Add(Private::Angle(N, V.Direction));
                    S.NativeError[Step].Add(Private::Angle(V.Normal, N));
                }
            }
        uint32 Inputs = Signature;
        Private::Hash(Inputs, Frame.Radius); Private::Hash(Inputs, Frame.NoiseScale); Private::Hash(Inputs, Frame.NoiseIntensity);
        Private::Hash(Inputs, Sea); Private::Hash(Inputs, Frame.bCoastalReliefCandidate); Private::Hash(Inputs, Frame.bApplyNativeLavaEnvelope);
        UE_LOG(LogTemp, Display, TEXT("[APS.NormalBandwidth] complete=1 root=%s signature=%08x inputsHash=%08x profileHash=%08x terrainSeed=%d biomeSeed=%d radiusCm=%.9g noiseScale=%.9g intensityCm=%.9g seaCm=%.9g coastal=%d lavaEnvelope=%d samples=%d heightCalls=%d maxHeightCalls=2400 scope=GT-published-CPU-main-section-only lightIndependent=1 GPUReadback=0 performanceMeasured=0 visualAcceptance=unassessed halfStepsM=15/60/300/1200 notLowPass=1 heightError=published-minus-canonical"),
            *Root->GetPathName(), Signature, Inputs, UAPSPlanetSurfaceProfileResolver::BuildProfileSignature(Frame.Profile),
            Frame.Profile.TerrainSeed, Frame.Profile.BiomeSeed, Frame.Radius, Frame.NoiseScale, Frame.NoiseIntensity,
            Sea, int32(Frame.bCoastalReliefCandidate), int32(Frame.bApplyNativeLavaEnvelope), Samples, HeightCalls);
        for (int32 L = 0; L < 3; ++L)
        {
            const auto& S = Lods[L];
            UE_LOG(LogTemp, Display, TEXT("[APS.NormalBandwidth.LOD] lod=%d mesh=%s vertices=%d triangles=%d samples=%d sampleHash=%08x nativeSlopeDeg={%s} heightErrorCm={%s}"),
                L, *S.MeshPath, S.Vertices, S.Triangles, S.Samples.Num(), S.SampleHash, *S.NativeSlope.Text(), *S.HeightError.Text());
            for (int32 Step = 0; Step < 4; ++Step)
                UE_LOG(LogTemp, Display, TEXT("[APS.NormalBandwidth.Step] lod=%d halfStepM=%.0f referenceSlopeDeg={%s} nativeVsReferenceDeg={%s}"),
                    L, HalfStepsCm[Step] / 100.0, *S.ReferenceSlope[Step].Text(), *S.NativeError[Step].Text());
        }
        return true;
    }
}
#endif
