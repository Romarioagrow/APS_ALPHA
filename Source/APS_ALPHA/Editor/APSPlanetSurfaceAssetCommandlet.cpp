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
#include "Materials/MaterialExpressionConstant.h"
#include "Materials/MaterialExpressionDesaturation.h"
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

	UMaterialExpression* AddActorPositionExpression(UMaterial* Material, int32 X, int32 Y)
	{
		// UMaterialExpressionActorPositionWS is intentionally not ENGINE_API in UE 5.4,
		// so referencing StaticClass() from a project module compiles but fails to link.
		// Resolve its registered UClass dynamically and still create the native material
		// node through the supported editor library. EPositionOrigin::Absolute is zero and
		// therefore the node's native default, preserving the LWC-safe actor centre.
		UClass* ExpressionClass = FindObject<UClass>(
			nullptr, TEXT("/Script/Engine.MaterialExpressionActorPositionWS"));
		if (!IsValid(ExpressionClass))
		{
			ExpressionClass = LoadObject<UClass>(
				nullptr, TEXT("/Script/Engine.MaterialExpressionActorPositionWS"));
		}
		return IsValid(ExpressionClass)
			? UMaterialEditingLibrary::CreateMaterialExpression(Material, ExpressionClass, X, Y)
			: nullptr;
	}

	UMaterialExpression* AddObjectPositionExpression(UMaterial* Material, int32 X, int32 Y)
	{
		// Keep the commandlet link-safe in the same way as ActorPositionWS. The object
		// centre is required only by the single closed orbital component; physical
		// WorldScape patches continue to use their shared actor/root centre.
		UClass* ExpressionClass = FindObject<UClass>(
			nullptr, TEXT("/Script/Engine.MaterialExpressionObjectPositionWS"));
		if (!IsValid(ExpressionClass))
		{
			ExpressionClass = LoadObject<UClass>(
				nullptr, TEXT("/Script/Engine.MaterialExpressionObjectPositionWS"));
		}
		return IsValid(ExpressionClass)
			? UMaterialEditingLibrary::CreateMaterialExpression(Material, ExpressionClass, X, Y)
			: nullptr;
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
		UMaterialExpressionVectorParameter* Slope = AddVectorParameter(
			Material, TEXT("SlopeColor"), FLinearColor(0.10f, 0.09f, 0.08f), -1500, -100, 6);
		UMaterialExpressionVectorParameter* Emissive = AddVectorParameter(
			Material, TEXT("EmissiveColor"), FLinearColor::Black, -800, 650, 7);
		UMaterialExpressionScalarParameter* ClimateBlend = AddScalarParameter(
			Material, TEXT("ClimateBlend"), 0.10f, 0.03f, 0.24f, -500, 300, 10);
		UMaterialExpressionScalarParameter* Roughness = AddScalarParameter(
			Material, TEXT("Roughness"), 0.72f, 0.0f, 1.0f, 200, 300, 11);
		UMaterialExpressionScalarParameter* Metallic = AddScalarParameter(
			Material, TEXT("Metallic"), 0.04f, 0.0f, 1.0f, 200, 400, 12);
		UMaterialExpressionScalarParameter* Specular = AddScalarParameter(
			Material, TEXT("Specular"), 0.35f, 0.0f, 1.0f, 200, 500, 13);
		UMaterialExpressionScalarParameter* PaletteGain = AddScalarParameter(
			Material, TEXT("PaletteGain"), 1.0f, 0.75f, 1.45f, -180, 300, 14);
		UMaterialExpressionScalarParameter* PaletteLift = AddScalarParameter(
			Material, TEXT("PaletteLift"), 0.0f, 0.0f, 0.035f, -180, 400, 15);
		UMaterialExpressionScalarParameter* PaletteSaturation = AddScalarParameter(
			Material, TEXT("PaletteSaturation"), 1.0f, 0.65f, 1.40f, -180, 500, 16);
		UMaterialExpressionScalarParameter* PaletteContrast = AddScalarParameter(
			Material, TEXT("PaletteContrast"), 1.0f, 0.75f, 1.40f, -180, 600, 17);
		UMaterialExpressionScalarParameter* SlopeTintStrength = AddScalarParameter(
			Material, TEXT("SlopeTintStrength"), 0.20f, 0.0f, 0.35f, -180, 700, 18);
		if (!Vertex || !Coast || !Lowland || !MidLowland || !Highland || !Dryland || !Peak
			|| !Slope || !Emissive || !ClimateBlend || !Roughness || !Metallic || !Specular
			|| !PaletteGain || !PaletteLift || !PaletteSaturation || !PaletteContrast
			|| !SlopeTintStrength)
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

		// The scaled presentation stays on the same real WorldScape mesh and keeps its
		// authoritative height/climate vertex payload. Root-relative coordinates below
		// are used only for seamless material detail and normals; they must never replace
		// the generated continents with an unrelated decorative shader pattern.
		UMaterialExpressionWorldPosition* WorldPosition = nullptr;
		UMaterialExpression* OrbitalCenterWS = nullptr;
		UMaterialExpressionSubtract* RootRelativePosition = nullptr;
		UMaterialExpressionNormalize* RadialNormal = nullptr;
		UMaterialExpressionScalarParameter* OrbitalPresentationBlend = nullptr;
		UMaterialExpressionVectorParameter* TerrainSeedOffset = nullptr;
		UMaterialExpression* PaletteSourceColor = FinalColor;
		if (bEnableNearFieldWorldDetail)
		{
			WorldPosition = AddExpression<UMaterialExpressionWorldPosition>(Material, 120, -1220);
			// ActorPositionWS resolves through the top render attachment, which is the
			// single AWorldScapeRoot for every Main/PatchA/PatchB mesh section. Unlike a
			// float vector parameter it is LWC-safe and cannot retain a stale centre after
			// preview swaps. ObjectPositionWS must not be used here because that is a
			// streamed patch centre.
			OrbitalCenterWS = AddActorPositionExpression(Material, 120, -1140);
			RootRelativePosition = AddExpression<UMaterialExpressionSubtract>(Material, 360, -1180);
			RadialNormal = AddExpression<UMaterialExpressionNormalize>(Material, 560, -1180);
			TerrainSeedOffset = AddVectorParameter(
				Material, TEXT("OrbitalSeedOffset"), FLinearColor(3.1f, 7.7f, 11.3f, 0.0f),
				360, -1100, 30);
			UMaterialExpressionScalarParameter* OrbitalFeatureScale = AddScalarParameter(
				Material, TEXT("OrbitalFeatureScale"), 3.2f, 1.2f, 9.0f,
				560, -1100, 31);
			UMaterialExpressionScalarParameter* OrbitalDetailScale = AddScalarParameter(
				Material, TEXT("OrbitalDetailScale"), 8.5f, 3.0f, 24.0f,
				560, -1040, 32);
			OrbitalPresentationBlend = AddScalarParameter(
				Material, TEXT("OrbitalPresentationBlend"), 0.0f, 0.0f, 1.0f,
				560, -980, 33);

			UMaterialExpressionMultiply* OrbitalMacroPosition =
				AddExpression<UMaterialExpressionMultiply>(Material, 780, -1160);
			UMaterialExpressionAdd* SeededOrbitalMacroPosition =
				AddExpression<UMaterialExpressionAdd>(Material, 980, -1160);
			UMaterialExpressionNoise* OrbitalMacroNoise =
				AddExpression<UMaterialExpressionNoise>(Material, 1180, -1160);
			UMaterialExpressionMultiply* OrbitalDetailPosition =
				AddExpression<UMaterialExpressionMultiply>(Material, 780, -1040);
			UMaterialExpressionAdd* SeededOrbitalDetailPosition =
				AddExpression<UMaterialExpressionAdd>(Material, 980, -1040);
			UMaterialExpressionNoise* OrbitalDetailNoise =
				AddExpression<UMaterialExpressionNoise>(Material, 1180, -1040);
			UMaterialExpressionMultiply* WeightedOrbitalMacro =
				AddExpression<UMaterialExpressionMultiply>(Material, 1400, -1140);
			UMaterialExpressionMultiply* WeightedOrbitalDetail =
				AddExpression<UMaterialExpressionMultiply>(Material, 1400, -1040);
			UMaterialExpressionAdd* CombinedOrbitalField =
				AddExpression<UMaterialExpressionAdd>(Material, 1600, -1100);
			UMaterialExpressionClamp* BoundedOrbitalField =
				AddExpression<UMaterialExpressionClamp>(Material, 1800, -1100);

			if (!WorldPosition || !OrbitalCenterWS || !RootRelativePosition || !RadialNormal
				|| !TerrainSeedOffset || !OrbitalFeatureScale || !OrbitalDetailScale
				|| !OrbitalPresentationBlend || !OrbitalMacroPosition
				|| !SeededOrbitalMacroPosition || !OrbitalMacroNoise
				|| !OrbitalDetailPosition || !SeededOrbitalDetailPosition
				|| !OrbitalDetailNoise || !WeightedOrbitalMacro
				|| !WeightedOrbitalDetail || !CombinedOrbitalField || !BoundedOrbitalField)
			{
				return nullptr;
			}

			WorldPosition->WorldPositionShaderOffset = WPT_ExcludeAllShaderOffsets;
			RootRelativePosition->A.Connect(0, WorldPosition);
			RootRelativePosition->B.Connect(0, OrbitalCenterWS);
			RadialNormal->VectorInput.Connect(0, RootRelativePosition);
			OrbitalMacroPosition->A.Connect(0, RadialNormal);
			OrbitalMacroPosition->B.Connect(0, OrbitalFeatureScale);
			SeededOrbitalMacroPosition->A.Connect(0, OrbitalMacroPosition);
			SeededOrbitalMacroPosition->B.Connect(0, TerrainSeedOffset);
			OrbitalDetailPosition->A.Connect(0, RadialNormal);
			OrbitalDetailPosition->B.Connect(0, OrbitalDetailScale);
			SeededOrbitalDetailPosition->A.Connect(0, OrbitalDetailPosition);
			SeededOrbitalDetailPosition->B.Connect(0, TerrainSeedOffset);

			auto ConfigureOrbitalNoise = [](UMaterialExpressionNoise* Noise)
			{
				Noise->WorldPositionOriginType = EPositionOrigin::Absolute;
				Noise->Scale = 1.0f;
				Noise->Quality = 1;
				Noise->NoiseFunction = NOISEFUNCTION_SimplexTex;
				Noise->bTurbulence = false;
				Noise->Levels = 4;
				Noise->OutputMin = 0.0f;
				Noise->OutputMax = 1.0f;
				Noise->bTiling = false;
			};
			OrbitalMacroNoise->Position.Connect(0, SeededOrbitalMacroPosition);
			ConfigureOrbitalNoise(OrbitalMacroNoise);
			OrbitalDetailNoise->Position.Connect(0, SeededOrbitalDetailPosition);
			ConfigureOrbitalNoise(OrbitalDetailNoise);
			OrbitalDetailNoise->Levels = 3;
			WeightedOrbitalMacro->A.Connect(0, OrbitalMacroNoise);
			WeightedOrbitalMacro->ConstB = 0.78f;
			WeightedOrbitalDetail->A.Connect(0, OrbitalDetailNoise);
			WeightedOrbitalDetail->ConstB = 0.22f;
			CombinedOrbitalField->A.Connect(0, WeightedOrbitalMacro);
			CombinedOrbitalField->B.Connect(0, WeightedOrbitalDetail);
			BoundedOrbitalField->Input.Connect(0, CombinedOrbitalField);
			BoundedOrbitalField->ClampMode = CMODE_Clamp;
			BoundedOrbitalField->MinDefault = 0.0f;
			BoundedOrbitalField->MaxDefault = 1.0f;

			UMaterialExpressionSmoothStep* OrbitalCoastToLowBand = AddSmoothStep(
				Material, BoundedOrbitalField, 0, 0.10f, 0.38f, 2000, -1240);
			UMaterialExpressionSmoothStep* OrbitalLowToMidBand = AddSmoothStep(
				Material, BoundedOrbitalField, 0, 0.26f, 0.56f, 2000, -1160);
			UMaterialExpressionSmoothStep* OrbitalMidToHighBand = AddSmoothStep(
				Material, BoundedOrbitalField, 0, 0.42f, 0.70f, 2000, -1080);
			UMaterialExpressionSmoothStep* OrbitalHighToDryBand = AddSmoothStep(
				Material, BoundedOrbitalField, 0, 0.58f, 0.84f, 2000, -1000);
			UMaterialExpressionSmoothStep* OrbitalDryToPeakBand = AddSmoothStep(
				Material, BoundedOrbitalField, 0, 0.74f, 0.97f, 2000, -920);
			UMaterialExpressionLinearInterpolate* OrbitalCoastLow = AddLerp(
				Material, Coast, Lowland, OrbitalCoastToLowBand, 0, 2220, -1240);
			UMaterialExpressionLinearInterpolate* OrbitalLowMid = AddLerp(
				Material, OrbitalCoastLow, MidLowland, OrbitalLowToMidBand, 0, 2420, -1160);
			UMaterialExpressionLinearInterpolate* OrbitalMidHigh = AddLerp(
				Material, OrbitalLowMid, Highland, OrbitalMidToHighBand, 0, 2620, -1080);
			UMaterialExpressionLinearInterpolate* OrbitalHighDry = AddLerp(
				Material, OrbitalMidHigh, Dryland, OrbitalHighToDryBand, 0, 2820, -1000);
			UMaterialExpressionLinearInterpolate* OrbitalHeightRamp = AddLerp(
				Material, OrbitalHighDry, Peak, OrbitalDryToPeakBand, 0, 3020, -1080);
			UMaterialExpressionLinearInterpolate* PresentationColor = AddLerp(
				Material, FinalColor, OrbitalHeightRamp, OrbitalPresentationBlend, 0,
				3220, -1080);
			if (!OrbitalCoastToLowBand || !OrbitalLowToMidBand || !OrbitalMidToHighBand
				|| !OrbitalHighToDryBand || !OrbitalDryToPeakBand || !OrbitalCoastLow
				|| !OrbitalLowMid || !OrbitalMidHigh || !OrbitalHighDry
				|| !OrbitalHeightRamp || !PresentationColor)
			{
				return nullptr;
			}
			// Keep the CPU WorldScape classification authoritative. The synthetic branch is
			// retained only as a migration-safe graph input for old instances and is pruned
			// by the material compiler because it does not feed the output.
			PaletteSourceColor = FinalColor;
		}
		// Family palettes are authored in linear space. Preserve their hue separation
		// before exposure compensation: the old gain-only correction lifted every band
		// toward the same milky grey from orbit and still collapsed to near-black at
		// ground level. Saturation and centered contrast remain inexpensive scalar
		// operations and are authored per family/profile below.
		UMaterialExpressionConstant* FullDesaturation =
			AddExpression<UMaterialExpressionConstant>(Material, 150, -120);
		UMaterialExpressionDesaturation* PaletteLuminance =
			AddExpression<UMaterialExpressionDesaturation>(Material, 340, -120);
		UMaterialExpressionSubtract* PaletteChrominance =
			AddExpression<UMaterialExpressionSubtract>(Material, 540, -80);
		UMaterialExpressionMultiply* SaturatedChrominance =
			AddExpression<UMaterialExpressionMultiply>(Material, 740, -80);
		UMaterialExpressionAdd* SaturatedPaletteColor =
			AddExpression<UMaterialExpressionAdd>(Material, 940, -160);
		UMaterialExpressionSubtract* CenteredPaletteColor =
			AddExpression<UMaterialExpressionSubtract>(Material, 1140, -160);
		UMaterialExpressionMultiply* ContrastedPaletteTerm =
			AddExpression<UMaterialExpressionMultiply>(Material, 1340, -160);
		UMaterialExpressionAdd* ContrastedPaletteColor =
			AddExpression<UMaterialExpressionAdd>(Material, 1540, -160);
		UMaterialExpressionMultiply* GainedPaletteColor =
			AddExpression<UMaterialExpressionMultiply>(Material, 1740, -160);
		UMaterialExpressionAdd* LiftedPaletteColor =
			AddExpression<UMaterialExpressionAdd>(Material, 1940, -160);
		UMaterialExpressionClamp* BoundedPaletteColor =
			AddExpression<UMaterialExpressionClamp>(Material, 2140, -160);
		if (!FullDesaturation || !PaletteLuminance || !PaletteChrominance
			|| !SaturatedChrominance || !SaturatedPaletteColor || !CenteredPaletteColor
			|| !ContrastedPaletteTerm || !ContrastedPaletteColor || !GainedPaletteColor
			|| !LiftedPaletteColor || !BoundedPaletteColor)
		{
			return nullptr;
		}
		FullDesaturation->R = 1.0f;
		PaletteLuminance->Input.Connect(0, PaletteSourceColor);
		PaletteLuminance->Fraction.Connect(0, FullDesaturation);
		PaletteChrominance->A.Connect(0, PaletteSourceColor);
		PaletteChrominance->B.Connect(0, PaletteLuminance);
		SaturatedChrominance->A.Connect(0, PaletteChrominance);
		SaturatedChrominance->B.Connect(0, PaletteSaturation);
		SaturatedPaletteColor->A.Connect(0, PaletteLuminance);
		SaturatedPaletteColor->B.Connect(0, SaturatedChrominance);
		CenteredPaletteColor->A.Connect(0, SaturatedPaletteColor);
		// Palettes are already linear. Contrasting around 0.5 clipped most dark rocky,
		// forest and metallic colours to black. A low-albedo pivot preserves chroma and
		// still gives the authored contrast control useful headroom.
		// Most terrain swatches are deliberately low-albedo linear colours (for example
		// forest lowlands are around 0.01).  A photographic 0.18 pivot drove those hues
		// below zero before the clamp and made unrelated families converge to grey/black.
		// Keep contrast centred inside the actual terrain range so authored greens,
		// ochres, ice blues and iron reds survive both orbit and ground lighting.
		constexpr float LinearPaletteContrastPivot = 0.06f;
		CenteredPaletteColor->ConstB = LinearPaletteContrastPivot;
		ContrastedPaletteTerm->A.Connect(0, CenteredPaletteColor);
		ContrastedPaletteTerm->B.Connect(0, PaletteContrast);
		ContrastedPaletteColor->A.Connect(0, ContrastedPaletteTerm);
		ContrastedPaletteColor->ConstB = LinearPaletteContrastPivot;
		GainedPaletteColor->A.Connect(0, ContrastedPaletteColor);
		GainedPaletteColor->B.Connect(0, PaletteGain);
		LiftedPaletteColor->A.Connect(0, GainedPaletteColor);
		LiftedPaletteColor->B.Connect(0, PaletteLift);
		BoundedPaletteColor->Input.Connect(0, LiftedPaletteColor);
		BoundedPaletteColor->ClampMode = CMODE_Clamp;
		BoundedPaletteColor->MinDefault = 0.0f;
		BoundedPaletteColor->MaxDefault = 1.0f;

		// The resolved family palette is authoritative in both gameplay and PLANET.
		// Preview exposure belongs to the preview lighting rig, not a second material
		// colour transform; otherwise the same body no longer matches at full scale.
		UMaterialExpression* SurfaceColor = BoundedPaletteColor;
		UMaterialExpression* SurfaceRoughness = Roughness;
		UMaterialExpression* SurfaceNormal = nullptr;
		if (bEnableNearFieldWorldDetail)
		{
			// WorldScape is assembled from independently streamed cube patches. UV- or
			// object-local detail exposes those patch boundaries immediately, especially
			// at LOD transitions. Three non-periodic absolute-world bands keep one visual
			// identity from orbit to the ground: planetary macro variation, kilometre-scale
			// meso structure and a distance-faded near gradient. None of them displaces a
			// second surface, so collision and the visible material remain the same real
			// WorldScape mesh.
			UMaterialExpressionScalarParameter* MacroScale = AddScalarParameter(
				Material, TEXT("MacroDetailScaleCm"), 8000000.0f, 1500000.0f, 20000000.0f,
				120, -880, 20);
			UMaterialExpressionScalarParameter* MesoScale = AddScalarParameter(
				Material, TEXT("MesoDetailScaleCm"), 450000.0f, 100000.0f, 1800000.0f,
				120, -830, 21);
			UMaterialExpressionScalarParameter* NearScale = AddScalarParameter(
				Material, TEXT("NearDetailScaleCm"), 7000.0f, 1200.0f, 30000.0f,
				120, -780, 22);
			UMaterialExpressionScalarParameter* MacroColorStrength = AddScalarParameter(
				Material, TEXT("MacroColorStrength"), 0.060f, 0.0f, 0.14f,
				120, -680, 23);
			UMaterialExpressionScalarParameter* MesoColorStrength = AddScalarParameter(
				Material, TEXT("MesoColorStrength"), 0.042f, 0.0f, 0.12f,
				120, -630, 24);
			UMaterialExpressionScalarParameter* NearColorStrength = AddScalarParameter(
				Material, TEXT("NearColorStrength"), 0.022f, 0.0f, 0.10f,
				120, -580, 25);
			UMaterialExpressionScalarParameter* DetailNormalStrength = AddScalarParameter(
				Material, TEXT("DetailNormalStrength"), 0.060f, 0.0f, 0.18f,
				120, -480, 26);
			UMaterialExpressionScalarParameter* MesoRoughnessStrength = AddScalarParameter(
				Material, TEXT("MesoRoughnessStrength"), 0.032f, 0.0f, 0.10f,
				120, -430, 27);
			UMaterialExpressionScalarParameter* DetailRoughnessStrength = AddScalarParameter(
				Material, TEXT("DetailRoughnessStrength"), 0.025f, 0.0f, 0.12f,
				120, -380, 28);
			UMaterialExpressionDivide* MacroPosition =
				AddExpression<UMaterialExpressionDivide>(Material, 400, -920);
			UMaterialExpressionDivide* MesoPosition =
				AddExpression<UMaterialExpressionDivide>(Material, 400, -840);
			UMaterialExpressionDivide* NearPosition =
				AddExpression<UMaterialExpressionDivide>(Material, 400, -760);
			UMaterialExpressionAdd* SeededMacroPosition =
				AddExpression<UMaterialExpressionAdd>(Material, 510, -960);
			UMaterialExpressionAdd* SeededMesoPosition =
				AddExpression<UMaterialExpressionAdd>(Material, 510, -860);
			UMaterialExpressionAdd* SeededNearPosition =
				AddExpression<UMaterialExpressionAdd>(Material, 510, -760);
			UMaterialExpressionNoise* MacroNoise =
				AddExpression<UMaterialExpressionNoise>(Material, 620, -920);
			UMaterialExpressionNoise* MesoNoise =
				AddExpression<UMaterialExpressionNoise>(Material, 620, -820);
			UMaterialExpressionVectorNoise* NearNoise =
				AddExpression<UMaterialExpressionVectorNoise>(Material, 620, -720);
			UMaterialExpressionComponentMask* NearGradient =
				AddExpression<UMaterialExpressionComponentMask>(Material, 850, -770);
			UMaterialExpressionComponentMask* NearScalar =
				AddExpression<UMaterialExpressionComponentMask>(Material, 850, -650);
			UMaterialExpressionSubtract* CenteredNearScalar =
				AddExpression<UMaterialExpressionSubtract>(Material, 1040, -650);
			UMaterialExpressionPixelDepth* PixelDepth =
				AddExpression<UMaterialExpressionPixelDepth>(Material, 620, -500);
			UMaterialExpressionSmoothStep* FarDetailFade = AddSmoothStep(
				Material, PixelDepth, 0, 5000.0f, 500000.0f, 830, -500);
			UMaterialExpressionOneMinus* NearFieldFade =
				AddExpression<UMaterialExpressionOneMinus>(Material, 1040, -500);
			UMaterialExpressionMultiply* MesoEnvelopeVariation =
				AddExpression<UMaterialExpressionMultiply>(Material, 830, -420);
			UMaterialExpressionAdd* NearDetailEnvelope =
				AddExpression<UMaterialExpressionAdd>(Material, 1040, -420);
			UMaterialExpressionMultiply* EnvelopedNearFieldFade =
				AddExpression<UMaterialExpressionMultiply>(Material, 1240, -500);

			UMaterialExpressionMultiply* MacroColorTerm =
				AddExpression<UMaterialExpressionMultiply>(Material, 1050, -900);
			UMaterialExpressionMultiply* MesoColorTerm =
				AddExpression<UMaterialExpressionMultiply>(Material, 1050, -800);
			UMaterialExpressionAdd* MacroMesoColorTerms =
				AddExpression<UMaterialExpressionAdd>(Material, 1260, -850);
			UMaterialExpressionMultiply* FadedNearColorStrength =
				AddExpression<UMaterialExpressionMultiply>(Material, 1050, -600);
			UMaterialExpressionMultiply* NearColorTerm =
				AddExpression<UMaterialExpressionMultiply>(Material, 1270, -650);
			UMaterialExpressionAdd* DetailColorTerms =
				AddExpression<UMaterialExpressionAdd>(Material, 1470, -720);
			UMaterialExpressionAdd* DetailColorScale =
				AddExpression<UMaterialExpressionAdd>(Material, 1650, -780);
			UMaterialExpressionMultiply* DetailedColor =
				AddExpression<UMaterialExpressionMultiply>(Material, 1840, -500);
			UMaterialExpressionClamp* BoundedDetailedColor =
				AddExpression<UMaterialExpressionClamp>(Material, 2040, -500);

			UMaterialExpressionMultiply* MesoRoughnessVariation =
				AddExpression<UMaterialExpressionMultiply>(Material, 1050, -340);
			UMaterialExpressionMultiply* FadedRoughnessStrength =
				AddExpression<UMaterialExpressionMultiply>(Material, 1050, -280);
			UMaterialExpressionMultiply* NearRoughnessVariation =
				AddExpression<UMaterialExpressionMultiply>(Material, 1270, -280);
			UMaterialExpressionAdd* RoughnessVariation =
				AddExpression<UMaterialExpressionAdd>(Material, 1450, -320);
			UMaterialExpressionAdd* DetailedRoughness =
				AddExpression<UMaterialExpressionAdd>(Material, 1650, -260);
			UMaterialExpressionClamp* BoundedDetailedRoughness =
				AddExpression<UMaterialExpressionClamp>(Material, 1850, -260);

			UMaterialExpressionVertexNormalWS* VertexNormal =
				AddExpression<UMaterialExpressionVertexNormalWS>(Material, 1050, -80);
			UMaterialExpressionScalarParameter* OrbitalNormalBlend = AddScalarParameter(
				Material, TEXT("OrbitalNormalBlend"), 0.0f, 0.0f, 1.0f,
				1050, 140, 30);
			UMaterialExpressionLinearInterpolate* BaseWorldNormal = AddLerp(
				Material, VertexNormal, RadialNormal, OrbitalNormalBlend, 0,
				1460, 40);
			UMaterialExpressionDotProduct* RadialGradient =
				AddExpression<UMaterialExpressionDotProduct>(Material, 1660, -80);
			UMaterialExpressionMultiply* RadialGradientVector =
				AddExpression<UMaterialExpressionMultiply>(Material, 1860, -20);
			UMaterialExpressionSubtract* TangentGradient =
				AddExpression<UMaterialExpressionSubtract>(Material, 2060, -20);
			UMaterialExpressionMultiply* FadedNormalStrength =
				AddExpression<UMaterialExpressionMultiply>(Material, 1660, 100);
			UMaterialExpressionMultiply* NormalPerturbation =
				AddExpression<UMaterialExpressionMultiply>(Material, 2260, 40);
			UMaterialExpressionAdd* PerturbedNormal =
				AddExpression<UMaterialExpressionAdd>(Material, 2460, 40);
			UMaterialExpressionNormalize* NormalizedWorldNormal =
				AddExpression<UMaterialExpressionNormalize>(Material, 2660, 40);
			UMaterialExpressionDotProduct* SurfaceNormalAlignment =
				AddExpression<UMaterialExpressionDotProduct>(Material, 2060, -500);
			UMaterialExpressionOneMinus* SurfaceSteepness =
				AddExpression<UMaterialExpressionOneMinus>(Material, 2260, -500);
			// Begin the cue below the typical six-degree Frozen walk slope. The old
			// 0.008 onset only became visible above roughly seven degrees, so real
			// traversable relief was present in geometry/collision but read as flat.
			UMaterialExpressionSmoothStep* GeologicalSlopeMask = AddSmoothStep(
				Material, SurfaceSteepness, 0, 0.004f, 0.12f, 2460, -500);
			UMaterialExpressionMultiply* WeightedSlopeMask =
				AddExpression<UMaterialExpressionMultiply>(Material, 2660, -500);
			UMaterialExpressionLinearInterpolate* SlopeTintedColor = AddLerp(
				Material, BoundedDetailedColor, Slope, WeightedSlopeMask, 0,
				2860, -500);

			if (!WorldPosition || !MacroScale || !MesoScale || !NearScale
				|| !MacroColorStrength || !MesoColorStrength || !NearColorStrength
				|| !DetailNormalStrength || !MesoRoughnessStrength || !DetailRoughnessStrength
				|| !MacroPosition || !MesoPosition || !NearPosition
				|| !SeededMacroPosition || !SeededMesoPosition || !SeededNearPosition
				|| !MacroNoise || !MesoNoise || !NearNoise
				|| !NearGradient || !NearScalar || !CenteredNearScalar
				|| !PixelDepth || !FarDetailFade || !NearFieldFade
				|| !MesoEnvelopeVariation || !NearDetailEnvelope || !EnvelopedNearFieldFade
				|| !MacroColorTerm || !MesoColorTerm || !MacroMesoColorTerms
				|| !FadedNearColorStrength
				|| !NearColorTerm || !DetailColorTerms || !DetailColorScale
				|| !DetailedColor || !BoundedDetailedColor || !MesoRoughnessVariation
				|| !FadedRoughnessStrength || !NearRoughnessVariation || !RoughnessVariation
				|| !DetailedRoughness || !BoundedDetailedRoughness
				|| !VertexNormal || !OrbitalCenterWS || !RootRelativePosition || !RadialNormal
				|| !OrbitalNormalBlend || !BaseWorldNormal
				|| !RadialGradient || !RadialGradientVector
				|| !TangentGradient || !FadedNormalStrength || !NormalPerturbation
				|| !PerturbedNormal || !NormalizedWorldNormal || !SurfaceNormalAlignment
				|| !SurfaceSteepness || !GeologicalSlopeMask || !WeightedSlopeMask
				|| !SlopeTintedColor)
			{
				return nullptr;
			}

			MacroPosition->A.Connect(0, RootRelativePosition);
			MacroPosition->B.Connect(0, MacroScale);
			MesoPosition->A.Connect(0, RootRelativePosition);
			MesoPosition->B.Connect(0, MesoScale);
			NearPosition->A.Connect(0, RootRelativePosition);
			NearPosition->B.Connect(0, NearScale);
			// Phase every band from the resolved terrain seed. The noises remain
			// non-periodic, but different bodies no longer repeat the same local
			// pattern around their root-relative origin.
			SeededMacroPosition->A.Connect(0, MacroPosition);
			SeededMacroPosition->B.Connect(0, TerrainSeedOffset);
			SeededMesoPosition->A.Connect(0, MesoPosition);
			SeededMesoPosition->B.Connect(0, TerrainSeedOffset);
			SeededNearPosition->A.Connect(0, NearPosition);
			SeededNearPosition->B.Connect(0, TerrainSeedOffset);

			MacroNoise->Position.Connect(0, SeededMacroPosition);
			MacroNoise->WorldPositionOriginType = EPositionOrigin::Absolute;
			MacroNoise->Scale = 1.0f;
			MacroNoise->Quality = 1;
			MacroNoise->NoiseFunction = NOISEFUNCTION_SimplexTex;
			MacroNoise->bTurbulence = false;
			MacroNoise->Levels = 2;
			MacroNoise->OutputMin = -1.0f;
			MacroNoise->OutputMax = 1.0f;
			MacroNoise->bTiling = false;

			MesoNoise->Position.Connect(0, SeededMesoPosition);
			MesoNoise->WorldPositionOriginType = EPositionOrigin::Absolute;
			MesoNoise->Scale = 1.0f;
			MesoNoise->Quality = 1;
			MesoNoise->NoiseFunction = NOISEFUNCTION_SimplexTex;
			MesoNoise->bTurbulence = false;
			MesoNoise->Levels = 2;
			MesoNoise->OutputMin = -1.0f;
			MesoNoise->OutputMax = 1.0f;
			MesoNoise->bTiling = false;

			NearNoise->Position.Connect(0, SeededNearPosition);
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
			// GradientALU stores its scalar field in A in the 0..1 range. Centre it
			// before colour/roughness modulation so the near band adds real local
			// contrast instead of uniformly bleaching the ground under flat lighting.
			CenteredNearScalar->A.Connect(0, NearScalar);
			CenteredNearScalar->ConstB = 0.5f;
			NearFieldFade->Input.Connect(0, FarDetailFade);
			// A broad non-periodic meso envelope clusters the near response instead of
			// laying an equally strong micro pattern over every square metre. This reuses
			// an existing sample, so it breaks the wallpaper read without another noise
			// layer or any proxy geometry.
			MesoEnvelopeVariation->A.Connect(0, MesoNoise);
			MesoEnvelopeVariation->ConstB = 0.28f;
			NearDetailEnvelope->A.Connect(0, MesoEnvelopeVariation);
			NearDetailEnvelope->ConstB = 0.72f;
			EnvelopedNearFieldFade->A.Connect(0, NearFieldFade);
			EnvelopedNearFieldFade->B.Connect(0, NearDetailEnvelope);

			MacroColorTerm->A.Connect(0, MacroNoise);
			MacroColorTerm->B.Connect(0, MacroColorStrength);
			MesoColorTerm->A.Connect(0, MesoNoise);
			MesoColorTerm->B.Connect(0, MesoColorStrength);
			MacroMesoColorTerms->A.Connect(0, MacroColorTerm);
			MacroMesoColorTerms->B.Connect(0, MesoColorTerm);
			FadedNearColorStrength->A.Connect(0, NearColorStrength);
			FadedNearColorStrength->B.Connect(0, EnvelopedNearFieldFade);
			NearColorTerm->A.Connect(0, CenteredNearScalar);
			NearColorTerm->B.Connect(0, FadedNearColorStrength);
			DetailColorTerms->A.Connect(0, MacroMesoColorTerms);
			DetailColorTerms->B.Connect(0, NearColorTerm);
			DetailColorScale->A.Connect(0, DetailColorTerms);
			DetailColorScale->ConstB = 1.0f;
			DetailedColor->A.Connect(0, BoundedPaletteColor);
			DetailedColor->B.Connect(0, DetailColorScale);
			BoundedDetailedColor->Input.Connect(0, DetailedColor);
			BoundedDetailedColor->ClampMode = CMODE_Clamp;
			BoundedDetailedColor->MinDefault = 0.0f;
			BoundedDetailedColor->MaxDefault = 1.0f;

			MesoRoughnessVariation->A.Connect(0, MesoNoise);
			MesoRoughnessVariation->B.Connect(0, MesoRoughnessStrength);
			FadedRoughnessStrength->A.Connect(0, DetailRoughnessStrength);
			FadedRoughnessStrength->B.Connect(0, EnvelopedNearFieldFade);
			NearRoughnessVariation->A.Connect(0, CenteredNearScalar);
			NearRoughnessVariation->B.Connect(0, FadedRoughnessStrength);
			RoughnessVariation->A.Connect(0, MesoRoughnessVariation);
			RoughnessVariation->B.Connect(0, NearRoughnessVariation);
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
			// WorldScape streams independently-normalised cube patches. Their face normals
			// are correct at ground scale, but the tiny PLANET presentation can expose the
			// patch boundaries as a dark rectangular grid. ActorPositionWS supplies one
			// root centre for every attached patch; full-scale gameplay keeps the actual
			// terrain normal and all relief cues.
			RadialGradient->A.Connect(0, NearGradient);
			RadialGradient->B.Connect(0, BaseWorldNormal);
			RadialGradientVector->A.Connect(0, BaseWorldNormal);
			RadialGradientVector->B.Connect(0, RadialGradient);
			TangentGradient->A.Connect(0, NearGradient);
			TangentGradient->B.Connect(0, RadialGradientVector);
			FadedNormalStrength->A.Connect(0, DetailNormalStrength);
			FadedNormalStrength->B.Connect(0, EnvelopedNearFieldFade);
			NormalPerturbation->A.Connect(0, TangentGradient);
			NormalPerturbation->B.Connect(0, FadedNormalStrength);
			PerturbedNormal->A.Connect(0, BaseWorldNormal);
			PerturbedNormal->B.Connect(0, NormalPerturbation);
			NormalizedWorldNormal->VectorInput.Connect(0, PerturbedNormal);
			// This is a material cue on the authoritative displaced mesh, not synthetic
			// relief.  Comparing the streamed vertex normal to the root-radial normal
			// darkens only genuinely steep WorldScape faces and makes mountains/readable
			// crater walls survive the wide range of preview and gameplay exposure.
			SurfaceNormalAlignment->A.Connect(0, VertexNormal);
			SurfaceNormalAlignment->B.Connect(0, RadialNormal);
			SurfaceSteepness->Input.Connect(0, SurfaceNormalAlignment);
			WeightedSlopeMask->A.Connect(0, GeologicalSlopeMask);
			WeightedSlopeMask->B.Connect(0, SlopeTintStrength);

			SurfaceColor = SlopeTintedColor;
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
		UMaterialExpressionScalarParameter* TerrainAmbientFill = AddScalarParameter(
			Material, TEXT("TerrainAmbientFill"),
			bEnableNearFieldWorldDetail ? 0.09f : 0.035f,
			0.0f, 0.20f, 0, 820, 31);
		UMaterialExpressionMultiply* AmbientEmissive =
			AddExpression<UMaterialExpressionMultiply>(Material, 220, 760);
		UMaterialExpressionAdd* CombinedEmissive =
			AddExpression<UMaterialExpressionAdd>(Material, 440, 680);
		UMaterialExpressionClamp* BoundedCombinedEmissive =
			AddExpression<UMaterialExpressionClamp>(Material, 660, 680);
		if (!CoastToLowBand || !LowToMidBand || !MidToHighBand || !HighToDryBand
			|| !DryToPeakBand || !CoastLow || !LowMid || !MidHigh || !HighDry || !HeightRamp || !Thermal
			|| !Climate || !FinalColor || !BoundedEmissive || !EmissiveHeightBand
			|| !LowlandEmissiveMask || !MaskedEmissive || !TerrainAmbientFill
			|| !AmbientEmissive || !CombinedEmissive || !BoundedCombinedEmissive)
		{
			return nullptr;
		}
		BoundedEmissive->Input.Connect(0, Emissive);
		BoundedEmissive->ClampMode = CMODE_Clamp;
		BoundedEmissive->MinDefault = 0.0f;
		// Keep the canonical full-scale surface conservative so molten lowlands do not
		// turn most of the gameplay disk display-white after exposure/tonemapping.
		// Keep both paths below display-white. The old 1.25 orbital headroom flattened
		// lava into a clipped orange disk and erased the authoritative height payload.
		BoundedEmissive->MaxDefault = bEnableNearFieldWorldDetail ? 0.42f : 0.38f;
		LowlandEmissiveMask->Input.Connect(0, EmissiveHeightBand);
		MaskedEmissive->A.Connect(0, BoundedEmissive);
		// WorldScape reserves vertex alpha for its hole mask. The retired closed-globe
		// proxy used that channel as a water/emissive mask, which made the two renderers
		// fundamentally disagree. Derive lava glow from the shared normalized-height R
		// channel so the exact same material is valid on real WorldScape LOD meshes.
		// The deliberately broad 0.04..0.82 transition prevents emissive lava from
		// outlining individual vertices or LOD patches as a square/dotted grid.
		MaskedEmissive->B.Connect(0, LowlandEmissiveMask);
		// A small palette-derived fill keeps physically displaced terrain readable on
		// the night side and under the deliberately sparse generation preview rig. It
		// is shading on the same WorldScape mesh, never a proxy shell or visual height
		// layer, and the final clamp keeps HDR response bounded.
		AmbientEmissive->A.Connect(0, SurfaceColor);
		AmbientEmissive->B.Connect(0, TerrainAmbientFill);
		CombinedEmissive->A.Connect(0, MaskedEmissive);
		CombinedEmissive->B.Connect(0, AmbientEmissive);
		BoundedCombinedEmissive->Input.Connect(0, CombinedEmissive);
		BoundedCombinedEmissive->ClampMode = CMODE_Clamp;
		BoundedCombinedEmissive->MinDefault = 0.0f;
		BoundedCombinedEmissive->MaxDefault = 0.48f;

		const bool bNormalConnected = !SurfaceNormal
			|| UMaterialEditingLibrary::ConnectMaterialProperty(SurfaceNormal, TEXT(""), MP_Normal);
		const bool bConnected = bNormalConnected
			&& UMaterialEditingLibrary::ConnectMaterialProperty(SurfaceColor, TEXT(""), MP_BaseColor)
			&& UMaterialEditingLibrary::ConnectMaterialProperty(BoundedCombinedEmissive, TEXT(""), MP_EmissiveColor)
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

	UMaterial* CreateCanonicalLiquidMaterial(IAssetTools& AssetTools,
		const FString& Path, const TCHAR* AssetName, const bool bOrbitalPresentation)
	{
		// The commandlet now invokes only the orbital branch. Physical gameplay liquids
		// are thin MICs over WorldScape's procedural water/lava templates below; keeping
		// this graph independent prevents marketplace coefficients or shading models
		// from leaking into the single closed translucent hierarchy component.
		UMaterial* Material = LoadOrCreateMaterial(
			AssetTools, Path, AssetName);
		if (!IsValid(Material)) return nullptr;
		Material->Modify();
		ClearMaterialExpressions(Material);
		Material->MaterialDomain = MD_Surface;
		Material->BlendMode = bOrbitalPresentation ? BLEND_Translucent : BLEND_Opaque;
		Material->SetShadingModel(MSM_DefaultLit);
		if (bOrbitalPresentation)
		{
			Material->TranslucencyLightingMode = TLM_SurfacePerPixelLighting;
		}
		// WorldScape's outer rings and stitch strips are authored as independent
		// sections. Keep the physical shell robust at grazing angles; the closed
		// orbital sphere has consistent outward winding and remains one-sided.
		Material->TwoSided = !bOrbitalPresentation;
		Material->bScreenSpaceReflections = true;
		Material->bTangentSpaceNormal = false;

		UMaterialExpressionVectorParameter* Deep = AddVectorParameter(
			Material, TEXT("LiquidDeepColor"), FLinearColor(0.004f, 0.018f, 0.065f), -900, -350, 0);
		UMaterialExpressionVectorParameter* Shallow = AddVectorParameter(
			Material, TEXT("LiquidShallowColor"), FLinearColor(0.03f, 0.25f, 0.52f), -900, -230, 1);
		UMaterialExpressionVectorParameter* Emissive = AddVectorParameter(
			Material, TEXT("LiquidEmissiveColor"), FLinearColor(0.002f, 0.008f, 0.02f), -900, 100, 2);
		// Opacity belongs only to the translucent closed-globe presentation. The
		// physical WorldScape shell is deliberately opaque/depth-writing, so exposing
		// the same parameter there would create a control that cannot affect rendering.
		UMaterialExpressionScalarParameter* Opacity = bOrbitalPresentation
			? AddScalarParameter(
				Material, TEXT("Opacity"), 0.42f, 0.05f, 0.68f, -600, 300, 10)
			: nullptr;
		UMaterialExpressionScalarParameter* Roughness = AddScalarParameter(
			Material, TEXT("Roughness"), 0.18f, 0.0f, 1.0f, 50, 300, 11);
		UMaterialExpressionScalarParameter* Metallic = AddScalarParameter(
			Material, TEXT("Metallic"), 0.0f, 0.0f, 1.0f, 50, 400, 12);
		UMaterialExpressionScalarParameter* Specular = AddScalarParameter(
			Material, TEXT("Specular"), 0.65f, 0.0f, 1.0f, 50, 500, 13);
		UMaterialExpressionFresnel* Fresnel =
			AddExpression<UMaterialExpressionFresnel>(Material, -600, -80);
		UMaterialExpressionWorldPosition* WorldPosition =
			AddExpression<UMaterialExpressionWorldPosition>(Material, -900, 480);
		UMaterialExpression* LiquidActorPosition =
			AddActorPositionExpression(Material, -900, 540);
		UMaterialExpression* LiquidObjectPosition = bOrbitalPresentation
			? AddObjectPositionExpression(Material, -900, 600) : nullptr;
		UMaterialExpressionScalarParameter* OrbitalNormalBlend = bOrbitalPresentation
			? AddScalarParameter(
				Material, TEXT("OrbitalNormalBlend"), 0.0f, 0.0f, 1.0f,
				-680, 620, 23)
			: nullptr;
		UMaterialExpressionLinearInterpolate* OrbitalCenterPosition = bOrbitalPresentation
			? AddLerp(Material, LiquidActorPosition, LiquidObjectPosition,
				OrbitalNormalBlend, 0, -680, 560)
			: nullptr;
		// WorldScape LOD components share their owning actor/root centre. Select it
		// directly in the physical graph so section-local ObjectPositionWS can never
		// enter the shader, even behind a nominally-zero runtime lerp.
		UMaterialExpression* LiquidCenterPosition = bOrbitalPresentation
			? static_cast<UMaterialExpression*>(OrbitalCenterPosition)
			: LiquidActorPosition;
		UMaterialExpressionSubtract* LiquidRootRelativePosition =
			AddExpression<UMaterialExpressionSubtract>(Material, -680, 500);
		UMaterialExpressionNormalize* LiquidRadialNormal = bOrbitalPresentation
			? AddExpression<UMaterialExpressionNormalize>(Material, -460, 440) : nullptr;
		UMaterialExpressionScalarParameter* WaveScale = AddScalarParameter(
			Material, TEXT("WaveScaleCm"), 18000.0f, 6000.0f, 350000.0f,
			-900, 580, 20);
		UMaterialExpressionScalarParameter* WaveColorStrength = AddScalarParameter(
			Material, TEXT("WaveColorStrength"), 0.006f, 0.0f, 0.06f,
			-900, 680, 21);
		UMaterialExpressionScalarParameter* WaveNormalStrength = AddScalarParameter(
			Material, TEXT("WaveNormalStrength"), 0.025f, 0.0f, 0.08f,
			-900, 780, 22);
		UMaterialExpressionScalarParameter* PhysicalWaveDetailScale = !bOrbitalPresentation
			? AddScalarParameter(
				Material, TEXT("PhysicalWaveDetailScaleCm"), 4300.0f,
				1200.0f, 12000.0f, -900, 860, 24)
			: nullptr;
		UMaterialExpressionScalarParameter* PhysicalWaveRoughnessStrength = !bOrbitalPresentation
			? AddScalarParameter(
				Material, TEXT("PhysicalWaveRoughnessStrength"), 0.04f,
				0.0f, 0.10f, -900, 940, 25)
			: nullptr;
		if (!Deep || !Shallow || !Emissive || (bOrbitalPresentation && !Opacity)
			|| !Roughness || !Metallic
			|| !Specular || !Fresnel || !WorldPosition || !LiquidActorPosition
			|| !LiquidCenterPosition
			|| !LiquidRootRelativePosition || !WaveScale
			|| !WaveColorStrength || !WaveNormalStrength
			|| (bOrbitalPresentation
				&& (!LiquidObjectPosition || !OrbitalNormalBlend
					|| !OrbitalCenterPosition || !LiquidRadialNormal))
			|| (!bOrbitalPresentation
				&& (!PhysicalWaveDetailScale || !PhysicalWaveRoughnessStrength)))
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
		UMaterialExpressionDivide* WavePosition =
			AddExpression<UMaterialExpressionDivide>(Material, -650, 560);
		UMaterialExpressionVectorNoise* WaveNoise =
			AddExpression<UMaterialExpressionVectorNoise>(Material, -430, 560);
		UMaterialExpressionComponentMask* WaveGradient =
			AddExpression<UMaterialExpressionComponentMask>(Material, -210, 520);
		UMaterialExpressionComponentMask* WaveScalar =
			AddExpression<UMaterialExpressionComponentMask>(Material, -210, 650);
		UMaterialExpressionPixelDepth* WavePixelDepth =
			AddExpression<UMaterialExpressionPixelDepth>(Material, -430, 760);
		UMaterialExpressionSmoothStep* FarWaveFade = AddSmoothStep(
			Material, WavePixelDepth, 0, 1000.0f, 3000000.0f, -210, 780);
		UMaterialExpressionOneMinus* NearWaveFade =
			AddExpression<UMaterialExpressionOneMinus>(Material, 0, 780);
		UMaterialExpressionMultiply* FadedWaveColorStrength =
			AddExpression<UMaterialExpressionMultiply>(Material, 20, 620);
		UMaterialExpressionMultiply* WaveColorTerm =
			AddExpression<UMaterialExpressionMultiply>(Material, 220, 620);
		UMaterialExpressionAdd* WaveColorScale =
			AddExpression<UMaterialExpressionAdd>(Material, 420, 620);
		UMaterialExpressionMultiply* DetailedLiquidColor =
			AddExpression<UMaterialExpressionMultiply>(Material, 620, 500);
		UMaterialExpressionClamp* BoundedLiquidColor =
			AddExpression<UMaterialExpressionClamp>(Material, 820, 500);

		// A second, incommensurate octave exists only on the physical WorldScape
		// material. It breaks the single-scale wallpaper read while keeping the closed
		// orbital preview graph and its cost/appearance unchanged.
		UMaterialExpressionDivide* PhysicalDetailWavePosition = !bOrbitalPresentation
			? AddExpression<UMaterialExpressionDivide>(Material, -650, 920) : nullptr;
		UMaterialExpressionVectorNoise* PhysicalDetailWaveNoise = !bOrbitalPresentation
			? AddExpression<UMaterialExpressionVectorNoise>(Material, -430, 920) : nullptr;
		UMaterialExpressionComponentMask* PhysicalDetailWaveGradient = !bOrbitalPresentation
			? AddExpression<UMaterialExpressionComponentMask>(Material, -210, 900) : nullptr;
		UMaterialExpressionComponentMask* PhysicalDetailWaveScalar = !bOrbitalPresentation
			? AddExpression<UMaterialExpressionComponentMask>(Material, -210, 980) : nullptr;
		UMaterialExpressionMultiply* PrimaryWaveGradientWeight = !bOrbitalPresentation
			? AddExpression<UMaterialExpressionMultiply>(Material, 0, 880) : nullptr;
		UMaterialExpressionMultiply* DetailWaveGradientWeight = !bOrbitalPresentation
			? AddExpression<UMaterialExpressionMultiply>(Material, 0, 940) : nullptr;
		UMaterialExpressionAdd* CombinedWaveGradient = !bOrbitalPresentation
			? AddExpression<UMaterialExpressionAdd>(Material, 200, 900) : nullptr;
		UMaterialExpressionMultiply* PrimaryWaveScalarWeight = !bOrbitalPresentation
			? AddExpression<UMaterialExpressionMultiply>(Material, 0, 1000) : nullptr;
		UMaterialExpressionMultiply* DetailWaveScalarWeight = !bOrbitalPresentation
			? AddExpression<UMaterialExpressionMultiply>(Material, 0, 1060) : nullptr;
		UMaterialExpressionAdd* CombinedWaveScalar = !bOrbitalPresentation
			? AddExpression<UMaterialExpressionAdd>(Material, 200, 1020) : nullptr;
		UMaterialExpressionSubtract* CenteredCombinedWaveScalar = !bOrbitalPresentation
			? AddExpression<UMaterialExpressionSubtract>(Material, 400, 1080) : nullptr;

		// WorldScape already authors every ocean vertex normal radially from its
		// double-precision patch position plus shared LOD-relative position. Use that
		// authoritative, rotation-aware normal for the physical shell instead of
		// reconstructing it per pixel from full-scale LWC positions. The separate
		// closed-globe orbital material retains its existing vertex/radial blend.
		UMaterialExpressionVertexNormalWS* LiquidVertexNormal =
			AddExpression<UMaterialExpressionVertexNormalWS>(Material, 20, 900);
		UMaterialExpressionLinearInterpolate* OrbitalBaseWorldNormal = bOrbitalPresentation
			? AddLerp(Material, LiquidVertexNormal, LiquidRadialNormal,
				OrbitalNormalBlend, 0, 220, 980)
			: nullptr;
		UMaterialExpression* LiquidBaseWorldNormal = bOrbitalPresentation
			? static_cast<UMaterialExpression*>(OrbitalBaseWorldNormal)
			: static_cast<UMaterialExpression*>(LiquidVertexNormal);
		UMaterialExpressionDotProduct* LiquidRadialGradient =
			AddExpression<UMaterialExpressionDotProduct>(Material, 420, 900);
		UMaterialExpressionMultiply* LiquidRadialGradientVector =
			AddExpression<UMaterialExpressionMultiply>(Material, 620, 900);
		UMaterialExpressionSubtract* LiquidTangentGradient =
			AddExpression<UMaterialExpressionSubtract>(Material, 820, 900);
		UMaterialExpressionMultiply* FadedWaveNormalStrength =
			AddExpression<UMaterialExpressionMultiply>(Material, 420, 1020);
		UMaterialExpressionMultiply* LiquidNormalPerturbation =
			AddExpression<UMaterialExpressionMultiply>(Material, 820, 900);
		UMaterialExpressionAdd* PerturbedLiquidNormal =
			AddExpression<UMaterialExpressionAdd>(Material, 1020, 900);
		UMaterialExpressionNormalize* NormalizedLiquidNormal =
			AddExpression<UMaterialExpressionNormalize>(Material, 1220, 900);
		UMaterialExpressionMultiply* FadedPhysicalWaveRoughnessStrength = !bOrbitalPresentation
			? AddExpression<UMaterialExpressionMultiply>(Material, 420, 1140) : nullptr;
		UMaterialExpressionMultiply* PhysicalWaveRoughnessTerm = !bOrbitalPresentation
			? AddExpression<UMaterialExpressionMultiply>(Material, 620, 1140) : nullptr;
		UMaterialExpressionAdd* DetailedPhysicalRoughness = !bOrbitalPresentation
			? AddExpression<UMaterialExpressionAdd>(Material, 820, 1140) : nullptr;
		UMaterialExpressionClamp* BoundedPhysicalRoughness = !bOrbitalPresentation
			? AddExpression<UMaterialExpressionClamp>(Material, 1020, 1140) : nullptr;
		UMaterialExpressionClamp* BoundedEmissive =
			AddExpression<UMaterialExpressionClamp>(Material, -300, 100);
		UMaterialExpressionVertexColor* LiquidVertexColor = bOrbitalPresentation
			? AddExpression<UMaterialExpressionVertexColor>(Material, -180, 1040) : nullptr;
		UMaterialExpressionSmoothStep* OrbitalWaterMask = bOrbitalPresentation
			? AddSmoothStep(Material, LiquidVertexColor, 4, 0.08f, 0.72f, 20, 1100)
			: nullptr;
		UMaterialExpressionConstant* FullLiquidVisibility = bOrbitalPresentation
			? AddExpression<UMaterialExpressionConstant>(Material, 220, 1100) : nullptr;
		UMaterialExpressionLinearInterpolate* LiquidVisibilityMask = bOrbitalPresentation
			? AddLerp(Material, FullLiquidVisibility, OrbitalWaterMask,
				OrbitalNormalBlend, 0, 420, 1100)
			: nullptr;
		UMaterialExpressionLinearInterpolate* FresnelOpacityScale = bOrbitalPresentation
			? AddExpression<UMaterialExpressionLinearInterpolate>(Material, -180, 260) : nullptr;
		UMaterialExpressionMultiply* FresnelOpacity = bOrbitalPresentation
			? AddExpression<UMaterialExpressionMultiply>(Material, 0, 300) : nullptr;
		UMaterialExpressionClamp* BoundedOpacity = bOrbitalPresentation
			? AddExpression<UMaterialExpressionClamp>(Material, 180, 300) : nullptr;
		UMaterialExpressionMultiply* MaskedOpacity = bOrbitalPresentation
			? AddExpression<UMaterialExpressionMultiply>(Material, 380, 300) : nullptr;
		UMaterialExpressionMultiply* MaskedEmissive = bOrbitalPresentation
			? AddExpression<UMaterialExpressionMultiply>(Material, 380, 100) : nullptr;
		if (!LiquidColor || !WavePosition || !WaveNoise || !WaveGradient || !WaveScalar
			|| !WavePixelDepth || !FarWaveFade || !NearWaveFade || !FadedWaveColorStrength
			|| !WaveColorTerm || !WaveColorScale || !DetailedLiquidColor || !BoundedLiquidColor
			|| !LiquidVertexNormal || !LiquidBaseWorldNormal
			|| !LiquidRadialGradient || !LiquidRadialGradientVector
			|| !LiquidTangentGradient || !FadedWaveNormalStrength
			|| !LiquidNormalPerturbation || !PerturbedLiquidNormal || !NormalizedLiquidNormal
			|| !BoundedEmissive)
		{
			return nullptr;
		}
		if (!bOrbitalPresentation
			&& (!PhysicalDetailWavePosition || !PhysicalDetailWaveNoise
				|| !PhysicalDetailWaveGradient || !PhysicalDetailWaveScalar
				|| !PrimaryWaveGradientWeight || !DetailWaveGradientWeight
				|| !CombinedWaveGradient || !PrimaryWaveScalarWeight
				|| !DetailWaveScalarWeight || !CombinedWaveScalar
				|| !CenteredCombinedWaveScalar
				|| !FadedPhysicalWaveRoughnessStrength
				|| !PhysicalWaveRoughnessTerm || !DetailedPhysicalRoughness
				|| !BoundedPhysicalRoughness))
		{
			return nullptr;
		}
		if (bOrbitalPresentation
			&& (!OrbitalBaseWorldNormal || !LiquidVertexColor
				|| !OrbitalWaterMask || !FullLiquidVisibility || !LiquidVisibilityMask
				|| !FresnelOpacityScale || !FresnelOpacity || !BoundedOpacity
				|| !MaskedOpacity || !MaskedEmissive))
		{
			return nullptr;
		}
		WorldPosition->WorldPositionShaderOffset = WPT_ExcludeAllShaderOffsets;
		LiquidRootRelativePosition->A.Connect(0, WorldPosition);
		LiquidRootRelativePosition->B.Connect(0, LiquidCenterPosition);
		if (!bOrbitalPresentation)
		{
			// Keep Fresnel colour stable on WorldScape's authored radial normal while
			// the perturbed output remains available to lighting/specular.
			Fresnel->Normal.Connect(0, LiquidVertexNormal);
		}
		else
		{
			LiquidRadialNormal->VectorInput.Connect(0, LiquidRootRelativePosition);
		}
		WavePosition->A.Connect(0, LiquidRootRelativePosition);
		WavePosition->B.Connect(0, WaveScale);
		WaveNoise->Position.Connect(0, WavePosition);
		WaveNoise->WorldPositionOriginType = EPositionOrigin::Absolute;
		WaveNoise->NoiseFunction = VNF_GradientALU;
		WaveNoise->Quality = 1;
		WaveNoise->bTiling = false;
		WaveGradient->Input.Connect(0, WaveNoise);
		WaveGradient->R = true;
		WaveGradient->G = true;
		WaveGradient->B = true;
		WaveGradient->A = false;
		WaveScalar->Input.Connect(0, WaveNoise);
		WaveScalar->R = false;
		WaveScalar->G = false;
		WaveScalar->B = false;
		WaveScalar->A = true;

		UMaterialExpression* SurfaceWaveGradient = WaveGradient;
		UMaterialExpression* SurfaceWaveScalar = WaveScalar;
		if (!bOrbitalPresentation)
		{
			PhysicalDetailWavePosition->A.Connect(0, LiquidRootRelativePosition);
			PhysicalDetailWavePosition->B.Connect(0, PhysicalWaveDetailScale);
			PhysicalDetailWaveNoise->Position.Connect(0, PhysicalDetailWavePosition);
			PhysicalDetailWaveNoise->WorldPositionOriginType = EPositionOrigin::Absolute;
			PhysicalDetailWaveNoise->NoiseFunction = VNF_GradientALU;
			PhysicalDetailWaveNoise->Quality = 1;
			PhysicalDetailWaveNoise->bTiling = false;
			PhysicalDetailWaveGradient->Input.Connect(0, PhysicalDetailWaveNoise);
			PhysicalDetailWaveGradient->R = true;
			PhysicalDetailWaveGradient->G = true;
			PhysicalDetailWaveGradient->B = true;
			PhysicalDetailWaveGradient->A = false;
			PhysicalDetailWaveScalar->Input.Connect(0, PhysicalDetailWaveNoise);
			PhysicalDetailWaveScalar->R = false;
			PhysicalDetailWaveScalar->G = false;
			PhysicalDetailWaveScalar->B = false;
			PhysicalDetailWaveScalar->A = true;
			PrimaryWaveGradientWeight->A.Connect(0, WaveGradient);
			PrimaryWaveGradientWeight->ConstB = 0.68f;
			DetailWaveGradientWeight->A.Connect(0, PhysicalDetailWaveGradient);
			DetailWaveGradientWeight->ConstB = 0.32f;
			CombinedWaveGradient->A.Connect(0, PrimaryWaveGradientWeight);
			CombinedWaveGradient->B.Connect(0, DetailWaveGradientWeight);
			PrimaryWaveScalarWeight->A.Connect(0, WaveScalar);
			PrimaryWaveScalarWeight->ConstB = 0.68f;
			DetailWaveScalarWeight->A.Connect(0, PhysicalDetailWaveScalar);
			DetailWaveScalarWeight->ConstB = 0.32f;
			CombinedWaveScalar->A.Connect(0, PrimaryWaveScalarWeight);
			CombinedWaveScalar->B.Connect(0, DetailWaveScalarWeight);
			CenteredCombinedWaveScalar->A.Connect(0, CombinedWaveScalar);
			CenteredCombinedWaveScalar->ConstB = 0.5f;
			SurfaceWaveGradient = CombinedWaveGradient;
			SurfaceWaveScalar = CenteredCombinedWaveScalar;
		}
		NearWaveFade->Input.Connect(0, FarWaveFade);
		FadedWaveColorStrength->A.Connect(0, WaveColorStrength);
		FadedWaveColorStrength->B.Connect(0, NearWaveFade);
		WaveColorTerm->A.Connect(0, SurfaceWaveScalar);
		WaveColorTerm->B.Connect(0, FadedWaveColorStrength);
		WaveColorScale->A.Connect(0, WaveColorTerm);
		WaveColorScale->ConstB = 1.0f;
		DetailedLiquidColor->A.Connect(0, LiquidColor);
		DetailedLiquidColor->B.Connect(0, WaveColorScale);
		BoundedLiquidColor->Input.Connect(0, DetailedLiquidColor);
		BoundedLiquidColor->ClampMode = CMODE_Clamp;
		BoundedLiquidColor->MinDefault = 0.0f;
		BoundedLiquidColor->MaxDefault = 1.0f;

		LiquidRadialGradient->A.Connect(0, SurfaceWaveGradient);
		LiquidRadialGradient->B.Connect(0, LiquidBaseWorldNormal);
		LiquidRadialGradientVector->A.Connect(0, LiquidBaseWorldNormal);
		LiquidRadialGradientVector->B.Connect(0, LiquidRadialGradient);
		LiquidTangentGradient->A.Connect(0, SurfaceWaveGradient);
		LiquidTangentGradient->B.Connect(0, LiquidRadialGradientVector);
		FadedWaveNormalStrength->A.Connect(0, WaveNormalStrength);
		FadedWaveNormalStrength->B.Connect(0, NearWaveFade);
		LiquidNormalPerturbation->A.Connect(0, LiquidTangentGradient);
		LiquidNormalPerturbation->B.Connect(0, FadedWaveNormalStrength);
		PerturbedLiquidNormal->A.Connect(0, LiquidBaseWorldNormal);
		PerturbedLiquidNormal->B.Connect(0, LiquidNormalPerturbation);
		NormalizedLiquidNormal->VectorInput.Connect(0, PerturbedLiquidNormal);
		BoundedEmissive->Input.Connect(0, Emissive);
		BoundedEmissive->ClampMode = CMODE_Clamp;
		BoundedEmissive->MinDefault = 0.0f;
		BoundedEmissive->MaxDefault = 1.25f;

		UMaterialExpression* SurfaceRoughness = Roughness;
		UMaterialExpression* SurfaceEmissive = BoundedEmissive;
		if (!bOrbitalPresentation)
		{
			FadedPhysicalWaveRoughnessStrength->A.Connect(
				0, PhysicalWaveRoughnessStrength);
			FadedPhysicalWaveRoughnessStrength->B.Connect(0, NearWaveFade);
			PhysicalWaveRoughnessTerm->A.Connect(0, SurfaceWaveScalar);
			PhysicalWaveRoughnessTerm->B.Connect(
				0, FadedPhysicalWaveRoughnessStrength);
			DetailedPhysicalRoughness->A.Connect(0, Roughness);
			DetailedPhysicalRoughness->B.Connect(0, PhysicalWaveRoughnessTerm);
			BoundedPhysicalRoughness->Input.Connect(0, DetailedPhysicalRoughness);
			BoundedPhysicalRoughness->ClampMode = CMODE_Clamp;
			BoundedPhysicalRoughness->MinDefault = 0.04f;
			BoundedPhysicalRoughness->MaxDefault = 0.96f;
			SurfaceRoughness = BoundedPhysicalRoughness;
		}
		else
		{
			FresnelOpacityScale->ConstA = 0.62f;
			FresnelOpacityScale->ConstB = 1.0f;
			FresnelOpacityScale->Alpha.Connect(0, BoundedFresnel);
			FresnelOpacity->A.Connect(0, Opacity);
			FresnelOpacity->B.Connect(0, FresnelOpacityScale);
			BoundedOpacity->Input.Connect(0, FresnelOpacity);
			BoundedOpacity->ClampMode = CMODE_Clamp;
			BoundedOpacity->MinDefault = 0.05f;
			BoundedOpacity->MaxDefault = 0.68f;
			FullLiquidVisibility->R = 1.0f;
			MaskedOpacity->A.Connect(0, BoundedOpacity);
			MaskedOpacity->B.Connect(0, LiquidVisibilityMask);
			MaskedEmissive->A.Connect(0, BoundedEmissive);
			MaskedEmissive->B.Connect(0, LiquidVisibilityMask);
			SurfaceEmissive = MaskedEmissive;
		}

		bool bConnected =
			UMaterialEditingLibrary::ConnectMaterialProperty(BoundedLiquidColor, TEXT(""), MP_BaseColor)
			&& UMaterialEditingLibrary::ConnectMaterialProperty(SurfaceEmissive, TEXT(""), MP_EmissiveColor)
			&& UMaterialEditingLibrary::ConnectMaterialProperty(SurfaceRoughness, TEXT(""), MP_Roughness)
			&& UMaterialEditingLibrary::ConnectMaterialProperty(Metallic, TEXT(""), MP_Metallic)
			&& UMaterialEditingLibrary::ConnectMaterialProperty(Specular, TEXT(""), MP_Specular)
			&& UMaterialEditingLibrary::ConnectMaterialProperty(NormalizedLiquidNormal, TEXT(""), MP_Normal);
		if (bOrbitalPresentation)
		{
			bConnected = bConnected
				&& UMaterialEditingLibrary::ConnectMaterialProperty(
					MaskedOpacity, TEXT(""), MP_Opacity);
		}
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

	UMaterialInstanceConstant* CreateOrbitalLiquidPreset(
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

	bool ParentExposesVectorParameter(
		const UMaterialInterface* Parent, const FName ParameterName)
	{
		if (!IsValid(Parent)) return false;
		// WorldScape 5.4 uses a DoubleVectorParameter for the lava emissive input.
		// That input resolves through the material interface but is omitted from
		// GetAllVectorParameterInfo on the marketplace MIC, so use the runtime
		// parameter lookup as the authoritative contract before enumerating names.
		FLinearColor ResolvedValue = FLinearColor::Black;
		if (Parent->GetVectorParameterValue(
			FHashedMaterialParameterInfo(ParameterName), ResolvedValue))
		{
			return true;
		}
		TArray<FMaterialParameterInfo> ParameterInfos;
		TArray<FGuid> ParameterIds;
		Parent->GetAllVectorParameterInfo(ParameterInfos, ParameterIds);
		return ParameterInfos.ContainsByPredicate(
			[ParameterName](const FMaterialParameterInfo& Info)
			{
				return Info.Name == ParameterName;
			});
	}

	bool SetRequiredWorldScapeVectorOverride(
		UMaterialInstanceConstant* Result, const UMaterialInterface* Parent,
		const FName ParameterName, const FLinearColor& Value)
	{
		if (!IsValid(Result) || !ParentExposesVectorParameter(Parent, ParameterName))
		{
			UE_LOG(LogTemp, Error,
				TEXT("[APS.PlanetSurfaceAssets] WorldScape liquid template %s does not expose required vector %s"),
				*GetPathNameSafe(Parent), *ParameterName.ToString());
			return false;
		}
		Result->SetVectorParameterValueEditorOnly(
			FMaterialParameterInfo(ParameterName), Value);
		return true;
	}

	UMaterialInstanceConstant* CreateWorldScapeLiquidPreset(
		IAssetTools& AssetTools, const FString& Path, const FString& Name,
		UMaterialInterface* Parent, const EAPSPlanetLiquidType LiquidType)
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

		// Keep project-owned, catalog-stable MIC paths while delegating the physical
		// shader contract to the WorldScape materials authored for these exact LOD
		// sections. Water inherits the marketplace template verbatim. Ammonia changes
		// only the two native WorldScape 5.4 SingleLayerWater coefficient vectors
		// (unfortunately authored as Param_1/Param_2 by the plugin) and lava only its
		// real emissive colour. No project-only parameter names can become inert
		// overrides on a marketplace parent.
		Result->SetParentEditorOnly(Parent);
		Result->ClearParameterValuesEditorOnly();
		switch (LiquidType)
		{
		case EAPSPlanetLiquidType::Water:
			break;
		case EAPSPlanetLiquidType::Ammonia:
			if (!SetRequiredWorldScapeVectorOverride(
					Result, Parent, TEXT("Param_1"),
					FLinearColor(0.720f, 1.000f, 0.800f, 1.0f))
				|| !SetRequiredWorldScapeVectorOverride(
					Result, Parent, TEXT("Param_2"),
					FLinearColor(0.110f, 0.300f, 0.160f, 1.0f)))
			{
				return nullptr;
			}
			break;
		case EAPSPlanetLiquidType::Lava:
			if (!SetRequiredWorldScapeVectorOverride(
					Result, Parent, TEXT("EmissiveColor"),
					FLinearColor(0.420f, 0.018f, 0.001f, 1.0f)))
			{
				return nullptr;
			}
			break;
		case EAPSPlanetLiquidType::None:
		default:
			UE_LOG(LogTemp, Error,
				TEXT("[APS.PlanetSurfaceAssets] Invalid physical liquid type %d for %s"),
				static_cast<int32>(LiquidType), *Name);
			return nullptr;
		}
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
		float PaletteLiftValue = 0.003f;
		float PaletteSaturationValue = 1.05f;
		float PaletteContrastValue = 1.08f;
		float MacroScaleCm = 8000000.0f;
		float MesoScaleCm = 450000.0f;
		float NearScaleCm = 7000.0f;
		float MacroColorStrengthValue = 0.060f;
		float MesoColorStrengthValue = 0.042f;
		float NearColorStrengthValue = 0.022f;
		float NormalStrengthValue = 0.060f;
		float MesoRoughnessStrengthValue = 0.032f;
		float DetailRoughnessStrengthValue = 0.025f;
		float TerrainAmbientFillValue = 0.09f;
		float SlopeTintStrengthValue = 0.20f;
		switch (Archetype)
		{
		case EAPSPlanetSurfaceArchetype::Temperate:
			PaletteGainValue = 1.00f;
			PaletteSaturationValue = 1.12f;
			PaletteContrastValue = 1.07f;
			MacroScaleCm = 9000000.0f;
			MesoScaleCm = 600000.0f;
			NearScaleCm = 7500.0f;
			MacroColorStrengthValue = 0.052f;
			MesoColorStrengthValue = 0.038f;
			NearColorStrengthValue = 0.022f;
			NormalStrengthValue = 0.065f;
			TerrainAmbientFillValue = 0.105f;
			SlopeTintStrengthValue = 0.20f;
			break;
		case EAPSPlanetSurfaceArchetype::Oceanic:
			PaletteGainValue = 0.99f;
			PaletteSaturationValue = 1.10f;
			PaletteContrastValue = 1.06f;
			MacroScaleCm = 12000000.0f;
			MesoScaleCm = 700000.0f;
			NearScaleCm = 9000.0f;
			MacroColorStrengthValue = 0.040f;
			MesoColorStrengthValue = 0.028f;
			NearColorStrengthValue = 0.018f;
			NormalStrengthValue = 0.045f;
			TerrainAmbientFillValue = 0.095f;
			SlopeTintStrengthValue = 0.14f;
			break;
		case EAPSPlanetSurfaceArchetype::Biosphere:
			PaletteGainValue = 1.00f;
			PaletteSaturationValue = 1.18f;
			PaletteContrastValue = 1.06f;
			MacroScaleCm = 7500000.0f;
			MesoScaleCm = 320000.0f;
			NearScaleCm = 5500.0f;
			MacroColorStrengthValue = 0.048f;
			MesoColorStrengthValue = 0.034f;
			NearColorStrengthValue = 0.024f;
			NormalStrengthValue = 0.070f;
			TerrainAmbientFillValue = 0.115f;
			SlopeTintStrengthValue = 0.19f;
			break;
		case EAPSPlanetSurfaceArchetype::Desert:
			PaletteGainValue = 0.97f;
			PaletteSaturationValue = 1.10f;
			PaletteContrastValue = 1.07f;
			MacroScaleCm = 11000000.0f;
			MesoScaleCm = 900000.0f;
			NearScaleCm = 9500.0f;
			MacroColorStrengthValue = 0.052f;
			MesoColorStrengthValue = 0.036f;
			NearColorStrengthValue = 0.018f;
			NormalStrengthValue = 0.050f;
			TerrainAmbientFillValue = 0.100f;
			SlopeTintStrengthValue = 0.23f;
			break;
		case EAPSPlanetSurfaceArchetype::Cryogenic:
			PaletteGainValue = 0.98f;
			PaletteLiftValue = 0.0f;
			PaletteSaturationValue = 1.03f;
			PaletteContrastValue = 1.08f;
			MacroScaleCm = 6000000.0f;
			MesoScaleCm = 350000.0f;
			NearScaleCm = 6500.0f;
			MacroColorStrengthValue = 0.050f;
			MesoColorStrengthValue = 0.040f;
			NearColorStrengthValue = 0.020f;
			NormalStrengthValue = 0.060f;
			// Preserve night-side readability without washing out local self-shadowing.
			TerrainAmbientFillValue = 0.105f;
			SlopeTintStrengthValue = 0.20f;
			break;
		case EAPSPlanetSurfaceArchetype::Magmatic:
			PaletteGainValue = 0.94f;
			PaletteSaturationValue = 1.12f;
			PaletteContrastValue = 1.08f;
			MacroScaleCm = 5000000.0f;
			MesoScaleCm = 250000.0f;
			NearScaleCm = 6000.0f;
			MacroColorStrengthValue = 0.050f;
			MesoColorStrengthValue = 0.036f;
			NearColorStrengthValue = 0.018f;
			NormalStrengthValue = 0.065f;
			TerrainAmbientFillValue = 0.045f;
			SlopeTintStrengthValue = 0.23f;
			break;
		case EAPSPlanetSurfaceArchetype::Rocky:
			PaletteGainValue = 1.02f;
			PaletteLiftValue = 0.004f;
			PaletteSaturationValue = 0.98f;
			PaletteContrastValue = 1.07f;
			MacroScaleCm = 7000000.0f;
			MesoScaleCm = 300000.0f;
			NearScaleCm = 7000.0f;
			MacroColorStrengthValue = 0.055f;
			MesoColorStrengthValue = 0.040f;
			NearColorStrengthValue = 0.020f;
			NormalStrengthValue = 0.070f;
			TerrainAmbientFillValue = 0.110f;
			SlopeTintStrengthValue = 0.27f;
			break;
		case EAPSPlanetSurfaceArchetype::Metallic:
			PaletteGainValue = 1.06f;
			PaletteLiftValue = 0.006f;
			PaletteSaturationValue = 0.94f;
			PaletteContrastValue = 1.05f;
			MacroScaleCm = 10000000.0f;
			MesoScaleCm = 500000.0f;
			NearScaleCm = 8500.0f;
			MacroColorStrengthValue = 0.044f;
			MesoColorStrengthValue = 0.032f;
			NearColorStrengthValue = 0.016f;
			NormalStrengthValue = 0.045f;
			MesoRoughnessStrengthValue = 0.028f;
			TerrainAmbientFillValue = 0.100f;
			SlopeTintStrengthValue = 0.18f;
			break;
		case EAPSPlanetSurfaceArchetype::ExoticChemical:
			PaletteGainValue = 1.00f;
			PaletteSaturationValue = 1.16f;
			PaletteContrastValue = 1.06f;
			MacroScaleCm = 6000000.0f;
			MesoScaleCm = 320000.0f;
			NearScaleCm = 7000.0f;
			MacroColorStrengthValue = 0.050f;
			MesoColorStrengthValue = 0.036f;
			NearColorStrengthValue = 0.019f;
			NormalStrengthValue = 0.055f;
			TerrainAmbientFillValue = 0.115f;
			SlopeTintStrengthValue = 0.19f;
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
		Scalar(TEXT("PaletteSaturation"), PaletteSaturationValue);
		Scalar(TEXT("PaletteContrast"), PaletteContrastValue);
		Scalar(TEXT("MacroDetailScaleCm"), MacroScaleCm);
		Scalar(TEXT("MesoDetailScaleCm"), MesoScaleCm);
		Scalar(TEXT("NearDetailScaleCm"), NearScaleCm);
		Scalar(TEXT("MacroColorStrength"), MacroColorStrengthValue);
		Scalar(TEXT("MesoColorStrength"), MesoColorStrengthValue);
		Scalar(TEXT("NearColorStrength"), NearColorStrengthValue);
		Scalar(TEXT("DetailNormalStrength"), NormalStrengthValue);
		Scalar(TEXT("OrbitalNormalBlend"), 0.0f);
		Scalar(TEXT("MesoRoughnessStrength"), MesoRoughnessStrengthValue);
		Scalar(TEXT("DetailRoughnessStrength"), DetailRoughnessStrengthValue);
		Scalar(TEXT("TerrainAmbientFill"), TerrainAmbientFillValue);
		Scalar(TEXT("SlopeTintStrength"), SlopeTintStrengthValue);
		Scalar(TEXT("MidVarient1Rough"), MeanRoughness);
		Scalar(TEXT("MidVarient2Rough"), MeanRoughness);
		Scalar(TEXT("MidVarient3Rough"), MeanRoughness);
		Scalar(TEXT("Roughness"), MeanRoughness);
		Scalar(TEXT("Metallic"), MeanMetallic);
		Scalar(TEXT("Specular"), FMath::Lerp(0.24f, 0.48f, MeanMetallic));
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

	// Terrain remains project-authored. Physical liquids deliberately use the
	// marketplace graphs designed for WorldScape's generated ocean sections, while
	// the closed hierarchy globe retains its independent project preview graph.
	const FString WorldScapeOceanPath =
		TEXT("/WorldScape/Ressources/Materials/WorldScapeMaterials/Ocean");
	UMaterialInstanceConstant* WorldScapeWaterTemplate =
		LoadAsset<UMaterialInstanceConstant>(
			WorldScapeOceanPath, TEXT("MI_Planetary_Ocean"));
	UMaterialInstanceConstant* WorldScapeLavaTemplate =
		LoadAsset<UMaterialInstanceConstant>(
			WorldScapeOceanPath, TEXT("MI_LavaOcean"));
	UMaterial* WorldScapeWaterMaster = IsValid(WorldScapeWaterTemplate)
		? WorldScapeWaterTemplate->GetMaterial() : nullptr;
	UMaterial* WorldScapeLavaMaster = IsValid(WorldScapeLavaTemplate)
		? WorldScapeLavaTemplate->GetMaterial() : nullptr;
	const bool bValidWorldScapeWater = IsValid(WorldScapeWaterMaster)
		&& WorldScapeWaterTemplate->Parent.Get() == WorldScapeWaterMaster
		&& WorldScapeWaterTemplate->GetBlendMode() == BLEND_Opaque
		&& WorldScapeWaterTemplate->GetShadingModels().HasShadingModel(
			MSM_SingleLayerWater)
		&& WorldScapeWaterMaster->GetBlendMode() == BLEND_Opaque
		&& WorldScapeWaterMaster->GetShadingModels().HasShadingModel(
			MSM_SingleLayerWater);
	const bool bValidWorldScapeLava = IsValid(WorldScapeLavaMaster)
		&& WorldScapeLavaTemplate->Parent.Get() == WorldScapeLavaMaster
		&& WorldScapeLavaTemplate->GetBlendMode() == BLEND_Opaque
		&& WorldScapeLavaMaster->GetBlendMode() == BLEND_Opaque
		&& WorldScapeLavaMaster->GetShadingModels().HasShadingModel(MSM_DefaultLit);

	UMaterial* WorldScapeTerrain = CreateWorldScapeTerrainMaterial(AssetTools);
	UMaterial* OrbitalTerrain = CreateOrbitalTerrainMaterial(AssetTools);
	UMaterial* OrbitalLiquid = CreateCanonicalLiquidMaterial(
		AssetTools, PreviewMaterialPath, TEXT("M_APS_OrbitalLiquid"), true);
	UMaterial* PreviewGuide = CreatePreviewGuideMaterial(AssetTools);
	if (!bValidWorldScapeWater || !bValidWorldScapeLava
		|| !WorldScapeTerrain || !OrbitalTerrain || !OrbitalLiquid || !PreviewGuide)
	{
		UE_LOG(LogTemp, Error,
			TEXT("[APS.PlanetSurfaceAssets] Missing or incompatible WorldScape physical liquid template, terrain master or preview master water=%s waterBase=%s lava=%s lavaBase=%s"),
			*GetPathNameSafe(WorldScapeWaterTemplate),
			*GetPathNameSafe(WorldScapeWaterMaster),
			*GetPathNameSafe(WorldScapeLavaTemplate),
			*GetPathNameSafe(WorldScapeLavaMaster));
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

	// Stable project MIC paths remain the catalog boundary. Water and ammonia wrap
	// MI_Planetary_Ocean (the opaque WorldScape SingleLayerWater graph), while lava
	// wraps the plugin's dedicated opaque WorldScape lava graph. MI_Original_Water is
	// intentionally excluded because its masked UE Water graph expects WaterBody data.
	UMaterialInstanceConstant* Water = CreateWorldScapeLiquidPreset(
		AssetTools, MaterialPath, TEXT("MI_APS_WS_Water"),
		WorldScapeWaterTemplate, EAPSPlanetLiquidType::Water);
	UMaterialInstanceConstant* Ammonia = CreateWorldScapeLiquidPreset(
		AssetTools, MaterialPath, TEXT("MI_APS_WS_Ammonia"),
		WorldScapeWaterTemplate, EAPSPlanetLiquidType::Ammonia);
	UMaterialInstanceConstant* Lava = CreateWorldScapeLiquidPreset(
		AssetTools, MaterialPath, TEXT("MI_APS_WS_Lava"),
		WorldScapeLavaTemplate, EAPSPlanetLiquidType::Lava);
	if (!Water || !Ammonia || !Lava) return 4;

	// The hierarchy globe uses its own translucent master and type-matched presets.
	// It never inherits the physical SingleLayerWater/lava parameter namespace or
	// clipmap render pass.
	UMaterialInstanceConstant* OrbitalWater = CreateOrbitalLiquidPreset(
		AssetTools, PreviewMaterialPath, TEXT("MI_APS_OrbitalLiquid_Water"), OrbitalLiquid,
		FLinearColor(0.002f, 0.012f, 0.045f), FLinearColor(0.010f, 0.105f, 0.145f),
		FLinearColor(0.001f, 0.004f, 0.009f), 0.52f, 0.24f, 0.0f, 0.50f);
	UMaterialInstanceConstant* OrbitalAmmonia = CreateOrbitalLiquidPreset(
		AssetTools, PreviewMaterialPath, TEXT("MI_APS_OrbitalLiquid_Ammonia"), OrbitalLiquid,
		FLinearColor(0.008f, 0.055f, 0.025f), FLinearColor(0.160f, 0.480f, 0.250f),
		FLinearColor(0.006f, 0.035f, 0.015f), 0.50f, 0.22f, 0.0f, 0.58f);
	UMaterialInstanceConstant* OrbitalLava = CreateOrbitalLiquidPreset(
		AssetTools, PreviewMaterialPath, TEXT("MI_APS_OrbitalLiquid_Lava"), OrbitalLiquid,
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

	UE_LOG(LogTemp, Display, TEXT("[APS.PlanetSurfaceAssets] Updated four project-owned terrain/preview masters, 9 terrain instances, 3 WorldScape-backed gameplay liquid wrappers, 3 hierarchy liquid instances and catalog under %s"),
		*RootPath);
	return 0;
}

#endif
