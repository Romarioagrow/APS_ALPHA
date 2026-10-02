#pragma once

#if WITH_EDITOR
#include "APS_ALPHA/Core/Planetary/APSWorldScapeFoliagePolicy.h"
#include "APS_ALPHA/Core/Planetary/APSPlanetFoliagePrototype.h"
#include "AssetCompilingManager.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Engine/StaticMesh.h"
#include "Misc/PackageName.h"
#include "StaticMeshResources.h"
#include "UObject/Package.h"
#include "UObject/SavePackage.h"
#include "UObject/StrongObjectPtr.h"
#include "WorldScapeFoliages/Public/WorldScapeFoliagesAsset.h"
#include "WorldScapeFoliages/Public/WorldScapeFoliagesCollection.h"

// Offline, replaceable palettes only. No catalog/profile mutation or runtime
// enablement. The separate fresh-root Prototype opt-in selects the shared
// geological/biological recipe without modifying a world's Biomass.
namespace APSPlanetFoliagePrototypeBuilder
{
    using APSPlanetFoliagePrototype::Root;
    using APSPlanetFoliagePrototype::EMesh;
    using APSPlanetFoliagePrototype::FRecipe;
    using APSPlanetFoliagePrototype::Recipe;

    // The supplied tree has 46k triangles at LOD0. Keep the budget, and build a
    // separate replaceable prototype instead of admitting that mesh unchanged.
    inline UStaticMesh* MakeBudgetTree(UStaticMesh* Source,
        const FString& OutputRoot = Root, const FString& OutputName = TEXT("SM_APS_Proto_TreeBudget"),
        uint32 TriangleCap = 2048)
    {
        const FString Name = OutputName;
        const FString PackageName = OutputRoot / Name;
        if (!Source || !Source->GetMeshDescription(0)
            || FPackageName::DoesPackageExist(PackageName) || FindPackage(nullptr, *PackageName))
        { UE_LOG(LogTemp, Error, TEXT("[APS.FoliagePrototype] Tree source missing or owned output already exists")); return nullptr; }
        UPackage* Package = CreatePackage(*PackageName);
        auto* Copy = Cast<UStaticMesh>(StaticDuplicateObject(Source, Package, FName(*Name), RF_AllFlags & ~RF_Transient));
        if (!Copy) return nullptr;
        TStrongObjectPtr<UStaticMesh> KeepAlive(Copy);
        Copy->SetFlags(RF_Public | RF_Standalone);
        Copy->SetLODGroup(NAME_None, false, false);
        Copy->NaniteSettings.bEnabled = false;
        Copy->bAutoComputeLODScreenSize = true;
        Copy->bGenerateMeshDistanceField = false;
        // Discard only this new copy's authored coarse meshes. All three LODs
        // reduce from the same original source, with explicit triangle ceilings.
        Copy->SetNumSourceModels(1);
        Copy->SetNumSourceModels(3);
        const uint32 Targets[] = {TriangleCap, FMath::Max(64u, TriangleCap / 2), FMath::Max(32u, TriangleCap / 8)};
        for (int32 LOD = 0; LOD < UE_ARRAY_COUNT(Targets); ++LOD)
        {
            auto& Model = Copy->GetSourceModel(LOD);
            Model.ReductionSettings = FMeshReductionSettings();
            Model.ReductionSettings.BaseLODModel = 0;
            // Also activate reduction for callers that do not pass source counts
            // to IsReductionActive; the absolute cap still controls the result.
            Model.ReductionSettings.PercentTriangles = 0.5f;
            Model.ReductionSettings.MaxNumOfTriangles = Targets[LOD];
            Model.ReductionSettings.TerminationCriterion = EStaticMeshReductionTerimationCriterion::Triangles;
            Model.BuildSettings.DistanceFieldResolutionScale = 0;
        }
        TArray<FText> Errors;
        Copy->Build(true, &Errors); // OutErrors also makes this build synchronous.
        FAssetCompilingManager::Get().FinishAllCompilation();
        const auto* Data = Copy->GetRenderData();
        if (!Errors.IsEmpty() || !Data || Data->LODResources.Num() != UE_ARRAY_COUNT(Targets))
        {
            for (const FText& Error : Errors) UE_LOG(LogTemp, Error, TEXT("[APS.FoliagePrototype] Tree build: %s"), *Error.ToString());
            UE_LOG(LogTemp, Error, TEXT("[APS.FoliagePrototype] Budget tree build failed; nothing saved"));
            return nullptr;
        }
        for (int32 LOD = 0; LOD < UE_ARRAY_COUNT(Targets); ++LOD)
        {
            const uint32 Triangles = Data->LODResources[LOD].GetNumTriangles();
            UE_LOG(LogTemp, Display, TEXT("[APS.FoliagePrototype] ownedTreeLOD=%d triangles=%u ceiling=%u source=%s"),
                LOD, Triangles, Targets[LOD], *Source->GetPathName());
            if (Triangles == 0 || Triangles > Targets[LOD]) return nullptr;
        }
        return Copy;
    }

    inline bool Build()
    {
        const UEnum* Types = StaticEnum<EPlanetType>();
        if (!Types) return false;
        TArray<EPlanetType> Supported;
        // Use the resolver's current surface support, including legacy types.
        // Unsupported giants have no invented terrain/foliage prescription.
        for (uint8 Value = 0; Value <= APSPlanetTypes::LastValue; ++Value)
        {
            const auto Type = static_cast<EPlanetType>(Value);
            if (!UAPSPlanetSurfaceProfileResolver::SupportsWorldScape(Type)) continue;
            const FString Name = TEXT("FC_APS_Proto_") + Types->GetNameStringByValue(Value);
            const FString Package = FString(Root) / Name;
            if (FPackageName::DoesPackageExist(Package) || FindPackage(nullptr, *Package))
            {
                UE_LOG(LogTemp, Error, TEXT("[APS.FoliagePrototype] Refusing existing output: %s"), *Package);
                return false;
            }
            Supported.Add(Type);
        }
        if (Supported.IsEmpty()) return false;

        const TCHAR* Paths[] = {
            TEXT("/WorldScape/Ressources/Mesh/Rock/Small_Rock/SM_Small_rock.SM_Small_rock"),
            // Rock_1 imports a missing experimental /Game material. Use the
            // intact small-rock placeholder at the recipe's outcrop scale.
            TEXT("/WorldScape/Ressources/Mesh/Rock/Small_Rock/SM_Small_rock.SM_Small_rock"),
            TEXT("/WorldScape/Ressources/Mesh/Grass/SM_Grass_Temp.SM_Grass_Temp"),
            TEXT("/WorldScape/Ressources/Mesh/Grass/SM_Grass_Cold.SM_Grass_Cold"),
            TEXT("/WorldScape/Ressources/Mesh/Grass/SM_Grass_Hot.SM_Grass_Hot"),
            TEXT("/WorldScape/Ressources/Mesh/Tree/SM_Tree_1.SM_Tree_1")
        };
        static_assert(UE_ARRAY_COUNT(Paths) == int32(EMesh::Count));
        TArray<TStrongObjectPtr<UStaticMesh>> Meshes;
        for (const TCHAR* Path : Paths)
        {
            UStaticMesh* Mesh = LoadObject<UStaticMesh>(nullptr, Path);
            if (!Mesh)
            { UE_LOG(LogTemp, Error, TEXT("[APS.FoliagePrototype] Mesh unavailable: %s"), Path); return false; }
            Meshes.Emplace(Mesh);
        }
        FAssetCompilingManager::Get().FinishAllCompilation();
        auto* Tree = MakeBudgetTree(Meshes[int32(EMesh::Tree)].Get());
        if (!Tree) return false;
        Meshes[int32(EMesh::Tree)].Reset(Tree);
        for (const auto& Mesh : Meshes)
        {
            const FStaticMeshRenderData* Data = Mesh->GetRenderData();
            const float Height = Mesh->GetBounds().BoxExtent.Z * 2;
            if (!Data || Data->LODResources.IsEmpty() || !FMath::IsFinite(Height) || Height < 1
                || Data->LODResources[0].GetNumTriangles() > 8192 || Mesh->GetStaticMaterials().Num() > 4)
            {
                UE_LOG(LogTemp, Error, TEXT("[APS.FoliagePrototype] Reject source=%s lods=%d lod0Triangles=%d materials=%d heightCm=%.2f"),
                    *Mesh->GetPathName(), Data ? Data->LODResources.Num() : 0,
                    Data && !Data->LODResources.IsEmpty() ? Data->LODResources[0].GetNumTriangles() : -1,
                    Mesh->GetStaticMaterials().Num(), Height);
                return false;
            }
            for (const auto& Slot : Mesh->GetStaticMaterials())
                if (!Slot.MaterialInterface)
                { UE_LOG(LogTemp, Error, TEXT("[APS.FoliagePrototype] Missing material on %s"), *Mesh->GetPathName()); return false; }
            UE_LOG(LogTemp, Display, TEXT("[APS.FoliagePrototype] mesh=%s lods=%d lod0Triangles=%d sourceHeightCm=%.2f"),
                *Mesh->GetPathName(), Data->LODResources.Num(), Data->LODResources[0].GetNumTriangles(), Height);
        }

        TArray<TStrongObjectPtr<UWorldScapeFoliagesCollection>> Outputs;
        for (EPlanetType Type : Supported)
        {
            const FRecipe Choice = Recipe(Type);
            const FString TypeName = Types->GetNameStringByValue(uint8(Type));
            const FString Name = TEXT("FC_APS_Proto_") + TypeName;
            auto* Source = NewObject<UWorldScapeFoliagesCollection>(GetTransientPackage(), NAME_None, RF_Transient);
            Source->Elevation = FWorldScapeFoliagesContraint(-1.e9f, -1.e9f, 1.e9f, 1.e9f);
            Source->Temperature = FWorldScapeFoliagesContraintNormalized(0, 0, 1, 1);
            Source->Humidity = FWorldScapeFoliagesContraintNormalized(0, 0, 1, 1);
            Source->Slope = FWorldScapeFoliagesContraintNormalized(0, 0, 0.30f, 0.55f);
            Source->SpawnInWater = EFoliageWaterSpawn::OutsideWater;
            auto* Entry = NewObject<UWorldScapeFoliagesAsset>(Source, NAME_None, RF_Transient);
            Entry->StaticMesh = Meshes[int32(Choice.Mesh)].Get();
            const float Scale = Choice.HeightCm / (Entry->StaticMesh->GetBounds().BoxExtent.Z * 2);
            Entry->MinScale = Scale * 0.75f; Entry->MaxScale = Scale * 1.25f;
            Entry->FoliagesCount = Choice.Attempts;
            Entry->FoliageSectorSize = 20000; // Sparse first trial: 200m sectors.
            Entry->FoliageCullDistanceMultiplier = 0.5f;
            Entry->bRandomRotation = true;
            Entry->GroundRotationInfluenceMin = Choice.bBiological ? 0 : 0.65f;
            Entry->GroundRotationInfluenceMax = Choice.bBiological ? 0.15f : 1.0f;
            Source->FoliageList.Add(Entry);

            FAPSFoliageActivationPlan Plan;
            Plan.bEnabled = true; Plan.HabitatDensityScale = 1;
            Plan.MaxCollections = 1; Plan.MaxTypesPerCollection = 1;
            Plan.MaxInstancesPerSectorPerCollection = 16; Plan.MaxClusterMeshesPerType = 1;
            Plan.MinSectorSizeCm = 20000; Plan.MaxCullDistanceMultiplier = 0.5f;
            Plan.bUseNoiseMask = Choice.bBiological; Plan.bCastShadows = false;
            TArray<UWorldScapeFoliagesCollection*> Safe;
            if (FAPSWorldScapeFoliagePolicy::BuildBudgetedCollections(GetTransientPackage(), {Source}, Plan, Safe) != 1)
                return false;
            UPackage* Package = CreatePackage(*(FString(Root) / Name));
            auto* Output = Cast<UWorldScapeFoliagesCollection>(StaticDuplicateObject(Safe[0], Package,
                FName(*Name), RF_AllFlags & ~RF_Transient));
            if (!Output || Output->FoliageList.Num() != 1) return false;
            Output->SetFlags(RF_Public | RF_Standalone);
            auto* SavedEntry = Cast<UWorldScapeFoliagesAsset>(Output->FoliageList[0]);
            if (!SavedEntry || SavedEntry->GetOutermost() != Package || SavedEntry->HasAnyFlags(RF_Transient)
                || SavedEntry->bCollision || SavedEntry->bCastShadows || SavedEntry->bGenerateOnServer
                || SavedEntry->Is_NaniteMesh || SavedEntry->bSpawnActorInstead || SavedEntry->FoliagesCount > 16)
            { UE_LOG(LogTemp, Error, TEXT("[APS.FoliagePrototype] Unsafe serialized entry: %s"), *Name); return false; }
            FAssetRegistryModule::AssetCreated(Output);
            Outputs.Emplace(Output);
            UE_LOG(LogTemp, Display, TEXT("[APS.FoliagePrototype] type=%s biological=%d mesh=%s attempts=%.0f mask=%d collision=0 shadows=0; not enabled"),
                *TypeName, Choice.bBiological, *Entry->StaticMesh->GetPathName(), SavedEntry->FoliagesCount, SavedEntry->bUseFoliageNoiseMask);
        }
        // Only new packages are saved, after all source/entry checks succeed.
        // Save the owned tree dependency first; the vendor source is read-only.
        {
            UPackage* Package = Tree->GetOutermost();
            FAssetRegistryModule::AssetCreated(Tree);
            FSavePackageArgs Args;
            Args.TopLevelFlags = RF_Public | RF_Standalone; Args.SaveFlags = SAVE_NoError;
            if (!UPackage::SavePackage(Package, Tree,
                *FPackageName::LongPackageNameToFilename(Package->GetName(), FPackageName::GetAssetPackageExtension()), Args)) return false;
        }
        for (const auto& Output : Outputs)
        {
            UPackage* Package = Output->GetOutermost();
            FSavePackageArgs Args;
            Args.TopLevelFlags = RF_Public | RF_Standalone; Args.SaveFlags = SAVE_NoError;
            if (!UPackage::SavePackage(Package, Output.Get(),
                *FPackageName::LongPackageNameToFilename(Package->GetName(), FPackageName::GetAssetPackageExtension()), Args)) return false;
        }
        UE_LOG(LogTemp, Display, TEXT("[APS.FoliagePrototype] saved=%d catalogWrites=0 activation=0; requires render/residency validation"), Outputs.Num());
        return true;
    }
}
#endif
