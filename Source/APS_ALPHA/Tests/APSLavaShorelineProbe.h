#pragma once

#if WITH_DEV_AUTOMATION_TESTS
#include "APS_ALPHA/Generation/WorldScapePayloadValidation.h"
#include "APS_ALPHA/Pawns/Characters/CustomGravityCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/Texture.h"
#include "Engine/Texture2D.h"

// Opt-in continuation AFTER the natural PlanetSurface landing's First/+3/+8
// frames. Only the real pawn/camera is leased by default. Optional ThermalAB
// changes scalars on the same live MID, snapshots/restores its overrides.
// FinePeriodAB isolates 400m versus 40m texture periods with coverage=1 in both.
// No hidden terrain, alternate MID, alpha replacement or lighting override.
namespace APSLavaShoreline
{
class FProbe final : public IAutomationLatentCommand
{
    FAutomationTestBase* Test;
    TWeakObjectPtr<UWorld> World;
    TWeakObjectPtr<APlanet> Planet;
    TWeakObjectPtr<APlanetarySurfaceGenerator> Surface;
    TWeakObjectPtr<AWorldScapeRoot> Root;
    TWeakObjectPtr<APlayerController> PC;
    TWeakObjectPtr<ACustomGravityCharacter> Pawn;
    TWeakObjectPtr<AActor> OriginalView;
    TWeakObjectPtr<ACameraActor> Camera;
    FVector OriginalECEF, OriginalVelocity, Shore, WetTangent;
    FQuat OriginalRotation;
    FRotator OriginalControl;
    bool bOriginalZeroG = false, bLeased = false, bDone = false;
    double Started = 0, PlacedAt = 0, LastPending = 0;
    int32 View = 0, Stable = 0;
    uint32 ProfileSignature = 0;
    const bool bThermalAB = FParse::Param(FCommandLine::Get(), TEXT("APSLavaThermalAB"));
    const bool bFinePeriodAB = FParse::Param(FCommandLine::Get(), TEXT("APSLavaFinePeriodAB"));
    const bool bStochasticAB = FParse::Param(FCommandLine::Get(), TEXT("APSLavaStochasticAB"));
    const bool bCrustNormalAB = FParse::Param(FCommandLine::Get(), TEXT("APSLavaCrustNormalAB"));
    const bool bCrustFieldAB = FParse::Param(FCommandLine::Get(), TEXT("APSLavaCrustFieldAB"));
    const bool bRadianceAB = FParse::Param(FCommandLine::Get(), TEXT("APSLavaJointRadianceAB"));
    const bool bFarShoreline = FParse::Param(FCommandLine::Get(), TEXT("APSLavaFarShoreline"));
    bool HasCrustAB() const { return bCrustNormalAB || bCrustFieldAB || bRadianceAB; }
    int32 ThermalVariant = 0;
    TStrongObjectPtr<UMaterialInstanceDynamic> ThermalSaved{nullptr};
    TWeakObjectPtr<UMaterialInstanceDynamic> ThermalLive;
    TArray<FScalarParameterValue> ThermalApplied;
    TArray<FTextureParameterValue> ThermalTexturesApplied;

    bool SetCrustField(bool bStructured)
    {
        const FString Name = bStructured ? TEXT("T_APS_LavaCrustField40m_v1") : TEXT("T_APS_LavaThermal400m_RGBA_v1");
        auto* Texture = LoadObject<UTexture2D>(nullptr, *(TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/SharedLiquid/") + Name + TEXT(".") + Name));
        if (!Texture || !ThermalLive.IsValid()) return false;
        ThermalLive->SetTextureParameterValue(TEXT("APS_LavaCrustField"), Texture);
        ThermalTexturesApplied = ThermalLive->TextureParameterValues;
        return true;
    }

    bool ValidateThermal() const
    {
        if (!bThermalAB) return true;
        float Value = -1;
        if (HasCrustAB() && (!ThermalLive.IsValid()
            || !ThermalLive->GetScalarParameterValue(FHashedMaterialParameterInfo(TEXT("APS_LavaCrustHeightCm")), Value)
            || Value != ((bCrustFieldAB || bRadianceAB || ThermalVariant == 1) ? 25.0f : 0.0f)
            || !ThermalLive->GetScalarParameterValue(FHashedMaterialParameterInfo(TEXT("APS_LavaFineStochastic")), Value) || Value != 1
            || !ThermalLive->GetScalarParameterValue(FHashedMaterialParameterInfo(TEXT("APS_LavaFinePeriodCm")), Value) || Value != 4000)) return false;
        if (bCrustFieldAB)
        {
            UTexture* Bound = nullptr;
            if (!ThermalLive.IsValid() || !ThermalLive->GetTextureParameterValue(FHashedMaterialParameterInfo(TEXT("APS_LavaCrustField")), Bound)
                || !Bound || Bound->GetName() != (ThermalVariant == 0 ? TEXT("T_APS_LavaThermal400m_RGBA_v1") : TEXT("T_APS_LavaCrustField40m_v1"))) return false;
        }
        if (bRadianceAB)
        {
            UTexture* Bound = nullptr;
            if (!ThermalLive.IsValid() || !ThermalLive->GetScalarParameterValue(FHashedMaterialParameterInfo(TEXT("APS_LavaRadianceIntegration")), Value)
                || Value != float(ThermalVariant)
                || !ThermalLive->GetTextureParameterValue(FHashedMaterialParameterInfo(TEXT("APS_LavaCrustField")), Bound)
                || !Bound || Bound->GetName() != TEXT("T_APS_LavaThermal400m_RGBA_v1")) return false;
        }
        if (bStochasticAB && (bFinePeriodAB || !ThermalLive.IsValid()
            || !ThermalLive->GetScalarParameterValue(FHashedMaterialParameterInfo(TEXT("APS_LavaFineStochastic")), Value)
            || Value != float(ThermalVariant)
            || !ThermalLive->GetScalarParameterValue(FHashedMaterialParameterInfo(TEXT("APS_LavaFinePeriodCm")), Value)
            || Value != 4000.0f)) return false;
        if (bFinePeriodAB && (!ThermalLive.IsValid()
            || !ThermalLive->GetScalarParameterValue(FHashedMaterialParameterInfo(TEXT("APS_LavaFinePeriodCm")), Value)
            || Value != (ThermalVariant == 0 ? 40000.0f : 4000.0f))) return false;
        return ThermalLive.IsValid() && ThermalSaved.IsValid()
            && ThermalLive.Get() == Surface->ResolvedOceanMaterialInstance
            && ThermalLive->Parent == ThermalSaved->Parent
            && ThermalLive->ScalarParameterValues == ThermalApplied
            && ThermalLive->VectorParameterValues == ThermalSaved->VectorParameterValues
            && ThermalLive->TextureParameterValues == ThermalTexturesApplied
            && ThermalLive->GetScalarParameterValue(FHashedMaterialParameterInfo(TEXT("APS_LavaThermalCoverage")), Value)
            && Value == ((bFinePeriodAB || bStochasticAB || HasCrustAB()) ? 1.0f : float(ThermalVariant));
    }

    void RestoreThermal()
    {
        if (ThermalLive.IsValid() && ThermalSaved.IsValid())
        {
            ThermalLive->CopyParameterOverrides(ThermalSaved.Get());
            if (ThermalLive->ScalarParameterValues != ThermalSaved->ScalarParameterValues
                || ThermalLive->VectorParameterValues != ThermalSaved->VectorParameterValues
                || ThermalLive->TextureParameterValues != ThermalSaved->TextureParameterValues
                || ThermalLive->DoubleVectorParameterValues != ThermalSaved->DoubleVectorParameterValues)
                Test->AddError(TEXT("Thermal A/B exact override restoration failed"));
            else Test->AddInfo(TEXT("LAVA_THERMAL_RESTORE exactOriginalOverrides=1 savedAssetsUntouched=1"));
        }
        ThermalLive.Reset(); ThermalSaved.Reset(); ThermalApplied.Reset(); ThermalTexturesApplied.Reset(); ThermalVariant = 0;
    }
    static constexpr double ClearancesCm[] = {1500000.0, 150000.0, 15000.0, 1500.0, 300.0};
    // Observe the real coast from orbit without replacing the liquid, hiding
    // terrain or leaving the streaming pawn at the surface. This remains a
    // settled view sequence, not proof of a continuous high-speed descent.
    static constexpr double FarClearancesCm[] = {30000000.0, 10000000.0, 1500000.0};
    double ViewClearanceCm() const { return bFarShoreline ? FarClearancesCm[View] : ClearancesCm[View]; }
    int32 ViewCount() const { return bFarShoreline ? 3 : (FParse::Param(FCommandLine::Get(), TEXT("APSLavaThermalMipAB"))
        || FParse::Param(FCommandLine::Get(), TEXT("APSLavaNearShoreline"))) ? 5 : 3; }

    double Height(const FVector& Direction) const
    {
        return Root->GetGroundHeight(Root->ECEFToWorld(Direction * Root->PlanetScale).ToFVector(), false);
    }

    bool FindShore()
    {
        const FVector Start = Root->WorldToECEF(Pawn->GetActorLocation()).ToFVector().GetSafeNormal();
        const double Sea = Root->OceanHeight;
        FVector Dry = Start, Wet = Start;
        const double Initial = Height(Start);
        if (!FMath::IsFinite(Initial) || !FMath::IsFinite(Sea)) return false;
        bool HaveDry = Initial > Sea + 1000, HaveWet = Initial < Sea - 1000;
        FVector U, V; Start.FindBestAxisVectors(U, V);
        int32 Samples = 1;
        // Local expanding rings keep the chosen coast near the verified day-side
        // landing. Local phase: at most 97 samples, followed if needed by
        // 128 hemisphere samples, then 24 bracket refinements.
        for (double Km : {1.0, 4.0, 16.0, 64.0, 256.0, 1024.0})
        {
            for (int32 I = 0; I < 16 && !(HaveDry && HaveWet); ++I)
            {
                const double Angle = I * (2.0 * PI / 16.0);
                const FVector D = (Start * Root->PlanetScale
                    + (U * FMath::Cos(Angle) + V * FMath::Sin(Angle)) * (Km * 100000.0)).GetSafeNormal();
                if (FVector::DotProduct(Start, D) < 0.85) continue;
                const double H = Height(D); ++Samples;
                if (!FMath::IsFinite(H)) return false;
                if (!HaveDry && H > Sea + 1000) { Dry = D; HaveDry = true; }
                if (!HaveWet && H < Sea - 1000) { Wet = D; HaveWet = true; }
            }
            if (HaveDry && HaveWet) break;
        }
        // A dry start can be deep inside a continent. Expand across the lit
        // hemisphere before declaring no coast, never invent a liquid patch.
        for (double Degrees : {20.0, 40.0, 60.0, 80.0})
        {
            const double Arc = FMath::DegreesToRadians(Degrees);
            for (int32 I = 0; I < 32 && !(HaveDry && HaveWet); ++I)
            {
                const double A = I * (2.0 * PI / 32.0);
                const FVector D = Start * FMath::Cos(Arc)
                    + (U * FMath::Cos(A) + V * FMath::Sin(A)) * FMath::Sin(Arc);
                const double H = Height(D); ++Samples;
                if (!FMath::IsFinite(H)) return false;
                if (!HaveDry && H > Sea + 1000) { Dry = D; HaveDry = true; }
                if (!HaveWet && H < Sea - 1000) { Wet = D; HaveWet = true; }
            }
            if (HaveDry && HaveWet) break;
        }
        if (!HaveDry || !HaveWet)
        {
            Test->AddInfo(FString::Printf(TEXT("LAVA_SHORE_SEARCH_EMPTY samples=%d initialCm=%.3f seaCm=%.3f dry=%d wet=%d"),
                Samples, Initial, Sea, HaveDry, HaveWet));
            return false;
        }
        const FVector WetSide = Wet, DrySide = Dry;
        for (int32 I = 0; I < 24; ++I)
        {
            const FVector Mid = (Dry + Wet).GetSafeNormal();
            const double H = Height(Mid); ++Samples;
            if (!FMath::IsFinite(H)) return false;
            if (H >= Sea) Dry = Mid; else Wet = Mid;
        }
        Shore = (Dry + Wet).GetSafeNormal();
        // Use the full original bracket in centimetres. Normalizing a difference
        // of unit directions with UE's default tolerance rejects valid sub-km
        // coasts on an Earth-sized sphere (squared magnitude below 1e-8).
        WetTangent = FVector::VectorPlaneProject(
            (WetSide - DrySide) * Root->PlanetScale, Shore).GetSafeNormal();
        if (WetTangent.IsNearlyZero())
        {
            Test->AddInfo(TEXT("LAVA_SHORE_SEARCH_DEGENERATE original bracket has no physical tangent"));
            return false;
        }
        const double Span = FVector::Distance(Dry, Wet) * Root->PlanetScale;
        Test->AddInfo(FString::Printf(TEXT("LAVA_SHORE_FOUND samples=%d nativeDryCm=%.3f nativeWetCm=%.3f seaCm=%.3f bracketCm=%.3f direction=%s geographyUnchanged=1"),
            Samples, Height(Dry), Height(Wet), Sea, Span, *Shore.ToCompactString()));
        return Span < 100 && Height(Dry) >= Sea && Height(Wet) < Sea;
    }

    void Restore()
    {
        RestoreThermal();
        if (!bLeased) return;
        bLeased = false;
        if (PC.IsValid() && Camera.IsValid() && PC->GetViewTarget() == Camera.Get() && OriginalView.IsValid())
            PC->SetViewTarget(OriginalView.Get());
        if (Pawn.IsValid() && Root.IsValid())
        {
            Pawn->SetActorLocationAndRotation(Root->ECEFToWorld(OriginalECEF).ToFVector(),
                OriginalRotation, false, nullptr, ETeleportType::TeleportPhysics);
            Pawn->SetManualZeroGOverride(bOriginalZeroG);
            if (Pawn->GetCharacterMovement()) Pawn->GetCharacterMovement()->Velocity = OriginalVelocity;
            if (PC.IsValid()) PC->SetControlRotation(OriginalControl);
        }
        if (Camera.IsValid()) Camera->Destroy();
        Test->AddInfo(TEXT("LAVA_SHORE_RESTORE pawn/camera lease restored; no material/mesh/light/tick state changed"));
    }

    bool Finish(const FString& Error = FString())
    {
        if (!Error.IsEmpty()) Test->AddError(TEXT("[APS.LavaShoreline] ") + Error);
        Restore(); bDone = true; return true;
    }

    bool Capture(double ObserverDelta, double ActualClearance)
    {
        if (!ValidateThermal()) return Finish(TEXT("thermal-only A/B parameter guard failed"));
        FString Evidence, Error;
        if (!APSProductionSharedLiquidAssertions::Validate(Surface->ResolvedOceanMaterialInstance,
            EAPSPlanetLiquidType::Lava, Root->GetRootComponent(), 1.0, 0.0f, Evidence, Error))
            return Finish(Error);
        if (FParse::Param(FCommandLine::Get(), TEXT("APSLavaThermalMipAB")))
        {
            if (!bThermalAB) return Finish(TEXT("mip binding check requires thermal A/B"));
            TArray<UTexture*> Used;
            Surface->ResolvedOceanMaterialInstance->GetUsedTextures(Used,
                EMaterialQualityLevel::High, false, World->GetFeatureLevel(), false);
            const FString ExpectedTexture = bRadianceAB ? TEXT("T_APS_LavaFineRadianceMoments_v1")
                : bCrustFieldAB && ThermalVariant == 1 ? TEXT("T_APS_LavaCrustField40m_v1") : TEXT("T_APS_LavaThermal400m_RGBA_v1");
            const FString ExpectedPath = TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/SharedLiquid/") + ExpectedTexture + TEXT(".") + ExpectedTexture;
            const bool PackedBound = Used.ContainsByPredicate([&ExpectedPath](const UTexture* T)
                { return T && T->GetPathName() == ExpectedPath; });
            if (!PackedBound) return Finish(TEXT("actual resolved MID does not use pre-integrated thermal texture"));
            if (bRadianceAB && !Used.ContainsByPredicate([](const UTexture* T) { return T && T->GetPathName()==TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/SharedLiquid/T_APS_LavaThermal400m_RGBA_v1.T_APS_LavaThermal400m_RGBA_v1"); }))
                return Finish(TEXT("radiance comparison lost original fine RGB sampler"));
            Test->AddInfo(TEXT("LAVA_THERMAL_MIPS_BINDING actualLiveMIDPackedTexture=1"));
            for (UTexture* Texture : Used)
            {
                auto* T = Cast<UTexture2D>(Texture);
                if (!T || T->GetName() != ExpectedTexture) continue;
#if WITH_EDITOR
                if (T->IsCompiling()) return Finish(TEXT("packed texture still compiling at capture"));
#endif
                Test->AddInfo(FString::Printf(TEXT("LAVA_THERMAL_TEXTURE view=%d variant=%d platform=%dx%d mips=%d resident=%d lodBias=%d maxTextureSize=%d filter=%d"),
                    View, ThermalVariant, T->GetSizeX(), T->GetSizeY(), T->GetNumMips(), T->GetNumResidentMips(),
                    T->GetCachedLODBias(), T->MaxTextureSize, int32(T->Filter)));
#if WITH_EDITORONLY_DATA
                Test->AddInfo(FString::Printf(TEXT("LAVA_THERMAL_SOURCE width=%lld height=%lld"),
                    int64(T->Source.GetSizeX()), int64(T->Source.GetSizeY())));
#endif
            }
        }
        int32 OceanSlots = 0, TerrainSlots = 0;
        auto Check = [&](const TArray<UWorldScapeLod*>& Lods, UMaterialInterface* Expected, int32& Count)
        {
            for (const UWorldScapeLod* Lod : Lods)
            {
                if (!IsValid(Lod) || !IsValid(Lod->Mesh) || !Lod->Mesh->IsRegistered()
                    || !Lod->Mesh->IsVisible() || Lod->Mesh->bHiddenInGame) return false;
                for (int32 I = 0; I < Lod->Mesh->GetNumSections(); ++I)
                {
                    if (Lod->Mesh->GetMaterial(I) != Expected) return false;
                    ++Count;
                }
            }
            return Count > 0;
        };
        if (!Check(Root->WorldScapeLodOcean, Surface->ResolvedOceanMaterialInstance, OceanSlots)
            || !Check(Root->WorldScapeLod, Surface->ResolvedTerrainMaterialInstance, TerrainSlots))
            return Finish(TEXT("actual visible terrain/liquid slots differ from resolved production materials"));
        UGameViewportClient* Client = World->GetGameViewport();
        TArray<FColor> Pixels; FIntVector Size = FIntVector::ZeroValue;
        if (!Client || !Client->GetGameViewportWidget().IsValid()
            || !FSlateApplication::Get().TakeScreenshot(Client->GetGameViewportWidget().ToSharedRef(), Pixels, Size)
            || Size.X <= 0 || Size.Y <= 0 || Pixels.Num() != Size.X * Size.Y)
            return Finish(TEXT("actual game viewport readback failed"));
        const FString Folder = FPaths::ProjectSavedDir() / TEXT("Automation/LavaNaturalShoreline")
            / UEnum::GetValueAsString(Planet->PlanetType).Replace(TEXT("::"), TEXT("_"));
        IFileManager::Get().MakeDirectory(*Folder, true);
        const FString Path = Folder / (bThermalAB
            ? FString::Printf(TEXT("%02d-%s-shore-%.0fm.png"), View,
                bRadianceAB ? (ThermalVariant == 0 ? TEXT("separate-means") : TEXT("joint-radiance"))
                    : bCrustFieldAB ? (ThermalVariant == 0 ? TEXT("cloud-source") : TEXT("plate-source"))
                    : bCrustNormalAB ? (ThermalVariant == 0 ? TEXT("flat-crust") : TEXT("bump-crust-25cm"))
                    : bStochasticAB ? (ThermalVariant == 0 ? TEXT("periodic-40m") : TEXT("stochastic-40m"))
                    : bFinePeriodAB ? (ThermalVariant == 0 ? TEXT("thermal-400m") : TEXT("thermal-40m"))
                    : (ThermalVariant == 0 ? TEXT("native") : TEXT("thermal")), ViewClearanceCm() / 100.0)
            : FString::Printf(TEXT("%02d-shore-%.0fm.png"), View, ViewClearanceCm() / 100.0));
        TArray64<uint8> Png; FImageUtils::PNGCompressImageArray(Size.X, Size.Y, Pixels, Png);
        if (!FFileHelper::SaveArrayToFile(Png, *Path)) return Finish(TEXT("cannot save shoreline frame"));
        Test->AddInfo(FString::Printf(TEXT("LAVA_SHORE_FRAME view=%d heightM=%.3f terrainClearanceM=%.3f oceanSlots=%d terrainSlots=%d observerDeltaCm=%.3f terrainVisible=1 noSubstitution=1 originalRGBA=1 streamingActive=1 profile=%u %s screenshot=%s; not a walking/FPS result"),
            View, ViewClearanceCm()/100.0, ActualClearance/100.0, OceanSlots, TerrainSlots, ObserverDelta, ProfileSignature, *Evidence, *Path));
        if (bThermalAB)
        {
            Test->AddInfo(FString::Printf(TEXT("LAVA_THERMAL_FRAME view=%d strength=%d finePeriodAB=%d stochasticAB=%d stochastic=%d periodCm=%.0f sameLiveMID=1 originalPaletteBrightness=1 candidateOnly=1"),
                View, (bFinePeriodAB || bStochasticAB || HasCrustAB()) ? 1 : ThermalVariant, bFinePeriodAB ? 1 : 0,
                bStochasticAB ? 1 : 0, HasCrustAB() ? 1 : bStochasticAB ? ThermalVariant : 0,
                (bStochasticAB || HasCrustAB() || (bFinePeriodAB && ThermalVariant == 1)) ? 4000.0f : 40000.0f));
            if (bCrustNormalAB) Test->AddInfo(FString::Printf(TEXT("LAVA_CRUST_NORMAL_FRAME view=%d heightCm=%d onlyNormalChanges=1 noAdditionalTextureReads=1"), View, ThermalVariant == 0 ? 0 : 25));
            if (bCrustFieldAB) Test->AddInfo(FString::Printf(TEXT("LAVA_CRUST_FIELD_FRAME view=%d plateSource=%d heightCm=25 stochastic=1 periodCm=4000 paletteFrameMaskUnchanged=1"), View, ThermalVariant));
            if (bRadianceAB) Test->AddInfo(FString::Printf(TEXT("LAVA_RADIANCE_FRAME view=%d joint=%d sameMomentTexture=1 heightCm=25 stochastic=1 periodCm=4000 onlyCovarianceChanges=1"), View, ThermalVariant));
            if (ThermalVariant == 0)
            {
                ThermalVariant = 1;
                ThermalLive->SetScalarParameterValue(TEXT("APS_LavaThermalCoverage"), 1.0f);
                if (bFinePeriodAB) ThermalLive->SetScalarParameterValue(TEXT("APS_LavaFinePeriodCm"), 4000.0f);
                if (bStochasticAB) ThermalLive->SetScalarParameterValue(TEXT("APS_LavaFineStochastic"), 1.0f);
                if (bCrustNormalAB) ThermalLive->SetScalarParameterValue(TEXT("APS_LavaCrustHeightCm"), 25.0f);
                if (bCrustFieldAB && !SetCrustField(true)) return Finish(TEXT("cannot bind private plate field"));
                if (bRadianceAB) ThermalLive->SetScalarParameterValue(TEXT("APS_LavaRadianceIntegration"), 1.0f);
                ThermalApplied = ThermalLive->ScalarParameterValues;
                PlacedAt = 0; Stable = 0;
                return false;
            }
            RestoreThermal();
        }
        ++View; PlacedAt = 0; Stable = 0;
        return View == ViewCount() ? Finish() : false;
    }

public:
    FProbe(FAutomationTestBase* InTest, UWorld* InWorld, APlanet* InPlanet)
        : Test(InTest), World(InWorld), Planet(InPlanet) {}
    ~FProbe() override { Restore(); }

    bool Update() override
    {
        if (bDone) return true;
        const double Now = FPlatformTime::Seconds(); if (Started == 0) Started = Now;
        const double Timeout = ViewCount() == 5 ? 110.0 : 70.0;
        if (Now - Started > Timeout) return Finish(TEXT("shoreline readiness timeout; no visual acceptance"));
        if (!World.IsValid() || !Planet.IsValid()) return Finish(TEXT("world/planet disappeared"));
        if (!bLeased)
        {
            if (bFarShoreline && (bThermalAB || bFinePeriodAB || bStochasticAB || HasCrustAB()
                || FParse::Param(FCommandLine::Get(), TEXT("APSLavaThermalMipAB"))
                || FParse::Param(FCommandLine::Get(), TEXT("APSLavaNearShoreline"))))
                return Finish(TEXT("far shoreline requires unchanged production materials without near/thermal controls"));
            if (HasCrustAB() && (!bThermalAB || bFinePeriodAB || bStochasticAB
                || int32(bCrustNormalAB)+int32(bCrustFieldAB)+int32(bRadianceAB)>1))
                return Finish(TEXT("crust normal comparison requires thermal mode without other comparisons"));
            if (bStochasticAB && (!bThermalAB || bFinePeriodAB))
                return Finish(TEXT("stochastic comparison requires thermal mode without period comparison"));
            if (!FParse::Param(FCommandLine::Get(), TEXT("APSLavaCoverage"))
                || FParse::Param(FCommandLine::Get(), TEXT("APSLavaCoverageOrbit"))
                || FParse::Param(FCommandLine::Get(), TEXT("APSLavaRadianceAB")))
                return Finish(TEXT("shoreline requires natural PlanetSurface continuation, no orbital/candidate mode"));
            Surface = Planet->PlanetaryEnvironmentGenerator;
            Root = Surface.IsValid() ? Surface->WorldScapeRootInstance : nullptr;
            PC = World->GetFirstPlayerController();
            Pawn = PC.IsValid() ? Cast<ACustomGravityCharacter>(PC->GetPawn()) : nullptr;
            if (!Surface.IsValid() || !Root.IsValid() || !PC.IsValid() || !Pawn.IsValid()
                || !PC->PlayerCameraManager || PC->GetViewTarget() != Pawn.Get()
                || !Pawn->GetCharacterMovement() || !Planet->bWorldScapeSurfaceReady
                || Planet->IsManual || !FMath::IsNearlyEqual(Planet->WorldScapePresentationScale, 1.0)
                || !Surface->IsSurfaceProfileCurrent(Planet.Get()) || !Root->bOcean
                || Surface->ResolvedSurfaceProfile.LiquidType != EAPSPlanetLiquidType::Lava
                || Root->bFreezeGeneration || !Root->IsActorTickEnabled())
                return Finish(TEXT("not the ready natural full-scale production lava surface"));
            if (Root->WorldScapeLodInGeneration.Num() > 0) return false;
            if (!FindShore()) return Finish(TEXT("bounded native day-side search did not resolve a wet/dry bracket"));
            ProfileSignature = UAPSPlanetSurfaceProfileResolver::BuildProfileSignature(Surface->ResolvedSurfaceProfile);
            OriginalECEF = Root->WorldToECEF(Pawn->GetActorLocation()).ToFVector();
            OriginalRotation = Pawn->GetActorQuat(); OriginalControl = PC->GetControlRotation();
            OriginalVelocity = Pawn->GetVelocity(); bOriginalZeroG = Pawn->bManualZeroGOverride;
            OriginalView = PC->GetViewTarget(); bLeased = true;
            FActorSpawnParameters Params; Params.ObjectFlags |= RF_Transient;
            Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
            Camera = World->SpawnActor<ACameraActor>(ACameraActor::StaticClass(), Pawn->GetActorLocation(), FRotator::ZeroRotator, Params);
            if (!Camera.IsValid()) return Finish(TEXT("camera allocation failed"));
            const FMinimalViewInfo& Natural = PC->PlayerCameraManager->GetCameraCacheView();
            auto* Optical = Camera->GetCameraComponent();
            Optical->SetFieldOfView(Natural.FOV); Optical->SetAspectRatio(Natural.AspectRatio);
            Optical->SetConstraintAspectRatio(Natural.bConstrainAspectRatio);
            Optical->PostProcessSettings = Natural.PostProcessSettings;
            Optical->PostProcessBlendWeight = Natural.PostProcessBlendWeight;
            Pawn->SetManualZeroGOverride(true);
            PC->SetViewTarget(Camera.Get());
        }
        if (!Root.IsValid() || !Surface.IsValid() || !Pawn.IsValid() || !PC.IsValid() || !Camera.IsValid()
            || Surface->WorldScapeRootInstance != Root.Get() || PC->GetViewTarget() != Camera.Get()
            || PC->GetPawn() != Pawn.Get() || Root->bFreezeGeneration || !Root->IsActorTickEnabled()
            || ProfileSignature != UAPSPlanetSurfaceProfileResolver::BuildProfileSignature(Surface->ResolvedSurfaceProfile))
            return Finish(TEXT("leased observer/root/profile changed"));
        const double H = ViewClearanceCm();
        const double SeaRadius = Root->PlanetScale + Root->OceanHeight;
        const FVector Direction = (Shore * SeaRadius + WetTangent * (H * 1.2)).GetSafeNormal();
        const double TerrainHeight = Height(Direction);
        if (!FMath::IsFinite(TerrainHeight)) return Finish(TEXT("nonfinite native height at shoreline observer"));
        const double Radius = Root->PlanetScale + FMath::Max(double(Root->OceanHeight), TerrainHeight) + H;
        const FVector Location = Root->ECEFToWorld(Direction * Radius).ToFVector();
        const FVector Target = Root->ECEFToWorld(Shore * SeaRadius).ToFVector();
        const FVector Up = Root->GetActorQuat().RotateVector(Direction);
        // Keep the real streaming pawn behind the lens, not around the lens.
        // Nothing is hidden; camera settings and shoreline viewpoint stay exact.
        const FVector Forward = (Target - Location).GetSafeNormal();
        USkeletalMeshComponent* CharacterMesh = Pawn->GetMesh();
        if (!IsValid(CharacterMesh)) return Finish(TEXT("controlled character has no skeletal mesh"));
        // Aggregate actor bounds are not necessarily visual character bounds.
        // Derive clearance from the actual rendered skeletal mesh.
        const double MeshForwardExtent = FVector::DotProduct(
            CharacterMesh->Bounds.Origin - Pawn->GetActorLocation(), Forward)
            + FVector::DotProduct(CharacterMesh->Bounds.BoxExtent, Forward.GetAbs());
        const double PawnBehindCm = FMath::Max(500.0, MeshForwardExtent + 200.0);
        if (!FMath::IsFinite(PawnBehindCm) || PawnBehindCm > 5000.0)
            return Finish(TEXT("unexpected rendered-character bounds; cannot safely frame coast"));
        const FVector PawnLocation = Location - Forward * PawnBehindCm;
        const FVector ObserverDirection = Root->WorldToECEF(PawnLocation).ToFVector().GetSafeNormal();
        Pawn->GetCharacterMovement()->StopMovementImmediately();
        Pawn->SetActorLocation(PawnLocation, false, nullptr, ETeleportType::TeleportPhysics);
        Camera->SetActorLocationAndRotation(Location, FRotationMatrix::MakeFromXZ(Target - Location, Up).ToQuat());
        if (!Pawn->GetActorLocation().Equals(PawnLocation, 1.0)) return Finish(TEXT("actual pawn placement failed"));
        if (FVector::DotProduct(CharacterMesh->Bounds.Origin - Location, Forward)
            + FVector::DotProduct(CharacterMesh->Bounds.BoxExtent, Forward.GetAbs()) >= -10.0)
            return Finish(TEXT("pawn bounds cross the camera plane; refusing obstructed evidence"));
        if (PlacedAt == 0) PlacedAt = Now;
        const FVector Observer = Root->bOverridePlayerPosition ? Root->OverridedPlayerPosition : Root->PlayerWorldPos.ToFVector();
        const double Delta = FVector::Distance(Observer, PawnLocation);
        bool Ready = Planet->bWorldScapeSurfaceReady && !Root->IsHidden() && Delta < 100
            && Root->WorldScapeLodInGeneration.IsEmpty();
        if (Ready)
        {
            Ready = APSWorldScapePayloadValidation::HasExactCenteredPayloadSet(Root->WorldScapeLod,
                Root->MaxLod, false, ObserverDirection, true)
                && APSWorldScapePayloadValidation::HasExactCenteredPayloadSet(Root->WorldScapeLodOcean,
                    Root->OceanMaxLod, true, ObserverDirection, false);
        }
        Stable = Ready ? Stable + 1 : 0;
        if (!Ready || Stable < 12 || Now - PlacedAt < 2.5
            || !PC->PlayerCameraManager->GetCameraLocation().Equals(Location, 10.0))
        {
            if (Now - LastPending > 5)
            {
                LastPending = Now;
                Test->AddInfo(FString::Printf(TEXT("LAVA_SHORE_PENDING view=%d elapsed=%.2f workers=%d terrain=%d/%d ocean=%d/%d observerCm=%.3f stable=%d"),
                    View, Now-Started, Root->WorldScapeLodInGeneration.Num(), Root->WorldScapeLod.Num(), Root->MaxLod,
                    Root->WorldScapeLodOcean.Num(), Root->OceanMaxLod, Delta, Stable));
            }
            return false;
        }
        if (bThermalAB && !ThermalLive.IsValid())
        {
            auto* Material = Surface->ResolvedOceanMaterialInstance;
            float Default = -1;
            if (!Material || !Material->GetScalarParameterValue(
                FHashedMaterialParameterInfo(TEXT("APS_LavaThermalCoverage")), Default) || Default != 0.0f)
                return Finish(TEXT("expected saved thermal trial disabled by default"));
            if ((bFinePeriodAB || bStochasticAB || HasCrustAB()) && (!FParse::Param(FCommandLine::Get(), TEXT("APSLavaThermalMipAB"))
                || !Material->GetScalarParameterValue(FHashedMaterialParameterInfo(TEXT("APS_LavaFinePeriodCm")), Default)
                || Default != 40000.0f)) return Finish(TEXT("fine-period test requires native saved 400m default and packed mip mode"));
            if ((bStochasticAB || HasCrustAB()) && (!Material->GetScalarParameterValue(FHashedMaterialParameterInfo(TEXT("APS_LavaFineStochastic")), Default)
                || Default != 0.0f)) return Finish(TEXT("stochastic candidate must be saved disabled"));
            if (HasCrustAB() && (!Material->GetScalarParameterValue(FHashedMaterialParameterInfo(TEXT("APS_LavaCrustHeightCm")), Default)
                || Default != 0.0f)) return Finish(TEXT("normal candidate must be saved disabled"));
            if (bRadianceAB && (!Material->GetScalarParameterValue(FHashedMaterialParameterInfo(TEXT("APS_LavaRadianceIntegration")), Default)
                || Default != 0.0f)) return Finish(TEXT("radiance candidate must be saved disabled"));
            ThermalSaved.Reset(UMaterialInstanceDynamic::Create(Material->Parent, GetTransientPackage()));
            if (!ThermalSaved.IsValid()) return Finish(TEXT("thermal snapshot allocation failed"));
            ThermalSaved->CopyParameterOverrides(Material);
            ThermalLive = Material;
            ThermalTexturesApplied = Material->TextureParameterValues;
            ThermalVariant = 0;
            Material->SetScalarParameterValue(TEXT("APS_LavaThermalCoverage"), (bFinePeriodAB || bStochasticAB || HasCrustAB()) ? 1.0f : 0.0f);
            if (bFinePeriodAB) Material->SetScalarParameterValue(TEXT("APS_LavaFinePeriodCm"), 40000.0f);
            if (bStochasticAB)
            {
                Material->SetScalarParameterValue(TEXT("APS_LavaFinePeriodCm"), 4000.0f);
                Material->SetScalarParameterValue(TEXT("APS_LavaFineStochastic"), 0.0f);
            }
            if (HasCrustAB())
            {
                Material->SetScalarParameterValue(TEXT("APS_LavaFinePeriodCm"), 4000.0f);
                Material->SetScalarParameterValue(TEXT("APS_LavaFineStochastic"), 1.0f);
                Material->SetScalarParameterValue(TEXT("APS_LavaCrustHeightCm"), (bCrustFieldAB || bRadianceAB) ? 25.0f : 0.0f);
                if ((bCrustFieldAB || bRadianceAB) && !SetCrustField(false)) return Finish(TEXT("cannot bind fine-field control"));
                if (bRadianceAB)
                {
                    Material->SetScalarParameterValue(TEXT("APS_LavaRadianceIntegration"), 0.0f);
                }
            }
            ThermalApplied = Material->ScalarParameterValues;
            PlacedAt = 0; Stable = 0;
            return false;
        }
        return Capture(Delta, Radius - Root->PlanetScale - TerrainHeight);
    }
};
}
#endif
