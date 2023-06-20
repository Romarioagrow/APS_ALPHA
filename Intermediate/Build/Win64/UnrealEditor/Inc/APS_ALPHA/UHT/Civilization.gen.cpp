// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
#include "APS_ALPHA/Civilization.h"
PRAGMA_DISABLE_DEPRECATION_WARNINGS
void EmptyLinkFunctionForGeneratedCodeCivilization() {}
// Cross Module References
	APS_ALPHA_API UClass* Z_Construct_UClass_UCivilization();
	APS_ALPHA_API UClass* Z_Construct_UClass_UCivilization_NoRegister();
	COREUOBJECT_API UClass* Z_Construct_UClass_UObject();
	UPackage* Z_Construct_UPackage__Script_APS_ALPHA();
// End Cross Module References
	void UCivilization::StaticRegisterNativesUCivilization()
	{
	}
	IMPLEMENT_CLASS_NO_AUTO_REGISTRATION(UCivilization);
	UClass* Z_Construct_UClass_UCivilization_NoRegister()
	{
		return UCivilization::StaticClass();
	}
	struct Z_Construct_UClass_UCivilization_Statics
	{
		static UObject* (*const DependentSingletons[])();
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam Class_MetaDataParams[];
#endif
		static const FCppClassTypeInfoStatic StaticCppClassTypeInfo;
		static const UECodeGen_Private::FClassParams ClassParams;
	};
	UObject* (*const Z_Construct_UClass_UCivilization_Statics::DependentSingletons[])() = {
		(UObject* (*)())Z_Construct_UClass_UObject,
		(UObject* (*)())Z_Construct_UPackage__Script_APS_ALPHA,
	};
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_UCivilization_Statics::Class_MetaDataParams[] = {
		{ "Comment", "/**\n * \n */" },
		{ "IncludePath", "Civilization.h" },
		{ "ModuleRelativePath", "Civilization.h" },
	};
#endif
	const FCppClassTypeInfoStatic Z_Construct_UClass_UCivilization_Statics::StaticCppClassTypeInfo = {
		TCppClassTypeTraits<UCivilization>::IsAbstract,
	};
	const UECodeGen_Private::FClassParams Z_Construct_UClass_UCivilization_Statics::ClassParams = {
		&UCivilization::StaticClass,
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
		0x001000A0u,
		METADATA_PARAMS(Z_Construct_UClass_UCivilization_Statics::Class_MetaDataParams, UE_ARRAY_COUNT(Z_Construct_UClass_UCivilization_Statics::Class_MetaDataParams))
	};
	UClass* Z_Construct_UClass_UCivilization()
	{
		if (!Z_Registration_Info_UClass_UCivilization.OuterSingleton)
		{
			UECodeGen_Private::ConstructUClass(Z_Registration_Info_UClass_UCivilization.OuterSingleton, Z_Construct_UClass_UCivilization_Statics::ClassParams);
		}
		return Z_Registration_Info_UClass_UCivilization.OuterSingleton;
	}
	template<> APS_ALPHA_API UClass* StaticClass<UCivilization>()
	{
		return UCivilization::StaticClass();
	}
	DEFINE_VTABLE_PTR_HELPER_CTOR(UCivilization);
	UCivilization::~UCivilization() {}
	struct Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_Civilization_h_Statics
	{
		static const FClassRegisterCompiledInInfo ClassInfo[];
	};
	const FClassRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_Civilization_h_Statics::ClassInfo[] = {
		{ Z_Construct_UClass_UCivilization, UCivilization::StaticClass, TEXT("UCivilization"), &Z_Registration_Info_UClass_UCivilization, CONSTRUCT_RELOAD_VERSION_INFO(FClassReloadVersionInfo, sizeof(UCivilization), 869204972U) },
	};
	static FRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_Civilization_h_1094812032(TEXT("/Script/APS_ALPHA"),
		Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_Civilization_h_Statics::ClassInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_Civilization_h_Statics::ClassInfo),
		nullptr, 0,
		nullptr, 0);
PRAGMA_ENABLE_DEPRECATION_WARNINGS
