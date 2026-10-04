#pragma once

#include "CoreMinimal.h"
#include "TechActor.h"
#include "Colony.generated.h"

class USceneComponent;
class UStaticMeshComponent;

UCLASS()
class APS_ALPHA_API AColony : public ATechActor
{
	GENERATED_BODY()

public:
	AColony(); virtual void BeginPlay() override; // B8: dressed in the colony modules' style.

	/** Rio 03.10: the home colony's main building, BP_ColonyHQ on its 80 x 55 x 1 m foundation (pivot at the
	 * foundation's underside, +X toward the landing pad), instead of the dome, towers and disc. False if it is
	 * switched off (aps.Colony.HQ 0) or its Blueprint is missing; the compact look then stays. */
	bool UseHeadquartersLook();
	/** Back to the compact dome base (a rough site the headquarters would sink into). */
	void UseCompactLook();
	bool HasHeadquartersLook() const { return HeadquartersBuilding.IsValid(); }
	/** Where a new surface start puts the pilot (Rio 03.10): on the headquarters hall's floor, X the facing, Z up. */
	bool GetHeadquartersArrival(FTransform& OutSpot) const;

private:
	UPROPERTY(VisibleAnywhere, Category = "Colony|Visual")
	TObjectPtr<USceneComponent> ColonyRoot;

	UPROPERTY(VisibleAnywhere, Category = "Colony|Visual")
	TObjectPtr<UStaticMeshComponent> Foundation;

	UPROPERTY(VisibleAnywhere, Category = "Colony|Visual")
	TObjectPtr<UStaticMeshComponent> HabitatDome;

	UPROPERTY(VisibleAnywhere, Category = "Colony|Visual")
	TArray<TObjectPtr<UStaticMeshComponent>> Towers;

	/** The headquarters (owned as an instance component) and what the compact look hid for it. */
	TWeakObjectPtr<class UChildActorComponent> HeadquartersBuilding;
	TArray<TPair<TWeakObjectPtr<class UPrimitiveComponent>, uint8>> HiddenCompactParts;
	TArray<TWeakObjectPtr<class ULightComponent>> HiddenCompactLights;
};
