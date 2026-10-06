#pragma once

#include "CoreMinimal.h"

class AActor;
class AAstroGenerator;
class AStarCluster;
class FAPSSystemMaterializer;
class UWorld;
enum class ESpectralClass : uint8;
enum class EStellarType : uint8;

/**
 * The cluster's star systems as places the civilization can reach (Rio 02.10: "colonize the other systems: first a
 * scan, a probe, a visit in person or a unit, then a station or an outpost; beacons for the systems on the maps").
 * Every catalogue record of the generated star cluster is a system here: its stable id, a generated name, where it is
 * relative to the home star, its spectral class, how many planets it may hold and the room it has among its
 * neighbours. What the civilization knows and holds there is kept per system and saved with the civilization.
 *
 * Plain C++ owned by UAPSFleetCommandSubsystem, reached through APSStarSystemsFind(World) like the fleet command.
 */
namespace APSStars
{
	/** What the civilization knows of a system: its catalogue entry, a scan (probe or scanner), a survey (a visit). */
	enum class EKnowledge : uint8
	{
		Catalogued,
		Scanned,
		Surveyed
	};

	APS_ALPHA_API FText KnowledgeName(EKnowledge Knowledge);
	APS_ALPHA_API FLinearColor KnowledgeColour(EKnowledge Knowledge);
	/** Astronomical unit, cm. */
	inline constexpr double AstronomicalUnitCm = 1.495978707e13;
	/**
	 * A catalogue system's name (Rio 02.10: "the system names are unreadable"): generated from the cluster seed and
	 * the system's stable id, the same in the generation menu, on the maps and on the HUD.
	 */
	APS_ALPHA_API FString SystemName(int32 ClusterSeed, const FGuid& Id);

	/**
	 * Rio 03.10 ("every star must be reachable"): what a galaxy catalogue star stands for, from its class and the
	 * population's radius factor (FGalaxyCatalogStarRecord::RadiusScale): its stellar type and a spectral name ("K4V").
	 */
	APS_ALPHA_API EStellarType GalaxyStellarType(ESpectralClass SpectralClass, float RadiusScale);
	APS_ALPHA_API FString GalaxySpectralName(ESpectralClass SpectralClass, int32 Subclass, float RadiusScale);
}

/** One catalogue system: fixed data read from the generated cluster (not saved; rebuilt from the catalogue). */
struct APS_ALPHA_API FAPSStarSystemInfo
{
	FGuid Id;
	/** The cluster record (its index equals the catalogue instance index). */
	int32 Record{INDEX_NONE};
	/** Generated, stable for the world seed (the same name in the menu, on the maps and on the HUD). */
	FString Name;
	/** Spectral class and subclass of the primary, e.g. "K4V". */
	FString Spectral;
	int32 StarCount{1};
	int32 PotentialPlanets{0};
	/** Where the primary is in the world now (the home system's frame), as of reading it through Get or Find. */
	FVector Location{FVector::ZeroVector};
	/** Room among its neighbours: half the distance to the nearest other star, cm. */
	double RoomCm{0.0};
	/** Distance to the home star, cm. */
	double HomeDistanceCm{0.0};
	bool bHome{false};
	/** A cluster star inside the home system's own sphere: suppressed in a game, never listed or offered. */
	bool bInsideHome{false};
	/** The primary's colour, for markers. */
	FLinearColor Colour{FLinearColor::White};
	/**
	 * Rio 03.10 ("every star must be reachable"): a drawn star of the galaxy catalogue (its catalogue index), registered
	 * once the pilot comes near it and kept from then on; INDEX_NONE for the cluster's systems (Record).
	 */
	int64 GalaxyIndex{INDEX_NONE};
	/** The primary's radius as the sky draws it, cm (galaxy systems; 0 for the cluster's). */
	double StarRadiusCm{0.0};
};

/** What the civilization has in a system (saved). */
struct APS_ALPHA_API FAPSStarSystemState
{
	APSStars::EKnowledge Knowledge{APSStars::EKnowledge::Catalogued};
	/** A beacon, relay or administration of the civilization stands there. */
	bool bClaimed{false};
	/** Structures raised there, by infrastructure type id (APSInfrastructureCatalog). */
	TArray<FName> Structures;
	/** World seconds of the first scan or visit (0: none yet). */
	double FirstContactSeconds{0.0};
	/** The system's deep-space anomaly, if it has one: 0 unknown, 1 detected, 2 located, 3 investigated. */
	uint8 Anomaly{0};

	friend FArchive& operator<<(FArchive& Ar, FAPSStarSystemState& State);
};

struct APS_ALPHA_API FAPSStarSystemsSaveData
{
	TArray<FGuid> Ids;
	TArray<FAPSStarSystemState> States;
	/** Version 3, one per id: the galaxy catalogue index of a galaxy system (registered again on load), else INDEX_NONE. */
	TArray<int64> GalaxyIndices;

	friend FArchive& operator<<(FArchive& Ar, FAPSStarSystemsSaveData& Data);
};

class APS_ALPHA_API FAPSStarSystems
{
public:
	explicit FAPSStarSystems(UWorld* InWorld);
	~FAPSStarSystems();

	/** Reads the catalogue once the generated cluster exists, follows the pilot for visits, keeps anchors in place. */
	void Tick(float DeltaSeconds);

	/** True once the cluster catalogue has been read. */
	bool IsReady() const { return !Systems.IsEmpty(); }
	int32 Num() const { return Systems.Num(); }
	const FAPSStarSystemInfo* Get(int32 Index) const { return Systems.IsValidIndex(Index) ? &Current(Index) : nullptr; }
	const FAPSStarSystemInfo* Find(const FGuid& Id) const;
	int32 IndexOf(const FGuid& Id) const;
	const FAPSStarSystemInfo* GetHome() const { return Get(HomeIndex); }
	int32 GetHomeIndex() const { return HomeIndex; }

	/** The systems nearest a world location, nearest first (the home included). */
	void FindNearest(const FVector& Location, int32 Count, TArray<int32>& OutIndices) const;
	/** Systems whose name contains the text (case-insensitive), nearest to home first. */
	void Search(const FString& Text, int32 Limit, TArray<int32>& OutIndices) const;
	/** The system whose room holds a world location (a ship inside it), or INDEX_NONE. */
	int32 FindContaining(const FVector& Location) const;

	/**
	 * Rio 03.10 ("every star must be reachable; only the nearest ones, those we fly to"): the drawn galaxy catalogue stars
	 * nearest the pilot become systems here (aps.Stars.GalaxyReach), a few times a second, appended so every index stays.
	 * Registers one now (a load, a test); returns its index, or INDEX_NONE without the galaxy's nearest-star index, or
	 * for a star in the home system's sphere or in a cluster system's room (that system stands for it).
	 */
	int32 RegisterGalaxyStar(int64 CatalogIndex);
	/** The registered galaxy system of a catalogue star, or INDEX_NONE. */
	int32 IndexOfGalaxyStar(int64 CatalogIndex) const;
	int32 NumGalaxySystems() const { return GalaxySystems.Num(); }
	/** Every system the civilization knows or holds, nearest to home first. */
	void GetKnown(TArray<int32>& OutIndices) const;

	FAPSStarSystemState GetState(const FGuid& Id) const;
	APSStars::EKnowledge GetKnowledge(const FGuid& Id) const;
	bool IsClaimed(const FGuid& Id) const;
	/** Raises what the civilization knows (never lowers it); posts to the journal when it changes. */
	void Learn(const FGuid& Id, APSStars::EKnowledge Knowledge, const FText& How);
	/** A structure finished there; a claiming one makes the system the civilization's. */
	void AddStructure(const FGuid& Id, FName StructureId, bool bClaims);

	/**
	 * Deep-space anomalies (Rio 02.10: "more than five kinds, chains of activity"): about one system in eight hides one,
	 * the same one for the world. A scan detects it, a survey of the system locates it, then a visit in person or an
	 * expedition investigates it. INDEX_NONE when the system has none.
	 */
	int32 AnomalyKindOf(const FGuid& Id) const;
	static FText AnomalyName(int32 Kind);
	/** Investigates a located anomaly (the pilot there, or an expedition): its reward and story; false if not located. */
	bool InvestigateAnomaly(const FGuid& Id, const FText& By);

	/**
	 * The relay network: claimed systems within reach of each other (the largest relay reach at either end), as index
	 * pairs, for the maps and routes. The home system is always on it.
	 */
	void GetNetwork(TArray<TPair<int32, int32>>& OutLinks) const;
	/** Within the network's reach: a claimed system with a relay that reaches it (or the home's own reach). */
	bool IsInReach(const FGuid& Id) const;
	/** Relay reach of a claimed system from what stands there, cm (the home system has a base reach). */
	double ReachCm(int32 Index) const;

	/**
	 * A world actor standing for the system, so fleet orders, the autopilot and HUD contacts can aim at it; spawned on
	 * demand at the primary's location and reused. Tagged APS.StarSystem and APS.StarSystem.<id>.
	 */
	AActor* GetAnchor(const FGuid& Id);
	/** The system an anchor stands for. */
	static bool AnchorSystem(const AActor* Actor, FGuid& OutId);

	/** Changes on every knowledge, claim or structure change, so screens rebuild only then. */
	uint32 GetRevision() const { return Revision; }

	void CaptureSave(FAPSStarSystemsSaveData& OutData) const;
	/** Applied now, or once the catalogue is read. */
	void RestoreSave(FAPSStarSystemsSaveData&& Data);

	/** Console: aps.Stars.List [N], aps.Stars.Learn <name> <level>. */
	void LogNearest(int32 Count) const;

	/** D.8: the cluster system the pilot comes to, made real (APSSystemMaterializer.h). */
	FAPSSystemMaterializer* GetMaterializer() const { return Materializer.Get(); }
	/** The generator and cluster the catalogue was read from, and the cluster's seed. */
	AAstroGenerator* GetGenerator() const;
	AStarCluster* GetCluster() const;
	int32 GetClusterSeed() const { return ClusterSeed; }

private:
	bool ReadCatalogue();
	void ApplyPendingRestore();
	void UpdateGalaxyNeighbours(float DeltaSeconds);
	/** RegisterGalaxyStar, timed for the galaxy-systems log line. */
	int32 RegisterGalaxyStarTimed(int64 CatalogIndex);
	/** The cluster system (the grid's) whose room holds a location, nearest first; INDEX_NONE if none. */
	int32 FindContainingCluster(const FVector& Location, double* OutDistanceSquared = nullptr) const;
	void UpdateVisit(float DeltaSeconds);
	void FollowHome();
	/** A system's world location now: the home's place plus its fixed offset. */
	FVector LocationOf(int32 Index) const { return HomeLocation + FromHome[Index]; }
	/**
	 * Rio 05.10 afternoon (flight FPS: at drive speed the world shifts every frame, and rewriting all ~36k locations
	 * took 0.45 ms a frame): a system's Location is brought to the home's place when it is read, not on every shift.
	 */
	const FAPSStarSystemInfo& Current(int32 Index) const;
	FIntVector CellOf(const FVector& FromHome) const;
	FAPSStarSystemState& EditState(const FGuid& Id);

	TWeakObjectPtr<UWorld> World;
	/** The home system actor every location is measured from. */
	TWeakObjectPtr<AActor> Home;
	FVector HomeLocation{FVector::ZeroVector};
	TArray<FAPSStarSystemInfo> Systems;
	/** Each system's offset from the home system (as the stellar view draws it), cm. */
	TArray<FVector> FromHome;
	TMap<FGuid, int32> IndexById;
	int32 HomeIndex{INDEX_NONE};
	int32 ClusterSeed{0};
	TMap<FGuid, FAPSStarSystemState> States;
	TMap<FGuid, TWeakObjectPtr<AActor>> Anchors;
	/** Uniform grid over the offsets for the containing and neighbour queries. */
	double CellCm{0.0};
	TMap<FIntVector, TArray<int32>> Grid;
	TOptional<FAPSStarSystemsSaveData> PendingRestore;
	/** The galaxy systems after the cluster's, by catalogue index, and their states a load brought that wait for them. */
	TMap<int64, int32> IndexByGalaxy;
	TArray<int32> GalaxySystems;
	/** Systems [0, ClusterSystemCount) are the cluster catalogue's, read once. */
	int32 ClusterSystemCount{0};
	TMap<FGuid, TPair<int64, FAPSStarSystemState>> HeldGalaxyStates;
	float GalaxyClock{0.0f};
	double GalaxyLogSeconds{0.0};
	/**
	 * Rio 05.10 night (a 50-107 ms hitch every half second at drive speed): the galaxy stars the last neighbour scan found
	 * unregistered, nearest first, registered a few a frame (aps.Stars.GalaxyAddsPerFrame, aps.Stars.GalaxyAddBudgetMs).
	 */
	TArray<int64> PendingGalaxyStars;
	/** Systems registered since the revision last changed: it changes at the half-second scan, as before. */
	int32 GalaxyAddsSinceRevision{0};
	/** For the log line, since the last one: registrations tried, their time, the slowest one and its parts, the worst frame. */
	int32 GalaxyTriesLogged{0};
	int32 GalaxyAddsLogged{0};
	double GalaxyAddSeconds{0.0};
	double GalaxyAddWorstSeconds{0.0};
	double GalaxyAddWorstNearSeconds{0.0};
	double GalaxyAddWorstRingSeconds{0.0};
	double GalaxyAddWorstFrameSeconds{0.0};
	/** RegisterGalaxyStar's last call: its nearest-star query and its search of the cluster grid, seconds. */
	double LastRegisterNearSeconds{0.0};
	double LastRegisterRingSeconds{0.0};
	/** The system the pilot is in and for how long (a visit counts after a few seconds). */
	int32 VisitIndex{INDEX_NONE};
	float VisitSeconds{0.0f};
	float VisitClock{0.0f};
	float AnchorClock{0.0f};
	float CatalogueRetry{0.0f};
	uint32 Revision{1};
	TWeakObjectPtr<AAstroGenerator> CatalogueGenerator;
	TWeakObjectPtr<AStarCluster> CatalogueCluster;
	TUniquePtr<FAPSSystemMaterializer> Materializer;
};

/** The world's star systems (registered by UAPSFleetCommandSubsystem), or null outside a game world. */
APS_ALPHA_API FAPSStarSystems* APSStarSystemsFind(const UWorld* World);
APS_ALPHA_API void APSStarSystemsRegister(const UWorld* World, FAPSStarSystems* Systems);
