// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
#include "APS_ALPHA/TechActor.h"
PRAGMA_DISABLE_DEPRECATION_WARNINGS
void EmptyLinkFunctionForGeneratedCodeTechActor() {}
// Cross Module References
	APS_ALPHA_API UClass* Z_Construct_UClass_ATechActor();
	APS_ALPHA_API UClass* Z_Construct_UClass_ATechActor_NoRegister();
	APS_ALPHA_API UClass* Z_Construct_UClass_AWorldActor();
	APS_ALPHA_API UClass* Z_Construct_UClass_UTechObject_NoRegister();
	UPackage* Z_Construct_UPackage__Script_APS_ALPHA();
// End Cross Module References
	void ATechActor::StaticRegisterNativesATechActor()
	{
	}
	IMPLEMENT_CLASS_NO_AUTO_REGISTRATION(ATechActor);
	UClass* Z_Construct_UClass_ATechActor_NoRegister()
	{
		return ATechActor::StaticClass();
	}
	struct Z_Construct_UClass_ATechActor_Statics
	{
		static UObject* (*const DependentSingletons[])();
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam Class_MetaDataParams[];
#endif
		static const UECodeGen_Private::FImplementedInterfaceParams InterfaceParams[];
		static const FCppClassTypeInfoStatic StaticCppClassTypeInfo;
		static const UECodeGen_Private::FClassParams ClassParams;
	};
	UObject* (*const Z_Construct_UClass_ATechActor_Statics::DependentSingletons[])() = {
		(UObject* (*)())Z_Construct_UClass_AWorldActor,
		(UObject* (*)())Z_Construct_UPackage__Script_APS_ALPHA,
	};
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_ATechActor_Statics::Class_MetaDataParams[] = {
		{ "Comment", "/**\n * \n */" },
		{ "IncludePath", "TechActor.h" },
		{ "ModuleRelativePath", "TechActor.h" },
	};
#endif
		const UECodeGen_Private::FImplementedInterfaceParams Z_Construct_UClass_ATechActor_Statics::InterfaceParams[] = {
			{ Z_Construct_UClass_UTechObject_NoRegister, (int32)VTABLE_OFFSET(ATechActor, ITechObject), false },  // 2508275508
		};
	const FCppClassTypeInfoStatic Z_Construct_UClass_ATechActor_Statics::StaticCppClassTypeInfo = {
		TCppClassTypeTraits<ATechActor>::IsAbstract,
	};
	const UECodeGen_Private::FClassParams Z_Construct_UClass_ATechActor_Statics::ClassParams = {
		&ATechActor::StaticClass,
		"Engine",
		&StaticCppClassTypeInfo,
		DependentSingletons,
		nullptr,
		nullptr,
		InterfaceParams,
		UE_ARRAY_COUNT(DependentSingletons),
		0,
		0,
		UE_ARRAY_COUNT(InterfaceParams),
		0x009000A4u,
		METADATA_PARAMS(Z_Construct_UClass_ATechActor_Statics::Class_MetaDataParams, UE_ARRAY_COUNT(Z_Construct_UClass_ATechActor_Statics::Class_MetaDataParams))
	};
	UClass* Z_Construct_UClass_ATechActor()
	{
		if (!Z_Registration_Info_UClass_ATechActor.OuterSingleton)
		{
			UECodeGen_Private::ConstructUClass(Z_Registration_Info_UClass_ATechActor.OuterSingleton, Z_Construct_UClass_ATechActor_Statics::ClassParams);
		}
		return Z_Registration_Info_UClass_ATechActor.OuterSingleton;
	}
	template<> APS_ALPHA_API UClass* StaticClass<ATechActor>()
	{
		return ATechActor::StaticClass();
	}
	DEFINE_VTABLE_PTR_HELPER_CTOR(ATechActor);
	ATechActor::~ATechActor() {}
	struct Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_TechActor_h_Statics
	{
		static const FClassRegisterCompiledInInfo ClassInfo[];
	};
	const FClassRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_TechActor_h_Statics::ClassInfo[] = {
		{ Z_Construct_UClass_ATechActor, ATechActor::StaticClass, TEXT("ATechActor"), &Z_Registration_Info_UClass_ATechActor, CONSTRUCT_RELOAD_VERSION_INFO(FClassReloadVersionInfo, sizeof(ATechActor), 3288074868U) },
	};
	static FRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_TechActor_h_1081373808(TEXT("/Script/APS_ALPHA"),
		Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_TechActor_h_Statics::ClassInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_TechActor_h_Statics::ClassInfo),
		nullptr, 0,
		nullptr, 0);
PRAGMA_ENABLE_DEPRECATION_WARNINGS
