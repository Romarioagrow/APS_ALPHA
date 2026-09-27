#include "APSFixStarHISMMaterialCommandlet.h"

#if WITH_EDITOR
#include "MaterialEditingLibrary.h"
#include "MaterialDomain.h"
#include "MaterialShared.h"
#include "Materials/Material.h"
#include "Materials/MaterialExpressionCameraVectorWS.h"
#include "Materials/MaterialExpressionCustom.h"
#include "Materials/MaterialExpressionScalarParameter.h"
#include "Materials/MaterialExpressionVectorParameter.h"
#include "Misc/PackageName.h"
#include "Misc/Parse.h"
#include "RHI.h"
#include "UObject/Package.h"
#include "UObject/SavePackage.h"
#include "UObject/UnrealType.h"
#include "UObject/UObjectGlobals.h"
#endif

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
			UMaterialExpression* Expression = Material->GetExpressions().Last();
			// Material compilation can temporarily root a VertexInterpolator. The
			// maintenance commandlet owns the old graph at this point, so release that
			// transient root before DeleteMaterialExpression marks the node as garbage.
			// Without this guard a second idempotent rebuild asserts in UE 5.4 after the
			// first master has compiled successfully.
			if (IsValid(Expression) && Expression->IsRooted())
			{
				Expression->RemoveFromRoot();
			}
			UMaterialEditingLibrary::DeleteMaterialExpression(Material, Expression);
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
			Material, TEXT("SurfaceVariation"), 0.32f, 0.0f, 1.0f, -1250, -320, 3);
		UMaterialExpressionScalarParameter* GranulationStrength = AddScalarParameter(
			Material, TEXT("GranulationStrength"), 0.22f, 0.0f, 1.0f, -1250, -220, 4);
		UMaterialExpressionScalarParameter* SpotStrength = AddScalarParameter(
			Material, TEXT("SpotStrength"), 0.68f, 0.0f, 1.0f, -1250, -120, 5);
		UMaterialExpressionScalarParameter* CoronaStrength = AddScalarParameter(
			Material, TEXT("CoronaStrength"), 0.16f, 0.10f, 0.24f, -1250, -20, 6);
		UMaterialExpressionScalarParameter* StellarArchetype = AddScalarParameter(
			Material, TEXT("StellarArchetype"), 0.0f, 0.0f, 6.0f, -1250, 80, 7);

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
		UMaterialExpression* Normal = AddReflectedExpression(Material,
			TEXT("/Script/Engine.MaterialExpressionPixelNormalWS"), -950, -150);
		UMaterialExpression* WorldPosition = AddReflectedExpression(Material,
			TEXT("/Script/Engine.MaterialExpressionWorldPosition"), -950, -60);
		UMaterialExpression* ObjectPosition = AddReflectedExpression(Material,
			TEXT("/Script/Engine.MaterialExpressionObjectPositionWS"), -950, 30);
		UMaterialExpression* InterpolatedObjectPosition = AddReflectedExpression(Material,
			TEXT("/Script/Engine.MaterialExpressionVertexInterpolator"), -700, 30);
		UMaterialExpression* GameTime = AddReflectedExpression(Material,
			TEXT("/Script/Engine.MaterialExpressionTime"), -950, 120);
		UMaterialExpressionCameraVectorWS* Camera =
			AddExpression<UMaterialExpressionCameraVectorWS>(Material, -950, 210);
		UMaterialExpressionCustom* StellarSurface =
			AddExpression<UMaterialExpressionCustom>(Material, -450, -320);

		if (!Color || !Multiplier || !SurfaceSeed || !SurfaceVariation || !GranulationStrength
			|| !SpotStrength || !CoronaStrength || !StellarArchetype
			|| !InstanceColor || !InstanceEmission
			|| !InstanceSeed || !SystemHighlight || !Normal || !WorldPosition
			|| !ObjectPosition || !InterpolatedObjectPosition || !GameTime || !Camera
			|| !StellarSurface)
		{
			return false;
		}
		if (!UMaterialEditingLibrary::ConnectMaterialExpressions(
			ObjectPosition, TEXT(""), InterpolatedObjectPosition, TEXT("VS")))
		{
			return false;
		}

		// RGB, luminosity/emission, deterministic seed and the potential-system bit
		// already occupy custom-data slots 0..5 on both Galaxy and Cluster HISM.
		// A wholly zero instance payload identifies an ordinary static-mesh star. Using
		// emission/seed/marker as well as colour keeps near-black HISM spectra on the
		// instance path while the same shader consumes actor material parameters.
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
		// Canonicalize it before appending the sixteen explicit stellar inputs.
		StellarSurface->Inputs.Reset();
		const FString StellarPatternCode = TEXT(R"APSSTELLAR(
// Direction-space photosphere, 2026-09-19. The six noise bands describe emitted
// intensity, never normals or displacement: granules must not become dimples.
float instanceEnergy = abs(InstanceColor.r) + abs(InstanceColor.g) + abs(InstanceColor.b);
float instanceSignal = instanceEnergy + abs(InstanceEmission)
                     + abs(InstanceSeed) + abs(SystemMarker);
float useInstance = step(0.0001, instanceSignal);
float3 spectralColor = max(lerp(ParamColor.rgb, InstanceColor.rgb, useInstance), 0.001);
float rawEmission = max(lerp(ParamEmission, InstanceEmission, useInstance), 0.0);
float seed = frac(lerp(ParamSeed, InstanceSeed, useInstance));
float actorOnly = 1.0 - useInstance;
float giantType = actorOnly * (1.0 - step(0.5, abs(StellarType - 1.0)));
float protostarType = actorOnly * (1.0 - step(0.5, abs(StellarType - 2.0)));
float compactType = actorOnly * (1.0 - step(0.5, abs(StellarType - 3.0)));
float pulsarType = actorOnly * (1.0 - step(0.5, abs(StellarType - 4.0)));
float coolDwarfType = actorOnly * (1.0 - step(0.5, abs(StellarType - 5.0)));
float blackHoleType = actorOnly * step(5.5, StellarType);

// Vertex-interpolated object position is required for the per-instance centre.
// Position-derived direction also avoids the legacy sphere's split normal islands.
float3 radial = WorldPositionWS - ObjectPositionWS;
float radialLengthSq = dot(radial, radial);
float3 n = radialLengthSq > 1.0e-8
    ? radial * rsqrt(radialLengthSq)
    : normalize(NormalWS);
float normalFootprint = max(length(ddx(n)), length(ddy(n)));
float resolvedInstanceDetail = 1.0 - smoothstep(0.18, 0.65, normalFootprint);
float spatialDetail = lerp(1.0, resolvedInstanceDetail, useInstance);
float3 v = normalize(CameraWS);
float facing = saturate(dot(n, v));
float projectedRadiusSq = saturate(1.0 - facing * facing);
float phase = seed * 37.6991118;
float photosphereFrequency = clamp(1.0 - giantType * 0.28
    - protostarType * 0.35 + compactType * 0.28
    + pulsarType * 0.38 - coolDwarfType * 0.08, 0.60, 1.40);
float3 seedOffset = frac(float3(seed * 7.13 + 0.17,
                                seed * 13.71 + 0.53,
                                seed * 19.37 + 0.89)) * 13.0;
float3 stellarAxis = normalize(float3(
    seed * 2.0 - 0.83,
    frac(seed * 7.13 + 0.31) * 2.0 - 1.0,
    frac(seed * 13.71 + 0.73) * 2.0 - 1.0));
float axisAlignment = abs(dot(n, stellarAxis));

// UE's volume Perlin returns signed noise. Independent rotations, unequal
// frequency ratios and a low-frequency domain warp hide its 15-unit repeat.
// Coarse bands arrange active regions; only the fine bands describe granulation.
float3 p = n * photosphereFrequency;
// Coherent, very slow actor-only advection keeps the accepted plasma structure
// intact. At a 500px radius its rotation is around a quarter pixel per second.
// Catalogue/resolved instance coordinates remain byte-stable over time.
if (actorOnly > 0.5)
{
    float flowAngle = GameTime * lerp(0.00045, 0.00060, seed);
    float flowSin = sin(flowAngle);
    float flowCos = cos(flowAngle);
    float3 flowingDirection = n * flowCos + cross(stellarAxis, n) * flowSin
        + stellarAxis * dot(stellarAxis, n) * (1.0 - flowCos);
    float slowFlowPhase = GameTime * 0.017 + phase;
    float3 slowFlow = float3(sin(slowFlowPhase),
        sin(slowFlowPhase * 0.83 + 1.7), sin(slowFlowPhase * 1.13 + 4.1));
    p = flowingDirection * photosphereFrequency + slowFlow * 0.009;
}
float macroConvection = FastGradientPerlinNoise3D_TEX(p * 3.70 + seedOffset);
float3 rotatedP = float3(
    dot(p, float3(0.36, 0.80, 0.48)),
    dot(p, float3(-0.78, 0.50, -0.37)),
    dot(p, float3(-0.51, -0.32, 0.80)));
float mesoCells = FastGradientPerlinNoise3D_TEX(
    rotatedP * 11.30 + seedOffset.yzx
    + macroConvection * float3(0.77, -0.58, 0.93));
float3 turbulentP = p + float3(mesoCells, macroConvection, -mesoCells) * 0.023;
float middle = FastGradientPerlinNoise3D_TEX(
    turbulentP * 31.70 + seedOffset.zxy);
float fineA = FastGradientPerlinNoise3D_TEX(
    turbulentP * 73.70 + seedOffset.xzy
    + middle * float3(0.34, -0.27, 0.19));
float fineB = FastGradientPerlinNoise3D_TEX(
    rotatedP * 127.10 + seedOffset.zyx
    + float3(mesoCells, middle, macroConvection) * 0.63);
float dust = FastGradientPerlinNoise3D_TEX(
    turbulentP.yzx * 213.70 + seedOffset.yxz
    + fineA * float3(0.27, -0.21, 0.31));

// Footprint filtering is applied to each band, including actors at the limb.
// It removes temporal aliasing without making an entire resolved star featureless.
float bandFootprint = normalFootprint * photosphereFrequency;
float middleDetail = 1.0 - smoothstep(0.40, 0.95, bandFootprint * 31.70);
float fineDetailA = 1.0 - smoothstep(0.40, 0.95, bandFootprint * 73.70);
float fineDetailB = 1.0 - smoothstep(0.40, 0.95, bandFootprint * 127.10);
float dustDetail = 1.0 - smoothstep(0.40, 0.95, bandFootprint * 213.70);
middle *= middleDetail;
fineA *= fineDetailA;
fineB *= fineDetailB;
dust *= dustDetail;


)APSSTELLAR");
		const FString StellarLightingCode = TEXT(R"APSSTELLAR(
float logEmission = log2(1.0 + rawEmission);
float actorActivity = saturate((logEmission - 6.65) / 2.32);
float proxyActivity = saturate(logEmission / 8.97);
float emissionActivity = lerp(actorActivity, proxyActivity, useInstance);
float variation = saturate(Variation);
float granulationStrength = saturate(Granulation);

// A continuous fractal signal gives branched, crumbling plage rather than
// isolated Perlin blobs. Its fine boundaries share the photosphere's grain.
float activeField = macroConvection * 0.40 + mesoCells * 0.40
                  + middle * 0.27 + fineA * 0.16 + fineB * 0.09;
float activeRegion = smoothstep(-0.01, 0.23, activeField);
float activeIsland = smoothstep(0.12, 0.36, activeField);
float activeFragments = smoothstep(-0.10, 0.30,
    middle * 0.58 + fineA * 0.51 + fineB * 0.34 + dust * 0.20);
float whitePatchMask = activeIsland * lerp(0.24, 1.0, activeFragments)
                     * spatialDetail;
float plage = activeRegion * lerp(0.40, 1.0, activeFragments) * spatialDetail;

// Thousands of irregular bright grains with shallow lanes. No abs(noise),
// nearest-cell distance or normal perturbation can form raised/golf-ball rims.
float granuleSignal = fineA * 0.59 + fineB * 0.30 + dust * 0.17;
float granuleCell = saturate(0.50 + granuleSignal * 1.15);
float grainContrast = lerp(0.12, 0.42, granulationStrength);
float granulation = granuleSignal * grainContrast * spatialDetail;
float grainSpark = smoothstep(0.20, 0.49, granuleSignal)
                 * activeRegion * spatialDetail;
float intergranularLane = smoothstep(0.10, 0.38, -granuleSignal)
                        * fineDetailA * spatialDetail;

// Sparse cool pores are embedded in activity rather than a broad brown tint.
// They remain an intensity deficit of the local spectrum, including blue stars.
float magneticField = macroConvection * 0.72 - mesoCells * 0.28;
float spotPenumbra = smoothstep(0.35, 0.58, magneticField) * spatialDetail;
float spotCore = smoothstep(0.46, 0.66, magneticField + middle * 0.16)
               * spatialDetail;
float spots = (spotPenumbra * 0.34 + spotCore * 0.66)
            * saturate(SpotAmount);
float surface = max(0.24, 0.78
    + (macroConvection * 0.10 + mesoCells * 0.14 + middle * 0.18)
      * lerp(0.65, 1.35, variation) * spatialDetail
    + granulation - intergranularLane * 0.08 - spots * 0.55);

// A luminous surface retains a modest centre-to-limb rolloff. Avoid brightening
// a whole broad rim: the corona shader owns off-disc emission and prominences.
float limb = lerp(0.63, 1.0, pow(facing, 0.48));
float rim = pow(1.0 - facing, 3.15);
float prominenceMask = activeIsland * spatialDetail;
float temporalFlicker = 1.0 + actorOnly * 0.012
    * sin(GameTime * lerp(0.060, 0.078, seed) + phase);
float localHeatBreathing = 1.0 + actorOnly * 0.020
    * sin(GameTime * 0.052 + phase + mesoCells * 2.1);

float maxSpectral = max(max(spectralColor.r, spectralColor.g), spectralColor.b);
float3 normalizedSpectralTint = spectralColor / max(maxSpectral, 0.001);
float spectralVisibility = smoothstep(0.08, 0.90, maxSpectral);
float3 spectralTint = lerp(spectralColor, normalizedSpectralTint, 0.36)
                    * spectralVisibility;
float warmth = saturate((normalizedSpectralTint.r - normalizedSpectralTint.b) * 2.2);
// The quiet plasma has richer spectral colour than the white-hot active regions.
// For a G star this stops the tonemapped result becoming a uniform beige ball.
float quietExponent = lerp(1.15, 2.75, warmth);
float3 quietTint = pow(max(lerp(spectralColor, normalizedSpectralTint, 0.36), 0.0001),
    quietExponent) * spectralVisibility;
float3 hotTint = lerp(spectralTint,
    float3(spectralVisibility, spectralVisibility, spectralVisibility), 0.46);
float3 whiteHotTint = lerp(spectralTint,
    float3(spectralVisibility, spectralVisibility, spectralVisibility), 0.82);
float cellHeat = saturate(0.26 + granuleCell * 0.30 + plage * 0.24);
float3 surfaceTint = lerp(quietTint, spectralTint, cellHeat * 0.38);
surfaceTint = lerp(surfaceTint, surfaceTint * float3(1.00, 0.56, 0.24),
    protostarType * 0.46);
surfaceTint = lerp(surfaceTint, spectralTint * float3(0.62, 0.78, 1.00),
    saturate(compactType * 0.40 + pulsarType * 0.55));

// The background radiance remains below the broad filmic shoulder; localized
// hot plages provide the range and bloom that make the disc incandescent.
float actorTone = lerp(1.94, 3.24, actorActivity);
float proxyTone = lerp(1.35, 3.80, proxyActivity);
float toneSafeEmission = lerp(actorTone, proxyTone, useInstance);
float activeBloom = plage * lerp(0.79, 1.48, emissionActivity);
float whitePatchBloom = (whitePatchMask * lerp(2.42, 5.94, emissionActivity)
                     + grainSpark * lerp(0.88, 1.98, emissionActivity))
                     * localHeatBreathing;
float typeCoronaGain = 1.0 + giantType * 0.24 + protostarType * 0.52
                     + compactType * 0.16 + pulsarType * 0.34
                     - coolDwarfType * 0.28;
float coronaBloom = actorOnly * CoronaAmount * pow(rim, 3.8)
    * lerp(0.32, 1.4, emissionActivity) * typeCoronaGain
    * (0.08 + prominenceMask * 0.92);
float polarCap = pow(axisAlignment, 14.0);
float compactLift = compactType * polarCap * 1.45;
float pulsarPulse = 0.58 + 0.42 * sin(GameTime * 6.4 + phase);
float pulsarLift = pulsarType * pow(axisAlignment, 28.0)
                  * lerp(2.2, 4.8, pulsarPulse);
float stellarSignal = toneSafeEmission * surface * limb
                    + compactLift + pulsarLift;
float unresolvedProxy = useInstance * (1.0 - spatialDetail);
float unresolvedProxyBloom = unresolvedProxy * lerp(0.16, 0.72, proxyActivity);
float proxyOpticalEnvelope = lerp(1.0,
    1.0 - smoothstep(0.48, 0.78, projectedRadiusSq), unresolvedProxy);
float3 ordinaryPreBloom = (surfaceTint * stellarSignal * temporalFlicker
    + hotTint * activeBloom * limb
    + whiteHotTint * (whitePatchBloom * limb + coronaBloom + unresolvedProxyBloom))
    * proxyOpticalEnvelope;


)APSSTELLAR");
		// MSVC limits a single wide string literal to 16,380 characters. Keep the
		// authored shader as one final FString while splitting only its C++ storage.
		const FString StellarOutputCode = TEXT(R"APSSTELLAR(
// A black hole is deliberately not resurrected as a glowing sphere. The centre
// stays dark while a thin hot accretion band and a sharp photon rim carry HDR.
// Bloom expands those bounded structures beyond the mesh silhouette into the
// expected compact halo without translucent overdraw on tens of thousands of HISM.
float diskLatitude = abs(dot(n, stellarAxis));
float accretionBand = exp2(-diskLatitude * diskLatitude * 92.0);
float diskTangent = dot(n, normalize(cross(stellarAxis, v) + 0.0001));
float dopplerAsymmetry = lerp(0.62, 1.38, diskTangent * 0.5 + 0.5);
float photonRing = smoothstep(0.56, 0.94, 1.0 - facing);
float blackHoleSignal = blackHoleType
    * (accretionBand * dopplerAsymmetry * 5.4 + photonRing * 6.8);
float3 blackHoleTint = lerp(float3(1.00, 0.26, 0.035),
                            float3(1.00, 0.72, 0.28),
                            saturate(accretionBand * 0.72 + photonRing * 0.28));
float3 preBloom = ordinaryPreBloom * (1.0 - blackHoleType)
                + blackHoleTint * blackHoleSignal;
float outputCeiling = lerp(9.0, 6.5, useInstance);
float peakChannel = max(max(preBloom.r, preBloom.g), preBloom.b);
return preBloom * min(1.0, outputCeiling / max(peakChannel, 0.0001));
)APSSTELLAR");
		StellarSurface->Code = StellarPatternCode + StellarLightingCode + StellarOutputCode;
		AddCustomInput(StellarSurface, TEXT("ParamColor"), Color);
		AddCustomInput(StellarSurface, TEXT("ParamEmission"), Multiplier);
		AddCustomInput(StellarSurface, TEXT("ParamSeed"), SurfaceSeed);
		AddCustomInput(StellarSurface, TEXT("Variation"), SurfaceVariation);
		AddCustomInput(StellarSurface, TEXT("Granulation"), GranulationStrength);
		AddCustomInput(StellarSurface, TEXT("SpotAmount"), SpotStrength);
		AddCustomInput(StellarSurface, TEXT("CoronaAmount"), CoronaStrength);
		AddCustomInput(StellarSurface, TEXT("StellarType"), StellarArchetype);
		AddCustomInput(StellarSurface, TEXT("InstanceColor"), InstanceColor);
		AddCustomInput(StellarSurface, TEXT("InstanceEmission"), InstanceEmission);
		AddCustomInput(StellarSurface, TEXT("InstanceSeed"), InstanceSeed);
		AddCustomInput(StellarSurface, TEXT("SystemMarker"), SystemHighlight);
		AddCustomInput(StellarSurface, TEXT("NormalWS"), Normal);
		AddCustomInput(StellarSurface, TEXT("WorldPositionWS"), WorldPosition);
		AddCustomInput(StellarSurface, TEXT("ObjectPositionWS"), InterpolatedObjectPosition);
		AddCustomInput(StellarSurface, TEXT("GameTime"), GameTime);
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

	bool RebuildPointAndCoronaMaterial(UMaterial* Material, const bool bStablePointPass = false)
	{
		if (!Material)
		{
			return false;
		}

		Material->Modify();
		while (!Material->GetExpressions().IsEmpty())
		{
			UMaterialExpression* Expression = Material->GetExpressions().Last();
			if (IsValid(Expression) && Expression->IsRooted())
			{
				Expression->RemoveFromRoot();
			}
			UMaterialEditingLibrary::DeleteMaterialExpression(Material, Expression);
		}
		Material->MaterialDomain = MD_Surface;
		Material->BlendMode = BLEND_Additive;
		// TSR reconstructs these subpixel lights differently during motion and rest.
		// Only catalogue points use the unjittered late pass; physical coronas keep
		// their accepted pass and the rest of the scene retains temporal AA.
		Material->TranslucencyPass = bStablePointPass ? MTP_AfterMotionBlur : MTP_AfterDOF;
		Material->bDisableDepthTest = false;
		Material->SetShadingModel(MSM_Unlit);
		Material->TwoSided = false;
		Material->DitheredLODTransition = false;
		Material->DitherOpacityMask = false;
		Material->bUsedWithInstancedStaticMeshes = true;

		UMaterialExpressionVectorParameter* Color = AddVectorParameter(
			Material, TEXT("Color"), FLinearColor(1.0f, 0.66f, 0.30f), -1050, -520, 0);
		UMaterialExpressionScalarParameter* CoronaIntensity = AddScalarParameter(
			Material, TEXT("CoronaIntensity"), 8.0f, 0.0f, 256.0f, -1050, -420, 1);
		UMaterialExpressionScalarParameter* CoronaOpacity = AddScalarParameter(
			Material, TEXT("CoronaOpacity"), 0.72f, 0.0f, 1.0f, -1050, -320, 2);
		UMaterialExpressionScalarParameter* CoronaSeed = AddScalarParameter(
			Material, TEXT("CoronaSeed"), 0.371f, 0.0f, 1.0f, -1050, -220, 3);
		UMaterialExpressionScalarParameter* CoronaShellMode = AddScalarParameter(
			Material, TEXT("CoronaShellMode"), 0.0f, 0.0f, 1.0f, -1050, -120, 4);
		UMaterialExpressionScalarParameter* CoronaInnerRadius = AddScalarParameter(
			Material, TEXT("CoronaInnerRadius"), 0.8064516f, 0.50f, 0.95f, -1050, -20, 5);
		UMaterialExpressionScalarParameter* GameplayPointProfile = AddScalarParameter(
			Material, TEXT("GameplayPointProfile"), 0.0f, 0.0f, 1.0f, -1050, 90, 6);

		UMaterialExpression* InstanceColor = AddReflectedExpression(Material,
			TEXT("/Script/Engine.MaterialExpressionPerInstanceCustomData3Vector"), -820, -520);
		UMaterialExpression* InstanceEmission = AddReflectedExpression(Material,
			TEXT("/Script/Engine.MaterialExpressionPerInstanceCustomData"), -820, -410);
		UMaterialExpression* InstanceSeed = AddReflectedExpression(Material,
			TEXT("/Script/Engine.MaterialExpressionPerInstanceCustomData"), -820, -300);
		UMaterialExpression* SystemHighlight = AddReflectedExpression(Material,
			TEXT("/Script/Engine.MaterialExpressionPerInstanceCustomData"), -820, -190);
		UMaterialExpression* InstanceLuminosityGain = AddReflectedExpression(Material,
			TEXT("/Script/Engine.MaterialExpressionPerInstanceCustomData"), -820, 360);
		UMaterialExpression* InstanceOpticalCoreScale = AddReflectedExpression(Material,
			TEXT("/Script/Engine.MaterialExpressionPerInstanceCustomData"), -820, 450);
		UMaterialExpression* InstanceRayStrength = AddReflectedExpression(Material,
			TEXT("/Script/Engine.MaterialExpressionPerInstanceCustomData"), -820, 540);
		UMaterialExpression* Normal = AddReflectedExpression(Material,
			TEXT("/Script/Engine.MaterialExpressionPixelNormalWS"), -820, -80);
		UMaterialExpression* WorldPosition = AddReflectedExpression(Material,
			TEXT("/Script/Engine.MaterialExpressionWorldPosition"), -820, 30);
		UMaterialExpression* ObjectPosition = AddReflectedExpression(Material,
			TEXT("/Script/Engine.MaterialExpressionObjectPositionWS"), -820, 140);
		UMaterialExpression* InterpolatedObjectPosition = AddReflectedExpression(Material,
			TEXT("/Script/Engine.MaterialExpressionVertexInterpolator"), -600, 140);
		UMaterialExpressionCustom* PointProjection =
			AddExpression<UMaterialExpressionCustom>(Material, -1050, 520);
		UMaterialExpression* InterpolatedPointProjection = AddReflectedExpression(Material,
			TEXT("/Script/Engine.MaterialExpressionVertexInterpolator"), -600, 520);
		UMaterialExpressionCameraVectorWS* Camera =
			AddExpression<UMaterialExpressionCameraVectorWS>(Material, -820, 250);
		UMaterialExpressionCustom* PointAndCorona =
			AddExpression<UMaterialExpressionCustom>(Material, -430, -260);
		if (!Color || !CoronaIntensity || !CoronaOpacity || !CoronaSeed
			|| !CoronaShellMode || !CoronaInnerRadius || !GameplayPointProfile
			|| !InstanceColor || !InstanceEmission || !InstanceSeed
			|| !SystemHighlight || !InstanceLuminosityGain || !InstanceOpticalCoreScale || !InstanceRayStrength || !Normal || !WorldPosition || !ObjectPosition
			|| !InterpolatedObjectPosition || !PointProjection || !InterpolatedPointProjection
			|| !Camera || !PointAndCorona)
		{
			return false;
		}
		// Vertex-only instance transform: pixel ObjectPosition/Radius are component bounds.
		// Keep the physical centre in LWC until translated, then project with the same
		// jittered matrix as the rasterizer. The carrier's triangle normals never enter.
		PointProjection->Description = TEXT("APS per-instance optical point projection");
		PointProjection->OutputType = CMOT_Float4;
		PointProjection->Code = TEXT(R"APSPROJECTION(
float4x4 instanceToTranslated = DFFastToTranslatedWorld(
    GetInstanceToWorldDF(Parameters), ResolvedView.PreViewTranslation);
float4 centreClip = mul(float4(instanceToTranslated[3].xyz, 1.0),
    ResolvedView.TranslatedWorldToClip);
float3 axis = instanceToTranslated[0].xyz;
float axisMax = max(max(abs(axis.x), abs(axis.y)), abs(axis.z));
float uniformScale = axisMax * length(axis / max(axisMax, 1.0e-20));
float3 localExtent = GetPrimitiveData(Parameters).InstanceLocalBoundsExtent;
float opticalRadius = max(max(localExtent.x, localExtent.y), localExtent.z) * uniformScale;
if (!all(isfinite(centreClip)) || !isfinite(opticalRadius)
    || centreClip.w <= 0.0 || opticalRadius <= 1.0e-20)
    return float4(0.0, 0.0, -1.0, -1.0);
float2 inverseRadiusNDC = (centreClip.w / opticalRadius)
    / float2(ResolvedView.ViewToClip[0][0], ResolvedView.ViewToClip[1][1]);
if (!all(isfinite(inverseRadiusNDC)) || any(inverseRadiusNDC <= 0.0))
    return float4(0.0, 0.0, -1.0, -1.0);
return float4(centreClip.xy / centreClip.w, inverseRadiusNDC);
)APSPROJECTION");
		if (!UMaterialEditingLibrary::ConnectMaterialExpressions(
			PointProjection, TEXT(""), InterpolatedPointProjection, TEXT("VS")))
		{
			return false;
		}
		if (!UMaterialEditingLibrary::ConnectMaterialExpressions(
			ObjectPosition, TEXT(""), InterpolatedObjectPosition, TEXT("VS")))
		{
			return false;
		}
		if (!SetUInt32Property(InstanceColor, TEXT("DataIndex"), 0)
			|| !SetLinearColorProperty(InstanceColor, TEXT("ConstDefaultValue"), FLinearColor::Black)
			|| !SetUInt32Property(InstanceEmission, TEXT("DataIndex"), 3)
			|| !SetFloatProperty(InstanceEmission, TEXT("ConstDefaultValue"), 0.0f)
			|| !SetUInt32Property(InstanceSeed, TEXT("DataIndex"), 4)
			|| !SetFloatProperty(InstanceSeed, TEXT("ConstDefaultValue"), 0.0f)
			|| !SetUInt32Property(SystemHighlight, TEXT("DataIndex"), 5)
			|| !SetFloatProperty(SystemHighlight, TEXT("ConstDefaultValue"), 0.0f)
			|| !SetUInt32Property(InstanceLuminosityGain, TEXT("DataIndex"), 6)
			|| !SetFloatProperty(InstanceLuminosityGain, TEXT("ConstDefaultValue"), 1.0f)
			|| !SetUInt32Property(InstanceOpticalCoreScale, TEXT("DataIndex"), 11)
			|| !SetFloatProperty(InstanceOpticalCoreScale, TEXT("ConstDefaultValue"), 1.0f)
			|| !SetUInt32Property(InstanceRayStrength, TEXT("DataIndex"), 12)
			|| !SetFloatProperty(InstanceRayStrength, TEXT("ConstDefaultValue"), 0.0f))
		{
			return false;
		}

		PointAndCorona->Description = TEXT("APS HDR point star and actor corona shell");
		PointAndCorona->OutputType = CMOT_Float3;
		PointAndCorona->Inputs.Reset();
		PointAndCorona->Code = TEXT(R"APSPOINT(
float shellMode = step(0.5, CoronaShellMode);
float3 instanceTint = max(InstanceColor.rgb, 0.0);
float maxSpectral = max(max(instanceTint.r, instanceTint.g), instanceTint.b);
float spectralVisibility = smoothstep(0.025, 0.35, maxSpectral);
float3 normalizedTint = instanceTint / max(maxSpectral, 0.001);
float3 pointTint = lerp(instanceTint, normalizedTint, 0.72) * spectralVisibility;
float rawEmission = max(InstanceEmission, 0.0);
float activity = saturate(log2(1.0 + rawEmission) / 8.97);
float marker = saturate(SystemMarker);
float seed = frac(lerp(InstanceSeed, CoronaSeed, shellMode));
float3 n = normalize(NormalWS);
float3 v = normalize(CameraWS);
// Gameplay may weight the optical energy differently, but every catalogue point
// uses the same raster-space geometry and spatial filtering in every view.
// The materialized star's corona remains independent of this energy control.
float gameplayProfile = saturate(GameplayPointProfile) * (1.0 - shellMode);
float facing = saturate(abs(dot(n, v)));
float projectedRadiusSq = saturate(1.0 - facing * facing);
float3 screenX = normalize(ResolvedView.ViewRight);
float3 screenY = normalize(ResolvedView.ViewUp);
// This is a local optical glyph at the actual instance centre, not a sky shell.
// All non-shell points use exact clip-space coordinates below. The continuous
// radial reconstruction is only a guarded fallback for malformed projection data,
// never a different menu/galaxy representation on low-poly carrier triangles.
float3 pointRadial = WorldPositionWS - ObjectPositionWS;
float pointRadialLengthSq = dot(pointRadial, pointRadial);
float3 pointNormal = pointRadialLengthSq > 1.0e-8
    ? pointRadial * rsqrt(pointRadialLengthSq) : n;
float3 pointScreenVector = pointNormal - v * dot(pointNormal, v);
float2 pointQ = float2(dot(pointScreenVector, screenX),
                       dot(pointScreenVector, screenY));
float projectionValid = step(1.0e-8, pointRadialLengthSq);
projectedRadiusSq = lerp(projectedRadiusSq, saturate(dot(pointQ, pointQ)),
                         projectionValid);
if (shellMode < 0.5 && all(isfinite(PointProjection)) && all(PointProjection.zw > 0.0))
{
    float4 pixelClip = GetScreenPosition(Parameters);
    float2 candidateQ = (pixelClip.xy / max(pixelClip.w, 1.0e-20) - PointProjection.xy)
        * PointProjection.zw;
    if (all(isfinite(candidateQ)))
    {
        // Beyond twice the carrier radius the envelope is already black. Clamp
        // before squaring so malformed bounds cannot contaminate other profiles.
        pointQ = clamp(candidateQ, -2.0, 2.0);
        projectedRadiusSq = dot(pointQ, pointQ);
        projectionValid = 1.0;
    }
}


// A sphere is only the conservative HISM bound. Its visible signal is a compact
// Gaussian point: a resolved hot HDR core plus a broader low-energy halo. Brighter
// stars receive a wider halo, while black pixels in the additive material are
// genuinely transparent instead of forming pastel opaque discs.
float pointActivity = saturate(activity + marker * 0.14);
float rayTemperature = sqrt(saturate(normalizedTint.g * normalizedTint.b));
float2 pointDx = ddx(pointQ);
float2 pointDy = ddy(pointQ);
float pointPixelFootprint = max(length(pointDx), length(pointDy));
float apparentRadiusPixels = rcp(max(pointPixelFootprint, 0.001));
// The CPU publishes carrier/core ratio explicitly. Derivatives describe pixel
// filtering only: a genuinely resolved star must never be inferred to be a point.
float opticalCarrierScale = isfinite(InstanceOpticalCoreScale)
    ? clamp(InstanceOpticalCoreScale, 1.0, 6.363637) : 1.0;
float2 opticalQ = pointQ * opticalCarrierScale;
float opticalRadiusSq = dot(opticalQ, opticalQ);
// At the accepted gameplay support the old core was mostly sub-pixel. Broaden
// its Gaussian and lower its peak together, retaining its integrated energy
// before the model-luminosity weight. No time, camera-speed or motion-history gain.

)APSPOINT");
		PointAndCorona->Code += TEXT(R"APSPOINT(
float coreSpread = lerp(1.0, 0.65, gameplayProfile);
float coreSharpness = lerp(28.0, 14.0, pointActivity) * coreSpread;
float haloSharpness = lerp(5.50, 3.00, pointActivity);
float edgeFade = smoothstep(0.02, 0.28, facing);
// The low-poly mesh is a bound, never the halo's visible edge. Fade to black
// inside that silhouette instead of filling triangles with a coverage floor.
float roundEnvelope = 1.0 - smoothstep(0.42, 0.76, opticalRadiusSq);
edgeFade = lerp(edgeFade, roundEnvelope, projectionValid);
float hotCore = exp2(-opticalRadiusSq * coreSharpness) * edgeFade;
float softHalo = exp2(-opticalRadiusSq * haloSharpness) * edgeFade;
if (shellMode < 0.5)
{
    // Spatial prefilter for unresolved points: convolve each Gaussian with the
    // pixel footprint's matched covariance. Compensate the peak, never add energy.
    // This is independent of movement, time and temporal-history accumulation.
    float2 qDx = ddx(opticalQ);
    float2 qDy = ddy(opticalQ);
    const float pixelVariance = 1.0 / 12.0;
    float covXX = pixelVariance * (qDx.x*qDx.x + qDy.x*qDy.x);
    float covXY = pixelVariance * (qDx.x*qDx.y + qDy.x*qDy.y);
    float covYY = pixelVariance * (qDx.y*qDx.y + qDy.y*qDy.y);
    float2 sigmaSq = rcp(1.38629436112 * float2(coreSharpness, haloSharpness));
    float2 filteredXX = sigmaSq + covXX;
    float2 filteredYY = sigmaSq + covYY;
    float2 determinant = max(filteredXX * filteredYY - covXY * covXY, 1.0e-12);
    float2 quadratic = (filteredYY * (opticalQ.x*opticalQ.x)
        - 2.0 * covXY * (opticalQ.x*opticalQ.y)
        + filteredXX * (opticalQ.y*opticalQ.y)) / determinant;
    float2 filteredLobes = sigmaSq * rsqrt(determinant) * exp(-0.5 * quadratic);
    filteredLobes = all(isfinite(filteredLobes))
        ? max(filteredLobes, 0.0) : float2(0.0, 0.0);
    hotCore = filteredLobes.x * edgeFade;
    softHalo = filteredLobes.y * edgeFade;
}
float seedGain = lerp(0.86, 1.14, frac(seed * 17.713 + 0.37));
// Original model luminosity arrives separately from the clamped, area-prefiltered
// emission channel. A luminous red giant must not be dimmed as a cool dwarf.
float modelGain = lerp(1.0, clamp(InstanceLuminosityGain, 0.0, 1.2), gameplayProfile);
float coreEnergy = lerp(4.8, 12.5, activity) * seedGain * (1.0 + marker * 0.22)
                 * coreSpread * modelGain;
float haloEnergy = lerp(0.82, 3.2, activity) * seedGain * (1.0 + marker * 0.18)
                 * lerp(1.0, 0.80, gameplayProfile) * modelGain;
// Keep a hot core, but do not bleach cool stars merely because their surface
// becomes unresolved. Partial luminance compensation preserves visibility
// without pushing the red channel as far into the tonemapper's white shoulder.
float coreWhitening = lerp(0.58, 0.78, activity);
float3 neutralCoreTint = float3(
    spectralVisibility, spectralVisibility, spectralVisibility);
float3 hotCoreTint = lerp(pointTint, neutralCoreTint, coreWhitening);
float pointTemperature = sqrt(saturate(normalizedTint.g * normalizedTint.b));
float warmCoreWeight = 1.0 - smoothstep(0.42, 0.72, pointTemperature);
if (warmCoreWeight > 0.0)
{
    float3 chromaCore = lerp(pointTint, neutralCoreTint, lerp(0.12, 0.30, activity));
    float3 luminanceWeights = float3(0.2126, 0.7152, 0.0722);
    float luminanceRatio = dot(hotCoreTint, luminanceWeights)
        / max(dot(chromaCore, luminanceWeights), 1.0e-6);
    hotCoreTint = lerp(hotCoreTint, chromaCore * sqrt(max(luminanceRatio, 0.0)), warmCoreWeight);
}
float3 haloTint = lerp(pointTint, neutralCoreTint, 0.04);
float3 pointSignal = hotCoreTint * (hotCore * coreEnergy)
                   + haloTint * (softHalo * haloEnergy);
// Finite diffraction spokes make a resolved catalogue object read as emitted
// light instead of a coloured dot. The green/blue spectral content controls both
// reach and energy: white/blue stars receive longer rays, but a bright warm star
// also receives a visible spectral cross. The CPU ranks apparent brightness and
// publishes eligibility explicitly; most cluster members remain compact points.
float rayCarrierGate = isfinite(InstanceRayStrength) ? saturate(InstanceRayStrength) : 0.0;
// Compact catalogue members need neither an extended carrier nor ray shading.
if (shellMode < 0.5 && rayCarrierGate == 0.0)
    return pointSignal;
// Emission is area-prefiltered before reaching the material. In distant views it
// can be far below one even for an optically selected luminous star; do not gate
// that selection a second time or force its rays back to the minimum energy.
float rayActivity = max(activity, rayCarrierGate * 0.35);
// A selected luminous point has a small spectral aura beyond its compact core.
// Use its explicit carrier, not opticalQ (which intentionally preserves core
// size). The Gaussian dies inside the conservative mesh; no filled disc or haze
// is added to the unselected majority of the catalogue.
float jewelHalo = exp2(-projectedRadiusSq * lerp(16.0, 8.0, rayCarrierGate))
                * (1.0 - smoothstep(0.36, 0.62, projectedRadiusSq));
float jewelHaloEnergy = lerp(0.14, 0.44, rayCarrierGate) * rayCarrierGate
                      * seedGain * modelGain;
pointSignal += haloTint * (jewelHalo * jewelHaloEnergy * projectionValid);

)APSPOINT");
		PointAndCorona->Code += TEXT(R"APSPOINT(
float raySpectralReach = smoothstep(0.16, 0.92, rayTemperature);
// Actual K/M palette values extend above the ray-reach threshold. Keep their
// accepted optics unchanged, and blend the correction continuously into G/F/A/B/O.
float raySoftening = smoothstep(0.42, 0.88, rayTemperature);
// Retain the approved warm/red optical profile exactly. Brighter white/blue
// shafts need a luminous root, not a longer isolated, nearly uniform line.
// Only selected unresolved points reach this branch; surface/corona are separate.
if (raySoftening > 0.0)
{
    float shoulderSpread = 1.0 + 64.0 * 0.11552453
        * pointPixelFootprint * pointPixelFootprint;
    float hotShoulder = exp2(-projectedRadiusSq * 64.0 / shoulderSpread) / shoulderSpread;
    pointSignal += lerp(haloTint, neutralCoreTint, 0.70)
        * (hotShoulder * 1.2 * raySoftening * rayCarrierGate
           * seedGain * modelGain * projectionValid);
}
float rayReach = lerp(0.72, 1.00, raySpectralReach)
               * lerp(0.90, 1.0, rayActivity);
float2 rayQ = abs(pointQ);
float primaryAlong = max(rayQ.x, rayQ.y);
float primaryAcross = min(rayQ.x, rayQ.y);
// A narrow native-pixel shaft stays readable without becoming a thick cross.
// Width follows the actual raster footprint, not carrier size; length can grow
// on ultrawide views without also thickening the shaft. Keep a finite AA floor.
float primaryWidth = max(pointPixelFootprint * 0.40, 0.006);
float primaryAngular = exp2(-primaryAcross * primaryAcross
                           / max(primaryWidth * primaryWidth, 1.0e-6));
if (raySoftening > 0.0)
{
    // Root is rounded; the transverse width and axial intensity both taper.
    // Integrating subpixel shafts preserves their continuity between pixels.
    float shaftTaper = smoothstep(0.08, max(rayReach * 0.78, 0.09), primaryAlong);
    float taperedWidth = max(pointPixelFootprint * lerp(0.66, 0.30, shaftTaper), 0.006);
    float primaryVariance = taperedWidth * taperedWidth;
    float filteredPrimaryVariance = primaryVariance
        + pointPixelFootprint * pointPixelFootprint * 0.11552453;
    float filteredPrimary = sqrt(primaryVariance / max(filteredPrimaryVariance, 1.0e-6))
        * exp2(-primaryAcross * primaryAcross / max(filteredPrimaryVariance, 1.0e-6));
    primaryAngular = lerp(primaryAngular, filteredPrimary, raySoftening);
    primaryWidth = lerp(primaryWidth, taperedWidth, raySoftening);
}
float primaryWing = exp2(-primaryAcross * primaryAcross
                        / max(primaryWidth * primaryWidth * 4.0, 1.0e-6));
float primaryLength = exp2(-primaryAlong * (lerp(3.6, 1.45, raySpectralReach) + 1.75 * raySoftening))
                    * (1.0 - smoothstep(rayReach * 0.68, rayReach, primaryAlong))
                    * smoothstep(0.055, 0.14, primaryAlong);
float diagonalAlong = (rayQ.x + rayQ.y) * 0.70710678;
float diagonalAcross = abs(rayQ.x - rayQ.y) * 0.70710678;
float diagonalWidth = max(pointPixelFootprint * 0.38, 0.008);
float diagonalAngular = exp2(-diagonalAcross * diagonalAcross
                            / max(diagonalWidth * diagonalWidth, 1.0e-6));
float diagonalLength = exp2(-diagonalAlong * lerp(6.4, 2.8, raySpectralReach))
                     * (1.0 - smoothstep(rayReach * 0.48,
                                         rayReach * 0.76, diagonalAlong))
                     * smoothstep(0.12, 0.24, diagonalAlong);
float rayTemperatureGain = lerp(0.24, 1.0,
                                raySpectralReach * raySpectralReach);
float rayEnvelope = 1.0 - smoothstep(0.60, 0.78, projectedRadiusSq);
float finiteRays = ((primaryAngular + primaryWing * 0.06) * primaryLength
                  + diagonalAngular * diagonalLength * 0.07)
                 * rayTemperatureGain * rayCarrierGate
                 * rayEnvelope * projectionValid;
float rayEnergy = lerp(0.45, 6.8, saturate(rayActivity * 3.2)) * seedGain
                * (1.0 + marker * 0.25) * modelGain;
float3 rayTint = lerp(haloTint, neutralCoreTint,
                      0.30 + raySpectralReach * 0.42);
pointSignal += rayTint * (finiteRays * rayEnergy);
// The gameplay profile broadens/dims the compact core; its white ray roots
// must blend into that core instead of forming a separate bright plus sign.
// Move inner-shaft energy into a filtered round root, keeping the ray tips,
// warm-ray shapes and preview profile intact. This is not an exposure boost.
float gameplayHot = gameplayProfile * raySoftening;
if (gameplayHot > 0.0)
{
    float innerWeight = 1.0 - smoothstep(0.30, 0.55, primaryAlong);
    float innerPrimary = (primaryAngular + primaryWing * 0.06) * primaryLength
        * rayEnvelope * innerWeight;
    float bridgeSpread = 1.0 + 36.0 * 0.11552453
        * pointPixelFootprint * pointPixelFootprint;
    float bridge = exp2(-projectedRadiusSq * 36.0 / bridgeSpread) / bridgeSpread;
    float redistribution = bridge * (3.0 * pointPixelFootprint)
        - 0.40 * innerPrimary;
    pointSignal += rayTint * (redistribution * gameplayHot * rayEnergy
        * rayTemperatureGain * rayCarrierGate * projectionValid);
}
// CoronaShellMode is uniform for the whole draw. Keep the increasingly detailed
// actor corona out of the hot path used by thousands of catalogue point pixels.
if (shellMode < 0.5)
    return pointSignal;


)APSPOINT");
		PointAndCorona->Code += TEXT(R"APSPOINT(
// Drop-in actor-corona body, after the point-mode early return. The sphere is
// only a raster carrier; all emission has faded before its outer silhouette.
float3 shellRadial = WorldPositionWS - ObjectPositionWS;
float shellRadialLengthSq = dot(shellRadial, shellRadial);
float3 shellNormal = shellRadialLengthSq > 1.0e-8
    ? shellRadial * rsqrt(shellRadialLengthSq) : n;
float shellFacing = saturate(abs(dot(shellNormal, v)));
float shellProjectedRadiusSq = saturate(1.0 - shellFacing * shellFacing);
float shellProjectedRadius = sqrt(shellProjectedRadiusSq);
float shellInnerRadius = clamp(CoronaInnerRadius, 0.001, 0.999);
float shellSpan = max(1.0 - shellInnerRadius, 0.001);
// This is the camera ray's impact parameter divided by photosphere radius.
// Unlike a screen-space offset, it remains attached under perspective projection.
float shellRadiusFromLimb = shellProjectedRadius / shellInnerRadius - 1.0;
float shellRadius01 = max(shellRadiusFromLimb, 0.0);
float shellCarrier01 = saturate(
    (shellProjectedRadius - shellInnerRadius) / shellSpan);
// The actor's authored scale already distinguishes subgiants through hypergiants.
// Normalize only the outer plasma coordinate: a main-sequence 1.24 carrier is
// the unit profile, while a hypergiant 1.32 carrier gives 1.333x physical reach.
// The longest plasma fade ends at 0.205/0.24 = 85.4% of the available span,
// and the independent carrier fade remains an additional silhouette guard.
float coronaExtent = clamp((rcp(shellInnerRadius) - 1.0) / 0.24, 0.55, 1.40);
float effectRadiusFromLimb = shellRadiusFromLimb / coronaExtent;
float effectRadius01 = max(effectRadiusFromLimb, 0.0);
// The front shell can overlap the opaque disc; hide that region analytically.
// A narrow underlap gives temporal AA coverage without exposing a black seam.
float photosphereOcclusion = smoothstep(-0.012, 0.001, shellRadiusFromLimb);
float3 coronaDirectionPlane = shellNormal - v * dot(shellNormal, v);
float coronaDirectionLengthSq = dot(coronaDirectionPlane, coronaDirectionPlane);
float3 coronaDirection = coronaDirectionLengthSq > 1.0e-8
    ? coronaDirectionPlane * rsqrt(coronaDirectionLengthSq) : screenX;

float3 seedOffset = frac(float3(seed * 7.13 + 0.17,
                                seed * 13.71 + 0.53,
                                seed * 19.37 + 0.89)) * 13.0;
float coronaBroad = FastGradientPerlinNoise3D_TEX(
    coronaDirection * 4.70 + seedOffset);
float coronaPlasma = FastGradientPerlinNoise3D_TEX(
    coronaDirection * 28.30 + seedOffset.yzx
    + effectRadius01 * float3(13.1, -17.3, 21.9));
float coronaFine = FastGradientPerlinNoise3D_TEX(
    coronaDirection.yzx * 43.70 + seedOffset.zxy
    + effectRadius01 * float3(-29.3, 45.1, 34.7)
    + coronaPlasma * float3(0.73, -0.51, 0.39));
float angularFootprint = max(length(ddx(coronaDirection)),
                             length(ddy(coronaDirection)));
float fineDetail = 1.0 - smoothstep(0.40, 0.95, angularFootprint * 43.70);
coronaFine *= fineDetail;
float coronaUnit = saturate(0.50 + coronaBroad * 0.72);
float plasmaUnit = saturate(0.50 + coronaPlasma * 0.68 + coronaFine * 0.25);

float3 rawShellTint = max(Color.rgb, 0.0);
float shellTintPeak = max(max(rawShellTint.r, rawShellTint.g), rawShellTint.b);
float shellVisibility = smoothstep(0.025, 0.35, shellTintPeak);
float3 normalizedShellTint = rawShellTint / max(shellTintPeak, 0.001);
float shellWarmth = saturate((normalizedShellTint.r - normalizedShellTint.b) * 2.2);
float3 spectralShellTint = pow(max(normalizedShellTint, 0.001),
    lerp(1.15, 2.30, shellWarmth)) * shellVisibility;
float3 hotShellTint = lerp(spectralShellTint,
    float3(shellVisibility, shellVisibility, shellVisibility), 0.27);
// A prominence is translucent plasma, not a white-hot optical spike. Keep its
// quieter filaments markedly warmer than the surface on a G/K/M star.
float3 prominenceTint = pow(max(normalizedShellTint, 0.001),
    lerp(1.22, 3.30, shellWarmth)) * shellVisibility;

// Low radiance falls continuously from the surface. Noise changes its density
// and extent, never forms a second disc or a detached bright annulus.
float contactReach = lerp(0.008, 0.017, plasmaUnit);
float contactFade = 1.0 - smoothstep(contactReach * 0.35,
                                      contactReach, shellRadius01);
float contactGlow = 0.0024 * exp2(-shellRadius01 * 96.0) * contactFade
                  * lerp(0.24, 0.94, plasmaUnit);
float hazeDensity = lerp(0.34, 1.0, coronaUnit)
                  * lerp(0.70, 1.0, plasmaUnit);
float hazeReach = lerp(0.11, 0.185, coronaUnit);
float hazeFade = 1.0 - smoothstep(hazeReach * 0.50, hazeReach, effectRadius01);
float colouredHaze = 0.0018 * exp2(-effectRadius01 * 23.0)
                   * hazeDensity * hazeFade;

)APSPOINT");
		PointAndCorona->Code += TEXT(R"APSPOINT(
// Three differently sized magnetic arches. Their feet lie just beneath the
// photosphere, so the visible upper arcs are attached rather than floating rings.
// A seed-derived world axis controls orientation; there are no cardinal spokes.
float3 loopReference = normalize(float3(
    seed * 2.0 - 0.83,
    frac(seed * 7.13 + 0.31) * 2.0 - 1.0,
    frac(seed * 13.71 + 0.73) * 2.0 - 1.0));
float3 loopAxisPlane = loopReference - v * dot(loopReference, v);
float loopAxisLengthSq = dot(loopAxisPlane, loopAxisPlane);
float3 loopAxis = loopAxisLengthSq > 1.0e-5
    ? loopAxisPlane * rsqrt(loopAxisLengthSq) : screenX;
float3 loopTangent = normalize(cross(v, loopAxis));
float localCos = dot(coronaDirection, loopAxis);
float localSin = dot(coronaDirection, loopTangent);
// Angular centres: 0, 2.03 and 4.42 radians. Unequal gaps prevent a radial icon.
float3 loopAcross = float3(localSin,
    localSin * -0.4432344 - localCos * 0.8964057,
    localSin * -0.2882406 + localCos * 0.9575580);
float3 loopFacing = float3(localCos,
    localCos * -0.4432344 + localSin * 0.8964057,
    localCos * -0.2882406 - localSin * 0.9575580);
float3 loopRandom = frac(seed * float3(11.73, 23.91, 37.17)
                       + float3(0.29, 0.61, 0.87));
float3 loopWidth = float3(0.090, 0.064, 0.118)
                 * lerp(0.82, 1.13, loopRandom);
float3 loopHeight = float3(0.115, 0.078, 0.146)
                  * lerp(0.82, 1.07, loopRandom.zxy);
// Unequal foot-to-apex slopes and leaning axes break the perfect semicircles
// visible in the first capture. The field remains rooted beneath the limb.
float3 archLean = effectRadiusFromLimb * float3(0.24, -0.31, 0.18);
float3 archX = (loopAcross - archLean) / loopWidth;
float3 archHeightBias = clamp(1.0 + archX * float3(0.19, -0.24, 0.27)
    + coronaBroad * 0.22, 0.65, 1.34);
float3 archY = (effectRadiusFromLimb + float3(0.006, 0.003, 0.009))
    / (loopHeight * archHeightBias);
float3 archDistance = (sqrt(archX * archX + archY * archY) - 1.0)
                    * min(loopWidth, loopHeight);
// Small multiscale irregularity and broken density stop the arch looking like
// a smooth neon tube. A soft shoulder joins bright strands to diffuse plasma.
float archWarp = coronaPlasma * 0.0080 + coronaFine * 0.0012;
archDistance += archWarp;
float radialPixel = max(abs(ddx(effectRadiusFromLimb)),
                        abs(ddy(effectRadiusFromLimb)));
float linePixel = max(radialPixel, angularFootprint) * 0.60;
float strandWidth = lerp(0.0028, 0.0058, plasmaUnit);
float3 absArch = abs(archDistance);
float filteredStrandWidth = strandWidth + max(linePixel, 0.00045);
float3 strand = exp2(-absArch * absArch
    / max(filteredStrandWidth * filteredStrandWidth, 1.0e-7));
// A broad, uneven envelope now owns most radiance. The thin strand is a sparse
// internal filament, preventing the former bright jewellery-wire silhouette.
float shoulderWidth = lerp(0.008, 0.019, plasmaUnit);
float3 archShoulder = exp2(-absArch / shoulderWidth);
float3 archGate = smoothstep(0.91, 0.98, loopFacing);
float strandDensity = smoothstep(-0.08, 0.27,
    coronaPlasma + coronaFine * 0.61);
float shoulderDensity = lerp(0.12, 0.90,
    smoothstep(-0.30, 0.31, coronaPlasma - coronaFine * 0.24));
float3 loopWeights = float3(1.0, 0.68, 0.82);
float prominenceLoops = dot((strand * strandDensity * 0.22
    + archShoulder * shoulderDensity * 0.72)
    * archGate, loopWeights);
float prominenceGlow = prominenceLoops * 0.0070
    * (1.0 - smoothstep(0.16, 0.205, effectRadius01));

// Zero energy precedes the silhouette even for an old oversized carrier.
// Actor shells do not carry optical crosses; those belong to distant point LODs.
float carrierSafetyFade = 1.0 - smoothstep(0.82, 0.93, shellCarrier01);
// Match the CPU's bounded type gain; the former 200 clamp flattened giants.
float effectiveCoronaIntensity = min(max(CoronaIntensity, 0.0), 228.0);
float shellEnvelope = photosphereOcclusion * carrierSafetyFade
                    * saturate(CoronaOpacity) * effectiveCoronaIntensity;
return (spectralShellTint * colouredHaze
    + hotShellTint * contactGlow
    + prominenceTint * prominenceGlow) * shellEnvelope;

)APSPOINT");
		// Keep the point-shaping block in a second literal. MSVC diagnoses each raw
		// literal before FString concatenation, so the generated HLSL is identical
		// while every individual C++ token remains safely below its size limit.

		// MSVC limits individual wide string literals. Append the independent corona
		// block at runtime; the resulting HLSL stays byte-identical to the saved master.

		AddCustomInput(PointAndCorona, TEXT("Color"), Color);
		AddCustomInput(PointAndCorona, TEXT("CoronaIntensity"), CoronaIntensity);
		AddCustomInput(PointAndCorona, TEXT("CoronaOpacity"), CoronaOpacity);
		AddCustomInput(PointAndCorona, TEXT("CoronaSeed"), CoronaSeed);
		AddCustomInput(PointAndCorona, TEXT("CoronaShellMode"), CoronaShellMode);
		AddCustomInput(PointAndCorona, TEXT("CoronaInnerRadius"), CoronaInnerRadius);
		AddCustomInput(PointAndCorona, TEXT("InstanceColor"), InstanceColor);
		AddCustomInput(PointAndCorona, TEXT("InstanceEmission"), InstanceEmission);
		AddCustomInput(PointAndCorona, TEXT("InstanceSeed"), InstanceSeed);
		AddCustomInput(PointAndCorona, TEXT("SystemMarker"), SystemHighlight);
		AddCustomInput(PointAndCorona, TEXT("NormalWS"), Normal);
		AddCustomInput(PointAndCorona, TEXT("WorldPositionWS"), WorldPosition);
		AddCustomInput(PointAndCorona, TEXT("ObjectPositionWS"), InterpolatedObjectPosition);
		AddCustomInput(PointAndCorona, TEXT("CameraWS"), Camera);
		AddCustomInput(PointAndCorona, TEXT("GameplayPointProfile"), GameplayPointProfile);
		AddCustomInput(PointAndCorona, TEXT("InstanceLuminosityGain"), InstanceLuminosityGain);
		AddCustomInput(PointAndCorona, TEXT("InstanceOpticalCoreScale"), InstanceOpticalCoreScale);
		AddCustomInput(PointAndCorona, TEXT("InstanceRayStrength"), InstanceRayStrength);
		AddCustomInput(PointAndCorona, TEXT("PointProjection"), InterpolatedPointProjection);
		if (bStablePointPass)
		{
			UMaterialExpression* SceneDepth = AddReflectedExpression(Material,
				TEXT("/Script/Engine.MaterialExpressionSceneDepth"), -1050, 800);
			if (!SceneDepth) return false;
			AddCustomInput(PointAndCorona, TEXT("SceneDepthForOcclusion"), SceneDepth);
			// The late pass can run at native output size while its view uniform
			// buffer retains primary-resolution dimensions. Carry real raster clip
			// coordinates from the vertex stage instead of reconstructing them from
			// SV_Position with that mismatched view. No changes to optical appearance.
			UMaterialExpressionCustom* RasterClip =
				AddExpression<UMaterialExpressionCustom>(Material, -1050, 950);
			UMaterialExpression* InterpolatedRasterClip = AddReflectedExpression(Material,
				TEXT("/Script/Engine.MaterialExpressionVertexInterpolator"), -600, 950);
			if (!RasterClip || !InterpolatedRasterClip) return false;
			RasterClip->Description = TEXT("APS actual raster clip position");
			RasterClip->OutputType = CMOT_Float4;
			RasterClip->Code = TEXT("return GetScreenPosition(Parameters);");
			if (!UMaterialEditingLibrary::ConnectMaterialExpressions(
				RasterClip, TEXT(""), InterpolatedRasterClip, TEXT("VS"))) return false;
			AddCustomInput(PointAndCorona, TEXT("RasterClip"), InterpolatedRasterClip);
			PointAndCorona->Code.ReplaceInline(
				TEXT("float4 pixelClip = GetScreenPosition(Parameters);"),
				TEXT("float4 pixelClip = RasterClip;"));
			// UE5.4 disables hardware depth tests after motion blur. Keep opaque
			// planets/characters in front of points, using reversed device depth;
			// a capped linear sky depth would incorrectly reject full-scale stars.
			const FString PointDepthGuard = TEXT(R"APSPOINTDEPTH(
if (!isfinite(SceneDepthForOcclusion)) return float3(0.0,0.0,0.0);
float2 rasterViewportUV = RasterClip.xy/max(RasterClip.w,1.0e-20)*float2(0.5,-0.5)+0.5;
float2 depthUV=ViewportUVToBufferUV(rasterViewportUV);
float sceneDeviceZ=LookupDeviceZ(depthUV);
clip(Parameters.SvPosition.z-sceneDeviceZ);
)APSPOINTDEPTH");
			PointAndCorona->Code = PointDepthGuard + PointAndCorona->Code;
			PointAndCorona->Description = TEXT("APS stable catalogue point with scene-depth occlusion");
		}
		if (!UMaterialEditingLibrary::ConnectMaterialProperty(
			PointAndCorona, TEXT(""), MP_EmissiveColor))
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
	const FString StablePointPackagePath =
		TEXT("/Game/APS/APS_ALPHA/Assets/Materials/Astro/M_SpectralStarMat_POINTS");
	const TArray<FString> UnifiedMasterPaths = FParse::Param(*Params, TEXT("PointsOnly"))
		? TArray<FString>{ StablePointPackagePath }
		: FParse::Param(*Params, TEXT("OpticsOnly"))
		? TArray<FString>{
			TEXT("/Game/APS/APS_ALPHA/Assets/Materials/Astro/M_SpectralStarMat_HISM"),
			StablePointPackagePath }
		: TArray<FString>{
		TEXT("/Game/APS/APS_ALPHA/Assets/Materials/Astro/M_SpectralStarMat"),
		TEXT("/Game/APS/APS_ALPHA/Assets/Materials/Astro/M_SpectralStarMat_HISM"),
		TEXT("/Game/APS/APS_ALPHA/Assets/Materials/Astro/M_SpectralStarMat_SUN"),
		StablePointPackagePath
	};

	int32 ChangedCount = 0;
	int32 ErrorCount = 0;
	for (const FString& PackagePath : UnifiedMasterPaths)
	{
		const FString AssetName = FPackageName::GetLongPackageAssetName(PackagePath);
		const FString ObjectPath = FString::Printf(TEXT("%s.%s"), *PackagePath, *AssetName);
		const bool bStablePointPass = PackagePath == StablePointPackagePath;
		UMaterial* Material = LoadObject<UMaterial>(nullptr, *ObjectPath);
		if (!Material && bStablePointPass)
		{
			UPackage* NewPackage = CreatePackage(*PackagePath);
			Material = NewObject<UMaterial>(NewPackage, FName(*AssetName), RF_Public | RF_Standalone);
		}
		if (!Material)
		{
			UE_LOG(LogAPSStarMaterialFix, Error, TEXT("Could not load %s"), *ObjectPath);
			++ErrorCount;
			continue;
		}

		const bool bPointAndCoronaMaster = bStablePointPass || PackagePath.EndsWith(TEXT("_HISM"));
		const bool bRebuilt = bPointAndCoronaMaster
			? APSStellarMaterial::RebuildPointAndCoronaMaterial(Material, bStablePointPass)
			: APSStellarMaterial::RebuildUnifiedStellarMaterial(Material);
		if (!bRebuilt)
		{
			UE_LOG(LogAPSStarMaterialFix, Error,
				TEXT("Could not rebuild stellar graph: %s"), *ObjectPath);
			++ErrorCount;
			continue;
		}

		UPackage* Package = Material->GetOutermost();
		// RecompileMaterial queues work; saving immediately can publish a graph whose
		// active-platform shader map failed or has not been finalized. Wait only for
		// this stellar resource, as MaterialEditingLibrary::GetStatistics does in
		// UE 5.4, and refuse to save a material that would use a fallback at runtime.
		FMaterialResource* Resource = Material->GetMaterialResource(GMaxRHIFeatureLevel);
		if (!Resource)
		{
			UE_LOG(LogAPSStarMaterialFix, Error,
				TEXT("No active-platform resource for %s; not saved. Run with -AllowCommandletRendering on the target RHI."),
				*ObjectPath);
			++ErrorCount;
			continue;
		}
		if (!Resource->IsGameThreadShaderMapComplete())
		{
			Resource->SubmitCompileJobs_GameThread(EShaderCompileJobPriority::High);
		}
		Resource->FinishCompilation();
		const TArray<FString>& CompileErrors = Resource->GetCompileErrors();
		for (const FString& CompileError : CompileErrors)
		{
			UE_LOG(LogAPSStarMaterialFix, Error, TEXT("%s: %s"),
				*ObjectPath, *CompileError);
		}
		const FMaterialShaderMap* ShaderMap = Resource->GetGameThreadShaderMap();
		if (!CompileErrors.IsEmpty() || !Resource->IsCompilationFinished()
			|| !Resource->IsGameThreadShaderMapComplete() || !ShaderMap
			|| !ShaderMap->IsCompilationFinalized() || !ShaderMap->CompiledSuccessfully()
			|| !ShaderMap->IsValidForRendering())
		{
			UE_LOG(LogAPSStarMaterialFix, Error,
				TEXT("Stellar shader validation failed for %s (feature level %d); existing package is not overwritten."),
				*ObjectPath, static_cast<int32>(GMaxRHIFeatureLevel));
			++ErrorCount;
			continue;
		}
		UE_LOG(LogAPSStarMaterialFix, Display,
			TEXT("Validated complete stellar shader map: %s (feature level %d)"),
			*ObjectPath, static_cast<int32>(GMaxRHIFeatureLevel));
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
			TEXT("Rebuilt %s stellar material: %s"),
			bStablePointPass ? TEXT("stable catalogue point")
				: bPointAndCoronaMaster ? TEXT("additive point/corona") : TEXT("photosphere"),
			*ObjectPath);
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
