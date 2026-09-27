#pragma once
#if WITH_DEV_AUTOMATION_TESTS
#include "Materials/MaterialInstanceDynamic.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "ProceduralMeshComponent.h"
#include "UObject/StrongObjectPtr.h"

// Test-only 2x2 isolation of the existing elevation/cold snow-transfer controls.
// Optional contribution mode codes palette endpoints for measurement only.
// No payload, graph, texture, normal, geometry or saved asset modifications.
namespace APSTundraLayerTransfer
{
    inline bool NormalRequested() { return FParse::Param(FCommandLine::Get(), TEXT("APSProbeTundraNormalContinuity")); }
    inline bool ClimateRequested() { return FParse::Param(FCommandLine::Get(), TEXT("APSProbeTundraClimateTransfer")); }
    inline bool ContributionsRequested() { return FParse::Param(FCommandLine::Get(), TEXT("APSProbeTundraLayerContributions")); }
    inline bool Requested() { return NormalRequested() || ClimateRequested() || ContributionsRequested() || FParse::Param(FCommandLine::Get(), TEXT("APSProbeTundraLayerTransfer")); }
    // Read the actual uploaded UNorm8 payload, not a newly sampled substitute.
    // Quantiles count mesh vertices, not surface area. No geometry/material writes.
    inline void LogPayload(const FProcMeshSection& Section)
    {
        int32 Histogram[4][256] = {};
        double Sums[4] = {};
        int32 WarmGate = 0;
        for (const auto& Vertex : Section.ProcVertexBuffer)
        {
            const uint8 Values[4] = {Vertex.Color.R, Vertex.Color.G, Vertex.Color.B, Vertex.Color.A};
            for (int32 C = 0; C < 4; ++C) { ++Histogram[C][Values[C]]; Sums[C] += Values[C] / 255.0; }
            // Native ContrastTemp=2, OffsetTemp=-.08, no variation:
            // LayerAdjustment(G)=square(saturate(5*(G+.08)-2)).
            if (5.0 * (Values[1] / 255.0 + 0.08) - 2.0 > 0.0) ++WarmGate;
        }
        const int32 Count = Section.ProcVertexBuffer.Num();
        if (!Count) return;
        const TCHAR* Names[] = {TEXT("height"), TEXT("temperature"), TEXT("humidity"), TEXT("water")};
        for (int32 C = 0; C < 4; ++C)
        {
            auto Quantile = [&](double Q)
            {
                const int32 Rank = FMath::Clamp(FMath::CeilToInt(Q * Count), 1, Count);
                int32 Sum = 0;
                for (int32 I = 0; I < 256; ++I) { Sum += Histogram[C][I]; if (Sum >= Rank) return I / 255.0; }
                return 1.0;
            };
            UE_LOG(LogTemp, Display, TEXT("TUNDRA_UPLOADED_PAYLOAD channel=%s vertices=%d min=%.6f p10=%.6f p50=%.6f p90=%.6f max=%.6f mean=%.6f zero=%d one=%d vertexWeightedNotArea=1"),
                Names[C], Count, Quantile(0), Quantile(.1), Quantile(.5), Quantile(.9), Quantile(1),
                Sums[C] / Count, Histogram[C][0], Histogram[C][255]);
        }
        UE_LOG(LogTemp, Display, TEXT("TUNDRA_NATIVE_TEMP_GATE nonzeroVertices=%d total=%d noResample=1"), WarmGate, Count);
    }
    inline constexpr float UnfilteredStartCm = 1.e12f;
    inline constexpr float UnfilteredEndCm = 2.e12f;
    inline bool CheckNormalGeometry(const FProcMeshSection& Section, const FTransform& Frame,
        const FVector& Camera, double InverseScale, FString& Error)
    {
        if (!FMath::IsFinite(InverseScale) || InverseScale <= 0 || Section.ProcVertexBuffer.IsEmpty())
        { Error = TEXT("Normal diagnostic lacks actual mesh/physical scale"); return false; }
        double MaxDistance = 0, SumAngle = 0, MaxAngle = 0;
        int32 AboveOneDegree = 0;
        for (const auto& V : Section.ProcVertexBuffer)
        {
            if (V.Position.ContainsNaN() || V.Normal.ContainsNaN() || V.Normal.IsNearlyZero() || V.Position.IsNearlyZero())
            { Error = TEXT("Normal diagnostic encountered invalid geometry"); return false; }
            const double D = FVector::Distance(Camera, Frame.TransformPosition(V.Position)) * InverseScale;
            if (!FMath::IsFinite(D) || D >= double(UnfilteredStartCm))
            { Error = TEXT("Actual physical camera distance exceeds normal bypass bound"); return false; }
            MaxDistance = FMath::Max(MaxDistance, D);
            const double Dot = FVector::DotProduct(V.Position.GetSafeNormal(), V.Normal.GetSafeNormal());
            const double Angle = FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(Dot, -1.0, 1.0)));
            SumAngle += Angle; MaxAngle = FMath::Max(MaxAngle, Angle); AboveOneDegree += Angle > 1.0;
        }
        UE_LOG(LogTemp, Display, TEXT("TUNDRA_GEOMETRIC_NORMAL vertices=%d maxPhysicalDistanceCm=%.12g bypassStartCm=%.12g meanRadialAngleDeg=%.6f maxRadialAngleDeg=%.6f aboveOneDegree=%d actualMeshUnchanged=1"),
            Section.ProcVertexBuffer.Num(), MaxDistance, double(UnfilteredStartCm),
            SumAngle / Section.ProcVertexBuffer.Num(), MaxAngle, AboveOneDegree);
        return true;
    }
    class FPair
    {
        TStrongObjectPtr<UMaterialInstanceDynamic> Live{nullptr}, Saved{nullptr};
        TArray<FScalarParameterValue> AppliedScalars;
        TArray<FVectorParameterValue> AppliedVectors;
        float Shift = 0, Transition = 0, Offset = 0, NormalStart = 0, NormalEnd = 0;
        int32 Variant = 0;
        bool Active = false;
        // Causal probes only, NOT accepted game styling. Separate each path
        // before trying their combination; color endpoints remain untouched.
        static constexpr float ProbeShift = -0.15f;
        static constexpr float ProbeTransition = -4.0f;
        // Existing 2-contrast biome gate spans Offset+.4 to Offset+.6.
        // This candidate uses the observed Tundra payload span .02 to .22,
        // without altering physical temperatures. Snow tested independently.
        static constexpr float TundraOffset = -0.38f;
        static constexpr float TundraSnowTransition = -1.60f;
    public:
        ~FPair() { Restore(); }
        bool IsActive() const { return Active; }
        static const TCHAR* Label(int32 V)
        {
            if (NormalRequested())
            {
                switch (V) { case 0: return TEXT("native"); case 1: return TEXT("geometric-unfiltered");
                    case 2: return TEXT("cold-off"); default: return TEXT("cold-off-geometric"); }
            }
            if (ClimateRequested())
            {
                switch (V) { case 0: return TEXT("native"); case 1: return TEXT("biome-range");
                    case 2: return TEXT("snow-range"); default: return TEXT("both-ranges"); }
            }
            if (ContributionsRequested())
            {
                switch (V) { case 0: return TEXT("native"); case 1: return TEXT("cold-off");
                    case 2: return TEXT("cold-off-coded"); default: return TEXT("native-coded"); }
            }
            switch (V) { case 0: return TEXT("native"); case 1: return TEXT("height-transfer");
                case 2: return TEXT("cold-transfer"); default: return TEXT("both-transfers"); }
        }
        bool Begin(UMaterialInstanceDynamic* Material, FString& Error)
        {
            if ((int32(NormalRequested()) + int32(ClimateRequested()) + int32(ContributionsRequested())
                + int32(FParse::Param(FCommandLine::Get(), TEXT("APSProbeTundraLayerTransfer"))) != 1)
                || Active || !IsValid(Material) || !Material->Parent
                || !Material->RuntimeVirtualTextureParameterValues.IsEmpty()
                || !Material->SparseVolumeTextureParameterValues.IsEmpty())
            { Error = TEXT("Unsupported/outstanding Tundra transfer snapshot"); return false; }
            float Contrast = 0, Influence = 0, BiomeContrast = 0;
            if (!Material->GetScalarParameterValue(FHashedMaterialParameterInfo(TEXT("TopLayerShift")), Shift)
                || !Material->GetScalarParameterValue(FHashedMaterialParameterInfo(TEXT("TopLayerTransition")), Transition)
                || !Material->GetScalarParameterValue(FHashedMaterialParameterInfo(TEXT("TopLayerContrast")), Contrast)
                || !Material->GetScalarParameterValue(FHashedMaterialParameterInfo(TEXT("TempInfluence")), Influence)
                || !FMath::IsNearlyEqual(Shift, -0.656f, 1.e-5f)
                || !FMath::IsNearlyEqual(Transition, -1.278681f, 1.e-5f)
                || !Material->GetScalarParameterValue(FHashedMaterialParameterInfo(TEXT("OffsetTemp")), Offset)
                || !Material->GetScalarParameterValue(FHashedMaterialParameterInfo(TEXT("ContrastTemp(S)")), BiomeContrast)
                || !FMath::IsNearlyEqual(Offset, -0.08f, 1.e-5f) || BiomeContrast != 2.0f
                || Contrast != 2.0f || Influence != 20.0f)
            { Error = TEXT("Audited native Tundra transfer parameters drifted"); return false; }
            if (NormalRequested() && (!Material->GetScalarParameterValue(FHashedMaterialParameterInfo(TEXT("APS_FarNormalStartCm")), NormalStart)
                || !Material->GetScalarParameterValue(FHashedMaterialParameterInfo(TEXT("APS_FarNormalEndCm")), NormalEnd)
                || NormalStart != 20000000.0f || NormalEnd != 70000000.0f))
            { Error = TEXT("Native normal continuity range drifted"); return false; }
            Live.Reset(Material);
            Saved.Reset(UMaterialInstanceDynamic::Create(Material->Parent, GetTransientPackage()));
            if (!Saved.IsValid()) { Error = TEXT("Tundra snapshot allocation failed"); return false; }
            Saved->CopyParameterOverrides(Material);
            AppliedScalars = Material->ScalarParameterValues;
            AppliedVectors = Material->VectorParameterValues;
            Variant = 0; Active = true;
            return Validate(Error);
        }
        bool Validate(FString& Error) const
        {
            float ActualShift = 0, ActualTransition = 0, ActualOffset = 0;
            const bool OriginalTransfer = !NormalRequested() && !ClimateRequested() && !ContributionsRequested();
            if (!Active || !Live.IsValid() || !Saved.IsValid() || Live->Parent != Saved->Parent
                || Live->ScalarParameterValues != AppliedScalars
                || Live->VectorParameterValues != AppliedVectors
                || Live->DoubleVectorParameterValues != Saved->DoubleVectorParameterValues
                || Live->TextureParameterValues != Saved->TextureParameterValues
                || Live->FontParameterValues != Saved->FontParameterValues
                || !Live->RuntimeVirtualTextureParameterValues.IsEmpty()
                || !Live->SparseVolumeTextureParameterValues.IsEmpty()
                || !Live->GetScalarParameterValue(FHashedMaterialParameterInfo(TEXT("TopLayerShift")), ActualShift)
                || !Live->GetScalarParameterValue(FHashedMaterialParameterInfo(TEXT("TopLayerTransition")), ActualTransition)
                || ActualShift != ((OriginalTransfer && (Variant & 1)) ? ProbeShift : Shift)
                || !Live->GetScalarParameterValue(FHashedMaterialParameterInfo(TEXT("OffsetTemp")), ActualOffset)
                || ActualOffset != ((ClimateRequested() && (Variant & 1)) ? TundraOffset : Offset)
                || ActualTransition != (ClimateRequested() ? ((Variant & 2) ? TundraSnowTransition : Transition)
                    : ((ContributionsRequested() ? (Variant == 1 || Variant == 2) : bool(Variant & 2)) ? ProbeTransition : Transition)))
            { Error = TEXT("Tundra transfer snapshot/parameters/physical frame drifted"); return false; }
            if (NormalRequested())
            {
                float ActualStart = 0, ActualEnd = 0;
                if (!Live->GetScalarParameterValue(FHashedMaterialParameterInfo(TEXT("APS_FarNormalStartCm")), ActualStart)
                    || !Live->GetScalarParameterValue(FHashedMaterialParameterInfo(TEXT("APS_FarNormalEndCm")), ActualEnd)
                    || ActualStart != ((Variant & 1) ? UnfilteredStartCm : NormalStart)
                    || ActualEnd != ((Variant & 1) ? UnfilteredEndCm : NormalEnd))
                { Error = TEXT("Normal diagnostic range drifted"); return false; }
            }
            return true;
        }
        bool Apply(int32 V, FString& Error)
        {
            if (V < 0 || V > 3 || !Validate(Error)) return false;
            Live->CopyParameterOverrides(Saved.Get());
            if (NormalRequested())
            {
                if (V & 1)
                {
                    Live->SetScalarParameterValue(TEXT("APS_FarNormalStartCm"), UnfilteredStartCm);
                    Live->SetScalarParameterValue(TEXT("APS_FarNormalEndCm"), UnfilteredEndCm);
                }
                if (V & 2) Live->SetScalarParameterValue(TEXT("TopLayerTransition"), ProbeTransition);
            }
            else if (ClimateRequested())
            {
                if (V & 1) Live->SetScalarParameterValue(TEXT("OffsetTemp"), TundraOffset);
                if (V & 2) Live->SetScalarParameterValue(TEXT("TopLayerTransition"), TundraSnowTransition);
            }
            else if (ContributionsRequested())
            {
                if (V == 1 || V == 2) Live->SetScalarParameterValue(TEXT("TopLayerTransition"), ProbeTransition);
                if (V >= 2)
                {
                    // Diagnostic color code, never a style candidate:
                    // slopes=red, mid biomes=green, bottom/sediment=blue, top=yellow.
                    for (const TCHAR* Name : {TEXT("Color1"), TEXT("Color2"), TEXT("Color3"),
                        TEXT("Color4"), TEXT("Color5"), TEXT("2_Color1"), TEXT("2_Color2"),
                        TEXT("2_Color3"), TEXT("2_Color4")})
                        Live->SetVectorParameterValue(Name, FLinearColor::Green);
                    for (const TCHAR* Name : {TEXT("BottomColor"), TEXT("Sedimentcolor")})
                        Live->SetVectorParameterValue(Name, FLinearColor::Blue);
                    Live->SetVectorParameterValue(TEXT("SlopeColor"), FLinearColor::Red);
                    Live->SetVectorParameterValue(TEXT("Color1_3"), FLinearColor::Yellow);
                    Live->SetVectorParameterValue(TEXT("Color2_3"), FLinearColor::Yellow);
                }
            }
            else
            {
                if (V & 1) Live->SetScalarParameterValue(TEXT("TopLayerShift"), ProbeShift);
                if (V & 2) Live->SetScalarParameterValue(TEXT("TopLayerTransition"), ProbeTransition);
            }
            Variant = V; AppliedScalars = Live->ScalarParameterValues;
            AppliedVectors = Live->VectorParameterValues;
            if (!Validate(Error)) return false;
            UE_LOG(LogTemp, Display, TEXT("TUNDRA_LAYER_TRANSFER variant=%s colorCode=%d exactOtherOverrides=1 noAssetSave=1 diagnosticOnly=1"),
                Label(V), ContributionsRequested() && V >= 2);
            return true;
        }
        bool Restore()
        {
            if (!Active) return true;
            if (!Live.IsValid() || !Saved.IsValid()) return false;
            Live->CopyParameterOverrides(Saved.Get());
            Variant = 0; AppliedScalars = Saved->ScalarParameterValues;
            AppliedVectors = Saved->VectorParameterValues;
            FString Error;
            const bool Restored = Validate(Error);
            if (Restored) Active = false;
            return Restored;
        }
    };
}
#endif
