// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
#include "APS_ALPHA/AstroGenerationLevel.h"
PRAGMA_DISABLE_DEPRECATION_WARNINGS
void EmptyLinkFunctionForGeneratedCodeAstroGenerationLevel() {}
// Cross Module References
	APS_ALPHA_API UEnum* Z_Construct_UEnum_APS_ALPHA_EAstroGenerationLevel();
	UPackage* Z_Construct_UPackage__Script_APS_ALPHA();
// End Cross Module References
	static FEnumRegistrationInfo Z_Registration_Info_UEnum_EAstroGenerationLevel;
	static UEnum* EAstroGenerationLevel_StaticEnum()
	{
		if (!Z_Registration_Info_UEnum_EAstroGenerationLevel.OuterSingleton)
		{
			Z_Registration_Info_UEnum_EAstroGenerationLevel.OuterSingleton = GetStaticEnum(Z_Construct_UEnum_APS_ALPHA_EAstroGenerationLevel, Z_Construct_UPackage__Script_APS_ALPHA(), TEXT("EAstroGenerationLevel"));
		}
		return Z_Registration_Info_UEnum_EAstroGenerationLevel.OuterSingleton;
	}
	template<> APS_ALPHA_API UEnum* StaticEnum<EAstroGenerationLevel>()
	{
		return EAstroGenerationLevel_StaticEnum();
	}
	struct Z_Construct_UEnum_APS_ALPHA_EAstroGenerationLevel_Statics
	{
		static const UECodeGen_Private::FEnumeratorParam Enumerators[];
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam Enum_MetaDataParams[];
#endif
		static const UECodeGen_Private::FEnumParams EnumParams;
	};
	const UECodeGen_Private::FEnumeratorParam Z_Construct_UEnum_APS_ALPHA_EAstroGenerationLevel_Statics::Enumerators[] = {
		{ "EAstroGenerationLevel::Galaxy", (int64)EAstroGenerationLevel::Galaxy },
		{ "EAstroGenerationLevel::Cluster", (int64)EAstroGenerationLevel::Cluster },
		{ "EAstroGenerationLevel::StarSystem", (int64)EAstroGenerationLevel::StarSystem },
		{ "EAstroGenerationLevel::PlanetSystem", (int64)EAstroGenerationLevel::PlanetSystem },
		{ "EAstroGenerationLevel::SinglePlanet", (int64)EAstroGenerationLevel::SinglePlanet },
		{ "EAstroGenerationLevel::Random", (int64)EAstroGenerationLevel::Random },
	};
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UEnum_APS_ALPHA_EAstroGenerationLevel_Statics::Enum_MetaDataParams[] = {
		{ "BlueprintType", "true" },
		{ "Cluster.Comment", "// Unknown\n" },
		{ "Cluster.DisplayName", "Cluster" },
		{ "Cluster.Name", "EAstroGenerationLevel::Cluster" },
		{ "Cluster.ToolTip", "Unknown" },
		{ "Galaxy.DisplayName", "Galaxy" },
		{ "Galaxy.Name", "EAstroGenerationLevel::Galaxy" },
		{ "ModuleRelativePath", "AstroGenerationLevel.h" },
		{ "PlanetSystem.Comment", "// Unknown\n" },
		{ "PlanetSystem.DisplayName", "PlanetSystem" },
		{ "PlanetSystem.Name", "EAstroGenerationLevel::PlanetSystem" },
		{ "PlanetSystem.ToolTip", "Unknown" },
		{ "Random.Comment", "// Unknown\n" },
		{ "Random.DisplayName", "Random" },
		{ "Random.Name", "EAstroGenerationLevel::Random" },
		{ "Random.ToolTip", "Unknown" },
		{ "SinglePlanet.Comment", "// Unknown\n" },
		{ "SinglePlanet.DisplayName", "SinglePlanet" },
		{ "SinglePlanet.Name", "EAstroGenerationLevel::SinglePlanet" },
		{ "SinglePlanet.ToolTip", "Unknown" },
		{ "StarSystem.Comment", "// Unknown\n" },
		{ "StarSystem.DisplayName", "StarSystem" },
		{ "StarSystem.Name", "EAstroGenerationLevel::StarSystem" },
		{ "StarSystem.ToolTip", "Unknown" },
	};
#endif
	const UECodeGen_Private::FEnumParams Z_Construct_UEnum_APS_ALPHA_EAstroGenerationLevel_Statics::EnumParams = {
		(UObject*(*)())Z_Construct_UPackage__Script_APS_ALPHA,
		nullptr,
		"EAstroGenerationLevel",
		"EAstroGenerationLevel",
		Z_Construct_UEnum_APS_ALPHA_EAstroGenerationLevel_Statics::Enumerators,
		UE_ARRAY_COUNT(Z_Construct_UEnum_APS_ALPHA_EAstroGenerationLevel_Statics::Enumerators),
		RF_Public|RF_Transient|RF_MarkAsNative,
		EEnumFlags::None,
		(uint8)UEnum::ECppForm::EnumClass,
		METADATA_PARAMS(Z_Construct_UEnum_APS_ALPHA_EAstroGenerationLevel_Statics::Enum_MetaDataParams, UE_ARRAY_COUNT(Z_Construct_UEnum_APS_ALPHA_EAstroGenerationLevel_Statics::Enum_MetaDataParams))
	};
	UEnum* Z_Construct_UEnum_APS_ALPHA_EAstroGenerationLevel()
	{
		if (!Z_Registration_Info_UEnum_EAstroGenerationLevel.InnerSingleton)
		{
			UECodeGen_Private::ConstructUEnum(Z_Registration_Info_UEnum_EAstroGenerationLevel.InnerSingleton, Z_Construct_UEnum_APS_ALPHA_EAstroGenerationLevel_Statics::EnumParams);
		}
		return Z_Registration_Info_UEnum_EAstroGenerationLevel.InnerSingleton;
	}
	struct Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_AstroGenerationLevel_h_Statics
	{
		static const FEnumRegisterCompiledInInfo EnumInfo[];
	};
	const FEnumRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_AstroGenerationLevel_h_Statics::EnumInfo[] = {
		{ EAstroGenerationLevel_StaticEnum, TEXT("EAstroGenerationLevel"), &Z_Registration_Info_UEnum_EAstroGenerationLevel, CONSTRUCT_RELOAD_VERSION_INFO(FEnumReloadVersionInfo, 3252446912U) },
	};
	static FRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_AstroGenerationLevel_h_2748523484(TEXT("/Script/APS_ALPHA"),
		nullptr, 0,
		nullptr, 0,
		Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_AstroGenerationLevel_h_Statics::EnumInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_AstroGenerationLevel_h_Statics::EnumInfo));
PRAGMA_ENABLE_DEPRECATION_WARNINGS
