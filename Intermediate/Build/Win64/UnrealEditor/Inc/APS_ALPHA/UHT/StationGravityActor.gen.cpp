// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
#include "APS_ALPHA/StationGravityActor.h"
PRAGMA_DISABLE_DEPRECATION_WARNINGS
void EmptyLinkFunctionForGeneratedCodeStationGravityActor() {}
// Cross Module References
	APS_ALPHA_API UClass* Z_Construct_UClass_AGravityActor();
	APS_ALPHA_API UClass* Z_Construct_UClass_AStationGravityActor();
	APS_ALPHA_API UClass* Z_Construct_UClass_AStationGravityActor_NoRegister();
	UPackage* Z_Construct_UPackage__Script_APS_ALPHA();
// End Cross Module References
	void AStationGravityActor::StaticRegisterNativesAStationGravityActor()
	{
	}
	IMPLEMENT_CLASS_NO_AUTO_REGISTRATION(AStationGravityActor);
	UClass* Z_Construct_UClass_AStationGravityActor_NoRegister()
	{
		return AStationGravityActor::StaticClass();
	}
	struct Z_Construct_UClass_AStationGravityActor_Statics
	{
		static UObject* (*const DependentSingletons[])();
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam Class_MetaDataParams[];
#endif
		static const FCppClassTypeInfoStatic StaticCppClassTypeInfo;
		static const UECodeGen_Private::FClassParams ClassParams;
	};
	UObject* (*const Z_Construct_UClass_AStationGravityActor_Statics::DependentSingletons[])() = {
		(UObject* (*)())Z_Construct_UClass_AGravityActor,
		(UObject* (*)())Z_Construct_UPackage__Script_APS_ALPHA,
	};
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_AStationGravityActor_Statics::Class_MetaDataParams[] = {
		{ "Comment", "/**\n * \n */" },
		{ "IncludePath", "StationGravityActor.h" },
		{ "ModuleRelativePath", "StationGravityActor.h" },
	};
#endif
	const FCppClassTypeInfoStatic Z_Construct_UClass_AStationGravityActor_Statics::StaticCppClassTypeInfo = {
		TCppClassTypeTraits<AStationGravityActor>::IsAbstract,
	};
	const UECodeGen_Private::FClassParams Z_Construct_UClass_AStationGravityActor_Statics::ClassParams = {
		&AStationGravityActor::StaticClass,
		"Engine",
		&StaticCppClassTypeInfo,
		DependentSingletons,
		nullptr,
		nullptr,
		nullptr,
		UE_ARRAY_COUNT(DependentSingletons),
		0,
		0,
		0,
		0x009000A4u,
		METADATA_PARAMS(Z_Construct_UClass_AStationGravityActor_Statics::Class_MetaDataParams, UE_ARRAY_COUNT(Z_Construct_UClass_AStationGravityActor_Statics::Class_MetaDataParams))
	};
	UClass* Z_Construct_UClass_AStationGravityActor()
	{
		if (!Z_Registration_Info_UClass_AStationGravityActor.OuterSingleton)
		{
			UECodeGen_Private::ConstructUClass(Z_Registration_Info_UClass_AStationGravityActor.OuterSingleton, Z_Construct_UClass_AStationGravityActor_Statics::ClassParams);
		}
		return Z_Registration_Info_UClass_AStationGravityActor.OuterSingleton;
	}
	template<> APS_ALPHA_API UClass* StaticClass<AStationGravityActor>()
	{
		return AStationGravityActor::StaticClass();
	}
	DEFINE_VTABLE_PTR_HELPER_CTOR(AStationGravityActor);
	AStationGravityActor::~AStationGravityActor() {}
	struct Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_StationGravityActor_h_Statics
	{
		static const FClassRegisterCompiledInInfo ClassInfo[];
	};
	const FClassRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_StationGravityActor_h_Statics::ClassInfo[] = {
		{ Z_Construct_UClass_AStationGravityActor, AStationGravityActor::StaticClass, TEXT("AStationGravityActor"), &Z_Registration_Info_UClass_AStationGravityActor, CONSTRUCT_RELOAD_VERSION_INFO(FClassReloadVersionInfo, sizeof(AStationGravityActor), 1690700164U) },
	};
	static FRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_StationGravityActor_h_3390896477(TEXT("/Script/APS_ALPHA"),
		Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_StationGravityActor_h_Statics::ClassInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_StationGravityActor_h_Statics::ClassInfo),
		nullptr, 0,
		nullptr, 0);
PRAGMA_ENABLE_DEPRECATION_WARNINGS
