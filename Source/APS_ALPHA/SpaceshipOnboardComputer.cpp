// Fill out your copyright notice in the Description page of Project Settings.


#include "SpaceshipOnboardComputer.h"
#include "UObject/UnrealType.h"
#include "UObject/Class.h"
#include "UObject/EnumProperty.h"

//USpaceshipOnboardComputer::USpaceshipOnboardComputer()
//{
//    
//}

USpaceshipOnboardComputer::USpaceshipOnboardComputer()
{
}

//FString USpaceshipOnboardComputer::GetCurrentFlightType()
//{
//    UEnum* EnumPtr = FindObject<UEnum>(GetTransientPackage(), TEXT("EFlightType"), true);
//    if (EnumPtr)
//    {
//        FString FlightTypeString = EnumPtr->GetNameStringByIndex(static_cast<int32>(FlightSystemInstance.CurrentFlightType));
//        return FlightTypeString;
//    }
//    
//    return FString();
//    
//    
//    
//    //UEnum::GetEnumName(FlightSystemInstance.CurrentFlightType);
//    //GETENUMSTRING("EFlightType", FlightSystemInstance.CurrentFlightType);
//    //return StaticEnum<EFlightType>()->GetNameStringByValue(static_cast<int64>(FlightSystemInstance.CurrentFlightType));
//    //return FString();
//}
//FString USpaceshipOnboardComputer::GetCurrentFlightType()
//{
//   
//    const TEnumAsByte<EFlightType> SurfaceEnum = FlightSystemInstance.CurrentFlightType;
//    FString EnumAsString = UEnum::GetValueAsString(SurfaceEnum.GetValue());
//    return EnumAsString;
//
//}

//const FString EnumToString(const TCHAR* Enum, int32 EnumValue)
//{
//    const UEnum* EnumPtr = FindObject<UEnum>(ANY_PACKAGE, Enum, true);
//    if (!EnumPtr)
//        return NSLOCTEXT("Invalid", "Invalid", "Invalid").ToString();
//
//#if WITH_EDITOR
//    return EnumPtr->GetDisplayNameText(EnumValue).ToString();
//#else
//    return EnumPtr->GetEnumName(EnumValue);
//#endif
//}

//FlightSystem::FlightSystem()
//{
//    CurrentFlightType = EFlightType::Station;
//	CurrentFlightMode = EFlightMode::ZeroG;
//	//CurrentEngineType = EEngineType::Impulse;
//	//CurrentEngineState = EEngineState::Idle;
//	CurrentFlightStatus = EFlightStatus::OK;
//}
