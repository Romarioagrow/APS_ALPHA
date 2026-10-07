#pragma once

#if WITH_EDITOR || WITH_DEV_AUTOMATION_TESTS
#include "CoreMinimal.h"
#include "Misc/FileHelper.h"
#include "Misc/PackageName.h"
#include "Misc/SecureHash.h"

// Immutable diagnostic provenance, shared by the offline builder and probe.
// This header does not participate in production material selection.
namespace APSContinuousWarpPixelAssets
{
    inline constexpr const TCHAR* Destination = TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/Diagnostics/ContinuousWarpPixel20261003V3");
    inline constexpr const TCHAR* MasterPath = TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/Diagnostics/ContinuousWarpPixel20261003V3/M_APS_ContinuousWarpPixel.M_APS_ContinuousWarpPixel");
    inline constexpr const TCHAR* TemplatePath = TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/Diagnostics/ContinuousWarpPixel20261003V3/MI_APS_ContinuousWarpPixel.MI_APS_ContinuousWarpPixel");
    struct FSource { const TCHAR* Package; const TCHAR* SHA1; int32 Serialized = -1; int32 Reachable = -1; };
    // Complete live cold-load inventory: bake-continuouswarppixel-v2-1021,
    // 2026-10-03 05:22:36 UTC. Counts are input-linked, not exported subobjects.
    inline constexpr FSource Sources[] = {
        {TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/ContinuityV1/M_APS_ContinuousTerrain"), TEXT("0CDF09601BCCB751D2FE3A1CA3B28F980829F0C3"), 742, 742},
        {TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/ContinuityV1/MI_APS_ContinuousTerra"), TEXT("5C108E7BC7B1681544DF5B9C15334151050EBAFF")},
        {TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/ContinuityV1/MF_APS_ContinuousNormalCoordinates"), TEXT("B659BA102FF75F79F0101079246114FB8C4F0D2E"), 38, 103},
        {TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/Shared/MF_APS_MF_SlopeBlock_f6be5407"), TEXT("3FF327494FFB83C3521A5BA1318FA913C3303AC2"), 47, 143},
        {TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/Shared/MF_APS_WorldAlignedTexture_a83aa78c"), TEXT("2DF40DDCAFA3BEB9C1AB3CE3DFFB489F29A878CF"), 38, 93},
        {TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/Shared/MF_APS_MF_PlanetMap_80d6e0bd"), TEXT("2957DE0FCC842614B088FCBBD30A7F3F887B2B3B"), 21, 50},
        {TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/Shared/MF_APS_OrbitalMacroV2"), TEXT("C4701C8CE10766B17F82A317B76334DB9B0FDC01"), 85, 85},
        {TEXT("/Game/Ressources/Materials/WorldScapeMaterials/Functions/Customizatation/MF_HDRBlock"), TEXT("F64F16ACD608CBD5BDB2E0E1324449C7EE039AC0"), 30, 30},
        {TEXT("/Game/Ressources/Materials/WorldScapeMaterials/Functions/Customizatation/MF_LayerAdjustment"), TEXT("220504C4F7D5FD15E991920C73E4A31A20EA6ADA"), 10, 10},
        {TEXT("/Game/Ressources/Materials/WorldScapeMaterials/Functions/Customizatation/MF_MidLayerBlock"), TEXT("4F50A3484436752FC1362059CCC3238475B877B5"), 14, 14},
        {TEXT("/Game/Ressources/Materials/WorldScapeMaterials/Functions/Customizatation/MF_MidLayerBlockVarient2"), TEXT("60A916847A5FC9BDAC89A4BF814C35DB0211B379"), 23, 23},
        {TEXT("/Game/Ressources/Materials/WorldScapeMaterials/Functions/Customizatation/MF_Sedimentlayer"), TEXT("1F82AC63DC456011A5F29D78E54D39C0E2AFAA29"), 14, 14},
        {TEXT("/Game/Ressources/Materials/WorldScapeMaterials/Functions/Customizatation/MF_CheapContrastNoClamp"), TEXT("E502783517ACAD02FC5F69F39989378216245A9D"), 9, 9},
        {TEXT("/Engine/Functions/Engine_MaterialFunctions01/ImageAdjustment/CheapContrast"), TEXT("AFBCDD07B6C0C91547B9CF8564B4D1FDC3EFB878"), 10, 10},
        {TEXT("/Engine/Functions/Engine_MaterialFunctions01/Texturing/FlattenNormal"), TEXT("E4B5D2CBC1CA3E385D57ACC5E4F37B95F8A8E795"), 5, 5},
        {TEXT("/Engine/Functions/Engine_MaterialFunctions02/Utility/BlendAngleCorrectedNormals"), TEXT("10B85BF025C0830712F0053BB02C7C17D687E4F3"), 15, 15},
        {TEXT("/Engine/Functions/Engine_MaterialFunctions01/Gradient/LinearGradient"), TEXT("AE0F9A7E8282C3976E286CE4AE3E757F551C114E"), 6, 6},
        {TEXT("/Engine/Functions/Engine_MaterialFunctions02/Utility/BreakOutFloat3Components"), TEXT("38EE0832E659EBF75B0839D18F3AF7DC9F942C5A"), 7, 7},
        {TEXT("/Engine/Functions/Engine_MaterialFunctions02/Math/Pi"), TEXT("AAACC0C0BE8615BC724BA19AB2D8167A18A34AA9"), 4, 4},
        {TEXT("/Engine/Functions/Engine_MaterialFunctions02/Utility/MakeFloat2"), TEXT("5128CF1567F93D6A2B2D153465CE1890BA707542"), 4, 4}
    };
    struct FPortableFunction
    {
        const TCHAR* Package;
        const TCHAR* Output;
        int32 MasterCalls, SlopeCalls, OrbitalCalls;
    };
    // Fixed child-first allowlist, not recursive migration. Orbital is complete
    // but must be private because its three WAT references need relocation.
    inline constexpr FPortableFunction PortableFunctions[] = {
        {TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/Shared/MF_APS_WorldAlignedTexture_a83aa78c"), TEXT("MF_APS_ContinuousWarpPixelWAT"), 11, 2, 3},
        {TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/Shared/MF_APS_MF_PlanetMap_80d6e0bd"), TEXT("MF_APS_ContinuousWarpPixelPlanetMap"), 2, 0, 0},
        {TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/Shared/MF_APS_MF_SlopeBlock_f6be5407"), TEXT("MF_APS_ContinuousWarpPixelSlope"), 1, 0, 0},
        {TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/Shared/MF_APS_OrbitalMacroV2"), TEXT("MF_APS_ContinuousWarpPixelOrbital"), 1, 0, 0},
        // Exact five calls recorded by ContinuityPublish; unchanged master SHA1.
        {TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/ContinuityV1/MF_APS_ContinuousNormalCoordinates"), TEXT("MF_APS_ContinuousWarpPixelNormalCoordinates"), 5, 0, 0}
    };
    inline FString Hash(const FString& Package)
    {
        TArray<uint8> Bytes;
        const FString File = FPackageName::LongPackageNameToFilename(Package, FPackageName::GetAssetPackageExtension());
        return FFileHelper::LoadFileToArray(Bytes, *File)
            ? FSHA1::HashBuffer(Bytes.GetData(), Bytes.Num()).ToString().ToUpper() : FString();
    }
    inline bool VerifySources(FString& Error)
    {
        Error.Reset();
        for (const FSource& Source : Sources)
        {
            const FString Actual = Hash(Source.Package);
            if (Actual != Source.SHA1)
            {
                Error += FString::Printf(TEXT("Continuous warp-pixel source changed: %s SHA1=%s expected=%s\n"),
                    Source.Package, *Actual, Source.SHA1);
            }
        }
        return Error.IsEmpty();
    }
}
#endif
