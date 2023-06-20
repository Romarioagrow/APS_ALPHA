// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
#include "APS_ALPHA/Galaxy.h"
PRAGMA_DISABLE_DEPRECATION_WARNINGS
void EmptyLinkFunctionForGeneratedCodeGalaxy() {}
// Cross Module References
	APS_ALPHA_API UClass* Z_Construct_UClass_ACelestialSystem();
	APS_ALPHA_API UClass* Z_Construct_UClass_AGalaxy();
	APS_ALPHA_API UClass* Z_Construct_UClass_AGalaxy_NoRegister();
	APS_ALPHA_API UEnum* Z_Construct_UEnum_APS_ALPHA_EGalaxyClass();
	APS_ALPHA_API UEnum* Z_Construct_UEnum_APS_ALPHA_EGalaxyType();
	ENGINE_API UClass* Z_Construct_UClass_UHierarchicalInstancedStaticMeshComponent_NoRegister();
	UPackage* Z_Construct_UPackage__Script_APS_ALPHA();
// End Cross Module References
	void AGalaxy::StaticRegisterNativesAGalaxy()
	{
	}
	IMPLEMENT_CLASS_NO_AUTO_REGISTRATION(AGalaxy);
	UClass* Z_Construct_UClass_AGalaxy_NoRegister()
	{
		return AGalaxy::StaticClass();
	}
	struct Z_Construct_UClass_AGalaxy_Statics
	{
		static UObject* (*const DependentSingletons[])();
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam Class_MetaDataParams[];
#endif
		static const UECodeGen_Private::FBytePropertyParams NewProp_GalaxyType_Underlying;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_GalaxyType_MetaData[];
#endif
		static const UECodeGen_Private::FEnumPropertyParams NewProp_GalaxyType;
		static const UECodeGen_Private::FBytePropertyParams NewProp_GalaxyGlass_Underlying;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_GalaxyGlass_MetaData[];
#endif
		static const UECodeGen_Private::FEnumPropertyParams NewProp_GalaxyGlass;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_StarMeshInstances_MetaData[];
#endif
		static const UECodeGen_Private::FObjectPropertyParams NewProp_StarMeshInstances;
		static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
		static const FCppClassTypeInfoStatic StaticCppClassTypeInfo;
		static const UECodeGen_Private::FClassParams ClassParams;
	};
	UObject* (*const Z_Construct_UClass_AGalaxy_Statics::DependentSingletons[])() = {
		(UObject* (*)())Z_Construct_UClass_ACelestialSystem,
		(UObject* (*)())Z_Construct_UPackage__Script_APS_ALPHA,
	};
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_AGalaxy_Statics::Class_MetaDataParams[] = {
		{ "Comment", "/**\n * \n */" },
		{ "IncludePath", "Galaxy.h" },
		{ "ModuleRelativePath", "Galaxy.h" },
	};
#endif
	const UECodeGen_Private::FBytePropertyParams Z_Construct_UClass_AGalaxy_Statics::NewProp_GalaxyType_Underlying = { "UnderlyingType", nullptr, (EPropertyFlags)0x0000000000000000, UECodeGen_Private::EPropertyGenFlags::Byte, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, 0, nullptr, METADATA_PARAMS(nullptr, 0) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_AGalaxy_Statics::NewProp_GalaxyType_MetaData[] = {
		{ "Category", "Galaxy" },
		{ "ModuleRelativePath", "Galaxy.h" },
	};
#endif
	const UECodeGen_Private::FEnumPropertyParams Z_Construct_UClass_AGalaxy_Statics::NewProp_GalaxyType = { "GalaxyType", nullptr, (EPropertyFlags)0x0010000000020001, UECodeGen_Private::EPropertyGenFlags::Enum, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(AGalaxy, GalaxyType), Z_Construct_UEnum_APS_ALPHA_EGalaxyType, METADATA_PARAMS(Z_Construct_UClass_AGalaxy_Statics::NewProp_GalaxyType_MetaData, UE_ARRAY_COUNT(Z_Construct_UClass_AGalaxy_Statics::NewProp_GalaxyType_MetaData)) }; // 2575129112
	const UECodeGen_Private::FBytePropertyParams Z_Construct_UClass_AGalaxy_Statics::NewProp_GalaxyGlass_Underlying = { "UnderlyingType", nullptr, (EPropertyFlags)0x0000000000000000, UECodeGen_Private::EPropertyGenFlags::Byte, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, 0, nullptr, METADATA_PARAMS(nullptr, 0) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_AGalaxy_Statics::NewProp_GalaxyGlass_MetaData[] = {
		{ "Category", "Galaxy" },
		{ "ModuleRelativePath", "Galaxy.h" },
	};
#endif
	const UECodeGen_Private::FEnumPropertyParams Z_Construct_UClass_AGalaxy_Statics::NewProp_GalaxyGlass = { "GalaxyGlass", nullptr, (EPropertyFlags)0x0010000000020001, UECodeGen_Private::EPropertyGenFlags::Enum, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(AGalaxy, GalaxyGlass), Z_Construct_UEnum_APS_ALPHA_EGalaxyClass, METADATA_PARAMS(Z_Construct_UClass_AGalaxy_Statics::NewProp_GalaxyGlass_MetaData, UE_ARRAY_COUNT(Z_Construct_UClass_AGalaxy_Statics::NewProp_GalaxyGlass_MetaData)) }; // 1326469318
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_AGalaxy_Statics::NewProp_StarMeshInstances_MetaData[] = {
		{ "Category", "Galaxy Stars" },
		{ "Comment", "// HISM \xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd \xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd \xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd \xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\n" },
		{ "EditInline", "true" },
		{ "ModuleRelativePath", "Galaxy.h" },
		{ "ToolTip", "HISM \xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd \xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd \xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd \xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd" },
	};
#endif
	const UECodeGen_Private::FObjectPropertyParams Z_Construct_UClass_AGalaxy_Statics::NewProp_StarMeshInstances = { "StarMeshInstances", nullptr, (EPropertyFlags)0x001000000008000d, UECodeGen_Private::EPropertyGenFlags::Object, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(AGalaxy, StarMeshInstances), Z_Construct_UClass_UHierarchicalInstancedStaticMeshComponent_NoRegister, METADATA_PARAMS(Z_Construct_UClass_AGalaxy_Statics::NewProp_StarMeshInstances_MetaData, UE_ARRAY_COUNT(Z_Construct_UClass_AGalaxy_Statics::NewProp_StarMeshInstances_MetaData)) };
	const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UClass_AGalaxy_Statics::PropPointers[] = {
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_AGalaxy_Statics::NewProp_GalaxyType_Underlying,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_AGalaxy_Statics::NewProp_GalaxyType,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_AGalaxy_Statics::NewProp_GalaxyGlass_Underlying,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_AGalaxy_Statics::NewProp_GalaxyGlass,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_AGalaxy_Statics::NewProp_StarMeshInstances,
	};
	const FCppClassTypeInfoStatic Z_Construct_UClass_AGalaxy_Statics::StaticCppClassTypeInfo = {
		TCppClassTypeTraits<AGalaxy>::IsAbstract,
	};
	const UECodeGen_Private::FClassParams Z_Construct_UClass_AGalaxy_Statics::ClassParams = {
		&AGalaxy::StaticClass,
		"Engine",
		&StaticCppClassTypeInfo,
		DependentSingletons,
		nullptr,
		Z_Construct_UClass_AGalaxy_Statics::PropPointers,
		nullptr,
		UE_ARRAY_COUNT(DependentSingletons),
		0,
		UE_ARRAY_COUNT(Z_Construct_UClass_AGalaxy_Statics::PropPointers),
		0,
		0x009000A4u,
		METADATA_PARAMS(Z_Construct_UClass_AGalaxy_Statics::Class_MetaDataParams, UE_ARRAY_COUNT(Z_Construct_UClass_AGalaxy_Statics::Class_MetaDataParams))
	};
	UClass* Z_Construct_UClass_AGalaxy()
	{
		if (!Z_Registration_Info_UClass_AGalaxy.OuterSingleton)
		{
			UECodeGen_Private::ConstructUClass(Z_Registration_Info_UClass_AGalaxy.OuterSingleton, Z_Construct_UClass_AGalaxy_Statics::ClassParams);
		}
		return Z_Registration_Info_UClass_AGalaxy.OuterSingleton;
	}
	template<> APS_ALPHA_API UClass* StaticClass<AGalaxy>()
	{
		return AGalaxy::StaticClass();
	}
	DEFINE_VTABLE_PTR_HELPER_CTOR(AGalaxy);
	AGalaxy::~AGalaxy() {}
	struct Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_Galaxy_h_Statics
	{
		static const FClassRegisterCompiledInInfo ClassInfo[];
	};
	const FClassRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_Galaxy_h_Statics::ClassInfo[] = {
		{ Z_Construct_UClass_AGalaxy, AGalaxy::StaticClass, TEXT("AGalaxy"), &Z_Registration_Info_UClass_AGalaxy, CONSTRUCT_RELOAD_VERSION_INFO(FClassReloadVersionInfo, sizeof(AGalaxy), 1922305871U) },
	};
	static FRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_Galaxy_h_1627124706(TEXT("/Script/APS_ALPHA"),
		Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_Galaxy_h_Statics::ClassInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_Galaxy_h_Statics::ClassInfo),
		nullptr, 0,
		nullptr, 0);
PRAGMA_ENABLE_DEPRECATION_WARNINGS
