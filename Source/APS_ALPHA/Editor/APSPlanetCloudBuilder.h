#pragma once
#if WITH_EDITOR
#include "APSSharedTerrainMaterialBuilder.h"
#include "APSPlanetCloudHlsl.h"
#include "APSPlanetCloudLayeredHlsl.h"
#include "APSPlanetCloudRefinedHlsl.h"
#include "APS_ALPHA/Core/Planetary/APSPlanetCloudPolicy.h"
#include "APS_ALPHA/Core/Planetary/APSPlanetCloudWeather.h"
#include "Factories/MaterialFactoryNew.h"
#include "Materials/MaterialExpressionCameraVectorWS.h"
#include "Materials/MaterialExpressionSceneDepth.h"
#include "Materials/MaterialExpressionTwoSidedSign.h"

namespace APSPlanetCloudBuilder
{
inline bool Build(IAssetTools& Tools)
{
    if(APSPlanetCloudWeather::CandidateRequested() && APSPlanetCloudWeather::LayeredRequested())
    { UE_LOG(LogTemp,Error,TEXT("[APS.CloudBake] Select ONE isolated cloud candidate")); return false; }
    // Rio 06.10 (clouds vanish at an altitude): V33 two-crossing is a third exclusive candidate.
    const bool TwoCrossing=APSPlanetCloudWeather::TwoCrossingRequested();
    if(TwoCrossing && (APSPlanetCloudWeather::CandidateRequested() || APSPlanetCloudWeather::LayeredRequested()))
    { UE_LOG(LogTemp,Error,TEXT("[APS.CloudBake] Select ONE isolated cloud candidate")); return false; }
    using FCore = APSSharedTerrainMaterialBuilder::FBuild;
    const FString ObjectPath(APSPlanetCloudWeather::SelectedMaterialPath());
    if(TwoCrossing && ObjectPath!=APSPlanetCloudWeather::TwoCrossingMaterialPath)
    { UE_LOG(LogTemp,Error,TEXT("[APS.CloudBake] Two-crossing output must be its own V33 package")); return false; }
    const FString PackagePath = FPackageName::ObjectPathToPackageName(ObjectPath);
    const FString Folder = FPackageName::GetLongPackagePath(PackagePath);
    const FString Name = FPackageName::GetShortName(PackagePath);
    if (!IsRunningCommandlet() || FPackageName::DoesPackageExist(PackagePath))
    { UE_LOG(LogTemp,Error,TEXT("[APS.CloudBake] Offline NEW output only")); return false; }
    const bool Layered=APSPlanetCloudWeather::LayeredRequested();
    const bool Refined=APSPlanetCloudWeather::CandidateRequested();
    const FString Shader=Layered?APSPlanetCloudLayeredHlsl::Code()
        :(Refined?APSPlanetCloudRefinedHlsl::Code()
        :(TwoCrossing?APSPlanetCloudHlsl::TwoCrossingCode():APSPlanetCloudHlsl::Code()));
    if(Shader.IsEmpty())
    { UE_LOG(LogTemp,Error,TEXT("[APS.CloudBake] Candidate shader anchors changed; no asset created")); return false; }
    FCore B(Tools,*Folder);
    auto* M=Cast<UMaterial>(Tools.CreateAsset(Name,Folder,UMaterial::StaticClass(),NewObject<UMaterialFactoryNew>()));
    if (!M) return false;
    M->MaterialDomain=MD_Surface; M->BlendMode=BLEND_Translucent;
    M->SetShadingModel(MSM_Unlit); M->TwoSided=true; M->bDisableDepthTest=true;
    M->TranslucencyPass=MTP_BeforeDOF;
    auto* C=B.Add<UMaterialExpressionCustom>(M); C->Inputs.Empty();
    C->OutputType=CMOT_Float4; C->Code=Shader;
    C->Description=TEXT("Bounded spherical cloud volume; shared orbit/ground field; adaptive16-32 samples");
    const auto Input=[&](const TCHAR* Key,UMaterialExpression* Node)
    { FCustomInput I; I.InputName=Key; I.Input.Connect(0,Node); C->Inputs.Add(I); };
    const auto Scalar=[&](const TCHAR* Key,const TCHAR* Param,float Value)
    {
        auto* S=B.Add<UMaterialExpressionScalarParameter>(M); S->ParameterName=Param; S->DefaultValue=Value;
        S->UpdateParameterGuid(true,true); Input(Key,S);
    };
    const auto Vector=[&](const TCHAR* Key,const TCHAR* Param,FLinearColor Value)
    {
        auto* V=B.Add<UMaterialExpressionVectorParameter>(M); V->ParameterName=Param; V->DefaultValue=Value;
        V->UpdateParameterGuid(true,true); Input(Key,V);
    };
    auto* Center=B.Add<UMaterialExpressionDoubleVectorParameter>(M);
    Center->ParameterName=TEXT("CloudCenter"); Center->UpdateParameterGuid(true,true); Input(TEXT("Center"),Center);
    Input(TEXT("CameraVector"),B.Add<UMaterialExpressionCameraVectorWS>(M));
    // SceneDepth has no exported StaticClass in the installed engine build.
    auto* DepthClass=FindObject<UClass>(nullptr,TEXT("/Script/Engine.MaterialExpressionSceneDepth"));
    if(!DepthClass) return false;
    Input(TEXT("SceneZ"),UMaterialEditingLibrary::CreateMaterialExpression(M,DepthClass));
    Input(TEXT("Face"),B.Add<UMaterialExpressionTwoSidedSign>(M));
    Scalar(TEXT("CmPerKm"),TEXT("CloudCmPerKm"),100000);
    Scalar(TEXT("Radius"),TEXT("CloudRadiusKm"),6750);
    Scalar(TEXT("Bottom"),TEXT("CloudBottomKm"),6);
    Scalar(TEXT("Thickness"),TEXT("CloudThicknessKm"),2);
    Scalar(TEXT("Coverage"),TEXT("CloudCoverage"),.45f);
    Scalar(TEXT("CloudDensity"),TEXT("CloudDensity"),1.f);
    Scalar(TEXT("WeatherScale"),TEXT("CloudWeatherScale"),1.f);
    Scalar(TEXT("Swirl"),TEXT("CloudSwirl"),0.f);
    Scalar(TEXT("Banding"),TEXT("CloudBanding"),.25f);
    Vector(TEXT("Albedo"),TEXT("CloudAlbedo"),FLinearColor::White);
    Vector(TEXT("WindRotation"),TEXT("CloudWindRotation"),FLinearColor(1,0,0,0));
    if(Layered)
    {
        Scalar(TEXT("LayeredStyle"),TEXT("CloudLayeredStyle"),0.f);
        Vector(TEXT("LowDeck"),TEXT("CloudLowDeck"),FLinearColor::Black);
        Vector(TEXT("MidDeck"),TEXT("CloudMidDeck"),FLinearColor::Black);
        Vector(TEXT("HighDeck"),TEXT("CloudHighDeck"),FLinearColor::Black);
        Vector(TEXT("DeckCoverage"),TEXT("CloudDeckCoverage"),FLinearColor::Black);
        C->Description=TEXT("Isolated V30 three-shell clouds; one32-tap budget, ordered compositing");
    }
    if(Refined) C->Description=TEXT("Isolated V31: filtered light taps and far-only phase stability; unchanged V27 cloud field");
    if(TwoCrossing) C->Description=TEXT("Isolated V33: unchanged V27 cloud field; a ray dipping under the deck marches both crossings in one16-32 sample budget");
    Scalar(TEXT("Visibility"),TEXT("CloudVisibility"),1);
    Scalar(TEXT("Debug"),TEXT("CloudDebug"),0);
    Scalar(TEXT("Aerial"),TEXT("CloudAerial"),1);
    Scalar(TEXT("AirHeight"),TEXT("CloudAirHeightKm"),100);
    Vector(TEXT("AirOptics"),TEXT("CloudAirOptics"),FLinearColor(.014f,.015f,8.f,1.2f));
    Scalar(TEXT("AirMieHeight"),TEXT("CloudAirMieHeightKm"),1.2f);
    Scalar(TEXT("AirRadius"),TEXT("CloudAirRadiusKm"),6750);
    Vector(TEXT("AirRayleigh"),TEXT("CloudAirRayleigh"),FLinearColor(.002f,.004f,.013f,0));
    Vector(TEXT("AirMie"),TEXT("CloudAirMie"),FLinearColor(.0004f,.00024f,.00011f,0));
    Vector(TEXT("AirOzone"),TEXT("CloudAirOzone"),FLinearColor(.003f,.007f,.013f,0));
    Vector(TEXT("AirSettings"),TEXT("CloudAirSettings"),FLinearColor(1,15,1,0));
    Vector(TEXT("AirFill"),TEXT("CloudAirFill"),FLinearColor(.001601f,.002576f,.005208f,0));
    // Like SceneDepth this expression has no exported StaticClass in UE5.4.
    auto* EyeClass=FindObject<UClass>(nullptr,TEXT("/Script/Engine.MaterialExpressionEyeAdaptation"));
    if(!EyeClass) return false;
    Input(TEXT("EyeAdaptation"),UMaterialEditingLibrary::CreateMaterialExpression(M,EyeClass));
    Vector(TEXT("AxisX"),TEXT("CloudAxisX"),FLinearColor(1,0,0,0));
    Vector(TEXT("AxisY"),TEXT("CloudAxisY"),FLinearColor(0,1,0,0));
    Vector(TEXT("AxisZ"),TEXT("CloudAxisZ"),FLinearColor(0,0,1,0));
    Vector(TEXT("SeedOffset"),TEXT("CloudSeedOffset"),FLinearColor(0,0,0,0));
    Vector(TEXT("Sun"),TEXT("CloudSun"),FLinearColor(0,0,1,0));
    Vector(TEXT("SunColor"),TEXT("CloudSunColor"),FLinearColor::White);
    auto* RGB=B.Add<UMaterialExpressionComponentMask>(M); RGB->Input.Connect(0,C); RGB->R=RGB->G=RGB->B=true; RGB->A=false;
    auto* A=B.Add<UMaterialExpressionComponentMask>(M); A->Input.Connect(0,C); A->R=A->G=A->B=false; A->A=true;
    M->GetExpressionInputForProperty(MP_EmissiveColor)->Connect(0,RGB);
    M->GetExpressionInputForProperty(MP_Opacity)->Connect(0,A);
    M->PostEditChange(); M->ForceRecompileForRendering(EMaterialShaderPrecompileMode::Default);
    FAssetCompilingManager::Get().FinishAllCompilation();
    if(GShaderCompilingManager)GShaderCompilingManager->FinishAllCompilation();
    auto* R=M->GetMaterialResource(GMaxRHIFeatureLevel);
    auto* Map=R?R->GetGameThreadShaderMap():nullptr;
    if(!R || !Map || R->GetCompileErrors().Num() || !R->IsGameThreadShaderMapComplete()
        || !Map->GetMeshShaderMap(&FLocalVertexFactory::StaticType))
    { UE_LOG(LogTemp,Error,TEXT("[APS.CloudBake] Incomplete shader/LocalVF; not saved")); return false; }
    auto* Package=M->GetOutermost(); Package->MarkPackageDirty();
    FSavePackageArgs Args; Args.TopLevelFlags=RF_Public|RF_Standalone; Args.SaveFlags=SAVE_NoError;
    const FString File=FPackageName::LongPackageNameToFilename(PackagePath,FPackageName::GetAssetPackageExtension());
    const bool Saved=UPackage::SavePackage(Package,M,*File,Args);
    UE_LOG(LogTemp,Display,TEXT("[APS.CloudBake] saved=%d outputs=1 runtimeDefault=0 path=%s; rendered/cost checks pending"),Saved,*ObjectPath);
    return Saved;
}
}
#endif
