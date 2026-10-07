#pragma once
#if WITH_DEV_AUTOMATION_TESTS
#include "APS_ALPHA/Generation/APSWorldScapePlanetNoise.h"
#include "APS_ALPHA/Generation/WorldScapePayloadValidation.h"
#include "APS_ALPHA/Pawns/Characters/CustomGravityCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "APS_ALPHA/Core/Planetary/APSUnifiedLavaAssets.h"

// Opt-in test observer only. No ship, physics policy, material or mesh mutation.
namespace APSUnifiedLavaProbe
{
class FProbe final : public IAutomationLatentCommand
{
    FAutomationTestBase* Test;
    TWeakObjectPtr<UWorld> World;
    TWeakObjectPtr<APlanet> Planet;
    TWeakObjectPtr<AWorldScapeRoot> Root;
    TWeakObjectPtr<APlayerController> PC;
    TWeakObjectPtr<ACustomGravityCharacter> Pawn;
    TWeakObjectPtr<ACameraActor> Camera;
    FVector OriginalECEF = FVector::ZeroVector, OriginalVelocity = FVector::ZeroVector;
    FVector Wet = FVector::ZeroVector, Tangent = FVector::ZeroVector;
    FQuat OriginalRotation = FQuat::Identity;
    bool OriginalZeroG = false, Leased = false, Done = false, MaterialParametersLogged = false;
    double Started = 0, Placed = 0, FrameSum = 0;
    int32 View = 0, Stable = 0, FrameCount = 0;
    static constexpr double Heights[] = {30000000, 1500000, 150000, 15000, 300};

    void Restore()
    {
        if (!Leased) return;
        Leased = false;
        if (PC.IsValid() && Camera.IsValid() && PC->GetViewTarget() == Camera.Get() && Pawn.IsValid()) PC->SetViewTarget(Pawn.Get());
        if (Pawn.IsValid() && Root.IsValid())
        {
            Pawn->SetActorLocationAndRotation(Root->ECEFToWorld(OriginalECEF).ToFVector(), OriginalRotation, false, nullptr, ETeleportType::TeleportPhysics);
            Pawn->SetManualZeroGOverride(OriginalZeroG);
            if (Pawn->GetCharacterMovement()) Pawn->GetCharacterMovement()->Velocity = OriginalVelocity;
        }
        if (Camera.IsValid()) Camera->Destroy();
    }
    bool Finish(const FString& Error = FString())
    {
        if (!Error.IsEmpty()) Test->AddError(TEXT("[APS.UnifiedLavaProbe] ") + Error);
        Restore(); Done = true; return true;
    }
    void LogMaterialParameters(APlanetarySurfaceGenerator* Surface)
    {
        if (MaterialParametersLogged) return;
        MaterialParametersLogged = true;
        auto* Material = Cast<UMaterialInstanceDynamic>(Root->TerrainMaterial.DefaultMaterial);
        Test->AddInfo(FString::Printf(TEXT("UNIFIED_MATERIAL bound=%s mid=%d parent=%s master=%s matchesResolved=%d"),
            *GetPathNameSafe(Root->TerrainMaterial.DefaultMaterial), Material != nullptr,
            *GetPathNameSafe(Material ? Material->Parent.Get() : nullptr),
            *GetPathNameSafe(Material ? Material->GetMaterial() : nullptr),
            Material && Material == Surface->ResolvedTerrainMaterialInstance));
        if (!Material) return;
        // Source contract, not a claim of runtime editor-graph inspection:
        // APSUnifiedLavaSurfaceBuilder rejects a source lava normal and supplies VertexNormalWS.
        Test->AddInfo(FString::Printf(TEXT("UNIFIED_MATERIAL detailCandidate=%d builderPureLavaNormal=%s runtimeNormalGraphInspected=0"),
            APSUnifiedLavaAssets::DetailCandidate(), APSUnifiedLavaAssets::DetailCandidate() ? TEXT("filtered-detail-candidate") : TEXT("geometric-only")));
        TArray<FMaterialParameterInfo> Infos;
        TArray<FGuid> Ids;
        int32 Matched = 0, Logged = 0;
        constexpr int32 MaxLoggedParameters = 40;
        for (int32 Kind = 0; Kind < 2; ++Kind)
        {
            Infos.Reset(); Ids.Reset();
            if (Kind == 0) Material->GetAllScalarParameterInfo(Infos, Ids);
            else Material->GetAllVectorParameterInfo(Infos, Ids);
            for (const FMaterialParameterInfo& Info : Infos)
            {
                if (!Info.Name.ToString().StartsWith(TEXT("APS_UL_"))
                    && !Info.Name.ToString().StartsWith(TEXT("APS_UnifiedDetail"))) continue;
                ++Matched;
                if (Logged >= MaxLoggedParameters) continue;
                FString Value = TEXT("unavailable");
                bool Readable = false;
                if (Kind == 0)
                {
                    float Scalar = 0.0f;
                    Readable = Material->GetScalarParameterValue(Info, Scalar);
                    if (Readable) Value = FString::Printf(TEXT("%.9g"), Scalar);
                }
                else
                {
                    FLinearColor Vector = FLinearColor::Black;
                    Readable = Material->GetVectorParameterValue(Info, Vector);
                    if (Readable) Value = FString::Printf(TEXT("(%.9g,%.9g,%.9g,%.9g)"), Vector.R, Vector.G, Vector.B, Vector.A);
                }
                Test->AddInfo(FString::Printf(TEXT("UNIFIED_PARAMETER kind=%s name=%s association=%d index=%d readable=%d value=%s"),
                    Kind == 0 ? TEXT("scalar") : TEXT("vector"), *Info.Name.ToString(),
                    int32(Info.Association), Info.Index, Readable, *Value));
                ++Logged;
            }
        }
        Test->AddInfo(FString::Printf(TEXT("UNIFIED_PARAMETERS matched=%d logged=%d omitted=%d"), Matched, Logged, Matched - Logged));
    }
    bool FindBasin(APlanetarySurfaceGenerator* Surface)
    {
        Wet = Tangent = FVector::ZeroVector;
        const auto* ResolvedNoise = Cast<UAPSWorldScapePlanetNoise>(Root->WorldScapeNoise);
        const FVector Start = Root->WorldToECEF(Pawn->GetActorLocation()).ToFVector().GetSafeNormal();
        const double Sea = double(Root->OceanHeight), Threshold = Sea - 10000.0;
        if (!ResolvedNoise || Start.ContainsNaN() || Start.IsNearlyZero()
            || !FMath::IsFinite(Sea) || !FMath::IsFinite(double(Root->PlanetScale)) || Root->PlanetScale <= 0.0)
        {
            Test->AddError(TEXT("[APS.UnifiedLavaProbe] basin sampling requires a finite ECEF direction/radius/datum and the owning resolved noise"));
            return false;
        }
        FVector U, V; Start.FindBestAxisVectors(U, V);
        CustomNoise State = Root->PlanetNoise;
        const bool bCoastalRelief = ResolvedNoise->UsesCoastalReliefCandidate();
        double Minimum = TNumericLimits<double>::Max(), BestAccepted = Threshold;
        int32 LocalSamples = 0, GlobalSamples = 0, FiniteSamples = 0, NonfiniteSamples = 0;
        bool Found = false, SelectedGlobally = false;
        const auto Sample = [&](const FVector& Direction, bool Global)
        {
            Global ? ++GlobalSamples : ++LocalSamples;
            const FVector D = Direction.GetSafeNormal();
            if (D.ContainsNaN() || D.IsNearlyZero()) { ++NonfiniteSamples; return; }
            // Raw root-local ECEF field, before the unified sea-level envelope.
            const double H = UAPSWorldScapePlanetNoise::SampleHeightResolvedProfile(Surface->ResolvedSurfaceProfile,
                State, DVector(D * Root->PlanetScale), DVector(0,0,0), Root->NoiseScale, Root->NoiseIntensity,
                Root->PlanetScale, D.Z, bCoastalRelief);
            if (!FMath::IsFinite(H)) { ++NonfiniteSamples; return; }
            ++FiniteSamples;
            Minimum = FMath::Min(Minimum, H);
            if (H < BestAccepted) { BestAccepted = H; Wet = D; Found = true; SelectedGlobally = Global; }
        };
        for (int32 Ring = 0; Ring < 5; ++Ring)
            for (int32 I = 0; I < 24; ++I)
            {
                const double Arc = FMath::DegreesToRadians(2.0 + Ring * 8.0), A = I * 2.0 * PI / 24.0;
                const FVector D = Start * FMath::Cos(Arc) + (U * FMath::Cos(A) + V * FMath::Sin(A)) * FMath::Sin(Arc);
                Sample(D, false);
            }
        // A deliberately dry spawn's neighbourhood need not contain a deep basin.
        // Fixed whole-sphere fallback observes the same fixture without changing it.
        if (!Found && NonfiniteSamples == 0)
        {
            constexpr int32 GlobalSampleCount = 1024;
            const double GoldenAngle = PI * (3.0 - FMath::Sqrt(5.0));
            for (int32 I = 0; I < GlobalSampleCount; ++I)
            {
                const double Z = 1.0 - 2.0 * (I + 0.5) / GlobalSampleCount;
                const double R = FMath::Sqrt(FMath::Max(0.0, 1.0 - Z * Z)), A = I * GoldenAngle;
                Sample(FVector(R * FMath::Cos(A), R * FMath::Sin(A), Z), true);
            }
        }
        if (NonfiniteSamples) Found = false; // Never accept a partially corrupt search.
        if (Found) Wet.FindBestAxisVectors(Tangent, V);
        const FString MinimumText = FiniteSamples ? FString::Printf(TEXT("%.3f"), Minimum) : TEXT("none");
        const FString SelectedText = Found ? FString::Printf(TEXT("%.3f"), BestAccepted) : TEXT("none");
        const FString DirectionText = Found ? Wet.ToString() : TEXT("none");
        Test->AddInfo(FString::Printf(TEXT("UNIFIED_BASIN found=%d source=%s localSamples=%d globalSamples=%d sampledCount=%d finiteSamples=%d nonfiniteSamples=%d minRawHeightCm=%s thresholdCm=%.3f seaCm=%.3f selectedRawHeightCm=%s direction=%s"),
            Found, NonfiniteSamples ? TEXT("invalid-samples") : Found ? (SelectedGlobally ? TEXT("global") : TEXT("local")) : TEXT("none"),
            LocalSamples, GlobalSamples, LocalSamples + GlobalSamples, FiniteSamples, NonfiniteSamples,
            *MinimumText, Threshold, Sea, *SelectedText, *DirectionText));
        return Found;
    }
public:
    FProbe(FAutomationTestBase* T, UWorld* W, APlanet* P) : Test(T), World(W), Planet(P) {}
    ~FProbe() override { Restore(); }
    bool Update() override
    {
        if (Done) return true;
        const double Now = FPlatformTime::Seconds(); if (!Started) Started = Now;
        if (Now - Started > 100) return Finish(TEXT("bounded observer timeout"));
        if (!World.IsValid() || !Planet.IsValid()) return Finish(TEXT("world lost"));
        auto* Surface = Planet->PlanetaryEnvironmentGenerator;
        if (!Leased)
        {
            Root = Surface ? Surface->WorldScapeRootInstance : nullptr;
            PC = World->GetFirstPlayerController();
            Pawn = PC.IsValid() ? Cast<ACustomGravityCharacter>(PC->GetPawn()) : nullptr;
            if (!Root.IsValid() || !Pawn.IsValid() || !PC->PlayerCameraManager || !Pawn->GetCharacterMovement()
                || PC->GetViewTarget() != Pawn.Get() || !Planet->bWorldScapeSurfaceReady
                || Surface->ResolvedSurfaceProfile.LiquidType != EAPSPlanetLiquidType::Lava)
                return Finish(TEXT("not a ready full-scale lava fixture"));
            if (!Root->WorldScapeLodInGeneration.IsEmpty()) return false;
            if (!FindBasin(Surface)) return Finish(TEXT("no original submerged basin found"));
            OriginalECEF = Root->WorldToECEF(Pawn->GetActorLocation()).ToFVector();
            OriginalRotation = Pawn->GetActorQuat(); OriginalVelocity = Pawn->GetVelocity(); OriginalZeroG = Pawn->bManualZeroGOverride;
            Leased = true;
            FActorSpawnParameters Params; Params.ObjectFlags |= RF_Transient;
            Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
            Camera = World->SpawnActor<ACameraActor>(Params);
            if (!Camera.IsValid()) return Finish(TEXT("camera unavailable"));
            const auto& Natural = PC->PlayerCameraManager->GetCameraCacheView();
            auto* Lens = Camera->GetCameraComponent(); Lens->SetFieldOfView(Natural.FOV);
            Lens->PostProcessSettings = Natural.PostProcessSettings; Lens->PostProcessBlendWeight = Natural.PostProcessBlendWeight;
            Pawn->SetManualZeroGOverride(true); PC->SetViewTarget(Camera.Get());
        }
        if (!Camera.IsValid() || !Root.IsValid() || !Pawn.IsValid() || Surface->WorldScapeRootInstance != Root.Get()) return Finish(TEXT("lease lost"));
        const double Sea = Root->PlanetScale + Root->OceanHeight;
        const FVector Location = Root->ECEFToWorld(Wet * (Sea + Heights[View])).ToFVector();
        const FVector Up = Root->GetActorQuat().RotateVector(Wet);
        const FVector Side = Root->GetActorQuat().RotateVector(Tangent);
        // Vertical approach views, then a near-surface horizon view. Actual pawn
        // follows the camera so WorldScape streams the observed area.
        const FVector Forward = View == 4 ? (Side - Up * 0.22).GetSafeNormal() : -Up;
        const FVector LensUp = View == 4 ? Up : Side;
        const FVector PawnLocation = Location - Forward * 700.0;
        Pawn->GetCharacterMovement()->StopMovementImmediately();
        Pawn->SetActorLocation(PawnLocation, false, nullptr, ETeleportType::TeleportPhysics);
        Camera->SetActorLocationAndRotation(Location, FRotationMatrix::MakeFromXZ(Forward, LensUp).ToQuat());
        if (!Placed) Placed = Now;
        const FVector Observer = Root->bOverridePlayerPosition ? Root->OverridedPlayerPosition : Root->PlayerWorldPos.ToFVector();
        const double ObserverDelta = FVector::Distance(Observer, PawnLocation);
        const FVector D = Root->WorldToECEF(PawnLocation).ToFVector().GetSafeNormal();
        const bool Ready = ObserverDelta < 100 && Root->WorldScapeLodInGeneration.IsEmpty()
            && APSWorldScapePayloadValidation::HasExactCenteredPayloadSet(Root->WorldScapeLod, Root->MaxLod, false, D, true);
        Stable = Ready ? Stable + 1 : 0;
        if (Ready && Now - Placed > 1) { FrameSum += World->GetDeltaSeconds(); ++FrameCount; }
        if (Stable < 12 || Now - Placed < 3.0 || !PC->PlayerCameraManager->GetCameraLocation().Equals(Location, 10.0)) return false;
        const bool Unified = Surface->ResolvedTerrainMaterialInstance
            && GetPathNameSafe(Surface->ResolvedTerrainMaterialInstance->GetMaterial()) == APSUnifiedLavaAssets::MasterPath();
        if (!Unified && FParse::Param(FCommandLine::Get(), TEXT("APSRequireUnifiedLava")))
            return Finish(TEXT("required default UnifiedLava material is not bound; fallback is not acceptance"));
        if (Unified)
        {
            LogMaterialParameters(Surface);
            if (APSUnifiedLavaAssets::DetailCandidate())
            {
                float Strength = -1.0f;
                if (!Surface->ResolvedTerrainMaterialInstance->GetScalarParameterValue(
                        FHashedMaterialParameterInfo(TEXT("APS_UnifiedDetailStrength")), Strength)
                    || !FMath::IsNearlyEqual(Strength, APSUnifiedLavaAssets::DetailStrength(), 1.e-6f))
                    return Finish(TEXT("detail candidate strength does not match explicit A/B request"));
            }
            float NormalStart = 0.0f, NormalEnd = 0.0f;
            if (!Surface->ResolvedTerrainMaterialInstance->GetScalarParameterValue(FHashedMaterialParameterInfo(TEXT("APS_FarNormalStartCm")), NormalStart)
                || !Surface->ResolvedTerrainMaterialInstance->GetScalarParameterValue(FHashedMaterialParameterInfo(TEXT("APS_FarNormalEndCm")), NormalEnd)
                || NormalStart != 200000.0f || NormalEnd != 2000000.0f)
                return Finish(TEXT("unified rock lost accepted 2..20 km normal filter"));
        }
        if (Unified && (Root->bOcean || !Root->WorldScapeLodOcean.IsEmpty())) return Finish(TEXT("unified root retained second ocean mesh"));
        if (View == 4)
        {
            FHitResult Hit;
            FCollisionQueryParams Query(SCENE_QUERY_STAT(UnifiedLavaProbe), false, Pawn.Get());
            const FVector Top = Root->ECEFToWorld(Wet * (Sea + 5000)).ToFVector();
            const FVector Bottom = Root->ECEFToWorld(Wet * (Sea - 5000)).ToFVector();
            const bool Contact = World->LineTraceSingleByChannel(Hit, Top, Bottom, ECC_Visibility, Query);
            const double ErrorCm = Contact ? Root->WorldToECEF(Hit.ImpactPoint).Lenght() - Sea : -1e9;
            Test->AddInfo(FString::Printf(TEXT("UNIFIED_CONTACT unified=%d hit=%d owner=%s radialErrorCm=%.3f"), Unified, Contact, *GetNameSafe(Hit.GetActor()), ErrorCm));
            if (Unified && (!Contact || Hit.GetActor() != Root.Get() || FMath::Abs(ErrorCm) > 100)) return Finish(TEXT("collision does not match visible lava datum"));
        }
        UGameViewportClient* Client = World->GetGameViewport(); TArray<FColor> Pixels; FIntVector Size = FIntVector::ZeroValue;
        if (!Client || !Client->GetGameViewportWidget().IsValid()
            || !FSlateApplication::Get().TakeScreenshot(Client->GetGameViewportWidget().ToSharedRef(), Pixels, Size)
            || Size.X <= 0 || Size.Y <= 0 || Pixels.Num() != Size.X * Size.Y) return Finish(TEXT("viewport capture failed"));
        FString Folder; FParse::Value(FCommandLine::Get(), TEXT("APSUnifiedEvidence="), Folder);
        if (Folder.IsEmpty()) return Finish(TEXT("explicit evidence directory required"));
        IFileManager::Get().MakeDirectory(*Folder, true);
        const FString Path = Folder / FString::Printf(TEXT("view-%d.png"), View);
        TArray64<uint8> Png; FImageUtils::PNGCompressImageArray(Size.X, Size.Y, Pixels, Png);
        if (!FFileHelper::SaveArrayToFile(Png, *Path)) return Finish(TEXT("save failed"));
        Test->AddInfo(FString::Printf(TEXT("UNIFIED_FRAME view=%d unified=%d heightM=%.1f observerCm=%.2f ocean=%d frameMeanMs=%.3f samples=%d path=%s; settled views, not continuous descent"),
            View, Unified, Heights[View]/100, ObserverDelta, Root->bOcean, FrameCount ? 1000*FrameSum/FrameCount : 0, FrameCount, *Path));
        ++View; Placed = 0; Stable = 0; FrameSum = 0; FrameCount = 0;
        return View == 5 ? Finish() : false;
    }
};
}
#endif
