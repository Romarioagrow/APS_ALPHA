#pragma once

#if WITH_EDITOR
#include "APSSharedTerrainMaterialBuilder.h"
#include "APS_ALPHA/Core/Planetary/APSSharedLavaMaterial.h"
#include "Materials/MaterialExpressionDistance.h"
#include "Materials/MaterialExpressionLinearInterpolate.h"
#include "Materials/MaterialExpressionSmoothStep.h"
#include "Materials/MaterialExpressionOneMinus.h"
#include "Materials/MaterialExpressionVertexInterpolator.h"
#include "Materials/MaterialExpressionVertexNormalWS.h"
#include "Materials/MaterialAttributeDefinitionMap.h"

// Protected candidate: accepted terrain and lava graphs remain read-only.
// No WorldScape vertex ABI change, new texture, palette override, or WPO.
namespace APSUnifiedLavaSurfaceBuilder
{
    using FCore = APSSharedTerrainMaterialBuilder::FBuild;
    inline constexpr const TCHAR* Destination = TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/UnifiedLava");

    struct FBuild : FCore
    {
        UMaterialInstance* LavaTemplate = nullptr;
        TMap<UMaterialFunction*, UMaterialFunction*> LavaFunctions;
        TSet<UMaterialFunction*> Visiting;

        explicit FBuild(IAssetTools& Tools) : FCore(Tools, Destination) {}

        FExpressionInput PropertyValue(UMaterial* Owner, UMaterial* Source, EMaterialProperty P)
        {
            FExpressionInput* Input = Source->GetExpressionInputForProperty(P);
            FExpressionInput Result = *Input;
            const FVector4f Default = FMaterialAttributeDefinitionMap::GetDefaultValue(P);
            if (P == MP_BaseColor || P == MP_EmissiveColor)
            {
                const auto* Typed = static_cast<const FColorMaterialInput*>(Input);
                if (!Typed->UseConstant && Input->Expression) return Result;
                auto* Constant = Add<UMaterialExpressionConstant3Vector>(Owner);
                Constant->Constant = Typed->UseConstant ? FLinearColor(Typed->Constant)
                    : FLinearColor(Default.X, Default.Y, Default.Z);
                Result = FExpressionInput(); Result.Expression = Constant;
            }
            else if (P == MP_Normal)
            {
                const auto* Typed = static_cast<const FVectorMaterialInput*>(Input);
                if (!Typed->UseConstant && Input->Expression) return Result;
                Result = FExpressionInput();
                if (Typed->UseConstant)
                {
                    auto* Constant = Add<UMaterialExpressionConstant3Vector>(Owner);
                    Constant->Constant = FLinearColor(Typed->Constant.X, Typed->Constant.Y, Typed->Constant.Z);
                    Result.Expression = Constant;
                }
                else Result.Expression = Add<UMaterialExpressionVertexNormalWS>(Owner);
            }
            else
            {
                const auto* Typed = static_cast<const FScalarMaterialInput*>(Input);
                if (!Typed->UseConstant && Input->Expression) return Result;
                auto* Constant = Add<UMaterialExpressionConstant>(Owner);
                Constant->R = Typed->UseConstant ? Typed->Constant : Default.X;
                Result = FExpressionInput(); Result.Expression = Constant;
            }
            return Result;
        }

        bool FreezeLavaParameters(UObject* Owner)
        {
            for (UMaterialExpression* E : Expressions(Owner))
            {
                if (auto* Call = Cast<UMaterialExpressionMaterialFunctionCall>(E))
                {
                    UMaterialFunction* Source = Cast<UMaterialFunction>(Call->MaterialFunction);
                    if (!Source) { Error = TEXT("Unsupported lava function instance"); return false; }
                    if (Visiting.Contains(Source)) { Error = TEXT("Recursive lava function"); return false; }
                    UMaterialFunction* Copy = LavaFunctions.FindRef(Source);
                    if (!Copy)
                    {
                        Copy = Cast<UMaterialFunction>(Duplicate(Source, FString::Printf(
                            TEXT("MF_APS_UnifiedLava_%08x"), FCrc::StrCrc32(*Source->GetPathName()))));
                        if (!Copy) return false;
                        LavaFunctions.Add(Source, Copy);
                        Visiting.Add(Source);
                        if (!FreezeLavaParameters(Copy)) return false;
                        Visiting.Remove(Source);
                        UMaterialEditingLibrary::UpdateMaterialFunction(Copy);
                    }
                    if (!ReconnectFunctionById(Call, Source, Copy)) return false;
                }
                if (!E->HasAParameterName()) continue;
                const FName Name = E->GetParameterName();
                // Both original graphs intentionally share ONLY their physical
                // frame. Chemistry/style parameters must not collide by name.
                if (Name.ToString().StartsWith(TEXT("APS_Shared"))) continue;
                FMaterialParameterMetadata Value;
                if (!LavaTemplate->GetParameterValue(E->GetParameterType(),
                        FMaterialParameterInfo(Name), Value)
                    || !E->SetParameterValue(Name, Value,
                        EMaterialExpressionSetParameterValueFlags::NoUpdateExpressionGuid))
                {
                    Error = TEXT("Cannot freeze evaluated lava parameter: ") + Name.ToString();
                    return false;
                }
                E->SetParameterName(FName(*(TEXT("APS_UL_") + Name.ToString())));
                E->UpdateParameterGuid(true, true);
            }
            return true;
        }

        bool Run()
        {
            auto* TerrainTemplate = LoadObject<UMaterialInstanceConstant>(nullptr,
                APSSharedTerrainMaterial::TemplatePath(EAPSPlanetSurfaceArchetype::Magmatic));
            LavaTemplate = LoadObject<UMaterialInstance>(nullptr, APSSharedLavaMaterial::TemplatePath());
            if (!APSSharedTerrainMaterial::IsSharedStack(TerrainTemplate)
                || !APSSharedLavaMaterial::IsSharedStack(LavaTemplate))
            { Error = TEXT("Accepted source stack missing"); return false; }
            UMaterial* Terrain = TerrainTemplate->GetMaterial();
            UMaterial* Lava = LavaTemplate->GetMaterial();
            const auto* TerrainCoverage = Cast<UMaterialExpressionConstant>(
                Terrain->GetExpressionInputForProperty(MP_OpacityMask)->Expression);
            const bool CompleteTerrain = Terrain->GetBlendMode() == BLEND_Opaque
                || (Terrain->GetBlendMode() == BLEND_Masked && TerrainCoverage && TerrainCoverage->R == 1.0f);
            if (!CompleteTerrain || Terrain->bUseMaterialAttributes
                || Lava->bUseMaterialAttributes || Terrain->bTangentSpaceNormal
                || !Terrain->GetShadingModels().HasShadingModel(MSM_DefaultLit)
                || !Lava->GetShadingModels().HasShadingModel(MSM_DefaultLit))
            { Error = TEXT("Unsupported source shading/normal contract"); return false; }
            for (UMaterial* Source : {Terrain, Lava})
                for (EMaterialProperty P : {MP_WorldPositionOffset, MP_PixelDepthOffset})
                    if (Source->GetExpressionInputForProperty(P)->Expression)
                    { Error = TEXT("Source displacement requires audit"); return false; }
            if (Lava->GetExpressionInputForProperty(MP_Normal)->Expression
                || static_cast<FVectorMaterialInput*>(Lava->GetExpressionInputForProperty(MP_Normal))->UseConstant)
            { Error = TEXT("New lava normal requires explicit space conversion"); return false; }

            auto* Master = Cast<UMaterial>(Duplicate(Terrain, TEXT("M_APS_UnifiedLavaSurface")));
            if (!Master) return false;
            // Duplicate the entire lava graph first so every internal expression
            // reference is remapped by Unreal; move its nodes, not hand-wired pins.
            auto* LavaCopy = DuplicateObject<UMaterial>(Lava, GetTransientPackage());
            if (!LavaCopy || !FreezeLavaParameters(LavaCopy)) return false;
            const TArray<UMaterialExpression*> LavaNodes = Expressions(LavaCopy);
            for (UMaterialExpression* E : LavaNodes)
            {
                const FName Unique = MakeUniqueObjectName(Master, E->GetClass(), E->GetFName());
                if (!E->Rename(*Unique.ToString(), Master, REN_DontCreateRedirectors | REN_NonTransactional))
                { Error = TEXT("Cannot move cloned lava node"); return false; }
                E->Material = Master;
                Master->GetExpressionCollection().AddExpression(E);
            }

            // Evaluate the radial height AT VERTICES, never from a pixel's chord
            // through a coarse spherical triangle (that creates square LOD masks).
            FFrame Frame(*this, Master);
            auto* Position = Add<UMaterialExpressionWorldPosition>(Master);
            auto* Radius = Add<UMaterialExpressionDistance>(Master);
            Radius->A.Expression = Frame.Position(Position);
            Radius->B.Expression = Frame.Zero;
            auto* Sea = Add<UMaterialExpressionScalarParameter>(Master);
            Sea->ParameterName = TEXT("APS_UnifiedSeaRadiusCm");
            Sea->DefaultValue = 675000000.0f;
            Sea->UpdateParameterGuid(true, true);
            auto* Delta = Add<UMaterialExpressionSubtract>(Master);
            Delta->A.Expression = Radius; Delta->B.Expression = Sea;
            auto* Height = Add<UMaterialExpressionVertexInterpolator>(Master);
            Height->Input.Expression = Delta;
            auto* Tolerance = Add<UMaterialExpressionScalarParameter>(Master);
            Tolerance->ParameterName = TEXT("APS_UnifiedRadiusToleranceCm");
            Tolerance->DefaultValue = 675.0f;
            Tolerance->UpdateParameterGuid(true, true);
            auto* RelativeHeight = Add<UMaterialExpressionDivide>(Master);
            RelativeHeight->A.Expression = Height; RelativeHeight->B.Expression = Tolerance;
            auto* Shore = Add<UMaterialExpressionSmoothStep>(Master);
            Shore->Value.Expression = RelativeHeight;
            Shore->ConstMin = 1.0f; Shore->ConstMax = 3.0f;
            auto* LavaWeight = Add<UMaterialExpressionOneMinus>(Master);
            LavaWeight->Input.Expression = Shore;

            for (EMaterialProperty P : {MP_BaseColor, MP_EmissiveColor, MP_Roughness,
                    MP_Metallic, MP_Specular, MP_AmbientOcclusion, MP_Normal})
            {
                FExpressionInput* Out = Master->GetExpressionInputForProperty(P);
                auto* Blend = Add<UMaterialExpressionLinearInterpolate>(Master);
                Blend->A = PropertyValue(Master, Master, P);
                Blend->B = PropertyValue(Master, LavaCopy, P);
                Blend->Alpha.Expression = LavaWeight;
                // Constant overrides take precedence over connected expressions.
                if (P == MP_BaseColor || P == MP_EmissiveColor) static_cast<FColorMaterialInput*>(Out)->UseConstant = false;
                else if (P == MP_Normal) static_cast<FVectorMaterialInput*>(Out)->UseConstant = false;
                else static_cast<FScalarMaterialInput*>(Out)->UseConstant = false;
                *Out = FExpressionInput(); Out->Expression = Blend;
            }
            // No second transparent shell and no presentation/Hole alpha mask.
            Master->BlendMode = BLEND_Opaque;
            *Master->GetExpressionInputForProperty(MP_OpacityMask) = FExpressionInput();
            auto* Template = Cast<UMaterialInstanceConstant>(Duplicate(TerrainTemplate, TEXT("MI_APS_UnifiedLavaSurface")));
            if (!Template) return false;
            Template->SetParentEditorOnly(Master, false);
            Master->PostEditChange(); Template->PostEditChange();
            if (Template->GetBlendMode() != BLEND_Opaque)
            { Error = TEXT("Inherited base-property override changed candidate coverage"); return false; }
            for (UObject* Output : Outputs)
                if (auto* M = Cast<UMaterialInterface>(Output))
                    M->ForceRecompileForRendering(EMaterialShaderPrecompileMode::Default);
            FAssetCompilingManager::Get().FinishAllCompilation();
            if (GShaderCompilingManager) GShaderCompilingManager->FinishAllCompilation();
            for (UObject* Output : Outputs)
            {
                if (auto* M = Cast<UMaterialInterface>(Output))
                {
                    FMaterialResource* R = M->GetMaterialResource(GMaxRHIFeatureLevel);
                    FMaterialShaderMap* Map = R ? R->GetGameThreadShaderMap() : nullptr;
                    if (!R || !R->IsGameThreadShaderMapComplete() || R->GetCompileErrors().Num()
                        || !Map || !Map->GetMeshShaderMap(&FLocalVertexFactory::StaticType))
                    { Error = TEXT("Incomplete candidate / missing LocalVF"); return false; }
                }
            }
            for (UObject* Output : Outputs)
            {
                UPackage* Package = Output->GetOutermost();
                if (!Package->GetName().StartsWith(FString(Destination) + TEXT("/")))
                { Error = TEXT("Output escaped candidate directory"); return false; }
                FSavePackageArgs Args;
                Args.TopLevelFlags = RF_Public | RF_Standalone; Args.SaveFlags = SAVE_NoError;
                const FString Filename = FPackageName::LongPackageNameToFilename(
                    Package->GetName(), FPackageName::GetAssetPackageExtension());
                if (!UPackage::SavePackage(Package, Output, *Filename, Args))
                { Error = TEXT("Candidate save failed"); return false; }
            }
            return true;
        }
    };

    inline bool Build(IAssetTools& Tools)
    {
        FBuild Builder(Tools);
        const bool OK = Builder.Run();
        UE_LOG(LogTemp, Display, TEXT("[APS.UnifiedLava] candidate=%d outputs=%d error=%s; NOT visual acceptance"),
            OK, Builder.Outputs.Num(), *Builder.Error);
        return OK;
    }
}
#endif
