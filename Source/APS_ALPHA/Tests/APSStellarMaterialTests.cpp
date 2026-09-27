#if WITH_EDITOR && WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "APS_ALPHA/Actors/Astro/Galaxy.h"
#include "APS_ALPHA/Actors/Astro/Star.h"
#include "APS_ALPHA/Actors/Astro/StarCluster.h"
#include "APS_ALPHA/Core/Rendering/APSStarRenderStabilitySubsystem.h"
#include "APS_ALPHA/Core/Rendering/APSStellarMaterialContract.h"
#include "APS_ALPHA/Core/Structs/StarGenerationModel.h"
#include "APS_ALPHA/Generation/StarGenerator.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "MaterialEditingLibrary.h"
#include "MaterialDomain.h"
#include "MaterialShared.h"
#include "RHI.h"
#include "Materials/Material.h"
#include "Materials/MaterialExpressionCameraVectorWS.h"
#include "Materials/MaterialExpressionCustom.h"
#include "Materials/MaterialExpressionParameter.h"
#include "Materials/MaterialExpressionScalarParameter.h"
#include "Materials/MaterialExpressionVectorParameter.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "UObject/UObjectGlobals.h"
#include "UObject/UnrealType.h"

namespace APSStellarMaterialTests
{
	// Provisional hard safety limits while the new appearance is measured on SM6.
	// These are not performance acceptance targets; record actual costs and GPU
	// timings before lowering them. A missing/failed shader must never pass as 0.
	constexpr int32 ExpectedExpressionCount = 19;
	constexpr int32 MaximumPixelInstructions = 1000;
	constexpr int32 MaximumPointCoronaPixelInstructions = 900;
	constexpr float MinimumValidSpectrum = 0.01f;

	const TMap<FName, int32>& RequiredParameters()
	{
		static const TMap<FName, int32> Parameters{
			{ TEXT("Color"), 0 },
			{ TEXT("Multiplier"), 1 },
			{ TEXT("SurfaceSeed"), 2 },
			{ TEXT("SurfaceVariation"), 3 },
			{ TEXT("GranulationStrength"), 4 },
			{ TEXT("SpotStrength"), 5 },
			{ TEXT("CoronaStrength"), 6 },
			{ TEXT("StellarArchetype"), 7 }
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
			TEXT("StellarType"),
			TEXT("InstanceColor"),
			TEXT("InstanceEmission"),
			TEXT("InstanceSeed"),
			TEXT("SystemMarker"),
			TEXT("NormalWS"),
			TEXT("WorldPositionWS"),
			TEXT("ObjectPositionWS"),
			TEXT("GameTime"),
			TEXT("CameraWS")
		};
		return Inputs;
	}

	FString Context(const UMaterial* Material, const TCHAR* Detail)
	{
		return FString::Printf(TEXT("%s: %s"), *GetNameSafe(Material), Detail);
	}

	FString CompactShaderCode(const FString& Code)
	{
		FString Compact = Code;
		Compact.ReplaceInline(TEXT(" "), TEXT(""));
		Compact.ReplaceInline(TEXT("\t"), TEXT(""));
		Compact.ReplaceInline(TEXT("\r"), TEXT(""));
		Compact.ReplaceInline(TEXT("\n"), TEXT(""));
		return Compact;
	}

	float MaximumColorChannel(const FLinearColor& Color)
	{
		return FMath::Max(Color.R, FMath::Max(Color.G, Color.B));
	}

	FLinearColor NormalizedHue(const FLinearColor& Color)
	{
		const float MaximumChannel = MaximumColorChannel(Color);
		return MaximumChannel > UE_SMALL_NUMBER
			? FLinearColor(Color.R / MaximumChannel, Color.G / MaximumChannel,
				Color.B / MaximumChannel, 1.0f)
			: FLinearColor::Black;
	}

	float HueDistance(const FLinearColor& A, const FLinearColor& B)
	{
		const FLinearColor NormalizedA = NormalizedHue(A);
		const FLinearColor NormalizedB = NormalizedHue(B);
		const float DeltaR = NormalizedA.R - NormalizedB.R;
		const float DeltaG = NormalizedA.G - NormalizedB.G;
		const float DeltaB = NormalizedA.B - NormalizedB.B;
		return FMath::Sqrt(DeltaR * DeltaR + DeltaG * DeltaG + DeltaB * DeltaB);
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

	// Shader text is not a perceptual oracle. Validate the actual compiled
	// resource and graph contract; rendered near/far/occlusion captures are the
	// acceptance test for noise, HDR contrast, finite halo and black-hole core.
	void ValidateCompiledResource(FAutomationTestBase& Test, UMaterial* Material,
		const int32 InstructionLimit)
	{
		const FMaterialStatistics Stats = UMaterialEditingLibrary::GetStatistics(Material);
		Test.AddInfo(FString::Printf(TEXT("%s: pixel=%d vertex=%d samplers=%d"),
			*GetNameSafe(Material), Stats.NumPixelShaderInstructions,
			Stats.NumVertexShaderInstructions, Stats.NumSamplers));
		Test.TestTrue(Context(Material, TEXT("positive compiled pixel instruction count")),
			Stats.NumPixelShaderInstructions > 0);
		Test.TestTrue(Context(Material, TEXT("provisional pixel safety ceiling")),
			Stats.NumPixelShaderInstructions <= InstructionLimit);
		FMaterialResource* Resource = Material->GetMaterialResource(GMaxRHIFeatureLevel);
		if (!Test.TestNotNull(Context(Material, TEXT("active RHI resource")), Resource)) return;
		for (const FString& Error : Resource->GetCompileErrors()) Test.AddError(Error);
		Test.TestTrue(Context(Material, TEXT("shader compilation completed")),
			Resource->IsCompilationFinished() && Resource->IsGameThreadShaderMapComplete());
		const FMaterialShaderMap* Map = Resource->GetGameThreadShaderMap();
		if (Test.TestNotNull(Context(Material, TEXT("active shader map")), Map))
		{
			Test.TestTrue(Context(Material, TEXT("successful finalized renderable shader map")),
				Map->IsCompilationFinalized() && Map->CompiledSuccessfully()
				&& Map->IsValidForRendering());
		}
	}

	const FCustomInput* FindInput(const UMaterialExpressionCustom* Custom, const FName Name)
	{
		return Custom ? Custom->Inputs.FindByPredicate([Name](const FCustomInput& Input)
			{ return Input.InputName == Name; }) : nullptr;
	}

	void ValidateInstanceScalar(FAutomationTestBase& Test, UMaterial* Material,
		const UMaterialExpressionCustom* Custom, const FName Name,
		const uint32 ExpectedIndex, const float ExpectedDefault)
	{
		const FCustomInput* Input = FindInput(Custom, Name);
		const FString Label = FString::Printf(TEXT("%s input %s"),
			*GetNameSafe(Material), *Name.ToString());
		if (!Test.TestNotNull(Label, Input)
			|| !Test.TestNotNull(Label + TEXT(" connected"), Input->Input.Expression)) return;
		UMaterialExpression* Expression = Input->Input.Expression;
		Test.TestEqual(Label + TEXT(" instance scalar node"), Expression->GetClass()->GetFName(),
			FName(TEXT("MaterialExpressionPerInstanceCustomData")));
		uint32 Index = MAX_uint32;
		float DefaultValue = TNumericLimits<float>::Max();
		Test.TestTrue(Label + TEXT(" readable index"),
			ReadUnsignedProperty(Expression, TEXT("DataIndex"), Index));
		Test.TestEqual(Label + TEXT(" data index"), Index, ExpectedIndex);
		Test.TestTrue(Label + TEXT(" readable fallback"),
			ReadFloatProperty(Expression, TEXT("ConstDefaultValue"), DefaultValue));
		Test.TestTrue(Label + TEXT(" fallback"),
			FMath::IsNearlyEqual(DefaultValue, ExpectedDefault));
	}

}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FAPSStellarMaterialTest,
	"APS.Rendered.Materials.UnifiedStellarMasters",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAPSStellarMaterialTest::RunTest(const FString& Parameters)
{
	// This is intentionally read-only. Run APSFixStarHISMMaterial first so both
	// opaque photosphere assets contain the canonical graph validated below. The
	// additive HISM/corona graph has a deliberately different contract.
	const TArray<FString> MasterObjectPaths{
		TEXT("/Game/APS/APS_ALPHA/Assets/Materials/Astro/M_SpectralStarMat.M_SpectralStarMat"),
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
		int32 VertexInterpolatorCount = 0;
		int32 TimeCount = 0;
		int32 CameraCount = 0;
		UMaterialExpressionCustom* StellarSurface = nullptr;
		UMaterialExpression* InstanceColor = nullptr;
		UMaterialExpression* ObjectPositionExpression = nullptr;
		UMaterialExpression* VertexInterpolatorExpression = nullptr;
		TMap<uint32, UMaterialExpression*> InstanceScalars;

		for (UMaterialExpression* Expression : Expressions)
		{
			if (!TestNotNull(APSStellarMaterialTests::Context(Material,
				TEXT("valid graph expression")), Expression)) continue;
			TestTrue(APSStellarMaterialTests::Context(Material, TEXT("expression ownership")),
				Expression->GetOuter() == Material);
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
				if (Scalar->ParameterName == TEXT("CoronaStrength"))
				{
					TestTrue(APSStellarMaterialTests::Context(
						Material, TEXT("corona default and editor range are finite and bounded")),
						FMath::IsFinite(Scalar->DefaultValue)
						&& FMath::IsFinite(Scalar->SliderMin) && FMath::IsFinite(Scalar->SliderMax)
						&& Scalar->SliderMin >= 0.0f && Scalar->SliderMax <= 1.0f
						&& Scalar->SliderMax > Scalar->SliderMin
						&& Scalar->DefaultValue >= Scalar->SliderMin
						&& Scalar->DefaultValue <= Scalar->SliderMax);
				}
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
				ObjectPositionExpression = Expression;
			}
			else if (Expression->GetClass()->GetFName() ==
				TEXT("MaterialExpressionVertexInterpolator"))
			{
				++VertexInterpolatorCount;
				VertexInterpolatorExpression = Expression;
			}
			else if (Expression->GetClass()->GetFName() ==
				TEXT("MaterialExpressionTime"))
			{
				++TimeCount;
			}
			else if (Cast<UMaterialExpressionCameraVectorWS>(Expression))
			{
				++CameraCount;
			}
		}

		TestEqual(APSStellarMaterialTests::Context(Material, TEXT("vector parameter count")),
			VectorParameterCount, 1);
		TestEqual(APSStellarMaterialTests::Context(Material, TEXT("scalar parameter count")),
			ScalarParameterCount, 7);
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
		TestEqual(APSStellarMaterialTests::Context(Material, TEXT("vertex-interpolator count")),
			VertexInterpolatorCount, 1);
		if (TestNotNull(APSStellarMaterialTests::Context(
			Material, TEXT("per-instance centre vertex interpolator")),
			VertexInterpolatorExpression))
		{
			const FExpressionInput* VertexInput = VertexInterpolatorExpression->GetInput(0);
			if (TestNotNull(APSStellarMaterialTests::Context(
				Material, TEXT("vertex interpolator input")), VertexInput))
			{
				TestTrue(APSStellarMaterialTests::Context(
					Material, TEXT("ObjectPositionWS is evaluated in vertex stage")),
					VertexInput->Expression == ObjectPositionExpression);
			}
		}
		TestEqual(APSStellarMaterialTests::Context(Material, TEXT("time-expression count")),
			TimeCount, 1);
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
				if (Input.InputName == TEXT("ObjectPositionWS"))
				{
					TestTrue(APSStellarMaterialTests::Context(Material,
						TEXT("pixel custom receives interpolated per-instance centre")),
						Input.Input.Expression == VertexInterpolatorExpression);
				}
			}
			for (const FName RequiredInput : APSStellarMaterialTests::RequiredCustomInputs())
			{
				TestTrue(
					APSStellarMaterialTests::Context(Material, *FString::Printf(
						TEXT("required custom input %s"), *RequiredInput.ToString())),
					ActualInputs.Contains(RequiredInput));
			}

			// Check data dependencies without freezing one artistic noise recipe,
			// limb exponent, colour coefficient or HDR highlight threshold.
			for (const TCHAR* Dependency : { TEXT("ParamColor"), TEXT("ParamEmission"),
				TEXT("InstanceColor"), TEXT("InstanceEmission"), TEXT("StellarType"),
				TEXT("WorldPositionWS"), TEXT("ObjectPositionWS") })
			{
				TestTrue(APSStellarMaterialTests::Context(Material,
					*FString::Printf(TEXT("shader consumes %s"), Dependency)),
					StellarSurface->Code.Contains(Dependency));
			}
			TestNull(APSStellarMaterialTests::Context(Material, TEXT("no silhouette displacement")),
				UMaterialEditingLibrary::GetMaterialPropertyInputNode(Material, MP_WorldPositionOffset));
			TestNull(APSStellarMaterialTests::Context(Material, TEXT("unlit photosphere does not depend on base colour")),
				UMaterialEditingLibrary::GetMaterialPropertyInputNode(Material, MP_BaseColor));

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

		APSStellarMaterialTests::ValidateCompiledResource(*this, Material,
			APSStellarMaterialTests::MaximumPixelInstructions);
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FAPSStellarPointCoronaMaterialTest,
	"APS.Rendered.Materials.AdditivePointAndCoronaMaster",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAPSStellarPointCoronaMaterialTest::RunTest(const FString& Parameters)
{
	using namespace APSStellarMaterialTests;
	UMaterial* Material = LoadObject<UMaterial>(nullptr,
		APSStellarMaterialContract::CoronaBaseObjectPath);
	if (!TestNotNull(TEXT("Canonical additive corona master"), Material)) return false;
	TestEqual(Context(Material, TEXT("surface domain")), Material->MaterialDomain.GetValue(), MD_Surface);
	TestEqual(Context(Material, TEXT("additive blend")), Material->GetBlendMode(), BLEND_Additive);
	TestEqual(Context(Material, TEXT("physical corona pass")),
		Material->TranslucencyPass.GetValue(), MTP_AfterDOF);
	TestTrue(Context(Material, TEXT("unlit")),
		Material->GetShadingModels().HasOnlyShadingModel(MSM_Unlit));
	TestFalse(Context(Material, TEXT("one-sided carrier")), Material->TwoSided != 0);
	TestFalse(Context(Material, TEXT("depth testing retained")), Material->bDisableDepthTest != 0);
	TestFalse(Context(Material, TEXT("no LOD dithering")), Material->DitheredLODTransition != 0);
	TestFalse(Context(Material, TEXT("no opacity dithering")), Material->DitherOpacityMask != 0);
	TestTrue(Context(Material, TEXT("instanced usage")),
		Material->GetUsageByFlag(MATUSAGE_InstancedStaticMeshes));
	TestNull(Context(Material, TEXT("no world position offset")),
		UMaterialEditingLibrary::GetMaterialPropertyInputNode(Material, MP_WorldPositionOffset));
	TestNull(Context(Material, TEXT("black additive output owns transparency")),
		UMaterialEditingLibrary::GetMaterialPropertyInputNode(Material, MP_Opacity));

	UMaterialExpressionCustom* Shader = Cast<UMaterialExpressionCustom>(
		UMaterialEditingLibrary::GetMaterialPropertyInputNode(Material, MP_EmissiveColor));
	if (!TestNotNull(Context(Material, TEXT("custom drives emissive")), Shader)) return false;
	TestEqual(Context(Material, TEXT("RGB custom output")), Shader->OutputType.GetValue(), CMOT_Float3);
	TestTrue(Context(Material, TEXT("nonempty shader")), !Shader->Code.TrimStartAndEnd().IsEmpty());
	const TSet<FName> RequiredInputs{
		TEXT("Color"), TEXT("CoronaIntensity"), TEXT("CoronaOpacity"), TEXT("CoronaSeed"),
		TEXT("CoronaShellMode"), TEXT("CoronaInnerRadius"), TEXT("InstanceColor"),
		TEXT("InstanceEmission"), TEXT("InstanceSeed"), TEXT("SystemMarker"),
		TEXT("NormalWS"), TEXT("WorldPositionWS"), TEXT("ObjectPositionWS"), TEXT("CameraWS"),
		TEXT("GameplayPointProfile"), TEXT("InstanceLuminosityGain"), TEXT("PointProjection"),
		TEXT("InstanceOpticalCoreScale"), TEXT("InstanceRayStrength")
	};
	TSet<FName> ActualInputs;
	for (const FCustomInput& Input : Shader->Inputs)
	{
		TestFalse(Context(Material, TEXT("unique named input")), ActualInputs.Contains(Input.InputName));
		ActualInputs.Add(Input.InputName);
		if (TestNotNull(Context(Material, *Input.InputName.ToString()), Input.Input.Expression))
			TestTrue(Context(Material, TEXT("input owned by this graph")),
				Input.Input.Expression->GetOuter() == Material);
	}
	TestEqual(Context(Material, TEXT("point/corona input contract")),
		ActualInputs.Num(), RequiredInputs.Num());
	for (const FName Name : RequiredInputs)
		TestTrue(Context(Material, *FString::Printf(TEXT("required input %s"), *Name.ToString())),
			ActualInputs.Contains(Name));

	ValidateInstanceScalar(*this, Material, Shader, TEXT("InstanceEmission"), 3u, 0.0f);
	ValidateInstanceScalar(*this, Material, Shader, TEXT("InstanceSeed"), 4u, 0.0f);
	ValidateInstanceScalar(*this, Material, Shader, TEXT("SystemMarker"), 5u, 0.0f);
	ValidateInstanceScalar(*this, Material, Shader, TEXT("InstanceLuminosityGain"), 6u, 1.0f);
	ValidateInstanceScalar(*this, Material, Shader, TEXT("InstanceOpticalCoreScale"), 11u, 1.0f);
	ValidateInstanceScalar(*this, Material, Shader, TEXT("InstanceRayStrength"), 12u, 0.0f);

	const FCustomInput* Colour = FindInput(Shader, TEXT("InstanceColor"));
	if (Colour && TestNotNull(Context(Material, TEXT("instance colour connected")), Colour->Input.Expression))
	{
		uint32 Index = MAX_uint32;
		FLinearColor Fallback = FLinearColor::White;
		TestEqual(Context(Material, TEXT("instance colour vector class")),
			Colour->Input.Expression->GetClass()->GetFName(),
			FName(TEXT("MaterialExpressionPerInstanceCustomData3Vector")));
		TestTrue(Context(Material, TEXT("colour index readable")),
			ReadUnsignedProperty(Colour->Input.Expression, TEXT("DataIndex"), Index));
		TestEqual(Context(Material, TEXT("RGB data channels")), Index, 0u);
		TestTrue(Context(Material, TEXT("colour fallback readable")),
			ReadLinearColorProperty(Colour->Input.Expression, TEXT("ConstDefaultValue"), Fallback));
		TestTrue(Context(Material, TEXT("empty colour payload is black")), Fallback.Equals(FLinearColor::Black));
	}
	for (const FName Name : { FName(TEXT("ObjectPositionWS")), FName(TEXT("PointProjection")) })
	{
		const FCustomInput* Input = FindInput(Shader, Name);
		if (Input && TestNotNull(Context(Material, *Name.ToString()), Input->Input.Expression))
		{
			TestEqual(Context(Material, TEXT("per-instance projection evaluated in vertex stage")),
				Input->Input.Expression->GetClass()->GetFName(),
				FName(TEXT("MaterialExpressionVertexInterpolator")));
			const FExpressionInput* VertexInput = Input->Input.Expression->GetInput(0);
			if (TestNotNull(Context(Material, TEXT("vertex input exists")), VertexInput))
			{
				if (!TestNotNull(Context(Material, TEXT("vertex input connected")), VertexInput->Expression)) continue;
				if (Name == TEXT("ObjectPositionWS"))
					TestEqual(Context(Material, TEXT("object centre uses native world-position node")),
						VertexInput->Expression->GetClass()->GetFName(),
						FName(TEXT("MaterialExpressionObjectPositionWS")));
				else
				{
					UMaterialExpressionCustom* Projection = Cast<UMaterialExpressionCustom>(VertexInput->Expression);
					if (TestNotNull(Context(Material, TEXT("optical projection custom")), Projection))
					{
						TestEqual(Context(Material, TEXT("projection carries centre and radius")),
							Projection->OutputType.GetValue(), CMOT_Float4);
						TestTrue(Context(Material, TEXT("projection uses native per-instance transform and finite guard")),
							Projection->Code.Contains(TEXT("GetInstanceToWorldDF("))
							&& Projection->Code.Contains(TEXT("isfinite(")));
					}
				}
			}
		}
	}
	// Coverage is mathematical, not mesh displacement. These broad operations
	// guard against removing filtering/finite falloff entirely; they do NOT prove
	// the resulting halo is attractive, seam-free or free of clipping in a frame.
	TestTrue(Context(Material, TEXT("smooth finite carrier falloff")),
		Shader->Code.Contains(TEXT("smoothstep(")));
	TestTrue(Context(Material, TEXT("pixel footprint filtering")),
		Shader->Code.Contains(TEXT("ddx(")) && Shader->Code.Contains(TEXT("ddy(")));
	for (const UMaterialExpression* Expression : Material->GetExpressions())
	{
		if (TestNotNull(Context(Material, TEXT("valid expression")), Expression))
		{
			TestTrue(Context(Material, TEXT("graph expression ownership")), Expression->GetOuter() == Material);
			TestFalse(Context(Material, TEXT("no artificial time-driven point flicker")),
				Expression->GetClass()->GetFName() == TEXT("MaterialExpressionTime"));
		}
	}
	ValidateCompiledResource(*this, Material, MaximumPointCoronaPixelInstructions);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FAPSStellarRuntimeBindingTest,
	"APS.Rendered.Materials.StellarRuntimeBindings",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAPSStellarRuntimeBindingTest::RunTest(const FString& Parameters)
{
	const auto ValidateBinding = [this](const TCHAR* Context,
		UMaterialInterface* Material, const TCHAR* ExpectedBasePath)
	{
		if (!TestNotNull(Context, Material))
		{
			return;
		}
		UMaterial* BaseMaterial = APSStellarMaterialContract::GetBaseMaterial(Material);
		if (TestNotNull(*FString::Printf(TEXT("%s base material"), Context), BaseMaterial))
		{
			TestEqual(*FString::Printf(TEXT("%s exact canonical base"), Context),
				BaseMaterial->GetPathName(), FString(ExpectedBasePath));
		}
		TestFalse(*FString::Printf(TEXT("%s never falls back to WorldGrid"), Context),
			APSStellarMaterialContract::UsesWorldGrid(Material));
	};

	const AGalaxy* NativeGalaxy = GetDefault<AGalaxy>();
	UMaterialInterface* NativeGalaxyMaterial = NativeGalaxy && NativeGalaxy->StarMeshInstances
		? NativeGalaxy->StarMeshInstances->GetMaterial(0) : nullptr;
	ValidateBinding(TEXT("Native galaxy HISM"), NativeGalaxyMaterial,
		APSStellarMaterialContract::HismBaseObjectPath);
	if (TestNotNull(TEXT("Native galaxy owns stellar HISM"),
		NativeGalaxy ? NativeGalaxy->StarMeshInstances : nullptr))
	{
		TestTrue(TEXT("Native galaxy additive HISM disallows Nanite"),
			NativeGalaxy->StarMeshInstances->bDisallowNanite);
		TestTrue(TEXT("Native galaxy additive HISM forces the runtime Nanite fallback"),
			NativeGalaxy->StarMeshInstances->bForceDisableNanite);
	}
	if (TestNotNull(TEXT("Native additive galaxy HISM material"), NativeGalaxyMaterial))
	{
		TestTrue(TEXT("Galaxy point master uses a Nanite-unsupported blend"),
			NativeGalaxyMaterial->GetBlendMode() != BLEND_Opaque
				&& NativeGalaxyMaterial->GetBlendMode() != BLEND_Masked);

		UHierarchicalInstancedStaticMeshComponent* AdditiveInstances =
			NewObject<UHierarchicalInstancedStaticMeshComponent>(GetTransientPackage());
		AdditiveInstances->SetMaterial(0, NativeGalaxyMaterial);
		UAPSStarRenderStabilitySubsystem::StabilizeInstances(AdditiveInstances);
		TestTrue(TEXT("Additive stellar HISM disallows Nanite before rendering"),
			AdditiveInstances->bDisallowNanite);
		TestTrue(TEXT("Additive stellar HISM forces the runtime Nanite fallback"),
			AdditiveInstances->bForceDisableNanite);
	}
	const AStar* NativeStar = GetDefault<AStar>();
	ValidateBinding(TEXT("Native star slot 0"),
		NativeStar && NativeStar->StarMesh
			? NativeStar->StarMesh->GetMaterial(0) : nullptr,
		APSStellarMaterialContract::ActorBaseObjectPath);
	if (TestNotNull(TEXT("Native AStar CDO"), NativeStar))
	{
		if (TestNotNull(TEXT("Native AStar owns CoronaMesh"), NativeStar->CoronaMesh))
		{
			TestTrue(TEXT("CoronaMesh remains a native default subobject"),
				NativeStar->CoronaMesh->CreationMethod == EComponentCreationMethod::Native);
			TestTrue(TEXT("Additive CoronaMesh disallows Nanite"),
				NativeStar->CoronaMesh->bDisallowNanite);
			TestTrue(TEXT("Additive CoronaMesh forces the runtime Nanite fallback"),
				NativeStar->CoronaMesh->bForceDisableNanite);
			TestFalse(TEXT("Opaque photosphere does not inherit the corona Nanite fallback"),
				NativeStar->StarMesh->bDisallowNanite
					|| NativeStar->StarMesh->bForceDisableNanite);
			TestTrue(TEXT("CoronaMesh is attached to the photosphere mesh"),
				NativeStar->CoronaMesh->GetAttachParent() == NativeStar->StarMesh);
			const FVector CoronaScale = NativeStar->CoronaMesh->GetRelativeScale3D();
			TestTrue(TEXT("Native corona support is finite, spherical and outside the photosphere"),
				!CoronaScale.ContainsNaN() && CoronaScale.X > 1.0 && CoronaScale.X < 2.0
				&& FMath::IsNearlyEqual(CoronaScale.X, CoronaScale.Y)
				&& FMath::IsNearlyEqual(CoronaScale.X, CoronaScale.Z));
			TestEqual(TEXT("Materialized photosphere forces LOD0"),
				NativeStar->StarMesh->ForcedLodModel, 1);
			TestEqual(TEXT("Materialized corona forces the matching LOD0"),
				NativeStar->CoronaMesh->ForcedLodModel, 1);
			TestTrue(TEXT("CoronaMesh is collision-free"),
				NativeStar->CoronaMesh->GetCollisionEnabled()
					== ECollisionEnabled::NoCollision);
			TestFalse(TEXT("CoronaMesh never casts a solid shell shadow"),
				NativeStar->CoronaMesh->CastShadow);
		}
		if (TestNotNull(TEXT("Native AStar owns StellarLight"), NativeStar->StellarLight))
		{
			TestTrue(TEXT("StellarLight remains a native default subobject"),
				NativeStar->StellarLight->CreationMethod == EComponentCreationMethod::Native);
			TestTrue(TEXT("StellarLight is attached to the photosphere mesh"),
				NativeStar->StellarLight->GetAttachParent() == NativeStar->StarMesh);
			TestTrue(TEXT("StellarLight remains movable for regenerated stars"),
				NativeStar->StellarLight->Mobility == EComponentMobility::Movable);
			TestTrue(TEXT("StellarLight has a finite positive native attenuation radius"),
				FMath::IsFinite(NativeStar->StellarLight->AttenuationRadius)
				&& NativeStar->StellarLight->AttenuationRadius > 0.0f);
		}
	}
	const AStarCluster* NativeCluster = GetDefault<AStarCluster>();
	ValidateBinding(TEXT("Native cluster HISM"),
		NativeCluster && NativeCluster->StarMeshInstances
			? NativeCluster->StarMeshInstances->GetMaterial(0) : nullptr,
		APSStellarMaterialContract::HismBaseObjectPath);
	if (TestNotNull(TEXT("Native cluster owns stellar HISM"),
		NativeCluster ? NativeCluster->StarMeshInstances : nullptr))
	{
		TestTrue(TEXT("Native cluster additive HISM disallows Nanite"),
			NativeCluster->StarMeshInstances->bDisallowNanite);
		TestTrue(TEXT("Native cluster additive HISM forces the runtime Nanite fallback"),
			NativeCluster->StarMeshInstances->bForceDisableNanite);
	}

	UClass* StarClass = LoadClass<AStar>(nullptr,
		TEXT("/Game/APS/APS_ALPHA/Core/BP_Star.BP_Star_C"));
	const AStar* StarDefault = StarClass ? Cast<AStar>(StarClass->GetDefaultObject()) : nullptr;
	ValidateBinding(TEXT("BP_Star slot 0"),
		StarDefault && StarDefault->StarMesh
			? StarDefault->StarMesh->GetMaterial(0) : nullptr,
		APSStellarMaterialContract::ActorBaseObjectPath);

	UClass* GalaxyClass = LoadClass<AGalaxy>(nullptr,
		TEXT("/Game/APS/APS_ALPHA/Core/BP_Galaxy.BP_Galaxy_C"));
	const AGalaxy* GalaxyDefault = GalaxyClass
		? Cast<AGalaxy>(GalaxyClass->GetDefaultObject()) : nullptr;
	ValidateBinding(TEXT("BP_Galaxy HISM slot 0"),
		GalaxyDefault && GalaxyDefault->StarMeshInstances
			? GalaxyDefault->StarMeshInstances->GetMaterial(0) : nullptr,
		APSStellarMaterialContract::HismBaseObjectPath);

	UClass* ClusterClass = LoadClass<AStarCluster>(nullptr,
		TEXT("/Game/APS/APS_ALPHA/Core/BP_StarCluster.BP_StarCluster_C"));
	const AStarCluster* ClusterDefault = ClusterClass
		? Cast<AStarCluster>(ClusterClass->GetDefaultObject()) : nullptr;
	ValidateBinding(TEXT("BP_StarCluster HISM slot 0"),
		ClusterDefault && ClusterDefault->StarMeshInstances
			? ClusterDefault->StarMeshInstances->GetMaterial(0) : nullptr,
		APSStellarMaterialContract::HismBaseObjectPath);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FAPSStellarPerceptualSafetyTest,
	"APS.Rendered.Materials.StellarPerceptualSafety",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAPSStellarPerceptualSafetyTest::RunTest(const FString& Parameters)
{
	// Exercise real generator spectra, not C++ copies of shader coefficients.
	// HDR comfort, surface contrast and dark black-hole centre require rendered
	// exposure-controlled captures and are not asserted by this data-level test.
	const TArray<ESpectralClass> MainSequenceClasses{
		ESpectralClass::O,
		ESpectralClass::B,
		ESpectralClass::A,
		ESpectralClass::F,
		ESpectralClass::G,
		ESpectralClass::K,
		ESpectralClass::M
	};
	for (int32 ClassIndex = 1; ClassIndex < MainSequenceClasses.Num(); ++ClassIndex)
	{
		const FLinearColor Hotter = UStarGenerator::GetStarColor(
			MainSequenceClasses[ClassIndex - 1], 0);
		const FLinearColor Cooler = UStarGenerator::GetStarColor(
			MainSequenceClasses[ClassIndex], 0);
		const float Distance = APSStellarMaterialTests::HueDistance(Hotter, Cooler);
		TestTrue(*FString::Printf(
			TEXT("Adjacent main-sequence hues %d/%d remain separated (distance %.3f)"),
			ClassIndex - 1, ClassIndex, Distance), Distance >= 0.14f);
	}

	const FLinearColor OStar = UStarGenerator::GetStarColor(ESpectralClass::O, 0);
	const FLinearColor GStar = UStarGenerator::GetStarColor(ESpectralClass::G, 0);
	const FLinearColor MStar = UStarGenerator::GetStarColor(ESpectralClass::M, 0);
	TestTrue(TEXT("O stars retain a blue-cyan hue"),
		OStar.B > OStar.G && OStar.G > OStar.R);
	TestTrue(TEXT("G stars retain a warm white-yellow hue"),
		GStar.R > GStar.G && GStar.G > GStar.B);
	TestTrue(TEXT("M stars retain a red hue"),
		MStar.R > MStar.G && MStar.G > MStar.B);
	TestTrue(TEXT("O, G and M representative hues stay perceptually distinct"),
		APSStellarMaterialTests::HueDistance(OStar, GStar) >= 0.35f
		&& APSStellarMaterialTests::HueDistance(GStar, MStar) >= 0.35f);

	const FLinearColor LStar = UStarGenerator::GetStarColor(ESpectralClass::L, 0);
	const FLinearColor TStar = UStarGenerator::GetStarColor(ESpectralClass::T, 0);
	const FLinearColor YStar = UStarGenerator::GetStarColor(ESpectralClass::Y, 0);
	const FLinearColor BlackHole = UStarGenerator::GetStarColor(ESpectralClass::BH, 0);
	const float LRawBrightness = APSStellarMaterialTests::MaximumColorChannel(LStar);
	const float TRawBrightness = APSStellarMaterialTests::MaximumColorChannel(TStar);
	const float YRawBrightness = APSStellarMaterialTests::MaximumColorChannel(YStar);
	const float BlackHoleRawBrightness =
		APSStellarMaterialTests::MaximumColorChannel(BlackHole);
	TestTrue(*FString::Printf(
		TEXT("Raw substellar brightness is ordered L %.3f > T %.3f > Y %.3f > BH %.4f"),
		LRawBrightness, TRawBrightness, YRawBrightness, BlackHoleRawBrightness),
		LRawBrightness > TRawBrightness && TRawBrightness > YRawBrightness
		&& YRawBrightness > BlackHoleRawBrightness);
	TestTrue(*FString::Printf(
		TEXT("Black-hole spectral source remains near-zero (max %.4f)"),
		BlackHoleRawBrightness),
		BlackHoleRawBrightness < APSStellarMaterialTests::MinimumValidSpectrum);


	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FAPSStellarTypeMappingTest,
	"APS.Gameplay.Generation.StellarTypeMapping",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAPSStellarTypeMappingTest::RunTest(const FString& Parameters)
{
	UStarGenerator* Generator = NewObject<UStarGenerator>(GetTransientPackage());
	if (!TestNotNull(TEXT("Transient star generator"), Generator))
	{
		return false;
	}

	struct FCompactObjectCase
	{
		EStellarType StellarType;
		ESpectralClass ExpectedSpectralClass;
		const TCHAR* Name;
	};
	const FCompactObjectCase Cases[] = {
		{ EStellarType::Neutron, ESpectralClass::NS, TEXT("Neutron") },
		{ EStellarType::Pulsar, ESpectralClass::NS, TEXT("Pulsar") },
		{ EStellarType::Protostar, ESpectralClass::PS, TEXT("Protostar") },
		{ EStellarType::BlackHole, ESpectralClass::BH, TEXT("BlackHole") },
		{ EStellarType::BrownDwarf, ESpectralClass::L, TEXT("BrownDwarf") }
	};

	for (const FCompactObjectCase& Case : Cases)
	{
		TSharedPtr<FStarModel> Model = MakeShared<FStarModel>();
		Model->StellarType = Case.StellarType;
		// GeneratedWorld's real editor/default path starts at G. A stellar-type
		// switch must normalize an incompatible retained spectrum instead of only
		// handling the synthetic Unknown value used by the older regression test.
		Model->SpectralClass = ESpectralClass::G;
		Generator->GenerateStarModel(Model);
		TestEqual(*FString::Printf(TEXT("%s receives its deterministic spectral class"),
			Case.Name), Model->SpectralClass, Case.ExpectedSpectralClass);
		if (Case.ExpectedSpectralClass == ESpectralClass::NS
			|| Case.ExpectedSpectralClass == ESpectralClass::BH)
		{
			TestEqual(*FString::Printf(TEXT("%s zero-span subclass remains finite"),
				Case.Name), Model->SpectralSubclass, 0);
		}

		const double ExpectedRadiusKm = Model->Radius * 6.957e5;
		const double RadiusToleranceKm = FMath::Max(
			0.01, FMath::Abs(ExpectedRadiusKm) * 1.0e-5);
		TestTrue(*FString::Printf(
			TEXT("%s non-main model populates RadiusKM from solar radius"), Case.Name),
			FMath::IsFinite(static_cast<double>(Model->RadiusKM))
			&& Model->RadiusKM > 0.0
			&& FMath::Abs(static_cast<double>(Model->RadiusKM) - ExpectedRadiusKm)
				<= RadiusToleranceKm);

		if (Case.StellarType == EStellarType::Neutron
			|| Case.StellarType == EStellarType::Pulsar)
		{
			TestTrue(*FString::Printf(TEXT("%s mass remains in neutron-star range"),
				Case.Name), Model->Mass >= 1.1 && Model->Mass <= 2.4);
			TestTrue(*FString::Printf(TEXT("%s solar radius remains physically compact"),
				Case.Name), Model->Radius >= 1.4374e-5 && Model->Radius <= 2.1561e-5);
			TestTrue(*FString::Printf(TEXT("%s physical radius remains about 10..15 km"),
				Case.Name), Model->RadiusKM >= 9.99f && Model->RadiusKM <= 15.01f);
		}
		else if (Case.StellarType == EStellarType::BlackHole)
		{
			TestTrue(TEXT("BlackHole mass remains in stellar-remnant range"),
				Model->Mass >= 3.0 && Model->Mass <= 100.0);
			TestTrue(TEXT("BlackHole solar radius remains in the authored event-scale range"),
				Model->Radius >= 1.2932e-5 && Model->Radius <= 4.3108e-4);
			TestTrue(TEXT("BlackHole physical radius remains about 9..300 km"),
				Model->RadiusKM >= 8.99f && Model->RadiusKM <= 300.01f);
		}
	}

	TSharedPtr<FStarModel> GiantModel = MakeShared<FStarModel>();
	GiantModel->StellarType = EStellarType::Giant;
	GiantModel->SpectralClass = ESpectralClass::G;
	Generator->GenerateStarModel(GiantModel);
	const double GiantExpectedRadiusKm = GiantModel->Radius * 6.957e5;
	TestTrue(TEXT("Ordinary non-main Giant model also populates RadiusKM"),
		GiantModel->Radius > 0.0 && GiantModel->RadiusKM > 0.0
		&& FMath::Abs(static_cast<double>(GiantModel->RadiusKM) - GiantExpectedRadiusKm)
			<= FMath::Max(0.01, GiantExpectedRadiusKm * 1.0e-5));

	const UEnum* StellarTypeEnum = StaticEnum<EStellarType>();
	if (TestNotNull(TEXT("Stellar type enum"), StellarTypeEnum))
	{
		TestEqual(TEXT("Unknown stellar type is not mislabeled as a black hole"),
			StellarTypeEnum->GetDisplayNameTextByValue(
				static_cast<int64>(EStellarType::Unknown)).ToString(),
			FString(TEXT("Unknown")));
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FAPSStellarBoostFringeSourceContractTest,
	"APS.SourceContracts.StellarBoostFringeDisabled",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAPSStellarBoostFringeSourceContractTest::RunTest(const FString& Parameters)
{
	const FString SpaceshipSourcePath = FPaths::ConvertRelativePathToFull(FPaths::Combine(
		FPaths::ProjectDir(), TEXT("Source/APS_ALPHA/Pawns/Spaceships/Spaceship.cpp")));
	FString SpaceshipSource;
	if (!TestTrue(TEXT("Load spaceship camera source for fringe contract"),
		FFileHelper::LoadFileToString(SpaceshipSource, *SpaceshipSourcePath)))
	{
		return false;
	}

	const int32 FunctionStart = SpaceshipSource.Find(
		TEXT("void ASpaceship::UpdateAdaptiveFlightCamera(float DeltaTime)"),
		ESearchCase::CaseSensitive);
	const int32 FunctionEnd = FunctionStart != INDEX_NONE
		? SpaceshipSource.Find(
			TEXT("void ASpaceship::InitializeFlightPostProcess()"),
			ESearchCase::CaseSensitive, ESearchDir::FromStart, FunctionStart + 1)
		: INDEX_NONE;
	if (!TestTrue(TEXT("Locate adaptive-flight-camera source body"),
		FunctionStart != INDEX_NONE && FunctionEnd > FunctionStart))
	{
		return false;
	}

	const FString FunctionSource = SpaceshipSource.Mid(
		FunctionStart, FunctionEnd - FunctionStart);
	const FString CompactSource =
		APSStellarMaterialTests::CompactShaderCode(FunctionSource);
	TestTrue(TEXT("Boost camera explicitly owns the scene-fringe override"),
		CompactSource.Contains(
			TEXT("PostProcess.bOverride_SceneFringeIntensity=true;")));
	TestTrue(TEXT("Boost camera keeps stellar chromatic fringe exactly zero"),
		CompactSource.Contains(TEXT("PostProcess.SceneFringeIntensity=0.0f;")));
	TestTrue(TEXT("Boost camera also keeps chromatic-aberration start at zero"),
		CompactSource.Contains(
			TEXT("PostProcess.ChromaticAberrationStartOffset=0.0f;")));

	const FString FringeAssignmentPrefix = TEXT("PostProcess.SceneFringeIntensity=");
	const int32 FirstAssignment = CompactSource.Find(
		FringeAssignmentPrefix, ESearchCase::CaseSensitive);
	const int32 SecondAssignment = FirstAssignment != INDEX_NONE
		? CompactSource.Find(FringeAssignmentPrefix, ESearchCase::CaseSensitive,
			ESearchDir::FromStart, FirstAssignment + FringeAssignmentPrefix.Len())
		: INDEX_NONE;
	TestEqual(TEXT("Adaptive boost camera has one authoritative fringe assignment"),
		SecondAssignment, INDEX_NONE);

	return true;
}

#endif // WITH_EDITOR && WITH_DEV_AUTOMATION_TESTS
