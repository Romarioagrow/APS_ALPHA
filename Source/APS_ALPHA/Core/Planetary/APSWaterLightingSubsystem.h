#pragma once
#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "APSWaterLightingSubsystem.generated.h"

class UMaterialInstanceDynamic;
class UDirectionalLightComponent;

// Only registered shore-water MIDs participate. No per-planet actor scan/tick,
// no owning asset references, no edits to sun/atmosphere intensity or colour.
UCLASS()
class APS_ALPHA_API UAPSWaterLightingSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()
public:
    virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;
    bool RegisterMaterial(UMaterialInstanceDynamic* Material);
    bool HasCurrentBinding(UMaterialInstanceDynamic* Material) const;
private:
    void Update(UWorld* World, ELevelTick TickType, float DeltaSeconds);
    void RestorePreviewPriority();
    TArray<TWeakObjectPtr<UMaterialInstanceDynamic>> Materials;
    TWeakObjectPtr<UDirectionalLightComponent> LeasedPreviewLight;
    int32 OriginalPreviewPriority=0;
    FDelegateHandle PostTick;
    FLinearColor LastDirection=FLinearColor::Black, LastIrradiance=FLinearColor::Black;
    uint64 LastBindingFrame=0;
    bool bLastBindingValid=false;
};
