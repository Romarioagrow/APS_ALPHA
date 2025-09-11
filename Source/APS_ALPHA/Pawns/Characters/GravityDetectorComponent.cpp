#include "GravityDetectorComponent.h"

#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/ArrowComponent.h"
#include "Engine/Engine.h"

// Зоны – поправь пути под свой проект
#include "APS_ALPHA/Gameplay/Gravity/PlanetGravityActor.h"
#include "APS_ALPHA/Gameplay/Gravity/SpaceshipGravityActor.h"
#include "APS_ALPHA/Gameplay/Gravity/StationGravityActor.h"

namespace
{
    FORCEINLINE FRotator GetOrientationRot(const ACharacter* OwnerChar, const USceneComponent* Ori)
    {
        if (Ori) return Ori->GetComponentRotation();
        return OwnerChar ? OwnerChar->GetActorRotation() : FRotator::ZeroRotator;
    }

    FORCEINLINE FVector AxisX(const FRotator& R) { return FRotationMatrix(R).GetScaledAxis(EAxis::X); }
    FORCEINLINE FVector AxisY(const FRotator& R) { return FRotationMatrix(R).GetScaledAxis(EAxis::Y); }
    FORCEINLINE FVector AxisZ(const FRotator& R) { return FRotationMatrix(R).GetScaledAxis(EAxis::Z); }
}

/* ===================== Конструктор ===================== */

UGravityDetectorComponent::UGravityDetectorComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

/* ===================== BeginPlay / EndPlay ===================== */

void UGravityDetectorComponent::BeginPlay()
{
    Super::BeginPlay();

    OwnerCharacter = Cast<ACharacter>(GetOwner());
    if (OwnerCharacter.IsValid())
    {
        OwnerCapsule = OwnerCharacter->GetCapsuleComponent();
    }

    BindOwnerOverlap();

    // Начинаем с ZeroG
    SetGravityType(EGravityType::ZeroG, nullptr);
}

void UGravityDetectorComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (BoundCollision.IsValid())
    {
        BoundCollision->OnComponentBeginOverlap.RemoveAll(this);
        BoundCollision->OnComponentEndOverlap.RemoveAll(this);
    }
    Super::EndPlay(EndPlayReason);
}

/* ===================== Bind overlaps ===================== */

void UGravityDetectorComponent::BindOwnerOverlap()
{
    AActor* Owner = GetOwner();
    if (!Owner) return;

    if (OwnerCharacter.IsValid() && OwnerCapsule.IsValid())
    {
        BoundCollision = OwnerCapsule.Get();
    }

    if (!BoundCollision.IsValid())
    {
        TArray<UPrimitiveComponent*> PrimComps;
        Owner->GetComponents<UPrimitiveComponent>(PrimComps);
        if (PrimComps.Num() > 0)
        {
            BoundCollision = PrimComps[0];
        }
    }

    if (BoundCollision.IsValid())
    {
        BoundCollision->SetGenerateOverlapEvents(true);
        BoundCollision->OnComponentBeginOverlap.AddDynamic(this, &UGravityDetectorComponent::HandleBeginOverlap);
        BoundCollision->OnComponentEndOverlap.AddDynamic(this, &UGravityDetectorComponent::HandleEndOverlap);
    }
}

/* ===================== ForceZeroG ===================== */

void UGravityDetectorComponent::ForceZeroG(bool bEnable)
{
    bForcedZeroG = bEnable;

    if (bForcedZeroG)
    {
        SetGravityType(EGravityType::ZeroG, nullptr);
    }
}

/* ===================== Overlap handlers ===================== */

void UGravityDetectorComponent::HandleBeginOverlap(
    UPrimitiveComponent* OverlappedComp,
    AActor* OtherActor,
    UPrimitiveComponent* OtherComp,
    int32 OtherBodyIndex,
    bool bFromSweep,
    const FHitResult& SweepResult)
{
    if (!OtherActor || bForcedZeroG) return;

    const EGravityType ZoneType = ClassToType(OtherActor);
    if (ZoneType == EGravityType::ZeroG) return;

    OverlappingZones.RemoveAll([](const TWeakObjectPtr<AActor>& Ptr){ return !Ptr.IsValid(); });
    if (!OverlappingZones.Contains(OtherActor))
    {
        OverlappingZones.Add(OtherActor);
    }

    OnGravityZoneEnter.Broadcast(ZoneType, OtherActor);
    ResolveCurrentZone();
}

void UGravityDetectorComponent::HandleEndOverlap(
    UPrimitiveComponent* OverlappedComp,
    AActor* OtherActor,
    UPrimitiveComponent* OtherComp,
    int32 OtherBodyIndex)
{
    if (!OtherActor) return;

    const EGravityType ZoneType = ClassToType(OtherActor);
    if (ZoneType == EGravityType::ZeroG) return;

    OverlappingZones.RemoveAll([OtherActor](const TWeakObjectPtr<AActor>& Ptr)
    {
        return !Ptr.IsValid() || Ptr.Get() == OtherActor;
    });

    OnGravityZoneExit.Broadcast(ZoneType, OtherActor);
    ResolveCurrentZone();
}

/* ===================== Управление состоянием ===================== */

void UGravityDetectorComponent::SetGravityType(EGravityType NewType, AActor* ZoneActor)
{
    if (CurrentType == NewType && CurrentZoneActor.Get() == ZoneActor) return;

    CurrentType = NewType;
    CurrentZoneActor = ZoneActor;

    OnGravityTypeChanged.Broadcast(CurrentType, CurrentZoneActor.Get());

    if (CurrentType == EGravityType::ZeroG)
    {
        ApplyZeroGMovementSettings();
    }
}

void UGravityDetectorComponent::FallbackAfterExit()
{
    ResolveCurrentZone();
}

void UGravityDetectorComponent::ResolveCurrentZone()
{
    if (bForcedZeroG)
    {
        SetGravityType(EGravityType::ZeroG, nullptr);
        return;
    }

    OverlappingZones.RemoveAll([](const TWeakObjectPtr<AActor>& Ptr){ return !Ptr.IsValid(); });

    AActor* Nearest = FindNearestZone();
    if (!Nearest)
    {
        SetGravityType(EGravityType::ZeroG, nullptr);
        return;
    }

    const EGravityType NewType = ClassToType(Nearest);
    SetGravityType(NewType, Nearest);
}

AActor* UGravityDetectorComponent::FindNearestZone() const
{
    AActor* Best = nullptr;
    float BestDistSq = TNumericLimits<float>::Max();

    for (const TWeakObjectPtr<AActor>& Ptr : OverlappingZones)
    {
        const AActor* Zone = Ptr.Get();
        if (!Zone) continue;

        const EGravityType T = ClassToType(Zone);
        if (T == EGravityType::ZeroG) continue;

        const float D = DistanceToActor(Zone);
        if (D < BestDistSq)
        {
            BestDistSq = D;
            Best = const_cast<AActor*>(Zone);
        }
    }

    return Best;
}

float UGravityDetectorComponent::DistanceToActor(const AActor* Zone) const
{
    const AActor* Owner = GetOwner();
    if (!Owner || !Zone) return TNumericLimits<float>::Max();

    return FVector::DistSquared(Owner->GetActorLocation(), Zone->GetActorLocation());
}

EGravityType UGravityDetectorComponent::ClassToType(const AActor* Zone)
{
    if (!Zone) return EGravityType::ZeroG;

    if (Zone->IsA(AStationGravityActor::StaticClass()))   return EGravityType::OnStation;
    if (Zone->IsA(APlanetGravityActor::StaticClass()))    return EGravityType::OnPlanet;
    if (Zone->IsA(ASpaceshipGravityActor::StaticClass())) return EGravityType::OnShip;

    return EGravityType::ZeroG;
}

/* ===================== ZeroG движение ===================== */

void UGravityDetectorComponent::ApplyZeroGMovementSettings()
{
    if (!OwnerCharacter.IsValid()) return;

    if (UCharacterMovementComponent* Move = OwnerCharacter->GetCharacterMovement())
    {
        Move->StopMovementImmediately();
        Move->SetMovementMode(MOVE_Flying);
        Move->GravityScale = 0.f;
        Move->BrakingFriction = 0.f;
        Move->AirControl = 1.f;
    }
}

void UGravityDetectorComponent::ZeroG_MoveForward(float Value)
{
    if (!OwnerCharacter.IsValid() || FMath::IsNearlyZero(Value)) return;

    const FRotator R = GetOrientationRot(OwnerCharacter.Get(), Orientation);
    const FVector  Dir = AxisX(R);
    OwnerCharacter->AddMovementInput(Dir, Value * (ZeroGMoveSpeed / 600.f));

    if (bRotateMeshToOrientation) { ZeroG_RotateMeshTowardsOrientation(); }
}

void UGravityDetectorComponent::ZeroG_MoveRight(float Value)
{
    if (!OwnerCharacter.IsValid() || FMath::IsNearlyZero(Value)) return;

    const FRotator R = GetOrientationRot(OwnerCharacter.Get(), Orientation);
    const FVector  Dir = AxisY(R);
    OwnerCharacter->AddMovementInput(Dir, Value * (ZeroGMoveSpeed / 600.f));

    if (bRotateMeshToOrientation) { ZeroG_RotateMeshTowardsOrientation(); }
}

void UGravityDetectorComponent::ZeroG_MoveUp(float Value)
{
    if (!OwnerCharacter.IsValid() || FMath::IsNearlyZero(Value)) return;

    const FRotator R = GetOrientationRot(OwnerCharacter.Get(), Orientation);
    const FVector  Dir = AxisZ(R);
    OwnerCharacter->AddMovementInput(Dir, Value * (ZeroGMoveSpeed / 600.f));

    if (bRotateMeshToOrientation) { ZeroG_RotateMeshTowardsOrientation(); }
}

void UGravityDetectorComponent::ZeroG_RotateMeshTowardsOrientation()
{
    if (!OwnerCharacter.IsValid()) return;

    if (USkeletalMeshComponent* Mesh = OwnerCharacter->GetMesh())
    {
        FRotator Target = GetOrientationRot(OwnerCharacter.Get(), Orientation);

        if (Orientation && Orientation->IsA(UArrowComponent::StaticClass()))
        {
            Target.Yaw -= 90.f;
        }

        Mesh->SetRelativeRotation(Target);
    }
}
