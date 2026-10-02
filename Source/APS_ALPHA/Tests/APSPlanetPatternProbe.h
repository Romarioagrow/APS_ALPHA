#pragma once

#if WITH_DEV_AUTOMATION_TESTS
#include "Engine/Texture2D.h"
#include "Engine/VolumeTexture.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "UObject/StrongObjectPtr.h"

// Causal isolation only. These ugly variants must never become game defaults:
// remove detailed albedo, remove legacy macro, then change volume-noise frequency.
// The retained mesh, masks, palette endpoints and physical frame remain untouched.
namespace APSPlanetPatternProbe
{
    inline bool AllFields() { return FParse::Param(FCommandLine::Get(), TEXT("APSProbeTerrainAllFields")); }
    inline bool VolumesOnly() { return AllFields() || FParse::Param(FCommandLine::Get(), TEXT("APSProbeTerrainVolumes")); }
    inline bool Requested() { return VolumesOnly() || FParse::Param(FCommandLine::Get(), TEXT("APSProbeTerrainPatterns")); }
    class FScope
    {
        TStrongObjectPtr<UMaterialInstanceDynamic> Live{nullptr}, Saved{nullptr};
        TStrongObjectPtr<UTexture2D> FlatTexture{nullptr};
        TStrongObjectPtr<UVolumeTexture> FlatVolume{nullptr};
        TArray<FScalarParameterValue> Scalars;
        TArray<FTextureParameterValue> Textures;
        bool Active = false;
        static constexpr const TCHAR* Albedos[] = {TEXT("Ground1Tex"), TEXT("Ground1MacroTex"),
            TEXT("Ground2Tex"), TEXT("Ground3Tex"), TEXT("SlopeTex")};
        static constexpr const TCHAR* NoiseSizes[] = {TEXT("Scale1"), TEXT("Scale2"), TEXT("Scale3"),
            TEXT("MicroNoise1"), TEXT("MicroNoise2"), TEXT("WarpedScale")};
        static constexpr const TCHAR* Volumes[] = {TEXT("Noise1"), TEXT("Noise2"), TEXT("Noise3"), TEXT("Noise4"), TEXT("WapredNoise")};
    public:
        static int32 Count() { return AllFields() ? 8 : VolumesOnly() ? 7 : 5; }
        static const TCHAR* Label(int32 V)
        {
            static const TCHAR* Labels[] = {TEXT("native"), TEXT("flat-albedo-textures"),
                TEXT("flat-orbital-macro"), TEXT("rescaled-volume-noise"), TEXT("native-restored")};
            static const TCHAR* VolumeLabels[] = {TEXT("native"), TEXT("flat-noise1"), TEXT("flat-noise2"),
                TEXT("flat-noise3"), TEXT("flat-noise4"), TEXT("flat-warp-noise"), TEXT("native-restored")};
            static const TCHAR* FieldLabels[] = {TEXT("native"), TEXT("flat-all-volumes"),
                TEXT("flat-volumes-and-albedo"), TEXT("flat-volumes-and-macro"),
                TEXT("flat-volumes-and-planetary"), TEXT("flat-color-textures-only"),
                TEXT("flat-all-color-fields"), TEXT("native-restored")};
            return V >= 0 && V < Count() ? (AllFields() ? FieldLabels[V] : VolumesOnly() ? VolumeLabels[V] : Labels[V]) : TEXT("invalid");
        }
        ~FScope() { Restore(); }
        bool IsActive() const { return Active; }
        bool Begin(UMaterialInstanceDynamic* Material, FString& Error)
        {
            if (Active || !IsValid(Material) || !Material->Parent
                || !Material->RuntimeVirtualTextureParameterValues.IsEmpty()
                || !Material->SparseVolumeTextureParameterValues.IsEmpty())
            { Error = TEXT("Pattern isolation needs an untouched native MID"); return false; }
            for (const TCHAR* Name : Albedos)
            {
                UTexture* Texture = nullptr;
                if (!Material->GetTextureParameterValue(FHashedMaterialParameterInfo(Name), Texture)
                    || !Cast<UTexture2D>(Texture))
                { Error = FString(TEXT("Missing native albedo texture parameter: ")) + Name; return false; }
                UE_LOG(LogTemp, Display, TEXT("PLANET_PATTERN_SOURCE parameter=%s texture=%s"), Name, *Texture->GetPathName());
            }
            for (const TCHAR* Name : NoiseSizes)
            {
                float Value = 0;
                if (!Material->GetScalarParameterValue(FHashedMaterialParameterInfo(Name), Value)
                    || !FMath::IsFinite(Value) || Value <= 0 || Value > 1.e12f)
                { Error = FString(TEXT("Invalid native volume noise scale: ")) + Name; return false; }
            }
            Live.Reset(Material);
            if (VolumesOnly())
            {
                for (const TCHAR* Name : Volumes)
                {
                    UTexture* Texture = nullptr;
                    Material->GetTextureParameterValue(FHashedMaterialParameterInfo(Name), Texture);
                    auto* Volume = Cast<UVolumeTexture>(Texture);
                    if (!Volume) { Error = FString(TEXT("Missing volume texture: ")) + Name; return false; }
                    UE_LOG(LogTemp, Display, TEXT("PLANET_VOLUME_SOURCE parameter=%s asset=%s size=%dx%dx%d mips=%d format=%d srgb=%d filter=%d address=%d"),
                        Name, *Volume->GetPathName(), Volume->GetSizeX(), Volume->GetSizeY(), Volume->GetSizeZ(),
                        Volume->GetNumMips(), int32(Volume->GetPixelFormat()), Volume->SRGB, int32(Volume->Filter), int32(Volume->AddressMode));
                }
                FlatVolume.Reset(UVolumeTexture::CreateTransient(4, 4, 4, PF_B8G8R8A8));
                if (!FlatVolume.IsValid() || !FlatVolume->GetPlatformData() || FlatVolume->GetPlatformData()->Mips.Num() != 1)
                { Error = TEXT("Constant volume allocation failed"); return false; }
                FlatVolume->SRGB = false; FlatVolume->NeverStream = true;
                auto& Data = FlatVolume->GetPlatformData()->Mips[0].BulkData;
                FColor* Voxels = static_cast<FColor*>(Data.Lock(LOCK_READ_WRITE));
                for (int32 I = 0; I < 64; ++I) Voxels[I] = FColor(128, 128, 128, 255);
                Data.Unlock(); FlatVolume->UpdateResource();
            }
            Saved.Reset(UMaterialInstanceDynamic::Create(Material->Parent, GetTransientPackage()));
            if (!Saved.IsValid()) { Error = TEXT("Pattern snapshot allocation failed"); return false; }
            if (!FlatTexture.IsValid())
            {
                FlatTexture.Reset(UTexture2D::CreateTransient(4, 4, PF_B8G8R8A8));
                if (!FlatTexture.IsValid() || !FlatTexture->GetPlatformData()
                    || FlatTexture->GetPlatformData()->Mips.Num() != 1)
                { Error = TEXT("Constant albedo texture allocation failed"); return false; }
                FlatTexture->SRGB = true;
                FlatTexture->NeverStream = true;
                auto& Data = FlatTexture->GetPlatformData()->Mips[0].BulkData;
                FColor* Pixels = static_cast<FColor*>(Data.Lock(LOCK_READ_WRITE));
                for (int32 I = 0; I < 16; ++I) Pixels[I] = FColor(128, 128, 128, 255);
                Data.Unlock();
                FlatTexture->UpdateResource();
            }
            Saved->CopyParameterOverrides(Material);
            Scalars = Material->ScalarParameterValues; Textures = Material->TextureParameterValues;
            Active = true;
            return Validate(Error);
        }
        bool Validate(FString& Error) const
        {
            if (!Active || !Live.IsValid() || !Saved.IsValid() || Live->Parent != Saved->Parent
                || Live->ScalarParameterValues != Scalars || Live->TextureParameterValues != Textures
                || Live->VectorParameterValues != Saved->VectorParameterValues
                || Live->DoubleVectorParameterValues != Saved->DoubleVectorParameterValues
                || Live->FontParameterValues != Saved->FontParameterValues
                || !Live->RuntimeVirtualTextureParameterValues.IsEmpty()
                || !Live->SparseVolumeTextureParameterValues.IsEmpty())
            { Error = TEXT("Pattern isolation parameters or physical frame drifted"); return false; }
            return true;
        }
        bool Apply(int32 Variant, FString& Error)
        {
            if (Variant < 0 || Variant >= Count() || !Validate(Error)) return false;
            Live->CopyParameterOverrides(Saved.Get());
            if (AllFields())
            {
                if ((Variant >= 1 && Variant <= 4) || Variant == 6)
                    for (const TCHAR* Name : Volumes) Live->SetTextureParameterValue(Name, FlatVolume.Get());
                if (Variant == 2 || Variant == 5 || Variant == 6)
                    for (const TCHAR* Name : Albedos) Live->SetTextureParameterValue(Name, FlatTexture.Get());
                if (Variant == 4 || Variant == 5 || Variant == 6)
                    Live->SetTextureParameterValue(TEXT("PlanetaryTexture"), FlatTexture.Get());
                if (Variant == 3 || Variant == 5 || Variant == 6)
                    Live->SetScalarParameterValue(TEXT("APS_OrbitalMacroMode"), 2.0f);
            }
            else if (VolumesOnly())
            {
                if (Variant >= 1 && Variant <= 5) Live->SetTextureParameterValue(Volumes[Variant - 1], FlatVolume.Get());
            }
            else if (Variant == 1)
                for (const TCHAR* Name : Albedos) Live->SetTextureParameterValue(Name, FlatTexture.Get());
            if (!VolumesOnly() && Variant == 2) Live->SetScalarParameterValue(TEXT("APS_OrbitalMacroMode"), 2.0f);
            if (!VolumesOnly() && Variant == 3)
                for (const TCHAR* Name : NoiseSizes)
                {
                    float Value = 0;
                    if (!Saved->GetScalarParameterValue(FHashedMaterialParameterInfo(Name), Value))
                    { Error = TEXT("Lost native noise scale snapshot"); return false; }
                    Live->SetScalarParameterValue(Name, Value * 1.731f);
                }
            Scalars = Live->ScalarParameterValues; Textures = Live->TextureParameterValues;
            UE_LOG(LogTemp, Display, TEXT("PLANET_PATTERN_ISOLATION variant=%s noSavedAssetWrites=1 geometryUnchanged=1"), Label(Variant));
            return Validate(Error);
        }
        bool Restore()
        {
            if (!Active) return true;
            if (!Live.IsValid() || !Saved.IsValid()) return false;
            Live->CopyParameterOverrides(Saved.Get());
            Scalars = Saved->ScalarParameterValues; Textures = Saved->TextureParameterValues;
            FString Error;
            const bool Result = Validate(Error);
            Active = false;
            return Result;
        }
    };
}
#endif
