#include "APSFixStarHISMMaterialCommandlet.h"

#include "MaterialEditingLibrary.h"
#include "MaterialDomain.h"
#include "Materials/Material.h"
#include "Materials/MaterialExpressionCameraVectorWS.h"
#include "Materials/MaterialExpressionCustom.h"
#include "Materials/MaterialExpressionScalarParameter.h"
#include "Materials/MaterialExpressionVectorParameter.h"
#include "Materials/MaterialExpressionVertexNormalWS.h"
#include "Misc/PackageName.h"
#include "UObject/Package.h"
#include "UObject/SavePackage.h"
#include "UObject/UnrealType.h"
#include "UObject/UObjectGlobals.h"

DEFINE_LOG_CATEGORY_STATIC(LogAPSStarMaterialFix, Log, All);

#if WITH_EDITOR
namespace APSStellarMaterial
{
	template <typename TExpression>
	TExpression* AddExpression(UMaterial* Material, const int32 X, const int32 Y)
	{
		return Cast<TExpression>(UMaterialEditingLibrary::CreateMaterialExpression(
			Material, TExpression::StaticClass(), X, Y));
	}

	UMaterialExpressionVectorParameter* AddVectorParameter(
		UMaterial* Material,
		const TCHAR* Name,
		const FLinearColor& DefaultValue,
		const int32 X,
		const int32 Y,
		const int32 SortPriority)
	{
		UMaterialExpressionVectorParameter* Parameter =
			AddExpression<UMaterialExpressionVectorParameter>(Material, X, Y);
		if (!Parameter)
		{
			return nullptr;
		}
		Parameter->ParameterName = Name;
		Parameter->DefaultValue = DefaultValue;
		Parameter->Group = TEXT("APS Stellar Surface");
		Parameter->SortPriority = SortPriority;
		Parameter->UpdateParameterGuid(true, true);
		return Parameter;
	}

	UMaterialExpressionScalarParameter* AddScalarParameter(
		UMaterial* Material,
		const TCHAR* Name,
		const float DefaultValue,
		const float Minimum,
		const float Maximum,
		const int32 X,
		const int32 Y,
		const int32 SortPriority)
	{
		UMaterialExpressionScalarParameter* Parameter =
			AddExpression<UMaterialExpressionScalarParameter>(Material, X, Y);
		if (!Parameter)
		{
			return nullptr;
		}
		Parameter->ParameterName = Name;
		Parameter->DefaultValue = DefaultValue;
		Parameter->SliderMin = Minimum;
		Parameter->SliderMax = Maximum;
		Parameter->Group = TEXT("APS Stellar Surface");
		Parameter->SortPriority = SortPriority;
		Parameter->UpdateParameterGuid(true, true);
		return Parameter;
	}

	void AddCustomInput(
		UMaterialExpressionCustom* Custom,
		const TCHAR* Name,
		UMaterialExpression* Expression,
		const int32 OutputIndex = 0)
	{
		FCustomInput& Input = Custom->Inputs.AddDefaulted_GetRef();
		Input.InputName = Name;
		Input.Input.Connect(OutputIndex, Expression);
	}

	UMaterialExpression* AddReflectedExpression(
		UMaterial* Material,
		const TCHAR* ClassPath,
		const int32 X,
		const int32 Y)
	{
		UClass* ExpressionClass = FindObject<UClass>(nullptr, ClassPath);
		return ExpressionClass
			? UMaterialEditingLibrary::CreateMaterialExpression(Material, ExpressionClass, X, Y)
			: nullptr;
	}

	bool SetUInt32Property(UObject* Object, const FName PropertyName, const uint32 Value)
	{
		if (!Object)
		{
			return false;
		}
		FUInt32Property* Property = FindFProperty<FUInt32Property>(
			Object->GetClass(), PropertyName);
		if (!Property)
		{
			return false;
		}
		Property->SetPropertyValue_InContainer(Object, Value);
		return true;
	}

	bool SetFloatProperty(UObject* Object, const FName PropertyName, const float Value)
	{
		if (!Object)
		{
			return false;
		}
		FFloatProperty* Property = FindFProperty<FFloatProperty>(
			Object->GetClass(), PropertyName);
		if (!Property)
		{
			return false;
		}
		Property->SetPropertyValue_InContainer(Object, Value);
		return true;
	}

	bool SetLinearColorProperty(
		UObject* Object, const FName PropertyName, const FLinearColor& Value)
	{
		if (!Object)
		{
			return false;
		}
		FStructProperty* Property = FindFProperty<FStructProperty>(
			Object->GetClass(), PropertyName);
		if (!Property || Property->Struct != TBaseStructure<FLinearColor>::Get())
		{
			return false;
		}
		*Property->ContainerPtrToValuePtr<FLinearColor>(Object) = Value;
		return true;
	}

	bool RebuildUnifiedStellarMaterial(UMaterial* Material)
	{
		if (!Material)
		{
			return false;
		}

		Material->Modify();
		// UE 5.4's DeleteAllMaterialExpressions iterates the live array while each
		// deletion removes from it, which can leave every second legacy node behind.
		// Remove from the tail until the collection is genuinely empty so rebuilding
		// an existing master is idempotent and cannot retain old texture samples.
		while (!Material->GetExpressions().IsEmpty())
		{
			UMaterialEditingLibrary::DeleteMaterialExpression(
				Material, Material->GetExpressions().Last());
		}
		Material->MaterialDomain = MD_Surface;
		Material->BlendMode = BLEND_Opaque;
		Material->SetShadingModel(MSM_Unlit);
		Material->TwoSided = false;
		Material->DitheredLODTransition = false;
		Material->DitherOpacityMask = false;
		Material->bUsedWithInstancedStaticMeshes = true;

		UMaterialExpressionVectorParameter* Color = AddVectorParameter(
			Material, TEXT("Color"), FLinearColor(1.0f, 0.62f, 0.28f), -1250, -620, 0);
		UMaterialExpressionScalarParameter* Multiplier = AddScalarParameter(
			Material, TEXT("Multiplier"), 18.0f, 0.0f, 100000.0f, -1250, -520, 1);
		UMaterialExpressionScalarParameter* SurfaceSeed = AddScalarParameter(
			Material, TEXT("SurfaceSeed"), 0.371f, 0.0f, 1.0f, -1250, -420, 2);
		UMaterialExpressionScalarParameter* SurfaceVariation = AddScalarParameter(
			Material, TEXT("SurfaceVariation"), 0.22f, 0.0f, 1.0f, -1250, -320, 3);
		UMaterialExpressionScalarParameter* GranulationStrength = AddScalarParameter(
			Material, TEXT("GranulationStrength"), 0.28f, 0.0f, 1.0f, -1250, -220, 4);
		UMaterialExpressionScalarParameter* SpotStrength = AddScalarParameter(
			Material, TEXT("SpotStrength"), 0.24f, 0.0f, 1.0f, -1250, -120, 5);
		UMaterialExpressionScalarParameter* CoronaStrength = AddScalarParameter(
			Material, TEXT("CoronaStrength"), 0.34f, 0.0f, 1.5f, -1250, -20, 6);

		// UE 5.4 declares these expression classes without ENGINE_API. Referencing
		// their StaticClass symbols from a game module links on some source builds but
		// fails on installed engines. Resolve the registered UClasses and their simple
		// properties reflectively while preserving the exact native material nodes.
		UMaterialExpression* InstanceColor = AddReflectedExpression(Material,
			TEXT("/Script/Engine.MaterialExpressionPerInstanceCustomData3Vector"), -950, -620);
		UMaterialExpression* InstanceEmission = AddReflectedExpression(Material,
			TEXT("/Script/Engine.MaterialExpressionPerInstanceCustomData"), -950, -510);
		UMaterialExpression* InstanceSeed = AddReflectedExpression(Material,
			TEXT("/Script/Engine.MaterialExpressionPerInstanceCustomData"), -950, -400);
		UMaterialExpression* SystemHighlight = AddReflectedExpression(Material,
			TEXT("/Script/Engine.MaterialExpressionPerInstanceCustomData"), -950, -290);
		UMaterialExpressionVertexNormalWS* Normal =
			AddExpression<UMaterialExpressionVertexNormalWS>(Material, -950, -150);
		UMaterialExpressionCameraVectorWS* Camera =
			AddExpression<UMaterialExpressionCameraVectorWS>(Material, -950, -40);
		UMaterialExpressionCustom* StellarSurface =
			AddExpression<UMaterialExpressionCustom>(Material, -450, -320);

		if (!Color || !Multiplier || !SurfaceSeed || !SurfaceVariation || !GranulationStrength
			|| !SpotStrength || !CoronaStrength || !InstanceColor || !InstanceEmission
			|| !InstanceSeed || !SystemHighlight || !Normal || !Camera || !StellarSurface)
		{
			return false;
		}

		// RGB, luminosity/emission, deterministic seed and the potential-system bit
		// already occupy custom-data slots 0..5 on both Galaxy and Cluster HISM.
		// A zero instance colour identifies an ordinary static-mesh star, allowing the
		// exact same shader recipe to consume dynamic material parameters instead.
		if (!SetUInt32Property(InstanceColor, TEXT("DataIndex"), 0)
			|| !SetLinearColorProperty(InstanceColor, TEXT("ConstDefaultValue"), FLinearColor::Black)
			|| !SetUInt32Property(InstanceEmission, TEXT("DataIndex"), 3)
			|| !SetFloatProperty(InstanceEmission, TEXT("ConstDefaultValue"), 0.0f)
			|| !SetUInt32Property(InstanceSeed, TEXT("DataIndex"), 4)
			|| !SetFloatProperty(InstanceSeed, TEXT("ConstDefaultValue"), 0.0f)
			|| !SetUInt32Property(SystemHighlight, TEXT("DataIndex"), 5)
			|| !SetFloatProperty(SystemHighlight, TEXT("ConstDefaultValue"), 0.0f))
		{
			return false;
		}

		StellarSurface->Description = TEXT("APS unified scale-independent stellar surface");
		StellarSurface->OutputType = CMOT_Float3;
		// A fresh Custom node owns one unnamed placeholder input in UE 5.4.
		// Canonicalize it before appending the thirteen explicit stellar inputs.
		StellarSurface->Inputs.Reset();
		StellarSurface->Code = TEXT(R"APSSTELLAR(
float instanceEnergy = abs(InstanceColor.r) + abs(InstanceColor.g) + abs(InstanceColor.b);
float useInstance = step(0.0001, instanceEnergy);
float3 spectralColor = max(lerp(ParamColor.rgb, InstanceColor.rgb, useInstance), 0.001);
float rawEmission = max(lerp(ParamEmission, InstanceEmission, useInstance), 0.0);
float seed = frac(lerp(ParamSeed, InstanceSeed, useInstance));
float3 n = normalize(NormalWS);
float3 v = normalize(CameraWS);
float facing = saturate(abs(dot(n, v)));

// Two scale-free cells derived from the sphere normal keep the pattern stable
// from a galaxy point through a full-screen star without texture swimming.
float3 coarseCell = floor((n + 1.0) * 13.0 + seed * 19.0);
float coarse = frac(sin(dot(coarseCell, float3(12.9898, 78.233, 37.719))) * 43758.5453);
float3 fineCell = floor((n + 1.0) * 47.0 + seed * 53.0);
float fine = frac(sin(dot(fineCell, float3(39.3468, 11.135, 83.155))) * 24634.6345);
float broadWave = 0.5 + 0.25 * sin(dot(n, float3(9.7, 13.1, 7.3)) + seed * 31.4)
                         + 0.25 * sin(dot(n, float3(17.3, -6.1, 11.9)) - seed * 19.7);
float emissionActivity = saturate(log2(1.0 + rawEmission) / 14.0);
float variation = saturate(Variation + useInstance * emissionActivity * 0.10);
float granulation = ((coarse - 0.5) * 0.65 + (fine - 0.5) * 0.35)
                  * Granulation * 0.34;
float spots = smoothstep(0.76, 0.96, broadWave) * SpotAmount
            * lerp(0.55, 1.0, emissionActivity);
float surface = max(0.24, 1.0 + granulation + (broadWave - 0.5) * variation * 0.32
                           - spots * 0.42);

// Limb darkening gives the disc volume; an irregular Fresnel rim and bloom form
// a cheap corona/prominence hint without particles, WPO, or extra draw calls.
float limb = lerp(0.70, 1.08, pow(facing, 0.42));
float rim = pow(1.0 - facing, 2.35);
float prominenceWave = pow(saturate(0.5 + 0.5
    * sin(dot(n, float3(27.0, 19.0, -23.0)) + seed * 43.0)), 7.0);
float corona = CoronaAmount * rim * (0.34 + prominenceWave * 0.66)
             * (1.0 + saturate(SystemMarker) * 0.10);

// Log compression preserves spectral/luminosity ordering while preventing a
// giant or close-up star from blowing out the entire preview exposure.
float displayEmission = 1.35 + min(log2(1.0 + rawEmission) * 1.75, 30.0);
float3 hotCore = lerp(spectralColor, float3(1.0, 0.97, 0.90),
                      saturate((surface - 1.0) * 0.22));
return hotCore * displayEmission * max(surface * limb + corona, 0.08);
)APSSTELLAR");
		AddCustomInput(StellarSurface, TEXT("ParamColor"), Color);
		AddCustomInput(StellarSurface, TEXT("ParamEmission"), Multiplier);
		AddCustomInput(StellarSurface, TEXT("ParamSeed"), SurfaceSeed);
		AddCustomInput(StellarSurface, TEXT("Variation"), SurfaceVariation);
		AddCustomInput(StellarSurface, TEXT("Granulation"), GranulationStrength);
		AddCustomInput(StellarSurface, TEXT("SpotAmount"), SpotStrength);
		AddCustomInput(StellarSurface, TEXT("CoronaAmount"), CoronaStrength);
		AddCustomInput(StellarSurface, TEXT("InstanceColor"), InstanceColor);
		AddCustomInput(StellarSurface, TEXT("InstanceEmission"), InstanceEmission);
		AddCustomInput(StellarSurface, TEXT("InstanceSeed"), InstanceSeed);
		AddCustomInput(StellarSurface, TEXT("SystemMarker"), SystemHighlight);
		AddCustomInput(StellarSurface, TEXT("NormalWS"), Normal);
		AddCustomInput(StellarSurface, TEXT("CameraWS"), Camera);
		if (!UMaterialEditingLibrary::ConnectMaterialProperty(
			StellarSurface, TEXT(""), MP_EmissiveColor))
		{
			return false;
		}

		Material->UpdateCachedExpressionData();
		Material->PostEditChange();
		UMaterialEditingLibrary::RecompileMaterial(Material);
		return true;
	}
}
#endif

UAPSFixStarHISMMaterialCommandlet::UAPSFixStarHISMMaterialCommandlet()
{
	IsClient = false;
	IsServer = false;
	IsEditor = true;
	LogToConsole = true;
}

int32 UAPSFixStarHISMMaterialCommandlet::Main(const FString& Params)
{
#if WITH_EDITOR
	const TArray<FString> UnifiedMasterPaths{
		TEXT("/Game/APS/APS_ALPHA/Assets/Materials/Astro/M_SpectralStarMat"),
		TEXT("/Game/APS/APS_ALPHA/Assets/Materials/Astro/M_SpectralStarMat_HISM"),
		TEXT("/Game/APS/APS_ALPHA/Assets/Materials/Astro/M_SpectralStarMat_SUN")
	};

	int32 ChangedCount = 0;
	int32 ErrorCount = 0;
	for (const FString& PackagePath : UnifiedMasterPaths)
	{
		const FString AssetName = FPackageName::GetLongPackageAssetName(PackagePath);
		const FString ObjectPath = FString::Printf(TEXT("%s.%s"), *PackagePath, *AssetName);
		UMaterial* Material = LoadObject<UMaterial>(nullptr, *ObjectPath);
		if (!Material)
		{
			UE_LOG(LogAPSStarMaterialFix, Error, TEXT("Could not load %s"), *ObjectPath);
			++ErrorCount;
			continue;
		}

		if (!APSStellarMaterial::RebuildUnifiedStellarMaterial(Material))
		{
			UE_LOG(LogAPSStarMaterialFix, Error,
				TEXT("Could not rebuild unified stellar graph: %s"), *ObjectPath);
			++ErrorCount;
			continue;
		}

		UPackage* Package = Material->GetOutermost();
		Package->SetDirtyFlag(true);
		const FString Filename = FPackageName::LongPackageNameToFilename(
			PackagePath, FPackageName::GetAssetPackageExtension());
		FSavePackageArgs SaveArgs;
		SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
		SaveArgs.SaveFlags = SAVE_NoError;
		if (!UPackage::SavePackage(Package, Material, *Filename, SaveArgs))
		{
			UE_LOG(LogAPSStarMaterialFix, Error, TEXT("Could not save %s"), *Filename);
			++ErrorCount;
			continue;
		}

		UE_LOG(LogAPSStarMaterialFix, Display,
			TEXT("Rebuilt scale-independent stellar surface: %s"), *ObjectPath);
		++ChangedCount;
	}

	UE_LOG(LogAPSStarMaterialFix, Display,
		TEXT("Complete: rebuilt=%d errors=%d targeted=%d"),
		ChangedCount, ErrorCount, UnifiedMasterPaths.Num());
	return ErrorCount == 0 ? 0 : 1;
#else
	UE_LOG(LogAPSStarMaterialFix, Error, TEXT("This maintenance commandlet requires an Editor build."));
	return 1;
#endif
}
