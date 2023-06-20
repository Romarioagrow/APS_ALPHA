// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
#include "APS_ALPHA/StarSystemType.h"
PRAGMA_DISABLE_DEPRECATION_WARNINGS
void EmptyLinkFunctionForGeneratedCodeStarSystemType() {}
// Cross Module References
	APS_ALPHA_API UEnum* Z_Construct_UEnum_APS_ALPHA_EStarSystemType();
	UPackage* Z_Construct_UPackage__Script_APS_ALPHA();
// End Cross Module References
	static FEnumRegistrationInfo Z_Registration_Info_UEnum_EStarSystemType;
	static UEnum* EStarSystemType_StaticEnum()
	{
		if (!Z_Registration_Info_UEnum_EStarSystemType.OuterSingleton)
		{
			Z_Registration_Info_UEnum_EStarSystemType.OuterSingleton = GetStaticEnum(Z_Construct_UEnum_APS_ALPHA_EStarSystemType, Z_Construct_UPackage__Script_APS_ALPHA(), TEXT("EStarSystemType"));
		}
		return Z_Registration_Info_UEnum_EStarSystemType.OuterSingleton;
	}
	template<> APS_ALPHA_API UEnum* StaticEnum<EStarSystemType>()
	{
		return EStarSystemType_StaticEnum();
	}
	struct Z_Construct_UEnum_APS_ALPHA_EStarSystemType_Statics
	{
		static const UECodeGen_Private::FEnumeratorParam Enumerators[];
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam Enum_MetaDataParams[];
#endif
		static const UECodeGen_Private::FEnumParams EnumParams;
	};
	const UECodeGen_Private::FEnumeratorParam Z_Construct_UEnum_APS_ALPHA_EStarSystemType_Statics::Enumerators[] = {
		{ "EStarSystemType::SingleStar", (int64)EStarSystemType::SingleStar },
		{ "EStarSystemType::DoubleStar", (int64)EStarSystemType::DoubleStar },
		{ "EStarSystemType::TripleStar", (int64)EStarSystemType::TripleStar },
		{ "EStarSystemType::MultipleStar", (int64)EStarSystemType::MultipleStar },
	};
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UEnum_APS_ALPHA_EStarSystemType_Statics::Enum_MetaDataParams[] = {
		{ "BlueprintType", "true" },
		{ "DoubleStar.DisplayName", "Double Star" },
		{ "DoubleStar.Name", "EStarSystemType::DoubleStar" },
		{ "ModuleRelativePath", "StarSystemType.h" },
		{ "MultipleStar.DisplayName", "Multiple Star" },
		{ "MultipleStar.Name", "EStarSystemType::MultipleStar" },
		{ "SingleStar.DisplayName", "Single Star" },
		{ "SingleStar.Name", "EStarSystemType::SingleStar" },
		{ "TripleStar.DisplayName", "Tripple Star" },
		{ "TripleStar.Name", "EStarSystemType::TripleStar" },
	};
#endif
	const UECodeGen_Private::FEnumParams Z_Construct_UEnum_APS_ALPHA_EStarSystemType_Statics::EnumParams = {
		(UObject*(*)())Z_Construct_UPackage__Script_APS_ALPHA,
		nullptr,
		"EStarSystemType",
		"EStarSystemType",
		Z_Construct_UEnum_APS_ALPHA_EStarSystemType_Statics::Enumerators,
		UE_ARRAY_COUNT(Z_Construct_UEnum_APS_ALPHA_EStarSystemType_Statics::Enumerators),
		RF_Public|RF_Transient|RF_MarkAsNative,
		EEnumFlags::None,
		(uint8)UEnum::ECppForm::EnumClass,
		METADATA_PARAMS(Z_Construct_UEnum_APS_ALPHA_EStarSystemType_Statics::Enum_MetaDataParams, UE_ARRAY_COUNT(Z_Construct_UEnum_APS_ALPHA_EStarSystemType_Statics::Enum_MetaDataParams))
	};
	UEnum* Z_Construct_UEnum_APS_ALPHA_EStarSystemType()
	{
		if (!Z_Registration_Info_UEnum_EStarSystemType.InnerSingleton)
		{
			UECodeGen_Private::ConstructUEnum(Z_Registration_Info_UEnum_EStarSystemType.InnerSingleton, Z_Construct_UEnum_APS_ALPHA_EStarSystemType_Statics::EnumParams);
		}
		return Z_Registration_Info_UEnum_EStarSystemType.InnerSingleton;
	}
	struct Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_StarSystemType_h_Statics
	{
		static const FEnumRegisterCompiledInInfo EnumInfo[];
	};
	const FEnumRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_StarSystemType_h_Statics::EnumInfo[] = {
		{ EStarSystemType_StaticEnum, TEXT("EStarSystemType"), &Z_Registration_Info_UEnum_EStarSystemType, CONSTRUCT_RELOAD_VERSION_INFO(FEnumReloadVersionInfo, 1630425490U) },
	};
	static FRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_StarSystemType_h_628253923(TEXT("/Script/APS_ALPHA"),
		nullptr, 0,
		nullptr, 0,
		Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_StarSystemType_h_Statics::EnumInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_StarSystemType_h_Statics::EnumInfo));
PRAGMA_ENABLE_DEPRECATION_WARNINGS
