#pragma once

#include "CoreMinimal.h"
#include "APS_ALPHA/Core/World/APSPlanetSurfacePlacementResolver.h"
#include "Subsystems/WorldSubsystem.h"
#include "APSCivilizationRuntimeManifest.h"
#include "APSCivilizationMaterializationSubsystem.generated.h"

class AAstroGenerator;
class APlanetaryBody;
class ASpaceship;

DECLARE_MULTICAST_DELEGATE_ThreeParams(
	FOnAPSCivilizationMaterializationStateChanged,
	const FAPSCivilizationRuntimeManifest&,
	EAPSCivilizationMaterializationState,
	EAPSCivilizationMaterializationState);

/**
 * Idempotently projects the committed menu civilization onto the Surface-owned
 * validated home-planet frame. Economy/NPC simulation remains a later hook.
 */
UCLASS()
class APS_ALPHA_API UAPSCivilizationMaterializationSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;
	virtual bool IsTickable() const override;
	virtual bool DoesSupportWorldType(EWorldType::Type WorldType) const override;

	UFUNCTION(BlueprintPure, Category="Civilization|Materialization")
	const FAPSCivilizationRuntimeManifest& GetRuntimeManifest() const { return RuntimeManifest; }

	UFUNCTION(BlueprintPure, Category="Civilization|Materialization")
	bool IsMaterializationComplete() const { return bMaterializationComplete; }

	FOnAPSCivilizationMaterializationStateChanged& OnMaterializationStateChanged()
	{
		return MaterializationStateChanged;
	}

	/** Restores persisted IDs and planet-relative transforms before actor upsert. */
	bool RestoreRuntimeManifest(const FAPSCivilizationRuntimeManifest& SavedManifest);

private:
	friend class FAPSCivilizationMaterializationAcceptanceTest;

	bool TryInitializeManifest(AAstroGenerator*& OutGenerator, APlanetaryBody*& OutHomeBody);
	void TransitionMaterializationState(EAPSCivilizationMaterializationState NewState);
	bool ValidateMaterializedStarterSet(APlanetaryBody* HomeBody,
		const FAPSCivilizationFootprintResult& Placement,
		FString& OutFailureReason) const;

	bool TryResolveSafeSite(APlanetaryBody* HomeBody,
		FAPSCivilizationFootprintResult& OutResult);
	bool TryMaterializeEntities(AAstroGenerator* Generator, APlanetaryBody* HomeBody,
		const FAPSCivilizationFootprintResult& Placement);
	AActor* FindMaterializedActor(const FAPSCivilizationManifestEntity& Entity) const;
	ASpaceship* FindSelectedStarterShip(const FAPSCivilizationManifestEntity& Entity) const;
	void BindIdentity(AActor* Actor, const FAPSCivilizationManifestEntity& Entity) const;
	void PersistRelativeTransform(AActor* Actor, APlanetaryBody* HomeBody,
		FAPSCivilizationManifestEntity& Entity);
	static void PlaceBoundsOnSupportPlane(AActor* Actor, const FTransform& SupportTransform,
		double ExtraClearanceCm);

	UPROPERTY(VisibleAnywhere, Category="Civilization|Materialization")
	FAPSCivilizationRuntimeManifest RuntimeManifest;

	UPROPERTY(VisibleAnywhere, Category="Civilization|Materialization")
	FAPSCivilizationFootprintResult LastPlacementResult;

	UPROPERTY(Transient)
	TObjectPtr<AActor> MaterializedBase;

	UPROPERTY(Transient)
	TObjectPtr<AActor> MaterializedPad;

	UPROPERTY(Transient)
	TObjectPtr<ASpaceship> MaterializedShip;

	TWeakObjectPtr<APlanetaryBody> MaterializedHomeBody;
	FOnAPSCivilizationMaterializationStateChanged MaterializationStateChanged;
	float RetryAccumulator{0.0f};
	bool bManifestInitialized{false};
	bool bManifestRestoredFromSave{false};
	bool bMaterializationComplete{false};
};
