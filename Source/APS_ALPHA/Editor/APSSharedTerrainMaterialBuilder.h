#pragma once

#if WITH_EDITOR

#include "IAssetTools.h"
#include "AssetCompilingManager.h"
#include "MaterialEditingLibrary.h"
#include "MaterialShared.h"
#include "LocalVertexFactory.h"
#include "ShaderCompiler.h"
#include "Materials/Material.h"
#include "Materials/MaterialFunction.h"
#include "Materials/MaterialExpressionAdd.h"
#include "Materials/MaterialExpressionAbs.h"
#include "Materials/MaterialExpressionDivide.h"
#include "Materials/MaterialExpressionTextureSample.h"
#include "Materials/MaterialExpressionAppendVector.h"
#include "Materials/MaterialExpressionCameraPositionWS.h"
#include "Materials/MaterialExpressionComponentMask.h"
#include "Materials/MaterialExpressionConstant.h"
#include "Materials/MaterialExpressionConstant3Vector.h"
#include "Materials/MaterialExpressionCustom.h"
#include "Materials/MaterialExpressionDoubleVectorParameter.h"
#include "Materials/MaterialExpressionFunctionInput.h"
#include "Materials/MaterialExpressionFunctionOutput.h"
#include "Materials/MaterialExpressionMaterialFunctionCall.h"
#include "Materials/MaterialExpressionMultiply.h"
#include "Materials/MaterialExpressionScalarParameter.h"
#include "Materials/MaterialExpressionSubtract.h"
#include "Materials/MaterialExpressionTransform.h"
#include "Materials/MaterialExpressionVectorParameter.h"
#include "Materials/MaterialExpressionWorldPosition.h"
#include "Materials/MaterialInstanceConstant.h"
#include "Misc/PackageName.h"
#include "UObject/SavePackage.h"
#include "APSSharedTerrainNormalContinuity.h"
#include "APSSharedTerrainColorBounds.h"

/** Protected native-graph adapter, not a second simplified terrain shader.
 * All original texture/layer/normal/distance blending remains in the duplicate.
 * Only spatial inputs are redirected into a single planet-fixed physical frame.
 * Original project and vendor assets are read-only. Outputs must not exist.
 * Unsupported coordinate operations abort before ANY output is saved.
 */
namespace APSSharedTerrainMaterialBuilder
{
    inline constexpr const TCHAR* OutputRoot = TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/Shared");
    inline constexpr const TCHAR* SourceMaster = TEXT("/Game/Ressources/Materials/WorldScapeMaterials/MastersMaterials/MM_WorldscapeMaterial_V2.MM_WorldscapeMaterial_V2");

    struct FBuild
    {
        IAssetTools& AssetTools;
        FString DestinationRoot;
        FString Error;
        TMap<UMaterialFunction*, bool> SpatialCache;
        TSet<UMaterialFunction*> Examining;
        TMap<UMaterialFunction*, UMaterialFunction*> Copies;
        TArray<UObject*> Outputs;
        int32 SpatialInputs = 0;

        explicit FBuild(IAssetTools& InTools, const TCHAR* InDestinationRoot = OutputRoot)
            : AssetTools(InTools), DestinationRoot(InDestinationRoot) {}

        static TArray<UMaterialExpression*> Expressions(UObject* Owner)
        {
            TArray<UMaterialExpression*> Result;
            if (const UMaterial* M = Cast<UMaterial>(Owner))
                for (UMaterialExpression* E : M->GetExpressions()) Result.Add(E);
            else if (const UMaterialFunction* F = Cast<UMaterialFunction>(Owner))
                for (UMaterialExpression* E : F->GetExpressions()) Result.Add(E);
            return Result;
        }

        template<class T> T* Add(UObject* Owner)
        {
            if (UMaterial* M = Cast<UMaterial>(Owner))
                return Cast<T>(UMaterialEditingLibrary::CreateMaterialExpression(M, T::StaticClass()));
            return Cast<T>(UMaterialEditingLibrary::CreateMaterialExpressionInFunction(
                CastChecked<UMaterialFunction>(Owner), T::StaticClass()));
        }

        static bool IsSpatial(UMaterialExpression* E)
        {
            const FString Name = E->GetClass()->GetName();
            return Name.Contains(TEXT("Position")) || Name.Contains(TEXT("NormalWS"))
                || Name.Contains(TEXT("CameraVector")) || Name.Contains(TEXT("Transform"))
                || Name.Contains(TEXT("Depth")) || Name.Contains(TEXT("ObjectRadius"))
                || Name.Contains(TEXT("ObjectOrientation")) || Name.Contains(TEXT("ObjectBounds"))
                || Name.Contains(TEXT("DistanceToNearestSurface"))
                || Name.Contains(TEXT("DistanceFieldGradient"))
                || (Cast<UMaterialExpressionDoubleVectorParameter>(E)
                    && CastChecked<UMaterialExpressionDoubleVectorParameter>(E)->ParameterName == TEXT("PlanetPosistion"));
        }

        bool NeedsCopy(UMaterialFunction* Function)
        {
            if (const bool* Cached = SpatialCache.Find(Function)) return *Cached;
            if (Examining.Contains(Function))
            {
                Error = TEXT("Recursive material function graph is unsupported: ") + Function->GetPathName();
                return true;
            }
            Examining.Add(Function);
            bool bSpatial = false;
            for (UMaterialExpression* E : Expressions(Function))
            {
                if (!IsValid(E)) continue;
                bSpatial |= IsSpatial(E);
                if (UMaterialExpressionMaterialFunctionCall* Call = Cast<UMaterialExpressionMaterialFunctionCall>(E))
                {
                    if (UMaterialFunction* Child = Cast<UMaterialFunction>(Call->MaterialFunction))
                        bSpatial |= NeedsCopy(Child);
                    else if (Call->MaterialFunction)
                        Error = TEXT("Function instance requires explicit audit: ") + Call->MaterialFunction->GetPathName();
                }
                if (UMaterialExpressionCustom* Custom = Cast<UMaterialExpressionCustom>(E))
                {
                    if (Custom->Code.Contains(TEXT("Parameters.")) || Custom->Code.Contains(TEXT("View."))
                        || Custom->Code.Contains(TEXT("WorldPosition")))
                        Error = TEXT("Custom HLSL accesses implicit coordinates: ") + Custom->GetPathName();
                }
            }
            Examining.Remove(Function);
            SpatialCache.Add(Function, bSpatial);
            UE_LOG(LogTemp, Display, TEXT("[APS.SharedTerrain] Function preflight: path=%s expressions=%d loadedCorrectly=%d spatial=%d"),
                *Function->GetPathName(), Expressions(Function).Num(),
                Function->bAllExpressionsLoadedCorrectly ? 1 : 0, bSpatial ? 1 : 0);
            return bSpatial;
        }

        UObject* Duplicate(UObject* Source, const FString& Name)
        {
            const FString PackagePath = DestinationRoot / Name;
            if (FPackageName::DoesPackageExist(PackagePath)
                || FindObject<UObject>(nullptr, *(PackagePath + TEXT(".") + Name)))
            {
                Error = TEXT("Refusing to overwrite existing shared output: ") + PackagePath;
                return nullptr;
            }
            UObject* Result = AssetTools.DuplicateAsset(Name, DestinationRoot, Source);
            if (!Result) Error = TEXT("Could not duplicate ") + Source->GetPathName();
            else
            {
                Outputs.Add(Result);
                UE_LOG(LogTemp, Display, TEXT("[APS.SharedTerrain] New owned duplicate: source=%s output=%s"),
                    *Source->GetPathName(), *Result->GetPathName());
            }
            return Result;
        }

        struct FFrame
        {
            FBuild& B;
            UObject* Owner;
            UMaterialExpression* X = nullptr;
            UMaterialExpression* Y = nullptr;
            UMaterialExpression* Z = nullptr;
            UMaterialExpression* Center = nullptr;
            UMaterialExpression* Scale = nullptr;
            UMaterialExpression* Zero = nullptr;

            FFrame(FBuild& InB, UObject* InOwner) : B(InB), Owner(InOwner)
            {
                auto Axis = [&](const TCHAR* Name, const FVector4& Value)
                {
                    auto* P = B.Add<UMaterialExpressionDoubleVectorParameter>(Owner);
                    P->ParameterName = Name;
                    P->DefaultValue = Value;
                    P->Group = TEXT("APS Physical Coordinate Contract");
                    P->UpdateParameterGuid(true, true);
                    return P;
                };
                X = Axis(TEXT("APS_SharedAxisX"), FVector4(1, 0, 0, 0));
                Y = Axis(TEXT("APS_SharedAxisY"), FVector4(0, 1, 0, 0));
                Z = Axis(TEXT("APS_SharedAxisZ"), FVector4(0, 0, 1, 0));
                auto* C = B.Add<UMaterialExpressionDoubleVectorParameter>(Owner);
                C->ParameterName = TEXT("APS_SharedPlanetCenter");
                C->UpdateParameterGuid(true, true);
                Center = C;
                auto* S = B.Add<UMaterialExpressionDoubleVectorParameter>(Owner);
                S->ParameterName = TEXT("APS_SharedInverseScale");
                S->DefaultValue = FVector4(1.0, 0.0, 0.0, 0.0);
                S->UpdateParameterGuid(true, true);
                Scale = S;
                auto* O = B.Add<UMaterialExpressionConstant3Vector>(Owner);
                O->Constant = FLinearColor(0, 0, 0, 0);
                Zero = O;
            }

            UMaterialExpressionCustom* Rotate(FExpressionInput Value, bool bToWorld)
            {
                auto* E = B.Add<UMaterialExpressionCustom>(Owner);
                E->Description = bToWorld ? TEXT("APS planet vector to rendered world") : TEXT("APS rendered vector to planet frame");
                E->OutputType = CMOT_Float3;
                E->Code = bToWorld ? TEXT("return V.x*X.xyz + V.y*Y.xyz + V.z*Z.xyz;")
                    : TEXT("return float3(dot(V.xyz,X.xyz),dot(V.xyz,Y.xyz),dot(V.xyz,Z.xyz));");
                const auto Input = [&](const TCHAR* Name, FExpressionInput In)
                {
                    FCustomInput Entry;
                    Entry.InputName = Name;
                    Entry.Input = In;
                    E->Inputs.Add(Entry);
                };
                Input(TEXT("V"), Value);
                for (const auto& Pair : {TPair<const TCHAR*, UMaterialExpression*>(TEXT("X"), X),
                    TPair<const TCHAR*, UMaterialExpression*>(TEXT("Y"), Y),
                    TPair<const TCHAR*, UMaterialExpression*>(TEXT("Z"), Z)})
                {
                    FExpressionInput In;
                    In.Expression = Pair.Value;
                    Input(Pair.Key, In);
                }
                return E;
            }

            UMaterialExpression* Rotate(UMaterialExpression* Value, bool bToWorld)
            {
                FExpressionInput In;
                In.Expression = Value;
                return Rotate(In, bToWorld);
            }

            UMaterialExpression* Position(UMaterialExpression* Raw)
            {
                // Keep positions LWC through subtraction, scale and rotation.
                // UE5.4 Custom inputs and even Dot demote to float. A physical
                // 6,750 km planet would lose metre/sub-metre detail if we did so
                // before native texture-size division. Mask/mul/add/append retain
                // the LWC type (FHLSLMaterialTranslator source checked).
                auto* Relative = B.Add<UMaterialExpressionSubtract>(Owner);
                Relative->A.Expression = Raw;
                Relative->B.Expression = Center;
                Relative->B.Mask = 1;
                Relative->B.MaskR = Relative->B.MaskG = Relative->B.MaskB = 1;
                auto* Physical = B.Add<UMaterialExpressionMultiply>(Owner);
                Physical->A.Expression = Relative;
                Physical->B.Expression = Scale;
                Physical->B.Mask = Physical->B.MaskR = 1;
                auto SumAxis = [&](UMaterialExpression* Axis) -> UMaterialExpression*
                {
                    auto* Product = B.Add<UMaterialExpressionMultiply>(Owner);
                    Product->A.Expression = Physical;
                    Product->B.Expression = Axis;
                    Product->B.Mask = 1;
                    Product->B.MaskR = Product->B.MaskG = Product->B.MaskB = 1;
                    UMaterialExpression* Parts[3] = {};
                    for (int32 Index = 0; Index < 3; ++Index)
                    {
                        auto* Part = B.Add<UMaterialExpressionComponentMask>(Owner);
                        Part->Input.Expression = Product;
                        Part->R = Index == 0; Part->G = Index == 1; Part->B = Index == 2; Part->A = false;
                        Parts[Index] = Part;
                    }
                    auto* XY = B.Add<UMaterialExpressionAdd>(Owner);
                    XY->A.Expression = Parts[0]; XY->B.Expression = Parts[1];
                    auto* XYZ = B.Add<UMaterialExpressionAdd>(Owner);
                    XYZ->A.Expression = XY; XYZ->B.Expression = Parts[2];
                    return XYZ;
                };
                auto* XY = B.Add<UMaterialExpressionAppendVector>(Owner);
                XY->A.Expression = SumAxis(X); XY->B.Expression = SumAxis(Y);
                auto* XYZ = B.Add<UMaterialExpressionAppendVector>(Owner);
                XYZ->A.Expression = XY; XYZ->B.Expression = SumAxis(Z);
                return XYZ;
            }
        };


        static bool IsPreciseDetailFunction(const UObject* Owner)
        {
            // Exact terrain-owned object only: liquid/ammonia builders also
            // create a function with this basename in their own output roots.
            return Owner && Owner->GetPathName() ==
                TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/Shared/MF_APS_WorldAlignedTexture_a83aa78c.MF_APS_WorldAlignedTexture_a83aa78c");
        }

        bool PatchDetailCoordinates(UObject* Owner, UMaterialExpression* Center)
        {
            if (!IsPreciseDetailFunction(Owner)) return true;

            UMaterialExpressionFunctionInput* PositionInput = nullptr;
            UMaterialExpressionFunctionInput* SizeInput = nullptr;
            UMaterialExpressionDivide* NativeDivide = nullptr;
            TArray<UMaterialExpressionTextureSample*> Samples;
            for (UMaterialExpression* E : Expressions(Owner))
            {
                if (auto* Input = Cast<UMaterialExpressionFunctionInput>(E))
                {
                    if (Input->InputName == TEXT("WorldPosition")) PositionInput = Input;
                    if (Input->InputName == TEXT("TextureSize")) SizeInput = Input;
                }
                if (auto* Sample = Cast<UMaterialExpressionTextureSample>(E)) Samples.Add(Sample);
            }
            for (UMaterialExpression* E : Expressions(Owner))
                if (auto* Divide = Cast<UMaterialExpressionDivide>(E);
                    Divide && Divide->A.Expression == PositionInput) NativeDivide = Divide;

            auto* SignedSize = NativeDivide
                ? Cast<UMaterialExpressionMultiply>(NativeDivide->B.Expression) : nullptr;
            auto* AbsoluteSize = SignedSize
                ? Cast<UMaterialExpressionAbs>(SignedSize->A.Expression) : nullptr;
            if (!PositionInput || !PositionInput->bUsePreviewValueAsDefault
                || !SizeInput || !NativeDivide || !SignedSize || !AbsoluteSize
                || AbsoluteSize->Input.Expression != SizeInput
                || SignedSize->B.Expression || SignedSize->ConstB != -1.0f
                || Samples.Num() != 3)
            {
                Error = TEXT("Unexpected native WAT coordinate contract: ") + Owner->GetPathName();
                return false;
            }

            uint32 ProjectionSet = 0;
            for (UMaterialExpressionTextureSample* Sample : Samples)
            {
                auto* Mask = Cast<UMaterialExpressionComponentMask>(Sample->Coordinates.Expression);
                if (!Mask || Mask->Input.Expression != NativeDivide || Mask->A
                    || (int32(Mask->R) + int32(Mask->G) + int32(Mask->B)) != 2
                    || Sample->SamplerSource != SSM_Wrap_WorldGroupSettings
                    || Sample->MipValueMode != TMVM_None || Sample->MipValue.Expression
                    || Sample->CoordinatesDX.Expression || Sample->CoordinatesDY.Expression)
                {
                    Error = TEXT("Unexpected native WAT sample contract: ") + Sample->GetPathName();
                    return false;
                }
                ProjectionSet |= 1u << ((Mask->R ? 1u : 0u)
                    | (Mask->G ? 2u : 0u) | (Mask->B ? 4u : 0u));
            }
            if (ProjectionSet != ((1u << 3) | (1u << 5) | (1u << 6)))
            {
                Error = TEXT("Native WAT must contain the XY/XZ/YZ projections.");
                return false;
            }

            auto* Raw = Add<UMaterialExpressionWorldPosition>(Owner);
            Raw->WorldPositionShaderOffset = WPT_ExcludeAllShaderOffsets;
            auto* UV = Add<UMaterialExpressionCustom>(Owner);
            UV->Description = TEXT("APS compensated planet-fixed detail UV, bounded detiling and unwrapped gradients");
            UV->OutputType = CMOT_Float3;
            UV->Inputs.Empty();
            const auto Input = [UV](const TCHAR* Name, UMaterialExpression* E, bool bRGB = false)
            {
                FCustomInput Entry;
                Entry.InputName = Name;
                Entry.Input.Expression = E;
                if (bRGB)
                {
                    Entry.Input.Mask = 1;
                    Entry.Input.MaskR = Entry.Input.MaskG = Entry.Input.MaskB = 1;
                }
                UV->Inputs.Add(Entry);
            };
            Input(TEXT("Raw"), Raw);
            Input(TEXT("Center"), Center, true);
            Input(TEXT("Size"), SignedSize);
            const auto Row = [&](const TCHAR* ParameterName, const FLinearColor& Default)
            {
                auto* Parameter = Add<UMaterialExpressionVectorParameter>(Owner);
                Parameter->ParameterName = ParameterName;
                Parameter->DefaultValue = Default;
                Parameter->Group = TEXT("APS Physical Coordinate Contract");
                Parameter->UpdateParameterGuid(true, true);
                return Parameter;
            };
            Input(TEXT("XHigh"), Row(TEXT("APS_SharedDetailRowXHigh"), FLinearColor(1, 0, 0, 0)), true);
            Input(TEXT("XLow"), Row(TEXT("APS_SharedDetailRowXLow"), FLinearColor(0, 0, 0, 0)), true);
            Input(TEXT("YHigh"), Row(TEXT("APS_SharedDetailRowYHigh"), FLinearColor(0, 1, 0, 0)), true);
            Input(TEXT("YLow"), Row(TEXT("APS_SharedDetailRowYLow"), FLinearColor(0, 0, 0, 0)), true);
            Input(TEXT("ZHigh"), Row(TEXT("APS_SharedDetailRowZHigh"), FLinearColor(0, 0, 1, 0)), true);
            Input(TEXT("ZLow"), Row(TEXT("APS_SharedDetailRowZLow"), FLinearColor(0, 0, 0, 0)), true);
            auto* WarpStrength = Add<UMaterialExpressionScalarParameter>(Owner);
            WarpStrength->ParameterName = TEXT("APS_SharedDetailWarpStrength");
            WarpStrength->DefaultValue = 1.0f;
            WarpStrength->Group = TEXT("APS Detail Sampling");
            WarpStrength->UpdateParameterGuid(true, true);
            Input(TEXT("WarpStrength"), WarpStrength);
            UV->Outputs.Reset(3);
            UV->Outputs.Add(FExpressionOutput(TEXT("return")));
            UV->bShowOutputNameOnPin = true;
            for (const TCHAR* Name : {TEXT("UVdx"), TEXT("UVdy")})
            {
                FCustomOutput Output;
                Output.OutputName = Name;
                Output.OutputType = CMOT_Float3;
                UV->AdditionalOutputs.Add(Output);
                UV->Outputs.Add(FExpressionOutput(Name));
            }
            // UE5.4 exposes each LWC Custom input as both a demoted ordinary
            // argument and an additional LWC<Name> argument. Read the latter.
            // Compensated math retains the low component through the complete
            // planet transform and division, not just the final texture wrap.
            // Explicit gradients are computed BEFORE periodic reduction: a
            // wrap seam must not look like a huge footprint/coarse mip.
            // Both displacement bands operate in physical centimetres before
            // TextureSize division, so colour/normal/roughness see one domain
            // even when their authored texture sizes differ. No projection or
            // sampled normal axes are rotated. The existing normal convention
            // is retained with a bounded local shear (Jacobian perturbation
            // below 0.14 at strength 1, no folding). Maximum shift is 10.24 m
            // per coordinate: local repeat suppression, not unique orbit detail.
            // Phase reduction stays compensated; sine joins across its wrap.
            // ddx/ddy of the warped UNWRAPPED result include the deformation's
            // footprint, while all three native samples and samplers survive.
            UV->Code =
                TEXT("FDFVector3 Relative = DFSubtract(WSToDF(LWCRaw), WSToDF(LWCCenter));\n")
                TEXT("FDFVector3 PX = DFMultiply(Relative, MakeDFVector3(XHigh, XLow));\n")
                TEXT("FDFVector3 PY = DFMultiply(Relative, MakeDFVector3(YHigh, YLow));\n")
                TEXT("FDFVector3 PZ = DFMultiply(Relative, MakeDFVector3(ZHigh, ZLow));\n")
                TEXT("FDFScalar X = DFAdd(DFAdd(DFGetX(PX), DFGetY(PX)), DFGetZ(PX));\n")
                TEXT("FDFScalar Y = DFAdd(DFAdd(DFGetX(PY), DFGetY(PY)), DFGetZ(PY));\n")
                TEXT("FDFScalar Z = DFAdd(DFAdd(DFGetX(PZ), DFGetY(PZ)), DFGetZ(PZ));\n")
                TEXT("FDFVector3 Physical = MakeDFVector(X, Y, Z);\n")
                TEXT("float Strength = saturate(WarpStrength);\n")
                TEXT("if (Strength > 0.0f)\n")
                TEXT("{\n")
                TEXT("    float3 PhaseA = DFFracDemote(DFDivide(Physical, float3(17389.0f, 21977.0f, 28793.0f)));\n")
                TEXT("    float3 PhaseB = DFFracDemote(DFDivide(Physical, float3(103231.0f, 131071.0f, 163819.0f)));\n")
                TEXT("    float3 OffsetA = sin((PhaseA.yzx + float3(0.173f, 0.419f, 0.731f)) * 6.28318530718f) * 256.0f;\n")
                TEXT("    float3 OffsetB = sin((PhaseB.zxy + float3(0.613f, 0.287f, 0.947f)) * 6.28318530718f) * 768.0f;\n")
                TEXT("    Physical = DFAdd(Physical, (OffsetA + OffsetB) * Strength);\n")
                TEXT("}\n")
                TEXT("FDFVector3 Unwrapped = DFDivide(Physical, Size);\n")
                TEXT("UVdx = DFDdxDemote(Unwrapped);\n")
                TEXT("UVdy = DFDdyDemote(Unwrapped);\n")
                TEXT("return DFFracDemote(Unwrapped);\n");

            for (UMaterialExpressionTextureSample* Sample : Samples)
            {
                auto* Mask = CastChecked<UMaterialExpressionComponentMask>(Sample->Coordinates.Expression);
                const auto Projection = [&](int32 OutputIndex)
                {
                    auto* Result = Add<UMaterialExpressionComponentMask>(Owner);
                    Result->Input.Expression = UV;
                    Result->Input.OutputIndex = OutputIndex;
                    Result->R = Mask->R; Result->G = Mask->G; Result->B = Mask->B; Result->A = false;
                    return Result;
                };
                Sample->Coordinates = FExpressionInput();
                Sample->Coordinates.Expression = Projection(0);
                Sample->CoordinatesDX.Expression = Projection(1);
                Sample->CoordinatesDY.Expression = Projection(2);
                Sample->MipValueMode = TMVM_Derivative;
            }
            UE_LOG(LogTemp, Display, TEXT("[APS.SharedTerrain] Compensated WAT detail coordinates: bounded physical-domain detiling; three native projections; warped unwrapped gradients; shared wrap samplers retained"));
            return true;
        }

        bool ReconnectFunctionById(UMaterialExpressionMaterialFunctionCall* Call,
            UMaterialFunction* Source, UMaterialFunction* Copy)
        {
            TArray<FFunctionExpressionInput> SourceInputs, CopyInputs;
            TArray<FFunctionExpressionOutput> SourceOutputs, CopyOutputs;
            Source->GetInputsAndOutputs(SourceInputs, SourceOutputs);
            Copy->GetInputsAndOutputs(CopyInputs, CopyOutputs);
            if (SourceInputs.Num() != CopyInputs.Num() || SourceOutputs.Num() != CopyOutputs.Num())
            {
                Error = TEXT("Shared function changed pin counts: ") + Source->GetPathName();
                return false;
            }
            for (const FFunctionExpressionInput& SourceInput : SourceInputs)
            {
                const FFunctionExpressionInput* CopyInput = CopyInputs.FindByPredicate(
                    [&](const FFunctionExpressionInput& Pin) { return Pin.ExpressionInputId == SourceInput.ExpressionInputId; });
                if (!CopyInput || !SourceInput.ExpressionInput || !CopyInput->ExpressionInput
                    || SourceInput.ExpressionInput->InputName != CopyInput->ExpressionInput->InputName
                    || SourceInput.ExpressionInput->InputType != CopyInput->ExpressionInput->InputType)
                {
                    Error = TEXT("Shared function changed input GUID/name/type: ") + Source->GetPathName();
                    return false;
                }
            }
            // The precise default implements the existing planet-frame mapping.
            // Refuse an authored position override instead of silently ignoring it.
            if (IsPreciseDetailFunction(Copy))
            {
                for (const FFunctionExpressionInput& Pin : SourceInputs)
                {
                    if (!Pin.ExpressionInput || Pin.ExpressionInput->InputName != TEXT("WorldPosition")) continue;
                    const FFunctionExpressionInput* Binding = Call->FunctionInputs.FindByPredicate(
                        [&](const FFunctionExpressionInput& Input) { return Input.ExpressionInputId == Pin.ExpressionInputId; });
                    if (!Binding || Binding->Input.Expression)
                    {
                        Error = TEXT("Precise shared WAT requires its audited default WorldPosition: ") + Call->GetPathName();
                        return false;
                    }
                }
            }
            for (const FFunctionExpressionOutput& SourceOutput : SourceOutputs)
            {
                const FFunctionExpressionOutput* CopyOutput = CopyOutputs.FindByPredicate(
                    [&](const FFunctionExpressionOutput& Pin) { return Pin.ExpressionOutputId == SourceOutput.ExpressionOutputId; });
                if (!CopyOutput || !SourceOutput.ExpressionOutput || !CopyOutput->ExpressionOutput
                    || SourceOutput.ExpressionOutput->OutputName != CopyOutput->ExpressionOutput->OutputName)
                {
                    Error = TEXT("Shared function changed output GUID/name: ") + Source->GetPathName();
                    return false;
                }
            }

            const TArray<FFunctionExpressionInput> PreviousInputs = Call->FunctionInputs;
            const TArray<FFunctionExpressionOutput> PreviousOutputs = Call->FunctionOutputs;
            // DuplicateAsset does not preserve transient ExpressionInput/Output
            // pointers. SetMaterialFunctionEx matches inputs by names obtained
            // through those pointers and can silently discard their connections.
            // Function Input/Output PostDuplicate preserves serialized pin GUIDs.
            // This path rehydrates the pointers and rewires by those stable IDs.
            Call->MaterialFunction = Copy;
            Call->UpdateFromFunctionResource(false);
            if (PreviousInputs.Num() != Call->FunctionInputs.Num()
                || PreviousOutputs.Num() != Call->FunctionOutputs.Num())
            {
                Error = TEXT("Shared function call changed pin counts: ") + Call->GetPathName();
                return false;
            }
            int32 ConnectedInputs = 0;
            for (const FFunctionExpressionInput& Previous : PreviousInputs)
            {
                const FFunctionExpressionInput* Current = Call->FunctionInputs.FindByPredicate(
                    [&](const FFunctionExpressionInput& Pin) { return Pin.ExpressionInputId == Previous.ExpressionInputId; });
                if (!Current || !Current->ExpressionInput
                    || Current->Input.Expression != Previous.Input.Expression
                    || Current->Input.OutputIndex != Previous.Input.OutputIndex
                    || Current->Input.Mask != Previous.Input.Mask
                    || Current->Input.MaskR != Previous.Input.MaskR || Current->Input.MaskG != Previous.Input.MaskG
                    || Current->Input.MaskB != Previous.Input.MaskB || Current->Input.MaskA != Previous.Input.MaskA)
                {
                    Error = TEXT("Shared function call lost an input connection: ") + Call->GetPathName();
                    return false;
                }
                ConnectedInputs += Previous.Input.Expression ? 1 : 0;
            }
            for (int32 Index = 0; Index < PreviousOutputs.Num(); ++Index)
            {
                // Identical pin order also proves all pre-existing consumer
                // OutputIndex values keep their meaning, including nested calls.
                if (Call->FunctionOutputs[Index].ExpressionOutputId != PreviousOutputs[Index].ExpressionOutputId
                    || !Call->FunctionOutputs[Index].ExpressionOutput)
                {
                    Error = TEXT("Shared function call changed output ordering: ") + Call->GetPathName();
                    return false;
                }
            }
            UE_LOG(LogTemp, Display, TEXT("[APS.SharedTerrain] Function binding verified: call=%s inputs=%d connected=%d outputs=%d (stable GUIDs)"),
                *Call->GetPathName(), PreviousInputs.Num(), ConnectedInputs, PreviousOutputs.Num());
            return true;
        }

        UMaterialFunction* CopyFunction(UMaterialFunction* Source)
        {
            if (!NeedsCopy(Source)) return Source;
            if (UMaterialFunction** Existing = Copies.Find(Source)) return *Existing;
            if (!Error.IsEmpty()) return nullptr;
            const FString Name = FString::Printf(TEXT("MF_APS_%s_%08x"),
                *Source->GetName(), FCrc::StrCrc32(*Source->GetPathName()));
            UMaterialFunction* Copy = Cast<UMaterialFunction>(Duplicate(Source, Name));
            if (!Copy) return nullptr;
            Copies.Add(Source, Copy);
            if (!Patch(Copy)) return nullptr;
            // Preserve function input/output IDs so parent bindings stay intact.
            UMaterialEditingLibrary::UpdateMaterialFunction(Copy);
            return Copy;
        }

        bool Patch(UObject* Owner)
        {
            const TArray<UMaterialExpression*> Original = Expressions(Owner);
            if (Original.IsEmpty())
            {
                Error = TEXT("Refusing empty source expression graph: ") + Owner->GetPathName();
                return false;
            }
            const int32 PreviousSpatialInputs = SpatialInputs;
            FFrame Frame(*this, Owner);
            TMap<UMaterialExpression*, FExpressionInput> Replacements;
            const auto Replace = [&](UMaterialExpression* From, UMaterialExpression* To)
            {
                FExpressionInput Input;
                Input.Expression = To;
                Replacements.Add(From, Input);
                ++SpatialInputs;
            };
            for (UMaterialExpression* E : Original)
            {
                if (!IsValid(E)) continue;
                const FString Class = E->GetClass()->GetName();
                if (auto* Call = Cast<UMaterialExpressionMaterialFunctionCall>(E))
                {
                    if (UMaterialFunction* Function = Cast<UMaterialFunction>(Call->MaterialFunction))
                    {
                        UMaterialFunction* Copy = CopyFunction(Function);
                        if (!Copy) return false;
                        if (Copy != Function && !ReconnectFunctionById(Call, Function, Copy)) return false;
                    }
                    continue;
                }
                if (Class == TEXT("MaterialExpressionWorldPosition"))
                {
                    auto* Raw = Add<UMaterialExpressionWorldPosition>(Owner);
                    Raw->WorldPositionShaderOffset = WPT_ExcludeAllShaderOffsets;
                    Replace(E, Frame.Position(Raw));
                }
                else if (Class == TEXT("MaterialExpressionCameraPositionWS"))
                    Replace(E, Frame.Position(Add<UMaterialExpressionCameraPositionWS>(Owner)));
                else if (Class == TEXT("MaterialExpressionActorPositionWS")
                    || Class == TEXT("MaterialExpressionObjectPositionWS"))
                    Replace(E, Frame.Zero);
                else if (auto* P = Cast<UMaterialExpressionDoubleVectorParameter>(E))
                {
                    if (P->ParameterName == TEXT("PlanetPosistion")) Replace(E, Frame.Zero);
                }
                else if (Class == TEXT("MaterialExpressionVertexNormalWS")
                    || Class == TEXT("MaterialExpressionPixelNormalWS")
                    || Class == TEXT("MaterialExpressionCameraVectorWS"))
                {
                    UMaterialExpression* Raw = Cast<UMaterial>(Owner)
                        ? UMaterialEditingLibrary::CreateMaterialExpression(CastChecked<UMaterial>(Owner), E->GetClass())
                        : UMaterialEditingLibrary::CreateMaterialExpressionInFunction(CastChecked<UMaterialFunction>(Owner), E->GetClass());
                    Replace(E, Frame.Rotate(Raw, false));
                }
                else if (Class == TEXT("MaterialExpressionPixelDepth"))
                {
                    UMaterialExpression* Raw = Cast<UMaterial>(Owner)
                        ? UMaterialEditingLibrary::CreateMaterialExpression(CastChecked<UMaterial>(Owner), E->GetClass())
                        : UMaterialEditingLibrary::CreateMaterialExpressionInFunction(CastChecked<UMaterialFunction>(Owner), E->GetClass());
                    auto* PhysicalDepth = Add<UMaterialExpressionMultiply>(Owner);
                    PhysicalDepth->A.Expression = Raw;
                    PhysicalDepth->B.Expression = Frame.Scale;
                    PhysicalDepth->B.Mask = PhysicalDepth->B.MaskR = 1;
                    Replace(E, PhysicalDepth);
                }
                else if (auto* Transform = Cast<UMaterialExpressionTransform>(E))
                {
                    const auto Source = Transform->TransformSourceType;
                    const auto Target = Transform->TransformType;
                    const bool bSourcePlanet = Source == TRANSFORMSOURCE_World || Source == TRANSFORMSOURCE_Local;
                    const bool bTargetPlanet = Target == TRANSFORM_World || Target == TRANSFORM_Local;
                    if (bSourcePlanet && bTargetPlanet)
                    {
                        // Virtual world and object-local both mean the planet frame.
                        Replacements.Add(E, Transform->Input);
                        ++SpatialInputs;
                    }
                    else if (bSourcePlanet && Target == TRANSFORM_Tangent)
                    {
                        Transform->TransformSourceType = TRANSFORMSOURCE_World;
                        Transform->Input.Expression = Frame.Rotate(Transform->Input, true);
                        Transform->Input.OutputIndex = 0;
                        Transform->Input.Mask = 0;
                    }
                    else if (Source == TRANSFORMSOURCE_Tangent && bTargetPlanet)
                    {
                        auto* Raw = Add<UMaterialExpressionTransform>(Owner);
                        Raw->Input = Transform->Input;
                        Raw->TransformSourceType = TRANSFORMSOURCE_Tangent;
                        Raw->TransformType = TRANSFORM_World;
                        Replace(E, Frame.Rotate(Raw, false));
                    }
                    else
                    {
                        Error = TEXT("Unsupported vector coordinate transform: ") + E->GetPathName();
                        return false;
                    }
                }
                else if (IsSpatial(E))
                {
                    Error = TEXT("Unadapted spatial input: ") + E->GetPathName();
                    return false;
                }
            }

            // Raw inputs created above are fresh nodes, never replacement keys.
            // Rewire all consumers, including copied transform inputs; their
            // references can still point at an original spatial expression.
            const auto Rewire = [&](FExpressionInput& Input)
            {
                TSet<UMaterialExpression*> Seen;
                while (const FExpressionInput* NewInput = Replacements.Find(Input.Expression))
                {
                    if (Seen.Contains(Input.Expression))
                    {
                        Error = TEXT("Cyclic spatial replacement: ") + Input.Expression->GetPathName();
                        break;
                    }
                    Seen.Add(Input.Expression);
                    const auto OldMask = Input.Mask;
                    const auto R = Input.MaskR, G = Input.MaskG, B = Input.MaskB, A = Input.MaskA;
                    Input = *NewInput;
                    if (OldMask) { Input.Mask = OldMask; Input.MaskR = R; Input.MaskG = G; Input.MaskB = B; Input.MaskA = A; }
                }
            };
            for (UMaterialExpression* E : Expressions(Owner))
                if (IsValid(E)) for (FExpressionInput* Input : E->GetInputsView()) if (Input) Rewire(*Input);
            if (UMaterial* Material = Cast<UMaterial>(Owner))
            {
                for (int32 Property = 0; Property < MP_MAX; ++Property)
                    if (FExpressionInput* Input = Material->GetExpressionInputForProperty(static_cast<EMaterialProperty>(Property)))
                        Rewire(*Input);
                FExpressionInput* Normal = Material->GetExpressionInputForProperty(MP_Normal);
                if (Material->bTangentSpaceNormal && Normal && Normal->Expression)
                {
                    Error = TEXT("Expected saved native world-space normal contract.");
                    return false;
                }
                if (Normal && Normal->Expression)
                {
                    UMaterialExpression* WorldNormal = Frame.Rotate(*Normal, true);
                    *Normal = FExpressionInput();
                    Normal->Expression = WorldNormal;
                }
            }
            if (!PatchDetailCoordinates(Owner, Frame.Center)) return false;
            UE_LOG(LogTemp, Display, TEXT("[APS.SharedTerrain] Patched graph: path=%s expressionsBefore=%d expressionsAfter=%d spatialInputs=%d"),
                *Owner->GetPathName(), Original.Num(), Expressions(Owner).Num(), SpatialInputs - PreviousSpatialInputs);
            return Error.IsEmpty();
        }

        bool Run()
        {
            UMaterial* Native = LoadObject<UMaterial>(nullptr, SourceMaster);
            if (!Native) { Error = TEXT("Project native master missing; do not substitute plugin master."); return false; }
            UE_LOG(LogTemp, Display, TEXT("[APS.SharedTerrain] Native preflight: path=%s expressions=%d featureLevel=%d tangentSpaceNormal=%d"),
                *Native->GetPathName(), Expressions(Native).Num(), static_cast<int32>(GMaxRHIFeatureLevel),
                Native->bTangentSpaceNormal ? 1 : 0);
            for (EMaterialProperty Property : {MP_WorldPositionOffset, MP_PixelDepthOffset, MP_MaterialAttributes})
                if (const FExpressionInput* Input = Native->GetExpressionInputForProperty(Property); Input && Input->Expression)
                { Error = TEXT("Native displacement/material-attributes contract needs explicit audit."); return false; }
            UMaterial* Master = Cast<UMaterial>(Duplicate(Native, TEXT("M_APS_SharedWorldScapeTerrain")));
            if (!Master || !Patch(Master)) return false;
            // The authored native OpacityMask is not a cave/hole channel: it is
            // lerp(ceil(VertexColor.R - heightThreshold), 1, cameraSphere10km).
            // Carrying that optimization into a complete generated globe cuts
            // low ground out at a camera-centred boundary, even on dry/ice
            // planets. Keep the entire solid terrain. The actual ocean/solid
            // secondary surface occludes submerged terrain through depth.
            // Only this new shared master changes; native palettes, height,
            // normal/distance bands and the authored source remain untouched.
            FExpressionInput* CoverageInput = Master->GetExpressionInputForProperty(MP_OpacityMask);
            if (!CoverageInput)
            {
                Error = TEXT("Shared master has no surface coverage input.");
                return false;
            }
            auto* CompleteSurfaceCoverage = Add<UMaterialExpressionConstant>(Master);
            CompleteSurfaceCoverage->R = 1.0f;
            CompleteSurfaceCoverage->Desc = TEXT("APS generated solid surface: no camera-centred height cutout");
            *CoverageInput = FExpressionInput();
            CoverageInput->Expression = CompleteSurfaceCoverage;
            UE_LOG(LogTemp, Display, TEXT("[APS.SharedTerrain] Complete generated solid coverage: OpacityMask=1; native camera-centred 10km height cutout removed from owned master only"));
            // Future new bakes must reproduce the protected shared-asset update.
            // Original authored functions and all five warp VIs stay untouched.
            APSSharedTerrainNormalContinuity::TTransform<FBuild> NormalContinuity(*this);
            if (!NormalContinuity.Apply(Master)) return false;
            bool TopColorChanged = false;
            if (!APSSharedTerrainColorBounds::Apply(*this, Master, TopColorChanged)) return false;
            for (const auto& Pair : {TPair<const TCHAR*, const TCHAR*>(TEXT("MI_Terra"), TEXT("MI_APS_SharedTerra")),
                TPair<const TCHAR*, const TCHAR*>(TEXT("MI_Magma"), TEXT("MI_APS_SharedMagma"))})
            {
                const FString Path = FString(TEXT("/Game/Ressources/Materials/WorldScapeMaterials/MaterialInstances/"))
                    + Pair.Key + TEXT(".") + Pair.Key;
                UMaterialInstanceConstant* Source = LoadObject<UMaterialInstanceConstant>(nullptr, *Path);
                if (!Source || Source->GetMaterial() != Native)
                { Error = TEXT("Unexpected native template parent: ") + Path; return false; }
                UMaterialInstanceConstant* Instance = Cast<UMaterialInstanceConstant>(Duplicate(Source, Pair.Value));
                if (!Instance) return false;
                Instance->SetParentEditorOnly(Master, false);
                // Preserve inherited as well as explicit texture/uniform/static
                // overrides if a future native template adds an intermediate MI.
                Instance->CopyMaterialUniformParametersEditorOnly(Source, true);
                Instance->PostEditChange();
            }
            Master->PostEditChange();
            UE_LOG(LogTemp, Display, TEXT("[APS.SharedTerrain] Graph adaptation finished: outputs=%d clonedFunctions=%d inspectedFunctions=%d spatialInputs=%d masterExpressions=%d"),
                Outputs.Num(), Copies.Num(), SpatialCache.Num(), SpatialInputs, Expressions(Master).Num());
            // UE 5.4 PostEditChange invalidates old jobs, then caches the new
            // graph with EMaterialShaderPrecompileMode::None. Finishing the
            // queue cannot submit the missing permutations. Explicitly request
            // full rendering compilation AFTER every graph/parent edit, for the
            // master and each template's own static permutation. Keep the
            // complete-map/zero-error gate below; an empty map is not success.
            for (UObject* Output : Outputs)
            {
                if (UMaterialInterface* Material = Cast<UMaterialInterface>(Output))
                {
                    UE_LOG(LogTemp, Display, TEXT("[APS.SharedTerrain] Submit complete rendering shader map: %s"),
                        *Material->GetPathName());
                    Material->ForceRecompileForRendering(EMaterialShaderPrecompileMode::Default);
                }
            }
            FAssetCompilingManager::Get().FinishAllCompilation();
            if (GShaderCompilingManager) GShaderCompilingManager->FinishAllCompilation();
            for (UObject* Output : Outputs)
            {
                UMaterialInterface* Material = Cast<UMaterialInterface>(Output);
                if (!Material) continue;
                FMaterialResource* Resource = Material->GetMaterialResource(GMaxRHIFeatureLevel);
                UE_LOG(LogTemp, Display, TEXT("[APS.SharedTerrain] Shader readiness: path=%s resource=%d requestedFeatureLevel=%d qualityLevel=%d hasMap=%d complete=%d compileErrors=%d remainingJobs=%d"),
                    *Material->GetPathName(), Resource ? 1 : 0, static_cast<int32>(GMaxRHIFeatureLevel),
                    Resource ? static_cast<int32>(Resource->GetQualityLevel()) : -1,
                    Resource && Resource->GetGameThreadShaderMap() ? 1 : 0,
                    Resource && Resource->IsGameThreadShaderMapComplete() ? 1 : 0,
                    Resource ? Resource->GetCompileErrors().Num() : -1,
                    GShaderCompilingManager ? GShaderCompilingManager->GetNumRemainingJobs() : -1);
                if (!Resource || !Resource->IsGameThreadShaderMapComplete()
                    || Resource->GetCompileErrors().Num() != 0
                    || !Resource->GetGameThreadShaderMap()
                    || !Resource->GetGameThreadShaderMap()->GetMeshShaderMap(&FLocalVertexFactory::StaticType))
                {
                    Error = TEXT("Shared native shader is incomplete: ") + Output->GetPathName();
                    if (Resource) for (const FString& CompileError : Resource->GetCompileErrors())
                        Error += TEXT("\n") + CompileError;
                    return false;
                }
            }
            // Saving is deliberately last. A malformed/unhandled graph cannot
            // replace a working generated surface or touch an authored material.
            for (UObject* Output : Outputs)
            {
                UPackage* Package = Output->GetOutermost();
                Package->MarkPackageDirty();
                const FString Filename = FPackageName::LongPackageNameToFilename(
                    Package->GetName(), FPackageName::GetAssetPackageExtension());
                FSavePackageArgs Args;
                Args.TopLevelFlags = RF_Public | RF_Standalone;
                Args.SaveFlags = SAVE_NoError;
                if (!UPackage::SavePackage(Package, Output, *Filename, Args))
                { Error = TEXT("Failed to save new shared output: ") + Filename; return false; }
            }
            UE_LOG(LogTemp, Display, TEXT("[APS.SharedTerrain] Native shared graph v1: outputs=%d spatialInputs=%d master=%s"),
                Outputs.Num(), SpatialInputs, *Master->GetPathName());
            return true;
        }
    };

    inline bool Build(IAssetTools& AssetTools)
    {
        FBuild Builder(AssetTools);
        const bool bResult = Builder.Run();
        if (!bResult) UE_LOG(LogTemp, Error, TEXT("[APS.SharedTerrain] Refused: %s"), *Builder.Error);
        return bResult;
    }
}
#endif
