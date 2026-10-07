#pragma once

#if WITH_DEV_AUTOMATION_TESTS
#include "Engine/Texture2D.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "UObject/StrongObjectPtr.h"

// Causal removal only, never a production material. Geometry is leased separately.
namespace APSPlanetNormalSources
{
    inline bool Curvature() { return FParse::Param(FCommandLine::Get(), TEXT("APSProbeMeshCurvature")); }
    inline bool Requested() { return Curvature() || FParse::Param(FCommandLine::Get(), TEXT("APSProbeNormalSources")); }
    inline int32 Count() { return Curvature()?3:6; }
    inline bool Radial(int32 Phase) { return Phase == 3 || Phase == 4; }
    inline bool ModifiedNormals(int32 Phase) { return Curvature()?Phase==1:Radial(Phase); }
    inline bool ReplacedTextures(int32 Phase) { return !Curvature() && (Phase == 1 || Phase == 2 || Phase == 4); }
    inline const TCHAR* Label(int32 Phase)
    {
        if(Curvature())return Phase==0?TEXT("Curvature0Native"):Phase==1?TEXT("Curvature1Corrected"):TEXT("Curvature2Restored");
        static const TCHAR* Names[] = { TEXT("Normals0Native"), TEXT("Normals1FlatMacro"),
            TEXT("Normals2FlatAll"), TEXT("Normals3RadialMesh"), TEXT("Normals4FlatAllRadial"), TEXT("Normals5Restored") };
        return Phase >= 0 && Phase < Count() ? Names[Phase] : TEXT("invalid");
    }
    class FScope
    {
        TStrongObjectPtr<UMaterialInstanceDynamic> Native{nullptr}, Copy{nullptr};
        TStrongObjectPtr<UTexture2D> Flat{nullptr};
        TArray<FTextureParameterValue> Expected;
        static constexpr const TCHAR* Names[] = {TEXT("SlopeNormal"), TEXT("SlopeNormal3"), TEXT("MacroSlopeNormal"),
            TEXT("Ground1N (T2d)"), TEXT("Ground2N(T2d)"), TEXT("Ground3"), TEXT("PlanetaryNormal")};
    public:
        UMaterialInstanceDynamic* Material() const { return Copy.Get(); }
        bool Begin(UMaterialInstanceDynamic* Source, FString& Error)
        {
            if (!Source || !Source->Parent) { Error=TEXT("Normal isolation has no native parent"); return false; }
            Flat.Reset(LoadObject<UTexture2D>(nullptr,TEXT("/Engine/EngineMaterials/DefaultNormal.DefaultNormal")));
            if (!Flat.IsValid() || Flat->SRGB || Flat->CompressionSettings != TC_Normalmap)
            { Error=TEXT("Engine flat normal has unexpected format"); return false; }
            TArray<FMaterialParameterInfo> Infos; TArray<FGuid> Ids;
            Source->GetAllTextureParameterInfo(Infos,Ids);
            for (const TCHAR* Name : Names)
            {
                if (!Infos.Contains(FMaterialParameterInfo(Name))) { Error=FString(TEXT("Missing normal parameter: "))+Name; return false; }
                UTexture* Texture=nullptr; Source->GetTextureParameterValue(FHashedMaterialParameterInfo(Name),Texture);
                UE_LOG(LogTemp,Display,TEXT("[APS.NormalSources] parameter=%s native=%s"),Name,*GetPathNameSafe(Texture));
            }
            Native.Reset(Source); Copy.Reset(UMaterialInstanceDynamic::Create(Source->Parent,GetTransientPackage()));
            if (!Copy.IsValid()) { Error=TEXT("Normal probe MID allocation failed"); return false; }
            return Apply(0,Error);
        }
        bool Apply(int32 Phase,FString& Error)
        {
            if (!Native.IsValid() || !Copy.IsValid() || Phase<0 || Phase>=Count())
            { Error=TEXT("Normal source probe invalid phase"); return false; }
            Copy->CopyParameterOverrides(Native.Get());
            if (ReplacedTextures(Phase))
                for (int32 I=0; I<(Phase==1?3:UE_ARRAY_COUNT(Names)); ++I)
                    Copy->SetTextureParameterValue(Names[I],Flat.Get());
            Expected=Copy->TextureParameterValues;
            return Validate(Error);
        }
        bool Validate(FString& Error) const
        {
            if (!Native.IsValid() || !Copy.IsValid() || Copy->Parent != Native->Parent
                || Copy->ScalarParameterValues != Native->ScalarParameterValues
                || Copy->VectorParameterValues != Native->VectorParameterValues
                || Copy->DoubleVectorParameterValues != Native->DoubleVectorParameterValues
                || Copy->FontParameterValues != Native->FontParameterValues
                || Copy->TextureParameterValues != Expected
                || !Native->RuntimeVirtualTextureParameterValues.IsEmpty() || !Native->SparseVolumeTextureParameterValues.IsEmpty())
            { Error=TEXT("Normal source probe changed non-texture parameters or lost overrides"); return false; }
            return true;
        }
    };
}
#endif
