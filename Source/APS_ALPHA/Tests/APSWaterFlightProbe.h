#pragma once
#if WITH_DEV_AUTOMATION_TESTS
#include "APSWaterNormalABProbe.h"
#include "APS_ALPHA/Generation/APSWorldScapePlanetNoise.h"
#include "HAL/PlatformMemory.h"
#include "Misc/App.h"
#include "HAL/IConsoleManager.h"
#include "RenderingThread.h"
#include "ProfilingDebugging/MiscTrace.h"

// A moving, explicitly wet fixture. No mesh freeze, CPU UV writes or per-frame
// bathymetry rebuild. Performance mode suppresses PNG capture during the route.
namespace APSWaterFlight
{
    inline bool NativeLava() { return FParse::Param(FCommandLine::Get(), TEXT("APSProbeNativeLavaFlight")); }
    inline bool Requested() { return NativeLava() || FParse::Param(FCommandLine::Get(), TEXT("APSProbeWaterFlight")); }
    inline bool Release() { return FParse::Param(FCommandLine::Get(), TEXT("APSProbeWaterRelease")); }
    inline bool FactoryShore() { return FParse::Param(FCommandLine::Get(), TEXT("APSProbeShoreWaterFactory")); }
    inline bool ShoreTransmission() { return FParse::Param(FCommandLine::Get(), TEXT("APSProbeWaterShoreTransmission")); }
    inline bool WaterPass() { return ShoreTransmission() || APSWaterNormalAB::SurfacePass(); }
    inline bool Payload() { return Release() || FParse::Param(FCommandLine::Get(), TEXT("APSWaterDepthPayload")); }
    inline bool Material() { return FParse::Param(FCommandLine::Get(), TEXT("APSProbeWaterFlightMaterial")); }
    inline bool Performance() { return Requested() && FParse::Param(FCommandLine::Get(), TEXT("APSProbeWaterFlightPerf")); }
    inline double NearHeightKm()
    {
        double Height=0.05;
        FParse::Value(FCommandLine::Get(),TEXT("APSWaterFlightNearKm="),Height);
        return FMath::IsFinite(Height) ? FMath::Clamp(Height,0.002,0.05) : 0.05;
    }
    inline double ViewPitchDegrees()
    {
        double Pitch=55.0;
        FParse::Value(FCommandLine::Get(),TEXT("APSLiquidFlightPitchDeg="),Pitch);
        return FMath::IsFinite(Pitch) ? FMath::Clamp(Pitch,30.0,89.0) : 55.0;
    }
    class FProbe
    {
        TWeakObjectPtr<AWorldScapeRoot> Root;
        TStrongObjectPtr<UMaterialInstanceDynamic> Native{nullptr}, Candidate{nullptr};
        FWSMaterialLodArray Original;
        FTransform LastFrame;
        bool bMeasuring=false;
        bool bLightingFailed=false;
        bool bFrameFailed=false;
        uint32 PreviousSignature=0;
        int32 Samples=0, Changes=0;
        int32 PeakHeightOnlyCollisionPatches=0;
        struct FFrame { uint64 Frame, Gap; double T, Dt, Wall, Game, Render, GPU; int32 Workers; };
        TArray<FFrame> Frames;
        uint64 LastCounter=0;
        double LastWall=0;
        void Style()
        {
            Candidate->CopyMaterialUniformParameters(Native.Get());
            Candidate->SetVectorParameterValue(TEXT("LiquidDeepColor"), FLinearColor(.0025f,.009f,.018f,1));
            Candidate->SetVectorParameterValue(TEXT("LiquidShallowColor"), FLinearColor(.025f,.060f,.055f,1));
            Candidate->SetScalarParameterValue(TEXT("APS_WaterDepthStrength"), 1);
            Candidate->SetScalarParameterValue(TEXT("APS_WaterHalfDepthM"), APSWaterDepthPalette::DefaultHalfDepthM);
            if (APSWaterNormalAB::SurfaceFilter()) APSWaterNormalAB::ApplySurfaceFilter(Candidate.Get());
            if (APSWaterNormalAB::SurfacePrecise() && !APSWaterNormalAB::BindPreciseFrame(Candidate.Get(), Root.Get())) bFrameFailed=true;
            if (ShoreTransmission() && !APSWaterNormalAB::BindPreciseFrame(Candidate.Get(), Root.Get())) bFrameFailed=true;
        }
    public:
        ~FProbe() { Restore(); }
        bool Begin(APlanetarySurfaceGenerator* Surface, FString& Error)
        {
            auto* R=Surface ? Surface->WorldScapeRootInstance : nullptr;
            auto* N=Surface ? Surface->ResolvedOceanMaterialInstance : nullptr;
            if (NativeLava() && (Payload() || Material() || Release()
                || FParse::Param(FCommandLine::Get(), TEXT("APSProbeWaterFlight"))
                || !Surface || Surface->ResolvedSurfaceProfile.PlanetType != EPlanetType::Volcanic
                || !APSSharedGeneratedLiquidMaterial::IsRenderReady(N, EAPSPlanetLiquidType::Lava, Surface->GetWorld())))
            { Error=TEXT("Native lava flight requires its own ready saved material; no Water substitution/payload"); return false; }
            if (!R || !R->bOcean || R->bFreezeGeneration || !R->IsActorTickEnabled()
                || !N || Surface->ResolvedSurfaceProfile.LiquidType!=(NativeLava()?EAPSPlanetLiquidType::Lava:EAPSPlanetLiquidType::Water)
                || R->OceanMaterial.DefaultMaterial!=N || !R->OceanMaterial.MaterialsLod.IsEmpty()
                || R->ActorHasTag(TEXT("APS.GeneratedOcean.BathymetryUV1"))!=Payload()
                || (Material() && !Payload()) || (APSWaterNormalAB::SurfaceFilter() && !Material())
                || APSWaterNormalAB::SurfaceAOIsolation()
                || (APSWaterNormalAB::SurfacePrecise() && (!APSWaterNormalAB::SurfaceFilter() || APSWaterNormalAB::SurfacePass()))
                || (APSWaterNormalAB::SurfacePass() && !APSWaterNormalAB::SurfaceFilter())
                || (ShoreTransmission() && (!Material() || Release() || APSWaterNormalAB::SurfaceFilter()))
                || (WaterPass() && !APSWaterSurfaceLighting::RenderContract()))
            { Error=TEXT("Water flight requires live Water root, exact native payload switch and single source MID"); return false; }
            if (Release() && (Material() || !APSCoastalWaterMaterial::IsInstance(N)))
            { Error=TEXT("Water release route requires the ordinary factory, not a test replacement"); return false; }
            if (FactoryShore() && (!Release() || !APSShoreWaterMaterial::IsInstance(N)))
            { Error=TEXT("Shore factory route must use the actual factory, no test replacement"); return false; }
            Root=R; Native.Reset(N); Original=R->OceanMaterial; LastFrame=R->GetActorTransform();
            if (Material())
            {
                auto* Template=LoadObject<UMaterialInstance>(nullptr,ShoreTransmission() ? TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/Diagnostics/WaterShoreTransmission20261001/MI_APS_WaterShoreTransmission.MI_APS_WaterShoreTransmission") : APSWaterNormalAB::SurfaceFilter() ? APSWaterNormalAB::SurfaceTemplatePath() : TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/Diagnostics/WaterDepthFiltered20260928/MI_APS_WaterDepth.MI_APS_WaterDepth"));
                if (!Template) { Error=TEXT("Water flight depth template missing"); return false; }
                Candidate.Reset(UMaterialInstanceDynamic::Create(Template,R));
                if (!Candidate.IsValid()) { Error=TEXT("Water flight MID allocation failed"); return false; }
                Style();
                if (bFrameFailed) { Error=TEXT("Water precision frame failed"); return false; }
                if (WaterPass() && (!Candidate->GetShadingModels().HasOnlyShadingModel(MSM_SingleLayerWater)
                    || !APSWaterSurfaceLighting::Bind(Candidate.Get(), R->GetWorld())))
                { Error=TEXT("Moving water pass requires actual scene fill binding"); return false; }
                auto* Resource=Candidate->GetMaterialResource(R->GetWorld()->GetFeatureLevel());
#if WITH_EDITOR
                if (Resource && !Resource->IsGameThreadShaderMapComplete())
                {
                    Resource->SubmitCompileJobs_GameThread(EShaderCompileJobPriority::ForceLocal);
                    if (GShaderCompilingManager) GShaderCompilingManager->FinishAllCompilation();
                }
#endif
                const auto* Map=Resource ? Resource->GetGameThreadShaderMap() : nullptr;
                if (!Resource || !Resource->IsGameThreadShaderMapComplete() || Resource->GetCompileErrors().Num()
                    || !Map || !Map->GetMeshShaderMap(&FLocalVertexFactory::StaticType))
                { Error=TEXT("Water flight candidate shaders not ready before measurement"); return false; }
                auto Replacement=Original; Replacement.DefaultMaterial=Candidate.Get();
                R->UpdateOceanMaterial(Replacement);
            }
            UE_LOG(LogTemp,Display,TEXT("[APS.WaterFlight] begin seed=%d ocean=1 payload=%d material=%d perf=%d noFreeze=1 UVwrites=0 nativeLava=%d parent=%s"),R->Seed,Payload(),Material(),Performance(),NativeLava(),*GetPathNameSafe(N->Parent.Get()));
            return true;
        }
        bool Active() const { return Root.IsValid(); }
        bool Inspect(FString& Error)
        {
            if (bLightingFailed) { Error=TEXT("Water flight dynamic scene fill failed"); return false; }
            if (bFrameFailed) { Error=TEXT("Water flight precise frame failed"); return false; }
            auto* R=Root.Get();
            auto* Expected=Candidate.IsValid()?Candidate.Get():Native.Get();
            if (!R || !R->bOcean || R->bFreezeGeneration || R->OceanMaterial.DefaultMaterial!=Expected
                || (NativeLava() && !APSSharedGeneratedLiquidMaterial::HasSavedParameterAuthority(Expected,EAPSPlanetLiquidType::Lava))
                || R->ActorHasTag(TEXT("APS.GeneratedOcean.BathymetryUV1"))!=Payload())
            { Error=TEXT("Water flight root/material/payload state changed"); return false; }
            if (FactoryShore())
            {
                auto* Binding=R->GetWorld()->GetSubsystem<UAPSWaterLightingSubsystem>();
                if (!Binding || !Binding->HasCurrentBinding(Expected))
                { Error=TEXT("Factory water lost runtime scene binding; probe does not repair it"); return false; }
            }
            if (Candidate.IsValid() && !LastFrame.Equals(R->GetActorTransform(),1.e-8))
            { Style(); LastFrame=R->GetActorTransform(); }
            if (bFrameFailed) { Error=TEXT("Water flight precise frame update failed"); return false; }
            if (WaterPass() && (!APSWaterSurfaceLighting::Bind(Candidate.Get(), R->GetWorld())
                || !APSWaterSurfaceLighting::Matches(Candidate.Get(), R->GetWorld())))
            { Error=TEXT("Water flight lost scene lighting parity"); return false; }
            int32 Vertices=0,Wet=0,Sections=0;
            double MaxErrorM=0;
            uint32 Signature=0;
            for (const auto* Lod:R->WorldScapeLodOcean)
            {
                auto* Mesh=IsValid(Lod)?Lod->Mesh:nullptr;
                if (!IsValid(Mesh)) { Error=TEXT("Water flight missing ocean mesh"); return false; }
                for (int32 S=0;S<Mesh->GetNumSections();++S)
                {
                    const auto* Section=Mesh->GetProcMeshSection(S);
                    if (!Section || Section->PlanetVertexBuffer.IsEmpty() || Mesh->GetMaterial(S)!=Expected)
                    { Error=TEXT("Live ocean section absent or lost material"); return false; }
                    ++Sections;
                    const FTransform Transform=Mesh->GetComponentTransform();
                    for (int32 I:{0,Section->PlanetVertexBuffer.Num()/2,Section->PlanetVertexBuffer.Num()-1})
                    {
                        const auto& V=Section->PlanetVertexBuffer[I];
                        const FVector P=Transform.TransformPosition(V.Position);
                        const double DepthKm=(double(R->OceanHeight)-R->GetGroundHeight(P,false))/100000.;
                        if (!FMath::IsFinite(DepthKm) || !FMath::IsFinite(V.UV1.X) || V.UV1.Y!=(Payload()?1.:0.))
                        { Error=TEXT("Regenerated ocean UV1 invalid; no snapshot fallback"); return false; }
                        if (Payload()) MaxErrorM=FMath::Max(MaxErrorM,FMath::Abs(DepthKm-V.UV1.X)*1000.);
                        Wet+=DepthKm>0; ++Vertices;
                        Signature=HashCombine(Signature,GetTypeHash(P));
                    }
                }
            }
            if (!Sections || !Vertices || MaxErrorM>.01)
            { Error=FString::Printf(TEXT("Water flight missing payload or depth error %.9g m"),MaxErrorM); return false; }
            Changes+=Samples>0 && Signature!=PreviousSignature; PreviousSignature=Signature; ++Samples;
#if defined(APS_COLLISION_HEIGHT_HOOK)
            int32 HeightOnlyPatches=0;
            for (const auto* Collision:R->CollisionLods)
                HeightOnlyPatches+=IsValid(Collision) && !Collision->CollisionHeightOnlyPositions.IsEmpty();
            PeakHeightOnlyCollisionPatches=FMath::Max(PeakHeightOnlyCollisionPatches,HeightOnlyPatches);
#endif
            UE_LOG(LogTemp,Display,TEXT("[APS.WaterFlight.Payload] sample=%d sections=%d vertices=%d wet=%d maxErrorM=%.9g signature=%u changes=%d workers=%d RAMMiB=%.2f"),
                Samples,Sections,Vertices,Wet,MaxErrorM,Signature,Changes,R->WorldScapeLodInGeneration.Num(),FPlatformMemory::GetStats().UsedPhysical/1048576.);
            return true;
        }
        void Start(double Now)
        {
            LastWall=Now; LastCounter=GFrameCounter; bMeasuring=true;
            Frames.Reserve(20000);
            TRACE_BEGIN_REGION(TEXT("APS_WaterFlight"));
        }
        void Tick(double Now, double T)
        {
            if (!bMeasuring || GFrameCounter==LastCounter) return;
            if (WaterPass() && (!Root.IsValid()
                || !APSWaterSurfaceLighting::Bind(Candidate.Get(), Root->GetWorld()))) bLightingFailed=true;
            const uint32 GT=GGameThreadTime,RT=GRenderThreadTime,GPU=GGPUFrameTime;
            Frames.Add({GFrameCounter,GFrameCounter-LastCounter,T,FApp::GetDeltaTime()*1000.,(Now-LastWall)*1000.,
                GT?FPlatformTime::ToMilliseconds(GT):-1.,RT?FPlatformTime::ToMilliseconds(RT):-1.,GPU?FPlatformTime::ToMilliseconds(GPU):-1.,
                Root.IsValid()?Root->WorldScapeLodInGeneration.Num():-1});
            LastWall=Now; LastCounter=GFrameCounter;
        }
        bool Finish(FString& Error)
        {
            if (bLightingFailed) { Error=TEXT("Water flight encountered an unsupported light state"); return false; }
            if (bMeasuring) { TRACE_END_REGION(TEXT("APS_WaterFlight")); bMeasuring=false; }
            FString Csv(TEXT("frame,gap,t,dt_ms,wall_ms,game_ms,render_ms,gpu_ms,workers\n"));
            for (const auto& F:Frames) Csv+=FString::Printf(TEXT("%llu,%llu,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f,%d\n"),F.Frame,F.Gap,F.T,F.Dt,F.Wall,F.Game,F.Render,F.GPU,F.Workers);
            const FString Path=FPaths::Combine(FPaths::ProjectSavedDir(),TEXT("Diagnostics/WaterFlight.csv"));
            IFileManager::Get().MakeDirectory(*FPaths::GetPath(Path),true);
            if (IFileManager::Get().FileExists(*Path) || !FFileHelper::SaveStringToFile(Csv,*Path,FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM))
            { Error=TEXT("Water flight evidence exists or cannot be saved"); return false; }
            if (Frames.Num()<100 || Changes<2) { Error=TEXT("Water flight did not exercise live regeneration"); return false; }
#if defined(APS_COLLISION_HEIGHT_HOOK)
            if (auto* HeightOnly=IConsoleManager::Get().FindConsoleVariable(TEXT("worldscape.CollisionHeightOnly"));
                HeightOnly && HeightOnly->GetInt()!=0 && PeakHeightOnlyCollisionPatches==0)
            { Error=TEXT("Height-only test switch enabled but no native collision patch used it"); return false; }
            UE_LOG(LogTemp,Display,TEXT("[APS.WaterFlight.Collision] peakHeightOnlyPatches=%d; actual sampler binding, NOT physical ship contact acceptance"),PeakHeightOnlyCollisionPatches);
#endif
            UE_LOG(LogTemp,Display,TEXT("[APS.WaterFlight] finished frames=%d changedSamples=%d perf=%d csv=%s; scripted observer route, NOT physical ship/residency acceptance"),Frames.Num(),Changes,Performance(),*Path);
            // Outside the measured region. Let queued render consumers release
            // their prepared-buffer leases before reporting native accounting.
            if (auto* Object=IConsoleManager::Get().FindConsoleObject(TEXT("worldscape.PreparedMesh.Dump")))
                if (auto* Command=Object->AsCommand())
                {
                    FlushRenderingCommands();
                    Command->Execute(TArray<FString>{},Root.IsValid()?Root->GetWorld():nullptr,*GLog);
                }
            return true;
        }
        void Restore()
        {
            if (bMeasuring) { TRACE_END_REGION(TEXT("APS_WaterFlight")); bMeasuring=false; }
            if (auto* R=Root.Get(); R && Candidate.IsValid() && R->OceanMaterial.DefaultMaterial==Candidate.Get())
                R->UpdateOceanMaterial(Original);
            Root.Reset(); Candidate.Reset(); Native.Reset();
        }
    };
}
#endif
