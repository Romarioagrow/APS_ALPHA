// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
#include "APS_ALPHA/AstroActor.h"
PRAGMA_DISABLE_DEPRECATION_WARNINGS
void EmptyLinkFunctionForGeneratedCodeAstroActor() {}
// Cross Module References
	APS_ALPHA_API UClass* Z_Construct_UClass_AAstroActor();
	APS_ALPHA_API UClass* Z_Construct_UClass_AAstroActor_NoRegister();
	APS_ALPHA_API UClass* Z_Construct_UClass_AWorldActor();
	APS_ALPHA_API UClass* Z_Construct_UClass_UAstroObject_NoRegister();
	UPackage* Z_Construct_UPackage__Script_APS_ALPHA();
// End Cross Module References
	void AAstroActor::StaticRegisterNativesAAstroActor()
	{
	}
	IMPLEMENT_CLASS_NO_AUTO_REGISTRATION(AAstroActor);
	UClass* Z_Construct_UClass_AAstroActor_NoRegister()
	{
		return AAstroActor::StaticClass();
	}
	struct Z_Construct_UClass_AAstroActor_Statics
	{
		static UObject* (*const DependentSingletons[])();
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam Class_MetaDataParams[];
#endif
		static const UECodeGen_Private::FImplementedInterfaceParams InterfaceParams[];
		static const FCppClassTypeInfoStatic StaticCppClassTypeInfo;
		static const UECodeGen_Private::FClassParams ClassParams;
	};
	UObject* (*const Z_Construct_UClass_AAstroActor_Statics::DependentSingletons[])() = {
		(UObject* (*)())Z_Construct_UClass_AWorldActor,
		(UObject* (*)())Z_Construct_UPackage__Script_APS_ALPHA,
	};
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_AAstroActor_Statics::Class_MetaDataParams[] = {
		{ "Comment", "/**\n * \n */" },
		{ "IncludePath", "AstroActor.h" },
		{ "ModuleRelativePath", "AstroActor.h" },
	};
#endif
		const UECodeGen_Private::FImplementedInterfaceParams Z_Construct_UClass_AAstroActor_Statics::InterfaceParams[] = {
			{ Z_Construct_UClass_UAstroObject_NoRegister, (int32)VTABLE_OFFSET(AAstroActor, IAstroObject), false },  // 1979397053
		};
	const FCppClassTypeInfoStatic Z_Construct_UClass_AAstroActor_Statics::StaticCppClassTypeInfo = {
		TCppClassTypeTraits<AAstroActor>::IsAbstract,
	};
	const UECodeGen_Private::FClassParams Z_Construct_UClass_AAstroActor_Statics::ClassParams = {
		&AAstroActor::StaticClass,
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
		METADATA_PARAMS(Z_Construct_UClass_AAstroActor_Statics::Class_MetaDataParams, UE_ARRAY_COUNT(Z_Construct_UClass_AAstroActor_Statics::Class_MetaDataParams))
	};
	UClass* Z_Construct_UClass_AAstroActor()
	{
		if (!Z_Registration_Info_UClass_AAstroActor.OuterSingleton)
		{
			UECodeGen_Private::ConstructUClass(Z_Registration_Info_UClass_AAstroActor.OuterSingleton, Z_Construct_UClass_AAstroActor_Statics::ClassParams);
		}
		return Z_Registration_Info_UClass_AAstroActor.OuterSingleton;
	}
	template<> APS_ALPHA_API UClass* StaticClass<AAstroActor>()
	{
		return AAstroActor::StaticClass();
	}
	DEFINE_VTABLE_PTR_HELPER_CTOR(AAstroActor);
	AAstroActor::~AAstroActor() {}
	struct Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_AstroActor_h_Statics
	{
		static const FClassRegisterCompiledInInfo ClassInfo[];
	};
	const FClassRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_AstroActor_h_Statics::ClassInfo[] = {
		{ Z_Construct_UClass_AAstroActor, AAstroActor::StaticClass, TEXT("AAstroActor"), &Z_Registration_Info_UClass_AAstroActor, CONSTRUCT_RELOAD_VERSION_INFO(FClassReloadVersionInfo, sizeof(AAstroActor), 630707387U) },
	};
	static FRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_AstroActor_h_2417221832(TEXT("/Script/APS_ALPHA"),
		Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_AstroActor_h_Statics::ClassInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_AstroActor_h_Statics::ClassInfo),
		nullptr, 0,
		nullptr, 0);
PRAGMA_ENABLE_DEPRECATION_WARNINGS
