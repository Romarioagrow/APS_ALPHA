// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
#include "APS_ALPHA/OrbitDistributionType.h"
PRAGMA_DISABLE_DEPRECATION_WARNINGS
void EmptyLinkFunctionForGeneratedCodeOrbitDistributionType() {}
// Cross Module References
	APS_ALPHA_API UEnum* Z_Construct_UEnum_APS_ALPHA_EOrbitDistributionType();
	UPackage* Z_Construct_UPackage__Script_APS_ALPHA();
// End Cross Module References
	static FEnumRegistrationInfo Z_Registration_Info_UEnum_EOrbitDistributionType;
	static UEnum* EOrbitDistributionType_StaticEnum()
	{
		if (!Z_Registration_Info_UEnum_EOrbitDistributionType.OuterSingleton)
		{
			Z_Registration_Info_UEnum_EOrbitDistributionType.OuterSingleton = GetStaticEnum(Z_Construct_UEnum_APS_ALPHA_EOrbitDistributionType, Z_Construct_UPackage__Script_APS_ALPHA(), TEXT("EOrbitDistributionType"));
		}
		return Z_Registration_Info_UEnum_EOrbitDistributionType.OuterSingleton;
	}
	template<> APS_ALPHA_API UEnum* StaticEnum<EOrbitDistributionType>()
	{
		return EOrbitDistributionType_StaticEnum();
	}
	struct Z_Construct_UEnum_APS_ALPHA_EOrbitDistributionType_Statics
	{
		static const UECodeGen_Private::FEnumeratorParam Enumerators[];
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam Enum_MetaDataParams[];
#endif
		static const UECodeGen_Private::FEnumParams EnumParams;
	};
	const UECodeGen_Private::FEnumeratorParam Z_Construct_UEnum_APS_ALPHA_EOrbitDistributionType_Statics::Enumerators[] = {
		{ "EOrbitDistributionType::Uniform", (int64)EOrbitDistributionType::Uniform },
		{ "EOrbitDistributionType::Gaussian", (int64)EOrbitDistributionType::Gaussian },
		{ "EOrbitDistributionType::Chaotic", (int64)EOrbitDistributionType::Chaotic },
		{ "EOrbitDistributionType::InnerOuter", (int64)EOrbitDistributionType::InnerOuter },
		{ "EOrbitDistributionType::Dense", (int64)EOrbitDistributionType::Dense },
		{ "EOrbitDistributionType::Count", (int64)EOrbitDistributionType::Count },
	};
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UEnum_APS_ALPHA_EOrbitDistributionType_Statics::Enum_MetaDataParams[] = {
		{ "BlueprintType", "true" },
		{ "Chaotic.DisplayName", "Chaotic" },
		{ "Chaotic.Name", "EOrbitDistributionType::Chaotic" },
		{ "Count.DisplayName", "Count" },
		{ "Count.Name", "EOrbitDistributionType::Count" },
		{ "Dense.DisplayName", "Dense" },
		{ "Dense.Name", "EOrbitDistributionType::Dense" },
		{ "Gaussian.DisplayName", "Gaussian" },
		{ "Gaussian.Name", "EOrbitDistributionType::Gaussian" },
		{ "InnerOuter.DisplayName", "InnerOuter" },
		{ "InnerOuter.Name", "EOrbitDistributionType::InnerOuter" },
		{ "ModuleRelativePath", "OrbitDistributionType.h" },
		{ "Uniform.DisplayName", "Uniform" },
		{ "Uniform.Name", "EOrbitDistributionType::Uniform" },
	};
#endif
	const UECodeGen_Private::FEnumParams Z_Construct_UEnum_APS_ALPHA_EOrbitDistributionType_Statics::EnumParams = {
		(UObject*(*)())Z_Construct_UPackage__Script_APS_ALPHA,
		nullptr,
		"EOrbitDistributionType",
		"EOrbitDistributionType",
		Z_Construct_UEnum_APS_ALPHA_EOrbitDistributionType_Statics::Enumerators,
		UE_ARRAY_COUNT(Z_Construct_UEnum_APS_ALPHA_EOrbitDistributionType_Statics::Enumerators),
		RF_Public|RF_Transient|RF_MarkAsNative,
		EEnumFlags::None,
		(uint8)UEnum::ECppForm::EnumClass,
		METADATA_PARAMS(Z_Construct_UEnum_APS_ALPHA_EOrbitDistributionType_Statics::Enum_MetaDataParams, UE_ARRAY_COUNT(Z_Construct_UEnum_APS_ALPHA_EOrbitDistributionType_Statics::Enum_MetaDataParams))
	};
	UEnum* Z_Construct_UEnum_APS_ALPHA_EOrbitDistributionType()
	{
		if (!Z_Registration_Info_UEnum_EOrbitDistributionType.InnerSingleton)
		{
			UECodeGen_Private::ConstructUEnum(Z_Registration_Info_UEnum_EOrbitDistributionType.InnerSingleton, Z_Construct_UEnum_APS_ALPHA_EOrbitDistributionType_Statics::EnumParams);
		}
		return Z_Registration_Info_UEnum_EOrbitDistributionType.InnerSingleton;
	}
	struct Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_OrbitDistributionType_h_Statics
	{
		static const FEnumRegisterCompiledInInfo EnumInfo[];
	};
	const FEnumRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_OrbitDistributionType_h_Statics::EnumInfo[] = {
		{ EOrbitDistributionType_StaticEnum, TEXT("EOrbitDistributionType"), &Z_Registration_Info_UEnum_EOrbitDistributionType, CONSTRUCT_RELOAD_VERSION_INFO(FEnumReloadVersionInfo, 2085035236U) },
	};
	static FRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_OrbitDistributionType_h_771953808(TEXT("/Script/APS_ALPHA"),
		nullptr, 0,
		nullptr, 0,
		Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_OrbitDistributionType_h_Statics::EnumInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_OrbitDistributionType_h_Statics::EnumInfo));
PRAGMA_ENABLE_DEPRECATION_WARNINGS
