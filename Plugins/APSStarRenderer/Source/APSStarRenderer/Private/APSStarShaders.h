// Rio 03.10 (galaxy phase 3): global shaders of the GPU star renderer. Every type compiles only when
// aps.Stars.CompileShaders=1 at start-up; 64-bit permutations only where 64-bit image atomics exist.
#pragma once

#include "CoreMinimal.h"
#include "GlobalShader.h"
#include "ShaderParameterStruct.h"
#include "RenderGraphResources.h"
#include "SceneView.h"

namespace APSStarRenderer::Private
{
	bool ShouldCompileStarShader(const FGlobalShaderPermutationParameters& Parameters, bool bAtomic64);
}

class FAPSStarClearCS : public FGlobalShader
{
public:
	DECLARE_GLOBAL_SHADER(FAPSStarClearCS);
	SHADER_USE_PARAMETER_STRUCT(FAPSStarClearCS, FGlobalShader);

	class FAtomic64Dim : SHADER_PERMUTATION_BOOL("APS_ATOMIC64");
	using FPermutationDomain = TShaderPermutationDomain<FAtomic64Dim>;

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER(FUintVector2, TargetSize)
		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D<uint>, OutStarBuffer32)
		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D<UlongType>, OutStarBuffer64)
	END_SHADER_PARAMETER_STRUCT()

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters);
	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment);
};

class FAPSStarRasterCS : public FGlobalShader
{
public:
	DECLARE_GLOBAL_SHADER(FAPSStarRasterCS);
	SHADER_USE_PARAMETER_STRUCT(FAPSStarRasterCS, FGlobalShader);

	class FAtomic64Dim : SHADER_PERMUTATION_BOOL("APS_ATOMIC64");
	using FPermutationDomain = TShaderPermutationDomain<FAtomic64Dim>;

	static constexpr int32 GroupSize = 64;
	static constexpr int32 MaxGroups = 8192;

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER_STRUCT_REF(FViewUniformShaderParameters, View)
		SHADER_PARAMETER(FMatrix44f, LocalRotation)
		SHADER_PARAMETER(FMatrix44f, TranslatedWorldToClip)
		SHADER_PARAMETER(FVector3f, BoundsMin)
		SHADER_PARAMETER(float, SetBrightness)
		SHADER_PARAMETER(FVector3f, BoundsStep)
		SHADER_PARAMETER(float, MinPixelValue)
		SHADER_PARAMETER(FVector3f, CameraLocal)
		SHADER_PARAMETER(float, MaxPixelValue)
		SHADER_PARAMETER(FVector3f, ObserverTranslated)
		SHADER_PARAMETER(float, MinDistanceSq)
		// Rio 03.10 (gameplay sky): local centre xyz, radius^2 w (APSStarRenderer::MaxExclusionSpheres).
		SHADER_PARAMETER_ARRAY(FVector4f, ExclusionSpheres, [8])
		SHADER_PARAMETER(FVector2f, TargetSizeF)
		SHADER_PARAMETER(float, CompressA)
		SHADER_PARAMETER(float, CompressB)
		SHADER_PARAMETER(FUintVector2, TargetSize)
		SHADER_PARAMETER(uint32, PointOffset)
		SHADER_PARAMETER(uint32, PointCount)
		SHADER_PARAMETER(uint32, ThreadCount)
		SHADER_PARAMETER(uint32, bWriteStats)
		SHADER_PARAMETER(uint32, NumExclusionSpheres)
		SHADER_PARAMETER_RDG_BUFFER_SRV(StructuredBuffer<uint2>, Points)
		SHADER_PARAMETER_RDG_TEXTURE(Texture2D, SceneDepthTexture)
		SHADER_PARAMETER_RDG_BUFFER_UAV(RWStructuredBuffer<uint>, OutStats)
		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D<uint>, OutStarBuffer32)
		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D<UlongType>, OutStarBuffer64)
	END_SHADER_PARAMETER_STRUCT()

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters);
	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment);
};

/** Rio 03.10 08:17: compute, not a pixel shader. The plugin creates no graphics PSO, which UE fails fatally at draw time. */
class FAPSStarCompositeCS : public FGlobalShader
{
public:
	DECLARE_GLOBAL_SHADER(FAPSStarCompositeCS);
	SHADER_USE_PARAMETER_STRUCT(FAPSStarCompositeCS, FGlobalShader);

	class FAtomic64Dim : SHADER_PERMUTATION_BOOL("APS_ATOMIC64");
	using FPermutationDomain = TShaderPermutationDomain<FAtomic64Dim>;

	static constexpr int32 GroupSize = 8;

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER_STRUCT_REF(FViewUniformShaderParameters, View)
		SHADER_PARAMETER_ARRAY(FVector4f, StarPalette, [256])
		SHADER_PARAMETER(FIntVector4, InputRect)
		SHADER_PARAMETER(FIntVector4, OutputRect)
		SHADER_PARAMETER(FVector4f, PsfParams)
		SHADER_PARAMETER(FVector4f, CompositeParams)
		SHADER_PARAMETER(FVector4f, MaskParams)
		SHADER_PARAMETER(uint32, bHasStars)
		SHADER_PARAMETER(uint32, bHasGlow)
		SHADER_PARAMETER(uint32, DebugMode)
		SHADER_PARAMETER(int32, PsfRadius)
		SHADER_PARAMETER_RDG_TEXTURE(Texture2D, SceneColorTexture)
		SHADER_PARAMETER_RDG_TEXTURE(Texture2D, SceneDepthTexture)
		SHADER_PARAMETER_RDG_TEXTURE(Texture2D<uint>, StarBuffer32)
		SHADER_PARAMETER_RDG_TEXTURE(Texture2D<UlongType>, StarBuffer64)
		SHADER_PARAMETER_RDG_TEXTURE(Texture2D, GlowTexture)
		SHADER_PARAMETER_SAMPLER(SamplerState, GlowSampler)
		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D<float4>, OutSceneColor)
	END_SHADER_PARAMETER_STRUCT()

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters);
	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment);
};

class FAPSGlowUploadCS : public FGlobalShader
{
public:
	DECLARE_GLOBAL_SHADER(FAPSGlowUploadCS);
	SHADER_USE_PARAMETER_STRUCT(FAPSGlowUploadCS, FGlobalShader);

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER(uint32, MapResolution)
		SHADER_PARAMETER_RDG_BUFFER_SRV(StructuredBuffer<float4>, SrcEmission)
		SHADER_PARAMETER_RDG_BUFFER_SRV(StructuredBuffer<float4>, SrcProfile)
		SHADER_PARAMETER_RDG_BUFFER_SRV(StructuredBuffer<float>, SrcShape)
		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D<float4>, DstEmission)
		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D<float4>, DstProfile)
		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D<float>, DstShape)
	END_SHADER_PARAMETER_STRUCT()

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters);
	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment);
};

class FAPSGlowRaymarchCS : public FGlobalShader
{
public:
	DECLARE_GLOBAL_SHADER(FAPSGlowRaymarchCS);
	SHADER_USE_PARAMETER_STRUCT(FAPSGlowRaymarchCS, FGlobalShader);

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER_STRUCT_REF(FViewUniformShaderParameters, View)
		SHADER_PARAMETER(FMatrix44f, InvProjection)
		SHADER_PARAMETER(FMatrix44f, ViewToMap)
		SHADER_PARAMETER(FVector3f, CameraMap)
		SHADER_PARAMETER(float, EmissionScale)
		SHADER_PARAMETER(FVector4f, VolumeParams)
		SHADER_PARAMETER(FVector2f, GlowSizeF)
		SHADER_PARAMETER(float, CompressA)
		SHADER_PARAMETER(float, CompressB)
		SHADER_PARAMETER(FUintVector2, GlowSize)
		SHADER_PARAMETER(uint32, StepCount)
		SHADER_PARAMETER(float, MinScaleHeight)
		SHADER_PARAMETER(float, FloorDistanceSq)
		SHADER_PARAMETER_RDG_TEXTURE(Texture2D, SceneDepthTexture)
		SHADER_PARAMETER_RDG_TEXTURE(Texture2D, EmissionMap)
		SHADER_PARAMETER_RDG_TEXTURE(Texture2D, ProfileMap)
		SHADER_PARAMETER_RDG_TEXTURE(Texture2D, ShapeMap)
		SHADER_PARAMETER_SAMPLER(SamplerState, MapSampler)
		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D<float4>, OutGlow)
	END_SHADER_PARAMETER_STRUCT()

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters);
	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment);
};
