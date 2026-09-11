#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "APS_ALPHA/Core/Rendering/APSStellarMaterialContract.h"
#include "MaterialDomain.h"
#include "MaterialEditingLibrary.h"
#include "Materials/MaterialExpressionCustom.h"

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
		|| !TestNotNull(TEXT("Accepted corona material"), Corona)) return false;
	TestTrue(TEXT("Points and physical coronas use separate materials"), Points != Corona);
	TestEqual(TEXT("Point pass bypasses temporal reconstruction"),
		Points->TranslucencyPass.GetValue(), MTP_AfterMotionBlur);
	TestEqual(TEXT("Physical coronas retain their accepted pass"),
		Corona->TranslucencyPass.GetValue(), MTP_AfterDOF);
	TestEqual(TEXT("Point surface domain"), Points->MaterialDomain.GetValue(), MD_Surface);
	TestEqual(TEXT("Point additive blend"), Points->GetBlendMode(), BLEND_Additive);
	TestTrue(TEXT("Points remain unlit"), Points->GetShadingModels().HasOnlyShadingModel(MSM_Unlit));
	TestFalse(TEXT("Points do not enable two-sided carriers"), Points->TwoSided != 0);
	TestFalse(TEXT("Point LOD dithering remains disabled"), Points->DitheredLODTransition != 0);
	TestFalse(TEXT("Point opacity dithering remains disabled"), Points->DitherOpacityMask != 0);
	TestTrue(TEXT("HISM usage is compiled"), Points->bUsedWithInstancedStaticMeshes != 0);

	const auto FindPointShader = [](UMaterial* Material) -> UMaterialExpressionCustom*
	{
		for (UMaterialExpression* Expression : Material->GetExpressions())
		{
			UMaterialExpressionCustom* Custom = Cast<UMaterialExpressionCustom>(Expression);
			if (Custom && Custom->Inputs.ContainsByPredicate([](const FCustomInput& Input)
				{ return Input.InputName == TEXT("GameplayPointProfile"); })) return Custom;
		}
		return nullptr;
	};
	UMaterialExpressionCustom* PointShader = FindPointShader(Points);
	UMaterialExpressionCustom* CoronaShader = FindPointShader(Corona);
	if (!TestNotNull(TEXT("Point shader"), PointShader)
		|| !TestNotNull(TEXT("Accepted appearance shader"), CoronaShader)) return false;

	// The late path adds occlusion, not a new photometry/halo recipe. This also
	// guards the luminosity custom-data channel, suppression and selection inputs.
	TestTrue(TEXT("Entire accepted appearance code is retained unchanged"),
		PointShader->Code.EndsWith(CoronaShader->Code));
	TestEqual(TEXT("Accepted input contract"), CoronaShader->Inputs.Num(), 17);
	TestEqual(TEXT("Only the depth dependency is appended"), PointShader->Inputs.Num(), 18);
	if (CoronaShader->Inputs.Num() != 17 || PointShader->Inputs.Num() != 18) return false;
	for (int32 Index = 0; Index < CoronaShader->Inputs.Num(); ++Index)
	{
		const FCustomInput& Original = CoronaShader->Inputs[Index];
		const FCustomInput& Copy = PointShader->Inputs[Index];
		TestEqual(FString::Printf(TEXT("Appearance input %d name preserved"), Index),
			Copy.InputName, Original.InputName);
		if (TestNotNull(TEXT("Appearance input remains connected"), Copy.Input.Expression)
			&& TestNotNull(TEXT("Accepted appearance input is connected"), Original.Input.Expression))
		{
			TestEqual(TEXT("Appearance expression class preserved"),
				Copy.Input.Expression->GetClass(), Original.Input.Expression->GetClass());
			TestEqual(TEXT("Appearance output selection preserved"),
				Copy.Input.OutputIndex, Original.Input.OutputIndex);
			TestTrue(TEXT("Copied graph is owned by the dedicated material"),
				Copy.Input.Expression->GetOuter() == Points);
		}
	}
	const FCustomInput& Depth = PointShader->Inputs.Last();
	TestEqual(TEXT("Explicit scene-depth dependency"), Depth.InputName, FName(TEXT("SceneDepthForOcclusion")));
	if (TestNotNull(TEXT("Depth expression connected"), Depth.Input.Expression))
	{
		TestEqual(TEXT("Depth expression type"), Depth.Input.Expression->GetClass()->GetFName(),
			FName(TEXT("MaterialExpressionSceneDepth")));
	}
	const FString Guard = PointShader->Code.Left(PointShader->Code.Len() - CoronaShader->Code.Len());
	TestTrue(TEXT("Depth is sampled in reversed device space, including the empty sky"),
		Guard.Contains(TEXT("LookupDeviceZ(depthUV)")));
	TestTrue(TEXT("Opaque geometry occludes astronomical points without a linear far-plane cap"),
		Guard.Contains(TEXT("clip(Parameters.SvPosition.z-sceneDeviceZ)")));
	TestEqual(TEXT("Only one new expression for occlusion"),
		Points->GetExpressions().Num(), Corona->GetExpressions().Num() + 1);
	for (UMaterialExpression* Expression : Points->GetExpressions())
	{
		if (Expression) TestFalse(TEXT("No artificial time-driven point flicker"),
			Expression->GetClass()->GetFName() == TEXT("MaterialExpressionTime"));
	}
	const FMaterialStatistics Stats = UMaterialEditingLibrary::GetStatistics(Points);
	TestTrue(TEXT("Bounded pixel instruction cost"), Stats.NumPixelShaderInstructions <= 450);
	TestEqual(TEXT("No vertex texture fetches"), Stats.NumVertexTextureSamples, 0);
	TestEqual(TEXT("No virtual textures"), Stats.NumVirtualTextureSamples, 0);
	if (Stats.NumPixelShaderInstructions == 0)
		AddWarning(TEXT("This RHI did not provide shader statistics; runtime capture is still required."));
	return true;
}
#endif
