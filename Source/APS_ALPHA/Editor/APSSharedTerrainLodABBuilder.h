#pragma once

#if WITH_EDITOR
#include "APSSharedTerrainMaterialBuilder.h"
#include "APSOrbitalColorFieldsAB.h"
#include "LocalVertexFactory.h"
#include "Engine/Texture2D.h"
#include "Materials/MaterialExpressionTextureObject.h"
#include "Materials/MaterialExpressionVertexInterpolator.h"

// Diagnostic duplicate of the CURRENT shared graph. Never rebuild from the
// authored vendor graph and never overwrite production or previous outputs.
namespace APSSharedTerrainLodABBuilder
{
    inline constexpr const TCHAR* Destination = TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/Diagnostics/LodPixelAB");
    inline constexpr const TCHAR* TemplatePath = TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/Diagnostics/LodPixelAB/MI_APS_LodPixelTerra.MI_APS_LodPixelTerra");

    struct FBuilder
    {
        APSSharedTerrainMaterialBuilder::FBuild B;
        TMap<UMaterialFunction*, bool> Needed;
        TSet<UMaterialFunction*> Visiting;
        TMap<UMaterialFunction*, UMaterialFunction*> Copies;
        const bool bSlopeOnly = FParse::Param(FCommandLine::Get(), TEXT("APSBuildTerrainSlopeOnly"));
        const bool bWarpOnly = FParse::Param(FCommandLine::Get(), TEXT("APSBuildTerrainWarpOnly"));
        const bool bSideOnly = FParse::Param(FCommandLine::Get(), TEXT("APSBuildTerrainSlopeSide"));
        const bool bFields = FParse::Param(FCommandLine::Get(), TEXT("APSBuildTerrainOrbitalFields"));
        const bool bMagmaFields = FParse::Param(FCommandLine::Get(), TEXT("APSBuildTerrainOrbitalMagma"));
        int32 Bypassed = 0;
        int32 MasterBypassed = 0;
        int32 FunctionBypassed = 0;

        explicit FBuilder(IAssetTools& Tools) : B(Tools,
            FParse::Param(FCommandLine::Get(), TEXT("APSBuildTerrainOrbitalFields"))
                ? (FParse::Param(FCommandLine::Get(), TEXT("APSBuildTerrainOrbitalMagma"))
                    ? TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/Diagnostics/OrbitalFields20260929MagmaV3")
                    : TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/Diagnostics/OrbitalFields20260929V3"))
                : FParse::Param(FCommandLine::Get(), TEXT("APSBuildTerrainSlopeSide"))
                ? TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/Diagnostics/LodSlopeSide20260929")
                : FParse::Param(FCommandLine::Get(), TEXT("APSBuildTerrainSlopeOnly"))
                ? TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/Diagnostics/LodSlopePreserve20260927")
                : FParse::Param(FCommandLine::Get(), TEXT("APSBuildTerrainWarpOnly"))
                    ? TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/Diagnostics/LodWarpPixel20260929") : Destination) {}

        bool NeedsCopy(UMaterialFunction* Function)
        {
            if (const bool* Found = Needed.Find(Function)) return *Found;
            if (Visiting.Contains(Function)) { B.Error = TEXT("Recursive function graph"); return false; }
            Visiting.Add(Function);
            bool Result = false;
            for (UMaterialExpression* E : B.Expressions(Function))
            {
                Result |= IsValid(Cast<UMaterialExpressionVertexInterpolator>(E));
                if (auto* Call = Cast<UMaterialExpressionMaterialFunctionCall>(E))
                {
                    if (auto* Child = Cast<UMaterialFunction>(Call->MaterialFunction)) Result |= NeedsCopy(Child);
                    else if (Call->MaterialFunction) B.Error = TEXT("Function instance requires explicit audit");
                }
            }
            Visiting.Remove(Function);
            Needed.Add(Function, Result);
            return Result;
        }

        bool Patch(UObject* Owner)
        {
            const auto Original = B.Expressions(Owner);
            TMap<UMaterialExpression*, UMaterialExpression*> Replacements;
            for (UMaterialExpression* E : Original)
            {
                if (auto* VI = Cast<UMaterialExpressionVertexInterpolator>(E))
                {
                    // The inspected graph uses identity Float1/Float3 pins. Do not
                    // silently reinterpret an unfamiliar channel swizzle/pin.
                    if (!VI->Input.Expression || VI->Input.OutputIndex != 0 || VI->Input.Mask)
                    { B.Error = TEXT("Unexpected VertexInterpolator input mapping: ") + VI->GetPathName(); return false; }
                    Replacements.Add(VI, VI->Input.Expression);
                    ++Bypassed;
                }
                else if (auto* Call = Cast<UMaterialExpressionMaterialFunctionCall>(E))
                {
                    auto* Source = Cast<UMaterialFunction>(Call->MaterialFunction);
                    // Isolate ONLY the five coordinate warps in the current master.
                    // Keep current slope/normal continuity and precision functions identical.
                    if (bWarpOnly || bFields || !Source || !NeedsCopy(Source)) continue;
                    if (!B.Error.IsEmpty()) return false;
                    UMaterialFunction* Copy = Copies.FindRef(Source);
                    if (!Copy)
                    {
                        const FString Name = FString::Printf(TEXT("MF_LodPixel_%s_%08x"), *Source->GetName(), FCrc::StrCrc32(*Source->GetPathName()));
                        Copy = Cast<UMaterialFunction>(B.Duplicate(Source, Name));
                        if (!Copy) return false;
                        Copies.Add(Source, Copy);
                        if (!Patch(Copy)) return false;
                        UMaterialEditingLibrary::UpdateMaterialFunction(Copy);
                    }
                    if (!B.ReconnectFunctionById(Call, Source, Copy)) return false;
                }
            }
            const auto Rewire = [&](FExpressionInput& Input)
            {
                TSet<UMaterialExpression*> Seen;
                while (UMaterialExpression** Source = Replacements.Find(Input.Expression))
                {
                    if (Input.OutputIndex != 0 || Seen.Contains(Input.Expression))
                    { B.Error = TEXT("Unexpected VertexInterpolator consumer mapping"); return; }
                    Seen.Add(Input.Expression);
                    // Keep all consumer masks, OutputIndex, InputName intact.
                    Input.Expression = *Source;
                }
            };
            for (UMaterialExpression* E : B.Expressions(Owner))
                if (E) for (FExpressionInput* Input : E->GetInputsView()) if (Input) Rewire(*Input);
            if (auto* Material = Cast<UMaterial>(Owner))
                for (int32 P = 0; P < MP_MAX; ++P)
                    if (FExpressionInput* Input = Material->GetExpressionInputForProperty(static_cast<EMaterialProperty>(P))) Rewire(*Input);
            if (Cast<UMaterial>(Owner)) MasterBypassed += Replacements.Num();
            else FunctionBypassed += Replacements.Num();
            UE_LOG(LogTemp, Display, TEXT("[APS.LodPixelAB] Bypass owner=%s interpolators=%d; exact inputs/palette/coordinates retained, evaluation stage only"), *Owner->GetPathName(), Replacements.Num());
            return B.Error.IsEmpty();
        }

        bool PatchSlopeOnly(UMaterial* Master)
        {
            int32 Calls = 0, Blends = 0;
            using FBuild = APSSharedTerrainMaterialBuilder::FBuild;
            APSSharedTerrainNormalContinuity::TTransform<FBuild> Reader(B);
            for (UMaterialExpression* E : B.Expressions(Master))
            {
                auto* Call = Cast<UMaterialExpressionMaterialFunctionCall>(E);
                auto* Source = Call ? Cast<UMaterialFunction>(Call->MaterialFunction) : nullptr;
                if (!Source || Source->GetPathName() != TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/Shared/MF_APS_MF_SlopeBlock_f6be5407.MF_APS_MF_SlopeBlock_f6be5407")) continue;
                ++Calls;
                auto* Copy = Cast<UMaterialFunction>(B.Duplicate(Source, TEXT("MF_APS_PreserveGeometricSlope")));
                if (!Copy) return false;
                if (bSideOnly)
                {
                    if (!PatchSlopeSide(Copy)) return false;
                    ++Blends;
                    UMaterialEditingLibrary::UpdateMaterialFunction(Copy);
                    if (!B.ReconnectFunctionById(Call, Source, Copy)) return false;
                    continue;
                }
                for (UMaterialExpression* Node : Reader.Graph(Copy))
                {
                    // Keep connected nodes in the owned copy's expression inventory
                    // so a fresh editor can restore function pins without DDC.
                    Copy->GetExpressionCollection().AddExpression(Node);
                    auto* Blend = Cast<UMaterialExpressionCustom>(Node);
                    if (!Blend || Blend->Description != APSSharedTerrainNormalContinuity::SlopeDescription) continue;
                    if (!Blend->Code.Contains(TEXT("return lerp(NativeSlope,PixelSlope,w);"))
                        || Blend->Inputs.Num() != 6 || Blend->Inputs[0].InputName != TEXT("NativeSlope")
                        || !Cast<UMaterialExpressionVertexInterpolator>(Blend->Inputs[0].Input.Expression))
                    { B.Error = TEXT("Audited slope blend contract changed"); return false; }
                    // One-variable comparison: preserve the same native slope at
                    // all distances. Do not alter triplanar projection normals,
                    // textures, palette, physical coordinates or mesh payload.
                    Blend->Code = TEXT("return NativeSlope;");
                    ++Blends;
                }
                if (!B.Error.IsEmpty()) return false;
                UMaterialEditingLibrary::UpdateMaterialFunction(Copy);
                if (!B.ReconnectFunctionById(Call, Source, Copy)) return false;
            }
            if (Calls != 1 || Blends != 1)
            { B.Error = FString::Printf(TEXT("Expected one slope call/blend, got %d/%d"), Calls, Blends); return false; }
            return true;
        }

        bool PatchSlopeSide(UMaterialFunction* Copy)
        {
            using FBuild = APSSharedTerrainMaterialBuilder::FBuild;
            APSSharedTerrainNormalContinuity::TTransform<FBuild> Reader(B);
            const auto Graph = Reader.Graph(Copy);
            UMaterialExpressionCustom* Slope = nullptr;
            UTexture2D* Texture = nullptr;
            for (UMaterialExpression* Node : Graph)
            {
                Reader.Register(Copy, Node);
                if (auto* C = Cast<UMaterialExpressionCustom>(Node);
                    C && C->Description == APSSharedTerrainNormalContinuity::SlopeDescription) Slope = C;
                if (auto* T = Cast<UMaterialExpressionTextureObject>(Node);
                    T && T->Texture && T->Texture->GetPathName() == TEXT("/Game/Ressources/Textures/Rock/T_MountainSide.T_MountainSide"))
                    Texture = Cast<UTexture2D>(T->Texture);
            }
            TArray64<uint8> Pixels;
            if (!Slope || Slope->Inputs.Num() != 6 || !Texture || !Texture->Source.GetMipData(Pixels, 0))
            { B.Error = TEXT("Slope side source contract changed"); return false; }
            const bool Gray = Texture->Source.GetFormat() == TSF_G8;
            const int64 Stride = Gray ? 1 : 4;
            const int64 Count = int64(Texture->Source.GetSizeX()) * Texture->Source.GetSizeY();
            if ((!Gray && Texture->Source.GetFormat() != TSF_BGRA8) || Count <= 0 || Pixels.Num() != Count * Stride)
            { B.Error = TEXT("Slope side source format changed"); return false; }
            FVector3d Sum = FVector3d::ZeroVector;
            for (int64 I = 0; I < Count; ++I)
            {
                const uint8* P = Pixels.GetData() + I * Stride;
                const FColor C = Gray ? FColor(P[0],P[0],P[0]) : FColor(P[2],P[1],P[0]);
                const FLinearColor L = Texture->SRGB ? FLinearColor(C) : C.ReinterpretAsLinear();
                Sum += FVector3d(L.R,L.G,L.B);
            }
            auto* Mean = Reader.Add<UMaterialExpressionConstant3Vector>(Copy);
            Mean->Constant = FLinearColor(Sum.X / Count, Sum.Y / Count, Sum.Z / Count);
            int32 Patched = 0, Consumers = 0;
            for (UMaterialExpression* Node : Graph)
            {
                auto* Call = Cast<UMaterialExpressionMaterialFunctionCall>(Node);
                if (!Call || !Call->MaterialFunction || Call->MaterialFunction->GetName() != TEXT("MF_APS_WorldAlignedTexture_a83aa78c")) continue;
                const auto* Pin = Call->FunctionInputs.FindByPredicate([](const FFunctionExpressionInput& P) { return P.Input.InputName == TEXT("TextureObject"); });
                const auto* Object = Pin ? Cast<UMaterialExpressionTextureObject>(Pin->Input.Expression) : nullptr;
                if (!Object || Object->Texture != Texture) { B.Error = TEXT("Unexpected slope texture sampler"); return false; }
                auto* Filter = Reader.Add<UMaterialExpressionCustom>(Copy);
                Filter->Description = TEXT("APS slope side repeat filter; native near, source mean far v1");
                Filter->OutputType = CMOT_Float3; Filter->Inputs.Empty();
                FCustomInput Legacy; Legacy.InputName = TEXT("Legacy"); Legacy.Input.Expression = Call; Legacy.Input.OutputIndex = 2;
                Filter->Inputs.Add(Legacy);
                FCustomInput Average; Average.InputName = TEXT("Mean"); Average.Input.Expression = Mean; Filter->Inputs.Add(Average);
                for (int32 I = 2; I < 6; ++I) Filter->Inputs.Add(Slope->Inputs[I]);
                Filter->Code = TEXT("float d=length(CameraDelta.xyz)*max(InverseScale.x,0.0);\n")
                    TEXT("float w=smoothstep(StartCm,max(EndCm,StartCm+1.0),d);\nreturn lerp(Legacy,Mean,w);\n");
                for (UMaterialExpression* Consumer : Graph)
                    for (FExpressionInput* In : Consumer->GetInputsView())
                        if (In && In->Expression == Call)
                        {
                            if (In->OutputIndex != 2) { B.Error = TEXT("Unexpected slope side projection consumer"); return false; }
                            In->Expression = Filter; In->OutputIndex = 0; ++Consumers;
                        }
                ++Patched;
            }
            UE_LOG(LogTemp, Display, TEXT("[APS.SlopeSideAB] samples=%d consumers=%d mean=(%.6f %.6f %.6f); no geometry/noise/palette changes"),
                Patched, Consumers, Mean->Constant.R, Mean->Constant.G, Mean->Constant.B);
            if (Patched != 2 || Consumers != 2) { B.Error = TEXT("Expected two side samplers/consumers"); return false; }
            return true;
        }

        bool RestoreTransientFunctionPins(UMaterial* Master)
        {
            using FBuild = APSSharedTerrainMaterialBuilder::FBuild;
            APSSharedTerrainNormalContinuity::TTransform<FBuild> Reader(B);
            TArray<UObject*> Pending{Master};
            TSet<UObject*> Visited;
            for (int32 Index = 0; Index < Pending.Num(); ++Index)
            {
                UObject* Owner = Pending[Index];
                if (Visited.Contains(Owner)) continue;
                if (Visited.Num() >= 128) { B.Error = TEXT("Function graph exceeds bound"); return false; }
                Visited.Add(Owner);
                for (UMaterialExpression* E : Reader.Graph(Owner))
                {
                    auto* Call = Cast<UMaterialExpressionMaterialFunctionCall>(E);
                    if (!Call || !Call->MaterialFunction) continue;
                    auto* Function = Cast<UMaterialFunction>(Call->MaterialFunction);
                    if (!Function) { B.Error = TEXT("Function instance requires audit"); return false; }
                    // Legacy copies contain connected expressions outside the flat
                    // list used by UE to restore transient pointers. Keep every
                    // serialized edge; only recover pointers by exact GUID/name.
                    TArray<FFunctionExpressionInput> Inputs;
                    TArray<FFunctionExpressionOutput> Outputs;
                    Function->GetInputsAndOutputs(Inputs, Outputs);
                    if (Inputs.Num() != Call->FunctionInputs.Num() || Outputs.Num() != Call->FunctionOutputs.Num())
                    { B.Error = TEXT("Function pin inventory changed: ") + Call->GetPathName(); return false; }
                    for (auto& Pin : Call->FunctionInputs)
                    {
                        const auto* Found = Inputs.FindByPredicate([&](const FFunctionExpressionInput& P)
                            { return P.ExpressionInputId == Pin.ExpressionInputId; });
                        if (!Found || !Found->ExpressionInput || Found->Input.InputName != Pin.Input.InputName)
                        { B.Error = TEXT("Function input GUID/name changed: ") + Call->GetPathName(); return false; }
                        Pin.ExpressionInput = Found->ExpressionInput;
                    }
                    for (auto& Pin : Call->FunctionOutputs)
                    {
                        const auto* Found = Outputs.FindByPredicate([&](const FFunctionExpressionOutput& P)
                            { return P.ExpressionOutputId == Pin.ExpressionOutputId; });
                        if (!Found || !Found->ExpressionOutput || Found->Output.OutputName != Pin.Output.OutputName)
                        { B.Error = TEXT("Function output GUID/name changed: ") + Call->GetPathName(); return false; }
                        Pin.ExpressionOutput = Found->ExpressionOutput;
                    }
                    Pending.AddUnique(Function);
                }
                if (!B.Error.IsEmpty()) return false;
            }
            return true;
        }

        bool Run()
        {
            if (int32(bSlopeOnly) + int32(bWarpOnly) + int32(bSideOnly) + int32(bFields) > 1) { B.Error = TEXT("Select exactly one isolated change"); return false; }
            if (bMagmaFields && !bFields) { B.Error = TEXT("Magma specialization requires orbital fields mode"); return false; }
            UMaterial* Source = LoadObject<UMaterial>(nullptr,
                TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/Shared/M_APS_SharedWorldScapeTerrain.M_APS_SharedWorldScapeTerrain"));
            UMaterialInstanceConstant* Template = LoadObject<UMaterialInstanceConstant>(nullptr,
                bMagmaFields ? TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/Shared/MI_APS_SharedMagma.MI_APS_SharedMagma")
                    : TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/Shared/MI_APS_SharedTerra.MI_APS_SharedTerra"));
            if (!Source || !Template || Template->GetMaterial() != Source)
            { B.Error = TEXT("Current production shared source/template missing"); return false; }
            bool bSourceLava = false; FGuid LavaGuid;
            if (bFields && (!Template->GetStaticSwitchParameterValue(FMaterialParameterInfo(TEXT("lavaPlanet")), bSourceLava, LavaGuid)
                || bSourceLava != bMagmaFields))
            { B.Error = TEXT("Native template lavaPlanet permutation differs from requested specialization"); return false; }
            auto* Master = Cast<UMaterial>(B.Duplicate(Source, TEXT("M_APS_LodPixelTerrain")));
            if (!Master || (!bFields && !((bSlopeOnly || bSideOnly) ? PatchSlopeOnly(Master) : Patch(Master)))) return false;
            if (!bSlopeOnly && !bSideOnly && !bFields && (Bypassed != (bWarpOnly ? 5 : 6) || MasterBypassed != 5 || FunctionBypassed != (bWarpOnly ? 0 : 1)))
            {
                B.Error = FString::Printf(TEXT("Unexpected interpolator bypass count: warpOnly=%d got %d (%d/%d)"),
                    bWarpOnly, Bypassed, MasterBypassed, FunctionBypassed);
                return false;
            }
            auto* Instance = Cast<UMaterialInstanceConstant>(B.Duplicate(Template,
                bMagmaFields ? TEXT("MI_APS_LodPixelMagma") : TEXT("MI_APS_LodPixelTerra")));
            if (bFields && !APSOrbitalColorFieldsAB::Patch(B, Master, Template)) return false;
            if (!Instance) return false;
            Instance->SetParentEditorOnly(Master, false);
            Instance->CopyMaterialUniformParametersEditorOnly(Template, true);
            Instance->PostEditChange();
            bool bCandidateLava = false;
            if (bFields && (!Instance->GetStaticSwitchParameterValue(FMaterialParameterInfo(TEXT("lavaPlanet")), bCandidateLava, LavaGuid)
                || bCandidateLava != bSourceLava))
            { B.Error = TEXT("Candidate lost the native lavaPlanet permutation after reparenting"); return false; }
            if (bFields) UE_LOG(LogTemp, Display, TEXT("[APS.OrbitalFieldsAB] template=%s candidate=%s lavaPlanet=%d preserved=1"),
                *Template->GetPathName(), *Instance->GetPathName(), bCandidateLava);
            Master->PostEditChange();
            if (!RestoreTransientFunctionPins(Master)) return false;
            for (UObject* Output : B.Outputs)
                if (auto* Material = Cast<UMaterialInterface>(Output)) Material->ForceRecompileForRendering(EMaterialShaderPrecompileMode::Default);
            FAssetCompilingManager::Get().FinishAllCompilation();
            if (GShaderCompilingManager) GShaderCompilingManager->FinishAllCompilation();
            for (UObject* Output : B.Outputs)
            {
                if (auto* Material = Cast<UMaterialInterface>(Output))
                {
                    FMaterialResource* Resource = Material->GetMaterialResource(GMaxRHIFeatureLevel);
                    const FMaterialShaderMap* Map = Resource ? Resource->GetGameThreadShaderMap() : nullptr;
                    if (!Resource || !Resource->IsGameThreadShaderMapComplete() || Resource->GetCompileErrors().Num()
                        || !Map || !Map->GetMeshShaderMap(&FLocalVertexFactory::StaticType))
                    {
                        B.Error = TEXT("Candidate shader incomplete, errors or missing LocalVF: ") + Material->GetPathName();
                        if (Resource) for (const FString& Error : Resource->GetCompileErrors()) B.Error += TEXT("\n") + Error;
                        return false;
                    }
                }
            }
            for (UObject* Output : B.Outputs)
            {
                UPackage* Package = Output->GetOutermost();
                Package->MarkPackageDirty();
                FSavePackageArgs Args; Args.TopLevelFlags = RF_Public | RF_Standalone; Args.SaveFlags = SAVE_NoError;
                if (!UPackage::SavePackage(Package, Output, *FPackageName::LongPackageNameToFilename(Package->GetName(), FPackageName::GetAssetPackageExtension()), Args))
                { B.Error = TEXT("Candidate save failed"); return false; }
            }
            if (bSlopeOnly) UE_LOG(LogTemp, Display, TEXT("[APS.LodPixelAB] Ready slope-only candidate: retainedWarpVIs=5 changedSlopeBlend=1 outputs=%d"), B.Outputs.Num());
            UE_LOG(LogTemp, Display, TEXT("[APS.LodPixelAB] Ready candidate only: warpOnly=%d interpolators=%d outputs=%d; not visual acceptance"), bWarpOnly, Bypassed, B.Outputs.Num());
            return true;
        }
    };

    inline bool Build(IAssetTools& Tools)
    {
        FBuilder Builder(Tools);
        const bool Result = Builder.Run();
        if (!Result) UE_LOG(LogTemp, Error, TEXT("[APS.LodPixelAB] Refused: %s"), *Builder.B.Error);
        return Result;
    }
}
#endif
