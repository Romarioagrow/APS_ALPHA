#pragma once

#include "CoreMinimal.h"
#include "APS_ALPHA/Actors/Tech/TechActor.h"
#include "APSCivilizationStarterActors.generated.h"

class UAPSCivilizationIdentityComponent;
class USceneComponent;
class UStaticMeshComponent;

/** Explicit native fallback used until an authored modular base class is present in the catalog. */
UCLASS(BlueprintType)
class APS_ALPHA_API AAPSCivilizationBaseModule : public ATechActor
{
	GENERATED_BODY()

public:
	AAPSCivilizationBaseModule();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Civilization")
	TObjectPtr<UAPSCivilizationIdentityComponent> CivilizationIdentity;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Civilization|Placeholder")
	TObjectPtr<USceneComponent> ModuleRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Civilization|Placeholder")
	TObjectPtr<UStaticMeshComponent> Foundation;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Civilization|Placeholder")
	TObjectPtr<UStaticMeshComponent> Habitat;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Civilization|Placeholder")
	TObjectPtr<UStaticMeshComponent> Airlock;
};

/** Walkable native landing-pad fallback with an explicit surface-route spawn point. */
UCLASS(BlueprintType)
class APS_ALPHA_API AAPSCivilizationLandingPad : public ATechActor
{
	GENERATED_BODY()

public:
	AAPSCivilizationLandingPad();

	UFUNCTION(BlueprintPure, Category="Civilization|Landing Pad")
	FTransform GetCharacterSpawnTransform() const;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Civilization")
	TObjectPtr<UAPSCivilizationIdentityComponent> CivilizationIdentity;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Civilization|Placeholder")
	TObjectPtr<USceneComponent> PadRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Civilization|Placeholder")
	TObjectPtr<UStaticMeshComponent> Deck;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Civilization|Placeholder")
	TObjectPtr<UStaticMeshComponent> AccessRamp;

	/** Clear point on the base-facing edge of the pad. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Civilization|Landing Pad")
	TObjectPtr<USceneComponent> CharacterSpawnPoint;
};
