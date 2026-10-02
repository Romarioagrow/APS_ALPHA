#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "APS_ALPHA/Core/Planetary/APSSharedGeneratedLiquidMaterial.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Components/SceneComponent.h"
#include "UObject/Package.h"
#include "UObject/StrongObjectPtr.h"
#include "Misc/ScopeExit.h"
#include <limits>

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAPSSharedGeneratedLiquidSelectionTest,
    "APS.Gameplay.World.PlanetSurface.SharedGeneratedLiquidSelection",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAPSSharedGeneratedLiquidSelectionTest::RunTest(const FString& Parameters)
{
    using namespace APSSharedGeneratedLiquidMaterial;
    const EAPSPlanetLiquidType Types[] = {EAPSPlanetLiquidType::Water, EAPSPlanetLiquidType::Ammonia, EAPSPlanetLiquidType::Lava};
    // Independent known asset names: do not load through the selector under test.
    const TCHAR* Sources[] = {
        TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/Materials/MI_APS_WS_Water.MI_APS_WS_Water"),
        TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/Materials/MI_APS_WS_Ammonia.MI_APS_WS_Ammonia"),
        TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/Materials/MI_APS_WS_Lava.MI_APS_WS_Lava")};
    TStrongObjectPtr<UMaterialInstance> LoadedSources[3] = {
        TStrongObjectPtr<UMaterialInstance>(LoadObject<UMaterialInstance>(nullptr, Sources[0])),
        TStrongObjectPtr<UMaterialInstance>(LoadObject<UMaterialInstance>(nullptr, Sources[1])),
        TStrongObjectPtr<UMaterialInstance>(LoadObject<UMaterialInstance>(nullptr, Sources[2]))};
    if (!TestNotNull(TEXT("Exact saved generated Water source exists"), LoadedSources[0].Get())
        || !TestNotNull(TEXT("Exact saved generated Ammonia source exists"), LoadedSources[1].Get())
        || !TestNotNull(TEXT("Exact saved generated Lava source exists"), LoadedSources[2].Get())) return false;

    for (int32 I = 0; I < UE_ARRAY_COUNT(Types); ++I)
    {
        const FString Prefix = I == 0 ? TEXT("Water: ") : I == 1 ? TEXT("Ammonia: ") : TEXT("Lava: ");
        const int32 Other = (I + 1) % UE_ARRAY_COUNT(Types);
        UMaterialInstance* Source = LoadedSources[I].Get();
        FAPSResolvedPlanetSurfaceProfile Profile;
        Profile.PlanetType = I == 0 ? EPlanetType::Water : I == 1 ? EPlanetType::Ammonia : EPlanetType::Volcanic;
        Profile.Archetype = I == 0 ? EAPSPlanetSurfaceArchetype::Oceanic
            : I == 1 ? EAPSPlanetSurfaceArchetype::ExoticChemical : EAPSPlanetSurfaceArchetype::Magmatic;
        Profile.LiquidType = Types[I];
        Profile.LandCoverage = 0.5f;
        TestEqual(Prefix + TEXT("selector uses exact known source path"), FString(SourcePath(Profile.LiquidType)), FString(Sources[I]));
        TestTrue(Prefix + TEXT("wet generated profile is eligible"), AllowsProfile(Profile, false));
        TestTrue(Prefix + TEXT("exact saved generated source migrates"), ShouldMigrate(Source, Profile, false));
        TestFalse(Prefix + TEXT("null source cannot migrate"), ShouldMigrate(nullptr, Profile, false));
        for (int32 J = 0; J < UE_ARRAY_COUNT(Types); ++J)
            if (J != I) TestFalse(Prefix + TEXT("other liquid source cannot migrate"), ShouldMigrate(LoadedSources[J].Get(), Profile, false));
        TestFalse(Prefix + TEXT("manual profile rejected"), AllowsProfile(Profile, true));
        TestFalse(Prefix + TEXT("manual exact source remains untouched"), ShouldMigrate(Source, Profile, true));

        for (float Land : {0.0f, 0.5f, 0.994f})
        {
            Profile.LandCoverage = Land;
            TestTrue(FString::Printf(TEXT("%svalid land coverage %.6g"), *Prefix, Land), AllowsProfile(Profile, false));
            TestTrue(FString::Printf(TEXT("%sexact source at valid coverage %.6g"), *Prefix, Land), ShouldMigrate(Source, Profile, false));
        }
        const float RejectedCoverage[] = {-0.001f, 0.995f, 1.0f, 1.01f,
            std::numeric_limits<float>::quiet_NaN(), std::numeric_limits<float>::infinity(),
            -std::numeric_limits<float>::infinity()};
        for (int32 Case = 0; Case < UE_ARRAY_COUNT(RejectedCoverage); ++Case)
        {
            Profile.LandCoverage = RejectedCoverage[Case];
            TestFalse(FString::Printf(TEXT("%sdry/non-finite/out-of-range profile case %d"), *Prefix, Case), AllowsProfile(Profile, false));
            TestFalse(FString::Printf(TEXT("%sexact source rejected for invalid coverage case %d"), *Prefix, Case), ShouldMigrate(Source, Profile, false));
        }
        Profile.LandCoverage = 0.5f;
        for (EAPSPlanetLiquidType Excluded : {EAPSPlanetLiquidType::None, static_cast<EAPSPlanetLiquidType>(255)})
        {
            Profile.LiquidType = Excluded;
            const FString Case = Prefix + UEnum::GetValueAsString(Excluded);
            TestFalse(Case + TEXT(" profile not promoted"), AllowsProfile(Profile, false));
            TestFalse(Case + TEXT(" source not promoted"), ShouldMigrate(Source, Profile, false));
            TestTrue(Case + TEXT(" has no migration source path"), SourcePath(Excluded) == nullptr);
            TestTrue(Case + TEXT(" has no migration template path"), TemplatePath(Excluded) == nullptr);
        }
        Profile.LiquidType = Types[I];

        // Same parent/master is intentionally NOT the exact catalog-source object.
        TStrongObjectPtr<UMaterialInstanceDynamic> Custom(UMaterialInstanceDynamic::Create(Source, GetTransientPackage()));
        if (!TestNotNull(Prefix + TEXT("transient custom child created"), Custom.Get())) return false;
        TestTrue(Prefix + TEXT("custom child retains exact source as parent"), Custom->Parent.Get() == Source);
        TestTrue(Prefix + TEXT("custom child shares source master"), Custom->GetMaterial() == Source->GetMaterial());
        TestFalse(Prefix + TEXT("custom identity is not substituted by matching master/parent"), ShouldMigrate(Custom.Get(), Profile, false));

        TStrongObjectPtr<UAPSPlanetSurfaceCatalog> Catalog(NewObject<UAPSPlanetSurfaceCatalog>(GetTransientPackage()));
        if (!TestNotNull(Prefix + TEXT("transient catalog created"), Catalog.Get())) return false;
        FAPSPlanetSurfaceArchetypeDefinition& Definition = Catalog->Archetypes.FindOrAdd(Profile.Archetype);
        Definition.LiquidType = Types[I];
        Definition.OceanMaterial = TSoftObjectPtr<UMaterialInstance>(Custom.Get());
        const FSoftObjectPath CustomPath = Definition.OceanMaterial.ToSoftObjectPath();
        TestTrue(Prefix + TEXT("matching catalog custom source is preserved"), ResolveSource(Profile, Catalog.Get()) == Custom.Get());
        TestFalse(Prefix + TEXT("resolved catalog custom source never migrates"), ShouldMigrate(ResolveSource(Profile, Catalog.Get()), Profile, false));
        TestTrue(Prefix + TEXT("catalog selection was not rewritten"), Definition.OceanMaterial.ToSoftObjectPath() == CustomPath);

        Definition.OceanMaterial = TSoftObjectPtr<UMaterialInstance>(Source);
        TestTrue(Prefix + TEXT("catalog exact saved source is eligible"), ShouldMigrate(ResolveSource(Profile, Catalog.Get()), Profile, false));
        TestFalse(Prefix + TEXT("manual catalog exact source stays untouched"), ShouldMigrate(ResolveSource(Profile, Catalog.Get()), Profile, true));

        Definition.OceanMaterial = TSoftObjectPtr<UMaterialInstance>(Custom.Get());
        Definition.LiquidType = Types[Other];
        TestTrue(Prefix + TEXT("mismatched catalog chemistry retains existing own-family fallback"), ResolveSource(Profile, Catalog.Get()) == Source);
        Definition.LiquidType = Types[I];
        Definition.OceanMaterial.Reset();
        TestTrue(Prefix + TEXT("empty catalog material retains existing generated fallback"), ResolveSource(Profile, Catalog.Get()) == Source);
        Catalog->Archetypes.Empty();
        TestTrue(Prefix + TEXT("missing archetype retains generated fallback"), ResolveSource(Profile, Catalog.Get()) == Source);
        TestTrue(Prefix + TEXT("null catalog retains generated fallback"), ResolveSource(Profile, nullptr) == Source);

        TStrongObjectPtr<USceneComponent> Frame(NewObject<USceneComponent>(GetTransientPackage()));
        TStrongObjectPtr<UMaterialInstanceDynamic> Ground(Create(GetTransientPackage(), Frame.Get(), 1.0, false, false, Profile, Source));
        TStrongObjectPtr<UMaterialInstanceDynamic> Orbit(Create(GetTransientPackage(), Frame.Get(), 0.001, true, false, Profile, Source));
        if (!TestNotNull(Prefix + TEXT("physical ground instance"), Ground.Get())
            || !TestNotNull(Prefix + TEXT("physical orbit instance"), Orbit.Get())) return false;
        TestTrue(Prefix + TEXT("same saved parent in both modes"), Ground->Parent == Orbit->Parent);
        TestTrue(Prefix + TEXT("ground respects saved authority"), HasSavedParameterAuthority(Ground.Get(), Types[I]));
        TestTrue(Prefix + TEXT("orbit respects saved authority"), HasSavedParameterAuthority(Orbit.Get(), Types[I]));
        float GroundMask = -1, OrbitMask = -1;
        Ground->GetScalarParameterValue(FHashedMaterialParameterInfo(TEXT("APS_UsePresentationWaterMask")), GroundMask);
        Orbit->GetScalarParameterValue(FHashedMaterialParameterInfo(TEXT("APS_UsePresentationWaterMask")), OrbitMask);
        TestEqual(Prefix + TEXT("physical Hole alpha ignored"), GroundMask, 0.0f);
        TestEqual(Prefix + TEXT("closed globe shoreline alpha used"), OrbitMask, 1.0f);
        TestNull(Prefix + TEXT("manual factory refuses migration"), Create(GetTransientPackage(), Frame.Get(), 1.0, false, true, Profile, Source));
        TestNull(Prefix + TEXT("custom factory refuses migration"), Create(GetTransientPackage(), Frame.Get(), 1.0, false, false, Profile, Custom.Get()));
        if (Types[I] == EAPSPlanetLiquidType::Lava)
        {
            float Saved = -1, GroundValue = -1, OrbitValue = -1;
            Ground->Parent->GetScalarParameterValue(FHashedMaterialParameterInfo(TEXT("Brightness")), Saved);
            Ground->GetScalarParameterValue(FHashedMaterialParameterInfo(TEXT("Brightness")), GroundValue);
            Orbit->GetScalarParameterValue(FHashedMaterialParameterInfo(TEXT("Brightness")), OrbitValue);
            TestEqual(TEXT("Lava ground inherits saved radiance"), GroundValue, Saved);
            TestEqual(TEXT("Lava orbit inherits saved radiance"), OrbitValue, Saved);
            Ground->SetScalarParameterValue(TEXT("Brightness"), std::numeric_limits<float>::quiet_NaN());
            TestFalse(TEXT("Lava invalid radiance rejected"), HasSavedParameterAuthority(Ground.Get(), Types[I]));
        }
    }
    AddInfo(TEXT("Selection/frame/parameter regression on transient instances; no production asset changes, actor spawn or rendered acceptance."));
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAPSCoastalWaterReleaseTest,
    "APS.Gameplay.World.PlanetSurface.CoastalWater.ReleaseContract",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FAPSCoastalWaterReleaseTest::RunTest(const FString& Parameters)
{
    using namespace APSSharedGeneratedLiquidMaterial;
    auto* Switch=IConsoleManager::Get().FindConsoleVariable(TEXT("aps.Surface.CoastalWater"));
    if (!TestNotNull(TEXT("Rollout switch exists"),Switch)) return false;
    const int32 Previous=Switch->GetInt();
    ON_SCOPE_EXIT { Switch->Set(Previous,ECVF_SetByCode); };
    FAPSResolvedPlanetSurfaceProfile P; P.LiquidType=EAPSPlanetLiquidType::Water; P.LandCoverage=.5f;
    Switch->Set(1,ECVF_SetByCode);
    for (auto Type:{EPlanetType::Water,EPlanetType::Terrestrial,EPlanetType::Oasis,EPlanetType::Frozen,EPlanetType::Forest,EPlanetType::Metallic})
    {
        P.PlanetType=Type;
        TestEqual(TEXT("Bounded family rollout"),APSCoastalWaterMaterial::EnabledFor(P),
            Type==EPlanetType::Water || Type==EPlanetType::Terrestrial || Type==EPlanetType::Oasis);
    }
    P.PlanetType=EPlanetType::Terrestrial;
    for (auto Type:{EAPSPlanetLiquidType::None,EAPSPlanetLiquidType::Lava,EAPSPlanetLiquidType::Ammonia})
    { P.LiquidType=Type; TestFalse(TEXT("No other chemistry promoted"),APSCoastalWaterMaterial::EnabledFor(P)); }
    P.LiquidType=EAPSPlanetLiquidType::Water;
    TStrongObjectPtr<UMaterialInstance> Source(LoadObject<UMaterialInstance>(nullptr,SourcePath(P.LiquidType)));
    TStrongObjectPtr<USceneComponent> Frame(NewObject<USceneComponent>(GetTransientPackage()));
    TStrongObjectPtr<UMaterialInstanceDynamic> Ground(Create(GetTransientPackage(),Frame.Get(),1.0,false,false,P,Source.Get()));
    TStrongObjectPtr<UMaterialInstanceDynamic> Orbit(Create(GetTransientPackage(),Frame.Get(),.001,true,false,P,Source.Get()));
    if (!TestNotNull(TEXT("Release ground created"),Ground.Get()) || !TestNotNull(TEXT("Release orbit created"),Orbit.Get())) return false;
    TestTrue(TEXT("Both paths use one saved version"),Ground->Parent==Orbit->Parent && APSCoastalWaterMaterial::IsInstance(Ground.Get()));
    TestTrue(TEXT("Own Water authority"),HasSavedParameterAuthority(Ground.Get(),EAPSPlanetLiquidType::Water));
    TestFalse(TEXT("Cannot pass as ammonia"),IsFamilyInstance(Ground.Get(),EAPSPlanetLiquidType::Ammonia));
    TestNull(TEXT("Manual untouched"),Create(GetTransientPackage(),Frame.Get(),1.0,false,true,P,Source.Get()));
    const TPair<FName,float> Scalars[]={{TEXT("WaveScaleCm"),120},{TEXT("PhysicalWaveDetailScaleCm"),41},
        {TEXT("APS_WaterDepthStrength"),1},{TEXT("APS_WaterHalfDepthM"),20},{TEXT("WaveColorStrength"),0},
        {TEXT("PhysicalWaveRoughnessStrength"),0},{TEXT("WaveNormalStrength"),.025f}};
    for (const auto& S:Scalars)
    {
        float A=-1,B=-1;
        TestTrue(TEXT("Ground scalar exists"),Ground->GetScalarParameterValue(FHashedMaterialParameterInfo(S.Key),A));
        TestTrue(TEXT("Orbit scalar exists"),Orbit->GetScalarParameterValue(FHashedMaterialParameterInfo(S.Key),B));
        TestEqual(S.Key.ToString()+TEXT(" tested style"),A,S.Value); TestEqual(TEXT("Mode parity"),A,B);
    }
    Switch->Set(0,ECVF_SetByCode);
    TestTrue(TEXT("Live release identity survives rollback switch"),IsFamilyInstance(Ground.Get(),P.LiquidType));
    TestEqual(TEXT("New profiles roll back"),FString(TemplatePath(P)),FString(APSSharedWaterMaterial::TemplatePath()));
    TStrongObjectPtr<UMaterialInstanceDynamic> Legacy(Create(GetTransientPackage(),Frame.Get(),1.0,false,false,P,Source.Get()));
    TestTrue(TEXT("Rollback creates legacy shared Water"),Legacy.IsValid() && Legacy->Parent->GetPathName()==APSSharedWaterMaterial::TemplatePath());
    AddInfo(TEXT("Saved release/factory/frame contracts only; rendered coverage is separate."));
    return true;
}
#endif
