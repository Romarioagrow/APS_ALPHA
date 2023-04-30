// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
#include "APS_ALPHA/AstroObject.h"
PRAGMA_DISABLE_DEPRECATION_WARNINGS
void EmptyLinkFunctionForGeneratedCodeAstroObject() {}
// Cross Module References
	APS_ALPHA_API UClass* Z_Construct_UClass_UAstroLocation();
	APS_ALPHA_API UClass* Z_Construct_UClass_UAstroObject();
	APS_ALPHA_API UClass* Z_Construct_UClass_UAstroObject_NoRegister();
	UPackage* Z_Construct_UPackage__Script_APS_ALPHA();
// End Cross Module References
	void UAstroObject::StaticRegisterNativesUAstroObject()
	{
	}
	IMPLEMENT_CLASS_NO_AUTO_REGISTRATION(UAstroObject);
	UClass* Z_Construct_UClass_UAstroObject_NoRegister()
	{
		return UAstroObject::StaticClass();
	}
	struct Z_Construct_UClass_UAstroObject_Statics
	{
		static UObject* (*const DependentSingletons[])();
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam Class_MetaDataParams[];
#endif
		static const FCppClassTypeInfoStatic StaticCppClassTypeInfo;
		static const UECodeGen_Private::FClassParams ClassParams;
	};
	UObject* (*const Z_Construct_UClass_UAstroObject_Statics::DependentSingletons[])() = {
		(UObject* (*)())Z_Construct_UClass_UAstroLocation,
		(UObject* (*)())Z_Construct_UPackage__Script_APS_ALPHA,
	};
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_UAstroObject_Statics::Class_MetaDataParams[] = {
		{ "ModuleRelativePath", "AstroObject.h" },
	};
#endif
	const FCppClassTypeInfoStatic Z_Construct_UClass_UAstroObject_Statics::StaticCppClassTypeInfo = {
		TCppClassTypeTraits<IAstroObject>::IsAbstract,
	};
	const UECodeGen_Private::FClassParams Z_Construct_UClass_UAstroObject_Statics::ClassParams = {
		&UAstroObject::StaticClass,
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
		METADATA_PARAMS(Z_Construct_UClass_UAstroObject_Statics::Class_MetaDataParams, UE_ARRAY_COUNT(Z_Construct_UClass_UAstroObject_Statics::Class_MetaDataParams))
	};
	UClass* Z_Construct_UClass_UAstroObject()
	{
		if (!Z_Registration_Info_UClass_UAstroObject.OuterSingleton)
		{
			UECodeGen_Private::ConstructUClass(Z_Registration_Info_UClass_UAstroObject.OuterSingleton, Z_Construct_UClass_UAstroObject_Statics::ClassParams);
		}
		return Z_Registration_Info_UClass_UAstroObject.OuterSingleton;
	}
	template<> APS_ALPHA_API UClass* StaticClass<UAstroObject>()
	{
		return UAstroObject::StaticClass();
	}
	DEFINE_VTABLE_PTR_HELPER_CTOR(UAstroObject);
	UAstroObject::~UAstroObject() {}
	struct Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_AstroObject_h_Statics
	{
		static const FClassRegisterCompiledInInfo ClassInfo[];
	};
	const FClassRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_AstroObject_h_Statics::ClassInfo[] = {
		{ Z_Construct_UClass_UAstroObject, UAstroObject::StaticClass, TEXT("UAstroObject"), &Z_Registration_Info_UClass_UAstroObject, CONSTRUCT_RELOAD_VERSION_INFO(FClassReloadVersionInfo, sizeof(UAstroObject), 1979397053U) },
	};
	static FRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_AstroObject_h_1830005438(TEXT("/Script/APS_ALPHA"),
		Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_AstroObject_h_Statics::ClassInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_AstroObject_h_Statics::ClassInfo),
		nullptr, 0,
		nullptr, 0);
PRAGMA_ENABLE_DEPRECATION_WARNINGS
