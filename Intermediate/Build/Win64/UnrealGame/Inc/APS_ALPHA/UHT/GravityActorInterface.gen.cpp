// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
#include "APS_ALPHA/GravityActorInterface.h"
PRAGMA_DISABLE_DEPRECATION_WARNINGS
void EmptyLinkFunctionForGeneratedCodeGravityActorInterface() {}
// Cross Module References
	APS_ALPHA_API UClass* Z_Construct_UClass_UGravityActorInterface();
	APS_ALPHA_API UClass* Z_Construct_UClass_UGravityActorInterface_NoRegister();
	COREUOBJECT_API UClass* Z_Construct_UClass_UInterface();
	UPackage* Z_Construct_UPackage__Script_APS_ALPHA();
// End Cross Module References
	void UGravityActorInterface::StaticRegisterNativesUGravityActorInterface()
	{
	}
	IMPLEMENT_CLASS_NO_AUTO_REGISTRATION(UGravityActorInterface);
	UClass* Z_Construct_UClass_UGravityActorInterface_NoRegister()
	{
		return UGravityActorInterface::StaticClass();
	}
	struct Z_Construct_UClass_UGravityActorInterface_Statics
	{
		static UObject* (*const DependentSingletons[])();
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam Class_MetaDataParams[];
#endif
		static const FCppClassTypeInfoStatic StaticCppClassTypeInfo;
		static const UECodeGen_Private::FClassParams ClassParams;
	};
	UObject* (*const Z_Construct_UClass_UGravityActorInterface_Statics::DependentSingletons[])() = {
		(UObject* (*)())Z_Construct_UClass_UInterface,
		(UObject* (*)())Z_Construct_UPackage__Script_APS_ALPHA,
	};
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_UGravityActorInterface_Statics::Class_MetaDataParams[] = {
		{ "ModuleRelativePath", "GravityActorInterface.h" },
	};
#endif
	const FCppClassTypeInfoStatic Z_Construct_UClass_UGravityActorInterface_Statics::StaticCppClassTypeInfo = {
		TCppClassTypeTraits<IGravityActorInterface>::IsAbstract,
	};
	const UECodeGen_Private::FClassParams Z_Construct_UClass_UGravityActorInterface_Statics::ClassParams = {
		&UGravityActorInterface::StaticClass,
		nullptr,
		&StaticCppClassTypeInfo,
		DependentSingletons,
		nullptr,
		nullptr,
		nullptr,
		UE_ARRAY_COUNT(DependentSingletons),
		0,
		0,
		0,
		0x000840A1u,
		METADATA_PARAMS(Z_Construct_UClass_UGravityActorInterface_Statics::Class_MetaDataParams, UE_ARRAY_COUNT(Z_Construct_UClass_UGravityActorInterface_Statics::Class_MetaDataParams))
	};
	UClass* Z_Construct_UClass_UGravityActorInterface()
	{
		if (!Z_Registration_Info_UClass_UGravityActorInterface.OuterSingleton)
		{
			UECodeGen_Private::ConstructUClass(Z_Registration_Info_UClass_UGravityActorInterface.OuterSingleton, Z_Construct_UClass_UGravityActorInterface_Statics::ClassParams);
		}
		return Z_Registration_Info_UClass_UGravityActorInterface.OuterSingleton;
	}
	template<> APS_ALPHA_API UClass* StaticClass<UGravityActorInterface>()
	{
		return UGravityActorInterface::StaticClass();
	}
	DEFINE_VTABLE_PTR_HELPER_CTOR(UGravityActorInterface);
	UGravityActorInterface::~UGravityActorInterface() {}
	struct Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_GravityActorInterface_h_Statics
	{
		static const FClassRegisterCompiledInInfo ClassInfo[];
	};
	const FClassRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_GravityActorInterface_h_Statics::ClassInfo[] = {
		{ Z_Construct_UClass_UGravityActorInterface, UGravityActorInterface::StaticClass, TEXT("UGravityActorInterface"), &Z_Registration_Info_UClass_UGravityActorInterface, CONSTRUCT_RELOAD_VERSION_INFO(FClassReloadVersionInfo, sizeof(UGravityActorInterface), 1909362526U) },
	};
	static FRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_GravityActorInterface_h_2241574065(TEXT("/Script/APS_ALPHA"),
		Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_GravityActorInterface_h_Statics::ClassInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_GravityActorInterface_h_Statics::ClassInfo),
		nullptr, 0,
		nullptr, 0);
PRAGMA_ENABLE_DEPRECATION_WARNINGS
