#pragma once

#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "CoreMinimal.h"
#include "CelestialSystem.h"
#include "Galaxy.generated.h"

enum class EGalaxyClass : uint8;
enum class EGalaxyType : uint8;

UCLASS()
class APS_ALPHA_API AGalaxy : public ACelestialSystem
{
	GENERATED_BODY()

public:
	AGalaxy();

	UPROPERTY(VisibleAnywhere, Category = "Galaxy")
	EGalaxyType GalaxyType;

	UPROPERTY(VisibleAnywhere, Category = "Galaxy")
	EGalaxyClass GalaxyGlass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Galaxy Stars")
	UHierarchicalInstancedStaticMeshComponent* StarMeshInstances;
};
