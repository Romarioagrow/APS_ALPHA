#pragma once
#if WITH_DEV_AUTOMATION_TESTS
#include "APS_ALPHA/Core/Rendering/APSPlanetCloudComponent.h"
#include "APS_ALPHA/Core/Planetary/APSPlanetCloudLayers.h"
#include "APS_ALPHA/Actors/Astro/Planet.h"
#include "APS_ALPHA/Generation/PlanetarySurfaceGenerator.h"
#include "PlanetaryAtmosphere.h"
#include "WorldScapeRoot.h"
#include "GameFramework/PlayerController.h"
#include "Camera/PlayerCameraManager.h"
#include "HAL/PlatformMemory.h"
#include "Misc/App.h"
#include "Misc/ScopeExit.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Components/MeshComponent.h"
#include "UObject/UnrealType.h"

namespace APSCloudFlightProbe
{
inline bool Requested(){return FParse::Param(FCommandLine::Get(),TEXT("APSProbeCloudFlight"));}
inline bool Performance(){return Requested()&&FParse::Param(FCommandLine::Get(),TEXT("APSProbeCloudFlightPerf"));}
inline bool Horizon(){return Requested()&&FParse::Param(FCommandLine::Get(),TEXT("APSProbeCloudHorizon"));}
inline bool GroundHorizon(){return Horizon()&&FParse::Param(FCommandLine::Get(),TEXT("APSProbeCloudGround"));}
inline bool ConfigureFeatureScaleFixture(FAPSPlanetCloudSettings& Settings,FString& Error)
{
    FString Value;
    if(!FParse::Value(FCommandLine::Get(),TEXT("APSCloudFeatureScale="),Value))
    {
        if(FParse::Param(FCommandLine::Get(),TEXT("APSCloudFeatureScale"))
            || FCString::Strifind(FCommandLine::Get(),TEXT("-APSCloudFeatureScale=")))
        {Error=TEXT("Cloud FeatureScale fixture has no valid value");return false;}
        return true;
    }
    if(!UAPSPlanetCloudComponent::WeatherModelEnabled()
        || (Value!=TEXT("0.5") && Value!=TEXT("1") && Value!=TEXT("2")))
    {Error=TEXT("Cloud FeatureScale fixture requires weather and exactly 0.5, 1 or 2");return false;}
    // Set before generation/first refresh, so every capture uses a stationary
    // field with the same seed. This is a fresh automation model, never a save.
    Settings.FeatureScale=FCString::Atod(*Value);
    Settings.WindScale=0.;
    UE_LOG(LogTemp,Display,TEXT("[APS.CloudFeatureScaleFixture] scale=%s windScale=0 parent=%s; applied before generation"),
        *Value,APSPlanetCloudWeather::SelectedMaterialPath());
    return true;
}
inline double GroundSeaAltitudeKm(double GroundCm,double OceanCm,bool HasOcean)
{
    // Two metres over the visible floor, not two metres above a submerged seabed.
    return (HasOcean?FMath::Max(GroundCm,OceanCm):GroundCm)/100000.+.002;
}
inline FVector GroundViewOutward(const FVector& Outward,const FVector& Tangent,double RadiusCm)
{
    // The launch colony covers the spawn point. Move the DIAGNOSTIC viewpoint
    // five kilometres away on the same sphere; never hide its opaque buildings.
    return (Outward*RadiusCm+Tangent*500000.).GetSafeNormal();
}
// Synchronous lifecycle operations must leave the accepted sky untouched. This
// detects settings, owner, transform, visibility and material-binding mutations;
// ON/OFF rendered captures are still needed for the actual composited picture.
inline FString SkyState(APlanet* P)
{
    auto* Air=IsValid(P)&&IsValid(P->PlanetaryEnvironmentGenerator)
        ?P->PlanetaryEnvironmentGenerator->PlanetAtmosphere:nullptr;
    if(!IsValid(Air))return TEXT("missing");
    FString State=FString::Printf(TEXT("%s|hidden=%d|%s"),*Air->GetPathName(),Air->IsHidden(),*Air->GetActorTransform().ToString());
    for(TFieldIterator<FProperty> It(AAtmoScape::StaticClass(),EFieldIteratorFlags::ExcludeSuper);It;++It)
    {
        FString Value;It->ExportText_InContainer(0,Value,Air,nullptr,Air,PPF_None);
        State+=TEXT("|")+It->GetName()+TEXT("=")+Value;
    }
    TInlineComponentArray<UMeshComponent*> Meshes(Air);
    for(auto* M:Meshes)if(IsValid(M))
    {
        State+=FString::Printf(TEXT("|mesh=%s|visible=%d|hidden=%d|%s"),*M->GetPathName(),M->IsVisible(),M->bHiddenInGame,*M->GetComponentTransform().ToString());
        for(int32 I=0;I<M->GetNumMaterials();++I)State+=TEXT("|")+GetPathNameSafe(M->GetMaterial(I));
    }
    return State;
}
inline double HorizonHeight(APlanet* P,double GroundCm,double T,double OceanCm,bool HasOcean)
{
    const APSPlanetCloudPolicy::FLayer L=UAPSPlanetCloudComponent::WeatherModelEnabled()
        ? static_cast<APSPlanetCloudPolicy::FLayer>(UAPSPlanetCloudComponent::Describe(P))
        : APSPlanetCloudPolicy::Resolve(UAPSPlanetSurfaceProfileResolver::ResolveForBody(P),P->AtmosphereHeight,P->IsManual);
    const auto Stack=UAPSPlanetCloudComponent::WeatherModelEnabled() && APSPlanetCloudWeather::LayeredRequested()
        ? APSPlanetCloudLayers::Resolve(UAPSPlanetCloudComponent::Describe(P),P->AtmosphereHeight)
        : APSPlanetCloudLayers::FStack{};
    const double Above=(Stack.Enabled?Stack.BoundsTopKm():L.BottomKm+L.ThicknessKm)+3.;
    const double Middle=Stack.Enabled?Stack.Middle.BottomKm+Stack.Middle.ThicknessKm*.5:L.BottomKm+L.ThicknessKm*.5;
    const double Below=GroundHorizon()?GroundSeaAltitudeKm(GroundCm,OceanCm,HasOcean)
        :FMath::Max(GroundCm/100000.+.05,double(L.BottomKm)-2.);
    auto Blend=[](double A,double B,double Time){const double X=FMath::Clamp(Time,0.,1.);return FMath::Lerp(A,B,X*X*(3.-2.*X));};
    const double SeaAltitude=T<4?Blend(Above,Middle,T/4.):T<10?Middle:T<14?Blend(Middle,Below,(T-10)/4.):T<18?Below:T<22?Blend(Below,Above,(T-18)/4.):Above;
    return SeaAltitude-GroundCm/100000.;
}
inline bool BoundWeather(UAPSPlanetCloudComponent* Cloud,APlanet* P,FString& Error)
{
    // Read actual MID values, not just a model field/source guard. Invoked on
    // visual runs and after edits, never inside a performance comparison.
    auto* MID=IsValid(Cloud)?Cast<UMaterialInstanceDynamic>(Cloud->GetMaterial(0)):nullptr;
    if(!MID || GetPathNameSafe(MID->Parent)!=FString(APSPlanetCloudWeather::SelectedMaterialPath()))
    {Error=TEXT("Weather controls are not bound to the exact weather graph");return false;}
    const auto L=UAPSPlanetCloudComponent::Describe(P);
    if(!L.Enabled){Error=TEXT("Weather controls fixture lost its eligible model");return false;}
    struct FScalar { const TCHAR* Name; float Expected; };
    for(const FScalar V:{FScalar{TEXT("CloudCoverage"),L.Coverage},
        FScalar{TEXT("CloudDensity"),L.Density},FScalar{TEXT("CloudWeatherScale"),L.FeatureScale},
        FScalar{TEXT("CloudBottomKm"),L.BottomKm},FScalar{TEXT("CloudThicknessKm"),L.ThicknessKm},
        FScalar{TEXT("CloudSwirl"),L.Swirl},FScalar{TEXT("CloudBanding"),L.Banding}})
    {
        float Actual=0;
        if(!MID->GetScalarParameterValue(FHashedMaterialParameterInfo(V.Name),Actual)
            || !FMath::IsFinite(Actual) || !FMath::IsNearlyEqual(Actual,V.Expected,1.e-5f))
        {Error=FString::Printf(TEXT("Weather uniform %s does not match the selected model"),V.Name);return false;}
    }
    const auto Color=[](const FVector& V){return FLinearColor(float(V.X),float(V.Y),float(V.Z),0);};
    // Wind phase is accumulated, not recomputed from elapsed time at the new
    // slider speed. Check that the actual component state reached its MID.
    const auto Wind=Cloud->GetWindRotation();
    struct FVectorValue { const TCHAR* Name; FLinearColor Expected; };
    for(const FVectorValue V:{FVectorValue{TEXT("CloudAlbedo"),L.Albedo},
        FVectorValue{TEXT("CloudSeedOffset"),Color(L.Offset)},FVectorValue{TEXT("CloudWindRotation"),Color(Wind)}})
    {
        FLinearColor Actual;
        if(!MID->GetVectorParameterValue(FMaterialParameterInfo(V.Name),Actual) || !Actual.Equals(V.Expected,1.e-5f))
        {Error=FString::Printf(TEXT("Weather vector %s does not match the selected model"),V.Name);return false;}
    }
    if(APSPlanetCloudWeather::LayeredRequested())
    {
        const auto Stack=APSPlanetCloudLayers::Resolve(L,P->AtmosphereHeight);
        float Style=-1;
        if(!MID->GetScalarParameterValue(FHashedMaterialParameterInfo(TEXT("CloudLayeredStyle")),Style)
            || !FMath::IsNearlyEqual(Style,Stack.Enabled?1.f:0.f))
        {Error=TEXT("Layered candidate does not match its climate admission");return false;}
        const auto DeckColor=[](const APSPlanetCloudLayers::FDeck& D)
        { return FLinearColor(D.BottomKm,D.ThicknessKm,D.Density,0); };
        for(const FVectorValue V:{FVectorValue{TEXT("CloudLowDeck"),DeckColor(Stack.Low)},
            FVectorValue{TEXT("CloudMidDeck"),DeckColor(Stack.Middle)},FVectorValue{TEXT("CloudHighDeck"),DeckColor(Stack.High)},
            FVectorValue{TEXT("CloudDeckCoverage"),FLinearColor(Stack.Low.Coverage,Stack.Middle.Coverage,Stack.High.Coverage,0)}})
        {
            FLinearColor Actual;
            if(!MID->GetVectorParameterValue(FMaterialParameterInfo(V.Name),Actual) || !Actual.Equals(V.Expected,1.e-5f))
            {Error=FString::Printf(TEXT("Layered uniform %s does not match the climate stack"),V.Name);return false;}
        }
    }
    return true;
}
class FProbe
{
    struct FFrame{uint64 Frame;double T,Height,Dt,Wall,Game,Render,GPU;};
    TArray<FFrame> Frames;
    TArray<FVector2D> RouteDecks;
    uint64 LastCounter=0;
    double LastWall=0;
    bool Above=false,Inside=false,Below=false,NearGround=false;
    bool LayeredRoute=false,InsideLow=false,InsideMiddle=false,InsideHigh=false;
    TWeakObjectPtr<APlanet> Planet;
    bool Lifecycle(FString& Error)
    {
        auto* P=Planet.Get();
        auto* Enabled=IConsoleManager::Get().FindConsoleVariable(TEXT("aps.Surface.Clouds"));
        if(!IsValid(P)||!Enabled){Error=TEXT("Cloud lifecycle lost planet/cvar");return false;}
        if(Enabled->GetInt()!=1)return true;
        const FString AcceptedSky=SkyState(P);
        if(AcceptedSky==TEXT("missing")){Error=TEXT("Cloud preservation test has no accepted atmosphere");return false;}
        const bool WasManual=P->IsManual;
        const auto SavedSettings=P->CloudSettings;
        const int32 WasEnabled=Enabled->GetInt();
        const auto Priority=static_cast<EConsoleVariableFlags>(Enabled->GetFlags()&ECVF_SetByMask);
        ON_SCOPE_EXIT { P->IsManual=WasManual; P->CloudSettings=SavedSettings; Enabled->Set(WasEnabled,Priority); UAPSPlanetCloudComponent::Refresh(P); };
        auto Count=[P]()
        {
            TArray<AActor*> Owned;P->GetAttachedActors(Owned);int32 N=0;
            for(auto* A:Owned)if(IsValid(A)&&!A->IsActorBeingDestroyed()&&A->GetOwner()==P
                &&A->FindComponentByClass<UAPSPlanetCloudComponent>())++N;
            return N;
        };
        for(int32 I=0;I<3;++I)UAPSPlanetCloudComponent::Refresh(P);
        if(Count()!=1){Error=TEXT("Repeated cloud refresh creates duplicate owners");return false;}
        P->IsManual=true;UAPSPlanetCloudComponent::Refresh(P);
        if(Count()!=0){Error=TEXT("Manual body retained generated cloud owner");return false;}
        P->IsManual=WasManual;UAPSPlanetCloudComponent::Refresh(P);
        if(Count()!=1){Error=TEXT("Generated cloud owner did not restore");return false;}
        Enabled->Set(0,Priority);UAPSPlanetCloudComponent::Refresh(P);
        if(Count()!=0){Error=TEXT("Disabled clouds retained their owner");return false;}
        Enabled->Set(WasEnabled,Priority);UAPSPlanetCloudComponent::Refresh(P);
        if(Count()!=1){Error=TEXT("Cloud owner did not return after disable");return false;}
        if(UAPSPlanetCloudComponent::WeatherModelEnabled())
        {
            const auto CheckBound=[&]()
            {
                if(Count()!=1){Error=TEXT("Weather edit changed owner count");return false;}
                TArray<AActor*> Owned;P->GetAttachedActors(Owned);
                for(auto* A:Owned)if(IsValid(A)&&!A->IsActorBeingDestroyed()&&A->GetOwner()==P)
                    if(auto* C=A->FindComponentByClass<UAPSPlanetCloudComponent>())return BoundWeather(C,P,Error);
                Error=TEXT("Weather edit lost cloud owner");return false;
            };
            // After all captured/timed frames. Never contaminate the ON/OFF
            // performance region or leave edits in a generated/saved model.
            P->CloudSettings.CoverageScale=.7;P->CloudSettings.DensityScale=.33;
            P->CloudSettings.FeatureScale=1.67;P->CloudSettings.AltitudeScale=.6;
            P->CloudSettings.WindScale=0;P->CloudSettings.StormScale=0;
            P->CloudSettings.SeedOffset=(SavedSettings.Sanitized().SeedOffset+37)%999984;
            UAPSPlanetCloudComponent::Refresh(P);
            if(!CheckBound())return false;
            P->CloudSettings.CoverageScale=0;UAPSPlanetCloudComponent::Refresh(P);
            if(Count()!=0){Error=TEXT("Clear-sky coverage retained a cloud owner");return false;}
            P->CloudSettings=SavedSettings;UAPSPlanetCloudComponent::Refresh(P);
            if(!CheckBound())return false;
            UE_LOG(LogTemp,Display,TEXT("[APS.CloudWeatherControls] modelUniforms=1 clearSkyRetired=1 restored=1; post-capture binding check, NOT UI/render acceptance"));
        }
        if(SkyState(P)!=AcceptedSky){Error=TEXT("Cloud lifecycle changed accepted sky state/materials");return false;}
        UE_LOG(LogTemp,Display,TEXT("[APS.CloudLifecycle] duplicate=0 manualRetired=1 disabledRetired=1 restored=1 skyUnchanged=1; after frame ROI, not rendered sky acceptance"));
        return true;
    }
public:
    bool Tick(APlanet* P,AWorldScapeRoot* Root,double Now,double T,FString& Error)
    {
        if(!Requested())return true;
        auto* PC=P&&P->GetWorld()?P->GetWorld()->GetFirstPlayerController():nullptr;
        if(!P||!Root||!PC||!PC->PlayerCameraManager){Error=TEXT("Cloud route lost body/root/camera");return false;}
        Planet=P;
        const bool Expected=IConsoleManager::Get().FindConsoleVariable(TEXT("aps.Surface.Clouds"))->GetInt()==1;
        UAPSPlanetCloudComponent* Cloud=nullptr; int Count=0;
        TArray<AActor*> Actors; P->GetAttachedActors(Actors);
        for(auto* A:Actors)if(IsValid(A)&&A->GetOwner()==P)
            if(auto* C=A->FindComponentByClass<UAPSPlanetCloudComponent>()){Cloud=C;++Count;}
        if(Count!=(Expected?1:0)||(Cloud&&(!Cloud->IsVisible()||Cloud->bHiddenInGame||Cloud->CastShadow
            ||Cloud->GetCollisionEnabled()!=ECollisionEnabled::NoCollision||Cloud->GetGenerateOverlapEvents())))
        {Error=TEXT("Cloud ownership/visibility/safety differs from ON/OFF contract");return false;}
        if(Cloud)
        {
            const auto* MID=Cast<UMaterialInstanceDynamic>(Cloud->GetMaterial(0));
            const TCHAR* ExpectedParent=UAPSPlanetCloudComponent::WeatherModelEnabled()
                ? APSPlanetCloudWeather::SelectedMaterialPath() : APSPlanetCloudPolicy::MaterialPath;
            if(!MID || GetPathNameSafe(MID->Parent)!=FString(ExpectedParent))
            {Error=TEXT("Cloud volume is not bound to the exact compiled candidate parent");return false;}
            if(!Performance() && !LastCounter && UAPSPlanetCloudComponent::WeatherModelEnabled() && !BoundWeather(Cloud,P,Error))return false;
        }
        const APSPlanetCloudPolicy::FLayer Layer=Cloud?Cloud->GetLayer():
            UAPSPlanetCloudComponent::WeatherModelEnabled()
                ? static_cast<APSPlanetCloudPolicy::FLayer>(UAPSPlanetCloudComponent::Describe(P))
                : APSPlanetCloudPolicy::Resolve(UAPSPlanetSurfaceProfileResolver::ResolveForBody(P),P->AtmosphereHeight,P->IsManual);
        if(!Layer.Enabled){Error=TEXT("Cloud route is not climate-eligible");return false;}
        const double Height=(PC->PlayerCameraManager->GetCameraLocation()-Root->GetActorLocation()).Size()/100000.-P->RadiusKM;
        const auto Stack=UAPSPlanetCloudComponent::WeatherModelEnabled() && APSPlanetCloudWeather::LayeredRequested()
            ? APSPlanetCloudLayers::Resolve(UAPSPlanetCloudComponent::Describe(P),P->AtmosphereHeight)
            : APSPlanetCloudLayers::FStack{};
        // Record the same physical regions for cloud-ON and cloud-OFF runs.
        // The legacy base layer is NOT the upper boundary of a layered stack.
        const auto DeckBounds=[](const APSPlanetCloudLayers::FDeck& D)
        { return FVector2D(D.BottomKm,D.BottomKm+D.ThicknessKm); };
        const FVector2D Bounds[3]={Stack.Enabled?DeckBounds(Stack.Low)
            :FVector2D(Layer.BottomKm,Layer.BottomKm+Layer.ThicknessKm),
            DeckBounds(Stack.Middle),DeckBounds(Stack.High)};
        const int32 DeckCount=Stack.Enabled?3:1;
        if(RouteDecks.IsEmpty())for(int32 I=0;I<DeckCount;++I)RouteDecks.Add(Bounds[I]);
        if(RouteDecks.Num()!=DeckCount)
        {Error=TEXT("Cloud layer count changed during measured route");return false;}
        for(int32 I=0;I<DeckCount;++I)if(!RouteDecks[I].Equals(Bounds[I],1.e-9))
        {Error=TEXT("Cloud physical bounds changed during measured route");return false;}
        if(Stack.Enabled)
        {
            const auto In=[Height](const APSPlanetCloudLayers::FDeck& D)
            { return Height>=D.BottomKm && Height<=D.BottomKm+D.ThicknessKm; };
            LayeredRoute=true; InsideLow|=In(Stack.Low); InsideMiddle|=In(Stack.Middle); InsideHigh|=In(Stack.High);
            Above|=Height>Stack.BoundsTopKm(); Below|=Height<Stack.BoundsBottomKm();
            Inside|=In(Stack.Low)||In(Stack.Middle)||In(Stack.High);
        }
        else
        {
            Above|=Height>Layer.BottomKm+Layer.ThicknessKm;
            Inside|=Height>=Layer.BottomKm&&Height<=Layer.BottomKm+Layer.ThicknessKm;
            Below|=Height<Layer.BottomKm;
        }
        if(GroundHorizon())
        {
            const double Floor=GroundSeaAltitudeKm(Root->GetGroundHeight(PC->PlayerCameraManager->GetCameraLocation(),false),Root->OceanHeight,Root->bOcean);
            NearGround|=FMath::Abs(Height-Floor)<.001;
        }
        if(GFrameCounter==LastCounter)return true;
        const uint32 GT=GGameThreadTime,RT=GRenderThreadTime,GPU=GGPUFrameTime;
        if(LastCounter)Frames.Add({GFrameCounter,T,Height,FApp::GetDeltaTime()*1000.,(Now-LastWall)*1000.,
            GT?FPlatformTime::ToMilliseconds(GT):-1.,RT?FPlatformTime::ToMilliseconds(RT):-1.,GPU?FPlatformTime::ToMilliseconds(GPU):-1.});
        LastCounter=GFrameCounter;LastWall=Now;
        return true;
    }
    bool Finish(FString& Error)
    {
        if(!Requested())return true;
        FString Csv(TEXT("frame,t,height_km,dt_ms,wall_ms,game_ms,render_ms,gpu_ms\n"));
        for(const auto& F:Frames)Csv+=FString::Printf(TEXT("%llu,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f\n"),F.Frame,F.T,F.Height,F.Dt,F.Wall,F.Game,F.Render,F.GPU);
        const FString Path=FPaths::Combine(FPaths::ProjectSavedDir(),TEXT("Diagnostics/CloudFlight.csv"));
        IFileManager::Get().MakeDirectory(*FPaths::GetPath(Path),true);
        if(IFileManager::Get().FileExists(*Path)||!FFileHelper::SaveStringToFile(Csv,*Path,FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM))
        {Error=TEXT("Cloud frame evidence exists or cannot be saved");return false;}
        if(Frames.Num()<100||!Above||!Inside||!Below){Error=TEXT("Cloud route did not cross all height regions");return false;}
        if(LayeredRoute && (!InsideLow||!InsideMiddle||!InsideHigh))
        {Error=TEXT("Layered cloud route did not cross all three occupied decks");return false;}
        if(GroundHorizon()&&!NearGround){Error=TEXT("Cloud ground route never reached two metres above the visible floor");return false;}
        // Parameter queries are outside the measured interval. Performance runs
        // must still prove that the ON material matches the recorded model.
        if(Performance()&&UAPSPlanetCloudComponent::WeatherModelEnabled())
        {
            auto* P=Planet.Get();
            if(!IsValid(P)){Error=TEXT("Cloud timing route lost its planet");return false;}
            if(IConsoleManager::Get().FindConsoleVariable(TEXT("aps.Surface.Clouds"))->GetInt()==1)
            {
                UAPSPlanetCloudComponent* Cloud=nullptr;
                TArray<AActor*> Actors;P->GetAttachedActors(Actors);
                for(auto* A:Actors)if(IsValid(A)&&A->GetOwner()==P)
                    if(auto* C=A->FindComponentByClass<UAPSPlanetCloudComponent>())Cloud=C;
                if(!BoundWeather(Cloud,P,Error))return false;
            }
        }
        FString BoundsText;
        for(const auto& D:RouteDecks)
        {
            if(!BoundsText.IsEmpty())BoundsText+=TEXT(";");
            BoundsText+=FString::Printf(TEXT("%.9f,%.9f"),D.X,D.Y);
        }
        UE_LOG(LogTemp,Display,TEXT("[APS.CloudRouteLayers] layered=%d boundsKm=%s"),LayeredRoute,*BoundsText);
        if(GroundHorizon())UE_LOG(LogTemp,Display,TEXT("[APS.CloudGround] nearGround=1 clearance=2m floor=terrain-or-ocean; camera view only, NOT walking/collision acceptance"));
        UE_LOG(LogTemp,Display,TEXT("[APS.CloudFlight] frames=%d above=1 inside=1 below=1 safety=1 perf=%d RAMMiB=%.2f csv=%s; camera route, NOT ship performance acceptance"),
            Frames.Num(),Performance(),FPlatformMemory::GetStats().UsedPhysical/1048576.,*Path);
        if(!Performance()&&!Lifecycle(Error))return false;
        return true;
    }
};
}
#endif
