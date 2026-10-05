#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "APSM5HullSweepComponent.generated.h"

class UStaticMeshComponent;
struct FAPSM5HullSweepTree;

/** Exact fitted-hull movement with conservative spatial rejection, opt-in only. */
UCLASS(ClassGroup=(APS), meta=(BlueprintSpawnableComponent))
class APS_ALPHA_API UAPSM5HullSweepComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    UAPSM5HullSweepComponent();
    virtual ~UAPSM5HullSweepComponent() override;

    /** False means unsupported state: caller must use normal engine movement. */
    bool TryMoveHull(UStaticMeshComponent* Hull, const FVector& Delta, FHitResult& OutHit);

    UFUNCTION(BlueprintCallable, Category="M5|Collision")
    bool TestMoveHull(const FVector& Delta, FHitResult& OutHit);

    UFUNCTION(BlueprintPure, Category="M5|Collision")
    FString GetSweepDiagnostics() const;

private:
    TSharedPtr<FAPSM5HullSweepTree> Tree;
    int32 LastBroadPhaseQueries = 0;
    int32 LastExactShapeQueries = 0;
    int32 LastShapeCount = 0;
    double LastMilliseconds = 0;
};
