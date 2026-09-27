#pragma once

#if WITH_DEV_AUTOMATION_TESTS
#include "CoreMinimal.h"
#include "APS_ALPHA/Actors/Astro/Planet.h"
#include "APS_ALPHA/Actors/Tech/SpaceStation.h"
#include "APS_ALPHA/Core/Instances/MainGameplayInstance.h"
#include "APS_ALPHA/Core/Model/SpawnParameters.h"
#include "APS_ALPHA/Generation/AstroGenerator.h"
#include "APS_ALPHA/Generation/PlanetarySurfaceGenerator.h"
#include "APS_ALPHA/Generation/WorldScapePayloadValidation.h"
#include "APS_ALPHA/Pawns/Characters/CustomGravityCharacter.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"

// Opt-in observation lease, not a surface-start fallback. The normal generated
// landing test must still succeed with PlanetSurface and its dry-ground gate.
namespace APSLavaCoverageOrbit
{
enum class EReadiness : uint8 { Pending, Ready, Failed };

class FObserver final
{
    TWeakObjectPtr<ACustomGravityCharacter> Pawn;
    TWeakObjectPtr<APlanet> Planet;
    TWeakObjectPtr<AWorldScapeRoot> Root;
    TWeakObjectPtr<APlayerController> Controller;
    FVector OriginalPlanetOffset = FVector::ZeroVector;
    FQuat OriginalRotation = FQuat::Identity;
    FRotator OriginalControlRotation = FRotator::ZeroRotator;
    FVector OriginalVelocity = FVector::ZeroVector;
    FVector Outward = FVector::ZeroVector;
    double RadiusCm = 0.0, Start = 0.0, PlacedAt = 0.0, LastPending = -1000.0;
    int32 StableFrames = 0;
    bool bLeased = false, bWetSelected = false, bOriginalManualZeroG = false;
    EAPSPlanetLiquidType LiquidType = EAPSPlanetLiquidType::Lava;

    const TCHAR* LogPrefix() const
    {
        return LiquidType == EAPSPlanetLiquidType::Ammonia
            ? TEXT("APS.SharedLiquidCoverage.Orbit") : TEXT("APS.LavaCoverage.Orbit");
    }

public:
    ~FObserver() { Restore(); }

    void Restore()
    {
        if (!bLeased) return;
        bLeased = false;
        if (Pawn.IsValid() && Planet.IsValid())
        {
            Pawn->SetActorLocationAndRotation(Planet->GetActorLocation() + OriginalPlanetOffset,
                OriginalRotation, false, nullptr, ETeleportType::TeleportPhysics);
            Pawn->SetManualZeroGOverride(bOriginalManualZeroG);
            if (UCharacterMovementComponent* Movement = Pawn->GetCharacterMovement())
                Movement->Velocity = OriginalVelocity;
            if (Controller.IsValid()) Controller->SetControlRotation(OriginalControlRotation);
        }
        UE_LOG(LogTemp, Display, TEXT("[%s] observer lease restored; no landing result"), LogPrefix());
    }

    EReadiness Update(UWorld* World, AAstroGenerator* Generator, UClass* SelectedClass,
        EPlanetType ExpectedType, uint32 ExpectedDataset, uint32 ExpectedProfile, FString& Error,
        EAPSPlanetLiquidType ExpectedLiquid = EAPSPlanetLiquidType::Lava)
    {
        Error.Reset();
        const double Now = FPlatformTime::Seconds();
        if (Start == 0.0) Start = Now;
        auto Failed = [this, &Error](const TCHAR* Message)
        {
            Error = FString::Printf(TEXT("[%s] %s"), LogPrefix(), Message);
            return EReadiness::Failed;
        };
        if ((ExpectedLiquid != EAPSPlanetLiquidType::Lava && ExpectedLiquid != EAPSPlanetLiquidType::Ammonia)
            || (bLeased && LiquidType != ExpectedLiquid))
            return Failed(TEXT("unsupported or changed observer liquid family"));
        LiquidType = ExpectedLiquid;
        if (Now - Start > 50.0) return Failed(TEXT("observer/ocean readiness timed out; no coverage or ground-lighting acceptance"));
        if (!IsValid(World) || !IsValid(Generator) || !SelectedClass) return EReadiness::Pending;
        UMainGameplayInstance* State = World->GetGameInstance()
            ? World->GetGameInstance()->GetSubsystem<UMainGameplayInstance>() : nullptr;
        APlayerController* PC = World->GetFirstPlayerController();
        ACustomGravityCharacter* Character = PC ? Cast<ACustomGravityCharacter>(PC->GetPawn()) : nullptr;
        APlanet* Home = Generator->HomePlanet;
        if (!State || !State->SpawnParameters || !State->CurrentCivilization || !PC || !PC->PlayerCameraManager
            || !IsValid(Character) || !Character->IsA(SelectedClass) || !IsValid(Home))
            return EReadiness::Pending;
        const auto& Projection = Generator->GetCanonicalStellarProjectionDescriptor();
        if (!Generator->bGenerateFullScaledWorld || Generator->bIntegrateStartPlanet
            || Generator->ActorHasTag(TEXT("WorldGenerationPreview")) || Home->IsManual
            || State->bIsLoadingMode || State->bUseAuthoredSinglePlayWorld || !State->bSpawnGeneratedCivilization
            || State->SpawnParameters->CharacterSpawnPlace != ECharSpawnPlace::PlanetOrbit
            || Generator->CharSpawnPlace != ECharSpawnPlace::PlanetOrbit
            || !Projection.bConsumedFinalizedDataset || Projection.CanonicalDatasetHash != ExpectedDataset
            || Home->PlanetType != ExpectedType || !FMath::IsNearlyEqual(Home->WorldScapePresentationScale, 1.0))
            return Failed(TEXT("not the committed full-scale Civilization PlanetOrbit route/type/dataset"));
        if (!bLeased)
        {
            ASpaceStation* Station = nullptr;
            for (TActorIterator<ASpaceStation> It(World); It; ++It)
            {
                AActor* Parent = It->GetAttachParentActor();
                if (It->GetClass() == State->SpawnParameters->BP_HomeSpaceStation.Get()
                    && IsValid(Parent) && Parent->GetAttachParentActor() == Home && IsValid(It->SpawnPoint))
                {
                    if (Station) return Failed(TEXT("more than one selected home station matches the orbital spawn anchor"));
                    Station = *It;
                }
            }
            // Wait for the real ResolveSpawnLocation(PlanetOrbit) station anchor,
            // not merely the temporary default pawn created during level travel.
            if (!Station || PC->GetViewTarget() != Character || Character->IsSurfaceHandoffSuspended()
                || FVector::Distance(Character->GetActorLocation(),
                    Station->SpawnPoint->GetComponentLocation()) > 1000.0)
                return EReadiness::Pending;
            Pawn = Character; Planet = Home; Controller = PC;
            OriginalPlanetOffset = Character->GetActorLocation() - Home->GetActorLocation();
            OriginalRotation = Character->GetActorQuat(); OriginalControlRotation = PC->GetControlRotation();
            OriginalVelocity = Character->GetVelocity(); bOriginalManualZeroG = Character->bManualZeroGOverride;
            Outward = OriginalPlanetOffset.GetSafeNormal();
            if (Outward.IsNearlyZero()) return Failed(TEXT("production orbital spawn has no radial direction"));
            RadiusCm = Home->GetWorldScapeBodyRadiusCm() + 10000000.0; // 100 km bootstrap observer.
            bLeased = true;
            Character->SetManualZeroGOverride(true); // Same public control as the real G free-flight toggle.
            UE_LOG(LogTemp, Display,
                TEXT("[%s] route verified type=%d liquid=%d dataset=%u pawn=%s stationSpawn=%s; test-only free-flight observer, natural landing NOT tested"),
                LogPrefix(), static_cast<int32>(ExpectedType), static_cast<int32>(LiquidType), ExpectedDataset, *Character->GetPathName(),
                *Character->GetActorLocation().ToCompactString());
        }
        if (Pawn.Get() != Character || Planet.Get() != Home || Controller.Get() != PC)
            return Failed(TEXT("leased controlled pawn/home planet was replaced"));
        if (!Character->bManualZeroGOverride || !Character->bIsZeroG
            || !Character->GetCharacterMovement() || Character->GetCharacterMovement()->MovementMode != MOVE_Flying)
            return Failed(TEXT("actual production pawn did not retain manual free-flight"));

        APlanetarySurfaceGenerator* Surface = Home->PlanetaryEnvironmentGenerator;
        AWorldScapeRoot* CurrentRoot = IsValid(Surface) ? Surface->WorldScapeRootInstance : nullptr;
        if (IsValid(CurrentRoot) && IsValid(CurrentRoot->WorldScapeNoise) && Surface->IsSurfaceProfileCurrent(Home))
        {
            if (Surface->ResolvedSurfaceProfile.LiquidType != ExpectedLiquid
                || UAPSPlanetSurfaceProfileResolver::BuildProfileSignature(Surface->ResolvedSurfaceProfile) != ExpectedProfile)
                return Failed(TEXT("actual root is not the committed requested liquid profile"));
            if (Root.IsValid() && Root.Get() != CurrentRoot) return Failed(TEXT("production root changed after observer lease"));
            Root = CurrentRoot;
            if (!bWetSelected)
            {
                // This chooses an actual submerged point, never modifies noise,
                // sea level, Hole alpha, or terrain geometry. Keep search bounded.
                const double SeaCm = static_cast<double>(CurrentRoot->OceanHeight);
                bool bFoundWet = false;
                for (int32 Index = 0; Index < 65 && !bFoundWet; ++Index)
                {
                    FVector Direction = Outward;
                    if (Index > 0)
                    {
                        const double Z = 1.0 - 2.0 * (Index - 0.5) / 64.0;
                        const double Angle = (Index - 1) * 2.399963229728653;
                        const double R = FMath::Sqrt(FMath::Max(0.0, 1.0 - Z * Z));
                        Direction = FVector(R * FMath::Cos(Angle), R * FMath::Sin(Angle), Z);
                    }
                    FVector U, V; Direction.FindBestAxisVectors(U, V);
                    bool bWet = FMath::IsFinite(SeaCm);
                    // Centre + four points within the 500 m auxiliary view footprint.
                    for (const FVector& Offset : {FVector::ZeroVector, U * 100000.0, -U * 100000.0,
                        V * 100000.0, -V * 100000.0})
                    {
                        const FVector SampleDirection = (Direction * CurrentRoot->PlanetScale + Offset).GetSafeNormal();
                        const double Height = CurrentRoot->GetGroundHeight(CurrentRoot->GetActorLocation()
                            + SampleDirection * CurrentRoot->PlanetScale, false);
                        bWet &= FMath::IsFinite(Height) && Height < SeaCm - 1000.0;
                    }
                    if (bWet)
                    {
                        Outward = Direction;
                        RadiusCm = CurrentRoot->PlanetScale + SeaCm + 60000.0; // Real pawn 600 m above sea.
                        bFoundWet = true;
                    }
                }
                if (!bFoundWet) return Failed(TEXT("bounded native heightfield search found no submerged ocean patch"));
                bWetSelected = true; PlacedAt = Now;
                UE_LOG(LogTemp, Display, TEXT("[%s] native wet patch direction=%s ocean=%.3fcm pawnClearance=600m root=%s; production streaming owns LOD"),
                    LogPrefix(), *Outward.ToCompactString(), SeaCm, *CurrentRoot->GetPathName());
            }
        }
        const FVector Center = Root.IsValid() ? Root->GetActorLocation() : Home->GetActorLocation();
        const FVector DesiredPawn = Center + Outward * RadiusCm;
        Character->GetCharacterMovement()->StopMovementImmediately();
        Character->SetActorLocation(DesiredPawn, false, nullptr, ETeleportType::TeleportPhysics);
        // A zero-distance MoveComponent can legitimately return false.
        if (!Character->GetActorLocation().Equals(DesiredPawn, 1.0))
            return Failed(TEXT("actual free-flight observer placement failed"));
        if (!bWetSelected || !Root.IsValid()) return EReadiness::Pending;

        const FVector Observer = Root->bOverridePlayerPosition ? Root->OverridedPlayerPosition : Root->PlayerWorldPos.ToFVector();
        const double ObserverDelta = FVector::Distance(Observer, DesiredPawn);
        bool bReady = Home->bWorldScapeSurfaceReady && !Root->IsHidden() && Root->bOcean
            && Root->WorldScapeLodInGeneration.Num() == 0 && ObserverDelta < 100.0;
        // Never inspect worker-owned arrays while generation is in flight.
        if (bReady)
        {
            bReady = APSWorldScapePayloadValidation::HasExactCenteredPayloadSet(
                Root->WorldScapeLodOcean, Root->OceanMaxLod, true,
                Root->WorldToECEF(DesiredPawn).ToFVector().GetSafeNormal(), false);
            for (const UWorldScapeLod* Lod : Root->WorldScapeLodOcean)
                bReady &= IsValid(Lod) && IsValid(Lod->Mesh) && Lod->Mesh->IsRegistered()
                    && Lod->Mesh->IsVisible() && !Lod->Mesh->bHiddenInGame;
        }
        StableFrames = bReady ? StableFrames + 1 : 0;
        if (Now - PlacedAt < 2.0 || StableFrames < 12)
        {
            if (Now - LastPending > 5.0)
            {
                LastPending = Now;
                UE_LOG(LogTemp, Display, TEXT("[%s] pending elapsed=%.2fs ready=%d oceanLods=%d/%d workers=%d observerDelta=%.2fcm frames=%d"),
                    LogPrefix(), Now - Start, Home->bWorldScapeSurfaceReady ? 1 : 0, Root->WorldScapeLodOcean.Num(),
                    Root->OceanMaxLod, Root->WorldScapeLodInGeneration.Num(), ObserverDelta, StableFrames);
            }
            return EReadiness::Pending;
        }
        UE_LOG(LogTemp, Display,
            TEXT("[%s] OBSERVER READY type=%d liquid=%d root=%s oceanLods=%d observerDelta=%.2fcm altitude=%.3fm; entering isolated 2x2 coverage only, no ground lighting/landing/FPS claim"),
            LogPrefix(), static_cast<int32>(ExpectedType), static_cast<int32>(LiquidType), *Root->GetPathName(), Root->WorldScapeLodOcean.Num(), ObserverDelta,
            (FVector::Distance(Character->GetActorLocation(), Center) - Root->PlanetScale - Root->OceanHeight) / 100.0);
        return EReadiness::Ready;
    }
};
}
#endif
