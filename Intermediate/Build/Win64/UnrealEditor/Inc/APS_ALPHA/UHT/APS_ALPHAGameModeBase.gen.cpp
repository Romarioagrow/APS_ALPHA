// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
#include "APS_ALPHA/APS_ALPHAGameModeBase.h"
PRAGMA_DISABLE_DEPRECATION_WARNINGS
void EmptyLinkFunctionForGeneratedCodeAPS_ALPHAGameModeBase() {}
// Cross Module References
	APS_ALPHA_API UClass* Z_Construct_UClass_AAPS_ALPHAGameModeBase();
	APS_ALPHA_API UClass* Z_Construct_UClass_AAPS_ALPHAGameModeBase_NoRegister();
	ENGINE_API UClass* Z_Construct_UClass_AGameModeBase();
	UPackage* Z_Construct_UPackage__Script_APS_ALPHA();
// End Cross Module References
	void AAPS_ALPHAGameModeBase::StaticRegisterNativesAAPS_ALPHAGameModeBase()
	{
	}
	IMPLEMENT_CLASS_NO_AUTO_REGISTRATION(AAPS_ALPHAGameModeBase);
	UClass* Z_Construct_UClass_AAPS_ALPHAGameModeBase_NoRegister()
	{
		return AAPS_ALPHAGameModeBase::StaticClass();
	}
	struct Z_Construct_UClass_AAPS_ALPHAGameModeBase_Statics
	{
		static UObject* (*const DependentSingletons[])();
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam Class_MetaDataParams[];
#endif
		static const FCppClassTypeInfoStatic StaticCppClassTypeInfo;
		static const UECodeGen_Private::FClassParams ClassParams;
	};
	UObject* (*const Z_Construct_UClass_AAPS_ALPHAGameModeBase_Statics::DependentSingletons[])() = {
		(UObject* (*)())Z_Construct_UClass_AGameModeBase,
		(UObject* (*)())Z_Construct_UPackage__Script_APS_ALPHA,
	};
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_AAPS_ALPHAGameModeBase_Statics::Class_MetaDataParams[] = {
		{ "Comment", "/**\n * \n */" },
		{ "HideCategories", "Info Rendering MovementReplication Replication Actor Input Movement Collision Rendering HLOD WorldPartition DataLayers Transformation" },
		{ "IncludePath", "APS_ALPHAGameModeBase.h" },
		{ "ModuleRelativePath", "APS_ALPHAGameModeBase.h" },
		{ "ShowCategories", "Input|MouseInput Input|TouchInput" },
	};
#endif
	const FCppClassTypeInfoStatic Z_Construct_UClass_AAPS_ALPHAGameModeBase_Statics::StaticCppClassTypeInfo = {
		TCppClassTypeTraits<AAPS_ALPHAGameModeBase>::IsAbstract,
	};
	const UECodeGen_Private::FClassParams Z_Construct_UClass_AAPS_ALPHAGameModeBase_Statics::ClassParams = {
		&AAPS_ALPHAGameModeBase::StaticClass,
		"Game",
		&StaticCppClassTypeInfo,
		DependentSingletons,
		nullptr,
		nullptr,
		nullptr,
		UE_ARRAY_COUNT(DependentSingletons),
		0,
		0,
		0,
		0x009002ACu,
		METADATA_PARAMS(Z_Construct_UClass_AAPS_ALPHAGameModeBase_Statics::Class_MetaDataParams, UE_ARRAY_COUNT(Z_Construct_UClass_AAPS_ALPHAGameModeBase_Statics::Class_MetaDataParams))
	};
	UClass* Z_Construct_UClass_AAPS_ALPHAGameModeBase()
	{
		if (!Z_Registration_Info_UClass_AAPS_ALPHAGameModeBase.OuterSingleton)
		{
			UECodeGen_Private::ConstructUClass(Z_Registration_Info_UClass_AAPS_ALPHAGameModeBase.OuterSingleton, Z_Construct_UClass_AAPS_ALPHAGameModeBase_Statics::ClassParams);
		}
		return Z_Registration_Info_UClass_AAPS_ALPHAGameModeBase.OuterSingleton;
	}
	template<> APS_ALPHA_API UClass* StaticClass<AAPS_ALPHAGameModeBase>()
	{
		return AAPS_ALPHAGameModeBase::StaticClass();
	}
	DEFINE_VTABLE_PTR_HELPER_CTOR(AAPS_ALPHAGameModeBase);
	AAPS_ALPHAGameModeBase::~AAPS_ALPHAGameModeBase() {}
	struct Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_APS_ALPHAGameModeBase_h_Statics
	{
		static const FClassRegisterCompiledInInfo ClassInfo[];
	};
	const FClassRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_APS_ALPHAGameModeBase_h_Statics::ClassInfo[] = {
		{ Z_Construct_UClass_AAPS_ALPHAGameModeBase, AAPS_ALPHAGameModeBase::StaticClass, TEXT("AAPS_ALPHAGameModeBase"), &Z_Registration_Info_UClass_AAPS_ALPHAGameModeBase, CONSTRUCT_RELOAD_VERSION_INFO(FClassReloadVersionInfo, sizeof(AAPS_ALPHAGameModeBase), 4199027834U) },
	};
	static FRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_APS_ALPHAGameModeBase_h_286916057(TEXT("/Script/APS_ALPHA"),
		Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_APS_ALPHAGameModeBase_h_Statics::ClassInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_APS_ALPHAGameModeBase_h_Statics::ClassInfo),
		nullptr, 0,
		nullptr, 0);
PRAGMA_ENABLE_DEPRECATION_WARNINGS
