#pragma once

#if WITH_DEV_AUTOMATION_TESTS
#include "DeviceProfiles/DeviceProfileManager.h"
#include "DeviceProfiles/DeviceProfile.h"
#include "Engine/TextureLODSettings.h"
#include "Engine/Texture2D.h"
#include "Engine/VolumeTexture.h"
#include "Materials/MaterialInterface.h"
#include "Misc/AutomationTest.h"
#include "RenderingThread.h"
#include "TextureResource.h"

namespace APSLavaTextureDiagnostics
{
    inline void LogWAT2D(FAutomationTestBase* Test, UMaterialInterface* Material,
        const UTextureLODSettings* LOD)
    {
        const TCHAR* ExpectedPath = TEXT("/Engine/EngineMaterials/T_Default_MacroVariation.T_Default_MacroVariation");
        UTexture2D* Expected = LoadObject<UTexture2D>(nullptr, ExpectedPath);
        TArray<UTexture*> Used;
        Material->GetUsedTextures(Used, EMaterialQualityLevel::High, true,
            GMaxRHIFeatureLevel, true);
        const bool bActuallyReferenced = Expected && Used.Contains(Expected);
        Test->AddInfo(FString::Printf(
            TEXT("PLANET_LAVA_WAT_REFERENCE material=%s expected=%s loaded=%d usedByMaterial=%d usedTextureCount=%d"),
            *Material->GetPathName(), ExpectedPath, Expected ? 1 : 0,
            bActuallyReferenced ? 1 : 0, Used.Num()));

        // The WAT TextureObjects are not named parameters. Query the material's
        // real texture set, and report the expected asset even if it was absent.
        if (Expected) Used.AddUnique(Expected);
        for (UTexture* UsedTexture : Used)
        {
            UTexture2D* Texture = Cast<UTexture2D>(UsedTexture);
            if (!Texture) continue;
            const bool bReady = !Texture->IsCompiling();
            const FTextureResource* Resource = bReady ? Texture->GetResource() : nullptr;
            struct FSnapshot
            {
                int32 Width = -1, Height = -1, Mips = -1, Format = -1;
                bool bHasRHI = false, bProxy = false, bPartiallyResident = false;
            } Snapshot;
            if (Resource)
            {
                // RHI state belongs to the render thread. Read only; do not
                // request mips/rebuild resources or change filter/source data.
                ENQUEUE_RENDER_COMMAND(APSReadLavaWATTexture)(
                    [Resource, &Snapshot](FRHICommandListImmediate&)
                    {
                        Snapshot.bProxy = Resource->IsProxy();
                        Snapshot.bPartiallyResident = Resource->IsTextureRHIPartiallyResident();
                        if (const FRHITexture* RHI = Resource->GetTexture2DRHI())
                        {
                            const FRHITextureDesc& Desc = RHI->GetDesc();
                            Snapshot.bHasRHI = true;
                            Snapshot.Width = Desc.Extent.X;
                            Snapshot.Height = Desc.Extent.Y;
                            Snapshot.Mips = Desc.NumMips;
                            Snapshot.Format = static_cast<int32>(Desc.Format);
                        }
                    });
                FlushRenderingCommands();
            }
            FString PlatformMips;
            if (bReady)
            {
                for (const FTexture2DMipMap& Mip : Texture->GetPlatformMips())
                {
                    if (!PlatformMips.IsEmpty()) PlatformMips += TEXT(",");
                    PlatformMips += FString::Printf(TEXT("%ux%u"),
                        static_cast<uint32>(Mip.SizeX), static_cast<uint32>(Mip.SizeY));
                }
            }
            Test->AddInfo(FString::Printf(
                TEXT("PLANET_LAVA_WAT_2D material=%s texture=%s expected=%d compiling=%d size=%dx%d mips=%d residentMips=%d cachedBias=%d group=%d configuredFilter=%d activeFilter=%d address=%d/%d srgb=%d neverStream=%d resource=%d proxy=%d rhi=%d rhiSize=%dx%d rhiMips=%d rhiFormat=%d partialResident=%d platformMips=[%s]"),
                *Material->GetPathName(), *Texture->GetPathName(), Texture == Expected ? 1 : 0,
                bReady ? 0 : 1, bReady ? Texture->GetSizeX() : -1,
                bReady ? Texture->GetSizeY() : -1, bReady ? Texture->GetNumMips() : -1,
                bReady ? Texture->GetNumResidentMips() : -1, Texture->GetCachedLODBias(),
                static_cast<int32>(Texture->LODGroup.GetValue()), static_cast<int32>(Texture->Filter.GetValue()),
                LOD ? static_cast<int32>(LOD->GetSamplerFilter(Texture)) : -1,
                static_cast<int32>(Texture->AddressX.GetValue()), static_cast<int32>(Texture->AddressY.GetValue()),
                Texture->SRGB ? 1 : 0, Texture->NeverStream ? 1 : 0, Resource ? 1 : 0,
                Snapshot.bProxy ? 1 : 0, Snapshot.bHasRHI ? 1 : 0,
                Snapshot.Width, Snapshot.Height, Snapshot.Mips, Snapshot.Format,
                Snapshot.bPartiallyResident ? 1 : 0, *PlatformMips));
        }
    }

    // Read actual inherited runtime objects. Do not PostEditChange/UpdateResource:
    // LUT validation itself would change sRGB and mip policy on the source asset.
    inline void Log(FAutomationTestBase* Test, UMaterialInterface* Material)
    {
        if (!Test || !Material) return;
        const UDeviceProfile* ActiveProfile = UDeviceProfileManager::Get().GetActiveProfile();
        const UTextureLODSettings* LOD = ActiveProfile ? ActiveProfile->GetTextureLODSettings() : nullptr;
        LogWAT2D(Test, Material, LOD);
        for (const TCHAR* Name : {TEXT("Noise1"), TEXT("Noise2"), TEXT("Noise3"),
            TEXT("Noise4"), TEXT("WapredNoise")})
        {
            UTexture* Texture = nullptr;
            const bool bFound = Material->GetTextureParameterValue(FHashedMaterialParameterInfo(Name), Texture);
            const UVolumeTexture* Volume = Cast<UVolumeTexture>(Texture);
            const bool bReady = Volume && !Volume->IsCompiling();
            Test->AddInfo(FString::Printf(
                TEXT("PLANET_LAVA_TEXTURE material=%s parameter=%s found=%d texture=%s volume=%d compiling=%d size=%dx%dx%d mips=%d group=%d configuredFilter=%d activeFilter=%d address=%d srgb=%d resource=%d"),
                *Material->GetPathName(), Name, bFound ? 1 : 0, *GetPathNameSafe(Texture),
                Volume ? 1 : 0, Volume && Volume->IsCompiling() ? 1 : 0,
                bReady ? Volume->GetSizeX() : -1, bReady ? Volume->GetSizeY() : -1,
                bReady ? Volume->GetSizeZ() : -1, bReady ? Volume->GetNumMips() : -1,
                Texture ? static_cast<int32>(Texture->LODGroup.GetValue()) : -1,
                Texture ? static_cast<int32>(Texture->Filter.GetValue()) : -1,
                Texture && LOD ? static_cast<int32>(LOD->GetSamplerFilter(Texture)) : -1,
                Volume ? static_cast<int32>(Volume->AddressMode.GetValue()) : -1,
                Texture && Texture->SRGB ? 1 : 0, Texture && Texture->GetResource() ? 1 : 0));
        }
    }
}
#endif
