#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "APS_ALPHA/Core/Planetary/APSPlanetSurfaceScatter.h"
#include "APS_ALPHA/Core/Planetary/APSPlanetScatterMaterial.h"
#include "APS_ALPHA/Core/Planetary/APSWorldScapeFoliagePolicy.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "WorldScapeFoliages/Public/WorldScapeFoliagesAsset.h"
#include "WorldScapeFoliages/Public/WorldScapeFoliagesCollection.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAPSSurfaceScatterPolicyTest,
    "APS.Gameplay.World.PlanetSurface.Foliage.SurfaceScatterV2",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FAPSSurfaceScatterPolicyTest::RunTest(const FString& Parameters)
{
    using namespace APSPlanetSurfaceScatter;
    using Policy = FAPSWorldScapeFoliagePolicy;
    TestEqual(TEXT("Sparse Oasis spends nine unused slots only"), BoundedOasisTreeAttempts(3,45,12,64), 12);
    TestEqual(TEXT("Original Oasis density is reversible"), BoundedOasisTreeAttempts(3,45,3,64), 3);
    TestEqual(TEXT("No spare capacity does not grow the envelope"), BoundedOasisTreeAttempts(3,61,12,64), 3);
    TestEqual(TEXT("Partial spare capacity clamps requested growth"), BoundedOasisTreeAttempts(3,58,12,64), 6);
    TestEqual(TEXT("Unbounded CVar input remains bounded"), BoundedOasisTreeAttempts(3,45,999,64), 16);
    TestEqual(TEXT("Bad budget preserves sanitized original"), BoundedOasisTreeAttempts(3,65,12,64), 3);
    for (uint8 V = 0; V <= APSPlanetTypes::LastValue; ++V)
        for (bool Habitat : {false, true})
        {
            FAPSResolvedPlanetSurfaceProfile P;
            P.PlanetType = EPlanetType(V); P.Biomass = P.Biodiversity = Habitat ? .7f : 0.f;
            const bool Expected = P.PlanetType == EPlanetType::Frozen || P.PlanetType == EPlanetType::Desert
                || P.PlanetType == EPlanetType::Volcanic
                || (!Habitat && (P.PlanetType == EPlanetType::Terrestrial || P.PlanetType == EPlanetType::Tundra
                    || P.PlanetType == EPlanetType::Water || P.PlanetType == EPlanetType::Metallic
                    || P.PlanetType == EPlanetType::Crystal))
                || (Habitat && (P.PlanetType == EPlanetType::Forest || P.PlanetType == EPlanetType::Oasis));
            TestEqual(TEXT("Published subset requires the reviewed type AND habitat"), UsesScatter(P, 2), Expected);
            TestFalse(TEXT("Scatter disabled retains old policy"), UsesScatter(P, 0));
            TestFalse(TEXT("Unknown scatter mode fails closed"), UsesScatter(P, 3));
            TestTrue(TEXT("Explicit candidate remains available separately"), UsesScatter(P, 1));
            if (!Expected) continue;
            const auto Plan = Policy::BuildRuntimeActivationPlan(P, 2, false, false, false, UsesScatter(P, 2));
            TestTrue(TEXT("Published candidate uses bounded five-mesh plan"), Plan.bEnabled && Plan.bSurfaceScatterPalette);
            TestFalse(TEXT("Published candidate cannot bypass master off"), Policy::BuildRuntimeActivationPlan(P, 0, false, false, false, true).bEnabled);
            TestFalse(TEXT("Published candidate cannot enter orbital preview"), Policy::BuildRuntimeActivationPlan(P, 2, true, false, false, true).bEnabled);
            TestFalse(TEXT("Published candidate cannot alter manual planet"), Policy::BuildRuntimeActivationPlan(P, 2, false, false, true, true).bEnabled);
        }
    for (double Bottom : {-800., -50., 0., 100.})
        for (bool Biological : {false, true})
        {
            const double Offset = GroundOffsetCm(Bottom, 200., .08, .12, Biological);
            for (double Scale : {.08, .10, .12})
                TestTrue(TEXT("Unscaled native asset offset keeps every size at or below ground"),
                    Bottom * Scale + Offset <= -.15);
        }
    TSet<FString> Paths;
    for (uint8 V = 0; V <= APSPlanetTypes::LastValue; ++V)
    {
        FAPSResolvedPlanetSurfaceProfile P;
        P.PlanetType = EPlanetType(V); P.Biomass = P.Biodiversity = .7f;
        const auto Signature = UAPSPlanetSurfaceProfileResolver::BuildProfileSignature(P);
        const bool Supported = UAPSPlanetSurfaceProfileResolver::SupportsWorldScape(P.PlanetType);
        const auto Plan = Policy::BuildRuntimeActivationPlan(P, 2, false, false, false, true);
        TestEqual(TEXT("Solid worlds only"), Plan.bEnabled, Supported);
        TestFalse(TEXT("Master off"), Policy::BuildRuntimeActivationPlan(P, 0, false, false, false, true).bEnabled);
        TestFalse(TEXT("Preview excluded"), Policy::BuildRuntimeActivationPlan(P, 2, true, false, false, true).bEnabled);
        TestFalse(TEXT("Manual excluded"), Policy::BuildRuntimeActivationPlan(P, 2, false, false, true, true).bEnabled);
        TestFalse(TEXT("Unknown mode rejected"), Policy::BuildRuntimeActivationPlan(P, 3, false, false, false, true).bEnabled);
        if (!Supported) continue;
        const auto Tint = APSPlanetScatterMaterial::Tint(P.PlanetType);
        TestTrue(TEXT("Minerals have finite low-chroma non-emissive linear albedo"),
            FMath::IsFinite(Tint.R) && FMath::IsFinite(Tint.G) && FMath::IsFinite(Tint.B)
            && Tint.GetMin() > 0 && Tint.GetMax() <= 1
            && FMath::Max3(Tint.R,Tint.G,Tint.B)-FMath::Min3(Tint.R,Tint.G,Tint.B) <= .12f);
        TestTrue(TEXT("Mixed palette preserves species-specific masks"), Plan.bSurfaceScatterPalette && Plan.bPreserveEntryNoiseMask);
        TestEqual(TEXT("Five allowed species"), Plan.MaxTypesPerCollection, 5);
        TestTrue(TEXT("Component envelope stays under existing cap"),
            Plan.MaxTypesPerCollection * Policy::PeakSectorEnvelopePerType <= Policy::MaximumPeakMeshComponentsPerRoot);
        TestTrue(TEXT("Instance envelope stays under existing cap"),
            Plan.MaxInstancesPerSectorPerCollection * Policy::PeakSectorEnvelopePerType <= Policy::MaximumPeakInstancesPerRoot);
        for (bool Habitat : {false, true})
        {
            const FString Path = CollectionPath(P.PlanetType, Habitat);
            TestFalse(TEXT("Independently replaceable per-type and mineral palette"), Paths.Contains(Path));
            Paths.Add(Path);
        }
        auto* Source = NewObject<UWorldScapeFoliagesCollection>();
        TSet<EMesh> Shapes;
        int32 Count = 0;
        for (const auto& S : Recipe(P.PlanetType).Species)
        {
            Shapes.Add(S.Mesh); Count += S.Attempts;
            TestTrue(TEXT("Plausible physical size and finite dimensions"), S.HeightCm >= 10 && S.HeightCm <= 1000);
            TestTrue(TEXT("Local-only bounded sectors"), S.SectorCm >= 8000 && S.SectorCm <= 18000);
            auto* Entry = NewObject<UWorldScapeFoliagesAsset>(Source);
            Entry->StaticMesh = NewObject<UStaticMesh>(); Entry->FoliagesCount = S.Attempts;
            Entry->FoliageSectorSize = S.SectorCm; Entry->bUseFoliageNoiseMask = S.Biological;
            Source->FoliageList.Add(Entry);
        }
        TestEqual(TEXT("Five source silhouettes, not scale duplicates"), Shapes.Num(), 5);
        TestTrue(TEXT("Total attempts remain bounded"), Count <= 64);
        TArray<UWorldScapeFoliagesCollection*> Out;
        TestEqual(TEXT("One safe palette produced"), Policy::BuildBudgetedCollections(GetTransientPackage(), {Source}, Plan, Out), 1);
        if (Out.Num() == 1 && TestEqual(TEXT("No species lost to admission"), Out[0]->FoliageList.Num(), 5))
        {
            for (int32 I = 0; I < 5; ++I)
            {
                auto* Entry = Cast<UWorldScapeFoliagesAsset>(Out[0]->FoliageList[I]);
                if (!TestNotNull(TEXT("HISM asset entry"), Entry)) continue;
                TestEqual(TEXT("Rock does not inherit biological mask"), Entry->bUseFoliageNoiseMask, Recipe(P.PlanetType).Species[I].Biological);
                TestFalse(TEXT("Visual-only flags"), Entry->bCollision || Entry->bCastShadows || Entry->bGenerateOnServer || Entry->bSpawnActorInstead);
            }
        }
        TestEqual(TEXT("Input save signature untouched"), UAPSPlanetSurfaceProfileResolver::BuildProfileSignature(P), Signature);
        P.Foliage.bEnabled = true;
        TestFalse(TEXT("Authored setting untouched"), Policy::BuildRuntimeActivationPlan(P, 2, false, false, false, true).bEnabled);
        P.Foliage.Collections = Settings(P.PlanetType, true).Collections;
        const auto AuthoredSignature = UAPSPlanetSurfaceProfileResolver::BuildProfileSignature(P);
        const auto AuthoredBefore = Policy::BuildRuntimeActivationPlan(P, 1, false, false, false, false);
        const auto AuthoredDuring = Policy::BuildRuntimeActivationPlan(P, 1, false, false, false, true);
        TestTrue(TEXT("Explicit authored palette stays admitted during scatter trial"), AuthoredBefore.bEnabled && AuthoredDuring.bEnabled);
        TestTrue(TEXT("Authored collections retained"), AuthoredBefore.Collections == AuthoredDuring.Collections);
        TestEqual(TEXT("Authored density retained"), AuthoredBefore.HabitatDensityScale, AuthoredDuring.HabitatDensityScale);
        TestFalse(TEXT("Authored palette is not tagged as automatic scatter"), AuthoredDuring.bSurfaceScatterPalette);
        TestFalse(TEXT("Authored palette is not auto-promoted in mode 2"), Policy::BuildRuntimeActivationPlan(P, 2, false, false, false, true).bEnabled);
        TestFalse(TEXT("Authored palette respects preview gate"), Policy::BuildRuntimeActivationPlan(P, 1, true, false, false, true).bEnabled);
        TestFalse(TEXT("Authored palette respects master-off"), Policy::BuildRuntimeActivationPlan(P, 0, false, false, false, true).bEnabled);
        TestEqual(TEXT("Authored save signature untouched"), UAPSPlanetSurfaceProfileResolver::BuildProfileSignature(P), AuthoredSignature);
        P.Foliage.bEnabled = false; P.Foliage.Collections = Settings(P.PlanetType, true).Collections;
        TestFalse(TEXT("Disabled authored collection stays disabled in mode 1"), Policy::BuildRuntimeActivationPlan(P, 1, false, false, false, true).bEnabled);
        TestFalse(TEXT("Disabled authored collection untouched"), Policy::BuildRuntimeActivationPlan(P, 2, false, false, false, true).bEnabled);
        P.Foliage.Collections.Reset(); P.Biomass = P.Biodiversity = 0;
        const auto Barren = Policy::BuildRuntimeActivationPlan(P, 2, false, false, false, true);
        TestTrue(TEXT("Barren worlds retain mineral detail without invented life"), Barren.bEnabled);
        if (Barren.Collections.Num() == 1) TestTrue(TEXT("Barren selects mineral-only asset"), Barren.Collections[0].ToString().Contains(TEXT("_Mineral")));
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAPSSurfaceScatterAssetCoverageTest,
    "APS.Gameplay.World.PlanetSurface.Foliage.ScatterAssetCoverage",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FAPSSurfaceScatterAssetCoverageTest::RunTest(const FString& Parameters)
{
    using namespace APSPlanetSurfaceScatter;
    int32 CheckedPalettes = 0;
    for (uint8 V = 0; V <= APSPlanetTypes::LastValue; ++V)
    {
        const auto Type = EPlanetType(V);
        if (!UAPSPlanetSurfaceProfileResolver::SupportsWorldScape(Type)) continue;
        for (bool Habitat : {false, true})
        {
            const FString Path = CollectionPath(Type, Habitat);
            auto* Palette = LoadObject<UWorldScapeFoliagesCollection>(nullptr, *Path);
            if (!TestNotNull(*Path, Palette)) continue;
            ++CheckedPalettes;
            TestEqual(*FString::Printf(TEXT("%s has all five baked species"), *Path), Palette->FoliageList.Num(), 5);
            TestTrue(TEXT("All palettes stay on exposed ground"), Palette->SpawnInWater == EFoliageWaterSpawn::OutsideWater);
            TSet<const UStaticMesh*> Meshes;
            int32 Attempts = 0;
            int32 BiologicalEntries = 0;
            for (auto* Item : Palette->FoliageList)
            {
                auto* Entry = Cast<UWorldScapeFoliagesAsset>(Item);
                if (!TestNotNull(TEXT("Bounded HISM asset, not actor foliage"), Entry)
                    || !TestNotNull(TEXT("Baked species mesh"), Entry->StaticMesh)) continue;
                Meshes.Add(Entry->StaticMesh);
                TestTrue(TEXT("Replaceable owned geometry only"), Entry->StaticMesh->GetPathName().StartsWith(FString(Root) + TEXT("/")));
                const bool FiniteCount = FMath::IsFinite(Entry->FoliagesCount)
                    && Entry->FoliagesCount >= 1.f && Entry->FoliagesCount <= 64.f;
                TestTrue(TEXT("Finite per-species attempts"), FiniteCount);
                if (FiniteCount) Attempts += FMath::CeilToInt(Entry->FoliagesCount);
                TestTrue(TEXT("Physical size range is finite and nondegenerate"),
                    FMath::IsFinite(Entry->MinScale) && FMath::IsFinite(Entry->MaxScale)
                    && Entry->MinScale > 0 && Entry->MaxScale >= Entry->MinScale);
                TestTrue(TEXT("Bounded local sectors"), FMath::IsFinite(Entry->FoliageSectorSize)
                    && Entry->FoliageSectorSize >= 8000. && Entry->FoliageSectorSize <= 18000.);
                TestFalse(TEXT("No whole-palette physics or expensive shadow/actor path"),
                    Entry->bCollision || Entry->bCastShadows || Entry->bGenerateOnServer || Entry->bSpawnActorInstead);
                TestTrue(TEXT("Species cannot spawn underwater"), Entry->SpawnInWater == EFoliageWaterSpawn::OutsideWater);
                TestEqual(TEXT("Every mesh material slot has a palette binding"), Entry->OverrideMaterial.Num(),
                    Entry->StaticMesh->GetStaticMaterials().Num());
                for (const auto& Material : Entry->OverrideMaterial)
                    TestTrue(TEXT("No null material binding"), IsValid(Material));
                BiologicalEntries += Entry->bUseFoliageNoiseMask ? 1 : 0;
                if (!Habitat) TestFalse(TEXT("Mineral fallback never invents biological coverage"), Entry->bUseFoliageNoiseMask);
            }
            TestEqual(TEXT("Five distinct geometries, not repeated scales"), Meshes.Num(), 5);
            TestTrue(TEXT("Baked recipe fits unchanged collection budget"), Attempts <= 64);
            int32 ExpectedBiologicalEntries = 0;
            if (Habitat)
                for (const auto& Species : Recipe(Type).Species) ExpectedBiologicalEntries += Species.Biological ? 1 : 0;
            TestEqual(TEXT("Biological roles agree with planet recipe"), BiologicalEntries, ExpectedBiologicalEntries);
        }
    }
    TestTrue(TEXT("Solid-family asset matrix was actually inspected"), CheckedPalettes > 0);
    AddInfo(FString::Printf(TEXT("Checked %d pre-existing palettes; no publication, bake or rendered acceptance."), CheckedPalettes));
    return true;
}
#endif
