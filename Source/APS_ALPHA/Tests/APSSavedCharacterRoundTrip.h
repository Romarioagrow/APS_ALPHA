#pragma once

#if WITH_EDITOR && WITH_DEV_AUTOMATION_TESTS

#include "CoreMinimal.h"
#include "APS_ALPHA/Pawns/Characters/CustomGravityCharacter.h"
#include "Components/InputComponent.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PhysicsVolume.h"
#include "GameFramework/PlayerController.h"
#include "InputCoreTypes.h"
#include "WorldScapeRoot.h"
#include "WorldScapeMeshComponent.h"

// Exact-save diagnostic only. Caller supplies every-frame ticks and releases on
// every exit. No transform, velocity, camera, collision or terrain writes.
namespace APSSavedCharacterRoundTrip
{
    class FRoute
    {
    public:
        FRoute() = default;
        FRoute(const FRoute&) = delete;
        FRoute& operator=(const FRoute&) = delete;
        ~FRoute() { Release(); }

        bool IsComplete() const { return Stage == EStage::Complete; }
        const FString& GetError() const { return Error; }
        const TCHAR* Phase() const
        {
            if (!Error.IsEmpty()) return TEXT("failed");
            switch (Stage)
            {
            case EStage::FirstDescent: return TEXT("first-natural-descent");
            case EStage::FirstGround: return TEXT("first-ground-hold");
            case EStage::Ascent: return TEXT("native-zero-g-ascent");
            case EStage::SecondDescent: return TEXT("second-natural-descent");
            case EStage::SecondGround: return TEXT("second-ground-hold");
            case EStage::Complete: return TEXT("complete");
            default: return TEXT("uninitialized");
            }
        }

        // true = valid progress (or complete); false = latched error, controls released.
        bool Tick(ACustomGravityCharacter* Character, AWorldScapeRoot* Root, double Elapsed)
        {
            if (!Error.IsEmpty()) return false;
            if (IsComplete()) return true;
            if (!IsInGameThread()) return Fail(TEXT("route requires game thread"));
            if (bReleased) return Fail(TEXT("route was already released"));
            if (!IsValid(Character) || !IsValid(Root) || !FMath::IsFinite(Elapsed) || Elapsed < 0.0)
                return Fail(TEXT("missing character/root or invalid elapsed time"));
            UCharacterMovementComponent* Movement = Character->GetCharacterMovement();
            APlayerController* Controller = Cast<APlayerController>(Character->GetController());
            UWorld* World = Character->GetWorld();
            if (!IsValid(Movement) || !IsValid(Controller) || !IsValid(World)
                || Controller->GetPawn() != Character || Controller->GetWorld() != World || Root->GetWorld() != World
                || Character->IsSurfaceHandoffSuspended() || Character->IsMoveInputIgnored())
                return Fail(TEXT("invalid possession/world/movement, suspended handoff or ignored movement input"));
            if (Controller->IsInputKeyDown(EKeys::LeftShift) || Controller->IsInputKeyDown(EKeys::RightShift))
                return Fail(TEXT("external sprint key held; route cannot own the sprint action"));
            if (Character->GetActorTransform().ContainsNaN() || Root->GetActorTransform().ContainsNaN()
                || !Finite(Character->GetVelocity()) || !Finite(Movement->Velocity)
                || !FMath::IsFinite(Root->PlanetScaleCode) || Root->PlanetScaleCode <= 0.0 || Root->bFlatWorld
                || !Root->GetActorScale3D().Equals(FVector::OneVector, 1.e-6))
                return Fail(TEXT("non-finite physical state or unsupported non-unit/non-spherical root"));
            if (!ValidSpeedSettings(Character, Movement)) return Fail(TEXT("invalid native character speed settings"));
            const FVector FromCenter = Character->GetActorLocation() - Root->GetActorLocation();
            const double Distance = FromCenter.Size();
            const double HeightCm = Distance - Root->PlanetScaleCode;
            const double SpeedCm = Character->GetVelocity().Size();
            if (!FMath::IsFinite(Distance) || Distance <= UE_DOUBLE_SMALL_NUMBER
                || !FMath::IsFinite(HeightCm) || !FMath::IsFinite(SpeedCm))
                return Fail(TEXT("invalid radial height or speed"));

            if (Stage == EStage::Uninitialized)
            {
                if (Character->bManualZeroGOverride || Character->bIsZeroG || !Movement->IsFalling() || HeightCm <= 0.0)
                    return Fail(TEXT("initial saved character must be naturally falling above the sphere, manual Zero-G off"));
                Pawn = Character; Terrain = Root; Player = Controller; SavedWorld = World;
                Input = Character->InputComponent; RadiusCm = Root->PlanetScaleCode; InitialHeightCm = HeightCm;
                StartElapsed = LastElapsed = Elapsed;
                if (!FindSprintBindings(Character)) return Fail(TEXT("expected one owned bound AccelerationBoost press and release"));
                const APhysicsVolume* Volume = Movement->GetPhysicsVolume();
                UE_LOG(LogTemp, Display, TEXT("[APS.SavedCharacterRoundTrip] BEGIN pawn=%s root=%s initialHeightKm=%.9f zeroGSpeedCm=%.3f sprintCm=%.3f..%.3f growthCm=%.3f accelerationCm=%.3f brakingCm=%.3f speedChangeCm=%.3f terminalCm=%.3f fluidFriction=%.6f timeoutSeconds=1100 groundHoldSeconds=10 native-settings-unmodified"),
                    *Character->GetPathName(), *Root->GetPathName(), InitialHeightCm / 100000.0,
                    Character->ZeroGMaxSpeed, Character->ZeroGSprintSpeed, Character->ZeroGSprintMaxSpeed,
                    Character->ZeroGSprintGrowthRate, Character->ZeroGAcceleration, Character->ZeroGBrakingDeceleration,
                    Character->SprintSpeedChangeRate, IsValid(Volume) ? Volume->TerminalVelocity : -1.f,
                    IsValid(Volume) ? Volume->FluidFriction : -1.f);
                SetStage(EStage::FirstDescent, Elapsed);
            }
            if (Pawn.Get() != Character || Terrain.Get() != Root || Player.Get() != Controller
                || SavedWorld.Get() != World || Input.Get() != Character->InputComponent
                || Root->PlanetScaleCode != RadiusCm)
                return Fail(TEXT("character/root/controller/world/input/radius identity changed"));
            if (Elapsed < LastElapsed || Elapsed - StartElapsed > 1100.0)
                return Fail(TEXT("non-monotonic time or 1100-second round-trip timeout"));
            LastElapsed = Elapsed;
            if (Character->bManualZeroGOverride != bOwnManualZeroG)
                return Fail(TEXT("manual Zero-G changed outside route ownership"));

            const UPrimitiveComponent* Floor = Movement->CurrentFloor.HitResult.GetComponent();
            const bool bGrounded = Movement->IsMovingOnGround();
            const bool bNativeFloor = IsValid(Floor) && Floor->IsA<UWorldScapeMeshComponent>()
                && Floor->GetOwner() == Root && Floor->IsRegistered() && Floor->IsQueryCollisionEnabled()
                && Floor->GetCollisionResponseToChannel(ECC_Pawn) == ECR_Block
                && Movement->CurrentFloor.IsWalkableFloor() && Movement->CurrentFloor.HitResult.bBlockingHit;
            if (bGrounded && !bNativeFloor)
                return Fail(TEXT("grounded on a floor other than the requested root's valid WorldScape collision"));
            if (Elapsed >= NextHeartbeat)
            {
                NextHeartbeat = Elapsed + 5.0;
                UE_LOG(LogTemp, Display, TEXT("[APS.SavedCharacterRoundTrip] t=%.3f phase=%s heightKm=%.9f speedMps=%.6f radialMps=%.6f movement=%d manualZeroG=%d grounded=%d floor=%s floorOwner=%s stableSeconds=%.3f maxFlyCm=%.3f"),
                    Elapsed - StartElapsed, Phase(), HeightCm / 100000.0, SpeedCm / 100.0,
                    FVector::DotProduct(Character->GetVelocity(), FromCenter / Distance) / 100.0,
                    static_cast<int32>(Movement->MovementMode), Character->bManualZeroGOverride,
                    bGrounded, *GetPathNameSafe(Floor), *GetPathNameSafe(IsValid(Floor) ? Floor->GetOwner() : nullptr),
                    GroundSince >= 0.0 ? Elapsed - GroundSince : 0.0, Movement->MaxFlySpeed);
            }

            if (Stage == EStage::Ascent)
            {
                if (!Character->bIsZeroG || Movement->MovementMode != MOVE_Flying)
                    return Fail(TEXT("native Zero-G ascent lost its flying movement mode"));
                if (HeightCm >= InitialHeightCm)
                {
                    ReleaseOwnedControls();
                    if (Character->bManualZeroGOverride || Character->bIsZeroG || !Movement->IsFalling())
                        return Fail(TEXT("native gravity did not resume falling at the original height"));
                    SetStage(EStage::SecondDescent, Elapsed);
                }
                else Character->AddMovementInput(FromCenter / Distance, 1.0f, false);
                return true;
            }
            if (Character->bIsZeroG || (!Movement->IsFalling() && !bGrounded))
                return Fail(TEXT("natural descent/ground hold left native falling/walking movement"));
            const bool bFirst = Stage == EStage::FirstDescent || Stage == EStage::FirstGround;
            if (bGrounded && bNativeFloor && SpeedCm <= 10.0)
            {
                if (GroundSince < 0.0)
                {
                    GroundSince = Elapsed;
                    SetStage(bFirst ? EStage::FirstGround : EStage::SecondGround, Elapsed);
                }
                if (Elapsed - GroundSince >= 10.0)
                {
                    UE_LOG(LogTemp, Display, TEXT("[APS.SavedCharacterRoundTrip] LANDING_CONFIRMED landing=%d t=%.3f stableSeconds=%.3f floor=%s owner=%s heightKm=%.9f speedCm=%.6f collision-observation-not-visual-acceptance"),
                        bFirst ? 1 : 2, Elapsed - StartElapsed, Elapsed - GroundSince,
                        *GetPathNameSafe(Floor), *Root->GetPathName(), HeightCm / 100000.0, SpeedCm);
                    if (!bFirst) { SetStage(EStage::Complete, Elapsed); Release(); return true; }
                    if (HeightCm >= InitialHeightCm - 100.0)
                        return Fail(TEXT("first landing leaves no meaningful ascent to the starting height"));
                    if (!FindSprintBindings(Character)) return Fail(TEXT("sprint bindings changed before ascent"));
                    bOwnManualZeroG = true;
                    Character->SetManualZeroGOverride(true);
                    bOwnSprint = true;
                    SprintPressed.Execute(EKeys::LeftShift);
                    GroundSince = -1.0;
                    SetStage(EStage::Ascent, Elapsed);
                    Character->AddMovementInput(FromCenter / Distance, 1.0f, false);
                }
            }
            else
            {
                GroundSince = -1.0;
                SetStage(bFirst ? EStage::FirstDescent : EStage::SecondDescent, Elapsed);
            }
            return true;
        }

        // Idempotent; never restores another actor or cancels unrelated movement.
        // Already-queued AddMovementInput is consumed normally by CharacterMovement.
        void Release()
        {
            if (!IsInGameThread() || bReleased) return;
            ReleaseOwnedControls();
            bReleased = true;
        }

    private:
        enum class EStage : uint8 { Uninitialized, FirstDescent, FirstGround, Ascent, SecondDescent, SecondGround, Complete };

        static bool Finite(const FVector& V)
        { return FMath::IsFinite(V.X) && FMath::IsFinite(V.Y) && FMath::IsFinite(V.Z); }

        static bool ValidSpeedSettings(const ACustomGravityCharacter* C, const UCharacterMovementComponent* M)
        {
            return FMath::IsFinite(C->ZeroGMaxSpeed) && C->ZeroGMaxSpeed > 0.f
                && FMath::IsFinite(C->ZeroGSprintSpeed) && C->ZeroGSprintSpeed > 0.f
                && FMath::IsFinite(C->ZeroGSprintMaxSpeed) && C->ZeroGSprintMaxSpeed >= C->ZeroGSprintSpeed
                && FMath::IsFinite(C->ZeroGSprintGrowthRate) && C->ZeroGSprintGrowthRate >= 0.f
                && FMath::IsFinite(C->ZeroGAcceleration) && C->ZeroGAcceleration > 0.f
                && FMath::IsFinite(C->ZeroGBrakingDeceleration) && C->ZeroGBrakingDeceleration >= 0.f
                && FMath::IsFinite(C->SprintSpeedChangeRate) && C->SprintSpeedChangeRate > 0.f
                && FMath::IsFinite(M->MaxFlySpeed) && FMath::IsFinite(M->MaxAcceleration);
        }

        bool FindSprintBindings(ACustomGravityCharacter* Character)
        {
            UInputComponent* Component = Character->InputComponent;
            if (!IsValid(Component) || Component != Input.Get() || Component->GetOwner() != Character) return false;
            int32 PressCount = 0, ReleaseCount = 0;
            for (int32 I = 0; I < Component->GetNumActionBindings(); ++I)
            {
                const FInputActionBinding& Binding = Component->GetActionBinding(I);
                if (Binding.GetActionName() != FName(TEXT("AccelerationBoost"))) continue;
                if (!Binding.ActionDelegate.IsBoundToObject(Character)) return false;
                if (Binding.KeyEvent == IE_Pressed) { ++PressCount; SprintPressed = Binding.ActionDelegate; }
                else if (Binding.KeyEvent == IE_Released) { ++ReleaseCount; SprintReleased = Binding.ActionDelegate; }
                else return false;
            }
            return PressCount == 1 && ReleaseCount == 1;
        }

        void ReleaseOwnedControls()
        {
            if (ACustomGravityCharacter* Character = Pawn.Get())
            {
                if (bOwnSprint && SprintReleased.IsBoundToObject(Character)) SprintReleased.Execute(EKeys::LeftShift);
                if (bOwnManualZeroG && Character->bManualZeroGOverride) Character->SetManualZeroGOverride(false);
            }
            bOwnSprint = bOwnManualZeroG = false;
        }

        void SetStage(EStage NewStage, double Elapsed)
        {
            if (Stage == NewStage) return;
            Stage = NewStage;
            NextHeartbeat = Elapsed;
            UE_LOG(LogTemp, Display, TEXT("[APS.SavedCharacterRoundTrip] PHASE t=%.3f phase=%s"), Elapsed - StartElapsed, Phase());
        }

        bool Fail(const TCHAR* Reason)
        {
            if (Error.IsEmpty()) Error = Reason;
            Release();
            return false;
        }

        TWeakObjectPtr<ACustomGravityCharacter> Pawn;
        TWeakObjectPtr<AWorldScapeRoot> Terrain;
        TWeakObjectPtr<APlayerController> Player;
        TWeakObjectPtr<UWorld> SavedWorld;
        TWeakObjectPtr<UInputComponent> Input;
        FInputActionUnifiedDelegate SprintPressed, SprintReleased;
        FString Error;
        EStage Stage = EStage::Uninitialized;
        double StartElapsed = 0.0, LastElapsed = 0.0, InitialHeightCm = 0.0, RadiusCm = 0.0;
        double GroundSince = -1.0, NextHeartbeat = 0.0;
        bool bOwnManualZeroG = false, bOwnSprint = false, bReleased = false;
    };
}

#endif
