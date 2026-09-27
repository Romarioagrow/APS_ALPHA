#pragma once

#if WITH_EDITOR

#include "APSSharedAmmoniaMaterialBuilder.h"
#include "Factories/MaterialInstanceConstantFactoryNew.h"

// Candidate authoring only. Reuse the already audited physical-liquid graph;
// despite its asset name, M_APS_SharedAmmonia has no baked-in ammonia chemistry.
// Never duplicate that family's MIC, mutate the master, or rewrite a catalog.
namespace APSSharedWaterMaterialBuilder
{
    inline constexpr const TCHAR* Destination = TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/SharedLiquid");
    inline constexpr const TCHAR* OutputName = TEXT("MI_APS_SharedWater");
    inline constexpr const TCHAR* ParentPath = TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/SharedLiquid/M_APS_SharedAmmonia.M_APS_SharedAmmonia");
    inline constexpr const TCHAR* MarinePath = TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/Preview/MI_APS_OrbitalLiquid_Water.MI_APS_OrbitalLiquid_Water");

    inline bool Build(IAssetTools& AssetTools)
    {
        const auto Fail = [](const TCHAR* Why)
        {
            UE_LOG(LogTemp, Error, TEXT("[APS.SharedWater] Refused before binding: %s"), Why);
            return false;
        };
        const FString PackagePath = FString(Destination) / OutputName;
        const FString ObjectPath = PackagePath + TEXT(".") + OutputName;
        if (FPackageName::DoesPackageExist(PackagePath)
            || FindObject<UObject>(nullptr, *ObjectPath) || FindPackage(nullptr, *PackagePath))
            return Fail(TEXT("New MIC only; refusing an existing output package/object"));

        UMaterial* Physical = LoadObject<UMaterial>(nullptr, APSSharedAmmoniaMaterialBuilder::SourcePath);
        UMaterial* Parent = LoadObject<UMaterial>(nullptr, ParentPath);
        auto* Marine = LoadObject<UMaterialInstanceConstant>(nullptr, MarinePath);
        using namespace APSSharedAmmoniaMaterialBuilder;
        if (!SavedSourceMatches(Physical, TEXT("6F941E5BEB7193391E8487989F636343CADC2904"))
            || !AuditSource(Physical)
            || !SavedSourceMatches(Parent, TEXT("91EA9BB6BAD5DE82BD29594B259DE4D94D52CCFC"))
            || !SavedSourceMatches(Marine, TEXT("4BBB79CC65ED5AAC0711C9219553E62DEC999ECD"))
            || !Marine->Parent || Marine->Parent->GetPathName() !=
                TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/Preview/M_APS_OrbitalWater.M_APS_OrbitalWater")
            || Parent->GetBlendMode() != BLEND_Masked || Parent->bTangentSpaceNormal
            || !Parent->TwoSided || !Parent->GetShadingModels().HasShadingModel(MSM_DefaultLit)
            || Parent->OpacityMaskClipValue != 0.3333f)
            return Fail(TEXT("Saved physical graph/shared parent/marine MIC changed; refresh the dependency audit"));

        // Read the actual saved physical defaults, not MI_APS_SharedAmmonia's
        // green overrides. The source hash plus parameter inventory fail closed.
        TMap<FName, float> Scalars;
        TMap<FName, FLinearColor> Vectors;
        TArray<FMaterialParameterInfo> Infos;
        TArray<FGuid> Ids;
        Physical->GetAllScalarParameterInfo(Infos, Ids);
        if (Infos.Num() != 11) return Fail(TEXT("Physical scalar inventory changed"));
        for (const FMaterialParameterInfo& Info : Infos)
        {
            float Value = 0.0f, ParentValue = 0.0f;
            if (Info.Association != EMaterialParameterAssociation::GlobalParameter
                || !Physical->GetScalarParameterValue(Info, Value) || !FMath::IsFinite(Value)
                || !Parent->GetScalarParameterValue(Info, ParentValue) || ParentValue != Value)
                return Fail(TEXT("Physical scalar default/shared-parent mismatch"));
            Scalars.Add(Info.Name, Value);
        }
        // Rendered same-camera Water optical A/B: opacity .78 lifted the saved
        // marine palette to cyan; zero restored accepted deep blue. This is the
        // opaque material's optical mix, not geometry/shoreline opacity. Keep
        // verifying the physical source .78 above, then author only this MIC.
        const float* PhysicalOpticalMix = Scalars.Find(TEXT("WaterSurfaceOpacity"));
        if (!PhysicalOpticalMix || *PhysicalOpticalMix != 0.78f)
            return Fail(TEXT("Physical Water optical default changed; repeat source/render audit"));
        Scalars[TEXT("WaterSurfaceOpacity")] = 0.0f;
        Infos.Reset(); Ids.Reset();
        Physical->GetAllVectorParameterInfo(Infos, Ids);
        if (Infos.Num() != 7) return Fail(TEXT("Physical vector inventory changed"));
        for (const FMaterialParameterInfo& Info : Infos)
        {
            FLinearColor Value, ParentValue;
            if (Info.Association != EMaterialParameterAssociation::GlobalParameter
                || !Physical->GetVectorParameterValue(Info, Value)
                || !FMath::IsFinite(Value.R) || !FMath::IsFinite(Value.G)
                || !FMath::IsFinite(Value.B) || !FMath::IsFinite(Value.A)
                || !Parent->GetVectorParameterValue(Info, ParentValue) || ParentValue != Value)
                return Fail(TEXT("Physical vector default/shared-parent mismatch"));
            Vectors.Add(Info.Name, Value);
        }

        // Only these three values come from PLANET Water. The source's blue
        // optical tint/absorption/scattering, PBR and both wave scales survive.
        const TPair<FName, FName> PaletteMap[] = {
            {TEXT("WaterDeepColor"), TEXT("LiquidDeepColor")},
            {TEXT("WaterShallowColor"), TEXT("LiquidShallowColor")},
            {TEXT("WaterAmbientRadiance"), TEXT("LiquidEmissiveColor")}
        };
        for (const auto& Pair : PaletteMap)
        {
            FLinearColor Value;
            if (!Vectors.Contains(Pair.Value)
                || !Marine->GetVectorParameterValue(FMaterialParameterInfo(Pair.Key), Value)
                || !FMath::IsFinite(Value.R) || !FMath::IsFinite(Value.G)
                || !FMath::IsFinite(Value.B) || !FMath::IsFinite(Value.A)
                || Value != Value.GetClamped(0.0f, 1.0f)
                || (Pair.Value == TEXT("LiquidEmissiveColor")
                    && FMath::Max3(Value.R, Value.G, Value.B) > 0.04f))
                return Fail(TEXT("Marine palette missing, nonfinite or exceeds physical emissive cap"));
            Vectors[Pair.Value] = Value;
        }
        float DefaultContext = -1.0f;
        if (!Parent->GetScalarParameterValue(FMaterialParameterInfo(TEXT("APS_UsePresentationWaterMask")), DefaultContext)
            || DefaultContext != 0.0f)
            return Fail(TEXT("Shared graph does not default to physical Hole-independent coverage"));

        auto* Factory = NewObject<UMaterialInstanceConstantFactoryNew>();
        Factory->InitialParent = Parent;
        auto* Candidate = Cast<UMaterialInstanceConstant>(AssetTools.CreateAsset(
            OutputName, Destination, UMaterialInstanceConstant::StaticClass(), Factory));
        if (!Candidate || Candidate->Parent.Get() != Parent)
            return Fail(TEXT("Could not create the one new MIC with the exact direct parent"));
        for (const auto& Pair : Scalars)
            Candidate->SetScalarParameterValueEditorOnly(FMaterialParameterInfo(Pair.Key), Pair.Value);
        for (const auto& Pair : Vectors)
            Candidate->SetVectorParameterValueEditorOnly(FMaterialParameterInfo(Pair.Key), Pair.Value);
        Candidate->SetScalarParameterValueEditorOnly(
            FMaterialParameterInfo(TEXT("APS_UsePresentationWaterMask")), 0.0f);
        Candidate->PostEditChange();

        for (const auto& Pair : Scalars)
        {
            float Value = 0.0f;
            if (!Candidate->GetScalarParameterValue(FMaterialParameterInfo(Pair.Key), Value) || Value != Pair.Value)
                return Fail(TEXT("Candidate failed physical scalar round-trip"));
            UE_LOG(LogTemp, Display, TEXT("[APS.SharedWater] Scalar %s=%.9g source=%s"),
                *Pair.Key.ToString(), Value, Pair.Key == TEXT("WaterSurfaceOpacity")
                    ? TEXT("rendered-marine-optical-refinement") : TEXT("physical-default"));
        }
        for (const auto& Pair : Vectors)
        {
            FLinearColor Value;
            if (!Candidate->GetVectorParameterValue(FMaterialParameterInfo(Pair.Key), Value) || Value != Pair.Value)
                return Fail(TEXT("Candidate failed physical/marine vector round-trip"));
            UE_LOG(LogTemp, Display, TEXT("[APS.SharedWater] Vector %s=%s"), *Pair.Key.ToString(), *Value.ToString());
        }
        if (Candidate->GetMaterial() != Parent || Candidate->GetBlendMode() != BLEND_Masked
            || !Candidate->GetStaticParameters().Equivalent(FStaticParameterSet()))
            return Fail(TEXT("New MIC changed the master/permutation instead of parameters"));

        Candidate->ForceRecompileForRendering(EMaterialShaderPrecompileMode::Default);
        FAssetCompilingManager::Get().FinishAllCompilation();
        if (GShaderCompilingManager) GShaderCompilingManager->FinishAllCompilation();
        FMaterialResource* Resource = Candidate->GetMaterialResource(GMaxRHIFeatureLevel);
        FMaterialShaderMap* Map = Resource ? Resource->GetGameThreadShaderMap() : nullptr;
        const bool Ready = Resource && Resource->IsGameThreadShaderMapComplete()
            && Resource->GetCompileErrors().Num() == 0 && Map
            && Map->GetMeshShaderMap(&FLocalVertexFactory::StaticType);
        UE_LOG(LogTemp, Display, TEXT("[APS.SharedWater] Ready=%d candidate=%s parent=%s outputs=1 newMaster=0"),
            Ready, *Candidate->GetPathName(), *Parent->GetPathName());
        if (!Ready)
        {
            if (Resource) for (const FString& Error : Resource->GetCompileErrors())
                UE_LOG(LogTemp, Error, TEXT("[APS.SharedWater] %s"), *Error);
            return Fail(TEXT("Exact candidate shader incomplete, compile errors or missing LocalVF"));
        }
        if (!SavedSourceMatches(Physical, TEXT("6F941E5BEB7193391E8487989F636343CADC2904"))
            || !SavedSourceMatches(Parent, TEXT("91EA9BB6BAD5DE82BD29594B259DE4D94D52CCFC"))
            || !SavedSourceMatches(Marine, TEXT("4BBB79CC65ED5AAC0711C9219553E62DEC999ECD")))
            return Fail(TEXT("Read-only source dependency became dirty/changed during candidate creation"));

        UPackage* Package = Candidate->GetOutermost();
        if (Package->GetName() != PackagePath) return Fail(TEXT("Output escaped exact candidate package"));
        Package->MarkPackageDirty();
        const FString File = FPackageName::LongPackageNameToFilename(
            PackagePath, FPackageName::GetAssetPackageExtension());
        FSavePackageArgs Args;
        Args.TopLevelFlags = RF_Public | RF_Standalone;
        Args.SaveFlags = SAVE_NoError;
        if (!UPackage::SavePackage(Package, Candidate, *File, Args)) return Fail(TEXT("Save new Water MIC"));
        UE_LOG(LogTemp, Display, TEXT("[APS.SharedWater] Candidate saved, NOT selected/bound: marinePalette=PLANET physicalOptics=source-defaults-except-WaterSurfaceOpacity0 ammoniaMIC=unused shoreline=shared-candidate-unverified"));
        return true;
    }
}
#endif
