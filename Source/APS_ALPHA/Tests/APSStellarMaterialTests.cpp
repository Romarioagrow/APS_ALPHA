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
	constexpr int32 ExpectedExpressionCount = 19;
	constexpr int32 ExpectedPointCoronaExpressionCount = 16;
	// Re-measured after the commandlet below; this remains a hard regression budget,
	// not a target. The archetype masks and compact-object presentation intentionally
	// add ALU while retaining zero texture samples and a single unified draw contract.
	constexpr int32 MaximumPixelInstructions = 420;
	constexpr int32 MaximumPointCoronaPixelInstructions = 256;
	constexpr float MaximumActorPreBloom = 8.0f;
	constexpr float MaximumHISMPreBloom = 5.0f;
	constexpr float ActorToneMinimum = 0.78f;
	constexpr float ActorToneMaximum = 1.42f;
	constexpr float HISMProxyToneMinimum = 1.35f;
	constexpr float HISMProxyToneMaximum = 3.80f;
	constexpr float CoronaDefault = 0.16f;
	constexpr float CoronaMinimum = 0.10f;
	constexpr float CoronaMaximum = 0.24f;
	constexpr float LimbEdgeIntensity = 0.22f;
	constexpr float LimbCenterIntensity = 1.0f;
	constexpr float LimbExponent = 0.62f;
	constexpr float RimExponent = 3.15f;
	constexpr float ActorFlickerAmplitude = 0.020f;
	constexpr float HISMProxyFlickerAmplitude = 0.0f;
	constexpr float JewelLiftMinimum = 0.08f;
	constexpr float JewelLiftMaximum = 0.18f;
	constexpr float RimJewelLiftMinimum = 0.035f;
	constexpr float RimJewelLiftMaximum = 0.090f;
	constexpr float DetailFootprintFull = 0.18f;
	constexpr float DetailFootprintNone = 0.65f;
	constexpr float ResolvedHISMSampleFootprint = 0.30f;
	constexpr float SubpixelHISMSampleFootprint = 0.75f;
	constexpr float HISMStaticJewelPulseMinimum = 0.58f;
	constexpr float HISMStaticJewelPulseMaximum = 0.82f;
	constexpr float InstancePayloadThreshold = 0.0001f;
	constexpr float MinimumProxyActivitySeparation = 0.08f;
	constexpr float SpectralNormalizationMix = 0.55f;
	constexpr float SpectralVisibilityMinimum = 0.08f;
	constexpr float SpectralVisibilityMaximum = 0.90f;
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

	float ActorEmissionActivity(const float RawEmission)
	{
		return FMath::Clamp((FMath::Log2(1.0f + FMath::Max(RawEmission, 0.0f)) - 6.65f)
			/ 2.32f, 0.0f, 1.0f);
	}

	float HISMProxyEmissionActivity(const float RawEmission)
	{
		return FMath::Clamp(FMath::Log2(1.0f + FMath::Max(RawEmission, 0.0f))
			/ 8.97f, 0.0f, 1.0f);
	}

	float ActorTone(const float RawEmission)
	{
		return FMath::Lerp(
			ActorToneMinimum, ActorToneMaximum, ActorEmissionActivity(RawEmission));
	}

	float HISMProxyTone(const float RawEmission)
	{
		return FMath::Lerp(HISMProxyToneMinimum, HISMProxyToneMaximum,
			HISMProxyEmissionActivity(RawEmission));
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

	float SmoothStep(const float Minimum, const float Maximum, const float Value)
	{
		const float Alpha = FMath::Clamp(
			(Value - Minimum) / FMath::Max(Maximum - Minimum, UE_SMALL_NUMBER),
			0.0f, 1.0f);
		return Alpha * Alpha * (3.0f - 2.0f * Alpha);
	}

	float StellarSpatialDetail(const float UseInstance, const float NormalFootprint)
	{
		const float ResolvedInstanceDetail = FMath::Clamp(1.0f - SmoothStep(
			DetailFootprintFull, DetailFootprintNone, NormalFootprint), 0.0f, 1.0f);
		return FMath::Lerp(1.0f, ResolvedInstanceDetail,
			FMath::Clamp(UseInstance, 0.0f, 1.0f));
	}

	float StellarTemporalDetail(const float UseInstance)
	{
		return 1.0f - FMath::Clamp(UseInstance, 0.0f, 1.0f);
	}

	float StellarUseInstance(const float ColorEnergy, const float Emission,
		const float Seed, const float SystemMarker)
	{
		const float InstanceSignal = FMath::Abs(ColorEnergy) + FMath::Abs(Emission)
			+ FMath::Abs(Seed) + FMath::Abs(SystemMarker);
		return InstanceSignal >= InstancePayloadThreshold ? 1.0f : 0.0f;
	}

	float ShaderSpectralPeak(const FLinearColor& RawColor)
	{
		const float MaximumRawChannel = MaximumColorChannel(RawColor);
		const float ValidSpectrum = MaximumRawChannel >= MinimumValidSpectrum ? 1.0f : 0.0f;
		const FLinearColor Normalized = NormalizedHue(RawColor);
		const FLinearColor PartiallyNormalized(
			FMath::Lerp(RawColor.R, Normalized.R, SpectralNormalizationMix),
			FMath::Lerp(RawColor.G, Normalized.G, SpectralNormalizationMix),
			FMath::Lerp(RawColor.B, Normalized.B, SpectralNormalizationMix),
			1.0f);
		const float Visibility = SmoothStep(
			SpectralVisibilityMinimum, SpectralVisibilityMaximum, MaximumRawChannel)
			* ValidSpectrum;
		return MaximumColorChannel(PartiallyNormalized) * Visibility;
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
						Material, TEXT("corona default remains compact")),
						FMath::IsNearlyEqual(Scalar->DefaultValue,
							APSStellarMaterialTests::CoronaDefault));
					TestTrue(APSStellarMaterialTests::Context(
						Material, TEXT("corona editor range remains physically bounded")),
						FMath::IsNearlyEqual(Scalar->SliderMin,
							APSStellarMaterialTests::CoronaMinimum)
						&& FMath::IsNearlyEqual(Scalar->SliderMax,
							APSStellarMaterialTests::CoronaMaximum));
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
			const FString CompactCode =
				APSStellarMaterialTests::CompactShaderCode(StellarSurface->Code);
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

			TestTrue(APSStellarMaterialTests::Context(Material, TEXT("continuous macro convection")),
				StellarSurface->Code.Contains(TEXT("macroConvection")));
			TestTrue(APSStellarMaterialTests::Context(Material, TEXT("organic domain warp")),
				StellarSurface->Code.Contains(TEXT("domainWarp")));
			TestTrue(APSStellarMaterialTests::Context(Material, TEXT("continuous mesoscopic cells")),
				StellarSurface->Code.Contains(TEXT("mesoCells")));
			TestTrue(APSStellarMaterialTests::Context(Material, TEXT("continuous micro granules")),
				StellarSurface->Code.Contains(TEXT("microGranules")));
			TestTrue(APSStellarMaterialTests::Context(Material, TEXT("three-axis cellular photosphere")),
				StellarSurface->Code.Contains(TEXT("float3 mesoWave"))
				&& StellarSurface->Code.Contains(TEXT("float3 granuleWave"))
				&& StellarSurface->Code.Contains(TEXT("mesoVolume"))
				&& StellarSurface->Code.Contains(TEXT("microVolume")));
			TestFalse(APSStellarMaterialTests::Context(Material, TEXT("no scalar zebra-wave projection")),
				StellarSurface->Code.Contains(TEXT("float macroA"))
				|| StellarSurface->Code.Contains(TEXT("float mesoA"))
				|| StellarSurface->Code.Contains(TEXT("float granuleA")));
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
				CompactCode.Contains(TEXT("logEmission=log2(1.0+rawEmission)")));
			TestTrue(APSStellarMaterialTests::Context(Material, TEXT("near-black-safe instance detection")),
				CompactCode.Contains(
					TEXT("instanceSignal=instanceEnergy+abs(InstanceEmission)+abs(InstanceSeed)+abs(SystemMarker)"))
				&& CompactCode.Contains(TEXT("useInstance=step(0.0001,instanceSignal)")));
			TestTrue(APSStellarMaterialTests::Context(Material, TEXT("actor emission normalization")),
				CompactCode.Contains(
					TEXT("actorActivity=saturate((logEmission-6.65)/2.32)")));
			TestTrue(APSStellarMaterialTests::Context(Material, TEXT("HISM proxy emission normalization")),
				CompactCode.Contains(TEXT("proxyActivity=saturate(logEmission/8.97)")));
			TestTrue(APSStellarMaterialTests::Context(Material, TEXT("actor and HISM activity selection")),
				CompactCode.Contains(
					TEXT("emissionActivity=lerp(actorActivity,proxyActivity,useInstance)")));
			TestTrue(APSStellarMaterialTests::Context(Material, TEXT("bounded scale-aware HISM detail")),
				CompactCode.Contains(
					TEXT("normalFootprint=max(length(ddx(n)),length(ddy(n)))"))
				&& CompactCode.Contains(
					TEXT("resolvedInstanceDetail=1.0-smoothstep(0.18,0.65,normalFootprint)"))
				&& CompactCode.Contains(
					TEXT("spatialDetail=lerp(1.0,resolvedInstanceDetail,useInstance)")));
			TestFalse(APSStellarMaterialTests::Context(Material, TEXT("no binary actor-only spatial mask")),
				CompactCode.Contains(TEXT("actorDetail")));
			TestTrue(APSStellarMaterialTests::Context(Material, TEXT("scale-aware granulation")),
				CompactCode.Contains(
					TEXT("granulation=(mesoCells*0.64+granuleRidges*0.36)*Granulation*0.17*spatialDetail")));
			TestTrue(APSStellarMaterialTests::Context(Material, TEXT("scale-aware spots and faculae")),
				CompactCode.Contains(
					TEXT("spots=spotCore*SpotAmount*lerp(0.72,1.0,emissionActivity)*spatialDetail"))
				&& CompactCode.Contains(
					TEXT("faculae=spotHalo*(0.095+variation*0.17)*spatialDetail")));
			TestTrue(APSStellarMaterialTests::Context(Material, TEXT("scale-aware photosphere contrast")),
				CompactCode.Contains(
					TEXT("macroConvection*variation*0.12*spatialDetail"))
				&& CompactCode.Contains(
					TEXT("cellHeat=lerp(0.5,granuleCell,spatialDetail)")));
			TestTrue(APSStellarMaterialTests::Context(Material, TEXT("contrast-preserving actor tone")),
				CompactCode.Contains(TEXT("actorTone=lerp(0.78,1.42,actorActivity)")));
			TestTrue(APSStellarMaterialTests::Context(Material, TEXT("HDR HISM proxy tone")),
				CompactCode.Contains(
					TEXT("proxyTone=lerp(1.35,3.80,proxyActivity)")));
			TestTrue(APSStellarMaterialTests::Context(Material, TEXT("actor and HISM tone selection")),
				CompactCode.Contains(
					TEXT("toneSafeEmission=lerp(actorTone,proxyTone,useInstance)")));
			TestTrue(APSStellarMaterialTests::Context(Material, TEXT("perceptual limb ratio")),
				CompactCode.Contains(
					TEXT("limb=lerp(0.22,1.0,pow(facing,0.62))")));
			TestTrue(APSStellarMaterialTests::Context(Material, TEXT("compact corona rim")),
				CompactCode.Contains(TEXT("rim=pow(1.0-facing,3.15)"))
				&& CompactCode.Contains(TEXT("resolvedProminence=prominenceMask*spatialDetail"))
				&& CompactCode.Contains(TEXT("CoronaAmount*rim*(0.28+resolvedProminence*0.72)")));
			TestTrue(APSStellarMaterialTests::Context(Material, TEXT("spectral normalization mix")),
				CompactCode.Contains(
					TEXT("spectralTint=lerp(spectralColor,normalizedSpectralTint,0.55)")));
			TestTrue(APSStellarMaterialTests::Context(Material, TEXT("near-black spectral rejection")),
				CompactCode.Contains(
					TEXT("spectralVisibility=smoothstep(0.08,0.90,maxSpectral)"))
				&& !CompactCode.Contains(TEXT("validStellarSpectrum")));
			TestTrue(APSStellarMaterialTests::Context(Material, TEXT("per-path HDR safety ceiling")),
				CompactCode.Contains(
					TEXT("outputCeiling=lerp(8.0,5.0,useInstance)")));
			TestTrue(APSStellarMaterialTests::Context(Material, TEXT("colour-preserving output ceiling")),
				CompactCode.Contains(
					TEXT("peakChannel=max(max(preBloom.r,preBloom.g),preBloom.b)"))
				&& CompactCode.Contains(
					TEXT("returnpreBloom*min(1.0,outputCeiling/max(peakChannel,0.0001))")));
			TestTrue(APSStellarMaterialTests::Context(Material, TEXT("substellar visibility preservation")),
				StellarSurface->Code.Contains(TEXT("spectralVisibility")));
			TestTrue(APSStellarMaterialTests::Context(Material, TEXT("spectral-coloured highlights")),
				CompactCode.Contains(
					TEXT("spectralHighlightTint=normalizedSpectralTint*spectralVisibility")));
			TestTrue(APSStellarMaterialTests::Context(Material, TEXT("sparse jewel cell mask")),
				CompactCode.Contains(
					TEXT("microUnit=microGranules*0.5+0.5"))
				&& CompactCode.Contains(
					TEXT("granuleCell=smoothstep(0.22,0.78,microUnit)"))
				&& CompactCode.Contains(
					TEXT("granuleSpark=smoothstep(0.84,0.97,microUnit)*spatialDetail"))
				&& CompactCode.Contains(
					TEXT("jewelCell=saturate((resolvedCellHeat-0.67)*4.166667)"))
				&& CompactCode.Contains(
					TEXT("jewelMask=jewelCell*lerp(0.52,1.0,saturate(faculae*5.0+granuleSpark))")));
			TestTrue(APSStellarMaterialTests::Context(Material, TEXT("spatially phased actor jewel twinkle")),
				CompactCode.Contains(
					TEXT("jewelPhase=GameTime*lerp(1.35,1.85,seed)+phase*0.61+mesoCells*4.0")));
			TestTrue(APSStellarMaterialTests::Context(Material, TEXT("deterministic instance jewel pulse")),
				CompactCode.Contains(
					TEXT("instanceJewelPulse=lerp(0.58,0.82,seed)"))
				&& CompactCode.Contains(
					TEXT("jewelPulse=lerp(actorJewelPulse,instanceJewelPulse,useInstance)")));
			TestTrue(APSStellarMaterialTests::Context(Material, TEXT("scale-aware jewel lift")),
				CompactCode.Contains(
					TEXT("jewelLift=spatialDetail*jewelMask*lerp(0.08,0.18,jewelPulse)")));
			TestTrue(APSStellarMaterialTests::Context(Material, TEXT("scale-aware spectral jewel rim")),
				CompactCode.Contains(
					TEXT("rimJewelMask=spatialDetail*rim*lerp(0.18,1.0,prominenceMask)"))
				&& CompactCode.Contains(
					TEXT("rimJewelLift=rimJewelMask*lerp(0.035,0.090,jewelPulse)")));
			TestTrue(APSStellarMaterialTests::Context(Material, TEXT("ordinary pre-bloom stellar output")),
				CompactCode.Contains(
					TEXT("ordinaryPreBloom=surfaceTint*stellarSignal*temporalFlicker"))
				&& CompactCode.Contains(
					TEXT("+spectralHighlightTint*coronaBloom"))
				&& CompactCode.Contains(
					TEXT("ordinaryPreBloom*(1.0-blackHoleType)")));
			TestTrue(APSStellarMaterialTests::Context(Material, TEXT("actor stellar archetype masks")),
				CompactCode.Contains(TEXT("giantType=actorOnly*"))
				&& CompactCode.Contains(TEXT("protostarType=actorOnly*"))
				&& CompactCode.Contains(TEXT("compactType=actorOnly*"))
				&& CompactCode.Contains(TEXT("pulsarType=actorOnly*"))
				&& CompactCode.Contains(TEXT("blackHoleType=actorOnly*")));
			TestTrue(APSStellarMaterialTests::Context(Material, TEXT("sparse core and limb HDR structures")),
				CompactCode.Contains(TEXT("coreBloom=actorOnly*pow(facing,8.0)"))
				&& CompactCode.Contains(TEXT("lerp(0.10,0.42,emissionActivity)"))
				&& CompactCode.Contains(TEXT("coronaGain=lerp(1.0,lerp(1.35,2.25,emissionActivity)"))
				&& CompactCode.Contains(TEXT("coronaBloom=actorOnly*CoronaAmount*pow(rim,1.35)"))
				&& CompactCode.Contains(TEXT("lerp(24.0,38.0,emissionActivity)*typeCoronaGain"))
				&& CompactCode.Contains(TEXT("lerp(0.72,1.0,resolvedProminence)"))
				&& CompactCode.Contains(TEXT("compactLift=compactType*polarCap*1.45"))
				&& CompactCode.Contains(TEXT("pulsarLift=pulsarType*pow(axisAlignment,28.0)")));
			TestTrue(APSStellarMaterialTests::Context(Material, TEXT("black-hole dark core and hot ring")),
				CompactCode.Contains(TEXT("accretionBand=exp2(-diskLatitude*diskLatitude*92.0)"))
				&& CompactCode.Contains(TEXT("photonRing=smoothstep(0.56,0.94,1.0-facing)"))
				&& CompactCode.Contains(TEXT("blackHoleSignal=blackHoleType*")));
			TestFalse(APSStellarMaterialTests::Context(Material, TEXT("no literal-white highlights")),
				CompactCode.Contains(TEXT("float3(1.0,1.0,1.0)")));
			TestFalse(APSStellarMaterialTests::Context(Material, TEXT("no quantised cell grid")),
				StellarSurface->Code.Contains(TEXT("floor(")));
			TestFalse(APSStellarMaterialTests::Context(Material, TEXT("no white clipping ceiling")),
				StellarSurface->Code.Contains(TEXT("30.0")));
			TestTrue(APSStellarMaterialTests::Context(Material, TEXT("seeded subtle temporal flicker")),
				StellarSurface->Code.Contains(TEXT("flickerPhase = GameTime"))
				&& StellarSurface->Code.Contains(TEXT("temporalFlicker")));
			TestTrue(APSStellarMaterialTests::Context(Material, TEXT("actor-only temporal flicker")),
				CompactCode.Contains(TEXT("temporalDetail=1.0-useInstance"))
				&& CompactCode.Contains(TEXT("flickerAmplitude=temporalDetail*0.020")));
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
		AddInfo(FString::Printf(
			TEXT("%s shader stats: pixel=%d vertex=%d samplers=%d vertexTextures=%d virtualTextures=%d"),
			*GetNameSafe(Material),
			Statistics.NumPixelShaderInstructions,
			Statistics.NumVertexShaderInstructions,
			Statistics.NumSamplers,
			Statistics.NumVertexTextureSamples,
			Statistics.NumVirtualTextureSamples));
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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FAPSStellarPointCoronaMaterialTest,
	"APS.Rendered.Materials.AdditivePointAndCoronaMaster",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAPSStellarPointCoronaMaterialTest::RunTest(const FString& Parameters)
{
	UMaterial* Material = LoadObject<UMaterial>(
		nullptr, APSStellarMaterialContract::HismBaseObjectPath);
	if (!TestNotNull(TEXT("Load additive HISM/corona master"), Material))
	{
		return false;
	}

	TestEqual(APSStellarMaterialTests::Context(Material, TEXT("surface domain")),
		Material->MaterialDomain.GetValue(), MD_Surface);
	TestEqual(APSStellarMaterialTests::Context(Material, TEXT("additive blend mode")),
		Material->GetBlendMode(), BLEND_Additive);
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
	TestTrue(APSStellarMaterialTests::Context(
		Material, TEXT("instanced-static-mesh usage flag")),
		Material->bUsedWithInstancedStaticMeshes != 0);
	TestTrue(APSStellarMaterialTests::Context(
		Material, TEXT("instanced-static-mesh usage query")),
		Material->GetUsageByFlag(MATUSAGE_InstancedStaticMeshes));

	const TConstArrayView<TObjectPtr<UMaterialExpression>> Expressions =
		Material->GetExpressions();
	TestEqual(APSStellarMaterialTests::Context(
		Material, TEXT("point/corona expression count")),
		Expressions.Num(), APSStellarMaterialTests::ExpectedPointCoronaExpressionCount);

	UMaterialExpressionVectorParameter* ColorParameter = nullptr;
	TMap<FName, UMaterialExpressionScalarParameter*> ScalarParameters;
	TSet<FName> ParameterNames;
	TSet<FGuid> ParameterGuids;
	UMaterialExpression* InstanceColor = nullptr;
	TMap<uint32, UMaterialExpression*> InstanceScalars;
	UMaterialExpression* PixelNormal = nullptr;
	UMaterialExpression* WorldPosition = nullptr;
	UMaterialExpression* ObjectPosition = nullptr;
	UMaterialExpression* VertexInterpolator = nullptr;
	UMaterialExpressionCameraVectorWS* CameraVector = nullptr;
	UMaterialExpressionCustom* PointAndCorona = nullptr;
	int32 VectorParameterCount = 0;
	int32 ScalarParameterCount = 0;
	int32 InstanceVectorCount = 0;
	int32 InstanceScalarCount = 0;
	int32 PixelNormalCount = 0;
	int32 WorldPositionCount = 0;
	int32 ObjectPositionCount = 0;
	int32 VertexInterpolatorCount = 0;
	int32 CameraVectorCount = 0;
	int32 CustomCount = 0;
	int32 TimeCount = 0;
	int32 TextureExpressionCount = 0;
	int32 RecognizedExpressionCount = 0;

	for (UMaterialExpression* Expression : Expressions)
	{
		if (!Expression)
		{
			AddError(APSStellarMaterialTests::Context(
				Material, TEXT("null expression in point/corona graph")));
			continue;
		}

		const FName ExpressionClassName = Expression->GetClass()->GetFName();
		const FString ExpressionClassString = ExpressionClassName.ToString();
		if (ExpressionClassName == TEXT("MaterialExpressionTime"))
		{
			++TimeCount;
		}
		if (ExpressionClassString.Contains(TEXT("Texture")))
		{
			++TextureExpressionCount;
		}

		if (UMaterialExpressionVectorParameter* Vector =
			Cast<UMaterialExpressionVectorParameter>(Expression))
		{
			++VectorParameterCount;
			++RecognizedExpressionCount;
			ColorParameter = Vector;
			ParameterNames.Add(Vector->ParameterName);
			ParameterGuids.Add(Vector->ExpressionGUID);
		}
		else if (UMaterialExpressionScalarParameter* Scalar =
			Cast<UMaterialExpressionScalarParameter>(Expression))
		{
			++ScalarParameterCount;
			++RecognizedExpressionCount;
			ScalarParameters.Add(Scalar->ParameterName, Scalar);
			ParameterNames.Add(Scalar->ParameterName);
			ParameterGuids.Add(Scalar->ExpressionGUID);
		}
		else if (ExpressionClassName ==
			TEXT("MaterialExpressionPerInstanceCustomData3Vector"))
		{
			++InstanceVectorCount;
			++RecognizedExpressionCount;
			InstanceColor = Expression;
		}
		else if (ExpressionClassName == TEXT("MaterialExpressionPerInstanceCustomData"))
		{
			++InstanceScalarCount;
			++RecognizedExpressionCount;
			uint32 DataIndex = MAX_uint32;
			if (APSStellarMaterialTests::ReadUnsignedProperty(
				Expression, TEXT("DataIndex"), DataIndex))
			{
				InstanceScalars.Add(DataIndex, Expression);
			}
		}
		else if (ExpressionClassName == TEXT("MaterialExpressionPixelNormalWS"))
		{
			++PixelNormalCount;
			++RecognizedExpressionCount;
			PixelNormal = Expression;
		}
		else if (ExpressionClassName == TEXT("MaterialExpressionWorldPosition"))
		{
			++WorldPositionCount;
			++RecognizedExpressionCount;
			WorldPosition = Expression;
		}
		else if (ExpressionClassName == TEXT("MaterialExpressionObjectPositionWS"))
		{
			++ObjectPositionCount;
			++RecognizedExpressionCount;
			ObjectPosition = Expression;
		}
		else if (ExpressionClassName == TEXT("MaterialExpressionVertexInterpolator"))
		{
			++VertexInterpolatorCount;
			++RecognizedExpressionCount;
			VertexInterpolator = Expression;
		}
		else if (UMaterialExpressionCameraVectorWS* Camera =
			Cast<UMaterialExpressionCameraVectorWS>(Expression))
		{
			++CameraVectorCount;
			++RecognizedExpressionCount;
			CameraVector = Camera;
		}
		else if (UMaterialExpressionCustom* Custom =
			Cast<UMaterialExpressionCustom>(Expression))
		{
			++CustomCount;
			++RecognizedExpressionCount;
			PointAndCorona = Custom;
		}
		else
		{
			AddError(APSStellarMaterialTests::Context(Material,
				*FString::Printf(TEXT("unexpected point/corona expression %s"),
					*ExpressionClassString)));
		}

		if (UMaterialExpressionParameter* Parameter =
			Cast<UMaterialExpressionParameter>(Expression))
		{
			TestEqual(APSStellarMaterialTests::Context(
				Material, TEXT("point/corona parameter group")),
				Parameter->Group, FName(TEXT("APS Stellar Surface")));
			TestTrue(APSStellarMaterialTests::Context(
				Material, TEXT("point/corona parameter GUID")),
				Parameter->ExpressionGUID.IsValid());
		}
	}

	TestEqual(APSStellarMaterialTests::Context(Material, TEXT("recognized expression count")),
		RecognizedExpressionCount, APSStellarMaterialTests::ExpectedPointCoronaExpressionCount);
	TestEqual(APSStellarMaterialTests::Context(Material, TEXT("vector parameter count")),
		VectorParameterCount, 1);
	TestEqual(APSStellarMaterialTests::Context(Material, TEXT("scalar parameter count")),
		ScalarParameterCount, 5);
	TestEqual(APSStellarMaterialTests::Context(Material, TEXT("instance-vector count")),
		InstanceVectorCount, 1);
	TestEqual(APSStellarMaterialTests::Context(Material, TEXT("instance-scalar count")),
		InstanceScalarCount, 3);
	TestEqual(APSStellarMaterialTests::Context(Material, TEXT("pixel-normal count")),
		PixelNormalCount, 1);
	TestEqual(APSStellarMaterialTests::Context(Material, TEXT("world-position count")),
		WorldPositionCount, 1);
	TestEqual(APSStellarMaterialTests::Context(Material, TEXT("object-position count")),
		ObjectPositionCount, 1);
	TestEqual(APSStellarMaterialTests::Context(Material, TEXT("vertex-interpolator count")),
		VertexInterpolatorCount, 1);
	if (TestNotNull(APSStellarMaterialTests::Context(
		Material, TEXT("corona centre vertex interpolator")), VertexInterpolator))
	{
		const FExpressionInput* VertexInput = VertexInterpolator->GetInput(0);
		if (TestNotNull(APSStellarMaterialTests::Context(
			Material, TEXT("corona centre vertex input")), VertexInput))
		{
			TestTrue(APSStellarMaterialTests::Context(
				Material, TEXT("corona ObjectPositionWS is evaluated in vertex stage")),
				VertexInput->Expression == ObjectPosition);
		}
	}
	TestEqual(APSStellarMaterialTests::Context(Material, TEXT("camera-vector count")),
		CameraVectorCount, 1);
	TestEqual(APSStellarMaterialTests::Context(Material, TEXT("custom expression count")),
		CustomCount, 1);
	TestEqual(APSStellarMaterialTests::Context(Material, TEXT("time-expression count")),
		TimeCount, 0);
	TestEqual(APSStellarMaterialTests::Context(Material, TEXT("texture-expression count")),
		TextureExpressionCount, 0);
	TestEqual(APSStellarMaterialTests::Context(Material, TEXT("exact parameter count")),
		ParameterNames.Num(), 6);
	TestEqual(APSStellarMaterialTests::Context(Material, TEXT("unique parameter GUIDs")),
		ParameterGuids.Num(), 6);

	if (TestNotNull(APSStellarMaterialTests::Context(
		Material, TEXT("Color parameter")), ColorParameter))
	{
		TestEqual(APSStellarMaterialTests::Context(Material, TEXT("Color name")),
			ColorParameter->ParameterName, FName(TEXT("Color")));
		TestEqual(APSStellarMaterialTests::Context(Material, TEXT("Color sort priority")),
			ColorParameter->SortPriority, 0);
		TestTrue(APSStellarMaterialTests::Context(Material, TEXT("Color default")),
			ColorParameter->DefaultValue.Equals(FLinearColor(1.0f, 0.66f, 0.30f)));
	}

	struct FScalarContract
	{
		FName Name;
		float DefaultValue;
		float SliderMin;
		float SliderMax;
		int32 SortPriority;
	};
	const FScalarContract ScalarContracts[] = {
		{ TEXT("CoronaIntensity"), 8.0f, 0.0f, 192.0f, 1 },
		{ TEXT("CoronaOpacity"), 0.72f, 0.0f, 1.0f, 2 },
		{ TEXT("CoronaSeed"), 0.371f, 0.0f, 1.0f, 3 },
		{ TEXT("CoronaShellMode"), 0.0f, 0.0f, 1.0f, 4 },
		{ TEXT("CoronaInnerRadius"), 0.8928571f, 0.50f, 0.95f, 5 }
	};
	for (const FScalarContract& Contract : ScalarContracts)
	{
		UMaterialExpressionScalarParameter* const* Found =
			ScalarParameters.Find(Contract.Name);
		if (TestNotNull(APSStellarMaterialTests::Context(Material,
			*FString::Printf(TEXT("scalar parameter %s"), *Contract.Name.ToString())),
			Found))
		{
			const UMaterialExpressionScalarParameter* Scalar = *Found;
			TestTrue(APSStellarMaterialTests::Context(Material,
				*FString::Printf(TEXT("%s default"), *Contract.Name.ToString())),
				FMath::IsNearlyEqual(Scalar->DefaultValue, Contract.DefaultValue));
			TestTrue(APSStellarMaterialTests::Context(Material,
				*FString::Printf(TEXT("%s slider range"), *Contract.Name.ToString())),
				FMath::IsNearlyEqual(Scalar->SliderMin, Contract.SliderMin)
				&& FMath::IsNearlyEqual(Scalar->SliderMax, Contract.SliderMax));
			TestEqual(APSStellarMaterialTests::Context(Material,
				*FString::Printf(TEXT("%s sort priority"), *Contract.Name.ToString())),
				Scalar->SortPriority, Contract.SortPriority);
		}
	}

	if (TestNotNull(APSStellarMaterialTests::Context(
		Material, TEXT("instance colour expression")), InstanceColor))
	{
		uint32 DataIndex = MAX_uint32;
		FLinearColor DefaultValue = FLinearColor::White;
		TestTrue(APSStellarMaterialTests::Context(
			Material, TEXT("read instance colour data index")),
			APSStellarMaterialTests::ReadUnsignedProperty(
				InstanceColor, TEXT("DataIndex"), DataIndex));
		TestTrue(APSStellarMaterialTests::Context(
			Material, TEXT("read instance colour fallback")),
			APSStellarMaterialTests::ReadLinearColorProperty(
				InstanceColor, TEXT("ConstDefaultValue"), DefaultValue));
		TestEqual(APSStellarMaterialTests::Context(
			Material, TEXT("instance colour data index")), DataIndex, 0u);
		TestTrue(APSStellarMaterialTests::Context(
			Material, TEXT("instance colour black fallback")),
			DefaultValue.Equals(FLinearColor::Black));
	}
	for (const uint32 DataIndex : { 3u, 4u, 5u })
	{
		UMaterialExpression* const* InstanceScalar = InstanceScalars.Find(DataIndex);
		if (TestNotNull(APSStellarMaterialTests::Context(Material,
			*FString::Printf(TEXT("instance scalar data index %u"), DataIndex)),
			InstanceScalar))
		{
			float DefaultValue = TNumericLimits<float>::Max();
			TestTrue(APSStellarMaterialTests::Context(
				Material, TEXT("read instance scalar fallback")),
				APSStellarMaterialTests::ReadFloatProperty(
					*InstanceScalar, TEXT("ConstDefaultValue"), DefaultValue));
			TestTrue(APSStellarMaterialTests::Context(
				Material, TEXT("instance scalar zero fallback")),
				FMath::IsNearlyZero(DefaultValue));
		}
	}

	if (TestNotNull(APSStellarMaterialTests::Context(
		Material, TEXT("point/corona custom expression")), PointAndCorona))
	{
		TestEqual(APSStellarMaterialTests::Context(Material, TEXT("custom output type")),
			PointAndCorona->OutputType.GetValue(), CMOT_Float3);
		TestEqual(APSStellarMaterialTests::Context(Material, TEXT("custom description")),
			PointAndCorona->Description,
			FString(TEXT("APS HDR point star and actor corona shell")));

		static const FName ExpectedInputOrder[] = {
			TEXT("Color"), TEXT("CoronaIntensity"), TEXT("CoronaOpacity"),
			TEXT("CoronaSeed"), TEXT("CoronaShellMode"), TEXT("CoronaInnerRadius"),
			TEXT("InstanceColor"),
			TEXT("InstanceEmission"), TEXT("InstanceSeed"), TEXT("SystemMarker"),
			TEXT("NormalWS"), TEXT("WorldPositionWS"), TEXT("ObjectPositionWS"),
			TEXT("CameraWS")
		};
		TestEqual(APSStellarMaterialTests::Context(Material, TEXT("custom input count")),
			PointAndCorona->Inputs.Num(),
			static_cast<int32>(UE_ARRAY_COUNT(ExpectedInputOrder)));

		TMap<FName, UMaterialExpression*> ExpectedInputExpressions;
		ExpectedInputExpressions.Add(TEXT("Color"), ColorParameter);
		ExpectedInputExpressions.Add(TEXT("CoronaIntensity"),
			ScalarParameters.FindRef(TEXT("CoronaIntensity")));
		ExpectedInputExpressions.Add(TEXT("CoronaOpacity"),
			ScalarParameters.FindRef(TEXT("CoronaOpacity")));
		ExpectedInputExpressions.Add(TEXT("CoronaSeed"),
			ScalarParameters.FindRef(TEXT("CoronaSeed")));
		ExpectedInputExpressions.Add(TEXT("CoronaShellMode"),
			ScalarParameters.FindRef(TEXT("CoronaShellMode")));
		ExpectedInputExpressions.Add(TEXT("CoronaInnerRadius"),
			ScalarParameters.FindRef(TEXT("CoronaInnerRadius")));
		ExpectedInputExpressions.Add(TEXT("InstanceColor"), InstanceColor);
		ExpectedInputExpressions.Add(TEXT("InstanceEmission"), InstanceScalars.FindRef(3u));
		ExpectedInputExpressions.Add(TEXT("InstanceSeed"), InstanceScalars.FindRef(4u));
		ExpectedInputExpressions.Add(TEXT("SystemMarker"), InstanceScalars.FindRef(5u));
		ExpectedInputExpressions.Add(TEXT("NormalWS"), PixelNormal);
		ExpectedInputExpressions.Add(TEXT("WorldPositionWS"), WorldPosition);
		ExpectedInputExpressions.Add(TEXT("ObjectPositionWS"), VertexInterpolator);
		ExpectedInputExpressions.Add(TEXT("CameraWS"), CameraVector);

		for (int32 InputIndex = 0;
			InputIndex < PointAndCorona->Inputs.Num()
				&& InputIndex < static_cast<int32>(UE_ARRAY_COUNT(ExpectedInputOrder));
			++InputIndex)
		{
			const FCustomInput& Input = PointAndCorona->Inputs[InputIndex];
			const FName ExpectedName = ExpectedInputOrder[InputIndex];
			TestEqual(APSStellarMaterialTests::Context(Material,
				*FString::Printf(TEXT("custom input %d name"), InputIndex)),
				Input.InputName, ExpectedName);
			UMaterialExpression* const* ExpectedExpression =
				ExpectedInputExpressions.Find(ExpectedName);
			if (TestNotNull(APSStellarMaterialTests::Context(Material,
				*FString::Printf(TEXT("custom input %s contract"), *ExpectedName.ToString())),
				ExpectedExpression))
			{
				TestTrue(APSStellarMaterialTests::Context(Material,
					*FString::Printf(TEXT("custom input %s connection"),
						*ExpectedName.ToString())),
					Input.Input.Expression == *ExpectedExpression);
			}
		}

		const FString CompactCode =
			APSStellarMaterialTests::CompactShaderCode(PointAndCorona->Code);
		TestTrue(APSStellarMaterialTests::Context(
			Material, TEXT("explicit point/corona mode switch")),
			CompactCode.Contains(TEXT("floatshellMode=step(0.5,CoronaShellMode);"))
			&& CompactCode.Contains(
				TEXT("returnlerp(pointSignal,coronaSignal,shellMode);")));
		TestTrue(APSStellarMaterialTests::Context(
			Material, TEXT("Gaussian hot core and soft halo")),
			CompactCode.Contains(
				TEXT("floatcoreSharpness=lerp(28.0,14.0,pointActivity);"))
			&& CompactCode.Contains(
				TEXT("floathaloSharpness=lerp(5.50,3.00,pointActivity);"))
			&& CompactCode.Contains(
				TEXT("floatedgeFade=smoothstep(0.02,0.28,facing);"))
			&& CompactCode.Contains(
				TEXT("floathotCore=exp2(-projectedRadiusSq*coreSharpness)*edgeFade;"))
			&& CompactCode.Contains(
				TEXT("floatsoftHalo=exp2(-projectedRadiusSq*haloSharpness)*edgeFade;"))
			&& CompactCode.Contains(
				TEXT("floatcoreEnergy=lerp(4.0,11.0,activity)*seedGain*(1.0+marker*0.22);"))
			&& CompactCode.Contains(
				TEXT("floathaloEnergy=lerp(0.62,2.6,activity)*seedGain*(1.0+marker*0.18);"))
			&& CompactCode.Contains(
				TEXT("floatcoreWhitening=lerp(0.58,0.78,activity);"))
			&& CompactCode.Contains(
				TEXT("float3neutralCoreTint=float3(spectralVisibility,spectralVisibility,spectralVisibility);"))
			&& CompactCode.Contains(
				TEXT("float3hotCoreTint=lerp(pointTint,neutralCoreTint,coreWhitening);"))
			&& CompactCode.Contains(
				TEXT("float3haloTint=lerp(pointTint,neutralCoreTint,0.04);"))
			&& CompactCode.Contains(
				TEXT("float3pointSignal=hotCoreTint*(hotCore*coreEnergy)+haloTint*(softHalo*haloEnergy);")));
		TestTrue(APSStellarMaterialTests::Context(
			Material, TEXT("corona shell reconstructs a continuous radial projection")),
			CompactCode.Contains(
				TEXT("float3shellRadial=WorldPositionWS-ObjectPositionWS;"))
			&& CompactCode.Contains(
				TEXT("float3shellNormal=shellRadialLengthSq>1.0e-8?shellRadial*rsqrt(shellRadialLengthSq):n;"))
			&& CompactCode.Contains(
				TEXT("floatshellProjectedRadiusSq=saturate(1.0-shellFacing*shellFacing);")));
		TestTrue(APSStellarMaterialTests::Context(
			Material, TEXT("corona shell begins at the opaque photosphere projection")),
			CompactCode.Contains(
				TEXT("floatshellInnerRadius=saturate(CoronaInnerRadius);"))
			&& CompactCode.Contains(
				TEXT("floatshellRadiusUnclamped=(shellProjectedRadiusSq-shellInnerRadiusSq)/shellSpanSq;"))
			&& CompactCode.Contains(
				TEXT("floatphotosphereOcclusion=smoothstep(0.0,0.030,shellRadiusUnclamped);")));
		TestTrue(APSStellarMaterialTests::Context(
			Material, TEXT("corona shell concentrates HDR at the limb without an LDR tire")),
			CompactCode.Contains(
				TEXT("floatouterBoundaryFade=1.0-smoothstep(0.72,1.0,shellRadius01);"))
			&& CompactCode.Contains(
				TEXT("floatradialHdrSeed=exp2(-shellRadius01*32.0);"))
			&& CompactCode.Contains(
				TEXT("floatradialSpectralTail=0.050*exp2(-shellRadius01*1.6)*outerBoundaryFade;"))
			&& CompactCode.Contains(
				TEXT("floatradialLimbFalloff=radialHdrSeed+radialSpectralTail;"))
			&& CompactCode.Contains(
				TEXT("floatshellVariation=lerp(0.94,1.06,magneticField);"))
			&& CompactCode.Contains(
				TEXT("floatshellSignal=photosphereOcclusion*radialLimbFalloff*shellVariation*saturate(CoronaOpacity);"))
			&& CompactCode.Contains(
				TEXT("float3spectralShellTint=pow(max(normalizedShellTint,0.001),1.20);")));
		TestFalse(APSStellarMaterialTests::Context(
			Material, TEXT("actor corona has no detached radial peak")),
			CompactCode.Contains(TEXT("bloomSeedOffset"))
			|| CompactCode.Contains(TEXT("radialBloomSeed")));
		TestFalse(APSStellarMaterialTests::Context(Material, TEXT("no shader time input")),
			CompactCode.Contains(TEXT("GameTime"))
			|| CompactCode.Contains(TEXT("Time(")));
		TestFalse(APSStellarMaterialTests::Context(Material, TEXT("no shader texture sampling")),
			CompactCode.Contains(TEXT("Texture"))
			|| CompactCode.Contains(TEXT(".Sample")));

		const FExpressionInput* Emissive =
			Material->GetExpressionInputForProperty(MP_EmissiveColor);
		if (TestNotNull(APSStellarMaterialTests::Context(
			Material, TEXT("emissive property input")), Emissive))
		{
			TestTrue(APSStellarMaterialTests::Context(
				Material, TEXT("custom drives emissive")),
				Emissive->Expression == PointAndCorona);
		}
		const FExpressionInput* Opacity =
			Material->GetExpressionInputForProperty(MP_Opacity);
		TestTrue(APSStellarMaterialTests::Context(
			Material, TEXT("black additive output owns transparency")),
			!Opacity || !Opacity->Expression);
	}

	const FMaterialStatistics Statistics = UMaterialEditingLibrary::GetStatistics(Material);
	AddInfo(FString::Printf(
		TEXT("%s point/corona stats: pixel=%d vertex=%d samplers=%d vertexTextures=%d virtualTextures=%d"),
		*GetNameSafe(Material), Statistics.NumPixelShaderInstructions,
		Statistics.NumVertexShaderInstructions, Statistics.NumSamplers,
		Statistics.NumVertexTextureSamples, Statistics.NumVirtualTextureSamples));
	TestEqual(APSStellarMaterialTests::Context(Material, TEXT("zero samplers")),
		Statistics.NumSamplers, 0);
	TestEqual(APSStellarMaterialTests::Context(
		Material, TEXT("zero vertex texture samples")),
		Statistics.NumVertexTextureSamples, 0);
	TestEqual(APSStellarMaterialTests::Context(
		Material, TEXT("zero virtual texture samples")),
		Statistics.NumVirtualTextureSamples, 0);
	TestTrue(APSStellarMaterialTests::Context(Material, TEXT("pixel instruction budget")),
		Statistics.NumPixelShaderInstructions <=
			APSStellarMaterialTests::MaximumPointCoronaPixelInstructions);
	if (Statistics.NumPixelShaderInstructions == 0)
	{
		AddWarning(FString::Printf(
			TEXT("%s has no compiled point/corona pixel statistic on this test RHI; "
				"the exact expression and texture-free budgets were still checked."),
			*GetNameSafe(Material)));
	}

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
			TestTrue(TEXT("Native corona starts at the compact main-sequence scale"),
				NativeStar->CoronaMesh->GetRelativeScale3D().Equals(
					FVector(1.12), 1.0e-6));
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
	float MaximumActorTone = 0.0f;
	float MaximumHISMTone = 0.0f;
	for (int32 RawEmission = 0; RawEmission <= 500; ++RawEmission)
	{
		MaximumActorTone = FMath::Max(MaximumActorTone,
			APSStellarMaterialTests::ActorTone(static_cast<float>(RawEmission)));
		MaximumHISMTone = FMath::Max(MaximumHISMTone,
			APSStellarMaterialTests::HISMProxyTone(static_cast<float>(RawEmission)));
	}

	TestTrue(*FString::Printf(
		TEXT("Actor maximum pre-bloom tone %.4f remains <= %.2f"),
		MaximumActorTone, APSStellarMaterialTests::MaximumActorPreBloom),
		MaximumActorTone <= APSStellarMaterialTests::MaximumActorPreBloom
			+ UE_SMALL_NUMBER);
	TestTrue(*FString::Printf(
		TEXT("HISM maximum pre-bloom tone %.4f remains <= %.2f"),
		MaximumHISMTone, APSStellarMaterialTests::MaximumHISMPreBloom),
		MaximumHISMTone <= APSStellarMaterialTests::MaximumHISMPreBloom
			+ UE_SMALL_NUMBER);
	TestTrue(TEXT("HISM zero-energy floor remains above the HDR bloom threshold"),
		APSStellarMaterialTests::HISMProxyTone(0.0f) > 1.0f);

	const float Cap6Activity = APSStellarMaterialTests::HISMProxyEmissionActivity(6.0f);
	const float Cap12Activity = APSStellarMaterialTests::HISMProxyEmissionActivity(12.0f);
	const float Cap24Activity = APSStellarMaterialTests::HISMProxyEmissionActivity(24.0f);
	TestTrue(*FString::Printf(
		TEXT("HISM raw caps have ordered activity: 6=%.4f 12=%.4f 24=%.4f"),
		Cap6Activity, Cap12Activity, Cap24Activity),
		Cap6Activity < Cap12Activity && Cap12Activity < Cap24Activity);
	TestTrue(*FString::Printf(
		TEXT("HISM raw caps remain perceptually separated: d6-12=%.4f d12-24=%.4f"),
		Cap12Activity - Cap6Activity, Cap24Activity - Cap12Activity),
		Cap12Activity - Cap6Activity
			>= APSStellarMaterialTests::MinimumProxyActivitySeparation
		&& Cap24Activity - Cap12Activity
			>= APSStellarMaterialTests::MinimumProxyActivitySeparation);

	const float LimbToCenterRatio = APSStellarMaterialTests::LimbEdgeIntensity
		/ APSStellarMaterialTests::LimbCenterIntensity;
	TestTrue(*FString::Printf(
		TEXT("Limb-to-centre ratio %.3f preserves spherical depth"), LimbToCenterRatio),
		LimbToCenterRatio >= 0.18f && LimbToCenterRatio <= 0.30f);
	TestTrue(TEXT("Limb curve remains gradual rather than a hard-edged disc"),
		APSStellarMaterialTests::LimbExponent >= 0.50f
		&& APSStellarMaterialTests::LimbExponent <= 0.75f);
	TestTrue(TEXT("Corona rim has fallen below twelve percent by mid-disc"),
		FMath::Pow(0.5f, APSStellarMaterialTests::RimExponent) <= 0.12f);
	TestTrue(TEXT("Actor flicker remains visible and bounded to a two-percent pulse"),
		APSStellarMaterialTests::ActorFlickerAmplitude > 0.005f
		&& APSStellarMaterialTests::ActorFlickerAmplitude <= 0.02f + UE_SMALL_NUMBER);
	TestTrue(TEXT("HISM temporal flicker is exactly disabled"),
		FMath::IsNearlyZero(APSStellarMaterialTests::HISMProxyFlickerAmplitude));

	const float ActorSpatialDetail = APSStellarMaterialTests::StellarSpatialDetail(
		0.0f, APSStellarMaterialTests::SubpixelHISMSampleFootprint);
	const float ResolvedHISMSpatialDetail = APSStellarMaterialTests::StellarSpatialDetail(
		1.0f, APSStellarMaterialTests::ResolvedHISMSampleFootprint);
	const float SubpixelHISMSpatialDetail = APSStellarMaterialTests::StellarSpatialDetail(
		1.0f, APSStellarMaterialTests::SubpixelHISMSampleFootprint);
	TestTrue(*FString::Printf(
		TEXT("Actor retains exact full spatial detail at every footprint (%.4f)"),
		ActorSpatialDetail),
		FMath::IsNearlyEqual(ActorSpatialDetail, 1.0f));
	TestTrue(*FString::Printf(
		TEXT("Resolved HISM retains strong nonzero spatial detail (%.4f)"),
		ResolvedHISMSpatialDetail),
		ResolvedHISMSpatialDetail > 0.5f && ResolvedHISMSpatialDetail < 1.0f);
	TestTrue(*FString::Printf(
		TEXT("Sub-pixel HISM removes alias-prone spatial detail (%.4f)"),
		SubpixelHISMSpatialDetail),
		FMath::IsNearlyZero(SubpixelHISMSpatialDetail));

	const float ActorTemporalDetail = APSStellarMaterialTests::StellarTemporalDetail(0.0f);
	const float ResolvedHISMTemporalDetail =
		APSStellarMaterialTests::StellarTemporalDetail(1.0f);
	const float SubpixelHISMTemporalDetail =
		APSStellarMaterialTests::StellarTemporalDetail(1.0f);
	TestTrue(TEXT("Actor retains the authored temporal pulse"),
		FMath::IsNearlyEqual(ActorTemporalDetail, 1.0f));
	TestTrue(TEXT("Resolved and sub-pixel HISM remain temporally stable"),
		FMath::IsNearlyZero(ResolvedHISMTemporalDetail)
		&& FMath::IsNearlyZero(SubpixelHISMTemporalDetail));
	TestTrue(TEXT("HISM static jewel pulse is deterministic and bounded"),
		APSStellarMaterialTests::HISMStaticJewelPulseMinimum > 0.0f
		&& APSStellarMaterialTests::HISMStaticJewelPulseMinimum
			< APSStellarMaterialTests::HISMStaticJewelPulseMaximum
		&& APSStellarMaterialTests::HISMStaticJewelPulseMaximum < 1.0f);
	const float ActorUseInstance = APSStellarMaterialTests::StellarUseInstance(
		0.0f, 0.0f, 0.0f, 0.0f);
	const float DarkHISMUseInstance = APSStellarMaterialTests::StellarUseInstance(
		0.0f, 6.0f, 0.0f, 0.0f);
	TestTrue(TEXT("Zero custom payload keeps a materialized actor on actor parameters"),
		FMath::IsNearlyZero(ActorUseInstance));
	TestTrue(TEXT("Near-black HISM still selects instance parameters through emission payload"),
		FMath::IsNearlyEqual(DarkHISMUseInstance, 1.0f));

	const float MinimumEmissionActorWithMaximumJewelResponse =
		APSStellarMaterialTests::ActorTone(100.0f)
		+ APSStellarMaterialTests::JewelLiftMaximum
		+ APSStellarMaterialTests::RimJewelLiftMaximum;
	TestTrue(*FString::Printf(
		TEXT("Sparse jewel response gives the minimum actor HDR headroom (%.4f > 1.0)"),
		MinimumEmissionActorWithMaximumJewelResponse),
		MinimumEmissionActorWithMaximumJewelResponse > 1.0f);
	TestTrue(TEXT("Combined jewel lift stays below a bounded 42-percent local increase"),
		APSStellarMaterialTests::JewelLiftMinimum > 0.0f
		&& APSStellarMaterialTests::JewelLiftMinimum
			< APSStellarMaterialTests::JewelLiftMaximum
		&& APSStellarMaterialTests::RimJewelLiftMinimum > 0.0f
		&& APSStellarMaterialTests::RimJewelLiftMinimum
			< APSStellarMaterialTests::RimJewelLiftMaximum
		&& APSStellarMaterialTests::JewelLiftMaximum
			+ APSStellarMaterialTests::RimJewelLiftMaximum < 0.42f);
	const float MaximumResolvedHISMJewelLift = ResolvedHISMSpatialDetail
		* (APSStellarMaterialTests::JewelLiftMaximum
			+ APSStellarMaterialTests::RimJewelLiftMaximum);
	const float MaximumSubpixelHISMJewelLift = SubpixelHISMSpatialDetail
		* (APSStellarMaterialTests::JewelLiftMaximum
			+ APSStellarMaterialTests::RimJewelLiftMaximum);
	TestTrue(*FString::Printf(
		TEXT("Resolved HISM receives nonzero static jewel headroom (%.4f)"),
		MaximumResolvedHISMJewelLift),
		MaximumResolvedHISMJewelLift > 0.0f);
	TestTrue(TEXT("Sub-pixel HISM jewel lift collapses to a stable spectral point"),
		FMath::IsNearlyZero(MaximumSubpixelHISMJewelLift));

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

	const float LShaderBrightness = APSStellarMaterialTests::ShaderSpectralPeak(LStar);
	const float TShaderBrightness = APSStellarMaterialTests::ShaderSpectralPeak(TStar);
	const float YShaderBrightness = APSStellarMaterialTests::ShaderSpectralPeak(YStar);
	const float BlackHoleShaderBrightness =
		APSStellarMaterialTests::ShaderSpectralPeak(BlackHole);
	TestTrue(*FString::Printf(
		TEXT("Shader spectral response is ordered L %.3f > T %.3f > Y %.3f > BH %.4f"),
		LShaderBrightness, TShaderBrightness, YShaderBrightness,
		BlackHoleShaderBrightness),
		LShaderBrightness > TShaderBrightness && TShaderBrightness > YShaderBrightness
		&& YShaderBrightness > BlackHoleShaderBrightness);
	TestTrue(*FString::Printf(
		TEXT("Shader validity mask keeps black-hole output at zero (peak %.4f)"),
		BlackHoleShaderBrightness), FMath::IsNearlyZero(BlackHoleShaderBrightness));

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
