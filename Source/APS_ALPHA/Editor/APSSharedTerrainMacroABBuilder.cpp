#include "APSSharedTerrainMacroABBuilder.h"

#if WITH_EDITOR
#include "APSSharedTerrainLodABBuilder.h"
#include "Engine/Texture2D.h"
#include "Materials/MaterialExpressionTextureObject.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/SecureHash.h"

namespace APSSharedTerrainMacroABBuilder
{
    namespace
    {
        constexpr const TCHAR* Destination = TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/Diagnostics/MacroAB20260928V2");
        constexpr const TCHAR* Shared = TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/Shared/");

        // Aperiodic, planet-fixed 3D field. No wrapped UVs or axis-plane seams.
        // The inherited 400m/5.4km/20km bands and all downstream macro arithmetic
        // are retained. Divide LWC coordinates BEFORE demotion to Custom float.
        // Derivatives are evaluated before branching; unresolved noise converges
        // to the measured source mean rather than flickering at orbital distance.
        const TCHAR* Shader = TEXT(R"HLSL(
struct APSMacroField
{
    float hash(int3 cell)
    {
        uint3 p = asuint(cell);
        uint h = p.x * 1597334677u ^ p.y * 3812015801u ^ p.z * 2798796415u;
        h ^= h >> 16; h *= 2246822519u; h ^= h >> 13; h *= 3266489917u; h ^= h >> 16;
        return float(h >> 8) * (1.0 / 16777216.0);
    }
    float gradient(int3 cell, float3 d)
    {
        uint g = (uint)(hash(cell)*12.0);
        float2 v = g < 4u ? d.xy : g < 8u ? d.xz : d.yz;
        return ((g&1u)!=0u ? v.x : -v.x) + ((g&2u)!=0u ? v.y : -v.y);
    }
    float noise(float3 p)
    {
        // Four-corner tetrahedral support avoids the large square billows of
        // trilinear value noise. Integer hash has no short wrapping period.
        int3 i = (int3)floor(p + dot(p,float3(1.0/3.0,1.0/3.0,1.0/3.0)));
        float3 x = p-float3(i)+dot(float3(i),float3(1.0/6.0,1.0/6.0,1.0/6.0));
        float3 e = step(x.yzx,x);
        int3 i1 = (int3)(e*(1.0-e.zxy));
        int3 i2 = (int3)(1.0-e.zxy*(1.0-e));
        float3 x1=x-float3(i1)+1.0/6.0, x2=x-float3(i2)+1.0/3.0, x3=x-0.5;
        float4 t=max(0.6-float4(dot(x,x),dot(x1,x1),dot(x2,x2),dot(x3,x3)),0.0);
        t*=t; t*=t;
        return 32.0*dot(t,float4(gradient(i,x),gradient(i+i1,x1),gradient(i+i2,x2),gradient(i+1,x3)));
    }
};
// Fixed orthonormal rotation hides the cubic interpolation axes without changing
// normal-map projection axes. Coordinates and seeds never depend on camera/LOD.
float3 p = float3(dot(P,float3(0.36,-0.48,0.80)),
                  dot(P,float3(0.80,0.60,0.00)),
                  dot(P,float3(-0.48,0.64,0.60))) + float3(19.17,73.31,41.83);
p *= max(Frequency,1.0);
float footprint = max(length(ddx(p)),length(ddy(p)));
float w = smoothstep(500000.0,5000000.0,length(CameraDelta)*abs(Scale));
if (Mode < 0.5 || w <= 0.0) return Legacy;
float value = Mean;
if (Mode < 1.5)
{
    APSMacroField f;
    float resolved = 1.0-smoothstep(0.25,1.0,footprint);
    // Only quiet albedo modulation at long range. High-frequency structure is
    // filtered independently; never trade repeating squares for giant billows.
    float n0 = f.noise(p);
    float n1 = f.noise(p*2.03+float3(3.7,9.2,1.4));
    float resolved1 = 1.0-smoothstep(0.25,1.0,footprint*2.03);
    value = saturate(Mean+(0.65*n0*resolved+0.35*n1*resolved1)*(StdDev/0.30)*saturate(Contrast));
}
return lerp(Legacy,value.xxx,w);
)HLSL");

        struct FBuilder
        {
            using FBuild = APSSharedTerrainMaterialBuilder::FBuild;
            FBuild B;
            APSSharedTerrainNormalContinuity::TTransform<FBuild> Reader;
            explicit FBuilder(IAssetTools& Tools) : B(Tools, Destination), Reader(B) {}
            bool Fail(const FString& Error) { B.Error = Error; return false; }

            bool TextureStatistics(UTexture2D* Texture, float& Mean, float& StdDev)
            {
                TArray64<uint8> Bytes;
                if (!Texture || !Texture->Source.GetMipData(Bytes, 0)) return Fail(TEXT("Macro source pixels unavailable"));
                const auto Format = Texture->Source.GetFormat();
                if (Format != TSF_BGRA8 && Format != TSF_G8) return Fail(TEXT("Macro source pixel format changed"));
                const int64 Stride = Format == TSF_BGRA8 ? 4 : 1;
                const int64 Count = int64(Texture->Source.GetSizeX()) * Texture->Source.GetSizeY();
                if (Count <= 0 || Bytes.Num() != Count * Stride) return Fail(TEXT("Macro source pixel count changed"));
                double Sum = 0, Sum2 = 0;
                for (int64 I = 0; I < Count; ++I)
                {
                    const uint8 R = Bytes[I * Stride + (Stride == 4 ? 2 : 0)];
                    const double V = Texture->SRGB ? FLinearColor(FColor(R,R,R)).R : double(R) / 255.0;
                    Sum += V; Sum2 += V*V;
                }
                Mean = Sum / Count;
                StdDev = FMath::Sqrt(FMath::Max(0.0, Sum2 / Count - double(Mean)*Mean));
                UE_LOG(LogTemp, Display, TEXT("[APS.MacroAB] Source macro texture linear R mean=%.6f stddev=%.6f srgb=%d pixels=%lld"), Mean, StdDev, Texture->SRGB, Count);
                return true;
            }

            bool Patch(UMaterialFunction* Function)
            {
                auto Graph = Reader.Graph(Function);
                if (!B.Error.IsEmpty()) return false;
                for (UMaterialExpression* Node : Graph) Reader.Register(Function, Node);
                auto* Mode = Reader.Add<UMaterialExpressionScalarParameter>(Function);
                Mode->ParameterName = TEXT("APS_OrbitalMacroMode"); Mode->DefaultValue = 1;
                Mode->Group = TEXT("APS Diagnostic Orbital Macro"); Mode->UpdateParameterGuid(true,true);
                const auto Parameter = [&](const TCHAR* Name, float Value)
                {
                    auto* P = Reader.Add<UMaterialExpressionScalarParameter>(Function);
                    P->ParameterName = Name; P->DefaultValue = Value; P->Group = Mode->Group;
                    P->UpdateParameterGuid(true,true); return P;
                };
                auto* Frequency = Parameter(TEXT("APS_OrbitalMacroFrequency"),8.0f);
                auto* Contrast = Parameter(TEXT("APS_OrbitalMacroContrast"),0.5f);
                UMaterialExpressionDoubleVectorParameter* Scale = nullptr;
                int32 Scales = 0;
                for (UMaterialExpression* Node : Graph)
                    if (auto* P = Cast<UMaterialExpressionDoubleVectorParameter>(Node);
                        P && P->ParameterName == TEXT("APS_SharedInverseScale")) { Scale = P; ++Scales; }
                if (Scales != 1) return Fail(TEXT("Expected one physical scale in macro function"));
                auto* Camera = Reader.Add<UMaterialExpressionCameraPositionWS>(Function);
                auto* World = Reader.Add<UMaterialExpressionWorldPosition>(Function);
                World->WorldPositionShaderOffset = WPT_ExcludeAllShaderOffsets;
                auto* Delta = Reader.Add<UMaterialExpressionSubtract>(Function);
                Delta->A.Expression = Camera; Delta->B.Expression = World;
                int32 Patched = 0, Consumers = 0;
                for (UMaterialExpression* Node : Graph)
                {
                    auto* Call = Cast<UMaterialExpressionMaterialFunctionCall>(Node);
                    if (!Call || !Call->MaterialFunction || Call->MaterialFunction->GetPathName() != FString(Shared)
                        + TEXT("MF_APS_WorldAlignedTexture_a83aa78c.MF_APS_WorldAlignedTexture_a83aa78c")) continue;
                    const auto Pin = [Call](const TCHAR* Name) -> const FExpressionInput*
                    {
                        const auto* Found = Call->FunctionInputs.FindByPredicate([Name](const FFunctionExpressionInput& P)
                            { return P.Input.InputName == Name; });
                        return Found ? &Found->Input : nullptr;
                    };
                    const auto* Position = Pin(TEXT("WorldPosition"));
                    const auto* Size = Pin(TEXT("TextureSize"));
                    const auto* Object = Pin(TEXT("TextureObject"));
                    const auto* TextureNode = Object ? Cast<UMaterialExpressionTextureObject>(Object->Expression) : nullptr;
                    auto* Texture = TextureNode ? Cast<UTexture2D>(TextureNode->Texture) : nullptr;
                    if (!Position || !Position->Expression || !Size || !Size->Expression || !Texture
                        || Texture->GetPathName() != TEXT("/Engine/EngineMaterials/T_Default_MacroVariation.T_Default_MacroVariation"))
                        return Fail(TEXT("Macro sampling input contract changed"));
                    float Mean = 0, StdDev = 0;
                    if (!TextureStatistics(Texture, Mean, StdDev)) return false;
                    auto* Domain = Reader.Add<UMaterialExpressionDivide>(Function);
                    Domain->A = *Position; Domain->B = *Size;
                    auto* Replacement = Reader.Add<UMaterialExpressionCustom>(Function);
                    Replacement->Description = TEXT("APS orbital aperiodic macro v1; exact native near detail");
                    Replacement->OutputType = CMOT_Float3; Replacement->Code = Shader; Replacement->Inputs.Empty();
                    const auto Input = [Replacement](const TCHAR* Name, FExpressionInput In)
                    { FCustomInput V; V.InputName = Name; V.Input = In; Replacement->Inputs.Add(V); };
                    const auto NodeInput = [&Input](const TCHAR* Name, UMaterialExpression* E)
                    { FExpressionInput In; In.Expression = E; Input(Name, In); };
                    FExpressionInput Legacy; Legacy.Expression = Call; Legacy.OutputIndex = 2;
                    Input(TEXT("Legacy"), Legacy); NodeInput(TEXT("P"), Domain);
                    NodeInput(TEXT("CameraDelta"), Delta); NodeInput(TEXT("Mode"), Mode);
                    NodeInput(TEXT("Frequency"), Frequency); NodeInput(TEXT("Contrast"), Contrast);
                    FExpressionInput ScalePin; ScalePin.Expression = Scale; ScalePin.Mask = ScalePin.MaskR = 1;
                    Input(TEXT("Scale"), ScalePin);
                    for (const auto& V : {TPair<const TCHAR*,float>(TEXT("Mean"),Mean), TPair<const TCHAR*,float>(TEXT("StdDev"),StdDev)})
                    {
                        auto* C = Reader.Add<UMaterialExpressionConstant>(Function); C->R = V.Value; NodeInput(V.Key, C);
                    }
                    for (UMaterialExpression* Consumer : Graph)
                        for (FExpressionInput* In : Consumer->GetInputsView())
                            if (In && In->Expression == Call)
                            {
                                if (In->OutputIndex != 2) return Fail(TEXT("Macro consumer no longer uses XYZ texture"));
                                In->Expression = Replacement; In->OutputIndex = 0; ++Consumers;
                            }
                    ++Patched;
                }
                if (Patched != 3 || Consumers != 3) return Fail(FString::Printf(TEXT("Expected three macro bands/consumers; got %d/%d"), Patched, Consumers));
                return B.Error.IsEmpty();
            }

            bool Run()
            {
                auto* Source = LoadObject<UMaterial>(nullptr, *(FString(Shared) + TEXT("M_APS_SharedWorldScapeTerrain.M_APS_SharedWorldScapeTerrain")));
                auto* Template = LoadObject<UMaterialInstanceConstant>(nullptr, *(FString(Shared) + TEXT("MI_APS_SharedTerra.MI_APS_SharedTerra")));
                if (!Source || !Template || Template->Parent != Source) return Fail(TEXT("Current shared master/template missing"));
                auto* Master = Cast<UMaterial>(B.Duplicate(Source, TEXT("M_APS_MacroTerrain")));
                if (!Master) return false;
                auto MasterGraph = Reader.Graph(Master);
                for (UMaterialExpression* Node : MasterGraph) Reader.Register(Master, Node);
                int32 Patched = 0;
                for (UMaterialExpression* Node : MasterGraph)
                {
                    auto* Call = Cast<UMaterialExpressionMaterialFunctionCall>(Node);
                    auto* Macro = Call ? Cast<UMaterialFunction>(Call->MaterialFunction) : nullptr;
                    if (!Macro || Macro->GetPathName() != FString(Shared) + TEXT("MF_APS_MF_MacroVariationBlock_50169517.MF_APS_MF_MacroVariationBlock_50169517")) continue;
                    auto* Copy = Cast<UMaterialFunction>(B.Duplicate(Macro, TEXT("MF_APS_AperiodicMacro")));
                    if (!Copy || !Patch(Copy)) return false;
                    UMaterialEditingLibrary::UpdateMaterialFunction(Copy);
                    if (!B.ReconnectFunctionById(Call, Macro, Copy)) return false;
                    ++Patched;
                }
                if (Patched != 1) return Fail(TEXT("Expected one shared macro call"));
                auto* Instance = Cast<UMaterialInstanceConstant>(B.Duplicate(Template, TEXT("MI_APS_MacroTerra")));
                if (!Instance) return false;
                Instance->SetParentEditorOnly(Master,false);
                Instance->CopyMaterialUniformParametersEditorOnly(Template,true);
                Instance->PostEditChange(); Master->PostEditChange();
                APSSharedTerrainLodABBuilder::FBuilder PinRepair(B.AssetTools);
                if (!PinRepair.RestoreTransientFunctionPins(Master)) return Fail(PinRepair.B.Error);
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
                            FString Error = TEXT("Candidate shader incomplete: ") + Material->GetPathName();
                            if (Resource) for (const FString& E : Resource->GetCompileErrors()) Error += TEXT("\n") + E;
                            return Fail(Error);
                        }
                    }
                }
                for (UObject* Output : B.Outputs)
                {
                    UPackage* Package = Output->GetOutermost(); Package->MarkPackageDirty();
                    FSavePackageArgs Args; Args.TopLevelFlags = RF_Public | RF_Standalone; Args.SaveFlags = SAVE_NoError;
                    if (!UPackage::SavePackage(Package,Output,*FPackageName::LongPackageNameToFilename(Package->GetName(),FPackageName::GetAssetPackageExtension()),Args))
                        return Fail(TEXT("Candidate save failed"));
                }
                UE_LOG(LogTemp,Display,TEXT("[APS.MacroAB] Saved isolated candidate only: three aperiodic macro bands, native below 5km, blend 5..50km, zero added texture samples; no production selection or visual acceptance"));
                return true;
            }
        };
    }

    bool Build(IAssetTools& Tools)
    {
        FBuilder Builder(Tools);
        const bool Result = Builder.Run();
        if (!Result) UE_LOG(LogTemp,Error,TEXT("[APS.MacroAB] Refused: %s"),*Builder.B.Error);
        return Result;
    }

    bool Promote(IAssetTools& Tools)
    {
        const auto Refuse = [](const FString& Error)
        { UE_LOG(LogTemp,Error,TEXT("[APS.MacroInstall] Refused: %s"),*Error); return false; };
        if (!IsRunningCommandlet()) return Refuse(TEXT("Offline commandlet only"));
        const auto Hash = [](const FString& File)
        {
            TArray<uint8> Bytes;
            if (!FFileHelper::LoadFileToArray(Bytes,*File)) return FString();
            return FSHA1::HashBuffer(Bytes.GetData(),Bytes.Num()).ToString().ToUpper();
        };
        struct FProtected { const TCHAR* Name; const TCHAR* SHA1; };
        const FProtected Protected[] = {
            {TEXT("M_APS_SharedWorldScapeTerrain"),TEXT("0C5D1F194936CCC5B332D0A26000BDD7D77E57FF")},
            {TEXT("MF_APS_MF_MacroVariationBlock_50169517"),TEXT("3FD8F36F4DC8A117BFFD9A8E1978CC8126137BBC")},
            {TEXT("MI_APS_SharedTerra"),TEXT("D05DBA6349ED39245B7BE1A0937F8225DBC84598")},
            {TEXT("MI_APS_SharedMagma"),TEXT("A198333D905DA52AA96FFFC3EBF988690062AAB5")}
        };
        const auto File = [](const TCHAR* Name)
        { return FPackageName::LongPackageNameToFilename(FString(Shared)+Name,FPackageName::GetAssetPackageExtension()); };
        TArray<UObject*> Objects;
        for (const FProtected& P : Protected)
        {
            if (Hash(File(P.Name)) != P.SHA1) return Refuse(TEXT("Accepted shared asset changed: ")+File(P.Name));
            auto* Object = LoadObject<UObject>(nullptr,*(FString(Shared)+P.Name+TEXT(".")+P.Name));
            if (!Object || Object->GetOutermost()->IsDirty()) return Refuse(TEXT("Missing/dirty shared object"));
            Objects.Add(Object);
        }
        const FString CandidatePackage = FString(Destination)+TEXT("/MF_APS_AperiodicMacro");
        if (Hash(FPackageName::LongPackageNameToFilename(CandidatePackage,FPackageName::GetAssetPackageExtension()))
            != TEXT("CF1CF65D8C3413DF8EB1D2C28CD884DA38F2A750")) return Refuse(TEXT("Rendered V2 candidate hash changed"));
        auto* Candidate = LoadObject<UMaterialFunction>(nullptr,*(CandidatePackage+TEXT(".MF_APS_AperiodicMacro")));
        auto* Master = Cast<UMaterial>(Objects[0]);
        auto* OriginalMacro = Cast<UMaterialFunction>(Objects[1]);
        auto* Terra = Cast<UMaterialInstanceConstant>(Objects[2]);
        auto* Magma = Cast<UMaterialInstanceConstant>(Objects[3]);
        if (!Candidate || !Master || !OriginalMacro || !Terra || !Magma || Terra->Parent != Master || Magma->Parent != Master)
            return Refuse(TEXT("Shared hierarchy changed"));
        FBuilder Builder(Tools);
        auto& B = Builder.B; auto& Reader = Builder.Reader;
        B.DestinationRoot = APSSharedTerrainMaterialBuilder::OutputRoot;
        const FString Backup = FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir()/TEXT("SharedTerrainMacroBackup20260928"));
        if (IFileManager::Get().DirectoryExists(*Backup)) return Refuse(TEXT("Backup already exists; inspect previous installation"));
        if (!IFileManager::Get().MakeDirectory(*Backup,true)) return Refuse(TEXT("Cannot create immutable backup"));
        for (const FProtected& P : Protected)
        {
            const FString Dest = Backup/(FString(P.Name)+TEXT(".uasset"));
            if (IFileManager::Get().Copy(*Dest,*File(P.Name),false,false) != COPY_OK || Hash(Dest) != P.SHA1)
                return Refuse(TEXT("Backup verification failed before mutation"));
        }
        auto* Copy = Cast<UMaterialFunction>(B.Duplicate(Candidate,TEXT("MF_APS_OrbitalMacroV2")));
        if (!Copy) return Refuse(B.Error);
        int32 Modes = 0, Fields = 0;
        for (UMaterialExpression* Node : Reader.Graph(Copy))
        {
            Reader.Register(Copy,Node);
            if (auto* P = Cast<UMaterialExpressionScalarParameter>(Node);
                P && P->ParameterName == TEXT("APS_OrbitalMacroMode"))
            {
                // Opt-in only for individually validated generated families.
                // All other families/manual references retain native macro output.
                P->DefaultValue = 0; ++Modes;
            }
            if (auto* C = Cast<UMaterialExpressionCustom>(Node);
                C && C->Description == TEXT("APS orbital aperiodic macro v1; exact native near detail"))
            {
                if (C->Code != Shader) return Refuse(TEXT("Rendered shader differs from current source"));
                ++Fields;
            }
        }
        if (!B.Error.IsEmpty() || Modes != 1 || Fields != 3) return Refuse(TEXT("Candidate graph contract changed"));
        UMaterialEditingLibrary::UpdateMaterialFunction(Copy);
        int32 Calls = 0;
        for (UMaterialExpression* Node : Reader.Graph(Master))
        {
            Reader.Register(Master,Node);
            if (auto* Call = Cast<UMaterialExpressionMaterialFunctionCall>(Node); Call && Call->MaterialFunction == OriginalMacro)
            {
                if (!B.ReconnectFunctionById(Call,OriginalMacro,Copy)) return Refuse(B.Error);
                ++Calls;
            }
        }
        if (Calls != 1 || !B.Error.IsEmpty()) return Refuse(TEXT("Expected one macro call in production master"));
        Master->PostEditChange();
        APSSharedTerrainLodABBuilder::FBuilder PinRepair(Tools);
        if (!PinRepair.RestoreTransientFunctionPins(Master)) return Refuse(PinRepair.B.Error);
        for (UMaterialInterface* M : {static_cast<UMaterialInterface*>(Master),static_cast<UMaterialInterface*>(Terra),static_cast<UMaterialInterface*>(Magma)})
            M->ForceRecompileForRendering(EMaterialShaderPrecompileMode::Default);
        FAssetCompilingManager::Get().FinishAllCompilation();
        if (GShaderCompilingManager) GShaderCompilingManager->FinishAllCompilation();
        for (UMaterialInterface* M : {static_cast<UMaterialInterface*>(Master),static_cast<UMaterialInterface*>(Terra),static_cast<UMaterialInterface*>(Magma)})
        {
            FMaterialResource* R = M->GetMaterialResource(GMaxRHIFeatureLevel);
            const FMaterialShaderMap* Map = R ? R->GetGameThreadShaderMap() : nullptr;
            if (!R || !R->IsGameThreadShaderMapComplete() || R->GetCompileErrors().Num() || !Map || !Map->GetMeshShaderMap(&FLocalVertexFactory::StaticType))
                return Refuse(TEXT("Incomplete shared shader: ")+M->GetPathName());
        }
        // A second writer must never be overwritten, even during compilation.
        for (const FProtected& P : Protected)
            if (Hash(File(P.Name)) != P.SHA1) return Refuse(TEXT("Shared file changed during compilation: ")+File(P.Name));
        // Save the self-contained function first, then the only replaced asset.
        for (UObject* Output : {static_cast<UObject*>(Copy),static_cast<UObject*>(Master)})
        {
            UPackage* Package = Output->GetOutermost(); Package->MarkPackageDirty();
            FSavePackageArgs Args; Args.TopLevelFlags=RF_Public|RF_Standalone; Args.SaveFlags=SAVE_NoError;
            const FString Filename=FPackageName::LongPackageNameToFilename(Package->GetName(),FPackageName::GetAssetPackageExtension());
            if (!UPackage::SavePackage(Package,Output,*Filename,Args)) return Refuse(TEXT("Save failed; verified backup: ")+Backup);
            UE_LOG(LogTemp,Display,TEXT("[APS.MacroInstall] Saved %s SHA1=%s"),*Filename,*Hash(Filename));
        }
        for (int32 I=1; I<UE_ARRAY_COUNT(Protected); ++I)
            if (Hash(File(Protected[I].Name)) != Protected[I].SHA1) return Refuse(TEXT("Protected source/template changed unexpectedly"));
        UE_LOG(LogTemp,Display,TEXT("[APS.MacroInstall] Installed V2 macro only, default OFF, native/detail/layer/geometry contracts retained. Backup=%s; runtime verification still required"),*Backup);
        return true;
    }
}
#endif
