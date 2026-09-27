#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Tests/AutomationCommon.h"
#include "APS_ALPHA/Actors/Astro/PlanetaryBody.h"
#include "APS_ALPHA/Actors/Astro/Star.h"
#include "APS_ALPHA/Core/Controllers/MainMenuController.h"
#include "APS_ALPHA/Core/Model/GeneratedWorld.h"
#include "APS_ALPHA/Generation/AstroGenerator.h"
#include "APS_ALPHA/UI/MainMenu/WorldGenerationViewModel.h"
#include "APS_ALPHA/Core/Planetary/APSSharedLavaMaterial.h"
#include "APS_ALPHA/Core/Planetary/APSSharedAmmoniaMaterial.h"
#include "APSLavaTextureDiagnostics.h"
#include "APSLavaSamplingABProbe.h"
#include "APSPlanetTerrainLodABProbe.h"
#include "APSProductionSharedLiquidAssertions.h"
#include "Camera/PlayerCameraManager.h"
#include "Misc/Crc.h"
#include "APS_ALPHA/Core/Planetary/APSSharedWaterMaterial.h"
#include "APS_ALPHA/Actors/Astro/Planet.h"
#include "APS_ALPHA/Generation/PlanetarySurfaceGenerator.h"
#include "PlanetaryAtmosphere.h"
#include "Components/LightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/GameViewportClient.h"
#include "EngineUtils.h"
#include "Framework/Application/SlateApplication.h"
#include "Widgets/SViewport.h"
#include "HAL/FileManager.h"
#include "ImageUtils.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "ProceduralMeshComponent.h"
#include "LocalVertexFactory.h"
#include "MaterialShared.h"
#include "UObject/StrongObjectPtr.h"

// Test-only changes are transient and restored. This is a causal A/B fixture,
// not a claim that any screenshot is visually accepted.
namespace APSPlanetRefinement
{
class FProbe final : public IAutomationLatentCommand
{
    FAutomationTestBase* Test;
    double Start = FPlatformTime::Seconds(), Next = 0, LastPendingDiagnostic = -1000;
    int32 Step = 0;
    FString Family = TEXT("Ocean");
    float Zoom = 0.5f;
    float AtmosphereOpacity = 1.0f;
    bool bTerrainLodAB = false, bTerrainFarNormalAB = false;
    APSPlanetTerrainLodAB::FProbe TerrainLodProbe;
    TWeakObjectPtr<UMaterialInterface> ProductionLiquidParent;
    TWeakObjectPtr<AAstroGenerator> Generator;
    TArray<TPair<TWeakObjectPtr<AAtmoScape>, bool>> Atmospheres;
    TArray<TPair<TWeakObjectPtr<ULightComponent>, bool>> Shadows;
    TArray<TPair<TWeakObjectPtr<UPrimitiveComponent>, bool>> Casters;
    TArray<FColor> SavedColors;
    bool bTerrainVisible = false, bGeneratorTick = false;
    bool bSharedLavaCandidate = false, bCandidateApplied = false, bSceneStateSaved = false;
    bool bCandidateCompileRequested = false;
    bool bSharedAmmoniaCandidate = false;
    bool bSharedWaterCandidate = false;
    bool bLavaEmissionAB = false, bOceanVisible = false, bEmissionStateSaved = false;
    bool bLavaSamplingAB = false, bSamplingFrameSaved = false;
    bool bLavaBandwidthLOD = false;
    bool bWaterOpacityAB = false, bWaterOpacitySaved = false;
    float OriginalWaterOpacity = 0.0f;
    FLinearColor WaterDeepBefore, WaterShallowBefore;
    FTransform SamplingOceanFrame;
    FVector SamplingCameraLocation = FVector::ZeroVector;
    FRotator SamplingCameraRotation = FRotator::ZeroRotator;
    uint32 SamplingVertexCRC = 0, SamplingIndexCRC = 0;
    float OriginalCandidateBrightness = 0.0f;
    FLinearColor OriginalCandidateEmission = FLinearColor::Black;
    bool HasSharedCandidate() const { return bSharedLavaCandidate || bSharedAmmoniaCandidate || bSharedWaterCandidate; }
    TWeakObjectPtr<UProceduralMeshComponent> CandidateOcean;
    TStrongObjectPtr<UMaterialInterface> OriginalOceanMaterial{nullptr};
    TStrongObjectPtr<UMaterialInstanceDynamic> CandidateMaterial{nullptr};

    void DiagnosePending(const TCHAR* Reason, UWorld* World, UWorldGenerationViewModel* VM, bool bForce = false)
    {
        const double Now = FPlatformTime::Seconds();
        if (!HasSharedCandidate() || (!bForce && Now - LastPendingDiagnostic < 5.0)) return;
        LastPendingDiagnostic = Now;
        UProceduralMeshComponent* Ocean = Generator.IsValid() ? Generator->GetActivePreviewOceanProxy() : nullptr;
        UMaterialInterface* Material = CandidateMaterial.IsValid() ? CandidateMaterial.Get() : Ocean ? Ocean->GetMaterial(0) : nullptr;
        FMaterialResource* Resource = Material && World ? Material->GetMaterialResource(World->GetFeatureLevel()) : nullptr;
        FMaterialShaderMap* Map = Resource ? Resource->GetGameThreadShaderMap() : nullptr;
        Test->AddInfo(FString::Printf(TEXT("%s reason=%s elapsed=%.2f frame=%llu step=%d world=%s worldType=%d featureLevel=%d vm=%d previewReady=%d generator=%s queueDrained=%d ocean=%s candidateCreated=%d applied=%d material=%s resource=%d map=%d complete=%d errors=%d localVF=%d"),
            bSharedWaterCandidate ? TEXT("PLANET_SHARED_WATER_PENDING") :
                bSharedAmmoniaCandidate ? TEXT("PLANET_SHARED_AMMONIA_PENDING") : TEXT("PLANET_SHARED_LAVA_PENDING"),
            Reason, Now - Start, static_cast<unsigned long long>(GFrameCounter), Step, *GetPathNameSafe(World),
            World ? static_cast<int32>(World->WorldType) : -1, World ? static_cast<int32>(World->GetFeatureLevel()) : -1,
            VM ? 1 : 0, VM && VM->bPreviewReady ? 1 : 0, *GetPathNameSafe(Generator.Get()),
            Generator.IsValid() ? (Generator->IsPreviewGlobeFamilyWarmQueueDrained() ? 1 : 0) : -1,
            *GetPathNameSafe(Ocean), CandidateMaterial.IsValid() ? 1 : 0, bCandidateApplied ? 1 : 0,
            *GetPathNameSafe(Material), Resource ? 1 : 0, Map ? 1 : 0,
            Resource && Resource->IsGameThreadShaderMapComplete() ? 1 : 0,
            Resource ? Resource->GetCompileErrors().Num() : -1,
            Map && Map->GetMeshShaderMap(&FLocalVertexFactory::StaticType) ? 1 : 0));
    }

    enum class ECandidateResult { Pending, Ready, Failed };
    ECandidateResult PrepareCandidate(UProceduralMeshComponent* Terrain, UProceduralMeshComponent* Ocean)
    {
        const auto Fail = [this](const TCHAR* Why)
        { Test->AddError(FString(TEXT("Shared liquid candidate: ")) + Why); return ECandidateResult::Failed; };
        if (!IsValid(Terrain) || !IsValid(Ocean) || !Ocean->GetProcMeshSection(0)
            || Ocean->GetProcMeshSection(0)->ProcIndexBuffer.Num() < 3)
            return Fail(TEXT("actual PLANET ocean geometry is absent"));
        auto* TerrainMID = Cast<UMaterialInstanceDynamic>(Terrain->GetMaterial(0));
        if (!APSSharedTerrainMaterial::IsSharedStack(TerrainMID))
            return Fail(TEXT("actual terrain does not expose the shared physical frame"));
        FVector4 InverseScale;
        if (!TerrainMID->GetDoubleVectorParameterValue(FHashedMaterialParameterInfo(TEXT("APS_SharedInverseScale")), InverseScale)
            || !FMath::IsFinite(InverseScale.X) || InverseScale.X <= 0.0)
            return Fail(TEXT("terrain physical inverse scale is invalid"));
        const FVector TerrainScale = Terrain->GetComponentScale();
        const FVector OceanScale = Ocean->GetComponentScale();
        const double UniformScale = TerrainScale.GetAbsMax();
        if (TerrainScale.ContainsNaN() || OceanScale.ContainsNaN() || !FMath::IsFinite(UniformScale)
            || TerrainScale.X <= 0 || TerrainScale.Y <= 0 || TerrainScale.Z <= 0
            || UniformScale - TerrainScale.GetAbsMin() > UniformScale * 1.e-5
            || !TerrainScale.Equals(OceanScale, UniformScale * 1.e-5)
            || !Terrain->GetComponentLocation().Equals(Ocean->GetComponentLocation(), 0.001)
            || !Terrain->GetComponentQuat().Equals(Ocean->GetComponentQuat(), 1.e-7))
            return Fail(TEXT("actual ocean and terrain do not share a positive uniform pose/scale"));
        // inverse = 1 / (local-vertex presentation scale * live component scale).
        // Do not mistake the focus/zoom scale or physical radius for profile scale.
        const double PresentationScale = (1.0 / InverseScale.X) / UniformScale;
        if (!FMath::IsFinite(PresentationScale) || PresentationScale <= 0.0)
            return Fail(TEXT("derived physical presentation scale is invalid"));
        FVector4 TerrainCenter;
        if (!TerrainMID->GetDoubleVectorParameterValue(FHashedMaterialParameterInfo(TEXT("APS_SharedPlanetCenter")), TerrainCenter)
            || !FVector(TerrainCenter.X, TerrainCenter.Y, TerrainCenter.Z).Equals(Ocean->GetComponentLocation(), 0.001))
            return Fail(TEXT("terrain material center is stale or differs from actual ocean center"));

        if (!CandidateMaterial.IsValid())
        {
            UMaterialInstanceDynamic* NewMaterial = nullptr;
            if (bLavaSamplingAB || bLavaBandwidthLOD)
                NewMaterial = APSLavaSamplingABProbe::Create(Ocean, PresentationScale);
            else if (bSharedWaterCandidate)
            {
                APlanet* Body = Cast<APlanet>(Generator->GetActivePreviewWorldScapeBody());
                const EPlanetType ExpectedType = Family == TEXT("Water") ? EPlanetType::Water : EPlanetType::Ocean;
                if (!IsValid(Body) || Body->IsManual || Body->PlanetType != ExpectedType)
                    return Fail(TEXT("Water probe requires the actual generated Ocean/Water planet"));
                // The body-owned gameplay generator is not the menu authority.
                // Read the persistent preview resolver after its family queue drains.
                APlanetarySurfaceGenerator* Resolver = nullptr;
                for (TActorIterator<APlanetarySurfaceGenerator> It(Ocean->GetWorld()); It; ++It)
                {
                    if (It->GetOwner() != Generator.Get() || !It->IsSurfaceProfileCurrent(Body)) continue;
                    if (Resolver) return Fail(TEXT("ambiguous actual preview Water profile resolver"));
                    Resolver = *It;
                }
                if (!Resolver || Resolver->ResolvedSurfaceProfile.PlanetType != ExpectedType
                    || !IsValid(Resolver->WorldScapeRootInstance)
                    || !APSSharedWaterMaterial::AllowsProfile(Resolver->ResolvedSurfaceProfile,
                        Body->IsManual, Resolver->WorldScapeRootInstance->bOcean))
                    return Fail(TEXT("actual preview profile is not resolved enabled Water; no synthetic profile substitution"));
                NewMaterial = APSSharedWaterMaterial::CreateUnboundCandidate(
                    Ocean, Ocean, PresentationScale, true, Resolver->ResolvedSurfaceProfile,
                    Body->IsManual, Resolver->WorldScapeRootInstance->bOcean);
                Test->AddInfo(FString::Printf(TEXT("PLANET_SHARED_WATER_PROFILE body=%s type=%d liquid=%d landCoverage=%.9g resolver=%s oceanEnabled=%d"),
                    *Body->GetPathName(), int32(Body->PlanetType), int32(Resolver->ResolvedSurfaceProfile.LiquidType),
                    Resolver->ResolvedSurfaceProfile.LandCoverage, *Resolver->GetPathName(), Resolver->WorldScapeRootInstance->bOcean ? 1 : 0));
            }
            else
            {
                NewMaterial = bSharedAmmoniaCandidate
                    ? APSSharedAmmoniaMaterial::Create(Ocean, Ocean, PresentationScale, true, false, EAPSPlanetLiquidType::Ammonia)
                    : APSSharedLavaMaterial::Create(Ocean, Ocean, PresentationScale, true, false, EAPSPlanetLiquidType::Lava);
            }
            if (!NewMaterial) return Fail(TEXT("candidate asset/parent/frame is unavailable; no legacy substitution"));
            CandidateMaterial.Reset(NewMaterial);
        }
        UMaterialInstanceDynamic* Material = CandidateMaterial.Get();
        if (bWaterOpacityAB && !bWaterOpacitySaved)
        {
            if (!Material->GetScalarParameterValue(FHashedMaterialParameterInfo(TEXT("WaterSurfaceOpacity")), OriginalWaterOpacity)
                || !FMath::IsNearlyEqual(OriginalWaterOpacity, 0.78f)
                || !Material->GetVectorParameterValue(FHashedMaterialParameterInfo(TEXT("LiquidDeepColor")), WaterDeepBefore)
                || !Material->GetVectorParameterValue(FHashedMaterialParameterInfo(TEXT("LiquidShallowColor")), WaterShallowBefore))
                return Fail(TEXT("Water optical A/B needs readable exact .78 source state/palette"));
            bWaterOpacitySaved = true;
        }
        const bool bExactSharedStack = (bLavaSamplingAB || bLavaBandwidthLOD) ? APSLavaSamplingABProbe::IsStack(Material) : (bSharedAmmoniaCandidate || bSharedWaterCandidate)
            ? APSSharedAmmoniaMaterial::IsSharedStack(Material) : APSSharedLavaMaterial::IsSharedStack(Material);
        const TCHAR* ExactTemplate = (bLavaSamplingAB || bLavaBandwidthLOD) ? APSLavaSamplingABProbe::TemplatePath() : bSharedWaterCandidate ? APSSharedWaterMaterial::TemplatePath() :
            bSharedAmmoniaCandidate ? APSSharedAmmoniaMaterial::TemplatePath() : APSSharedLavaMaterial::TemplatePath();
        if (!bExactSharedStack
            || !IsValid(Material->Parent.Get())
            || Material->Parent->GetPathName() != ExactTemplate
            || Material->GetBlendMode() != BLEND_Masked)
            return Fail(TEXT("candidate is not the exact shared family MIC"));
        FMaterialResource* Resource = Material->GetMaterialResource(Ocean->GetWorld()->GetFeatureLevel());
        if (Resource && Resource->GetCompileErrors().Num() != 0)
            return Fail(TEXT("candidate has shader compilation errors"));
        if (Resource && !bCandidateCompileRequested)
        {
            // Loading an uncooked asset can leave a deferred partial map. Match
            // production preview warmup: request once, then wait asynchronously.
            if (!Resource->IsGameThreadShaderMapComplete())
                Resource->SubmitCompileJobs_GameThread(EShaderCompileJobPriority::ForceLocal);
            FPSOPrecacheParams Params;
            Ocean->SetupPrecachePSOParams(Params);
            Params.bStaticLighting = false;
            Params.bCastShadow = false;
            Params.SetMobility(EComponentMobility::Movable);
            static_cast<UMaterialInterface*>(Material)->PrecachePSOs(&FLocalVertexFactory::StaticType, Params);
            bCandidateCompileRequested = true;
        }
        FMaterialShaderMap* Map = Resource ? Resource->GetGameThreadShaderMap() : nullptr;
        if (!Resource || !Resource->IsGameThreadShaderMapComplete() || !Map
            || !Map->GetMeshShaderMap(&FLocalVertexFactory::StaticType))
            return ECandidateResult::Pending;
        if (bSharedWaterCandidate && !APSSharedWaterMaterial::IsReadyToPublish(Material, Ocean->GetWorld()->GetFeatureLevel()))
            return Fail(TEXT("exact Water candidate readiness contract failed after shader warmup"));
        FVector4 CandidateInverse;
        if (!Material->GetDoubleVectorParameterValue(FHashedMaterialParameterInfo(TEXT("APS_SharedInverseScale")), CandidateInverse)
            || FMath::Abs(CandidateInverse.X - InverseScale.X) > InverseScale.X * 1.e-9)
            return Fail(TEXT("candidate effective physical scale differs from terrain"));
        float CoverageContext = 0;
        if (!Material->GetScalarParameterValue(FHashedMaterialParameterInfo(TEXT("APS_UsePresentationWaterMask")), CoverageContext)
            || CoverageContext != 1.0f)
            return Fail(TEXT("PLANET candidate does not use the actual WaterMask"));

        if (bLavaEmissionAB || bLavaSamplingAB)
        {
            UMaterialInterface* Source = LoadObject<UMaterialInterface>(nullptr,
                TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/Materials/MI_APS_WS_Lava.MI_APS_WS_Lava"));
            float SourceBrightness = 0.0f;
            FLinearColor SourceEmission;
            if (!Source
                || !Source->GetScalarParameterValue(FHashedMaterialParameterInfo(TEXT("Brightness")), SourceBrightness)
                || !Material->GetScalarParameterValue(FHashedMaterialParameterInfo(TEXT("Brightness")), OriginalCandidateBrightness)
                || !Source->GetVectorParameterValue(FHashedMaterialParameterInfo(TEXT("EmissiveColor")), SourceEmission)
                || !Material->GetVectorParameterValue(FHashedMaterialParameterInfo(TEXT("EmissiveColor")), OriginalCandidateEmission)
                || !FMath::IsFinite(SourceBrightness) || SourceBrightness != OriginalCandidateBrightness
                || !SourceEmission.Equals(OriginalCandidateEmission))
                return Fail(TEXT("Lava emission audit requires matching readable source/candidate state"));
            bEmissionStateSaved = true;
            Test->AddInfo(FString::Printf(TEXT("PLANET_LAVA_EMISSION_SOURCE source=%s candidate=%s sourceBrightness=%.9g candidateBrightness=%.9g sourceColor=%s candidateColor=%s"),
                *Source->GetPathName(), *Material->GetPathName(), SourceBrightness, OriginalCandidateBrightness,
                *SourceEmission.ToString(), *OriginalCandidateEmission.ToString()));
            APSLavaTextureDiagnostics::Log(Test, Source);
            APSLavaTextureDiagnostics::Log(Test, Material);
        }
        OriginalOceanMaterial.Reset(Ocean->GetMaterial(0));
        CandidateOcean = Ocean;
        Ocean->SetMaterial(0, Material);
        bCandidateApplied = true;
        Test->AddInfo(FString::Printf(TEXT("%s exactParent=%s ocean=%s presentationScale=%.17g componentScale=%.17g inverseScale=%.17g complete=1 localVF=1 mask=1"),
            bSharedWaterCandidate ? TEXT("PLANET_SHARED_WATER") :
                bSharedAmmoniaCandidate ? TEXT("PLANET_SHARED_AMMONIA") : TEXT("PLANET_SHARED_LAVA"),
            *Material->Parent->GetPathName(), *Ocean->GetPathName(), PresentationScale, UniformScale, InverseScale.X));
        return ECandidateResult::Ready;
    }

    void RestoreCandidate()
    {
        if (bWaterOpacitySaved && CandidateMaterial.IsValid())
            CandidateMaterial->SetScalarParameterValue(TEXT("WaterSurfaceOpacity"), OriginalWaterOpacity);
        if (bLavaSamplingAB && CandidateMaterial.IsValid())
        {
            CandidateMaterial->SetScalarParameterValue(TEXT("APS_DebugUseVnoise1"), 1.0f);
            CandidateMaterial->SetScalarParameterValue(TEXT("APS_DebugUseWATCrust"), 1.0f);
            if (APSLavaSamplingABProbe::UsesScaleIsolation())
                for (const TCHAR* Name : {TEXT("APS_DebugUseWAT400m"), TEXT("APS_DebugUseWAT5431m"), TEXT("APS_DebugUseWAT20km")})
                    CandidateMaterial->SetScalarParameterValue(Name, 1.0f);
        }
        if (bEmissionStateSaved && CandidateMaterial.IsValid())
            CandidateMaterial->SetScalarParameterValue(TEXT("Brightness"), OriginalCandidateBrightness);
        // Existing Restore() calls reset causal scene changes between phases.
        // Keep the candidate through those phases; restore the original only
        // when the latent command finishes, including failure/timeout paths.
        if (bCandidateApplied && CandidateOcean.IsValid())
            CandidateOcean->SetMaterial(0, OriginalOceanMaterial.Get());
        bCandidateApplied = false;
    }

    bool SetProbeBrightness(float Value)
    {
        if (!bLavaEmissionAB || !bEmissionStateSaved || !CandidateMaterial.IsValid()) return false;
        CandidateMaterial->SetScalarParameterValue(TEXT("Brightness"), Value);
        float Readback = 0.0f;
        FLinearColor Emission;
        if (!CandidateMaterial->GetScalarParameterValue(FHashedMaterialParameterInfo(TEXT("Brightness")), Readback)
            || Readback != Value
            || !CandidateMaterial->GetVectorParameterValue(FHashedMaterialParameterInfo(TEXT("EmissiveColor")), Emission)
            || !Emission.Equals(OriginalCandidateEmission))
        { Test->AddError(TEXT("Lava emission A/B changed chemistry or failed scalar readback")); return false; }
        Test->AddInfo(FString::Printf(TEXT("PLANET_LAVA_EMISSION_AB ocean=%s material=%s brightness=%.9g color=%s"),
            *GetPathNameSafe(CandidateOcean.Get()), *CandidateMaterial->GetPathName(), Readback, *Emission.ToString()));
        return true;
    }

    bool SetProbeWaterOpacity(float Value)
    {
        auto* Material = CandidateMaterial.Get();
        if (!bWaterOpacityAB || !bWaterOpacitySaved || !Material) return false;
        Material->SetScalarParameterValue(TEXT("WaterSurfaceOpacity"), Value);
        float Actual = -1, Mask = -1; FLinearColor Deep, Shallow;
        if (!Material->GetScalarParameterValue(FHashedMaterialParameterInfo(TEXT("WaterSurfaceOpacity")), Actual) || Actual != Value
            || !Material->GetScalarParameterValue(FHashedMaterialParameterInfo(TEXT("APS_UsePresentationWaterMask")), Mask) || Mask != 1.0f
            || !Material->GetVectorParameterValue(FHashedMaterialParameterInfo(TEXT("LiquidDeepColor")), Deep) || !Deep.Equals(WaterDeepBefore)
            || !Material->GetVectorParameterValue(FHashedMaterialParameterInfo(TEXT("LiquidShallowColor")), Shallow) || !Shallow.Equals(WaterShallowBefore))
        { Test->AddError(TEXT("Water optical A/B changed palette/coverage or failed scalar readback")); return false; }
        Test->AddInfo(FString::Printf(TEXT("PLANET_WATER_OPACITY_AB material=%s opacity=%.9g mask=1 deep=%s shallow=%s ocean=%s transform=%s"),
            *Material->GetPathName(), Actual, *Deep.ToString(), *Shallow.ToString(), *GetPathNameSafe(CandidateOcean.Get()),
            *CandidateOcean->GetComponentTransform().ToHumanReadableString()));
        return true;
    }

    bool SetProbeSampling(float UseNoise, float UseCrust)
    {
        UProceduralMeshComponent* Ocean = CandidateOcean.Get();
        FProcMeshSection* Section = Ocean ? Ocean->GetProcMeshSection(0) : nullptr;
        APlayerController* Controller = Ocean ? Ocean->GetWorld()->GetFirstPlayerController() : nullptr;
        APlayerCameraManager* Camera = Controller ? Controller->PlayerCameraManager : nullptr;
        UMaterialInstanceDynamic* Material = CandidateMaterial.Get();
        if (!bLavaSamplingAB || !bEmissionStateSaved || !Section || !Camera || !Material)
        { Test->AddError(TEXT("Sampling A/B missing exact material/camera/geometry")); return false; }
        const uint32 VertexCRC = FCrc::MemCrc32(Section->ProcVertexBuffer.GetData(), Section->ProcVertexBuffer.Num() * sizeof(FProcMeshVertex));
        const uint32 IndexCRC = FCrc::MemCrc32(Section->ProcIndexBuffer.GetData(), Section->ProcIndexBuffer.Num() * sizeof(int32));
        if (!bSamplingFrameSaved)
        {
            SamplingOceanFrame = Ocean->GetComponentTransform();
            SamplingCameraLocation = Camera->GetCameraLocation(); SamplingCameraRotation = Camera->GetCameraRotation();
            SamplingVertexCRC = VertexCRC; SamplingIndexCRC = IndexCRC; bSamplingFrameSaved = true;
        }
        Material->SetScalarParameterValue(TEXT("APS_DebugUseVnoise1"), UseNoise);
        Material->SetScalarParameterValue(TEXT("APS_DebugUseWATCrust"), UseCrust);
        float Noise = -1, Crust = -1, Brightness = -1, Coverage = -1;
        FLinearColor Color;
        if (!Material->GetScalarParameterValue(FHashedMaterialParameterInfo(TEXT("APS_DebugUseVnoise1")), Noise) || Noise != UseNoise
            || !Material->GetScalarParameterValue(FHashedMaterialParameterInfo(TEXT("APS_DebugUseWATCrust")), Crust) || Crust != UseCrust
            || !Material->GetScalarParameterValue(FHashedMaterialParameterInfo(TEXT("Brightness")), Brightness) || Brightness != 0.0f
            || !Material->GetScalarParameterValue(FHashedMaterialParameterInfo(TEXT("APS_UsePresentationWaterMask")), Coverage) || Coverage != 1.0f
            || !Material->GetVectorParameterValue(FHashedMaterialParameterInfo(TEXT("EmissiveColor")), Color) || !Color.Equals(OriginalCandidateEmission)
            || !Ocean->GetComponentTransform().Equals(SamplingOceanFrame, 0.0001)
            || !Camera->GetCameraLocation().Equals(SamplingCameraLocation, 0.001)
            || !Camera->GetCameraRotation().Equals(SamplingCameraRotation, 0.0001)
            || VertexCRC != SamplingVertexCRC || IndexCRC != SamplingIndexCRC)
        { Test->AddError(TEXT("Sampling A/B changed camera/geometry/coverage/chemistry or failed scalar readback")); return false; }
        Test->AddInfo(FString::Printf(TEXT("PLANET_LAVA_SAMPLING_AB material=%s useVnoise1=%.0f useWATCrust=%.0f brightness=0 coverage=1 vertices=%d vertexCRC=%u indexCRC=%u camera=%s rotation=%s"),
            *Material->GetPathName(), Noise, Crust, Section->ProcVertexBuffer.Num(), VertexCRC, IndexCRC,
            *SamplingCameraLocation.ToString(), *SamplingCameraRotation.ToString()));
        return true;
    }

    bool SetProbeWATScales(float Fine, float Middle, float Large)
    {
        if (!APSLavaSamplingABProbe::UsesScaleIsolation() || !SetProbeSampling(1, 1)) return false;
        const TCHAR* Names[] = {TEXT("APS_DebugUseWAT400m"), TEXT("APS_DebugUseWAT5431m"), TEXT("APS_DebugUseWAT20km")};
        const float Values[] = {Fine, Middle, Large};
        for (int32 I = 0; I < 3; ++I)
        {
            CandidateMaterial->SetScalarParameterValue(Names[I], Values[I]);
            float Readback = -1.0f;
            if (!CandidateMaterial->GetScalarParameterValue(FHashedMaterialParameterInfo(Names[I]), Readback) || Readback != Values[I])
            { Test->AddError(TEXT("WAT scale isolation scalar readback failed")); return false; }
        }
        Test->AddInfo(FString::Printf(TEXT("PLANET_LAVA_WAT_SCALE_AB use400m=%.0f use5431m=%.0f use20km=%.0f nativeVnoise=1 aggregateCrust=1 brightness=0"), Fine, Middle, Large));
        return true;
    }

    void Restore()
    {
        if (bTerrainLodAB) { TerrainLodProbe.Restore(); return; }
        for (const auto& Pair : Atmospheres)
            if (Pair.Key.IsValid()) Pair.Key->SetActorHiddenInGame(Pair.Value);
        for (const auto& Pair : Shadows)
            if (Pair.Key.IsValid()) Pair.Key->SetCastShadows(Pair.Value);
        for (const auto& Pair : Casters)
            if (Pair.Key.IsValid()) Pair.Key->SetCastShadow(Pair.Value);
        if (!Generator.IsValid() || (HasSharedCandidate() && !bSceneStateSaved)) return;
        if (UProceduralMeshComponent* Terrain = Generator->GetActivePreviewTerrainProxy())
            Terrain->SetVisibility(bTerrainVisible);
        if (UProceduralMeshComponent* Ocean = Generator->GetActivePreviewOceanProxy())
        {
            if (bLavaEmissionAB) Ocean->SetVisibility(bOceanVisible);
            if (FProcMeshSection* Section = Ocean->GetProcMeshSection(0);
                Section && Section->ProcVertexBuffer.Num() == SavedColors.Num())
            {
                for (int32 I = 0; I < SavedColors.Num(); ++I)
                    Section->ProcVertexBuffer[I].Color = SavedColors[I];
                Ocean->MarkRenderStateDirty();
            }
        }
        Generator->SetActorTickEnabled(bGeneratorTick);
    }
    bool AssertProductionLiquid(UProceduralMeshComponent* Ocean, const TCHAR* Phase)
    {
        const bool bLava = Family == TEXT("Volcanic") || Family == TEXT("Melted") || Family == TEXT("Lava");
        if (HasSharedCandidate() || (!bLava && Family != TEXT("Ocean") && Family != TEXT("Water") && Family != TEXT("Ammonia"))) return true;
        auto Fail = [this](const FString& Why) { Test->AddError(TEXT("PLANET_PRODUCTION_LIQUID: ") + Why); return false; };
        APlanet* Body = Cast<APlanet>(Generator->GetActivePreviewWorldScapeBody());
        const EPlanetType ExpectedType = Family == TEXT("Ocean") ? EPlanetType::Ocean
            : Family == TEXT("Water") ? EPlanetType::Water
            : Family == TEXT("Melted") ? EPlanetType::Melted
            : Family == TEXT("Volcanic") ? EPlanetType::Volcanic
            : Family == TEXT("Lava") ? EPlanetType::Lava : EPlanetType::Ammonia;
        const EAPSPlanetLiquidType Type = bLava ? EAPSPlanetLiquidType::Lava
            : Family == TEXT("Ammonia") ? EAPSPlanetLiquidType::Ammonia : EAPSPlanetLiquidType::Water;
        if (!IsValid(Body) || Body->IsManual || Body->PlanetType != ExpectedType || !IsValid(Ocean))
            return Fail(TEXT("missing actual generated requested planet/ocean"));
        APlanetarySurfaceGenerator* Resolver = nullptr;
        for (TActorIterator<APlanetarySurfaceGenerator> It(Ocean->GetWorld()); It; ++It)
        {
            if (It->GetOwner() != Generator.Get() || !It->IsSurfaceProfileCurrent(Body)) continue;
            if (Resolver) return Fail(TEXT("ambiguous actual preview resolver"));
            Resolver = *It;
        }
        if (!Resolver || Resolver->ResolvedSurfaceProfile.LiquidType != Type
            || !IsValid(Resolver->WorldScapeRootInstance) || !Resolver->WorldScapeRootInstance->bOcean)
            return Fail(TEXT("requested production profile is not the actual enabled liquid"));
        auto* Preview = Cast<UMaterialInstanceDynamic>(Ocean->GetMaterial(0));
        auto* Resolved = Resolver->ResolvedOceanMaterialInstance;
        FString PreviewEvidence, ResolverEvidence, Error;
        if (!APSProductionSharedLiquidAssertions::Validate(Preview, Type, Ocean,
            Body->WorldScapePresentationScale, 1.0f, PreviewEvidence, Error)) return Fail(Error);
        if (!APSProductionSharedLiquidAssertions::Validate(Resolved, Type, Resolver->WorldScapeRootInstance->GetRootComponent(),
            Body->WorldScapePresentationScale, 0.0f, ResolverEvidence, Error)) return Fail(Error);
        if (Preview->Parent != Resolved->Parent || Resolver->WorldScapeRootInstance->OceanMaterial.DefaultMaterial != Resolved
            || (ProductionLiquidParent.IsValid() && ProductionLiquidParent.Get() != Preview->Parent.Get()))
            return Fail(TEXT("production parent/slot identity changed across modes or baseline views"));
        ProductionLiquidParent = Preview->Parent.Get();
        Test->AddInfo(FString::Printf(TEXT("PLANET_PRODUCTION_LIQUID phase=%s family=%s noCandidate=1 sameResolvedParent=1 frameMatchesActual=1 %s resolverMaterial=%s"),
            Phase, *Family, *PreviewEvidence, *Resolved->GetPathName()));
        return true;
    }

    void Capture(const TCHAR* Label)
    {
        UGameViewportClient* Client = AutomationCommon::GetAnyGameViewportClient();
        if (!Client || !Client->GetGameViewportWidget().IsValid())
        { Test->AddError(TEXT("No composited viewport")); return; }
        TArray<FColor> Pixels; FIntVector Size;
        if (!FSlateApplication::Get().TakeScreenshot(Client->GetGameViewportWidget().ToSharedRef(), Pixels, Size)
            || Size.X <= 0 || Size.Y <= 0 || Pixels.Num() != Size.X * Size.Y)
        { Test->AddError(TEXT("Capture failed")); return; }
        FString Folder = FPaths::ProjectSavedDir() / TEXT("Automation/PlanetRefinement") / Family;
        if (bSharedLavaCandidate) Folder /= TEXT("SharedLavaCandidate");
        if (bLavaSamplingAB) Folder /= APSLavaSamplingABProbe::UsesScaleIsolation() ? TEXT("LavaScaleAB") : TEXT("LavaSamplingAB");
        if (bLavaBandwidthLOD) Folder /= TEXT("LavaBandwidthLOD");
        if (bSharedAmmoniaCandidate) Folder /= TEXT("SharedAmmoniaCandidate");
        if (bSharedWaterCandidate) Folder /= TEXT("SharedWaterCandidate");
        if (bWaterOpacityAB) Folder /= TEXT("WaterOpacityAB");
        if (APSPlanetTerrainLodAB::UsesNativeViews()) Folder /= TEXT("TerrainNativeViews");
        else if (bTerrainLodAB) Folder /= bTerrainFarNormalAB ? TEXT("TerrainPixelFarNormalAB") : TEXT("TerrainPixelAB");
        IFileManager::Get().MakeDirectory(*Folder, true);
        TArray64<uint8> Png; FImageUtils::PNGCompressImageArray(Size.X, Size.Y, Pixels, Png);
        Test->TestTrue(TEXT("Saved real PLANET capture"), FFileHelper::SaveArrayToFile(Png, *(Folder / (FString(Label) + TEXT(".png")))));
        Test->AddInfo(FString::Printf(TEXT("PLANET_PROBE family=%s phase=%s"), *Family, Label));
    }
public:
    explicit FProbe(FAutomationTestBase* InTest) : Test(InTest)
    {
        FParse::Value(FCommandLine::Get(), TEXT("APSPlanetProbeFamily="), Family);
        FParse::Value(FCommandLine::Get(), TEXT("APSPlanetProbeZoom="), Zoom);
        FParse::Value(FCommandLine::Get(), TEXT("APSPlanetProbeAtmosphereOpacity="), AtmosphereOpacity);
        bTerrainLodAB = FParse::Param(FCommandLine::Get(), TEXT("APSProbeTerrainLodAB")) || APSPlanetTerrainLodAB::UsesNativeViews();
        bTerrainFarNormalAB = FParse::Param(FCommandLine::Get(), TEXT("APSProbeTerrainFarNormalAB"));
        bSharedLavaCandidate = FParse::Param(FCommandLine::Get(), TEXT("APSProbeSharedLavaCandidate"));
        bLavaEmissionAB = FParse::Param(FCommandLine::Get(), TEXT("APSProbeLavaEmissionAB"));
        bLavaSamplingAB = FParse::Param(FCommandLine::Get(), TEXT("APSProbeLavaSamplingAB"));
        bLavaBandwidthLOD = APSLavaSamplingABProbe::UsesBandwidthLOD();
        bWaterOpacityAB = FParse::Param(FCommandLine::Get(), TEXT("APSProbeWaterOpacityAB"));
        bSharedAmmoniaCandidate = FParse::Param(FCommandLine::Get(), TEXT("APSProbeSharedAmmoniaCandidate"));
        bSharedWaterCandidate = FParse::Param(FCommandLine::Get(), TEXT("APSProbeSharedWaterCandidate"));
    }
    ~FProbe() override { Restore(); RestoreCandidate(); }
    bool Update() override
    {
        const double Now = FPlatformTime::Seconds();
        if (Now < Next) return false;
        UWorld* World = AutomationCommon::GetAnyGameWorld();
        AMainMenuController* Controller = World ? Cast<AMainMenuController>(World->GetFirstPlayerController()) : nullptr;
        UWorldGenerationViewModel* VM = Controller ? Controller->GetWorldGenerationViewModel() : nullptr;
        if (Now - Start > 100) { DiagnosePending(TEXT("Timeout"), World, VM, true); Test->AddError(TEXT("PLANET causal probe timeout")); Restore(); return true; }
        if (!VM || !VM->GeneratedWorld) { DiagnosePending(TEXT("ViewModelOrGeneratedWorld"), World, VM); return false; }
        if (Step == 0)
        {
            if (!FMath::IsFinite(AtmosphereOpacity) || AtmosphereOpacity < 0.0f || AtmosphereOpacity > 20.0f)
            { Test->AddError(TEXT("Invalid explicit probe atmosphere opacity")); return true; }
            if (bLavaBandwidthLOD && (!bSharedLavaCandidate || bSharedAmmoniaCandidate || bSharedWaterCandidate
                || bLavaSamplingAB || bLavaEmissionAB || bWaterOpacityAB || bTerrainLodAB || bTerrainFarNormalAB
                || APSLavaSamplingABProbe::UsesScaleIsolation() || APSLavaSamplingABProbe::UsesExplicitDerivatives()))
            { Test->AddError(TEXT("Clean Lava bandwidth LOD requires only shared Lava; no causal permutations")); return true; }
            if (APSLavaSamplingABProbe::UsesScaleIsolation()
                && (!bLavaSamplingAB || APSLavaSamplingABProbe::UsesExplicitDerivatives()))
            { Test->AddError(TEXT("WAT scale isolation requires sampling A/B and forbids derivative candidate mixing")); return true; }
            if ((bTerrainFarNormalAB && !bTerrainLodAB) || (bTerrainLodAB && (Family != TEXT("Tundra")
                || HasSharedCandidate() || bWaterOpacityAB || bLavaSamplingAB || bLavaEmissionAB
                || FParse::Param(FCommandLine::Get(), TEXT("APSProbeLavaDerivativeFix")))))
            { Test->AddError(TEXT("PLANET terrain A/B requires Tundra and no liquid diagnostic flags; far-normal requires terrain A/B")); return true; }
            if (bWaterOpacityAB && (!bSharedWaterCandidate || bLavaSamplingAB || bLavaEmissionAB))
            { Test->AddError(TEXT("Water optical A/B requires only shared Water")); return true; }
            if (bLavaSamplingAB && (!bSharedLavaCandidate || bSharedAmmoniaCandidate || bSharedWaterCandidate || bLavaEmissionAB))
            { Test->AddError(TEXT("Lava sampling A/B requires only shared Lava, without emission A/B")); return true; }
            if (bLavaEmissionAB && (!bSharedLavaCandidate || bSharedAmmoniaCandidate || bSharedWaterCandidate))
            { Test->AddError(TEXT("Lava emission A/B requires only the shared Lava candidate")); return true; }
            if (int32(bSharedLavaCandidate) + int32(bSharedAmmoniaCandidate) + int32(bSharedWaterCandidate) > 1)
            { Test->AddError(TEXT("Select exactly one shared-liquid candidate family")); return true; }
            if (bSharedWaterCandidate && Family != TEXT("Ocean") && Family != TEXT("Water"))
            { Test->AddError(TEXT("Shared Water candidate probe requires Ocean or Water")); return true; }
            if (bSharedAmmoniaCandidate && Family != TEXT("Ammonia"))
            { Test->AddError(TEXT("Shared Ammonia candidate probe requires Ammonia")); return true; }
            if (bSharedLavaCandidate && Family != TEXT("Melted") && Family != TEXT("Volcanic") && Family != TEXT("Lava"))
            { Test->AddError(TEXT("Shared Lava candidate probe requires Melted, Volcanic or Lava")); return true; }
            UGeneratedWorld* Model = VM->GeneratedWorld;
            Model->AstroGenerationLevel = EAstroGenerationLevel::StarCluster;
            Model->bGenerateFullScaledWorld = true; Model->bGenerateHomeSystem = true;
            Model->bStartWithHomePlanet = true; Model->GenerationSeed = 271828;
            Model->GalaxyStarCount = 10000;
            Model->bRandomHomeStar = false; Model->StellarType = EStellarType::MainSequence;
            Model->SpectralClass = ESpectralClass::G; Model->StarType = EStarType::SingleStar;
            Model->PlanetsAmount = 1; Model->MoonsAmount = 0; Model->StartPlanetIndex = 1;
            Model->PlanetRadius = 6750; Model->PlanetSurfaceSeed = 1337;
            Model->PlanetType = Family == TEXT("Melted") ? EPlanetType::Melted :
                Family == TEXT("Tundra") ? EPlanetType::Tundra :
                Family == TEXT("Volcanic") ? EPlanetType::Volcanic :
                Family == TEXT("Lava") ? EPlanetType::Lava :
                Family == TEXT("Ice") ? EPlanetType::Ice :
                Family == TEXT("Frozen") ? EPlanetType::Frozen :
                Family == TEXT("Ammonia") ? EPlanetType::Ammonia :
                Family == TEXT("Water") ? EPlanetType::Water :
                Family == TEXT("Metal") ? EPlanetType::Metal : EPlanetType::Ocean;
            // Match the current generated-world default. The old diagnostic
            // value 12 hid the surface under a dense white atmosphere.
            Model->AtmosphereHeight = 100; Model->AtmosphereOpacity = AtmosphereOpacity;
            Test->AddInfo(FString::Printf(TEXT("PLANET_PROBE_SETUP family=%s opacity=%.6g zoom=%.6g candidate=%d"),
                *Family, AtmosphereOpacity, Zoom, HasSharedCandidate() ? 1 : 0));
            if (!Controller->OpenAstronomicalGenerationForAutomation(EAstroPreviewFocus::HomePlanet, EAPSGenerationRoute::Civilization)) return false;
            Step = 1; Next = Now + 3; return false;
        }
        if (!VM->bPreviewReady) { DiagnosePending(TEXT("PreviewReady"), World, VM); return false; }
        if (!Generator.IsValid())
            for (TActorIterator<AAstroGenerator> It(World); It; ++It)
                if (It->GetActivePreviewTerrainProxy()) { Generator = *It; break; }
        if (!Generator.IsValid()) { DiagnosePending(TEXT("GeneratorWithActiveTerrain"), World, VM); return false; }
        if (!Generator->IsPreviewGlobeFamilyWarmQueueDrained()) { DiagnosePending(TEXT("FamilyWarmQueue"), World, VM); return false; }
        UProceduralMeshComponent* Terrain = Generator->GetActivePreviewTerrainProxy();
        UProceduralMeshComponent* Ocean = Generator->GetActivePreviewOceanProxy();
        if (bTerrainLodAB)
        {
            FString Error;
            const auto Result = TerrainLodProbe.Update(Generator.Get(), VM, bTerrainFarNormalAB, Zoom,
                [this](const TCHAR* Label) { Capture(Label); }, Error);
            if (Result == APSPlanetTerrainLodAB::EResult::Failed) { Test->AddError(Error); Restore(); }
            return Result != APSPlanetTerrainLodAB::EResult::Pending;
        }
        if (bCandidateApplied && (Ocean != CandidateOcean.Get() || !Ocean || Ocean->GetMaterial(0) != CandidateMaterial.Get()))
        { Test->AddError(TEXT("PLANET replaced the candidate ocean/material during the probe")); return true; }
        if (Step == 1)
        {
            if (HasSharedCandidate() && !bCandidateApplied)
            {
                const ECandidateResult Result = PrepareCandidate(Terrain, Ocean);
                if (Result == ECandidateResult::Failed) return true;
                if (Result == ECandidateResult::Pending) DiagnosePending(TEXT("CandidateShader"), World, VM);
                // Let the actual viewport publish the new render proxy before
                // taking the first frame. This also bounds shader-ready polling.
                Next = Now + 1.0;
                return false;
            }
            if (!AssertProductionLiquid(Ocean, TEXT("00-orbit"))) return true;
            Capture(TEXT("00-orbit"));
            VM->ZoomPreview(Zoom);
            VM->BeginPreviewOrbit(); VM->OrbitPreview(FVector2D(240, 50)); VM->EndPreviewOrbit();
            Step = 2; Next = Now + 4; return false;
        }
        if (Step == 2)
        {
            if (!AssertProductionLiquid(Ocean, TEXT("01-close-baseline"))) return true;
            Capture(TEXT("01-close-baseline"));
            if (FParse::Param(FCommandLine::Get(), TEXT("APSProbeProductionLiquidOnly")))
            {
                Test->AddInfo(TEXT("PLANET_PRODUCTION_LIQUID_ONLY originalOrbitAndClose=1 noMaterialOrLayerOverride=1"));
                return true;
            }
            // Two ordinary candidate views; no causal layer/noise/brightness edits.
            if (bLavaBandwidthLOD)
            {
                Test->AddInfo(TEXT("PLANET_LAVA_BANDWIDTH_LOD cleanCandidate=1 orbitCloseCaptured=1 renderedAcceptancePending=1"));
                return true;
            }
            bGeneratorTick = Generator->IsActorTickEnabled();
            bTerrainVisible = Terrain && Terrain->IsVisible();
            bOceanVisible = Ocean && Ocean->IsVisible();
            bSceneStateSaved = true;
            Generator->SetActorTickEnabled(false);
            if (bWaterOpacityAB)
            {
                if (!SetProbeWaterOpacity(OriginalWaterOpacity)) return true;
                Step = 30; Next = Now + 3; return false;
            }
            if (bLavaSamplingAB)
            {
                if (!SetProbeSampling(1.0f, 1.0f)) return true;
                Step = 20; Next = Now + 3; return false;
            }
            for (TActorIterator<AAtmoScape> It(World); It; ++It)
            { Atmospheres.Emplace(*It, It->IsHidden()); It->SetActorHiddenInGame(true); }
            for (TActorIterator<AActor> It(World); It; ++It)
            {
                TArray<UPrimitiveComponent*> Components; It->GetComponents(Components);
                for (auto* Component : Components)
                {
                    if (Component->CastShadow)
                        Test->AddInfo(FString::Printf(TEXT("PLANET_CASTER %s owner=%s visible=%d hidden=%d ownerhidden=%d hiddenShadow=%d pos=%s radius=%.1f"),
                            *Component->GetName(), *It->GetName(), Component->IsVisible(), Component->bHiddenInGame, It->IsHidden(),
                            Component->bCastHiddenShadow, *Component->Bounds.Origin.ToString(), Component->Bounds.SphereRadius));
                    if (Component->IsVisible() && !Component->bHiddenInGame)
                        Test->AddInfo(FString::Printf(TEXT("PLANET_VISIBLE %s owner=%s pos=%s scale=%s mat=%s"),
                            *Component->GetName(), *It->GetName(), *Component->GetComponentLocation().ToString(),
                            *Component->GetComponentScale().ToString(), *GetPathNameSafe(Component->GetMaterial(0))));
                }
            }
        }
        else if (Step == 3)
        {
            Capture(TEXT("02-no-atmosphere"));
            for (const auto& Pair : Atmospheres) if (Pair.Key.IsValid()) Pair.Key->SetActorHiddenInGame(Pair.Value);
            if (Terrain) Terrain->SetVisibility(false);
        }
        else if (Step == 4)
        {
            Capture(TEXT("03-no-terrain"));
            if (Terrain) Terrain->SetVisibility(bTerrainVisible);
            for (TActorIterator<AActor> It(World); It; ++It)
            {
                TArray<ULightComponent*> Lights; It->GetComponents(Lights);
                for (auto* Light : Lights) { Shadows.Emplace(Light, bool(Light->CastShadows)); Light->SetCastShadows(false); }
            }
        }
        else if (Step == 5)
        {
            Capture(TEXT("04-no-shadows"));
            for (const auto& Pair : Shadows) if (Pair.Key.IsValid()) Pair.Key->SetCastShadows(Pair.Value);
            if (Ocean)
                if (auto* Section = Ocean->GetProcMeshSection(0))
                {
                    SavedColors.Reserve(Section->ProcVertexBuffer.Num());
                    for (auto& Vertex : Section->ProcVertexBuffer)
                    { SavedColors.Add(Vertex.Color); Vertex.Color.A = 255; }
                    Ocean->MarkRenderStateDirty();
                }
        }
        else if (Step == 6)
        {
            Capture(TEXT("05-uniform-ocean-alpha")); Restore();
        }
        else if (Step == 7)
        {
            Capture(TEXT("06-rotated"));
            for (TActorIterator<APlanetaryBody> It(World); It; ++It)
            {
                TArray<UPrimitiveComponent*> Components; It->GetComponents(Components);
                for (auto* Component : Components)
                { Casters.Emplace(Component, bool(Component->CastShadow)); Component->SetCastShadow(false); }
            }
        }
        else if (Step == 8)
        {
            Capture(TEXT("07-no-body-casters")); Restore();
            for (TActorIterator<AStar> It(World); It; ++It)
                if (It->StarMesh)
                { Casters.Emplace(It->StarMesh, bool(It->StarMesh->CastShadow)); It->StarMesh->SetCastShadow(false); }
        }
        else if (Step == 9)
        {
            Capture(TEXT("08-no-emitter-shadow")); Restore();
            if (!bLavaEmissionAB) return true;
            Generator->SetActorTickEnabled(false);
            Ocean->SetVisibility(false);
        }
        else if (Step == 10)
        {
            Capture(TEXT("09-no-ocean"));
            Ocean->SetVisibility(bOceanVisible);
            if (!SetProbeBrightness(1.0f)) return true;
        }
        else if (Step == 11)
        {
            Capture(TEXT("10-lava-brightness-1"));
            if (!SetProbeBrightness(2.0f)) return true;
        }
        else if (Step == 12)
        {
            Capture(TEXT("11-lava-brightness-2"));
            if (!SetProbeBrightness(3.0f)) return true;
        }
        else if (Step == 13)
        {
            Capture(TEXT("12-lava-brightness-3"));
            if (!SetProbeBrightness(OriginalCandidateBrightness)) return true;
        }
        else if (Step == 14)
        { Capture(TEXT("13-lava-brightness-restored")); Restore(); return true; }
        else if (APSLavaSamplingABProbe::UsesScaleIsolation() && Step >= 20 && Step <= 24)
        {
            const float Settings[5][3] = {{1,1,1}, {1,1,0}, {1,0,1}, {0,1,1}, {1,1,1}};
            const TCHAR* Labels[] = {TEXT("20-scale-all-on"), TEXT("21-scale-20km-off"),
                TEXT("22-scale-5431m-off"), TEXT("23-scale-400m-off"), TEXT("24-scale-restored")};
            const int32 Index = Step - 20;
            if (!SetProbeWATScales(Settings[Index][0], Settings[Index][1], Settings[Index][2])) return true;
            Capture(Labels[Index]);
            if (Step == 24) { Restore(); return true; }
            if (!SetProbeWATScales(Settings[Index+1][0], Settings[Index+1][1], Settings[Index+1][2])) return true;
        }
        else if (Step == 20)
        {
            if (!SetProbeSampling(1, 1)) return true;
            Capture(TEXT("20-sampling-both-on"));
            if (!SetProbeSampling(0, 1)) return true;
        }
        else if (Step == 21)
        {
            if (!SetProbeSampling(0, 1)) return true;
            Capture(TEXT("21-sampling-no-vnoise1"));
            if (!SetProbeSampling(1, 0)) return true;
        }
        else if (Step == 22)
        {
            if (!SetProbeSampling(1, 0)) return true;
            Capture(TEXT("22-sampling-no-wat-crust"));
            if (!SetProbeSampling(0, 0)) return true;
        }
        else if (Step == 23)
        {
            if (!SetProbeSampling(0, 0)) return true;
            Capture(TEXT("23-sampling-both-neutral"));
            if (!SetProbeSampling(1, 1)) return true;
        }
        else if (Step == 24)
        {
            if (!SetProbeSampling(1, 1)) return true;
            Capture(TEXT("24-sampling-restored")); Restore(); return true;
        }
        else if (Step == 30)
        {
            Capture(TEXT("30-optical-opacity-078"));
            if (!SetProbeWaterOpacity(0.25f)) return true;
        }
        else if (Step == 31)
        {
            Capture(TEXT("31-optical-opacity-025"));
            if (!SetProbeWaterOpacity(0.0f)) return true;
        }
        else if (Step == 32)
        {
            Capture(TEXT("32-optical-opacity-000"));
            if (!SetProbeWaterOpacity(OriginalWaterOpacity)) return true;
        }
        else if (Step == 33)
        { Capture(TEXT("33-optical-opacity-restored")); Restore(); return true; }
        ++Step; Next = Now + 3; return false;
    }
};
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAPSPlanetRefinementRenderedProbe,
    "APS.Rendered.PlanetRefinement.CausalLayers", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext | EAutomationTestFlags::EngineFilter)
bool FAPSPlanetRefinementRenderedProbe::RunTest(const FString& Parameters)
{
    if (!AutomationOpenMap(TEXT("/Game/APS/APS_ALPHA/Menu/L_APS_MainMenu_Alpha"), true)) return false;
    ADD_LATENT_AUTOMATION_COMMAND(APSPlanetRefinement::FProbe(this));
    return true;
}
#endif
