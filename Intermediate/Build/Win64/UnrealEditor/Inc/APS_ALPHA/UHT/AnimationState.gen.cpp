// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
#include "APS_ALPHA/AnimationState.h"
PRAGMA_DISABLE_DEPRECATION_WARNINGS
void EmptyLinkFunctionForGeneratedCodeAnimationState() {}
// Cross Module References
	APS_ALPHA_API UEnum* Z_Construct_UEnum_APS_ALPHA_EAnimationState();
	UPackage* Z_Construct_UPackage__Script_APS_ALPHA();
// End Cross Module References
	static FEnumRegistrationInfo Z_Registration_Info_UEnum_EAnimationState;
	static UEnum* EAnimationState_StaticEnum()
	{
		if (!Z_Registration_Info_UEnum_EAnimationState.OuterSingleton)
		{
			Z_Registration_Info_UEnum_EAnimationState.OuterSingleton = GetStaticEnum(Z_Construct_UEnum_APS_ALPHA_EAnimationState, Z_Construct_UPackage__Script_APS_ALPHA(), TEXT("EAnimationState"));
		}
		return Z_Registration_Info_UEnum_EAnimationState.OuterSingleton;
	}
	template<> APS_ALPHA_API UEnum* StaticEnum<EAnimationState>()
	{
		return EAnimationState_StaticEnum();
	}
	struct Z_Construct_UEnum_APS_ALPHA_EAnimationState_Statics
	{
		static const UECodeGen_Private::FEnumeratorParam Enumerators[];
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam Enum_MetaDataParams[];
#endif
		static const UECodeGen_Private::FEnumParams EnumParams;
	};
	const UECodeGen_Private::FEnumeratorParam Z_Construct_UEnum_APS_ALPHA_EAnimationState_Statics::Enumerators[] = {
		{ "EAnimationState::Idle", (int64)EAnimationState::Idle },
		{ "EAnimationState::Walk", (int64)EAnimationState::Walk },
		{ "EAnimationState::Run", (int64)EAnimationState::Run },
		{ "EAnimationState::Jumping", (int64)EAnimationState::Jumping },
		{ "EAnimationState::Falling", (int64)EAnimationState::Falling },
		{ "EAnimationState::Floating", (int64)EAnimationState::Floating },
		{ "EAnimationState::OnGround", (int64)EAnimationState::OnGround },
	};
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UEnum_APS_ALPHA_EAnimationState_Statics::Enum_MetaDataParams[] = {
		{ "BlueprintType", "true" },
		{ "Falling.Name", "EAnimationState::Falling" },
		{ "Floating.Name", "EAnimationState::Floating" },
		{ "Idle.Name", "EAnimationState::Idle" },
		{ "Jumping.Name", "EAnimationState::Jumping" },
		{ "ModuleRelativePath", "AnimationState.h" },
		{ "OnGround.Name", "EAnimationState::OnGround" },
		{ "Run.Name", "EAnimationState::Run" },
		{ "Walk.Name", "EAnimationState::Walk" },
	};
#endif
	const UECodeGen_Private::FEnumParams Z_Construct_UEnum_APS_ALPHA_EAnimationState_Statics::EnumParams = {
		(UObject*(*)())Z_Construct_UPackage__Script_APS_ALPHA,
		nullptr,
		"EAnimationState",
		"EAnimationState",
		Z_Construct_UEnum_APS_ALPHA_EAnimationState_Statics::Enumerators,
		UE_ARRAY_COUNT(Z_Construct_UEnum_APS_ALPHA_EAnimationState_Statics::Enumerators),
		RF_Public|RF_Transient|RF_MarkAsNative,
		EEnumFlags::None,
		(uint8)UEnum::ECppForm::EnumClass,
		METADATA_PARAMS(Z_Construct_UEnum_APS_ALPHA_EAnimationState_Statics::Enum_MetaDataParams, UE_ARRAY_COUNT(Z_Construct_UEnum_APS_ALPHA_EAnimationState_Statics::Enum_MetaDataParams))
	};
	UEnum* Z_Construct_UEnum_APS_ALPHA_EAnimationState()
	{
		if (!Z_Registration_Info_UEnum_EAnimationState.InnerSingleton)
		{
			UECodeGen_Private::ConstructUEnum(Z_Registration_Info_UEnum_EAnimationState.InnerSingleton, Z_Construct_UEnum_APS_ALPHA_EAnimationState_Statics::EnumParams);
		}
		return Z_Registration_Info_UEnum_EAnimationState.InnerSingleton;
	}
	struct Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_AnimationState_h_Statics
	{
		static const FEnumRegisterCompiledInInfo EnumInfo[];
	};
	const FEnumRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_AnimationState_h_Statics::EnumInfo[] = {
		{ EAnimationState_StaticEnum, TEXT("EAnimationState"), &Z_Registration_Info_UEnum_EAnimationState, CONSTRUCT_RELOAD_VERSION_INFO(FEnumReloadVersionInfo, 2413416102U) },
	};
	static FRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_AnimationState_h_2194803722(TEXT("/Script/APS_ALPHA"),
		nullptr, 0,
		nullptr, 0,
		Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_AnimationState_h_Statics::EnumInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_AnimationState_h_Statics::EnumInfo));
PRAGMA_ENABLE_DEPRECATION_WARNINGS
