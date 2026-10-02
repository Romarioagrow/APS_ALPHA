#include "APSWaterLightingSubsystem.h"
#include "APSShoreWaterMaterial.h"
#include "APS_ALPHA/Generation/AstroGenerator.h"
#include "ProceduralMeshComponent.h"

bool UAPSWaterLightingSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
    const auto* W=Cast<UWorld>(Outer);
    return W && (W->WorldType==EWorldType::Game || W->WorldType==EWorldType::PIE);
}
void UAPSWaterLightingSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    PostTick=FWorldDelegates::OnWorldPostActorTick.AddUObject(this,&UAPSWaterLightingSubsystem::Update);
}
void UAPSWaterLightingSubsystem::RestorePreviewPriority()
{
    if (auto* L=LeasedPreviewLight.Get(); L && L->ForwardShadingPriority==0)
        L->SetForwardShadingPriority(OriginalPreviewPriority);
    LeasedPreviewLight.Reset();
}
void UAPSWaterLightingSubsystem::Deinitialize()
{
    FWorldDelegates::OnWorldPostActorTick.Remove(PostTick);
    RestorePreviewPriority();
    Materials.Reset();
    Super::Deinitialize();
}
bool UAPSWaterLightingSubsystem::RegisterMaterial(UMaterialInstanceDynamic* M)
{
    if (!APSShoreWaterMaterial::IsInstance(M)) return false;
    Materials.AddUnique(M);
    return true;
}
void UAPSWaterLightingSubsystem::Update(UWorld* W, ELevelTick, float)
{
    if (W!=GetWorld()) return;
    Materials.RemoveAllSwap([](const auto& M){return !M.IsValid();});
    if (Materials.IsEmpty()) { RestorePreviewPriority(); bLastBindingValid=false; return; }

    bool bShorePreview=false;
    for (TActorIterator<AAstroGenerator> It(W); It; ++It)
        if (It->ActorHasTag(TEXT("WorldGenerationPreview")))
        {
            const auto* Ocean=It->GetActivePreviewOceanProxy();
            bShorePreview=It->UsesContinuousPreviewFrame()
                && APSShoreWaterMaterial::IsVisiblePreviewOcean(*It,Ocean)
                && Materials.Contains(Cast<UMaterialInstanceDynamic>(Ocean->GetMaterial(0)));
            if(bShorePreview) break;
        }
    UDirectionalLightComponent* Preview=nullptr;
    if (bShorePreview) for (TActorIterator<ADirectionalLight> It(W); It; ++It)
        if (It->ActorHasTag(TEXT("APSPreviewFillLight")))
        {
            auto* L=Cast<UDirectionalLightComponent>(It->GetLightComponent());
            if (L && It->HasAnyFlags(RF_Transient) && !It->IsHidden() && L->IsVisible()
                && L->bAffectsWorld && !L->bHiddenInGame && !L->CastShadows
                && L->SpecularScale==0 && L->LightingChannels.bChannel0
                && !L->LightingChannels.bChannel1 && !L->LightingChannels.bChannel2)
                Preview=L;
            break;
        }
    if (LeasedPreviewLight.Get()!=Preview)
    {
        RestorePreviewPriority();
        if (Preview && Preview->ForwardShadingPriority==2)
        {
            LeasedPreviewLight=Preview; OriginalPreviewPriority=2;
            // The real sun stays the forward key. The existing camera fill is
            // transported through the water material, with unchanged irradiance.
            Preview->SetForwardShadingPriority(0);
        }
    }
    FLinearColor Direction, Irradiance;
    bLastBindingValid=APSWaterSurfaceLighting::RenderContract()
        && APSWaterSurfaceLighting::Resolve(W,Direction,Irradiance);
    if (!bLastBindingValid) return; // Trial is not admitted for unsupported scenes.
    LastDirection=Direction; LastIrradiance=Irradiance; LastBindingFrame=GFrameCounter;
    for (const auto& M:Materials) APSWaterSurfaceLighting::Write(M.Get(),Direction,Irradiance);
}
bool UAPSWaterLightingSubsystem::HasCurrentBinding(UMaterialInstanceDynamic* M) const
{
    FLinearColor Direction,Irradiance;
    return bLastBindingValid && GFrameCounter-LastBindingFrame<=1 && Materials.Contains(M) && M
        && M->GetVectorParameterValue(FMaterialParameterInfo(TEXT("APS_WaterFillDirection")),Direction)
        && M->GetVectorParameterValue(FMaterialParameterInfo(TEXT("APS_WaterFillIrradiance")),Irradiance)
        && Direction.Equals(LastDirection,1.e-6f) && Irradiance.Equals(LastIrradiance,1.e-6f);
}
