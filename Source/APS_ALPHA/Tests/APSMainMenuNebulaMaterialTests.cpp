#if WITH_EDITOR && WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "APS_ALPHA/Core/Rendering/APSMainMenuNebulaMaterial.h"
#include "Materials/Material.h"
#include "Materials/MaterialExpressionCustom.h"
#include "Materials/MaterialExpressionScalarParameter.h"
#include "Materials/MaterialExpressionVectorParameter.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FAPSMainMenuNebulaMaterialContractTest,
	"APS.UI.MainMenu.NebulaMaterialContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAPSMainMenuNebulaMaterialContractTest::RunTest(const FString& Parameters)
{
	UMaterial* Material = LoadObject<UMaterial>(nullptr,
		APSMainMenuNebulaMaterial::ObjectPath);
	if (!TestNotNull(TEXT("Reusable main-menu nebula master exists"), Material))
	{
		return false;
	}

	TestEqual(TEXT("Nebula master is additive"), Material->BlendMode, BLEND_Additive);
	TestTrue(TEXT("Nebula master is unlit"),
		Material->GetShadingModels().HasShadingModel(MSM_Unlit));
	TestTrue(TEXT("Nebula sheets render from either side"), Material->TwoSided);
	TestTrue(TEXT("Nebula master is compiled for HISM"),
		Material->bUsedWithInstancedStaticMeshes);

	TSet<FName> ScalarParameters;
	TSet<FName> VectorParameters;
	const UMaterialExpressionCustom* ProceduralCloud = nullptr;
	for (UMaterialExpression* Expression : Material->GetExpressions())
	{
		if (const auto* Scalar = Cast<UMaterialExpressionScalarParameter>(Expression))
		{
			ScalarParameters.Add(Scalar->ParameterName);
		}
		if (const auto* Vector = Cast<UMaterialExpressionVectorParameter>(Expression))
		{
			VectorParameters.Add(Vector->ParameterName);
		}
		if (const auto* Custom = Cast<UMaterialExpressionCustom>(Expression);
			Custom && Custom->Description.Contains(TEXT("nebula"), ESearchCase::IgnoreCase))
		{
			ProceduralCloud = Custom;
		}
	}

	for (const FName Name : {FName(TEXT("NebulaOpacity")), FName(TEXT("NebulaEmission")),
		FName(TEXT("NebulaDensity")), FName(TEXT("NebulaFilamentStrength")),
		FName(TEXT("NebulaDistortion")), FName(TEXT("NebulaEdgeSoftness")),
		FName(TEXT("NebulaDriftSpeed"))})
	{
		TestTrue(FString::Printf(TEXT("Extensible scalar parameter %s exists"), *Name.ToString()),
			ScalarParameters.Contains(Name));
	}
	for (const FName Name : {FName(TEXT("NebulaColorA")), FName(TEXT("NebulaColorB")),
		FName(TEXT("NebulaWarmColor"))})
	{
		TestTrue(FString::Printf(TEXT("Extensible palette parameter %s exists"), *Name.ToString()),
			VectorParameters.Contains(Name));
	}
	TestNotNull(TEXT("Material owns the domain-warped procedural cloud node"),
		ProceduralCloud);
	if (ProceduralCloud)
	{
		TestTrue(TEXT("Shader contains multi-octave procedural structure"),
			ProceduralCloud->Code.Contains(TEXT("warpValues"))
			&& ProceduralCloud->Code.Contains(TEXT("fieldValues"))
			&& ProceduralCloud->Code.Contains(TEXT("[unroll]")));
		TestFalse(TEXT("Custom node does not declare illegal nested HLSL helpers"),
			ProceduralCloud->Code.Contains(TEXT("float Hash21("))
			|| ProceduralCloud->Code.Contains(TEXT("float Noise21("))
			|| ProceduralCloud->Code.Contains(TEXT("float Fbm(")));
		TestTrue(TEXT("Shader consumes per-instance palette data"),
			ProceduralCloud->Code.Contains(TEXT("InstanceTint")));
		TestTrue(TEXT("Carrier-plane silhouette is broken up by warped cloud edges"),
			ProceduralCloud->Code.Contains(TEXT("edgeWarpedP"))
			&& ProceduralCloud->Code.Contains(TEXT("breakup")));
		TestTrue(TEXT("Emissive output is masked by sparse cloud density"),
			ProceduralCloud->Code.Contains(TEXT("premultipliedEnergy"))
			&& ProceduralCloud->Code.Contains(
				TEXT("Emission * premultipliedEnergy")));
	}
	return true;
}

#endif
