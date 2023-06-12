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
		{ "EAstroGenerationLevel::GalaxiesCluster", (int64)EAstroGenerationLevel::GalaxiesCluster },
		{ "EAstroGenerationLevel::Galaxy", (int64)EAstroGenerationLevel::Galaxy },
		{ "EAstroGenerationLevel::StarCluster", (int64)EAstroGenerationLevel::StarCluster },
		{ "EAstroGenerationLevel::StarSystem", (int64)EAstroGenerationLevel::StarSystem },
		{ "EAstroGenerationLevel::PlanetSystem", (int64)EAstroGenerationLevel::PlanetSystem },
		{ "EAstroGenerationLevel::SinglePlanet", (int64)EAstroGenerationLevel::SinglePlanet },
		{ "EAstroGenerationLevel::Random", (int64)EAstroGenerationLevel::Random },
	};
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UEnum_APS_ALPHA_EAstroGenerationLevel_Statics::Enum_MetaDataParams[] = {
		{ "BlueprintType", "true" },
		{ "GalaxiesCluster.DisplayName", "Galaxies Cluster" },
		{ "GalaxiesCluster.Name", "EAstroGenerationLevel::GalaxiesCluster" },
		{ "Galaxy.DisplayName", "Galaxy" },
		{ "Galaxy.Name", "EAstroGenerationLevel::Galaxy" },
		{ "ModuleRelativePath", "AstroGenerationLevel.h" },
		{ "PlanetSystem.DisplayName", "Planet System" },
		{ "PlanetSystem.Name", "EAstroGenerationLevel::PlanetSystem" },
		{ "Random.DisplayName", "Random" },
		{ "Random.Name", "EAstroGenerationLevel::Random" },
		{ "SinglePlanet.DisplayName", "Single Planet" },
		{ "SinglePlanet.Name", "EAstroGenerationLevel::SinglePlanet" },
		{ "StarCluster.DisplayName", "Star Cluster" },
		{ "StarCluster.Name", "EAstroGenerationLevel::StarCluster" },
		{ "StarSystem.DisplayName", "Star System" },
		{ "StarSystem.Name", "EAstroGenerationLevel::StarSystem" },
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
		{ EAstroGenerationLevel_StaticEnum, TEXT("EAstroGenerationLevel"), &Z_Registration_Info_UEnum_EAstroGenerationLevel, CONSTRUCT_RELOAD_VERSION_INFO(FEnumReloadVersionInfo, 2796898956U) },
	};
	static FRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_AstroGenerationLevel_h_839422616(TEXT("/Script/APS_ALPHA"),
		nullptr, 0,
		nullptr, 0,
		Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_AstroGenerationLevel_h_Statics::EnumInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_AstroGenerationLevel_h_Statics::EnumInfo));
PRAGMA_ENABLE_DEPRECATION_WARNINGS
