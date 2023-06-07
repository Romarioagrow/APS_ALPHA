// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
#include "APS_ALPHA/StarSystem.h"
PRAGMA_DISABLE_DEPRECATION_WARNINGS
void EmptyLinkFunctionForGeneratedCodeStarSystem() {}
// Cross Module References
	APS_ALPHA_API UClass* Z_Construct_UClass_ACelestialSystem();
	APS_ALPHA_API UClass* Z_Construct_UClass_AStar_NoRegister();
	APS_ALPHA_API UClass* Z_Construct_UClass_AStarSystem();
	APS_ALPHA_API UClass* Z_Construct_UClass_AStarSystem_NoRegister();
	APS_ALPHA_API UEnum* Z_Construct_UEnum_APS_ALPHA_EStarSystemType();
	UPackage* Z_Construct_UPackage__Script_APS_ALPHA();
// End Cross Module References
	void AStarSystem::StaticRegisterNativesAStarSystem()
	{
	}
	IMPLEMENT_CLASS_NO_AUTO_REGISTRATION(AStarSystem);
	UClass* Z_Construct_UClass_AStarSystem_NoRegister()
	{
		return AStarSystem::StaticClass();
	}
	struct Z_Construct_UClass_AStarSystem_Statics
	{
		static UObject* (*const DependentSingletons[])();
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam Class_MetaDataParams[];
#endif
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_StarsAmount_MetaData[];
#endif
		static const UECodeGen_Private::FUnsizedIntPropertyParams NewProp_StarsAmount;
		static const UECodeGen_Private::FBytePropertyParams NewProp_StarSystemType_Underlying;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_StarSystemType_MetaData[];
#endif
		static const UECodeGen_Private::FEnumPropertyParams NewProp_StarSystemType;
		static const UECodeGen_Private::FObjectPropertyParams NewProp_Stars_Inner;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_Stars_MetaData[];
#endif
		static const UECodeGen_Private::FArrayPropertyParams NewProp_Stars;
		static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
		static const FCppClassTypeInfoStatic StaticCppClassTypeInfo;
		static const UECodeGen_Private::FClassParams ClassParams;
	};
	UObject* (*const Z_Construct_UClass_AStarSystem_Statics::DependentSingletons[])() = {
		(UObject* (*)())Z_Construct_UClass_ACelestialSystem,
		(UObject* (*)())Z_Construct_UPackage__Script_APS_ALPHA,
	};
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_AStarSystem_Statics::Class_MetaDataParams[] = {
		{ "Comment", "/**\n * \n */" },
		{ "IncludePath", "StarSystem.h" },
		{ "ModuleRelativePath", "StarSystem.h" },
	};
#endif
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_AStarSystem_Statics::NewProp_StarsAmount_MetaData[] = {
		{ "Category", "Star System" },
		{ "ModuleRelativePath", "StarSystem.h" },
	};
#endif
	const UECodeGen_Private::FUnsizedIntPropertyParams Z_Construct_UClass_AStarSystem_Statics::NewProp_StarsAmount = { "StarsAmount", nullptr, (EPropertyFlags)0x0040000000020001, UECodeGen_Private::EPropertyGenFlags::Int, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(AStarSystem, StarsAmount), METADATA_PARAMS(Z_Construct_UClass_AStarSystem_Statics::NewProp_StarsAmount_MetaData, UE_ARRAY_COUNT(Z_Construct_UClass_AStarSystem_Statics::NewProp_StarsAmount_MetaData)) };
	const UECodeGen_Private::FBytePropertyParams Z_Construct_UClass_AStarSystem_Statics::NewProp_StarSystemType_Underlying = { "UnderlyingType", nullptr, (EPropertyFlags)0x0000000000000000, UECodeGen_Private::EPropertyGenFlags::Byte, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, 0, nullptr, METADATA_PARAMS(nullptr, 0) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_AStarSystem_Statics::NewProp_StarSystemType_MetaData[] = {
		{ "Category", "StarSystem" },
		{ "ModuleRelativePath", "StarSystem.h" },
	};
#endif
	const UECodeGen_Private::FEnumPropertyParams Z_Construct_UClass_AStarSystem_Statics::NewProp_StarSystemType = { "StarSystemType", nullptr, (EPropertyFlags)0x0040000000020001, UECodeGen_Private::EPropertyGenFlags::Enum, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(AStarSystem, StarSystemType), Z_Construct_UEnum_APS_ALPHA_EStarSystemType, METADATA_PARAMS(Z_Construct_UClass_AStarSystem_Statics::NewProp_StarSystemType_MetaData, UE_ARRAY_COUNT(Z_Construct_UClass_AStarSystem_Statics::NewProp_StarSystemType_MetaData)) }; // 1630425490
	const UECodeGen_Private::FObjectPropertyParams Z_Construct_UClass_AStarSystem_Statics::NewProp_Stars_Inner = { "Stars", nullptr, (EPropertyFlags)0x0000000000000000, UECodeGen_Private::EPropertyGenFlags::Object, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, 0, Z_Construct_UClass_AStar_NoRegister, METADATA_PARAMS(nullptr, 0) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_AStarSystem_Statics::NewProp_Stars_MetaData[] = {
		{ "Comment", "/*UPROPERTY()\n\x09\x09TArray<APlanetarySystem*> PlanetarySystems;*/" },
		{ "ModuleRelativePath", "StarSystem.h" },
		{ "ToolTip", "UPROPERTY()\n               TArray<APlanetarySystem*> PlanetarySystems;" },
	};
#endif
	const UECodeGen_Private::FArrayPropertyParams Z_Construct_UClass_AStarSystem_Statics::NewProp_Stars = { "Stars", nullptr, (EPropertyFlags)0x0040000000000000, UECodeGen_Private::EPropertyGenFlags::Array, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(AStarSystem, Stars), EArrayPropertyFlags::None, METADATA_PARAMS(Z_Construct_UClass_AStarSystem_Statics::NewProp_Stars_MetaData, UE_ARRAY_COUNT(Z_Construct_UClass_AStarSystem_Statics::NewProp_Stars_MetaData)) };
	const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UClass_AStarSystem_Statics::PropPointers[] = {
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_AStarSystem_Statics::NewProp_StarsAmount,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_AStarSystem_Statics::NewProp_StarSystemType_Underlying,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_AStarSystem_Statics::NewProp_StarSystemType,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_AStarSystem_Statics::NewProp_Stars_Inner,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_AStarSystem_Statics::NewProp_Stars,
	};
	const FCppClassTypeInfoStatic Z_Construct_UClass_AStarSystem_Statics::StaticCppClassTypeInfo = {
		TCppClassTypeTraits<AStarSystem>::IsAbstract,
	};
	const UECodeGen_Private::FClassParams Z_Construct_UClass_AStarSystem_Statics::ClassParams = {
		&AStarSystem::StaticClass,
		"Engine",
		&StaticCppClassTypeInfo,
		DependentSingletons,
		nullptr,
		Z_Construct_UClass_AStarSystem_Statics::PropPointers,
		nullptr,
		UE_ARRAY_COUNT(DependentSingletons),
		0,
		UE_ARRAY_COUNT(Z_Construct_UClass_AStarSystem_Statics::PropPointers),
		0,
		0x009000A4u,
		METADATA_PARAMS(Z_Construct_UClass_AStarSystem_Statics::Class_MetaDataParams, UE_ARRAY_COUNT(Z_Construct_UClass_AStarSystem_Statics::Class_MetaDataParams))
	};
	UClass* Z_Construct_UClass_AStarSystem()
	{
		if (!Z_Registration_Info_UClass_AStarSystem.OuterSingleton)
		{
			UECodeGen_Private::ConstructUClass(Z_Registration_Info_UClass_AStarSystem.OuterSingleton, Z_Construct_UClass_AStarSystem_Statics::ClassParams);
		}
		return Z_Registration_Info_UClass_AStarSystem.OuterSingleton;
	}
	template<> APS_ALPHA_API UClass* StaticClass<AStarSystem>()
	{
		return AStarSystem::StaticClass();
	}
	DEFINE_VTABLE_PTR_HELPER_CTOR(AStarSystem);
	AStarSystem::~AStarSystem() {}
	struct Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_StarSystem_h_Statics
	{
		static const FClassRegisterCompiledInInfo ClassInfo[];
	};
	const FClassRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_StarSystem_h_Statics::ClassInfo[] = {
		{ Z_Construct_UClass_AStarSystem, AStarSystem::StaticClass, TEXT("AStarSystem"), &Z_Registration_Info_UClass_AStarSystem, CONSTRUCT_RELOAD_VERSION_INFO(FClassReloadVersionInfo, sizeof(AStarSystem), 1221982160U) },
	};
	static FRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_StarSystem_h_1577632824(TEXT("/Script/APS_ALPHA"),
		Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_StarSystem_h_Statics::ClassInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_StarSystem_h_Statics::ClassInfo),
		nullptr, 0,
		nullptr, 0);
PRAGMA_ENABLE_DEPRECATION_WARNINGS
