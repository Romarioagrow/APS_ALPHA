#pragma once
#if WITH_DEV_AUTOMATION_TESTS
#include "APS_ALPHA/Generation/APSWorldScapePlanetNoise.h"
#include "APS_ALPHA/Generation/WorldScapePayloadValidation.h"
#include "APS_ALPHA/Pawns/Characters/CustomGravityCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"

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
    FVector OriginalECEF, OriginalVelocity, Wet, Tangent;
    FQuat OriginalRotation;
    bool OriginalZeroG = false, Leased = false, Done = false;
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
    bool FindBasin(APlanetarySurfaceGenerator* Surface)
    {
        const FVector Start = Root->WorldToECEF(Pawn->GetActorLocation()).ToFVector().GetSafeNormal();
        FVector U, V; Start.FindBestAxisVectors(U, V);
        CustomNoise State; State.SetSeed(Root->Seed + 1); State.SetSeed(Root->Seed);
        double Best = double(Root->OceanHeight) - 10000;
        bool Found = false;
        for (int32 Ring = 0; Ring < 5; ++Ring)
            for (int32 I = 0; I < 24; ++I)
            {
                const double Arc = FMath::DegreesToRadians(2.0 + Ring * 8.0), A = I * 2.0 * PI / 24.0;
                const FVector D = Start * FMath::Cos(Arc) + (U * FMath::Cos(A) + V * FMath::Sin(A)) * FMath::Sin(Arc);
                const double H = UAPSWorldScapePlanetNoise::SampleHeightResolvedProfile(Surface->ResolvedSurfaceProfile,
                    State, DVector(D * Root->PlanetScale), DVector(0,0,0), Root->NoiseScale, Root->NoiseIntensity, Root->PlanetScale, D.Z);
                if (H < Best) { Best = H; Wet = D; Found = true; }
            }
        Wet.FindBestAxisVectors(Tangent, V);
        Test->AddInfo(FString::Printf(TEXT("UNIFIED_BASIN found=%d rawHeightCm=%.3f seaCm=%.3f direction=%s"), Found, Best, Root->OceanHeight, *Wet.ToString()));
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
        const bool Unified = Surface->ResolvedTerrainMaterialInstance && Surface->ResolvedTerrainMaterialInstance->GetMaterial()->GetName() == TEXT("M_APS_UnifiedLavaSurface");
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
