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
#include "APSPlanetBufferViewsProbe.h"
#include "APSPlanetNormalSourcesProbe.h"
#include "APSMeshCurvatureProbe.h"

// A small test-only lease for the EXISTING settled orbit camera. The fixture owns
// capture/2s settling and invokes Restore before its normal world cleanup.
// No palette/mesh/noise/source-asset edits. No external camera or lighting changes.
namespace APSSharedTerrainLodAB
{
    inline bool UsesOrbitalFields() { return FParse::Param(FCommandLine::Get(), TEXT("APSProbeOrbitalFieldsAB")); }
    inline bool UsesFieldsFlight() { return FParse::Param(FCommandLine::Get(), TEXT("APSProbeOrbitalFieldsFlight")); }
    inline bool UsesPublishedFlight() { return FParse::Param(FCommandLine::Get(), TEXT("APSProbePublishedTerrainFlight")); }
    inline bool UsesCombined() { return FParse::Param(FCommandLine::Get(), TEXT("APSProbeContinuityCombined")); }
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
            TArray<FVector> Positions, Normals, RadialNormals, CurvatureNormals;
            TStrongObjectPtr<UMaterialInterface> Material{nullptr};
            TStrongObjectPtr<UMaterialInstanceDynamic> PixelMaterial{nullptr};
            TStrongObjectPtr<UMaterialInstanceDynamic> FarNormalMaterial{nullptr};
            APSPlanetNormalSources::FScope NormalSources;
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
        const bool bMacroAB = FParse::Param(FCommandLine::Get(), TEXT("APSProbeMacroAB"));
        const bool bNormalHex = FParse::Param(FCommandLine::Get(), TEXT("APSProbeNormalHex"));
        const bool bNormalWarp = bNormalHex || FParse::Param(FCommandLine::Get(), TEXT("APSProbeNormalMacroWarp"));
        const bool bBuffers = APSPlanetBufferViews::Requested();
        const bool bNormalSources = APSPlanetNormalSources::Requested();
        APSPlanetBufferViews::FScope BufferViews;
        FString BufferError;
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
        UMaterialInstance* PreparedTemplate() const { return CandidateTemplate.Get(); }
        ECandidateReadiness PrepareCandidate(UWorld* World, FString& Error)
        {
            Error.Reset();
            if (!IsValid(World))
            { Error = TEXT("LOD A/B candidate warmup has no world"); return ECandidateReadiness::Failed; }
            if (bNormalWarp && (!bMacroAB || FParse::Param(FCommandLine::Get(), TEXT("APSProbeMacroApproachRange"))))
            { Error = TEXT("Normal domain comparison requires isolated macro route without approach variant"); return ECandidateReadiness::Failed; }
            if(UsesCombined() && !bNormalHex)
            {Error=TEXT("Combined continuity comparison requires the normal-hex route");return ECandidateReadiness::Failed;}
            if (int32(bMacroAB) + int32(bFarNormalAB) + int32(UsesOrbitalFields()) + int32(bBuffers) + int32(bNormalSources) > 1)
            { Error = TEXT("Macro and far-normal comparisons must be isolated"); return ECandidateReadiness::Failed; }
            if (bBuffers || bNormalSources) return ECandidateReadiness::Ready; // Native shader, no candidate warmup.
            if (FParse::Param(FCommandLine::Get(), TEXT("APSProbeMacroApproachRange")) && !bMacroAB)
            { Error = TEXT("Approach range probe requires the isolated macro comparison"); return ECandidateReadiness::Failed; }
            if (!CandidateTemplate.IsValid())
                CandidateTemplate.Reset(LoadObject<UMaterialInstance>(nullptr,
                    UsesCombined() ? TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/Diagnostics/ContinuityCombined20260929V1/MI_APS_NormalWarpTerra.MI_APS_NormalWarpTerra")
                        : bNormalHex ? TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/Diagnostics/NormalHex20260929V1/MI_APS_NormalWarpTerra.MI_APS_NormalWarpTerra")
                        : bNormalWarp ? TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/Diagnostics/NormalMacroWarp20260929V1/MI_APS_NormalWarpTerra.MI_APS_NormalWarpTerra")
                        : UsesOrbitalFields() ? TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/Diagnostics/OrbitalFields20260929V3/MI_APS_LodPixelTerra.MI_APS_LodPixelTerra")
                        : bMacroAB ? (FParse::Param(FCommandLine::Get(), TEXT("APSProbeMacroApproachRange"))
                            ? TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/Diagnostics/MacroApproach20260929V1/MI_APS_MacroTerra.MI_APS_MacroTerra")
                            : TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/Diagnostics/MacroAB20260928V2/MI_APS_MacroTerra.MI_APS_MacroTerra"))
                        : TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/Diagnostics/LodPixelAB/MI_APS_LodPixelTerra.MI_APS_LodPixelTerra")));
            if (!CandidateTemplate.IsValid())
            { Error = TEXT("LOD A/B candidate missing; run OnlySharedTerrainLodAB first"); return ECandidateReadiness::Failed; }
            if (bNormalWarp && (!CandidateTemplate->Parent || CandidateTemplate->Parent->GetPathName()!=
                (UsesCombined() ? TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/Diagnostics/ContinuityCombined20260929V1/M_APS_NormalWarpTerrain.M_APS_NormalWarpTerrain")
                    : bNormalHex ? TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/Diagnostics/NormalHex20260929V1/M_APS_NormalWarpTerrain.M_APS_NormalWarpTerrain")
                    : TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/Diagnostics/NormalMacroWarp20260929V1/M_APS_NormalWarpTerrain.M_APS_NormalWarpTerrain"))))
            { Error=TEXT("Normal warp comparison has wrong master"); return ECandidateReadiness::Failed; }
            if (FParse::Param(FCommandLine::Get(), TEXT("APSProbeMacroApproachRange"))
                && (!CandidateTemplate->Parent || CandidateTemplate->Parent->GetPathName() !=
                    TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/Diagnostics/MacroApproach20260929V1/M_APS_MacroTerrain.M_APS_MacroTerrain")))
            { Error = TEXT("Approach range candidate has the wrong master"); return ECandidateReadiness::Failed; }
            if (UsesOrbitalFields() && (!CandidateTemplate->Parent || CandidateTemplate->Parent->GetPathName() !=
                TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/Diagnostics/OrbitalFields20260929V3/M_APS_LodPixelTerrain.M_APS_LodPixelTerrain")))
            { Error = TEXT("Orbital field candidate has the wrong saved master"); return ECandidateReadiness::Failed; }
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
            if (bBuffers) return APSPlanetBufferViews::FScope::Label(Phase);
            if (bNormalSources) return APSPlanetNormalSources::Label(Phase);
            if (UsesCombined()) return Phase==0?TEXT("Combined0Native"):Phase==1?TEXT("Combined1Control"):Phase==2?TEXT("Combined2Candidate"):TEXT("Combined3Restored");
            if (bNormalHex) return Phase==0?TEXT("Hex0Native"):Phase==1?TEXT("Hex1Control"):Phase==2?TEXT("Hex2Candidate"):TEXT("Hex3Restored");
            if (bNormalWarp) return Phase==0?TEXT("Warp0Native"):Phase==1?TEXT("Warp1Control"):Phase==2?TEXT("Warp2Candidate"):TEXT("Warp3Restored");
            if (UsesOrbitalFields()) return Phase == 0 ? TEXT("Fields0Native") : TEXT("Fields1Aperiodic");
            if (bMacroAB) return Phase == 0 ? TEXT("Macro0Native") : Phase == 1 ? TEXT("Macro1Control")
                : Phase == 2 ? TEXT("Macro2Mean") : TEXT("Macro3Aperiodic");
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
            if (!bBuffers && !bNormalSources && (!bCandidateCompileRequested || !Template || !Resource || !Resource->IsGameThreadShaderMapComplete()
                || Resource->GetCompileErrors().Num() || !Map || !Map->GetMeshShaderMap(&FLocalVertexFactory::StaticType)))
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
                        || (Current->Parent->GetPathName() != TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/Shared/MI_APS_SharedTerra.MI_APS_SharedTerra")
                            && !((bNormalSources || bBuffers) && Current->Parent->GetPathName()==APSTerrainContinuityMaterial::TemplatePath)))
                    { Error = TEXT("LOD A/B requires generated Terra MID on each actual slot"); return false; }
                    FSlot& S = Slots.AddDefaulted_GetRef();
                    S.Mesh = Lod->Mesh; S.Index = I; S.Transform = Lod->Mesh->GetComponentTransform();
                    S.Material.Reset(Current);
                    if (bNormalSources && !S.NormalSources.Begin(Current, Error)) return false;
                    if (!bBuffers && !bNormalSources)
                    {
                        S.PixelMaterial.Reset(UMaterialInstanceDynamic::Create(Template, R));
                        S.PixelMaterial->CopyMaterialUniformParameters(Current);
                    }
                    if (UsesOrbitalFields()) S.PixelMaterial->SetScalarParameterValue(TEXT("APS_OrbitalMacroMode"), 1.0f);
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
                    if(APSPlanetNormalSources::Curvature())
                    {
                        double MaxDegrees=0,MeanDegrees=0;
                        if(!APSMeshCurvatureProbe::Build(*Section,S.Transform.InverseTransformPosition(R->GetActorLocation()),
                            R->bGenerateTangents,S.CurvatureNormals,MaxDegrees,MeanDegrees))
                        {Error=TEXT("Curvature probe could not reconstruct outward reference mesh");return false;}
                        UE_LOG(LogTemp,Display,TEXT("[APS.MeshCurvature] lod=%s section=%d vertices=%d unitFaces=%d correctionMeanDeg=%.8f maxDeg=%.8f"),
                            *Lod->GetName(),I,S.CurvatureNormals.Num(),R->bGenerateTangents?1:0,MeanDegrees,MaxDegrees);
                    }
                }
            }
            if (Slots.IsEmpty()) { Error = TEXT("LOD A/B has no actual terrain sections"); return false; }
            Root = R; Surface = InSurface; Body = InBody; RootTransform = R->GetActorTransform();
            bFrozen = R->bFreezeGeneration; bRootTick = R->IsActorTickEnabled(); bSurfaceTick = InSurface->IsActorTickEnabled();
            bBodyTick = InBody->IsActorTickEnabled(); InBody->SetActorTickEnabled(false);
            R->bFreezeGeneration = true; R->SetActorTickEnabled(false); InSurface->SetActorTickEnabled(false);
            Phase = 0; bLeased = true;
            BufferError.Reset();
            if (bBuffers && !BufferViews.Begin(Error)) { Restore(); return false; }
            UE_LOG(LogTemp, Display, TEXT("[APS.LodAB] Lease begun sections=%d camera/lighting unchanged; temporary frozen payload for independent A/B, not production performance"), Slots.Num());
            return true;
        }

        bool Validate(FString& Error) const
        {
            if (!BufferError.IsEmpty()) { Error = BufferError; return false; }
            if (bBuffers && !BufferViews.Validate(Phase, Error)) return false;
            AWorldScapeRoot* R = Root.Get();
            if (!bLeased || !R || !Surface.IsValid() || Surface->WorldScapeRootInstance != R
                || R->WorldScapeLodInGeneration.Num() || !R->GetActorTransform().Equals(RootTransform))
            { Error = TEXT("LOD A/B root changed during lease"); return false; }
            for (const FSlot& S : Slots)
            {
                auto* Mesh = S.Mesh.Get();
                const auto* Section = Mesh ? Mesh->GetProcMeshSection(S.Index) : nullptr;
                if (bNormalSources)
                {
                    if (!S.NormalSources.Validate(Error)) return false;
                    const bool bRadial = APSPlanetNormalSources::ModifiedNormals(Phase);
                    const TArray<FVector>& ChangedNormals=APSPlanetNormalSources::Curvature()?S.CurvatureNormals:S.RadialNormals;
                    UMaterialInterface* ExpectedMaterial = APSPlanetNormalSources::ReplacedTextures(Phase)
                        ? S.NormalSources.Material() : S.Material.Get();
                    if (!Section || !Mesh->GetComponentTransform().Equals(S.Transform)
                        || PayloadHash(*Section, false) != S.Payload || Mesh->GetMaterial(S.Index) != ExpectedMaterial
                        || (!bRadial && PayloadHash(*Section, true) != S.FullPayload))
                    { Error = TEXT("Normal isolation changed original mesh payload/material/transform"); return false; }
                    if (bRadial)
                        for (int32 I=0; I<ChangedNormals.Num(); ++I)
                            if (Section->PlanetVertexBuffer[I].Normal != ChangedNormals[I])
                            { Error=TEXT("Normal isolation radial payload not published"); return false; }
                    continue;
                }
                if (!Section || !Mesh->GetComponentTransform().Equals(S.Transform) || PayloadHash(*Section, false) != S.Payload
                    || ((bBuffers || bMacroAB || UsesOrbitalFields() || Phase != 1) && PayloadHash(*Section, true) != S.FullPayload)
                    || Mesh->GetMaterial(S.Index) != ((bBuffers || (bNormalWarp && Phase==3)) ? S.Material.Get() : (bMacroAB || UsesOrbitalFields()) ? (Phase == 0 ? S.Material.Get() : static_cast<UMaterialInterface*>(S.PixelMaterial.Get()))
                        : Phase == 3 ? static_cast<UMaterialInterface*>(S.FarNormalMaterial.Get())
                        : Phase == 2 ? static_cast<UMaterialInterface*>(S.PixelMaterial.Get()) : S.Material.Get()))
                { Error = TEXT("LOD A/B actual mesh payload/material/transform drifted"); return false; }
                if (bNormalWarp && (Phase==1 || Phase==2))
                {
                    float Mode=-1;
                    if (!S.PixelMaterial->GetScalarParameterValue(FHashedMaterialParameterInfo(TEXT("APS_NormalMacroWarpMode")),Mode) || Mode!=(Phase==1?0.0f:1.0f))
                    { Error=TEXT("Normal warp candidate mode mismatch"); return false; }
                    if(UsesCombined())
                    {
                        float NativeMacro=-1,CandidateMacro=-1;
                        if(!S.Material->GetScalarParameterValue(FHashedMaterialParameterInfo(TEXT("APS_OrbitalMacroMode")),NativeMacro)
                            || !S.PixelMaterial->GetScalarParameterValue(FHashedMaterialParameterInfo(TEXT("APS_OrbitalMacroMode")),CandidateMacro)
                            || CandidateMacro!=(Phase==1?NativeMacro:1.0f))
                        {Error=TEXT("Combined continuity macro mode mismatch");return false;}
                    }
                }
                if (UsesOrbitalFields() || bNormalWarp)
                {
                    float Mode = -1.0f;
                    if (UsesOrbitalFields() && (!S.PixelMaterial->GetScalarParameterValue(FHashedMaterialParameterInfo(TEXT("APS_OrbitalMacroMode")), Mode) || Mode != 1.0f))
                    { Error = TEXT("Gameplay orbital fields lost macro mode"); return false; }
                    auto* Native = Cast<UMaterialInstanceDynamic>(S.Material.Get());
                    for (const TCHAR* Name : { TEXT("APS_SharedPlanetCenter"), TEXT("APS_SharedInverseScale"),
                        TEXT("APS_SharedAxisX"), TEXT("APS_SharedAxisY"), TEXT("APS_SharedAxisZ") })
                    {
                        FVector4 A, B;
                        if (!Native || !Native->GetDoubleVectorParameterValue(FHashedMaterialParameterInfo(Name), A)
                            || !S.PixelMaterial->GetDoubleVectorParameterValue(FHashedMaterialParameterInfo(Name), B) || A != B)
                        { Error = FString(TEXT("Gameplay orbital fields physical frame mismatch: ")) + Name; return false; }
                    }
                    for (const TCHAR* Name : {TEXT("APS_SharedDetailRowXHigh"),TEXT("APS_SharedDetailRowXLow"),
                        TEXT("APS_SharedDetailRowYHigh"),TEXT("APS_SharedDetailRowYLow"),TEXT("APS_SharedDetailRowZHigh"),TEXT("APS_SharedDetailRowZLow")})
                    {
                        FLinearColor A,B;
                        if (!Native->GetVectorParameterValue(FHashedMaterialParameterInfo(Name),A)
                            || !S.PixelMaterial->GetVectorParameterValue(FHashedMaterialParameterInfo(Name),B) || A!=B)
                        { Error=FString(TEXT("Gameplay candidate compensated detail frame mismatch: "))+Name; return false; }
                    }
                }
                if (!bBuffers && !bMacroAB && !UsesOrbitalFields() && Phase == 1)
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
            if (bNormalSources)
            {
                if (++Phase >= APSPlanetNormalSources::Count()) { Restore(); return false; }
                for (FSlot& S : Slots)
                {
                    if (!S.NormalSources.Apply(Phase, BufferError)) return true; // Validate fails before capture.
                    const bool bRadial=APSPlanetNormalSources::ModifiedNormals(Phase);
                    const TArray<FVector>& ChangedNormals=APSPlanetNormalSources::Curvature()?S.CurvatureNormals:S.RadialNormals;
                    if (bRadial != APSPlanetNormalSources::ModifiedNormals(Phase-1)) WriteNormals(S,bRadial?ChangedNormals:S.Normals);
                    if (auto* Mesh=S.Mesh.Get()) Mesh->SetMaterial(S.Index,APSPlanetNormalSources::ReplacedTextures(Phase)
                        ? S.NormalSources.Material() : S.Material.Get());
                }
                UE_LOG(LogTemp,Display,TEXT("[APS.NormalSources] phase=%s original positions/indices/UV/colors/tangents and non-texture uniforms retained"),Label());
                return true;
            }
            if (bBuffers)
            {
                if (++Phase >= APSPlanetBufferViews::FScope::Count) { Restore(); return false; }
                BufferViews.Apply(Phase, BufferError); // Validate reports any failure before the next capture.
                return true;
            }
            if (UsesOrbitalFields())
            {
                if (++Phase > 1) { Restore(); return false; }
                for (FSlot& S : Slots)
                    if (auto* Mesh = S.Mesh.Get()) Mesh->SetMaterial(S.Index, S.PixelMaterial.Get());
                UE_LOG(LogTemp, Display, TEXT("[APS.OrbitalFieldsAB] phase=%s macroMode=1; full native mesh payload/physical frame retained"), Label());
                return true;
            }
            if (bMacroAB)
            {
                if (++Phase > 3) { Restore(); return false; }
                if (bNormalWarp)
                {
                    for (FSlot& S : Slots)
                    {
                        S.PixelMaterial->SetScalarParameterValue(TEXT("APS_NormalMacroWarpMode"),Phase==1?0.0f:1.0f);
                        if(UsesCombined())
                        {
                            float NativeMacro=-1;
                            if(!S.Material->GetScalarParameterValue(FHashedMaterialParameterInfo(TEXT("APS_OrbitalMacroMode")),NativeMacro))
                            {BufferError=TEXT("Combined native macro mode unavailable");return true;} // Validate fails before capture.
                            S.PixelMaterial->SetScalarParameterValue(TEXT("APS_OrbitalMacroMode"),Phase==1?NativeMacro:1.0f);
                        }
                        if (auto* Mesh=S.Mesh.Get()) Mesh->SetMaterial(S.Index,Phase==3?S.Material.Get():S.PixelMaterial.Get());
                    }
                    UE_LOG(LogTemp,Display,TEXT("[APS.NormalMacroWarp] phase=%s original full mesh and physical frame retained"),Label());
                    return true;
                }
                for (FSlot& S : Slots)
                {
                    S.PixelMaterial->SetScalarParameterValue(TEXT("APS_OrbitalMacroMode"), Phase == 1 ? 0.0f : Phase == 2 ? 2.0f : 1.0f);
                    if (auto* Mesh = S.Mesh.Get()) Mesh->SetMaterial(S.Index, S.PixelMaterial.Get());
                }
                UE_LOG(LogTemp, Display, TEXT("[APS.MacroAB] phase=%s; full mesh payload and physical frame unchanged"), Label());
                return true;
            }
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
            BufferViews.Restore();
            if (!bLeased) { Slots.Reset(); return; }
            for (FSlot& S : Slots)
            {
                if (auto* Mesh = S.Mesh.Get())
                {
                    // Never replace a concurrently regenerated payload with an old
                    // snapshot. Such a run fails Validate and is not A/B evidence.
                    const auto* Before = Mesh->GetProcMeshSection(S.Index);
                    if (Before && PayloadHash(*Before, false) == S.Payload)
                    { if (!bBuffers && !bMacroAB && !UsesOrbitalFields()) WriteNormals(S, S.Normals); }
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

    // Moving-observer companion: change only the material source used by future
    // LODs as well as current sections. Never freeze/tick or write mesh payload.
    class FLiveMaterialLease
    {
        TWeakObjectPtr<AWorldScapeRoot> Root;
        TStrongObjectPtr<UMaterialInstanceDynamic> Native{nullptr}, Candidate{nullptr};
        FWSMaterialLodArray Original;
        FTransform LastFrame;
        void ApplyCandidateMode()
        {
            if(FParse::Param(FCommandLine::Get(),TEXT("APSProbeNormalHex")))
                Candidate->SetScalarParameterValue(TEXT("APS_NormalMacroWarpMode"),1.0f);
            else Candidate->SetScalarParameterValue(TEXT("APS_OrbitalMacroMode"),1.0f);
            if(UsesCombined())Candidate->SetScalarParameterValue(TEXT("APS_OrbitalMacroMode"),1.0f);
        }
    public:
        ~FLiveMaterialLease() { Restore(); }
        bool Begin(AWorldScapeRoot* R, UMaterialInstance* Template, FString& Error)
        {
            auto* N = R ? Cast<UMaterialInstanceDynamic>(R->TerrainMaterial.DefaultMaterial) : nullptr;
            if (Root.IsValid() || !R || !Template || !N || !APSSharedTerrainMaterial::IsSharedStack(N)
                || !N->Parent || N->Parent->GetPathName()!=TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/Shared/MI_APS_SharedTerra.MI_APS_SharedTerra")
                || R->Terrain_MakeMaterialInstance || !R->TerrainMaterial.MaterialsLod.IsEmpty()
                || R->bFreezeGeneration || !R->IsActorTickEnabled())
            { Error=TEXT("Live color lease requires running native single-MID WorldScape"); return false; }
            Candidate.Reset(UMaterialInstanceDynamic::Create(Template,R));
            if (!Candidate.IsValid()) { Error=TEXT("Live candidate allocation failed"); return false; }
            Native.Reset(N); Original=R->TerrainMaterial; LastFrame=R->GetActorTransform();
            Candidate->CopyMaterialUniformParameters(N);
            ApplyCandidateMode();
            Root=R;
            FWSMaterialLodArray Replacement=Original;
            Replacement.DefaultMaterial=Candidate.Get();
            R->UpdateTerrainMaterial(Replacement);
            return Validate(R,Error);
        }
        bool Validate(AWorldScapeRoot* R,FString& Error)
        {
            if (!R || R!=Root.Get() || R->TerrainMaterial.DefaultMaterial!=Candidate.Get()
                || R->bFreezeGeneration || !R->IsActorTickEnabled() || !Native.IsValid())
            { Error=TEXT("Live material source or generation changed during flight"); return false; }
            if (!R->GetActorTransform().Equals(LastFrame,1.e-8))
            {
                // The original MID keeps its production transform callback.
                Candidate->CopyMaterialUniformParameters(Native.Get());
                ApplyCandidateMode();
                LastFrame=R->GetActorTransform();
            }
            for (const auto* Lod:R->WorldScapeLod)
                if (IsValid(Lod) && IsValid(Lod->Mesh))
                    for (int32 S=0;S<Lod->Mesh->GetNumSections();++S)
                        if (Lod->Mesh->GetMaterial(S)!=Candidate.Get())
                        { Error=TEXT("A live/new LOD lost the diagnostic color material"); return false; }
            for (const TCHAR* Name:{TEXT("APS_SharedPlanetCenter"),TEXT("APS_SharedInverseScale"),
                TEXT("APS_SharedAxisX"),TEXT("APS_SharedAxisY"),TEXT("APS_SharedAxisZ")})
            {
                FVector4 A,B;
                if (!Native->GetDoubleVectorParameterValue(FHashedMaterialParameterInfo(Name),A)
                    || !Candidate->GetDoubleVectorParameterValue(FHashedMaterialParameterInfo(Name),B) || A!=B)
                { Error=TEXT("Live material physical frame diverged from native"); return false; }
            }
            for (const TCHAR* Name:{TEXT("APS_SharedDetailRowXHigh"),TEXT("APS_SharedDetailRowXLow"),
                TEXT("APS_SharedDetailRowYHigh"),TEXT("APS_SharedDetailRowYLow"),TEXT("APS_SharedDetailRowZHigh"),TEXT("APS_SharedDetailRowZLow")})
            {
                FLinearColor A,B;
                if(!Native->GetVectorParameterValue(FHashedMaterialParameterInfo(Name),A)
                    || !Candidate->GetVectorParameterValue(FHashedMaterialParameterInfo(Name),B) || A!=B)
                {Error=TEXT("Live material high/low rows diverged from native");return false;}
            }
            if(FParse::Param(FCommandLine::Get(),TEXT("APSProbeNormalHex")))
            {
                float Mode=-1;
                if(!Candidate->GetScalarParameterValue(FHashedMaterialParameterInfo(TEXT("APS_NormalMacroWarpMode")),Mode) || Mode!=1)
                {Error=TEXT("Live normal-hex mode lost");return false;}
            }
            if(UsesCombined())
            {
                float Mode=-1;
                if(!Candidate->GetScalarParameterValue(FHashedMaterialParameterInfo(TEXT("APS_OrbitalMacroMode")),Mode) || Mode!=1)
                {Error=TEXT("Live combined macro mode lost");return false;}
            }
            return true;
        }
        void Restore()
        {
            if (auto* R=Root.Get(); R && R->TerrainMaterial.DefaultMaterial==Candidate.Get())
                R->UpdateTerrainMaterial(Original);
            Root.Reset(); Candidate.Reset(); Native.Reset();
        }
    };
}
#endif
