// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
#include "APS_ALPHA/StarClusterPopulation.h"
PRAGMA_DISABLE_DEPRECATION_WARNINGS
void EmptyLinkFunctionForGeneratedCodeStarClusterPopulation() {}
// Cross Module References
	APS_ALPHA_API UEnum* Z_Construct_UEnum_APS_ALPHA_EStarClusterPopulation();
	UPackage* Z_Construct_UPackage__Script_APS_ALPHA();
// End Cross Module References
	static FEnumRegistrationInfo Z_Registration_Info_UEnum_EStarClusterPopulation;
	static UEnum* EStarClusterPopulation_StaticEnum()
	{
		if (!Z_Registration_Info_UEnum_EStarClusterPopulation.OuterSingleton)
		{
			Z_Registration_Info_UEnum_EStarClusterPopulation.OuterSingleton = GetStaticEnum(Z_Construct_UEnum_APS_ALPHA_EStarClusterPopulation, Z_Construct_UPackage__Script_APS_ALPHA(), TEXT("EStarClusterPopulation"));
		}
		return Z_Registration_Info_UEnum_EStarClusterPopulation.OuterSingleton;
	}
	template<> APS_ALPHA_API UEnum* StaticEnum<EStarClusterPopulation>()
	{
		return EStarClusterPopulation_StaticEnum();
	}
	struct Z_Construct_UEnum_APS_ALPHA_EStarClusterPopulation_Statics
	{
		static const UECodeGen_Private::FEnumeratorParam Enumerators[];
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam Enum_MetaDataParams[];
#endif
		static const UECodeGen_Private::FEnumParams EnumParams;
	};
	const UECodeGen_Private::FEnumeratorParam Z_Construct_UEnum_APS_ALPHA_EStarClusterPopulation_Statics::Enumerators[] = {
		{ "EStarClusterPopulation::AllSequenses", (int64)EStarClusterPopulation::AllSequenses },
		{ "EStarClusterPopulation::MainSequence", (int64)EStarClusterPopulation::MainSequence },
		{ "EStarClusterPopulation::Giants", (int64)EStarClusterPopulation::Giants },
		{ "EStarClusterPopulation::Dwarfs", (int64)EStarClusterPopulation::Dwarfs },
		{ "EStarClusterPopulation::Protostars", (int64)EStarClusterPopulation::Protostars },
		{ "EStarClusterPopulation::Unknown", (int64)EStarClusterPopulation::Unknown },
	};
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UEnum_APS_ALPHA_EStarClusterPopulation_Statics::Enum_MetaDataParams[] = {
		{ "AllSequenses.DisplayName", "All Sequenses Stars" },
		{ "AllSequenses.Name", "EStarClusterPopulation::AllSequenses" },
		{ "BlueprintType", "true" },
		{ "Dwarfs.DisplayName", "Mostly Dwarfs" },
		{ "Dwarfs.Name", "EStarClusterPopulation::Dwarfs" },
		{ "Giants.DisplayName", "Mostly Giants" },
		{ "Giants.Name", "EStarClusterPopulation::Giants" },
		{ "MainSequence.DisplayName", "Only Main Sequence Stars" },
		{ "MainSequence.Name", "EStarClusterPopulation::MainSequence" },
		{ "ModuleRelativePath", "StarClusterPopulation.h" },
		{ "Protostars.DisplayName", "Mostly Protostars" },
		{ "Protostars.Name", "EStarClusterPopulation::Protostars" },
		{ "Unknown.DisplayName", "Unknown" },
		{ "Unknown.Name", "EStarClusterPopulation::Unknown" },
	};
#endif
	const UECodeGen_Private::FEnumParams Z_Construct_UEnum_APS_ALPHA_EStarClusterPopulation_Statics::EnumParams = {
		(UObject*(*)())Z_Construct_UPackage__Script_APS_ALPHA,
		nullptr,
		"EStarClusterPopulation",
		"EStarClusterPopulation",
		Z_Construct_UEnum_APS_ALPHA_EStarClusterPopulation_Statics::Enumerators,
		UE_ARRAY_COUNT(Z_Construct_UEnum_APS_ALPHA_EStarClusterPopulation_Statics::Enumerators),
		RF_Public|RF_Transient|RF_MarkAsNative,
		EEnumFlags::None,
		(uint8)UEnum::ECppForm::EnumClass,
		METADATA_PARAMS(Z_Construct_UEnum_APS_ALPHA_EStarClusterPopulation_Statics::Enum_MetaDataParams, UE_ARRAY_COUNT(Z_Construct_UEnum_APS_ALPHA_EStarClusterPopulation_Statics::Enum_MetaDataParams))
	};
	UEnum* Z_Construct_UEnum_APS_ALPHA_EStarClusterPopulation()
	{
		if (!Z_Registration_Info_UEnum_EStarClusterPopulation.InnerSingleton)
		{
			UECodeGen_Private::ConstructUEnum(Z_Registration_Info_UEnum_EStarClusterPopulation.InnerSingleton, Z_Construct_UEnum_APS_ALPHA_EStarClusterPopulation_Statics::EnumParams);
		}
		return Z_Registration_Info_UEnum_EStarClusterPopulation.InnerSingleton;
	}
	struct Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_StarClusterPopulation_h_Statics
	{
		static const FEnumRegisterCompiledInInfo EnumInfo[];
	};
	const FEnumRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_StarClusterPopulation_h_Statics::EnumInfo[] = {
		{ EStarClusterPopulation_StaticEnum, TEXT("EStarClusterPopulation"), &Z_Registration_Info_UEnum_EStarClusterPopulation, CONSTRUCT_RELOAD_VERSION_INFO(FEnumReloadVersionInfo, 2512550455U) },
	};
	static FRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_StarClusterPopulation_h_508967209(TEXT("/Script/APS_ALPHA"),
		nullptr, 0,
		nullptr, 0,
		Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_StarClusterPopulation_h_Statics::EnumInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_StarClusterPopulation_h_Statics::EnumInfo));
PRAGMA_ENABLE_DEPRECATION_WARNINGS
