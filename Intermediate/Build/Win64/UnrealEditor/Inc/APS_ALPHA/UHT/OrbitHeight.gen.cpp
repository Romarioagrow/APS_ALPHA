// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
#include "APS_ALPHA/OrbitHeight.h"
PRAGMA_DISABLE_DEPRECATION_WARNINGS
void EmptyLinkFunctionForGeneratedCodeOrbitHeight() {}
// Cross Module References
	APS_ALPHA_API UEnum* Z_Construct_UEnum_APS_ALPHA_EOrbitHeight();
	UPackage* Z_Construct_UPackage__Script_APS_ALPHA();
// End Cross Module References
	static FEnumRegistrationInfo Z_Registration_Info_UEnum_EOrbitHeight;
	static UEnum* EOrbitHeight_StaticEnum()
	{
		if (!Z_Registration_Info_UEnum_EOrbitHeight.OuterSingleton)
		{
			Z_Registration_Info_UEnum_EOrbitHeight.OuterSingleton = GetStaticEnum(Z_Construct_UEnum_APS_ALPHA_EOrbitHeight, Z_Construct_UPackage__Script_APS_ALPHA(), TEXT("EOrbitHeight"));
		}
		return Z_Registration_Info_UEnum_EOrbitHeight.OuterSingleton;
	}
	template<> APS_ALPHA_API UEnum* StaticEnum<EOrbitHeight>()
	{
		return EOrbitHeight_StaticEnum();
	}
	struct Z_Construct_UEnum_APS_ALPHA_EOrbitHeight_Statics
	{
		static const UECodeGen_Private::FEnumeratorParam Enumerators[];
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam Enum_MetaDataParams[];
#endif
		static const UECodeGen_Private::FEnumParams EnumParams;
	};
	const UECodeGen_Private::FEnumeratorParam Z_Construct_UEnum_APS_ALPHA_EOrbitHeight_Statics::Enumerators[] = {
		{ "EOrbitHeight::UpperAtmosphere", (int64)EOrbitHeight::UpperAtmosphere },
		{ "EOrbitHeight::LowOrbit", (int64)EOrbitHeight::LowOrbit },
		{ "EOrbitHeight::Geostationary", (int64)EOrbitHeight::Geostationary },
		{ "EOrbitHeight::HighOrbit", (int64)EOrbitHeight::HighOrbit },
		{ "EOrbitHeight::VeryHighOrbit", (int64)EOrbitHeight::VeryHighOrbit },
	};
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UEnum_APS_ALPHA_EOrbitHeight_Statics::Enum_MetaDataParams[] = {
		{ "BlueprintType", "true" },
		{ "Geostationary.DisplayName", "Geostationary Orbit" },
		{ "Geostationary.Name", "EOrbitHeight::Geostationary" },
		{ "HighOrbit.DisplayName", "High Orbit" },
		{ "HighOrbit.Name", "EOrbitHeight::HighOrbit" },
		{ "LowOrbit.DisplayName", "Low Orbit" },
		{ "LowOrbit.Name", "EOrbitHeight::LowOrbit" },
		{ "ModuleRelativePath", "OrbitHeight.h" },
		{ "UpperAtmosphere.DisplayName", "Upper Atmosphere" },
		{ "UpperAtmosphere.Name", "EOrbitHeight::UpperAtmosphere" },
		{ "VeryHighOrbit.DisplayName", "Very High Orbit" },
		{ "VeryHighOrbit.Name", "EOrbitHeight::VeryHighOrbit" },
	};
#endif
	const UECodeGen_Private::FEnumParams Z_Construct_UEnum_APS_ALPHA_EOrbitHeight_Statics::EnumParams = {
		(UObject*(*)())Z_Construct_UPackage__Script_APS_ALPHA,
		nullptr,
		"EOrbitHeight",
		"EOrbitHeight",
		Z_Construct_UEnum_APS_ALPHA_EOrbitHeight_Statics::Enumerators,
		UE_ARRAY_COUNT(Z_Construct_UEnum_APS_ALPHA_EOrbitHeight_Statics::Enumerators),
		RF_Public|RF_Transient|RF_MarkAsNative,
		EEnumFlags::None,
		(uint8)UEnum::ECppForm::EnumClass,
		METADATA_PARAMS(Z_Construct_UEnum_APS_ALPHA_EOrbitHeight_Statics::Enum_MetaDataParams, UE_ARRAY_COUNT(Z_Construct_UEnum_APS_ALPHA_EOrbitHeight_Statics::Enum_MetaDataParams))
	};
	UEnum* Z_Construct_UEnum_APS_ALPHA_EOrbitHeight()
	{
		if (!Z_Registration_Info_UEnum_EOrbitHeight.InnerSingleton)
		{
			UECodeGen_Private::ConstructUEnum(Z_Registration_Info_UEnum_EOrbitHeight.InnerSingleton, Z_Construct_UEnum_APS_ALPHA_EOrbitHeight_Statics::EnumParams);
		}
		return Z_Registration_Info_UEnum_EOrbitHeight.InnerSingleton;
	}
	struct Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_OrbitHeight_h_Statics
	{
		static const FEnumRegisterCompiledInInfo EnumInfo[];
	};
	const FEnumRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_OrbitHeight_h_Statics::EnumInfo[] = {
		{ EOrbitHeight_StaticEnum, TEXT("EOrbitHeight"), &Z_Registration_Info_UEnum_EOrbitHeight, CONSTRUCT_RELOAD_VERSION_INFO(FEnumReloadVersionInfo, 2567945888U) },
	};
	static FRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_OrbitHeight_h_898025415(TEXT("/Script/APS_ALPHA"),
		nullptr, 0,
		nullptr, 0,
		Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_OrbitHeight_h_Statics::EnumInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_OrbitHeight_h_Statics::EnumInfo));
PRAGMA_ENABLE_DEPRECATION_WARNINGS
