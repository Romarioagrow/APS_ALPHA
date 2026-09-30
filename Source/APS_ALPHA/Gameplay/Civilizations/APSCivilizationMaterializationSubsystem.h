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
	void ResetPlacementSearch();
	void TransitionMaterializationState(EAPSCivilizationMaterializationState NewState);
	bool ValidateMaterializedStarterSet(APlanetaryBody* HomeBody,
		const FAPSCivilizationFootprintResult& Placement,
		FString& OutFailureReason) const;

	bool TryResolveSafeSite(APlanetaryBody* HomeBody,
		FAPSCivilizationFootprintResult& OutResult);
	/**
	 * A new surface start places the colony just ahead of the landed pilot instead of at the seed site, which could be
	 * thousands of kilometres away (Rio, 30.09). False while the pilot is still coming down; zero OutPreferredUp keeps
	 * the seed site (orbital starts, saves, a pilot who never lands).
	 */
	bool ResolvePilotSite(APlanetaryBody* HomeBody, FVector& OutPreferredUp);
	/**
	 * After a new surface start materializes, the pilot walks up to the colony: a stand point beside the base and the
	 * landed ship, facing them and the sun (Rio, 30.09: the start showed a lone pilot). The pilot waits in the surface
	 * handoff until the terrain collision under the stand point exists, like the generator's own landing.
	 */
	void BeginPilotArrival(APlanetaryBody* HomeBody, const FVector& BaseLocation, const FVector& PadLocation);
	void TickPilotArrival();
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
	TWeakObjectPtr<APlanetaryBody> PlacementHomeBody;
	TWeakObjectPtr<ASpaceship> PlacementEnvelopeShip;
	FVector PlacementEnvelopeScale{FVector::ZeroVector};
	double PlacementShipEnvelopeDiameterCm{0.0};
	FAPSCivilizationFootprintSearch PlacementSearch;
	/** Surface start: the colony site in home-body space, once the pilot has landed (ResolvePilotSite). */
	FVector PilotSiteLocal{FVector::ZeroVector};
	FVector PilotSettleLocation{FVector::ZeroVector};
	double PilotSettleStartSeconds{-1.0};
	double PilotWaitStartSeconds{-1.0};
	double LastPlacementLogSeconds{-100.0};
	bool bPilotSiteResolved{false};
	/** The strict footprint found no site: search once more with a smaller, steeper one. */
	bool bRelaxedFootprint{false};
	/** Pilot arrival at the colony (BeginPilotArrival): target, facing, where the pilot came from, deadline. */
	bool bPilotArrivalPending{false};
	/** One arrival per session: it starts as soon as the site is known, since WorldScape builds collision around the pilot. */
	bool bPilotArrivalBegun{false};
	/**
	 * The starter ship landed on the colony pad (a surface start, or a saved parking). In orbit it stays docked at the
	 * headquarters, and a piloted ship stays under its pilot: in service, not parked (Rio 29.09/30.09: every start works).
	 */
	bool bShipParkedAtColony{false};
	FVector PilotArrivalLocation{FVector::ZeroVector};
	FVector PilotArrivalView{FVector::ZeroVector};
	FVector PilotArrivalReturn{FVector::ZeroVector};
	double PilotArrivalDeadlineSeconds{0.0};
	FOnAPSCivilizationMaterializationStateChanged MaterializationStateChanged;
	float RetryAccumulator{0.0f};
	bool bManifestInitialized{false};
	bool bManifestRestoredFromSave{false};
	bool bMaterializationComplete{false};
};
