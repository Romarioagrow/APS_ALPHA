#include "APSFoliageCollisionComponent.h"
#include "APSPlanetSurfaceScatter.h"
#include "Components/BoxComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "ProfilingDebugging/CpuProfilerTrace.h"
#include "WorldScapeCore/Public/WorldScapeRoot.h"

namespace
{
TAutoConsoleVariable<int32> WalkingCollision(TEXT("aps.WorldScapeFoliage.WalkingCollision"), 0,
    TEXT("Bounded generated-scatter walking boxes; 0 until lifecycle/rendered validation, 1 opt-in. Fresh roots only."));
}

UAPSFoliageCollisionComponent::UAPSFoliageCollisionComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.TickInterval = .2f;
    PrimaryComponentTick.TickGroup = TG_PrePhysics;
}

bool UAPSFoliageCollisionComponent::MakeLocalBox(const FString& Name,
    const FBoxSphereBounds& B, FVector& Center, FVector& Extent)
{
    Center = Extent = FVector::ZeroVector;
    const bool Tree = Name == TEXT("SM_APS_Scatter_TreeA") || Name == TEXT("SM_APS_Scatter_TreeB");
    const bool Rock = Name == TEXT("SM_APS_Scatter_RockA") || Name == TEXT("SM_APS_Scatter_RockB")
        || Name == TEXT("SM_APS_Scatter_SlabA") || Name == TEXT("SM_APS_Scatter_SlabB");
    if ((!Tree && !Rock) || B.Origin.ContainsNaN() || B.BoxExtent.ContainsNaN()
        || B.BoxExtent.GetMin() <= 0) return false;
    // Replaceable prototype collision: rock box or narrow lower trunk, never canopy/grass.
    Extent = B.BoxExtent * (Tree ? FVector(.10, .10, .65) : FVector(.85));
    Center = B.Origin;
    if (Tree) Center.Z = B.Origin.Z - B.BoxExtent.Z + Extent.Z;
    return true;
}

void UAPSFoliageCollisionComponent::ReleaseProxies()
{
    for (const auto& Proxy : Proxies) if (IsValid(Proxy)) Proxy->DestroyComponent();
    Proxies.Reset(); Sources.Reset(); ActiveCount = 0; NextSourceRefresh = 0;
}

void UAPSFoliageCollisionComponent::EndPlay(const EEndPlayReason::Type Reason)
{
    ReleaseProxies();
    Super::EndPlay(Reason);
}

void UAPSFoliageCollisionComponent::TickComponent(float DeltaTime, ELevelTick TickType,
    FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
    auto* Root = Cast<AWorldScapeRoot>(GetOwner());
    auto* PC = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
    auto* Walker = PC ? Cast<ACharacter>(PC->GetPawn()) : nullptr;
    if (!Root || !Walker || !Root->bGenerateFoliages || !Root->IsActorTickEnabled()
        || !Root->GetActorEnableCollision() || Root->IsHidden()
        || WalkingCollision.GetValueOnGameThread() != 1 || Walker->GetVelocity().SizeSquared() > FMath::Square(3000.0))
    { ReleaseProxies(); return; }
    TRACE_CPUPROFILER_EVENT_SCOPE(APSFoliageWalkingCollision);
    const double Now = GetWorld()->GetTimeSeconds();
    if (Now >= NextSourceRefresh)
    {
        Sources.Reset();
        TInlineComponentArray<UInstancedStaticMeshComponent*> Components(Root);
        for (auto* ISM : Components)
            if (IsValid(ISM) && ISM->IsRegistered() && IsValid(ISM->GetStaticMesh())
                && ISM->GetStaticMesh()->GetPathName().StartsWith(FString(APSPlanetSurfaceScatter::Root) + TEXT("/")))
                Sources.Add(ISM);
        NextSourceRefresh = Now + 1.0;
    }
    struct FCandidate { FTransform Transform; FVector Extent; double Distance; };
    TArray<FCandidate> Nearby;
    Nearby.Reserve(MaximumProxies);
    const FVector Observer = Walker->GetActorLocation();
    for (const auto& Weak : Sources)
    {
        auto* ISM = Weak.Get();
        if (!IsValid(ISM) || !ISM->IsRegistered() || !ISM->IsVisible() || ISM->bHiddenInGame || !IsValid(ISM->GetStaticMesh())
            || ISM->Bounds.GetBox().ComputeSquaredDistanceToPoint(Observer) > FMath::Square(RadiusCm)) continue;
        FVector Center, Extent;
        if (!MakeLocalBox(ISM->GetStaticMesh()->GetName(), ISM->GetStaticMesh()->GetBounds(), Center, Extent)) continue;
        for (int32 Index : ISM->GetInstancesOverlappingSphere(Observer, RadiusCm, true))
        {
            FTransform Transform;
            if (!ISM->GetInstanceTransform(Index, Transform, true) || Transform.ContainsNaN()
                || Transform.GetScale3D().GetAbsMin() <= SMALL_NUMBER) continue;
            Transform.SetLocation(Transform.TransformPosition(Center));
            const double Distance = FVector::DistSquared(Transform.GetLocation(), Observer);
            if (Distance > FMath::Square(RadiusCm)) continue;
            if (Nearby.Num() == MaximumProxies && Distance >= Nearby.Last().Distance) continue;
            if (Nearby.Num() == MaximumProxies) Nearby.Pop(EAllowShrinking::No);
            Nearby.Add({Transform, Extent, Distance});
            Nearby.Sort([](const FCandidate& A, const FCandidate& B) { return A.Distance < B.Distance; });
        }
    }
    while (Proxies.Num() < Nearby.Num())
    {
        auto* Box = NewObject<UBoxComponent>(Root, NAME_None, RF_Transient);
        Box->SetMobility(EComponentMobility::Movable);
        Box->SetupAttachment(Root->GetRootComponent());
        Box->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Box->SetCollisionObjectType(ECC_WorldStatic);
        Box->SetCollisionResponseToAllChannels(ECR_Block);
        Box->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
        Box->SetGenerateOverlapEvents(false);
        Box->SetCanEverAffectNavigation(false);
        Box->SetHiddenInGame(true);
        Root->AddInstanceComponent(Box); Box->RegisterComponent();
        Proxies.Add(Box);
    }
    for (int32 I = 0; I < Proxies.Num(); ++I)
    {
        if (I >= Nearby.Num()) { Proxies[I]->SetCollisionEnabled(ECollisionEnabled::NoCollision); continue; }
        // SetBoxExtent rebuilds body setup / shape scale even for equal input.
        // Stationary walking scenery must not churn physics every 200 ms.
        if (!Proxies[I]->GetUnscaledBoxExtent().Equals(Nearby[I].Extent, .0001))
            Proxies[I]->SetBoxExtent(Nearby[I].Extent, false);
        if (!Proxies[I]->GetComponentTransform().Equals(Nearby[I].Transform, .0001))
            Proxies[I]->SetWorldTransform(Nearby[I].Transform, false, nullptr, ETeleportType::TeleportPhysics);
        Proxies[I]->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    }
    ActiveCount = Nearby.Num();
}
