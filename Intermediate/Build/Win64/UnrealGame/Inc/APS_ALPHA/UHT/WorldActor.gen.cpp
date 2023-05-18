// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
#include "APS_ALPHA/WorldActor.h"
PRAGMA_DISABLE_DEPRECATION_WARNINGS
void EmptyLinkFunctionForGeneratedCodeWorldActor() {}
// Cross Module References
	APS_ALPHA_API UClass* Z_Construct_UClass_AWorldActor();
	APS_ALPHA_API UClass* Z_Construct_UClass_AWorldActor_NoRegister();
	APS_ALPHA_API UClass* Z_Construct_UClass_UAstroLocation_NoRegister();
	ENGINE_API UClass* Z_Construct_UClass_AActor();
	UPackage* Z_Construct_UPackage__Script_APS_ALPHA();
// End Cross Module References
	void AWorldActor::StaticRegisterNativesAWorldActor()
	{
	}
	IMPLEMENT_CLASS_NO_AUTO_REGISTRATION(AWorldActor);
	UClass* Z_Construct_UClass_AWorldActor_NoRegister()
	{
		return AWorldActor::StaticClass();
	}
	struct Z_Construct_UClass_AWorldActor_Statics
	{
		static UObject* (*const DependentSingletons[])();
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam Class_MetaDataParams[];
#endif
		static const UECodeGen_Private::FImplementedInterfaceParams InterfaceParams[];
		static const FCppClassTypeInfoStatic StaticCppClassTypeInfo;
		static const UECodeGen_Private::FClassParams ClassParams;
	};
	UObject* (*const Z_Construct_UClass_AWorldActor_Statics::DependentSingletons[])() = {
		(UObject* (*)())Z_Construct_UClass_AActor,
		(UObject* (*)())Z_Construct_UPackage__Script_APS_ALPHA,
	};
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_AWorldActor_Statics::Class_MetaDataParams[] = {
		{ "IncludePath", "WorldActor.h" },
		{ "ModuleRelativePath", "WorldActor.h" },
	};
#endif
		const UECodeGen_Private::FImplementedInterfaceParams Z_Construct_UClass_AWorldActor_Statics::InterfaceParams[] = {
			{ Z_Construct_UClass_UAstroLocation_NoRegister, (int32)VTABLE_OFFSET(AWorldActor, IAstroLocation), false },  // 1807021522
		};
	const FCppClassTypeInfoStatic Z_Construct_UClass_AWorldActor_Statics::StaticCppClassTypeInfo = {
		TCppClassTypeTraits<AWorldActor>::IsAbstract,
	};
	const UECodeGen_Private::FClassParams Z_Construct_UClass_AWorldActor_Statics::ClassParams = {
		&AWorldActor::StaticClass,
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
		METADATA_PARAMS(Z_Construct_UClass_AWorldActor_Statics::Class_MetaDataParams, UE_ARRAY_COUNT(Z_Construct_UClass_AWorldActor_Statics::Class_MetaDataParams))
	};
	UClass* Z_Construct_UClass_AWorldActor()
	{
		if (!Z_Registration_Info_UClass_AWorldActor.OuterSingleton)
		{
			UECodeGen_Private::ConstructUClass(Z_Registration_Info_UClass_AWorldActor.OuterSingleton, Z_Construct_UClass_AWorldActor_Statics::ClassParams);
		}
		return Z_Registration_Info_UClass_AWorldActor.OuterSingleton;
	}
	template<> APS_ALPHA_API UClass* StaticClass<AWorldActor>()
	{
		return AWorldActor::StaticClass();
	}
	DEFINE_VTABLE_PTR_HELPER_CTOR(AWorldActor);
	AWorldActor::~AWorldActor() {}
	struct Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_WorldActor_h_Statics
	{
		static const FClassRegisterCompiledInInfo ClassInfo[];
	};
	const FClassRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_WorldActor_h_Statics::ClassInfo[] = {
		{ Z_Construct_UClass_AWorldActor, AWorldActor::StaticClass, TEXT("AWorldActor"), &Z_Registration_Info_UClass_AWorldActor, CONSTRUCT_RELOAD_VERSION_INFO(FClassReloadVersionInfo, sizeof(AWorldActor), 3679414150U) },
	};
	static FRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_WorldActor_h_2003548073(TEXT("/Script/APS_ALPHA"),
		Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_WorldActor_h_Statics::ClassInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_WorldActor_h_Statics::ClassInfo),
		nullptr, 0,
		nullptr, 0);
PRAGMA_ENABLE_DEPRECATION_WARNINGS
