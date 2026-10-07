#pragma once
#if WITH_DEV_AUTOMATION_TESTS
#include "APS_ALPHA/Core/Planetary/APSFoliageCollisionComponent.h"
#include "APS_ALPHA/Core/Planetary/APSPlanetSurfaceScatter.h"
#include "APS_ALPHA/Core/Planetary/APSFoliageExclusionComponent.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"
#include "Engine/StaticMesh.h"
#include "WorldScapeRoot.h"

// Queries the real generated instances and physical world. Does not relocate
// the pawn/foliage, disable gravity, force a collision tick or create test boxes.
namespace APSFoliageWalkingRendered
{
inline bool Requested() { return FParse::Param(FCommandLine::Get(), TEXT("APSProbeFoliageWalkingContact")); }
inline UBoxComponent* MatchingProxy(AWorldScapeRoot* Root, UStaticMesh* Mesh, FTransform Transform)
{
    FVector Center, Extent;
    if (!Root || !Mesh || !UAPSFoliageCollisionComponent::MakeLocalBox(Mesh->GetName(), Mesh->GetBounds(), Center, Extent)) return nullptr;
    Transform.SetLocation(Transform.TransformPosition(Center));
    TInlineComponentArray<UBoxComponent*> Boxes(Root);
    for (auto* Box : Boxes)
        if (IsValid(Box) && Box->IsRegistered() && Box->GetCollisionEnabled() == ECollisionEnabled::QueryAndPhysics
            && Box->GetComponentTransform().Equals(Transform, .01) && Box->GetUnscaledBoxExtent().Equals(Extent, .01)) return Box;
    return nullptr;
}
inline bool Snapshot(AWorldScapeRoot* Root, const TCHAR* Phase, bool Final, FString& Error)
{
    if (!Requested()) return true;
    auto* PC = Root && Root->GetWorld() ? Root->GetWorld()->GetFirstPlayerController() : nullptr;
    auto* Walker = PC ? Cast<ACharacter>(PC->GetPawn()) : nullptr;
    auto* Capsule = Walker ? Walker->GetCapsuleComponent() : nullptr;
    auto* Collision = Root ? Root->FindComponentByClass<UAPSFoliageCollisionComponent>() : nullptr;
    if (!Capsule || !Collision) { Error = TEXT("Natural walking probe lost character/capsule/collision owner"); return false; }
    if (FParse::Param(FCommandLine::Get(), TEXT("APSProbeFoliageStructureExclusion")))
    {
        auto* Exclusion = Root->FindComponentByClass<UAPSFoliageExclusionComponent>();
        if (!Exclusion) { Error = TEXT("No structure exclusion component"); return false; }
        int32 Remaining = 0;
        TInlineComponentArray<UInstancedStaticMeshComponent*> All(Root);
        for (auto* ISM : All)
        {
            if (!IsValid(ISM) || !ISM->GetStaticMesh()
                || !ISM->GetStaticMesh()->GetPathName().StartsWith(FString(APSPlanetSurfaceScatter::Root) + TEXT("/"))) continue;
            for (int32 I = 0; I < ISM->GetInstanceCount(); ++I)
            { FTransform T; if (ISM->GetInstanceTransform(I, T, true) && Exclusion->Excludes(ISM->GetStaticMesh()->GetBoundingBox(), T)) ++Remaining; }
        }
        UE_LOG(LogTemp, Display, TEXT("APS_FOLIAGE_EXCLUSION_STATE phase=%s volumes=%d suppressed=%d remaining=%d pending=%d"),
            Phase, Exclusion->GetVolumeCount(), Exclusion->GetSuppressedCount(), Remaining, Exclusion->GetPendingCount());
        if (Final && (Exclusion->GetVolumeCount() == 0 || Exclusion->GetSuppressedCount() == 0 || Remaining || Exclusion->GetPendingCount()))
        { Error = TEXT("Structure exclusion lacks natural suppressed instances or leaves overlaps/pending work"); return false; }
    }
    int32 Matched = 0, Contacts = 0;
    TInlineComponentArray<UInstancedStaticMeshComponent*> Meshes(Root);
    FCollisionQueryParams Query(SCENE_QUERY_STAT(APSFoliageNaturalCapsule), false);
    Query.AddIgnoredActor(Walker);
    const auto Shape = FCollisionShape::MakeCapsule(Capsule->GetScaledCapsuleRadius(), Capsule->GetScaledCapsuleHalfHeight());
    TSet<UBoxComponent*> Seen;
    for (auto* ISM : Meshes)
    {
        if (!IsValid(ISM) || !ISM->IsVisible() || ISM->bHiddenInGame || !ISM->GetStaticMesh()) continue;
        for (int32 Index : ISM->GetInstancesOverlappingSphere(Walker->GetActorLocation(), UAPSFoliageCollisionComponent::RadiusCm, true))
        {
            FTransform Instance;
            if (!ISM->GetInstanceTransform(Index, Instance, true)) continue;
            auto* Box = MatchingProxy(Root, ISM->GetStaticMesh(), Instance);
            if (!Box || Seen.Contains(Box)) continue;
            Seen.Add(Box); ++Matched;
            const FVector Up = (Box->GetComponentLocation() - Root->GetActorLocation()).GetSafeNormal();
            FVector Tangent, Other; Up.FindBestAxisVectors(Tangent, Other);
            const double Ground = Root->PlanetScale + Root->GetGroundHeight(Box->GetComponentLocation(), false);
            const FVector End = Root->GetActorLocation() + Up * (Ground + Capsule->GetScaledCapsuleHalfHeight() + 2.);
            const double Distance = Box->Bounds.SphereRadius + Capsule->GetScaledCapsuleRadius() + 100.;
            for (int32 Angle = 0; Angle < 8; ++Angle)
            {
                const FVector Start = End + Tangent.RotateAngleAxis(Angle * 45., Up) * Distance;
                if (Root->GetWorld()->OverlapBlockingTestByChannel(Start, Capsule->GetComponentQuat(), ECC_Pawn, Shape, Query)) continue;
                FHitResult Hit;
                if (Root->GetWorld()->SweepSingleByChannel(Hit, Start, End, Capsule->GetComponentQuat(), ECC_Pawn, Shape, Query)
                    && !Hit.bStartPenetrating && Hit.GetComponent() == Box && Hit.Time > 0 && Hit.Time < 1)
                {
                    ++Contacts;
                    UE_LOG(LogTemp, Display, TEXT("APS_FOLIAGE_CAPSULE_CONTACT phase=%s mesh=%s instance=%d time=%.5f distanceCm=%.3f radiusCm=%.3f halfHeightCm=%.3f; world sweep of real pawn shape, NOT injected walking"),
                        Phase, *ISM->GetStaticMesh()->GetName(), Index, Hit.Time, Hit.Distance, Shape.GetCapsuleRadius(), Shape.GetCapsuleHalfHeight());
                    break;
                }
            }
        }
    }
    UE_LOG(LogTemp, Display, TEXT("APS_FOLIAGE_WALKING_STATE phase=%s active=%d allocated=%d naturalMatches=%d capsuleContacts=%d pawnSpeedCm=%.3f; no pawn/instance/physics overrides"),
        Phase, Collision->GetActiveProxyCount(), Collision->GetAllocatedProxyCount(), Matched, Contacts, Walker->GetVelocity().Size());
    const bool ExclusionTrial = FParse::Param(FCommandLine::Get(), TEXT("APSProbeFoliageStructureExclusion"));
    // Clearing a pad legitimately removes nearby capsule targets. This explicit
    // trial proves clearance/identity only, not walking contact. The ordinary
    // contact mode still requires positive real contacts, unchanged.
    if (Final && (Matched != Collision->GetActiveProxyCount() || (!ExclusionTrial && (Matched == 0 || Contacts == 0))))
    { Error = TEXT("Walking collision lacks exact natural-instance matches or real capsule world contact; no acceptance"); return false; }
    return true;
}
}
#endif
