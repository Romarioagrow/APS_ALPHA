#pragma once

#include "CoreMinimal.h"
#include "GravityParamStruct.generated.h"

USTRUCT(BlueprintType)
struct FGravityParamStruct
{
    GENERATED_BODY()

    FGravityParamStruct()
        : LinearDampingZeroG(0.0f)
        , AngularDampingZeroG(100.0f)
        , LinearDampingStation(1.0f)
        , AngularDampingStation(1000.0f)
        , LinearDampingPlanet(1.0f)
        , AngularDampingPlanet(2000.0f)
        , LinearDampingShip(2.0f)
        , AngularDampingShip(3000.0f)
        , LinearDampingLowG(0.1)
        , AngularDampingLowG(100.0f)
    {}

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Linear Damping - ZeroG")
        float LinearDampingZeroG;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Angular Damping - ZeroG")
        float AngularDampingZeroG;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Linear Damping - Station")
        float LinearDampingStation;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Angular Damping - Station")
        float AngularDampingStation;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Linear Damping - Planet")
        float LinearDampingPlanet;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Angular Damping - Planet")
        float AngularDampingPlanet;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Linear Damping - Ship")
        float LinearDampingShip;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Angular Damping - Ship")
        float AngularDampingShip;
    
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Linear Damping - LowG")
        float LinearDampingLowG;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Angular Damping - LowG")
        float AngularDampingLowG;
};

