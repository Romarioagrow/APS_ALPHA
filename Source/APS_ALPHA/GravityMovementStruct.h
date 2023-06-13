#pragma once

#include "CoreMinimal.h"
#include "GravityMovementStruct.generated.h"

USTRUCT(BlueprintType)
struct FGravityMovementStruct
{
	GENERATED_BODY()


    FGravityMovementStruct()
        //: MovementsForceSpeedZeroG(25000000000.0)
        : MovementsForceSpeedZeroG(25.0)
        , MovementsForceSpeedStation(1500.0)
        , MovementsForceSpeedPlanet(2000.0)
        , MovementsForceSpeedShip(1500.0)
        , MovementsForceSpeedLowG(250.0)
        , JumpForceSpeedZeroG(25.0)
        , JumpForceSpeedStation(100.0)
        , JumpForceSpeedPlanet(2100.0)
        , JumpForceSpeedShip(35.0)
        , JumpForceSpeedLowG(50.0)
    {}

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movements Force Speed - ZeroG")
        double MovementsForceSpeedZeroG;
    
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movements Force Speed - Station")
        double MovementsForceSpeedStation;
    
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movements Force Speed - Planet")
        double MovementsForceSpeedPlanet;
    
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movements Force Speed - Ship")
        double MovementsForceSpeedShip;
    
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movements Force Speed - LowG")
        double MovementsForceSpeedLowG;
    
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Jump Force Speed - ZeroG")
        double JumpForceSpeedZeroG;
    
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Jump Force Speed - Station")
        double JumpForceSpeedStation;
    
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Jump Force Speed - Planet")
        double JumpForceSpeedPlanet;
    
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Jump Force Speed - Ship")
        double JumpForceSpeedShip;
    
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Jump Force Speed - LowG")
        double JumpForceSpeedLowG;

   
};

