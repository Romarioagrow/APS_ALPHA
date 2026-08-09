#if WITH_EDITOR && WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "MaterialEditingLibrary.h"
#include "MaterialDomain.h"
#include "Materials/Material.h"
#include "Materials/MaterialExpressionCameraVectorWS.h"
#include "Materials/MaterialExpressionCustom.h"
#include "Materials/MaterialExpressionParameter.h"
#include "Materials/MaterialExpressionScalarParameter.h"
#include "Materials/MaterialExpressionVectorParameter.h"
#include "UObject/UObjectGlobals.h"
#include "UObject/UnrealType.h"

namespace APSStellarMaterialTests
{
	constexpr int32 ExpectedExpressionCount = 16;
	constexpr int32 MaximumPixelInstructions = 320;

	const TMap<FName, int32>& RequiredParameters()
	{
		static const TMap<FName, int32> Parameters{
			{ TEXT("Color"), 0 },
			{ TEXT("Multiplier"), 1 },
			{ TEXT("SurfaceSeed"), 2 },
			{ TEXT("SurfaceVariation"), 3 },
			{ TEXT("GranulationStrength"), 4 },
			{ TEXT("SpotStrength"), 5 },
			{ TEXT("CoronaStrength"), 6 }
		};
		return Parameters;
	}

	const TSet<FName>& RequiredCustomInputs()
	{
		static const TSet<FName> Inputs{
			TEXT("ParamColor"),
			TEXT("ParamEmission"),
			TEXT("ParamSeed"),
			TEXT("Variation"),
			TEXT("Granulation"),
			TEXT("SpotAmount"),
			TEXT("CoronaAmount"),
			TEXT("InstanceColor"),
			TEXT("InstanceEmission"),
			TEXT("InstanceSeed"),
			TEXT("SystemMarker"),
			TEXT("NormalWS"),
			TEXT("WorldPositionWS"),
			TEXT("ObjectPositionWS"),
			TEXT("CameraWS")
		};
		return Inputs;
	}

	FString Context(const UMaterial* Material, const TCHAR* Detail)
	{
		return FString::Printf(TEXT("%s: %s"), *GetNameSafe(Material), Detail);
	}

	bool ReadUnsignedProperty(const UObject* Object, const FName PropertyName, uint32& OutValue)
	{
		if (!Object)
		{
			return false;
		}

		const FNumericProperty* Property =
			FindFProperty<FNumericProperty>(Object->GetClass(), PropertyName);
		if (!Property || !Property->IsInteger())
		{
			return false;
		}

		const void* Value = Property->ContainerPtrToValuePtr<void>(Object);
		OutValue = static_cast<uint32>(Property->GetUnsignedIntPropertyValue(Value));
		return true;
	}

	bool ReadFloatProperty(const UObject* Object, const FName PropertyName, float& OutValue)
	{
		if (!Object)
		{
			return false;
		}

		const FNumericProperty* Property =
			FindFProperty<FNumericProperty>(Object->GetClass(), PropertyName);
		if (!Property || !Property->IsFloatingPoint())
		{
			return false;
		}

		const void* Value = Property->ContainerPtrToValuePtr<void>(Object);
		OutValue = static_cast<float>(Property->GetFloatingPointPropertyValue(Value));
		return true;
	}

	bool ReadLinearColorProperty(
		const UObject* Object,
		const FName PropertyName,
		FLinearColor& OutValue)
	{
		if (!Object)
		{
			return false;
		}

		const FStructProperty* Property =
			FindFProperty<FStructProperty>(Object->GetClass(), PropertyName);
		if (!Property || Property->Struct != TBaseStructure<FLinearColor>::Get())
		{
			return false;
		}

		OutValue = *Property->ContainerPtrToValuePtr<FLinearColor>(Object);
		return true;
	}

}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FAPSStellarMaterialTest,
	"APS.Rendered.Materials.UnifiedStellarMasters",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAPSStellarMaterialTest::RunTest(const FString& Parameters)
{
	// This is intentionally read-only. Run APSFixStarHISMMaterial first so all
	// three saved assets contain the canonical graph validated below.
	const TArray<FString> MasterObjectPaths{
		TEXT("/Game/APS/APS_ALPHA/Assets/Materials/Astro/M_SpectralStarMat.M_SpectralStarMat"),
		TEXT("/Game/APS/APS_ALPHA/Assets/Materials/Astro/M_SpectralStarMat_HISM.M_SpectralStarMat_HISM"),
		TEXT("/Game/APS/APS_ALPHA/Assets/Materials/Astro/M_SpectralStarMat_SUN.M_SpectralStarMat_SUN")
	};
	FString CanonicalCustomCode;
	for (const FString& ObjectPath : MasterObjectPaths)
	{
		UMaterial* Material = LoadObject<UMaterial>(nullptr, *ObjectPath);
		if (!TestNotNull(*FString::Printf(TEXT("Load canonical stellar master %s"), *ObjectPath), Material))
		{
			continue;
		}

		TestEqual(APSStellarMaterialTests::Context(Material, TEXT("surface domain")),
			Material->MaterialDomain.GetValue(), MD_Surface);
		TestEqual(APSStellarMaterialTests::Context(Material, TEXT("opaque blend mode")),
			Material->GetBlendMode(), BLEND_Opaque);

		const FMaterialShadingModelField ShadingModels = Material->GetShadingModels();
		TestTrue(APSStellarMaterialTests::Context(Material, TEXT("unlit shading model")),
			ShadingModels.HasOnlyShadingModel(MSM_Unlit));
		TestEqual(APSStellarMaterialTests::Context(Material, TEXT("one shading model")),
			ShadingModels.CountShadingModels(), 1);
		TestFalse(APSStellarMaterialTests::Context(Material, TEXT("one-sided surface")),
			Material->TwoSided != 0);
		TestFalse(APSStellarMaterialTests::Context(Material, TEXT("LOD dithering disabled")),
			Material->DitheredLODTransition != 0);
		TestFalse(APSStellarMaterialTests::Context(Material, TEXT("opacity dithering disabled")),
			Material->DitherOpacityMask != 0);
		TestTrue(APSStellarMaterialTests::Context(Material, TEXT("instanced-static-mesh usage flag")),
			Material->bUsedWithInstancedStaticMeshes != 0);
		TestTrue(APSStellarMaterialTests::Context(Material, TEXT("instanced-static-mesh usage query")),
			Material->GetUsageByFlag(MATUSAGE_InstancedStaticMeshes));

		const TConstArrayView<TObjectPtr<UMaterialExpression>> Expressions = Material->GetExpressions();
		TestEqual(APSStellarMaterialTests::Context(Material, TEXT("canonical expression count")),
			Expressions.Num(), APSStellarMaterialTests::ExpectedExpressionCount);
		TestTrue(APSStellarMaterialTests::Context(Material, TEXT("expression budget")),
			Expressions.Num() <= APSStellarMaterialTests::ExpectedExpressionCount);

		TSet<FName> ParameterNames;
		TSet<FGuid> ParameterGuids;
		int32 VectorParameterCount = 0;
		int32 ScalarParameterCount = 0;
		int32 CustomCount = 0;
		int32 InstanceVectorCount = 0;
		int32 InstanceScalarCount = 0;
		int32 NormalCount = 0;
		int32 WorldPositionCount = 0;
		int32 ObjectPositionCount = 0;
		int32 CameraCount = 0;
		UMaterialExpressionCustom* StellarSurface = nullptr;
		UMaterialExpression* InstanceColor = nullptr;
		TMap<uint32, UMaterialExpression*> InstanceScalars;

		for (UMaterialExpression* Expression : Expressions)
		{
			if (UMaterialExpressionVectorParameter* Vector =
				Cast<UMaterialExpressionVectorParameter>(Expression))
			{
				++VectorParameterCount;
				ParameterNames.Add(Vector->ParameterName);
				ParameterGuids.Add(Vector->ExpressionGUID);
			}
			else if (UMaterialExpressionScalarParameter* Scalar =
				Cast<UMaterialExpressionScalarParameter>(Expression))
			{
				++ScalarParameterCount;
				ParameterNames.Add(Scalar->ParameterName);
				ParameterGuids.Add(Scalar->ExpressionGUID);
			}

			if (UMaterialExpressionParameter* Parameter =
				Cast<UMaterialExpressionParameter>(Expression))
			{
				TestEqual(
					APSStellarMaterialTests::Context(Material, TEXT("parameter group")),
					Parameter->Group, FName(TEXT("APS Stellar Surface")));
				TestTrue(
					APSStellarMaterialTests::Context(Material, TEXT("valid parameter GUID")),
					Parameter->ExpressionGUID.IsValid());
				const int32* ExpectedSortPriority =
					APSStellarMaterialTests::RequiredParameters().Find(Parameter->ParameterName);
				if (TestNotNull(
					APSStellarMaterialTests::Context(Material, TEXT("known parameter name")),
					ExpectedSortPriority))
				{
					TestEqual(
						APSStellarMaterialTests::Context(Material, TEXT("parameter sort priority")),
						Parameter->SortPriority, *ExpectedSortPriority);
				}
			}

			if (UMaterialExpressionCustom* Custom = Cast<UMaterialExpressionCustom>(Expression))
			{
				++CustomCount;
				StellarSurface = Custom;
			}
			else if (Expression->GetClass()->GetFName() ==
				TEXT("MaterialExpressionPerInstanceCustomData3Vector"))
			{
				++InstanceVectorCount;
				InstanceColor = Expression;
			}
			else if (Expression->GetClass()->GetFName() ==
				TEXT("MaterialExpressionPerInstanceCustomData"))
			{
				++InstanceScalarCount;
				uint32 DataIndex = MAX_uint32;
				if (APSStellarMaterialTests::ReadUnsignedProperty(
					Expression, TEXT("DataIndex"), DataIndex))
				{
					InstanceScalars.Add(DataIndex, Expression);
				}
			}
			else if (Expression->GetClass()->GetFName() ==
				TEXT("MaterialExpressionPixelNormalWS"))
			{
				++NormalCount;
			}
			else if (Expression->GetClass()->GetFName() ==
				TEXT("MaterialExpressionWorldPosition"))
			{
				++WorldPositionCount;
			}
			else if (Expression->GetClass()->GetFName() ==
				TEXT("MaterialExpressionObjectPositionWS"))
			{
				++ObjectPositionCount;
			}
			else if (Cast<UMaterialExpressionCameraVectorWS>(Expression))
			{
				++CameraCount;
			}
		}

		TestEqual(APSStellarMaterialTests::Context(Material, TEXT("vector parameter count")),
			VectorParameterCount, 1);
		TestEqual(APSStellarMaterialTests::Context(Material, TEXT("scalar parameter count")),
			ScalarParameterCount, 6);
		TestEqual(APSStellarMaterialTests::Context(Material, TEXT("custom expression count")),
			CustomCount, 1);
		TestEqual(APSStellarMaterialTests::Context(Material, TEXT("instance-vector count")),
			InstanceVectorCount, 1);
		TestEqual(APSStellarMaterialTests::Context(Material, TEXT("instance-scalar count")),
			InstanceScalarCount, 3);
		TestEqual(APSStellarMaterialTests::Context(Material, TEXT("pixel-normal count")),
			NormalCount, 1);
		TestEqual(APSStellarMaterialTests::Context(Material, TEXT("world-position count")),
			WorldPositionCount, 1);
		TestEqual(APSStellarMaterialTests::Context(Material, TEXT("object-position count")),
			ObjectPositionCount, 1);
		TestEqual(APSStellarMaterialTests::Context(Material, TEXT("camera-vector count")),
			CameraCount, 1);
		TestEqual(APSStellarMaterialTests::Context(Material, TEXT("required parameter count")),
			ParameterNames.Num(), APSStellarMaterialTests::RequiredParameters().Num());
		TestEqual(APSStellarMaterialTests::Context(Material, TEXT("unique parameter GUIDs")),
			ParameterGuids.Num(), APSStellarMaterialTests::RequiredParameters().Num());
		for (const TPair<FName, int32>& Required : APSStellarMaterialTests::RequiredParameters())
		{
			TestTrue(
				APSStellarMaterialTests::Context(Material, *FString::Printf(
					TEXT("required parameter %s"), *Required.Key.ToString())),
				ParameterNames.Contains(Required.Key));
		}

		if (TestNotNull(APSStellarMaterialTests::Context(Material, TEXT("instance colour expression")),
			InstanceColor))
		{
			uint32 DataIndex = MAX_uint32;
			FLinearColor DefaultValue = FLinearColor::White;
			TestTrue(APSStellarMaterialTests::Context(Material, TEXT("read instance colour data index")),
				APSStellarMaterialTests::ReadUnsignedProperty(
					InstanceColor, TEXT("DataIndex"), DataIndex));
			TestTrue(APSStellarMaterialTests::Context(Material, TEXT("read instance colour fallback")),
				APSStellarMaterialTests::ReadLinearColorProperty(
					InstanceColor, TEXT("ConstDefaultValue"), DefaultValue));
			TestEqual(APSStellarMaterialTests::Context(Material, TEXT("instance colour data index")),
				DataIndex, 0u);
			TestTrue(APSStellarMaterialTests::Context(Material, TEXT("instance colour black fallback")),
				DefaultValue.Equals(FLinearColor::Black));
		}

		for (const uint32 DataIndex : { 3u, 4u, 5u })
		{
			UMaterialExpression* const* InstanceScalar =
				InstanceScalars.Find(DataIndex);
			if (TestNotNull(
				APSStellarMaterialTests::Context(Material, *FString::Printf(
					TEXT("instance scalar data index %u"), DataIndex)),
				InstanceScalar))
			{
				float DefaultValue = TNumericLimits<float>::Max();
				TestTrue(
					APSStellarMaterialTests::Context(Material, TEXT("read instance scalar fallback")),
					APSStellarMaterialTests::ReadFloatProperty(
						*InstanceScalar, TEXT("ConstDefaultValue"), DefaultValue));
				TestTrue(
					APSStellarMaterialTests::Context(Material, TEXT("instance scalar zero fallback")),
					FMath::IsNearlyZero(DefaultValue));
			}
		}

		if (TestNotNull(APSStellarMaterialTests::Context(Material, TEXT("stellar custom expression")),
			StellarSurface))
		{
			TestEqual(APSStellarMaterialTests::Context(Material, TEXT("custom output type")),
				StellarSurface->OutputType.GetValue(), CMOT_Float3);
			TestEqual(APSStellarMaterialTests::Context(Material, TEXT("custom description")),
				StellarSurface->Description,
				FString(TEXT("APS unified scale-independent stellar surface")));
			TestEqual(APSStellarMaterialTests::Context(Material, TEXT("custom input count")),
				StellarSurface->Inputs.Num(), APSStellarMaterialTests::RequiredCustomInputs().Num());

			TSet<FName> ActualInputs;
			for (const FCustomInput& Input : StellarSurface->Inputs)
			{
				ActualInputs.Add(Input.InputName);
				TestNotNull(
					APSStellarMaterialTests::Context(Material, *FString::Printf(
						TEXT("connected custom input %s"), *Input.InputName.ToString())),
					Input.Input.Expression);
			}
			for (const FName RequiredInput : APSStellarMaterialTests::RequiredCustomInputs())
			{
				TestTrue(
					APSStellarMaterialTests::Context(Material, *FString::Printf(
						TEXT("required custom input %s"), *RequiredInput.ToString())),
					ActualInputs.Contains(RequiredInput));
			}

			TestTrue(APSStellarMaterialTests::Context(Material, TEXT("continuous macro convection")),
				StellarSurface->Code.Contains(TEXT("macroConvection")));
			TestTrue(APSStellarMaterialTests::Context(Material, TEXT("organic domain warp")),
				StellarSurface->Code.Contains(TEXT("domainWarp")));
			TestTrue(APSStellarMaterialTests::Context(Material, TEXT("continuous mesoscopic cells")),
				StellarSurface->Code.Contains(TEXT("mesoCells")));
			TestTrue(APSStellarMaterialTests::Context(Material, TEXT("continuous micro granules")),
				StellarSurface->Code.Contains(TEXT("microGranules")));
			TestTrue(APSStellarMaterialTests::Context(Material, TEXT("coherent dark spots")),
				StellarSurface->Code.Contains(TEXT("spotCore")));
			TestTrue(APSStellarMaterialTests::Context(Material, TEXT("rare prominence mask")),
				StellarSurface->Code.Contains(TEXT("prominenceMask")));
			TestTrue(APSStellarMaterialTests::Context(Material, TEXT("instance system marker")),
				StellarSurface->Code.Contains(TEXT("SystemMarker")));
			TestTrue(APSStellarMaterialTests::Context(Material, TEXT("mesh-normal-independent radial domain")),
				StellarSurface->Code.Contains(TEXT("WorldPositionWS - ObjectPositionWS"))
				&& StellarSurface->Code.Contains(TEXT("radialLengthSq")));
			TestTrue(APSStellarMaterialTests::Context(Material, TEXT("physically oriented limb facing")),
				StellarSurface->Code.Contains(TEXT("saturate(dot(n, v))")));
			TestFalse(APSStellarMaterialTests::Context(Material, TEXT("no mirrored-normal workaround")),
				StellarSurface->Code.Contains(TEXT("abs(dot(n, v))")));
			TestTrue(APSStellarMaterialTests::Context(Material, TEXT("emission compression")),
				StellarSurface->Code.Contains(TEXT("log2(1.0 + rawEmission)")));
			TestTrue(APSStellarMaterialTests::Context(Material, TEXT("bounded tone-safe emission")),
				StellarSurface->Code.Contains(TEXT("lerp(1.15, 2.35, emissionActivity)")));
			TestTrue(APSStellarMaterialTests::Context(Material, TEXT("generator emission range normalization")),
				StellarSurface->Code.Contains(TEXT("(logEmission - 6.65) / 2.32")));
			TestTrue(APSStellarMaterialTests::Context(Material, TEXT("HISM point visibility compensation")),
				StellarSurface->Code.Contains(TEXT("lerp(1.0, 1.18, useInstance)")));
			TestTrue(APSStellarMaterialTests::Context(Material, TEXT("substellar visibility preservation")),
				StellarSurface->Code.Contains(TEXT("spectralVisibility")));
			TestTrue(APSStellarMaterialTests::Context(Material, TEXT("stellar output")),
				StellarSurface->Code.Contains(TEXT("return surfaceTint")));
			TestFalse(APSStellarMaterialTests::Context(Material, TEXT("no quantised cell grid")),
				StellarSurface->Code.Contains(TEXT("floor(")));
			TestFalse(APSStellarMaterialTests::Context(Material, TEXT("no white clipping ceiling")),
				StellarSurface->Code.Contains(TEXT("30.0")));
			TestFalse(APSStellarMaterialTests::Context(Material, TEXT("no time dependency")),
				StellarSurface->Code.Contains(TEXT("Time")));
			TestFalse(APSStellarMaterialTests::Context(Material, TEXT("no texture sampling")),
				StellarSurface->Code.Contains(TEXT("Texture2DSample")));

			if (CanonicalCustomCode.IsEmpty())
			{
				CanonicalCustomCode = StellarSurface->Code;
			}
			else
			{
				TestEqual(
					APSStellarMaterialTests::Context(Material, TEXT("same unified custom shader")),
					StellarSurface->Code, CanonicalCustomCode);
			}

			const FExpressionInput* Emissive =
				Material->GetExpressionInputForProperty(MP_EmissiveColor);
			if (TestNotNull(APSStellarMaterialTests::Context(Material, TEXT("emissive property input")),
				Emissive))
			{
				TestTrue(APSStellarMaterialTests::Context(Material, TEXT("custom drives emissive")),
					Emissive->Expression == StellarSurface);
			}
		}

		const FMaterialStatistics Statistics = UMaterialEditingLibrary::GetStatistics(Material);
		TestEqual(APSStellarMaterialTests::Context(Material, TEXT("zero samplers")),
			Statistics.NumSamplers, 0);
		TestEqual(APSStellarMaterialTests::Context(Material, TEXT("zero vertex texture samples")),
			Statistics.NumVertexTextureSamples, 0);
		// UE 5.4's representative pixel/vertex counters include engine permutation
		// overhead (three implicit pixel fetches on this D3D12 RHI and the maximum
		// instruction count across heavy vertex factories). The exact canonical
		// expression whitelist and custom-code checks above prove this graph adds no
		// texture nodes or sampling; NumSamplers and explicit VS/VT counts remain
		// stable material-level guards.
		TestEqual(APSStellarMaterialTests::Context(Material, TEXT("zero virtual texture samples")),
			Statistics.NumVirtualTextureSamples, 0);
		TestTrue(APSStellarMaterialTests::Context(Material, TEXT("pixel instruction budget")),
			Statistics.NumPixelShaderInstructions <=
				APSStellarMaterialTests::MaximumPixelInstructions);
		if (Statistics.NumPixelShaderInstructions == 0)
		{
			AddWarning(FString::Printf(
				TEXT("%s has no compiled pixel-shader statistic on this test RHI; "
					"the expression and texture-free budgets were still checked."),
				*GetNameSafe(Material)));
		}
	}

	return true;
}

#endif // WITH_EDITOR && WITH_DEV_AUTOMATION_TESTS
