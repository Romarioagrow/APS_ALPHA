#pragma once
#if WITH_EDITOR
#include "MaterialShared.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstance.h"
#include "Engine/Texture.h"
#include "Engine/StaticMesh.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/DateTime.h"

namespace APSPlanetMaterialSourceAudit
{
inline bool Export(bool Scatter = false)
{
    // Translation only. No PostEditChange, dirty packages or asset saves.
    const FString Folder = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("Diagnostics"),
        TEXT("LeafWaterHlsl-") + FDateTime::UtcNow().ToString(TEXT("%Y%m%d-%H%M%S")));
    if (IFileManager::Get().DirectoryExists(*Folder) || !IFileManager::Get().MakeDirectory(*Folder, true)) return false;
    TArray<FString> Assets = {
        TEXT("/WorldScape/Ressources/Mesh/Tree/MI_Grass_Leaf.MI_Grass_Leaf"),
        TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/Diagnostics/WaterAnalytic20260930/MI_APS_WaterAnalytic.MI_APS_WaterAnalytic")
    };
    if (Scatter)
    {
        Assets = {TEXT("/WorldScape/Ressources/Materials/WorldScapeMaterials/Foliage/MI_Cliff_5.MI_Cliff_5")};
        for (const TCHAR* Shape : {TEXT("Grass"), TEXT("ColdGrass"), TEXT("DryGrass"), TEXT("TreeA"), TEXT("TreeB")})
        {
            const FString Name = FString(TEXT("SM_APS_Scatter_")) + Shape;
            const FString Path = TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/Diagnostics/SurfaceScatter20260930V2/") + Name + TEXT(".") + Name;
            auto* Mesh = LoadObject<UStaticMesh>(nullptr, *Path);
            if (!Mesh) return false;
            for (const auto& Slot : Mesh->GetStaticMaterials())
                if (Slot.MaterialInterface) Assets.AddUnique(Slot.MaterialInterface->GetPathName());
        }
    }
    bool Complete = true;
    for (const FString& Path : Assets)
    {
        auto* Material = LoadObject<UMaterialInterface>(nullptr, *Path);
        if (!Material) { Complete=false; continue; }
        TArray<FMaterialParameterInfo> TextureInfos; TArray<FGuid> TextureIds;
        Material->GetAllTextureParameterInfo(TextureInfos,TextureIds);
        if (auto* MI=Cast<UMaterialInstance>(Material))
        {
            UE_LOG(LogTemp,Display,TEXT("[APS.MaterialParameter] material=%s parent=%s"),*Path,*GetPathNameSafe(MI->Parent));
            // Retain authored overrides even when a broken vendor parent cannot translate.
            for (const auto& Value : MI->TextureParameterValues) TextureInfos.AddUnique(Value.ParameterInfo);
        }
        for (const auto& Info : TextureInfos)
        {
            UTexture* Value=nullptr;
            Material->GetTextureParameterValue(Info,Value);
            UE_LOG(LogTemp,Display,TEXT("[APS.MaterialParameter] material=%s texture=%s value=%s"),
                *Path,*Info.Name.ToString(),*GetPathNameSafe(Value));
        }
        auto* Resource = Material ? Material->GetMaterialResource(GMaxRHIFeatureLevel) : nullptr;
        FString Source;
        if (!Resource || !Resource->GetMaterialExpressionSource(Source) || Source.IsEmpty())
        { UE_LOG(LogTemp, Error, TEXT("[APS.MaterialSourceAudit] Translation failed: %s"), *Path); Complete=false; continue; }
        const FString Filename = FPaths::Combine(Folder, Material->GetName() + TEXT(".usf"));
        if (!FFileHelper::SaveStringToFile(Source, *Filename, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM)) return false;
        UE_LOG(LogTemp, Display, TEXT("[APS.MaterialSourceAudit] translated=%s chars=%d output=%s; not rendered acceptance"), *Path, Source.Len(), *Filename);
        TArray<FMaterialParameterInfo> Infos; TArray<FGuid> Ids;
        Material->GetAllScalarParameterInfo(Infos, Ids);
        for (const auto& Info : Infos)
        {
            float Value = 0;
            if (Material->GetScalarParameterValue(Info, Value))
                UE_LOG(LogTemp, Display, TEXT("[APS.MaterialParameter] material=%s scalar=%s value=%.9g"), *Path, *Info.Name.ToString(), Value);
        }
        Infos.Reset(); Ids.Reset(); Material->GetAllVectorParameterInfo(Infos, Ids);
        for (const auto& Info : Infos)
        {
            FLinearColor Value;
            if (Material->GetVectorParameterValue(Info, Value))
                UE_LOG(LogTemp, Display, TEXT("[APS.MaterialParameter] material=%s vector=%s value=%s"), *Path, *Info.Name.ToString(), *Value.ToString());
        }
    }
    return Complete;
}
}
#endif
