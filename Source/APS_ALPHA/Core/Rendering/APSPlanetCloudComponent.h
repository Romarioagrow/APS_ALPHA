#pragma once
#include "Components/StaticMeshComponent.h"
#include "APS_ALPHA/Core/Planetary/APSPlanetCloudWeather.h"
#include "APSPlanetCloudComponent.generated.h"

class APlanet;
class UMaterialInstanceDynamic;

/** One bounded, no-collision shell; the shader integrates the same density field
 * from space, inside and below. Prototype is off until rendered/cost validation. */
UCLASS(Transient)
class APS_ALPHA_API UAPSPlanetCloudComponent : public UStaticMeshComponent
{
    GENERATED_BODY()
public:
    UAPSPlanetCloudComponent();
    static void Refresh(APlanet* Planet);
    static bool WeatherModelEnabled();
    static APSPlanetCloudWeather::FWeather Describe(const APlanet* Planet);
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* Tick) override;
    const APSPlanetCloudPolicy::FLayer& GetLayer() const { return Layer; }
    FVector GetWindRotation() const { return APSPlanetCloudWeather::RotationFromPhase(WindPhase); }
private:
    void UpdateFrame();
    APSPlanetCloudWeather::FWeather Layer;
    bool bWeatherMaterial = false;
    double WindPhase = 0;
    TWeakObjectPtr<APlanet> CloudPlanet;
    double NextDiagnosticSeconds = 0;
    UPROPERTY(Transient) TObjectPtr<UMaterialInstanceDynamic> CloudMaterial;
};
