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
			Material, TEXT("SurfaceVariation"), 0.44f, 0.0f, 1.0f, -1250, -320, 3);
		UMaterialExpressionScalarParameter* GranulationStrength = AddScalarParameter(
			Material, TEXT("GranulationStrength"), 0.56f, 0.0f, 1.0f, -1250, -220, 4);
		UMaterialExpressionScalarParameter* SpotStrength = AddScalarParameter(
			Material, TEXT("SpotStrength"), 0.50f, 0.0f, 1.0f, -1250, -120, 5);
		UMaterialExpressionScalarParameter* CoronaStrength = AddScalarParameter(
			Material, TEXT("CoronaStrength"), 0.16f, 0.0f, 0.45f, -1250, -20, 6);

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
		UMaterialExpression* GameTime = AddReflectedExpression(Material,
			TEXT("/Script/Engine.MaterialExpressionTime"), -950, 120);
		UMaterialExpressionCameraVectorWS* Camera =
			AddExpression<UMaterialExpressionCameraVectorWS>(Material, -950, 210);
		UMaterialExpressionCustom* StellarSurface =
			AddExpression<UMaterialExpressionCustom>(Material, -450, -320);

		if (!Color || !Multiplier || !SurfaceSeed || !SurfaceVariation || !GranulationStrength
			|| !SpotStrength || !CoronaStrength || !InstanceColor || !InstanceEmission
			|| !InstanceSeed || !SystemHighlight || !Normal || !WorldPosition
			|| !ObjectPosition || !GameTime || !Camera || !StellarSurface)
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
		// Canonicalize it before appending the sixteen explicit stellar inputs.
		StellarSurface->Inputs.Reset();
		const FString StellarPatternCode = TEXT(R"APSSTELLAR(
float instanceEnergy = abs(InstanceColor.r) + abs(InstanceColor.g) + abs(InstanceColor.b);
float useInstance = step(0.0001, instanceEnergy);
float actorDetail = 1.0 - useInstance;
float3 spectralColor = max(lerp(ParamColor.rgb, InstanceColor.rgb, useInstance), 0.001);
float rawEmission = max(lerp(ParamEmission, InstanceEmission, useInstance), 0.0);
float seed = frac(lerp(ParamSeed, InstanceSeed, useInstance));
// The old shader used the authored PixelNormalWS as its pattern coordinate. The
// legacy star sphere has faceted/mirrored normal islands, so a close STAR view
// exposed a square checker even though the procedural waves themselves never
// sampled a tiled texture. A radial coordinate reconstructed from position is
// continuous across those triangles. ObjectPositionWS also resolves per HISM
// instance, keeping GALAXY/CLUSTER and the materialized AStar on one recipe.
float3 radial = WorldPositionWS - ObjectPositionWS;
float radialLengthSq = dot(radial, radial);
float3 n = radialLengthSq > 1.0e-8
    ? radial * rsqrt(radialLengthSq)
    : normalize(NormalWS);
float3 v = normalize(CameraWS);
float facing = saturate(dot(n, v));

// Continuous direction-domain waves are seamless over a sphere and remain at
// the same apparent layout from GALAXY/HISM to the full-screen STAR view.  This
// deliberately avoids quantised cube cells (floor/fract grids), which used to
// turn a close star into flat blocks while still reading as an untextured point
// at cluster scale.
float phase = seed * 37.6991118;
// Nested, cross-modulated waves avoid the long parallel bands produced by a
// simple sum of high-frequency planes.  The resulting fields read as soft
// convection cells at close range and collapse into stable spectral light at
// HISM distance, without UVs, cube projections or mesh-normal seams.
float3 lowDomain = n * 4.2;
float3 domainWarp = float3(
    sin(lowDomain.x + sin(lowDomain.y * 1.31 + phase * 0.37)
                    + cos(lowDomain.z * 0.79 - phase * 0.23)),
    sin(lowDomain.y * 1.11 + cos(lowDomain.z * 1.43 - phase * 0.29)
                           - sin(lowDomain.x * 0.73 + phase * 0.19)),
    sin(lowDomain.z * 0.93 + sin(lowDomain.x * 1.27 + phase * 0.43)
                           + cos(lowDomain.y * 0.69 - phase * 0.31)));
float3 p = normalize(n + domainWarp * 0.055);

float3 macroP = p * 6.4 + domainWarp * 0.62;
float macroA = sin(macroP.x + sin(macroP.y * 0.83 + phase * 0.41)
                            + cos(macroP.z * 0.57 - phase * 0.27));
float macroB = cos(macroP.y * 1.07 + sin(macroP.z * 0.71 - phase * 0.33)
                                   - cos(macroP.x * 0.49 + phase * 0.21));
float macroConvection = (macroA + macroB + macroA * macroB * 0.32) / 2.32;

float3 mesoP = p * 18.5 + domainWarp * 1.65
             + macroConvection * float3(0.73, -0.41, 0.29);
float mesoA = sin(mesoP.x + sin(mesoP.y * 0.71 + phase * 0.73)
                          + cos(mesoP.z * 0.47 - phase * 0.51));
float mesoB = cos(mesoP.y * 1.09 + sin(mesoP.z * 0.83 - phase * 0.67)
                                  - cos(mesoP.x * 0.61 + phase * 0.43));
float mesoC = sin(mesoP.z * 0.91 + cos(mesoP.x * 0.63 + phase * 0.59)
                                  + sin(mesoP.y * 0.53 - phase * 0.37));
float mesoCells = (mesoA * mesoB + mesoB * mesoC + mesoC * mesoA) * 0.3333333;

float3 microP = p * 47.0 + domainWarp * 3.4
              + mesoCells * float3(-1.3, 0.9, 1.1);
float granuleA = sin(microP.x + sin(microP.y * 0.67 + phase * 1.31)
                             + cos(microP.z * 0.43 - phase * 1.07));
float granuleB = cos(microP.y * 1.13 + sin(microP.z * 0.79 - phase * 1.19)
                                     - cos(microP.x * 0.47 + phase * 0.83));
float granuleC = sin(microP.z * 0.89 + cos(microP.x * 0.61 + phase * 1.43)
                                     + sin(microP.y * 0.51 - phase * 0.97));
float microGranules = (granuleA * granuleB + granuleB * granuleC
                     + granuleC * granuleA) * 0.3333333;

// Broad magnetic fields form a few coherent dark spots.  A narrow surrounding
// facular band keeps them organic instead of looking like stamped black dots.
float magneticField = 0.5 + 0.32 * macroConvection + 0.14 * mesoCells;
float spotCore = smoothstep(0.67, 0.80, magneticField)
               * saturate(0.76 + microGranules * 0.24);
float spotHalo = saturate(smoothstep(0.54, 0.68, magneticField)
               - smoothstep(0.68, 0.82, magneticField));
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
// Distant HISM points intentionally discard alias-prone spatial contrast. Their
// one-pixel representation keeps emission hierarchy and spectral colour, while the
// materialized actor receives the complete procedural photosphere.
float variation = saturate(Variation);
float granuleRidges = (smoothstep(0.31, 0.69, microGranules * 0.5 + 0.5)
                     - 0.5) * 2.0;
float granulation = (mesoCells * 0.40 + granuleRidges * 0.60)
                  * Granulation * 0.33 * actorDetail;
float spots = spotCore * SpotAmount * lerp(0.72, 1.0, emissionActivity)
            * actorDetail;
float faculae = spotHalo * (0.075 + variation * 0.14) * actorDetail;
float surface = max(0.24, 1.0 + macroConvection * variation * 0.31 * actorDetail
                           + granulation + faculae - spots * 0.68);

// Limb darkening gives the disc volume.  The edge is brighter only in sparse
// magnetic lobes, so post-process bloom reads as a soft corona with occasional
// prominence hints rather than one large opaque halo.
float limb = lerp(0.22, 1.0, pow(facing, 0.62));
float rim = pow(1.0 - facing, 4.0);
float prominenceField = 0.5 + 0.30 * sin(dot(n, float3(0.707, -0.236, 0.667)) * 17.0 + phase * 1.43)
                            + 0.20 * sin(dot(n, float3(-0.324, 0.811, 0.487)) * 31.0 - phase * 2.21);
float prominenceMask = smoothstep(0.77, 0.93, prominenceField);
float resolvedProminence = prominenceMask * actorDetail;
float corona = CoronaAmount * rim * (0.12 + resolvedProminence * 0.88)
             * (1.0 + saturate(SystemMarker) * 0.08);

// A seeded two-frequency pulse keeps close stars alive without swimming the
// surface coordinates. Far HISM points receive exactly zero temporal modulation,
// preventing a cluster from sparkling while preserving its spectral response.
float flickerPhase = GameTime * lerp(0.34, 0.49, seed) + phase;
float flickerSignal = sin(flickerPhase) * 0.68
                    + sin(flickerPhase * 0.413 + phase * 1.73) * 0.32;
float flickerAmplitude = actorDetail * 0.012
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
float validStellarSpectrum = step(0.01, maxSpectral);
float spectralVisibility = smoothstep(0.08, 0.90, maxSpectral)
                         * validStellarSpectrum;
float3 quietTint = lerp(spectralTint * spectralTint, spectralTint, 0.64)
                 * spectralVisibility;
float cellHeat = lerp(0.5, saturate(granuleRidges * 0.5 + 0.5), actorDetail);
float3 spectralHighlightTint = normalizedSpectralTint * spectralVisibility;
float3 hotGranuleTint = lerp(quietTint * 1.03,
                             spectralHighlightTint * 1.08, 0.10);
float3 surfaceTint = lerp(quietTint * 0.68,
                          hotGranuleTint, cellHeat * 0.52);
surfaceTint = lerp(surfaceTint, quietTint * 0.30, saturate(spots * 1.12));
surfaceTint = lerp(surfaceTint,
                   spectralHighlightTint * 1.08,
                   saturate(faculae * 1.70));

// Sparse actor-only highlights cross the HDR threshold without lifting the whole
// disc. Their stationary cell mask preserves the procedural pattern while a local,
// spatially phased pulse produces jewel-like twinkle. HISM points deliberately get
// no additional lift, so galaxy and cluster hierarchy samples remain stable.
float jewelMask = smoothstep(0.74, 0.94, cellHeat)
                * lerp(0.45, 1.0, saturate(faculae * 5.0));
float jewelPhase = GameTime * lerp(1.35, 1.85, seed)
                 + phase * 0.61
                 + mesoCells * 4.0;
float jewelPulse = 0.5 + 0.5 * sin(jewelPhase);
float jewelLift = actorDetail * jewelMask
                * lerp(0.11, 0.21, jewelPulse);
// A second sparse signal sits outside the limb-darkened photosphere but remains
// inside the opaque sphere. Bloom turns these coloured magnetic lobes into a
// compact jewel rim without ever adding literal white or destabilising HISM.
float rimJewelMask = actorDetail * rim
                   * lerp(0.18, 1.0, prominenceMask);
float rimJewelLift = rimJewelMask
                   * lerp(0.025, 0.075, jewelPulse);

float visibleSurface = max(surface * limb + corona, 0.02);
float stellarSignal = (toneSafeEmission + jewelLift) * visibleSurface
                    + rimJewelLift;
float3 preBloom = surfaceTint * stellarSignal
                * temporalFlicker * validStellarSpectrum;
// Preserve hue and local contrast while hard-bounding the signal that enters the
// menu's fixed-exposure, full-resolution bloom pass.
float outputCeiling = lerp(1.65, 1.50, useInstance);
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
		AddCustomInput(StellarSurface, TEXT("ObjectPositionWS"), ObjectPosition);
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
