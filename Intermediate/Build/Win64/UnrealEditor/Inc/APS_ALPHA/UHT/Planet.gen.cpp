// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
#include "APS_ALPHA/Planet.h"
PRAGMA_DISABLE_DEPRECATION_WARNINGS
void EmptyLinkFunctionForGeneratedCodePlanet() {}
// Cross Module References
	APS_ALPHA_API UClass* Z_Construct_UClass_AOrbitalBody();
	APS_ALPHA_API UClass* Z_Construct_UClass_APlanet();
	APS_ALPHA_API UClass* Z_Construct_UClass_APlanet_NoRegister();
	APS_ALPHA_API UClass* Z_Construct_UClass_UPlanetaryEnvironment_NoRegister();
	APS_ALPHA_API UEnum* Z_Construct_UEnum_APS_ALPHA_EPlanetType();
	ENGINE_API UClass* Z_Construct_UClass_USphereComponent_NoRegister();
	UPackage* Z_Construct_UPackage__Script_APS_ALPHA();
// End Cross Module References
	void APlanet::StaticRegisterNativesAPlanet()
	{
	}
	IMPLEMENT_CLASS_NO_AUTO_REGISTRATION(APlanet);
	UClass* Z_Construct_UClass_APlanet_NoRegister()
	{
		return APlanet::StaticClass();
	}
	struct Z_Construct_UClass_APlanet_Statics
	{
		static UObject* (*const DependentSingletons[])();
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam Class_MetaDataParams[];
#endif
		static const UECodeGen_Private::FBytePropertyParams NewProp_PlanetType_Underlying;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_PlanetType_MetaData[];
#endif
		static const UECodeGen_Private::FEnumPropertyParams NewProp_PlanetType;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_SphereCollisionComponent_MetaData[];
#endif
		static const UECodeGen_Private::FObjectPropertyParams NewProp_SphereCollisionComponent;
		static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
		static const UECodeGen_Private::FImplementedInterfaceParams InterfaceParams[];
		static const FCppClassTypeInfoStatic StaticCppClassTypeInfo;
		static const UECodeGen_Private::FClassParams ClassParams;
	};
	UObject* (*const Z_Construct_UClass_APlanet_Statics::DependentSingletons[])() = {
		(UObject* (*)())Z_Construct_UClass_AOrbitalBody,
		(UObject* (*)())Z_Construct_UPackage__Script_APS_ALPHA,
	};
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_APlanet_Statics::Class_MetaDataParams[] = {
		{ "Comment", "/**\n * \n */" },
		{ "IncludePath", "Planet.h" },
		{ "ModuleRelativePath", "Planet.h" },
	};
#endif
	const UECodeGen_Private::FBytePropertyParams Z_Construct_UClass_APlanet_Statics::NewProp_PlanetType_Underlying = { "UnderlyingType", nullptr, (EPropertyFlags)0x0000000000000000, UECodeGen_Private::EPropertyGenFlags::Byte, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, 0, nullptr, METADATA_PARAMS(nullptr, 0) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_APlanet_Statics::NewProp_PlanetType_MetaData[] = {
		{ "Category", "Planet" },
		{ "ModuleRelativePath", "Planet.h" },
	};
#endif
	const UECodeGen_Private::FEnumPropertyParams Z_Construct_UClass_APlanet_Statics::NewProp_PlanetType = { "PlanetType", nullptr, (EPropertyFlags)0x0040000000020001, UECodeGen_Private::EPropertyGenFlags::Enum, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(APlanet, PlanetType), Z_Construct_UEnum_APS_ALPHA_EPlanetType, METADATA_PARAMS(Z_Construct_UClass_APlanet_Statics::NewProp_PlanetType_MetaData, UE_ARRAY_COUNT(Z_Construct_UClass_APlanet_Statics::NewProp_PlanetType_MetaData)) }; // 3639436251
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_APlanet_Statics::NewProp_SphereCollisionComponent_MetaData[] = {
		{ "Category", "Components" },
		{ "EditInline", "true" },
		{ "ModuleRelativePath", "Planet.h" },
	};
#endif
	const UECodeGen_Private::FObjectPropertyParams Z_Construct_UClass_APlanet_Statics::NewProp_SphereCollisionComponent = { "SphereCollisionComponent", nullptr, (EPropertyFlags)0x00200800000a001d, UECodeGen_Private::EPropertyGenFlags::Object, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(APlanet, SphereCollisionComponent), Z_Construct_UClass_USphereComponent_NoRegister, METADATA_PARAMS(Z_Construct_UClass_APlanet_Statics::NewProp_SphereCollisionComponent_MetaData, UE_ARRAY_COUNT(Z_Construct_UClass_APlanet_Statics::NewProp_SphereCollisionComponent_MetaData)) };
	const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UClass_APlanet_Statics::PropPointers[] = {
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_APlanet_Statics::NewProp_PlanetType_Underlying,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_APlanet_Statics::NewProp_PlanetType,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_APlanet_Statics::NewProp_SphereCollisionComponent,
	};
		const UECodeGen_Private::FImplementedInterfaceParams Z_Construct_UClass_APlanet_Statics::InterfaceParams[] = {
			{ Z_Construct_UClass_UPlanetaryEnvironment_NoRegister, (int32)VTABLE_OFFSET(APlanet, IPlanetaryEnvironment), false },  // 3707047530
		};
	const FCppClassTypeInfoStatic Z_Construct_UClass_APlanet_Statics::StaticCppClassTypeInfo = {
		TCppClassTypeTraits<APlanet>::IsAbstract,
	};
	const UECodeGen_Private::FClassParams Z_Construct_UClass_APlanet_Statics::ClassParams = {
		&APlanet::StaticClass,
		"Engine",
		&StaticCppClassTypeInfo,
		DependentSingletons,
		nullptr,
		Z_Construct_UClass_APlanet_Statics::PropPointers,
		InterfaceParams,
		UE_ARRAY_COUNT(DependentSingletons),
		0,
		UE_ARRAY_COUNT(Z_Construct_UClass_APlanet_Statics::PropPointers),
		UE_ARRAY_COUNT(InterfaceParams),
		0x009000A4u,
		METADATA_PARAMS(Z_Construct_UClass_APlanet_Statics::Class_MetaDataParams, UE_ARRAY_COUNT(Z_Construct_UClass_APlanet_Statics::Class_MetaDataParams))
	};
	UClass* Z_Construct_UClass_APlanet()
	{
		if (!Z_Registration_Info_UClass_APlanet.OuterSingleton)
		{
			UECodeGen_Private::ConstructUClass(Z_Registration_Info_UClass_APlanet.OuterSingleton, Z_Construct_UClass_APlanet_Statics::ClassParams);
		}
		return Z_Registration_Info_UClass_APlanet.OuterSingleton;
	}
	template<> APS_ALPHA_API UClass* StaticClass<APlanet>()
	{
		return APlanet::StaticClass();
	}
	DEFINE_VTABLE_PTR_HELPER_CTOR(APlanet);
	APlanet::~APlanet() {}
	struct Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_Planet_h_Statics
	{
		static const FClassRegisterCompiledInInfo ClassInfo[];
	};
	const FClassRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_Planet_h_Statics::ClassInfo[] = {
		{ Z_Construct_UClass_APlanet, APlanet::StaticClass, TEXT("APlanet"), &Z_Registration_Info_UClass_APlanet, CONSTRUCT_RELOAD_VERSION_INFO(FClassReloadVersionInfo, sizeof(APlanet), 416613581U) },
	};
	static FRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_Planet_h_3451580852(TEXT("/Script/APS_ALPHA"),
		Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_Planet_h_Statics::ClassInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_Planet_h_Statics::ClassInfo),
		nullptr, 0,
		nullptr, 0);
PRAGMA_ENABLE_DEPRECATION_WARNINGS
