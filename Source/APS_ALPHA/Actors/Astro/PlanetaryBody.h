#pragma once

#include "CoreMinimal.h"
#include "OrbitalBody.h"
#include "APS_ALPHA/Actors/Planetary/PlanetaryEnvironment.h"
#include "Kismet/GameplayStatics.h"
#include "PlanetaryBody.generated.h"

class APlanetaryEnvironmentGenerator;
enum class EPlanetType : uint8;

UCLASS()
class APS_ALPHA_API APlanetaryBody : public AOrbitalBody, public IPlanetaryEnvironment
{
	GENERATED_BODY()

public:
	APlanetaryBody();

protected:
	virtual void BeginPlay() override;

public:
	virtual void Tick(float DeltaTime) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Planet Body")
	APlanetaryEnvironmentGenerator* PlanetaryEnvironmentGenerator;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Planet Body")
	bool bGenerateByDefault{false};

	APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Planet Body")
	double WSCZoneScale{10.0};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Planet Body")
	bool bEnvironmentSpawned{false};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Planet Body")
	double AtmosphereHeight{0.0};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Planet Body")
	double OrbitHeight{0.0};

	UPROPERTY(VisibleAnywhere, Category = "Planet")
	EPlanetType PlanetType;
};
