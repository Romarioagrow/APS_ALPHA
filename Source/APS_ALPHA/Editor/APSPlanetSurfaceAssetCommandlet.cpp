#include "APSPlanetSurfaceAssetCommandlet.h"

#if WITH_EDITOR

#include "AssetToolsModule.h"
#include "Factories/DataAssetFactory.h"
#include "Factories/MaterialFactoryNew.h"
#include "Factories/MaterialInstanceConstantFactoryNew.h"
#include "IAssetTools.h"
#include "Materials/Material.h"
#include "Materials/MaterialExpressionAdd.h"
#include "Materials/MaterialExpressionClamp.h"
#include "Materials/MaterialExpressionComponentMask.h"
#include "Materials/MaterialExpressionDivide.h"
#include "Materials/MaterialExpressionDotProduct.h"
#include "Materials/MaterialExpressionFresnel.h"
#include "Materials/MaterialExpressionLinearInterpolate.h"
#include "Materials/MaterialExpressionMultiply.h"
#include "Materials/MaterialExpressionNoise.h"
#include "Materials/MaterialExpressionNormalize.h"
#include "Materials/MaterialExpressionOneMinus.h"
#include "Materials/MaterialExpressionPixelDepth.h"
#include "Materials/MaterialExpressionScalarParameter.h"
#include "Materials/MaterialExpressionSmoothStep.h"
#include "Materials/MaterialExpressionSubtract.h"
#include "Materials/MaterialExpressionVectorParameter.h"
#include "Materials/MaterialExpressionVectorNoise.h"
#include "Materials/MaterialExpressionVertexColor.h"
#include "Materials/MaterialExpressionVertexNormalWS.h"
#include "Materials/MaterialExpressionWorldPosition.h"
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

	UMaterial* LoadOrCreateMaterial(
		IAssetTools& AssetTools, const FString& Path, const FString& Name)
	{
		if (UMaterial* Existing = LoadAsset<UMaterial>(Path, Name))
		{
			return Existing;
		}

		UMaterialFactoryNew* Factory = NewObject<UMaterialFactoryNew>();
		return Cast<UMaterial>(AssetTools.CreateAsset(
			Name, Path, UMaterial::StaticClass(), Factory));
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
		Parameter->Group = TEXT("APS WorldScape Surface");
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
		Parameter->Group = TEXT("APS WorldScape Surface");
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

	void ClearMaterialExpressions(UMaterial* Material)
	{
		if (!IsValid(Material))
		{
			return;
		}

		// UE 5.4's bulk helper mutates the same live expression array it iterates.
		// On an already-authored material that can leave stale nodes behind and, for
		// some expression counts, attempt to garbage-mark an expression twice.  The
		// latter trips UObject's !IsRooted assertion in headless commandlets.  The
		// tail loop is the engine-safe, idempotent path already used by the stellar
		// material maintenance commandlet in this project.
		while (!Material->GetExpressions().IsEmpty())
		{
			UMaterialExpression* Expression = Material->GetExpressions().Last();
			// M_APS_PreviewGuide is synchronously loaded by the AstroGenerator CDO.
			// ConstructorHelpers keeps its existing expression objects rooted for the
			// lifetime of the commandlet process, so the editor deletion helper cannot
			// garbage-mark them until that transient root is released.  We are replacing
			// these subobjects immediately and the material itself remains strongly held.
			if (IsValid(Expression) && Expression->IsRooted())
			{
				Expression->RemoveFromRoot();
			}
			UMaterialEditingLibrary::DeleteMaterialExpression(
				Material, Expression);
		}
	}

	UMaterial* CreateTerrainMaterial(
		IAssetTools& AssetTools, const FString& Path, const FString& Name,
		bool bEnableNearFieldWorldDetail)
	{
		UMaterial* Material = LoadOrCreateMaterial(AssetTools, Path, Name);
		if (!IsValid(Material)) return nullptr;
		Material->Modify();
		ClearMaterialExpressions(Material);
		Material->MaterialDomain = MD_Surface;
		Material->BlendMode = BLEND_Opaque;
		Material->SetShadingModel(MSM_DefaultLit);
		Material->TwoSided = false;
		// The canonical gameplay graph emits a world-space perturbed normal.  This
		// avoids relying on WorldScape patch tangents (which are intentionally not
		// generated for every streaming LOD) and keeps the cue continuous across cube
		// faces. The cheap hierarchy material keeps the default tangent-space contract.
		Material->bTangentSpaceNormal = !bEnableNearFieldWorldDetail;

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
			Material, TEXT("ClimateBlend"), 0.10f, 0.03f, 0.24f, -500, 300, 10);
		UMaterialExpressionScalarParameter* Roughness = AddScalarParameter(
			Material, TEXT("Roughness"), 0.72f, 0.0f, 1.0f, 200, 300, 11);
		UMaterialExpressionScalarParameter* Metallic = AddScalarParameter(
			Material, TEXT("Metallic"), 0.04f, 0.0f, 1.0f, 200, 400, 12);
		UMaterialExpressionScalarParameter* Specular = AddScalarParameter(
			Material, TEXT("Specular"), 0.35f, 0.0f, 1.0f, 200, 500, 13);
		UMaterialExpressionScalarParameter* PaletteGain = AddScalarParameter(
			Material, TEXT("PaletteGain"), 1.0f, 0.75f, 1.65f, -180, 300, 14);
		UMaterialExpressionScalarParameter* PaletteLift = AddScalarParameter(
			Material, TEXT("PaletteLift"), 0.0f, 0.0f, 0.06f, -180, 400, 15);
		if (!Vertex || !Coast || !Lowland || !MidLowland || !Highland || !Dryland || !Peak
			|| !Emissive || !ClimateBlend || !Roughness || !Metallic || !Specular
			|| !PaletteGain || !PaletteLift)
		{
			return nullptr;
		}

		// Height is WorldScape's normalized vertex-colour R channel. Give every
		// authored palette colour its own bounded elevation transition. Reusing raw
		// height as the alpha of several nested lerps produces a polynomial average
		// that washes distinct presets into the same pale orbital globe.
		// Broad overlapping ramps suppress contour rings when adjacent WorldScape
		// patches resolve at different LODs. All family identity still comes from its
		// six authored colours; this only makes the transitions geological instead of
		// reading like discrete elevation isolines.
		UMaterialExpressionSmoothStep* CoastToLowBand = AddSmoothStep(
			Material, Vertex, 1, 0.02f, 0.30f, -1180, -680);
		UMaterialExpressionSmoothStep* LowToMidBand = AddSmoothStep(
			Material, Vertex, 1, 0.14f, 0.46f, -1180, -540);
		UMaterialExpressionSmoothStep* MidToHighBand = AddSmoothStep(
			Material, Vertex, 1, 0.30f, 0.62f, -1180, -400);
		UMaterialExpressionSmoothStep* HighToDryBand = AddSmoothStep(
			Material, Vertex, 1, 0.46f, 0.80f, -1180, -260);
		UMaterialExpressionSmoothStep* DryToPeakBand = AddSmoothStep(
			Material, Vertex, 1, 0.64f, 0.98f, -1180, -120);
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
		// Family palettes are authored in linear space. The deliberately dark barren
		// palettes need a small per-family exposure correction, otherwise physically
		// plausible values collapse to almost black after scene lighting. Keep this as
		// an instance parameter rather than altering the runtime-resolved palette, so
		// hue jitter and profile identity remain intact.
		UMaterialExpressionMultiply* GainedPaletteColor =
			AddExpression<UMaterialExpressionMultiply>(Material, 190, -260);
		UMaterialExpressionAdd* LiftedPaletteColor =
			AddExpression<UMaterialExpressionAdd>(Material, 370, -260);
		if (!GainedPaletteColor || !LiftedPaletteColor)
		{
			return nullptr;
		}
		GainedPaletteColor->A.Connect(0, FinalColor);
		GainedPaletteColor->B.Connect(0, PaletteGain);
		LiftedPaletteColor->A.Connect(0, GainedPaletteColor);
		LiftedPaletteColor->B.Connect(0, PaletteLift);

		UMaterialExpression* SurfaceColor = LiftedPaletteColor;
		UMaterialExpression* SurfaceRoughness = Roughness;
		UMaterialExpression* SurfaceNormal = nullptr;
		if (bEnableNearFieldWorldDetail)
		{
			// WorldScape is assembled from independently streamed cube patches. UV- or
			// object-local detail exposes those patch boundaries immediately, especially
			// at LOD transitions. Both bands below sample one continuous absolute-world
			// volume instead. The slow band adds only a restrained regional variation;
			// the near band contributes subtle roughness and a very low-amplitude
			// tangent-projected normal without adding a second proxy surface or visibly
			// re-tiling the authored terrain while the camera moves.
			UMaterialExpressionWorldPosition* WorldPosition =
				AddExpression<UMaterialExpressionWorldPosition>(Material, 120, -980);
			UMaterialExpressionScalarParameter* MacroScale = AddScalarParameter(
				Material, TEXT("MacroDetailScaleCm"), 900000.0f, 200000.0f, 3000000.0f,
				120, -880, 20);
			UMaterialExpressionScalarParameter* NearScale = AddScalarParameter(
				Material, TEXT("NearDetailScaleCm"), 18000.0f, 8000.0f, 60000.0f,
				120, -780, 21);
			UMaterialExpressionScalarParameter* MacroColorStrength = AddScalarParameter(
				Material, TEXT("MacroColorStrength"), 0.014f, 0.0f, 0.05f,
				120, -680, 22);
			UMaterialExpressionScalarParameter* NearColorStrength = AddScalarParameter(
				Material, TEXT("NearColorStrength"), 0.003f, 0.0f, 0.02f,
				120, -580, 23);
			UMaterialExpressionScalarParameter* DetailNormalStrength = AddScalarParameter(
				Material, TEXT("DetailNormalStrength"), 0.022f, 0.0f, 0.08f,
				120, -480, 24);
			UMaterialExpressionScalarParameter* DetailRoughnessStrength = AddScalarParameter(
				Material, TEXT("DetailRoughnessStrength"), 0.040f, 0.0f, 0.12f,
				120, -380, 25);
			UMaterialExpressionDivide* MacroPosition =
				AddExpression<UMaterialExpressionDivide>(Material, 400, -920);
			UMaterialExpressionDivide* NearPosition =
				AddExpression<UMaterialExpressionDivide>(Material, 400, -760);
			UMaterialExpressionNoise* MacroNoise =
				AddExpression<UMaterialExpressionNoise>(Material, 620, -920);
			UMaterialExpressionVectorNoise* NearNoise =
				AddExpression<UMaterialExpressionVectorNoise>(Material, 620, -720);
			UMaterialExpressionComponentMask* NearGradient =
				AddExpression<UMaterialExpressionComponentMask>(Material, 850, -770);
			UMaterialExpressionComponentMask* NearScalar =
				AddExpression<UMaterialExpressionComponentMask>(Material, 850, -650);
			UMaterialExpressionPixelDepth* PixelDepth =
				AddExpression<UMaterialExpressionPixelDepth>(Material, 620, -500);
			UMaterialExpressionSmoothStep* FarDetailFade = AddSmoothStep(
				Material, PixelDepth, 0, 5000.0f, 500000.0f, 830, -500);
			UMaterialExpressionOneMinus* NearFieldFade =
				AddExpression<UMaterialExpressionOneMinus>(Material, 1040, -500);

			UMaterialExpressionMultiply* MacroColorTerm =
				AddExpression<UMaterialExpressionMultiply>(Material, 1050, -900);
			UMaterialExpressionMultiply* FadedNearColorStrength =
				AddExpression<UMaterialExpressionMultiply>(Material, 1050, -600);
			UMaterialExpressionMultiply* NearColorTerm =
				AddExpression<UMaterialExpressionMultiply>(Material, 1270, -650);
			UMaterialExpressionAdd* DetailColorTerms =
				AddExpression<UMaterialExpressionAdd>(Material, 1470, -780);
			UMaterialExpressionAdd* DetailColorScale =
				AddExpression<UMaterialExpressionAdd>(Material, 1650, -780);
			UMaterialExpressionMultiply* DetailedColor =
				AddExpression<UMaterialExpressionMultiply>(Material, 1840, -500);
			UMaterialExpressionClamp* BoundedDetailedColor =
				AddExpression<UMaterialExpressionClamp>(Material, 2040, -500);

			UMaterialExpressionMultiply* FadedRoughnessStrength =
				AddExpression<UMaterialExpressionMultiply>(Material, 1050, -280);
			UMaterialExpressionMultiply* RoughnessVariation =
				AddExpression<UMaterialExpressionMultiply>(Material, 1270, -280);
			UMaterialExpressionAdd* DetailedRoughness =
				AddExpression<UMaterialExpressionAdd>(Material, 1480, -240);
			UMaterialExpressionClamp* BoundedDetailedRoughness =
				AddExpression<UMaterialExpressionClamp>(Material, 1690, -240);

			UMaterialExpressionVertexNormalWS* VertexNormal =
				AddExpression<UMaterialExpressionVertexNormalWS>(Material, 1050, -80);
			UMaterialExpressionDotProduct* RadialGradient =
				AddExpression<UMaterialExpressionDotProduct>(Material, 1260, -80);
			UMaterialExpressionMultiply* RadialGradientVector =
				AddExpression<UMaterialExpressionMultiply>(Material, 1460, -20);
			UMaterialExpressionSubtract* TangentGradient =
				AddExpression<UMaterialExpressionSubtract>(Material, 1650, -20);
			UMaterialExpressionMultiply* FadedNormalStrength =
				AddExpression<UMaterialExpressionMultiply>(Material, 1260, 100);
			UMaterialExpressionMultiply* NormalPerturbation =
				AddExpression<UMaterialExpressionMultiply>(Material, 1840, 40);
			UMaterialExpressionAdd* PerturbedNormal =
				AddExpression<UMaterialExpressionAdd>(Material, 2040, 40);
			UMaterialExpressionNormalize* NormalizedWorldNormal =
				AddExpression<UMaterialExpressionNormalize>(Material, 2240, 40);

			if (!WorldPosition || !MacroScale || !NearScale || !MacroColorStrength
				|| !NearColorStrength || !DetailNormalStrength || !DetailRoughnessStrength
				|| !MacroPosition || !NearPosition || !MacroNoise || !NearNoise
				|| !NearGradient || !NearScalar || !PixelDepth || !FarDetailFade
				|| !NearFieldFade || !MacroColorTerm || !FadedNearColorStrength
				|| !NearColorTerm || !DetailColorTerms || !DetailColorScale
				|| !DetailedColor || !BoundedDetailedColor || !FadedRoughnessStrength
				|| !RoughnessVariation || !DetailedRoughness || !BoundedDetailedRoughness
				|| !VertexNormal || !RadialGradient || !RadialGradientVector
				|| !TangentGradient || !FadedNormalStrength || !NormalPerturbation
				|| !PerturbedNormal || !NormalizedWorldNormal)
			{
				return nullptr;
			}

			WorldPosition->WorldPositionShaderOffset = WPT_ExcludeAllShaderOffsets;
			MacroPosition->A.Connect(0, WorldPosition);
			MacroPosition->B.Connect(0, MacroScale);
			NearPosition->A.Connect(0, WorldPosition);
			NearPosition->B.Connect(0, NearScale);

			MacroNoise->Position.Connect(0, MacroPosition);
			MacroNoise->WorldPositionOriginType = EPositionOrigin::Absolute;
			MacroNoise->Scale = 1.0f;
			MacroNoise->Quality = 1;
			MacroNoise->NoiseFunction = NOISEFUNCTION_SimplexTex;
			MacroNoise->bTurbulence = false;
			MacroNoise->Levels = 1;
			MacroNoise->OutputMin = -1.0f;
			MacroNoise->OutputMax = 1.0f;
			MacroNoise->bTiling = false;

			NearNoise->Position.Connect(0, NearPosition);
			NearNoise->WorldPositionOriginType = EPositionOrigin::Absolute;
			NearNoise->NoiseFunction = VNF_GradientALU;
			NearNoise->Quality = 1;
			NearNoise->bTiling = false;
			NearGradient->Input.Connect(0, NearNoise);
			NearGradient->R = true;
			NearGradient->G = true;
			NearGradient->B = true;
			NearGradient->A = false;
			NearScalar->Input.Connect(0, NearNoise);
			NearScalar->R = false;
			NearScalar->G = false;
			NearScalar->B = false;
			NearScalar->A = true;
			NearFieldFade->Input.Connect(0, FarDetailFade);

			MacroColorTerm->A.Connect(0, MacroNoise);
			MacroColorTerm->B.Connect(0, MacroColorStrength);
			FadedNearColorStrength->A.Connect(0, NearColorStrength);
			FadedNearColorStrength->B.Connect(0, NearFieldFade);
			NearColorTerm->A.Connect(0, NearScalar);
			NearColorTerm->B.Connect(0, FadedNearColorStrength);
			DetailColorTerms->A.Connect(0, MacroColorTerm);
			DetailColorTerms->B.Connect(0, NearColorTerm);
			DetailColorScale->A.Connect(0, DetailColorTerms);
			DetailColorScale->ConstB = 1.0f;
			DetailedColor->A.Connect(0, LiftedPaletteColor);
			DetailedColor->B.Connect(0, DetailColorScale);
			BoundedDetailedColor->Input.Connect(0, DetailedColor);
			BoundedDetailedColor->ClampMode = CMODE_Clamp;
			BoundedDetailedColor->MinDefault = 0.0f;
			BoundedDetailedColor->MaxDefault = 1.0f;

			FadedRoughnessStrength->A.Connect(0, DetailRoughnessStrength);
			FadedRoughnessStrength->B.Connect(0, NearFieldFade);
			RoughnessVariation->A.Connect(0, NearScalar);
			RoughnessVariation->B.Connect(0, FadedRoughnessStrength);
			DetailedRoughness->A.Connect(0, Roughness);
			DetailedRoughness->B.Connect(0, RoughnessVariation);
			BoundedDetailedRoughness->Input.Connect(0, DetailedRoughness);
			BoundedDetailedRoughness->ClampMode = CMODE_Clamp;
			BoundedDetailedRoughness->MinDefault = 0.04f;
			BoundedDetailedRoughness->MaxDefault = 0.98f;

			// VectorNoise GradientALU returns the signed volume gradient in RGB and
			// scalar noise in A. Remove the component along the actual streamed vertex
			// normal before applying it, so detail follows the spherical terrain rather
			// than biasing normals toward an arbitrary world axis.
			RadialGradient->A.Connect(0, NearGradient);
			RadialGradient->B.Connect(0, VertexNormal);
			RadialGradientVector->A.Connect(0, VertexNormal);
			RadialGradientVector->B.Connect(0, RadialGradient);
			TangentGradient->A.Connect(0, NearGradient);
			TangentGradient->B.Connect(0, RadialGradientVector);
			FadedNormalStrength->A.Connect(0, DetailNormalStrength);
			FadedNormalStrength->B.Connect(0, NearFieldFade);
			NormalPerturbation->A.Connect(0, TangentGradient);
			NormalPerturbation->B.Connect(0, FadedNormalStrength);
			PerturbedNormal->A.Connect(0, VertexNormal);
			PerturbedNormal->B.Connect(0, NormalPerturbation);
			NormalizedWorldNormal->VectorInput.Connect(0, PerturbedNormal);

			SurfaceColor = BoundedDetailedColor;
			SurfaceRoughness = BoundedDetailedRoughness;
			SurfaceNormal = NormalizedWorldNormal;
		}

		UMaterialExpressionClamp* BoundedEmissive =
			AddExpression<UMaterialExpressionClamp>(Material, -300, 650);
		UMaterialExpressionSmoothStep* EmissiveHeightBand = AddSmoothStep(
			Material, Vertex, 1, 0.04f, 0.82f, -500, 760);
		UMaterialExpressionOneMinus* LowlandEmissiveMask =
			AddExpression<UMaterialExpressionOneMinus>(Material, -250, 760);
		UMaterialExpressionMultiply* MaskedEmissive =
			AddExpression<UMaterialExpressionMultiply>(Material, 0, 650);
		if (!CoastToLowBand || !LowToMidBand || !MidToHighBand || !HighToDryBand
			|| !DryToPeakBand || !CoastLow || !LowMid || !MidHigh || !HighDry || !HeightRamp || !Thermal
			|| !Climate || !FinalColor || !BoundedEmissive || !EmissiveHeightBand
			|| !LowlandEmissiveMask || !MaskedEmissive)
		{
			return nullptr;
		}
		BoundedEmissive->Input.Connect(0, Emissive);
		BoundedEmissive->ClampMode = CMODE_Clamp;
		BoundedEmissive->MinDefault = 0.0f;
		// Keep the canonical full-scale surface conservative so molten lowlands do not
		// turn most of the gameplay disk display-white after exposure/tonemapping.
		// The hierarchy/orbital master retains its established 1.25 headroom: it is a
		// compact distant representation and its material contract is checked by the
		// catalog integrity test. Both paths remain explicitly bounded.
		BoundedEmissive->MaxDefault = bEnableNearFieldWorldDetail ? 0.42f : 1.25f;
		LowlandEmissiveMask->Input.Connect(0, EmissiveHeightBand);
		MaskedEmissive->A.Connect(0, BoundedEmissive);
		// WorldScape reserves vertex alpha for its hole mask. The retired closed-globe
		// proxy used that channel as a water/emissive mask, which made the two renderers
		// fundamentally disagree. Derive lava glow from the shared normalized-height R
		// channel so the exact same material is valid on real WorldScape LOD meshes.
		// The deliberately broad 0.04..0.82 transition prevents emissive lava from
		// outlining individual vertices or LOD patches as a square/dotted grid.
		MaskedEmissive->B.Connect(0, LowlandEmissiveMask);

		const bool bNormalConnected = !SurfaceNormal
			|| UMaterialEditingLibrary::ConnectMaterialProperty(SurfaceNormal, TEXT(""), MP_Normal);
		const bool bConnected = bNormalConnected
			&& UMaterialEditingLibrary::ConnectMaterialProperty(SurfaceColor, TEXT(""), MP_BaseColor)
			&& UMaterialEditingLibrary::ConnectMaterialProperty(MaskedEmissive, TEXT(""), MP_EmissiveColor)
			&& UMaterialEditingLibrary::ConnectMaterialProperty(SurfaceRoughness, TEXT(""), MP_Roughness)
			&& UMaterialEditingLibrary::ConnectMaterialProperty(Metallic, TEXT(""), MP_Metallic)
			&& UMaterialEditingLibrary::ConnectMaterialProperty(Specular, TEXT(""), MP_Specular);
		return bConnected && FinalizePreviewMaterial(Material) ? Material : nullptr;
	}

	UMaterial* CreateWorldScapeTerrainMaterial(IAssetTools& AssetTools)
	{
		// This graph is assigned to the real WorldScape LOD meshes. It deliberately
		// contains no UV texture sampling, cube-face projection or proxy geometry:
		// the continuous WorldScape vertex payload drives every visible colour band.
		return CreateTerrainMaterial(
			AssetTools, MaterialPath, TEXT("M_APS_WorldScapeTerrain"), true);
	}

	UMaterial* CreateOrbitalTerrainMaterial(IAssetTools& AssetTools)
	{
		// Keep the cheap hierarchy material as an independent asset, but generate it
		// from the same graph contract so preview and gameplay cannot visually diverge.
		return CreateTerrainMaterial(
			AssetTools, PreviewMaterialPath, TEXT("M_APS_OrbitalTerrain"), false);
	}

	UMaterial* CreateCanonicalLiquidMaterial(IAssetTools& AssetTools)
	{
		// Keep the established asset path because hierarchy preview MICs already refer
		// to it, but make this one graph authoritative for WorldScape ocean LODs too.
		// A project-owned graph avoids the marketplace materials' UV/custom-node
		// assumptions that expose individual WorldScape cube patches from orbit.
		UMaterial* Material = LoadOrCreateMaterial(
			AssetTools, PreviewMaterialPath, TEXT("M_APS_OrbitalLiquid"));
		if (!IsValid(Material)) return nullptr;
		Material->Modify();
		ClearMaterialExpressions(Material);
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
		UMaterialExpressionScalarParameter* Opacity = AddScalarParameter(
			Material, TEXT("Opacity"), 0.42f, 0.05f, 0.68f, -600, 300, 10);
		UMaterialExpressionScalarParameter* Roughness = AddScalarParameter(
			Material, TEXT("Roughness"), 0.18f, 0.0f, 1.0f, 50, 300, 11);
		UMaterialExpressionScalarParameter* Metallic = AddScalarParameter(
			Material, TEXT("Metallic"), 0.0f, 0.0f, 1.0f, 50, 400, 12);
		UMaterialExpressionScalarParameter* Specular = AddScalarParameter(
			Material, TEXT("Specular"), 0.65f, 0.0f, 1.0f, 50, 500, 13);
		UMaterialExpressionFresnel* Fresnel =
			AddExpression<UMaterialExpressionFresnel>(Material, -600, -80);
		if (!Deep || !Shallow || !Emissive || !Opacity || !Roughness || !Metallic
			|| !Specular || !Fresnel)
		{
			return nullptr;
		}
		Fresnel->Exponent = 4.0f;
		Fresnel->BaseReflectFraction = 0.04f;
		UMaterialExpressionClamp* BoundedFresnel =
			AddExpression<UMaterialExpressionClamp>(Material, -420, -80);
		if (!BoundedFresnel)
		{
			return nullptr;
		}
		BoundedFresnel->Input.Connect(0, Fresnel);
		BoundedFresnel->ClampMode = CMODE_Clamp;
		BoundedFresnel->MinDefault = 0.0f;
		BoundedFresnel->MaxDefault = 1.0f;
		UMaterialExpressionLinearInterpolate* LiquidColor = AddLerp(
			Material, Deep, Shallow, BoundedFresnel, 0, -180, -260);
		UMaterialExpressionClamp* BoundedEmissive =
			AddExpression<UMaterialExpressionClamp>(Material, -300, 100);
		UMaterialExpressionLinearInterpolate* FresnelOpacityScale =
			AddExpression<UMaterialExpressionLinearInterpolate>(Material, -180, 260);
		UMaterialExpressionMultiply* FresnelOpacity =
			AddExpression<UMaterialExpressionMultiply>(Material, 0, 300);
		UMaterialExpressionClamp* BoundedOpacity =
			AddExpression<UMaterialExpressionClamp>(Material, 180, 300);
		if (!LiquidColor || !BoundedEmissive || !FresnelOpacityScale
			|| !FresnelOpacity || !BoundedOpacity)
		{
			return nullptr;
		}
		BoundedEmissive->Input.Connect(0, Emissive);
		BoundedEmissive->ClampMode = CMODE_Clamp;
		BoundedEmissive->MinDefault = 0.0f;
		BoundedEmissive->MaxDefault = 1.25f;
		FresnelOpacityScale->ConstA = 0.62f;
		FresnelOpacityScale->ConstB = 1.0f;
		FresnelOpacityScale->Alpha.Connect(0, BoundedFresnel);
		FresnelOpacity->A.Connect(0, Opacity);
		FresnelOpacity->B.Connect(0, FresnelOpacityScale);
		BoundedOpacity->Input.Connect(0, FresnelOpacity);
		BoundedOpacity->ClampMode = CMODE_Clamp;
		BoundedOpacity->MinDefault = 0.05f;
		BoundedOpacity->MaxDefault = 0.68f;
		// WorldScape writes zero to vertex alpha for an ordinary, non-hole ocean
		// vertex. Opacity and emissive therefore must never use alpha as a visibility
		// multiplier. Fresnel attenuates face-on opacity so terrain relief remains
		// readable while the limb still identifies the liquid. The final clamp also
		// prevents one malformed MIC value from turning the ocean into a black shell.

		const bool bConnected =
			UMaterialEditingLibrary::ConnectMaterialProperty(LiquidColor, TEXT(""), MP_BaseColor)
			&& UMaterialEditingLibrary::ConnectMaterialProperty(BoundedEmissive, TEXT(""), MP_EmissiveColor)
			&& UMaterialEditingLibrary::ConnectMaterialProperty(BoundedOpacity, TEXT(""), MP_Opacity)
			&& UMaterialEditingLibrary::ConnectMaterialProperty(Roughness, TEXT(""), MP_Roughness)
			&& UMaterialEditingLibrary::ConnectMaterialProperty(Metallic, TEXT(""), MP_Metallic)
			&& UMaterialEditingLibrary::ConnectMaterialProperty(Specular, TEXT(""), MP_Specular);
		return bConnected && FinalizePreviewMaterial(Material) ? Material : nullptr;
	}

	UMaterial* CreatePreviewGuideMaterial(IAssetTools& AssetTools)
	{
		// Scope boundaries are geometry-only wire spheres. Their master must not add a
		// surface fill, lighting response or debug-material tonemapping; colour and
		// opacity remain independent so STAR and SYSTEM can share the same graph.
		UMaterial* Material = LoadOrCreateMaterial(
			AssetTools, PreviewMaterialPath, TEXT("M_APS_PreviewGuide"));
		if (!IsValid(Material)) return nullptr;
		Material->Modify();
		ClearMaterialExpressions(Material);
		Material->MaterialDomain = MD_Surface;
		Material->BlendMode = BLEND_Translucent;
		Material->SetShadingModel(MSM_Unlit);
		Material->TwoSided = true;
		Material->bCastRayTracedShadows = false;

		UMaterialExpressionVectorParameter* GuideColor = AddVectorParameter(
			Material, TEXT("GuideColor"), FLinearColor(0.80f, 0.20f, 0.025f),
			-420, -80, 0);
		UMaterialExpressionScalarParameter* GuideOpacity = AddScalarParameter(
			Material, TEXT("GuideOpacity"), 0.32f, 0.05f, 0.50f,
			-420, 100, 1);
		if (!GuideColor || !GuideOpacity)
		{
			return nullptr;
		}
		GuideColor->Group = TEXT("APS Preview Guides");
		GuideOpacity->Group = TEXT("APS Preview Guides");

		const bool bConnected =
			UMaterialEditingLibrary::ConnectMaterialProperty(
				GuideColor, TEXT(""), MP_EmissiveColor)
			&& UMaterialEditingLibrary::ConnectMaterialProperty(
				GuideOpacity, TEXT(""), MP_Opacity);
		return bConnected && FinalizePreviewMaterial(Material) ? Material : nullptr;
	}

	UMaterialInstanceConstant* CreateLiquidPreset(
		IAssetTools& AssetTools, const FString& Path, const FString& Name, UMaterial* Parent,
		const FLinearColor& DeepColor, const FLinearColor& ShallowColor,
		const FLinearColor& EmissiveColor, float Opacity, float Roughness,
		float Metallic, float Specular)
	{
		UMaterialInstanceConstant* Result =
			LoadAsset<UMaterialInstanceConstant>(Path, Name);
		if (!IsValid(Result))
		{
			UMaterialInstanceConstantFactoryNew* Factory =
				NewObject<UMaterialInstanceConstantFactoryNew>();
			Factory->InitialParent = Parent;
			Result = Cast<UMaterialInstanceConstant>(AssetTools.CreateAsset(
				Name, Path, UMaterialInstanceConstant::StaticClass(), Factory));
		}
		if (!IsValid(Result) || !IsValid(Parent)) return nullptr;
		Result->SetParentEditorOnly(Parent);
		Result->ClearParameterValuesEditorOnly();
		Result->SetVectorParameterValueEditorOnly(
			FMaterialParameterInfo(TEXT("LiquidDeepColor")), DeepColor.GetClamped(0.0f, 1.0f));
		Result->SetVectorParameterValueEditorOnly(
			FMaterialParameterInfo(TEXT("LiquidShallowColor")), ShallowColor.GetClamped(0.0f, 1.0f));
		Result->SetVectorParameterValueEditorOnly(
			FMaterialParameterInfo(TEXT("LiquidEmissiveColor")), EmissiveColor.GetClamped(0.0f, 1.25f));
		Result->SetScalarParameterValueEditorOnly(
			FMaterialParameterInfo(TEXT("Opacity")), FMath::Clamp(Opacity, 0.05f, 0.68f));
		Result->SetScalarParameterValueEditorOnly(
			FMaterialParameterInfo(TEXT("Roughness")), FMath::Clamp(Roughness, 0.0f, 1.0f));
		Result->SetScalarParameterValueEditorOnly(
			FMaterialParameterInfo(TEXT("Metallic")), FMath::Clamp(Metallic, 0.0f, 1.0f));
		Result->SetScalarParameterValueEditorOnly(
			FMaterialParameterInfo(TEXT("Specular")), FMath::Clamp(Specular, 0.0f, 1.0f));
		Result->PostEditChange();
		return SaveAsset(Result) ? Result : nullptr;
	}

	void ApplyDefinition(
		UMaterialInstanceConstant* Material, EAPSPlanetSurfaceArchetype Archetype,
		const FAPSPlanetSurfaceArchetypeDefinition& D)
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
		// Full-scale families inherit the project-owned canonical WorldScape graph.
		// Keep compatibility writes for old material variants, while also authoring
		// every parameter consumed by the canonical grid-free material.
		const float BiomeContrast = static_cast<float>(
			(D.BiomeContrast.X + D.BiomeContrast.Y) * 0.5);
		const float MeanRoughness = FMath::Clamp(static_cast<float>(
			(D.Roughness.X + D.Roughness.Y) * 0.5), 0.0f, 1.0f);
		const float MeanMetallic = FMath::Clamp(static_cast<float>(
			(D.Metallic.X + D.Metallic.Y) * 0.5), 0.0f, 1.0f);
		float PaletteGainValue = 1.0f;
		float PaletteLiftValue = 0.0f;
		switch (Archetype)
		{
		case EAPSPlanetSurfaceArchetype::Rocky:
			PaletteGainValue = 1.38f;
			PaletteLiftValue = 0.018f;
			break;
		case EAPSPlanetSurfaceArchetype::Metallic:
			PaletteGainValue = 1.30f;
			PaletteLiftValue = 0.014f;
			break;
		default:
			break;
		}
		Scalar(TEXT("HeightContrast"), BiomeContrast);
		Scalar(TEXT("ContrastTemp"), BiomeContrast);
		Scalar(TEXT("WarpedScale"),
			UAPSPlanetSurfaceProfileResolver::ResolveMaterialWarpScale(D.MaterialFamily));
		Scalar(TEXT("ClimateBlend"),
			UAPSPlanetSurfaceProfileResolver::ResolveMaterialClimateBlend(Archetype));
		Scalar(TEXT("PaletteGain"), PaletteGainValue);
		Scalar(TEXT("PaletteLift"), PaletteLiftValue);
		Scalar(TEXT("MidVarient1Rough"), MeanRoughness);
		Scalar(TEXT("MidVarient2Rough"), MeanRoughness);
		Scalar(TEXT("MidVarient3Rough"), MeanRoughness);
		Scalar(TEXT("Roughness"), MeanRoughness);
		Scalar(TEXT("Metallic"), MeanMetallic);
		Scalar(TEXT("Specular"), FMath::Lerp(0.28f, 0.72f, MeanMetallic));
		Material->PostEditChange();
	}

	UMaterialInstanceConstant* CreateFamilyMaterial(
		IAssetTools& AssetTools, EAPSPlanetSurfaceArchetype Archetype,
		const FString& Name, UMaterialInterface* Parent,
		const FAPSPlanetSurfaceArchetypeDefinition& Definition)
	{
		UMaterialInstanceConstant* Result = LoadAsset<UMaterialInstanceConstant>(MaterialPath, Name);
		if (!IsValid(Result))
		{
			UMaterialInstanceConstantFactoryNew* Factory =
				NewObject<UMaterialInstanceConstantFactoryNew>();
			Factory->InitialParent = Parent;
			Result = Cast<UMaterialInstanceConstant>(AssetTools.CreateAsset(
				Name, MaterialPath, UMaterialInstanceConstant::StaticClass(), Factory));
		}
		if (!IsValid(Result) || !IsValid(Parent)) return nullptr;
		// These are the catalog's authoritative full-scale WorldScape materials. Every
		// family shares one topology-safe graph; only resolved palette and PBR values
		// differ. This prevents template UVs from exposing WorldScape cube-patch grids.
		Result->SetParentEditorOnly(Parent);
		Result->ClearParameterValuesEditorOnly();
		ApplyDefinition(Result, Archetype, Definition);
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

	// Build both presentations from the same grid-free terrain contract. Gameplay
	// uses the full-scale master below on the real WorldScape LOD meshes; the preview
	// asset remains separate only so hierarchy rendering can be tuned independently.
	UMaterial* WorldScapeTerrain = CreateWorldScapeTerrainMaterial(AssetTools);
	UMaterial* OrbitalTerrain = CreateOrbitalTerrainMaterial(AssetTools);
	UMaterial* CanonicalLiquid = CreateCanonicalLiquidMaterial(AssetTools);
	UMaterial* PreviewGuide = CreatePreviewGuideMaterial(AssetTools);
	if (!WorldScapeTerrain || !OrbitalTerrain || !CanonicalLiquid || !PreviewGuide)
	{
		UE_LOG(LogTemp, Error,
			TEXT("[APS.PlanetSurfaceAssets] Failed to create canonical WorldScape/preview masters"));
		return 2;
	}

	struct FFamilySpec
	{
		EAPSPlanetSurfaceArchetype Archetype;
		const TCHAR* Name;
	};
	const FFamilySpec Families[] =
	{
		{EAPSPlanetSurfaceArchetype::Rocky, TEXT("MI_APS_WS_Rocky")},
		{EAPSPlanetSurfaceArchetype::Temperate, TEXT("MI_APS_WS_Temperate")},
		{EAPSPlanetSurfaceArchetype::Oceanic, TEXT("MI_APS_WS_Oceanic")},
		{EAPSPlanetSurfaceArchetype::Biosphere, TEXT("MI_APS_WS_Biosphere")},
		{EAPSPlanetSurfaceArchetype::Desert, TEXT("MI_APS_WS_Desert")},
		{EAPSPlanetSurfaceArchetype::Cryogenic, TEXT("MI_APS_WS_Cryogenic")},
		{EAPSPlanetSurfaceArchetype::Magmatic, TEXT("MI_APS_WS_Magmatic")},
		{EAPSPlanetSurfaceArchetype::Metallic, TEXT("MI_APS_WS_Metallic")},
		{EAPSPlanetSurfaceArchetype::ExoticChemical, TEXT("MI_APS_WS_ExoticChemical")}
	};

	TMap<EAPSPlanetSurfaceArchetype, UMaterialInstanceConstant*> FamilyMaterials;
	for (const FFamilySpec& Family : Families)
	{
		const FAPSPlanetSurfaceArchetypeDefinition Definition =
			UAPSPlanetSurfaceProfileResolver::GetNativeDefinition(Family.Archetype);
		UMaterialInstanceConstant* Material = CreateFamilyMaterial(
			AssetTools, Family.Archetype, Family.Name, WorldScapeTerrain, Definition);
		if (!Material)
		{
			UE_LOG(LogTemp, Error, TEXT("[APS.PlanetSurfaceAssets] Failed to create %s"), Family.Name);
			return 3;
		}
		FamilyMaterials.Add(Family.Archetype, Material);
	}

	// These three MICs are the catalog's authoritative WorldScape ocean materials.
	// Keep each liquid visually distinct while every scalar stays inside the master
	// graph's display-safe clamps.
	UMaterialInstanceConstant* Water = CreateLiquidPreset(
		AssetTools, MaterialPath, TEXT("MI_APS_WS_Water"), CanonicalLiquid,
		FLinearColor(0.005f, 0.025f, 0.090f), FLinearColor(0.030f, 0.300f, 0.580f),
		FLinearColor(0.008f, 0.035f, 0.075f), 0.34f, 0.18f, 0.0f, 0.62f);
	UMaterialInstanceConstant* Ammonia = CreateLiquidPreset(
		AssetTools, MaterialPath, TEXT("MI_APS_WS_Ammonia"), CanonicalLiquid,
		FLinearColor(0.008f, 0.055f, 0.025f), FLinearColor(0.160f, 0.480f, 0.250f),
		FLinearColor(0.006f, 0.035f, 0.015f), 0.32f, 0.22f, 0.0f, 0.58f);
	UMaterialInstanceConstant* Lava = CreateLiquidPreset(
		AssetTools, MaterialPath, TEXT("MI_APS_WS_Lava"), CanonicalLiquid,
		FLinearColor(0.055f, 0.001f, 0.0005f), FLinearColor(0.720f, 0.025f, 0.001f),
		FLinearColor(0.420f, 0.018f, 0.001f), 0.42f, 0.42f, 0.04f, 0.30f);
	if (!Water || !Ammonia || !Lava) return 4;

	// The hierarchy globe assets share the exact master used by WorldScape. They
	// remain separate MICs only because their small-scale presentation needs more
	// opacity than a near-field gameplay ocean.
	UMaterialInstanceConstant* OrbitalWater = CreateLiquidPreset(
		AssetTools, PreviewMaterialPath, TEXT("MI_APS_OrbitalLiquid_Water"), CanonicalLiquid,
		FLinearColor(0.005f, 0.025f, 0.090f), FLinearColor(0.030f, 0.300f, 0.580f),
		FLinearColor(0.008f, 0.035f, 0.075f), 0.52f, 0.18f, 0.0f, 0.62f);
	UMaterialInstanceConstant* OrbitalAmmonia = CreateLiquidPreset(
		AssetTools, PreviewMaterialPath, TEXT("MI_APS_OrbitalLiquid_Ammonia"), CanonicalLiquid,
		FLinearColor(0.008f, 0.055f, 0.025f), FLinearColor(0.160f, 0.480f, 0.250f),
		FLinearColor(0.006f, 0.035f, 0.015f), 0.50f, 0.22f, 0.0f, 0.58f);
	UMaterialInstanceConstant* OrbitalLava = CreateLiquidPreset(
		AssetTools, PreviewMaterialPath, TEXT("MI_APS_OrbitalLiquid_Lava"), CanonicalLiquid,
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

	UE_LOG(LogTemp, Display, TEXT("[APS.PlanetSurfaceAssets] Updated four project-owned WorldScape/preview masters, 9 terrain instances, 3 gameplay liquid instances, 3 hierarchy liquid instances and catalog under %s"),
		*RootPath);
	return 0;
}

#endif
