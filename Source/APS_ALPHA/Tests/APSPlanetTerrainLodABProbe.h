#pragma once

#if WITH_DEV_AUTOMATION_TESTS
#include "APS_ALPHA/Core/Planetary/APSSharedTerrainMaterial.h"
#include "APS_ALPHA/Core/Planetary/APSShoreWaterMaterial.h"
#include "APS_ALPHA/Actors/Astro/PlanetaryBody.h"
#include "APS_ALPHA/Actors/Astro/Planet.h"
#include "APS_ALPHA/Core/Rendering/APSStellarVisualSubsystem.h"
#include "APS_ALPHA/Core/Rendering/APSPreviewCameraBounds.h"
#include "APS_ALPHA/Generation/AstroGenerator.h"
#include "APS_ALPHA/UI/MainMenu/WorldGenerationViewModel.h"
#include "Camera/CameraComponent.h"
#include "Components/LightComponent.h"
#include "EngineUtils.h"
#include "EngineGlobals.h"
#include "LocalVertexFactory.h"
#include "MaterialShared.h"
#include "Misc/Crc.h"
#include "ProceduralMeshComponent.h"
#include "UObject/StrongObjectPtr.h"
#include "APSTundraLayerTransferProbe.h"
#include "APSPlanetBufferViewsProbe.h"
#include "APSPlanetPatternProbe.h"
#if WITH_EDITOR
#include "ShaderCompiler.h"
#endif

// Test-only: the real retained PLANET mesh, not a new sphere or a live clipmap.
// Each view compares exact native parameters against previously baked graph-only
// candidates. Two real production orbits occur only after restoring the material.
namespace APSPlanetTerrainLodAB
{
    inline bool UsesSlopeOnly() { return FParse::Param(FCommandLine::Get(), TEXT("APSProbeTerrainSlopeOnly")); }
    inline bool UsesWarpOnly() { return FParse::Param(FCommandLine::Get(), TEXT("APSProbeTerrainWarpOnly")); }
    inline bool UsesSlopeSide() { return FParse::Param(FCommandLine::Get(), TEXT("APSProbeTerrainSlopeSide")); }
    inline bool UsesCombined() { return FParse::Param(FCommandLine::Get(), TEXT("APSProbeTerrainCombined")); }
    inline bool UsesOrbitalFields() { return UsesCombined() || FParse::Param(FCommandLine::Get(), TEXT("APSProbeTerrainOrbitalFields")); }
    inline bool UsesFamilyScaleAB() { return FParse::Param(FCommandLine::Get(), TEXT("APSProbeTerrainFamilyScaleAB")); }
    inline bool UsesNativeViews() { return FParse::Param(FCommandLine::Get(), TEXT("APSProbeTerrainNativeViews")); }
    inline bool UsesPublishedViews() { return FParse::Param(FCommandLine::Get(), TEXT("APSProbeTerrainPublishedViews")); }
    inline bool UsesWheelSweep() { return FParse::Param(FCommandLine::Get(), TEXT("APSProbeTerrainWheelSweep")); }
    inline bool UsesZoomSweep() { return UsesWheelSweep() || FParse::Param(FCommandLine::Get(), TEXT("APSProbeTerrainZoomSweep")); }
    enum class EResult : uint8 { Pending, Finished, Failed };
    // Fixed-view A/B must use the exact published native uniforms. Rebuilding
    // the inverse from a recovered presentation scale adds a reciprocal round
    // trip and may change a double by one ULP despite identical geometry.
    inline bool SamePhysicalUniforms(UMaterialInstanceDynamic* A, UMaterialInstanceDynamic* B, FString& Error)
    {
        if (!A || !B) { Error = TEXT("PLANET terrain A/B missing frame material"); return false; }
        for (const TCHAR* Name : {TEXT("APS_SharedPlanetCenter"), TEXT("APS_SharedInverseScale"),
            TEXT("APS_SharedAxisX"), TEXT("APS_SharedAxisY"), TEXT("APS_SharedAxisZ")})
        {
            FVector4 AV(0, 0, 0, 0), BV(0, 0, 0, 0);
            if (!A->GetDoubleVectorParameterValue(FHashedMaterialParameterInfo(Name), AV)
                || !B->GetDoubleVectorParameterValue(FHashedMaterialParameterInfo(Name), BV) || AV != BV)
            { Error = FString::Printf(TEXT("PLANET terrain A/B physical frame mismatch: %s candidate=(%.17g,%.17g,%.17g,%.17g) native=(%.17g,%.17g,%.17g,%.17g)"),
                Name, AV.X, AV.Y, AV.Z, AV.W, BV.X, BV.Y, BV.Z, BV.W); return false; }
        }
        for (const TCHAR* Name : {TEXT("APS_SharedDetailRowXHigh"), TEXT("APS_SharedDetailRowXLow"),
            TEXT("APS_SharedDetailRowYHigh"), TEXT("APS_SharedDetailRowYLow"),
            TEXT("APS_SharedDetailRowZHigh"), TEXT("APS_SharedDetailRowZLow")})
        {
            FLinearColor AV, BV;
            if (!A->GetVectorParameterValue(FHashedMaterialParameterInfo(Name), AV)
                || !B->GetVectorParameterValue(FHashedMaterialParameterInfo(Name), BV) || AV != BV)
            { Error = FString(TEXT("PLANET terrain A/B detail frame mismatch: ")) + Name; return false; }
        }
        return true;
    }
    class FProbe
    {
        static constexpr const TCHAR* PixelTemplate = TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/Diagnostics/LodPixelAB/MI_APS_LodPixelTerra.MI_APS_LodPixelTerra");
        static constexpr const TCHAR* PixelMaster = TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/Diagnostics/LodPixelAB/M_APS_LodPixelTerrain.M_APS_LodPixelTerrain");
        static constexpr const TCHAR* FarTemplate = TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/Diagnostics/LodFarNormalAB/MI_APS_LodFarNormalTerra.MI_APS_LodFarNormalTerra");
        static constexpr const TCHAR* FarMaster = TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/Diagnostics/LodFarNormalAB/M_APS_LodFarNormalTerrain.M_APS_LodFarNormalTerrain");
        TStrongObjectPtr<UMaterialInstance> Templates[2] = { TStrongObjectPtr<UMaterialInstance>(nullptr), TStrongObjectPtr<UMaterialInstance>(nullptr) };
        TStrongObjectPtr<UMaterialInstanceDynamic> Candidates[2] = { TStrongObjectPtr<UMaterialInstanceDynamic>(nullptr), TStrongObjectPtr<UMaterialInstanceDynamic>(nullptr) };
        TStrongObjectPtr<UMaterialInstanceDynamic> Original{nullptr};
        APSTundraLayerTransfer::FPair LayerTransfer;
        APSPlanetBufferViews::FScope BufferViews;
        APSPlanetPatternProbe::FScope PatternProbe;
        TWeakObjectPtr<AAstroGenerator> Generator;
        TWeakObjectPtr<UProceduralMeshComponent> Terrain;
        bool Requested[2] = { false, false }, bActive = false, bFar = false, bViewSettled = false;
        int32 View = 0, Variant = 0;
        double Next = 0.0, PresentationScale = 0.0;
        uint32 VertexCRC = 0, IndexCRC = 0;
        FTransform MeshFrame, CameraFrame;
        FVector Observer;
        float FieldOfView = 0.0f;
        float SavedMacroMode = 0.0f;
        bool bMacroOverridden = false;
        TArray<double> GpuMs;
        FAPSContinuousPreviewOrbit InitialOrbit;
        FAPSContinuousPreviewOrbit PendingOrbit;
        TWeakObjectPtr<APlanetaryBody> PendingBody;
        bool bPresentationPending = false, bRequireOrbitMovement = false;
        double PresentationStarted = 0.0, PresentationSettleDelay = 0.0;
        uint64 PresentationRequestFrame = 0, PresentationMatchedFrame = MAX_uint64;
        FVector BeforePresentationObserver = FVector::ZeroVector;
        FTransform BeforePresentationBody = FTransform::Identity;
        static constexpr double SweepHeightsKm[] = {20000, 10000, 5000, 2000, 1000, 500, 300, 200, 120, 90, 70};
        static constexpr int32 SweepInCount = UE_ARRAY_COUNT(SweepHeightsKm);
        static constexpr int32 SweepViewCount = SweepInCount * 2 - 1;

        bool SetSweepView(UWorldGenerationViewModel* VM, FString& Error)
        {
            const auto* Body = Generator->GetActivePreviewWorldScapeBody();
            if (!Body || Body->PlanetRadiusKM <= 0)
            { Error = TEXT("Zoom sweep needs the actual physical body radius"); return false; }
            const int32 Index = View < SweepInCount ? View : SweepViewCount - 1 - View;
            const double RadiusCm = double(Body->PlanetRadiusKM) * 100000.0;
            const double TargetHeightKm = UsesWheelSweep() ? 1000.0 * FMath::Pow(0.82, Index) : SweepHeightsKm[Index];
            const double TargetCm = RadiusCm + TargetHeightKm * 100000.0;
            const double BeforeCm = Generator->GetContinuousPreviewOrbit().DistanceCm;
            const float WheelDelta = UsesWheelSweep() && View > 0 ? (View < SweepInCount ? 1.0f : -1.0f)
                : float(FAPSPreviewCameraBounds::WheelDeltaBetween(BeforeCm, TargetCm, RadiusCm));
            VM->ZoomPreview(WheelDelta);
            const double ActualCm = Generator->GetContinuousPreviewOrbit().DistanceCm;
            UE_LOG(LogTemp, Display, TEXT("PLANET_TERRAIN_ZOOM_SWEEP view=%d direction=%s requestedKm=%.3f orbitTargetKm=%.6f distanceCm=%.12g; production wheel path, presentation still pending"),
                View, View < SweepInCount ? TEXT("in") : TEXT("out"), TargetHeightKm,
                (ActualCm - RadiusCm) / 100000.0, ActualCm);
            if (UsesWheelSweep()) UE_LOG(LogTemp, Display, TEXT("PLANET_TERRAIN_WHEEL_STEP view=%d wheelDelta=%.6f clearanceRatio=%.9f"),
                View, WheelDelta, (ActualCm - RadiusCm) / (BeforeCm - RadiusCm));
            if (!FMath::IsNearlyEqual(ActualCm / TargetCm, 1.0, 1.e-6))
            { Error = TEXT("Zoom sweep target was clamped or not reached; do not mislabel its altitude"); return false; }
            return true;
        }
        struct FLightState
        {
            TWeakObjectPtr<ULightComponent> Component;
            FTransform Frame;
            float Intensity = 0;
            FColor Color;
            bool CastShadows = false;
        };
        TArray<FLightState> Lights;

        // Match the actual native branch, including static permutations and the
        // volume textures whose statistics were baked into its diagnostic copy.
        bool ValidateFieldTemplate(UMaterialInstanceDynamic* Native, FString& Error) const
        {
            auto* Template = Templates[0].Get();
            TArray<FMaterialParameterInfo> Infos;
            TArray<FGuid> Ids;
            Native->GetAllStaticSwitchParameterInfo(Infos, Ids);
            for (const auto& Info : Infos)
            {
                bool A = false, B = false; FGuid AG, BG;
                if (!Native->GetStaticSwitchParameterValue(Info, A, AG)
                    || !Template->GetStaticSwitchParameterValue(Info, B, BG) || A != B)
                { Error = TEXT("Orbital candidate static switch differs: ") + Info.Name.ToString(); return false; }
            }
            Infos.Reset(); Ids.Reset();
            Native->GetAllStaticComponentMaskParameterInfo(Infos, Ids);
            for (const auto& Info : Infos)
            {
                bool AR, AG, AB, AA, BR, BG, BB, BA; FGuid AId, BId;
                if (!Native->GetStaticComponentMaskParameterValue(Info, AR, AG, AB, AA, AId)
                    || !Template->GetStaticComponentMaskParameterValue(Info, BR, BG, BB, BA, BId)
                    || AR != BR || AG != BG || AB != BB || AA != BA)
                { Error = TEXT("Orbital candidate static mask differs: ") + Info.Name.ToString(); return false; }
            }
            for (const TCHAR* Name : { TEXT("Noise2"), TEXT("Noise3"), TEXT("Noise4") })
            {
                UTexture* A = nullptr; UTexture* B = nullptr;
                if (!Native->GetTextureParameterValue(FHashedMaterialParameterInfo(Name), A)
                    || !Template->GetTextureParameterValue(FHashedMaterialParameterInfo(Name), B) || !A || A != B)
                { Error = FString(TEXT("Orbital field statistics source differs: ")) + Name; return false; }
            }
            UE_LOG(LogTemp, Display, TEXT("PLANET_TERRAIN_FIELDS_TEMPLATE native=%s staticValuesAndVolumeSourcesMatch=1"),
                *Native->Parent->GetPathName());
            return true;
        }

        EResult Warm(UWorld* World, bool bNativeMagma, FString& Error)
        {
            if (UsesCombined() && bNativeMagma)
            { Error = TEXT("Combined normal/field candidate is Terra-only; no audited Magma permutation"); return EResult::Failed; }
            const TCHAR* CurrentPixelTemplate = UsesCombined()
                ? TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/Diagnostics/ContinuityCombined20260929V1/MI_APS_NormalWarpTerra.MI_APS_NormalWarpTerra") : UsesOrbitalFields()
                ? (bNativeMagma
                    ? TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/Diagnostics/OrbitalFields20260929MagmaV3/MI_APS_LodPixelMagma.MI_APS_LodPixelMagma")
                    : TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/Diagnostics/OrbitalFields20260929V3/MI_APS_LodPixelTerra.MI_APS_LodPixelTerra")) : UsesSlopeSide()
                ? TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/Diagnostics/LodSlopeSide20260929/MI_APS_LodPixelTerra.MI_APS_LodPixelTerra") : UsesSlopeOnly()
                ? TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/Diagnostics/LodSlopePreserve20260927/MI_APS_LodPixelTerra.MI_APS_LodPixelTerra") : UsesWarpOnly()
                ? TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/Diagnostics/LodWarpPixel20260929/MI_APS_LodPixelTerra.MI_APS_LodPixelTerra") : PixelTemplate;
            const TCHAR* CurrentPixelMaster = UsesCombined()
                ? TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/Diagnostics/ContinuityCombined20260929V1/M_APS_NormalWarpTerrain.M_APS_NormalWarpTerrain") : UsesOrbitalFields()
                ? (bNativeMagma
                    ? TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/Diagnostics/OrbitalFields20260929MagmaV3/M_APS_LodPixelTerrain.M_APS_LodPixelTerrain")
                    : TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/Diagnostics/OrbitalFields20260929V3/M_APS_LodPixelTerrain.M_APS_LodPixelTerrain")) : UsesSlopeSide()
                ? TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/Diagnostics/LodSlopeSide20260929/M_APS_LodPixelTerrain.M_APS_LodPixelTerrain") : UsesSlopeOnly()
                ? TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/Diagnostics/LodSlopePreserve20260927/M_APS_LodPixelTerrain.M_APS_LodPixelTerrain") : UsesWarpOnly()
                ? TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/Diagnostics/LodWarpPixel20260929/M_APS_LodPixelTerrain.M_APS_LodPixelTerrain") : PixelMaster;
            bool Ready = true;
            for (int32 I = 0; I < (bFar ? 2 : 1); ++I)
            {
                if (!Templates[I].IsValid()) Templates[I].Reset(LoadObject<UMaterialInstance>(nullptr, I ? FarTemplate : CurrentPixelTemplate));
                auto* M = Templates[I].Get();
                if (!M || M->GetPathName() != (I ? FarTemplate : CurrentPixelTemplate)
                    || !M->GetMaterial() || M->GetMaterial()->GetPathName() != (I ? FarMaster : CurrentPixelMaster)
                    || M->GetBlendMode() != BLEND_Masked)
                { Error = TEXT("Exact saved branch-specific diagnostic candidate is absent or has the wrong master"); return EResult::Failed; }
                auto* R = M->GetMaterialResource(World->GetFeatureLevel());
                if (R && R->GetCompileErrors().Num())
                { Error = TEXT("PLANET terrain candidate has compile errors"); return EResult::Failed; }
                if (R && !Requested[I])
                {
#if WITH_EDITOR
                    if (!R->IsGameThreadShaderMapComplete()) R->SubmitCompileJobs_GameThread(EShaderCompileJobPriority::ForceLocal);
#endif
                    FPSOPrecacheParams Params;
                    Params.bStaticLighting = false; Params.bCastShadow = true;
                    Params.SetMobility(EComponentMobility::Movable);
                    static_cast<UMaterialInterface*>(M)->PrecachePSOs(&FLocalVertexFactory::StaticType, Params);
                    Requested[I] = true;
                    UE_LOG(LogTemp, Display, TEXT("PLANET_TERRAIN_AB_WARM material=%s blend=%d expected=Masked once=1"),
                        *M->GetPathName(), int32(M->GetBlendMode()));
                }
                const auto* Map = R ? R->GetGameThreadShaderMap() : nullptr;
                Ready &= R && R->IsGameThreadShaderMapComplete() && Map && Map->GetMeshShaderMap(&FLocalVertexFactory::StaticType);
            }
            return Ready ? EResult::Finished : EResult::Pending;
        }

        void SnapshotView()
        {
            MeshFrame = Terrain->GetComponentTransform();
            CameraFrame = Generator->GetPreviewCameraComponent()->GetComponentTransform();
            FieldOfView = Generator->GetPreviewCameraComponent()->FieldOfView;
            Observer = Generator->GetContinuousPreviewFrame().ObserverCm;
            Lights.Reset();
            for (TActorIterator<AActor> It(Generator->GetWorld()); It; ++It)
            {
                TInlineComponentArray<ULightComponent*> Components;
                It->GetComponents(Components);
                for (auto* Light : Components)
                    if (IsValid(Light)) Lights.Add({Light, Light->GetComponentTransform(),
                        Light->Intensity, Light->LightColor, bool(Light->CastShadows)});
            }
        }

        void AwaitPresentation(double Now, double SettleDelay, bool bRequireMovement = false)
        {
            // Public camera controls queue presentation on the normal actor tick.
            // Do not freeze a target orbit while its physical/render frame is stale.
            PendingOrbit = Generator->GetContinuousPreviewOrbit();
            PendingBody = Generator->GetActivePreviewWorldScapeBody();
            PresentationStarted = Now; PresentationSettleDelay = SettleDelay;
            PresentationRequestFrame = GFrameCounter; PresentationMatchedFrame = MAX_uint64;
            bPresentationPending = true; bRequireOrbitMovement = bRequireMovement;
            bViewSettled = false; GpuMs.Reset(); Next = Now;
            Generator->SetActorTickEnabled(true);
        }

        EResult PollPresentation(double Now, FString& Error)
        {
            auto* G = Generator.Get();
            auto* Mesh = Terrain.Get();
            const auto* Body = PendingBody.Get();
            const auto* Section = Mesh ? Mesh->GetProcMeshSection(0) : nullptr;
            const auto* Camera = G ? G->GetPreviewCameraComponent() : nullptr;
            if (!G || !Body || !Section || !Camera || !Original.IsValid() || !PendingOrbit.IsValid()
                || G->GetActivePreviewWorldScapeBody() != Body || G->GetActivePreviewTerrainProxy() != Mesh
                || Mesh->GetMaterial(0) != Original.Get()
                || FCrc::MemCrc32(Section->ProcVertexBuffer.GetData(), Section->ProcVertexBuffer.Num() * sizeof(FProcMeshVertex)) != VertexCRC
                || FCrc::MemCrc32(Section->ProcIndexBuffer.GetData(), Section->ProcIndexBuffer.Num() * sizeof(uint32)) != IndexCRC)
            { Error = TEXT("PLANET A/B pending presentation lost its retained geometry, body or native binding"); return EResult::Failed; }

            const auto& Orbit = G->GetContinuousPreviewOrbit();
            if (!Orbit.CenterCm.Equals(PendingOrbit.CenterCm, 0.01)
                || !Orbit.Outward.Equals(PendingOrbit.Outward, 1.e-12)
                || !FMath::IsNearlyEqual(Orbit.DistanceCm / PendingOrbit.DistanceCm, 1.0, 1.e-12))
            { Error = TEXT("PLANET A/B requested orbit drifted while waiting for production presentation"); return EResult::Failed; }

            const auto& Frame = G->GetContinuousPreviewFrame();
            const FVector ExpectedObserver = PendingOrbit.ObserverCm();
            const double ExpectedScale = 1.e7 / PendingOrbit.DistanceCm;
            const FVector BodyCenter = G->GetContinuousPreviewPhysicalPosition(Body);
            const double RadiusCm = FMath::Max(Body->RadiusKM > 0.0 ? double(Body->RadiusKM) : double(Body->PlanetRadiusKM), 0.001) * 1.e5;
            FVector PresentedCenter = FVector::ZeroVector;
            double PresentedRadius = 0.0;
            const bool bBodyPresentation = G->GetPreviewPresentationLocation(Body, PresentedCenter)
                && G->GetPreviewPresentationRadius(Body, PresentedRadius) && PresentedRadius > 0.0;
            // Match the production radial bound, not the rotated world AABB's sphere.
            const double MeshRadius = Mesh->CalcBounds(FTransform::Identity).BoxExtent.GetMax()
                / FMath::Max(double(Mesh->BoundsScale), 1.e-12) * Mesh->GetComponentScale().GetAbsMax();
            const double ActualHeightKm = (FVector::Distance(Frame.ObserverCm, BodyCenter) - RadiusCm) / 1.e5;
            const double TargetHeightKm = (FVector::Distance(ExpectedObserver, BodyCenter) - RadiusCm) / 1.e5;
            const double RadiusRatio = PresentedRadius > 0.0 ? MeshRadius / PresentedRadius : 0.0;
            FVector4 Center(0, 0, 0, 0), Inverse(0, 0, 0, 0);
            const bool bNativeFrame = Original->GetDoubleVectorParameterValue(FHashedMaterialParameterInfo(TEXT("APS_SharedPlanetCenter")), Center)
                && Original->GetDoubleVectorParameterValue(FHashedMaterialParameterInfo(TEXT("APS_SharedInverseScale")), Inverse)
                && FVector(Center.X, Center.Y, Center.Z).Equals(Mesh->GetComponentLocation(), 0.001)
                && FMath::IsNearlyEqual(Inverse.X * PresentationScale * Mesh->GetComponentScale().GetAbsMax(), 1.0, 1.e-9);
            const bool bObserverMoved = !Frame.ObserverCm.Equals(BeforePresentationObserver, 1.0);
            const bool bBodyMoved = !Mesh->GetComponentTransform().Equals(BeforePresentationBody, 0.001);
            const bool bPresented = GFrameCounter > PresentationRequestFrame && !G->IsPreviewCameraTransitionActive()
                && Frame.ObserverCm.Equals(ExpectedObserver, 0.01)
                && FMath::IsNearlyEqual(Frame.RenderCmPerPhysicalCm / ExpectedScale, 1.0, 1.e-12)
                && bBodyPresentation && Mesh->GetComponentLocation().Equals(PresentedCenter, 0.001)
                && FMath::IsNearlyEqual(RadiusRatio, 1.0, 1.e-6)
                && Camera->GetComponentLocation().Equals(FVector::ZeroVector, 0.001)
                && Camera->GetForwardVector().Equals(-PendingOrbit.Outward, 1.e-6)
                && FMath::IsNearlyEqual(ActualHeightKm, TargetHeightKm, 1.e-6) && bNativeFrame
                && (!bRequireOrbitMovement || (bObserverMoved && bBodyMoved));
            if (!bPresented) PresentationMatchedFrame = MAX_uint64;
            else if (PresentationMatchedFrame == MAX_uint64) PresentationMatchedFrame = GFrameCounter;
            else if (GFrameCounter > PresentationMatchedFrame)
            {
                G->SetActorTickEnabled(false); bPresentationPending = false;
                SnapshotView(); Next = Now + PresentationSettleDelay;
                UE_LOG(LogTemp, Display, TEXT("PLANET_TERRAIN_PRESENTED view=%d targetHeightKm=%.9f actualHeightKm=%.9f observerErrorCm=%.9g radiusRatio=%.12g normalTick=1"),
                    View, TargetHeightKm, ActualHeightKm, FVector::Distance(Frame.ObserverCm, ExpectedObserver), RadiusRatio);
                if (bRequireOrbitMovement)
                    UE_LOG(LogTemp, Display, TEXT("PLANET_TERRAIN_AB_REAL_ORBIT ordinal=%d physicalObserverDeltaCm=%.9g bodyMoved=1 nativeRestored=1"),
                        View, FVector::Distance(BeforePresentationObserver, Frame.ObserverCm));
                return EResult::Pending;
            }
            if (Now - PresentationStarted >= 12.0)
            {
                Error = FString::Printf(TEXT("PLANET A/B production presentation timeout: view=%d targetHeightKm=%.9f actualHeightKm=%.9f observerErrorCm=%.9g centerErrorCm=%.9g radiusRatio=%.12g nativeFrame=%d requireMovement=%d observerMoved=%d bodyMoved=%d tick=%d transition=%d"),
                    View, TargetHeightKm, ActualHeightKm, FVector::Distance(Frame.ObserverCm, ExpectedObserver),
                    FVector::Distance(Mesh->GetComponentLocation(), PresentedCenter), RadiusRatio,
                    bNativeFrame, bRequireOrbitMovement, bObserverMoved, bBodyMoved, G->IsActorTickEnabled(), G->IsPreviewCameraTransitionActive());
                return EResult::Failed;
            }
            return EResult::Pending;
        }

        bool Validate(FString& Error) const
        {
            auto* Mesh = Terrain.Get();
            const auto* G = Generator.Get();
            const auto* S = Mesh ? Mesh->GetProcMeshSection(0) : nullptr;
            const auto* Camera = G ? G->GetPreviewCameraComponent() : nullptr;
            UMaterialInterface* Expected = (Variant == 0 || APSTundraLayerTransfer::Requested() || APSPlanetBufferViews::Requested() || APSPlanetPatternProbe::Requested())
                ? static_cast<UMaterialInterface*>(Original.Get()) : Candidates[Variant - 1].Get();
            if (!S || !Camera || G->GetActivePreviewTerrainProxy() != Mesh || Mesh->GetMaterial(0) != Expected
                || !Mesh->GetComponentTransform().Equals(MeshFrame, 1.e-6)
                || !Camera->GetComponentTransform().Equals(CameraFrame, 1.e-6) || Camera->FieldOfView != FieldOfView
                || !G->GetContinuousPreviewFrame().ObserverCm.Equals(Observer, 0.01)
                || FCrc::MemCrc32(S->ProcVertexBuffer.GetData(), S->ProcVertexBuffer.Num() * sizeof(FProcMeshVertex)) != VertexCRC
                || FCrc::MemCrc32(S->ProcIndexBuffer.GetData(), S->ProcIndexBuffer.Num() * sizeof(uint32)) != IndexCRC)
            { Error = TEXT("PLANET terrain A/B geometry, camera, physical observer or exact binding drifted"); return false; }
            for (const auto& Light : Lights)
                if (!Light.Component.IsValid() || !Light.Component->GetComponentTransform().Equals(Light.Frame, 1.e-6)
                    || Light.Component->Intensity != Light.Intensity || Light.Component->LightColor != Light.Color
                    || bool(Light.Component->CastShadows) != Light.CastShadows)
                { Error = FString::Printf(TEXT("PLANET terrain A/B light state drifted: light=%s oldIntensity=%.9g newIntensity=%.9g oldFrame=%s newFrame=%s; fixed-view evidence rejected"),
                    *GetPathNameSafe(Light.Component.Get()), Light.Intensity, Light.Component.IsValid() ? Light.Component->Intensity : -1.0f,
                    *Light.Frame.ToHumanReadableString(), Light.Component.IsValid() ? *Light.Component->GetComponentTransform().ToHumanReadableString() : TEXT("invalid")); return false; }
            auto* MID = Cast<UMaterialInstanceDynamic>(Expected);
            if (UsesOrbitalFields() && Variant == 1)
            {
                float Mode = -1.0f;
                if (!MID || !MID->GetScalarParameterValue(FHashedMaterialParameterInfo(TEXT("APS_OrbitalMacroMode")), Mode) || Mode != 1.0f)
                { Error = TEXT("Orbital field candidate lost its intended macro mode"); return false; }
                if (UsesCombined() && (!MID->GetScalarParameterValue(FHashedMaterialParameterInfo(TEXT("APS_NormalMacroWarpMode")), Mode) || Mode != 1.0f))
                { Error = TEXT("Combined candidate lost its normal/field enable mode"); return false; }
            }
            if (!SamePhysicalUniforms(MID, Original.Get(), Error)) return false;
            FVector4 Center, Inverse;
            Original->GetDoubleVectorParameterValue(FHashedMaterialParameterInfo(TEXT("APS_SharedPlanetCenter")), Center);
            Original->GetDoubleVectorParameterValue(FHashedMaterialParameterInfo(TEXT("APS_SharedInverseScale")), Inverse);
            if (!FVector(Center.X, Center.Y, Center.Z).Equals(Mesh->GetComponentLocation(), 0.001)
                || !FMath::IsNearlyEqual(Inverse.X * PresentationScale * Mesh->GetComponentScale().GetAbsMax(), 1.0, 1.e-9))
            { Error = TEXT("PLANET native physical frame is stale after production orbit"); return false; }
            if(FParse::Param(FCommandLine::Get(),TEXT("APSProbeShoreWaterFactory")))
            {
                auto* Ocean=G->GetActivePreviewOceanProxy();
                auto* Water=IsValid(Ocean)?Cast<UMaterialInstanceDynamic>(Ocean->GetMaterial(0)):nullptr;
                auto* Binding=G->GetWorld()->GetSubsystem<UAPSWaterLightingSubsystem>();
                if(!UsesPublishedViews() || !APSShoreWaterMaterial::IsVisiblePreviewOcean(G,Ocean)
                    || !Binding || !Binding->HasCurrentBinding(Water)
                    || !APSWaterSurfaceLighting::Matches(Water,G->GetWorld()))
                { Error=TEXT("PLANET shore factory lost visible ocean/runtime light binding; probe does not repair it");return false; }
                UE_LOG(LogTemp,Display,TEXT("PLANET_SHORE_FACTORY view=%d material=%s visible=1 runtimeBinding=1 noRepair=1"),
                    View,*GetPathNameSafe(Water));
            }
            return true;
        }

    public:
        ~FProbe() { Restore(); }
        void Restore()
        {
            BufferViews.Restore();
            if (!PatternProbe.Restore()) UE_LOG(LogTemp, Error, TEXT("Pattern probe restore failed"));
            if (!LayerTransfer.Restore()) UE_LOG(LogTemp, Error, TEXT("Tundra transfer restore failed"));
            if (!bActive) return;
            if (bMacroOverridden && Original.IsValid()) Original->SetScalarParameterValue(TEXT("APS_OrbitalMacroMode"), SavedMacroMode);
            bMacroOverridden = false;
            if (Terrain.IsValid()) Terrain->SetMaterial(0, Original.Get());
            if (Generator.IsValid())
            {
                // Restore through the same public camera controls. Neither mesh
                // frame nor private observer state is assigned by this fixture.
                const auto Current = Generator->GetContinuousPreviewOrbit();
                const FRotator From = Current.Outward.Rotation(), To = InitialOrbit.Outward.Rotation();
                Generator->BeginPreviewCameraOrbit();
                Generator->OrbitPreviewCamera(FVector2D(FRotator::NormalizeAxis(From.Yaw - To.Yaw) / 0.18,
                    (To.Pitch - From.Pitch) / 0.14));
                Generator->EndPreviewCameraOrbit();
                const auto* Body = Generator->GetActivePreviewWorldScapeBody();
                const double RadiusCm = Body ? double(Body->PlanetRadiusKM) * 100000.0 : 0.0;
                Generator->ZoomPreviewCamera(float(FAPSPreviewCameraBounds::WheelDeltaBetween(
                    Current.DistanceCm, InitialOrbit.DistanceCm, RadiusCm)));
                // Restore is queued too. Keep its normal tick alive; production
                // disables idle ticks after applying the restored presentation.
                Generator->SetActorTickEnabled(true);
            }
            bActive = false; bPresentationPending = false;
        }

        EResult Update(AAstroGenerator* G, UWorldGenerationViewModel* VM, bool InFar, float Zoom,
            TFunctionRef<void(const TCHAR*)> Capture, FString& Error)
        {
            Error.Reset(); bFar = InFar;
            const bool bNativeViews = UsesNativeViews();
            const bool bBuffers = APSPlanetBufferViews::Requested();
            const bool bPatterns = APSPlanetPatternProbe::Requested();
            if (bPatterns && (!bNativeViews || UsesZoomSweep() || bBuffers || APSTundraLayerTransfer::Requested()
                || FParse::Value(FCommandLine::Get(), TEXT("APSProbeTerrainMacroMode="), SavedMacroMode)))
            { Error = TEXT("Pattern isolation requires native fixed views with no other overrides"); return EResult::Failed; }
            if (bBuffers && (!bNativeViews || UsesZoomSweep() || APSTundraLayerTransfer::Requested()
                || FParse::Param(FCommandLine::Get(), TEXT("APSProbeTerrainLodAB"))))
            { Error = TEXT("Buffer isolation requires native fixed views without other A/B modes"); return EResult::Failed; }
            float FramedHeightKm = -1.0f;
            const bool bFramedHeight = FParse::Value(FCommandLine::Get(), TEXT("APSPlanetProbeHeightKm="), FramedHeightKm);
            if (UsesPublishedViews() && (!bNativeViews || !APSTerrainContinuityMaterial::Enabled()
                || bBuffers || bPatterns || APSTundraLayerTransfer::Requested()))
            { Error = TEXT("Published views require the enabled production route without diagnostic material overrides"); return EResult::Failed; }
            if (UsesFamilyScaleAB() && (!(UsesOrbitalFields() || UsesPublishedViews()) || !bFramedHeight || FramedHeightKm != 20000.0f))
            { Error = TEXT("Family scale A/B requires OrbitalFields and the 20000km initial view"); return EResult::Failed; }
            if (bFramedHeight && (UsesZoomSweep() || !FMath::IsFinite(FramedHeightKm) || FramedHeightKm < 70.0f))
            { Error = TEXT("Explicit PLANET probe height must be finite, >=70km, without zoom sweep"); return EResult::Failed; }
            if (UsesZoomSweep() && (!bNativeViews || APSTundraLayerTransfer::Requested()))
            { Error = TEXT("Zoom sweep requires native views without layer-transfer diagnostics"); return EResult::Failed; }
            if (APSTundraLayerTransfer::Requested() && !bNativeViews)
            { Error = TEXT("Tundra layer transfer requires explicit native views"); return EResult::Failed; }
            if (int32(UsesSlopeOnly()) + int32(UsesWarpOnly()) + int32(UsesSlopeSide()) + int32(UsesOrbitalFields()) > 1 || ((UsesSlopeOnly() || UsesWarpOnly() || UsesSlopeSide() || UsesOrbitalFields()) && (bNativeViews || InFar || !FParse::Param(FCommandLine::Get(), TEXT("APSProbeTerrainLodAB")))))
            { Error = TEXT("Warp-only diagnostic requires isolated terrain A/B, not native/far mode"); return EResult::Failed; }
            if (bNativeViews && (InFar || FParse::Param(FCommandLine::Get(), TEXT("APSProbeTerrainLodAB"))))
            { Error = TEXT("Native terrain views cannot mix with diagnostic material A/B"); return EResult::Failed; }
            if (!G || !VM || !G->UsesContinuousPreviewFrame() || !G->GetPreviewCameraComponent()
                || !FMath::IsFinite(Zoom) || Zoom <= 0.0f)
            { Error = TEXT("PLANET terrain A/B requires a valid continuous view and positive finite zoom"); return EResult::Failed; }
            const double Now = FPlatformTime::Seconds();
            if (Now < Next)
            {
                const uint32 Cycles = GGPUFrameTime;
                if (bActive && bViewSettled && Now > Next - 0.75 && Cycles && GpuMs.Num() < 256)
                    GpuMs.Add(FPlatformTime::ToMilliseconds(Cycles));
                return EResult::Pending;
            }
            if (!bActive)
            {
                auto* M = G->GetActivePreviewTerrainProxy();
                const auto* S = M ? M->GetProcMeshSection(0) : nullptr;
                auto* Native = M ? Cast<UMaterialInstanceDynamic>(M->GetMaterial(0)) : nullptr;
                const FString NativeParent = Native && Native->Parent ? Native->Parent->GetPathName() : FString();
                const bool bNativeTerra = NativeParent == TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/Shared/MI_APS_SharedTerra.MI_APS_SharedTerra");
                const bool bNativeMagma = (UsesOrbitalFields() || bNativeViews) && NativeParent == TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/Shared/MI_APS_SharedMagma.MI_APS_SharedMagma");
                const bool bContinuous = bNativeViews && NativeParent == APSTerrainContinuityMaterial::TemplatePath;
                if (!S || S->ProcVertexBuffer.IsEmpty() || S->ProcIndexBuffer.IsEmpty() || M->GetNumSections() != 1
                    || !M->IsVisible() || !APSSharedTerrainMaterial::IsSharedStack(Native) || !Native->Parent
                    || Native->GetBlendMode() != BLEND_Masked
                    || (!bNativeTerra && !bNativeMagma && !bContinuous)
                    || Native->GetMaterial()->GetPathName() != (bContinuous ? APSTerrainContinuityMaterial::MasterPath : APSSharedTerrainMaterial::MasterPath()))
                { Error = TEXT("PLANET A/B requires the visible closed shared Terra (or audited field-only Magma) mesh"); return EResult::Failed; }
                if (UsesPublishedViews())
                {
                    const auto* Planet=Cast<APlanet>(G->GetActivePreviewWorldScapeBody());
                    if (!Planet || bContinuous != APSTerrainContinuityMaterial::Allows(Planet->PlanetType))
                    { Error=TEXT("Published material family gate did not select the intended stack");return EResult::Failed; }
                    UE_LOG(LogTemp,Display,TEXT("PLANET_TERRAIN_PUBLISHED type=%d parent=%s noTestSubstitution=1"),int32(Planet->PlanetType),*NativeParent);
                }
                // Select by the real bound material, not by a requested family
                // label. Static/texture compatibility is still required below.
                const auto Ready = bNativeViews ? EResult::Finished : Warm(G->GetWorld(), bNativeMagma, Error);
                if (Ready != EResult::Finished) return Ready;
                if (UsesOrbitalFields() && !ValidateFieldTemplate(Native, Error)) return EResult::Failed;
                FVector4 Inverse;
                if (!Native->GetDoubleVectorParameterValue(FHashedMaterialParameterInfo(TEXT("APS_SharedInverseScale")), Inverse)
                    || !FMath::IsFinite(Inverse.X) || Inverse.X <= 0.0 || M->GetComponentScale().GetMin() <= 0.0)
                { Error = TEXT("PLANET A/B native physical frame missing"); return EResult::Failed; }
                PresentationScale = 1.0 / (Inverse.X * M->GetComponentScale().GetAbsMax());
                if (bNativeViews)
                {
                    auto* R = Native->GetMaterialResource(G->GetWorld()->GetFeatureLevel());
                    const auto* Map = R ? R->GetGameThreadShaderMap() : nullptr;
                    if (R && R->GetCompileErrors().Num())
                    { Error = TEXT("Native shared terrain shader compile errors"); return EResult::Failed; }
                    if (!R || !R->IsGameThreadShaderMapComplete() || !Map || !Map->GetMeshShaderMap(&FLocalVertexFactory::StaticType))
                        return EResult::Pending; // Production warm queue owns native shader submission.
                }
                for (int32 I = 0; I < (bNativeViews ? 0 : bFar ? 2 : 1); ++I)
                {
                    Candidates[I].Reset(UMaterialInstanceDynamic::Create(Templates[I].Get(), M));
                    if (!Candidates[I].IsValid()) { Error = TEXT("PLANET A/B MID allocation failed"); return EResult::Failed; }
                    // Candidate graphs duplicate the native parameter GUIDs. Do
                    // not apply a resolver or palette preset over these values.
                    Candidates[I]->CopyMaterialUniformParameters(Native);
                    if (UsesOrbitalFields()) Candidates[I]->SetScalarParameterValue(TEXT("APS_OrbitalMacroMode"), 1.0f);
                    if (UsesCombined()) Candidates[I]->SetScalarParameterValue(TEXT("APS_NormalMacroWarpMode"), 1.0f);
                }
                Generator = G; Terrain = M; Original.Reset(Native);
                if (APSTundraLayerTransfer::ContributionsRequested()) APSTundraLayerTransfer::LogPayload(*S);
                InitialOrbit = G->GetContinuousPreviewOrbit();
                bActive = true; // Own restoration before any optional test-only mutation can fail.
                if (bBuffers && !BufferViews.Begin(Error)) return EResult::Failed;
                float MacroMode = 0.0f;
                if (FParse::Value(FCommandLine::Get(), TEXT("APSProbeTerrainMacroMode="), MacroMode))
                {
                    if (!bNativeViews || !FMath::IsFinite(MacroMode) || (MacroMode != 0 && MacroMode != 1 && MacroMode != 2)
                        || !Original->GetScalarParameterValue(FHashedMaterialParameterInfo(TEXT("APS_OrbitalMacroMode")), SavedMacroMode))
                    { Error = TEXT("Macro override requires native views and an existing mode parameter, exactly 0/1/2"); return EResult::Failed; }
                    bMacroOverridden = true;
                    Original->SetScalarParameterValue(TEXT("APS_OrbitalMacroMode"), MacroMode);
                    UE_LOG(LogTemp, Display, TEXT("PLANET_TERRAIN_TRANSIENT_MACRO previous=%.6f diagnostic=%.6f; no saved asset writes"), SavedMacroMode, MacroMode);
                }
                VertexCRC = FCrc::MemCrc32(S->ProcVertexBuffer.GetData(), S->ProcVertexBuffer.Num() * sizeof(FProcMeshVertex));
                IndexCRC = FCrc::MemCrc32(S->ProcIndexBuffer.GetData(), S->ProcIndexBuffer.Num() * sizeof(uint32));
                // Native-only option frames once before all three real views;
                // it never loads, allocates, or binds diagnostic materials.
                if (bFramedHeight)
                {
                    const auto* Body = G->GetActivePreviewWorldScapeBody();
                    if (!Body || Body->PlanetRadiusKM <= 0) { Error = TEXT("Physical radius missing for explicit probe height"); return EResult::Failed; }
                    const double RadiusCm = double(Body->PlanetRadiusKM) * 100000.0;
                    const double TargetCm = RadiusCm + FramedHeightKm * 100000.0;
                    VM->ZoomPreview(float(FAPSPreviewCameraBounds::WheelDeltaBetween(InitialOrbit.DistanceCm, TargetCm, RadiusCm)));
                    if (!FMath::IsNearlyEqual(G->GetContinuousPreviewOrbit().DistanceCm / TargetCm, 1.0, 1.e-6))
                    { Error = TEXT("Explicit probe height was not reached"); return EResult::Failed; }
                    UE_LOG(LogTemp, Display, TEXT("PLANET_TERRAIN_FRAMED_HEIGHT heightKm=%.6f orbitTargetKm=%.6f presentationPending=1"),
                        FramedHeightKm, (G->GetContinuousPreviewOrbit().DistanceCm - RadiusCm) / 100000.0);
                }
                else if (UsesZoomSweep())
                {
                    if (!SetSweepView(VM, Error)) return EResult::Failed;
                }
                else if (bNativeViews)
                {
                    VM->ZoomPreview(Zoom);
                    UE_LOG(LogTemp, Display, TEXT("PLANET_TERRAIN_NATIVE_ZOOM wheelDelta=%.6f distanceBeforeCm=%.12g distanceAfterCm=%.12g fov=%.3f productionMaterial=%s"),
                        Zoom, InitialOrbit.DistanceCm, G->GetContinuousPreviewOrbit().DistanceCm,
                        G->GetPreviewCameraComponent()->FieldOfView, *Native->GetPathName());
                }
                AwaitPresentation(Now, 2.0);
                return EResult::Pending;
            }
            if (bPresentationPending) return PollPresentation(Now, Error);
            // Production preview lighting is updated by the world subsystem,
            // independently of the frozen generator. Let it settle after an
            // orbit before taking the paired baseline; never resnapshot it
            // between material variants to conceal a lighting change.
            if (!bViewSettled)
            {
                // Resolve preview-only light interpolation to its own current
                // target. This does not tick the world, move bodies, alter time
                // or choose different light settings. Normal ticks continue,
                // and the exact-state check remains active for every pair.
                auto* Stellar = G->GetWorld()->GetSubsystem<UAPSStellarVisualSubsystem>();
                if (!Stellar) { Error = TEXT("PLANET A/B stellar subsystem missing"); return EResult::Failed; }
                Stellar->Tick(100.0f);
                UE_LOG(LogTemp, Display, TEXT("PLANET_TERRAIN_AB_LIGHT_SETTLE view=%d forced interpolation to current target before baseline; no world-time advance"), View);
                SnapshotView(); bViewSettled = true; Next = Now + 1.0;
                return EResult::Pending;
            }
            if (APSTundraLayerTransfer::Requested())
            {
                if (!LayerTransfer.IsActive() && !LayerTransfer.Begin(Original.Get(), Error)) return EResult::Failed;
                if (!LayerTransfer.Validate(Error)) return EResult::Failed;
            }
            if (!Validate(Error)) return EResult::Failed;
            if (bPatterns)
            {
                if (!PatternProbe.IsActive() && !PatternProbe.Begin(Original.Get(), Error)) return EResult::Failed;
                if (!PatternProbe.Validate(Error)) return EResult::Failed;
            }
            if (bBuffers && !BufferViews.Validate(Variant, Error)) return EResult::Failed;
            if (APSTundraLayerTransfer::NormalRequested() && Variant == 0)
            {
                FVector4 Inverse;
                if (!Original->GetDoubleVectorParameterValue(FHashedMaterialParameterInfo(TEXT("APS_SharedInverseScale")), Inverse)
                    || !APSTundraLayerTransfer::CheckNormalGeometry(*Terrain->GetProcMeshSection(0),
                        MeshFrame, CameraFrame.GetLocation(), Inverse.X, Error)) return EResult::Failed;
            }
            const FString ScaleViewName = FString::Printf(TEXT("%02d-%s-%dkm"), View, View < 3 ? TEXT("whole") : TEXT("close"), View < 3 ? 20000 : 2000);
            const TCHAR* ViewName = UsesFamilyScaleAB() ? *ScaleViewName : bNativeViews
                ? (View == 0 ? TEXT("00-framed") : View == 1 ? TEXT("01-framed-orbit1") : TEXT("02-framed-orbit2"))
                : (View == 0 ? TEXT("00-whole") : View == 1 ? TEXT("01-close-orbit1") : TEXT("02-close-orbit2"));
            const TCHAR* VariantName = bPatterns ? APSPlanetPatternProbe::FScope::Label(Variant)
                : bBuffers ? APSPlanetBufferViews::FScope::Label(Variant)
                : APSTundraLayerTransfer::Requested() ? APSTundraLayerTransfer::FPair::Label(Variant)
                : Variant == 0 ? (UsesPublishedViews()?TEXT("published"):TEXT("native")) : Variant == 1 ? (UsesCombined() ? TEXT("combined-fields-normal") : UsesOrbitalFields() ? TEXT("orbital-color-fields") : UsesSlopeSide() ? TEXT("filtered-slope-side") : UsesSlopeOnly() ? TEXT("native-slope") : UsesWarpOnly() ? TEXT("pixel-warp-only") : TEXT("pixel-vi")) : TEXT("pixel-far-normal");
            const FString Label = UsesZoomSweep() ? FString::Printf(TEXT("zoom-%02d-%s"), View, VariantName)
                : FString::Printf(TEXT("%s-%s"), ViewName, VariantName);
            UE_LOG(LogTemp, Display, TEXT("PLANET_TERRAIN_AB view=%s variant=%s observer=%s bodyFrame=%s camera=%s verticesCRC=%u indicesCRC=%u material=%s"),
                ViewName, VariantName, *Observer.ToString(), *MeshFrame.ToHumanReadableString(), *CameraFrame.ToHumanReadableString(),
                VertexCRC, IndexCRC, *Terrain->GetMaterial(0)->GetPathName());
            Capture(*Label);
            GpuMs.Sort();
            double GpuSum = 0.0;
            for (double Ms : GpuMs) GpuSum += Ms;
            UE_LOG(LogTemp, Display, TEXT("PLANET_TERRAIN_AB_GPU view=%s variant=%s samples=%d meanMs=%.5f p95Ms=%.5f; async counter, settled camera; not a live-flight FPS claim"),
                *Label, VariantName, GpuMs.Num(), GpuMs.IsEmpty() ? -1.0 : GpuSum / GpuMs.Num(),
                GpuMs.IsEmpty() ? -1.0 : GpuMs[FMath::FloorToInt((GpuMs.Num()-1)*0.95)]);
            GpuMs.Reset();
            if (++Variant < (bPatterns ? APSPlanetPatternProbe::FScope::Count() : bBuffers ? APSPlanetBufferViews::FScope::Count : APSTundraLayerTransfer::Requested() ? 4 : bNativeViews ? 1 : bFar ? 3 : 2))
            {
                if (bPatterns)
                {
                    if (!PatternProbe.Apply(Variant, Error)) return EResult::Failed;
                    Next = Now + 2.0; return EResult::Pending;
                }
                if (bBuffers)
                {
                    if (!BufferViews.Apply(Variant, Error)) return EResult::Failed;
                    Next = Now + 2.0; return EResult::Pending;
                }
                if (APSTundraLayerTransfer::Requested())
                {
                    if (!LayerTransfer.Apply(Variant, Error)) return EResult::Failed;
                    Next = Now + 2.0; return EResult::Pending;
                }
                auto* Candidate = Candidates[Variant - 1].Get();
                Candidate->CopyMaterialUniformParameters(Original.Get());
                // Copy includes scalar overrides. Apply the candidate mode AFTER
                // the last copy, not only at MID allocation, and validate it above.
                if (UsesOrbitalFields())
                {
                    Candidate->SetScalarParameterValue(TEXT("APS_OrbitalMacroMode"), 1.0f);
                    if (UsesCombined()) Candidate->SetScalarParameterValue(TEXT("APS_NormalMacroWarpMode"), 1.0f);
                    UE_LOG(LogTemp, Display, TEXT("PLANET_TERRAIN_ORBITAL_FIELDS boundMacroMode=1 afterUniformCopy=1"));
                }
                // CopyMaterialUniformParameters includes double vectors and the
                // split detail rows. The frozen native frame was validated above;
                // do not recompute it through a recovered/rounded inverse scale.
                if (!SamePhysicalUniforms(Candidate, Original.Get(), Error)) return EResult::Failed;
                FVector4 CopiedInverse;
                Candidate->GetDoubleVectorParameterValue(FHashedMaterialParameterInfo(TEXT("APS_SharedInverseScale")), CopiedInverse);
                const double ReconstructedInverse = 1.0 / (PresentationScale * Terrain->GetComponentScale().GetAbsMax());
                UE_LOG(LogTemp, Display, TEXT("PLANET_TERRAIN_EXACT_FRAME copiedInverse=%.17g reconstructedInverse=%.17g roundTripDelta=%.17g; exact native uniforms retained"),
                    CopiedInverse.X, ReconstructedInverse, ReconstructedInverse - CopiedInverse.X);
                Terrain->SetMaterial(0, Candidate); Next = Now + 2.0; return EResult::Pending;
            }
            if (!LayerTransfer.Restore()) { Error = TEXT("Tundra transfer exact restore failed"); return EResult::Failed; }
            if (!PatternProbe.Restore()) { Error = TEXT("Pattern probe exact restore failed"); return EResult::Failed; }
            Terrain->SetMaterial(0, Original.Get()); Variant = 0;
            if (++View == (UsesZoomSweep() ? SweepViewCount : UsesFamilyScaleAB() ? 6 : 3)) { Restore(); return EResult::Finished; }
            if (UsesZoomSweep())
            {
                if (!SetSweepView(VM, Error)) return EResult::Failed;
                AwaitPresentation(Now, 1.0); return EResult::Pending;
            }
            BeforePresentationObserver = G->GetContinuousPreviewFrame().ObserverCm;
            BeforePresentationBody = Terrain->GetComponentTransform();
            if (UsesFamilyScaleAB() && View == 3)
            {
                const auto* Body = G->GetActivePreviewWorldScapeBody();
                const double RadiusCm = double(Body->PlanetRadiusKM) * 100000.0;
                const double TargetCm = RadiusCm + 2000.0 * 100000.0;
                VM->ZoomPreview(float(FAPSPreviewCameraBounds::WheelDeltaBetween(
                    G->GetContinuousPreviewOrbit().DistanceCm, TargetCm, RadiusCm)));
                const double ActualCm = G->GetContinuousPreviewOrbit().DistanceCm;
                if (!FMath::IsNearlyEqual(ActualCm / TargetCm, 1.0, 1.e-6))
                { Error = TEXT("Family close-orbit height not reached"); return EResult::Failed; }
                UE_LOG(LogTemp, Display, TEXT("PLANET_TERRAIN_FRAMED_HEIGHT heightKm=2000 orbitTargetKm=%.6f presentationPending=1"), (ActualCm - RadiusCm) / 100000.0);
            }
            if (View == 1 && !bNativeViews && !bFramedHeight) VM->ZoomPreview(Zoom);
            VM->BeginPreviewOrbit();
            VM->OrbitPreview((UsesFamilyScaleAB() ? View % 3 != 2 : View == 1) ? FVector2D(240, 50) : FVector2D(-480, -100));
            VM->EndPreviewOrbit();
            AwaitPresentation(Now, 3.0, true); return EResult::Pending;
        }
    };
}
#endif
