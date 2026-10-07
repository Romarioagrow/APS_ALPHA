#include "APSM5EntranceRampComponent.h"
#include "APS_ALPHA/Pawns/Spaceships/Spaceship.h"
#include "Components/StaticMeshComponent.h"
#include "CollisionQueryParams.h"
#include "Engine/World.h"

UAPSM5EntranceRampComponent::UAPSM5EntranceRampComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.TickInterval = 0.1f;
}

void UAPSM5EntranceRampComponent::BeginPlay()
{
    Super::BeginPlay();
    Ship = Cast<ASpaceship>(GetOwner());
    TInlineComponentArray<UStaticMeshComponent*> Meshes(GetOwner());
    for (UStaticMeshComponent* Mesh : Meshes)
    {
        if (Mesh->ComponentHasTag(TEXT("APS.Ship.EntranceRamp")))
        {
            Ramp = Mesh;
            break;
        }
    }
    UpdateDeployment();
}

void UAPSM5EntranceRampComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
    UpdateDeployment();
}

void UAPSM5EntranceRampComponent::UpdateDeployment()
{
    ASpaceship* OwnerShip = Ship.Get();
    UStaticMeshComponent* RampMesh = Ramp.Get();
    UStaticMeshComponent* Hull = OwnerShip ? OwnerShip->GetSpaceshipHull() : nullptr;
    if (!OwnerShip || !RampMesh || !Hull || !GetWorld()) return;

    const FVector Velocity = Hull->IsSimulatingPhysics()
        ? Hull->GetPhysicsLinearVelocity() : OwnerShip->GetKinematicVelocity();
    bool bDeploy = false;
    if (!OwnerShip->HasPilot() && Velocity.SizeSquared() < FMath::Square(50.0))
    {
        const FVector Up = OwnerShip->GetActorUpVector();
        const FVector Foot = Hull->GetComponentTransform().TransformPosition(DeploymentFootLocal);
        FHitResult Hit;
        FCollisionQueryParams Query(SCENE_QUERY_STAT(M5RampDocking), true, OwnerShip);
        if (GetWorld()->LineTraceSingleByChannel(Hit, Foot + Up * 40.0, Foot - Up * 300.0, ECC_Visibility, Query))
        {
            bDeploy = FVector::DotProduct(Hit.ImpactNormal, Up) > 0.5
                && FMath::Abs(FVector::DotProduct(Hit.ImpactPoint - Foot, Up)) <= 45.0;
        }
    }
    if (!bAppliedState || bDeploy != bRampDeployed)
    {
        bAppliedState = true;
        bRampDeployed = bDeploy;
        RampMesh->SetHiddenInGame(!bDeploy, true);
        RampMesh->SetCollisionEnabled(bDeploy ? ECollisionEnabled::QueryOnly : ECollisionEnabled::NoCollision);
        UE_LOG(LogTemp, Log, TEXT("[APS.M5Ramp] ship=%s deployed=%d pilot=%d speed=%.1f cm/s"),
            *OwnerShip->GetName(), bDeploy, OwnerShip->HasPilot(), Velocity.Size());
    }
}
