#include "IMovementStrategy.h"
#include "StarSystem.h" 
// ImpulseMovementStrategy.cpp
void ImpulseMovementStrategy::ThrustForward(float Value)
{
    if (FMath::Abs(Value) < KINDA_SMALL_NUMBER) return;
    GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Green, FString::Printf(TEXT("ImpulseMovementStrategy::ThrustForward")));
    GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Green, FString::Printf(TEXT("SpaceshipHull, %s"), *SpaceshipHull->GetName()));

    /*const FVector Direction = ForwardVector->GetForwardVector();
    const FVector Impulse = Direction * Value * OnboardComputer->GetEngineThrustForce();
    SpaceshipHull->AddImpulse(Impulse, NAME_None, true);*/

    const FVector Direction = SpaceshipHull->GetForwardVector();
    const FVector Impulse = Direction * Value * 100000000; // Установите вашу силу тяги
    SpaceshipHull->AddImpulse(Impulse, NAME_None, true);
}

void ImpulseMovementStrategy::ThrustSide(float Value)
{
    if (FMath::Abs(Value) < KINDA_SMALL_NUMBER) return;

    const FVector Direction = SpaceshipHull->GetRightVector();
    const FVector Impulse = Direction * Value * 10000; // Установите вашу силу тяги
    SpaceshipHull->AddImpulse(Impulse, NAME_None, true);
}

void ImpulseMovementStrategy::ThrustVertical(float Value)
{
    if (FMath::Abs(Value) < KINDA_SMALL_NUMBER) return;

    const FVector Direction = SpaceshipHull->GetUpVector();
    const FVector Impulse = Direction * Value * 10000; // Установите вашу силу тяги
    SpaceshipHull->AddImpulse(Impulse, NAME_None, true);
}


// OffsetMovementStrategy.cpp
void OffsetMovementStrategy::ThrustForward(float Value)
{
    if (FMath::Abs(Value) < KINDA_SMALL_NUMBER) return;

    const FVector Direction = SpaceshipHull->GetForwardVector();
    const FVector NewLocation = SpaceshipHull->GetComponentLocation() + Direction * Value * 10000; // Установите вашу скорость смещения
    SpaceshipHull->SetWorldLocation(NewLocation, false);
}

void OffsetMovementStrategy::ThrustSide(float Value)
{
    if (FMath::Abs(Value) < KINDA_SMALL_NUMBER) return;

    const FVector Direction = SpaceshipHull->GetRightVector();
    const FVector NewLocation = SpaceshipHull->GetComponentLocation() + Direction * Value * 10000; // Установите вашу скорость смещения
    SpaceshipHull->SetWorldLocation(NewLocation, false);
}

void OffsetMovementStrategy::ThrustVertical(float Value)
{
    if (FMath::Abs(Value) < KINDA_SMALL_NUMBER) return;

    const FVector Direction = SpaceshipHull->GetUpVector();
    const FVector NewLocation = SpaceshipHull->GetComponentLocation() + Direction * Value * 10000; // Установите вашу скорость смещения
    SpaceshipHull->SetWorldLocation(NewLocation, false);
}
void OffsetWrapMovementStrategy::ThrustForward(float Value)
{
    if (FMath::Abs(Value) < KINDA_SMALL_NUMBER) return;

    // Получаем вектор вперед корабля
    const FVector Direction = SpaceshipHull->GetForwardVector();

    // Получаем ссылку на StarSystem
    //AStarSystem* StarSystem = /* получить ссылку на StarSystem в вашем коде */

        // Сдвигаем StarSystem
    if (StarSystem)
    {
        StarSystem->AddActorWorldOffset(-Direction * Value * 1000000);

    }
    else
    {
        GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Red, FString::Printf(TEXT("NO StarSystem")));

    }
}

void OffsetWrapMovementStrategy::ThrustSide(float Value)
{
    if (FMath::Abs(Value) < KINDA_SMALL_NUMBER) return;

    // Получаем вектор в сторону корабля
    const FVector Direction = SpaceshipHull->GetRightVector();

    // Получаем ссылку на StarSystem
   // AStarSystem* StarSystem = /* получить ссылку на StarSystem в вашем коде */
    if (StarSystem)
    {
        // Сдвигаем StarSystem
        StarSystem->AddActorLocalOffset(-Direction * Value);
    }
    else
    {
        GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Red, FString::Printf(TEXT("NO StarSystem")));

    }
}

void OffsetWrapMovementStrategy::ThrustVertical(float Value)
{
    if (FMath::Abs(Value) < KINDA_SMALL_NUMBER) return;

    // Получаем вектор вверх корабля
    const FVector Direction = SpaceshipHull->GetUpVector();

    // Получаем ссылку на StarSystem
   // AStarSystem* StarSystem = /* получить ссылку на StarSystem в вашем коде */
    if (StarSystem)
    {
        // Сдвигаем StarSystem
        StarSystem->AddActorLocalOffset(-Direction * Value);
    }
    else
    {
        GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Red, FString::Printf(TEXT("NO StarSystem")));

    } 
}