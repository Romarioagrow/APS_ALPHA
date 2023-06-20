// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
#include "APS_ALPHA/Star.h"
PRAGMA_DISABLE_DEPRECATION_WARNINGS
void EmptyLinkFunctionForGeneratedCodeStar() {}
// Cross Module References
	APS_ALPHA_API UClass* Z_Construct_UClass_ACelestialBody();
	APS_ALPHA_API UClass* Z_Construct_UClass_APlanet_NoRegister();
	APS_ALPHA_API UClass* Z_Construct_UClass_APlanetarySystem_NoRegister();
	APS_ALPHA_API UClass* Z_Construct_UClass_AStar();
	APS_ALPHA_API UClass* Z_Construct_UClass_AStar_NoRegister();
	APS_ALPHA_API UEnum* Z_Construct_UEnum_APS_ALPHA_ESpectralClass();
	APS_ALPHA_API UEnum* Z_Construct_UEnum_APS_ALPHA_ESpectralType();
	APS_ALPHA_API UEnum* Z_Construct_UEnum_APS_ALPHA_EStellarType();
	ENGINE_API UClass* Z_Construct_UClass_UMaterialInstanceDynamic_NoRegister();
	ENGINE_API UClass* Z_Construct_UClass_USphereComponent_NoRegister();
	ENGINE_API UClass* Z_Construct_UClass_UStaticMeshComponent_NoRegister();
	UPackage* Z_Construct_UPackage__Script_APS_ALPHA();
// End Cross Module References
	void AStar::StaticRegisterNativesAStar()
	{
	}
	IMPLEMENT_CLASS_NO_AUTO_REGISTRATION(AStar);
	UClass* Z_Construct_UClass_AStar_NoRegister()
	{
		return AStar::StaticClass();
	}
	struct Z_Construct_UClass_AStar_Statics
	{
		static UObject* (*const DependentSingletons[])();
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam Class_MetaDataParams[];
#endif
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_StarMesh_MetaData[];
#endif
		static const UECodeGen_Private::FObjectPropertyParams NewProp_StarMesh;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_StarDynamicMaterial_MetaData[];
#endif
		static const UECodeGen_Private::FObjectPropertyParams NewProp_StarDynamicMaterial;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_PlanetarySystem_MetaData[];
#endif
		static const UECodeGen_Private::FObjectPropertyParams NewProp_PlanetarySystem;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_PlanetarySystemZone_MetaData[];
#endif
		static const UECodeGen_Private::FObjectPropertyParams NewProp_PlanetarySystemZone;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_StarRadiusKM_MetaData[];
#endif
		static const UECodeGen_Private::FUnsizedIntPropertyParams NewProp_StarRadiusKM;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_StarAffectionZoneRadius_MetaData[];
#endif
		static const UECodeGen_Private::FDoublePropertyParams NewProp_StarAffectionZoneRadius;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_Luminosity_MetaData[];
#endif
		static const UECodeGen_Private::FFloatPropertyParams NewProp_Luminosity;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_SurfaceTemperature_MetaData[];
#endif
		static const UECodeGen_Private::FUnsizedIntPropertyParams NewProp_SurfaceTemperature;
		static const UECodeGen_Private::FBytePropertyParams NewProp_StellarClass_Underlying;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_StellarClass_MetaData[];
#endif
		static const UECodeGen_Private::FEnumPropertyParams NewProp_StellarClass;
		static const UECodeGen_Private::FBytePropertyParams NewProp_SpectralClass_Underlying;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_SpectralClass_MetaData[];
#endif
		static const UECodeGen_Private::FEnumPropertyParams NewProp_SpectralClass;
		static const UECodeGen_Private::FBytePropertyParams NewProp_SpectralType_Underlying;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_SpectralType_MetaData[];
#endif
		static const UECodeGen_Private::FEnumPropertyParams NewProp_SpectralType;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_FullSpectralClass_MetaData[];
#endif
		static const UECodeGen_Private::FNamePropertyParams NewProp_FullSpectralClass;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_FullSpectralName_MetaData[];
#endif
		static const UECodeGen_Private::FNamePropertyParams NewProp_FullSpectralName;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_SpectralSubclass_MetaData[];
#endif
		static const UECodeGen_Private::FUnsizedIntPropertyParams NewProp_SpectralSubclass;
		static const UECodeGen_Private::FObjectPropertyParams NewProp_Planets_Inner;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_Planets_MetaData[];
#endif
		static const UECodeGen_Private::FArrayPropertyParams NewProp_Planets;
		static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
		static const FCppClassTypeInfoStatic StaticCppClassTypeInfo;
		static const UECodeGen_Private::FClassParams ClassParams;
	};
	UObject* (*const Z_Construct_UClass_AStar_Statics::DependentSingletons[])() = {
		(UObject* (*)())Z_Construct_UClass_ACelestialBody,
		(UObject* (*)())Z_Construct_UPackage__Script_APS_ALPHA,
	};
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_AStar_Statics::Class_MetaDataParams[] = {
		{ "Comment", "/**\n * \n */" },
		{ "IncludePath", "Star.h" },
		{ "ModuleRelativePath", "Star.h" },
	};
#endif
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_AStar_Statics::NewProp_StarMesh_MetaData[] = {
		{ "Category", "Star" },
		{ "EditInline", "true" },
		{ "ModuleRelativePath", "Star.h" },
	};
#endif
	const UECodeGen_Private::FObjectPropertyParams Z_Construct_UClass_AStar_Statics::NewProp_StarMesh = { "StarMesh", nullptr, (EPropertyFlags)0x00100000000a0009, UECodeGen_Private::EPropertyGenFlags::Object, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(AStar, StarMesh), Z_Construct_UClass_UStaticMeshComponent_NoRegister, METADATA_PARAMS(Z_Construct_UClass_AStar_Statics::NewProp_StarMesh_MetaData, UE_ARRAY_COUNT(Z_Construct_UClass_AStar_Statics::NewProp_StarMesh_MetaData)) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_AStar_Statics::NewProp_StarDynamicMaterial_MetaData[] = {
		{ "Category", "Star" },
		{ "ModuleRelativePath", "Star.h" },
	};
#endif
	const UECodeGen_Private::FObjectPropertyParams Z_Construct_UClass_AStar_Statics::NewProp_StarDynamicMaterial = { "StarDynamicMaterial", nullptr, (EPropertyFlags)0x0010000000020001, UECodeGen_Private::EPropertyGenFlags::Object, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(AStar, StarDynamicMaterial), Z_Construct_UClass_UMaterialInstanceDynamic_NoRegister, METADATA_PARAMS(Z_Construct_UClass_AStar_Statics::NewProp_StarDynamicMaterial_MetaData, UE_ARRAY_COUNT(Z_Construct_UClass_AStar_Statics::NewProp_StarDynamicMaterial_MetaData)) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_AStar_Statics::NewProp_PlanetarySystem_MetaData[] = {
		{ "Category", "Star" },
		{ "ModuleRelativePath", "Star.h" },
	};
#endif
	const UECodeGen_Private::FObjectPropertyParams Z_Construct_UClass_AStar_Statics::NewProp_PlanetarySystem = { "PlanetarySystem", nullptr, (EPropertyFlags)0x0010000000020001, UECodeGen_Private::EPropertyGenFlags::Object, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(AStar, PlanetarySystem), Z_Construct_UClass_APlanetarySystem_NoRegister, METADATA_PARAMS(Z_Construct_UClass_AStar_Statics::NewProp_PlanetarySystem_MetaData, UE_ARRAY_COUNT(Z_Construct_UClass_AStar_Statics::NewProp_PlanetarySystem_MetaData)) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_AStar_Statics::NewProp_PlanetarySystemZone_MetaData[] = {
		{ "Category", "Star" },
		{ "EditInline", "true" },
		{ "ModuleRelativePath", "Star.h" },
	};
#endif
	const UECodeGen_Private::FObjectPropertyParams Z_Construct_UClass_AStar_Statics::NewProp_PlanetarySystemZone = { "PlanetarySystemZone", nullptr, (EPropertyFlags)0x00100000000a0009, UECodeGen_Private::EPropertyGenFlags::Object, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(AStar, PlanetarySystemZone), Z_Construct_UClass_USphereComponent_NoRegister, METADATA_PARAMS(Z_Construct_UClass_AStar_Statics::NewProp_PlanetarySystemZone_MetaData, UE_ARRAY_COUNT(Z_Construct_UClass_AStar_Statics::NewProp_PlanetarySystemZone_MetaData)) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_AStar_Statics::NewProp_StarRadiusKM_MetaData[] = {
		{ "Category", "Star" },
		{ "ModuleRelativePath", "Star.h" },
	};
#endif
	const UECodeGen_Private::FUnsizedIntPropertyParams Z_Construct_UClass_AStar_Statics::NewProp_StarRadiusKM = { "StarRadiusKM", nullptr, (EPropertyFlags)0x0010000000020001, UECodeGen_Private::EPropertyGenFlags::Int, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(AStar, StarRadiusKM), METADATA_PARAMS(Z_Construct_UClass_AStar_Statics::NewProp_StarRadiusKM_MetaData, UE_ARRAY_COUNT(Z_Construct_UClass_AStar_Statics::NewProp_StarRadiusKM_MetaData)) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_AStar_Statics::NewProp_StarAffectionZoneRadius_MetaData[] = {
		{ "Category", "Star" },
		{ "ModuleRelativePath", "Star.h" },
	};
#endif
	const UECodeGen_Private::FDoublePropertyParams Z_Construct_UClass_AStar_Statics::NewProp_StarAffectionZoneRadius = { "StarAffectionZoneRadius", nullptr, (EPropertyFlags)0x0010000000020001, UECodeGen_Private::EPropertyGenFlags::Double, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(AStar, StarAffectionZoneRadius), METADATA_PARAMS(Z_Construct_UClass_AStar_Statics::NewProp_StarAffectionZoneRadius_MetaData, UE_ARRAY_COUNT(Z_Construct_UClass_AStar_Statics::NewProp_StarAffectionZoneRadius_MetaData)) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_AStar_Statics::NewProp_Luminosity_MetaData[] = {
		{ "Category", "Star" },
		{ "ModuleRelativePath", "Star.h" },
	};
#endif
	const UECodeGen_Private::FFloatPropertyParams Z_Construct_UClass_AStar_Statics::NewProp_Luminosity = { "Luminosity", nullptr, (EPropertyFlags)0x0040000000020001, UECodeGen_Private::EPropertyGenFlags::Float, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(AStar, Luminosity), METADATA_PARAMS(Z_Construct_UClass_AStar_Statics::NewProp_Luminosity_MetaData, UE_ARRAY_COUNT(Z_Construct_UClass_AStar_Statics::NewProp_Luminosity_MetaData)) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_AStar_Statics::NewProp_SurfaceTemperature_MetaData[] = {
		{ "Category", "Star" },
		{ "ModuleRelativePath", "Star.h" },
	};
#endif
	const UECodeGen_Private::FUnsizedIntPropertyParams Z_Construct_UClass_AStar_Statics::NewProp_SurfaceTemperature = { "SurfaceTemperature", nullptr, (EPropertyFlags)0x0040000000020001, UECodeGen_Private::EPropertyGenFlags::Int, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(AStar, SurfaceTemperature), METADATA_PARAMS(Z_Construct_UClass_AStar_Statics::NewProp_SurfaceTemperature_MetaData, UE_ARRAY_COUNT(Z_Construct_UClass_AStar_Statics::NewProp_SurfaceTemperature_MetaData)) };
	const UECodeGen_Private::FBytePropertyParams Z_Construct_UClass_AStar_Statics::NewProp_StellarClass_Underlying = { "UnderlyingType", nullptr, (EPropertyFlags)0x0000000000000000, UECodeGen_Private::EPropertyGenFlags::Byte, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, 0, nullptr, METADATA_PARAMS(nullptr, 0) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_AStar_Statics::NewProp_StellarClass_MetaData[] = {
		{ "Category", "Star" },
		{ "ModuleRelativePath", "Star.h" },
	};
#endif
	const UECodeGen_Private::FEnumPropertyParams Z_Construct_UClass_AStar_Statics::NewProp_StellarClass = { "StellarClass", nullptr, (EPropertyFlags)0x0040000000020001, UECodeGen_Private::EPropertyGenFlags::Enum, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(AStar, StellarClass), Z_Construct_UEnum_APS_ALPHA_EStellarType, METADATA_PARAMS(Z_Construct_UClass_AStar_Statics::NewProp_StellarClass_MetaData, UE_ARRAY_COUNT(Z_Construct_UClass_AStar_Statics::NewProp_StellarClass_MetaData)) }; // 3048584829
	const UECodeGen_Private::FBytePropertyParams Z_Construct_UClass_AStar_Statics::NewProp_SpectralClass_Underlying = { "UnderlyingType", nullptr, (EPropertyFlags)0x0000000000000000, UECodeGen_Private::EPropertyGenFlags::Byte, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, 0, nullptr, METADATA_PARAMS(nullptr, 0) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_AStar_Statics::NewProp_SpectralClass_MetaData[] = {
		{ "Category", "Star" },
		{ "ModuleRelativePath", "Star.h" },
	};
#endif
	const UECodeGen_Private::FEnumPropertyParams Z_Construct_UClass_AStar_Statics::NewProp_SpectralClass = { "SpectralClass", nullptr, (EPropertyFlags)0x0040000000020001, UECodeGen_Private::EPropertyGenFlags::Enum, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(AStar, SpectralClass), Z_Construct_UEnum_APS_ALPHA_ESpectralClass, METADATA_PARAMS(Z_Construct_UClass_AStar_Statics::NewProp_SpectralClass_MetaData, UE_ARRAY_COUNT(Z_Construct_UClass_AStar_Statics::NewProp_SpectralClass_MetaData)) }; // 1101356590
	const UECodeGen_Private::FBytePropertyParams Z_Construct_UClass_AStar_Statics::NewProp_SpectralType_Underlying = { "UnderlyingType", nullptr, (EPropertyFlags)0x0000000000000000, UECodeGen_Private::EPropertyGenFlags::Byte, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, 0, nullptr, METADATA_PARAMS(nullptr, 0) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_AStar_Statics::NewProp_SpectralType_MetaData[] = {
		{ "Category", "Star" },
		{ "ModuleRelativePath", "Star.h" },
	};
#endif
	const UECodeGen_Private::FEnumPropertyParams Z_Construct_UClass_AStar_Statics::NewProp_SpectralType = { "SpectralType", nullptr, (EPropertyFlags)0x0040000000020001, UECodeGen_Private::EPropertyGenFlags::Enum, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(AStar, SpectralType), Z_Construct_UEnum_APS_ALPHA_ESpectralType, METADATA_PARAMS(Z_Construct_UClass_AStar_Statics::NewProp_SpectralType_MetaData, UE_ARRAY_COUNT(Z_Construct_UClass_AStar_Statics::NewProp_SpectralType_MetaData)) }; // 3694631182
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_AStar_Statics::NewProp_FullSpectralClass_MetaData[] = {
		{ "Category", "Star" },
		{ "ModuleRelativePath", "Star.h" },
	};
#endif
	const UECodeGen_Private::FNamePropertyParams Z_Construct_UClass_AStar_Statics::NewProp_FullSpectralClass = { "FullSpectralClass", nullptr, (EPropertyFlags)0x0040000000020001, UECodeGen_Private::EPropertyGenFlags::Name, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(AStar, FullSpectralClass), METADATA_PARAMS(Z_Construct_UClass_AStar_Statics::NewProp_FullSpectralClass_MetaData, UE_ARRAY_COUNT(Z_Construct_UClass_AStar_Statics::NewProp_FullSpectralClass_MetaData)) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_AStar_Statics::NewProp_FullSpectralName_MetaData[] = {
		{ "Category", "Star" },
		{ "ModuleRelativePath", "Star.h" },
	};
#endif
	const UECodeGen_Private::FNamePropertyParams Z_Construct_UClass_AStar_Statics::NewProp_FullSpectralName = { "FullSpectralName", nullptr, (EPropertyFlags)0x0040000000020001, UECodeGen_Private::EPropertyGenFlags::Name, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(AStar, FullSpectralName), METADATA_PARAMS(Z_Construct_UClass_AStar_Statics::NewProp_FullSpectralName_MetaData, UE_ARRAY_COUNT(Z_Construct_UClass_AStar_Statics::NewProp_FullSpectralName_MetaData)) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_AStar_Statics::NewProp_SpectralSubclass_MetaData[] = {
		{ "Category", "Star" },
		{ "ModuleRelativePath", "Star.h" },
	};
#endif
	const UECodeGen_Private::FUnsizedIntPropertyParams Z_Construct_UClass_AStar_Statics::NewProp_SpectralSubclass = { "SpectralSubclass", nullptr, (EPropertyFlags)0x0040000000020001, UECodeGen_Private::EPropertyGenFlags::Int, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(AStar, SpectralSubclass), METADATA_PARAMS(Z_Construct_UClass_AStar_Statics::NewProp_SpectralSubclass_MetaData, UE_ARRAY_COUNT(Z_Construct_UClass_AStar_Statics::NewProp_SpectralSubclass_MetaData)) };
	const UECodeGen_Private::FObjectPropertyParams Z_Construct_UClass_AStar_Statics::NewProp_Planets_Inner = { "Planets", nullptr, (EPropertyFlags)0x0000000000000000, UECodeGen_Private::EPropertyGenFlags::Object, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, 0, Z_Construct_UClass_APlanet_NoRegister, METADATA_PARAMS(nullptr, 0) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_AStar_Statics::NewProp_Planets_MetaData[] = {
		{ "ModuleRelativePath", "Star.h" },
	};
#endif
	const UECodeGen_Private::FArrayPropertyParams Z_Construct_UClass_AStar_Statics::NewProp_Planets = { "Planets", nullptr, (EPropertyFlags)0x0040000000000000, UECodeGen_Private::EPropertyGenFlags::Array, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(AStar, Planets), EArrayPropertyFlags::None, METADATA_PARAMS(Z_Construct_UClass_AStar_Statics::NewProp_Planets_MetaData, UE_ARRAY_COUNT(Z_Construct_UClass_AStar_Statics::NewProp_Planets_MetaData)) };
	const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UClass_AStar_Statics::PropPointers[] = {
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_AStar_Statics::NewProp_StarMesh,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_AStar_Statics::NewProp_StarDynamicMaterial,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_AStar_Statics::NewProp_PlanetarySystem,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_AStar_Statics::NewProp_PlanetarySystemZone,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_AStar_Statics::NewProp_StarRadiusKM,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_AStar_Statics::NewProp_StarAffectionZoneRadius,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_AStar_Statics::NewProp_Luminosity,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_AStar_Statics::NewProp_SurfaceTemperature,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_AStar_Statics::NewProp_StellarClass_Underlying,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_AStar_Statics::NewProp_StellarClass,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_AStar_Statics::NewProp_SpectralClass_Underlying,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_AStar_Statics::NewProp_SpectralClass,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_AStar_Statics::NewProp_SpectralType_Underlying,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_AStar_Statics::NewProp_SpectralType,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_AStar_Statics::NewProp_FullSpectralClass,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_AStar_Statics::NewProp_FullSpectralName,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_AStar_Statics::NewProp_SpectralSubclass,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_AStar_Statics::NewProp_Planets_Inner,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_AStar_Statics::NewProp_Planets,
	};
	const FCppClassTypeInfoStatic Z_Construct_UClass_AStar_Statics::StaticCppClassTypeInfo = {
		TCppClassTypeTraits<AStar>::IsAbstract,
	};
	const UECodeGen_Private::FClassParams Z_Construct_UClass_AStar_Statics::ClassParams = {
		&AStar::StaticClass,
		"Engine",
		&StaticCppClassTypeInfo,
		DependentSingletons,
		nullptr,
		Z_Construct_UClass_AStar_Statics::PropPointers,
		nullptr,
		UE_ARRAY_COUNT(DependentSingletons),
		0,
		UE_ARRAY_COUNT(Z_Construct_UClass_AStar_Statics::PropPointers),
		0,
		0x009000A4u,
		METADATA_PARAMS(Z_Construct_UClass_AStar_Statics::Class_MetaDataParams, UE_ARRAY_COUNT(Z_Construct_UClass_AStar_Statics::Class_MetaDataParams))
	};
	UClass* Z_Construct_UClass_AStar()
	{
		if (!Z_Registration_Info_UClass_AStar.OuterSingleton)
		{
			UECodeGen_Private::ConstructUClass(Z_Registration_Info_UClass_AStar.OuterSingleton, Z_Construct_UClass_AStar_Statics::ClassParams);
		}
		return Z_Registration_Info_UClass_AStar.OuterSingleton;
	}
	template<> APS_ALPHA_API UClass* StaticClass<AStar>()
	{
		return AStar::StaticClass();
	}
	DEFINE_VTABLE_PTR_HELPER_CTOR(AStar);
	AStar::~AStar() {}
	struct Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_Star_h_Statics
	{
		static const FClassRegisterCompiledInInfo ClassInfo[];
	};
	const FClassRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_Star_h_Statics::ClassInfo[] = {
		{ Z_Construct_UClass_AStar, AStar::StaticClass, TEXT("AStar"), &Z_Registration_Info_UClass_AStar, CONSTRUCT_RELOAD_VERSION_INFO(FClassReloadVersionInfo, sizeof(AStar), 866838055U) },
	};
	static FRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_Star_h_2742883271(TEXT("/Script/APS_ALPHA"),
		Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_Star_h_Statics::ClassInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_Star_h_Statics::ClassInfo),
		nullptr, 0,
		nullptr, 0);
PRAGMA_ENABLE_DEPRECATION_WARNINGS
