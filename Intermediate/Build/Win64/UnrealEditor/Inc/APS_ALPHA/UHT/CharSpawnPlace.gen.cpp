// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
#include "APS_ALPHA/CharSpawnPlace.h"
PRAGMA_DISABLE_DEPRECATION_WARNINGS
void EmptyLinkFunctionForGeneratedCodeCharSpawnPlace() {}
// Cross Module References
	APS_ALPHA_API UEnum* Z_Construct_UEnum_APS_ALPHA_ECharSpawnPlace();
	UPackage* Z_Construct_UPackage__Script_APS_ALPHA();
// End Cross Module References
	static FEnumRegistrationInfo Z_Registration_Info_UEnum_ECharSpawnPlace;
	static UEnum* ECharSpawnPlace_StaticEnum()
	{
		if (!Z_Registration_Info_UEnum_ECharSpawnPlace.OuterSingleton)
		{
			Z_Registration_Info_UEnum_ECharSpawnPlace.OuterSingleton = GetStaticEnum(Z_Construct_UEnum_APS_ALPHA_ECharSpawnPlace, Z_Construct_UPackage__Script_APS_ALPHA(), TEXT("ECharSpawnPlace"));
		}
		return Z_Registration_Info_UEnum_ECharSpawnPlace.OuterSingleton;
	}
	template<> APS_ALPHA_API UEnum* StaticEnum<ECharSpawnPlace>()
	{
		return ECharSpawnPlace_StaticEnum();
	}
	struct Z_Construct_UEnum_APS_ALPHA_ECharSpawnPlace_Statics
	{
		static const UECodeGen_Private::FEnumeratorParam Enumerators[];
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam Enum_MetaDataParams[];
#endif
		static const UECodeGen_Private::FEnumParams EnumParams;
	};
	const UECodeGen_Private::FEnumeratorParam Z_Construct_UEnum_APS_ALPHA_ECharSpawnPlace_Statics::Enumerators[] = {
		{ "ECharSpawnPlace::PlanetOrbit", (int64)ECharSpawnPlace::PlanetOrbit },
		{ "ECharSpawnPlace::PlanetSurface", (int64)ECharSpawnPlace::PlanetSurface },
		{ "ECharSpawnPlace::MoonOrbit", (int64)ECharSpawnPlace::MoonOrbit },
		{ "ECharSpawnPlace::MoonSurface", (int64)ECharSpawnPlace::MoonSurface },
		{ "ECharSpawnPlace::SpaceShip", (int64)ECharSpawnPlace::SpaceShip },
	};
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UEnum_APS_ALPHA_ECharSpawnPlace_Statics::Enum_MetaDataParams[] = {
		{ "BlueprintType", "true" },
		{ "ModuleRelativePath", "CharSpawnPlace.h" },
		{ "MoonOrbit.DisplayName", "Moon Orbit" },
		{ "MoonOrbit.Name", "ECharSpawnPlace::MoonOrbit" },
		{ "MoonSurface.DisplayName", "Moon Surface" },
		{ "MoonSurface.Name", "ECharSpawnPlace::MoonSurface" },
		{ "PlanetOrbit.DisplayName", "Planet Orbit" },
		{ "PlanetOrbit.Name", "ECharSpawnPlace::PlanetOrbit" },
		{ "PlanetSurface.DisplayName", "Planet Surface" },
		{ "PlanetSurface.Name", "ECharSpawnPlace::PlanetSurface" },
		{ "SpaceShip.DisplayName", "Space Ship" },
		{ "SpaceShip.Name", "ECharSpawnPlace::SpaceShip" },
	};
#endif
	const UECodeGen_Private::FEnumParams Z_Construct_UEnum_APS_ALPHA_ECharSpawnPlace_Statics::EnumParams = {
		(UObject*(*)())Z_Construct_UPackage__Script_APS_ALPHA,
		nullptr,
		"ECharSpawnPlace",
		"ECharSpawnPlace",
		Z_Construct_UEnum_APS_ALPHA_ECharSpawnPlace_Statics::Enumerators,
		UE_ARRAY_COUNT(Z_Construct_UEnum_APS_ALPHA_ECharSpawnPlace_Statics::Enumerators),
		RF_Public|RF_Transient|RF_MarkAsNative,
		EEnumFlags::None,
		(uint8)UEnum::ECppForm::EnumClass,
		METADATA_PARAMS(Z_Construct_UEnum_APS_ALPHA_ECharSpawnPlace_Statics::Enum_MetaDataParams, UE_ARRAY_COUNT(Z_Construct_UEnum_APS_ALPHA_ECharSpawnPlace_Statics::Enum_MetaDataParams))
	};
	UEnum* Z_Construct_UEnum_APS_ALPHA_ECharSpawnPlace()
	{
		if (!Z_Registration_Info_UEnum_ECharSpawnPlace.InnerSingleton)
		{
			UECodeGen_Private::ConstructUEnum(Z_Registration_Info_UEnum_ECharSpawnPlace.InnerSingleton, Z_Construct_UEnum_APS_ALPHA_ECharSpawnPlace_Statics::EnumParams);
		}
		return Z_Registration_Info_UEnum_ECharSpawnPlace.InnerSingleton;
	}
	struct Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_CharSpawnPlace_h_Statics
	{
		static const FEnumRegisterCompiledInInfo EnumInfo[];
	};
	const FEnumRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_CharSpawnPlace_h_Statics::EnumInfo[] = {
		{ ECharSpawnPlace_StaticEnum, TEXT("ECharSpawnPlace"), &Z_Registration_Info_UEnum_ECharSpawnPlace, CONSTRUCT_RELOAD_VERSION_INFO(FEnumReloadVersionInfo, 4286103121U) },
	};
	static FRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_CharSpawnPlace_h_2668666950(TEXT("/Script/APS_ALPHA"),
		nullptr, 0,
		nullptr, 0,
		Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_CharSpawnPlace_h_Statics::EnumInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_CharSpawnPlace_h_Statics::EnumInfo));
PRAGMA_ENABLE_DEPRECATION_WARNINGS
