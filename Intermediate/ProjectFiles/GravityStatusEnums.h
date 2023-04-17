#pragma once

#include "CoreMinimal.h"
#include "GravityStatusEnums.generated.h"

UENUM(BlueprintType)
enum class EGravityType : uint8
{
    ZeroG,
    OnStation,
    OnPlanet,
    OnShip
}; 
