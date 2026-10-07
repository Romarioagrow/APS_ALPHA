#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "APSFoliageExclusionComponent.generated.h"

class UInstancedStaticMeshComponent;

/** Only APS scatter, after native publication. Never mutates worker/sector data. */
UCLASS(Transient)
class APS_ALPHA_API UAPSFoliageExclusionComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    UAPSFoliageExclusionComponent();
    static constexpr int32 MaxInstancesPerTick = 2048;
    static constexpr int32 MaxVolumes = 256;
    struct FVolume { FTransform ToRoot; FBox LocalBox; };
    static bool Intersects(const FBox& MeshBox, const FTransform& InstanceToRoot, const FVolume& Volume);
    int32 GetSuppressedCount() const;
    int32 GetVolumeCount() const { return Volumes.Num(); }
    int32 GetPendingCount() const { return PendingCount; }
    bool Excludes(const FBox& MeshBox, const FTransform& InstanceToWorld) const;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
private:
    friend class FAPSFoliageExclusionLifecycleTest;
    struct FSource
    {
        int32 Count = -1;
        uint32 Revision = 0;
        // Component-local: survives floating-origin/planet attachment movement.
        TArray<FTransform> Removed;
    };
    TMap<TWeakObjectPtr<UInstancedStaticMeshComponent>, FSource> Sources;
    TArray<FVolume> Volumes;
    uint32 Revision = 1;
    double NextFootprints = 0;
    int32 PendingCount = 0;
    void RefreshFootprints();
};
