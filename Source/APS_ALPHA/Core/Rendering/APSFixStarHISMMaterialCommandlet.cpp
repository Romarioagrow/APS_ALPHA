#include "APSFixStarHISMMaterialCommandlet.h"

#if WITH_EDITOR
#include "MaterialEditingLibrary.h"
#include "MaterialDomain.h"
#include "Materials/Material.h"
#include "Materials/MaterialExpressionCameraVectorWS.h"
#include "Materials/MaterialExpressionCustom.h"
#include "Materials/MaterialExpressionScalarParameter.h"
#include "Materials/MaterialExpressionVectorParameter.h"
#include "Misc/PackageName.h"
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
			Material, TEXT("CoronaStrength"), 0.20f, 0.0f, 0.45f, -1250, -20, 6);

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
			|| !SpotStrength || !CoronaStrength || !InstanceColor || !InstanceEmission
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
float instanceEnergy = abs(InstanceColor.r) + abs(InstanceColor.g) + abs(InstanceColor.b);
float instanceSignal = instanceEnergy + abs(InstanceEmission)
                     + abs(InstanceSeed) + abs(SystemMarker);
float useInstance = step(0.0001, instanceSignal);
float3 spectralColor = max(lerp(ParamColor.rgb, InstanceColor.rgb, useInstance), 0.001);
float rawEmission = max(lerp(ParamEmission, InstanceEmission, useInstance), 0.0);
float seed = frac(lerp(ParamSeed, InstanceSeed, useInstance));
// The old shader used the authored PixelNormalWS as its pattern coordinate. The
// legacy star sphere has faceted/mirrored normal islands, so a close STAR view
// exposed a square checker even though the procedural waves themselves never
// sampled a tiled texture. A radial coordinate reconstructed from position is
// continuous across those triangles. Pixel-stage ObjectPositionWS is only the
// HISM primitive centre in UE 5.4, so its vertex-stage per-instance value is
// explicitly interpolated before it reaches this custom pixel expression.
float3 radial = WorldPositionWS - ObjectPositionWS;
float radialLengthSq = dot(radial, radial);
float3 n = radialLengthSq > 1.0e-8
    ? radial * rsqrt(radialLengthSq)
    : normalize(NormalWS);
// Normal variation per pixel is a projection-independent footprint estimate.
// Actors keep the exact full-detail path. Resolved HISM spheres retain the same
// stationary surface recipe, while footprints at and below point scale smoothly
// discard alias-prone spatial contrast instead of switching every instance off.
float normalFootprint = max(length(ddx(n)), length(ddy(n)));
float resolvedInstanceDetail =
    1.0 - smoothstep(0.18, 0.65, normalFootprint);
float spatialDetail = lerp(1.0, resolvedInstanceDetail, useInstance);
// Instance highlights are deterministic below; only a materialized actor may use
// GameTime, so a sub-pixel hierarchy point cannot shimmer under temporal AA.
float temporalDetail = 1.0 - useInstance;
float3 v = normalize(CameraWS);
float facing = saturate(dot(n, v));

// Continuous direction-domain waves are seamless over a sphere and remain at
// the same apparent layout from GALAXY/HISM to the full-screen STAR view.  This
// deliberately avoids quantised cube cells (floor/fract grids), which used to
// turn a close star into flat blocks while still reading as an untextured point
// at cluster scale.
float phase = seed * 37.6991118;
// Each octave is a three-axis vector field. A weighted signed-volume term breaks
// up the former gyroid-like pairwise pattern, while a small pairwise remainder
// keeps neighbouring convection cells connected. Lower incommensurate frequencies
// produce a few readable photospheric structures instead of dozens of identical
// stretched grains. The same seamless direction-domain field remains stable on
// HISM proxies and on the actor sphere.
float3 lowDomain = n * 3.4;
float3 domainWarp = sin(
    lowDomain
    + lowDomain.yzx * float3(1.31, -1.43, 1.27)
    + phase * float3(0.37, -0.29, 0.43));
float3 p = normalize(n + domainWarp * 0.045);

float macroConvection = (domainWarp.x * domainWarp.y
                       + domainWarp.y * domainWarp.z
                       + domainWarp.z * domainWarp.x) * 0.333333;

float3 mesoP = p * 7.25 + domainWarp * 1.15
             + macroConvection * float3(0.73, -0.41, 0.29);
float3 mesoWave = sin(mesoP
                    + mesoP.yzx * float3(0.47, -0.61, 0.53)
                    + phase * float3(0.73, -0.67, 0.59));
float mesoPair = (mesoWave.x * mesoWave.y
                + mesoWave.y * mesoWave.z
                + mesoWave.z * mesoWave.x) * 0.333333;
float mesoVolume = mesoWave.x * mesoWave.y * mesoWave.z;
float mesoCells = mesoVolume * 0.68 + mesoPair * 0.32;

float3 microP = p * 18.5 + domainWarp * 2.10
              + mesoCells * float3(-1.17, 0.83, 1.03);
float3 granuleWave = sin(microP
                       + microP.yzx * float3(-0.39, 0.51, 0.43)
                       + phase * float3(1.31, -1.19, 1.07));
float microPair = (granuleWave.x * granuleWave.y
                 + granuleWave.y * granuleWave.z
                 + granuleWave.z * granuleWave.x) * 0.333333;
float microVolume = granuleWave.x * granuleWave.y * granuleWave.z;
float microGranules = microVolume * 0.72 + microPair * 0.28;

// Broad magnetic fields form a few coherent dark spots.  A narrow surrounding
// facular band keeps them organic instead of looking like stamped black dots.
float magneticField = 0.5 + 0.32 * macroConvection + 0.14 * mesoCells;
float spotCore = saturate((magneticField - 0.67) * 7.692308)
               * (0.76 + microGranules * 0.24);
// The two former smoothsteps described the same 0.54..0.82 facular band. A
// triangular response preserves its centre and zero crossings with half the ALU.
float spotHalo = saturate(1.0 - abs(magneticField - 0.68) * 7.142857);
)APSSTELLAR");
		const FString StellarLightingCode = TEXT(R"APSSTELLAR(

// Materialized stars use the generator's stable 100..500 range. Far HISM proxies
// may be energy-prefiltered far below 100, and the deep hierarchy deliberately
// caps them at 6/12/24. Give the two paths separate logarithmic response curves so
// those caps remain ordered instead of collapsing to one bright activity floor.
float logEmission = log2(1.0 + rawEmission);
float actorActivity = saturate((logEmission - 6.65) / 2.32);
float proxyActivity = saturate(logEmission / 8.97);
float emissionActivity = lerp(actorActivity, proxyActivity, useInstance);
// Spatial contrast follows the bounded footprint mask. One-pixel HISM keeps only
// emission hierarchy and spectral colour; a resolved proxy receives the same
// stationary photosphere fields as the materialized actor.
float variation = saturate(Variation);
float microUnit = microGranules * 0.5 + 0.5;
float granuleCell = smoothstep(0.22, 0.78, microUnit);
float granuleRidges = (granuleCell - 0.5) * 2.0;
float granulation = (mesoCells * 0.64 + granuleRidges * 0.36)
                  * Granulation * 0.17 * spatialDetail;
// A narrow subset of the micro cells carries the faceted, jewel-like highlights.
// Footprint attenuation preserves it on resolved HISM without sub-pixel aliasing.
float granuleSpark = smoothstep(0.84, 0.97, microUnit) * spatialDetail;
float spots = spotCore * SpotAmount * lerp(0.72, 1.0, emissionActivity)
            * spatialDetail;
float faculae = spotHalo * (0.095 + variation * 0.17) * spatialDetail;
float surface = max(0.24, 1.0 + macroConvection * variation * 0.12 * spatialDetail
                           + granulation + faculae - spots * 0.72);
surface *= lerp(0.96, 1.07, granuleSpark);

// Limb darkening gives the disc volume.  The edge is brighter only in sparse
// magnetic lobes, so post-process bloom reads as a soft corona with occasional
// prominence hints rather than one large opaque halo.
float limb = lerp(0.22, 1.0, pow(facing, 0.62));
float rim = pow(1.0 - facing, 3.15);
// The rare magnetic cores already identify coherent large-scale lobes. Reusing
// that bounded field keeps prominences attached to the visible activity instead
// of evaluating a second mask over the same photosphere.
float prominenceMask = spotCore;
float resolvedProminence = prominenceMask * spatialDetail;
float corona = CoronaAmount * rim * (0.28 + resolvedProminence * 0.72)
             * (1.0 + saturate(SystemMarker) * 0.08);

// A seeded two-harmonic pulse keeps close stars alive without swimming the
// surface coordinates. Cubic shaping supplies the higher harmonic without a
// second trig evaluation. Every HISM receives exactly zero temporal modulation;
// its resolved jewel pattern remains deterministic and sub-pixel points stay still.
float flickerPhase = GameTime * lerp(0.34, 0.49, seed) + phase;
float flickerBase = sin(flickerPhase);
float flickerSignal = flickerBase * (0.84 + flickerBase * flickerBase * 0.16);
float flickerAmplitude = temporalDetail * 0.020
                       * lerp(0.72, 1.0, saturate(CoronaAmount));
float temporalFlicker = 1.0 + flickerSignal * flickerAmplitude;

// Tone-safe compression is intentionally bounded. Actor stars retain enough
// headroom for readable granulation; HISM energy starts close to black and rises
// logarithmically so proxy enlargement and the 6/12/24 caps have visible effect.
float actorTone = lerp(0.78, 1.42, actorActivity);
float proxyTone = lerp(0.08, 1.42, proxyActivity) * 1.03;
float toneSafeEmission = lerp(actorTone, proxyTone, useInstance);
float maxSpectral = max(max(spectralColor.r, spectralColor.g), spectralColor.b);
float3 normalizedSpectralTint = spectralColor / max(maxSpectral, 0.001);
// Partial normalization retains authored substellar brightness instead of making
// every class equally bright. The explicit validity mask prevents the deliberately
// near-black BH palette from being resurrected by spectral highlights or faculae.
float3 spectralTint = lerp(spectralColor, normalizedSpectralTint, 0.55);
// The visibility ramp already evaluates to exactly zero below 0.08.  A second
// 0.01 step and two later validity multiplies were therefore mathematically
// redundant, while costing every hierarchy point several pixel instructions.
// Keeping one continuous ramp also avoids a needless branch-like threshold on
// very dark spectral classes without resurrecting their emission.
float spectralVisibility = smoothstep(0.08, 0.90, maxSpectral);
float3 quietTint = lerp(spectralTint * spectralTint, spectralTint, 0.64)
                 * spectralVisibility;
float cellHeat = lerp(0.5, granuleCell, spatialDetail);
float3 spectralHighlightTint = normalizedSpectralTint * spectralVisibility;
float3 hotGranuleTint = lerp(quietTint * 1.02,
                             spectralHighlightTint * 1.16, 0.18);
float resolvedCellHeat = cellHeat * 0.84 + granuleSpark * 0.16;
float3 surfaceTint = lerp(quietTint * 0.78,
                          hotGranuleTint, resolvedCellHeat * 0.42);
surfaceTint = lerp(surfaceTint, quietTint * 0.18, saturate(spots * 1.18));
surfaceTint = lerp(surfaceTint,
                   spectralHighlightTint * 1.16,
                   faculae * 1.85 + granuleSpark * 0.10);

// Sparse highlights cross the HDR threshold without lifting the whole disc. Actors
// retain the existing spatially phased pulse. HISM selects a seed-bounded static
// pulse and the footprint mask removes all jewel lift when the proxy is sub-pixel.
float jewelCell = saturate((resolvedCellHeat - 0.67) * 4.166667);
float jewelMask = jewelCell
                * lerp(0.52, 1.0, saturate(faculae * 5.0 + granuleSpark));
float jewelPhase = GameTime * lerp(1.35, 1.85, seed)
                 + phase * 0.61
                 + mesoCells * 4.0;
float actorJewelPulse = 0.5 + 0.5 * sin(jewelPhase);
float instanceJewelPulse = lerp(0.58, 0.82, seed);
float jewelPulse = lerp(actorJewelPulse, instanceJewelPulse, useInstance);
float jewelLift = spatialDetail * jewelMask
                * lerp(0.08, 0.18, jewelPulse);
// A second sparse signal sits outside the limb-darkened photosphere but remains
// inside the opaque sphere. Bloom turns these coloured magnetic lobes into a
// compact jewel rim without ever adding literal white or destabilising HISM.
float rimJewelMask = spatialDetail * rim
                   * lerp(0.18, 1.0, prominenceMask);
float rimJewelLift = rimJewelMask
                   * lerp(0.035, 0.090, jewelPulse);

float visibleSurface = max(surface * limb + corona, 0.02);
float stellarSignal = (toneSafeEmission + jewelLift) * visibleSurface
                    + rimJewelLift;
float3 preBloom = surfaceTint * stellarSignal
                * temporalFlicker;
// Preserve hue and local contrast while hard-bounding the signal that enters the
// menu's fixed-exposure, full-resolution bloom pass.
float outputCeiling = lerp(1.72, 1.50, useInstance);
float peakChannel = max(max(preBloom.r, preBloom.g), preBloom.b);
return preBloom * min(1.0, outputCeiling / max(peakChannel, 0.0001));
)APSSTELLAR");
		StellarSurface->Code = StellarPatternCode + StellarLightingCode;
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
