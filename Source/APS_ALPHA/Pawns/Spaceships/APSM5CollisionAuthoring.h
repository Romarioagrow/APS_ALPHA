#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "APSM5CollisionAuthoring.generated.h"

class UStaticMesh;

/** Explicit offline authoring operations; never attached to gameplay actors. */
UCLASS()
class APS_ALPHA_API UAPSM5CollisionAuthoring : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()
public:
    /** Stable geometry digest, shape name, collision mode, mass flag per line. */
    UFUNCTION(BlueprintCallable, Category="M5|Authoring")
    static FString GetConvexSnapshot(UStaticMesh* Mesh);

    /** Preserve every prior shape and filter; set only added shapes to PhysicsOnly. */
    UFUNCTION(BlueprintCallable, Category="M5|Authoring")
    static FString ConfigureAddedHullSkin(UStaticMesh* Mesh, const FString& PriorSnapshot, int32 ExpectedAdded);
};
