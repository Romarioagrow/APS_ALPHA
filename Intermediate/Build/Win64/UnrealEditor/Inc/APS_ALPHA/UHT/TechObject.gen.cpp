// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
#include "APS_ALPHA/TechObject.h"
PRAGMA_DISABLE_DEPRECATION_WARNINGS
void EmptyLinkFunctionForGeneratedCodeTechObject() {}
// Cross Module References
	APS_ALPHA_API UClass* Z_Construct_UClass_UAstroLocation();
	APS_ALPHA_API UClass* Z_Construct_UClass_UTechObject();
	APS_ALPHA_API UClass* Z_Construct_UClass_UTechObject_NoRegister();
	UPackage* Z_Construct_UPackage__Script_APS_ALPHA();
// End Cross Module References
	void UTechObject::StaticRegisterNativesUTechObject()
	{
	}
	IMPLEMENT_CLASS_NO_AUTO_REGISTRATION(UTechObject);
	UClass* Z_Construct_UClass_UTechObject_NoRegister()
	{
		return UTechObject::StaticClass();
	}
	struct Z_Construct_UClass_UTechObject_Statics
	{
		static UObject* (*const DependentSingletons[])();
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam Class_MetaDataParams[];
#endif
		static const FCppClassTypeInfoStatic StaticCppClassTypeInfo;
		static const UECodeGen_Private::FClassParams ClassParams;
	};
	UObject* (*const Z_Construct_UClass_UTechObject_Statics::DependentSingletons[])() = {
		(UObject* (*)())Z_Construct_UClass_UAstroLocation,
		(UObject* (*)())Z_Construct_UPackage__Script_APS_ALPHA,
	};
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_UTechObject_Statics::Class_MetaDataParams[] = {
		{ "ModuleRelativePath", "TechObject.h" },
	};
#endif
	const FCppClassTypeInfoStatic Z_Construct_UClass_UTechObject_Statics::StaticCppClassTypeInfo = {
		TCppClassTypeTraits<ITechObject>::IsAbstract,
	};
	const UECodeGen_Private::FClassParams Z_Construct_UClass_UTechObject_Statics::ClassParams = {
		&UTechObject::StaticClass,
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
		METADATA_PARAMS(Z_Construct_UClass_UTechObject_Statics::Class_MetaDataParams, UE_ARRAY_COUNT(Z_Construct_UClass_UTechObject_Statics::Class_MetaDataParams))
	};
	UClass* Z_Construct_UClass_UTechObject()
	{
		if (!Z_Registration_Info_UClass_UTechObject.OuterSingleton)
		{
			UECodeGen_Private::ConstructUClass(Z_Registration_Info_UClass_UTechObject.OuterSingleton, Z_Construct_UClass_UTechObject_Statics::ClassParams);
		}
		return Z_Registration_Info_UClass_UTechObject.OuterSingleton;
	}
	template<> APS_ALPHA_API UClass* StaticClass<UTechObject>()
	{
		return UTechObject::StaticClass();
	}
	DEFINE_VTABLE_PTR_HELPER_CTOR(UTechObject);
	UTechObject::~UTechObject() {}
	struct Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_TechObject_h_Statics
	{
		static const FClassRegisterCompiledInInfo ClassInfo[];
	};
	const FClassRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_TechObject_h_Statics::ClassInfo[] = {
		{ Z_Construct_UClass_UTechObject, UTechObject::StaticClass, TEXT("UTechObject"), &Z_Registration_Info_UClass_UTechObject, CONSTRUCT_RELOAD_VERSION_INFO(FClassReloadVersionInfo, sizeof(UTechObject), 2508275508U) },
	};
	static FRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_TechObject_h_1027995494(TEXT("/Script/APS_ALPHA"),
		Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_TechObject_h_Statics::ClassInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_TechObject_h_Statics::ClassInfo),
		nullptr, 0,
		nullptr, 0);
PRAGMA_ENABLE_DEPRECATION_WARNINGS
