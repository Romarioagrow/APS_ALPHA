#include "APSGasGiantAssetCommandlet.h"

#if WITH_EDITOR

#include "AssetToolsModule.h"
#include "Factories/MaterialFactoryNew.h"
#include "IAssetTools.h"
#include "MaterialEditingLibrary.h"
#include "MaterialDomain.h"
#include "Materials/Material.h"
#include "Materials/MaterialExpressionConstant.h"
#include "Materials/MaterialExpressionCustom.h"
#include "Materials/MaterialExpressionScalarParameter.h"
#include "Materials/MaterialExpressionTransform.h"
#include "Materials/MaterialExpressionVectorParameter.h"
#include "Materials/MaterialExpressionVertexNormalWS.h"
#include "Misc/PackageName.h"
#include "Modules/ModuleManager.h"
#include "UObject/Package.h"
#include "UObject/SavePackage.h"

namespace APSGasGiantAssets
{
	const TCHAR* MaterialFolder = TEXT("/Game/APS/APS_ALPHA/Assets/Materials");
	const TCHAR* MaterialName = TEXT("M_APS_GasGiantAtmosphere");

	template <typename T>
	T* Add(UMaterial* Material, int32 X, int32 Y)
	{
		return Cast<T>(UMaterialEditingLibrary::CreateMaterialExpression(
			Material, T::StaticClass(), X, Y));
	}

	void Input(UMaterialExpressionCustom* Custom, const TCHAR* Name, UMaterialExpression* Node)
	{
		FCustomInput& Item = Custom->Inputs.AddDefaulted_GetRef();
		Item.InputName = Name;
		Item.Input.Connect(0, Node);
	}

	UMaterialExpressionScalarParameter* Scalar(UMaterial* Material,
		const TCHAR* Name, float Value, int32 Y)
	{
		auto* Node = Add<UMaterialExpressionScalarParameter>(Material, -800, Y);
		Node->ParameterName = Name;
		Node->DefaultValue = Value;
		Node->Group = TEXT("Gas Giant Clouds");
		Node->UpdateParameterGuid(true, true);
		return Node;
	}

	UMaterialExpressionVectorParameter* Color(UMaterial* Material,
		const TCHAR* Name, const FLinearColor& Value, int32 Y)
	{
		auto* Node = Add<UMaterialExpressionVectorParameter>(Material, -800, Y);
		Node->ParameterName = Name;
		Node->DefaultValue = Value;
		Node->Group = TEXT("Gas Giant Clouds");
		Node->UpdateParameterGuid(true, true);
		return Node;
	}

	bool Build(UMaterial* Material)
	{
		// UE 5.4's bulk deletion helper mutates its own iteration array. Use the
		// established tail-delete pattern so rerunning this exact-asset bake is safe.
		while (!Material->GetExpressions().IsEmpty())
		{
			UMaterialExpression* Expression = Material->GetExpressions().Last();
			if (IsValid(Expression) && Expression->IsRooted()) Expression->RemoveFromRoot();
			UMaterialEditingLibrary::DeleteMaterialExpression(Material, Expression);
		}
		Material->MaterialDomain = MD_Surface;
		Material->BlendMode = BLEND_Opaque;
		Material->SetShadingModel(MSM_DefaultLit);
		Material->TwoSided = false;
		Material->bTangentSpaceNormal = true;

		// Object-space unit normals avoid spherical UV seams, pole singularities,
		// absolute-world-position precision loss and scale-dependent cloud frequency.
		auto* Normal = Add<UMaterialExpressionVertexNormalWS>(Material, -1200, -500);
		auto* LocalNormal = Add<UMaterialExpressionTransform>(Material, -1000, -500);
		LocalNormal->TransformSourceType = TRANSFORMSOURCE_World;
		LocalNormal->TransformType = TRANSFORM_Local;
		LocalNormal->Input.Connect(0, Normal);
		auto* Clouds = Add<UMaterialExpressionCustom>(Material, -300, -200);
		Clouds->Description = TEXT("Seamless gas cloud belts and oval storms");
		Clouds->OutputType = CMOT_Float3;
		Clouds->Inputs.Reset();
		Input(Clouds, TEXT("DirectionLocal"), LocalNormal);
		Input(Clouds, TEXT("LightCloud"), Color(Material, TEXT("GasCloudLight"),
			FLinearColor(0.78f, 0.68f, 0.49f), -320));
		Input(Clouds, TEXT("DarkCloud"), Color(Material, TEXT("GasCloudDark"),
			FLinearColor(0.24f, 0.13f, 0.065f), -120));
		Input(Clouds, TEXT("StormColor"), Color(Material, TEXT("GasStormColor"),
			FLinearColor(0.50f, 0.20f, 0.085f), 80));
		Input(Clouds, TEXT("Seed"), Scalar(Material, TEXT("GasPatternSeed"), 17.0f, 280));
		Input(Clouds, TEXT("Contrast"), Scalar(Material, TEXT("GasBandContrast"), 0.78f, 400));
		Input(Clouds, TEXT("StormStrength"), Scalar(Material, TEXT("GasStormStrength"), 0.85f, 520));

		// Each literal remains below MSVC's 4095-wide-character limit. No time/WPO:
		// cloud structure stays attached to the body and stable between temporal samples.
		const FString NoiseCode = TEXT(R"APSGAS(
struct FGasNoise
{
    float Hash(float3 p)
    {
        p = frac(p * 0.1031);
        p += dot(p, p.yzx + 33.33);
        return frac((p.x + p.y) * p.z);
    }
    float Noise(float3 p)
    {
        float3 i = floor(p);
        float3 f = frac(p);
        f = f * f * (3.0 - 2.0 * f);
        return lerp(lerp(lerp(Hash(i), Hash(i + float3(1,0,0)), f.x),
                         lerp(Hash(i + float3(0,1,0)), Hash(i + float3(1,1,0)), f.x), f.y),
                    lerp(lerp(Hash(i + float3(0,0,1)), Hash(i + float3(1,0,1)), f.x),
                         lerp(Hash(i + float3(0,1,1)), Hash(i + float3(1,1,1)), f.x), f.y), f.z);
    }
};
FGasNoise N;
// Remove uniform scale before the dot product in normalize to avoid underflow
// for astronomical actor transforms. This is a vector: no world translation enters.
float maxDirection = max(max(abs(DirectionLocal.x), abs(DirectionLocal.y)), abs(DirectionLocal.z));
float3 q = normalize(DirectionLocal / max(maxDirection, 1.0e-30));
float phase = frac(Seed * 0.0618034) * 6.2831853;
float cs = cos(phase), sn = sin(phase);
q.xy = float2(cs * q.x - sn * q.y, sn * q.x + cs * q.y);
float3 seedOffset = float3(frac(Seed * 0.137), frac(Seed * 0.319), frac(Seed * 0.731)) * 23.0;
float broad = N.Noise(q * 6.0 + seedOffset);
float middle = N.Noise(q * 19.0 + seedOffset.yzx);
float detail = N.Noise(q * 61.0 + seedOffset.zxy);
float footprint = max(length(ddx(q)), length(ddy(q)));
float detailWeight = 1.0 - smoothstep(0.006, 0.035, footprint);
float middleWeight = 1.0 - smoothstep(0.03, 0.12, footprint);
middle = lerp(0.5, middle, middleWeight);
detail = lerp(0.5, detail, detailWeight);
)APSGAS");
		const FString CloudCode = TEXT(R"APSGAS(
// A spherical oval with a smoothly decaying rotating edge, never a UV decal seam.
float stormLatitude = -0.25 + frac(Seed * 0.017) * 0.12;
float3 stormCenter = normalize(float3(1.0, 0.0, stormLatitude));
float3 north = normalize(cross(stormCenter, float3(0,1,0)));
float2 oval = float2(dot(q, float3(0,1,0)) / 0.18, dot(q, north) / 0.075);
float radius2 = dot(oval, oval);
float visibleSide = smoothstep(0.75, 0.93, dot(q, stormCenter));
float stormEnvelope = exp(-radius2 * 0.55) * visibleSide * StormStrength;
float angle = atan2(oval.y, oval.x);
float swirl = sin(angle + radius2 * 2.8) * stormEnvelope;
float latitude = q.z + (broad - 0.5) * 0.047 + (middle - 0.5) * 0.019 + swirl * 0.024;
float beltPhase = latitude * 58.0 + (broad - 0.5) * 1.2;
float beltWeight = 1.0 - smoothstep(0.045, 0.16, footprint);
float belt = 0.5 + 0.30 * sin(beltPhase) * beltWeight;
belt += 0.14 * sin(latitude * 119.0 + middle * 2.0) * middleWeight;
belt += 0.065 * (detail - 0.5) * detailWeight;
belt = saturate(0.55 + (belt - 0.5) * Contrast);
float3 cloudColor = lerp(DarkCloud, LightCloud, smoothstep(0.1, 0.94, belt));
cloudColor *= 0.94 + 0.12 * middle + 0.045 * (detail - 0.5);
float stormCore = exp(-radius2 * 1.35) * visibleSide * StormStrength;
cloudColor = lerp(cloudColor, StormColor * (0.88 + 0.24 * middle), saturate(stormCore * 0.87));
// Filter narrow storm rings as they become smaller than a pixel.
float ring = exp(-abs(radius2 - 1.05) * 4.0) * visibleSide * StormStrength;
ring *= 1.0 - smoothstep(0.15, 1.0, fwidth(radius2));
cloudColor = lerp(cloudColor, LightCloud, ring * 0.28);
float polarHaze = smoothstep(0.70, 0.97, abs(q.z));
cloudColor = lerp(cloudColor, lerp(DarkCloud, LightCloud, 0.60), polarHaze * 0.32);
return saturate(cloudColor);
)APSGAS");
		Clouds->Code = NoiseCode + CloudCode;

		auto* Roughness = Scalar(Material, TEXT("GasCloudRoughness"), 0.86f, 700);
		auto* Metallic = Add<UMaterialExpressionConstant>(Material, -300, 650);
		Metallic->R = 0.0f;
		auto* Specular = Add<UMaterialExpressionConstant>(Material, -300, 750);
		Specular->R = 0.22f;
		// Default-lit cloud tops retain the real sun/terminator. No emissive fill,
		// displacement, shiny rocky normals or transparent overlap layers are added.
		const bool bConnected =
			UMaterialEditingLibrary::ConnectMaterialProperty(Clouds, TEXT(""), MP_BaseColor)
			&& UMaterialEditingLibrary::ConnectMaterialProperty(Roughness, TEXT(""), MP_Roughness)
			&& UMaterialEditingLibrary::ConnectMaterialProperty(Metallic, TEXT(""), MP_Metallic)
			&& UMaterialEditingLibrary::ConnectMaterialProperty(Specular, TEXT(""), MP_Specular);
		if (!bConnected) return false;
		Material->UpdateCachedExpressionData();
		Material->PostEditChange();
		UMaterialEditingLibrary::RecompileMaterial(Material);
		Material->ForceRecompileForRendering();
		Material->EnsureIsComplete();
		const FMaterialStatistics Statistics = UMaterialEditingLibrary::GetStatistics(Material);
		if (Statistics.NumPixelShaderInstructions <= 0 || Statistics.NumVertexShaderInstructions <= 0)
		{
			UE_LOG(LogTemp, Error, TEXT("[APS.GasGiant] Material has no usable compiled shader resource"));
			return false;
		}
		UE_LOG(LogTemp, Display, TEXT("[APS.GasGiant] Shader resource ready pixelInstructions=%d vertexInstructions=%d samplers=%d"),
			Statistics.NumPixelShaderInstructions, Statistics.NumVertexShaderInstructions, Statistics.NumSamplers);
		return true;
	}
}
#endif // WITH_EDITOR

UAPSGasGiantAssetCommandlet::UAPSGasGiantAssetCommandlet()
{
	IsClient = false;
	IsServer = false;
	IsEditor = true;
	LogToConsole = true;
}

int32 UAPSGasGiantAssetCommandlet::Main(const FString& Params)
{
#if WITH_EDITOR
	using namespace APSGasGiantAssets;
	const FString ObjectPath = FString(MaterialFolder) / MaterialName + TEXT(".") + MaterialName;
	UMaterial* Material = LoadObject<UMaterial>(nullptr, *ObjectPath, nullptr, LOAD_NoWarn);
	if (!Material)
	{
		IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools")).Get();
		Material = Cast<UMaterial>(AssetTools.CreateAsset(MaterialName, MaterialFolder,
			UMaterial::StaticClass(), NewObject<UMaterialFactoryNew>()));
	}
	if (!IsValid(Material) || !Build(Material))
	{
		UE_LOG(LogTemp, Error, TEXT("[APS.GasGiant] Could not build %s"), *ObjectPath);
		return 1;
	}
	UPackage* Package = Material->GetOutermost();
	Package->MarkPackageDirty();
	FSavePackageArgs Args;
	Args.TopLevelFlags = RF_Public | RF_Standalone;
	Args.SaveFlags = SAVE_NoError;
	Args.Error = GError;
	const FString Filename = FPackageName::LongPackageNameToFilename(
		Package->GetName(), FPackageName::GetAssetPackageExtension());
	const bool bSaved = UPackage::SavePackage(Package, Material, *Filename, Args);
	UE_LOG(LogTemp, Display, TEXT("[APS.GasGiant] Material saved=%d path=%s; no worlds or imported assets changed"),
		bSaved ? 1 : 0, *ObjectPath);
	return bSaved ? 0 : 1;
#else
	UE_LOG(LogTemp, Error, TEXT("[APS.GasGiant] This asset commandlet requires an Editor build."));
	return 1;
#endif
}
