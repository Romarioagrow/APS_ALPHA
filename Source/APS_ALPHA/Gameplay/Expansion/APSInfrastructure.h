#pragma once

#include "CoreMinimal.h"
#include "APSInfrastructureCatalog.h"

class AActor;
class APlanetaryBody;
class FAPSMegastructureYard;
class FAPSStarSystems;
class UWorld;

/** A structure the civilization raised from the infrastructure catalogue. */
struct APS_ALPHA_API FAPSBuiltStructure
{
	FName Type;
	/** Where: a planet or moon (its fleet key) or a star system (its id). */
	FString SiteKey;
	FGuid SystemId;
	TWeakObjectPtr<AActor> Actor;
	FString ActorName;
	/** Relative to the site (body or system anchor), so a load puts it back. */
	FTransform RelativeTransform{FTransform::Identity};
	double BuiltSeconds{0.0};
	/** Raised by hand in build mode: stands where it was put (never settled again), also after a load. */
	bool bPlaced{false};
	/**
	 * Rio 06.10 (audit: structures in galaxy-reach systems were dropped for good on load): restored from a save while its
	 * site (a star system registered a moment later, a materialized body) was not standing yet. The record is kept and
	 * written to the next save; Actor stays null until the site appears and the visual is raised (aps.Save.HoldUnresolvedStructures).
	 * Rio 07.10: also set when its world went with a released star system (aps.Stars.HoldReleasedStructures).
	 */
	bool bAwaitingSite{false};
	/**
	 * Rio 07.10 (a held record was raised by the first body of its name, a home world's included): the star system whose
	 * world it stands on, set when that system was released; such a record is raised only on that system's own bodies.
	 * Runtime only, never saved (a load holds it by its body's name, as before).
	 */
	FGuid HostSystemId;
};

/** A small object placed by hand in build mode (Gameplay/Construction), kept here so the save holds it. */
struct APS_ALPHA_API FAPSPlacedProp
{
	/** APSConstruction catalogue id; the mesh path stands in when a later catalogue no longer has it. */
	FName PropId;
	FString MeshPath;
	/** The body it stands on (FAPSFleetCommand::KeyOf). */
	FString SiteKey;
	/** Relative to the body, so a load puts it back. */
	FTransform RelativeTransform{FTransform::Identity};
	FVector Scale{FVector::OneVector};
	/** Stood on the ground (not on another placed prop): a load settles it on the terrain again. */
	bool bOnGround{true};
	TWeakObjectPtr<AActor> Actor;
	/** Restored from a save and not yet checked against the terrain near the pilot. */
	bool bPendingSettle{false};
	/**
	 * Rio 07.10 (placed props on the worlds of other star systems were gone with the system, and a load never brought back
	 * one whose world was not standing): its world is not standing; raised there when it stands
	 * (aps.Stars.HoldReleasedStructures). HostSystemId as for FAPSBuiltStructure; both runtime only, never saved.
	 */
	bool bAwaitingSite{false};
	FGuid HostSystemId;
};

struct APS_ALPHA_API FAPSInfrastructureSaveData
{
	struct FStructure
	{
		FName Type;
		FString SiteKey;
		FGuid SystemId;
		FString ActorName;
		FTransform RelativeTransform{FTransform::Identity};
		double BuiltSeconds{0.0};
	};
	TArray<FStructure> Structures;
	/** Stocks, in EResource order. */
	TArray<float> Stocks;

	/** Version 2 (build mode, Rio 02.10): the props placed by hand. */
	struct FProp
	{
		FName PropId;
		FString MeshPath;
		FString SiteKey;
		FTransform RelativeTransform{FTransform::Identity};
		FVector Scale{FVector::OneVector};
		bool bOnGround{true};
	};
	TArray<FProp> Props;
	/** Version 2: the actor names of the structures raised by hand (FAPSBuiltStructure::bPlaced). */
	TArray<FString> PlacedStructures;

	friend FArchive& operator<<(FArchive& Ar, FAPSInfrastructureSaveData& Data);
};

/**
 * The civilization's infrastructure and economy (Rio 02.10: "it is only a list now, and what to do with it is not
 * clear"): what stands where, what each place may still take, the stocks every structure feeds per minute and the
 * costs of building, and the rule changes the structures make (survey, build and ship speed, scan and relay reach).
 * Plain C++ owned by UAPSFleetCommandSubsystem; reached through APSInfrastructureFind(World).
 */
class APS_ALPHA_API FAPSInfrastructure
{
public:
	explicit FAPSInfrastructure(UWorld* InWorld);
	~FAPSInfrastructure();

	void Tick(float DeltaSeconds);

	/**
	 * Why this type cannot be built at this place now (empty when it can): the placement (a planet or moon for orbit
	 * and surface, a star system anchor or star for the rest), what the civilization knows of the place, the structure
	 * it needs there first, the department level, the limit per place, the stocks for the cost.
	 */
	FText CheckBuild(FName Type, const AActor* Site) const;
	/** Every type that suits this place, buildable or not (the refusal says why). */
	void GetOptions(const AActor* Site, TArray<TPair<FName, FText>>& OutTypesAndRefusals) const;
	/** Takes the cost when the construction ships set out (refunded if the order is cancelled before the work ends). */
	bool Reserve(FName Type);
	void Refund(FName Type);
	/** The construction ships finished: spawns the structure at the site (beside the builder when given) and records it. */
	AActor* Complete(FName Type, AActor* Site, const FVector* NearLocation = nullptr);
	/**
	 * Build mode (Rio 02.10): Complete, then the structure stands exactly at WorldTransform (where the player put it, as
	 * turned) instead of the runtime's own spot, is never settled again and keeps that place through a load. The cost is
	 * the caller's (Reserve first, Refund when this returns null).
	 */
	AActor* CompleteAt(FName Type, AActor* Site, const FTransform& WorldTransform);
	/** Build mode: a prop the player placed (the actor is APSConstruction::SpawnProp's) is kept for the save. */
	void AddPlacedProp(AActor* Actor, AActor* Site, FName PropId, const FString& MeshPath, bool bOnGround);
	/** Build mode: forgets a placed prop (the caller destroys the actor). False when it was not one. */
	bool RemovePlacedProp(const AActor* Actor);
	const TArray<FAPSPlacedProp>& GetPlacedProps() const { return PlacedProps; }
	/** A department's level for the catalogue's needs: the founding level, work growth and mission rewards. */
	static int32 DepartmentLevel(const UWorld* World, APSInfrastructure::EDepartment Department);
	/** The star system a site stands for (a system anchor or a star), or false for a planet or moon. */
	static bool SiteSystem(const UWorld* World, const AActor* Site, FGuid& OutSystemId);

	/**
	 * Rio 07.10 ("buildings on planets of other star systems disappear when that system goes away"): aps.Stars.
	 * HoldReleasedStructures, read by the fleet too. 0: every path below returns at once and nothing is held.
	 */
	static bool HoldsReleased();
	/**
	 * The materializer releases a foreign star system (FAPSSystemMaterializer::Finish, before its actors go): every
	 * structure and placed prop standing on these bodies becomes a held record of that system (still saved, still
	 * producing, not counted at its place), raised again on the same body when the system stands.
	 */
	void HoldOnRelease(const TArray<APlanetaryBody*>& Bodies, const FGuid& HostSystemId);

	const TArray<FAPSBuiltStructure>& GetStructures() const { return Structures; }
	/** Structures standing at a place (a body or a star system anchor). */
	void GetAt(const AActor* Site, TArray<const FAPSBuiltStructure*>& OutStructures) const;
	int32 CountAt(const AActor* Site, FName Type) const;
	const FAPSBuiltStructure* FindByActor(const AActor* Actor) const;
	/** The actor of the first structure of a type standing at a place, or null. */
	AActor* FindActorAt(const AActor* Site, FName Type) const;

	/**
	 * Chains (Rio 03.10): an orbital station of the civilization over this world: a catalogue station or hub there, or a
	 * station, shipyard or headquarters of the fleet's (the home complex counts).
	 */
	bool HasStationAt(const AActor* Site) const;
	/**
	 * Structures of a type in the star system of a place (a planet or moon belongs to the home system). Rio 07.10: at a
	 * foreign system's star or anchor also those on that system's own worlds, standing or held (aps.Stars.ForeignStarSites).
	 */
	int32 CountInSystem(const AActor* Site, FName Type) const;
	/** Hubs: the extra berths their place gives an orbital station type there (0 for every other type). */
	int32 BerthsAt(const AActor* Site, const APSInfrastructure::FType& Type) const;
	/** Hubs and megastructures under construction and their scaffolds (Gameplay/Megastructures). */
	FAPSMegastructureYard* GetMegastructureYard() const { return Yard.Get(); }

	float GetStock(APSInfrastructure::EResource Resource) const;
	/** Per minute, from everything standing. */
	float GetRate(APSInfrastructure::EResource Resource) const;
	bool CanAfford(const TArray<APSInfrastructure::FAmount>& Cost) const;
	void AddStock(APSInfrastructure::EResource Resource, float Value);

	/**
	 * Rio 07-09.10, ORIGIN ladder (T-08, concept section 5): the stocks open one by one. A closed stock is not earned and
	 * not shown as income: its home yield is nothing, a structure that costs or yields it is refused unless it is the one
	 * that opens it (the mining outpost opens metals, the gas harvester volatiles, the research station research, a claim
	 * influence), and that one pays without the closed part. Energy is open from the first minute. Every stock is open
	 * outside the ladder (SANDBOX, old saves).
	 */
	bool IsResourceOpen(APSInfrastructure::EResource Resource) const;
	/** The RES_* token a type opens when it stands (APSProgressionTokens); NAME_None when it opens none. */
	static FName OpensResourceToken(const APSInfrastructure::FType& Type);
	/** The cost actually taken: the catalogue's, without a closed stock the type opens; nothing with unlimited resources. */
	void EffectiveCost(const APSInfrastructure::FType& Type, TArray<APSInfrastructure::FAmount>& OutCost) const;
	/** Takes a cost at once (a hull laid down in the ladder); false and nothing taken when it cannot be afforded. */
	bool Spend(const TArray<APSInfrastructure::FAmount>& Cost);
	/** Rates again: the colony's modules were built or a stock opened (colony construction, the Ancients, the tokens). */
	void RefreshRates();

	/** Summed rule changes (+0.1 = 10% faster). */
	float SurveySpeedBonus() const;
	float BuildSpeedBonus() const;
	float ShipSpeedBonus() const;
	/** Work at this place is faster by this share (stations, docks, labs there). */
	float LocalWorkBonus(const AActor* Site) const;
	/** The largest relay reach standing in a star system (cm); 0 when none. */
	double RelayReachCm(const FGuid& SystemId) const;

	uint32 GetRevision() const { return Revision; }
	void CaptureSave(FAPSInfrastructureSaveData& OutData) const;
	void RestoreSave(FAPSInfrastructureSaveData&& Data);

private:
	void RecountRates();
	void ApplyPendingRestore();
	/**
	 * The actor a saved structure stands at: for a star system the home star (the home system) or the system's anchor, for
	 * a planet or moon the body whose fleet key is SiteKey (BodiesByKey, when given, instead of a walk over the bodies).
	 */
	AActor* ResolveSite(UWorld* LiveWorld, FAPSStarSystems* Stars, const FGuid& SystemId, const FString& SiteKey,
		const TMap<FString, AActor*>* BodiesByKey = nullptr) const;
	/**
	 * Rio 06.10: the structures held at load (bAwaitingSite) whose site now stands are raised (twice a second). Rio 07.10:
	 * also the structures and props held when their system was released, on that system's own bodies once it stands
	 * again (a few a time), and the props a load could not place.
	 */
	void RaiseHeldStructures();
	FString SiteKeyOf(const AActor* Site, FGuid& OutSystemId) const;
	AActor* SpawnVisual(const APSInfrastructure::FType& Type, AActor* Site, const FTransform& Transform,
		const FString& ActorName, const FText& Name);
	/** Ground settlements and surface structures stand at sea level until the pilot is near; then on the ground. */
	void SettleSurfaceActors();
	/**
	 * Props from a save: spawned again on their bodies; those whose body is not found stay as records for the next save
	 * (Rio 07.10: and with aps.Stars.HoldReleasedStructures are raised once a body of that name stands).
	 */
	void RestorePlacedProps(const TArray<FAPSInfrastructureSaveData::FProp>& Saved);
	/**
	 * Props restored on the ground settle on the terrain once the pilot has stood near them for a moment (its collision
	 * exists then): only along the body's up, keeping their turn, and only when the ground moved (a changed generator).
	 */
	void SettlePlacedProps(float DeltaSeconds);

	TWeakObjectPtr<UWorld> World;
	TArray<FAPSBuiltStructure> Structures;
	TArray<FAPSPlacedProp> PlacedProps;
	/** How long the pilot has stood on the ground (props settle after a moment of it). */
	float PilotGroundedSeconds{0.0f};
	float Stocks[static_cast<int32>(APSInfrastructure::EResource::Count)]{};
	float Rates[static_cast<int32>(APSInfrastructure::EResource::Count)]{};
	/** Rio 07-09.10 (T-08): the stocks the ladder has not opened (bits in EResource order), set in RecountRates. */
	uint8 ClosedMask{0};
	/** The world's resources knob says unlimited: nothing is ever taken. */
	bool bUnlimited{false};
	float RulesClock{0.0f};
	TOptional<FAPSInfrastructureSaveData> PendingRestore;
	float RestoreWait{0.0f};
	float SettleClock{0.0f};
	int32 Serial{0};
	uint32 Revision{1};
	/** Rio 03.10: the scaffolds of the hubs and megastructures the construction ships are raising. */
	TUniquePtr<FAPSMegastructureYard> Yard;
};

APS_ALPHA_API FAPSInfrastructure* APSInfrastructureFind(const UWorld* World);
APS_ALPHA_API void APSInfrastructureRegister(const UWorld* World, FAPSInfrastructure* Infrastructure);
