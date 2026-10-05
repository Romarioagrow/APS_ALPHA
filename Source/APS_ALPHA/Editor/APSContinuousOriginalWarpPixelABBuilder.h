#pragma once

#if WITH_EDITOR
#include "APSContinuousWarpPixelABBuilder.h"
#include "Materials/MaterialExpressionConstant.h"

// Diagnostic copy of the accepted ORIGINAL-COLOUR Continuous master, not the
// older orbital-field V3 experiment. Reuse its exact five-VI-only transform and
// cold-safe private function closure; never patch/save the production source.
namespace APSContinuousOriginalWarpPixelABBuilder
{
    inline constexpr const TCHAR* Destination = TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/Diagnostics/ContinuousOriginalWarpPixel20261003");
    inline constexpr const TCHAR* MasterPath = TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/Diagnostics/ContinuousOriginalWarpPixel20261003/M_APS_ContinuousWarpPixel.M_APS_ContinuousWarpPixel");
    inline constexpr const TCHAR* TemplatePath = TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/Diagnostics/ContinuousOriginalWarpPixel20261003/MI_APS_ContinuousWarpPixel.MI_APS_ContinuousWarpPixel");
    inline constexpr const TCHAR* SourceMasterSHA1 = TEXT("9541C8FB506D3F95E277D97C2FB1E7D626A420B0");

    inline bool VerifySources(FString& Error)
    {
        using namespace APSContinuousWarpPixelAssets;
        Error.Reset();
        int32 MasterCount = 0;
        for (const FSource& Source : Sources)
        {
            const bool IsMaster = FString(Source.Package) == TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/ContinuityV1/M_APS_ContinuousTerrain");
            MasterCount += IsMaster ? 1 : 0;
            const TCHAR* Expected = IsMaster ? SourceMasterSHA1 : Source.SHA1;
            const FString Actual = Hash(Source.Package);
            if (Actual != Expected)
                Error += FString::Printf(TEXT("Original-colour warp source changed: %s SHA1=%s expected=%s\n"), Source.Package, *Actual, Expected);
        }
        if (MasterCount != 1) Error += TEXT("Expected exactly one canonical Continuous master in the source manifest\n");
        return Error.IsEmpty();
    }

    inline bool Build(IAssetTools& Tools)
    {
        if (!IsRunningCommandlet()) return false;
        FString Error;
        if (!VerifySources(Error))
        {
            UE_LOG(LogTemp, Error, TEXT("[APS.ContinuousOriginalWarpPixel.Bake] Refused: %s"), *Error);
            return false;
        }
        auto* Source = LoadObject<UMaterial>(nullptr, APSTerrainContinuityMaterial::MasterPath);
        const auto* Mask = Source ? Source->GetExpressionInputForProperty(MP_OpacityMask) : nullptr;
        const auto* Offset = Source ? Source->GetExpressionInputForProperty(MP_WorldPositionOffset) : nullptr;
        const auto* Coverage = Mask ? Cast<UMaterialExpressionConstant>(Mask->Expression) : nullptr;
        UE_LOG(LogTemp, Display, TEXT("[APS.ContinuousOriginalWarpPixel.Bake] sourceOutputs master=%s activeEOD=%s opacityMask=%s output=%d mask=%d constant=%.9g desc=%s WPO=%s WPOOutput=%d WPOMask=%d"),
            *GetPathNameSafe(Source), *GetPathNameSafe(Source ? Source->GetEditorOnlyData() : nullptr),
            *GetPathNameSafe(Mask ? Mask->Expression : nullptr), Mask ? Mask->OutputIndex : -1, Mask ? Mask->Mask : -1,
            Coverage ? double(Coverage->R) : -1.0, Coverage ? *Coverage->Desc : TEXT("not-constant"),
            *GetPathNameSafe(Offset ? Offset->Expression : nullptr), Offset ? Offset->OutputIndex : -1, Offset ? Offset->Mask : -1);
        if (!Source || !Coverage || !Coverage->IsIn(Source) || Coverage->R != 1.0f || Mask->OutputIndex != 0 || Mask->Mask
            || Coverage->Desc != TEXT("APS generated solid surface: no camera-centred height cutout")
            || (Offset && Offset->Expression))
        {
            UE_LOG(LogTemp, Error, TEXT("[APS.ContinuousOriginalWarpPixel.Bake] Refused before duplication: loaded source lacks exact generated OpacityMask=1 or has WPO; investigate coverage instead of the warp hypothesis"));
            return false;
        }
        UE_LOG(LogTemp, Display, TEXT("[APS.ContinuousOriginalWarpPixel.Bake] sourceMasterSHA1=%s destination=%s change=exact-five-master-warp-VI-bypass no-alternate-colour-fields no-normal-slope-palette-noise-changes production-save=0"),
            SourceMasterSHA1, Destination);
        return APSContinuousWarpPixelABBuilder::Build(Tools, Destination, VerifySources);
    }
}
#endif
