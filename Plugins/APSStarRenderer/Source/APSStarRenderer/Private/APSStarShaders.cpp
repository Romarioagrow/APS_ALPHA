// Rio 03.10 (galaxy phase 3): global shader registration and compile gating.
#include "APSStarShaders.h"

#include "APSStarRendererPrivate.h"
#include "DataDrivenShaderPlatformInfo.h"
#include "RenderUtils.h"

namespace APSStarRenderer::Private
{
	bool ShouldCompileStarShader(const FGlobalShaderPermutationParameters& Parameters, const bool bAtomic64)
	{
		if (!IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5) || IsMobilePlatform(Parameters.Platform))
		{
			return false;
		}
		if (bAtomic64 && !FDataDrivenShaderPlatformInfo::GetSupportsUInt64ImageAtomics(Parameters.Platform))
		{
			return false;
		}
		// Last: the decision reads the ini value and may write the crash guard once.
		return ShouldCompileShaders();
	}
}

bool FAPSStarClearCS::ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
{
	const FPermutationDomain PermutationVector(Parameters.PermutationId);
	return APSStarRenderer::Private::ShouldCompileStarShader(Parameters, PermutationVector.Get<FAtomic64Dim>());
}

void FAPSStarClearCS::ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
{
	FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);
}

bool FAPSStarRasterCS::ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
{
	const FPermutationDomain PermutationVector(Parameters.PermutationId);
	return APSStarRenderer::Private::ShouldCompileStarShader(Parameters, PermutationVector.Get<FAtomic64Dim>());
}

void FAPSStarRasterCS::ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
{
	FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);
}

bool FAPSStarCompositeCS::ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
{
	const FPermutationDomain PermutationVector(Parameters.PermutationId);
	return APSStarRenderer::Private::ShouldCompileStarShader(Parameters, PermutationVector.Get<FAtomic64Dim>());
}

void FAPSStarCompositeCS::ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
{
	FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);
}

bool FAPSGlowUploadCS::ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
{
	return APSStarRenderer::Private::ShouldCompileStarShader(Parameters, false);
}

void FAPSGlowUploadCS::ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
{
	FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);
}

bool FAPSGlowRaymarchCS::ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
{
	return APSStarRenderer::Private::ShouldCompileStarShader(Parameters, false);
}

void FAPSGlowRaymarchCS::ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
{
	FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);
}

IMPLEMENT_GLOBAL_SHADER(FAPSStarClearCS, "/Plugin/APSStarRenderer/Private/APSStarPoints.usf", "ClearCS", SF_Compute);
IMPLEMENT_GLOBAL_SHADER(FAPSStarRasterCS, "/Plugin/APSStarRenderer/Private/APSStarPoints.usf", "RasterCS", SF_Compute);
IMPLEMENT_GLOBAL_SHADER(FAPSStarCompositeCS, "/Plugin/APSStarRenderer/Private/APSStarComposite.usf", "CompositeCS", SF_Compute);
IMPLEMENT_GLOBAL_SHADER(FAPSGlowUploadCS, "/Plugin/APSStarRenderer/Private/APSGalaxyGlow.usf", "UploadCS", SF_Compute);
IMPLEMENT_GLOBAL_SHADER(FAPSGlowRaymarchCS, "/Plugin/APSStarRenderer/Private/APSGalaxyGlow.usf", "RaymarchCS", SF_Compute);
