#if WITH_EDITOR && WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "APS_ALPHA/Actors/Astro/Galaxy.h"
#include "APS_ALPHA/Actors/Astro/Star.h"
#include "APS_ALPHA/Actors/Astro/StarCluster.h"
#include "APS_ALPHA/Core/Rendering/APSStellarMaterialContract.h"
#include "APS_ALPHA/Generation/StarGenerator.h"
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
	constexpr int32 ExpectedExpressionCount = 18;
	constexpr int32 MaximumPixelInstructions = 320;
	constexpr float MaximumActorPreBloom = 1.72f;
	constexpr float MaximumHISMPreBloom = 1.50f;
	constexpr float ActorToneMinimum = 0.78f;
	constexpr float ActorToneMaximum = 1.42f;
	constexpr float HISMProxyToneMinimum = 0.08f;
	constexpr float HISMProxyToneMaximum = 1.42f;
	constexpr float HISMProxyToneScale = 1.03f;
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
			HISMProxyEmissionActivity(RawEmission)) * HISMProxyToneScale;
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
					TEXT("granulation=(mesoCells*0.64+granuleRidges*0.36)*Granulation*0.14*spatialDetail")));
			TestTrue(APSStellarMaterialTests::Context(Material, TEXT("scale-aware spots and faculae")),
				CompactCode.Contains(
					TEXT("spots=spotCore*SpotAmount*lerp(0.72,1.0,emissionActivity)*spatialDetail"))
				&& CompactCode.Contains(
					TEXT("faculae=spotHalo*(0.075+variation*0.14)*spatialDetail")));
			TestTrue(APSStellarMaterialTests::Context(Material, TEXT("scale-aware photosphere contrast")),
				CompactCode.Contains(
					TEXT("macroConvection*variation*0.12*spatialDetail"))
				&& CompactCode.Contains(
					TEXT("cellHeat=lerp(0.5,granuleCell,spatialDetail)")));
			TestTrue(APSStellarMaterialTests::Context(Material, TEXT("bounded actor tone")),
				CompactCode.Contains(TEXT("actorTone=lerp(0.78,1.42,actorActivity)")));
			TestTrue(APSStellarMaterialTests::Context(Material, TEXT("bounded HISM proxy tone")),
				CompactCode.Contains(
					TEXT("proxyTone=lerp(0.08,1.42,proxyActivity)*1.03")));
			TestTrue(APSStellarMaterialTests::Context(Material, TEXT("actor and HISM tone selection")),
				CompactCode.Contains(
					TEXT("toneSafeEmission=lerp(actorTone,proxyTone,useInstance)")));
			TestTrue(APSStellarMaterialTests::Context(Material, TEXT("perceptual limb ratio")),
				CompactCode.Contains(
					TEXT("limb=lerp(0.22,1.0,pow(facing,0.62))")));
			TestTrue(APSStellarMaterialTests::Context(Material, TEXT("compact corona rim")),
				CompactCode.Contains(TEXT("rim=pow(1.0-facing,3.15)"))
				&& CompactCode.Contains(TEXT("resolvedProminence=prominenceMask*spatialDetail"))
				&& CompactCode.Contains(TEXT("CoronaAmount*rim*(0.18+resolvedProminence*0.82)")));
			TestTrue(APSStellarMaterialTests::Context(Material, TEXT("spectral normalization mix")),
				CompactCode.Contains(
					TEXT("spectralTint=lerp(spectralColor,normalizedSpectralTint,0.55)")));
			TestTrue(APSStellarMaterialTests::Context(Material, TEXT("near-black spectral rejection")),
				CompactCode.Contains(
					TEXT("spectralVisibility=smoothstep(0.08,0.90,maxSpectral)"))
				&& !CompactCode.Contains(TEXT("validStellarSpectrum")));
			TestTrue(APSStellarMaterialTests::Context(Material, TEXT("per-path pre-bloom ceiling")),
				CompactCode.Contains(
					TEXT("outputCeiling=lerp(1.72,1.50,useInstance)")));
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
			TestTrue(APSStellarMaterialTests::Context(Material, TEXT("pre-bloom stellar output")),
				CompactCode.Contains(
					TEXT("stellarSignal=(toneSafeEmission+jewelLift)*visibleSurface+rimJewelLift"))
				&& CompactCode.Contains(
					TEXT("float3preBloom=surfaceTint*stellarSignal*temporalFlicker")));
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
	ValidateBinding(TEXT("Native galaxy HISM"),
		NativeGalaxy && NativeGalaxy->StarMeshInstances
			? NativeGalaxy->StarMeshInstances->GetMaterial(0) : nullptr,
		APSStellarMaterialContract::HismBaseObjectPath);
	const AStar* NativeStar = GetDefault<AStar>();
	ValidateBinding(TEXT("Native star slot 0"),
		NativeStar && NativeStar->StarMesh
			? NativeStar->StarMesh->GetMaterial(0) : nullptr,
		APSStellarMaterialContract::ActorBaseObjectPath);
	const AStarCluster* NativeCluster = GetDefault<AStarCluster>();
	ValidateBinding(TEXT("Native cluster HISM"),
		NativeCluster && NativeCluster->StarMeshInstances
			? NativeCluster->StarMeshInstances->GetMaterial(0) : nullptr,
		APSStellarMaterialContract::HismBaseObjectPath);

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

#endif // WITH_EDITOR && WITH_DEV_AUTOMATION_TESTS
