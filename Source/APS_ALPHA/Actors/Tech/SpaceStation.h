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

protected:
	virtual void BeginPlay() override;
	void ConfigureGravityVolume(bool bWriteDiagnosticLog);
	virtual float GetGravityVolumeRadiusMultiplier() const { return 1.0f; }

public:

	// TODO: To parent component
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Components")
	USceneComponent* SpawnPoint;

	/**
	 * Where an orbital start places the pilot, when authored (non-zero). Otherwise stations
	 * use SpawnPoint; shipyards keep SpawnPoint for the ships they launch.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Components")
	USceneComponent* PlayerStartPoint;

	UFUNCTION(BlueprintPure, Category = "Player Start")
	virtual FVector GetPlayerStartLocation() const;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gravity")
	USphereComponent* GravityCollisionZone;
};
