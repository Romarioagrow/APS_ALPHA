#include "APSPlanetSurfaceAssetCommandlet.h"

#if WITH_EDITOR

#include "AssetToolsModule.h"
#include "Factories/DataAssetFactory.h"
#include "Factories/MaterialFactoryNew.h"
#include "Factories/MaterialInstanceConstantFactoryNew.h"
#include "IAssetTools.h"
#include "Materials/Material.h"
#include "Materials/MaterialExpressionClamp.h"
#include "Materials/MaterialExpressionFresnel.h"
#include "Materials/MaterialExpressionLinearInterpolate.h"
#include "Materials/MaterialExpressionMultiply.h"
#include "Materials/MaterialExpressionMaterialAttributeLayers.h"
#include "Materials/MaterialExpressionMaterialFunctionCall.h"
#include "Materials/MaterialExpressionScalarParameter.h"
#include "Materials/MaterialExpressionSmoothStep.h"
#include "Materials/MaterialExpressionVectorParameter.h"
#include "Materials/MaterialExpressionVertexColor.h"
#include "Materials/MaterialFunctionInterface.h"
#include "Materials/MaterialInstance.h"
#include "Materials/MaterialInstanceConstant.h"
#include "Materials/MaterialInterface.h"
#include "MaterialDomain.h"
#include "MaterialEditingLibrary.h"
#include "Misc/PackageName.h"
#include "Modules/ModuleManager.h"
#include "UObject/Package.h"
#include "UObject/SavePackage.h"
#include "APS_ALPHA/Core/Planetary/APSPlanetSurfaceProfile.h"

namespace APSPlanetSurfaceAssets
{
	enum class ELiquidMaterialStyle : uint8
	{
		WaterBaseline,
		Ammonia,
		Lava
	};

	const FString RootPath = TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface");
	const FString MaterialPath = RootPath / TEXT("Materials");
	const FString PreviewMaterialPath = RootPath / TEXT("Preview");

	bool SaveAsset(UObject* Asset)
	{
		if (!IsValid(Asset)) return false;
		UPackage* Package = Asset->GetOutermost();
		Package->MarkPackageDirty();
		const FString Filename = FPackageName::LongPackageNameToFilename(
			Package->GetName(), FPackageName::GetAssetPackageExtension());
		FSavePackageArgs Args;
		Args.TopLevelFlags = RF_Public | RF_Standalone;
		Args.SaveFlags = SAVE_NoError;
		Args.Error = GError;
		return UPackage::SavePackage(Package, Asset, *Filename, Args);
	}

	template <typename TObject>
	TObject* LoadAsset(const FString& PackagePath, const FString& AssetName)
	{
		return LoadObject<TObject>(nullptr, *(PackagePath / AssetName + TEXT(".") + AssetName));
	}

	UObject* LoadOrDuplicate(IAssetTools& AssetTools, const FString& Name, const FString& Path, UObject* Source)
	{
		if (UObject* Existing = StaticLoadObject(UObject::StaticClass(), nullptr, *(Path / Name + TEXT(".") + Name)))
		{
			return Existing;
		}
		return IsValid(Source) ? AssetTools.DuplicateAsset(Name, Path, Source) : nullptr;
	}

	UMaterial* LoadOrCreateMaterial(IAssetTools& AssetTools, const FString& Name)
	{
		if (UMaterial* Existing = LoadAsset<UMaterial>(PreviewMaterialPath, Name))
		{
			return Existing;
		}

		UMaterialFactoryNew* Factory = NewObject<UMaterialFactoryNew>();
		return Cast<UMaterial>(AssetTools.CreateAsset(
			Name, PreviewMaterialPath, UMaterial::StaticClass(), Factory));
	}

	template <typename TExpression>
	TExpression* AddExpression(UMaterial* Material, int32 X, int32 Y)
	{
		return Cast<TExpression>(UMaterialEditingLibrary::CreateMaterialExpression(
			Material, TExpression::StaticClass(), X, Y));
	}

	UMaterialExpressionVectorParameter* AddVectorParameter(
		UMaterial* Material, const TCHAR* Name, const FLinearColor& DefaultValue,
		int32 X, int32 Y, int32 SortPriority)
	{
		UMaterialExpressionVectorParameter* Parameter =
			AddExpression<UMaterialExpressionVectorParameter>(Material, X, Y);
		if (!IsValid(Parameter)) return nullptr;
		Parameter->ParameterName = Name;
		Parameter->DefaultValue = DefaultValue;
		Parameter->Group = TEXT("APS Orbital Preview");
		Parameter->SortPriority = SortPriority;
		Parameter->UpdateParameterGuid(true, true);
		return Parameter;
	}

	UMaterialExpressionScalarParameter* AddScalarParameter(
		UMaterial* Material, const TCHAR* Name, float DefaultValue,
		float SliderMinimum, float SliderMaximum, int32 X, int32 Y, int32 SortPriority)
	{
		UMaterialExpressionScalarParameter* Parameter =
			AddExpression<UMaterialExpressionScalarParameter>(Material, X, Y);
		if (!IsValid(Parameter)) return nullptr;
		Parameter->ParameterName = Name;
		Parameter->DefaultValue = DefaultValue;
		Parameter->SliderMin = SliderMinimum;
		Parameter->SliderMax = SliderMaximum;
		Parameter->Group = TEXT("APS Orbital Preview");
		Parameter->SortPriority = SortPriority;
		Parameter->UpdateParameterGuid(true, true);
		return Parameter;
	}

	UMaterialExpressionLinearInterpolate* AddLerp(
		UMaterial* Material, UMaterialExpression* A, UMaterialExpression* B,
		UMaterialExpression* Alpha, int32 AlphaOutputIndex, int32 X, int32 Y)
	{
		UMaterialExpressionLinearInterpolate* Lerp =
			AddExpression<UMaterialExpressionLinearInterpolate>(Material, X, Y);
		if (!IsValid(Lerp) || !IsValid(A) || !IsValid(B) || !IsValid(Alpha)) return nullptr;
		Lerp->A.Connect(0, A);
		Lerp->B.Connect(0, B);
		Lerp->Alpha.Connect(AlphaOutputIndex, Alpha);
		return Lerp;
	}

	UMaterialExpressionSmoothStep* AddSmoothStep(
		UMaterial* Material, UMaterialExpression* Value, int32 ValueOutputIndex,
		float Minimum, float Maximum, int32 X, int32 Y)
	{
		UMaterialExpressionSmoothStep* SmoothStep =
			AddExpression<UMaterialExpressionSmoothStep>(Material, X, Y);
		if (!IsValid(SmoothStep) || !IsValid(Value) || Minimum >= Maximum)
		{
			return nullptr;
		}
		SmoothStep->Value.Connect(ValueOutputIndex, Value);
		SmoothStep->ConstMin = Minimum;
		SmoothStep->ConstMax = Maximum;
		return SmoothStep;
	}

	bool FinalizePreviewMaterial(UMaterial* Material)
	{
		if (!IsValid(Material)) return false;
		Material->UpdateCachedExpressionData();
		Material->PostEditChange();
		UMaterialEditingLibrary::RecompileMaterial(Material);
		return SaveAsset(Material);
	}

	UMaterial* CreateOrbitalTerrainMaterial(IAssetTools& AssetTools)
	{
		UMaterial* Material = LoadOrCreateMaterial(AssetTools, TEXT("M_APS_OrbitalTerrain"));
		if (!IsValid(Material)) return nullptr;
		Material->Modify();
		UMaterialEditingLibrary::DeleteAllMaterialExpressions(Material);
		Material->MaterialDomain = MD_Surface;
		Material->BlendMode = BLEND_Opaque;
		Material->SetShadingModel(MSM_DefaultLit);
		Material->TwoSided = false;

		UMaterialExpressionVertexColor* Vertex =
			AddExpression<UMaterialExpressionVertexColor>(Material, -1500, 0);
		UMaterialExpressionVectorParameter* Coast = AddVectorParameter(
			Material, TEXT("BottomColor"), FLinearColor(0.08f, 0.10f, 0.12f), -1500, -700, 0);
		UMaterialExpressionVectorParameter* Lowland = AddVectorParameter(
			Material, TEXT("Color1"), FLinearColor(0.16f, 0.20f, 0.14f), -1500, -600, 1);
		UMaterialExpressionVectorParameter* MidLowland = AddVectorParameter(
			Material, TEXT("Color2"), FLinearColor(0.28f, 0.30f, 0.20f), -1500, -500, 2);
		UMaterialExpressionVectorParameter* Highland = AddVectorParameter(
			Material, TEXT("Color3"), FLinearColor(0.38f, 0.36f, 0.28f), -1500, -400, 3);
		UMaterialExpressionVectorParameter* Dryland = AddVectorParameter(
			Material, TEXT("Color4"), FLinearColor(0.48f, 0.40f, 0.27f), -1500, -300, 4);
		UMaterialExpressionVectorParameter* Peak = AddVectorParameter(
			Material, TEXT("Color5"), FLinearColor(0.72f, 0.72f, 0.68f), -1500, -200, 5);
		UMaterialExpressionVectorParameter* Emissive = AddVectorParameter(
			Material, TEXT("EmissiveColor"), FLinearColor::Black, -800, 650, 6);
		UMaterialExpressionScalarParameter* ClimateBlend = AddScalarParameter(
			Material, TEXT("ClimateBlend"), 0.14f, 0.08f, 0.25f, -500, 300, 10);
		UMaterialExpressionScalarParameter* Roughness = AddScalarParameter(
			Material, TEXT("Roughness"), 0.72f, 0.0f, 1.0f, 200, 300, 11);
		UMaterialExpressionScalarParameter* Metallic = AddScalarParameter(
			Material, TEXT("Metallic"), 0.04f, 0.0f, 1.0f, 200, 400, 12);
		UMaterialExpressionScalarParameter* Specular = AddScalarParameter(
			Material, TEXT("Specular"), 0.35f, 0.0f, 1.0f, 200, 500, 13);
		if (!Vertex || !Coast || !Lowland || !MidLowland || !Highland || !Dryland || !Peak
			|| !Emissive || !ClimateBlend || !Roughness || !Metallic || !Specular)
		{
			return nullptr;
		}

		// Height is WorldScape's normalized vertex-colour R channel. Give every
		// authored palette colour its own bounded elevation transition. Reusing raw
		// height as the alpha of several nested lerps produces a polynomial average
		// that washes distinct presets into the same pale orbital globe.
		UMaterialExpressionSmoothStep* CoastToLowBand = AddSmoothStep(
			Material, Vertex, 1, 0.08f, 0.20f, -1180, -680);
		UMaterialExpressionSmoothStep* LowToMidBand = AddSmoothStep(
			Material, Vertex, 1, 0.24f, 0.38f, -1180, -540);
		UMaterialExpressionSmoothStep* MidToHighBand = AddSmoothStep(
			Material, Vertex, 1, 0.42f, 0.56f, -1180, -400);
		UMaterialExpressionSmoothStep* HighToDryBand = AddSmoothStep(
			Material, Vertex, 1, 0.60f, 0.72f, -1180, -260);
		UMaterialExpressionSmoothStep* DryToPeakBand = AddSmoothStep(
			Material, Vertex, 1, 0.78f, 0.90f, -1180, -120);
		UMaterialExpressionLinearInterpolate* CoastLow = AddLerp(
			Material, Coast, Lowland, CoastToLowBand, 0, -900, -660);
		UMaterialExpressionLinearInterpolate* LowMid = AddLerp(
			Material, CoastLow, MidLowland, LowToMidBand, 0, -700, -520);
		UMaterialExpressionLinearInterpolate* MidHigh = AddLerp(
			Material, LowMid, Highland, MidToHighBand, 0, -500, -400);
		UMaterialExpressionLinearInterpolate* HighDry = AddLerp(
			Material, MidHigh, Dryland, HighToDryBand, 0, -300, -280);
		UMaterialExpressionLinearInterpolate* HeightRamp = AddLerp(
			Material, HighDry, Peak, DryToPeakBand, 0, -100, -400);

		// WorldScape writes temperature and humidity to G/B. They provide a subtle
		// climate tint while height remains the dominant orbital-scale signal.
		UMaterialExpressionLinearInterpolate* Thermal = AddLerp(
			Material, MidLowland, Dryland, Vertex, 2, -650, -100);
		UMaterialExpressionLinearInterpolate* Climate = AddLerp(
			Material, Thermal, Coast, Vertex, 3, -350, -80);
		UMaterialExpressionLinearInterpolate* FinalColor = AddLerp(
			Material, HeightRamp, Climate, ClimateBlend, 0, 0, -260);

		UMaterialExpressionClamp* BoundedEmissive =
			AddExpression<UMaterialExpressionClamp>(Material, -300, 650);
		UMaterialExpressionMultiply* MaskedEmissive =
			AddExpression<UMaterialExpressionMultiply>(Material, 0, 650);
		if (!CoastToLowBand || !LowToMidBand || !MidToHighBand || !HighToDryBand
			|| !DryToPeakBand || !CoastLow || !LowMid || !MidHigh || !HighDry || !HeightRamp || !Thermal
			|| !Climate || !FinalColor || !BoundedEmissive || !MaskedEmissive)
		{
			return nullptr;
		}
		BoundedEmissive->Input.Connect(0, Emissive);
		BoundedEmissive->ClampMode = CMODE_Clamp;
		BoundedEmissive->MinDefault = 0.0f;
		BoundedEmissive->MaxDefault = 1.25f;
		MaskedEmissive->A.Connect(0, BoundedEmissive);
		MaskedEmissive->B.Connect(4, Vertex);

		const bool bConnected =
			UMaterialEditingLibrary::ConnectMaterialProperty(FinalColor, TEXT(""), MP_BaseColor)
			&& UMaterialEditingLibrary::ConnectMaterialProperty(MaskedEmissive, TEXT(""), MP_EmissiveColor)
			&& UMaterialEditingLibrary::ConnectMaterialProperty(Roughness, TEXT(""), MP_Roughness)
			&& UMaterialEditingLibrary::ConnectMaterialProperty(Metallic, TEXT(""), MP_Metallic)
			&& UMaterialEditingLibrary::ConnectMaterialProperty(Specular, TEXT(""), MP_Specular);
		return bConnected && FinalizePreviewMaterial(Material) ? Material : nullptr;
	}

	UMaterial* CreateOrbitalLiquidMaterial(IAssetTools& AssetTools)
	{
		UMaterial* Material = LoadOrCreateMaterial(AssetTools, TEXT("M_APS_OrbitalLiquid"));
		if (!IsValid(Material)) return nullptr;
		Material->Modify();
		UMaterialEditingLibrary::DeleteAllMaterialExpressions(Material);
		Material->MaterialDomain = MD_Surface;
		Material->BlendMode = BLEND_Translucent;
		Material->SetShadingModel(MSM_DefaultLit);
		Material->TranslucencyLightingMode = TLM_SurfacePerPixelLighting;
		Material->TwoSided = false;
		Material->bScreenSpaceReflections = true;

		UMaterialExpressionVectorParameter* Deep = AddVectorParameter(
			Material, TEXT("LiquidDeepColor"), FLinearColor(0.004f, 0.018f, 0.065f), -900, -350, 0);
		UMaterialExpressionVectorParameter* Shallow = AddVectorParameter(
			Material, TEXT("LiquidShallowColor"), FLinearColor(0.03f, 0.25f, 0.52f), -900, -230, 1);
		UMaterialExpressionVectorParameter* Emissive = AddVectorParameter(
			Material, TEXT("LiquidEmissiveColor"), FLinearColor(0.002f, 0.008f, 0.02f), -900, 100, 2);
		UMaterialExpressionVertexColor* Vertex =
			AddExpression<UMaterialExpressionVertexColor>(Material, -900, 250);
		UMaterialExpressionScalarParameter* Opacity = AddScalarParameter(
			Material, TEXT("Opacity"), 0.58f, 0.05f, 0.9f, -250, 300, 10);
		UMaterialExpressionScalarParameter* Roughness = AddScalarParameter(
			Material, TEXT("Roughness"), 0.18f, 0.0f, 1.0f, 50, 300, 11);
		UMaterialExpressionScalarParameter* Metallic = AddScalarParameter(
			Material, TEXT("Metallic"), 0.0f, 0.0f, 1.0f, 50, 400, 12);
		UMaterialExpressionScalarParameter* Specular = AddScalarParameter(
			Material, TEXT("Specular"), 0.65f, 0.0f, 1.0f, 50, 500, 13);
		UMaterialExpressionFresnel* Fresnel =
			AddExpression<UMaterialExpressionFresnel>(Material, -600, -80);
		if (!Deep || !Shallow || !Emissive || !Vertex || !Opacity || !Roughness || !Metallic
			|| !Specular || !Fresnel)
		{
			return nullptr;
		}
		Fresnel->Exponent = 4.0f;
		Fresnel->BaseReflectFraction = 0.04f;
		UMaterialExpressionLinearInterpolate* LiquidColor = AddLerp(
			Material, Deep, Shallow, Fresnel, 0, -300, -260);
		UMaterialExpressionClamp* BoundedEmissive =
			AddExpression<UMaterialExpressionClamp>(Material, -300, 100);
		UMaterialExpressionMultiply* MaskedEmissive =
			AddExpression<UMaterialExpressionMultiply>(Material, 0, 100);
		UMaterialExpressionMultiply* MaskedOpacity =
			AddExpression<UMaterialExpressionMultiply>(Material, 0, 300);
		if (!LiquidColor || !BoundedEmissive || !MaskedEmissive || !MaskedOpacity)
		{
			return nullptr;
		}
		BoundedEmissive->Input.Connect(0, Emissive);
		BoundedEmissive->ClampMode = CMODE_Clamp;
		BoundedEmissive->MinDefault = 0.0f;
		BoundedEmissive->MaxDefault = 1.25f;
		MaskedEmissive->A.Connect(0, BoundedEmissive);
		MaskedEmissive->B.Connect(4, Vertex);
		MaskedOpacity->A.Connect(0, Opacity);
		MaskedOpacity->B.Connect(4, Vertex);

		const bool bConnected =
			UMaterialEditingLibrary::ConnectMaterialProperty(LiquidColor, TEXT(""), MP_BaseColor)
			&& UMaterialEditingLibrary::ConnectMaterialProperty(MaskedEmissive, TEXT(""), MP_EmissiveColor)
			&& UMaterialEditingLibrary::ConnectMaterialProperty(MaskedOpacity, TEXT(""), MP_Opacity)
			&& UMaterialEditingLibrary::ConnectMaterialProperty(Roughness, TEXT(""), MP_Roughness)
			&& UMaterialEditingLibrary::ConnectMaterialProperty(Metallic, TEXT(""), MP_Metallic)
			&& UMaterialEditingLibrary::ConnectMaterialProperty(Specular, TEXT(""), MP_Specular);
		return bConnected && FinalizePreviewMaterial(Material) ? Material : nullptr;
	}

	UMaterialInstanceConstant* CreateOrbitalLiquidPreset(
		IAssetTools& AssetTools, const FString& Name, UMaterial* Parent,
		const FLinearColor& DeepColor, const FLinearColor& ShallowColor,
		const FLinearColor& EmissiveColor, float Opacity, float Roughness,
		float Metallic, float Specular)
	{
		UMaterialInstanceConstant* Result =
			LoadAsset<UMaterialInstanceConstant>(PreviewMaterialPath, Name);
		if (!IsValid(Result))
		{
			UMaterialInstanceConstantFactoryNew* Factory =
				NewObject<UMaterialInstanceConstantFactoryNew>();
			Factory->InitialParent = Parent;
			Result = Cast<UMaterialInstanceConstant>(AssetTools.CreateAsset(
				Name, PreviewMaterialPath, UMaterialInstanceConstant::StaticClass(), Factory));
		}
		if (!IsValid(Result) || !IsValid(Parent)) return nullptr;
		Result->SetParentEditorOnly(Parent);
		Result->ClearParameterValuesEditorOnly();
		Result->SetVectorParameterValueEditorOnly(
			FMaterialParameterInfo(TEXT("LiquidDeepColor")), DeepColor);
		Result->SetVectorParameterValueEditorOnly(
			FMaterialParameterInfo(TEXT("LiquidShallowColor")), ShallowColor);
		Result->SetVectorParameterValueEditorOnly(
			FMaterialParameterInfo(TEXT("LiquidEmissiveColor")), EmissiveColor);
		Result->SetScalarParameterValueEditorOnly(FMaterialParameterInfo(TEXT("Opacity")), Opacity);
		Result->SetScalarParameterValueEditorOnly(FMaterialParameterInfo(TEXT("Roughness")), Roughness);
		Result->SetScalarParameterValueEditorOnly(FMaterialParameterInfo(TEXT("Metallic")), Metallic);
		Result->SetScalarParameterValueEditorOnly(FMaterialParameterInfo(TEXT("Specular")), Specular);
		Result->PostEditChange();
		return SaveAsset(Result) ? Result : nullptr;
	}

	void ApplyDefinition(UMaterialInstanceConstant* Material, const FAPSPlanetSurfaceArchetypeDefinition& D)
	{
		if (!IsValid(Material)) return;
		auto Vector = [Material](const TCHAR* Name, const FLinearColor& Value)
		{
			Material->SetVectorParameterValueEditorOnly(FMaterialParameterInfo(Name), Value);
		};
		auto Scalar = [Material](const TCHAR* Name, float Value)
		{
			Material->SetScalarParameterValueEditorOnly(FMaterialParameterInfo(Name), Value);
		};

		Vector(TEXT("BottomColor"), D.Palette.Coast);
		Vector(TEXT("Color1"), D.Palette.Lowland);
		Vector(TEXT("Color2"), D.Palette.MidLowland);
		Vector(TEXT("Color3"), D.Palette.Highland);
		Vector(TEXT("Color4"), D.Palette.Dryland);
		Vector(TEXT("Color5"), D.Palette.Peak);
		Vector(TEXT("SlopeColor"), D.Palette.Slope);
		Vector(TEXT("Sedimentcolor"), D.Palette.Coast);
		Vector(TEXT("EmissiveColor"), D.Palette.Emissive);
		Scalar(TEXT("HeightContrast"), static_cast<float>((D.BiomeContrast.X + D.BiomeContrast.Y) * 0.5));
		Scalar(TEXT("ContrastTemp"), static_cast<float>((D.BiomeContrast.X + D.BiomeContrast.Y) * 0.5));
		Scalar(TEXT("WarpedScale"), static_cast<float>((D.NoiseScale.X + D.NoiseScale.Y) / 1300.0));
		Scalar(TEXT("MidVarient1Rough"), static_cast<float>((D.Roughness.X + D.Roughness.Y) * 0.5));
		Scalar(TEXT("MidVarient2Rough"), static_cast<float>((D.Roughness.X + D.Roughness.Y) * 0.5));
		Scalar(TEXT("MidVarient3Rough"), static_cast<float>((D.Roughness.X + D.Roughness.Y) * 0.5));
		Material->PostEditChange();
	}

	UMaterialInstanceConstant* CreateFamilyMaterial(
		IAssetTools& AssetTools, const FString& Name, UMaterialInstanceConstant* Template,
		const FAPSPlanetSurfaceArchetypeDefinition& Definition)
	{
		UMaterialInstanceConstant* Result = Cast<UMaterialInstanceConstant>(
			LoadOrDuplicate(AssetTools, Name, MaterialPath, Template));
		if (!IsValid(Result)) return nullptr;
		// Keep the complete, battle-tested WorldScape material graph through the
		// family template. Duplicating a UMaterial from commandlet/editor data can
		// leave an empty graph in cooked project assets, making every planet render
		// with the same default surface despite valid profile overrides.
		Result->SetParentEditorOnly(Template);
		ApplyDefinition(Result, Definition);
		if (!SaveAsset(Result)) return nullptr;
		return Result;
	}

	void CollectLiquidParameterNames(
		TConstArrayView<TObjectPtr<UMaterialExpression>> Expressions,
		TSet<FName>& VectorNames,
		TSet<FName>& ScalarNames,
		TSet<const UMaterialFunctionInterface*>& VisitedFunctions)
	{
		for (UMaterialExpression* Expression : Expressions)
		{
			if (const UMaterialExpressionVectorParameter* VectorParameter =
				Cast<UMaterialExpressionVectorParameter>(Expression))
			{
				VectorNames.Add(VectorParameter->ParameterName);
			}
			else if (const UMaterialExpressionScalarParameter* ScalarParameter =
				Cast<UMaterialExpressionScalarParameter>(Expression))
			{
				ScalarNames.Add(ScalarParameter->ParameterName);
			}

			if (const UMaterialExpressionMaterialFunctionCall* FunctionCall =
				Cast<UMaterialExpressionMaterialFunctionCall>(Expression))
			{
				UMaterialFunctionInterface* Function = FunctionCall->MaterialFunction;
				if (IsValid(Function) && !VisitedFunctions.Contains(Function))
				{
					VisitedFunctions.Add(Function);
					Function->ConditionalPostLoad();
					CollectLiquidParameterNames(
						Function->GetExpressions(), VectorNames, ScalarNames, VisitedFunctions);
				}
			}
			else if (const UMaterialExpressionMaterialAttributeLayers* Layers =
				Cast<UMaterialExpressionMaterialAttributeLayers>(Expression))
			{
				auto CollectFunction = [&VectorNames, &ScalarNames, &VisitedFunctions](
					UMaterialFunctionInterface* Function)
				{
					if (!IsValid(Function) || VisitedFunctions.Contains(Function)) return;
					VisitedFunctions.Add(Function);
					Function->ConditionalPostLoad();
					CollectLiquidParameterNames(
						Function->GetExpressions(), VectorNames, ScalarNames, VisitedFunctions);
				};
				for (UMaterialFunctionInterface* Layer : Layers->GetLayers()) CollectFunction(Layer);
				for (UMaterialFunctionInterface* Blend : Layers->GetBlends()) CollectFunction(Blend);
			}
		}
	}

	void CollectInstanceParameterNames(
		UMaterialInterface* Material,
		TSet<FName>& VectorNames,
		TSet<FName>& ScalarNames)
	{
		TSet<const UMaterialInterface*> VisitedMaterials;
		while (UMaterialInstance* Instance = Cast<UMaterialInstance>(Material))
		{
			if (VisitedMaterials.Contains(Instance)) return;
			VisitedMaterials.Add(Instance);
			Instance->ConditionalPostLoad();
			for (const FVectorParameterValue& Parameter : Instance->VectorParameterValues)
			{
				VectorNames.Add(Parameter.ParameterInfo.Name);
			}
			for (const FScalarParameterValue& Parameter : Instance->ScalarParameterValues)
			{
				ScalarNames.Add(Parameter.ParameterInfo.Name);
			}
			Material = Instance->Parent;
		}
	}

	UMaterialInstanceConstant* CreateLiquidMaterial(
		IAssetTools& AssetTools, const FString& Name, UMaterialInstanceConstant* Template,
		ELiquidMaterialStyle Style)
	{
		UMaterialInstanceConstant* Result = Cast<UMaterialInstanceConstant>(
			LoadOrDuplicate(AssetTools, Name, MaterialPath, Template));
		if (!IsValid(Result)) return nullptr;
		Result->SetParentEditorOnly(Template);
		// Remove stale overrides left by older commandlet runs. In particular,
		// ShallowColor/DeepColor never existed in the WorldScape parent graphs and
		// gave a false impression that liquid presets were being authored.
		Result->ClearParameterValuesEditorOnly();

		Template->ConditionalPostLoad();
		UMaterial* BaseMaterial = Template->GetMaterial();
		if (!IsValid(BaseMaterial))
		{
			UE_LOG(LogTemp, Error,
				TEXT("[APS.PlanetSurfaceAssets] %s parent %s has no base material"),
				*Result->GetName(), *GetNameSafe(Template));
			return nullptr;
		}
		BaseMaterial->ConditionalPostLoad();
		// Custom commandlets do not necessarily build editor expression metadata while
		// loading marketplace materials. Relink function calls and rebuild the cache
		// before querying inherited parameters, matching the normal material editor.
		BaseMaterial->UpdateTransientExpressionData();
		BaseMaterial->UpdateCachedExpressionData();

		TArray<FMaterialParameterInfo> VectorInfos;
		TArray<FGuid> VectorIds;
		TArray<FMaterialParameterInfo> ScalarInfos;
		TArray<FGuid> ScalarIds;
		// A minimal custom commandlet can load a MIC before its inheritance-chain
		// cache is populated. Query the final UMaterial, then supplement the cache
		// from actual parameter expression nodes across nested material functions.
		// The fallback validates authored graph parameters; it never treats arbitrary
		// requested names as valid.
		BaseMaterial->GetAllVectorParameterInfo(VectorInfos, VectorIds);
		BaseMaterial->GetAllScalarParameterInfo(ScalarInfos, ScalarIds);
		TSet<FName> VectorNames;
		TSet<FName> ScalarNames;
		for (const FMaterialParameterInfo& Info : VectorInfos) VectorNames.Add(Info.Name);
		for (const FMaterialParameterInfo& Info : ScalarInfos) ScalarNames.Add(Info.Name);
		// Use the same public editor-library query path exercised by Python and the
		// material editor. Some UE 5.4 marketplace materials expose complete cached
		// parameter metadata through their MIC while a direct query of the terminal
		// UMaterial omits one inherited custom-node parameter (lava Brightness).
		// Query both assets and still validate every authored override by name.
		auto CollectEditorLibraryNames = [&VectorNames, &ScalarNames](UMaterialInterface* Material)
		{
			TArray<FName> Names;
			UMaterialEditingLibrary::GetVectorParameterNames(Material, Names);
			for (const FName Name : Names) VectorNames.Add(Name);
			UMaterialEditingLibrary::GetScalarParameterNames(Material, Names);
			for (const FName Name : Names) ScalarNames.Add(Name);
		};
		CollectEditorLibraryNames(BaseMaterial);
		CollectEditorLibraryNames(Template);
		// Established WorldScape MICs retain authored override records even when the
		// minimal commandlet has not built the base material's expression cache yet.
		// These are serialized real parameter records, so they are a safe headless
		// source and cannot legitimize arbitrary/fake requested names.
		CollectInstanceParameterNames(Template, VectorNames, ScalarNames);
		TArray<UMaterialExpressionVectorParameter*> VectorExpressions;
		TArray<UMaterialExpressionScalarParameter*> ScalarExpressions;
		BaseMaterial->GetAllExpressionsInMaterialAndFunctionsOfType(VectorExpressions);
		BaseMaterial->GetAllExpressionsInMaterialAndFunctionsOfType(ScalarExpressions);
		for (const UMaterialExpressionVectorParameter* Expression : VectorExpressions)
		{
			if (IsValid(Expression)) VectorNames.Add(Expression->ParameterName);
		}
		for (const UMaterialExpressionScalarParameter* Expression : ScalarExpressions)
		{
			if (IsValid(Expression)) ScalarNames.Add(Expression->ParameterName);
		}
		TSet<const UMaterialFunctionInterface*> VisitedFunctions;
		CollectLiquidParameterNames(
			BaseMaterial->GetExpressions(), VectorNames, ScalarNames, VisitedFunctions);
		auto HasVector = [&VectorNames, Template](FName Name)
		{
			if (VectorNames.Contains(Name)) return true;
			FLinearColor InheritedValue;
			return Template->GetVectorParameterValue(
				FHashedMaterialParameterInfo(Name), InheritedValue);
		};
		auto HasScalar = [&ScalarNames, Template, BaseMaterial, Style](FName Name)
		{
			if (ScalarNames.Contains(Name)) return true;
			float InheritedValue = 0.0f;
			if (Template->GetScalarParameterValue(
				FHashedMaterialParameterInfo(Name), InheritedValue))
			{
				return true;
			}

			// UE 5.4's minimal custom-commandlet path omits the marketplace lava
			// master's Brightness entry from every C++ cache/query above, although the
			// normal editor and Python MaterialEditingLibrary both enumerate it. Keep
			// this compatibility exception deliberately exact: it cannot authorize a
			// parameter on another material, template, style, or name. The normal-editor
			// MaterialCatalogIntegrity automation test independently verifies the graph
			// enumeration, owned override, and effective value after generation.
			const bool bKnownUE54LavaBrightness =
				Style == ELiquidMaterialStyle::Lava
				&& Name == TEXT("Brightness")
				&& Template->GetPathName()
					== TEXT("/Game/APS/APS_ALPHA/WSC/WSC_MI_LavaOcean.WSC_MI_LavaOcean")
				&& BaseMaterial->GetPathName()
					== TEXT("/WorldScape/Ressources/Materials/WorldScapeMaterials/Ocean/M_Lava_WorldScape.M_Lava_WorldScape")
				&& Template->GetMaterial() == BaseMaterial;
			if (bKnownUE54LavaBrightness)
			{
				UE_LOG(LogTemp, Warning,
					TEXT("[APS.PlanetSurfaceAssets] Applying UE 5.4 compatibility validation for authored lava Brightness"));
			}
			return bKnownUE54LavaBrightness;
		};
		bool bParametersValid = true;
		auto RequireVector = [Result, &HasVector, &bParametersValid](
			const TCHAR* Name, const TOptional<FLinearColor>& Value = TOptional<FLinearColor>())
		{
			const FName ParameterName(Name);
			if (!HasVector(ParameterName))
			{
				UE_LOG(LogTemp, Error,
					TEXT("[APS.PlanetSurfaceAssets] %s parent %s is missing required vector parameter %s"),
					*Result->GetName(), *GetNameSafe(Result->Parent), Name);
				bParametersValid = false;
				return;
			}
			if (Value.IsSet())
			{
				Result->SetVectorParameterValueEditorOnly(
					FMaterialParameterInfo(ParameterName), Value.GetValue());
			}
		};
		auto RequireScalar = [Result, &HasScalar, &bParametersValid](
			const TCHAR* Name, const TOptional<float>& Value = TOptional<float>())
		{
			const FName ParameterName(Name);
			if (!HasScalar(ParameterName))
			{
				UE_LOG(LogTemp, Error,
					TEXT("[APS.PlanetSurfaceAssets] %s parent %s is missing required scalar parameter %s"),
					*Result->GetName(), *GetNameSafe(Result->Parent), Name);
				bParametersValid = false;
				return;
			}
			if (Value.IsSet())
			{
				Result->SetScalarParameterValueEditorOnly(
					FMaterialParameterInfo(ParameterName), Value.GetValue());
			}
		};

		switch (Style)
		{
		case ELiquidMaterialStyle::WaterBaseline:
			// Deliberately no overrides: keep the established
			// WSC_MI_Planetary_Ocean appearance byte-for-byte through inheritance.
			RequireVector(TEXT("Param"));
			RequireVector(TEXT("Param_1"));
			RequireVector(TEXT("Param_2"));
			RequireScalar(TEXT("ColorScaleBehindWater"));
			RequireScalar(TEXT("Roughness"));
			RequireScalar(TEXT("Specular"));
			break;
		case ELiquidMaterialStyle::Ammonia:
			// The established WorldScape water graph exposes its three custom-node
			// colour inputs under the authored parameter names Param/Param_1/Param_2.
			// BaseColor/AbsorptionCoefficients/ScatteringCoefficients are custom-node
			// input labels, not material parameter names, and must never be authored
			// as inert instance overrides.
			RequireVector(TEXT("Param"), FLinearColor(0.012f, 0.035f, 0.024f, 1.0f));
			RequireVector(TEXT("Param_1"), FLinearColor(0.35f, 0.82f, 0.68f, 1.0f));
			RequireVector(TEXT("Param_2"), FLinearColor(0.08f, 0.25f, 0.18f, 1.0f));
			RequireScalar(TEXT("ColorScaleBehindWater"), 0.68f);
			RequireScalar(TEXT("Roughness"), 0.18f);
			RequireScalar(TEXT("Specular"), 0.58f);
			break;
		case ELiquidMaterialStyle::Lava:
			// M_Lava_WorldScape intentionally exposes emissive/noise controls rather
			// than the water graph's optical controls. Keep the authored values inside
			// a display-safe range: the material graph already applies Brightness and
			// bloom, so an HDR colour here would illuminate the complete preview frame.
			RequireVector(TEXT("EmissiveColor"), FLinearColor(1.0f, 0.08f, 0.005f, 1.0f));
			RequireScalar(TEXT("Brightness"), 0.65f);
			break;
		}
		if (!bParametersValid) return nullptr;
		Result->PostEditChange();
		if (!SaveAsset(Result)) return nullptr;
		return Result;
	}
}

UAPSPlanetSurfaceAssetCommandlet::UAPSPlanetSurfaceAssetCommandlet()
{
	IsClient = false;
	IsEditor = true;
	LogToConsole = true;
	ShowErrorCount = true;
}

int32 UAPSPlanetSurfaceAssetCommandlet::Main(const FString& Params)
{
	using namespace APSPlanetSurfaceAssets;
	IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools")).Get();
	// Match the fully initialized editor/Python material path. Without this module,
	// a minimal custom commandlet can load marketplace MIC parents before their
	// editor parameter metadata is registered.
	FModuleManager::LoadModuleChecked<IModuleInterface>(TEXT("MaterialEditor"));

	// Load master materials as top-level assets before their project MICs. Loading
	// them only as a dependency of a MIC leaves some UE 5.4 marketplace graph
	// metadata (notably the lava Brightness scalar) unavailable in commandlets.
	UMaterial* WaterMaster = LoadObject<UMaterial>(nullptr,
		TEXT("/WorldScape/Ressources/Materials/WorldScapeMaterials/Ocean/M_Water_WorldScape.M_Water_WorldScape"));
	UMaterial* LavaMaster = LoadObject<UMaterial>(nullptr,
		TEXT("/WorldScape/Ressources/Materials/WorldScapeMaterials/Ocean/M_Lava_WorldScape.M_Lava_WorldScape"));

	UMaterialInstanceConstant* TerraTemplate = LoadObject<UMaterialInstanceConstant>(nullptr,
		TEXT("/Game/APS/APS_ALPHA/WSC/WSC_MI_Terra.WSC_MI_Terra"));
	UMaterialInstanceConstant* BarrenTemplate = LoadObject<UMaterialInstanceConstant>(nullptr,
		TEXT("/Game/APS/APS_ALPHA/WSC/WSC_MI_Selenae.WSC_MI_Selenae"));
	UMaterialInstanceConstant* MagmaTemplate = LoadObject<UMaterialInstanceConstant>(nullptr,
		TEXT("/Game/APS/APS_ALPHA/WSC/WSC_MI_Magma.WSC_MI_Magma"));
	UMaterialInstanceConstant* WaterTemplate = LoadObject<UMaterialInstanceConstant>(nullptr,
		TEXT("/Game/APS/APS_ALPHA/WSC/WSC_MI_Planetary_Ocean.WSC_MI_Planetary_Ocean"));
	UMaterialInstanceConstant* LavaTemplate = LoadObject<UMaterialInstanceConstant>(nullptr,
		TEXT("/Game/APS/APS_ALPHA/WSC/WSC_MI_LavaOcean.WSC_MI_LavaOcean"));
	if (!WaterMaster || !LavaMaster || !TerraTemplate || !BarrenTemplate || !MagmaTemplate
		|| !WaterTemplate || !LavaTemplate)
	{
		UE_LOG(LogTemp, Error,
			TEXT("[APS.PlanetSurfaceAssets] One or more WorldScape master/template assets are missing"));
		return 1;
	}
	if (WaterTemplate->GetMaterial() != WaterMaster || LavaTemplate->GetMaterial() != LavaMaster)
	{
		UE_LOG(LogTemp, Error,
			TEXT("[APS.PlanetSurfaceAssets] Project liquid templates no longer inherit the expected WorldScape masters"));
		return 1;
	}

	struct FFamilySpec
	{
		EAPSPlanetSurfaceArchetype Archetype;
		const TCHAR* Name;
		UMaterialInstanceConstant* Template;
	};
	const FFamilySpec Families[] =
	{
		{EAPSPlanetSurfaceArchetype::Rocky, TEXT("MI_APS_WS_Rocky"), BarrenTemplate},
		{EAPSPlanetSurfaceArchetype::Temperate, TEXT("MI_APS_WS_Temperate"), TerraTemplate},
		{EAPSPlanetSurfaceArchetype::Oceanic, TEXT("MI_APS_WS_Oceanic"), TerraTemplate},
		{EAPSPlanetSurfaceArchetype::Biosphere, TEXT("MI_APS_WS_Biosphere"), TerraTemplate},
		{EAPSPlanetSurfaceArchetype::Desert, TEXT("MI_APS_WS_Desert"), TerraTemplate},
		{EAPSPlanetSurfaceArchetype::Cryogenic, TEXT("MI_APS_WS_Cryogenic"), BarrenTemplate},
		{EAPSPlanetSurfaceArchetype::Magmatic, TEXT("MI_APS_WS_Magmatic"), MagmaTemplate},
		{EAPSPlanetSurfaceArchetype::Metallic, TEXT("MI_APS_WS_Metallic"), BarrenTemplate},
		{EAPSPlanetSurfaceArchetype::ExoticChemical, TEXT("MI_APS_WS_ExoticChemical"), BarrenTemplate}
	};

	TMap<EAPSPlanetSurfaceArchetype, UMaterialInstanceConstant*> FamilyMaterials;
	for (const FFamilySpec& Family : Families)
	{
		const FAPSPlanetSurfaceArchetypeDefinition Definition =
			UAPSPlanetSurfaceProfileResolver::GetNativeDefinition(Family.Archetype);
		UMaterialInstanceConstant* Material = CreateFamilyMaterial(
			AssetTools, Family.Name, Family.Template, Definition);
		if (!Material)
		{
			UE_LOG(LogTemp, Error, TEXT("[APS.PlanetSurfaceAssets] Failed to create %s"), Family.Name);
			return 3;
		}
		FamilyMaterials.Add(Family.Archetype, Material);
	}

	UMaterialInstanceConstant* Water = CreateLiquidMaterial(
		AssetTools, TEXT("MI_APS_WS_Water"), WaterTemplate, ELiquidMaterialStyle::WaterBaseline);
	UMaterialInstanceConstant* Ammonia = CreateLiquidMaterial(
		AssetTools, TEXT("MI_APS_WS_Ammonia"), WaterTemplate, ELiquidMaterialStyle::Ammonia);
	UMaterialInstanceConstant* Lava = CreateLiquidMaterial(
		AssetTools, TEXT("MI_APS_WS_Lava"), LavaTemplate, ELiquidMaterialStyle::Lava);
	if (!Water || !Ammonia || !Lava) return 4;

	// Orbital preview meshes are generic procedural meshes, not WorldScape LOD
	// primitives. Give them self-contained materials so the masked terrain graph and
	// SingleLayerWater/world-position assumptions cannot clip or black out the globe.
	// These assets are deliberately not written into DA_PlanetSurfaceCatalog: the
	// catalog remains the authoritative full-scale gameplay pipeline.
	UMaterial* OrbitalTerrain = CreateOrbitalTerrainMaterial(AssetTools);
	UMaterial* OrbitalLiquid = CreateOrbitalLiquidMaterial(AssetTools);
	if (!OrbitalTerrain || !OrbitalLiquid)
	{
		UE_LOG(LogTemp, Error, TEXT("[APS.PlanetSurfaceAssets] Failed to create orbital preview masters"));
		return 5;
	}
	UMaterialInstanceConstant* OrbitalWater = CreateOrbitalLiquidPreset(
		AssetTools, TEXT("MI_APS_OrbitalLiquid_Water"), OrbitalLiquid,
		FLinearColor(0.005f, 0.025f, 0.090f), FLinearColor(0.030f, 0.300f, 0.580f),
		FLinearColor(0.003f, 0.012f, 0.030f), 0.52f, 0.18f, 0.0f, 0.65f);
	UMaterialInstanceConstant* OrbitalAmmonia = CreateOrbitalLiquidPreset(
		AssetTools, TEXT("MI_APS_OrbitalLiquid_Ammonia"), OrbitalLiquid,
		FLinearColor(0.008f, 0.055f, 0.025f), FLinearColor(0.160f, 0.480f, 0.250f),
		FLinearColor(0.005f, 0.025f, 0.012f), 0.50f, 0.22f, 0.0f, 0.60f);
	UMaterialInstanceConstant* OrbitalLava = CreateOrbitalLiquidPreset(
		AssetTools, TEXT("MI_APS_OrbitalLiquid_Lava"), OrbitalLiquid,
		FLinearColor(0.055f, 0.001f, 0.0005f), FLinearColor(0.720f, 0.025f, 0.001f),
		FLinearColor(0.420f, 0.018f, 0.001f), 0.54f, 0.42f, 0.04f, 0.30f);
	if (!OrbitalWater || !OrbitalAmmonia || !OrbitalLava)
	{
		UE_LOG(LogTemp, Error, TEXT("[APS.PlanetSurfaceAssets] Failed to create orbital preview materials"));
		return 5;
	}

	UAPSPlanetSurfaceCatalog* Catalog = LoadAsset<UAPSPlanetSurfaceCatalog>(RootPath, TEXT("DA_PlanetSurfaceCatalog"));
	if (!Catalog)
	{
		UDataAssetFactory* Factory = NewObject<UDataAssetFactory>();
		Factory->DataAssetClass = UAPSPlanetSurfaceCatalog::StaticClass();
		Catalog = Cast<UAPSPlanetSurfaceCatalog>(AssetTools.CreateAsset(
			TEXT("DA_PlanetSurfaceCatalog"), RootPath, UAPSPlanetSurfaceCatalog::StaticClass(), Factory));
	}
	if (!Catalog) return 6;

	Catalog->Archetypes.Empty();
	for (const FFamilySpec& Family : Families)
	{
		FAPSPlanetSurfaceArchetypeDefinition Definition =
			UAPSPlanetSurfaceProfileResolver::GetNativeDefinition(Family.Archetype);
		Definition.TerrainMaterial = FamilyMaterials.FindRef(Family.Archetype);
		switch (Definition.LiquidType)
		{
		case EAPSPlanetLiquidType::Water: Definition.OceanMaterial = Water; break;
		case EAPSPlanetLiquidType::Ammonia: Definition.OceanMaterial = Ammonia; break;
		case EAPSPlanetLiquidType::Lava: Definition.OceanMaterial = Lava; break;
		case EAPSPlanetLiquidType::None: default: break;
		}
		Catalog->Archetypes.Add(Family.Archetype, Definition);
	}
	Catalog->PostEditChange();
	if (!SaveAsset(Catalog)) return 7;

	UE_LOG(LogTemp, Display, TEXT("[APS.PlanetSurfaceAssets] Updated 9 WorldScape family materials, 3 gameplay liquids, 5 orbital preview materials and catalog under %s"),
		*RootPath);
	return 0;
}

#endif
