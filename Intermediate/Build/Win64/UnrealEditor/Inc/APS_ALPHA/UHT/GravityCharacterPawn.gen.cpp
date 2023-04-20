// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
#include "APS_ALPHA/GravityCharacterPawn.h"
PRAGMA_DISABLE_DEPRECATION_WARNINGS
void EmptyLinkFunctionForGeneratedCodeGravityCharacterPawn() {}
// Cross Module References
	APS_ALPHA_API UClass* Z_Construct_UClass_AGravityCharacterPawn();
	APS_ALPHA_API UClass* Z_Construct_UClass_AGravityCharacterPawn_NoRegister();
	ENGINE_API UClass* Z_Construct_UClass_APawn();
	ENGINE_API UClass* Z_Construct_UClass_UArrowComponent_NoRegister();
	ENGINE_API UClass* Z_Construct_UClass_UCameraComponent_NoRegister();
	ENGINE_API UClass* Z_Construct_UClass_UCapsuleComponent_NoRegister();
	ENGINE_API UClass* Z_Construct_UClass_USceneComponent_NoRegister();
	ENGINE_API UClass* Z_Construct_UClass_USkeletalMeshComponent_NoRegister();
	ENGINE_API UClass* Z_Construct_UClass_USpringArmComponent_NoRegister();
	UPackage* Z_Construct_UPackage__Script_APS_ALPHA();
// End Cross Module References
	DEFINE_FUNCTION(AGravityCharacterPawn::execRotateYaw)
	{
		P_GET_PROPERTY(FFloatProperty,Z_Param_Value);
		P_FINISH;
		P_NATIVE_BEGIN;
		P_THIS->RotateYaw(Z_Param_Value);
		P_NATIVE_END;
	}
	DEFINE_FUNCTION(AGravityCharacterPawn::execRotateRoll)
	{
		P_GET_PROPERTY(FFloatProperty,Z_Param_Value);
		P_FINISH;
		P_NATIVE_BEGIN;
		P_THIS->RotateRoll(Z_Param_Value);
		P_NATIVE_END;
	}
	DEFINE_FUNCTION(AGravityCharacterPawn::execRotatePitch)
	{
		P_GET_PROPERTY(FFloatProperty,Z_Param_Value);
		P_FINISH;
		P_NATIVE_BEGIN;
		P_THIS->RotatePitch(Z_Param_Value);
		P_NATIVE_END;
	}
	DEFINE_FUNCTION(AGravityCharacterPawn::execMoveUp)
	{
		P_GET_PROPERTY(FFloatProperty,Z_Param_Value);
		P_FINISH;
		P_NATIVE_BEGIN;
		P_THIS->MoveUp(Z_Param_Value);
		P_NATIVE_END;
	}
	DEFINE_FUNCTION(AGravityCharacterPawn::execMoveRight)
	{
		P_GET_PROPERTY(FFloatProperty,Z_Param_Value);
		P_FINISH;
		P_NATIVE_BEGIN;
		P_THIS->MoveRight(Z_Param_Value);
		P_NATIVE_END;
	}
	DEFINE_FUNCTION(AGravityCharacterPawn::execMoveForward)
	{
		P_GET_PROPERTY(FFloatProperty,Z_Param_Value);
		P_FINISH;
		P_NATIVE_BEGIN;
		P_THIS->MoveForward(Z_Param_Value);
		P_NATIVE_END;
	}
	DEFINE_FUNCTION(AGravityCharacterPawn::execLookUp)
	{
		P_GET_PROPERTY(FFloatProperty,Z_Param_Value);
		P_FINISH;
		P_NATIVE_BEGIN;
		P_THIS->LookUp(Z_Param_Value);
		P_NATIVE_END;
	}
	DEFINE_FUNCTION(AGravityCharacterPawn::execTurn)
	{
		P_GET_PROPERTY(FFloatProperty,Z_Param_Value);
		P_FINISH;
		P_NATIVE_BEGIN;
		P_THIS->Turn(Z_Param_Value);
		P_NATIVE_END;
	}
	void AGravityCharacterPawn::StaticRegisterNativesAGravityCharacterPawn()
	{
		UClass* Class = AGravityCharacterPawn::StaticClass();
		static const FNameNativePtrPair Funcs[] = {
			{ "LookUp", &AGravityCharacterPawn::execLookUp },
			{ "MoveForward", &AGravityCharacterPawn::execMoveForward },
			{ "MoveRight", &AGravityCharacterPawn::execMoveRight },
			{ "MoveUp", &AGravityCharacterPawn::execMoveUp },
			{ "RotatePitch", &AGravityCharacterPawn::execRotatePitch },
			{ "RotateRoll", &AGravityCharacterPawn::execRotateRoll },
			{ "RotateYaw", &AGravityCharacterPawn::execRotateYaw },
			{ "Turn", &AGravityCharacterPawn::execTurn },
		};
		FNativeFunctionRegistrar::RegisterFunctions(Class, Funcs, UE_ARRAY_COUNT(Funcs));
	}
	struct Z_Construct_UFunction_AGravityCharacterPawn_LookUp_Statics
	{
		struct GravityCharacterPawn_eventLookUp_Parms
		{
			float Value;
		};
		static const UECodeGen_Private::FFloatPropertyParams NewProp_Value;
		static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam Function_MetaDataParams[];
#endif
		static const UECodeGen_Private::FFunctionParams FuncParams;
	};
	const UECodeGen_Private::FFloatPropertyParams Z_Construct_UFunction_AGravityCharacterPawn_LookUp_Statics::NewProp_Value = { "Value", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Float, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(GravityCharacterPawn_eventLookUp_Parms, Value), METADATA_PARAMS(nullptr, 0) };
	const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UFunction_AGravityCharacterPawn_LookUp_Statics::PropPointers[] = {
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_AGravityCharacterPawn_LookUp_Statics::NewProp_Value,
	};
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UFunction_AGravityCharacterPawn_LookUp_Statics::Function_MetaDataParams[] = {
		{ "ModuleRelativePath", "GravityCharacterPawn.h" },
	};
#endif
	const UECodeGen_Private::FFunctionParams Z_Construct_UFunction_AGravityCharacterPawn_LookUp_Statics::FuncParams = { (UObject*(*)())Z_Construct_UClass_AGravityCharacterPawn, nullptr, "LookUp", nullptr, nullptr, sizeof(Z_Construct_UFunction_AGravityCharacterPawn_LookUp_Statics::GravityCharacterPawn_eventLookUp_Parms), Z_Construct_UFunction_AGravityCharacterPawn_LookUp_Statics::PropPointers, UE_ARRAY_COUNT(Z_Construct_UFunction_AGravityCharacterPawn_LookUp_Statics::PropPointers), RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x00020401, 0, 0, METADATA_PARAMS(Z_Construct_UFunction_AGravityCharacterPawn_LookUp_Statics::Function_MetaDataParams, UE_ARRAY_COUNT(Z_Construct_UFunction_AGravityCharacterPawn_LookUp_Statics::Function_MetaDataParams)) };
	UFunction* Z_Construct_UFunction_AGravityCharacterPawn_LookUp()
	{
		static UFunction* ReturnFunction = nullptr;
		if (!ReturnFunction)
		{
			UECodeGen_Private::ConstructUFunction(&ReturnFunction, Z_Construct_UFunction_AGravityCharacterPawn_LookUp_Statics::FuncParams);
		}
		return ReturnFunction;
	}
	struct Z_Construct_UFunction_AGravityCharacterPawn_MoveForward_Statics
	{
		struct GravityCharacterPawn_eventMoveForward_Parms
		{
			float Value;
		};
		static const UECodeGen_Private::FFloatPropertyParams NewProp_Value;
		static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam Function_MetaDataParams[];
#endif
		static const UECodeGen_Private::FFunctionParams FuncParams;
	};
	const UECodeGen_Private::FFloatPropertyParams Z_Construct_UFunction_AGravityCharacterPawn_MoveForward_Statics::NewProp_Value = { "Value", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Float, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(GravityCharacterPawn_eventMoveForward_Parms, Value), METADATA_PARAMS(nullptr, 0) };
	const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UFunction_AGravityCharacterPawn_MoveForward_Statics::PropPointers[] = {
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_AGravityCharacterPawn_MoveForward_Statics::NewProp_Value,
	};
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UFunction_AGravityCharacterPawn_MoveForward_Statics::Function_MetaDataParams[] = {
		{ "ModuleRelativePath", "GravityCharacterPawn.h" },
	};
#endif
	const UECodeGen_Private::FFunctionParams Z_Construct_UFunction_AGravityCharacterPawn_MoveForward_Statics::FuncParams = { (UObject*(*)())Z_Construct_UClass_AGravityCharacterPawn, nullptr, "MoveForward", nullptr, nullptr, sizeof(Z_Construct_UFunction_AGravityCharacterPawn_MoveForward_Statics::GravityCharacterPawn_eventMoveForward_Parms), Z_Construct_UFunction_AGravityCharacterPawn_MoveForward_Statics::PropPointers, UE_ARRAY_COUNT(Z_Construct_UFunction_AGravityCharacterPawn_MoveForward_Statics::PropPointers), RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x00020401, 0, 0, METADATA_PARAMS(Z_Construct_UFunction_AGravityCharacterPawn_MoveForward_Statics::Function_MetaDataParams, UE_ARRAY_COUNT(Z_Construct_UFunction_AGravityCharacterPawn_MoveForward_Statics::Function_MetaDataParams)) };
	UFunction* Z_Construct_UFunction_AGravityCharacterPawn_MoveForward()
	{
		static UFunction* ReturnFunction = nullptr;
		if (!ReturnFunction)
		{
			UECodeGen_Private::ConstructUFunction(&ReturnFunction, Z_Construct_UFunction_AGravityCharacterPawn_MoveForward_Statics::FuncParams);
		}
		return ReturnFunction;
	}
	struct Z_Construct_UFunction_AGravityCharacterPawn_MoveRight_Statics
	{
		struct GravityCharacterPawn_eventMoveRight_Parms
		{
			float Value;
		};
		static const UECodeGen_Private::FFloatPropertyParams NewProp_Value;
		static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam Function_MetaDataParams[];
#endif
		static const UECodeGen_Private::FFunctionParams FuncParams;
	};
	const UECodeGen_Private::FFloatPropertyParams Z_Construct_UFunction_AGravityCharacterPawn_MoveRight_Statics::NewProp_Value = { "Value", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Float, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(GravityCharacterPawn_eventMoveRight_Parms, Value), METADATA_PARAMS(nullptr, 0) };
	const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UFunction_AGravityCharacterPawn_MoveRight_Statics::PropPointers[] = {
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_AGravityCharacterPawn_MoveRight_Statics::NewProp_Value,
	};
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UFunction_AGravityCharacterPawn_MoveRight_Statics::Function_MetaDataParams[] = {
		{ "ModuleRelativePath", "GravityCharacterPawn.h" },
	};
#endif
	const UECodeGen_Private::FFunctionParams Z_Construct_UFunction_AGravityCharacterPawn_MoveRight_Statics::FuncParams = { (UObject*(*)())Z_Construct_UClass_AGravityCharacterPawn, nullptr, "MoveRight", nullptr, nullptr, sizeof(Z_Construct_UFunction_AGravityCharacterPawn_MoveRight_Statics::GravityCharacterPawn_eventMoveRight_Parms), Z_Construct_UFunction_AGravityCharacterPawn_MoveRight_Statics::PropPointers, UE_ARRAY_COUNT(Z_Construct_UFunction_AGravityCharacterPawn_MoveRight_Statics::PropPointers), RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x00020401, 0, 0, METADATA_PARAMS(Z_Construct_UFunction_AGravityCharacterPawn_MoveRight_Statics::Function_MetaDataParams, UE_ARRAY_COUNT(Z_Construct_UFunction_AGravityCharacterPawn_MoveRight_Statics::Function_MetaDataParams)) };
	UFunction* Z_Construct_UFunction_AGravityCharacterPawn_MoveRight()
	{
		static UFunction* ReturnFunction = nullptr;
		if (!ReturnFunction)
		{
			UECodeGen_Private::ConstructUFunction(&ReturnFunction, Z_Construct_UFunction_AGravityCharacterPawn_MoveRight_Statics::FuncParams);
		}
		return ReturnFunction;
	}
	struct Z_Construct_UFunction_AGravityCharacterPawn_MoveUp_Statics
	{
		struct GravityCharacterPawn_eventMoveUp_Parms
		{
			float Value;
		};
		static const UECodeGen_Private::FFloatPropertyParams NewProp_Value;
		static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam Function_MetaDataParams[];
#endif
		static const UECodeGen_Private::FFunctionParams FuncParams;
	};
	const UECodeGen_Private::FFloatPropertyParams Z_Construct_UFunction_AGravityCharacterPawn_MoveUp_Statics::NewProp_Value = { "Value", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Float, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(GravityCharacterPawn_eventMoveUp_Parms, Value), METADATA_PARAMS(nullptr, 0) };
	const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UFunction_AGravityCharacterPawn_MoveUp_Statics::PropPointers[] = {
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_AGravityCharacterPawn_MoveUp_Statics::NewProp_Value,
	};
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UFunction_AGravityCharacterPawn_MoveUp_Statics::Function_MetaDataParams[] = {
		{ "ModuleRelativePath", "GravityCharacterPawn.h" },
	};
#endif
	const UECodeGen_Private::FFunctionParams Z_Construct_UFunction_AGravityCharacterPawn_MoveUp_Statics::FuncParams = { (UObject*(*)())Z_Construct_UClass_AGravityCharacterPawn, nullptr, "MoveUp", nullptr, nullptr, sizeof(Z_Construct_UFunction_AGravityCharacterPawn_MoveUp_Statics::GravityCharacterPawn_eventMoveUp_Parms), Z_Construct_UFunction_AGravityCharacterPawn_MoveUp_Statics::PropPointers, UE_ARRAY_COUNT(Z_Construct_UFunction_AGravityCharacterPawn_MoveUp_Statics::PropPointers), RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x00020401, 0, 0, METADATA_PARAMS(Z_Construct_UFunction_AGravityCharacterPawn_MoveUp_Statics::Function_MetaDataParams, UE_ARRAY_COUNT(Z_Construct_UFunction_AGravityCharacterPawn_MoveUp_Statics::Function_MetaDataParams)) };
	UFunction* Z_Construct_UFunction_AGravityCharacterPawn_MoveUp()
	{
		static UFunction* ReturnFunction = nullptr;
		if (!ReturnFunction)
		{
			UECodeGen_Private::ConstructUFunction(&ReturnFunction, Z_Construct_UFunction_AGravityCharacterPawn_MoveUp_Statics::FuncParams);
		}
		return ReturnFunction;
	}
	struct Z_Construct_UFunction_AGravityCharacterPawn_RotatePitch_Statics
	{
		struct GravityCharacterPawn_eventRotatePitch_Parms
		{
			float Value;
		};
		static const UECodeGen_Private::FFloatPropertyParams NewProp_Value;
		static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam Function_MetaDataParams[];
#endif
		static const UECodeGen_Private::FFunctionParams FuncParams;
	};
	const UECodeGen_Private::FFloatPropertyParams Z_Construct_UFunction_AGravityCharacterPawn_RotatePitch_Statics::NewProp_Value = { "Value", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Float, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(GravityCharacterPawn_eventRotatePitch_Parms, Value), METADATA_PARAMS(nullptr, 0) };
	const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UFunction_AGravityCharacterPawn_RotatePitch_Statics::PropPointers[] = {
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_AGravityCharacterPawn_RotatePitch_Statics::NewProp_Value,
	};
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UFunction_AGravityCharacterPawn_RotatePitch_Statics::Function_MetaDataParams[] = {
		{ "ModuleRelativePath", "GravityCharacterPawn.h" },
	};
#endif
	const UECodeGen_Private::FFunctionParams Z_Construct_UFunction_AGravityCharacterPawn_RotatePitch_Statics::FuncParams = { (UObject*(*)())Z_Construct_UClass_AGravityCharacterPawn, nullptr, "RotatePitch", nullptr, nullptr, sizeof(Z_Construct_UFunction_AGravityCharacterPawn_RotatePitch_Statics::GravityCharacterPawn_eventRotatePitch_Parms), Z_Construct_UFunction_AGravityCharacterPawn_RotatePitch_Statics::PropPointers, UE_ARRAY_COUNT(Z_Construct_UFunction_AGravityCharacterPawn_RotatePitch_Statics::PropPointers), RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x00020401, 0, 0, METADATA_PARAMS(Z_Construct_UFunction_AGravityCharacterPawn_RotatePitch_Statics::Function_MetaDataParams, UE_ARRAY_COUNT(Z_Construct_UFunction_AGravityCharacterPawn_RotatePitch_Statics::Function_MetaDataParams)) };
	UFunction* Z_Construct_UFunction_AGravityCharacterPawn_RotatePitch()
	{
		static UFunction* ReturnFunction = nullptr;
		if (!ReturnFunction)
		{
			UECodeGen_Private::ConstructUFunction(&ReturnFunction, Z_Construct_UFunction_AGravityCharacterPawn_RotatePitch_Statics::FuncParams);
		}
		return ReturnFunction;
	}
	struct Z_Construct_UFunction_AGravityCharacterPawn_RotateRoll_Statics
	{
		struct GravityCharacterPawn_eventRotateRoll_Parms
		{
			float Value;
		};
		static const UECodeGen_Private::FFloatPropertyParams NewProp_Value;
		static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam Function_MetaDataParams[];
#endif
		static const UECodeGen_Private::FFunctionParams FuncParams;
	};
	const UECodeGen_Private::FFloatPropertyParams Z_Construct_UFunction_AGravityCharacterPawn_RotateRoll_Statics::NewProp_Value = { "Value", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Float, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(GravityCharacterPawn_eventRotateRoll_Parms, Value), METADATA_PARAMS(nullptr, 0) };
	const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UFunction_AGravityCharacterPawn_RotateRoll_Statics::PropPointers[] = {
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_AGravityCharacterPawn_RotateRoll_Statics::NewProp_Value,
	};
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UFunction_AGravityCharacterPawn_RotateRoll_Statics::Function_MetaDataParams[] = {
		{ "ModuleRelativePath", "GravityCharacterPawn.h" },
	};
#endif
	const UECodeGen_Private::FFunctionParams Z_Construct_UFunction_AGravityCharacterPawn_RotateRoll_Statics::FuncParams = { (UObject*(*)())Z_Construct_UClass_AGravityCharacterPawn, nullptr, "RotateRoll", nullptr, nullptr, sizeof(Z_Construct_UFunction_AGravityCharacterPawn_RotateRoll_Statics::GravityCharacterPawn_eventRotateRoll_Parms), Z_Construct_UFunction_AGravityCharacterPawn_RotateRoll_Statics::PropPointers, UE_ARRAY_COUNT(Z_Construct_UFunction_AGravityCharacterPawn_RotateRoll_Statics::PropPointers), RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x00020401, 0, 0, METADATA_PARAMS(Z_Construct_UFunction_AGravityCharacterPawn_RotateRoll_Statics::Function_MetaDataParams, UE_ARRAY_COUNT(Z_Construct_UFunction_AGravityCharacterPawn_RotateRoll_Statics::Function_MetaDataParams)) };
	UFunction* Z_Construct_UFunction_AGravityCharacterPawn_RotateRoll()
	{
		static UFunction* ReturnFunction = nullptr;
		if (!ReturnFunction)
		{
			UECodeGen_Private::ConstructUFunction(&ReturnFunction, Z_Construct_UFunction_AGravityCharacterPawn_RotateRoll_Statics::FuncParams);
		}
		return ReturnFunction;
	}
	struct Z_Construct_UFunction_AGravityCharacterPawn_RotateYaw_Statics
	{
		struct GravityCharacterPawn_eventRotateYaw_Parms
		{
			float Value;
		};
		static const UECodeGen_Private::FFloatPropertyParams NewProp_Value;
		static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam Function_MetaDataParams[];
#endif
		static const UECodeGen_Private::FFunctionParams FuncParams;
	};
	const UECodeGen_Private::FFloatPropertyParams Z_Construct_UFunction_AGravityCharacterPawn_RotateYaw_Statics::NewProp_Value = { "Value", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Float, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(GravityCharacterPawn_eventRotateYaw_Parms, Value), METADATA_PARAMS(nullptr, 0) };
	const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UFunction_AGravityCharacterPawn_RotateYaw_Statics::PropPointers[] = {
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_AGravityCharacterPawn_RotateYaw_Statics::NewProp_Value,
	};
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UFunction_AGravityCharacterPawn_RotateYaw_Statics::Function_MetaDataParams[] = {
		{ "ModuleRelativePath", "GravityCharacterPawn.h" },
	};
#endif
	const UECodeGen_Private::FFunctionParams Z_Construct_UFunction_AGravityCharacterPawn_RotateYaw_Statics::FuncParams = { (UObject*(*)())Z_Construct_UClass_AGravityCharacterPawn, nullptr, "RotateYaw", nullptr, nullptr, sizeof(Z_Construct_UFunction_AGravityCharacterPawn_RotateYaw_Statics::GravityCharacterPawn_eventRotateYaw_Parms), Z_Construct_UFunction_AGravityCharacterPawn_RotateYaw_Statics::PropPointers, UE_ARRAY_COUNT(Z_Construct_UFunction_AGravityCharacterPawn_RotateYaw_Statics::PropPointers), RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x00020401, 0, 0, METADATA_PARAMS(Z_Construct_UFunction_AGravityCharacterPawn_RotateYaw_Statics::Function_MetaDataParams, UE_ARRAY_COUNT(Z_Construct_UFunction_AGravityCharacterPawn_RotateYaw_Statics::Function_MetaDataParams)) };
	UFunction* Z_Construct_UFunction_AGravityCharacterPawn_RotateYaw()
	{
		static UFunction* ReturnFunction = nullptr;
		if (!ReturnFunction)
		{
			UECodeGen_Private::ConstructUFunction(&ReturnFunction, Z_Construct_UFunction_AGravityCharacterPawn_RotateYaw_Statics::FuncParams);
		}
		return ReturnFunction;
	}
	struct Z_Construct_UFunction_AGravityCharacterPawn_Turn_Statics
	{
		struct GravityCharacterPawn_eventTurn_Parms
		{
			float Value;
		};
		static const UECodeGen_Private::FFloatPropertyParams NewProp_Value;
		static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam Function_MetaDataParams[];
#endif
		static const UECodeGen_Private::FFunctionParams FuncParams;
	};
	const UECodeGen_Private::FFloatPropertyParams Z_Construct_UFunction_AGravityCharacterPawn_Turn_Statics::NewProp_Value = { "Value", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Float, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(GravityCharacterPawn_eventTurn_Parms, Value), METADATA_PARAMS(nullptr, 0) };
	const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UFunction_AGravityCharacterPawn_Turn_Statics::PropPointers[] = {
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_AGravityCharacterPawn_Turn_Statics::NewProp_Value,
	};
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UFunction_AGravityCharacterPawn_Turn_Statics::Function_MetaDataParams[] = {
		{ "ModuleRelativePath", "GravityCharacterPawn.h" },
	};
#endif
	const UECodeGen_Private::FFunctionParams Z_Construct_UFunction_AGravityCharacterPawn_Turn_Statics::FuncParams = { (UObject*(*)())Z_Construct_UClass_AGravityCharacterPawn, nullptr, "Turn", nullptr, nullptr, sizeof(Z_Construct_UFunction_AGravityCharacterPawn_Turn_Statics::GravityCharacterPawn_eventTurn_Parms), Z_Construct_UFunction_AGravityCharacterPawn_Turn_Statics::PropPointers, UE_ARRAY_COUNT(Z_Construct_UFunction_AGravityCharacterPawn_Turn_Statics::PropPointers), RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x00020401, 0, 0, METADATA_PARAMS(Z_Construct_UFunction_AGravityCharacterPawn_Turn_Statics::Function_MetaDataParams, UE_ARRAY_COUNT(Z_Construct_UFunction_AGravityCharacterPawn_Turn_Statics::Function_MetaDataParams)) };
	UFunction* Z_Construct_UFunction_AGravityCharacterPawn_Turn()
	{
		static UFunction* ReturnFunction = nullptr;
		if (!ReturnFunction)
		{
			UECodeGen_Private::ConstructUFunction(&ReturnFunction, Z_Construct_UFunction_AGravityCharacterPawn_Turn_Statics::FuncParams);
		}
		return ReturnFunction;
	}
	IMPLEMENT_CLASS_NO_AUTO_REGISTRATION(AGravityCharacterPawn);
	UClass* Z_Construct_UClass_AGravityCharacterPawn_NoRegister()
	{
		return AGravityCharacterPawn::StaticClass();
	}
	struct Z_Construct_UClass_AGravityCharacterPawn_Statics
	{
		static UObject* (*const DependentSingletons[])();
		static const FClassFunctionLinkInfo FuncInfo[];
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam Class_MetaDataParams[];
#endif
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_RootSceneComponent_MetaData[];
#endif
		static const UECodeGen_Private::FObjectPropertyParams NewProp_RootSceneComponent;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_CapsuleComponent_MetaData[];
#endif
		static const UECodeGen_Private::FObjectPropertyParams NewProp_CapsuleComponent;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_ArrowForwardVector_MetaData[];
#endif
		static const UECodeGen_Private::FObjectPropertyParams NewProp_ArrowForwardVector;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_MeshComponent_MetaData[];
#endif
		static const UECodeGen_Private::FObjectPropertyParams NewProp_MeshComponent;
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
	UObject* (*const Z_Construct_UClass_AGravityCharacterPawn_Statics::DependentSingletons[])() = {
		(UObject* (*)())Z_Construct_UClass_APawn,
		(UObject* (*)())Z_Construct_UPackage__Script_APS_ALPHA,
	};
	const FClassFunctionLinkInfo Z_Construct_UClass_AGravityCharacterPawn_Statics::FuncInfo[] = {
		{ &Z_Construct_UFunction_AGravityCharacterPawn_LookUp, "LookUp" }, // 3537675169
		{ &Z_Construct_UFunction_AGravityCharacterPawn_MoveForward, "MoveForward" }, // 3752209854
		{ &Z_Construct_UFunction_AGravityCharacterPawn_MoveRight, "MoveRight" }, // 2636117551
		{ &Z_Construct_UFunction_AGravityCharacterPawn_MoveUp, "MoveUp" }, // 2822253874
		{ &Z_Construct_UFunction_AGravityCharacterPawn_RotatePitch, "RotatePitch" }, // 2322532327
		{ &Z_Construct_UFunction_AGravityCharacterPawn_RotateRoll, "RotateRoll" }, // 1331998239
		{ &Z_Construct_UFunction_AGravityCharacterPawn_RotateYaw, "RotateYaw" }, // 2294616694
		{ &Z_Construct_UFunction_AGravityCharacterPawn_Turn, "Turn" }, // 693459909
	};
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_AGravityCharacterPawn_Statics::Class_MetaDataParams[] = {
		{ "HideCategories", "Navigation" },
		{ "IncludePath", "GravityCharacterPawn.h" },
		{ "ModuleRelativePath", "GravityCharacterPawn.h" },
	};
#endif
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_AGravityCharacterPawn_Statics::NewProp_RootSceneComponent_MetaData[] = {
		{ "Category", "Components" },
		{ "EditInline", "true" },
		{ "ModuleRelativePath", "GravityCharacterPawn.h" },
	};
#endif
	const UECodeGen_Private::FObjectPropertyParams Z_Construct_UClass_AGravityCharacterPawn_Statics::NewProp_RootSceneComponent = { "RootSceneComponent", nullptr, (EPropertyFlags)0x00100000000a001d, UECodeGen_Private::EPropertyGenFlags::Object, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(AGravityCharacterPawn, RootSceneComponent), Z_Construct_UClass_USceneComponent_NoRegister, METADATA_PARAMS(Z_Construct_UClass_AGravityCharacterPawn_Statics::NewProp_RootSceneComponent_MetaData, UE_ARRAY_COUNT(Z_Construct_UClass_AGravityCharacterPawn_Statics::NewProp_RootSceneComponent_MetaData)) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_AGravityCharacterPawn_Statics::NewProp_CapsuleComponent_MetaData[] = {
		{ "Category", "Components" },
		{ "EditInline", "true" },
		{ "ModuleRelativePath", "GravityCharacterPawn.h" },
	};
#endif
	const UECodeGen_Private::FObjectPropertyParams Z_Construct_UClass_AGravityCharacterPawn_Statics::NewProp_CapsuleComponent = { "CapsuleComponent", nullptr, (EPropertyFlags)0x00100000000a001d, UECodeGen_Private::EPropertyGenFlags::Object, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(AGravityCharacterPawn, CapsuleComponent), Z_Construct_UClass_UCapsuleComponent_NoRegister, METADATA_PARAMS(Z_Construct_UClass_AGravityCharacterPawn_Statics::NewProp_CapsuleComponent_MetaData, UE_ARRAY_COUNT(Z_Construct_UClass_AGravityCharacterPawn_Statics::NewProp_CapsuleComponent_MetaData)) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_AGravityCharacterPawn_Statics::NewProp_ArrowForwardVector_MetaData[] = {
		{ "Category", "Components" },
		{ "EditInline", "true" },
		{ "ModuleRelativePath", "GravityCharacterPawn.h" },
	};
#endif
	const UECodeGen_Private::FObjectPropertyParams Z_Construct_UClass_AGravityCharacterPawn_Statics::NewProp_ArrowForwardVector = { "ArrowForwardVector", nullptr, (EPropertyFlags)0x00100000000a001d, UECodeGen_Private::EPropertyGenFlags::Object, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(AGravityCharacterPawn, ArrowForwardVector), Z_Construct_UClass_UArrowComponent_NoRegister, METADATA_PARAMS(Z_Construct_UClass_AGravityCharacterPawn_Statics::NewProp_ArrowForwardVector_MetaData, UE_ARRAY_COUNT(Z_Construct_UClass_AGravityCharacterPawn_Statics::NewProp_ArrowForwardVector_MetaData)) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_AGravityCharacterPawn_Statics::NewProp_MeshComponent_MetaData[] = {
		{ "Category", "Components" },
		{ "EditInline", "true" },
		{ "ModuleRelativePath", "GravityCharacterPawn.h" },
	};
#endif
	const UECodeGen_Private::FObjectPropertyParams Z_Construct_UClass_AGravityCharacterPawn_Statics::NewProp_MeshComponent = { "MeshComponent", nullptr, (EPropertyFlags)0x00100000000a001d, UECodeGen_Private::EPropertyGenFlags::Object, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(AGravityCharacterPawn, MeshComponent), Z_Construct_UClass_USkeletalMeshComponent_NoRegister, METADATA_PARAMS(Z_Construct_UClass_AGravityCharacterPawn_Statics::NewProp_MeshComponent_MetaData, UE_ARRAY_COUNT(Z_Construct_UClass_AGravityCharacterPawn_Statics::NewProp_MeshComponent_MetaData)) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_AGravityCharacterPawn_Statics::NewProp_CameraSpringArm_MetaData[] = {
		{ "Category", "Components" },
		{ "EditInline", "true" },
		{ "ModuleRelativePath", "GravityCharacterPawn.h" },
	};
#endif
	const UECodeGen_Private::FObjectPropertyParams Z_Construct_UClass_AGravityCharacterPawn_Statics::NewProp_CameraSpringArm = { "CameraSpringArm", nullptr, (EPropertyFlags)0x00100000000a001d, UECodeGen_Private::EPropertyGenFlags::Object, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(AGravityCharacterPawn, CameraSpringArm), Z_Construct_UClass_USpringArmComponent_NoRegister, METADATA_PARAMS(Z_Construct_UClass_AGravityCharacterPawn_Statics::NewProp_CameraSpringArm_MetaData, UE_ARRAY_COUNT(Z_Construct_UClass_AGravityCharacterPawn_Statics::NewProp_CameraSpringArm_MetaData)) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_AGravityCharacterPawn_Statics::NewProp_PlayerCamera_MetaData[] = {
		{ "Category", "Components" },
		{ "EditInline", "true" },
		{ "ModuleRelativePath", "GravityCharacterPawn.h" },
	};
#endif
	const UECodeGen_Private::FObjectPropertyParams Z_Construct_UClass_AGravityCharacterPawn_Statics::NewProp_PlayerCamera = { "PlayerCamera", nullptr, (EPropertyFlags)0x00100000000a001d, UECodeGen_Private::EPropertyGenFlags::Object, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(AGravityCharacterPawn, PlayerCamera), Z_Construct_UClass_UCameraComponent_NoRegister, METADATA_PARAMS(Z_Construct_UClass_AGravityCharacterPawn_Statics::NewProp_PlayerCamera_MetaData, UE_ARRAY_COUNT(Z_Construct_UClass_AGravityCharacterPawn_Statics::NewProp_PlayerCamera_MetaData)) };
	const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UClass_AGravityCharacterPawn_Statics::PropPointers[] = {
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_AGravityCharacterPawn_Statics::NewProp_RootSceneComponent,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_AGravityCharacterPawn_Statics::NewProp_CapsuleComponent,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_AGravityCharacterPawn_Statics::NewProp_ArrowForwardVector,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_AGravityCharacterPawn_Statics::NewProp_MeshComponent,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_AGravityCharacterPawn_Statics::NewProp_CameraSpringArm,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_AGravityCharacterPawn_Statics::NewProp_PlayerCamera,
	};
	const FCppClassTypeInfoStatic Z_Construct_UClass_AGravityCharacterPawn_Statics::StaticCppClassTypeInfo = {
		TCppClassTypeTraits<AGravityCharacterPawn>::IsAbstract,
	};
	const UECodeGen_Private::FClassParams Z_Construct_UClass_AGravityCharacterPawn_Statics::ClassParams = {
		&AGravityCharacterPawn::StaticClass,
		"Game",
		&StaticCppClassTypeInfo,
		DependentSingletons,
		FuncInfo,
		Z_Construct_UClass_AGravityCharacterPawn_Statics::PropPointers,
		nullptr,
		UE_ARRAY_COUNT(DependentSingletons),
		UE_ARRAY_COUNT(FuncInfo),
		UE_ARRAY_COUNT(Z_Construct_UClass_AGravityCharacterPawn_Statics::PropPointers),
		0,
		0x009000A4u,
		METADATA_PARAMS(Z_Construct_UClass_AGravityCharacterPawn_Statics::Class_MetaDataParams, UE_ARRAY_COUNT(Z_Construct_UClass_AGravityCharacterPawn_Statics::Class_MetaDataParams))
	};
	UClass* Z_Construct_UClass_AGravityCharacterPawn()
	{
		if (!Z_Registration_Info_UClass_AGravityCharacterPawn.OuterSingleton)
		{
			UECodeGen_Private::ConstructUClass(Z_Registration_Info_UClass_AGravityCharacterPawn.OuterSingleton, Z_Construct_UClass_AGravityCharacterPawn_Statics::ClassParams);
		}
		return Z_Registration_Info_UClass_AGravityCharacterPawn.OuterSingleton;
	}
	template<> APS_ALPHA_API UClass* StaticClass<AGravityCharacterPawn>()
	{
		return AGravityCharacterPawn::StaticClass();
	}
	DEFINE_VTABLE_PTR_HELPER_CTOR(AGravityCharacterPawn);
	AGravityCharacterPawn::~AGravityCharacterPawn() {}
	struct Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_GravityCharacterPawn_h_Statics
	{
		static const FClassRegisterCompiledInInfo ClassInfo[];
	};
	const FClassRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_GravityCharacterPawn_h_Statics::ClassInfo[] = {
		{ Z_Construct_UClass_AGravityCharacterPawn, AGravityCharacterPawn::StaticClass, TEXT("AGravityCharacterPawn"), &Z_Registration_Info_UClass_AGravityCharacterPawn, CONSTRUCT_RELOAD_VERSION_INFO(FClassReloadVersionInfo, sizeof(AGravityCharacterPawn), 2222584720U) },
	};
	static FRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_GravityCharacterPawn_h_1704721357(TEXT("/Script/APS_ALPHA"),
		Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_GravityCharacterPawn_h_Statics::ClassInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_Users_romar_Documents_Unreal_Projects_APS_APS_ALPHA_Source_APS_ALPHA_GravityCharacterPawn_h_Statics::ClassInfo),
		nullptr, 0,
		nullptr, 0);
PRAGMA_ENABLE_DEPRECATION_WARNINGS
