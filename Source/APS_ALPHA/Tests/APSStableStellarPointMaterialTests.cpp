#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "APS_ALPHA/Core/Rendering/APSStellarMaterialContract.h"
#include "MaterialDomain.h"
#include "MaterialEditingLibrary.h"
#include "MaterialShared.h"
#include "Materials/MaterialExpressionCustom.h"
#include "RHI.h"
#include "UObject/UnrealType.h"

namespace
{
	// These source guards cover routing/data ownership, never artistic constants.
	// Strip comments and whitespace so documentation/formatting cannot satisfy or
	// invalidate a branch contract accidentally.
	FString CompactStellarShader(const FString& Source)
	{
		FString Result;
		for (int32 Index = 0; Index < Source.Len(); ++Index)
		{
			if (Source.Mid(Index, 2) == TEXT("//"))
			{
				while (Index < Source.Len() && Source[Index] != TEXT('\n')) ++Index;
			}
			else if (Source.Mid(Index, 2) == TEXT("/*"))
			{
				Index += 2;
				while (Index < Source.Len() && Source.Mid(Index, 2) != TEXT("*/")) ++Index;
				++Index;
			}
			else if (!FChar::IsWhitespace(Source[Index])) Result.AppendChar(Source[Index]);
		}
		return Result;
	}

	struct FStellarShaderBranch { FString Condition; FString Body; };
	TArray<FStellarShaderBranch> StellarShaderBranches(const FString& Code)
	{
		TArray<FStellarShaderBranch> Branches;
		for (int32 Start = Code.Find(TEXT("if(")); Start != INDEX_NONE;
			Start = Code.Find(TEXT("if("), ESearchCase::CaseSensitive,
				ESearchDir::FromStart, Start + 3))
		{
			int32 End = Start + 3;
			int32 Depth = 1;
			for (; End < Code.Len() && Depth > 0; ++End)
			{
				if (Code[End] == TEXT('(')) ++Depth;
				if (Code[End] == TEXT(')')) --Depth;
			}
			if (Depth != 0 || End >= Code.Len() || Code[End] != TEXT('{')) continue;
			const int32 BodyStart = End + 1;
			Depth = 1;
			for (++End; End < Code.Len() && Depth > 0; ++End)
			{
				if (Code[End] == TEXT('{')) ++Depth;
				if (Code[End] == TEXT('}')) --Depth;
			}
			if (Depth == 0)
				Branches.Add({Code.Mid(Start + 3, BodyStart - Start - 5),
					Code.Mid(BodyStart, End - BodyStart - 1)});
		}
		return Branches;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAPSStableStellarPointMaterialTest,
	"APS.Rendered.Materials.StablePointPass",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAPSStableStellarPointMaterialTest::RunTest(const FString& Parameters)
{
	UMaterial* Points = APSStellarMaterialContract::LoadCanonicalBase(
		APSStellarMaterialContract::HismBaseObjectPath);
	UMaterial* Corona = APSStellarMaterialContract::LoadCanonicalBase(
		APSStellarMaterialContract::CoronaBaseObjectPath);
	if (!TestNotNull(TEXT("Saved dedicated point material"), Points)
		|| !TestNotNull(TEXT("Physical corona material"), Corona)) return false;
	TestTrue(TEXT("Points and physical coronas use separate materials"), Points != Corona);
	TestEqual(TEXT("Point pass bypasses temporal reconstruction"),
		Points->TranslucencyPass.GetValue(), MTP_AfterMotionBlur);
	TestEqual(TEXT("Physical corona pass"), Corona->TranslucencyPass.GetValue(), MTP_AfterDOF);
	TestEqual(TEXT("Point surface domain"), Points->MaterialDomain.GetValue(), MD_Surface);
	TestEqual(TEXT("Point additive blend"), Points->GetBlendMode(), BLEND_Additive);
	TestTrue(TEXT("Points remain unlit"), Points->GetShadingModels().HasOnlyShadingModel(MSM_Unlit));
	TestFalse(TEXT("One-sided carriers"), Points->TwoSided != 0);
	TestFalse(TEXT("Point LOD dithering disabled"), Points->DitheredLODTransition != 0);
	TestFalse(TEXT("Point opacity dithering disabled"), Points->DitherOpacityMask != 0);
	TestTrue(TEXT("HISM usage compiled"), Points->GetUsageByFlag(MATUSAGE_InstancedStaticMeshes));
	TestNull(TEXT("No carrier padding through WPO"),
		UMaterialEditingLibrary::GetMaterialPropertyInputNode(Points, MP_WorldPositionOffset));
	TestNull(TEXT("Black additive output controls transparency"),
		UMaterialEditingLibrary::GetMaterialPropertyInputNode(Points, MP_Opacity));

	UMaterialExpressionCustom* PointShader = Cast<UMaterialExpressionCustom>(
		UMaterialEditingLibrary::GetMaterialPropertyInputNode(Points, MP_EmissiveColor));
	UMaterialExpressionCustom* CoronaShader = Cast<UMaterialExpressionCustom>(
		UMaterialEditingLibrary::GetMaterialPropertyInputNode(Corona, MP_EmissiveColor));
	if (!TestNotNull(TEXT("Point emissive custom"), PointShader)
		|| !TestNotNull(TEXT("Corona emissive custom"), CoronaShader)) return false;
	TestEqual(TEXT("Point custom RGB output"), PointShader->OutputType.GetValue(), CMOT_Float3);
	const auto FindInput = [](const UMaterialExpressionCustom* Shader, const FName Name)
	{
		return Shader->Inputs.FindByPredicate([Name](const FCustomInput& Input)
			{ return Input.InputName == Name; });
	};

	// The dedicated late pass shares all authored appearance code with the
	// corona master. Its only additions are explicit depth and native raster
	// coordinates. This tests synchronization, not a particular artistic recipe.
	FString ExpectedAppearance = CoronaShader->Code;
	ExpectedAppearance.ReplaceInline(TEXT("float4 pixelClip = GetScreenPosition(Parameters);"),
		TEXT("float4 pixelClip = RasterClip;"));
	TestTrue(TEXT("Shared appearance code remains synchronized"),
		!ExpectedAppearance.IsEmpty() && PointShader->Code.EndsWith(ExpectedAppearance));
	// The saved base point material uses GameplayPointProfile=0 in preview/maps.
	// Exact projection and spatial filtering are geometric correctness, not a
	// gameplay-only look. CPU-published optical eligibility must also survive the
	// much smaller area-prefiltered emission values used in the preview catalogue.
	const FString PointCode = CompactStellarShader(PointShader->Code);
	const TArray<FStellarShaderBranch> Branches = StellarShaderBranches(PointCode);
	const FStellarShaderBranch* ProjectionBranch = Branches.FindByPredicate(
		[](const FStellarShaderBranch& Branch)
		{ return Branch.Condition.Contains(TEXT("PointProjection"))
			&& Branch.Body.Contains(TEXT("pixelClip")); });
	if (TestNotNull(TEXT("Guarded exact point projection"), ProjectionBranch))
	{
		TestTrue(TEXT("Every point profile uses exact projection"),
			ProjectionBranch->Condition.Contains(TEXT("shellMode<0.5"))
			&& !ProjectionBranch->Condition.Contains(TEXT("gameplayProfile")));
		TestTrue(TEXT("Projection validates its finite per-instance payload"),
			ProjectionBranch->Condition.Contains(TEXT("isfinite(PointProjection)")));
		TestFalse(TEXT("Projection result is not blended away for preview points"),
			ProjectionBranch->Body.Contains(TEXT("gameplayProfile")));
	}
	const FStellarShaderBranch* FilterBranch = Branches.FindByPredicate(
		[](const FStellarShaderBranch& Branch)
		{ return Branch.Body.Contains(TEXT("filteredLobes"))
			&& Branch.Body.Contains(TEXT("ddx(")); });
	if (TestNotNull(TEXT("Guarded spatial point filtering"), FilterBranch))
	{
		TestTrue(TEXT("Every point profile receives spatial filtering"),
			FilterBranch->Condition.Contains(TEXT("shellMode<0.5"))
			&& !FilterBranch->Condition.Contains(TEXT("gameplayProfile")));
		TestFalse(TEXT("Filtered lobes are not blended away for preview points"),
			FilterBranch->Body.Contains(TEXT("gameplayProfile")));
		TestTrue(TEXT("Spatial filtering rejects non-finite inputs or lobes"),
			FilterBranch->Body.Contains(TEXT("isfinite(")));
	}
	TestTrue(TEXT("Explicit CPU ray eligibility is finite and bounded"),
		PointCode.Contains(TEXT("rayCarrierGate=isfinite(InstanceRayStrength)?saturate(InstanceRayStrength):0.0;")));
	TestFalse(TEXT("Area-prefiltered emission cannot re-threshold CPU ray eligibility"),
		PointCode.Contains(TEXT("rayEmissionGate")));
	TestFalse(TEXT("No duplicate brightness gate after CPU ray selection"),
		PointCode.Contains(TEXT("rayBrightnessGate")));
	const int32 RayStart = PointCode.Find(TEXT("floatfiniteRays="));
	const int32 RayEnd = RayStart == INDEX_NONE ? INDEX_NONE : PointCode.Find(
		TEXT(";"), ESearchCase::CaseSensitive, ESearchDir::FromStart, RayStart);
	TestTrue(TEXT("Finite rays consume the explicit CPU eligibility directly"),
		RayStart != INDEX_NONE && RayEnd > RayStart
		&& PointCode.Mid(RayStart, RayEnd - RayStart).Contains(TEXT("rayCarrierGate")));
	TestEqual(TEXT("Depth and native raster are the only additional dependencies"),
		PointShader->Inputs.Num(), CoronaShader->Inputs.Num() + 2);
	TSet<FName> PointNames;
	for (const FCustomInput& Input : PointShader->Inputs)
	{
		TestFalse(TEXT("Point input names are unique"), PointNames.Contains(Input.InputName));
		PointNames.Add(Input.InputName);
		if (TestNotNull(TEXT("Point input connected"), Input.Input.Expression))
			TestTrue(TEXT("Point input owned by dedicated material"),
				Input.Input.Expression->GetOuter() == Points);
	}
	for (const FCustomInput& Original : CoronaShader->Inputs)
	{
		const FCustomInput* Copy = FindInput(PointShader, Original.InputName);
		if (!TestNotNull(*FString::Printf(TEXT("Copied input %s"), *Original.InputName.ToString()), Copy)
			|| !TestNotNull(TEXT("Original connected input"), Original.Input.Expression)
			|| !TestNotNull(TEXT("Copied connected input"), Copy->Input.Expression)) continue;
		TestEqual(TEXT("Appearance expression class preserved"),
			Copy->Input.Expression->GetClass(), Original.Input.Expression->GetClass());
		TestEqual(TEXT("Appearance output selection preserved"),
			Copy->Input.OutputIndex, Original.Input.OutputIndex);
		// Matching class alone would miss a shifted instance data channel.
		for (const FName PropertyName : { FName(TEXT("DataIndex")), FName(TEXT("ConstDefaultValue")) })
		{
			const FProperty* Property = FindFProperty<FProperty>(
				Original.Input.Expression->GetClass(), PropertyName);
			if (Property)
				TestTrue(*FString::Printf(TEXT("%s copies %s payload contract"),
					*Original.InputName.ToString(), *PropertyName.ToString()),
					Property->Identical_InContainer(
						Original.Input.Expression, Copy->Input.Expression));
		}
	}
	for (const FName Name : { FName(TEXT("PointProjection")),
		FName(TEXT("InstanceOpticalCoreScale")), FName(TEXT("InstanceRayStrength")) })
		TestNotNull(*FString::Printf(TEXT("Required extended point input %s"), *Name.ToString()),
			FindInput(PointShader, Name));

	const FCustomInput* Depth = FindInput(PointShader, TEXT("SceneDepthForOcclusion"));
	if (TestNotNull(TEXT("Explicit scene-depth dependency"), Depth)
		&& TestNotNull(TEXT("Depth expression connected"), Depth->Input.Expression))
		TestEqual(TEXT("Depth expression type"), Depth->Input.Expression->GetClass()->GetFName(),
			FName(TEXT("MaterialExpressionSceneDepth")));
	const FCustomInput* Raster = FindInput(PointShader, TEXT("RasterClip"));
	if (TestNotNull(TEXT("Explicit raster dependency"), Raster)
		&& TestNotNull(TEXT("Raster expression connected"), Raster->Input.Expression))
	{
		TestEqual(TEXT("Raster arrives from vertex stage"),
			Raster->Input.Expression->GetClass()->GetFName(),
			FName(TEXT("MaterialExpressionVertexInterpolator")));
		const FExpressionInput* VertexInput = Raster->Input.Expression->GetInput(0);
		if (TestNotNull(TEXT("Raster vertex input"), VertexInput))
			TestNotNull(TEXT("Raster vertex source connected"), VertexInput->Expression);
	}
	const FString Guard = PointShader->Code.Left(
		FMath::Max(0, PointShader->Code.Len() - ExpectedAppearance.Len()));
	TestTrue(TEXT("Scene depth follows native raster rather than primary resolution"),
		Guard.Contains(TEXT("ViewportUVToBufferUV(")) && Guard.Contains(TEXT("RasterClip")));
	TestTrue(TEXT("Device depth includes full-scale stars and empty sky"),
		Guard.Contains(TEXT("LookupDeviceZ(")));
	TestTrue(TEXT("Opaque geometry clips astronomical point fragments"),
		Guard.Contains(TEXT("clip(Parameters.SvPosition.z-sceneDeviceZ)")));
	TestTrue(TEXT("Invalid scene depth rejected before colour output"),
		Guard.Contains(TEXT("isfinite(")));
	TestEqual(TEXT("Depth and two native raster expressions added"),
		Points->GetExpressions().Num(), Corona->GetExpressions().Num() + 3);
	for (UMaterialExpression* Expression : Points->GetExpressions())
	{
		if (TestNotNull(TEXT("Valid point graph expression"), Expression))
			TestFalse(TEXT("No artificial time-driven point flicker"),
				Expression->GetClass()->GetFName() == TEXT("MaterialExpressionTime"));
	}

	// Provisional safety ceiling, not a measured GPU performance target. Keep
	// compilation failures distinct from a legitimate inexpensive shader.
	const FMaterialStatistics Stats = UMaterialEditingLibrary::GetStatistics(Points);
	AddInfo(FString::Printf(TEXT("Stable point shader: pixel=%d vertex=%d samplers=%d"),
		Stats.NumPixelShaderInstructions, Stats.NumVertexShaderInstructions, Stats.NumSamplers));
	TestTrue(TEXT("Positive compiled pixel count"), Stats.NumPixelShaderInstructions > 0);
	TestTrue(TEXT("Provisional point pixel safety ceiling"), Stats.NumPixelShaderInstructions <= 900);
	FMaterialResource* Resource = Points->GetMaterialResource(GMaxRHIFeatureLevel);
	if (TestNotNull(TEXT("Point active RHI resource"), Resource))
	{
		for (const FString& Error : Resource->GetCompileErrors()) AddError(Error);
		TestTrue(TEXT("Point shader compilation complete"),
			Resource->IsCompilationFinished() && Resource->IsGameThreadShaderMapComplete());
		const FMaterialShaderMap* Map = Resource->GetGameThreadShaderMap();
		if (TestNotNull(TEXT("Point compiled shader map"), Map))
			TestTrue(TEXT("Point map finalized, successful and renderable"),
				Map->IsCompilationFinalized() && Map->CompiledSuccessfully()
				&& Map->IsValidForRendering());
	}
	// Native-resolution still/motion captures and an opaque occluder remain
	// required: a valid graph alone does not prove coverage or perceptual quality.
	return true;
}
#endif
