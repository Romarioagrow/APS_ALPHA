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
float instanceEnergy = abs(InstanceColor.r) + abs(InstanceColor.g) + abs(InstanceColor.b);
float instanceSignal = instanceEnergy + abs(InstanceEmission)
                     + abs(InstanceSeed) + abs(SystemMarker);
float useInstance = step(0.0001, instanceSignal);
float3 spectralColor = max(lerp(ParamColor.rgb, InstanceColor.rgb, useInstance), 0.001);
float rawEmission = max(lerp(ParamEmission, InstanceEmission, useInstance), 0.0);
float seed = frac(lerp(ParamSeed, InstanceSeed, useInstance));
// Actor-only archetypes keep every distant HISM on one cheap, stable point-star
// presentation while allowing a selected star to communicate its actual stellar
// type. Values: 0 photosphere, 1 giant, 2 protostar, 3 compact remnant,
// 4 pulsar, 5 cool/sub dwarf, 6 black hole.
float actorOnly = 1.0 - useInstance;
float giantType = actorOnly * (1.0 - step(0.5, abs(StellarType - 1.0)));
float protostarType = actorOnly * (1.0 - step(0.5, abs(StellarType - 2.0)));
float compactType = actorOnly * (1.0 - step(0.5, abs(StellarType - 3.0)));
float pulsarType = actorOnly * (1.0 - step(0.5, abs(StellarType - 4.0)));
float coolDwarfType = actorOnly * (1.0 - step(0.5, abs(StellarType - 5.0)));
float blackHoleType = actorOnly * step(5.5, StellarType);
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
float photosphereFrequency = clamp(1.0 - giantType * 0.40
    - protostarType * 0.50 + compactType * 0.38
    + pulsarType * 0.58 - coolDwarfType * 0.12, 0.45, 1.65);
float3 lowDomain = n * (3.4 * photosphereFrequency);
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
// A stable object-space axis supplies large-scale structures only to the one
// materialized actor. It adds no temporal noise to galaxy/cluster points.
float3 stellarAxis = normalize(float3(
    seed * 2.0 - 0.83,
    frac(seed * 7.13 + 0.31) * 2.0 - 1.0,
    frac(seed * 13.71 + 0.73) * 2.0 - 1.0));
float axisAlignment = abs(dot(n, stellarAxis));
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
// Keep the broad actor photosphere below the filmic shoulder so its procedural
// contrast survives. Sparse core/limb emitters below provide the HDR bloom seed.
// The raised HISM floor remains independent: it fixes energy-prefiltered galaxy
// stars that were numerically present but visually black.
float actorTone = lerp(0.78, 1.42, actorActivity);
float proxyTone = lerp(1.35, 3.80, proxyActivity);
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
// Spectral temperature remains authoritative. Archetypes only bias it toward the
// physically expected warm protostellar envelope or blue-white compact remnant.
float3 protostarTint = float3(1.00, 0.56, 0.24);
float3 compactTint = float3(0.62, 0.78, 1.00);
surfaceTint = lerp(surfaceTint, surfaceTint * protostarTint,
                   protostarType * 0.46);
surfaceTint = lerp(surfaceTint, normalizedSpectralTint * compactTint,
                   saturate(compactType * 0.40 + pulsarType * 0.55));
)APSSTELLAR");
		// MSVC limits a single wide string literal to 16,380 characters. Keep the
		// authored shader as one final FString while splitting only its C++ storage.
		const FString StellarOutputCode = TEXT(R"APSSTELLAR(

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

// The broad corona shapes the limb but intentionally stays below the HDR seed.
// A narrow emitter below provides real post-process bloom without lifting the
// whole opaque disc into the filmic shoulder.
float typeCoronaGain = 1.0 + giantType * 0.24 + protostarType * 0.52
                     + compactType * 0.16 + pulsarType * 0.34
                     - coolDwarfType * 0.28;
float coronaGain = lerp(1.0, lerp(1.35, 2.25, emissionActivity)
                        * typeCoronaGain, actorOnly);
float visibleSurface = max(surface * limb + corona * coronaGain, 0.02);
float coreBloom = actorOnly * pow(facing, 8.0)
                * lerp(0.10, 0.42, emissionActivity)
                * (1.0 + giantType * 0.18 + protostarType * 0.28
                   + pulsarType * 0.18 - coolDwarfType * 0.42);
// Opaque geometry cannot draw outside its silhouette. A thin HDR edge is instead
// fed directly into full-resolution bloom, which expands it into a soft exterior
// halo while leaving the detailed photosphere in its high-contrast range.
float coronaBloom = actorOnly * CoronaAmount * pow(rim, 1.35)
                  * lerp(24.0, 38.0, emissionActivity) * typeCoronaGain
                  * lerp(0.72, 1.0, resolvedProminence);
float polarCap = pow(axisAlignment, 14.0);
float compactLift = compactType * polarCap * 1.45;
float pulsarPulse = 0.58 + 0.42 * sin(GameTime * 6.4 + phase);
float pulsarLift = pulsarType * pow(axisAlignment, 28.0)
                  * lerp(2.2, 4.8, pulsarPulse);
float stellarSignal = (toneSafeEmission + jewelLift) * visibleSurface
                    + rimJewelLift + coreBloom + compactLift + pulsarLift;
float3 ordinaryPreBloom = surfaceTint * stellarSignal * temporalFlicker
                        + spectralHighlightTint * coronaBloom;

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
// Preserve hue and local contrast while retaining enough HDR energy for the
// fixed-exposure full-resolution bloom pass. This is a safety ceiling, not the
// normal disc intensity; only sparse peaks approach it.
float outputCeiling = lerp(8.0, 5.0, useInstance);
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

	bool RebuildPointAndCoronaMaterial(UMaterial* Material)
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
		Material->SetShadingModel(MSM_Unlit);
		Material->TwoSided = false;
		Material->DitheredLODTransition = false;
		Material->DitherOpacityMask = false;
		Material->bUsedWithInstancedStaticMeshes = true;

		UMaterialExpressionVectorParameter* Color = AddVectorParameter(
			Material, TEXT("Color"), FLinearColor(1.0f, 0.66f, 0.30f), -1050, -520, 0);
		UMaterialExpressionScalarParameter* CoronaIntensity = AddScalarParameter(
			Material, TEXT("CoronaIntensity"), 8.0f, 0.0f, 192.0f, -1050, -420, 1);
		UMaterialExpressionScalarParameter* CoronaOpacity = AddScalarParameter(
			Material, TEXT("CoronaOpacity"), 0.72f, 0.0f, 1.0f, -1050, -320, 2);
		UMaterialExpressionScalarParameter* CoronaSeed = AddScalarParameter(
			Material, TEXT("CoronaSeed"), 0.371f, 0.0f, 1.0f, -1050, -220, 3);
		UMaterialExpressionScalarParameter* CoronaShellMode = AddScalarParameter(
			Material, TEXT("CoronaShellMode"), 0.0f, 0.0f, 1.0f, -1050, -120, 4);
		UMaterialExpressionScalarParameter* CoronaInnerRadius = AddScalarParameter(
			Material, TEXT("CoronaInnerRadius"), 0.8928571f, 0.50f, 0.95f, -1050, -20, 5);

		UMaterialExpression* InstanceColor = AddReflectedExpression(Material,
			TEXT("/Script/Engine.MaterialExpressionPerInstanceCustomData3Vector"), -820, -520);
		UMaterialExpression* InstanceEmission = AddReflectedExpression(Material,
			TEXT("/Script/Engine.MaterialExpressionPerInstanceCustomData"), -820, -410);
		UMaterialExpression* InstanceSeed = AddReflectedExpression(Material,
			TEXT("/Script/Engine.MaterialExpressionPerInstanceCustomData"), -820, -300);
		UMaterialExpression* SystemHighlight = AddReflectedExpression(Material,
			TEXT("/Script/Engine.MaterialExpressionPerInstanceCustomData"), -820, -190);
		UMaterialExpression* Normal = AddReflectedExpression(Material,
			TEXT("/Script/Engine.MaterialExpressionPixelNormalWS"), -820, -80);
		UMaterialExpression* WorldPosition = AddReflectedExpression(Material,
			TEXT("/Script/Engine.MaterialExpressionWorldPosition"), -820, 30);
		UMaterialExpression* ObjectPosition = AddReflectedExpression(Material,
			TEXT("/Script/Engine.MaterialExpressionObjectPositionWS"), -820, 140);
		UMaterialExpression* InterpolatedObjectPosition = AddReflectedExpression(Material,
			TEXT("/Script/Engine.MaterialExpressionVertexInterpolator"), -600, 140);
		UMaterialExpressionCameraVectorWS* Camera =
			AddExpression<UMaterialExpressionCameraVectorWS>(Material, -820, 250);
		UMaterialExpressionCustom* PointAndCorona =
			AddExpression<UMaterialExpressionCustom>(Material, -430, -260);
		if (!Color || !CoronaIntensity || !CoronaOpacity || !CoronaSeed
			|| !CoronaShellMode || !CoronaInnerRadius
			|| !InstanceColor || !InstanceEmission || !InstanceSeed
			|| !SystemHighlight || !Normal || !WorldPosition || !ObjectPosition
			|| !InterpolatedObjectPosition || !Camera || !PointAndCorona)
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
			|| !SetFloatProperty(SystemHighlight, TEXT("ConstDefaultValue"), 0.0f))
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
float facing = saturate(abs(dot(n, v)));
float projectedRadiusSq = saturate(1.0 - facing * facing);

// A sphere is only the conservative HISM bound. Its visible signal is a compact
// Gaussian point: a resolved hot HDR core plus a broader low-energy halo. Brighter
// stars receive a wider halo, while black pixels in the additive material are
// genuinely transparent instead of forming pastel opaque discs.
float pointActivity = saturate(activity + marker * 0.14);
// Parent-scope proxy spheres project to only a few pixels. Keep the neutral HDR
// seed sub-pixel-to-one-pixel and the spectral halo close to one surrounding
// pixel. The previous 12/4 and 3.60/1.35 profile filled too much of every proxy;
// dense Ring/Arc samples then merged through bloom into white polygonal blobs.
// The sphere remains only a conservative bound and its outer silhouette is black.
float coreSharpness = lerp(28.0, 14.0, pointActivity);
float haloSharpness = lerp(5.50, 3.00, pointActivity);
float edgeFade = smoothstep(0.02, 0.28, facing);
float hotCore = exp2(-projectedRadiusSq * coreSharpness) * edgeFade;
float softHalo = exp2(-projectedRadiusSq * haloSharpness) * edgeFade;
float seedGain = lerp(0.86, 1.14, frac(seed * 17.713 + 0.37));
float coreEnergy = lerp(4.0, 11.0, activity) * seedGain * (1.0 + marker * 0.22);
float haloEnergy = lerp(0.62, 2.6, activity) * seedGain * (1.0 + marker * 0.18);
// Photographic point stars saturate toward a neutral core while their lower-energy
// halo retains the spectral hue. Tinting both lobes identically made the numerous
// M/K stars register as red pixels in max-channel tests but supplied very little
// luminance to bloom. Preserve the authored energy and spectral halo; only split
// the core chroma so unresolved stars read as white-hot light sources.
float coreWhitening = lerp(0.58, 0.78, activity);
float3 neutralCoreTint = float3(
    spectralVisibility, spectralVisibility, spectralVisibility);
float3 hotCoreTint = lerp(pointTint, neutralCoreTint, coreWhitening);
float3 haloTint = lerp(pointTint, neutralCoreTint, 0.04);
float3 pointSignal = hotCoreTint * (hotCore * coreEnergy)
                   + haloTint * (softHalo * haloEnergy);

// The actor corona runs on a second, slightly enlarged fallback mesh. Reconstruct
// its radial normal from continuous position rather than the fallback proxy's
// faceted normals. Its envelope starts at the opaque photosphere silhouette and
// only decreases outwards: a detached local maximum reads as a geometric ring,
// while a hot limb seed produces the broad optical bloom expected from a star.
// Point/HISM mode continues to return the byte-identical pointSignal path above.
float3 shellRadial = WorldPositionWS - ObjectPositionWS;
float shellRadialLengthSq = dot(shellRadial, shellRadial);
float3 shellNormal = shellRadialLengthSq > 1.0e-8
    ? shellRadial * rsqrt(shellRadialLengthSq) : n;
float shellFacing = saturate(abs(dot(shellNormal, v)));
float shellProjectedRadiusSq = saturate(1.0 - shellFacing * shellFacing);
float magneticField = 0.5 + 0.5 * sin(
    dot(shellNormal, float3(7.3, 11.1, 5.7)) + seed * 37.699);
float shellInnerRadius = saturate(CoronaInnerRadius);
float shellInnerRadiusSq = shellInnerRadius * shellInnerRadius;
float shellSpanSq = max(1.0 - shellInnerRadiusSq, 0.001);
float shellRadiusUnclamped =
    (shellProjectedRadiusSq - shellInnerRadiusSq) / shellSpanSq;
float shellRadius01 = saturate(shellRadiusUnclamped);
float photosphereOcclusion = smoothstep(0.0, 0.030, shellRadiusUnclamped);
float outerBoundaryFade = 1.0 - smoothstep(0.72, 1.0, shellRadius01);
// Preserve roughly the previous integrated HDR energy, but concentrate it into
// a sub-pixel-to-few-pixel limb seed. A very low spectral tail communicates the
// falloff on the shell without pushing the full 1.00..1.12 band through ACES white.
float radialHdrSeed = exp2(-shellRadius01 * 32.0);
float radialSpectralTail = 0.050 * exp2(-shellRadius01 * 1.6)
                         * outerBoundaryFade;
float radialLimbFalloff = radialHdrSeed + radialSpectralTail;
float shellVariation = lerp(0.94, 1.06, magneticField);
float shellSignal = photosphereOcclusion * radialLimbFalloff * shellVariation
                  * saturate(CoronaOpacity);
float3 rawShellTint = max(Color.rgb, 0.0);
float shellTintPeak = max(max(rawShellTint.r, rawShellTint.g), rawShellTint.b);
float3 normalizedShellTint = rawShellTint / max(shellTintPeak, 0.001);
float3 spectralShellTint = pow(max(normalizedShellTint, 0.001), 1.20);
float3 shellTint = lerp(spectralShellTint, float3(1.0, 0.90, 0.72), 0.10);
float3 coronaSignal = shellTint * max(CoronaIntensity, 0.0) * shellSignal;
return lerp(pointSignal, coronaSignal, shellMode);
)APSPOINT");
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

		const bool bPointAndCoronaMaster = PackagePath.EndsWith(TEXT("_HISM"));
		const bool bRebuilt = bPointAndCoronaMaster
			? APSStellarMaterial::RebuildPointAndCoronaMaterial(Material)
			: APSStellarMaterial::RebuildUnifiedStellarMaterial(Material);
		if (!bRebuilt)
		{
			UE_LOG(LogAPSStarMaterialFix, Error,
				TEXT("Could not rebuild stellar graph: %s"), *ObjectPath);
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
			TEXT("Rebuilt %s stellar material: %s"),
			bPointAndCoronaMaster ? TEXT("additive point/corona") : TEXT("photosphere"),
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
