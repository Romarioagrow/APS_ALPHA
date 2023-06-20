// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
#include "APS_ALPHA/GalaxyClass.h"
PRAGMA_DISABLE_DEPRECATION_WARNINGS
void EmptyLinkFunctionForGeneratedCodeGalaxyClass() {}
// Cross Module References
	APS_ALPHA_API UEnum* Z_Construct_UEnum_APS_ALPHA_EGalaxyClass();
	UPackage* Z_Construct_UPackage__Script_APS_ALPHA();
// End Cross Module References
	static FEnumRegistrationInfo Z_Registration_Info_UEnum_EGalaxyClass;
	static UEnum* EGalaxyClass_StaticEnum()
	{
		if (!Z_Registration_Info_UEnum_EGalaxyClass.OuterSingleton)
		{
			Z_Registration_Info_UEnum_EGalaxyClass.OuterSingleton = GetStaticEnum(Z_Construct_UEnum_APS_ALPHA_EGalaxyClass, Z_Construct_UPackage__Script_APS_ALPHA(), TEXT("EGalaxyClass"));
		}
		return Z_Registration_Info_UEnum_EGalaxyClass.OuterSingleton;
	}
	template<> APS_ALPHA_API UEnum* StaticEnum<EGalaxyClass>()
	{
		return EGalaxyClass_StaticEnum();
	}
	struct Z_Construct_UEnum_APS_ALPHA_EGalaxyClass_Statics
	{
		static const UECodeGen_Private::FEnumeratorParam Enumerators[];
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam Enum_MetaDataParams[];
#endif
		static const UECodeGen_Private::FEnumParams EnumParams;
	};
	const UECodeGen_Private::FEnumeratorParam Z_Construct_UEnum_APS_ALPHA_EGalaxyClass_Statics::Enumerators[] = {
		{ "EGalaxyClass::E0", (int64)EGalaxyClass::E0 },
		{ "EGalaxyClass::E1", (int64)EGalaxyClass::E1 },
		{ "EGalaxyClass::E2", (int64)EGalaxyClass::E2 },
		{ "EGalaxyClass::E3", (int64)EGalaxyClass::E3 },
		{ "EGalaxyClass::E4", (int64)EGalaxyClass::E4 },
		{ "EGalaxyClass::E5", (int64)EGalaxyClass::E5 },
		{ "EGalaxyClass::E6", (int64)EGalaxyClass::E6 },
		{ "EGalaxyClass::E7", (int64)EGalaxyClass::E7 },
		{ "EGalaxyClass::S0", (int64)EGalaxyClass::S0 },
		{ "EGalaxyClass::Sa", (int64)EGalaxyClass::Sa },
		{ "EGalaxyClass::Sb", (int64)EGalaxyClass::Sb },
		{ "EGalaxyClass::Sc", (int64)EGalaxyClass::Sc },
		{ "EGalaxyClass::Sd", (int64)EGalaxyClass::Sd },
		{ "EGalaxyClass::SBa", (int64)EGalaxyClass::SBa },
		{ "EGalaxyClass::SBb", (int64)EGalaxyClass::SBb },
		{ "EGalaxyClass::SBc", (int64)EGalaxyClass::SBc },
		{ "EGalaxyClass::SBd", (int64)EGalaxyClass::SBd },
		{ "EGalaxyClass::Irr", (int64)EGalaxyClass::Irr },
		{ "EGalaxyClass::Unknown", (int64)EGalaxyClass::Unknown },
	};
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UEnum_APS_ALPHA_EGalaxyClass_Statics::Enum_MetaDataParams[] = {
		{ "BlueprintType", "true" },
		{ "E0.DisplayName", "E0 Elliptical" },
		{ "E0.Name", "EGalaxyClass::E0" },
		{ "E1.DisplayName", "E1 Elliptical" },
		{ "E1.Name", "EGalaxyClass::E1" },
		{ "E2.DisplayName", "E2 Elliptical" },
		{ "E2.Name", "EGalaxyClass::E2" },
		{ "E3.DisplayName", "E3 Elliptical" },
		{ "E3.Name", "EGalaxyClass::E3" },
		{ "E4.DisplayName", "E4 Elliptical" },
		{ "E4.Name", "EGalaxyClass::E4" },
		{ "E5.DisplayName", "E5 Elliptical" },
		{ "E5.Name", "EGalaxyClass::E5" },
		{ "E6.DisplayName", "E6 Elliptical" },
		{ "E6.Name", "EGalaxyClass::E6" },
		{ "E7.DisplayName", "E7 Elliptical" },
		{ "E7.Name", "EGalaxyClass::E7" },
		{ "Irr.DisplayName", "Irr Irregular" },
		{ "Irr.Name", "EGalaxyClass::Irr" },
		{ "ModuleRelativePath", "GalaxyClass.h" },
		{ "S0.DisplayName", "S0 Lenticular" },
		{ "S0.Name", "EGalaxyClass::S0" },
		{ "Sa.DisplayName", "Sa Spiral" },
		{ "Sa.Name", "EGalaxyClass::Sa" },
		{ "Sb.DisplayName", "Sb Spiral" },
		{ "Sb.Name", "EGalaxyClass::Sb" },
		{ "SBa.DisplayName", "SBa Barred Spiral" },
		{ "SBa.Name", "EGalaxyClass::SBa" },
		{ "SBb.DisplayName", "SBb Barred Spiral" },
		{ "SBb.Name", "EGalaxyClass::SBb" },
		{ "SBc.DisplayName", "SBc Barred Spiral" },
		{ "SBc.Name", "EGalaxyClass::SBc" },
		{ "SBd.DisplayName", "SBd Barred Spiral" },
		{ "SBd.Name", "EGalaxyClass::SBd" },
		{ "Sc.DisplayName", "Sc Spiral" },
		{ "Sc.Name", "EGalaxyClass::Sc" },
		{ "Sd.DisplayName", "Sd Spiral" },
		{ "Sd.Name", "EGalaxyClass::Sd" },
		{ "Unknown.DisplayName", "Unknown" },
		{ "Unknown.Name", "EGalaxyClass::Unknown" },
	};
#endif
	const UECodeGen_Private::FEnumParams Z_Construct_UEnum_APS_ALPHA_EGalaxyClass_Statics::EnumParams = {
		(UObject*(*)())Z_Construct_UPackage__Script_APS_ALPHA,
		nullptr,
		"EGalaxyClass",
		"EGalaxyClass",
		Z_Construct_UEnum_APS_ALPHA_EGalaxyClass_Statics::Enumerators,
		UE_ARRAY_COUNT(Z_Construct_UEnum_APS_ALPHA_EGalaxyClass_Statics::Enumerators),
		RF_Public|RF_Transient|RF_MarkAsNative,
		EEnumFlags::None,
		(uint8)UEnum::ECppForm::EnumClass,
		METADATA_PARAMS(Z_Construct_UEnum_APS_ALPHA_EGalaxyClass_Statics::Enum_MetaDataParams, UE_ARRAY_COUNT(Z_Construct_UEnum_APS_ALPHA_EGalaxyClass_Statics::Enum_MetaDataParams))
	};
	UEnum* Z_Construct_UEnum_APS_ALPHA_EGalaxyClass()
	{
		if (!Z_Registration_Info_UEnum_EGalaxyClass.InnerSingleton)
		{
			UECodeGen_Private::ConstructUEnum(Z_Registration_Info_UEnum_EGalaxyClass.InnerSingleton, Z_Construct_UEnum_APS_ALPHA_EGalaxyClass_Statics::EnumParams);
		}
		return Z_Registration_Info_UEnum_EGalaxyClass.InnerSingleton;
	}
	struct Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_GalaxyClass_h_Statics
	{
		static const FEnumRegisterCompiledInInfo EnumInfo[];
	};
	const FEnumRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_GalaxyClass_h_Statics::EnumInfo[] = {
		{ EGalaxyClass_StaticEnum, TEXT("EGalaxyClass"), &Z_Registration_Info_UEnum_EGalaxyClass, CONSTRUCT_RELOAD_VERSION_INFO(FEnumReloadVersionInfo, 1326469318U) },
	};
	static FRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_GalaxyClass_h_2208800511(TEXT("/Script/APS_ALPHA"),
		nullptr, 0,
		nullptr, 0,
		Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_GalaxyClass_h_Statics::EnumInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_GalaxyClass_h_Statics::EnumInfo));
PRAGMA_ENABLE_DEPRECATION_WARNINGS
