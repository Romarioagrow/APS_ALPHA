#pragma once

#if WITH_DEV_AUTOMATION_TESTS
#include "APS_ALPHA/Core/Planetary/APSSharedTerrainMaterial.h"
#include "APS_ALPHA/Core/Rendering/APSStellarVisualSubsystem.h"
#include "APS_ALPHA/Generation/AstroGenerator.h"
#include "APS_ALPHA/UI/MainMenu/WorldGenerationViewModel.h"
#include "Camera/CameraComponent.h"
#include "Components/LightComponent.h"
#include "EngineUtils.h"
#include "LocalVertexFactory.h"
#include "MaterialShared.h"
#include "Misc/Crc.h"
#include "ProceduralMeshComponent.h"
#include "UObject/StrongObjectPtr.h"
#include "APSTundraLayerTransferProbe.h"
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
    inline bool UsesNativeViews() { return FParse::Param(FCommandLine::Get(), TEXT("APSProbeTerrainNativeViews")); }
    enum class EResult : uint8 { Pending, Finished, Failed };
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
        TWeakObjectPtr<AAstroGenerator> Generator;
        TWeakObjectPtr<UProceduralMeshComponent> Terrain;
        bool Requested[2] = { false, false }, bActive = false, bOriginalTick = false, bFar = false, bViewSettled = false;
        int32 View = 0, Variant = 0;
        double Next = 0.0, PresentationScale = 0.0;
        uint32 VertexCRC = 0, IndexCRC = 0;
        FTransform MeshFrame, CameraFrame;
        FVector Observer;
        float FieldOfView = 0.0f;
        FAPSContinuousPreviewOrbit InitialOrbit;
        struct FLightState
        {
            TWeakObjectPtr<ULightComponent> Component;
            FTransform Frame;
            float Intensity = 0;
            FColor Color;
            bool CastShadows = false;
        };
        TArray<FLightState> Lights;

        EResult Warm(UWorld* World, FString& Error)
        {
            const TCHAR* CurrentPixelTemplate = UsesSlopeOnly()
                ? TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/Diagnostics/LodSlopePreserve20260927/MI_APS_LodPixelTerra.MI_APS_LodPixelTerra") : UsesWarpOnly()
                ? TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/Diagnostics/LodWarpPixel20260926/MI_APS_LodPixelTerra.MI_APS_LodPixelTerra") : PixelTemplate;
            const TCHAR* CurrentPixelMaster = UsesSlopeOnly()
                ? TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/Diagnostics/LodSlopePreserve20260927/M_APS_LodPixelTerrain.M_APS_LodPixelTerrain") : UsesWarpOnly()
                ? TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/Diagnostics/LodWarpPixel20260926/M_APS_LodPixelTerrain.M_APS_LodPixelTerrain") : PixelMaster;
            bool Ready = true;
            for (int32 I = 0; I < (bFar ? 2 : 1); ++I)
            {
                if (!Templates[I].IsValid()) Templates[I].Reset(LoadObject<UMaterialInstance>(nullptr, I ? FarTemplate : CurrentPixelTemplate));
                auto* M = Templates[I].Get();
                if (!M || !M->GetMaterial() || M->GetMaterial()->GetPathName() != (I ? FarMaster : CurrentPixelMaster)
                    || M->GetBlendMode() != BLEND_Masked)
                { Error = TEXT("Exact saved Terra diagnostic candidate is absent or has the wrong master"); return EResult::Failed; }
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

        bool Validate(FString& Error) const
        {
            auto* Mesh = Terrain.Get();
            const auto* G = Generator.Get();
            const auto* S = Mesh ? Mesh->GetProcMeshSection(0) : nullptr;
            const auto* Camera = G ? G->GetPreviewCameraComponent() : nullptr;
            UMaterialInterface* Expected = (Variant == 0 || APSTundraLayerTransfer::Requested())
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
            for (const TCHAR* Name : { TEXT("APS_SharedPlanetCenter"), TEXT("APS_SharedInverseScale"),
                TEXT("APS_SharedAxisX"), TEXT("APS_SharedAxisY"), TEXT("APS_SharedAxisZ") })
            {
                FVector4 A, B;
                if (!MID || !MID->GetDoubleVectorParameterValue(FHashedMaterialParameterInfo(Name), A)
                    || !Original->GetDoubleVectorParameterValue(FHashedMaterialParameterInfo(Name), B) || A != B)
                { Error = FString(TEXT("PLANET terrain A/B physical frame mismatch: ")) + Name; return false; }
            }
            FVector4 Center, Inverse;
            Original->GetDoubleVectorParameterValue(FHashedMaterialParameterInfo(TEXT("APS_SharedPlanetCenter")), Center);
            Original->GetDoubleVectorParameterValue(FHashedMaterialParameterInfo(TEXT("APS_SharedInverseScale")), Inverse);
            if (!FVector(Center.X, Center.Y, Center.Z).Equals(Mesh->GetComponentLocation(), 0.001)
                || !FMath::IsNearlyEqual(Inverse.X * PresentationScale * Mesh->GetComponentScale().GetAbsMax(), 1.0, 1.e-9))
            { Error = TEXT("PLANET native physical frame is stale after production orbit"); return false; }
            return true;
        }

    public:
        ~FProbe() { Restore(); }
        void Restore()
        {
            if (!LayerTransfer.Restore()) UE_LOG(LogTemp, Error, TEXT("Tundra transfer restore failed"));
            if (!bActive) return;
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
                Generator->ZoomPreviewCamera(static_cast<float>(FMath::Loge(InitialOrbit.DistanceCm / Current.DistanceCm) / FMath::Loge(0.82)));
                Generator->SetActorTickEnabled(bOriginalTick);
            }
            bActive = false;
        }

        EResult Update(AAstroGenerator* G, UWorldGenerationViewModel* VM, bool InFar, float Zoom,
            TFunctionRef<void(const TCHAR*)> Capture, FString& Error)
        {
            Error.Reset(); bFar = InFar;
            const bool bNativeViews = UsesNativeViews();
            if (APSTundraLayerTransfer::Requested() && !bNativeViews)
            { Error = TEXT("Tundra layer transfer requires explicit native views"); return EResult::Failed; }
            if ((UsesSlopeOnly() && UsesWarpOnly()) || ((UsesSlopeOnly() || UsesWarpOnly()) && (bNativeViews || InFar || !FParse::Param(FCommandLine::Get(), TEXT("APSProbeTerrainLodAB")))))
            { Error = TEXT("Warp-only diagnostic requires isolated terrain A/B, not native/far mode"); return EResult::Failed; }
            if (bNativeViews && (InFar || FParse::Param(FCommandLine::Get(), TEXT("APSProbeTerrainLodAB"))))
            { Error = TEXT("Native terrain views cannot mix with diagnostic material A/B"); return EResult::Failed; }
            if (!G || !VM || !G->UsesContinuousPreviewFrame() || !G->GetPreviewCameraComponent()
                || !FMath::IsFinite(Zoom) || Zoom <= 0.0f)
            { Error = TEXT("PLANET terrain A/B requires a valid continuous view and positive finite zoom"); return EResult::Failed; }
            const double Now = FPlatformTime::Seconds();
            if (Now < Next) return EResult::Pending;
            if (!bActive)
            {
                const auto Ready = bNativeViews ? EResult::Finished : Warm(G->GetWorld(), Error);
                if (Ready != EResult::Finished) return Ready;
                auto* M = G->GetActivePreviewTerrainProxy();
                const auto* S = M ? M->GetProcMeshSection(0) : nullptr;
                auto* Native = M ? Cast<UMaterialInstanceDynamic>(M->GetMaterial(0)) : nullptr;
                if (!S || S->ProcVertexBuffer.IsEmpty() || S->ProcIndexBuffer.IsEmpty() || M->GetNumSections() != 1
                    || !M->IsVisible() || !APSSharedTerrainMaterial::IsSharedStack(Native) || !Native->Parent
                    || Native->GetBlendMode() != BLEND_Masked
                    || Native->Parent->GetPathName() != TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/Shared/MI_APS_SharedTerra.MI_APS_SharedTerra"))
                { Error = TEXT("PLANET A/B requires the visible actual closed native Terra mesh"); return EResult::Failed; }
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
                }
                Generator = G; Terrain = M; Original.Reset(Native);
                if (APSTundraLayerTransfer::ContributionsRequested()) APSTundraLayerTransfer::LogPayload(*S);
                InitialOrbit = G->GetContinuousPreviewOrbit(); bOriginalTick = G->IsActorTickEnabled();
                VertexCRC = FCrc::MemCrc32(S->ProcVertexBuffer.GetData(), S->ProcVertexBuffer.Num() * sizeof(FProcMeshVertex));
                IndexCRC = FCrc::MemCrc32(S->ProcIndexBuffer.GetData(), S->ProcIndexBuffer.Num() * sizeof(uint32));
                // Native-only option frames once before all three real views;
                // it never loads, allocates, or binds diagnostic materials.
                if (bNativeViews)
                {
                    VM->ZoomPreview(Zoom);
                    UE_LOG(LogTemp, Display, TEXT("PLANET_TERRAIN_NATIVE_ZOOM wheelDelta=%.6f distanceBeforeCm=%.12g distanceAfterCm=%.12g fov=%.3f productionMaterial=%s"),
                        Zoom, InitialOrbit.DistanceCm, G->GetContinuousPreviewOrbit().DistanceCm,
                        G->GetPreviewCameraComponent()->FieldOfView, *Native->GetPathName());
                }
                G->SetActorTickEnabled(false); bActive = true; SnapshotView(); Next = Now + 2.0;
                return EResult::Pending;
            }
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
            if (APSTundraLayerTransfer::NormalRequested() && Variant == 0)
            {
                FVector4 Inverse;
                if (!Original->GetDoubleVectorParameterValue(FHashedMaterialParameterInfo(TEXT("APS_SharedInverseScale")), Inverse)
                    || !APSTundraLayerTransfer::CheckNormalGeometry(*Terrain->GetProcMeshSection(0),
                        MeshFrame, CameraFrame.GetLocation(), Inverse.X, Error)) return EResult::Failed;
            }
            const TCHAR* ViewName = bNativeViews
                ? (View == 0 ? TEXT("00-framed") : View == 1 ? TEXT("01-framed-orbit1") : TEXT("02-framed-orbit2"))
                : (View == 0 ? TEXT("00-whole") : View == 1 ? TEXT("01-close-orbit1") : TEXT("02-close-orbit2"));
            const TCHAR* VariantName = APSTundraLayerTransfer::Requested() ? APSTundraLayerTransfer::FPair::Label(Variant)
                : Variant == 0 ? TEXT("native") : Variant == 1 ? (UsesSlopeOnly() ? TEXT("native-slope") : UsesWarpOnly() ? TEXT("pixel-warp-only") : TEXT("pixel-vi")) : TEXT("pixel-far-normal");
            const FString Label = FString::Printf(TEXT("%s-%s"), ViewName, VariantName);
            UE_LOG(LogTemp, Display, TEXT("PLANET_TERRAIN_AB view=%s variant=%s observer=%s bodyFrame=%s camera=%s verticesCRC=%u indicesCRC=%u material=%s"),
                ViewName, VariantName, *Observer.ToString(), *MeshFrame.ToHumanReadableString(), *CameraFrame.ToHumanReadableString(),
                VertexCRC, IndexCRC, *Terrain->GetMaterial(0)->GetPathName());
            Capture(*Label);
            if (++Variant < (APSTundraLayerTransfer::Requested() ? 4 : bNativeViews ? 1 : bFar ? 3 : 2))
            {
                if (APSTundraLayerTransfer::Requested())
                {
                    if (!LayerTransfer.Apply(Variant, Error)) return EResult::Failed;
                    Next = Now + 2.0; return EResult::Pending;
                }
                auto* Candidate = Candidates[Variant - 1].Get();
                Candidate->CopyMaterialUniformParameters(Original.Get());
                if (!APSSharedTerrainMaterial::WritePhysicalFrame(Candidate, Terrain.Get(), PresentationScale))
                { Error = TEXT("PLANET A/B candidate frame refresh failed"); return EResult::Failed; }
                Terrain->SetMaterial(0, Candidate); Next = Now + 2.0; return EResult::Pending;
            }
            if (!LayerTransfer.Restore()) { Error = TEXT("Tundra transfer exact restore failed"); return EResult::Failed; }
            Terrain->SetMaterial(0, Original.Get()); Variant = 0;
            if (++View == 3) { Restore(); return EResult::Finished; }
            const FVector BeforeObserver = G->GetContinuousPreviewFrame().ObserverCm;
            const FTransform BeforeBody = Terrain->GetComponentTransform();
            if (View == 1 && !bNativeViews) VM->ZoomPreview(Zoom);
            VM->BeginPreviewOrbit();
            VM->OrbitPreview(View == 1 ? FVector2D(240, 50) : FVector2D(-480, -100));
            VM->EndPreviewOrbit();
            if (G->GetContinuousPreviewFrame().ObserverCm.Equals(BeforeObserver, 1.0)
                || Terrain->GetComponentTransform().Equals(BeforeBody, 0.001))
            { Error = TEXT("PLANET A/B requested orbit did not move physical observer and body presentation"); return EResult::Failed; }
            UE_LOG(LogTemp, Display, TEXT("PLANET_TERRAIN_AB_REAL_ORBIT ordinal=%d physicalObserverDeltaCm=%.9g bodyMoved=1 nativeRestored=1"),
                View, FVector::Distance(BeforeObserver, G->GetContinuousPreviewFrame().ObserverCm));
            bViewSettled = false; Next = Now + 3.0; return EResult::Pending;
        }
    };
}
#endif
