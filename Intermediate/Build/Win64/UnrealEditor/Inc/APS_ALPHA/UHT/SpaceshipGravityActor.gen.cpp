// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
#include "APS_ALPHA/SpaceshipGravityActor.h"
PRAGMA_DISABLE_DEPRECATION_WARNINGS
void EmptyLinkFunctionForGeneratedCodeSpaceshipGravityActor() {}
// Cross Module References
	APS_ALPHA_API UClass* Z_Construct_UClass_AGravityActor();
	APS_ALPHA_API UClass* Z_Construct_UClass_ASpaceshipGravityActor();
	APS_ALPHA_API UClass* Z_Construct_UClass_ASpaceshipGravityActor_NoRegister();
	ENGINE_API UClass* Z_Construct_UClass_UStaticMeshComponent_NoRegister();
	UPackage* Z_Construct_UPackage__Script_APS_ALPHA();
// End Cross Module References
	void ASpaceshipGravityActor::StaticRegisterNativesASpaceshipGravityActor()
	{
	}
	IMPLEMENT_CLASS_NO_AUTO_REGISTRATION(ASpaceshipGravityActor);
	UClass* Z_Construct_UClass_ASpaceshipGravityActor_NoRegister()
	{
		return ASpaceshipGravityActor::StaticClass();
	}
	struct Z_Construct_UClass_ASpaceshipGravityActor_Statics
	{
		static UObject* (*const DependentSingletons[])();
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam Class_MetaDataParams[];
#endif
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_StaticMeshComponent_MetaData[];
#endif
		static const UECodeGen_Private::FObjectPropertyParams NewProp_StaticMeshComponent;
		static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
		static const FCppClassTypeInfoStatic StaticCppClassTypeInfo;
		static const UECodeGen_Private::FClassParams ClassParams;
	};
	UObject* (*const Z_Construct_UClass_ASpaceshipGravityActor_Statics::DependentSingletons[])() = {
		(UObject* (*)())Z_Construct_UClass_AGravityActor,
		(UObject* (*)())Z_Construct_UPackage__Script_APS_ALPHA,
	};
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_ASpaceshipGravityActor_Statics::Class_MetaDataParams[] = {
		{ "Comment", "/**\n * \n */" },
		{ "IncludePath", "SpaceshipGravityActor.h" },
		{ "ModuleRelativePath", "SpaceshipGravityActor.h" },
	};
#endif
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_ASpaceshipGravityActor_Statics::NewProp_StaticMeshComponent_MetaData[] = {
		{ "Category", "Components" },
		{ "EditInline", "true" },
		{ "ModuleRelativePath", "SpaceshipGravityActor.h" },
	};
#endif
	const UECodeGen_Private::FObjectPropertyParams Z_Construct_UClass_ASpaceshipGravityActor_Statics::NewProp_StaticMeshComponent = { "StaticMeshComponent", nullptr, (EPropertyFlags)0x00100000000a001d, UECodeGen_Private::EPropertyGenFlags::Object, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(ASpaceshipGravityActor, StaticMeshComponent), Z_Construct_UClass_UStaticMeshComponent_NoRegister, METADATA_PARAMS(Z_Construct_UClass_ASpaceshipGravityActor_Statics::NewProp_StaticMeshComponent_MetaData, UE_ARRAY_COUNT(Z_Construct_UClass_ASpaceshipGravityActor_Statics::NewProp_StaticMeshComponent_MetaData)) };
	const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UClass_ASpaceshipGravityActor_Statics::PropPointers[] = {
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_ASpaceshipGravityActor_Statics::NewProp_StaticMeshComponent,
	};
	const FCppClassTypeInfoStatic Z_Construct_UClass_ASpaceshipGravityActor_Statics::StaticCppClassTypeInfo = {
		TCppClassTypeTraits<ASpaceshipGravityActor>::IsAbstract,
	};
	const UECodeGen_Private::FClassParams Z_Construct_UClass_ASpaceshipGravityActor_Statics::ClassParams = {
		&ASpaceshipGravityActor::StaticClass,
		"Engine",
		&StaticCppClassTypeInfo,
		DependentSingletons,
		nullptr,
		Z_Construct_UClass_ASpaceshipGravityActor_Statics::PropPointers,
		nullptr,
		UE_ARRAY_COUNT(DependentSingletons),
		0,
		UE_ARRAY_COUNT(Z_Construct_UClass_ASpaceshipGravityActor_Statics::PropPointers),
		0,
		0x009000A4u,
		METADATA_PARAMS(Z_Construct_UClass_ASpaceshipGravityActor_Statics::Class_MetaDataParams, UE_ARRAY_COUNT(Z_Construct_UClass_ASpaceshipGravityActor_Statics::Class_MetaDataParams))
	};
	UClass* Z_Construct_UClass_ASpaceshipGravityActor()
	{
		if (!Z_Registration_Info_UClass_ASpaceshipGravityActor.OuterSingleton)
		{
			UECodeGen_Private::ConstructUClass(Z_Registration_Info_UClass_ASpaceshipGravityActor.OuterSingleton, Z_Construct_UClass_ASpaceshipGravityActor_Statics::ClassParams);
		}
		return Z_Registration_Info_UClass_ASpaceshipGravityActor.OuterSingleton;
	}
	template<> APS_ALPHA_API UClass* StaticClass<ASpaceshipGravityActor>()
	{
		return ASpaceshipGravityActor::StaticClass();
	}
	DEFINE_VTABLE_PTR_HELPER_CTOR(ASpaceshipGravityActor);
	ASpaceshipGravityActor::~ASpaceshipGravityActor() {}
	struct Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_SpaceshipGravityActor_h_Statics
	{
		static const FClassRegisterCompiledInInfo ClassInfo[];
	};
	const FClassRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_SpaceshipGravityActor_h_Statics::ClassInfo[] = {
		{ Z_Construct_UClass_ASpaceshipGravityActor, ASpaceshipGravityActor::StaticClass, TEXT("ASpaceshipGravityActor"), &Z_Registration_Info_UClass_ASpaceshipGravityActor, CONSTRUCT_RELOAD_VERSION_INFO(FClassReloadVersionInfo, sizeof(ASpaceshipGravityActor), 3169675669U) },
	};
	static FRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_SpaceshipGravityActor_h_2519759739(TEXT("/Script/APS_ALPHA"),
		Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_SpaceshipGravityActor_h_Statics::ClassInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_SpaceshipGravityActor_h_Statics::ClassInfo),
		nullptr, 0,
		nullptr, 0);
PRAGMA_ENABLE_DEPRECATION_WARNINGS
