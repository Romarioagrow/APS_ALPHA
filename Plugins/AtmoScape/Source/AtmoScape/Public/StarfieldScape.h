#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "StarfieldScape.generated.h"

UCLASS(BlueprintType)
class ATMOSCAPE_API AStarfieldScape : public AActor
{
    GENERATED_BODY()

public:
    // Sets default values for this actor's properties
    AStarfieldScape();

public:
    // Called every frame
    virtual void OnConstruction(const FTransform& Transform) override;

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Settings|Starfield")
        float SunIntensity;

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Settings|Starfield")
        float SunAngularDiameter;

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Settings|Starfield")
        float StarFieldIntensity;

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Settings|Starfield")
        float StarfieldDecrease;

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Settings|Starfield")
        float StarfieldTileScale;

private:
    USceneComponent* Root;
    UMaterialInterface* Material;
    UMaterialInstanceDynamic* StarfieldInstance;
    UStaticMeshComponent* StarfieldMesh;
};
