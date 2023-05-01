// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
#include "APS_ALPHA/Spaceship.h"
PRAGMA_DISABLE_DEPRECATION_WARNINGS
void EmptyLinkFunctionForGeneratedCodeSpaceship() {}
// Cross Module References
	APS_ALPHA_API UClass* Z_Construct_UClass_ASpacecraft();
	APS_ALPHA_API UClass* Z_Construct_UClass_ASpaceship();
	APS_ALPHA_API UClass* Z_Construct_UClass_ASpaceship_NoRegister();
	APS_ALPHA_API UClass* Z_Construct_UClass_UGravitySource_NoRegister();
	ENGINE_API UClass* Z_Construct_UClass_USphereComponent_NoRegister();
	UPackage* Z_Construct_UPackage__Script_APS_ALPHA();
// End Cross Module References
	void ASpaceship::StaticRegisterNativesASpaceship()
	{
	}
	IMPLEMENT_CLASS_NO_AUTO_REGISTRATION(ASpaceship);
	UClass* Z_Construct_UClass_ASpaceship_NoRegister()
	{
		return ASpaceship::StaticClass();
	}
	struct Z_Construct_UClass_ASpaceship_Statics
	{
		static UObject* (*const DependentSingletons[])();
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam Class_MetaDataParams[];
#endif
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_SphereCollisionComponent_MetaData[];
#endif
		static const UECodeGen_Private::FObjectPropertyParams NewProp_SphereCollisionComponent;
		static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
		static const UECodeGen_Private::FImplementedInterfaceParams InterfaceParams[];
		static const FCppClassTypeInfoStatic StaticCppClassTypeInfo;
		static const UECodeGen_Private::FClassParams ClassParams;
	};
	UObject* (*const Z_Construct_UClass_ASpaceship_Statics::DependentSingletons[])() = {
		(UObject* (*)())Z_Construct_UClass_ASpacecraft,
		(UObject* (*)())Z_Construct_UPackage__Script_APS_ALPHA,
	};
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_ASpaceship_Statics::Class_MetaDataParams[] = {
		{ "Comment", "/**\n * \n */" },
		{ "HideCategories", "Navigation" },
		{ "IncludePath", "Spaceship.h" },
		{ "ModuleRelativePath", "Spaceship.h" },
	};
#endif
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_ASpaceship_Statics::NewProp_SphereCollisionComponent_MetaData[] = {
		{ "Category", "Components" },
		{ "EditInline", "true" },
		{ "ModuleRelativePath", "Spaceship.h" },
	};
#endif
	const UECodeGen_Private::FObjectPropertyParams Z_Construct_UClass_ASpaceship_Statics::NewProp_SphereCollisionComponent = { "SphereCollisionComponent", nullptr, (EPropertyFlags)0x00200800000a001d, UECodeGen_Private::EPropertyGenFlags::Object, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(ASpaceship, SphereCollisionComponent), Z_Construct_UClass_USphereComponent_NoRegister, METADATA_PARAMS(Z_Construct_UClass_ASpaceship_Statics::NewProp_SphereCollisionComponent_MetaData, UE_ARRAY_COUNT(Z_Construct_UClass_ASpaceship_Statics::NewProp_SphereCollisionComponent_MetaData)) };
	const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UClass_ASpaceship_Statics::PropPointers[] = {
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_ASpaceship_Statics::NewProp_SphereCollisionComponent,
	};
		const UECodeGen_Private::FImplementedInterfaceParams Z_Construct_UClass_ASpaceship_Statics::InterfaceParams[] = {
			{ Z_Construct_UClass_UGravitySource_NoRegister, (int32)VTABLE_OFFSET(ASpaceship, IGravitySource), false },  // 1846880873
		};
	const FCppClassTypeInfoStatic Z_Construct_UClass_ASpaceship_Statics::StaticCppClassTypeInfo = {
		TCppClassTypeTraits<ASpaceship>::IsAbstract,
	};
	const UECodeGen_Private::FClassParams Z_Construct_UClass_ASpaceship_Statics::ClassParams = {
		&ASpaceship::StaticClass,
		"Game",
		&StaticCppClassTypeInfo,
		DependentSingletons,
		nullptr,
		Z_Construct_UClass_ASpaceship_Statics::PropPointers,
		InterfaceParams,
		UE_ARRAY_COUNT(DependentSingletons),
		0,
		UE_ARRAY_COUNT(Z_Construct_UClass_ASpaceship_Statics::PropPointers),
		UE_ARRAY_COUNT(InterfaceParams),
		0x009000A4u,
		METADATA_PARAMS(Z_Construct_UClass_ASpaceship_Statics::Class_MetaDataParams, UE_ARRAY_COUNT(Z_Construct_UClass_ASpaceship_Statics::Class_MetaDataParams))
	};
	UClass* Z_Construct_UClass_ASpaceship()
	{
		if (!Z_Registration_Info_UClass_ASpaceship.OuterSingleton)
		{
			UECodeGen_Private::ConstructUClass(Z_Registration_Info_UClass_ASpaceship.OuterSingleton, Z_Construct_UClass_ASpaceship_Statics::ClassParams);
		}
		return Z_Registration_Info_UClass_ASpaceship.OuterSingleton;
	}
	template<> APS_ALPHA_API UClass* StaticClass<ASpaceship>()
	{
		return ASpaceship::StaticClass();
	}
	DEFINE_VTABLE_PTR_HELPER_CTOR(ASpaceship);
	ASpaceship::~ASpaceship() {}
	struct Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_Spaceship_h_Statics
	{
		static const FClassRegisterCompiledInInfo ClassInfo[];
	};
	const FClassRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_Spaceship_h_Statics::ClassInfo[] = {
		{ Z_Construct_UClass_ASpaceship, ASpaceship::StaticClass, TEXT("ASpaceship"), &Z_Registration_Info_UClass_ASpaceship, CONSTRUCT_RELOAD_VERSION_INFO(FClassReloadVersionInfo, sizeof(ASpaceship), 1653448215U) },
	};
	static FRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_Spaceship_h_3133365054(TEXT("/Script/APS_ALPHA"),
		Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_Spaceship_h_Statics::ClassInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_Spaceship_h_Statics::ClassInfo),
		nullptr, 0,
		nullptr, 0);
PRAGMA_ENABLE_DEPRECATION_WARNINGS
