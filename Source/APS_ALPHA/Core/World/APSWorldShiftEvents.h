#pragma once

#include "CoreMinimal.h"
#include "Misc/CoreDelegates.h"

class UWorld;

// The double-precision floating origin does not change UWorld::OriginLocation.
// It must not impersonate the engine's FIntVector origin event. Consumers of
// world-space uniforms need the same post-shift barrier for both mechanisms.
namespace APSWorldShiftEvents
{
    DECLARE_MULTICAST_DELEGATE_TwoParams(FPostDoubleShift, UWorld*, const FVector&);
    APS_ALPHA_API FPostDoubleShift& OnPostDoubleShift();

    // Read final transforms, do not add Offset to cached positions: attached
    // hierarchies have already applied the shift exactly once. The caller keeps
    // its existing world/owner guards and must bind once per instance.
    template<typename TCallback>
    void BindPostShift(UObject* Lifetime, TCallback Callback)
    {
        check(IsInGameThread());
        FCoreDelegates::PostWorldOriginOffset.AddWeakLambda(Lifetime,
            [Callback](UWorld* World, FIntVector, FIntVector) { Callback(World); });
        OnPostDoubleShift().AddWeakLambda(Lifetime,
            [Callback](UWorld* World, const FVector&) { Callback(World); });
    }
}
