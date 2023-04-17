// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
#include "APS_ALPHA/GravityCharacter.h"
PRAGMA_DISABLE_DEPRECATION_WARNINGS
void EmptyLinkFunctionForGeneratedCodeGravityCharacter() {}
// Cross Module References
	APS_ALPHA_API UClass* Z_Construct_UClass_AGravityCharacter();
	APS_ALPHA_API UClass* Z_Construct_UClass_AGravityCharacter_NoRegister();
	APS_ALPHA_API UEnum* Z_Construct_UEnum_APS_ALPHA_EGravityType();
	COREUOBJECT_API UScriptStruct* Z_Construct_UScriptStruct_FVector();
	ENGINE_API UClass* Z_Construct_UClass_ACharacter();
	ENGINE_API UClass* Z_Construct_UClass_UCameraComponent_NoRegister();
	ENGINE_API UClass* Z_Construct_UClass_USpringArmComponent_NoRegister();
	UPackage* Z_Construct_UPackage__Script_APS_ALPHA();
// End Cross Module References
	DEFINE_FUNCTION(AGravityCharacter::execUpdateCameraOrientation)
	{
		P_FINISH;
		P_NATIVE_BEGIN;
		P_THIS->UpdateCameraOrientation();
		P_NATIVE_END;
	}
	DEFINE_FUNCTION(AGravityCharacter::execUpdateGravity)
	{
		P_FINISH;
		P_NATIVE_BEGIN;
		P_THIS->UpdateGravity();
		P_NATIVE_END;
	}
	DEFINE_FUNCTION(AGravityCharacter::execSetGravityType)
	{
		P_GET_ENUM(EGravityType,Z_Param_NewGravityType);
		P_FINISH;
		P_NATIVE_BEGIN;
		P_THIS->SetGravityType(EGravityType(Z_Param_NewGravityType));
		P_NATIVE_END;
	}
	void AGravityCharacter::StaticRegisterNativesAGravityCharacter()
	{
		UClass* Class = AGravityCharacter::StaticClass();
		static const FNameNativePtrPair Funcs[] = {
			{ "SetGravityType", &AGravityCharacter::execSetGravityType },
			{ "UpdateCameraOrientation", &AGravityCharacter::execUpdateCameraOrientation },
			{ "UpdateGravity", &AGravityCharacter::execUpdateGravity },
		};
		FNativeFunctionRegistrar::RegisterFunctions(Class, Funcs, UE_ARRAY_COUNT(Funcs));
	}
	struct Z_Construct_UFunction_AGravityCharacter_SetGravityType_Statics
	{
		struct GravityCharacter_eventSetGravityType_Parms
		{
			EGravityType NewGravityType;
		};
		static const UECodeGen_Private::FBytePropertyParams NewProp_NewGravityType_Underlying;
		static const UECodeGen_Private::FEnumPropertyParams NewProp_NewGravityType;
		static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam Function_MetaDataParams[];
#endif
		static const UECodeGen_Private::FFunctionParams FuncParams;
	};
	const UECodeGen_Private::FBytePropertyParams Z_Construct_UFunction_AGravityCharacter_SetGravityType_Statics::NewProp_NewGravityType_Underlying = { "UnderlyingType", nullptr, (EPropertyFlags)0x0000000000000000, UECodeGen_Private::EPropertyGenFlags::Byte, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, 0, nullptr, METADATA_PARAMS(nullptr, 0) };
	const UECodeGen_Private::FEnumPropertyParams Z_Construct_UFunction_AGravityCharacter_SetGravityType_Statics::NewProp_NewGravityType = { "NewGravityType", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Enum, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(GravityCharacter_eventSetGravityType_Parms, NewGravityType), Z_Construct_UEnum_APS_ALPHA_EGravityType, METADATA_PARAMS(nullptr, 0) }; // 459840205
	const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UFunction_AGravityCharacter_SetGravityType_Statics::PropPointers[] = {
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_AGravityCharacter_SetGravityType_Statics::NewProp_NewGravityType_Underlying,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_AGravityCharacter_SetGravityType_Statics::NewProp_NewGravityType,
	};
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UFunction_AGravityCharacter_SetGravityType_Statics::Function_MetaDataParams[] = {
		{ "Category", "Gravity" },
		{ "ModuleRelativePath", "GravityCharacter.h" },
	};
#endif
	const UECodeGen_Private::FFunctionParams Z_Construct_UFunction_AGravityCharacter_SetGravityType_Statics::FuncParams = { (UObject*(*)())Z_Construct_UClass_AGravityCharacter, nullptr, "SetGravityType", nullptr, nullptr, sizeof(Z_Construct_UFunction_AGravityCharacter_SetGravityType_Statics::GravityCharacter_eventSetGravityType_Parms), Z_Construct_UFunction_AGravityCharacter_SetGravityType_Statics::PropPointers, UE_ARRAY_COUNT(Z_Construct_UFunction_AGravityCharacter_SetGravityType_Statics::PropPointers), RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x04020401, 0, 0, METADATA_PARAMS(Z_Construct_UFunction_AGravityCharacter_SetGravityType_Statics::Function_MetaDataParams, UE_ARRAY_COUNT(Z_Construct_UFunction_AGravityCharacter_SetGravityType_Statics::Function_MetaDataParams)) };
	UFunction* Z_Construct_UFunction_AGravityCharacter_SetGravityType()
	{
		static UFunction* ReturnFunction = nullptr;
		if (!ReturnFunction)
		{
			UECodeGen_Private::ConstructUFunction(&ReturnFunction, Z_Construct_UFunction_AGravityCharacter_SetGravityType_Statics::FuncParams);
		}
		return ReturnFunction;
	}
	struct Z_Construct_UFunction_AGravityCharacter_UpdateCameraOrientation_Statics
	{
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam Function_MetaDataParams[];
#endif
		static const UECodeGen_Private::FFunctionParams FuncParams;
	};
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UFunction_AGravityCharacter_UpdateCameraOrientation_Statics::Function_MetaDataParams[] = {
		{ "Category", "GravityCamera" },
		{ "ModuleRelativePath", "GravityCharacter.h" },
	};
#endif
	const UECodeGen_Private::FFunctionParams Z_Construct_UFunction_AGravityCharacter_UpdateCameraOrientation_Statics::FuncParams = { (UObject*(*)())Z_Construct_UClass_AGravityCharacter, nullptr, "UpdateCameraOrientation", nullptr, nullptr, 0, nullptr, 0, RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x04080401, 0, 0, METADATA_PARAMS(Z_Construct_UFunction_AGravityCharacter_UpdateCameraOrientation_Statics::Function_MetaDataParams, UE_ARRAY_COUNT(Z_Construct_UFunction_AGravityCharacter_UpdateCameraOrientation_Statics::Function_MetaDataParams)) };
	UFunction* Z_Construct_UFunction_AGravityCharacter_UpdateCameraOrientation()
	{
		static UFunction* ReturnFunction = nullptr;
		if (!ReturnFunction)
		{
			UECodeGen_Private::ConstructUFunction(&ReturnFunction, Z_Construct_UFunction_AGravityCharacter_UpdateCameraOrientation_Statics::FuncParams);
		}
		return ReturnFunction;
	}
	struct Z_Construct_UFunction_AGravityCharacter_UpdateGravity_Statics
	{
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam Function_MetaDataParams[];
#endif
		static const UECodeGen_Private::FFunctionParams FuncParams;
	};
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UFunction_AGravityCharacter_UpdateGravity_Statics::Function_MetaDataParams[] = {
		{ "Category", "Gravity" },
		{ "ModuleRelativePath", "GravityCharacter.h" },
	};
#endif
	const UECodeGen_Private::FFunctionParams Z_Construct_UFunction_AGravityCharacter_UpdateGravity_Statics::FuncParams = { (UObject*(*)())Z_Construct_UClass_AGravityCharacter, nullptr, "UpdateGravity", nullptr, nullptr, 0, nullptr, 0, RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x04080401, 0, 0, METADATA_PARAMS(Z_Construct_UFunction_AGravityCharacter_UpdateGravity_Statics::Function_MetaDataParams, UE_ARRAY_COUNT(Z_Construct_UFunction_AGravityCharacter_UpdateGravity_Statics::Function_MetaDataParams)) };
	UFunction* Z_Construct_UFunction_AGravityCharacter_UpdateGravity()
	{
		static UFunction* ReturnFunction = nullptr;
		if (!ReturnFunction)
		{
			UECodeGen_Private::ConstructUFunction(&ReturnFunction, Z_Construct_UFunction_AGravityCharacter_UpdateGravity_Statics::FuncParams);
		}
		return ReturnFunction;
	}
	IMPLEMENT_CLASS_NO_AUTO_REGISTRATION(AGravityCharacter);
	UClass* Z_Construct_UClass_AGravityCharacter_NoRegister()
	{
		return AGravityCharacter::StaticClass();
	}
	struct Z_Construct_UClass_AGravityCharacter_Statics
	{
		static UObject* (*const DependentSingletons[])();
		static const FClassFunctionLinkInfo FuncInfo[];
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam Class_MetaDataParams[];
#endif
		static const UECodeGen_Private::FBytePropertyParams NewProp_CurrentGravityType_Underlying;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_CurrentGravityType_MetaData[];
#endif
		static const UECodeGen_Private::FEnumPropertyParams NewProp_CurrentGravityType;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_GravityDirection_MetaData[];
#endif
		static const UECodeGen_Private::FStructPropertyParams NewProp_GravityDirection;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_GravityStrength_MetaData[];
#endif
		static const UECodeGen_Private::FFloatPropertyParams NewProp_GravityStrength;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_CameraSpringArm_MetaData[];
#endif
		static const UECodeGen_Private::FObjectPropertyParams NewProp_CameraSpringArm;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_PlayerCamera_MetaData[];
#endif
		static const UECodeGen_Private::FObjectPropertyParams NewProp_PlayerCamera;
		static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
		static const FCppClassTypeInfoStatic StaticCppClassTypeInfo;
		static const UECodeGen_Private::FClassParams ClassParams;
	};
	UObject* (*const Z_Construct_UClass_AGravityCharacter_Statics::DependentSingletons[])() = {
		(UObject* (*)())Z_Construct_UClass_ACharacter,
		(UObject* (*)())Z_Construct_UPackage__Script_APS_ALPHA,
	};
	const FClassFunctionLinkInfo Z_Construct_UClass_AGravityCharacter_Statics::FuncInfo[] = {
		{ &Z_Construct_UFunction_AGravityCharacter_SetGravityType, "SetGravityType" }, // 1696404953
		{ &Z_Construct_UFunction_AGravityCharacter_UpdateCameraOrientation, "UpdateCameraOrientation" }, // 1841873133
		{ &Z_Construct_UFunction_AGravityCharacter_UpdateGravity, "UpdateGravity" }, // 1565443279
	};
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_AGravityCharacter_Statics::Class_MetaDataParams[] = {
		{ "HideCategories", "Navigation" },
		{ "IncludePath", "GravityCharacter.h" },
		{ "ModuleRelativePath", "GravityCharacter.h" },
	};
#endif
	const UECodeGen_Private::FBytePropertyParams Z_Construct_UClass_AGravityCharacter_Statics::NewProp_CurrentGravityType_Underlying = { "UnderlyingType", nullptr, (EPropertyFlags)0x0000000000000000, UECodeGen_Private::EPropertyGenFlags::Byte, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, 0, nullptr, METADATA_PARAMS(nullptr, 0) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_AGravityCharacter_Statics::NewProp_CurrentGravityType_MetaData[] = {
		{ "Category", "Gravity" },
		{ "ModuleRelativePath", "GravityCharacter.h" },
	};
#endif
	const UECodeGen_Private::FEnumPropertyParams Z_Construct_UClass_AGravityCharacter_Statics::NewProp_CurrentGravityType = { "CurrentGravityType", nullptr, (EPropertyFlags)0x0020080000000005, UECodeGen_Private::EPropertyGenFlags::Enum, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(AGravityCharacter, CurrentGravityType), Z_Construct_UEnum_APS_ALPHA_EGravityType, METADATA_PARAMS(Z_Construct_UClass_AGravityCharacter_Statics::NewProp_CurrentGravityType_MetaData, UE_ARRAY_COUNT(Z_Construct_UClass_AGravityCharacter_Statics::NewProp_CurrentGravityType_MetaData)) }; // 459840205
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_AGravityCharacter_Statics::NewProp_GravityDirection_MetaData[] = {
		{ "Category", "Gravity" },
		{ "ModuleRelativePath", "GravityCharacter.h" },
	};
#endif
	const UECodeGen_Private::FStructPropertyParams Z_Construct_UClass_AGravityCharacter_Statics::NewProp_GravityDirection = { "GravityDirection", nullptr, (EPropertyFlags)0x0020080000000015, UECodeGen_Private::EPropertyGenFlags::Struct, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(AGravityCharacter, GravityDirection), Z_Construct_UScriptStruct_FVector, METADATA_PARAMS(Z_Construct_UClass_AGravityCharacter_Statics::NewProp_GravityDirection_MetaData, UE_ARRAY_COUNT(Z_Construct_UClass_AGravityCharacter_Statics::NewProp_GravityDirection_MetaData)) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_AGravityCharacter_Statics::NewProp_GravityStrength_MetaData[] = {
		{ "Category", "Gravity" },
		{ "ModuleRelativePath", "GravityCharacter.h" },
	};
#endif
	const UECodeGen_Private::FFloatPropertyParams Z_Construct_UClass_AGravityCharacter_Statics::NewProp_GravityStrength = { "GravityStrength", nullptr, (EPropertyFlags)0x0020080000000015, UECodeGen_Private::EPropertyGenFlags::Float, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(AGravityCharacter, GravityStrength), METADATA_PARAMS(Z_Construct_UClass_AGravityCharacter_Statics::NewProp_GravityStrength_MetaData, UE_ARRAY_COUNT(Z_Construct_UClass_AGravityCharacter_Statics::NewProp_GravityStrength_MetaData)) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_AGravityCharacter_Statics::NewProp_CameraSpringArm_MetaData[] = {
		{ "Category", "Camera" },
		{ "EditInline", "true" },
		{ "ModuleRelativePath", "GravityCharacter.h" },
	};
#endif
	const UECodeGen_Private::FObjectPropertyParams Z_Construct_UClass_AGravityCharacter_Statics::NewProp_CameraSpringArm = { "CameraSpringArm", nullptr, (EPropertyFlags)0x001000000008000d, UECodeGen_Private::EPropertyGenFlags::Object, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(AGravityCharacter, CameraSpringArm), Z_Construct_UClass_USpringArmComponent_NoRegister, METADATA_PARAMS(Z_Construct_UClass_AGravityCharacter_Statics::NewProp_CameraSpringArm_MetaData, UE_ARRAY_COUNT(Z_Construct_UClass_AGravityCharacter_Statics::NewProp_CameraSpringArm_MetaData)) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_AGravityCharacter_Statics::NewProp_PlayerCamera_MetaData[] = {
		{ "Category", "Camera" },
		{ "EditInline", "true" },
		{ "ModuleRelativePath", "GravityCharacter.h" },
	};
#endif
	const UECodeGen_Private::FObjectPropertyParams Z_Construct_UClass_AGravityCharacter_Statics::NewProp_PlayerCamera = { "PlayerCamera", nullptr, (EPropertyFlags)0x001000000008000d, UECodeGen_Private::EPropertyGenFlags::Object, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(AGravityCharacter, PlayerCamera), Z_Construct_UClass_UCameraComponent_NoRegister, METADATA_PARAMS(Z_Construct_UClass_AGravityCharacter_Statics::NewProp_PlayerCamera_MetaData, UE_ARRAY_COUNT(Z_Construct_UClass_AGravityCharacter_Statics::NewProp_PlayerCamera_MetaData)) };
	const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UClass_AGravityCharacter_Statics::PropPointers[] = {
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_AGravityCharacter_Statics::NewProp_CurrentGravityType_Underlying,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_AGravityCharacter_Statics::NewProp_CurrentGravityType,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_AGravityCharacter_Statics::NewProp_GravityDirection,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_AGravityCharacter_Statics::NewProp_GravityStrength,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_AGravityCharacter_Statics::NewProp_CameraSpringArm,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_AGravityCharacter_Statics::NewProp_PlayerCamera,
	};
	const FCppClassTypeInfoStatic Z_Construct_UClass_AGravityCharacter_Statics::StaticCppClassTypeInfo = {
		TCppClassTypeTraits<AGravityCharacter>::IsAbstract,
	};
	const UECodeGen_Private::FClassParams Z_Construct_UClass_AGravityCharacter_Statics::ClassParams = {
		&AGravityCharacter::StaticClass,
		"Game",
		&StaticCppClassTypeInfo,
		DependentSingletons,
		FuncInfo,
		Z_Construct_UClass_AGravityCharacter_Statics::PropPointers,
		nullptr,
		UE_ARRAY_COUNT(DependentSingletons),
		UE_ARRAY_COUNT(FuncInfo),
		UE_ARRAY_COUNT(Z_Construct_UClass_AGravityCharacter_Statics::PropPointers),
		0,
		0x009000A4u,
		METADATA_PARAMS(Z_Construct_UClass_AGravityCharacter_Statics::Class_MetaDataParams, UE_ARRAY_COUNT(Z_Construct_UClass_AGravityCharacter_Statics::Class_MetaDataParams))
	};
	UClass* Z_Construct_UClass_AGravityCharacter()
	{
		if (!Z_Registration_Info_UClass_AGravityCharacter.OuterSingleton)
		{
			UECodeGen_Private::ConstructUClass(Z_Registration_Info_UClass_AGravityCharacter.OuterSingleton, Z_Construct_UClass_AGravityCharacter_Statics::ClassParams);
		}
		return Z_Registration_Info_UClass_AGravityCharacter.OuterSingleton;
	}
	template<> APS_ALPHA_API UClass* StaticClass<AGravityCharacter>()
	{
		return AGravityCharacter::StaticClass();
	}
	DEFINE_VTABLE_PTR_HELPER_CTOR(AGravityCharacter);
	AGravityCharacter::~AGravityCharacter() {}
	struct Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_GravityCharacter_h_Statics
	{
		static const FClassRegisterCompiledInInfo ClassInfo[];
	};
	const FClassRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_GravityCharacter_h_Statics::ClassInfo[] = {
		{ Z_Construct_UClass_AGravityCharacter, AGravityCharacter::StaticClass, TEXT("AGravityCharacter"), &Z_Registration_Info_UClass_AGravityCharacter, CONSTRUCT_RELOAD_VERSION_INFO(FClassReloadVersionInfo, sizeof(AGravityCharacter), 2904076544U) },
	};
	static FRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_GravityCharacter_h_802882401(TEXT("/Script/APS_ALPHA"),
		Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_GravityCharacter_h_Statics::ClassInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_GravityCharacter_h_Statics::ClassInfo),
		nullptr, 0,
		nullptr, 0);
PRAGMA_ENABLE_DEPRECATION_WARNINGS
