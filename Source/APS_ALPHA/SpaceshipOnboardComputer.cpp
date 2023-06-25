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
    if (!AffectedActor) return;

    // Проверить тип актора и установить соответствующий режим полета и тип полета
    if (AffectedActor->IsA(ACelestialBody::StaticClass()))
    {
        ACelestialBody* CelestialBody = Cast<ACelestialBody>(AffectedActor);

        // Изменение параметров полета в зависимости от типа тела
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
        // Добавьте больше условий, если необходимо
    }
    else if (AffectedActor->IsA(ATechActor::StaticClass()))
    {
        // Здесь вы можете настроить параметры полета для технических объектов
        FlightSystem.CurrentFlightMode = EFlightMode::Station;
        FlightSystem.CurrentFlightType = EFlightType::ArtificialGravity;
    }

    FFlightParams* Params = FlightModeParams.Find(FlightSystem.CurrentFlightMode);

    // Проверка, были ли найдены параметры
    if (Params && !IsBoosting)
    {
        // Установка параметров
        FlightSystem.FlightParams.ThrustForce = Params->ThrustForce;
        FlightSystem.FlightParams.LinearResistance = Params->LinearResistance;
        FlightSystem.FlightParams.AngularResistance = Params->AngularResistance;

    }

    //// Настройка типа диапазона полета, например, в зависимости от расстояния до объекта
    //float Distance = FVector::Dist(Actor->GetActorLocation(), ShipLocation); // Допустим, ShipLocation - это положение корабля
    //if (Distance < 1000) // Примерное расстояние, замените на реальные значения
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

double USpaceshipOnboardComputer::GetEngineThrustForce()
{
    return FlightSystem.FlightParams.ThrustForce;
}

void USpaceshipOnboardComputer::IncreaseFlightMode()
{
    // Проверяем, что текущий FlightMode не является последним в енаме.
    if (static_cast<int32>(FlightSystem.CurrentFlightMode) < static_cast<int32>(EFlightMode::Intergalaxy))
    {
        FlightSystem.CurrentFlightSafeMode = EFlightSafeMode::Unsafe;

        // Увеличиваем FlightMode на 1.
        FlightSystem.CurrentFlightMode = static_cast<EFlightMode>(static_cast<int32>(FlightSystem.CurrentFlightMode) + 1);

        // Обновляем параметры полета для нового FlightMode.
        if (FlightModeParams.Contains(FlightSystem.CurrentFlightMode))
        {
            FlightSystem.FlightParams = FlightModeParams[FlightSystem.CurrentFlightMode];

            /// TODO: Switch to unsafe flight mode
        }
    }
}

void USpaceshipOnboardComputer::DecreaseFlightMode()
{
    // Проверяем, что текущий FlightMode не является первым в енаме.
    if (static_cast<int32>(FlightSystem.CurrentFlightMode) > static_cast<int32>(EFlightMode::Station))
    {
        // Уменьшаем FlightMode на 1.
        FlightSystem.CurrentFlightMode = static_cast<EFlightMode>(static_cast<int32>(FlightSystem.CurrentFlightMode) - 1);

        // Обновляем параметры полета для нового FlightMode.
        if (FlightModeParams.Contains(FlightSystem.CurrentFlightMode))
        {
            FlightSystem.FlightParams = FlightModeParams[FlightSystem.CurrentFlightMode];
        }
    }
}

void USpaceshipOnboardComputer::AccelerateBoost(float DeltaTime)
{
    // Увеличиваем силу тяги на определенный процент. Здесь я использую 10% в качестве примера.
    FlightSystem.FlightParams.ThrustForce += FlightSystem.FlightParams.ThrustForce * 0.1 * DeltaTime;
}

void USpaceshipOnboardComputer::DecelerateBoost(float DeltaTime)
{
    // Уменьшаем силу тяги на определенный процент. Здесь я использую 10% в качестве примера.
    FlightSystem.FlightParams.ThrustForce -= FlightSystem.FlightParams.ThrustForce * 0.1 * DeltaTime;
}