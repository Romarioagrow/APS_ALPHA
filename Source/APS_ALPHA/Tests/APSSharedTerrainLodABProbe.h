#pragma once

#if WITH_DEV_AUTOMATION_TESTS
#include "APS_ALPHA/Generation/PlanetarySurfaceGenerator.h"
#include "APS_ALPHA/Core/Planetary/APSSharedTerrainMaterial.h"
#include "Engine/World.h"
#include "LocalVertexFactory.h"
#include "MaterialShared.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#if WITH_EDITOR
#include "ShaderCompiler.h"
#endif
#include "UObject/StrongObjectPtr.h"

// A small test-only lease for the EXISTING settled orbit camera. The fixture owns
// capture/2s settling and invokes Restore before its normal world cleanup.
// No palette/mesh/noise/source-asset edits. No external camera or lighting changes.
namespace APSSharedTerrainLodAB
{
    enum class ECandidateReadiness : uint8 { Ready, Pending, Failed };

    inline uint32 PayloadHash(const FWorldScapeMeshSection& S, bool IncludeNormals)
    {
        uint32 H = GetTypeHash(S.PlanetVertexBuffer.Num());
        for (const FWorldScapeMeshVertex& V : S.PlanetVertexBuffer)
        {
            H = HashCombine(H, GetTypeHash(V.Position));
            if (IncludeNormals) H = HashCombine(H, GetTypeHash(V.Normal));
            H = HashCombine(H, GetTypeHash(V.Tangent.TangentX));
            H = HashCombine(H, GetTypeHash(V.Tangent.bFlipTangentY));
            for (const FVector2D& UV : {V.UV0, V.UV1, V.UV2, V.UV3}) H = HashCombine(H, GetTypeHash(UV));
            H = HashCombine(H, GetTypeHash(V.Color));
        }
        for (uint32 I : S.PlanetIndexBuffer) H = HashCombine(H, GetTypeHash(I));
        H = HashCombine(H, GetTypeHash(S.PlanetIndexBuffer.Num()));
        H = HashCombine(H, GetTypeHash(S.bEnableCollision));
        return HashCombine(H, GetTypeHash(S.bSectionVisible));
    }

    class FLease
    {
        struct FSlot
        {
            TWeakObjectPtr<UWorldScapeMeshComponent> Mesh;
            int32 Index = INDEX_NONE;
            FTransform Transform;
            TArray<FVector> Positions, Normals, RadialNormals;
            TStrongObjectPtr<UMaterialInterface> Material{nullptr};
            TStrongObjectPtr<UMaterialInstanceDynamic> PixelMaterial{nullptr};
            TStrongObjectPtr<UMaterialInstanceDynamic> FarNormalMaterial{nullptr};
            uint32 Payload = 0, FullPayload = 0;
        };
        TWeakObjectPtr<AWorldScapeRoot> Root;
        TWeakObjectPtr<APlanetarySurfaceGenerator> Surface;
        TWeakObjectPtr<AActor> Body;
        TArray<FSlot> Slots;
        TStrongObjectPtr<UMaterialInstance> CandidateTemplate{nullptr};
        TStrongObjectPtr<UMaterialInstance> FarNormalTemplate{nullptr};
        bool bCandidateCompileRequested = false;
        bool bFarNormalCompileRequested = false;
        const bool bFarNormalAB = FParse::Param(FCommandLine::Get(), TEXT("APSProbeFarNormalAB"));
        FTransform RootTransform;
        bool bFrozen = false, bRootTick = false, bSurfaceTick = false, bBodyTick = false, bLeased = false;
        int32 Phase = 0;

        ECandidateReadiness PrepareFarNormalCandidate(UWorld* World, FString& Error)
        {
            if (!bFarNormalAB) return ECandidateReadiness::Ready;
            if (!FarNormalTemplate.IsValid())
                FarNormalTemplate.Reset(LoadObject<UMaterialInstance>(nullptr,
                    TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/Diagnostics/LodFarNormalAB/MI_APS_LodFarNormalTerra.MI_APS_LodFarNormalTerra")));
            if (!FarNormalTemplate.IsValid() || !FarNormalTemplate->Parent
                || FarNormalTemplate->Parent->GetPathName() != TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/Diagnostics/LodFarNormalAB/M_APS_LodFarNormalTerrain.M_APS_LodFarNormalTerrain"))
            { Error = TEXT("Exact AB3 far-normal candidate missing; run OnlySharedTerrainFarNormalAB first"); return ECandidateReadiness::Failed; }
            FMaterialResource* Resource = FarNormalTemplate->GetMaterialResource(World->GetFeatureLevel());
            if (Resource && Resource->GetCompileErrors().Num())
            {
                Error = TEXT("AB3 far-normal shader compilation errors");
                for (const FString& CompileError : Resource->GetCompileErrors()) Error += TEXT("\n") + CompileError;
                return ECandidateReadiness::Failed;
            }
            if (Resource && !bFarNormalCompileRequested)
            {
#if WITH_EDITOR
                if (!Resource->IsGameThreadShaderMapComplete())
                    Resource->SubmitCompileJobs_GameThread(EShaderCompileJobPriority::ForceLocal);
#endif
                FPSOPrecacheParams Params;
                Params.bStaticLighting = false; Params.bCastShadow = true;
                Params.SetMobility(EComponentMobility::Movable);
                static_cast<UMaterialInterface*>(FarNormalTemplate.Get())->PrecachePSOs(&FLocalVertexFactory::StaticType, Params);
                bFarNormalCompileRequested = true;
                UE_LOG(LogTemp, Display, TEXT("[APS.LodAB] AB3 async warmup requested once material=%s feature=%d fadePhysicalKm=200..700"),
                    *FarNormalTemplate->GetPathName(), static_cast<int32>(World->GetFeatureLevel()));
            }
            const FMaterialShaderMap* Map = Resource ? Resource->GetGameThreadShaderMap() : nullptr;
            return Resource && Resource->IsGameThreadShaderMapComplete() && Map && Map->GetMeshShaderMap(&FLocalVertexFactory::StaticType)
                ? ECandidateReadiness::Ready : ECandidateReadiness::Pending;
        }

        static void WriteNormals(FSlot& Slot, const TArray<FVector>& Normals)
        {
            if (auto* Mesh = Slot.Mesh.Get())
            {
                // This API only updates if the unchanged positions are supplied.
                // Empty UV/color/tangent arrays preserve every other payload field.
                const TArray<FVector2D> UV;
                const TArray<FColor> Colors;
                const TArray<FWorldScapeMeshTangent> Tangents;
                Mesh->UpdateMeshSection(Slot.Index, Slot.Positions, Normals, UV, Colors, Tangents);
            }
        }

    public:
        ~FLease() { Restore(); }
        bool IsActive() const { return bLeased; }
        ECandidateReadiness PrepareCandidate(UWorld* World, FString& Error)
        {
            Error.Reset();
            if (!IsValid(World))
            { Error = TEXT("LOD A/B candidate warmup has no world"); return ECandidateReadiness::Failed; }
            if (!CandidateTemplate.IsValid())
                CandidateTemplate.Reset(LoadObject<UMaterialInstance>(nullptr,
                    TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/Diagnostics/LodPixelAB/MI_APS_LodPixelTerra.MI_APS_LodPixelTerra")));
            if (!CandidateTemplate.IsValid())
            { Error = TEXT("LOD A/B candidate missing; run OnlySharedTerrainLodAB first"); return ECandidateReadiness::Failed; }
            FMaterialResource* Resource = CandidateTemplate->GetMaterialResource(World->GetFeatureLevel());
            if (Resource && Resource->GetCompileErrors().Num())
            {
                Error = TEXT("LOD A/B candidate shader compilation errors");
                for (const FString& CompileError : Resource->GetCompileErrors()) Error += TEXT("\n") + CompileError;
                return ECandidateReadiness::Failed;
            }
            if (Resource && !bCandidateCompileRequested)
            {
                // Uncooked loading may provide only a partial deferred shader map.
                // Request once and poll; never stall the game thread to compile.
#if WITH_EDITOR
                if (!Resource->IsGameThreadShaderMapComplete())
                    Resource->SubmitCompileJobs_GameThread(EShaderCompileJobPriority::ForceLocal);
#endif
                FPSOPrecacheParams Params;
                Params.bStaticLighting = false;
                Params.bCastShadow = true;
                Params.SetMobility(EComponentMobility::Movable);
                static_cast<UMaterialInterface*>(CandidateTemplate.Get())->PrecachePSOs(&FLocalVertexFactory::StaticType, Params);
                bCandidateCompileRequested = true;
                UE_LOG(LogTemp, Display, TEXT("[APS.LodAB] Candidate async warmup requested once material=%s feature=%d"),
                    *CandidateTemplate->GetPathName(), static_cast<int32>(World->GetFeatureLevel()));
            }
            const FMaterialShaderMap* Map = Resource ? Resource->GetGameThreadShaderMap() : nullptr;
            if (!Resource || !Resource->IsGameThreadShaderMapComplete() || !Map
                || !Map->GetMeshShaderMap(&FLocalVertexFactory::StaticType))
                return ECandidateReadiness::Pending;
            return PrepareFarNormalCandidate(World, Error);
        }
        const TCHAR* Label() const
        {
            return Phase == 0 ? TEXT("AB0Native") : Phase == 1 ? TEXT("AB1RadialMeshNormals")
                : Phase == 2 ? TEXT("AB2PixelInterpolators") : TEXT("AB3PhysicalFarNormal");
        }

        bool Begin(APlanetarySurfaceGenerator* InSurface, AActor* InBody, FString& Error)
        {
            if (bLeased) { Error = TEXT("LOD A/B lease already active"); return false; }
            AWorldScapeRoot* R = IsValid(InSurface) ? InSurface->WorldScapeRootInstance : nullptr;
            if (!IsValid(InBody) || !IsValid(R) || R->WorldScapeLodInGeneration.Num() || R->WorldScapeLod.IsEmpty())
            { Error = TEXT("LOD A/B requires an already settled production root"); return false; }
            UMaterialInstance* Template = CandidateTemplate.Get();
            FMaterialResource* Resource = Template ? Template->GetMaterialResource(R->GetWorld()->GetFeatureLevel()) : nullptr;
            const FMaterialShaderMap* Map = Resource ? Resource->GetGameThreadShaderMap() : nullptr;
            if (!bCandidateCompileRequested || !Template || !Resource || !Resource->IsGameThreadShaderMapComplete()
                || Resource->GetCompileErrors().Num() || !Map || !Map->GetMeshShaderMap(&FLocalVertexFactory::StaticType))
            { Error = TEXT("LOD A/B candidate is not fully ready; poll PrepareCandidate before Begin"); return false; }
            if (PrepareFarNormalCandidate(R->GetWorld(), Error) != ECandidateReadiness::Ready)
            { if (Error.IsEmpty()) Error = TEXT("AB3 candidate still pending"); return false; }
            // Snapshot everything before any mesh/material writes. This Ice-only
            // probe requires the actual native Terra static template, not Magma.
            for (UWorldScapeLod* Lod : R->WorldScapeLod)
            {
                if (!IsValid(Lod) || !IsValid(Lod->Mesh) || Lod->WaterBody)
                { Error = TEXT("LOD A/B invalid terrain LOD"); return false; }
                for (int32 I = 0; I < Lod->Mesh->GetNumSections(); ++I)
                {
                    const FWorldScapeMeshSection* Section = Lod->Mesh->GetProcMeshSection(I);
                    if (!Section || Section->PlanetVertexBuffer.IsEmpty()) continue;
                    auto* Current = Cast<UMaterialInstanceDynamic>(Lod->Mesh->GetMaterial(I));
                    if (!APSSharedTerrainMaterial::IsSharedStack(Current) || !Current->Parent
                        || Current->Parent->GetPathName() != TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/Shared/MI_APS_SharedTerra.MI_APS_SharedTerra"))
                    { Error = TEXT("LOD A/B requires generated Terra MID on each actual slot"); return false; }
                    FSlot& S = Slots.AddDefaulted_GetRef();
                    S.Mesh = Lod->Mesh; S.Index = I; S.Transform = Lod->Mesh->GetComponentTransform();
                    S.Material.Reset(Current);
                    S.PixelMaterial.Reset(UMaterialInstanceDynamic::Create(Template, R));
                    S.PixelMaterial->CopyMaterialUniformParameters(Current);
                    if (bFarNormalAB)
                    {
                        S.FarNormalMaterial.Reset(UMaterialInstanceDynamic::Create(FarNormalTemplate.Get(), R));
                        S.FarNormalMaterial->CopyMaterialUniformParameters(Current);
                    }
                    S.Payload = PayloadHash(*Section, false); S.FullPayload = PayloadHash(*Section, true);
                    for (const FWorldScapeMeshVertex& V : Section->PlanetVertexBuffer)
                    {
                        S.Positions.Add(V.Position); S.Normals.Add(V.Normal);
                        const FVector WorldNormal = (S.Transform.TransformPosition(V.Position) - R->GetActorLocation()).GetSafeNormal();
                        const FVector LocalNormal = S.Transform.InverseTransformVectorNoScale(WorldNormal).GetSafeNormal();
                        if (LocalNormal.IsNearlyZero() || LocalNormal.ContainsNaN())
                        { Error = TEXT("LOD A/B invalid analytical radial normal"); return false; }
                        S.RadialNormals.Add(LocalNormal);
                    }
                }
            }
            if (Slots.IsEmpty()) { Error = TEXT("LOD A/B has no actual terrain sections"); return false; }
            Root = R; Surface = InSurface; Body = InBody; RootTransform = R->GetActorTransform();
            bFrozen = R->bFreezeGeneration; bRootTick = R->IsActorTickEnabled(); bSurfaceTick = InSurface->IsActorTickEnabled();
            bBodyTick = InBody->IsActorTickEnabled(); InBody->SetActorTickEnabled(false);
            R->bFreezeGeneration = true; R->SetActorTickEnabled(false); InSurface->SetActorTickEnabled(false);
            Phase = 0; bLeased = true;
            UE_LOG(LogTemp, Display, TEXT("[APS.LodAB] Lease begun sections=%d camera/lighting unchanged; temporary frozen payload for independent A/B, not production performance"), Slots.Num());
            return true;
        }

        bool Validate(FString& Error) const
        {
            AWorldScapeRoot* R = Root.Get();
            if (!bLeased || !R || !Surface.IsValid() || Surface->WorldScapeRootInstance != R
                || R->WorldScapeLodInGeneration.Num() || !R->GetActorTransform().Equals(RootTransform))
            { Error = TEXT("LOD A/B root changed during lease"); return false; }
            for (const FSlot& S : Slots)
            {
                auto* Mesh = S.Mesh.Get();
                const auto* Section = Mesh ? Mesh->GetProcMeshSection(S.Index) : nullptr;
                if (!Section || !Mesh->GetComponentTransform().Equals(S.Transform) || PayloadHash(*Section, false) != S.Payload
                    || (Phase != 1 && PayloadHash(*Section, true) != S.FullPayload)
                    || Mesh->GetMaterial(S.Index) != (Phase == 3 ? static_cast<UMaterialInterface*>(S.FarNormalMaterial.Get())
                        : Phase == 2 ? static_cast<UMaterialInterface*>(S.PixelMaterial.Get()) : S.Material.Get()))
                { Error = TEXT("LOD A/B actual mesh payload/material/transform drifted"); return false; }
                if (Phase == 1)
                    for (int32 I = 0; I < S.RadialNormals.Num(); ++I)
                        if (Section->PlanetVertexBuffer[I].Normal != S.RadialNormals[I])
                        { Error = TEXT("LOD A/B analytical radial normals were not published"); return false; }
            }
            return true;
        }

        // True means another variant needs its own >=2s / stable-camera capture.
        bool Advance()
        {
            if (!bLeased) return false;
            if (++Phase == 1)
                for (FSlot& S : Slots) WriteNormals(S, S.RadialNormals);
            else if (Phase == 2)
                for (FSlot& S : Slots)
                {
                    WriteNormals(S, S.Normals);
                    if (auto* Mesh = S.Mesh.Get()) Mesh->SetMaterial(S.Index, S.PixelMaterial.Get());
                }
            else if (Phase == 3 && bFarNormalAB)
            {
                for (FSlot& S : Slots)
                    if (auto* Mesh = S.Mesh.Get()) Mesh->SetMaterial(S.Index, S.FarNormalMaterial.Get());
            }
            else { Restore(); return false; }
            UE_LOG(LogTemp, Display, TEXT("[APS.LodAB] phase=%s; original positions/indices/UV/RGBA/tangents, fixed camera and light"), Label());
            return true;
        }

        void Restore()
        {
            if (!bLeased) { Slots.Reset(); return; }
            for (FSlot& S : Slots)
            {
                if (auto* Mesh = S.Mesh.Get())
                {
                    // Never replace a concurrently regenerated payload with an old
                    // snapshot. Such a run fails Validate and is not A/B evidence.
                    const auto* Before = Mesh->GetProcMeshSection(S.Index);
                    if (Before && PayloadHash(*Before, false) == S.Payload) WriteNormals(S, S.Normals);
                    else UE_LOG(LogTemp, Error, TEXT("[APS.LodAB] Payload drifted; refused stale normal/position restore mesh=%s slot=%d"), *Mesh->GetPathName(), S.Index);
                    Mesh->SetMaterial(S.Index, S.Material.Get());
                    const auto* Section = Mesh->GetProcMeshSection(S.Index);
                    ensureMsgf(Section && PayloadHash(*Section, true) == S.FullPayload, TEXT("LOD A/B full payload restore mismatch"));
                }
            }
            if (auto* R = Root.Get()) { R->bFreezeGeneration = bFrozen; R->SetActorTickEnabled(bRootTick); }
            if (auto* S = Surface.Get()) S->SetActorTickEnabled(bSurfaceTick);
            if (auto* B = Body.Get()) B->SetActorTickEnabled(bBodyTick);
            bLeased = false; Slots.Reset(); Root.Reset(); Surface.Reset(); Body.Reset();
            UE_LOG(LogTemp, Display, TEXT("[APS.LodAB] Original terrain normals/materials/freeze/ticks restored"));
        }
    };
}
#endif
