#pragma once
#if WITH_EDITOR
#include "APSLavaCrustFieldUpdate.h"
#include "Materials/MaterialExpressionPower.h"
#include "Materials/MaterialExpressionReroute.h"

namespace APSLavaRadianceIntegrationUpdate
{
using FCore = APSSharedTerrainMaterialBuilder::FBuild;
using APSLavaAntiGridUpdate::Filename;
using APSLavaAntiGridUpdate::Hash;
inline bool Inspect()
{
    if (!IsRunningCommandlet() || Hash(Filename(TEXT("M_APS_SharedLava"))) != TEXT("2DB3405BECA5CE8B178CE728F731AB76F24AEFF3")) return false;
    const FString Root(APSSharedLavaMaterialBuilder::Destination);
    auto* M=LoadObject<UMaterial>(nullptr,*(Root/TEXT("M_APS_SharedLava.M_APS_SharedLava")));
    auto* APS=LoadObject<UMaterialInstanceConstant>(nullptr,*(Root/TEXT("MI_APS_SharedLava.MI_APS_SharedLava")));
    if (!M || !APS || M->GetOutermost()->IsDirty() || FCore::Expressions(M).Num()!=119) return false;
    TSet<UMaterialExpression*> Visited;
    TFunction<void(UMaterialExpression*)> Dump = [&](UMaterialExpression* E) {
        if (!E || Visited.Contains(E) || E->GetOuter()!=M) return;
        Visited.Add(E); FString Extra;
        if (auto* A=Cast<UMaterialExpressionAdd>(E)) Extra+=FString::Printf(TEXT(" constA=%g constB=%g"),A->ConstA,A->ConstB);
        if (auto* A=Cast<UMaterialExpressionMultiply>(E)) Extra+=FString::Printf(TEXT(" constA=%g constB=%g"),A->ConstA,A->ConstB);
        if (auto* A=Cast<UMaterialExpressionPower>(E)) Extra+=FString::Printf(TEXT(" exponent=%g"),A->ConstExponent);
        if (auto* A=Cast<UMaterialExpressionConstant>(E)) Extra+=FString::Printf(TEXT(" scalar=%g"),A->R);
        if (auto* A=Cast<UMaterialExpressionConstant3Vector>(E)) Extra+=TEXT(" vector=")+A->Constant.ToString();
        if (auto* A=Cast<UMaterialExpressionScalarParameter>(E)) {float V=0; APS->GetScalarParameterValue(FMaterialParameterInfo(A->ParameterName),V); Extra+=FString::Printf(TEXT(" parameter=%s default=%g resolved=%g"),*A->ParameterName.ToString(),A->DefaultValue,V);}
        if (auto* A=Cast<UMaterialExpressionVectorParameter>(E)) {FLinearColor V; APS->GetVectorParameterValue(FMaterialParameterInfo(A->ParameterName),V); Extra+=TEXT(" parameter=")+A->ParameterName.ToString()+TEXT(" resolved=")+V.ToString();}
        if (auto* A=Cast<UMaterialExpressionMaterialFunctionCall>(E)) Extra+=TEXT(" function=")+GetPathNameSafe(A->MaterialFunction);
        FString Edges;
        int32 I=0;
        for (const auto* In:E->GetInputsView())
        {
            if (In && In->Expression) Edges+=FString::Printf(TEXT(" [%d:%s/%d mask=%d:%d%d%d%d]"),I,*In->Expression->GetName(),In->OutputIndex,In->Mask,In->MaskR,In->MaskG,In->MaskB,In->MaskA);
            ++I;
        }
        UE_LOG(LogTemp,Display,TEXT("[APS.LavaRadianceGraph] %s%s%s"),*E->GetName(),*Extra,*Edges);
        for (const auto* In:E->GetInputsView()) if (In) Dump(In->Expression);
    };
    auto* Emission=M->GetExpressionInputForProperty(MP_EmissiveColor);
    if (!Emission || !Emission->Expression) return false;
    UE_LOG(LogTemp,Display,TEXT("[APS.LavaRadianceGraph] emissiveRoot=%s"),*Emission->Expression->GetName());
    Dump(Emission->Expression);
    if (Hash(Filename(TEXT("M_APS_SharedLava")))!=TEXT("2DB3405BECA5CE8B178CE728F731AB76F24AEFF3")) return false;
    UE_LOG(LogTemp,Display,TEXT("[APS.LavaRadianceGraph] inspectedNoSave=1 nodes=%d"),Visited.Num());
    return true;
}

// Audited graph: E = K * B * (P*(h+.3)+.6); fifth power is in COARSE K.
// Thermal coverage requires the extra filtered moment mean(h*a).
// Native RGB is NOT grayscale and has other consumers: keep it untouched.
// A separate matching sampler preserves RGB, alpha and normals. This costs
// three periodic or nine stochastic extra reads; performance is NOT accepted.
inline bool Run(IAssetTools& Tools)
{
    FCore B(Tools, APSSharedLavaMaterialBuilder::Destination);
    const auto Refuse=[&B](const FString& Why) { UE_LOG(LogTemp,Error,TEXT("[APS.LavaRadianceIntegration] Refused: %s %s"),*Why,*B.Error); return false; };
    TArray<APSLavaFinePeriodUpdate::FExpected> Protected;
    Protected.Append(APSLavaFinePeriodUpdate::Expected,UE_ARRAY_COUNT(APSLavaFinePeriodUpdate::Expected));
    Protected[0].SHA1=TEXT("2039C370AC6641EA3C0FBDD915FFAA9A11A4764C");
    Protected.Add({TEXT("MF_APS_LavaFineStochastic_v1"),TEXT("75CF7DF479BB11412917E1869FA0018CEADEE645")});
    if (!IsRunningCommandlet()) return Refuse(TEXT("Offline only"));
    for (const auto& E:Protected) if (Hash(Filename(E.Name))!=E.SHA1) return Refuse(FString(TEXT("Asset drift: "))+E.Name);
    const FString Root(APSSharedLavaMaterialBuilder::Destination);
    const TCHAR* MomentName=TEXT("T_APS_LavaFineRadianceMoments_v1");
    if (FPackageName::DoesPackageExist(Root/MomentName)) return Refuse(TEXT("Private moments already exist"));
    auto* Master=LoadObject<UMaterial>(nullptr,*(Root/TEXT("M_APS_SharedLava.M_APS_SharedLava")));
    auto* Native=LoadObject<UMaterialInstanceConstant>(nullptr,*(Root/TEXT("MI_APS_SharedLavaNative.MI_APS_SharedLavaNative")));
    auto* APS=LoadObject<UMaterialInstanceConstant>(nullptr,*(Root/TEXT("MI_APS_SharedLava.MI_APS_SharedLava")));
    if (!Master || !Native || !APS || Master->GetOutermost()->IsDirty() || Native->Parent.Get()!=Master
        || APS->Parent.Get()!=Native || FCore::Expressions(Master).Num()!=138) return Refuse(TEXT("Candidate graph/chain"));
    auto* Fine=FindObject<UMaterialExpressionMaterialFunctionCall>(Master,TEXT("MaterialExpressionMaterialFunctionCall_14"));
    auto* Offset=FindObject<UMaterialExpressionAdd>(Master,TEXT("MaterialExpressionAdd_26"));
    auto* Product=FindObject<UMaterialExpressionMultiply>(Master,TEXT("MaterialExpressionMultiply_105"));
    auto* Coarse=FindObject<UMaterialExpressionMultiply>(Master,TEXT("MaterialExpressionMultiply_104"));
    auto* Mask=FindObject<UMaterialExpressionComponentMask>(Master,TEXT("MaterialExpressionComponentMask_1"));
    auto* Power=FindObject<UMaterialExpressionPower>(Master,TEXT("MaterialExpressionPower_1"));
    auto* Bound=FindObject<UMaterialExpressionClamp>(Master,TEXT("MaterialExpressionClamp_0"));
    auto* Add=FindObject<UMaterialExpressionAdd>(Master,TEXT("MaterialExpressionAdd_3"));
    auto* NativeEmission=FindObject<UMaterialExpressionMultiply>(Master,TEXT("MaterialExpressionMultiply_27"));
    auto* NativeFactor=FindObject<UMaterialExpressionMultiply>(Master,TEXT("MaterialExpressionMultiply_3"));
    auto* Reroute=FindObject<UMaterialExpressionReroute>(Master,TEXT("MaterialExpressionReroute_6"));
    auto* Emission=Master->GetExpressionInputForProperty(MP_EmissiveColor);
    auto* Pin=Fine ? Fine->FunctionInputs.FindByPredicate([](const FFunctionExpressionInput& P) {return P.Input.InputName==TEXT("TextureObject");}):nullptr;
    auto* Object=Pin ? Cast<UMaterialExpressionTextureObjectParameter>(Pin->Input.Expression):nullptr;
    auto* Source=Object ? Cast<UTexture2D>(Object->Texture):nullptr;
    UMaterialExpressionScalarParameter* Thermal=nullptr;
    for (auto* E:FCore::Expressions(Master))
        if (auto* P=Cast<UMaterialExpressionScalarParameter>(E); P && P->ParameterName==TEXT("APS_LavaThermalCoverage")) Thermal=P;
    if (!Fine || !Offset || !Product || !Coarse || !Mask || !Power || !Bound || !Add || !NativeEmission || !NativeFactor || !Reroute
        || !Emission || !Emission->Expression || !Thermal || !Source || Source->GetName()!=TEXT("T_APS_LavaThermal400m_RGBA_v1")
        || Object->ParameterName!=TEXT("APS_LavaCrustField") || Offset->A.Expression!=Fine || Offset->A.OutputIndex!=2 || Offset->ConstB!=.3f
        || Offset->B.Expression || Product->A.Expression!=Offset || Product->B.Expression!=Coarse
        || Mask->Input.Expression!=Product || !Mask->R || Mask->G || Mask->B || Mask->A
        || Power->ConstExponent!=5 || Power->Exponent.Expression || Reroute->Input.Expression!=Mask
        || Add->A.Expression!=Reroute || Add->B.Expression || Add->ConstB!=.6f
        || NativeFactor->A.Expression!=Add || NativeFactor->B.Expression!=Bound
        || NativeEmission->A.Expression!=Power || NativeEmission->B.Expression!=NativeFactor)
        return Refuse(TEXT("Audited factorization changed"));
    if (Source->Source.GetFormat()!=TSF_BGRA8 || !Source->SRGB || Source->Source.GetNumLayers()!=1 || Source->Source.GetNumSlices()!=1)
        return Refuse(TEXT("Unsupported source layout"));
    const int64 W=Source->Source.GetSizeX(), H=Source->Source.GetSizeY(); TArray64<uint8> Bytes;
    if (W!=2048 || H!=2048 || !Source->Source.GetMipData(Bytes,0) || Bytes.Num()!=W*H*4) return Refuse(TEXT("Bounded source read failed"));
    auto* Pixels=reinterpret_cast<FColor*>(Bytes.GetData());
    double MeanH=0, MeanA=0, MeanJoint=0, MeanStored=0, MaxError=0;
    for (int64 I=0; I<W*H; ++I)
    {
        const FLinearColor C(Pixels[I]); const double Joint=double(C.R)*C.A;
        const auto Encoded=FLinearColor(float(Joint),float(Joint),float(Joint),C.A).ToFColor(true);
        Pixels[I]=Encoded;
        const double Stored=FLinearColor(Pixels[I]).R;
        MeanH+=C.R; MeanA+=C.A; MeanJoint+=Joint; MeanStored+=Stored;
        MaxError=FMath::Max(MaxError,FMath::Abs(Joint-Stored));
    }
    MeanH/=W*H; MeanA/=W*H; MeanJoint/=W*H; MeanStored/=W*H;
    if (MaxError>.005 || FMath::Abs(MeanStored-MeanJoint)>.0005 || MeanJoint<MeanH*MeanA)
        return Refuse(TEXT("Moment quantization/integration error"));
    TMap<int32,FExpressionInput> Properties;
    for (int32 P=0; P<MP_MAX; ++P) if (auto* In=Master->GetExpressionInputForProperty(static_cast<EMaterialProperty>(P))) Properties.Add(P,*In);
    const FString Backup=FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir()/TEXT("SharedLavaRadianceIntegrationBackup_20260927"));
    if (IFileManager::Get().DirectoryExists(*Backup) || !IFileManager::Get().MakeDirectory(*Backup,true)) return Refuse(TEXT("Immutable backup exists/unavailable"));
    for (const auto& E:Protected)
        if (Hash(Filename(E.Name))!=E.SHA1 || IFileManager::Get().Copy(*(Backup/(FString(E.Name)+TEXT(".uasset"))),*Filename(E.Name),false,false)!=COPY_OK
            || Hash(Backup/(FString(E.Name)+TEXT(".uasset")))!=E.SHA1) return Refuse(TEXT("Backup mismatch"));
    auto* Texture=Cast<UTexture2D>(B.Duplicate(Source,MomentName));
    if (!Texture) return Refuse(TEXT("Texture duplication failed"));
    Texture->PreEditChange(nullptr); Texture->Source.Init(int32(W),int32(H),1,1,TSF_BGRA8,Bytes.GetData());
    Texture->SRGB=true; Texture->CompressionSettings=TC_BC7; Texture->CompressionNoAlpha=false; Texture->CompressionForceAlpha=true;
    Texture->MipGenSettings=TMGS_SimpleAverage; Texture->bDoScaleMipsForAlphaCoverage=false; Texture->PostEditChange();
    FAssetCompilingManager::Get().FinishAllCompilation();
    TArray64<uint8> Check;
    if (!Texture->Source.GetMipData(Check,0) || Check!=Bytes || !Texture->GetPlatformData()
        || Texture->GetPlatformData()->PixelFormat!=PF_BC7 || Texture->GetNumMips()!=12) return Refuse(TEXT("Moment platform/source mismatch"));
    auto* Strength=B.Add<UMaterialExpressionScalarParameter>(Master);
    Strength->ParameterName=TEXT("APS_LavaRadianceIntegration"); Strength->DefaultValue=0;
    Strength->SliderMin=0; Strength->SliderMax=1; Strength->Group=TEXT("APS Lava Surface"); Strength->UpdateParameterGuid(true,true);
    auto* MomentObject=B.Add<UMaterialExpressionTextureObject>(Master);
    MomentObject->Texture=Texture; MomentObject->SamplerType=SAMPLERTYPE_Color;
    auto* MomentCall=B.Add<UMaterialExpressionMaterialFunctionCall>(Master);
    MomentCall->SetMaterialFunction(Fine->MaterialFunction);
    for (auto& Input:MomentCall->FunctionInputs)
    {
        const auto* Original=Fine->FunctionInputs.FindByPredicate([&Input](const FFunctionExpressionInput& P) { return P.Input.InputName==Input.Input.InputName; });
        if (!Original) return Refuse(TEXT("Moment sampler input mismatch"));
        Input.Input=Original->Input;
        if (Input.Input.InputName==TEXT("TextureObject")) Input.Input.Connect(0,MomentObject);
    }
    auto* Correct=B.Add<UMaterialExpressionCustom>(Master);
    Correct->Description=TEXT("APS joint fine thermal radiance: covariance correction, separate matching moment samples");
    Correct->OutputType=CMOT_Float3; Correct->Inputs.Empty();
    const auto Input=[Correct](const TCHAR* Name,UMaterialExpression* E,int32 Index=0) {
        FCustomInput In; In.InputName=Name; In.Input.Connect(Index,E); Correct->Inputs.Add(In);
    };
    Input(TEXT("Legacy"),Emission->Expression,Emission->OutputIndex); Input(TEXT("Fine"),Fine,2);
    Input(TEXT("K"),Power); Input(TEXT("P"),Coarse); Input(TEXT("B"),Bound);
    Input(TEXT("Thermal"),Thermal); Input(TEXT("Integration"),Strength);
    Input(TEXT("Moment"),MomentCall,2);
    Correct->Code=TEXT("return Legacy + saturate(Integration)*saturate(Thermal)*K*B*P.r*(Moment.r-Fine.r*Fine.a);\n");
    Emission->Connect(0,Correct);
    if (FCore::Expressions(Master).Num()!=142) return Refuse(TEXT("Unexpected node count"));
    for (const auto& Pair:Properties) if (Pair.Key!=MP_EmissiveColor)
    {
        auto* Now=Master->GetExpressionInputForProperty(static_cast<EMaterialProperty>(Pair.Key));
        if (!Now || !APSLavaCrustReflectanceUpdate::SameInput(*Now,Pair.Value)) return Refuse(TEXT("Non-emissive output changed"));
    }
    Master->PostEditChange(); UMaterialInterface* Materials[]={Master,Native,APS};
    for (auto* M:Materials) M->ForceRecompileForRendering(EMaterialShaderPrecompileMode::Default);
    FAssetCompilingManager::Get().FinishAllCompilation(); if (GShaderCompilingManager) GShaderCompilingManager->FinishAllCompilation();
    for (auto* M:Materials)
    {
        auto* R=M->GetMaterialResource(GMaxRHIFeatureLevel); const auto* Map=R?R->GetGameThreadShaderMap():nullptr; float Value=-1; UTexture* BoundTexture=nullptr;
        if (!R || !R->IsGameThreadShaderMapComplete() || R->GetCompileErrors().Num() || !Map || !Map->GetMeshShaderMap(&FLocalVertexFactory::StaticType)
            || !M->GetScalarParameterValue(FMaterialParameterInfo(TEXT("APS_LavaRadianceIntegration")),Value) || Value!=0
            || !M->GetTextureParameterValue(FMaterialParameterInfo(TEXT("APS_LavaCrustField")),BoundTexture) || BoundTexture!=Source)
            return Refuse(TEXT("Compile/disabled default/texture binding invalid"));
    }
    for (const auto& E:Protected) if (Hash(Filename(E.Name))!=E.SHA1) return Refuse(TEXT("Concurrent edit"));
    for (UObject* Output:{static_cast<UObject*>(Texture),static_cast<UObject*>(Master)})
    {
        auto* Package=Output->GetOutermost(); Package->MarkPackageDirty(); FSavePackageArgs Args; Args.TopLevelFlags=RF_Public|RF_Standalone; Args.SaveFlags=SAVE_NoError;
        if (!UPackage::SavePackage(Package,Output,*Filename(*Output->GetName()),Args)) return Refuse(TEXT("Save failed; retain partial output/backups"));
    }
    for (int32 I=1; I<Protected.Num(); ++I) if (Hash(Filename(Protected[I].Name))!=Protected[I].SHA1) return Refuse(TEXT("Protected asset changed"));
    UE_LOG(LogTemp,Display,TEXT("[APS.LavaRadianceIntegration] SAVED disabledDefault=1 meanH=%.9f meanA=%.9f meanJoint=%.9f meanEncodedJoint=%.9f maxSourceQuantError=%.9f covariance=%.9f originalFineRGBUntouched=1 extraPeriodicReads=3 extraStochasticReads=9 nativePaletteGeographyMaskNormalUnchanged=1 backup=%s renderedAcceptancePending=1"),
        MeanH,MeanA,MeanJoint,MeanStored,MaxError,MeanJoint-MeanH*MeanA,*Backup);
    return true;
}
}
#endif
