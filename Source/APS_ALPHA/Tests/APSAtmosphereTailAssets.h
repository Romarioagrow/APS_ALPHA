#pragma once

#if WITH_EDITOR || WITH_DEV_AUTOMATION_TESTS
#include "CoreMinimal.h"
#include "Misc/FileHelper.h"
#include "Misc/PackageName.h"
#include "Misc/SecureHash.h"

// Diagnostic provenance only; never participates in production parent selection.
namespace APSAtmosphereTailAssets
{
    inline constexpr const TCHAR* Destination = TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/Diagnostics/AtmosphereTail20261003V1");
    inline constexpr const TCHAR* MasterPath = TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/Diagnostics/AtmosphereTail20261003V1/M_APS_AtmosphereTail.M_APS_AtmosphereTail");
    inline constexpr const TCHAR* SourceMasterPath = TEXT("/AtmoScape/Materials/Master/MM_PlanetaryAtmo.MM_PlanetaryAtmo");
    inline constexpr const TCHAR* TailParameter = TEXT("APS_DiagAtmosphereTail");
    inline constexpr float ControlValue = 0.0f;
    inline constexpr float CandidateValue = 1.0f;
    inline constexpr float TailStartFraction = 0.8f;

    struct FSource { const TCHAR* Package; const TCHAR* SHA1; };
    // SHA1 of the actual .uasset files, not the earlier text exports.
    inline constexpr FSource Sources[] = {
        {TEXT("/AtmoScape/Materials/Master/MM_PlanetaryAtmo"), TEXT("2921DDB932E1B5500390E9F1D8A3FA10C55DF6D9")},
        {TEXT("/AtmoScape/Materials/Mat_Function/MF_IntersectionAtmo"), TEXT("DA32CE312F0F1BF888706FE6B1993894EFD69187")},
        {TEXT("/AtmoScape/Materials/Mat_Function/MF_CustomLightDirection"), TEXT("1B5CFAE346EF01FAC0FB3396E62DCC46CA21637F")},
        {TEXT("/AtmoScape/Materials/Mat_Function/MF_Phase"), TEXT("1EA42B967AEAEF8D457F7CFC533BCB6372E87207")},
        {TEXT("/Engine/Functions/Engine_MaterialFunctions02/Utility/BreakOutFloat3Components"), TEXT("38EE0832E659EBF75B0839D18F3AF7DC9F942C5A")},
        {TEXT("/Engine/Functions/Engine_MaterialFunctions02/Utility/MakeFloat3"), TEXT("D12657ED511785EAA568D94C550605C460071FB9")},
        {TEXT("/Engine/Functions/Engine_MaterialFunctions02/SampleSceneDepth"), TEXT("AD824B12374B47A49263F1A88BFC34BECA4A3120")}
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
                Error += FString::Printf(TEXT("%s SHA1=%s expected=%s\n"), Source.Package, *Actual, Source.SHA1);
        }
        return Error.IsEmpty();
    }
}
#endif
