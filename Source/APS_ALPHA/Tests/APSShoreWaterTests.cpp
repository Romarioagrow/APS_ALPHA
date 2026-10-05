#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/ScopeExit.h"
#include "MaterialDomain.h"
#include "APS_ALPHA/Core/Planetary/APSSharedGeneratedLiquidMaterial.h"
#include "UObject/StrongObjectPtr.h"
#include "ProceduralMeshComponent.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAPSShoreWaterFactoryTest,
    "APS.Gameplay.World.PlanetSurface.ShoreWater.Factory",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FAPSShoreWaterFactoryTest::RunTest(const FString&)
{
    auto* Trial=IConsoleManager::Get().FindConsoleVariable(TEXT("aps.Surface.ShoreWater"));
    auto* Coastal=IConsoleManager::Get().FindConsoleVariable(TEXT("aps.Surface.CoastalWater"));
    if (!TestNotNull(TEXT("trial exists"),Trial) || !TestNotNull(TEXT("coastal exists"),Coastal)) return false;
    const int32 Previous=Trial->GetInt(), PreviousCoastal=Coastal->GetInt();
    ON_SCOPE_EXIT {Trial->Set(Previous,ECVF_SetByCode); Coastal->Set(PreviousCoastal,ECVF_SetByCode);};
    Trial->Set(1,ECVF_SetByCode); Coastal->Set(1,ECVF_SetByCode);
    using namespace APSSharedGeneratedLiquidMaterial;
    FAPSResolvedPlanetSurfaceProfile P; P.PlanetType=EPlanetType::Terrestrial;
    P.LiquidType=EAPSPlanetLiquidType::Water; P.LandCoverage=.5f;
    // Independent saved IDs: all concrete solid Water profiles are eligible in
    // the unified default. This fixture does not add water to native dry/lava presets.
    const TArray<int32> UnifiedWaterTypes={0,1,2,3,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,31,32,33,34};
    const TArray<int32> LegacyWaterTypes={1,9,25};
    TestEqual(TEXT("explicit shore fixture covers the current preset enum"),int32(APSPlanetTypes::LastValue),34);
#if WITH_EDITOR
    const bool bExpectUnified=!FParse::Param(FCommandLine::Get(),TEXT("APSLegacySurfacePipelineControl"));
#else
    const bool bExpectUnified=true;
#endif
    for (int32 TypeId=0;TypeId<=34;++TypeId)
    {
        P.PlanetType=static_cast<EPlanetType>(TypeId);
        const bool bExpectShore=(bExpectUnified ? UnifiedWaterTypes : LegacyWaterTypes).Contains(TypeId);
        TestEqual(FString::Printf(TEXT("Water-profile shore route type %d unified %d"),TypeId,bExpectUnified),
            FString(TemplatePath(P))==APSShoreWaterMaterial::TemplatePath,bExpectShore);
    }
    P.PlanetType=EPlanetType::Terrestrial;
    for (auto T:{EAPSPlanetLiquidType::Lava,EAPSPlanetLiquidType::Ammonia})
    { P.LiquidType=T; TestEqual(TEXT("other chemistry unchanged"),FString(TemplatePath(P)),FString(TemplatePath(T))); }
    P.LiquidType=EAPSPlanetLiquidType::Water;
    TStrongObjectPtr<UMaterialInstance> Source(LoadObject<UMaterialInstance>(nullptr,SourcePath(P.LiquidType)));
    TStrongObjectPtr<USceneComponent> Frame(NewObject<USceneComponent>(GetTransientPackage()));
    TStrongObjectPtr<UMaterialInstanceDynamic> Ground(Create(GetTransientPackage(),Frame.Get(),1.,false,false,P,Source.Get()));
    TStrongObjectPtr<UMaterialInstanceDynamic> Orbit(Create(GetTransientPackage(),Frame.Get(),.001,true,false,P,Source.Get()));
    if (!TestNotNull(TEXT("factory ground"),Ground.Get()) || !TestNotNull(TEXT("factory preview"),Orbit.Get())) return false;
    TestTrue(TEXT("same saved shore template"),Ground->Parent==Orbit->Parent && APSShoreWaterMaterial::IsInstance(Ground.Get()));
    TestTrue(TEXT("native depth admits shore stack"),APSCoastalWaterMaterial::IsInstance(Ground.Get()));
    TestNull(TEXT("manual source unchanged"),Create(GetTransientPackage(),Frame.Get(),1.,false,true,P,Source.Get()));
    FAPSResolvedPlanetSurfaceProfile Dry=P;
    Dry.PlanetType=EPlanetType::Frozen; Dry.Archetype=EAPSPlanetSurfaceArchetype::Cryogenic;
    Dry.LiquidType=EAPSPlanetLiquidType::None;
    TestNull(TEXT("Frozen without liquid has no liquid template"),TemplatePath(Dry));
    TestFalse(TEXT("Frozen without liquid cannot enter the coastal route"),APSCoastalWaterMaterial::Allows(Dry));
    TestNull(TEXT("Frozen without liquid cannot create an ocean from a Water source"),
        Create(GetTransientPackage(),Frame.Get(),1.,false,false,Dry,Source.Get()));
    FAPSResolvedPlanetSurfaceProfile Magmatic=P;
    Magmatic.Archetype=EAPSPlanetSurfaceArchetype::Magmatic; Magmatic.LiquidType=EAPSPlanetLiquidType::Lava;
    for (auto Type:{EPlanetType::Melted,EPlanetType::Volcanic,EPlanetType::Lava})
    {
        Magmatic.PlanetType=Type;
        TestEqual(FString::Printf(TEXT("magmatic Lava template preserved type %d"),int32(Type)),
            FString(TemplatePath(Magmatic)),FString(APSSharedLavaMaterial::TemplatePath()));
        TestFalse(FString::Printf(TEXT("magmatic Lava never enters coastal Water type %d"),int32(Type)),
            APSCoastalWaterMaterial::Allows(Magmatic));
    }
    float Opacity=-1; Ground->GetScalarParameterValue(FHashedMaterialParameterInfo(TEXT("APS_ShoreOpaqueDepthM")),Opacity);
    TestEqual(TEXT("saved metre feather"),Opacity,3.f);
    Trial->Set(0,ECVF_SetByCode);
    TestTrue(TEXT("live identity independent of switch"),IsFamilyInstance(Ground.Get(),P.LiquidType));
    TestEqual(TEXT("next profile returns to WaterV1"),FString(TemplatePath(P)),FString(APSCoastalWaterMaterial::TemplatePath));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAPSShoreWaterLightingTest,
    "APS.Gameplay.World.PlanetSurface.ShoreWater.Lighting",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FAPSShoreWaterLightingTest::RunTest(const FString&)
{
    const auto Values=UWorld::InitializationValues().AllowAudioPlayback(false).CreatePhysicsScene(false)
        .CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false).SetTransactional(false);
    UWorld* World=UWorld::CreateWorld(EWorldType::Editor,false,NAME_None,nullptr,false,ERHIFeatureLevel::Num,&Values);
    if (!TestNotNull(TEXT("isolated light world"),World)) return false;
    ON_SCOPE_EXIT {World->DestroyWorld(false);};
    FActorSpawnParameters Params; Params.ObjectFlags=RF_Transient;
    auto* Key=World->SpawnActor<ADirectionalLight>(ADirectionalLight::StaticClass(),FVector::ZeroVector,FRotator::ZeroRotator,Params);
    auto* KeyLight=Key ? Cast<UDirectionalLightComponent>(Key->GetLightComponent()) : nullptr;
    if (!TestNotNull(TEXT("key light"),KeyLight)) return false;
    KeyLight->SetForwardShadingPriority(1); KeyLight->SetIntensity(9.5f);
    FLinearColor Direction,Irradiance;
    TestTrue(TEXT("key only requires no synthetic fill"),APSWaterSurfaceLighting::Resolve(World,Direction,Irradiance));
    TestEqual(TEXT("absent fill is black"),Irradiance,FLinearColor::Black);
    auto* Fill=World->SpawnActor<ADirectionalLight>(ADirectionalLight::StaticClass(),FVector::ZeroVector,FRotator(35,42,0),Params);
    auto* L=Fill ? Cast<UDirectionalLightComponent>(Fill->GetLightComponent()) : nullptr;
    if (!TestNotNull(TEXT("fill light"),L)) return false;
    Fill->Tags.Add(TEXT("APSGameplaySurfaceFillLight"));
    L->SetForwardShadingPriority(0); L->SetCastShadows(false); L->SetSpecularScale(0);
    L->SetLightColor(FLinearColor(.72,.82,1)); L->SetIntensity(.65);
    TestTrue(TEXT("gameplay fill supported"),APSWaterSurfaceLighting::Resolve(World,Direction,Irradiance));
    TestEqual(TEXT("actual coloured brightness, not palette gain"),Irradiance,L->GetColoredLightBrightness());
    Fill->Tags.Reset(); Fill->Tags.Add(TEXT("APSPreviewFillLight"));
    TestTrue(TEXT("preview same diffuse contract"),APSWaterSurfaceLighting::Resolve(World,Direction,Irradiance));
    L->SetSpecularScale(1);
    TestFalse(TEXT("specular secondary cannot be silently approximated"),APSWaterSurfaceLighting::Resolve(World,Direction,Irradiance));
    L->SetSpecularScale(0); L->SetCastShadows(true);
    TestFalse(TEXT("shadowed secondary cannot be silently approximated"),APSWaterSurfaceLighting::Resolve(World,Direction,Irradiance));
    TestEqual(TEXT("read-only resolver preserves sun intensity"),KeyLight->Intensity,9.5f);
    TestEqual(TEXT("read-only resolver preserves fill priority"),L->ForwardShadingPriority,0);
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAPSShoreWaterPreviewBindingTest,
    "APS.Gameplay.World.PlanetSurface.ShoreWater.VisiblePreviewBinding",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FAPSShoreWaterPreviewBindingTest::RunTest(const FString&)
{
    const auto Values=UWorld::InitializationValues().AllowAudioPlayback(false).CreatePhysicsScene(false)
        .CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false).SetTransactional(false);
    auto* World=UWorld::CreateWorld(EWorldType::Editor,false,NAME_None,nullptr,false,ERHIFeatureLevel::Num,&Values);
    if(!TestNotNull(TEXT("isolated preview ownership world"),World))return false;
    ON_SCOPE_EXIT {World->DestroyWorld(false);};
    FActorSpawnParameters Params;Params.ObjectFlags=RF_Transient;
    auto* Preview=World->SpawnActor<AActor>(AActor::StaticClass(),FVector::ZeroVector,FRotator::ZeroRotator,Params);
    auto* Other=World->SpawnActor<AActor>(AActor::StaticClass(),FVector::ZeroVector,FRotator::ZeroRotator,Params);
    auto* Template=LoadObject<UMaterialInstance>(nullptr,APSShoreWaterMaterial::TemplatePath);
    if(!TestNotNull(TEXT("preview owner"),Preview)||!TestNotNull(TEXT("other owner"),Other)
        ||!TestNotNull(TEXT("saved shore template"),Template))return false;
    Preview->Tags.Add(TEXT("WorldGenerationPreview"));Other->Tags.Add(TEXT("WorldGenerationPreview"));
    auto* Ocean=NewObject<UProceduralMeshComponent>(Preview,NAME_None,RF_Transient);
    Preview->AddInstanceComponent(Ocean);Preview->SetRootComponent(Ocean);Ocean->RegisterComponent();
    auto* MID=UMaterialInstanceDynamic::Create(Template,Ocean);
    Ocean->SetMaterial(0,MID);Ocean->SetVisibility(true);Ocean->SetHiddenInGame(false);
    using namespace APSShoreWaterMaterial;
    TestTrue(TEXT("visible committed ocean qualifies without a body-owned resolver"),IsVisiblePreviewOcean(Preview,Ocean));
    TestFalse(TEXT("other preview cannot borrow this ocean"),IsVisiblePreviewOcean(Other,Ocean));
    Ocean->SetVisibility(false);
    TestFalse(TEXT("hidden retained buffer does not request a light lease"),IsVisiblePreviewOcean(Preview,Ocean));
    Ocean->SetVisibility(true);Ocean->SetHiddenInGame(true);
    TestFalse(TEXT("game-hidden buffer excluded"),IsVisiblePreviewOcean(Preview,Ocean));
    Ocean->SetHiddenInGame(false);Preview->SetActorHiddenInGame(true);
    TestFalse(TEXT("hidden preview excluded"),IsVisiblePreviewOcean(Preview,Ocean));
    Preview->SetActorHiddenInGame(false);Preview->Tags.Reset();
    TestFalse(TEXT("ordinary gameplay owner excluded"),IsVisiblePreviewOcean(Preview,Ocean));
    Preview->Tags.Add(TEXT("WorldGenerationPreview"));Ocean->SetMaterial(0,UMaterial::GetDefaultMaterial(MD_Surface));
    TestFalse(TEXT("other material does not request a light lease"),IsVisiblePreviewOcean(Preview,Ocean));
    Ocean->SetMaterial(0,MID);
    TestTrue(TEXT("restored candidate binding qualifies"),IsVisiblePreviewOcean(Preview,Ocean));
    Ocean->UnregisterComponent();
    TestFalse(TEXT("retired component excluded even with a live material"),IsVisiblePreviewOcean(Preview,Ocean));
    return true;
}
#endif
