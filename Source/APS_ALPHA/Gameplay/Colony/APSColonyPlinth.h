#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "APSColonyPlinth.generated.h"

class UStaticMeshComponent;

/**
 * A concrete plinth under a colony structure that stands above sloped ground (S4, 30.09): the base and the landing pad
 * are set at the highest point of their footprint, so on a slope their low side hangs metres above the terrain. The
 * plinth fills that gap down to the lowest ground. A separate child actor: the structure's own bounds, placement and
 * validation stay untouched.
 */
UCLASS()
class APS_ALPHA_API AAPSColonyPlinth : public AActor
{
	GENERATED_BODY()

public:
	AAPSColonyPlinth();

	/** Before FinishSpawning: a box or a cylinder. The spawn transform's scale sizes it (engine shapes are 100 cm and
	 * centred on the pivot). */
	void Configure(bool bRound);

private:
	UPROPERTY(VisibleAnywhere, Category="Colony")
	TObjectPtr<UStaticMeshComponent> Body;
};
