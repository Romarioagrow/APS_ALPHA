#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "APSFoliageCollisionComponent.generated.h"

class UBoxComponent;
class UInstancedStaticMeshComponent;

/** Small, root-owned walking collision proxies. Never enables HISM-wide physics. */
UCLASS(Transient)
class APS_ALPHA_API UAPSFoliageCollisionComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    UAPSFoliageCollisionComponent();
    static constexpr int32 MaximumProxies = 32;
    static constexpr double RadiusCm = 4000.0;
    static bool MakeLocalBox(const FString& MeshName, const FBoxSphereBounds& Bounds,
        FVector& Center, FVector& Extent);
    int32 GetActiveProxyCount() const { return ActiveCount; }
    int32 GetAllocatedProxyCount() const { return Proxies.Num(); }
    virtual void TickComponent(float DeltaTime, ELevelTick TickType,
        FActorComponentTickFunction* ThisTickFunction) override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
private:
    UPROPERTY(Transient) TArray<TObjectPtr<UBoxComponent>> Proxies;
    TArray<TWeakObjectPtr<UInstancedStaticMeshComponent>> Sources;
    double NextSourceRefresh = 0;
    int32 ActiveCount = 0;
    void ReleaseProxies();
};
