// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
#include "APS_ALPHA/GravityObject.h"
PRAGMA_DISABLE_DEPRECATION_WARNINGS
void EmptyLinkFunctionForGeneratedCodeGravityObject() {}
// Cross Module References
	APS_ALPHA_API UClass* Z_Construct_UClass_UBaseInterface();
	APS_ALPHA_API UClass* Z_Construct_UClass_UGravityObject();
	APS_ALPHA_API UClass* Z_Construct_UClass_UGravityObject_NoRegister();
	UPackage* Z_Construct_UPackage__Script_APS_ALPHA();
// End Cross Module References
	void UGravityObject::StaticRegisterNativesUGravityObject()
	{
	}
	IMPLEMENT_CLASS_NO_AUTO_REGISTRATION(UGravityObject);
	UClass* Z_Construct_UClass_UGravityObject_NoRegister()
	{
		return UGravityObject::StaticClass();
	}
	struct Z_Construct_UClass_UGravityObject_Statics
	{
		static UObject* (*const DependentSingletons[])();
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam Class_MetaDataParams[];
#endif
		static const FCppClassTypeInfoStatic StaticCppClassTypeInfo;
		static const UECodeGen_Private::FClassParams ClassParams;
	};
	UObject* (*const Z_Construct_UClass_UGravityObject_Statics::DependentSingletons[])() = {
		(UObject* (*)())Z_Construct_UClass_UBaseInterface,
		(UObject* (*)())Z_Construct_UPackage__Script_APS_ALPHA,
	};
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_UGravityObject_Statics::Class_MetaDataParams[] = {
		{ "ModuleRelativePath", "GravityObject.h" },
	};
#endif
	const FCppClassTypeInfoStatic Z_Construct_UClass_UGravityObject_Statics::StaticCppClassTypeInfo = {
		TCppClassTypeTraits<IGravityObject>::IsAbstract,
	};
	const UECodeGen_Private::FClassParams Z_Construct_UClass_UGravityObject_Statics::ClassParams = {
		&UGravityObject::StaticClass,
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
		METADATA_PARAMS(Z_Construct_UClass_UGravityObject_Statics::Class_MetaDataParams, UE_ARRAY_COUNT(Z_Construct_UClass_UGravityObject_Statics::Class_MetaDataParams))
	};
	UClass* Z_Construct_UClass_UGravityObject()
	{
		if (!Z_Registration_Info_UClass_UGravityObject.OuterSingleton)
		{
			UECodeGen_Private::ConstructUClass(Z_Registration_Info_UClass_UGravityObject.OuterSingleton, Z_Construct_UClass_UGravityObject_Statics::ClassParams);
		}
		return Z_Registration_Info_UClass_UGravityObject.OuterSingleton;
	}
	template<> APS_ALPHA_API UClass* StaticClass<UGravityObject>()
	{
		return UGravityObject::StaticClass();
	}
	DEFINE_VTABLE_PTR_HELPER_CTOR(UGravityObject);
	UGravityObject::~UGravityObject() {}
	struct Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_GravityObject_h_Statics
	{
		static const FClassRegisterCompiledInInfo ClassInfo[];
	};
	const FClassRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_GravityObject_h_Statics::ClassInfo[] = {
		{ Z_Construct_UClass_UGravityObject, UGravityObject::StaticClass, TEXT("UGravityObject"), &Z_Registration_Info_UClass_UGravityObject, CONSTRUCT_RELOAD_VERSION_INFO(FClassReloadVersionInfo, sizeof(UGravityObject), 864500402U) },
	};
	static FRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_GravityObject_h_2694714935(TEXT("/Script/APS_ALPHA"),
		Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_GravityObject_h_Statics::ClassInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_GravityObject_h_Statics::ClassInfo),
		nullptr, 0,
		nullptr, 0);
PRAGMA_ENABLE_DEPRECATION_WARNINGS
