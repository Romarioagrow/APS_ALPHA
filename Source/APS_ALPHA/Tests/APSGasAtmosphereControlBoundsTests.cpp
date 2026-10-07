#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "APS_ALPHA/Actors/Astro/Planet.h"
#include "APS_ALPHA/Core/Model/GeneratedWorld.h"
#include "APS_ALPHA/Core/Saves/APSWorldSaveSnapshot.h"
#include "APS_ALPHA/Core/Saves/GameSave.h"
#include "APS_ALPHA/Generation/AstroGenerator.h"
#include "APS_ALPHA/Generation/PlanetarySurfaceGenerator.h"
#include "APS_ALPHA/UI/MainMenu/WorldGenerationViewModel.h"
#include "Engine/World.h"
#include <limits>

namespace APSGasAtmosphereControlBoundsTests
{
    struct FScopedWorld
    {
        UWorld* World = nullptr;
        ~FScopedWorld() { if (World) World->DestroyWorld(false); }
    };
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAPSGasAtmosphereEditorRoundtripTest,
    "APS.UI.Generation.GasAtmosphere.InitializationAndSaveRoundtrip",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAPSGasAtmosphereEditorRoundtripTest::RunTest(const FString& Parameters)
{
    const UWorld::InitializationValues Values = UWorld::InitializationValues()
        .AllowAudioPlayback(false).RequiresHitProxies(false).CreatePhysicsScene(false)
        .CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false).SetTransactional(false);
    APSGasAtmosphereControlBoundsTests::FScopedWorld Fixture{UWorld::CreateWorld(
        EWorldType::Game, false, NAME_None, nullptr, false, ERHIFeatureLevel::Num, &Values)};
    UWorld* World = Fixture.World;
    if (!TestNotNull(TEXT("Body override test world"), World)) return false;
    AAstroGenerator* Generator = World->SpawnActor<AAstroGenerator>();
    APlanet* Planet = World->SpawnActor<APlanet>();
    APlanetarySurfaceGenerator* Surface = World->SpawnActor<APlanetarySurfaceGenerator>();
    AAtmoScape* Atmosphere = World->SpawnActor<AAtmoScape>();
    if (!TestTrue(TEXT("Body override fixture actors"), Generator && Planet && Surface && Atmosphere)) return false;
    // The public single-body hierarchy uses the same canonical key as PLANET/SYSTEM.
    // No test-only access to view-model selection or generator internals is needed.
    Generator->HomePlanet = Planet;
    Planet->PlanetaryEnvironmentGenerator = Surface;
    Surface->PlanetAtmosphere = Atmosphere;
    const FString StableKey = Generator->GetPreviewBodyStableKey(Planet);
    if (!TestEqual(TEXT("Public home body resolves its stable path"), StableKey, FString(TEXT("SYS0/S0/P0")))) return false;

    for (const EPlanetType Type : {EPlanetType::GasGiant, EPlanetType::HotGiant,
        EPlanetType::IceGiant, EPlanetType::Terrestrial})
    for (const double RadiusKm : {5262.0, 50000.0, 60000.0, 70000.0, 200000.0})
    {
        const FString Label = FString::Printf(TEXT("type=%d radius=%.0f"), int32(Type), RadiusKm);
        // Use the generated body values, including AtmoScape's float density height.
        // At 70000 km the former view-model limits destroyed 2333.33/80 -> 2000/64.
        const double GeneratedHeight = RadiusKm / 30.0;
        const double GeneratedRayleigh = FMath::Clamp(8.0f * float(RadiusKm / 6371.0), 4.1f, 80.0f);
        const bool bSolid = Type == EPlanetType::Terrestrial;
        const double ExpectedHeight = bSolid ? FMath::Min(GeneratedHeight, 2000.0) : GeneratedHeight;
        const double ExpectedRayleigh = bSolid ? FMath::Min(GeneratedRayleigh, 64.0) : GeneratedRayleigh;

        UGeneratedWorld* Model = NewObject<UGeneratedWorld>();
        Model->PlanetType = Type;
        Model->PlanetRadius = RadiusKm;
        Model->GenerationSeed = 883716;
        Model->PlanetSurfaceSeed = 31791;
        Model->SurfaceReliefScale = 1.7;
        Model->AtmosphereHeight = GeneratedHeight;
        Model->AtmosphereRayleighScattering = GeneratedRayleigh;
        Model->AtmosphereOpacity = 1.25;
        Model->AtmosphereColor = FLinearColor(4.0f, 9.0f, 33.0f, 0.0f);
        UWorldGenerationViewModel* VM = NewObject<UWorldGenerationViewModel>();
        VM->Initialize(GetTransientPackage(), Model);
        TestEqual(Label + TEXT(" initialization retains height"), Model->AtmosphereHeight, ExpectedHeight);
        TestEqual(Label + TEXT(" initialization retains density height"), Model->AtmosphereRayleighScattering, ExpectedRayleigh);

        // An unrelated authored edit must not turn the already-initialized sky into
        // a truncated save, and reopening that save must retain the same values.
        Model->CloudSettings.CoverageScale = 0.7;
        Planet->PlanetType = Type;
        Planet->RadiusKM = RadiusKm;
        Planet->PlanetRadiusKM = FMath::RoundToInt(RadiusKm);
        if (!TestTrue(Label + TEXT(" public body override saves"), Generator->SavePreviewBodyEditOverride(Model, Planet)))
            continue;
        const FAPSPreviewBodyEditOverride* Captured = Model->FindPreviewBodyEditOverride(StableKey);
        if (!TestNotNull(Label + TEXT(" actual captured body override"), Captured)) continue;
        // These assertions fail against CaptureEditorBuffer's old 2000/64 clamps
        // even if model initialization and the snapshot serializer already pass.
        TestEqual(Label + TEXT(" captured body height survives"), Captured->AtmosphereHeight, ExpectedHeight);
        TestEqual(Label + TEXT(" captured body density height survives"), Captured->AtmosphereRayleighScattering, ExpectedRayleigh);
        Model->PlanetType = EPlanetType::Rocky;
        Model->AtmosphereHeight = 17.0;
        Model->AtmosphereRayleighScattering = 2.0;
        if (!TestTrue(Label + TEXT(" public body override reloads"), Generator->LoadPreviewBodyEditOverride(Model, Planet)))
            continue;
        TestEqual(Label + TEXT(" reselected type survives"), Model->PlanetType, Type);
        TestEqual(Label + TEXT(" reselected body height survives"), Model->AtmosphereHeight, ExpectedHeight);
        TestEqual(Label + TEXT(" reselected body density height survives"), Model->AtmosphereRayleighScattering, ExpectedRayleigh);

        UGameSave* Save = NewObject<UGameSave>();
        Save->GeneratedWorldsDataArray.Add(Model->SaveWorldData());
        if (!TestTrue(Label + TEXT(" snapshot captures"), APSWorldSaveSnapshot::Capture(Model, Save->GeneratedWorldModelData)))
            continue;
        UGeneratedWorld* Restored = APSWorldSaveSnapshot::Restore(
            Save, GetTransientPackage(), TEXT("APS_GAS_ATMOSPHERE_BOUNDS"));
        if (!TestNotNull(Label + TEXT(" snapshot restores"), Restored)) continue;
        VM->Initialize(GetTransientPackage(), Restored);
        TestEqual(Label + TEXT(" reopened height survives"), Restored->AtmosphereHeight, ExpectedHeight);
        TestEqual(Label + TEXT(" reopened density height survives"), Restored->AtmosphereRayleighScattering, ExpectedRayleigh);
        TestEqual(Label + TEXT(" physical radius unchanged"), Restored->PlanetRadius, RadiusKm);
        TestEqual(Label + TEXT(" generation seed unchanged"), Restored->GenerationSeed, 883716);
        TestEqual(Label + TEXT(" surface seed unchanged"), Restored->PlanetSurfaceSeed, 31791);
        TestEqual(Label + TEXT(" relief unchanged"), Restored->SurfaceReliefScale, 1.7);
        TestEqual(Label + TEXT(" opacity unchanged"), Restored->AtmosphereOpacity, 1.25);
        TestEqual(Label + TEXT(" scattering color unchanged"), Restored->AtmosphereColor, Model->AtmosphereColor);
        TestEqual(Label + TEXT(" unrelated cloud edit survives"), Restored->CloudSettings.CoverageScale, 0.7);
        const FAPSPreviewBodyEditOverride* RestoredOverride = Restored->FindPreviewBodyEditOverride(StableKey);
        if (TestNotNull(Label + TEXT(" snapshot contains actual body override"), RestoredOverride))
        {
            TestEqual(Label + TEXT(" snapshot body height survives"), RestoredOverride->AtmosphereHeight, ExpectedHeight);
            TestEqual(Label + TEXT(" snapshot body density height survives"), RestoredOverride->AtmosphereRayleighScattering, ExpectedRayleigh);
        }
        Planet->AtmosphereHeight = 1.0;
        Atmosphere->AtmosphereHeight = 1.0f;
        Atmosphere->RayleighHeight = 1.0f;
        if (TestTrue(Label + TEXT(" restored override applies to body"),
            AAstroGenerator::ApplyPreviewBodyEditOverrideByKey(Restored, StableKey, Planet)))
        {
            TestEqual(Label + TEXT(" restored actor height survives"), Planet->AtmosphereHeight, ExpectedHeight);
            TestEqual(Label + TEXT(" AtmoScape receives restored height"), Atmosphere->AtmosphereHeight, float(ExpectedHeight));
            TestEqual(Label + TEXT(" AtmoScape receives restored density height"), Atmosphere->RayleighHeight, float(ExpectedRayleigh));
            TestEqual(Label + TEXT(" actor physical radius unchanged"), Planet->RadiusKM, RadiusKm);
            TestEqual(Label + TEXT(" actor surface seed unchanged"), Planet->WorldScapeSeed, 31791);
            TestEqual(Label + TEXT(" actor unrelated cloud edit survives"), Planet->CloudSettings.CoverageScale, 0.7);
        }
        VM->Shutdown();
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAPSGasAtmosphereEditorSanitationTest,
    "APS.UI.Generation.GasAtmosphere.FiniteBoundsAndSolidCompatibility",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAPSGasAtmosphereEditorSanitationTest::RunTest(const FString& Parameters)
{
    const double NaN = std::numeric_limits<double>::quiet_NaN();
    const double Infinity = std::numeric_limits<double>::infinity();
    UGeneratedWorld* Model = NewObject<UGeneratedWorld>();
    UWorldGenerationViewModel* VM = NewObject<UWorldGenerationViewModel>();
    for (uint8 Value = 0; Value <= APSPlanetTypes::LastValue; ++Value)
    {
        const EPlanetType Type = static_cast<EPlanetType>(Value);
        const bool bGas = Type == EPlanetType::GasGiant || Type == EPlanetType::HotGiant
            || Type == EPlanetType::IceGiant;
        for (const double Input : {-10.0, 0.0, 40.0, 64.0, 80.0, 2000.0, 7000.0, 10000.0, NaN, Infinity, -Infinity})
        {
            Model->PlanetType = Type;
            Model->AtmosphereHeight = Input;
            Model->AtmosphereRayleighScattering = Input;
            VM->Initialize(GetTransientPackage(), Model);
            const FString Label = FString::Printf(TEXT("type=%d input=%g"), int32(Type), Input);
            const double SafeGasInput = FMath::IsFinite(Input) ? Input : 0.0;
            const double ExpectedHeight = bGas ? FMath::Clamp(SafeGasInput, 0.0, 7000.0)
                : FMath::Clamp(Input, 0.0, 2000.0);
            const double ExpectedRayleigh = bGas ? FMath::Clamp(SafeGasInput, 0.0, 80.0)
                : FMath::Clamp(Input, 0.0, 64.0);
            TestEqual(Label + TEXT(" height bound"), Model->AtmosphereHeight, ExpectedHeight);
            TestEqual(Label + TEXT(" density height bound"), Model->AtmosphereRayleighScattering, ExpectedRayleigh);
            TestTrue(Label + TEXT(" finite atmosphere inputs"), FMath::IsFinite(Model->AtmosphereHeight)
                && FMath::IsFinite(Model->AtmosphereRayleighScattering));
        }
    }
    VM->Shutdown();
    return true;
}

#endif
