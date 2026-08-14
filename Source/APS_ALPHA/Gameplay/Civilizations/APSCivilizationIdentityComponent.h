#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "APSCivilizationRuntimeManifest.h"
#include "APSCivilizationIdentityComponent.generated.h"

/** Stable identity and ownership attached to every materialized civilization actor. */
UCLASS(ClassGroup=(APS), BlueprintType, meta=(BlueprintSpawnableComponent))
class APS_ALPHA_API UAPSCivilizationIdentityComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UAPSCivilizationIdentityComponent();

	void InitializeFromManifest(
		const FAPSCivilizationRuntimeManifest& Manifest,
		const FAPSCivilizationManifestEntity& Entity);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, SaveGame, Category="Civilization")
	int32 ManifestSchemaVersion{0};

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, SaveGame, Category="Civilization")
	FGuid StableEntityId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, SaveGame, Category="Civilization")
	FGuid ParentStableId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, SaveGame, Category="Civilization")
	FGuid OwnerCivilizationId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, SaveGame, Category="Civilization")
	FGuid FactionId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, SaveGame, Category="Civilization")
	EAPSCivilizationEntityRole Role{EAPSCivilizationEntityRole::BaseModule};

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, SaveGame, Category="Civilization")
	FString HomeBodyKey;
};
