#pragma once
#if WITH_EDITOR
#include "APSLavaFinePeriodUpdate.h"
#include "APSSharedTerrainNormalContinuity.h"
#include "UObject/UObjectHash.h"

// Offline candidate only. Clone the precise WAT for Call14; leave Call16, the
// accepted source function and the 20km branch untouched. Three taps/projection
// (nine total), using original unwrapped gradients and native RGBA filtering.
namespace APSLavaFineStochasticUpdate
{
using FCore = APSSharedTerrainMaterialBuilder::FBuild;
using APSLavaAntiGridUpdate::Filename;
using APSLavaAntiGridUpdate::Hash;
inline constexpr const TCHAR* PrivateName = TEXT("MF_APS_LavaFineStochastic_v1");
inline bool SynchronizeEditorCollections(UMaterialFunction* Function)
{
    // UE5.4 DuplicateAsset can retain the source-named EditorOnlyData alongside
    // the constructor-created destination-named object. Their Serialize/PostLoad
    // hooks can switch the active pointer or copy the stale array over the new
    // one. Keep every direct editor-data copy consistent; do not delete objects.
    if (!Function || Function->GetName() != PrivateName) return false;
    const FMaterialExpressionCollection Collection = Function->GetExpressionCollection();
    TArray<UObject*> Owned; GetObjectsWithOuter(Function, Owned, false);
    int32 Copies = 0;
    for (auto* O : Owned)
        if (auto* Data = Cast<UMaterialFunctionEditorOnlyData>(O))
        {
            if (Data->IsTemplate() || Data->GetOuter() != Function) return false;
            UE_LOG(LogTemp, Display, TEXT("[APS.LavaFineStorage] synchronize=%s before=%d after=%d"),
                *Data->GetPathName(), Data->ExpressionCollection.Expressions.Num(), Collection.Expressions.Num());
            Data->ExpressionCollection = Collection; ++Copies;
        }
    return Copies > 0 && Copies <= 4;
}
inline bool InspectReload(IAssetTools& Tools)
{
    FCore B(Tools, APSSharedLavaMaterialBuilder::Destination);
    const FString Root(APSSharedLavaMaterialBuilder::Destination);
    auto* Master = LoadObject<UMaterial>(nullptr, *(Root / TEXT("M_APS_SharedLava.M_APS_SharedLava")));
    auto* Native = LoadObject<UMaterialInstanceConstant>(nullptr, *(Root / TEXT("MI_APS_SharedLavaNative.MI_APS_SharedLavaNative")));
    auto* APS = LoadObject<UMaterialInstanceConstant>(nullptr, *(Root / TEXT("MI_APS_SharedLava.MI_APS_SharedLava")));
    if (!IsRunningCommandlet() || !Master || !Native || !APS) return false;
    auto* Fine = FindObject<UMaterialExpressionMaterialFunctionCall>(Master, TEXT("MaterialExpressionMaterialFunctionCall_14"));
    auto* Function = Fine ? Cast<UMaterialFunction>(Fine->MaterialFunction) : nullptr;
    if (!Function || Function->GetName() != PrivateName) return false;
    const bool bStorageRepair = FParse::Param(FCommandLine::Get(), TEXT("RepairSharedLavaFineRegistrationV2"));
    const bool bRepair = bStorageRepair || FParse::Param(FCommandLine::Get(), TEXT("RepairSharedLavaFineRegistration"));
    TArray<APSLavaFinePeriodUpdate::FExpected> Protected;
    Protected.Append(APSLavaFinePeriodUpdate::Expected, UE_ARRAY_COUNT(APSLavaFinePeriodUpdate::Expected));
    Protected[0].SHA1 = bStorageRepair ? TEXT("AAB8DBCD215B92B7B2A06B1AC019965834D86CEB") : TEXT("5E5923B93A4AE2DEC2F8E1534C3AC096B5C73F9A");
    if (bRepair)
    {
        for (const auto& E : Protected) if (Hash(Filename(E.Name)) != E.SHA1) return false;
        if (Hash(Filename(PrivateName)) != (bStorageRepair ? TEXT("4E52BAAC57082E1556B7E903562B048CC2A640F4") : TEXT("E48668205FAD693B214DC0F79BA27BD87F39CC55"))) return false;
    }
    APSSharedTerrainNormalContinuity::TTransform<FCore> Reader(B);
    const auto Graph = Reader.Graph(Function);
    UE_LOG(LogTemp, Display, TEXT("[APS.LavaFineReload] function=%s flatNodes=%d graphNodes=%d fineCallInMasterFlat=%d"),
        *Function->GetPathName(), FCore::Expressions(Function).Num(), Graph.Num(), FCore::Expressions(Master).Contains(Fine));
    for (auto* E : Graph)
        if (auto* P = Cast<UMaterialExpressionScalarParameter>(E); P && P->ParameterName == TEXT("APS_LavaFineStochastic"))
            UE_LOG(LogTemp, Display, TEXT("[APS.LavaFineReload] parameter=%s default=%g inFunctionFlat=%d"), *P->GetPathName(), P->DefaultValue, FCore::Expressions(Function).Contains(P));
    const auto Report = [&](const TCHAR* Phase) {
        UMaterialInterface* Materials[] = {Master, Native, APS};
        for (auto* M : Materials)
            for (const TCHAR* Name : {TEXT("APS_LavaFineStochastic"), TEXT("APS_LavaFinePeriodCm"), TEXT("APS_LavaThermalCoverage")})
            {
                float V = -123; bool Found = M->GetScalarParameterValue(FMaterialParameterInfo(Name), V);
                UE_LOG(LogTemp, Display, TEXT("[APS.LavaFineReload] phase=%s owner=%s parameter=%s found=%d value=%g"), Phase, *M->GetName(), Name, Found, V);
            }
    };
    Report(TEXT("loaded"));
    if (bRepair)
    {
        if (!B.Error.IsEmpty() || Graph.Num() != 134 || FCore::Expressions(Function).Num() != 38) return false;
        const FString Backup = FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir()
            / (bStorageRepair ? TEXT("SharedLavaFineRegistrationBackup_20260927_v2") : TEXT("SharedLavaFineRegistrationBackup_20260927")));
        if (IFileManager::Get().DirectoryExists(*Backup) || !IFileManager::Get().MakeDirectory(*Backup, true)) return false;
        for (const TCHAR* Name : {TEXT("M_APS_SharedLava"), PrivateName})
            if (IFileManager::Get().Copy(*(Backup / (FString(Name)+TEXT(".uasset"))), *Filename(Name), false, false) != COPY_OK
                || Hash(Backup / (FString(Name)+TEXT(".uasset"))) != Hash(Filename(Name))) return false;
        for (auto* E : Graph) Reader.Register(Function, E);
        if (bStorageRepair && !SynchronizeEditorCollections(Function)) return false;
        if (FCore::Expressions(Function).Num() != Graph.Num()) return false;
        Function->PostEditChange(); UMaterialEditingLibrary::UpdateMaterialFunction(Function); Master->PostEditChange();
        UMaterialInterface* Materials[] = {Master, Native, APS};
        for (auto* M : Materials) M->ForceRecompileForRendering(EMaterialShaderPrecompileMode::Default);
        FAssetCompilingManager::Get().FinishAllCompilation();
        if (GShaderCompilingManager) GShaderCompilingManager->FinishAllCompilation();
        for (auto* M : Materials)
        {
            auto* R = M->GetMaterialResource(GMaxRHIFeatureLevel); const auto* Map = R ? R->GetGameThreadShaderMap() : nullptr;
            float Value = -1;
            if (!R || !R->IsGameThreadShaderMapComplete() || R->GetCompileErrors().Num()
                || !Map || !Map->GetMeshShaderMap(&FLocalVertexFactory::StaticType)
                || !M->GetScalarParameterValue(FMaterialParameterInfo(TEXT("APS_LavaFineStochastic")), Value) || Value != 0) return false;
        }
        for (const auto& E : Protected) if (Hash(Filename(E.Name)) != E.SHA1) return false;
        UObject* Save[] = {Function, Master};
        for (auto* O : Save)
        {
            auto* Package = O->GetOutermost(); Package->MarkPackageDirty();
            FSavePackageArgs Args; Args.TopLevelFlags = RF_Public | RF_Standalone; Args.SaveFlags = SAVE_NoError;
            if (!UPackage::SavePackage(Package, O, *Filename(*O->GetName()), Args)) return false;
        }
        for (int32 I=1; I<Protected.Num(); ++I) if (Hash(Filename(Protected[I].Name)) != Protected[I].SHA1) return false;
        Report(TEXT("registered"));
        UE_LOG(LogTemp, Display, TEXT("[APS.LavaFineRegistration] SAVED nodes=%d graphUnchanged=1 onlyPrivateFunctionAndMaster=1 backup=%s"), Graph.Num(), *Backup);
        return true;
    }
    Master->UpdateCachedExpressionData(); Native->UpdateCachedData(); APS->UpdateCachedData();
    Report(TEXT("refreshedInMemory"));
    UE_LOG(LogTemp, Display, TEXT("[APS.LavaFineReload] inspectedNoSave=1"));
    return B.Error.IsEmpty();
}
inline bool Run(IAssetTools& Tools)
{
    if (FParse::Param(FCommandLine::Get(), TEXT("InspectSharedLavaFineStochastic"))) return InspectReload(Tools);
    FCore B(Tools, APSSharedLavaMaterialBuilder::Destination);
    const auto Refuse = [&B](const FString& Why) {
        UE_LOG(LogTemp, Error, TEXT("[APS.LavaFineStochastic] Refused: %s %s"), *Why, *B.Error); return false;
    };
    if (!IsRunningCommandlet()) return Refuse(TEXT("Offline only"));
    TArray<APSLavaFinePeriodUpdate::FExpected> Expected;
    Expected.Append(APSLavaFinePeriodUpdate::Expected, UE_ARRAY_COUNT(APSLavaFinePeriodUpdate::Expected));
    Expected[0].SHA1 = TEXT("1925E420AFEB95167989714E494A434BA7B3DE9E");
    for (const auto& E : Expected)
        if (Hash(Filename(E.Name)) != E.SHA1) return Refuse(FString(TEXT("Asset drift: ")) + E.Name);
    const FString Root(APSSharedLavaMaterialBuilder::Destination);
    if (FPackageName::DoesPackageExist(Root / PrivateName)) return Refuse(TEXT("Private output already exists"));
    auto* Master = LoadObject<UMaterial>(nullptr, *(Root / TEXT("M_APS_SharedLava.M_APS_SharedLava")));
    auto* Native = LoadObject<UMaterialInstanceConstant>(nullptr, *(Root / TEXT("MI_APS_SharedLavaNative.MI_APS_SharedLavaNative")));
    auto* APS = LoadObject<UMaterialInstanceConstant>(nullptr, *(Root / TEXT("MI_APS_SharedLava.MI_APS_SharedLava")));
    auto* Source = LoadObject<UMaterialFunction>(nullptr, *(Root / TEXT("MF_APS_WorldAlignedTexture_a83aa78c.MF_APS_WorldAlignedTexture_a83aa78c")));
    if (!Master || !Native || !APS || !Source || Master->GetOutermost()->IsDirty()
        || Source->GetOutermost()->IsDirty() || !Source->bAllExpressionsLoadedCorrectly
        || Native->Parent.Get() != Master || APS->Parent.Get() != Native || FCore::Expressions(Master).Num() != 128)
        return Refuse(TEXT("Unexpected candidate chain/graph"));
    auto* Fine = FindObject<UMaterialExpressionMaterialFunctionCall>(Master, TEXT("MaterialExpressionMaterialFunctionCall_14"));
    auto* Medium = FindObject<UMaterialExpressionMaterialFunctionCall>(Master, TEXT("MaterialExpressionMaterialFunctionCall_16"));
    if (!Fine || !Medium || Fine->MaterialFunction != Source || Medium->MaterialFunction != Source)
        return Refuse(TEXT("Expected fine/medium source bindings"));
    const auto* Position = Fine->FunctionInputs.FindByPredicate([](const FFunctionExpressionInput& P)
        {return P.Input.InputName == TEXT("WorldPosition");});
    if (!Position || Position->Input.Expression) return Refuse(TEXT("Unexpected fine WorldPosition override"));
    const FString Backup = FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir() / TEXT("SharedLavaFineStochasticBackup_20260927"));
    if (IFileManager::Get().DirectoryExists(*Backup)) return Refuse(TEXT("Immutable backup exists"));
    if (!IFileManager::Get().MakeDirectory(*Backup, true)) return Refuse(TEXT("Cannot create backup"));
    for (const auto& E : Expected)
    {
        const FString Dest = Backup / (FString(E.Name)+TEXT(".uasset"));
        if (Hash(Filename(E.Name)) != E.SHA1 || IFileManager::Get().Copy(*Dest, *Filename(E.Name), false, false) != COPY_OK
            || Hash(Dest) != E.SHA1) return Refuse(TEXT("Backup failed"));
    }
    auto* Copy = Cast<UMaterialFunction>(B.Duplicate(Source, PrivateName));
    if (!Copy) return Refuse(TEXT("Clone failed"));
    APSSharedTerrainNormalContinuity::TTransform<FCore> Reader(B);
    const auto Graph = Reader.Graph(Copy);
    if (!B.Error.IsEmpty()) return Refuse(TEXT("Graph closure"));
    UMaterialExpressionCustom* Precise = nullptr;
    TArray<UMaterialExpressionTextureSample*> Samples;
    for (auto* E : Graph)
    {
        if (auto* C = Cast<UMaterialExpressionCustom>(E);
            C && C->Description == TEXT("APS lava precise physical UV: compensated transform/division, wrap after unwrapped gradients, no warp"))
        {
            if (Precise) return Refuse(TEXT("Duplicate precise kernel"));
            Precise = C;
        }
        if (auto* T = Cast<UMaterialExpressionTextureSample>(E)) Samples.Add(T);
    }
    const FString Return(TEXT("return DFFracDemote(U);\n"));
    if (!Precise || Samples.Num() != 3 || Precise->AdditionalOutputs.Num() != 2
        || Precise->Outputs.Num() != 3 || !Precise->Code.EndsWith(Return)
        || Precise->AdditionalOutputs[0].OutputName != TEXT("UVdx")
        || Precise->AdditionalOutputs[1].OutputName != TEXT("UVdy")) return Refuse(TEXT("Precise kernel contract changed"));
    Precise->Code.LeftChopInline(Return.Len());
    // Derive lattice IDs/fraction from compensated, UNWRAPPED U. Skewing frac(U)
    // instead causes seams at every texture wrap. Modulo65536 keeps IDs exactly
    // representable without demoting the planet-scale integral coordinate.
    for (int32 I = 0; I < 3; ++I)
    {
        const TCHAR* Names[] = {TEXT("GridXY"), TEXT("GridXZ"), TEXT("GridYZ")};
        const int32 X[] = {0,0,1}, Y[] = {1,2,2};
        FCustomOutput O; O.OutputName = Names[I]; O.OutputType = CMOT_Float4;
        Precise->AdditionalOutputs.Add(O); Precise->Outputs.Add(FExpressionOutput(Names[I]));
        Precise->Code += FString::Printf(TEXT(
            "FDFVector2 P%d = DFSwizzle(U, %d, %d);\n"
            "FDFVector2 S%d = MakeDFVector(DFSubtract(DFGetX(P%d), DFMultiply(DFGetY(P%d), 0.5773502691896258)), DFMultiply(DFGetY(P%d), 1.1547005383792515));\n"
            "%s = float4(DFFracDemote(DFMultiply(DFFloor(S%d), 1.0/65536.0))*65536.0, DFFracDemote(S%d));\n"),
            I, X[I], Y[I], I, I, I, I, Names[I], I, I);
    }
    Precise->Code += Return;
    auto* Enable = B.Add<UMaterialExpressionScalarParameter>(Copy);
    Enable->ParameterName = TEXT("APS_LavaFineStochastic"); Enable->DefaultValue = 0;
    Enable->SliderMin = 0; Enable->SliderMax = 1; Enable->Group = TEXT("APS Lava Surface"); Enable->UpdateParameterGuid(true, true);
    uint32 ProjectionSet = 0;
    int32 ChangedEdges = 0;
    for (auto* Sample : Samples)
    {
        auto* Phase = Cast<UMaterialExpressionComponentMask>(Sample->Coordinates.Expression);
        auto* DX = Cast<UMaterialExpressionComponentMask>(Sample->CoordinatesDX.Expression);
        auto* DY = Cast<UMaterialExpressionComponentMask>(Sample->CoordinatesDY.Expression);
        if (!Phase || !DX || !DY || Phase->Input.Expression != Precise || Phase->Input.OutputIndex != 0
            || DX->Input.Expression != Precise || DX->Input.OutputIndex != 1
            || DY->Input.Expression != Precise || DY->Input.OutputIndex != 2
            || Phase->A || DX->A || DY->A || Phase->R != DX->R || Phase->G != DX->G || Phase->B != DX->B
            || Phase->R != DY->R || Phase->G != DY->G || Phase->B != DY->B
            || Sample->MipValueMode != TMVM_Derivative || Sample->SamplerType != SAMPLERTYPE_Color
            || Sample->SamplerSource != SSM_Wrap_WorldGroupSettings || !Sample->TextureObject.Expression
            || Sample->MipValue.Expression) return Refuse(TEXT("Projection/gradient/sampler contract changed"));
        const uint32 Bits = (Phase->R?1u:0u)|(Phase->G?2u:0u)|(Phase->B?4u:0u);
        const int32 I = Bits == 3 ? 0 : Bits == 5 ? 1 : Bits == 6 ? 2 : -1;
        if (I < 0 || (ProjectionSet & (1u<<I))) return Refuse(TEXT("Invalid/duplicate projection"));
        ProjectionSet |= 1u<<I;
        TArray<FExpressionInput*> RGB, Alpha;
        for (auto* E : Graph)
            for (auto* Input : E->GetInputsView())
            {
                if (!Input || Input->Expression != Sample) continue;
                if (Input->OutputIndex == 0 && Input->Mask && Input->MaskR && Input->MaskG && Input->MaskB && !Input->MaskA) RGB.Add(Input);
                else if (Input->OutputIndex == 4 && Input->Mask && !Input->MaskR && !Input->MaskG && !Input->MaskB && Input->MaskA) Alpha.Add(Input);
                else return Refuse(TEXT("Unexpected sample consumer channel"));
            }
        if (RGB.Num() != 2 || Alpha.Num() != 1) return Refuse(TEXT("Expected two RGB and one alpha consumer"));
        auto* C = B.Add<UMaterialExpressionCustom>(Copy);
        C->Description = TEXT("APS fine lattice shared-vertex translations; RGBA reads remain native"); C->OutputType = CMOT_Float2;
        C->Inputs.Reset(); C->AdditionalOutputs.Reset(); C->Outputs.Reset(); C->Outputs.Add(FExpressionOutput(TEXT("return")));
        const auto AddInput = [C](const TCHAR* Name, UMaterialExpression* E, int32 Output=0) {
            FCustomInput P; P.InputName = Name; P.Input.Connect(Output, E); C->Inputs.Add(P);
        };
        AddInput(TEXT("Phase"), Phase); AddInput(TEXT("Grid"), Precise, 3+I); AddInput(TEXT("Enabled"), Enable);
        for (const TCHAR* Name : {TEXT("UV1"), TEXT("UV2"), TEXT("Weights")})
        {
            FCustomOutput O; O.OutputName = Name; O.OutputType = O.OutputName == TEXT("Weights") ? CMOT_Float3 : CMOT_Float2;
            C->AdditionalOutputs.Add(O); C->Outputs.Add(FExpressionOutput(Name));
        }
        C->Code = FString::Printf(TEXT(
            "if (Enabled < 0.5) { UV1=Phase; UV2=Phase; Weights=float3(1,0,0); return Phase; }\n"
            "uint2 cell=((uint2)floor(Grid.xy+0.5)) & 65535u; float2 f=saturate(Grid.zw);\n"
            "uint2 v0,v1,v2;\n"
            "if(f.x+f.y<=1.0){v0=cell;v1=cell+uint2(1,0);v2=cell+uint2(0,1);Weights=float3(1-f.x-f.y,f.x,f.y);}\n"
            "else{v0=cell+uint2(1,1);v1=cell+uint2(0,1);v2=cell+uint2(1,0);Weights=float3(f.x+f.y-1,1-f.x,1-f.y);}\n"
            "v0&=65535u;v1&=65535u;v2&=65535u;\n"
            "uint3 h=uint3(v0.x^(v0.y<<16),v1.x^(v1.y<<16),v2.x^(v2.y<<16)) ^ %uu;\n"
            "h^=h>>16;h*=0x7feb352du;h^=h>>15;h*=0x846ca68bu;h^=h>>16;\n"
            "float3 ox=float3(h&65535u)*(1.0/65536.0),oy=float3(h>>16)*(1.0/65536.0);\n"
            "Weights=saturate(Weights);Weights*=Weights;Weights*=Weights;Weights/=max(dot(Weights,float3(1,1,1)),1e-8);\n"
            "UV1=frac(Phase+float2(ox.y,oy.y));UV2=frac(Phase+float2(ox.z,oy.z));return frac(Phase+float2(ox.x,oy.x));\n"),
            0x2c9277b5u + uint32(I)*0x9e3779b9u);
        UMaterialExpressionTextureSample* Taps[] = {Sample, nullptr, nullptr};
        for (int32 J=1; J<3; ++J)
        {
            auto* T = B.Add<UMaterialExpressionTextureSample>(Copy); Taps[J] = T;
            T->Texture = Sample->Texture; T->TextureObject = Sample->TextureObject;
            T->SamplerSource = Sample->SamplerSource; T->SamplerType = Sample->SamplerType;
            T->MipValueMode = TMVM_Derivative; T->CoordinatesDX = Sample->CoordinatesDX; T->CoordinatesDY = Sample->CoordinatesDY;
            T->AutomaticViewMipBias = Sample->AutomaticViewMipBias;
        }
        UMaterialExpression* Sums[] = {nullptr, nullptr};
        for (int32 J=0; J<3; ++J)
        {
            Taps[J]->Coordinates = FExpressionInput(); Taps[J]->Coordinates.Connect(J, C);
            auto* Weight = B.Add<UMaterialExpressionComponentMask>(Copy); Weight->Input.Connect(3, C);
            Weight->R = J==0; Weight->G = J==1; Weight->B = J==2; Weight->A = false;
            for (int32 Channel=0; Channel<2; ++Channel)
            {
                auto* Product = B.Add<UMaterialExpressionMultiply>(Copy); Product->A.Connect(Channel ? 4 : 0, Taps[J]);
                Product->B.Connect(0, Weight);
                if (!Sums[Channel]) Sums[Channel] = Product;
                else {auto* Add = B.Add<UMaterialExpressionAdd>(Copy); Add->A.Connect(0, Sums[Channel]); Add->B.Connect(0, Product); Sums[Channel] = Add;}
            }
        }
        for (auto* Input : RGB) {*Input = FExpressionInput(); Input->Connect(0, Sums[0]); ++ChangedEdges;}
        for (auto* Input : Alpha) {*Input = FExpressionInput(); Input->Connect(0, Sums[1]); ++ChangedEdges;}
    }
    int32 Count = 0;
    for (auto* E : Reader.Graph(Copy))
    {
        Reader.Register(Copy, E); // Persist full closure, including editor-created parameter nodes.
        if (Cast<UMaterialExpressionTextureSample>(E)) ++Count;
    }
    if (!B.Error.IsEmpty() || ProjectionSet != 7 || ChangedEdges != 9 || Count != 9)
        return Refuse(TEXT("Final graph invariants"));
    if (!SynchronizeEditorCollections(Copy)) return Refuse(TEXT("Editor-only storage synchronization failed"));
    Copy->PostEditChange(); UMaterialEditingLibrary::UpdateMaterialFunction(Copy);
    if (!B.ReconnectFunctionById(Fine, Source, Copy)) return Refuse(TEXT("Pin-ID-preserving reconnect failed"));
    Master->PostEditChange();
    UMaterialInterface* Materials[] = {Master, Native, APS};
    for (auto* M : Materials) M->ForceRecompileForRendering(EMaterialShaderPrecompileMode::Default);
    FAssetCompilingManager::Get().FinishAllCompilation();
    if (GShaderCompilingManager) GShaderCompilingManager->FinishAllCompilation();
    for (auto* M : Materials)
    {
        auto* R = M->GetMaterialResource(GMaxRHIFeatureLevel); const auto* Map = R ? R->GetGameThreadShaderMap() : nullptr;
        if (!R || !R->IsGameThreadShaderMapComplete() || R->GetCompileErrors().Num()
            || !Map || !Map->GetMeshShaderMap(&FLocalVertexFactory::StaticType)) return Refuse(TEXT("Shader/LocalVF incomplete"));
    }
    float V = -1;
    if (!APS->GetScalarParameterValue(FMaterialParameterInfo(TEXT("APS_LavaFineStochastic")), V) || V != 0
        || !APS->GetScalarParameterValue(FMaterialParameterInfo(TEXT("APS_LavaFinePeriodCm")), V) || V != 40000
        || !APS->GetScalarParameterValue(FMaterialParameterInfo(TEXT("APS_LavaThermalCoverage")), V) || V != 0
        || Medium->MaterialFunction != Source) return Refuse(TEXT("Saved defaults/medium branch changed"));
    for (const auto& E : Expected) if (Hash(Filename(E.Name)) != E.SHA1) return Refuse(TEXT("Concurrent asset edit"));
    // Save the new dependency first. If master save fails, production can still
    // be restored from the immutable wrapper backup; no prior function is lost.
    UObject* Save[] = {Copy, Master};
    for (auto* O : Save)
    {
        auto* P = O->GetOutermost(); P->MarkPackageDirty();
        FSavePackageArgs Args; Args.TopLevelFlags = RF_Public | RF_Standalone; Args.SaveFlags = SAVE_NoError;
        if (!UPackage::SavePackage(P, O, *Filename(*O->GetName()), Args)) return Refuse(TEXT("Save failed"));
    }
    for (int32 I=1; I<Expected.Num(); ++I)
        if (Hash(Filename(Expected[I].Name)) != Expected[I].SHA1) return Refuse(TEXT("Protected asset changed"));
    UE_LOG(LogTemp, Display, TEXT("[APS.LavaFineStochastic] SAVED fineSamples=9 addedSamples=6 changedCalls=1 defaultStochastic=0 defaultCoverage=0 defaultPeriodCm=40000 unchangedSourceMedium20km=1 backup=%s renderedAcceptancePending=1"), *Backup);
    return true;
}
}
#endif
