#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "APS_ALPHA/Actors/Astro/Planet.h"
#include "APS_ALPHA/Core/Model/GeneratedWorld.h"
#include "APS_ALPHA/Core/Saves/APSWorldSaveSnapshot.h"
#include "APS_ALPHA/Core/Saves/GameSave.h"
#include "APS_ALPHA/Generation/AstroGenerator.h"
#include "APS_ALPHA/UI/MainMenu/WorldGenerationViewModel.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"

namespace APSGasMaterialSeedTests
{
    struct FScopedWorld
    {
        UWorld* World = nullptr;
        ~FScopedWorld() { if (World) World->DestroyWorld(false); }
    };
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAPSGasMaterialSeedRoundtripTest,
    "APS.UI.Generation.GasMaterialSeed.SetterAndPersistence",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAPSGasMaterialSeedRoundtripTest::RunTest(const FString& Parameters)
{
    const UWorld::InitializationValues Values = UWorld::InitializationValues()
        .AllowAudioPlayback(false).RequiresHitProxies(false).CreatePhysicsScene(false)
        .CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false).SetTransactional(false);
    APSGasMaterialSeedTests::FScopedWorld Fixture{UWorld::CreateWorld(
        EWorldType::Game, false, NAME_None, nullptr, false, ERHIFeatureLevel::Num, &Values)};
    UWorld* World = Fixture.World;
    if (!TestNotNull(TEXT("Gas material seed test world"), World)) return false;
    AAstroGenerator* Generator = World->SpawnActor<AAstroGenerator>();
    APlanet* Planet = World->SpawnActor<APlanet>();
    if (!TestTrue(TEXT("Public body fixture actors"), Generator && Planet)) return false;
    Generator->HomePlanet = Planet;
    const FString StableKey = Generator->GetPreviewBodyStableKey(Planet);
    if (!TestEqual(TEXT("Stable home body key"), StableKey, FString(TEXT("SYS0/S0/P0")))) return false;

    constexpr double RadiusKm = 70000.0;
    for (const EPlanetType Type : {EPlanetType::GasGiant, EPlanetType::HotGiant, EPlanetType::IceGiant})
    {
        const FString Label = StaticEnum<EPlanetType>()->GetNameStringByValue(static_cast<int64>(Type));
        UGeneratedWorld* Model = NewObject<UGeneratedWorld>();
        Model->PlanetType = Type;
        Model->PlanetRadius = RadiusKm;
        Model->PlanetSurfaceSeed = 17;
        Model->GenerationSeed = 271828;
        UWorldGenerationViewModel* VM = NewObject<UWorldGenerationViewModel>();
        // Exercise the real public editor-buffer setter, but do not pretend this
        // non-Slate world proves UI enablement, debounced live preview or pixels.
        VM->Initialize(GetTransientPackage(), Model);
        AAstroGenerator::ApplyPlanetaryBodyRadius(*Planet, RadiusKm);
        UMaterialInstanceDynamic* OriginalMID = nullptr;
        float FirstPatternSeed = -1.0f;
        float SecondPatternSeed = -1.0f;

        const auto CheckMaterial = [&](const FString& Step, const int32 ExpectedSeed, float& PatternSeed)
        {
            TestEqual(Step + TEXT(" actor seed"), Planet->WorldScapeSeed, ExpectedSeed);
            TestEqual(Step + TEXT(" physical radius unchanged"), Planet->RadiusKM, RadiusKm);
            TestEqual(Step + TEXT(" type unchanged"), Planet->PlanetType, Type);
            UMaterialInstanceDynamic* MID = Planet->GasGiantMaterialInstance;
            if (!TestNotNull(Step + TEXT(" real gas MID"), MID)) return false;
            if (!OriginalMID) OriginalMID = MID;
            TestTrue(Step + TEXT(" reuses existing MID"), MID == OriginalMID);
            TestTrue(Step + TEXT(" MID is bound to the real gas mesh"),
                IsValid(Planet->GasGiantVisualComponent) && Planet->GasGiantVisualComponent->GetMaterial(0) == MID);
            if (!TestTrue(Step + TEXT(" GasPatternSeed uniform exists"),
                MID->GetScalarParameterValue(FMaterialParameterInfo(TEXT("GasPatternSeed")), PatternSeed))) return false;
            const uint32 StableHash = HashCombine(GetTypeHash(ExpectedSeed), GetTypeHash(static_cast<uint8>(Type)));
            TestEqual(Step + TEXT(" material receives canonical seed hash"), PatternSeed, static_cast<float>(StableHash % 4096u));
            return true;
        };

        int32 EditIndex = 0;
        for (const int32 Seed : {1, 2, 1})
        {
            const FString Step = FString::Printf(TEXT("%s edit%d seed%d"), *Label, EditIndex, Seed);
            VM->SetPlanetSurfaceSeed(Seed);
            TestEqual(Step + TEXT(" public setter updates editor buffer"), Model->PlanetSurfaceSeed, Seed);
            if (!TestTrue(Step + TEXT(" saves per-body override"), Generator->SavePreviewBodyEditOverride(Model, Planet))) return false;
            if (!TestTrue(Step + TEXT(" applies real override"),
                AAstroGenerator::ApplyPreviewBodyEditOverrideByKey(Model, StableKey, Planet))) return false;
            float PatternSeed = -1.0f;
            if (!CheckMaterial(Step, Seed, PatternSeed)) return false;
            if (EditIndex == 0) FirstPatternSeed = PatternSeed;
            else if (EditIndex == 1)
            {
                SecondPatternSeed = PatternSeed;
                TestNotEqual(Step + TEXT(" chosen seeds produce distinct material inputs"), SecondPatternSeed, FirstPatternSeed);
            }
            else TestEqual(Step + TEXT(" seed A-B-A restores exact material input"), PatternSeed, FirstPatternSeed);
            ++EditIndex;
        }

        VM->SetPlanetSurfaceSeed(0);
        TestEqual(Label + TEXT(" setter preserves Auto sentinel in editor buffer"), Model->PlanetSurfaceSeed, 0);
        const int32 AutoSeed = UGeneratedWorld::ResolveCanonicalSurfaceSeed(0, Model->GenerationSeed, StableKey);
        TestTrue(Label + TEXT(" Auto resolves to supported positive seed"), AutoSeed > 0 && AutoSeed <= 999983);
        if (!TestTrue(Label + TEXT(" Auto override saves"), Generator->SavePreviewBodyEditOverride(Model, Planet))) return false;
        const FAPSPreviewBodyEditOverride* Captured = Model->FindPreviewBodyEditOverride(StableKey);
        if (!TestNotNull(Label + TEXT(" Auto override exists"), Captured)) return false;
        TestEqual(Label + TEXT(" Auto is persisted canonically"), Captured->SurfaceSeed, AutoSeed);
        if (!TestTrue(Label + TEXT(" Auto override applies"),
            AAstroGenerator::ApplyPreviewBodyEditOverrideByKey(Model, StableKey, Planet))) return false;
        float AutoPatternSeed = -1.0f;
        if (!CheckMaterial(Label + TEXT(" Auto"), AutoSeed, AutoPatternSeed)) return false;
        if (!TestTrue(Label + TEXT(" public reselect reloads buffer"), Generator->LoadPreviewBodyEditOverride(Model, Planet))) return false;
        TestEqual(Label + TEXT(" reselected Auto seed is canonical"), Model->PlanetSurfaceSeed, AutoSeed);

        UGameSave* Save = NewObject<UGameSave>();
        Save->GeneratedWorldsDataArray.Add(Model->SaveWorldData());
        if (!TestTrue(Label + TEXT(" snapshot captures actual override"),
            APSWorldSaveSnapshot::Capture(Model, Save->GeneratedWorldModelData))) return false;
        // Change the live actor through the same public path before restoring;
        // otherwise applying the snapshot could pass without updating the MID.
        const int32 ReplacementSeed = AutoPatternSeed != SecondPatternSeed ? 2 : 1;
        VM->SetPlanetSurfaceSeed(ReplacementSeed);
        if (!TestTrue(Label + TEXT(" replacement seed saves"), Generator->SavePreviewBodyEditOverride(Model, Planet))) return false;
        if (!TestTrue(Label + TEXT(" replacement seed applies"),
            AAstroGenerator::ApplyPreviewBodyEditOverrideByKey(Model, StableKey, Planet))) return false;
        float ReplacementPatternSeed = -1.0f;
        if (!CheckMaterial(Label + TEXT(" replacement"), ReplacementSeed, ReplacementPatternSeed)) return false;
        TestEqual(Label + TEXT(" replacement remains deterministic"), ReplacementPatternSeed,
            ReplacementSeed == 2 ? SecondPatternSeed : FirstPatternSeed);
        TestNotEqual(Label + TEXT(" live uniform differs before snapshot restore"), ReplacementPatternSeed, AutoPatternSeed);

        UGeneratedWorld* Restored = APSWorldSaveSnapshot::Restore(Save, GetTransientPackage(), TEXT("APS_GAS_MATERIAL_SEED"));
        if (!TestNotNull(Label + TEXT(" snapshot restores"), Restored)) return false;
        TestEqual(Label + TEXT(" restored editor seed"), Restored->PlanetSurfaceSeed, AutoSeed);
        const FAPSPreviewBodyEditOverride* RestoredOverride = Restored->FindPreviewBodyEditOverride(StableKey);
        if (!TestNotNull(Label + TEXT(" restored per-body override"), RestoredOverride)) return false;
        TestEqual(Label + TEXT(" restored per-body seed"), RestoredOverride->SurfaceSeed, AutoSeed);
        if (!TestTrue(Label + TEXT(" restored override applies"),
            AAstroGenerator::ApplyPreviewBodyEditOverrideByKey(Restored, StableKey, Planet))) return false;
        float RestoredPatternSeed = -1.0f;
        if (!CheckMaterial(Label + TEXT(" snapshot"), AutoSeed, RestoredPatternSeed)) return false;
        TestEqual(Label + TEXT(" snapshot restores exact material input"), RestoredPatternSeed, AutoPatternSeed);
        VM->Shutdown();
    }
    return true;
}

#endif
