#pragma once

#if WITH_DEV_AUTOMATION_TESTS
#include "APSSharedTerrainLodABProbe.h"
#include "APSPlanetBufferViewsProbe.h"
#include "APS_ALPHA/Core/Planetary/APSSharedGeneratedLiquidMaterial.h"
#include "APS_ALPHA/Core/Planetary/APSWaterDepthPalette.h"
#include "APS_ALPHA/Core/Planetary/APSWaterSurfaceFilter.h"
#include "APS_ALPHA/Core/Planetary/APSWaterSurfaceLighting.h"
#include "APS_ALPHA/Core/Planetary/APSWaterAnalyticWaves.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"
#include "APSCoastGeometryProbe.h"
#include "Materials/Material.h"
#include "Materials/MaterialExpressionConstant.h"

// Diagnostic only. Reuses the settled production observer, light and material.
// Normal mode never changes payloads. Optional column mode writes/restores UV1
// on frozen published ocean sections only; not a production streaming adapter.
namespace APSWaterNormalAB
{
    inline bool Enabled() { return FParse::Param(FCommandLine::Get(), TEXT("APSProbeWaterNormalAB")); }
    inline bool OpenWater() { return FParse::Param(FCommandLine::Get(), TEXT("APSWaterABOpenWater")); }
    inline bool GeometryOnly() { return FParse::Param(FCommandLine::Get(), TEXT("APSProbeCoastGeometryOnly")); }
    inline bool CameraMotion() { return GeometryOnly() && FParse::Param(FCommandLine::Get(), TEXT("APSProbeCoastCameraMotion")); }
    inline bool CameraTranslation() { return CameraMotion() && FParse::Param(FCommandLine::Get(), TEXT("APSProbeCoastCameraTranslation")); }
    inline bool WaterReference() { return FParse::Param(FCommandLine::Get(), TEXT("APSCoastReferenceWater487132")); }
    inline bool LavaReference() { return FParse::Param(FCommandLine::Get(), TEXT("APSCoastReferenceLava73875")); }
    inline bool ColumnDepth() { return FParse::Param(FCommandLine::Get(), TEXT("APSProbeWaterColumnSnapshot")); }
    inline bool ColumnArtPalette() { return ColumnDepth() && FParse::Param(FCommandLine::Get(), TEXT("APSProbeWaterColumnArtPalette")); }
    inline bool NativeColumn() { return ColumnDepth() && FParse::Param(FCommandLine::Get(), TEXT("APSProbeWaterColumnNative")); }
    inline bool SurfaceFilter() { return FParse::Param(FCommandLine::Get(), TEXT("APSProbeWaterSurfaceFilter")); }
    inline bool SurfaceAOIsolation() { return FParse::Param(FCommandLine::Get(), TEXT("APSProbeWaterSurfaceAOIsolation")); }
    inline bool SurfaceNormalBuffer() { return FParse::Param(FCommandLine::Get(), TEXT("APSProbeWaterNormalBuffer")); }
    inline bool SurfacePass() { return FParse::Param(FCommandLine::Get(), TEXT("APSProbeWaterSurfacePass")); }
    inline bool SurfaceRelative() { return FParse::Param(FCommandLine::Get(), TEXT("APSProbeWaterSurfaceRelative")); }
    inline bool SurfaceWaveIsolation() { return FParse::Param(FCommandLine::Get(), TEXT("APSProbeWaterSurfaceWaveIsolation")); }
    inline bool SurfaceAnalytic() { return FParse::Param(FCommandLine::Get(), TEXT("APSProbeWaterAnalytic")); }
    inline bool SurfaceAnchored() { return SurfaceAnalytic() && FParse::Param(FCommandLine::Get(), TEXT("APSWaterAnchorSplit")); }
    inline bool SurfaceAnchorNoise() { return SurfaceAnchored() && FParse::Param(FCommandLine::Get(), TEXT("APSWaterAnchorNoise")); }
    inline bool DomainAudit() { return FParse::Param(FCommandLine::Get(), TEXT("APSProbeWaterDomainAudit")); }
    inline bool FinalNormalAudit() { return DomainAudit() && FParse::Param(FCommandLine::Get(), TEXT("APSWaterFinalNormalAudit")); }
    inline bool SecondaryDomainAudit() { return DomainAudit() && FParse::Param(FCommandLine::Get(), TEXT("APSWaterSecondaryDomainAudit")); }
    inline bool SurfacePrecise() { return SurfaceRelative() || FParse::Param(FCommandLine::Get(), TEXT("APSProbeWaterSurfacePrecise")); }
    inline const TCHAR* SurfaceTemplatePath() { return SurfaceAnchorNoise() ? TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/Diagnostics/WaterAnchoredNoise20260930/MI_APS_WaterAnalytic.MI_APS_WaterAnalytic") : SurfaceAnchored() ? TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/Diagnostics/WaterAnchored20260930/MI_APS_WaterAnalytic.MI_APS_WaterAnalytic") : SecondaryDomainAudit() ? TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/Diagnostics/WaterSecondaryAudit20260930/MI_APS_WaterDomainAudit.MI_APS_WaterDomainAudit") : FinalNormalAudit() ? TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/Diagnostics/WaterFinalNormalAudit20260930/MI_APS_WaterDomainAudit.MI_APS_WaterDomainAudit") : DomainAudit() ? TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/Diagnostics/WaterDomainAudit20260930/MI_APS_WaterDomainAudit.MI_APS_WaterDomainAudit") : SurfaceAnalytic() ? APSWaterAnalyticWaves::TemplatePath : SurfaceRelative() ? APSWaterSurfaceFilter::RelativeTemplatePath : SurfacePrecise() ? APSWaterSurfaceFilter::PreciseTemplatePath : SurfacePass() ? APSWaterSurfaceFilter::PassTemplatePath : APSWaterSurfaceFilter::TemplatePath; }
    inline const TCHAR* SurfaceMasterPath() { return SurfaceAnchorNoise() ? TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/Diagnostics/WaterAnchoredNoise20260930/M_APS_WaterAnalytic.M_APS_WaterAnalytic") : SurfaceAnchored() ? TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/Diagnostics/WaterAnchored20260930/M_APS_WaterAnalytic.M_APS_WaterAnalytic") : SecondaryDomainAudit() ? TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/Diagnostics/WaterSecondaryAudit20260930/M_APS_WaterDomainAudit.M_APS_WaterDomainAudit") : FinalNormalAudit() ? TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/Diagnostics/WaterFinalNormalAudit20260930/M_APS_WaterDomainAudit.M_APS_WaterDomainAudit") : DomainAudit() ? TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/Diagnostics/WaterDomainAudit20260930/M_APS_WaterDomainAudit.M_APS_WaterDomainAudit") : SurfaceAnalytic() ? APSWaterAnalyticWaves::MasterPath : SurfaceRelative() ? APSWaterSurfaceFilter::RelativeMasterPath : SurfacePrecise() ? APSWaterSurfaceFilter::PreciseMasterPath : SurfacePass() ? APSWaterSurfaceFilter::PassMasterPath : APSWaterSurfaceFilter::MasterPath; }
    inline bool BindPreciseFrame(UMaterialInstanceDynamic* M, AWorldScapeRoot* R)
    {
        // NativeColumn's tag is emitted only for eligible full-scale Water.
        return R && R->ActorHasTag(TEXT("APS.GeneratedOcean.BathymetryUV1"))
            && APSSharedTerrainMaterial::WritePhysicalFrame(M, R->GetRootComponent(), 1.0);
    }
    inline float SurfaceWaveScaleFactor()
    {
        float Factor = 1.0f;
        FParse::Value(FCommandLine::Get(), TEXT("APSWaterSurfaceScaleMultiplier="), Factor);
        return FMath::IsFinite(Factor) && Factor > 0.0f ? FMath::Clamp(Factor, 0.05f, 1.0f) : 1.0f;
    }
    inline void ApplySurfaceFilter(UMaterialInstanceDynamic* Material)
    {
        Material->SetScalarParameterValue(TEXT("WaveScaleCm"), APSWaterSurfaceFilter::PrimaryScaleCm * SurfaceWaveScaleFactor());
        Material->SetScalarParameterValue(TEXT("PhysicalWaveDetailScaleCm"), APSWaterSurfaceFilter::SecondaryScaleCm * SurfaceWaveScaleFactor());
        Material->SetScalarParameterValue(TEXT("WaveColorStrength"), 0.0f);
        Material->SetScalarParameterValue(TEXT("PhysicalWaveRoughnessStrength"), 0.0f);
    }

    // Numerical evidence, not a rendered acceptance test. It reveals actual
    // wet/dry fragmentation separately from raster/LOD/material discontinuities.
    inline bool ExportCoastGrid(AWorldScapeRoot* R, const FVector& Center, const FVector& U, FString& Error)
    {
        constexpr int32 Side = 129;
        constexpr double StepCm = 25000.0; // 32km square at 250m spacing
        const FVector V = FVector::CrossProduct(Center, U).GetSafeNormal();
        TArray<double> Depths;
        Depths.Reserve(Side * Side);
        FString Csv(TEXT("x_m,y_m,signed_depth_m\n"));
        int32 Wet = 0, Edges = 0;
        for (int32 Y = 0; Y < Side; ++Y)
            for (int32 X = 0; X < Side; ++X)
            {
                const double DX = (X - Side / 2) * StepCm, DY = (Y - Side / 2) * StepCm;
                const FVector Direction = (Center + (U * DX + V * DY) / R->PlanetScale).GetSafeNormal();
                const double Depth = double(R->OceanHeight) - R->GetGroundHeight(
                    R->GetActorLocation() + Direction * R->PlanetScale, false);
                if (!FMath::IsFinite(Depth)) { Error = TEXT("Coast grid contains a non-finite depth"); return false; }
                Depths.Add(Depth);
                Wet += Depth > 0.0 ? 1 : 0;
                if (X) Edges += (Depth > 0.0) != (Depths[Depths.Num() - 2] > 0.0) ? 1 : 0;
                if (Y) Edges += (Depth > 0.0) != (Depths[Depths.Num() - 1 - Side] > 0.0) ? 1 : 0;
                Csv += FString::Printf(TEXT("%.2f,%.2f,%.6f\n"), DX * 0.01, DY * 0.01, Depth * 0.01);
            }
        TArray<uint8> Visited;
        Visited.Init(0, Depths.Num());
        int32 Components[2] = {0, 0}, Interior[2] = {0, 0};
        TArray<int32> Queue;
        for (int32 Start = 0; Start < Depths.Num(); ++Start)
        {
            if (Visited[Start]) continue;
            const bool bWet = Depths[Start] > 0.0;
            ++Components[bWet ? 1 : 0];
            bool bTouchesBoundary = false;
            Queue.Reset(); Queue.Add(Start); Visited[Start] = 1;
            for (int32 Head = 0; Head < Queue.Num(); ++Head)
            {
                const int32 I = Queue[Head], X = I % Side, Y = I / Side;
                bTouchesBoundary |= X == 0 || X == Side - 1 || Y == 0 || Y == Side - 1;
                const int32 Neighbours[] = {X ? I - 1 : -1, X + 1 < Side ? I + 1 : -1,
                    Y ? I - Side : -1, Y + 1 < Side ? I + Side : -1};
                for (int32 N : Neighbours)
                    if (N >= 0 && !Visited[N] && (Depths[N] > 0.0) == bWet)
                    { Visited[N] = 1; Queue.Add(N); }
            }
            if (!bTouchesBoundary) ++Interior[bWet ? 1 : 0];
        }
        const FString Folder = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("Diagnostics/WaterNormalAB"));
        const FString Path = FPaths::Combine(Folder, TEXT("coast-depth.csv"));
        if (IFileManager::Get().FileExists(*Path) || !IFileManager::Get().MakeDirectory(*Folder, true)
            || !FFileHelper::SaveStringToFile(Csv, *Path, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM))
        { Error = TEXT("Coast grid evidence exists or could not be saved; use a fresh UserDir"); return false; }
        UE_LOG(LogTemp, Display, TEXT("[APS.CoastGrid] side=%d spacingM=250 wetFraction=%.6f signChangeEdges=%d landComponents=%d waterComponents=%d enclosedIslands=%d enclosedLakes=%d csv=%s; four-neighbour sampled field, not mesh or visual acceptance"),
            Side, double(Wet) / Depths.Num(), Edges, Components[0], Components[1], Interior[0], Interior[1], *Path);
        return true;
    }

    inline bool IsSettled(const APlanetarySurfaceGenerator* Surface)
    {
        const AWorldScapeRoot* R = Surface ? Surface->WorldScapeRootInstance : nullptr;
        if (!R || !R->bOcean || R->OceanMaxLod <= 0 || R->WorldScapeLodInGeneration.Num()
            || R->WorldScapeLodOcean.Num() != R->OceanMaxLod) return false;
        for (const UWorldScapeLod* L : R->WorldScapeLodOcean)
            if (!IsValid(L) || !IsValid(L->Mesh) || L->Vertices.IsEmpty() || L->Triangles.IsEmpty()
                || !L->Mesh->IsVisible() || L->Mesh->bHiddenInGame) return false;
        return APSSharedGeneratedLiquidMaterial::IsRenderReady(Surface->ResolvedOceanMaterialInstance,
            Surface->ResolvedSurfaceProfile.LiquidType, R->GetWorld());
    }

    // Select using the actual full-scale field, not a low-resolution preview or
    // material water mask. Keep the landing hemisphere (and its natural light).
    inline bool SelectView(AWorldScapeRoot* R, FVector& Outward, FVector& Tangent, FString& Error)
    {
        if (!R || !R->bOcean || !FMath::IsFinite(R->PlanetScale) || R->PlanetScale <= 0.0)
        { Error = TEXT("Water A/B needs a finite full-scale ocean root"); return false; }
        struct FSample { FVector Direction; double Depth; };
        TArray<FSample> Dry, Wet;
        const FVector Initial = Outward;
        double TargetDepthM = 20.0;
        FParse::Value(FCommandLine::Get(), TEXT("APSWaterABDepthM="), TargetDepthM);
        if (!FMath::IsFinite(TargetDepthM) || TargetDepthM < 0.05 || TargetDepthM > 1000.0)
        { Error = TEXT("Water A/B target depth must be 0.05..1000 metres"); return false; }
        const double TargetDepthCm = TargetDepthM * 100.0;
        auto Depth = [R](const FVector& D)
        { return double(R->OceanHeight) - R->GetGroundHeight(R->GetActorLocation() + D * R->PlanetScale, false); };
        constexpr int32 Count = 1024;
        for (int32 I = 0; I < Count; ++I)
        {
            const double Z = 1.0 - 2.0 * (I + 0.5) / Count;
            const double Radial = FMath::Sqrt(FMath::Max(0.0, 1.0 - Z * Z));
            const double Angle = I * 2.39996322972865332;
            const FVector D(Radial * FMath::Cos(Angle), Radial * FMath::Sin(Angle), Z);
            if (FVector::DotProduct(D, Initial) < 0.6) continue;
            const double H = Depth(D);
            if (!FMath::IsFinite(H)) { Error = TEXT("Water A/B field sample is non-finite"); return false; }
            if (H >= TargetDepthCm) Wet.Add({D, H});
            else if (H < 0.0) Dry.Add({D, H});
        }
        if (Wet.IsEmpty() || (!OpenWater() && Dry.IsEmpty()))
        { Error = TEXT("Water A/B could not find both required signs in the landing hemisphere"); return false; }
        Wet.Sort([&](const FSample& A, const FSample& B)
        { return FVector::DotProduct(A.Direction, Initial) > FVector::DotProduct(B.Direction, Initial); });
        Outward = Wet[0].Direction;
        if (!OpenWater())
        {
            Dry.Sort([&](const FSample& A, const FSample& B)
            { return FVector::DotProduct(A.Direction, Outward) > FVector::DotProduct(B.Direction, Outward); });
            FVector Land = Dry[0].Direction, Sea = Outward;
            // Bracket the requested underwater depth. This is observer selection only;
            // the terrain function and all generated heights remain untouched.
            for (int32 I = 0; I < 40; ++I)
            {
                const FVector Mid = (Land + Sea).GetSafeNormal();
                const double H = Depth(Mid);
                if (!FMath::IsFinite(H)) { Error = TEXT("Water A/B coast refinement is non-finite"); return false; }
                if (H >= TargetDepthCm) Sea = Mid; else Land = Mid;
            }
            Outward = Sea;
            Tangent = FVector::VectorPlaneProject(Land - Sea, Sea).GetSafeNormal();
            // Very small final brackets may lose a tangent in subtraction.
            if (Tangent.IsNearlyZero())
                Tangent = FVector::VectorPlaneProject(Dry[0].Direction, Sea).GetSafeNormal();
        }
        else Tangent = FVector::VectorPlaneProject(Initial, Outward).GetSafeNormal();
        if (Tangent.IsNearlyZero())
        { FVector Unused; Outward.FindBestAxisVectors(Tangent, Unused); }
        // Replay the logged candidate frame in an unmodified-field process, so
        // shoreline comparisons do not accidentally compare different locations.
        const TCHAR* Keys[] = {TEXT("APSWaterABViewX="), TEXT("APSWaterABViewY="), TEXT("APSWaterABViewZ="),
            TEXT("APSWaterABViewU="), TEXT("APSWaterABViewV="), TEXT("APSWaterABViewW=")};
        double Frame[6] = {};
        int32 FrameValues = 0;
        for (int32 I = 0; I < 6; ++I)
        {
            FrameValues += FParse::Value(FCommandLine::Get(), Keys[I], Frame[I]) ? 1 : 0;
            if (!FMath::IsFinite(Frame[I])) { Error = TEXT("Non-finite comparison frame"); return false; }
        }
        if (FrameValues != 0 && FrameValues != 6) { Error = TEXT("Comparison frame needs all six values"); return false; }
        if (FrameValues == 6)
        {
            Outward = FVector(Frame[0], Frame[1], Frame[2]).GetSafeNormal();
            Tangent = FVector::VectorPlaneProject(FVector(Frame[3], Frame[4], Frame[5]), Outward).GetSafeNormal();
            if (Outward.IsNearlyZero() || Tangent.IsNearlyZero()) { Error = TEXT("Degenerate comparison frame"); return false; }
        }
        const double SelectedDepth = Depth(Outward);
        if (!FMath::IsFinite(SelectedDepth) || (FrameValues == 0 && SelectedDepth < TargetDepthCm - 1.0))
        { Error = TEXT("Water A/B selected observer is not over water"); return false; }
        UE_LOG(LogTemp, Display, TEXT("[APS.WaterNormalAB.Field] coastCandidate=%d seed=%d radius=%.17g scale=%.17g intensity=%.17g direction=(%.17g,%.17g,%.17g) tangent=(%.17g,%.17g,%.17g)"),
            FParse::Param(FCommandLine::Get(), TEXT("APSProbeCoastalReliefV1")) ? 1 : 0, R->Seed,
            R->PlanetScale, double(R->NoiseScale), double(R->NoiseIntensity), Outward.X, Outward.Y, Outward.Z,
            Tangent.X, Tangent.Y, Tangent.Z);
        UE_LOG(LogTemp, Display, TEXT("[APS.WaterNormalAB.View] mode=%s direction=%s depthCm=%.6f hemisphereDot=%.6f wet=%d dry=%d; actual height field, camera height measured above sea"),
            OpenWater() ? TEXT("open-water") : TEXT("coast"), *Outward.ToString(), SelectedDepth,
            FVector::DotProduct(Initial, Outward), Wet.Num(), Dry.Num());
        return OpenWater() || ExportCoastGrid(R, Outward, Tangent, Error);
    }

    class FLease
    {
        struct FSlot
        {
            TWeakObjectPtr<UWorldScapeMeshComponent> Mesh;
            TStrongObjectPtr<UMaterialInterface> Original{nullptr};
            FTransform Transform;
            int32 Index = INDEX_NONE;
            uint32 Hash = 0;
            uint32 DepthHash = 0;
            TArray<FVector2D> DepthUV;
            bool bOcean = false;
        };
        TArray<FSlot> Slots;
        TWeakObjectPtr<AWorldScapeRoot> Root;
        TWeakObjectPtr<APlanetarySurfaceGenerator> Surface;
        TWeakObjectPtr<AActor> Body;
        TStrongObjectPtr<UMaterialInstanceDynamic> Native{nullptr}, Candidate{nullptr}, DepthControl{nullptr};
        FTransform RootTransform;
        float OriginalStrength = 0.0f;
        float ColumnStrength = 0.0f;
        bool bDepthPayloadApplied = false;
        int32 Phase = 0;
        IConsoleVariable* ShortRangeAO = nullptr;
        int32 OriginalShortRangeAO = 0;
        uint32 OriginalAOSetBy = 0;
        APSPlanetBufferViews::FScope NormalBuffer;
        bool bActive = false, bFrozen = false, bRootTick = false, bSurfaceTick = false, bBodyTick = false;

        float Strength() const { return GeometryOnly() ? OriginalStrength : SurfaceWaveIsolation() && Phase == 2 ? 0.0f : ColumnDepth() ? OriginalStrength : OriginalStrength * (Phase == 2 ? 0.4f : Phase == 3 ? 0.2f : Phase == 4 ? 0.0f : 1.0f); }
        bool UsesCandidate() const { return !GeometryOnly() && Phase >= 1 && Phase <= (SurfaceAOIsolation() ? 4 : ColumnDepth() ? (SurfaceFilter() ? 3 : 2) : 4); }
        float DepthStrength() const { return SurfaceWaveIsolation() || SurfaceAOIsolation() || Phase == 1 || (SurfaceFilter() && Phase == 2) ? ColumnStrength : 0.0f; }
        UMaterialInstanceDynamic* ExpectedMaterial() const
        {
            if (!UsesCandidate()) return Native.Get();
            return SurfaceFilter() && Phase == 1 && !SurfaceWaveIsolation() && !DomainAudit() ? DepthControl.Get() : Candidate.Get();
        }

        bool ValidateNativeColumn(FString& Error)
        {
            auto* R = Root.Get();
            if (!R || !R->ActorHasTag(TEXT("APS.GeneratedOcean.BathymetryUV1")))
            { Error = TEXT("Native column requires an opted-in full-scale Water root"); return false; }
            int64 Count = 0, Wet = 0, Shallow = 0;
            int32 OracleSamples = 0;
            double MaxErrorM = 0.0;
            const double Start = FPlatformTime::Seconds();
            for (FSlot& Slot : Slots)
            {
                if (!Slot.bOcean) continue;
                auto* Mesh = Slot.Mesh.Get();
                const auto* Section = Mesh ? Mesh->GetProcMeshSection(Slot.Index) : nullptr;
                if (!Section || (Count += Section->PlanetVertexBuffer.Num()) > 2000000)
                { Error = TEXT("Native column missing or beyond diagnostic vertex bound"); return false; }
                const int32 Stride = FMath::Max(1, Section->PlanetVertexBuffer.Num() / 32);
                for (int32 I = 0; I < Section->PlanetVertexBuffer.Num(); ++I)
                {
                    const auto& V = Section->PlanetVertexBuffer[I];
                    if (!FMath::IsFinite(V.UV1.X) || V.UV1.Y != 1.0)
                    { Error = TEXT("Native column absent/invalid; verify matched plugin build, no snapshot fallback"); return false; }
                    Wet += V.UV1.X > 0.0 ? 1 : 0;
                    Shallow += V.UV1.X > 0.0 && V.UV1.X <= 0.08 ? 1 : 0;
                    if (I % Stride == 0 || I + 1 == Section->PlanetVertexBuffer.Num())
                    {
                        const FVector P = Slot.Transform.TransformPosition(V.Position);
                        const double ExpectedKm = (double(R->OceanHeight) - R->GetGroundHeight(P, false)) / 100000.0;
                        if (!FMath::IsFinite(ExpectedKm))
                        { Error = TEXT("Native column oracle nonfinite"); return false; }
                        MaxErrorM = FMath::Max(MaxErrorM, FMath::Abs(V.UV1.X - ExpectedKm) * 1000.0);
                        ++OracleSamples;
                    }
                }
                Slot.DepthHash = Slot.Hash; // Both materials read the same worker payload.
            }
            if (!Wet || !Shallow || MaxErrorM > 0.01)
            { Error = FString::Printf(TEXT("Native column wet=%lld shallow=%lld maxOracleErrorM=%.9g"), Wet, Shallow, MaxErrorM); return false; }
            UE_LOG(LogTemp, Display, TEXT("[APS.WaterColumnNative] vertices=%lld wet=%lld shallow80m=%lld oracleSamples=%d maxErrorM=%.9g checkMs=%.3f UVwrites=0 materialOnlyAB=1; frozen capture, not moving performance acceptance"),
                Count, Wet, Shallow, OracleSamples, MaxErrorM, (FPlatformTime::Seconds()-Start)*1000.0);
            return true;
        }

        bool BuildColumnPayload(FString& Error)
        {
            if (NativeColumn()) return ValidateNativeColumn(Error);
            auto* R = Root.Get();
            int64 Count = 0, Wet = 0, Shallow = 0;
            double Minimum = TNumericLimits<double>::Max(), Maximum = -TNumericLimits<double>::Max();
            const double Start = FPlatformTime::Seconds();
            for (FSlot& Slot : Slots)
            {
                if (!Slot.bOcean) continue;
                auto* Mesh = Slot.Mesh.Get();
                const auto* Section = Mesh ? Mesh->GetProcMeshSection(Slot.Index) : nullptr;
                if (!Section || (Count += Section->PlanetVertexBuffer.Num()) > 2000000)
                { Error = TEXT("Column snapshot missing section or exceeds two-million-vertex diagnostic bound"); return false; }
                // Snapshot copy is never submitted as geometry. Only UV1 differs.
                FWorldScapeMeshSection WithDepth = *Section;
                Slot.DepthUV.Reserve(Section->PlanetVertexBuffer.Num());
                for (auto& Vertex : WithDepth.PlanetVertexBuffer)
                {
                    if (Vertex.UV1 != FVector2D::ZeroVector)
                    { Error = TEXT("Column snapshot refuses to replace an occupied UV1 channel"); return false; }
                    const FVector WorldPosition = Slot.Transform.TransformPosition(Vertex.Position);
                    const double DepthKm = (double(R->OceanHeight) - R->GetGroundHeight(WorldPosition, false)) / 100000.0;
                    if (!FMath::IsFinite(DepthKm))
                    { Error = TEXT("Non-finite physical column sample"); return false; }
                    Vertex.UV1 = FVector2D(DepthKm, 1.0);
                    Slot.DepthUV.Add(Vertex.UV1);
                    Wet += DepthKm > 0.0 ? 1 : 0;
                    Shallow += DepthKm > 0.0 && DepthKm <= 0.08 ? 1 : 0;
                    Minimum = FMath::Min(Minimum, DepthKm); Maximum = FMath::Max(Maximum, DepthKm);
                }
                Slot.DepthHash = APSSharedTerrainLodAB::PayloadHash(WithDepth, true);
            }
            if (!Wet || !Shallow)
            { Error = TEXT("Column fixture has no wet and shallow physical samples"); return false; }
            UE_LOG(LogTemp, Display, TEXT("[APS.WaterColumnSnapshot] vertices=%lld wet=%lld shallow80m=%lld minKm=%.9g maxKm=%.9g sampleMs=%.3f strength=%.9g halfDepthM=20 source=live-root-GetGroundHeight UV1-only=1 workers=0 streamingAcceptance=0"),
                Count, Wet, Shallow, Minimum, Maximum, (FPlatformTime::Seconds()-Start)*1000.0, ColumnStrength);
            return true;
        }

        bool SetColumnPayload(bool bApply)
        {
            // In this mode the worker already published UV1 with its geometry.
            // Original and candidate share it; never overwrite live payloads,
            // including when restoring the old water material.
            if (NativeColumn()) { bDepthPayloadApplied = bApply; return true; }
            if (bDepthPayloadApplied == bApply) return true;
            // Preflight every section before the first write, including restore.
            for (FSlot& Slot : Slots)
                if (Slot.bOcean)
                {
                    auto* Mesh = Slot.Mesh.Get();
                    auto* Section = Mesh ? Mesh->GetProcMeshSection(Slot.Index) : nullptr;
                    if (!Section || Section->PlanetVertexBuffer.Num() != Slot.DepthUV.Num()
                        || APSSharedTerrainLodAB::PayloadHash(*Section, true) != (bDepthPayloadApplied ? Slot.DepthHash : Slot.Hash))
                    { UE_LOG(LogTemp, Error, TEXT("[APS.WaterColumnSnapshot] payload drift: refusing stale UV write")); return false; }
                }
            TSet<UWorldScapeMeshComponent*> Changed;
            for (FSlot& Slot : Slots)
                if (Slot.bOcean)
                {
                    auto* Mesh = Slot.Mesh.Get();
                    auto* Section = Mesh->GetProcMeshSection(Slot.Index);
                    for (int32 I = 0; I < Slot.DepthUV.Num(); ++I)
                        Section->PlanetVertexBuffer[I].UV1 = bApply ? Slot.DepthUV[I] : FVector2D::ZeroVector;
                    Changed.Add(Mesh);
                }
            for (auto* Mesh : Changed) Mesh->MarkRenderStateDirty();
            bDepthPayloadApplied = bApply;
            return true;
        }
    public:
        ~FLease() { Restore(); }
        bool IsActive() const { return bActive; }
        double CameraYawDegrees() const { return CameraMotion() && !CameraTranslation() ? (Phase == 1 ? 0.35 : Phase == 2 ? -0.35 : 0.0) : 0.0; }
        double CameraTranslationCm() const { return CameraTranslation() ? (Phase == 1 ? 25.0 : Phase == 2 ? -25.0 : 0.0) : 0.0; }
        const TCHAR* Label() const
        {
            if (CameraTranslation())
            {
                static const TCHAR* Names[] = {TEXT("Coast0Frozen"), TEXT("Coast1TranslatePlus"), TEXT("Coast2TranslateMinus"), TEXT("Coast3Return")};
                return Phase >= 0 && Phase < UE_ARRAY_COUNT(Names) ? Names[Phase] : TEXT("CoastInvalid");
            }
            if (CameraMotion())
            {
                static const TCHAR* Names[] = {TEXT("Coast0Frozen"), TEXT("Coast1YawPlus"), TEXT("Coast2YawMinus"), TEXT("Coast3Return")};
                return Phase >= 0 && Phase < UE_ARRAY_COUNT(Names) ? Names[Phase] : TEXT("CoastInvalid");
            }
            if (SurfaceAnalytic())
            {
                if (SurfaceAnchored())
                {
                    if (SurfaceAnchorNoise())
                    {
                        static const TCHAR* NoiseNames[] = {TEXT("NoiseAnchor0Native"), TEXT("NoiseAnchor1Unsplit"),
                            TEXT("NoiseAnchor2Split"), TEXT("NoiseAnchor3DepthZero"), TEXT("NoiseAnchor4NativeReturn")};
                        return Phase >= 0 && Phase < UE_ARRAY_COUNT(NoiseNames) ? NoiseNames[Phase] : TEXT("NoiseAnchorInvalid");
                    }
                    static const TCHAR* Names[] = {TEXT("Anchor0Native"), TEXT("Anchor1Unsplit"),
                        TEXT("Anchor2Split"), TEXT("Anchor3DepthZero"), TEXT("Anchor4NativeReturn")};
                    return Phase >= 0 && Phase < UE_ARRAY_COUNT(Names) ? Names[Phase] : TEXT("AnchorInvalid");
                }
                if (DomainAudit())
                {
                    if (SecondaryDomainAudit())
                    {
                        static const TCHAR* SecondaryNames[] = {TEXT("Secondary0Native"), TEXT("Secondary1Phase"),
                            TEXT("Secondary2Unfiltered"), TEXT("Secondary3Visibility"), TEXT("Secondary4NativeReturn")};
                        return Phase >= 0 && Phase < UE_ARRAY_COUNT(SecondaryNames) ? SecondaryNames[Phase] : TEXT("SecondaryInvalid");
                    }
                    if (FinalNormalAudit())
                    {
                        static const TCHAR* FinalNames[] = {TEXT("Final0Native"), TEXT("Final1SecondaryGradient"),
                            TEXT("Final2LocalResidual"), TEXT("Final3WorldResidual"), TEXT("Final4NativeReturn")};
                        return Phase >= 0 && Phase < UE_ARRAY_COUNT(FinalNames) ? FinalNames[Phase] : TEXT("FinalInvalid");
                    }
                    static const TCHAR* DebugNames[] = {TEXT("Domain0Native"), TEXT("Domain1RawCameraPhase"),
                        TEXT("Domain2PhysicalPhase"), TEXT("Domain3FilteredGradient"), TEXT("Domain4NativeReturn")};
                    return Phase >= 0 && Phase < UE_ARRAY_COUNT(DebugNames) ? DebugNames[Phase] : TEXT("DomainInvalid");
                }
                static const TCHAR* Names[] = {TEXT("Analytic0Native"), TEXT("Analytic1GradientControl"),
                    TEXT("Analytic2SmoothGradient"), TEXT("Analytic3DepthZero"), TEXT("Analytic4NativeReturn")};
                return Phase >= 0 && Phase < UE_ARRAY_COUNT(Names) ? Names[Phase] : TEXT("AnalyticInvalid");
            }
            if (SurfaceWaveIsolation())
            {
                static const TCHAR* Names[] = {TEXT("WaveIsolation0Native"), TEXT("WaveIsolation1On"),
                    TEXT("WaveIsolation2Zero_DIAGNOSTIC"), TEXT("WaveIsolation3OnReturn"), TEXT("WaveIsolation4NativeReturn")};
                return Phase >= 0 && Phase < UE_ARRAY_COUNT(Names) ? Names[Phase] : TEXT("WaveIsolationInvalid");
            }
            if (SurfaceRelative())
            {
                static const TCHAR* Names[] = {TEXT("Relative0Native"), TEXT("Relative1AbsoluteControl"),
                    TEXT("Relative2CameraDomain"), TEXT("Relative3DepthZero"), TEXT("Relative4NativeReturn")};
                return Phase >= 0 && Phase < UE_ARRAY_COUNT(Names) ? Names[Phase] : TEXT("RelativeInvalid");
            }
            if (SurfacePrecise())
            {
                static const TCHAR* Names[] = {TEXT("Precision0Native"), TEXT("Precision1FoldControl"),
                    TEXT("Precision2DoubleFloat"), TEXT("Precision3DepthZero"), TEXT("Precision4NativeReturn")};
                return Phase >= 0 && Phase < UE_ARRAY_COUNT(Names) ? Names[Phase] : TEXT("PrecisionInvalid");
            }
            if (SurfaceAOIsolation())
            {
                switch (Phase)
                {
                case 0: return TEXT("SurfaceAO0Native");
                case 1: return TEXT("SurfaceAO1DepthControl");
                case 2: return TEXT("SurfaceAO2FilteredOrdinary");
                case 3: return TEXT("SurfaceAO3FilteredAOOff_DIAGNOSTIC");
                case 4: return TEXT("SurfaceAO4FilteredAOReturn");
                default: return TEXT("SurfaceAO5NativeReturn");
                }
            }
            if (SurfaceFilter())
            {
                switch (Phase)
                {
                case 0: return TEXT("Surface0Native");
                case 1: return TEXT("Surface1DepthControl");
                case 2: return TEXT("Surface2Filtered");
                case 3: return TEXT("Surface3DepthZero");
                default: return TEXT("Surface4NativeReturn");
                }
            }
            if (ColumnDepth())
            {
                switch (Phase)
                {
                case 0: return TEXT("Column0Native");
                case 1: return TEXT("Column1Depth");
                case 2: return ColumnArtPalette() ? TEXT("Column2PaletteOnly") : TEXT("Column2StrengthZero");
                default: return TEXT("Column3NativeReturn");
                }
            }
            switch (Phase)
            {
            case 0: return TEXT("Water0Native");
            case 1: return TEXT("Water1CopyControl");
            case 2: return TEXT("Water2Normal40");
            case 3: return TEXT("Water3Normal20");
            case 4: return TEXT("Water4SmoothControl");
            default: return TEXT("Water5NativeReturn");
            }
        }
        bool Begin(APlanetarySurfaceGenerator* S, AActor* B, FString& Error)
        {
            if (DomainAudit() && (!SurfaceAnalytic() || SurfaceNormalBuffer() || SurfaceWaveIsolation()))
            { Error = TEXT("Domain audit needs analytic route, Lit output, no wave isolation"); return false; }
            if (SurfaceAnalytic() && (!SurfaceRelative() || SurfaceWaveIsolation()))
            { Error = TEXT("Analytic kernel comparison requires camera-relative control and no wave isolation"); return false; }
            if (SurfaceWaveIsolation() && (!SurfaceRelative() || SurfaceAOIsolation() || SurfacePass()))
            { Error = TEXT("Wave isolation requires the camera-relative water candidate and ordinary lighting"); return false; }
            if (SurfacePrecise() && (!SurfaceFilter() || !NativeColumn() || SurfacePass() || SurfaceAOIsolation()))
            { Error = TEXT("Water precision A/B requires native filtered Water with unchanged ordinary lighting"); return false; }
            if (SurfaceNormalBuffer() && (!SurfaceFilter() || SurfaceAOIsolation() || SurfacePass()))
            { Error = TEXT("Normal-buffer inspection requires the ordinary filtered surface, without lighting isolation"); return false; }
            if (SurfacePass() && (!SurfaceFilter() || !NativeColumn() || SurfaceAOIsolation()
                || !APSWaterSurfaceLighting::RenderContract()))
            { Error = TEXT("Water pass requires native filtered water, filtered shadows and ordinary AO; no lighting isolation"); return false; }
            if (SurfaceAOIsolation())
            {
                auto* AO = IConsoleManager::Get().FindConsoleVariable(TEXT("r.Lumen.ScreenProbeGather.ShortRangeAO"));
                if (!SurfaceFilter() || !NativeColumn() || !AO || AO->GetInt() != 1)
                { Error = TEXT("AO isolation requires native surface-filter fixture with ordinary ShortRangeAO=1"); return false; }
            }
            if (SurfaceFilter() && !NativeColumn())
            { Error = TEXT("Surface filter A/B requires native column; no fallback CPU snapshot"); return false; }
            if (bActive || !IsValid(B) || !IsSettled(S)
                || (S->ResolvedSurfaceProfile.LiquidType != EAPSPlanetLiquidType::Water
                    && !(GeometryOnly() && S->ResolvedSurfaceProfile.LiquidType == EAPSPlanetLiquidType::Lava)))
            { Error = TEXT("Water A/B requires settled Water; geometry-only also permits authoritative Lava, no workers"); return false; }
            auto* M = S->ResolvedOceanMaterialInstance;
            // Lava geometry capture never edits shading and has no Water wave
            // parameter contract. Its own saved-family authority was checked above.
            const bool bLavaGeometry = GeometryOnly() && S->ResolvedSurfaceProfile.LiquidType == EAPSPlanetLiquidType::Lava;
            if (!IsValid(M) || (!bLavaGeometry && (!M->GetScalarParameterValue(FHashedMaterialParameterInfo(TEXT("WaveNormalStrength")), OriginalStrength)
                || !FMath::IsFinite(OriginalStrength) || OriginalStrength <= 0.0f)))
            { Error = TEXT("Water A/B production wave parameter absent/nonpositive"); return false; }
            UMaterialInterface* CandidateParent = M->Parent.Get();
            if (ColumnDepth())
            {
                if (GeometryOnly()) { Error = TEXT("Column and geometry-only captures are mutually exclusive"); return false; }
                CandidateParent = LoadObject<UMaterialInstance>(nullptr,
                    SurfaceFilter() ? SurfaceTemplatePath() : TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/Diagnostics/WaterDepthFiltered20260928/MI_APS_WaterDepth.MI_APS_WaterDepth"));
                if (!CandidateParent || !CandidateParent->GetMaterial()
                    || CandidateParent->GetMaterial()->GetPathName() != (SurfaceFilter() ? SurfaceMasterPath() : TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/Diagnostics/WaterDepthFiltered20260928/M_APS_WaterDepth.M_APS_WaterDepth")))
                { Error = TEXT("Exact filtered-column material is absent"); return false; }
                FLinearColor Deep, Shallow;
                if (!M->GetVectorParameterValue(FMaterialParameterInfo(TEXT("LiquidDeepColor")), Deep)
                    || !M->GetVectorParameterValue(FMaterialParameterInfo(TEXT("LiquidShallowColor")), Shallow)
                    || !APSWaterDepthPalette::ResolveStrength(Deep, Shallow, APSWaterDepthPalette::DefaultRelativeBudget, ColumnStrength)
                    || ColumnStrength <= 0.0f)
                { Error = TEXT("Actual water palette cannot support the column response"); return false; }
#if WITH_EDITOR
                if (GShaderCompilingManager) GShaderCompilingManager->FinishAllCompilation();
#endif
            }
            Candidate.Reset(UMaterialInstanceDynamic::Create(CandidateParent, GetTransientPackage()));
            if (!Candidate.IsValid()) { Error = TEXT("Water A/B temporary MID creation failed"); return false; }
            Candidate->CopyMaterialUniformParameters(M);
            if (ColumnArtPalette())
            {
                // Deliberate visual experiment, not a claim that the old blue
                // palette is preserved. Phase2 isolates palette from depth.
                Candidate->SetVectorParameterValue(TEXT("LiquidDeepColor"), FLinearColor(0.0025f, 0.009f, 0.018f, 1.0f));
                Candidate->SetVectorParameterValue(TEXT("LiquidShallowColor"), FLinearColor(0.025f, 0.060f, 0.055f, 1.0f));
                ColumnStrength = 1.0f;
                UE_LOG(LogTemp, Display, TEXT("[APS.WaterColumnSnapshot] artPalette=1 deepLinear=(.0025,.009,.018) shallowLinear=(.025,.060,.055) strength=1; old palette budget intentionally not applied; transient MID only"));
            }
            if (SurfaceFilter())
            {
                auto* ControlParent = LoadObject<UMaterialInstance>(nullptr, SurfaceAnchorNoise() ? APSWaterSurfaceFilter::RelativeTemplatePath : SurfaceAnchored() ? APSWaterAnalyticWaves::TemplatePath : SurfaceAnalytic() ? APSWaterSurfaceFilter::RelativeTemplatePath : SurfaceRelative() ? APSWaterSurfaceFilter::PreciseTemplatePath : SurfacePrecise() ? APSWaterSurfaceFilter::TemplatePath
                    : TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/Diagnostics/WaterDepthFiltered20260928/MI_APS_WaterDepth.MI_APS_WaterDepth"));
                if (!ControlParent) { Error = TEXT("Previous depth template required for paired wave control"); return false; }
                DepthControl.Reset(UMaterialInstanceDynamic::Create(ControlParent, GetTransientPackage()));
                if (!DepthControl.IsValid()) { Error = TEXT("Wave control MID allocation failed"); return false; }
                // Same depth, palette, frame and geometry as the new candidate;
                // only the wave graph/parameters differ in phases 1 versus 2.
                DepthControl->CopyMaterialUniformParameters(Candidate.Get());
                auto* ControlResource = DepthControl->GetMaterialResource(S->GetWorld()->GetFeatureLevel());
#if WITH_EDITOR
                if (ControlResource && !ControlResource->IsGameThreadShaderMapComplete())
                {
                    ControlResource->SubmitCompileJobs_GameThread(EShaderCompileJobPriority::ForceLocal);
                    if (GShaderCompilingManager) GShaderCompilingManager->FinishAllCompilation();
                    ControlResource = DepthControl->GetMaterialResource(S->GetWorld()->GetFeatureLevel());
                }
#endif
                const auto* ControlMap = ControlResource ? ControlResource->GetGameThreadShaderMap() : nullptr;
                if (!ControlResource || !ControlResource->IsGameThreadShaderMapComplete() || ControlResource->GetCompileErrors().Num()
                    || !ControlMap || !ControlMap->GetMeshShaderMap(&FLocalVertexFactory::StaticType))
                { Error = TEXT("Wave control LocalVF incomplete"); return false; }
                ApplySurfaceFilter(Candidate.Get());
                if (SurfacePrecise())
                {
                    ApplySurfaceFilter(DepthControl.Get());
                    if (!BindPreciseFrame(Candidate.Get(), S->WorldScapeRootInstance))
                    { Error = TEXT("Water precision requires actual native physical frame"); return false; }
                    if (SurfaceRelative() && !BindPreciseFrame(DepthControl.Get(), S->WorldScapeRootInstance))
                    { Error = TEXT("Camera-relative control requires the same physical frame"); return false; }
                }
                if (SurfacePass() && (!Candidate->GetShadingModels().HasOnlyShadingModel(MSM_SingleLayerWater)
                    || !APSWaterSurfaceLighting::Bind(Candidate.Get(), S->GetWorld())))
                { Error = TEXT("Water-pass model/actual scene fill binding failed"); return false; }
                UE_LOG(LogTemp, Display, TEXT("[APS.WaterSurfaceFilter] paired previous-depth/new-wave capture wavesCm=%g/%g scaleFactor=%g footprintFilter=1 rootFrame=unchanged"),
                    APSWaterSurfaceFilter::PrimaryScaleCm * SurfaceWaveScaleFactor(),
                    APSWaterSurfaceFilter::SecondaryScaleCm * SurfaceWaveScaleFactor(), SurfaceWaveScaleFactor());
            }
            auto* CandidateResource = Candidate->GetMaterialResource(S->GetWorld()->GetFeatureLevel());
#if WITH_EDITOR
            if (ColumnDepth() && CandidateResource && !CandidateResource->IsGameThreadShaderMapComplete())
            {
                CandidateResource->SubmitCompileJobs_GameThread(EShaderCompileJobPriority::ForceLocal);
                if (GShaderCompilingManager) GShaderCompilingManager->FinishAllCompilation();
                CandidateResource = Candidate->GetMaterialResource(S->GetWorld()->GetFeatureLevel());
            }
#endif
            const auto* CandidateMap = CandidateResource ? CandidateResource->GetGameThreadShaderMap() : nullptr;
            if (!CandidateResource || !CandidateResource->IsGameThreadShaderMapComplete() || CandidateResource->GetCompileErrors().Num()
                || !CandidateMap || !CandidateMap->GetMeshShaderMap(&FLocalVertexFactory::StaticType)
                || (!ColumnDepth() && !APSSharedGeneratedLiquidMaterial::IsRenderReady(Candidate.Get(), S->ResolvedSurfaceProfile.LiquidType, S->GetWorld())))
            { Error = FString::Printf(TEXT("Water A/B LocalVF not ready column=%d resource=%d complete=%d errors=%d map=%d localVF=%d"),
                int(ColumnDepth()), int(CandidateResource != nullptr), int(CandidateResource && CandidateResource->IsGameThreadShaderMapComplete()),
                CandidateResource ? CandidateResource->GetCompileErrors().Num() : -1, int(CandidateMap != nullptr),
                int(CandidateMap && CandidateMap->GetMeshShaderMap(&FLocalVertexFactory::StaticType))); return false; }
            AWorldScapeRoot* R = S->WorldScapeRootInstance;
            Slots.Reset();
            int32 OceanSlots = 0, TerrainSlots = 0;
            for (int32 Group = 0; Group < 2; ++Group)
                for (const UWorldScapeLod* L : Group == 0 ? R->WorldScapeLod : R->WorldScapeLodOcean)
                {
                    if (!IsValid(L) || !IsValid(L->Mesh))
                    { Error = TEXT("Water A/B missing terrain/ocean LOD"); return false; }
                    for (int32 I = 0; I < L->Mesh->GetNumSections(); ++I)
                    {
                        const auto* Section = L->Mesh->GetProcMeshSection(I);
                        if (!Section || Section->PlanetVertexBuffer.IsEmpty()) continue;
                        if (Group == 1 && L->Mesh->GetMaterial(I) != M)
                        { Error = TEXT("Water A/B ocean slot is not the resolved production MID"); return false; }
                        FSlot& Slot = Slots.AddDefaulted_GetRef();
                        Slot.Mesh = L->Mesh; Slot.Index = I; Slot.bOcean = Group == 1;
                        Slot.Original.Reset(L->Mesh->GetMaterial(I)); Slot.Transform = L->Mesh->GetComponentTransform();
                        Slot.Hash = APSSharedTerrainLodAB::PayloadHash(*Section, true);
                        if (Slot.bOcean) ++OceanSlots; else ++TerrainSlots;
                    }
                }
            if (!OceanSlots || !TerrainSlots) { Error = TEXT("Water A/B requires actual terrain and ocean sections"); return false; }
            Root = R; Surface = S; Body = B; Native.Reset(M); RootTransform = R->GetActorTransform();
            bFrozen = R->bFreezeGeneration; bRootTick = R->IsActorTickEnabled();
            bSurfaceTick = S->IsActorTickEnabled(); bBodyTick = B->IsActorTickEnabled();
            R->bFreezeGeneration = true; R->SetActorTickEnabled(false); S->SetActorTickEnabled(false); B->SetActorTickEnabled(false);
            Phase = 0; bActive = true;
            if (SurfaceNormalBuffer() && (!NormalBuffer.Begin(Error) || !NormalBuffer.Apply(2, Error)))
            { Restore(); return false; }
            if (SurfaceAOIsolation())
            {
                ShortRangeAO = IConsoleManager::Get().FindConsoleVariable(TEXT("r.Lumen.ScreenProbeGather.ShortRangeAO"));
                OriginalShortRangeAO = ShortRangeAO->GetInt();
                OriginalAOSetBy = ShortRangeAO->GetFlags() & ECVF_SetByMask;
            }
            if (ColumnDepth())
            {
                Candidate->SetScalarParameterValue(TEXT("APS_WaterDepthStrength"), ColumnStrength);
                Candidate->SetScalarParameterValue(TEXT("APS_WaterHalfDepthM"), APSWaterDepthPalette::DefaultHalfDepthM);
                if (DepthControl.IsValid())
                {
                    DepthControl->SetScalarParameterValue(TEXT("APS_WaterDepthStrength"), ColumnStrength);
                    DepthControl->SetScalarParameterValue(TEXT("APS_WaterHalfDepthM"), APSWaterDepthPalette::DefaultHalfDepthM);
                }
                if (!BuildColumnPayload(Error)) { Restore(); return false; }
            }
            UE_LOG(LogTemp, Display, TEXT("[APS.WaterNormalAB] BEGIN material=%s parent=%s wave=%.9g terrainSlots=%d oceanSlots=%d; geometry frozen, visible terrain/atmosphere/lighting unchanged; no production performance claim"),
                *M->GetPathName(), *GetPathNameSafe(M->Parent.Get()), OriginalStrength, TerrainSlots, OceanSlots);
            return true;
        }
        bool Validate(FString& Error) const
        {
            if (SurfaceNormalBuffer() && !NormalBuffer.Validate(2, Error)) return false;
            if (SurfacePass() && (!Surface.IsValid()
                || !APSWaterSurfaceLighting::Matches(Candidate.Get(), Surface->GetWorld())))
            { Error = TEXT("Water-pass scene fill/settings changed during the frozen comparison"); return false; }
            if (ShortRangeAO && ShortRangeAO->GetInt() != (Phase == 3 ? 0 : OriginalShortRangeAO))
            { Error = TEXT("AO isolation CVar drift; captures are invalid"); return false; }
            auto* R = Root.Get();
            if (!bActive || !R || !Surface.IsValid() || !Body.IsValid() || !R->bFreezeGeneration
                || R->IsActorTickEnabled() || Surface->IsActorTickEnabled() || Body->IsActorTickEnabled()
                || Surface->WorldScapeRootInstance != R
                || Surface->ResolvedOceanMaterialInstance != Native.Get() || R->OceanMaterial.DefaultMaterial != Native.Get()
                || R->WorldScapeLodInGeneration.Num() || !R->GetActorTransform().Equals(RootTransform))
            { Error = TEXT("Water A/B production frame/authority drifted"); return false; }
            float Readback = -1.0f;
            UMaterialInterface* Expected = ExpectedMaterial();
            if (DomainAudit() && UsesCandidate())
            {
                float Mode = -1;
                if (!Expected->GetScalarParameterValue(FHashedMaterialParameterInfo(TEXT("APS_WaterDomainAuditMode")), Mode)
                    || Mode != Phase - 1) { Error = TEXT("Coordinate audit mode readback failed"); return false; }
            }
            const bool bLavaGeometry = GeometryOnly() && Surface->ResolvedSurfaceProfile.LiquidType == EAPSPlanetLiquidType::Lava;
            if (bLavaGeometry)
            {
                if (!APSSharedGeneratedLiquidMaterial::IsRenderReady(Expected, EAPSPlanetLiquidType::Lava, Surface->GetWorld()))
                { Error = TEXT("Lava native geometry material authority changed"); return false; }
            }
            else if (!Expected->GetScalarParameterValue(FHashedMaterialParameterInfo(TEXT("WaveNormalStrength")), Readback)
                || Readback != Strength()) { Error = TEXT("Water A/B scalar readback mismatch"); return false; }
            if (SurfaceFilter() && UsesCandidate())
            {
                const TCHAR* Keys[] = {TEXT("WaveScaleCm"), TEXT("PhysicalWaveDetailScaleCm"),
                    TEXT("WaveColorStrength"), TEXT("PhysicalWaveRoughnessStrength")};
                const float Filtered[] = {APSWaterSurfaceFilter::PrimaryScaleCm * SurfaceWaveScaleFactor(),
                    APSWaterSurfaceFilter::SecondaryScaleCm * SurfaceWaveScaleFactor(), 0.0f, 0.0f};
                for (int32 I = 0; I < UE_ARRAY_COUNT(Keys); ++I)
                {
                    float Wanted = Filtered[I], Actual = -1.0f;
                    if ((Phase == 1 && !SurfacePrecise() && !Native->GetScalarParameterValue(FHashedMaterialParameterInfo(Keys[I]), Wanted))
                        || !Expected->GetScalarParameterValue(FHashedMaterialParameterInfo(Keys[I]), Actual)
                        || !FMath::IsFinite(Actual) || Actual != Wanted)
                    { Error = FString::Printf(TEXT("Water wave readback mismatch: %s phase=%d"), Keys[I], Phase); return false; }
                }
            }
            for (const FSlot& Slot : Slots)
            {
                auto* Mesh = Slot.Mesh.Get();
                const auto* Section = Mesh ? Mesh->GetProcMeshSection(Slot.Index) : nullptr;
                if (!Section || !Mesh->GetComponentTransform().Equals(Slot.Transform)
                    || APSSharedTerrainLodAB::PayloadHash(*Section, true) != (Slot.bOcean && bDepthPayloadApplied ? Slot.DepthHash : Slot.Hash)
                    || Mesh->GetMaterial(Slot.Index) != (Slot.bOcean ? Expected : Slot.Original.Get()))
                { Error = TEXT("Water A/B terrain/ocean payload, transform or material changed"); return false; }
            }
            if (ColumnDepth() && UsesCandidate())
            {
                float DepthReadback = -1, HalfReadback = -1;
                if (!Expected->GetScalarParameterValue(FHashedMaterialParameterInfo(TEXT("APS_WaterDepthStrength")), DepthReadback)
                    || DepthReadback != DepthStrength()
                    || !Expected->GetScalarParameterValue(FHashedMaterialParameterInfo(TEXT("APS_WaterHalfDepthM")), HalfReadback)
                    || HalfReadback != APSWaterDepthPalette::DefaultHalfDepthM)
                { Error = TEXT("Column scalar readback mismatch"); return false; }
            }
            if (ColumnDepth())
            {
                UE_LOG(LogTemp, Display, TEXT("[APS.WaterColumnSnapshot] verified phase=%s wave=%.9g payloadHashesMatchExpected=1 columnUV1=%d"), Label(), Readback, int(bDepthPayloadApplied));
            }
            else
            {
                UE_LOG(LogTemp, Display, TEXT("[APS.WaterNormalAB] verified phase=%s wave=%.9g allTerrainOceanPayloadHashesUnchanged=1"), Label(), Readback);
            }
            if (GeometryOnly())
            {
                double MaxRadiusErrorCm = 0;
                int64 Vertices = 0;
                for (const auto& Slot : Slots)
                {
                    if (!Slot.bOcean) continue;
                    const auto* Section = Slot.Mesh->GetProcMeshSection(Slot.Index);
                    for (const auto& Vertex : Section->PlanetVertexBuffer)
                    {
                        const FVector World = Slot.Transform.TransformPosition(Vertex.Position);
                        const double Radius = R->WorldToECEF(World).ToFVector().Size();
                        MaxRadiusErrorCm = FMath::Max(MaxRadiusErrorCm,
                            FMath::Abs(Radius - (double(R->PlanetScale) + double(R->OceanHeight))));
                        ++Vertices;
                    }
                }
                UE_LOG(LogTemp, Display, TEXT("[APS.WaterShape] vertices=%lld maxRadialErrorCm=%.9g; vertex radius only, triangle chord error is separate"), Vertices, MaxRadiusErrorCm);
#if WITH_EDITOR
                UMaterialInstanceDynamic* AuditedMaterials[] = {Surface->ResolvedTerrainMaterialInstance, Surface->ResolvedOceanMaterialInstance};
                for (auto* Material : AuditedMaterials)
                    if (Material && Material->GetMaterial())
                    {
                        auto* Master = Material->GetMaterial();
                        for (EMaterialProperty Property : {MP_OpacityMask, MP_WorldPositionOffset, MP_PixelDepthOffset})
                        {
                            auto* Input = Master->GetExpressionInputForProperty(Property);
                            UMaterialExpression* Expression = Input ? Input->Expression : nullptr;
                            auto* Constant = Cast<UMaterialExpressionConstant>(Expression);
                            UE_LOG(LogTemp, Display, TEXT("[APS.CoastGraph] master=%s property=%d node=%s constant=%g"),
                                *Master->GetPathName(), int32(Property), *GetNameSafe(Expression), Constant ? Constant->R : -999.f);
                        }
                    }
#endif
                struct FReporter { void AddInfo(const FString& Message) { UE_LOG(LogTemp, Display, TEXT("%s"), *Message); } } Reporter;
                const FString Folder = FPaths::ProjectSavedDir() / TEXT("Diagnostics/CoastGeometry");
                IFileManager::Get().MakeDirectory(*Folder, true);
                auto* PC = R->GetWorld()->GetFirstPlayerController();
                if (!PC || !APSCoastGeometryProbe::Capture(Reporter, *R, *PC,
                    Folder / FString::Printf(TEXT("water-%lld.csv"), FDateTime::UtcNow().GetTicks()), Phase))
                { Error = TEXT("No valid visible coast geometry sample; inspect framing/topology before interpreting the capture"); return false; }
            }
            return true;
        }
        bool Advance()
        {
            if (!bActive) return false;
            if (ColumnDepth())
            {
                if (++Phase > (SurfaceAOIsolation() ? 5 : SurfaceFilter() ? 4 : 3)) { Restore(); return false; }
                if (ShortRangeAO)
                {
                    ShortRangeAO->Set(Phase == 3 ? 0 : OriginalShortRangeAO, ECVF_SetByCode);
                    UE_LOG(LogTemp, Display, TEXT("[APS.WaterAOIsolation] phase=%s shortAO=%d diagnosticOnly=1 sameWaveDepthPalette=1 NOT a production setting or performance result"), Label(), ShortRangeAO->GetInt());
                }
                if (!SetColumnPayload(UsesCandidate())) { Restore(); return false; }
                Candidate->SetScalarParameterValue(TEXT("APS_WaterDepthStrength"), DepthStrength());
                if (DomainAudit()) Candidate->SetScalarParameterValue(TEXT("APS_WaterDomainAuditMode"), float(Phase - 1));
                // On/off/on of exactly one scalar on the same MID. Zero waves
                // isolate the gradient path; this is NOT a flat-water fix.
                if (SurfaceWaveIsolation()) Candidate->SetScalarParameterValue(TEXT("WaveNormalStrength"), Strength());
                for (FSlot& Slot : Slots)
                    if (Slot.bOcean)
                        if (auto* Mesh = Slot.Mesh.Get()) Mesh->SetMaterial(Slot.Index, ExpectedMaterial());
                return true;
            }
            // A paired coastal-field run needs the unchanged production water
            // at each angle, not the previously rejected amplitude experiment.
            if (GeometryOnly())
            {
                // No material rebind, scalar change or thaw between the small
                // camera rotations. Validate hashes every captured phase.
                if (CameraMotion() && ++Phase <= 3) return true;
                Restore(); return false;
            }
            if (++Phase > 5) { Restore(); return false; }
            Candidate->SetScalarParameterValue(TEXT("WaveNormalStrength"), Strength());
            for (FSlot& Slot : Slots)
                if (Slot.bOcean)
                    if (auto* Mesh = Slot.Mesh.Get()) Mesh->SetMaterial(Slot.Index, UsesCandidate() ? Candidate.Get() : Native.Get());
            return true;
        }
        void Restore()
        {
            NormalBuffer.Restore();
            if (ShortRangeAO)
            {
                ShortRangeAO->Set(OriginalShortRangeAO, ECVF_SetByCode);
                ensureAlwaysMsgf(ShortRangeAO->GetInt() == OriginalShortRangeAO, TEXT("Water diagnostic must restore ShortRangeAO"));
                ShortRangeAO->ClearFlags(ECVF_SetByMask);
                ShortRangeAO->SetFlags(static_cast<EConsoleVariableFlags>(OriginalAOSetBy));
                UE_LOG(LogTemp, Display, TEXT("[APS.WaterAOIsolation] restored ShortRangeAO=%d"), ShortRangeAO->GetInt());
                ShortRangeAO = nullptr;
            }
            if (bActive)
            {
                if (bDepthPayloadApplied) SetColumnPayload(false);
                for (FSlot& Slot : Slots)
                    if (Slot.bOcean)
                        if (auto* Mesh = Slot.Mesh.Get()) Mesh->SetMaterial(Slot.Index, Slot.Original.Get());
                if (auto* R = Root.Get()) { R->bFreezeGeneration = bFrozen; R->SetActorTickEnabled(bRootTick); }
                if (auto* S = Surface.Get()) S->SetActorTickEnabled(bSurfaceTick);
                if (auto* B = Body.Get()) B->SetActorTickEnabled(bBodyTick);
                UE_LOG(LogTemp, Display, TEXT("[APS.WaterNormalAB] original bindings/freeze/ticks restored; original MID and assets never edited"));
            }
            bActive = false; Slots.Reset(); Native.Reset(); Candidate.Reset(); DepthControl.Reset(); Root.Reset(); Surface.Reset(); Body.Reset();
        }
    };
}
#endif
