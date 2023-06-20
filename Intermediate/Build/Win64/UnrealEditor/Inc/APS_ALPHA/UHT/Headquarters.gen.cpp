// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
#include "APS_ALPHA/Headquarters.h"
PRAGMA_DISABLE_DEPRECATION_WARNINGS
void EmptyLinkFunctionForGeneratedCodeHeadquarters() {}
// Cross Module References
	APS_ALPHA_API UClass* Z_Construct_UClass_AHeadquarters();
	APS_ALPHA_API UClass* Z_Construct_UClass_AHeadquarters_NoRegister();
	APS_ALPHA_API UClass* Z_Construct_UClass_ATechActor();
	APS_ALPHA_API UClass* Z_Construct_UClass_UCivilization_NoRegister();
	UPackage* Z_Construct_UPackage__Script_APS_ALPHA();
// End Cross Module References
	void AHeadquarters::StaticRegisterNativesAHeadquarters()
	{
	}
	IMPLEMENT_CLASS_NO_AUTO_REGISTRATION(AHeadquarters);
	UClass* Z_Construct_UClass_AHeadquarters_NoRegister()
	{
		return AHeadquarters::StaticClass();
	}
	struct Z_Construct_UClass_AHeadquarters_Statics
	{
		static UObject* (*const DependentSingletons[])();
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam Class_MetaDataParams[];
#endif
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_Civilization_MetaData[];
#endif
		static const UECodeGen_Private::FObjectPropertyParams NewProp_Civilization;
		static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
		static const FCppClassTypeInfoStatic StaticCppClassTypeInfo;
		static const UECodeGen_Private::FClassParams ClassParams;
	};
	UObject* (*const Z_Construct_UClass_AHeadquarters_Statics::DependentSingletons[])() = {
		(UObject* (*)())Z_Construct_UClass_ATechActor,
		(UObject* (*)())Z_Construct_UPackage__Script_APS_ALPHA,
	};
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_AHeadquarters_Statics::Class_MetaDataParams[] = {
		{ "Comment", "/**\n * \n */" },
		{ "IncludePath", "Headquarters.h" },
		{ "ModuleRelativePath", "Headquarters.h" },
	};
#endif
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_AHeadquarters_Statics::NewProp_Civilization_MetaData[] = {
		{ "Comment", "/*\n\x09""All Gameplay Logic \n\x09*/" },
		{ "ModuleRelativePath", "Headquarters.h" },
		{ "ToolTip", "All Gameplay Logic" },
	};
#endif
	const UECodeGen_Private::FObjectPropertyParams Z_Construct_UClass_AHeadquarters_Statics::NewProp_Civilization = { "Civilization", nullptr, (EPropertyFlags)0x0010000000000000, UECodeGen_Private::EPropertyGenFlags::Object, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(AHeadquarters, Civilization), Z_Construct_UClass_UCivilization_NoRegister, METADATA_PARAMS(Z_Construct_UClass_AHeadquarters_Statics::NewProp_Civilization_MetaData, UE_ARRAY_COUNT(Z_Construct_UClass_AHeadquarters_Statics::NewProp_Civilization_MetaData)) };
	const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UClass_AHeadquarters_Statics::PropPointers[] = {
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_AHeadquarters_Statics::NewProp_Civilization,
	};
	const FCppClassTypeInfoStatic Z_Construct_UClass_AHeadquarters_Statics::StaticCppClassTypeInfo = {
		TCppClassTypeTraits<AHeadquarters>::IsAbstract,
	};
	const UECodeGen_Private::FClassParams Z_Construct_UClass_AHeadquarters_Statics::ClassParams = {
		&AHeadquarters::StaticClass,
		"Engine",
		&StaticCppClassTypeInfo,
		DependentSingletons,
		nullptr,
		Z_Construct_UClass_AHeadquarters_Statics::PropPointers,
		nullptr,
		UE_ARRAY_COUNT(DependentSingletons),
		0,
		UE_ARRAY_COUNT(Z_Construct_UClass_AHeadquarters_Statics::PropPointers),
		0,
		0x009000A5u,
		METADATA_PARAMS(Z_Construct_UClass_AHeadquarters_Statics::Class_MetaDataParams, UE_ARRAY_COUNT(Z_Construct_UClass_AHeadquarters_Statics::Class_MetaDataParams))
	};
	UClass* Z_Construct_UClass_AHeadquarters()
	{
		if (!Z_Registration_Info_UClass_AHeadquarters.OuterSingleton)
		{
			UECodeGen_Private::ConstructUClass(Z_Registration_Info_UClass_AHeadquarters.OuterSingleton, Z_Construct_UClass_AHeadquarters_Statics::ClassParams);
		}
		return Z_Registration_Info_UClass_AHeadquarters.OuterSingleton;
	}
	template<> APS_ALPHA_API UClass* StaticClass<AHeadquarters>()
	{
		return AHeadquarters::StaticClass();
	}
	DEFINE_VTABLE_PTR_HELPER_CTOR(AHeadquarters);
	AHeadquarters::~AHeadquarters() {}
	struct Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_Headquarters_h_Statics
	{
		static const FClassRegisterCompiledInInfo ClassInfo[];
	};
	const FClassRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_Headquarters_h_Statics::ClassInfo[] = {
		{ Z_Construct_UClass_AHeadquarters, AHeadquarters::StaticClass, TEXT("AHeadquarters"), &Z_Registration_Info_UClass_AHeadquarters, CONSTRUCT_RELOAD_VERSION_INFO(FClassReloadVersionInfo, sizeof(AHeadquarters), 395346757U) },
	};
	static FRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_Headquarters_h_1659770324(TEXT("/Script/APS_ALPHA"),
		Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_Headquarters_h_Statics::ClassInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_Headquarters_h_Statics::ClassInfo),
		nullptr, 0,
		nullptr, 0);
PRAGMA_ENABLE_DEPRECATION_WARNINGS
