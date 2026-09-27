#pragma once
#if WITH_EDITOR
#include "APSLavaCrustNormalUpdate.h"
#include "Materials/MaterialExpressionTextureObjectParameter.h"

// Offline deterministic microstructure, not a climate/geography replacement.
// The texture is periodic; precise stochastic sampling hides its repeated tile.
// RGB is a continuous cold-height/hot-intensity field, alpha is pre-integrated
// hot area. These are art-calibrated crust features, not a thermodynamic solver.
namespace APSLavaCrustFieldUpdate
{
using FCore = APSSharedTerrainMaterialBuilder::FBuild;
using APSLavaAntiGridUpdate::Filename;
using APSLavaAntiGridUpdate::Hash;
inline constexpr const TCHAR* FieldName = TEXT("T_APS_LavaCrustField40m_v1");
inline double Hash2(int32 X, int32 Y, int32 Period, uint32 Salt)
{
    const uint32 A = uint32((X % Period + Period) % Period), C = uint32((Y % Period + Period) % Period);
    uint32 H = A*0x9e3779b9u ^ C*0x85ebca6bu ^ Salt;
    H ^= H >> 16; H *= 0x7feb352du; H ^= H >> 15; H *= 0x846ca68bu; H ^= H >> 16;
    return double(H & 0x00ffffffu) / 16777215.0;
}
inline double Smooth(double T) { T = FMath::Clamp(T, 0.0, 1.0); return T*T*T*(T*(T*6-15)+10); }
inline double Noise(double U, double V, int32 Period, uint32 Salt)
{
    const double X = U*Period, Y = V*Period;
    const int32 IX = FMath::FloorToInt(X), IY = FMath::FloorToInt(Y);
    const double A = Smooth(X-IX), C = Smooth(Y-IY);
    return FMath::Lerp(FMath::Lerp(Hash2(IX,IY,Period,Salt),Hash2(IX+1,IY,Period,Salt),A),
        FMath::Lerp(Hash2(IX,IY+1,Period,Salt),Hash2(IX+1,IY+1,Period,Salt),A),C);
}
inline FLinearColor Field(double U, double V)
{
    constexpr int32 Cells = 16; // 2.5m mean cell spacing at a 40m physical tile
    // Periodic domain warp bends cell boundaries; no screen/camera coordinates.
    const FVector2D P(U*Cells + .65*(Noise(U,V,7,19)-.5) + .18*(Noise(U,V,23,29)-.5),
        V*Cells + .65*(Noise(U,V,7,41)-.5) + .18*(Noise(U,V,23,53)-.5));
    const int32 IX = FMath::FloorToInt(P.X), IY = FMath::FloorToInt(P.Y);
    FVector2D Best(0,0); int32 BX=0, BY=0; double Distance=DBL_MAX;
    auto Site = [](int32 X, int32 Y) {
        return FVector2D(X+.1+.8*Hash2(X,Y,Cells,0x2171),Y+.1+.8*Hash2(X,Y,Cells,0x4937));
    };
    for (int32 Y=-1; Y<=1; ++Y) for (int32 X=-1; X<=1; ++X)
    {
        const FVector2D D=Site(IX+X,IY+Y)-P; const double D2=D.SizeSquared();
        if (D2<Distance) { Distance=D2; Best=D; BX=IX+X; BY=IY+Y; }
    }
    // True distance to each nearest-cell bisector, not F2-F1 banding.
    double Edge=DBL_MAX;
    for (int32 Y=-2; Y<=2; ++Y) for (int32 X=-2; X<=2; ++X)
    {
        if (X==0 && Y==0) continue;
        const FVector2D Other=Site(BX+X,BY+Y)-P, Across=Other-Best;
        Edge=FMath::Min(Edge,FVector2D::DotProduct((Other+Best)*.5,Across)/FMath::Max(Across.Size(),1.e-9));
    }
    // Total hot opening about 8-20cm, with cooled transition lips outside it.
    const double Width=.018+.020*Noise(U,V,31,97);
    const double Heat=1-Smooth((Edge-Width*.3)/Width);
    const double Interior=Smooth(Edge/.12);
    const double Grain=.65*Noise(U,V,137,127)+.35*Noise(U,V,389,151);
    const double Cold=.27+.12*Hash2(BX,BY,Cells,181)+.05*(Grain-.5)+.12*(1-Interior);
    const float R=float(FMath::Lerp(Cold,.98,Heat));
    return FLinearColor(R,R,R,float(Heat));
}
inline bool Run(IAssetTools& Tools)
{
    FCore B(Tools, APSSharedLavaMaterialBuilder::Destination);
    const auto Refuse=[&B](const FString& Why) { UE_LOG(LogTemp,Error,TEXT("[APS.LavaCrustField] Refused: %s %s"),*Why,*B.Error); return false; };
    TArray<APSLavaFinePeriodUpdate::FExpected> Protected;
    Protected.Append(APSLavaFinePeriodUpdate::Expected,UE_ARRAY_COUNT(APSLavaFinePeriodUpdate::Expected));
    Protected[0].SHA1=TEXT("E0AB78CDA96B86880167A540C8E1BB9A2558BFD1");
    Protected.Add({TEXT("MF_APS_LavaFineStochastic_v1"),TEXT("75CF7DF479BB11412917E1869FA0018CEADEE645")});
    if (!IsRunningCommandlet()) return Refuse(TEXT("Offline only"));
    for (const auto& E:Protected) if (Hash(Filename(E.Name))!=E.SHA1) return Refuse(FString(TEXT("Asset drift: "))+E.Name);
    const FString Root(APSSharedLavaMaterialBuilder::Destination);
    if (FPackageName::DoesPackageExist(Root/FieldName)) return Refuse(TEXT("Private field already exists"));
    auto* Master=LoadObject<UMaterial>(nullptr,*(Root/TEXT("M_APS_SharedLava.M_APS_SharedLava")));
    auto* Native=LoadObject<UMaterialInstanceConstant>(nullptr,*(Root/TEXT("MI_APS_SharedLavaNative.MI_APS_SharedLavaNative")));
    auto* APS=LoadObject<UMaterialInstanceConstant>(nullptr,*(Root/TEXT("MI_APS_SharedLava.MI_APS_SharedLava")));
    if (!Master || !Native || !APS || Master->GetOutermost()->IsDirty() || Native->Parent.Get()!=Master
        || APS->Parent.Get()!=Native || FCore::Expressions(Master).Num()!=137) return Refuse(TEXT("Candidate chain/graph"));
    auto* Fine=FindObject<UMaterialExpressionMaterialFunctionCall>(Master,TEXT("MaterialExpressionMaterialFunctionCall_14"));
    auto* Pin=Fine ? Fine->FunctionInputs.FindByPredicate([](const FFunctionExpressionInput& P) {return P.Input.InputName==TEXT("TextureObject");}):nullptr;
    auto* Object=Pin ? Cast<UMaterialExpressionTextureObject>(Pin->Input.Expression):nullptr;
    auto* Source=Object ? Cast<UTexture2D>(Object->Texture):nullptr;
    if (!Source || Source->GetName()!=TEXT("T_APS_LavaThermal400m_RGBA_v1")) return Refuse(TEXT("Expected original packed field"));
    TMap<int32,FExpressionInput> Properties;
    for (int32 P=0; P<MP_MAX; ++P) if (auto* In=Master->GetExpressionInputForProperty(static_cast<EMaterialProperty>(P))) Properties.Add(P,*In);
    // Test periodic construction independently of compressed runtime filtering.
    double Seam=0;
    for (int32 I=0; I<=256; ++I)
    {
        const double T=double(I)/256;
        const auto A=Field(0,T), C=Field(1,T), D=Field(T,0), E=Field(T,1);
        Seam=FMath::Max(Seam,double(FMath::Max(FMath::Max(FMath::Abs(A.R-C.R),FMath::Abs(A.A-C.A)),FMath::Max(FMath::Abs(D.R-E.R),FMath::Abs(D.A-E.A)))));
    }
    if (Seam>1.e-5) return Refuse(TEXT("Nonperiodic crust field"));
    const FString Backup=FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir()/TEXT("SharedLavaCrustFieldBackup_20260927"));
    if (IFileManager::Get().DirectoryExists(*Backup) || !IFileManager::Get().MakeDirectory(*Backup,true)) return Refuse(TEXT("Immutable backup exists/unavailable"));
    for (const auto& E:Protected)
        if (Hash(Filename(E.Name))!=E.SHA1 || IFileManager::Get().Copy(*(Backup/(FString(E.Name)+TEXT(".uasset"))),*Filename(E.Name),false,false)!=COPY_OK
            || Hash(Backup/(FString(E.Name)+TEXT(".uasset")))!=E.SHA1) return Refuse(TEXT("Backup mismatch"));
    constexpr int32 Size=2048;
    TArray64<uint8> Bytes; Bytes.SetNumUninitialized(int64(Size)*Size*4);
    auto* Pixels=reinterpret_cast<FColor*>(Bytes.GetData()); double Mean=0;
    for (int32 Y=0; Y<Size; ++Y) for (int32 X=0; X<Size; ++X)
    {
        const FColor C=Field((X+.5)/Size,(Y+.5)/Size).ToFColor(true);
        Pixels[int64(Y)*Size+X]=C; Mean+=C.A/255.0;
    }
    Mean/=double(Size)*Size;
    if (!FMath::IsFinite(Mean) || Mean<.01 || Mean>.4) return Refuse(TEXT("Invalid integrated hot coverage"));
    auto* Texture=Cast<UTexture2D>(B.Duplicate(Source,FieldName));
    if (!Texture) return Refuse(TEXT("Texture duplication failed"));
    Texture->PreEditChange(nullptr); Texture->Source.Init(Size,Size,1,1,TSF_BGRA8,Bytes.GetData());
    Texture->SRGB=true; Texture->CompressionSettings=TC_BC7; Texture->CompressionNoAlpha=false; Texture->CompressionForceAlpha=true;
    Texture->MipGenSettings=TMGS_SimpleAverage; Texture->bDoScaleMipsForAlphaCoverage=false; Texture->PostEditChange();
    FAssetCompilingManager::Get().FinishAllCompilation();
    TArray64<uint8> Check;
    if (!Texture->Source.GetMipData(Check,0) || Check!=Bytes || !Texture->GetPlatformData()
        || Texture->GetPlatformData()->PixelFormat!=PF_BC7 || Texture->GetNumMips()!=12) return Refuse(TEXT("Texture source/platform mismatch"));
    auto* Parameter=B.Add<UMaterialExpressionTextureObjectParameter>(Master);
    Parameter->ParameterName=TEXT("APS_LavaCrustField"); Parameter->Texture=Source; Parameter->SamplerType=SAMPLERTYPE_Color;
    Parameter->Group=TEXT("APS Lava Surface"); Parameter->UpdateParameterGuid(true,true); Pin->Input.Connect(0,Parameter);
    if (FCore::Expressions(Master).Num()!=138) return Refuse(TEXT("Unexpected graph change"));
    for (const auto& Pair:Properties)
    {
        auto* Now=Master->GetExpressionInputForProperty(static_cast<EMaterialProperty>(Pair.Key));
        if (!Now || !APSLavaCrustReflectanceUpdate::SameInput(*Now,Pair.Value)) return Refuse(TEXT("Material output edge changed"));
    }
    Master->PostEditChange(); UMaterialInterface* Materials[]={Master,Native,APS};
    for (auto* M:Materials) M->ForceRecompileForRendering(EMaterialShaderPrecompileMode::Default);
    FAssetCompilingManager::Get().FinishAllCompilation(); if (GShaderCompilingManager) GShaderCompilingManager->FinishAllCompilation();
    for (auto* M:Materials)
    {
        auto* R=M->GetMaterialResource(GMaxRHIFeatureLevel); const auto* Map=R?R->GetGameThreadShaderMap():nullptr; UTexture* Bound=nullptr;
        if (!R || !R->IsGameThreadShaderMapComplete() || R->GetCompileErrors().Num() || !Map || !Map->GetMeshShaderMap(&FLocalVertexFactory::StaticType)
            || !M->GetTextureParameterValue(FMaterialParameterInfo(TEXT("APS_LavaCrustField")),Bound) || Bound!=Source) return Refuse(TEXT("Compile/default binding invalid"));
    }
    for (const auto& E:Protected) if (Hash(Filename(E.Name))!=E.SHA1) return Refuse(TEXT("Concurrent asset edit"));
    for (UObject* Output:{static_cast<UObject*>(Texture),static_cast<UObject*>(Master)})
    {
        auto* Package=Output->GetOutermost(); Package->MarkPackageDirty(); FSavePackageArgs Args; Args.TopLevelFlags=RF_Public|RF_Standalone; Args.SaveFlags=SAVE_NoError;
        if (!UPackage::SavePackage(Package,Output,*Filename(*Output->GetName()),Args)) return Refuse(TEXT("Save failed; retain partial output/backups"));
    }
    for (int32 I=1; I<Protected.Num(); ++I) if (Hash(Filename(Protected[I].Name))!=Protected[I].SHA1) return Refuse(TEXT("Protected asset changed"));
    UE_LOG(LogTemp,Display,TEXT("[APS.LavaCrustField] SAVED sourceSize=2048x2048 format=BC7 mips=12 sourceHotMean=%.6f sourceSeamError=%.9f originalTextureDefault=1 sameCoordinates=1 noNewTextureReads=1 geographyPaletteMaskUnchanged=1 backup=%s renderedAcceptancePending=1"),Mean,Seam,*Backup);
    return true;
}
}
#endif
