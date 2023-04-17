// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
#include "APS_ALPHA/GravityTypeEnum.h"
PRAGMA_DISABLE_DEPRECATION_WARNINGS
void EmptyLinkFunctionForGeneratedCodeGravityTypeEnum() {}
// Cross Module References
	APS_ALPHA_API UEnum* Z_Construct_UEnum_APS_ALPHA_EGravityType();
	UPackage* Z_Construct_UPackage__Script_APS_ALPHA();
// End Cross Module References
	static FEnumRegistrationInfo Z_Registration_Info_UEnum_EGravityType;
	static UEnum* EGravityType_StaticEnum()
	{
		if (!Z_Registration_Info_UEnum_EGravityType.OuterSingleton)
		{
			Z_Registration_Info_UEnum_EGravityType.OuterSingleton = GetStaticEnum(Z_Construct_UEnum_APS_ALPHA_EGravityType, Z_Construct_UPackage__Script_APS_ALPHA(), TEXT("EGravityType"));
		}
		return Z_Registration_Info_UEnum_EGravityType.OuterSingleton;
	}
	template<> APS_ALPHA_API UEnum* StaticEnum<EGravityType>()
	{
		return EGravityType_StaticEnum();
	}
	struct Z_Construct_UEnum_APS_ALPHA_EGravityType_Statics
	{
		static const UECodeGen_Private::FEnumeratorParam Enumerators[];
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam Enum_MetaDataParams[];
#endif
		static const UECodeGen_Private::FEnumParams EnumParams;
	};
	const UECodeGen_Private::FEnumeratorParam Z_Construct_UEnum_APS_ALPHA_EGravityType_Statics::Enumerators[] = {
		{ "EGravityType::ZeroG", (int64)EGravityType::ZeroG },
		{ "EGravityType::OnStation", (int64)EGravityType::OnStation },
		{ "EGravityType::OnPlanet", (int64)EGravityType::OnPlanet },
		{ "EGravityType::OnShip", (int64)EGravityType::OnShip },
	};
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UEnum_APS_ALPHA_EGravityType_Statics::Enum_MetaDataParams[] = {
		{ "BlueprintType", "true" },
		{ "ModuleRelativePath", "GravityTypeEnum.h" },
		{ "OnPlanet.Name", "EGravityType::OnPlanet" },
		{ "OnShip.Name", "EGravityType::OnShip" },
		{ "OnStation.Name", "EGravityType::OnStation" },
		{ "ZeroG.Name", "EGravityType::ZeroG" },
	};
#endif
	const UECodeGen_Private::FEnumParams Z_Construct_UEnum_APS_ALPHA_EGravityType_Statics::EnumParams = {
		(UObject*(*)())Z_Construct_UPackage__Script_APS_ALPHA,
		nullptr,
		"EGravityType",
		"EGravityType",
		Z_Construct_UEnum_APS_ALPHA_EGravityType_Statics::Enumerators,
		UE_ARRAY_COUNT(Z_Construct_UEnum_APS_ALPHA_EGravityType_Statics::Enumerators),
		RF_Public|RF_Transient|RF_MarkAsNative,
		EEnumFlags::None,
		(uint8)UEnum::ECppForm::EnumClass,
		METADATA_PARAMS(Z_Construct_UEnum_APS_ALPHA_EGravityType_Statics::Enum_MetaDataParams, UE_ARRAY_COUNT(Z_Construct_UEnum_APS_ALPHA_EGravityType_Statics::Enum_MetaDataParams))
	};
	UEnum* Z_Construct_UEnum_APS_ALPHA_EGravityType()
	{
		if (!Z_Registration_Info_UEnum_EGravityType.InnerSingleton)
		{
			UECodeGen_Private::ConstructUEnum(Z_Registration_Info_UEnum_EGravityType.InnerSingleton, Z_Construct_UEnum_APS_ALPHA_EGravityType_Statics::EnumParams);
		}
		return Z_Registration_Info_UEnum_EGravityType.InnerSingleton;
	}
	struct Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_GravityTypeEnum_h_Statics
	{
		static const FEnumRegisterCompiledInInfo EnumInfo[];
	};
	const FEnumRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_GravityTypeEnum_h_Statics::EnumInfo[] = {
		{ EGravityType_StaticEnum, TEXT("EGravityType"), &Z_Registration_Info_UEnum_EGravityType, CONSTRUCT_RELOAD_VERSION_INFO(FEnumReloadVersionInfo, 459840205U) },
	};
	static FRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_GravityTypeEnum_h_1327031895(TEXT("/Script/APS_ALPHA"),
		nullptr, 0,
		nullptr, 0,
		Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_GravityTypeEnum_h_Statics::EnumInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_GravityTypeEnum_h_Statics::EnumInfo));
PRAGMA_ENABLE_DEPRECATION_WARNINGS
