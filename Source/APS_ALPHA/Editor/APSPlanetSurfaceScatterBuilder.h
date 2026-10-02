#pragma once
#if WITH_EDITOR
#include "APSPlanetFoliagePrototypeBuilder.h"
#include "APS_ALPHA/Core/Planetary/APSPlanetSurfaceScatter.h"
#include "APS_ALPHA/Core/Planetary/APSFoliageLeafMaterial.h"

namespace APSPlanetSurfaceScatterBuilder
{
using namespace APSPlanetSurfaceScatter;
inline bool Build()
{
    const UEnum* Types = StaticEnum<EPlanetType>();
    if (!Types) return false;
    const TCHAR* Names[] = {TEXT("Pebble"), TEXT("RockA"), TEXT("RockB"), TEXT("SlabA"), TEXT("SlabB"),
        TEXT("Grass"), TEXT("ColdGrass"), TEXT("DryGrass"), TEXT("TreeA"), TEXT("TreeB")};
    const TCHAR* Paths[] = {
        TEXT("/WorldScape/Ressources/Mesh/Rock/Small_Rock/SM_Small_rock.SM_Small_rock"),
        TEXT("/WorldScape/Ressources/Mesh/Rock/Rock_1/SM_Rock_1.SM_Rock_1"),
        TEXT("/WorldScape/Ressources/Mesh/Rock/Rock_2/SM_Rock_2.SM_Rock_2"),
        TEXT("/WorldScape/Ressources/Mesh/Rock/Cliff_1/SM_Cliff_1.SM_Cliff_1"),
        TEXT("/WorldScape/Ressources/Mesh/Rock/Cliff_2/SM_Cliff_2.SM_Cliff_2"),
        TEXT("/WorldScape/Ressources/Mesh/Grass/SM_Grass_Temp.SM_Grass_Temp"),
        TEXT("/WorldScape/Ressources/Mesh/Grass/SM_Grass_Cold.SM_Grass_Cold"),
        TEXT("/WorldScape/Ressources/Mesh/Grass/SM_Grass_Hot.SM_Grass_Hot"),
        TEXT("/WorldScape/Ressources/Mesh/Tree/SM_Tree_1.SM_Tree_1"),
        TEXT("/WorldScape/Ressources/Mesh/Tree/SM_Tree_2.SM_Tree_2")};
    static_assert(UE_ARRAY_COUNT(Paths) == int32(EMesh::Count));
    static_assert(UE_ARRAY_COUNT(Names) == UE_ARRAY_COUNT(Paths));
    const auto NewPackage = [](const FString& Name)
    { return !FPackageName::DoesPackageExist(Name) && !FindPackage(nullptr, *Name); };
    for (const TCHAR* Name : Names)
        if (!NewPackage(FString(Root) / (TEXT("SM_APS_Scatter_") + FString(Name)))) return false;
    for (uint8 V = 0; V <= APSPlanetTypes::LastValue; ++V)
    {
        const auto Type = EPlanetType(V);
        if (!UAPSPlanetSurfaceProfileResolver::SupportsWorldScape(Type)) continue;
        for (bool Habitat : {false, true})
            if (!NewPackage(FSoftObjectPath(CollectionPath(Type, Habitat)).GetLongPackageName())) return false;
    }

    TArray<TStrongObjectPtr<UStaticMesh>> Sources, Meshes;
    for (const TCHAR* Path : Paths)
    {
        auto* Mesh = LoadObject<UStaticMesh>(nullptr, Path);
        if (!Mesh) { UE_LOG(LogTemp, Error, TEXT("[APS.SurfaceScatter] missing source %s"), Path); return false; }
        Sources.Emplace(Mesh);
    }
    FAssetCompilingManager::Get().FinishAllCompilation();
    UMaterialInterface* MineralMaterial = Sources[0]->GetMaterial(0);
    UMaterialInterface* FixedLeaf = LoadObject<UMaterialInterface>(nullptr, APSFoliageLeafMaterial::TemplatePath);
    if (!MineralMaterial || !FixedLeaf) return false;
    for (int32 I = 0; I < Sources.Num(); ++I)
    {
        const uint32 Cap = I >= int32(EMesh::TreeA) ? 2048 : I >= int32(EMesh::Grass) ? 512 : 1024;
        auto* Mesh = APSPlanetFoliagePrototypeBuilder::MakeBudgetTree(Sources[I].Get(), Root,
            TEXT("SM_APS_Scatter_") + FString(Names[I]), Cap);
        if (!Mesh) return false;
        // Geometry is owned; vendor assets and their materials are never saved.
        Meshes.Emplace(Mesh);
    }
    TArray<TStrongObjectPtr<UWorldScapeFoliagesCollection>> Palettes;
    for (uint8 V = 0; V <= APSPlanetTypes::LastValue; ++V)
    {
        const auto Type = EPlanetType(V);
        if (!UAPSPlanetSurfaceProfileResolver::SupportsWorldScape(Type)) continue;
        for (bool Habitat : {false, true})
        {
            // Mineral-only fallback still has five shapes: no grass on a barren
            // Forest preset, and no invented biomass to make placement pass.
            const FPalette RecipeSet = Recipe(Habitat ? Type :
                (Type == EPlanetType::Forest || Type == EPlanetType::Terrestrial
                 || Type == EPlanetType::Pangea || Type == EPlanetType::SuperEarth
                 || Type == EPlanetType::Tundra || Type == EPlanetType::Nordic
                 || Type == EPlanetType::Oasis || Type == EPlanetType::Savanna ? EPlanetType::Rocky : Type));
            const FSoftObjectPath Path(CollectionPath(Type, Habitat));
            UPackage* Package = CreatePackage(*Path.GetLongPackageName());
            auto* Palette = NewObject<UWorldScapeFoliagesCollection>(Package,
                FName(*Path.GetAssetName()), RF_Public | RF_Standalone);
            Palettes.Emplace(Palette);
            Palette->Elevation = FWorldScapeFoliagesContraint(-1.e9f, -1.e9f, 1.e9f, 1.e9f);
            Palette->Temperature = FWorldScapeFoliagesContraintNormalized(0, 0, 1, 1);
            Palette->Humidity = FWorldScapeFoliagesContraintNormalized(0, 0, 1, 1);
            Palette->Slope = FWorldScapeFoliagesContraintNormalized(0, 0, .30f, .55f);
            Palette->SpawnInWater = EFoliageWaterSpawn::OutsideWater;
            int32 Attempts = 0;
            TSet<UStaticMesh*> Unique;
            for (const auto& S : RecipeSet.Species)
            {
                auto* Entry = NewObject<UWorldScapeFoliagesAsset>(Palette);
                Entry->StaticMesh = Meshes[int32(S.Mesh)].Get();
                Unique.Add(Entry->StaticMesh);
                const auto Bounds = Entry->StaticMesh->GetBounds();
                const float Height = Bounds.BoxExtent.Z * 2;
                if (!FMath::IsFinite(Height) || Height < 1) return false;
                const float Width = 2.f * float(FMath::Max(Bounds.BoxExtent.X, Bounds.BoxExtent.Y));
                const float MaxWidth = S.Mesh >= EMesh::TreeA ? 900.f : S.Biological ? 120.f : 450.f;
                if (!FMath::IsFinite(Width) || Width < .01f) return false;
                // Cliff source aspect ratios vary enormously. Height alone can
                // turn a 1m-high slab into a 50m obstacle: also cap its footprint,
                // including the largest random-scale variant.
                const float Scale = FMath::Min(S.HeightCm / Height, MaxWidth / (Width * 1.2f));
                Entry->MinScale = Scale * .8f; Entry->MaxScale = Scale * 1.2f;
                // Mesh pivots vary. Place the bottom at terrain, with a small
                // embed for rocks, not the centre floating above the surface.
                Entry->Offset = FVector(0, 0, -Bounds.Origin.Z + Bounds.BoxExtent.Z
                    - Height * (S.Biological ? .01f : .08f));
                Entry->FoliagesCount = S.Attempts; Attempts += S.Attempts;
                Entry->FoliageSectorSize = S.SectorCm;
                Entry->FoliageCullDistanceMultiplier = S.Mesh >= EMesh::TreeA ? 10000.f / S.SectorCm : 1.25f;
                Entry->bRandomRotation = Entry->bAlignedToGround = true;
                Entry->GroundRotationInfluenceMin = S.Biological ? 0 : .65f;
                Entry->GroundRotationInfluenceMax = S.Biological ? .15f : 1;
                Entry->bUseFoliageNoiseMask = S.Biological;
                Entry->SpawnInWater = EFoliageWaterSpawn::OutsideWater;
                Entry->bCollision = Entry->bCastShadows = Entry->bCastFarShadow = false;
                Entry->bGenerateOnServer = Entry->Is_NaniteMesh = Entry->bSpawnActorInstead = false;
                Entry->bCastDistanceFieldIndirectShadow = Entry->bAffectDynamicIndirectLighting = false;
                Entry->bAffectDistanceFieldLighting = Entry->bNeverDistanceCull = false;
                Entry->bUsePoissonDisc = false;
                const auto& Slots = Entry->StaticMesh->GetStaticMaterials();
                if (Slots.IsEmpty() || Slots.Num() > 4) return false;
                for (const auto& Slot : Slots)
                {
                    // Rock_1 has a missing vendor experiment material. Override
                    // every mineral slot with the intact textured small-rock
                    // material; no silent default-checker fallback is accepted.
                    UMaterialInterface* Mat = int32(S.Mesh) <= int32(EMesh::SlabB)
                        ? MineralMaterial : Slot.MaterialInterface.Get();
                    if (!Mat) return false;
                    if (Mat->GetPathName() == TEXT("/WorldScape/Ressources/Mesh/Tree/MI_Grass_Leaf.MI_Grass_Leaf")) Mat = FixedLeaf;
                    Entry->OverrideMaterial.Add(Mat);
                }
                Palette->FoliageList.Add(Entry);
            }
            if (Unique.Num() != 5 || Attempts > 64) return false;
            UE_LOG(LogTemp, Display, TEXT("[APS.SurfaceScatter] palette=%s uniqueMeshes=5 attempts=%d habitat=%d; no activation"),
                *Path.ToString(), Attempts, Habitat);
        }
    }
    // Validate everything before the first save. New-only preflight means an
    // interrupted bake cannot overwrite any prior palette, tree or evidence.
    TArray<UObject*> Outputs;
    for (auto& Mesh : Meshes) Outputs.Add(Mesh.Get());
    for (auto& Palette : Palettes) Outputs.Add(Palette.Get());
    for (UObject* Output : Outputs)
    {
        UPackage* Package = Output->GetOutermost();
        FAssetRegistryModule::AssetCreated(Output);
        FSavePackageArgs Args;
        Args.TopLevelFlags = RF_Public | RF_Standalone; Args.SaveFlags = SAVE_NoError;
        if (!UPackage::SavePackage(Package, Output,
            *FPackageName::LongPackageNameToFilename(Package->GetName(), FPackageName::GetAssetPackageExtension()), Args)) return false;
    }
    UE_LOG(LogTemp, Display, TEXT("[APS.SurfaceScatter] saved=%d meshes=%d palettes=%d activation=0; render and flight validation required"),
        Outputs.Num(), Meshes.Num(), Palettes.Num());
    return true;
}
}
#endif
