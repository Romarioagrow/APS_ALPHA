// Fill out your copyright notice in the Description page of Project Settings.


#include "SpaceshipOnboardComputer.h"
#include "UObject/UnrealType.h"
#include "UObject/Class.h"
#include "UObject/EnumProperty.h"
#include "CelestialBody.h"
#include "Planet.h"
#include "Star.h"



USpaceshipOnboardComputer::USpaceshipOnboardComputer()
{
}

void USpaceshipOnboardComputer::ComputeFlightStatus(AWorldActor* AffectedActor)
{
    if (!AffectedActor) // �������� �� ����������
        return;

    // ��������� ��� ������ � ���������� ��������������� ����� ������ � ��� ������
    if (AffectedActor->IsA(ACelestialBody::StaticClass()))
    {
        ACelestialBody* CelestialBody = Cast<ACelestialBody>(AffectedActor);

        // ��������� ���������� ������ � ����������� �� ���� ����
        if (CelestialBody->IsA(APlanet::StaticClass()))
        {
            FlightSystem.CurrentFlightMode = EFlightMode::Planetary;
            FlightSystem.CurrentFlightType = EFlightType::ZeroG;

            // check proximity!
        }
        else if (CelestialBody->IsA(AStar::StaticClass()))
        {
            FlightSystem.CurrentFlightMode = EFlightMode::Interplanetray;
            FlightSystem.CurrentFlightType = EFlightType::LightSpeed;
        }
        // �������� ������ �������, ���� ����������
    }
    else if (AffectedActor->IsA(ATechActor::StaticClass()))
    {
        // ����� �� ������ ��������� ��������� ������ ��� ����������� ��������
        FlightSystem.CurrentFlightMode = EFlightMode::Station;
        FlightSystem.CurrentFlightType = EFlightType::ArtificialGravity;
    }

    //// ��������� ���� ��������� ������, ��������, � ����������� �� ���������� �� �������
    //float Distance = FVector::Dist(Actor->GetActorLocation(), ShipLocation); // ��������, ShipLocation - ��� ��������� �������
    //if (Distance < 1000) // ��������� ����������, �������� �� �������� ��������
    //{
    //    FlightRangeType = EFlightRangeType::Nearby;
    //}
    //else if (Distance < 10000)
    //{
    //    FlightRangeType = EFlightRangeType::Medium;
    //}
    //else
    //{
    //    FlightRangeType = EFlightRangeType::Distant;
    //}

}

FString USpaceshipOnboardComputer::GetEnumValueAsString(const TCHAR* EnumName, int32 EnumValue)
{
    const UEnum* EnumPtr = FindObject<UEnum>(ANY_PACKAGE, EnumName, true);
    if (!EnumPtr)
    {
        return FString("Invalid");
    }

    FString EnumValueName = EnumPtr->GetNameByValue((int64)EnumValue).ToString();
    int32 DoubleColonIndex = EnumValueName.Find(TEXT("::"));

    if (DoubleColonIndex != INDEX_NONE)
    {
        EnumValueName = EnumValueName.Mid(DoubleColonIndex + 2);
    }

    return EnumValueName;
}

FString USpaceshipOnboardComputer::GetFlightTypeAsString()
{
    return UEnum::GetValueAsString(FlightSystem.CurrentFlightType);
}

FString USpaceshipOnboardComputer::GetFlightModeAsString()
{
    return UEnum::GetValueAsString(FlightSystem.CurrentFlightMode);
}

FString USpaceshipOnboardComputer::GetFlightStatusAsString()
{
    return UEnum::GetValueAsString(FlightSystem.CurrentFlightStatus);
}

FString USpaceshipOnboardComputer::GetEngineStateAsString()
{
    return UEnum::GetValueAsString(EngineSystem.CurrentEngineState);
}

FString USpaceshipOnboardComputer::GetEngineTypeAsString()
{
    return UEnum::GetValueAsString(EngineSystem.CurrentEngineMode);
}