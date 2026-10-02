#pragma once

#include "CoreMinimal.h"
#include "APS_ALPHA/Gameplay/Production/APSProductionTypes.h"
#include "APS_ALPHA/Gameplay/Spawn/APSSpawnPlacementSubsystem.h"
#include "Subsystems/WorldSubsystem.h"
#include "APSColonyConstructionSubsystem.generated.h"

class AAPSColonyModule;
class ACameraActor;
class ACustomGravityCharacter;
class APawn;
class APlanetaryBody;
class UAPSProductionSubsystem;

/**
 * Colony construction (U1/U4, Rio 29.09: installable objects at the base, the base by default the headquarters). The
 * two build sites are the surface colony base and the headquarters in orbit. An order goes through the APS-81
 * production system (a Building job on the site's actor-gated context, pilot as subject); when a job finishes, the
 * module is placed by the S5 spawn service beside the site and the job resolves as materialized, so production
 * publishes APS.Building.Build like any other build. Built modules are not saved yet: saves are a shared zone.
 */
UCLASS()
class APS_ALPHA_API UAPSColonyConstructionSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual bool DoesSupportWorldType(EWorldType::Type WorldType) const override;
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;
	virtual bool IsTickable() const override { return !IsTemplate(); }
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	/** A site takes orders once its anchor exists and its production context is registered. */
	bool IsSiteReady(EAPSSpawnSite Site) const;
	/** The surface colony's base and pad stand on the ground with their plinths, stilts and ramp. */
	bool IsColonyGrounded() const { return bColonyGrounded; }
	AActor* GetSiteAnchor(EAPSSpawnSite Site) const;

	/** Orders one module at its site; the instigator is the pilot's pawn. False with a player-facing reason. */
	bool RequestBuild(FName ModuleId, AActor* Instigator, FText& OutFailure);
	bool RequestBuild(FName ModuleId, FText& OutFailure);

	/** The site's live production queue. */
	bool GetSiteSnapshot(EAPSSpawnSite Site, FAPSProductionSnapshot& OutSnapshot) const;
	/** Modules standing at a site, oldest first. */
	void GetBuiltModules(EAPSSpawnSite Site, TArray<AAPSColonyModule*>& OutModules) const;
	/**
	 * Loads (APSCivilizationSave): one saved module back at its site, placed relative to the site's anchor as it was.
	 * Null while the site is not ready (the colony materializes after the load), so the caller tries again later.
	 */
	AAPSColonyModule* RestoreModule(FName ModuleId, const FGuid& StableId, EAPSSpawnSite Site,
		const FTransform& RelativeToAnchor, double FoundationDepthCm, double BoomLengthCm);
	/** Why a finished job still waits for its place (surface not loaded); empty when it does not wait. */
	FText GetJobNote(const FGuid& JobId) const;
	/** A failed job's code (APS.Spawn.TooSteep, ...) in words. */
	static FText DescribeFailure(FName FailureCode);
	/** The founding pilot's stable identity: the subject of construction orders and of the onboarding quest. */
	static FGuid MakePilotStableId(const FGuid& CivilizationId);

	/** Registers a site's production context on its anchor; the tick does this for the live colony and headquarters. */
	bool RegisterSite(EAPSSpawnSite Site, AActor* Anchor, APlanetaryBody* Body, const FGuid& InCivilizationId);
	/** Places every finished job that has its site ready. Deterministic seam for the tick and the tests. */
	void MaterializeFinishedJobs();

	/** aps.Colony.ModuleShots finished photographing the modules (the terminal then captures its tabs). */
	FSimpleMulticastDelegate& OnTestShotsFinished() { return TestShotsFinished; }

private:
	struct FSite
	{
		TWeakObjectPtr<AActor> Anchor;
		TWeakObjectPtr<APlanetaryBody> Body;
		FGuid ContextId;
		bool bRegistered{false};
	};

	UAPSProductionSubsystem* GetProduction() const;
	void EnsureDefinitions(UAPSProductionSubsystem& Production);
	void UpdateSites();
	/**
	 * S4, 30.09: the base and the landing pad stand at the highest point of their footprint, so on a slope their low
	 * side hangs metres above the ground. Once the surface is loaded, a concrete plinth fills that gap under each.
	 */
	void GroundColonyStructures();
	/** True when the job is finished with (built or refused); false to retry later. */
	bool MaterializeJob(EAPSSpawnSite Kind, FSite& Site, UAPSProductionSubsystem& Production,
		const FAPSProductionJobSnapshot& Job);
	APawn* GetPlayerPawn() const;
	/** C19: orders the new game's founding package once the base stands on its ground. */
	void OrderStartPackage();
	void TickTestAutomation();
	void TickModuleShots();

	FSite Sites[2];
	FGuid CivilizationId;
	FGuid PilotStableId;
	TMap<FGuid, FText> JobNotes;
	TMap<FGuid, double> JobRetrySeconds;
	TArray<TWeakObjectPtr<AAPSColonyModule>> BuiltModules;
	float PollAccumulator{0.0f};
	bool bDefinitionsRegistered{false};
	bool bColonyGrounded{false};
	/** C19: the founding package of a new game (USpawnParameters::ColonyStartPackage); empty for a loaded game. */
	TArray<FName> StartPackage;
	bool bStartPackageOrdered{false};

	/** aps.Colony.AutoBuild / aps.Colony.ModuleShots test runs. */
	TSet<FName> AutoBuildIssued;
	bool bAutoOnboardingDone{false};
	int32 AutoBuildOrdered{0};
	double AutoBuildStartSeconds{-1.0};
	/** Photo steps: two per module (aim, then shoot); -1 when idle. */
	int32 ShotIndex{-1};
	double ShotStepSeconds{0.0};
	bool bShotsDone{false};
	TArray<TWeakObjectPtr<AAPSColonyModule>> ShotModules;
	TWeakObjectPtr<ACameraActor> ShotCamera;
	TWeakObjectPtr<ACustomGravityCharacter> ShotHeldPilot;
	FSimpleMulticastDelegate TestShotsFinished;
};
