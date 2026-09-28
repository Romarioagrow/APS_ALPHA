#pragma once
#if WITH_DEV_AUTOMATION_TESTS
#include "APS_ALPHA/Actors/Astro/Planet.h"
#include "APS_ALPHA/Generation/APSWorldScapePlanetNoise.h"
#include "APS_ALPHA/Generation/PlanetarySurfaceGenerator.h"
#include "APS_ALPHA/Core/Planetary/APSSharedGeneratedLiquidMaterial.h"
#include "Async/ParallelFor.h"
#include "Camera/CameraComponent.h"
#include "ProceduralMeshComponent.h"
#include "APSProductionSharedLiquidAssertions.h"
#include "ContentStreaming.h"
#include "Engine/Texture2D.h"
#include "SceneView.h"
#include "SceneViewExtension.h"

// Controlled PLANET A/B only. Uses the real retained globe and a transient UV1
// payload; no WorldScape worker/ABI or production material binding is changed.
namespace APSWaterDepthRendered
{
inline bool Enabled() { return FParse::Param(FCommandLine::Get(), TEXT("APSProbeWaterDepth")); }
inline bool Filtered() { return FParse::Param(FCommandLine::Get(), TEXT("APSWaterDepthFiltered")); }
inline bool ResolveFamily(const FString& Name, EPlanetType& Type)
{
    const TPair<const TCHAR*, EPlanetType> Families[] = {
        {TEXT("Ocean"), EPlanetType::Ocean}, {TEXT("Water"), EPlanetType::Water},
        {TEXT("Terrestrial"), EPlanetType::Terrestrial}, {TEXT("Forest"), EPlanetType::Forest},
        {TEXT("Oasis"), EPlanetType::Oasis}, {TEXT("Savanna"), EPlanetType::Savanna}};
    for (const auto& Family : Families)
        if (Name == Family.Key) { Type = Family.Value; return true; }
    return false;
}
enum class EResult { Pending, Finished, Failed };
// Observe the final blended game view, not just the camera component defaults.
// This does not override exposure, streaming, lights or any production setting.
class FViewEvidence final : public FWorldSceneViewExtension
{
public:
    FViewEvidence(const FAutoRegister& Register, UWorld* World)
        : FWorldSceneViewExtension(Register, World) {}
    virtual void SetupViewFamily(FSceneViewFamily&) override {}
    virtual void BeginRenderViewFamily(FSceneViewFamily&) override {}
    virtual void SetupView(FSceneViewFamily& Family, FSceneView& View) override
    {
        if (View.bIsSceneCapture) return;
        PP = View.FinalPostProcessSettings;
        Location = View.ViewLocation; Rotation = View.ViewRotation;
        Frame = GFrameCounter;
        bEye = Family.EngineShowFlags.EyeAdaptation;
        bLocal = Family.EngineShowFlags.LocalExposure;
        bCut = View.bCameraCut;
    }
    FPostProcessSettings PP;
    FVector Location = FVector::ZeroVector;
    FRotator Rotation = FRotator::ZeroRotator;
    uint64 Frame = 0;
    bool bEye = false, bLocal = false, bCut = false;
};
class FProbe
{
    TWeakObjectPtr<AAstroGenerator> Generator;
    TWeakObjectPtr<UProceduralMeshComponent> Ocean;
    TStrongObjectPtr<UMaterialInterface> Original{nullptr};
    TStrongObjectPtr<UMaterialInstanceDynamic> Candidate{nullptr};
    TArray<FVector2D> OriginalUV1;
    uint32 GeometryCRC = 0;
    bool bSavedTick = false, bSceneSaved = false, bCompileRequested = false;
    int32 Step = 0;
    double Next = 0, Scale = 0;
    float Strength = 0.65f, HalfDepthM = 80.0f;
    EPlanetType ExpectedType = EPlanetType::Ocean;
    bool bFamilyValid = false;
    FTransform PairCamera, PairMesh;
    TSharedPtr<FViewEvidence, ESPMode::ThreadSafe> ViewEvidence;

    bool Trace(const TCHAR* Phase, FString& Error) const
    {
        FString Baseline;
        if (!APSProductionSharedLiquidAssertions::Validate(
            Cast<UMaterialInstanceDynamic>(Original.Get()), EAPSPlanetLiquidType::Water,
            Ocean.Get(), Scale, 1.0f, Baseline, Error)) return false;
        if (!ViewEvidence || !ViewEvidence->Frame || GFrameCounter - ViewEvidence->Frame > 3)
        { Error = TEXT("No recent actual game view for optical comparison"); return false; }
        const auto& V = *ViewEvidence;
        const auto& P = V.PP;
        UE_LOG(LogTemp, Display, TEXT("[APS.WaterDepthView] phase=%s age=%llu location=%s rotation=%s cut=%d exposure=%d min=%.9g max=%.9g bias=%.9g physical=%d eye=%d local=%d localHighlight=%.9g localShadow=%.9g localGrey=%.9g bloom=%.9g wanting=%d baselineFrameValid=1"),
            Phase, GFrameCounter - V.Frame, *V.Location.ToString(), *V.Rotation.ToString(), int(V.bCut),
            int(P.AutoExposureMethod), P.AutoExposureMinBrightness, P.AutoExposureMaxBrightness,
            P.AutoExposureBias, int(P.AutoExposureApplyPhysicalCameraExposure), int(V.bEye), int(V.bLocal),
            P.LocalExposureHighlightContrastScale, P.LocalExposureShadowContrastScale,
            P.LocalExposureMiddleGreyBias, P.BloomIntensity, IStreamingManager::Get().GetNumWantingResources());
        TArray<UTexture*> Textures;
        Ocean->GetMaterial(0)->GetUsedTextures(Textures, EMaterialQualityLevel::High, true,
            Ocean->GetWorld()->GetFeatureLevel(), false);
        for (UTexture* Texture : Textures)
            if (const UTexture2D* T = Cast<UTexture2D>(Texture))
                UE_LOG(LogTemp, Display, TEXT("[APS.WaterDepthTexture] phase=%s texture=%s resident=%d"),
                    Phase, *T->GetPathName(), T->GetNumResidentMips());
        return true;
    }

    static uint32 CRC(const FProcMeshSection& Section)
    {
        uint32 Result = FCrc::MemCrc32(Section.ProcIndexBuffer.GetData(), Section.ProcIndexBuffer.Num() * sizeof(uint32));
        for (const FProcMeshVertex& V : Section.ProcVertexBuffer)
        {
            Result = FCrc::MemCrc32(&V.Position, sizeof(V.Position), Result);
            Result = FCrc::MemCrc32(&V.Normal, sizeof(V.Normal), Result);
            Result = FCrc::MemCrc32(&V.Color, sizeof(V.Color), Result);
            Result = FCrc::MemCrc32(&V.UV0, sizeof(V.UV0), Result);
            Result = FCrc::MemCrc32(&V.UV2, sizeof(V.UV2), Result);
            Result = FCrc::MemCrc32(&V.UV3, sizeof(V.UV3), Result);
            Result = FCrc::MemCrc32(&V.Tangent.TangentX, sizeof(V.Tangent.TangentX), Result);
            Result = FCrc::MemCrc32(&V.Tangent.bFlipTangentY, sizeof(V.Tangent.bFlipTangentY), Result);
        }
        return Result;
    }
    bool SamePair() const
    {
        return Generator.IsValid() && Ocean.IsValid() && Generator->GetPreviewCameraComponent()
            && PairCamera.Equals(Generator->GetPreviewCameraComponent()->GetComponentTransform(), 1.e-6)
            && PairMesh.Equals(Ocean->GetComponentTransform(), 1.e-6);
    }
    void RememberPair()
    {
        PairCamera = Generator->GetPreviewCameraComponent()->GetComponentTransform();
        PairMesh = Ocean->GetComponentTransform();
    }
    bool ApplyCandidate()
    {
        if (!APSSharedTerrainMaterial::WritePhysicalFrame(Candidate.Get(), Ocean.Get(), Scale)) return false;
        Candidate->SetScalarParameterValue(TEXT("APS_UsePresentationWaterMask"), 1.0f);
        Candidate->SetScalarParameterValue(TEXT("APS_WaterDepthStrength"), Strength);
        Candidate->SetScalarParameterValue(TEXT("APS_WaterHalfDepthM"), HalfDepthM);
        Ocean->SetMaterial(0, Candidate.Get());
        return true;
    }
public:
    FProbe()
    {
        FParse::Value(FCommandLine::Get(), TEXT("APSWaterDepthStrength="), Strength);
        FParse::Value(FCommandLine::Get(), TEXT("APSWaterHalfDepthM="), HalfDepthM);
        FString Family = TEXT("Ocean");
        FParse::Value(FCommandLine::Get(), TEXT("APSPlanetProbeFamily="), Family);
        bFamilyValid = ResolveFamily(Family, ExpectedType);
    }
    ~FProbe() { Restore(); }
    void Restore()
    {
        if (bSceneSaved && Ocean.IsValid())
        {
            Ocean->SetMaterial(0, Original.Get());
            if (FProcMeshSection* S = Ocean->GetProcMeshSection(0);
                S && S->ProcVertexBuffer.Num() == OriginalUV1.Num() && CRC(*S) == GeometryCRC)
            {
                for (int32 I = 0; I < OriginalUV1.Num(); ++I) S->ProcVertexBuffer[I].UV1 = OriginalUV1[I];
                Ocean->MarkRenderStateDirty();
            }
        }
        if (bSceneSaved && Generator.IsValid()) Generator->SetActorTickEnabled(bSavedTick);
        ViewEvidence.Reset();
        bSceneSaved = false;
    }
    EResult Update(AAstroGenerator* InGenerator, UWorldGenerationViewModel* VM, float Zoom,
        TFunctionRef<void(const TCHAR*)> Capture, FString& Error)
    {
        auto Fail = [&](const TCHAR* Why) { Error = Why; Restore(); return EResult::Failed; };
        const double Now = FPlatformTime::Seconds();
        if (Now < Next) return EResult::Pending;
        if (!InGenerator || !VM) return Fail(TEXT("Missing actual preview context"));
        if (!bFamilyValid) return Fail(TEXT("Unsupported explicit Water depth family"));
        if (!FMath::IsFinite(Strength) || Strength < 0.0f || Strength > 1.0f
            || !FMath::IsFinite(HalfDepthM) || HalfDepthM < 1.0f || HalfDepthM > 500.0f)
            return Fail(TEXT("Invalid bounded optical controls"));
        if (!Ocean.IsValid())
        {
            Generator = InGenerator;
            Ocean = InGenerator->GetActivePreviewOceanProxy();
            APlanet* Body = Cast<APlanet>(InGenerator->GetActivePreviewWorldScapeBody());
            if (!Body || Body->IsManual || Body->PlanetType != ExpectedType
                || !Ocean.IsValid() || !InGenerator->GetPreviewCameraComponent())
                return Fail(TEXT("Expected generated Water PLANET mesh"));
            Scale = Body->WorldScapePresentationScale;
            if (!FMath::IsFinite(Scale) || Scale <= 0.0) return Fail(TEXT("Invalid actual presentation scale"));
            Original.Reset(Ocean->GetMaterial(0));
            if (!APSSharedGeneratedLiquidMaterial::HasSavedParameterAuthority(Original.Get(), EAPSPlanetLiquidType::Water))
                return Fail(TEXT("Actual baseline is not the accepted Water family"));
            const FString Folder = Filtered()
                ? TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/Diagnostics/WaterDepthFiltered20260927/")
                : TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/Diagnostics/WaterDepth20260927/");
            auto* Template = LoadObject<UMaterialInstance>(nullptr, *(Folder + TEXT("MI_APS_WaterDepth.MI_APS_WaterDepth")));
            if (!Template || !Template->GetMaterial() || Template->GetMaterial()->GetPathName() !=
                Folder + TEXT("M_APS_WaterDepth.M_APS_WaterDepth"))
                return Fail(TEXT("Exact saved depth candidate absent"));
            Candidate.Reset(UMaterialInstanceDynamic::Create(Template, Ocean.Get()));
            UE_LOG(LogTemp, Display, TEXT("[APS.WaterDepthCandidate] filtered=%d template=%s"), int(Filtered()), *Template->GetPathName());
            ViewEvidence = FSceneViewExtensions::NewExtension<FViewEvidence>(Ocean->GetWorld());
        }
        if (InGenerator != Generator.Get() || InGenerator->GetActivePreviewOceanProxy() != Ocean.Get())
            return Fail(TEXT("Preview rebuilt/replaced during depth comparison"));
        if (!Candidate.IsValid()) return Fail(TEXT("Depth MID unavailable"));
        if (!bSceneSaved)
        {
            FMaterialResource* R = Candidate->GetMaterialResource(Ocean->GetWorld()->GetFeatureLevel());
            if (!R || R->GetCompileErrors().Num()) return Fail(TEXT("Depth shader resource missing/failed"));
            if (!R->IsGameThreadShaderMapComplete())
            {
                if (!bCompileRequested) { R->SubmitCompileJobs_GameThread(EShaderCompileJobPriority::ForceLocal); bCompileRequested = true; }
                Next = Now + 0.25; return EResult::Pending;
            }
            if (!R->GetGameThreadShaderMap() || !R->GetGameThreadShaderMap()->GetMeshShaderMap(&FLocalVertexFactory::StaticType))
                return Fail(TEXT("Depth LocalVF missing"));
            APlanet* Body = Cast<APlanet>(Generator->GetActivePreviewWorldScapeBody());
            APlanetarySurfaceGenerator* Resolver = nullptr;
            for (TActorIterator<APlanetarySurfaceGenerator> It(Ocean->GetWorld()); It; ++It)
                if (It->GetOwner() == Generator.Get() && It->IsSurfaceProfileCurrent(Body))
                { if (Resolver) return Fail(TEXT("Ambiguous surface resolver")); Resolver = *It; }
            if (!Resolver || !Resolver->WorldScapeRootInstance
                || Resolver->ResolvedSurfaceProfile.LiquidType != EAPSPlanetLiquidType::Water
                || Resolver->ResolvedSurfaceProfile.PlanetType != ExpectedType
                || !Resolver->WorldScapeRootInstance->bOcean)
                return Fail(TEXT("Missing actual Water resolver"));
            const auto* Root = Resolver->WorldScapeRootInstance;
            if (!Cast<UAPSWorldScapePlanetNoise>(Root->WorldScapeNoise)) return Fail(TEXT("Unsupported generator"));
            FProcMeshSection* Section = Ocean->GetProcMeshSection(0);
            if (!Section || Section->ProcVertexBuffer.IsEmpty()) return Fail(TEXT("Missing retained water geometry"));
            const auto Profile = Resolver->ResolvedSurfaceProfile;
            UE_LOG(LogTemp, Display, TEXT("[APS.WaterDepthProfile] type=%d actualType=%d liquid=%d landCoverage=%.9g resolver=%s"),
                int(ExpectedType), int(Body->PlanetType), int(Profile.LiquidType), Profile.LandCoverage, *Resolver->GetPathName());
            // Reproduce the full-scale root's float property conversion from the
            // physical body, not a division of an already rounded preview radius.
            const double Radius = static_cast<float>(FMath::Max(100000.0,
                FMath::Max(Body->RadiusKM, double(Body->PlanetRadiusKM)) * 100000.0));
            const double Intensity = FMath::Max(1.0, double(FMath::RoundToInt(Profile.NoiseIntensity)));
            const double NoiseScale = Root->NoiseScale;
            const double Sea = FMath::Max(Profile.OceanLevel * double(Profile.NoiseIntensity), Profile.OceanLevel * Intensity);
            if (!FMath::IsFinite(Radius) || Radius <= 0.0) return Fail(TEXT("Invalid physical reference radius"));
            const CustomNoise Seeded = Root->PlanetNoise;
            const UAPSWorldScapePlanetNoise* ResolvedNoise = Cast<UAPSWorldScapePlanetNoise>(Root->WorldScapeNoise);
            if (!ResolvedNoise) return Fail(TEXT("Physical depth requires the owning resolved field"));
            const bool bCoastalReliefCandidate = ResolvedNoise->UsesCoastalReliefCandidate();
            TArray<FVector> Directions;
            for (const auto& V : Section->ProcVertexBuffer) Directions.Add(V.Position.GetSafeNormal());
            TArray<double> Depths; Depths.SetNumUninitialized(Directions.Num());
            const double WorkStart = FPlatformTime::Seconds();
            constexpr int32 BatchSize = 1024;
            ParallelFor(FMath::DivideAndRoundUp(Directions.Num(), BatchSize), [&](int32 Batch)
            {
                CustomNoise Noise = Seeded;
                for (int32 I = Batch * BatchSize; I < FMath::Min((Batch + 1) * BatchSize, Directions.Num()); ++I)
                {
                    const FVector D = Directions[I];
                    const double Height = UAPSWorldScapePlanetNoise::SampleHeightResolvedProfile(
                        Profile, Noise, DVector(D * Radius), DVector(0.0), NoiseScale, Intensity, Radius, D.Z,
                        bCoastalReliefCandidate);
                    Depths[I] = (Sea - Height) / 100000.0;
                }
            });
            double Minimum = TNumericLimits<double>::Max(), Maximum = -Minimum;
            for (double D : Depths)
            {
                if (!FMath::IsFinite(D) || FMath::Abs(D) > 65504.0) return Fail(TEXT("Invalid/FP16-overflow signed depth"));
                Minimum = FMath::Min(Minimum, D); Maximum = FMath::Max(Maximum, D);
            }
            GeometryCRC = CRC(*Section);
            bSavedTick = Generator->IsActorTickEnabled();
            bSceneSaved = true;
            Generator->SetActorTickEnabled(false);
            for (int32 I = 0; I < Depths.Num(); ++I)
            {
                OriginalUV1.Add(Section->ProcVertexBuffer[I].UV1);
                Section->ProcVertexBuffer[I].UV1 = FVector2D(Depths[I], 1.0);
            }
            Ocean->MarkRenderStateDirty();
            UE_LOG(LogTemp, Display, TEXT("[APS.WaterDepthRender] strength=%.6g halfDepthM=%.6g physicalReference=full-scale-root-property"), Strength, HalfDepthM);
            UE_LOG(LogTemp, Display, TEXT("[APS.WaterDepthRender] vertices=%d physicalRadiusCm=%.9g intensity=%.9g minKm=%.9g maxKm=%.9g sampleMs=%.3f crc=%u payload=UV1-only previewGeometryUnchanged=1 productionBinding=0"),
                Depths.Num(), Radius, Intensity, Minimum, Maximum, (FPlatformTime::Seconds() - WorkStart) * 1000.0, GeometryCRC);
            Next = Now + 6; Step = 1; return EResult::Pending;
        }
        FProcMeshSection* Section = Ocean->GetProcMeshSection(0);
        if (!Section || CRC(*Section) != GeometryCRC) return Fail(TEXT("Geometry/RGBA/other UV channels changed during A/B"));
        if (Step == 1)
        {
            if (!Trace(TEXT("00-orbit-accepted"), Error)) { Restore(); return EResult::Failed; }
            RememberPair(); Capture(TEXT("00-orbit-accepted"));
            if (!ApplyCandidate()) return Fail(TEXT("Candidate frame failed"));
        }
        else if (Step == 2)
        {
            if (!SamePair()) return Fail(TEXT("Orbit camera/mesh changed between A/B"));
            if (!Trace(TEXT("01-orbit-depth"), Error)) { Restore(); return EResult::Failed; }
            Capture(TEXT("01-orbit-depth"));
            Ocean->SetMaterial(0, Original.Get());
            Generator->SetActorTickEnabled(bSavedTick);
            VM->ZoomPreview(Zoom); VM->BeginPreviewOrbit(); VM->OrbitPreview(FVector2D(240, 50)); VM->EndPreviewOrbit();
            Step = 3; Next = Now + 4; return EResult::Pending;
        }
        else if (Step == 3)
        {
            Generator->SetActorTickEnabled(false); RememberPair();
            // Camera tick being disabled is not evidence that its final rendered
            // view/history has settled. Add original-only controls on both sides.
            Step = 31; Next = Now + 2; return EResult::Pending;
        }
        else if (Step == 31)
        {
            if (!SamePair()) return Fail(TEXT("Close settling changed camera/mesh"));
            if (!Trace(TEXT("02-close-accepted"), Error)) { Restore(); return EResult::Failed; }
            Capture(TEXT("02-close-accepted"));
            Step = 32; Next = Now + 2; return EResult::Pending;
        }
        else if (Step == 32)
        {
            if (!SamePair()) return Fail(TEXT("Repeated original frame changed camera/mesh"));
            if (!Trace(TEXT("02b-close-accepted-repeat"), Error)) { Restore(); return EResult::Failed; }
            Capture(TEXT("02b-close-accepted-repeat"));
            if (!ApplyCandidate()) return Fail(TEXT("Close candidate frame failed"));
            Step = 4; Next = Now + 2; return EResult::Pending;
        }
        else if (Step == 4)
        {
            if (!SamePair()) return Fail(TEXT("Close camera/mesh changed between A/B"));
            if (!Trace(TEXT("03-close-depth"), Error)) { Restore(); return EResult::Failed; }
            Capture(TEXT("03-close-depth")); Candidate->SetScalarParameterValue(TEXT("APS_WaterDepthStrength"), 0.0f);
        }
        else if (Step == 5)
        {
            if (!SamePair()) return Fail(TEXT("Close fallback frame changed"));
            if (!Trace(TEXT("04-close-strength-zero"), Error)) { Restore(); return EResult::Failed; }
            Capture(TEXT("04-close-strength-zero")); Candidate->SetScalarParameterValue(TEXT("APS_WaterDepthStrength"), Strength);
            for (auto& V : Section->ProcVertexBuffer) V.UV1.Y = 0.0;
            Ocean->MarkRenderStateDirty();
        }
        else if (Step == 6)
        {
            if (!SamePair()) return Fail(TEXT("Invalid payload fallback frame changed"));
            if (!Trace(TEXT("05-close-invalid-payload"), Error)) { Restore(); return EResult::Failed; }
            Capture(TEXT("05-close-invalid-payload"));
            Ocean->SetMaterial(0, Original.Get());
        }
        else
        {
            if (!SamePair()) return Fail(TEXT("Original-return frame changed camera/mesh"));
            if (!Trace(TEXT("06-close-accepted-return"), Error)) { Restore(); return EResult::Failed; }
            Capture(TEXT("06-close-accepted-return")); Restore();
            UE_LOG(LogTemp, Display, TEXT("[APS.WaterDepthRender] comparisons=orbit,close,original-repeat,zero-strength,invalid-payload,original-return restored=1 visualAcceptance=UNVERIFIED gameplayCoverage=0"));
            return EResult::Finished;
        }
        ++Step; Next = Now + 2; return EResult::Pending;
    }
};
}
#endif
