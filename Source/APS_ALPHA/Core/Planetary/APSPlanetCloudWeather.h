#pragma once
#include "APSPlanetCloudPolicy.h"
#include "APSPlanetCloudSettings.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

namespace APSPlanetCloudWeather
{
// Separate package: never overwrite the accepted V24 cloud/sky assets.
inline constexpr const TCHAR* MaterialPath=TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/Diagnostics/CloudWeather20261002V27/M_APS_PlanetCloud.M_APS_PlanetCloud");
inline constexpr const TCHAR* CandidateMaterialPath=TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/Diagnostics/CloudWeather20261003V31/M_APS_PlanetCloud.M_APS_PlanetCloud");
inline constexpr const TCHAR* LayeredMaterialPath=TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/Diagnostics/CloudWeather20261002V30/M_APS_PlanetCloud.M_APS_PlanetCloud");
// Rio 06.10 (clouds vanish at an altitude): V27 field, both deck crossings of a
// dipping ray in one march (APSPlanetCloudHlsl::TwoCrossingCode). NEW package only;
// V27/V30/V31 are never touched. "V32" is already the unconnected clustered-shape
// experiment (APSPlanetCloudClusteredHlsl.h), so this graph is V33. At runtime it is
// chosen by aps.Surface.CloudTwoCrossings (APSPlanetCloudComponent.cpp), V27 if absent.
inline constexpr const TCHAR* TwoCrossingMaterialPath=TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/Diagnostics/CloudWeather20261006V33/M_APS_PlanetCloud.M_APS_PlanetCloud");
// Process-only diagnostic selection. The accepted path and saved settings stay V27.
inline bool CandidateRequested()
{
    return FParse::Param(FCommandLine::Get(),TEXT("APSCloudWeatherCandidate"));
}
inline bool LayeredRequested()
{
    return FParse::Param(FCommandLine::Get(),TEXT("APSCloudLayeredCandidate"));
}
// Rio 06.10 (clouds vanish at an altitude): bake flag for V33; also pins V33 for probes.
inline bool TwoCrossingRequested()
{
    return FParse::Param(FCommandLine::Get(),TEXT("APSCloudTwoCrossingCandidate"));
}
inline const TCHAR* SelectedMaterialPath()
{
    if(LayeredRequested()) return LayeredMaterialPath;
    if(TwoCrossingRequested()) return TwoCrossingMaterialPath;
    return CandidateRequested()?CandidateMaterialPath:MaterialPath;
}
enum class ECondensate : uint8 { None, Water, Ice, Ammonia, AcidAerosol, Dust, Ash, Hydrocarbon };
struct FWeather : APSPlanetCloudPolicy::FLayer
{
    ECondensate Condensate=ECondensate::None;
    FLinearColor Albedo=FLinearColor::White;
    float Density=1, FeatureScale=1, WindKmPerSecond=0, Swirl=0, Banding=0;
};
inline const TCHAR* Name(ECondensate C)
{
    switch(C)
    {
    case ECondensate::Water:return TEXT("WATER DROPLETS");
    case ECondensate::Ice:return TEXT("ICE CRYSTALS");
    case ECondensate::Ammonia:return TEXT("AMMONIA CONDENSATE");
    case ECondensate::AcidAerosol:return TEXT("ACID AEROSOL");
    case ECondensate::Dust:return TEXT("MINERAL DUST");
    case ECondensate::Ash:return TEXT("VOLCANIC ASH");
    case ECondensate::Hydrocarbon:return TEXT("HYDROCARBON HAZE");
    default:return TEXT("NO SUPPORTED CLOUD LAYER");
    }
}
inline FWeather Resolve(const FAPSResolvedPlanetSurfaceProfile& P,double AtmosphereKm,
    bool Manual,float WindMetresPerSecond,const FAPSPlanetCloudSettings& Settings)
{
    FWeather L;
    // An explicit game climate approximation. The model does not currently
    // contain species partial pressures; do not claim a chemical simulation.
    if(Manual || !FMath::IsFinite(AtmosphereKm) || AtmosphereKm<4.
        || !FMath::IsFinite(P.AtmosphericPressure) || P.AtmosphericPressure<.05f
        || !FMath::IsFinite(P.Temperature) || !FMath::IsFinite(P.Humidity)) return L;
    const float Wet=FMath::Clamp(P.Humidity,0.f,1.f), Temp=FMath::Clamp(P.Temperature,0.f,1.f);
    const float Pressure=FMath::Clamp(P.AtmosphericPressure,0.f,4.f);
    const float Wind=FMath::IsFinite(WindMetresPerSecond)?FMath::Clamp(WindMetresPerSecond,0.f,300.f):0.f;
    const float Activity=FMath::IsFinite(P.SeismicActivity)?FMath::Clamp(P.SeismicActivity,0.f,1.f):0.f;
    const auto S=Settings.Sanitized();
    switch(P.PlanetType)
    {
    case EPlanetType::Terrestrial: case EPlanetType::Ocean: case EPlanetType::Water:
    case EPlanetType::Oasis: case EPlanetType::Forest: case EPlanetType::SuperEarth:
    case EPlanetType::Nordic: case EPlanetType::Tundra: case EPlanetType::HighMountain:
    case EPlanetType::Archipelago: case EPlanetType::Pangea: case EPlanetType::Savanna:
        if(Wet<.12f || Temp>.82f) return L;
        L.Condensate=Temp<.28f?ECondensate::Ice:ECondensate::Water;
        break;
    case EPlanetType::Frozen: case EPlanetType::Ice:
        if(Wet<.12f || Temp>.45f) return L;
        L.Condensate=ECondensate::Ice; break;
    case EPlanetType::Ammonia:
        if(Temp>.60f) return L;
        L.Condensate=ECondensate::Ammonia; break;
    case EPlanetType::Greenhouse: case EPlanetType::Sulfur:
        L.Condensate=ECondensate::AcidAerosol; break;
    case EPlanetType::Volcanic: case EPlanetType::Lava: case EPlanetType::Melted:
        if(Activity<.05f) return L;
        L.Condensate=ECondensate::Ash; break;
    case EPlanetType::Desert: case EPlanetType::Sand:
        if(Wind<2.f) return L;
        L.Condensate=ECondensate::Dust; break;
    case EPlanetType::Carbon:
        if(Temp>.70f) return L;
        L.Condensate=ECondensate::Hydrocarbon; break;
    default: return L; // Gas-giant shader, vacuum rock and authored worlds unchanged.
    }
    L.Coverage=.18f+.62f*Wet;
    L.ThicknessKm=1.f+2.f*Wet;
    L.BottomKm=2.f+3.f*(1.f-Wet);
    L.Swirl=FMath::Clamp((Wind-5.f)/45.f,0.f,1.f)*Wet;
    L.Banding=.25f;
    switch(L.Condensate)
    {
    case ECondensate::Ice:
        L.Albedo=FLinearColor(.87f,.93f,1.f,1); L.Density=.55f;
        L.ThicknessKm=1.4f; L.Banding=.75f; L.Swirl*=.3f; break;
    case ECondensate::Ammonia:
        L.Albedo=FLinearColor(.88f,.86f,.73f,1); L.Coverage=.3f+.1f*Pressure;
        L.Density=.8f; L.Banding=.65f; L.Swirl=FMath::Clamp(Wind/100.f,0.f,.5f); break;
    case ECondensate::AcidAerosol:
        L.Albedo=FLinearColor(.87f,.76f,.49f,1); L.Coverage=.5f+.1f*Pressure;
        L.Density=.65f; L.ThicknessKm=4.f; L.BottomKm=10.f;
        L.Banding=.9f; L.Swirl=0; break;
    case ECondensate::Ash:
        L.Albedo=FLinearColor(.27f,.25f,.23f,1); L.Coverage=.12f+.35f*Activity;
        L.Density=.8f; L.Banding=.35f; L.Swirl=0; break;
    case ECondensate::Dust:
        L.Albedo=FLinearColor(.62f,.43f,.27f,1); L.Coverage=FMath::Clamp(Wind/100.f,.08f,.45f);
        L.Density=.3f; L.BottomKm=.6f; L.ThicknessKm=1.2f; L.Banding=.8f; L.Swirl=0; break;
    case ECondensate::Hydrocarbon:
        L.Albedo=FLinearColor(.59f,.42f,.24f,1); L.Coverage=.35f+.08f*Pressure;
        L.Density=.4f; L.Banding=.8f; L.Swirl=0; break;
    default: break;
    }
    // Pressure controls optical mass; sliders cannot create clouds in vacuum.
    L.Density*=FMath::Clamp(Pressure,.15f,1.5f)*float(S.DensityScale);
    L.Coverage=FMath::Clamp(L.Coverage*float(S.CoverageScale),0.f,1.f);
    L.BottomKm=FMath::Clamp(L.BottomKm*float(S.AltitudeScale),.2f,float(AtmosphereKm)*.55f);
    L.ThicknessKm=FMath::Min(L.ThicknessKm,float(AtmosphereKm)*.85f-L.BottomKm);
    L.FeatureScale=float(S.FeatureScale);
    L.WindKmPerSecond=Wind*.001f*float(S.WindScale);
    L.Swirl*=float(S.StormScale);
    FRandomStream R(int32(uint32(P.BiomeSeed)^uint32(S.SeedOffset)*1664525u^0x2A43F91u));
    L.Offset=FVector(R.FRandRange(-50,50),R.FRandRange(-50,50),R.FRandRange(-50,50));
    L.Enabled=L.Coverage>0 && L.Density>0 && L.ThicknessKm>0;
    return L;
}
// Whole field advection: macro fronts AND physical billows rotate together.
// Modulo is safe because the shader uses sin/cos, no weather reseeding on return.
// UE's PI is a float; use a double period to avoid a seam in double-precision sin/cos.
inline double AdvanceWindPhase(double Phase,double DeltaSeconds,double RadiusKm,float SpeedKmPerSecond)
{
    if(!FMath::IsFinite(Phase)) Phase=0.;
    if(!FMath::IsFinite(DeltaSeconds)||DeltaSeconds<=0.||!FMath::IsFinite(RadiusKm)||RadiusKm<=0.
        ||!FMath::IsFinite(SpeedKmPerSecond)||SpeedKmPerSecond<=0.f) return Phase;
    const double DeltaPhase=DeltaSeconds*(double(SpeedKmPerSecond)/RadiusKm);
    if(!FMath::IsFinite(DeltaPhase)) return Phase;
    return FMath::Fmod(FMath::Fmod(Phase,2.*UE_DOUBLE_PI)+FMath::Fmod(DeltaPhase,2.*UE_DOUBLE_PI),2.*UE_DOUBLE_PI);
}
inline FVector RotationFromPhase(double Phase)
{
    if(!FMath::IsFinite(Phase)) return FVector(1,0,0);
    const double Wrapped=FMath::Fmod(Phase,2.*UE_DOUBLE_PI);
    return FVector(FMath::Cos(Wrapped),FMath::Sin(Wrapped),0);
}
// Fixed-speed diagnostic helper; runtime integrates phase so changing speed
// never reapplies that speed to the entire elapsed world time.
inline FVector WindRotation(double Seconds,double RadiusKm,float SpeedKmPerSecond)
{
    if(!FMath::IsFinite(Seconds)||!FMath::IsFinite(RadiusKm)||RadiusKm<=0
        ||!FMath::IsFinite(SpeedKmPerSecond)) return FVector(1,0,0);
    const double Phase=FMath::Fmod(Seconds*double(SpeedKmPerSecond)/RadiusKm,2.*UE_DOUBLE_PI);
    return FVector(FMath::Cos(Phase),FMath::Sin(Phase),0);
}
}
