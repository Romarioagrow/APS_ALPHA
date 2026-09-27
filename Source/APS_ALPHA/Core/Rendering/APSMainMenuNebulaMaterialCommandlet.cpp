#include "APSMainMenuNebulaMaterialCommandlet.h"

#include "APSMainMenuNebulaMaterial.h"

// The contract uses a versioned asset name so an open editor can keep its old
// preview resource alive while the commandlet authors the replacement safely.

#if WITH_EDITOR
#include "AssetToolsModule.h"
#include "Factories/MaterialFactoryNew.h"
#include "IAssetTools.h"
#include "MaterialEditingLibrary.h"
#include "MaterialDomain.h"
#include "Materials/Material.h"
#include "Materials/MaterialExpressionComponentMask.h"
#include "Materials/MaterialExpressionCustom.h"
#include "Materials/MaterialExpressionScalarParameter.h"
#include "Materials/MaterialExpressionVectorParameter.h"
#include "Misc/PackageName.h"
#include "Modules/ModuleManager.h"
#include "UObject/Package.h"
#include "UObject/SavePackage.h"
#include "UObject/UnrealType.h"
#endif

DEFINE_LOG_CATEGORY_STATIC(LogAPSMainMenuNebula, Log, All);

#if WITH_EDITOR
namespace APSMainMenuNebulaAssets
{
	template <typename TExpression>
	TExpression* Add(UMaterial* Material, const int32 X, const int32 Y)
	{
		return Cast<TExpression>(UMaterialEditingLibrary::CreateMaterialExpression(
			Material, TExpression::StaticClass(), X, Y));
	}

	UMaterialExpression* AddReflected(UMaterial* Material, const TCHAR* ClassPath,
		const int32 X, const int32 Y)
	{
		UClass* ExpressionClass = FindObject<UClass>(nullptr, ClassPath);
		return ExpressionClass
			? UMaterialEditingLibrary::CreateMaterialExpression(Material, ExpressionClass, X, Y)
			: nullptr;
	}

	bool SetUInt32(UObject* Object, const FName Name, const uint32 Value)
	{
		FUInt32Property* Property = Object
			? FindFProperty<FUInt32Property>(Object->GetClass(), Name) : nullptr;
		if (!Property) return false;
		Property->SetPropertyValue_InContainer(Object, Value);
		return true;
	}

	bool SetFloat(UObject* Object, const FName Name, const float Value)
	{
		FFloatProperty* Property = Object
			? FindFProperty<FFloatProperty>(Object->GetClass(), Name) : nullptr;
		if (!Property) return false;
		Property->SetPropertyValue_InContainer(Object, Value);
		return true;
	}

	bool SetColor(UObject* Object, const FName Name, const FLinearColor& Value)
	{
		FStructProperty* Property = Object
			? FindFProperty<FStructProperty>(Object->GetClass(), Name) : nullptr;
		if (!Property || Property->Struct != TBaseStructure<FLinearColor>::Get()) return false;
		*Property->ContainerPtrToValuePtr<FLinearColor>(Object) = Value;
		return true;
	}

	UMaterialExpressionVectorParameter* Color(UMaterial* Material, const TCHAR* Name,
		const FLinearColor& Value, const int32 Y, const int32 SortPriority)
	{
		UMaterialExpressionVectorParameter* Node =
			Add<UMaterialExpressionVectorParameter>(Material, -1180, Y);
		if (!Node) return nullptr;
		Node->ParameterName = Name;
		Node->DefaultValue = Value;
		Node->Group = TEXT("Nebula Palette");
		Node->SortPriority = SortPriority;
		Node->UpdateParameterGuid(true, true);
		return Node;
	}

	UMaterialExpressionScalarParameter* Scalar(UMaterial* Material, const TCHAR* Name,
		const float Value, const float Minimum, const float Maximum,
		const int32 Y, const int32 SortPriority, const TCHAR* Group = TEXT("Nebula Shape"))
	{
		UMaterialExpressionScalarParameter* Node =
			Add<UMaterialExpressionScalarParameter>(Material, -1180, Y);
		if (!Node) return nullptr;
		Node->ParameterName = Name;
		Node->DefaultValue = Value;
		Node->SliderMin = Minimum;
		Node->SliderMax = Maximum;
		Node->Group = Group;
		Node->SortPriority = SortPriority;
		Node->UpdateParameterGuid(true, true);
		return Node;
	}

	void Input(UMaterialExpressionCustom* Custom, const TCHAR* Name,
		UMaterialExpression* Expression, const int32 OutputIndex = 0)
	{
		FCustomInput& Item = Custom->Inputs.AddDefaulted_GetRef();
		Item.InputName = Name;
		Item.Input.Connect(OutputIndex, Expression);
	}

	bool Build(UMaterial* Material)
	{
		if (!Material) return false;
		Material->Modify();
		while (!Material->GetExpressions().IsEmpty())
		{
			UMaterialExpression* Expression = Material->GetExpressions().Last();
			if (IsValid(Expression) && Expression->IsRooted()) Expression->RemoveFromRoot();
			UMaterialEditingLibrary::DeleteMaterialExpression(Material, Expression);
		}

		Material->MaterialDomain = MD_Surface;
		Material->BlendMode = BLEND_Additive;
		Material->SetShadingModel(MSM_Unlit);
		Material->TwoSided = true;
		Material->bDisableDepthTest = false;
		Material->bUsedWithInstancedStaticMeshes = true;
		Material->DitheredLODTransition = false;
		Material->DitherOpacityMask = false;
		Material->TranslucencyPass = MTP_AfterDOF;

		UMaterialExpression* UV = AddReflected(Material,
			TEXT("/Script/Engine.MaterialExpressionTextureCoordinate"), -900, -700);
		UMaterialExpression* Time = AddReflected(Material,
			TEXT("/Script/Engine.MaterialExpressionTime"), -900, 470);
		UMaterialExpression* InstanceTint = AddReflected(Material,
			TEXT("/Script/Engine.MaterialExpressionPerInstanceCustomData3Vector"), -900, -570);
		UMaterialExpression* InstanceOpacity = AddReflected(Material,
			TEXT("/Script/Engine.MaterialExpressionPerInstanceCustomData"), -900, -450);
		UMaterialExpression* InstanceSeed = AddReflected(Material,
			TEXT("/Script/Engine.MaterialExpressionPerInstanceCustomData"), -900, -340);
		UMaterialExpression* InstanceScale = AddReflected(Material,
			TEXT("/Script/Engine.MaterialExpressionPerInstanceCustomData"), -900, -230);
		UMaterialExpression* InstanceWarmth = AddReflected(Material,
			TEXT("/Script/Engine.MaterialExpressionPerInstanceCustomData"), -900, -120);
		UMaterialExpression* InstanceDetail = AddReflected(Material,
			TEXT("/Script/Engine.MaterialExpressionPerInstanceCustomData"), -900, -10);

		auto* ColorA = Color(Material, TEXT("NebulaColorA"),
			FLinearColor(0.025f, 0.18f, 0.62f), -610, 0);
		auto* ColorB = Color(Material, TEXT("NebulaColorB"),
			FLinearColor(0.04f, 0.72f, 0.95f), -510, 1);
		auto* WarmColor = Color(Material, TEXT("NebulaWarmColor"),
			FLinearColor(0.86f, 0.19f, 0.42f), -410, 2);
		auto* Opacity = Scalar(Material, TEXT("NebulaOpacity"), 0.62f, 0.0f, 1.0f, -300, 3);
		auto* Emission = Scalar(Material, TEXT("NebulaEmission"), 2.60f, 0.0f, 12.0f, -200, 4,
			TEXT("Nebula Light"));
		auto* Density = Scalar(Material, TEXT("NebulaDensity"), 1.15f, 0.2f, 4.0f, -100, 5);
		auto* Filaments = Scalar(Material, TEXT("NebulaFilamentStrength"), 0.82f,
			0.0f, 2.0f, 0, 6);
		auto* Distortion = Scalar(Material, TEXT("NebulaDistortion"), 0.68f,
			0.0f, 2.0f, 100, 7);
		auto* EdgeSoftness = Scalar(Material, TEXT("NebulaEdgeSoftness"), 0.72f,
			0.1f, 1.5f, 200, 8);
		auto* DriftSpeed = Scalar(Material, TEXT("NebulaDriftSpeed"), 0.004f,
			0.0f, 0.05f, 300, 9, TEXT("Nebula Motion"));
		auto* Nebula = Add<UMaterialExpressionCustom>(Material, -330, -250);
		auto* EmissiveMask = Add<UMaterialExpressionComponentMask>(Material, 110, -180);
		auto* OpacityMask = Add<UMaterialExpressionComponentMask>(Material, 110, 40);

		if (!UV || !Time || !InstanceTint || !InstanceOpacity || !InstanceSeed
			|| !InstanceScale || !InstanceWarmth || !InstanceDetail || !ColorA || !ColorB
			|| !WarmColor || !Opacity || !Emission || !Density || !Filaments || !Distortion
			|| !EdgeSoftness || !DriftSpeed || !Nebula || !EmissiveMask || !OpacityMask)
		{
			return false;
		}

		if (!SetUInt32(InstanceTint, TEXT("DataIndex"), APSMainMenuNebulaMaterial::TintR)
			|| !SetColor(InstanceTint, TEXT("ConstDefaultValue"), FLinearColor(0.18f, 0.56f, 1.0f))
			|| !SetUInt32(InstanceOpacity, TEXT("DataIndex"), APSMainMenuNebulaMaterial::Opacity)
			|| !SetFloat(InstanceOpacity, TEXT("ConstDefaultValue"), 1.0f)
			|| !SetUInt32(InstanceSeed, TEXT("DataIndex"), APSMainMenuNebulaMaterial::Seed)
			|| !SetFloat(InstanceSeed, TEXT("ConstDefaultValue"), 0.371f)
			|| !SetUInt32(InstanceScale, TEXT("DataIndex"), APSMainMenuNebulaMaterial::PatternScale)
			|| !SetFloat(InstanceScale, TEXT("ConstDefaultValue"), 1.0f)
			|| !SetUInt32(InstanceWarmth, TEXT("DataIndex"), APSMainMenuNebulaMaterial::Warmth)
			|| !SetFloat(InstanceWarmth, TEXT("ConstDefaultValue"), 0.0f)
			|| !SetUInt32(InstanceDetail, TEXT("DataIndex"), APSMainMenuNebulaMaterial::Detail)
			|| !SetFloat(InstanceDetail, TEXT("ConstDefaultValue"), 1.0f))
		{
			return false;
		}

		Nebula->Description = TEXT("Sparse domain-warped APOSFERA nebula cloud");
		Nebula->OutputType = CMOT_Float4;
		Nebula->Inputs.Reset();
		Input(Nebula, TEXT("UV"), UV);
		Input(Nebula, TEXT("ColorA"), ColorA);
		Input(Nebula, TEXT("ColorB"), ColorB);
		Input(Nebula, TEXT("WarmColor"), WarmColor);
		Input(Nebula, TEXT("MasterOpacity"), Opacity);
		Input(Nebula, TEXT("Emission"), Emission);
		Input(Nebula, TEXT("Density"), Density);
		Input(Nebula, TEXT("FilamentStrength"), Filaments);
		Input(Nebula, TEXT("Distortion"), Distortion);
		Input(Nebula, TEXT("EdgeSoftness"), EdgeSoftness);
		Input(Nebula, TEXT("DriftSpeed"), DriftSpeed);
		Input(Nebula, TEXT("GameTime"), Time);
		Input(Nebula, TEXT("InstanceTint"), InstanceTint);
		Input(Nebula, TEXT("InstanceOpacity"), InstanceOpacity);
		Input(Nebula, TEXT("InstanceSeed"), InstanceSeed);
		Input(Nebula, TEXT("InstanceScale"), InstanceScale);
		Input(Nebula, TEXT("InstanceWarmth"), InstanceWarmth);
		Input(Nebula, TEXT("InstanceDetail"), InstanceDetail);
		Nebula->Code = TEXT(R"APSNEBULA(
float2 p = (UV - 0.5) * 2.0;
float seedAngle = frac(InstanceSeed) * 6.2831853;
float cs = cos(seedAngle), sn = sin(seedAngle);
p = float2(cs * p.x - sn * p.y, sn * p.x + cs * p.y);
float scale = max(InstanceScale, 0.15) * max(Density, 0.1);
float drift = GameTime * DriftSpeed;
float2 seedOffset = float2(frac(InstanceSeed * 17.71), frac(InstanceSeed * 41.37)) * 31.0;
float2 q = p * scale + seedOffset;

// A Material Custom node is emitted inside an engine-generated pixel function,
// so HLSL helper function definitions are illegal here. Evaluate the two warp
// channels and four cloud fields with fixed, unrolled local loops instead.
float2 warpPoints[2] = { q * 0.72 + drift, q * 0.72 + 9.7 - drift };
float warpValues[2] = { 0.0, 0.0 };
float warpWeight = 0.54;
[unroll] for (int octave = 0; octave < 5; ++octave)
{
    [unroll] for (int channel = 0; channel < 2; ++channel)
    {
        float2 samplePoint = warpPoints[channel];
        float2 cell = floor(samplePoint);
        float2 blend = frac(samplePoint);
        blend = blend * blend * (3.0 - 2.0 * blend);

        float2 h00 = frac(cell * float2(123.34, 456.21));
        float2 h10 = frac((cell + float2(1.0, 0.0)) * float2(123.34, 456.21));
        float2 h01 = frac((cell + float2(0.0, 1.0)) * float2(123.34, 456.21));
        float2 h11 = frac((cell + 1.0) * float2(123.34, 456.21));
        h00 += dot(h00, h00 + 45.32);
        h10 += dot(h10, h10 + 45.32);
        h01 += dot(h01, h01 + 45.32);
        h11 += dot(h11, h11 + 45.32);
        float row0 = lerp(frac(h00.x * h00.y), frac(h10.x * h10.y), blend.x);
        float row1 = lerp(frac(h01.x * h01.y), frac(h11.x * h11.y), blend.x);
        warpValues[channel] += lerp(row0, row1, blend.y) * warpWeight;
        warpPoints[channel] = mul(float2x2(1.47, -1.13, 1.13, 1.47), samplePoint) + 13.17;
    }
    warpWeight *= 0.51;
}
float2 warp = float2(warpValues[0], warpValues[1]) - 0.5;

float2 fieldPoints[4] = {
    q + warp * (2.0 + 3.0 * Distortion),
    q * 2.35 - warp * (1.2 + Distortion),
    q * (4.6 + max(InstanceDetail, 0.0) * 1.8) + warp,
    q * 1.38 + float2(5.2, -7.1)
};
float fieldValues[4] = { 0.0, 0.0, 0.0, 0.0 };
float fieldWeight = 0.54;
[unroll] for (int octave = 0; octave < 5; ++octave)
{
    [unroll] for (int field = 0; field < 4; ++field)
    {
        float2 samplePoint = fieldPoints[field];
        float2 cell = floor(samplePoint);
        float2 blend = frac(samplePoint);
        blend = blend * blend * (3.0 - 2.0 * blend);

        float2 h00 = frac(cell * float2(123.34, 456.21));
        float2 h10 = frac((cell + float2(1.0, 0.0)) * float2(123.34, 456.21));
        float2 h01 = frac((cell + float2(0.0, 1.0)) * float2(123.34, 456.21));
        float2 h11 = frac((cell + 1.0) * float2(123.34, 456.21));
        h00 += dot(h00, h00 + 45.32);
        h10 += dot(h10, h10 + 45.32);
        h01 += dot(h01, h01 + 45.32);
        h11 += dot(h11, h11 + 45.32);
        float row0 = lerp(frac(h00.x * h00.y), frac(h10.x * h10.y), blend.x);
        float row1 = lerp(frac(h01.x * h01.y), frac(h11.x * h11.y), blend.x);
        fieldValues[field] += lerp(row0, row1, blend.y) * fieldWeight;
        fieldPoints[field] = mul(float2x2(1.47, -1.13, 1.13, 1.47), samplePoint) + 13.17;
    }
    fieldWeight *= 0.51;
}
float broad = fieldValues[0];
float middle = fieldValues[1];
float fine = fieldValues[2];
float ridge = 1.0 - abs(middle * 2.0 - 1.0);
ridge = pow(saturate(ridge), lerp(4.8, 2.1, saturate(FilamentStrength)));
float knots = smoothstep(0.57, 0.86, fine) * smoothstep(0.40, 0.74, broad);
float edgeNoise = saturate(fieldValues[3]);
float2 edgeP = p * float2(0.76, 1.08);
float2 edgeWarpedP = edgeP + warp * (0.22 + 0.14 * saturate(Distortion));
float edgeRadius = lerp(0.48, 0.82, edgeNoise);
float radial = 1.0 - smoothstep(edgeRadius,
    edgeRadius + 0.20 + 0.10 * EdgeSoftness, length(edgeWarpedP));

// A raised cloud threshold and a second independent breakup field remove the
// readable oval of the carrier plane. Only disconnected wisps and knots survive.
float cloudSignal = (broad - 0.47) * 1.55
    + ridge * FilamentStrength * 0.56 + knots * 0.34;
float cloud = smoothstep(0.06, 0.62, cloudSignal);
float breakup = smoothstep(0.34, 0.72, edgeNoise + (fine - 0.5) * 0.22);
cloud *= lerp(0.18, 1.0, breakup);
float densityMask = saturate(radial * cloud);
densityMask *= smoothstep(0.02, 0.18, densityMask);
float alpha = densityMask * MasterOpacity * max(InstanceOpacity, 0.0);
float palette = saturate(middle * 0.76 + fine * 0.24);
float3 cool = lerp(ColorA.rgb, ColorB.rgb, palette);
float3 color = lerp(cool, WarmColor.rgb, saturate(InstanceWarmth) * smoothstep(0.52, 0.88, broad));
color *= max(InstanceTint.rgb, 0.001);
float energy = 0.22 + ridge * 0.38 + knots * 0.22 + broad * 0.24;
float premultipliedEnergy = densityMask * (0.42 + densityMask * 0.58);
return float4(color * energy * Emission * premultipliedEnergy, saturate(alpha));
)APSNEBULA");

		EmissiveMask->R = true;
		EmissiveMask->G = true;
		EmissiveMask->B = true;
		EmissiveMask->A = false;
		OpacityMask->R = false;
		OpacityMask->G = false;
		OpacityMask->B = false;
		OpacityMask->A = true;

		const bool bConnected =
			UMaterialEditingLibrary::ConnectMaterialExpressions(Nebula, TEXT(""), EmissiveMask, TEXT(""))
			&& UMaterialEditingLibrary::ConnectMaterialExpressions(Nebula, TEXT(""), OpacityMask, TEXT(""))
			&& UMaterialEditingLibrary::ConnectMaterialProperty(EmissiveMask, TEXT(""), MP_EmissiveColor)
			&& UMaterialEditingLibrary::ConnectMaterialProperty(OpacityMask, TEXT(""), MP_Opacity);
		if (!bConnected) return false;

		Material->UpdateCachedExpressionData();
		Material->PostEditChange();
		UMaterialEditingLibrary::RecompileMaterial(Material);
		Material->ForceRecompileForRendering();
		Material->EnsureIsComplete();
		const FMaterialStatistics Statistics = UMaterialEditingLibrary::GetStatistics(Material);
		UE_LOG(LogAPSMainMenuNebula, Display,
			TEXT("Validated extensible nebula material pixelInstructions=%d vertexInstructions=%d samplers=%d"),
			Statistics.NumPixelShaderInstructions, Statistics.NumVertexShaderInstructions,
			Statistics.NumSamplers);
		// A newly-created material does not own a feature-level FMaterialResource until
		// after its first save/reload in UE 5.4 commandlets. GetStatistics can likewise
		// report zero here. The graph contract is validated above; a separate fresh-load
		// automation test validates the saved asset after this commandlet exits.
		return true;
	}
}
#endif

UAPSMainMenuNebulaMaterialCommandlet::UAPSMainMenuNebulaMaterialCommandlet()
{
	IsClient = false;
	IsServer = false;
	IsEditor = true;
	LogToConsole = true;
}

int32 UAPSMainMenuNebulaMaterialCommandlet::Main(const FString& Params)
{
#if WITH_EDITOR
	using namespace APSMainMenuNebulaAssets;
	UMaterial* Material = LoadObject<UMaterial>(nullptr,
		APSMainMenuNebulaMaterial::ObjectPath, nullptr, LOAD_NoWarn);
	if (!Material)
	{
		IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>(
			TEXT("AssetTools")).Get();
		Material = Cast<UMaterial>(AssetTools.CreateAsset(
			APSMainMenuNebulaMaterial::AssetName,
			APSMainMenuNebulaMaterial::PackagePath,
			UMaterial::StaticClass(), NewObject<UMaterialFactoryNew>()));
	}
	if (!IsValid(Material) || !Build(Material))
	{
		UE_LOG(LogAPSMainMenuNebula, Error, TEXT("Could not build %s"),
			APSMainMenuNebulaMaterial::ObjectPath);
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
	UE_LOG(LogAPSMainMenuNebula, Display,
		TEXT("Saved=%d path=%s; no worlds, gameplay assets or generation data changed"),
		bSaved ? 1 : 0, APSMainMenuNebulaMaterial::ObjectPath);
	return bSaved ? 0 : 1;
#else
	UE_LOG(LogAPSMainMenuNebula, Error,
		TEXT("The nebula asset commandlet requires an Editor build."));
	return 1;
#endif
}
