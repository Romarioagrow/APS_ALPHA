#pragma once

#if WITH_DEV_AUTOMATION_TESTS
#include "APSSharedTerrainLodABProbe.h"
#include "APS_ALPHA/Core/Planetary/APSSharedGeneratedLiquidMaterial.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"

// Diagnostic only. Reuses the settled production observer, light and material.
// No new shader, hidden terrain, saved MIC edit, depth payload or water geometry.
namespace APSWaterNormalAB
{
    inline bool Enabled() { return FParse::Param(FCommandLine::Get(), TEXT("APSProbeWaterNormalAB")); }
    inline bool OpenWater() { return FParse::Param(FCommandLine::Get(), TEXT("APSWaterABOpenWater")); }

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
            EAPSPlanetLiquidType::Water, R->GetWorld());
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
            if (H >= 2000.0) Wet.Add({D, H});
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
            // Bracket a point 20m underwater. This is observer selection only;
            // the terrain function and all generated heights remain untouched.
            for (int32 I = 0; I < 40; ++I)
            {
                const FVector Mid = (Land + Sea).GetSafeNormal();
                const double H = Depth(Mid);
                if (!FMath::IsFinite(H)) { Error = TEXT("Water A/B coast refinement is non-finite"); return false; }
                if (H >= 2000.0) Sea = Mid; else Land = Mid;
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
        const double SelectedDepth = Depth(Outward);
        if (!FMath::IsFinite(SelectedDepth) || SelectedDepth < 1999.0)
        { Error = TEXT("Water A/B selected observer is not over water"); return false; }
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
            bool bOcean = false;
        };
        TArray<FSlot> Slots;
        TWeakObjectPtr<AWorldScapeRoot> Root;
        TWeakObjectPtr<APlanetarySurfaceGenerator> Surface;
        TWeakObjectPtr<AActor> Body;
        TStrongObjectPtr<UMaterialInstanceDynamic> Native{nullptr}, Candidate{nullptr};
        FTransform RootTransform;
        float OriginalStrength = 0.0f;
        int32 Phase = 0;
        bool bActive = false, bFrozen = false, bRootTick = false, bSurfaceTick = false, bBodyTick = false;

        float Strength() const { return OriginalStrength * (Phase == 2 ? 0.4f : Phase == 3 ? 0.2f : Phase == 4 ? 0.0f : 1.0f); }
        bool UsesCandidate() const { return Phase >= 1 && Phase <= 4; }
    public:
        ~FLease() { Restore(); }
        bool IsActive() const { return bActive; }
        const TCHAR* Label() const
        {
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
            if (bActive || !IsValid(B) || !IsSettled(S)
                || S->ResolvedSurfaceProfile.LiquidType != EAPSPlanetLiquidType::Water)
            { Error = TEXT("Water A/B requires settled authoritative Water, no workers"); return false; }
            auto* M = S->ResolvedOceanMaterialInstance;
            if (!IsValid(M) || !M->GetScalarParameterValue(FHashedMaterialParameterInfo(TEXT("WaveNormalStrength")), OriginalStrength)
                || !FMath::IsFinite(OriginalStrength) || OriginalStrength <= 0.0f)
            { Error = TEXT("Water A/B production wave parameter absent/nonpositive"); return false; }
            Candidate.Reset(UMaterialInstanceDynamic::Create(M->Parent.Get(), GetTransientPackage()));
            if (!Candidate.IsValid()) { Error = TEXT("Water A/B temporary MID creation failed"); return false; }
            Candidate->CopyMaterialUniformParameters(M);
            if (!APSSharedGeneratedLiquidMaterial::IsRenderReady(Candidate.Get(), EAPSPlanetLiquidType::Water, S->GetWorld()))
            { Error = TEXT("Water A/B native-copy LocalVF not ready"); return false; }
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
            UE_LOG(LogTemp, Display, TEXT("[APS.WaterNormalAB] BEGIN material=%s parent=%s wave=%.9g terrainSlots=%d oceanSlots=%d; geometry frozen, visible terrain/atmosphere/lighting unchanged; no production performance claim"),
                *M->GetPathName(), *GetPathNameSafe(M->Parent.Get()), OriginalStrength, TerrainSlots, OceanSlots);
            return true;
        }
        bool Validate(FString& Error) const
        {
            auto* R = Root.Get();
            if (!bActive || !R || !Surface.IsValid() || Surface->WorldScapeRootInstance != R
                || Surface->ResolvedOceanMaterialInstance != Native.Get() || R->OceanMaterial.DefaultMaterial != Native.Get()
                || R->WorldScapeLodInGeneration.Num() || !R->GetActorTransform().Equals(RootTransform))
            { Error = TEXT("Water A/B production frame/authority drifted"); return false; }
            float Readback = -1.0f;
            UMaterialInterface* Expected = UsesCandidate() ? Candidate.Get() : Native.Get();
            if (!Expected->GetScalarParameterValue(FHashedMaterialParameterInfo(TEXT("WaveNormalStrength")), Readback)
                || Readback != Strength()) { Error = TEXT("Water A/B scalar readback mismatch"); return false; }
            for (const FSlot& Slot : Slots)
            {
                auto* Mesh = Slot.Mesh.Get();
                const auto* Section = Mesh ? Mesh->GetProcMeshSection(Slot.Index) : nullptr;
                if (!Section || !Mesh->GetComponentTransform().Equals(Slot.Transform)
                    || APSSharedTerrainLodAB::PayloadHash(*Section, true) != Slot.Hash
                    || Mesh->GetMaterial(Slot.Index) != (Slot.bOcean ? Expected : Slot.Original.Get()))
                { Error = TEXT("Water A/B terrain/ocean payload, transform or material changed"); return false; }
            }
            UE_LOG(LogTemp, Display, TEXT("[APS.WaterNormalAB] verified phase=%s wave=%.9g allTerrainOceanPayloadHashesUnchanged=1"), Label(), Readback);
            return true;
        }
        bool Advance()
        {
            if (!bActive) return false;
            if (++Phase > 5) { Restore(); return false; }
            Candidate->SetScalarParameterValue(TEXT("WaveNormalStrength"), Strength());
            for (FSlot& Slot : Slots)
                if (Slot.bOcean)
                    if (auto* Mesh = Slot.Mesh.Get()) Mesh->SetMaterial(Slot.Index, UsesCandidate() ? Candidate.Get() : Native.Get());
            return true;
        }
        void Restore()
        {
            if (bActive)
            {
                for (FSlot& Slot : Slots)
                    if (Slot.bOcean)
                        if (auto* Mesh = Slot.Mesh.Get()) Mesh->SetMaterial(Slot.Index, Slot.Original.Get());
                if (auto* R = Root.Get()) { R->bFreezeGeneration = bFrozen; R->SetActorTickEnabled(bRootTick); }
                if (auto* S = Surface.Get()) S->SetActorTickEnabled(bSurfaceTick);
                if (auto* B = Body.Get()) B->SetActorTickEnabled(bBodyTick);
                UE_LOG(LogTemp, Display, TEXT("[APS.WaterNormalAB] original bindings/freeze/ticks restored; original MID and assets never edited"));
            }
            bActive = false; Slots.Reset(); Native.Reset(); Candidate.Reset(); Root.Reset(); Surface.Reset(); Body.Reset();
        }
    };
}
#endif
