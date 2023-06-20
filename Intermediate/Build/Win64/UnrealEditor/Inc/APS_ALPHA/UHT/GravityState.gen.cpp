// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
#include "APS_ALPHA/GravityState.h"
PRAGMA_DISABLE_DEPRECATION_WARNINGS
void EmptyLinkFunctionForGeneratedCodeGravityState() {}
// Cross Module References
	APS_ALPHA_API UEnum* Z_Construct_UEnum_APS_ALPHA_EGravityState();
	UPackage* Z_Construct_UPackage__Script_APS_ALPHA();
// End Cross Module References
	static FEnumRegistrationInfo Z_Registration_Info_UEnum_EGravityState;
	static UEnum* EGravityState_StaticEnum()
	{
		if (!Z_Registration_Info_UEnum_EGravityState.OuterSingleton)
		{
			Z_Registration_Info_UEnum_EGravityState.OuterSingleton = GetStaticEnum(Z_Construct_UEnum_APS_ALPHA_EGravityState, Z_Construct_UPackage__Script_APS_ALPHA(), TEXT("EGravityState"));
		}
		return Z_Registration_Info_UEnum_EGravityState.OuterSingleton;
	}
	template<> APS_ALPHA_API UEnum* StaticEnum<EGravityState>()
	{
		return EGravityState_StaticEnum();
	}
	struct Z_Construct_UEnum_APS_ALPHA_EGravityState_Statics
	{
		static const UECodeGen_Private::FEnumeratorParam Enumerators[];
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam Enum_MetaDataParams[];
#endif
		static const UECodeGen_Private::FEnumParams EnumParams;
	};
	const UECodeGen_Private::FEnumeratorParam Z_Construct_UEnum_APS_ALPHA_EGravityState_Statics::Enumerators[] = {
		{ "EGravityState::LowG", (int64)EGravityState::LowG },
		{ "EGravityState::Attracting", (int64)EGravityState::Attracting },
		{ "EGravityState::Jumping", (int64)EGravityState::Jumping },
		{ "EGravityState::Falling", (int64)EGravityState::Falling },
		{ "EGravityState::Attracted", (int64)EGravityState::Attracted },
	};
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UEnum_APS_ALPHA_EGravityState_Statics::Enum_MetaDataParams[] = {
		{ "Attracted.Name", "EGravityState::Attracted" },
		{ "Attracting.Name", "EGravityState::Attracting" },
		{ "BlueprintType", "true" },
		{ "Falling.Name", "EGravityState::Falling" },
		{ "Jumping.Name", "EGravityState::Jumping" },
		{ "LowG.Name", "EGravityState::LowG" },
		{ "ModuleRelativePath", "GravityState.h" },
	};
#endif
	const UECodeGen_Private::FEnumParams Z_Construct_UEnum_APS_ALPHA_EGravityState_Statics::EnumParams = {
		(UObject*(*)())Z_Construct_UPackage__Script_APS_ALPHA,
		nullptr,
		"EGravityState",
		"EGravityState",
		Z_Construct_UEnum_APS_ALPHA_EGravityState_Statics::Enumerators,
		UE_ARRAY_COUNT(Z_Construct_UEnum_APS_ALPHA_EGravityState_Statics::Enumerators),
		RF_Public|RF_Transient|RF_MarkAsNative,
		EEnumFlags::None,
		(uint8)UEnum::ECppForm::EnumClass,
		METADATA_PARAMS(Z_Construct_UEnum_APS_ALPHA_EGravityState_Statics::Enum_MetaDataParams, UE_ARRAY_COUNT(Z_Construct_UEnum_APS_ALPHA_EGravityState_Statics::Enum_MetaDataParams))
	};
	UEnum* Z_Construct_UEnum_APS_ALPHA_EGravityState()
	{
		if (!Z_Registration_Info_UEnum_EGravityState.InnerSingleton)
		{
			UECodeGen_Private::ConstructUEnum(Z_Registration_Info_UEnum_EGravityState.InnerSingleton, Z_Construct_UEnum_APS_ALPHA_EGravityState_Statics::EnumParams);
		}
		return Z_Registration_Info_UEnum_EGravityState.InnerSingleton;
	}
	struct Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_GravityState_h_Statics
	{
		static const FEnumRegisterCompiledInInfo EnumInfo[];
	};
	const FEnumRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_GravityState_h_Statics::EnumInfo[] = {
		{ EGravityState_StaticEnum, TEXT("EGravityState"), &Z_Registration_Info_UEnum_EGravityState, CONSTRUCT_RELOAD_VERSION_INFO(FEnumReloadVersionInfo, 2783392099U) },
	};
	static FRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_GravityState_h_253916584(TEXT("/Script/APS_ALPHA"),
		nullptr, 0,
		nullptr, 0,
		Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_GravityState_h_Statics::EnumInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_GravityState_h_Statics::EnumInfo));
PRAGMA_ENABLE_DEPRECATION_WARNINGS
