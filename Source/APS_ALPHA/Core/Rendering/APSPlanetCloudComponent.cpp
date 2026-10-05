#include "APSPlanetCloudComponent.h"
#include "APS_ALPHA/Core/Planetary/APSPlanetCloudLayers.h"
#include "APS_ALPHA/Core/World/APSWorldShiftEvents.h"
#include "APS_ALPHA/Actors/Astro/Planet.h"
#include "APS_ALPHA/Actors/Astro/Star.h"
#include "APS_ALPHA/Generation/PlanetarySurfaceGenerator.h"
#include "Camera/PlayerCameraManager.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/Material.h"

static TAutoConsoleVariable<int32> CVarAPSPlanetClouds(TEXT("aps.Surface.Clouds"), 0,
    TEXT("Water-condensate cloud prototype. 0 off; 1 generated Terrestrial/Oasis/Water eligible bodies. Recreate body after enabling."));
static TAutoConsoleVariable<int32> CVarAPSPlanetCloudWeather(TEXT("aps.Surface.CloudWeather"),0,
    TEXT("Opt-in model-driven cloud candidate; requires separately baked V27. Refresh body after changing. Does not alter sky."));

bool UAPSPlanetCloudComponent::WeatherModelEnabled()
{
    return CVarAPSPlanetCloudWeather.GetValueOnGameThread()==1;
}
APSPlanetCloudWeather::FWeather UAPSPlanetCloudComponent::Describe(const APlanet* P)
{
    if(!IsValid(P)) return {};
    FPlanetAtmosphere A=P->PlanetAtmosphere;
    const float Wind=A.WindSpeed>0?A.WindSpeed:A.CalculateWindSpeed(A.WindSpeedLevel);
    return APSPlanetCloudWeather::Resolve(UAPSPlanetSurfaceProfileResolver::ResolveForBody(P),
        P->AtmosphereHeight,P->IsManual,Wind,P->CloudSettings);
}
static TAutoConsoleVariable<int32> CVarAPSPlanetCloudDebug(TEXT("aps.Surface.CloudDebug"),0,
    TEXT("Cloud diagnostics only: 0 normal, 1 both-sided bounds, 2 ray interval, 3 unsafe depth bypass, 4 numeric depth (V29 unsupported/magenta), 5 back-face bounds, 6 ray RGB, 7 segment, 8 integral alpha."));
static TAutoConsoleVariable<int32> CVarAPSPlanetCloudAerial(TEXT("aps.Surface.CloudAerial"),1,
    TEXT("Cloud-only atmospheric contrast transfer. 0 diagnostic bypass; no changes to the atmosphere or terrain."));

UAPSPlanetCloudComponent::UAPSPlanetCloudComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.TickGroup = TG_PostUpdateWork;
    SetCollisionEnabled(ECollisionEnabled::NoCollision);
    SetGenerateOverlapEvents(false);
    SetCanEverAffectNavigation(false);
    CastShadow = false;
    bAffectDistanceFieldLighting = false;
    bAffectDynamicIndirectLighting = false;
    bVisibleInRayTracing = false;
    bReceivesDecals = false;
    SetMobility(EComponentMobility::Movable);
    SetAbsolute(false, false, true);
    TranslucencySortPriority = 1;
}

void UAPSPlanetCloudComponent::OnRegister()
{
    Super::OnRegister();
    if (!APSWorldShiftEvents::OnPostDoubleShift().IsBoundToObject(this))
        APSWorldShiftEvents::BindPostShift(this, [this](UWorld* World)
        {
            // CloudCenter is a material uniform, not a scene transform. A late
            // floating-origin shift occurs after our tick; refresh after ALL
            // actors/camera moved, without advancing wind or changing the model.
            if (IsRegistered() && World == GetWorld()) UpdateFrame();
        });
}

void UAPSPlanetCloudComponent::Refresh(APlanet* P)
{
    if (!IsValid(P) || !P->GetWorld() || P->GetNetMode() == NM_DedicatedServer) return;
    // Body mesh enumeration applies proxy visibility/scale/readability transforms.
    // Like AtmoScape, clouds therefore have a separate, transient visual owner.
    UAPSPlanetCloudComponent* Existing = nullptr;
    TArray<AActor*> Attached; P->GetAttachedActors(Attached);
    for (auto* A : Attached)
        if (IsValid(A) && !A->IsActorBeingDestroyed() && A->GetOwner()==P)
            if (auto* C=A->FindComponentByClass<UAPSPlanetCloudComponent>()) { Existing=C; break; }
    const auto Profile = UAPSPlanetSurfaceProfileResolver::ResolveForBody(P);
    const bool Weather=WeatherModelEnabled();
    APSPlanetCloudWeather::FWeather L;
    if(Weather)
    {
        FPlanetAtmosphere A=P->PlanetAtmosphere;
        const float Wind=A.WindSpeed>0?A.WindSpeed:A.CalculateWindSpeed(A.WindSpeedLevel);
        L=APSPlanetCloudWeather::Resolve(Profile,P->AtmosphereHeight,P->IsManual,Wind,P->CloudSettings);
    }
    else static_cast<APSPlanetCloudPolicy::FLayer&>(L)=APSPlanetCloudPolicy::Resolve(Profile,P->AtmosphereHeight,P->IsManual);
    if (CVarAPSPlanetClouds.GetValueOnGameThread() != 1 || !L.Enabled || !IsValid(P->ParentStar))
    {
        if (Existing) Existing->GetOwner()->Destroy();
        return;
    }
    if (!Existing)
    {
        auto* Mesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Sphere.Sphere"));
        auto* Material = LoadObject<UMaterialInterface>(nullptr, Weather
            ? APSPlanetCloudWeather::SelectedMaterialPath() : APSPlanetCloudPolicy::MaterialPath);
        if (!Mesh || !Material) { UE_LOG(LogTemp, Warning, TEXT("[APS.Clouds] Prototype assets unavailable; no substitute created")); return; }
        FActorSpawnParameters Spawn; Spawn.Owner=P; Spawn.ObjectFlags=RF_Transient;
        Spawn.SpawnCollisionHandlingOverride=ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        auto* Owner=P->GetWorld()->SpawnActor<AActor>(AActor::StaticClass(),P->GetActorTransform(),Spawn);
        if (!Owner) return;
        Owner->SetActorEnableCollision(false); Owner->SetActorTickEnabled(false);
        Existing = NewObject<UAPSPlanetCloudComponent>(Owner, TEXT("APSPlanetCloudVolume"));
        Owner->AddInstanceComponent(Existing); Owner->SetRootComponent(Existing);
        Existing->SetAbsolute(true,true,true);
        Existing->CloudPlanet=P;
        Existing->SetStaticMesh(Mesh);
        Existing->CloudMaterial = UMaterialInstanceDynamic::Create(Material, Existing);
        Existing->bWeatherMaterial=Weather;
        Existing->SetMaterial(0, Existing->CloudMaterial);
        Existing->RegisterComponent();
        Owner->AttachToActor(P,FAttachmentTransformRules::KeepWorldTransform);
    }
    // Changing the diagnostic gate must never silently feed new parameters into
    // the old graph. Asset absence leaves the current visual untouched.
    if(Existing->bWeatherMaterial!=Weather)
    {
        auto* M=LoadObject<UMaterialInterface>(nullptr,Weather?APSPlanetCloudWeather::SelectedMaterialPath():APSPlanetCloudPolicy::MaterialPath);
        if(!M){UE_LOG(LogTemp,Warning,TEXT("[APS.Clouds] Requested cloud graph unavailable; retained current layer"));return;}
        Existing->CloudMaterial=UMaterialInstanceDynamic::Create(M,Existing);
        Existing->SetMaterial(0,Existing->CloudMaterial);
        Existing->bWeatherMaterial=Weather;
    }
    Existing->Layer = L;
    Existing->UpdateFrame();
    if(Weather) UE_LOG(LogTemp,Verbose,TEXT("[APS.CloudWeather] type=%s density=%g size=%g windMps=%g swirl=%g banding=%g"),
        APSPlanetCloudWeather::Name(L.Condensate),L.Density,L.FeatureScale,L.WindKmPerSecond*1000.f,L.Swirl,L.Banding);
    UE_LOG(LogTemp, Display, TEXT("[APS.Clouds] owner=%s type=%d seed=%d bottom=%.3f thickness=%.3f coverage=%.3f pressure=%.3f humidity=%.3f temperature=%.3f material=%s parent=%s collision=0 shadow=0 maxSamples=32"),
        *P->GetName(), int32(P->PlanetType), Profile.BiomeSeed, L.BottomKm,L.ThicknessKm,L.Coverage,
        Profile.AtmosphericPressure,Profile.Humidity,Profile.Temperature,*Existing->CloudMaterial->GetPathName(),
        *GetPathNameSafe(Existing->CloudMaterial->Parent));
}

void UAPSPlanetCloudComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* Tick)
{
    Super::TickComponent(DeltaTime, TickType, Tick);
    if(bWeatherMaterial)
        if(const auto* P=CloudPlanet.Get())
            WindPhase=APSPlanetCloudWeather::AdvanceWindPhase(
                WindPhase,DeltaTime,P->RadiusKM,Layer.WindKmPerSecond);
    UpdateFrame();
}

void UAPSPlanetCloudComponent::UpdateFrame()
{
    auto* P = CloudPlanet.Get();
    if (!IsValid(P)) { if(IsValid(GetOwner())) GetOwner()->Destroy(); return; }
    auto* Atmosphere=IsValid(P->PlanetaryEnvironmentGenerator)?P->PlanetaryEnvironmentGenerator->PlanetAtmosphere:nullptr;
    const bool Valid = IsValid(P) && IsValid(P->ParentStar) && Layer.Enabled && CloudMaterial
        && CVarAPSPlanetClouds.GetValueOnGameThread() == 1 && IsValid(Atmosphere) && !Atmosphere->IsHidden();
    if (!Valid) { SetVisibility(false); return; }
    const double Radius = P->RadiusKM;
    // Actor scale is the legacy basic-sphere proxy diameter, NOT presentation
    // scale. Use the atmosphere's already committed displayed physical radius.
    const double Scale = Atmosphere->PresentationPlanetRadiusCm > 0
        ? double(Atmosphere->PresentationPlanetRadiusCm)/(Radius*100000.)
        : P->WorldScapePresentationScale;
    if (!FMath::IsFinite(Scale) || Scale <= 0 || !FMath::IsFinite(Radius) || Radius <= 0) { SetVisibility(false); return; }
    auto* PC = GetWorld()->GetFirstPlayerController();
    const FVector Center = Atmosphere->GetActorLocation();
    const float Fade = PC && PC->PlayerCameraManager ? APSPlanetCloudPolicy::Visibility(Radius*100000.*Scale,
        FVector::Distance(Center, PC->PlayerCameraManager->GetCameraLocation())) : 1.f;
    SetVisibility(Fade > 0);
    SetHiddenInGame(false);
    if (Fade <= 0) return;
    // Mesh only bounds rays; preserve sphere radius even between its coarse vertices.
    const auto Stack=bWeatherMaterial && APSPlanetCloudWeather::LayeredRequested()
        ? APSPlanetCloudLayers::Resolve(Layer,P->AtmosphereHeight) : APSPlanetCloudLayers::FStack{};
    const double CloudTop=Stack.Enabled?Stack.BoundsTopKm():Layer.BottomKm+Layer.ThicknessKm;
    const double OuterCm = (Radius+CloudTop)*100000.*Scale*1.015;
    const FVector TargetScale(OuterCm/50.);
    if (!GetComponentScale().Equals(TargetScale, .0001)) SetWorldScale3D(TargetScale);
    const FQuat Q = P->GetActorQuat();
    if (!GetComponentLocation().Equals(Center,.01)) SetWorldLocation(Center);
    if (!GetComponentQuat().Equals(Q,.000001)) SetWorldRotation(Q);
    auto Vec = [this](const TCHAR* Name, const FVector& V)
    { CloudMaterial->SetVectorParameterValue(Name,FLinearColor(float(V.X),float(V.Y),float(V.Z),0)); };
    CloudMaterial->SetDoubleVectorParameterValue(TEXT("CloudCenter"), FVector4(Center.X,Center.Y,Center.Z,0));
    CloudMaterial->SetScalarParameterValue(TEXT("CloudCmPerKm"), float(100000.*Scale));
    CloudMaterial->SetScalarParameterValue(TEXT("CloudRadiusKm"), float(Radius));
    CloudMaterial->SetScalarParameterValue(TEXT("CloudBottomKm"), Layer.BottomKm);
    CloudMaterial->SetScalarParameterValue(TEXT("CloudThicknessKm"), Layer.ThicknessKm);
    CloudMaterial->SetScalarParameterValue(TEXT("CloudCoverage"), Layer.Coverage);
    if(bWeatherMaterial)
    {
        CloudMaterial->SetVectorParameterValue(TEXT("CloudAlbedo"),Layer.Albedo);
        CloudMaterial->SetScalarParameterValue(TEXT("CloudDensity"),Layer.Density);
        CloudMaterial->SetScalarParameterValue(TEXT("CloudWeatherScale"),Layer.FeatureScale);
        CloudMaterial->SetScalarParameterValue(TEXT("CloudSwirl"),Layer.Swirl);
        CloudMaterial->SetScalarParameterValue(TEXT("CloudBanding"),Layer.Banding);
        Vec(TEXT("CloudWindRotation"),GetWindRotation());
        if(APSPlanetCloudWeather::LayeredRequested())
        {
            // Separate candidate parameters; baseline uniforms and accepted
            // atmosphere are unchanged. No additional component or draw pass.
            CloudMaterial->SetScalarParameterValue(TEXT("CloudLayeredStyle"),Stack.Enabled?1.f:0.f);
            const auto Deck=[&](const TCHAR* Name,const APSPlanetCloudLayers::FDeck& D)
            { Vec(Name,FVector(D.BottomKm,D.ThicknessKm,D.Density)); };
            Deck(TEXT("CloudLowDeck"),Stack.Low); Deck(TEXT("CloudMidDeck"),Stack.Middle);
            Deck(TEXT("CloudHighDeck"),Stack.High);
            Vec(TEXT("CloudDeckCoverage"),FVector(Stack.Low.Coverage,Stack.Middle.Coverage,Stack.High.Coverage));
        }
    }
    CloudMaterial->SetScalarParameterValue(TEXT("CloudVisibility"), Fade);
    CloudMaterial->SetScalarParameterValue(TEXT("CloudDebug"),float(CVarAPSPlanetCloudDebug.GetValueOnGameThread()));
    // Read the committed sky optics, never mutate the atmosphere. Its material
    // rescales coefficients with the displayed planet; documentation units
    // alone are not the actual shader coefficients. Convert those back to /km.
    const auto SafeAir=[](float Value,float Fallback,float Low,float High)
    { return FMath::Clamp(FMath::IsFinite(Value)?Value:Fallback,Low,High); };
    const auto Luma=[](FLinearColor C){return .2126f*C.R+.7152f*C.G+.0722f*C.B;};
    const float CmPerKm=float(100000.*Scale);
    float AirRadius=SafeAir(Atmosphere->PlanetRadius,float(Radius),1,1.e7f);
    float AirHeight=SafeAir(Atmosphere->AtmosphereHeight,100,.001f,10000);
    float RayHeight=SafeAir(Atmosphere->RayleighHeight,8,.05f,80);
    float MieHeight=SafeAir(Atmosphere->MieHeight,1.2f,.05f,15);
    float AirOpacity=SafeAir(Atmosphere->AtmosphereOpacity*Atmosphere->PresentationOpacityScale,1,0,100);
    float Particulate=SafeAir(Atmosphere->AtmosphereParticulatesDensity,15,0,100);
    float AirIntensity=1;
    const float CoefficientScale=2.5f*SafeAir(Atmosphere->MultiScatering,1,0,100)/AirRadius;
    FLinearColor Rayleigh=Atmosphere->RayleighScattering*CoefficientScale;
    FLinearColor Mie=Atmosphere->MieScattering*CoefficientScale;
    FLinearColor Ozone=(Atmosphere->RayleighScattering+Atmosphere->Absorption*Atmosphere->OzoneContribution)*CoefficientScale;
    FLinearColor Fill(.001601f,.002576f,.005208f,0);
    TInlineComponentArray<UStaticMeshComponent*> AirMeshes(Atmosphere);
    for(auto* Mesh:AirMeshes)
    {
        auto* Material=IsValid(Mesh)&&Mesh->IsVisible()?Mesh->GetMaterial(0):nullptr;
        if(!Material || !Material->GetMaterial() || Material->GetMaterial()->GetName()!=TEXT("MM_PlanetaryAtmo")) continue;
        const auto ReadScalar=[&](const TCHAR* Name,float& Result,float Factor=1.f)
        {
            float Value=0;
            if(Material->GetScalarParameterValue(FHashedMaterialParameterInfo(Name),Value) && FMath::IsFinite(Value))
                Result=FMath::Max(0.f,Value*Factor);
        };
        const auto ReadColor=[&](const TCHAR* Name,FLinearColor& Result,float Factor=1.f)
        {
            FLinearColor Value;
            if(Material->GetVectorParameterValue(FMaterialParameterInfo(Name),Value))
                APSPlanetCloudPolicy::ReadAirColor(Value,Factor,Result);
        };
        ReadScalar(TEXT("EarthRadius"),AirRadius,1.f/CmPerKm);
        float Outer=AirRadius+AirHeight;
        ReadScalar(TEXT("AtmosRadius"),Outer,1.f/CmPerKm); AirHeight=FMath::Max(.001f,Outer-AirRadius);
        ReadScalar(TEXT("ScaleHeight_R"),RayHeight,1.f/CmPerKm);
        ReadScalar(TEXT("ScaleHeight_M"),MieHeight,1.f/CmPerKm);
        ReadScalar(TEXT("AtmosOpacity"),AirOpacity); ReadScalar(TEXT("ParticulateIntensity"),Particulate);
        ReadScalar(TEXT("Int"),AirIntensity);
        ReadColor(TEXT("coef_R"),Rayleigh,CmPerKm); ReadColor(TEXT("coef_M"),Mie,CmPerKm);
        ReadColor(TEXT("coef_RO"),Ozone,CmPerKm); ReadColor(TEXT("AtmosFill"),Fill);
        break;
    }
    const FLinearColor AirOptics(Luma(Ozone),Luma(Mie)*1.1f*Particulate,RayHeight,MieHeight);
    CloudMaterial->SetVectorParameterValue(TEXT("CloudAirOptics"),AirOptics);
    CloudMaterial->SetScalarParameterValue(TEXT("CloudAirMieHeightKm"),AirOptics.A);
    CloudMaterial->SetScalarParameterValue(TEXT("CloudAirHeightKm"),AirHeight);
    CloudMaterial->SetScalarParameterValue(TEXT("CloudAirRadiusKm"),AirRadius);
    CloudMaterial->SetVectorParameterValue(TEXT("CloudAirRayleigh"),Rayleigh);
    CloudMaterial->SetVectorParameterValue(TEXT("CloudAirMie"),Mie);
    CloudMaterial->SetVectorParameterValue(TEXT("CloudAirOzone"),Ozone);
    CloudMaterial->SetVectorParameterValue(TEXT("CloudAirSettings"),FLinearColor(AirOpacity,Particulate,AirIntensity,0));
    CloudMaterial->SetVectorParameterValue(TEXT("CloudAirFill"),Fill);
    CloudMaterial->SetScalarParameterValue(TEXT("CloudAerial"),CVarAPSPlanetCloudAerial.GetValueOnGameThread()!=0?1.f:0.f);
    Vec(TEXT("CloudAxisX"),Q.GetAxisX()); Vec(TEXT("CloudAxisY"),Q.GetAxisY()); Vec(TEXT("CloudAxisZ"),Q.GetAxisZ());
    Vec(TEXT("CloudSeedOffset"),Layer.Offset);
    const FVector PhysicalSun=APSPlanetCloudPolicy::SunDirection(
        P->ParentStar->GetActorLocation(),P->GetActorLocation(),Q);
    if(PhysicalSun.IsNearlyZero()){SetVisibility(false);return;}
    Vec(TEXT("CloudSun"),PhysicalSun);
    const auto SunColor = FLinearColor::MakeFromColorTemperature(float(FMath::Clamp(P->ParentStar->SurfaceTemperature, 2000, 12000)));
    CloudMaterial->SetVectorParameterValue(TEXT("CloudSunColor"),SunColor);
    if (FParse::Param(FCommandLine::Get(), TEXT("APSCloudDiagnostics")) && GetWorld()->GetTimeSeconds() >= NextDiagnosticSeconds)
    {
        NextDiagnosticSeconds = GetWorld()->GetTimeSeconds()+2.;
        UE_LOG(LogTemp, Display, TEXT("[APS.CloudFrame] owner=%s visible=%d hidden=%d ownerHidden=%d scale=%.12g radius=%.3f componentScale=%g boundsRadius=%g centerErrorCm=%g fade=%g cameraLocalKm=%s sun=%s air=%s aerial=%d"),
            *P->GetName(),IsVisible(),int32(bHiddenInGame),P->IsHidden(),Scale,Radius,GetComponentScale().GetMax(),Bounds.SphereRadius,
            FVector::Distance(GetComponentLocation(),Center),Fade,
            *Q.UnrotateVector(((PC&&PC->PlayerCameraManager)?PC->PlayerCameraManager->GetCameraLocation()-Center:FVector::ZeroVector)/(100000.*Scale)).ToString(),
            *PhysicalSun.ToString(),*AirOptics.ToString(),CVarAPSPlanetCloudAerial.GetValueOnGameThread());
    }
}
