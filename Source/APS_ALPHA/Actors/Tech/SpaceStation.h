#pragma once

#include <Components/SphereComponent.h>
#include "CoreMinimal.h"
#include "TechActor.h"
#include "APS_ALPHA/Core/Interfaces/NavigatableBody.h"
#include "APS_ALPHA/Gameplay/Gravity/GravitySource.h"
#include "SpaceStation.generated.h"

UCLASS()
class APS_ALPHA_API ASpaceStation : public ATechActor, public IGravitySource, public INavigatableBody
{
	GENERATED_BODY()

public:
	ASpaceStation();

	// TODO: To parent component
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Components")
	USceneComponent* SpawnPoint;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gravity")
	USphereComponent* GravityCollisionZone;
};
