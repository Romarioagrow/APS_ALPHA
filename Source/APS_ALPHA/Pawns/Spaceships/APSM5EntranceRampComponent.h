#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "APSM5EntranceRampComponent.generated.h"

class ASpaceship;
class UStaticMeshComponent;

/** Only attached to the five authored M5 Blueprints. */
UCLASS(ClassGroup=(APS), meta=(BlueprintSpawnableComponent))
class APS_ALPHA_API UAPSM5EntranceRampComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    UAPSM5EntranceRampComponent();
    /** Authored ramp foot, in the hull component's native centimetres. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="M5|Ramp")
    FVector DeploymentFootLocal = FVector::ZeroVector;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category="M5|Ramp")
    bool bRampDeployed = false;

protected:
    virtual void BeginPlay() override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
    TWeakObjectPtr<ASpaceship> Ship;
    TWeakObjectPtr<UStaticMeshComponent> Ramp;
    bool bAppliedState = false;
    void UpdateDeployment();
};
