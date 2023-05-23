// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
#include "APS_ALPHA/Moon.h"
PRAGMA_DISABLE_DEPRECATION_WARNINGS
void EmptyLinkFunctionForGeneratedCodeMoon() {}
// Cross Module References
	APS_ALPHA_API UClass* Z_Construct_UClass_AMoon();
	APS_ALPHA_API UClass* Z_Construct_UClass_AMoon_NoRegister();
	APS_ALPHA_API UClass* Z_Construct_UClass_AOrbitalBody();
	APS_ALPHA_API UClass* Z_Construct_UClass_APlanet_NoRegister();
	APS_ALPHA_API UClass* Z_Construct_UClass_UPlanetaryEnvironment_NoRegister();
	APS_ALPHA_API UEnum* Z_Construct_UEnum_APS_ALPHA_EMoonType();
	UPackage* Z_Construct_UPackage__Script_APS_ALPHA();
// End Cross Module References
	void AMoon::StaticRegisterNativesAMoon()
	{
	}
	IMPLEMENT_CLASS_NO_AUTO_REGISTRATION(AMoon);
	UClass* Z_Construct_UClass_AMoon_NoRegister()
	{
		return AMoon::StaticClass();
	}
	struct Z_Construct_UClass_AMoon_Statics
	{
		static UObject* (*const DependentSingletons[])();
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam Class_MetaDataParams[];
#endif
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_ParentPlanet_MetaData[];
#endif
		static const UECodeGen_Private::FObjectPropertyParams NewProp_ParentPlanet;
		static const UECodeGen_Private::FBytePropertyParams NewProp_MoonType_Underlying;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_MoonType_MetaData[];
#endif
		static const UECodeGen_Private::FEnumPropertyParams NewProp_MoonType;
		static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
		static const UECodeGen_Private::FImplementedInterfaceParams InterfaceParams[];
		static const FCppClassTypeInfoStatic StaticCppClassTypeInfo;
		static const UECodeGen_Private::FClassParams ClassParams;
	};
	UObject* (*const Z_Construct_UClass_AMoon_Statics::DependentSingletons[])() = {
		(UObject* (*)())Z_Construct_UClass_AOrbitalBody,
		(UObject* (*)())Z_Construct_UPackage__Script_APS_ALPHA,
	};
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_AMoon_Statics::Class_MetaDataParams[] = {
		{ "Comment", "/**\n * \n */" },
		{ "IncludePath", "Moon.h" },
		{ "ModuleRelativePath", "Moon.h" },
	};
#endif
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_AMoon_Statics::NewProp_ParentPlanet_MetaData[] = {
		{ "Category", "Moon" },
		{ "ModuleRelativePath", "Moon.h" },
	};
#endif
	const UECodeGen_Private::FObjectPropertyParams Z_Construct_UClass_AMoon_Statics::NewProp_ParentPlanet = { "ParentPlanet", nullptr, (EPropertyFlags)0x0040000000020001, UECodeGen_Private::EPropertyGenFlags::Object, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(AMoon, ParentPlanet), Z_Construct_UClass_APlanet_NoRegister, METADATA_PARAMS(Z_Construct_UClass_AMoon_Statics::NewProp_ParentPlanet_MetaData, UE_ARRAY_COUNT(Z_Construct_UClass_AMoon_Statics::NewProp_ParentPlanet_MetaData)) };
	const UECodeGen_Private::FBytePropertyParams Z_Construct_UClass_AMoon_Statics::NewProp_MoonType_Underlying = { "UnderlyingType", nullptr, (EPropertyFlags)0x0000000000000000, UECodeGen_Private::EPropertyGenFlags::Byte, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, 0, nullptr, METADATA_PARAMS(nullptr, 0) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_AMoon_Statics::NewProp_MoonType_MetaData[] = {
		{ "Category", "Moon" },
		{ "ModuleRelativePath", "Moon.h" },
	};
#endif
	const UECodeGen_Private::FEnumPropertyParams Z_Construct_UClass_AMoon_Statics::NewProp_MoonType = { "MoonType", nullptr, (EPropertyFlags)0x0040000000020001, UECodeGen_Private::EPropertyGenFlags::Enum, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(AMoon, MoonType), Z_Construct_UEnum_APS_ALPHA_EMoonType, METADATA_PARAMS(Z_Construct_UClass_AMoon_Statics::NewProp_MoonType_MetaData, UE_ARRAY_COUNT(Z_Construct_UClass_AMoon_Statics::NewProp_MoonType_MetaData)) }; // 1724636030
	const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UClass_AMoon_Statics::PropPointers[] = {
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_AMoon_Statics::NewProp_ParentPlanet,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_AMoon_Statics::NewProp_MoonType_Underlying,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_AMoon_Statics::NewProp_MoonType,
	};
		const UECodeGen_Private::FImplementedInterfaceParams Z_Construct_UClass_AMoon_Statics::InterfaceParams[] = {
			{ Z_Construct_UClass_UPlanetaryEnvironment_NoRegister, (int32)VTABLE_OFFSET(AMoon, IPlanetaryEnvironment), false },  // 3707047530
		};
	const FCppClassTypeInfoStatic Z_Construct_UClass_AMoon_Statics::StaticCppClassTypeInfo = {
		TCppClassTypeTraits<AMoon>::IsAbstract,
	};
	const UECodeGen_Private::FClassParams Z_Construct_UClass_AMoon_Statics::ClassParams = {
		&AMoon::StaticClass,
		"Engine",
		&StaticCppClassTypeInfo,
		DependentSingletons,
		nullptr,
		Z_Construct_UClass_AMoon_Statics::PropPointers,
		InterfaceParams,
		UE_ARRAY_COUNT(DependentSingletons),
		0,
		UE_ARRAY_COUNT(Z_Construct_UClass_AMoon_Statics::PropPointers),
		UE_ARRAY_COUNT(InterfaceParams),
		0x009000A4u,
		METADATA_PARAMS(Z_Construct_UClass_AMoon_Statics::Class_MetaDataParams, UE_ARRAY_COUNT(Z_Construct_UClass_AMoon_Statics::Class_MetaDataParams))
	};
	UClass* Z_Construct_UClass_AMoon()
	{
		if (!Z_Registration_Info_UClass_AMoon.OuterSingleton)
		{
			UECodeGen_Private::ConstructUClass(Z_Registration_Info_UClass_AMoon.OuterSingleton, Z_Construct_UClass_AMoon_Statics::ClassParams);
		}
		return Z_Registration_Info_UClass_AMoon.OuterSingleton;
	}
	template<> APS_ALPHA_API UClass* StaticClass<AMoon>()
	{
		return AMoon::StaticClass();
	}
	DEFINE_VTABLE_PTR_HELPER_CTOR(AMoon);
	AMoon::~AMoon() {}
	struct Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_Moon_h_Statics
	{
		static const FClassRegisterCompiledInInfo ClassInfo[];
	};
	const FClassRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_Moon_h_Statics::ClassInfo[] = {
		{ Z_Construct_UClass_AMoon, AMoon::StaticClass, TEXT("AMoon"), &Z_Registration_Info_UClass_AMoon, CONSTRUCT_RELOAD_VERSION_INFO(FClassReloadVersionInfo, sizeof(AMoon), 1926991654U) },
	};
	static FRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_Moon_h_108907983(TEXT("/Script/APS_ALPHA"),
		Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_Moon_h_Statics::ClassInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_Moon_h_Statics::ClassInfo),
		nullptr, 0,
		nullptr, 0);
PRAGMA_ENABLE_DEPRECATION_WARNINGS
