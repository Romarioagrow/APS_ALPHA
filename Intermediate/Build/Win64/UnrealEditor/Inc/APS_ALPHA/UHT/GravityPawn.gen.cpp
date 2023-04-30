// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
#include "APS_ALPHA/GravityPawn.h"
PRAGMA_DISABLE_DEPRECATION_WARNINGS
void EmptyLinkFunctionForGeneratedCodeGravityPawn() {}
// Cross Module References
	APS_ALPHA_API UClass* Z_Construct_UClass_UGravityPawn();
	APS_ALPHA_API UClass* Z_Construct_UClass_UGravityPawn_NoRegister();
	COREUOBJECT_API UClass* Z_Construct_UClass_UInterface();
	UPackage* Z_Construct_UPackage__Script_APS_ALPHA();
// End Cross Module References
	void UGravityPawn::StaticRegisterNativesUGravityPawn()
	{
	}
	IMPLEMENT_CLASS_NO_AUTO_REGISTRATION(UGravityPawn);
	UClass* Z_Construct_UClass_UGravityPawn_NoRegister()
	{
		return UGravityPawn::StaticClass();
	}
	struct Z_Construct_UClass_UGravityPawn_Statics
	{
		static UObject* (*const DependentSingletons[])();
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam Class_MetaDataParams[];
#endif
		static const FCppClassTypeInfoStatic StaticCppClassTypeInfo;
		static const UECodeGen_Private::FClassParams ClassParams;
	};
	UObject* (*const Z_Construct_UClass_UGravityPawn_Statics::DependentSingletons[])() = {
		(UObject* (*)())Z_Construct_UClass_UInterface,
		(UObject* (*)())Z_Construct_UPackage__Script_APS_ALPHA,
	};
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_UGravityPawn_Statics::Class_MetaDataParams[] = {
		{ "ModuleRelativePath", "GravityPawn.h" },
	};
#endif
	const FCppClassTypeInfoStatic Z_Construct_UClass_UGravityPawn_Statics::StaticCppClassTypeInfo = {
		TCppClassTypeTraits<IGravityPawn>::IsAbstract,
	};
	const UECodeGen_Private::FClassParams Z_Construct_UClass_UGravityPawn_Statics::ClassParams = {
		&UGravityPawn::StaticClass,
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
		METADATA_PARAMS(Z_Construct_UClass_UGravityPawn_Statics::Class_MetaDataParams, UE_ARRAY_COUNT(Z_Construct_UClass_UGravityPawn_Statics::Class_MetaDataParams))
	};
	UClass* Z_Construct_UClass_UGravityPawn()
	{
		if (!Z_Registration_Info_UClass_UGravityPawn.OuterSingleton)
		{
			UECodeGen_Private::ConstructUClass(Z_Registration_Info_UClass_UGravityPawn.OuterSingleton, Z_Construct_UClass_UGravityPawn_Statics::ClassParams);
		}
		return Z_Registration_Info_UClass_UGravityPawn.OuterSingleton;
	}
	template<> APS_ALPHA_API UClass* StaticClass<UGravityPawn>()
	{
		return UGravityPawn::StaticClass();
	}
	DEFINE_VTABLE_PTR_HELPER_CTOR(UGravityPawn);
	UGravityPawn::~UGravityPawn() {}
	struct Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_GravityPawn_h_Statics
	{
		static const FClassRegisterCompiledInInfo ClassInfo[];
	};
	const FClassRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_GravityPawn_h_Statics::ClassInfo[] = {
		{ Z_Construct_UClass_UGravityPawn, UGravityPawn::StaticClass, TEXT("UGravityPawn"), &Z_Registration_Info_UClass_UGravityPawn, CONSTRUCT_RELOAD_VERSION_INFO(FClassReloadVersionInfo, sizeof(UGravityPawn), 1138939472U) },
	};
	static FRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_GravityPawn_h_1128936097(TEXT("/Script/APS_ALPHA"),
		Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_GravityPawn_h_Statics::ClassInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_GravityPawn_h_Statics::ClassInfo),
		nullptr, 0,
		nullptr, 0);
PRAGMA_ENABLE_DEPRECATION_WARNINGS
